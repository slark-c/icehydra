#include "shm_manager.h"

#include <sys/mman.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iso646.h>
#include <fcntl.h>
#include <stdlib.h>


#define MAX_SHELL_CMD  512
#define strLen(str)        ((!str)?0:strlen(str))

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

	if(stat(path,&stat_buffer) == 0 && S_ISDIR(stat_buffer.st_mode))
		return true;
	else
		return false;
}

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

int make_dir(const char *path)
{
#define STRING_LEN(a) ({sizeof(a)-1;})
#define memzero(buf, n)       (void) memset(buf, 0, n)
#define strsame(x,y)   (memcmp(x,y,STRING_LEN(y)) == 0)

	if(isDirExists(path))
		return 0;
	
	char cmd[MAX_SHELL_CMD] = {0};
	sprintf(cmd,"mkdir -p %s ",path);
    int ret = icehydra_system(cmd,NULL);
	if(strsame(path,"/run/shm/huge")){
		memzero(cmd, MAX_SHELL_CMD);
		ret = icehydra_system("mount none /run/shm/huge -t hugetlbfs",NULL);
	}
#undef STRING_LEN
#undef memzero
#undef strsame
	return ret;
}

static void *shm_mmap_memfd(icehydra_shm_t *shm)
{
	shm->mem_ptr = mmap(NULL,shm->size,PROT_READ|PROT_WRITE,MAP_SHARED,shm->fd,0);
	if(shm->mem_ptr == MAP_FAILED)
	{
		shm->mem_ptr = NULL;
		return NULL;
	}
	
	return shm->mem_ptr;
}

static void* shm_create_memfd(icehydra_shm_t *shm)
{
	int fd = memfd_create(shm->filename,0);
	if(fd < 0 )
	{
		fprintf(stderr,"%s line = %d , %s \n",__FILE__,__LINE__,strerror(errno));
		return NULL;
	}
	
	int ret = ftruncate(fd,shm->size);
	if(ret < 0)
	{
		close(fd);
		return NULL;
	}
	
	shm->fd = fd;
	return shm_mmap_memfd(shm);
}

static int  shm_munmap_memfd(icehydra_shm_t *shm)
{
	return munmap(shm->mem_ptr,shm->size);
}

static void *shm_mmap_posix(icehydra_shm_t *shm)
{
	int fd = shm_open(shm->filename,O_RDWR,0666);
	if(fd < 0 ){
		fprintf(stderr,"%s line = %d , %s \n",__FILE__,__LINE__,strerror(errno));
		return NULL;
	}

	shm->fd = fd;
	int ret = ftruncate(fd,shm->size);
	if(ret < 0){
		close(fd);
		return NULL;
	}
	
	shm->mem_ptr = mmap(NULL,shm->size,PROT_READ|PROT_WRITE,MAP_SHARED,shm->fd,0);
	if(shm->mem_ptr == MAP_FAILED){
		close(fd);
		shm->mem_ptr = NULL;
		return NULL;
	}
	
	return shm->mem_ptr;
}

static void* shm_create_posix(icehydra_shm_t *shm)
{
	int fd = shm_open(shm->filename,O_CREAT|O_RDWR,0666);
	if(fd < 0 ){
		fprintf(stderr,"%s line = %d , %s \n",__FILE__,__LINE__,strerror(errno));
		return NULL;
	}
	
	int ret = ftruncate(fd,shm->size);
	if(ret < 0){
		close(fd);
		return NULL;
	}
	
	shm->fd = fd;
	shm->mem_ptr = mmap(NULL,shm->size,PROT_READ|PROT_WRITE,MAP_SHARED,shm->fd,0);
	if(shm->mem_ptr == MAP_FAILED){
		shm->mem_ptr = NULL;
		return NULL;
	}
	return shm->mem_ptr;
}

static int  shm_munmap_posix(icehydra_shm_t *shm)
{
	close(shm->fd);
	return munmap(shm->mem_ptr,shm->size);
}

static void *shm_create_sysv(icehydra_shm_t *shm)
{
	make_dir(shm->filename);
	
	int proj_id = shm->proj_id > 0 ? shm->proj_id : 1;
	key_t key  = ftok(shm->filename,proj_id);
	int id = shmget(key,shm->size,IPC_CREAT|0666);	
	shm->mem_ptr = shmat(id,NULL,0);
	if(shm->mem_ptr == (void*)-1 )
	{
		fprintf(stderr,"%s line = %d , %s \n",__FILE__,__LINE__,strerror(errno));
		shm->mem_ptr = NULL;
		return NULL;
	}
	
	memset(shm->mem_ptr,0,shm->size);	
	shm->id = id;
	return shm->mem_ptr;
}

static void *shm_mmap_sysv(icehydra_shm_t *shm)
{	
	int id = shm->id;
	if(id <= 0)
	{
		int proj_id = shm->proj_id > 0 ? shm->proj_id : 1;
		key_t key  = ftok(shm->filename,proj_id);
		id = shmget(key,0,0666);		
		shm->id = id;
	}
	
	shm->mem_ptr = shmat(id,NULL,0);
	if(shm->mem_ptr == (void*)-1 )
	{
		fprintf(stderr,"%s line = %d , %s \n",__FILE__,__LINE__,strerror(errno));
		shm->mem_ptr = NULL;
		return NULL;
	}

	struct shmid_ds ds;
	shmctl(id,IPC_STAT,&ds);
	shm->size = ds.shm_segsz;
	return shm->mem_ptr;
}

static int  shm_munmap_sysv(icehydra_shm_t *shm)
{
	return shmdt(shm->mem_ptr);
}
static void* shm_create_hugepagestlbfs(icehydra_shm_t *shm)
{
	int fd = open(shm->filename, O_CREAT | O_RDWR,0666);
	if(fd < 0 ){
		fprintf(stderr,"%s line = %d , %s \n",__FILE__,__LINE__,strerror(errno));
		return NULL;
	}

	shm->fd = fd;
	shm->mem_ptr = mmap(NULL,shm->size,PROT_READ|PROT_WRITE,MAP_SHARED,shm->fd,0);
	if(shm->mem_ptr == MAP_FAILED){
		close(fd);
        unlink(shm->filename);
		shm->mem_ptr = NULL;
		return NULL;
	}
	return shm->mem_ptr;	
}
static void *shm_mmap_hupagestlbfs(icehydra_shm_t *shm)
{
	int fd = open(shm->filename,O_RDWR,0666);
	if(fd < 0 ){
		fprintf(stderr,"%s line = %d , %s \n",__FILE__,__LINE__,strerror(errno));
		return NULL;
	}

	shm->fd = fd;
	shm->mem_ptr = mmap(NULL,shm->size,PROT_READ|PROT_WRITE,MAP_SHARED,shm->fd,0);
	if(shm->mem_ptr == MAP_FAILED){
		close(fd);
		shm->mem_ptr = NULL;
		return NULL;
	}
	
	return NULL;
}
static int shm_munmap_hugepagestlbfs(icehydra_shm_t *shm)
{
	close(shm->fd);
	return munmap(shm->mem_ptr,shm->size);
}
void *shm_create(icehydra_shm_t* shm)
{
	if(shm->type == SHM_SYSV)
		return shm_create_sysv(shm);
	else if(shm->type == SHM_POSIX)
		return shm_create_posix(shm);
	else if(shm->type == SHM_MEMFD)
		return shm_create_memfd(shm);
	else if(shm->type == SHM_HUGEPAGESTLBFS)
		return shm_create_hugepagestlbfs(shm);
	
	return NULL;
}

void *shm_mmap(icehydra_shm_t* shm)
{
	if(shm->type == SHM_SYSV)
		return shm_mmap_sysv(shm);
	else if(shm->type == SHM_POSIX)
		return shm_mmap_posix(shm);
	else if(shm->type == SHM_MEMFD)
		return shm_mmap_memfd(shm);
	else if(shm->type == SHM_HUGEPAGESTLBFS)
		return shm_mmap_hupagestlbfs(shm);
	return NULL;
}

int  shm_munmap(icehydra_shm_t* shm)
{
	if(shm->type == SHM_SYSV)
		return shm_munmap_sysv(shm);
	else if(shm->type == SHM_POSIX)
		return shm_munmap_posix(shm);
	else if(shm->type == SHM_MEMFD)
		return shm_munmap_memfd(shm);
	else if(shm->type == SHM_HUGEPAGESTLBFS)
		return shm_munmap_hugepagestlbfs(shm);
	
	return -1;
}


