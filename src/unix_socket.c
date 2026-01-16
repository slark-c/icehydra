#include <stddef.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <pthread.h>

#include "socket99.h"
#include "conf.h"

static bool delete_or_ignore(char *path) {
    int ures = unlink(path);
    if (ures == -1) {
        if (errno == ENOENT) {
            errno = 0;
        } else {
            return false;
        }
    }
    return true;
}

int create_bind_unix_tcp(char *path,bool isServer)
{
	socket99_config cfg = {
		.path = path,
	};

	if(isServer){
		cfg.server = isServer;
		if (!delete_or_ignore(ICEHYDRA_ROUTE_NAME))
			return -1;
	}
	
	socket99_result res;
	bool ok = socket99_open(&cfg, &res);
	if (!ok) {
		socket99_fprintf(stderr, &res);
		return -1;
	}
	
	int recv_buf_size = 0;
	socklen_t optlen = sizeof(recv_buf_size);
	getsockopt(res.fd, SOL_SOCKET, SO_RCVBUF,&recv_buf_size, &optlen);
	if(recv_buf_size < 2*Mib){
		optlen = sizeof(recv_buf_size);
		recv_buf_size = 2*Mib;
		setsockopt(res.fd, SOL_SOCKET, SO_RCVBUF,&recv_buf_size,optlen);
		optlen = sizeof(recv_buf_size);
		getsockopt(res.fd, SOL_SOCKET, SO_RCVBUF,&recv_buf_size, &optlen);
	}
	
	return res.fd;
}

int send_fd_unix_domain(int sockout , int fd)
{
	/* From the cmsg(3) manpage: */
	struct msghdr msg = { 0 };
	struct cmsghdr *cmsg;
	struct iovec iov;
	char c = 0;
	union {         /* Ancillary data buffer, wrapped in a union
			   in order to ensure it is suitably aligned */
		char buf[CMSG_SPACE(sizeof(fd))];
		struct cmsghdr align;
	} u;

	msg.msg_control = u.buf;
	msg.msg_controllen = sizeof(u.buf);
	memset(&u, 0, sizeof(u));
	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
	memcpy(CMSG_DATA(cmsg), &fd, sizeof(fd));

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;

	/* Keith Packard reports that 0-length sends don't work, so we
	 * always send 1 byte. */
	iov.iov_base = &c;
	iov.iov_len = 1;

	return sendmsg(sockout, &msg, 0) == 1;
}


int send_errno_unix_domain(int sockfd , int errcode , const char *msg)
{
	return 0;
}

int recv_fd_unix_domain(int sockin)
{
	/* From the cmsg(3) manpage: */
	struct msghdr msg = { 0 };
	struct cmsghdr *cmsg;
	struct iovec iov;
	int fd;
	char c;
	union {         /* Ancillary data buffer, wrapped in a union
			   in order to ensure it is suitably aligned */
		char buf[CMSG_SPACE(sizeof(fd))];
		struct cmsghdr align;
	} u;

	msg.msg_control = u.buf;
	msg.msg_controllen = sizeof(u.buf);

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;

	iov.iov_base = &c;
	iov.iov_len = 1;

	if (recvmsg(sockin, &msg, 0) < 0)
		return -1;

	cmsg = CMSG_FIRSTHDR(&msg);
        if (!cmsg
	    || cmsg->cmsg_len != CMSG_LEN(sizeof(fd))
	    || cmsg->cmsg_level != SOL_SOCKET
	    || cmsg->cmsg_type != SCM_RIGHTS) {
		errno = -EINVAL;
		return -1;
	}

	memcpy(&fd, CMSG_DATA(cmsg), sizeof(fd));
	return fd;
}


int tcp_recv_protocol_cmd(int sockfd,void* cmd_head,void *data)
{
	int totallen = 0,readlen = 0;
	int times = 500;
	
	if(cmd_head == NULL || data ==NULL || sockfd < 0)
		return -1;

	for (totallen = 0; totallen < PROTOCOL_CMD_HEAD_LEN; )
	{
		readlen = recv(sockfd,cmd_head + totallen, PROTOCOL_CMD_HEAD_LEN-totallen,MSG_DONTWAIT);
		if(readlen < 0){
			if(!(times--)){
				fprintf(stderr,"tcp_recv_protocol_cmd time out  \n");
				break;
			}
			
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR){
				usleep(2000);
				continue;
			}
			fprintf(stderr,"%s \n",strerror(errno));

			return readlen;
		}
		else if(readlen == 0)
			break;
		else
			totallen += readlen;	
	}

	if(totallen != PROTOCOL_CMD_HEAD_LEN)
		return -1;

	if(!CHECK_MAGIC(cmd_head))
		return -1;

	
	int body_len = GET_PROTOCOL(cmd_head,bodylen);

	for (totallen = 0;totallen < body_len;){
		readlen = recv(sockfd,data + totallen,body_len-totallen,MSG_DONTWAIT);
		if(readlen < 0){
			if(!(times--)){
				fprintf(stderr,"tcp_recv_protocol_cmd time out  \n");
				break;
			}
			
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR){
				usleep(2000);
				continue;
			}
			fprintf(stderr,"%s \n",strerror(errno));

			return readlen;
		}
		else if(readlen == 0)
			break;
		else
			totallen += readlen;	
	}
	
	if(totallen != body_len)
		return -1;
	
	return 0;
}

int tcp_send_protocol_cmd(int sockfd,void* cmd_head,void *data)
{
	int totallen = 0,sendlen = 0;
	int times = 500;
	
	if(data ==NULL || cmd_head == NULL || sockfd < 0)
		return -1;

	if(!CHECK_MAGIC(cmd_head))
		return -1;
	
	for (totallen = 0; totallen < PROTOCOL_CMD_HEAD_LEN; ){
		sendlen = send(sockfd,cmd_head + totallen, PROTOCOL_CMD_HEAD_LEN-totallen,MSG_DONTWAIT);
		if(sendlen < 0 ){
			if(!(times--))
				break;
			
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR){
				usleep(2000);
				continue;
			}
			fprintf(stderr,"%s \n",strerror(errno));
			return sendlen;
		}
		else if(sendlen == 0)
			break;
		else
			totallen += sendlen;
	}

	if(totallen != PROTOCOL_CMD_HEAD_LEN)
		return -1;

	
	int body_len = GET_PROTOCOL(cmd_head,bodylen);
	
	for (totallen = 0; totallen < body_len; )
	{
		sendlen = send(sockfd,data + totallen, body_len-totallen,MSG_DONTWAIT);
		if(sendlen < 0 ){
			if(!(times--))
				break;
			
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR){
				usleep(2000);
				continue;
			}
			
			fprintf(stderr,"%s \n",strerror(errno));
			return sendlen;
		}
		else if(sendlen == 0)
			break;
		else
			totallen += sendlen;
	}

	if(totallen != body_len)
		return -1;
	
	return 0;	
}

int tcp_send_protocol(int sockfd,void* cmd_head,void *cmd_head_data,int cmd_head_data_len,void *data,int datelen)
{
	int totallen = 0,sendlen = 0;
	int times = 500;
	
	if(data ==NULL || cmd_head == NULL || sockfd < 0 || cmd_head_data == NULL)
		return -1;

	if(!CHECK_MAGIC(cmd_head))
		return -1;
	
	for (totallen = 0; totallen < PROTOCOL_CMD_HEAD_LEN; ){
		sendlen = send(sockfd,cmd_head + totallen, PROTOCOL_CMD_HEAD_LEN-totallen,MSG_DONTWAIT);
		if(sendlen < 0 ){
			if(!(times--))
				break;
			
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR){
				usleep(2000);
				continue;
			}
			fprintf(stderr,"%s \n",strerror(errno));
			return sendlen;
		}
		else if(sendlen == 0)
			break;
		else
			totallen += sendlen;
	}

	if(totallen != PROTOCOL_CMD_HEAD_LEN)
		return -1;


	for (totallen = 0; totallen < cmd_head_data_len; )
	{
		sendlen = send(sockfd,cmd_head_data + totallen, cmd_head_data_len-totallen,MSG_DONTWAIT);
		if(sendlen < 0 ){
			if(!(times--))
				break;
			
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR){
				usleep(2000);
				continue;
			}
			
			fprintf(stderr,"%s \n",strerror(errno));
			return sendlen;
		}
		else if(sendlen == 0)
			break;
		else
			totallen += sendlen;
	}

	if(totallen != cmd_head_data_len)
		return -1;

	for (totallen = 0; totallen < datelen; )
	{
		sendlen = send(sockfd,data + totallen, datelen-totallen,MSG_DONTWAIT);
		if(sendlen < 0 ){
			if(!(times--))
				break;
			
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR){
				usleep(2000);
				continue;
			}
			
			fprintf(stderr,"%s \n",strerror(errno));
			return sendlen;
		}
		else if(sendlen == 0)
			break;
		else
			totallen += sendlen;
	}

	if(totallen != datelen)
		return -1;
	
	return 0;	




}
uint16_t parse_decimal_dot_string_to_hex(const char *input) {
    char *dot = NULL;
    char first_part[4] = {0};
    char second_part[4] = {0};

    dot = strchr(input, '.');
    if (!dot || dot == input || (dot-input) > 3) {
        fprintf(stderr, "Invalid format: missing or misplaced dot\n");
        return 0;
    }

    size_t first_len = dot - input;
   //size_t second_len = strlen(dot + 1);

    strncpy(first_part, input, first_len);
    strcpy(second_part, dot + 1);

    for (int i = 0; first_part[i]; i++) {
        if (!isdigit(first_part[i])) {
            fprintf(stderr, "Invalid decimal number: %s\n", first_part);
            return 0;
        }
    }
    for (int i = 0; second_part[i]; i++) {
        if (!isdigit(second_part[i])) {
            fprintf(stderr, "Invalid decimal number: %s\n", second_part);
            return 0;
        }
    }

    int first_val = atoi(first_part);
    int second_val = atoi(second_part);

    if (first_val < 0 || first_val > 255 || second_val < 0 || second_val > 255) {
        fprintf(stderr, "Value out of range (must be 0-255): %s.%s\n", first_part, second_part);
        return 0;
    }

    unsigned short result = ((unsigned char)first_val << 8) | (unsigned char)second_val;

    return result;
}

int wait_all_unix_connet(int sockfd)
{
	return 0;
}
