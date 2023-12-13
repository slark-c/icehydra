#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <pthread.h>

#include "icehydra_util.h"
#include "icehydra_cmd.h"
#include "conf.h"

int creat_bind_unix_tcp(const char *name)
{
	int fd,size;

	struct sockaddr_un un;

	if(strlen(name) > sizeof(un.sun_path))
		return -1;

	unlink(name);
	
	memset(&un,0,sizeof(un));
	un.sun_family = AF_UNIX;	
	strcpy(un.sun_path,name);
	size = offsetof(struct sockaddr_un , sun_path) + strlen(un.sun_path);

	if((fd=socket(AF_UNIX,SOCK_STREAM,0)) < 0){
		printf("%s %d, %s \n",__FILE__,__LINE__,strerror(errno));
		return -1;
	}
	
	if(bind(fd , (struct sockaddr *)&un,size) < 0){		
		printf("%s %d, %s \n",__FILE__,__LINE__,strerror(errno));
		return -1;
	}

	
	chmod(un.sun_path,0666);
	
	//if(listen(fd , 64) < 0)
	//	return -1;

	return fd;
}

int listen_unix_tcp(int sockfd , int backlog)
{
	return listen(sockfd,backlog);
}

int accept_unix_tcp(int sockfd,char *cli_name)
{
	int connfd ;
	char name[128] = {0};
	struct sockaddr_un un;
	struct stat statbuf;
	socklen_t  len  = sizeof(un);
	
	connfd = accept(sockfd,(struct sockaddr *)&un,&len);
	if(connfd > 0){
		//printf("len = %d %s \n",len,un.sun_path);
	}
	else if(connfd < 0)
		printf("%s line = %d , %s \n",__FILE__,__LINE__,strerror(errno));
	
	len -= offsetof(struct sockaddr_un , sun_path);
	memset(name,0,sizeof(name));
	memcpy(name,un.sun_path,len);
	name[len] = 0;

	if(stat(name , &statbuf) < 0){
		printf("%s line = %d , %s name %s\n",__FILE__,__LINE__,strerror(errno),name);
		return -1;
	}
	
	
	if(S_ISSOCK(statbuf.st_mode) == 0){
		printf("%s line = %d , %s \n",__FILE__,__LINE__,strerror(errno));
		return -1;
	}

	strcpy(cli_name,name);
	
	return connfd;
}

/*int multi_client_conn_thread(void *args,void *(*start_routine) (void *))
{
	pthread_t thread_id;
	
	if(pthread_create(&thread_id,NULL,start_routine,args) < 0){		
		printf("%s line = %d , %s \n",__FILE__,__LINE__,strerror(errno));
		return -1;
	}

	pthread_detach(thread_id);

	return 0;
}*/

int connect_unix_tcp(int sockfd , const char *server_name)
{
	struct sockaddr_un un;
	socklen_t len ;

	if(sockfd < 0)
		return -1;
	
	memset(&un,0,sizeof(un));
	un.sun_family = AF_UNIX;
	strcpy(un.sun_path,server_name);
	len = offsetof(struct sockaddr_un , sun_path)+strlen(server_name);
	
	if(connect(sockfd,(struct sockaddr *)&un,len) < 0){
		printf("%s line=%d %s \n",__FUNCTION__,__LINE__,strerror(errno));
		return -1;
	}
	
	return 0;
}

int send_fd_unix_domain(int sockfd , int fd_to_send)
{
	if(sockfd < 0 )
		return -1;

	struct iovec iov[1];
	struct msghdr msg;
	unsigned char buf[2] = {0};
	struct cmsghdr *cmsgptr = (struct cmsghdr*)(char [CMSG_LEN(sizeof(int))]){0};
	
	iov[0].iov_base = buf;
	iov[0].iov_len = sizeof(buf);

	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	if(fd_to_send < 0)
	{
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
		buf[1] = -fd_to_send;
		if(buf[1] == 0)
			buf[1] = 1;
	}
	else
	{
		cmsgptr->cmsg_level = SOL_SOCKET;
		cmsgptr->cmsg_type = SCM_RIGHTS;
		cmsgptr->cmsg_len = CMSG_LEN(sizeof(int));

		msg.msg_control = cmsgptr;
		msg.msg_controllen = CMSG_LEN(sizeof(int));
		*(int *)CMSG_DATA(cmsgptr) = fd_to_send;
	}

	
	int ret = sendmsg(sockfd, &msg, 0); 
	if( ret !=  sizeof(buf)){
		printf("%s line=%d %s \n",__FUNCTION__,__LINE__,strerror(errno));
		return -1;
	}

	return 0;
}

int send_errno_unix_domain(int sockfd , int errcode , const char *msg)
{
	int n ;

	if(errcode >= 0)
		return -1;

	if((n =  strlen(msg)) > 0)
		if(tcp_send_noblock(sockfd,(void*)msg,n) != n)
			return -1;

	if(send_fd_unix_domain(sockfd,errcode) < 0)
		return -1;

	return 0;
}

int recv_fd_unix_domain(int sockfd)
{
	struct iovec iov[1];
	struct msghdr msg;
	unsigned char buf[128] = {0};
	unsigned char *ptr = NULL;
	int nr,newfd,status = -1;
	struct cmsghdr *cmsgptr = (struct cmsghdr*)(char [CMSG_LEN(sizeof(int))]){0};

	for(;;)
	{
		iov[0].iov_base = buf;
		iov[0].iov_len = sizeof(buf);

		msg.msg_iov = iov;
		msg.msg_iovlen = 1;
		msg.msg_name = NULL;
		msg.msg_namelen = 0;
		msg.msg_control = cmsgptr;
		msg.msg_controllen = CMSG_LEN(sizeof(int));

		nr = recvmsg(sockfd,&msg,0);
		if(nr < 0)
			return -1;
		else if(nr == 0)
			return -1;

		for(ptr=buf;ptr<&buf[nr];)
		{
			if(*ptr++ == 0){

				if(ptr != &buf[nr-1])
					printf("message format error \n");

				status = *ptr & 0xFF;
				if(status == 0){

					if(msg.msg_controllen < CMSG_LEN(sizeof(int)))
						printf("status 0 but no fd \n");
					newfd = *(int *)CMSG_DATA(cmsgptr);
				}else {
					
					newfd = -status;
				}

				nr -= 2;
			}

		}

		//if(nr > 0 && status != 0)
		//	return -1;
		if(status >= 0)
			return newfd;
		
	}

	return -1;
}

int tcp_recv_protocol_cmd(int sockfd,void* cmd_head,void *data)
{
	int totallen = 0,readlen = 0;
	int times = 2000;
	
	if(cmd_head == NULL || data ==NULL || sockfd < 0)
		return 0;

	for (totallen = 0; totallen < PROTOCOL_CMD_HEAD_LEN; )
	{
		readlen = recv(sockfd,cmd_head + totallen, PROTOCOL_CMD_HEAD_LEN-totallen,MSG_DONTWAIT);
		if(readlen < 0){
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

	if(totallen != PROTOCOL_CMD_HEAD_LEN)
		return -1;

	int bodylen = GET_PROTOCOL_BODY_LEN(cmd_head);

	for (totallen = 0;totallen < bodylen;)
	{
		readlen = recv(sockfd,data + totallen,bodylen-totallen,MSG_DONTWAIT);
		if(readlen < 0){
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
	
	if(totallen != bodylen)
		return -1;
	
	return 0;
}

int tcp_send_protocol_cmd(int sockfd,void* cmd_head,void *data)
{
	int totallen = 0,sendlen = 0;
	int times = 2000;
	
	if(data ==NULL || cmd_head == NULL || sockfd < 0)
		return 0;

	for (totallen = 0; totallen < PROTOCOL_CMD_HEAD_LEN; )
	{
		sendlen = send(sockfd,cmd_head + totallen, PROTOCOL_CMD_HEAD_LEN-totallen,MSG_DONTWAIT);
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

	if(totallen != PROTOCOL_CMD_HEAD_LEN)
		return -1;

	
	int bodylen = GET_PROTOCOL_BODY_LEN(cmd_head);

	
	for (totallen = 0; totallen < bodylen; )
	{
		sendlen = send(sockfd,data + totallen, bodylen-totallen,MSG_DONTWAIT);
		if(sendlen < 0 ){
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

	if(totallen != bodylen)
		return -1;
	
	return 0;	
}

int wait_all_unix_connet(int sockfd)
{
	uint8_t buf[1+CMD_ALL_CONNECT_OK_LEN] = {0};

	int ret = recv(sockfd,buf,CMD_ALL_CONNECT_OK_LEN,MSG_WAITALL);
	if(ret != CMD_ALL_CONNECT_OK_LEN)
		return -1;

	//for(int i=0;i<CMD_ALL_CONNECT_OK_LEN;i++)
	//	printf("[%d] %x \n",i,CMD_ALL_CONNECT_OK[i]);
		
	if(isMemSameN(buf,CMD_ALL_CONNECT_OK,CMD_ALL_CONNECT_OK_LEN))
		return 0;
	
	return -1;
}
