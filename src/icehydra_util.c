#define _GNU_SOURCE
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <iso646.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "ih_types.h"
#include "icehydra_util.h"

int  bind_cpu(int cpuid)
{	
	cpu_set_t mask;
	CPU_ZERO(&mask);
    CPU_SET(cpuid, &mask); 
	return sched_setaffinity(0, sizeof(mask), &mask);
}

int file_read_lock(int fd)
{
    struct flock fl;
	fl.l_type = F_RDLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;

	return (fcntl(fd, F_SETLK, &fl));
}

int file_write_lock(int fd)
{
    struct flock fl;
	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;

	return (fcntl(fd, F_SETLK, &fl));
}

int file_unlock(int fd)
{
	struct flock fl;
	fl.l_type = F_UNLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;
	
	return (fcntl(fd, F_SETLK, &fl));
}

int get_file_size(char* filename) 
{ 
  struct stat sstat; 
  stat(filename, &sstat); 
  int size=sstat.st_size; 

  return size; 
}

int icehydra_system(const char *cmd,char *resp)
{
	FILE *p = popen(cmd, "r");
    if (p == NULL) return -1;

	if(resp == NULL)
		pclose(p);
	else
		while(fgets(resp,MAX_SHELL_CMD,p) != NULL);

	return 0;
}
bool isDirExists(const char *path)
{
	struct stat stat_buffer;

	if(stat(path,&stat_buffer) == 0 and S_ISDIR(stat_buffer.st_mode))
		return true;
	else
		return false;
}

int make_dir(const char *path)
{
	if(isDirExists(path))
		return 0;
	
	char cmd[MAX_SHELL_CMD] = {0};
	sprintf(cmd,"mkdir -p %s ",path);
   
    return icehydra_system(cmd,NULL);
}

/*int mutexattr_init(pthread_mutex_t *mutex,pthread_mutexattr_t *mutexattr)
{	
	pthread_mutexattr_init(mutexattr);

	pthread_mutexattr_setpshared(mutexattr, PTHREAD_PROCESS_SHARED);

	pthread_mutex_init(mutex, mutexattr);

	return 0;
}*/

char *name_fix(char *prefix,char *name,char *suffix)
{
	int len = strLen(prefix)+strLen(name)+strLen(suffix);
	char *fixed = calloc(1,len);
	if(!fixed)
		return NULL;
	
	if(prefix == NULL and suffix != NULL)
		sprintf(fixed,"%s%s",name,suffix);
	else if(suffix == NULL and prefix != NULL)
		sprintf(fixed,"%s%s",prefix,name);
	else if(suffix and prefix)
		sprintf(fixed,"%s%s%s",prefix,name,suffix);
		
	return fixed;
}

int read_file(char* filename , char **data)
{
	int readlen;
	int size = get_file_size(filename);
	
	*data = (char *)malloc(size+10);
	if(*data == NULL)
	{
		return -1;
	}
	memset(*data,0,size+10);
	
	FILE *fp = fopen(filename,"rb");
	if (fp == NULL){
		free(*data);
		return -1;
	}

	readlen = fread(*data,size,1,fp);
	if(readlen != 1)
	{
		fclose(fp);
		free(*data);
		return -1;
	}

	fclose(fp);
	return readlen;

}

int tcp_send_noblock(int sockfd,void *data,int len)
{
	int totallen = 0,sendlen = 0;
	int times = 2000;
	
	if(data ==NULL || len <=0 || sockfd < 0)
		return 0;

	for (totallen = 0; totallen < len; )
	{
		sendlen = send(sockfd,data + totallen, len-totallen,MSG_DONTWAIT);
		if(sendlen < 0 )
		{
			if(!(times--))
				break;
			
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR){
				usleep(2000);
				continue;
			}
			
			break;
		}
		else if(sendlen == 0)
			break;
		else
			totallen += sendlen;
	}

	if(totallen != len)
		return -1;

	return 0;
}

int tcp_recv_noblock(int sockfd,void *data,int len)
{	
	int totallen = 0,readlen = 0;
	int times = 2000;
	
	if(data ==NULL || len <=0 || sockfd < 0)
		return 0;

	for (totallen = 0; totallen < len; )
	{
		readlen = recv(sockfd,data + totallen, len-totallen,MSG_DONTWAIT);
		if(readlen < 0)
		{
			if(!(times--))
				break;
			
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR){
				usleep(2000);
				continue;
			}
			printf("%s \n",strerror(errno));
			break;
		}
		else if(readlen == 0)
			break;
		else
			totallen += readlen;	
	}

	if(totallen != len)
		return -1;

	return 0;
}

