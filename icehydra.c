#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include "conf.h"
#include "unix_socket.h"
#include "icehydra_util.h"
#include "icehydra_cmd.h"
#include "shm_manager.h"

#include "icehydra.h"

static int client_connfd = -1;
int ih_subscriber_create_connect(char *subscriber_name,char *publisher_name)
{	
	char *un_client_name = name_fix(NAME_PREFIX,subscriber_name,SOCK_NAME_SUFFIX);	
	char *un_server_name = name_fix(NAME_PREFIX,publisher_name,SOCK_NAME_SUFFIX);

	int sockfd = creat_bind_unix_tcp(un_client_name);	
	
	int ret = connect_unix_tcp(sockfd,un_server_name);
	if(ret<0){
		close_s(sockfd);
	}
	
	free_s(un_client_name);
	free_s(un_server_name);

	client_connfd = sockfd;
	return sockfd;
}

void ih_wait_other_process(void)
{
	int ret = wait_all_unix_connet(client_connfd);
	
	if(ret <0)
		exit(-1);
	
	return ;
}

int ih_send_broadcast_data(IH_BROADCAST_CMD_T *cmd)
{
	uint8_t cmd_head[64] = {0};
	int ret = 0;
	
	set_broadcast_cmd_head(cmd_head,cmd->datalen,IH_CMD_ID_DEFAULT,cmd->br_num);	
	set_pure_broadcast_field(cmd_head,NULL,cmd->br_num,cmd->br_ids);

	ret = tcp_send_noblock(client_connfd,cmd_head,PROTOCOL_CMD_HEAD_LEN+round4(cmd->br_num));
	ret += tcp_send_noblock(client_connfd,cmd->data,cmd->datalen);
	if(ret < 0){
		close_s(client_connfd);
	}
	
	return ret;
}

int ih_send_getShmIDs_cmd(void)
{
	uint8_t wbuf[64] = {0};
	set_pure_protocol_head(wbuf,0,IH_CMD_TYPE_SEND_RESERVE,IH_RESERVE_CMD_ID_GET_SHM_INFO);
	return tcp_send_protocol_cmd(client_connfd,wbuf,wbuf+PROTOCOL_CMD_HEAD_LEN);
}

int ih_recv_data(void *data,int *datalen)
{
	PROTOCOL_CMD cmd_head = {0};
	*datalen = 0;
	int ret = tcp_recv_protocol_cmd(client_connfd,&cmd_head,data);	
	if(ret < 0){
		close_s(client_connfd);
		return -1;
	}
	
	*datalen = cmd_head.bodylen;
	
#ifndef NDEBUG
	print_cmd_head(&cmd_head);
	print_data_char(GET_PROTOCOL_CMD_BROADCAST_NUM(&cmd_head),data);
#endif

	if(cmd_head.cmdid != IH_RESERVE_CMD_ID_GET_SHM_INFO)
		return 0;
	
	return IH_RESERVE_CMD_ID_GET_SHM_INFO;
}

int ih_get_shm_by_id(int shm_id,void **ptr)
{
	icehydra_shm_t shm_tmp = {0};
	shm_tmp.id = shm_id;
	*ptr = shm_mmap(&shm_tmp);

	return shm_tmp.size;
}

int ih_get_shm_by_name(void *infobuf,char *name,void **ptr)
{
	SHM_NODE_T *shmst_list = (SHM_NODE_T*)infobuf;

	do{
		if(isStrSame(name,shmst_list->shm_name)
			&& strlen(name) == strlen(shmst_list->shm_name))
			break;
		
		if(strlen(shmst_list->shm_name) == 0)
			return -1;

		shmst_list++;
		
	}while(1);
	
	return ih_get_shm_by_id(shmst_list->shm_id,ptr);
}

bool ih_recvIsShmIDs(int ret)
{
	return (IH_RESERVE_CMD_ID_GET_SHM_INFO == ret);
}

int ih_get_shm_infos(void *buffer,int buffersize)
{
	uint8_t wbuf[64] = {0};
	int ret;
	set_pure_protocol_head(wbuf,0,IH_CMD_TYPE_SEND_RESERVE,IH_RESERVE_CMD_ID_GET_SHM_NUM);
	tcp_send_protocol_cmd(client_connfd,wbuf,wbuf+PROTOCOL_CMD_HEAD_LEN);

	do{
		memzero(wbuf,sizeof(wbuf));
		ret = tcp_recv_protocol_cmd(client_connfd,wbuf,wbuf+PROTOCOL_CMD_HEAD_LEN);
		if(ret <0)
			return -1;
		
	}while(GET_PROTOCOL_CMD_ID(wbuf) != IH_RESERVE_CMD_ID_GET_SHM_NUM);
	
	int shm_num = wbuf[PROTOCOL_CMD_HEAD_LEN];
	if(sizeof(SHM_NODE_T)*shm_num > buffersize){
		fprintf(stderr,"buffsersize %d too small \n",buffersize);
		return -1;
	}

	if(shm_num == 0)
		return 0;
	
	ret = ih_send_getShmIDs_cmd();
	if(ret < 0)
		return ret;

	int datalen = 0;	
	ret = ih_recv_data(buffer,&datalen);
	return datalen/sizeof(SHM_NODE_T);	
}

bool ih_select_recv_ready(struct timeval *timeout)
{
	fd_set rset;
	struct timeval ih_timeout;

	FD_ZERO(&rset);
	FD_SET(client_connfd, &rset);
	
	memcpy(&ih_timeout,timeout,sizeof(struct timeval));
	
	int ret = select(client_connfd + 1, &rset, NULL, NULL, &ih_timeout);
	if(ret <= 0){
		return false;
	}
	
	if(!FD_ISSET(client_connfd, &rset))
		return false;

	return true;
}
