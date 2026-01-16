#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>

#include "conf.h"
#include "unix_socket.h"
#include "list.h"
#include "shm_manager.h"
#include "icehydra.h"

typedef struct 
{
	int connfd;
	uint16_t address_u16;

	struct list_head next;
}SendTable;

static pthread_mutex_t   send_table_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct list_head  send_table_head = {0};

static SendTable *find_table_node(IH_BROADCAST_CMD_T *to_send)
{
	if(list_empty(&send_table_head)){
		return NULL;
	}

	struct list_head *current_list = NULL;

	if(to_send->from_address_string){
		to_send->from_address_u16 = parse_decimal_dot_string_to_hex(to_send->from_address_string);
	}

	if(!to_send->from_address_u16)
		return NULL;
	
pthread_mutex_lock(&send_table_mutex);

	list_for_each(current_list,&send_table_head){
		SendTable *table_node = list_entry(current_list,SendTable,next);
		if(to_send->from_address_u16 == table_node->address_u16){
			pthread_mutex_unlock(&send_table_mutex);
			return table_node;
		}		
	}	
pthread_mutex_unlock(&send_table_mutex);

	return NULL;
}
static void remove_fd(IH_BROADCAST_CMD_T *to_send)
{
	SendTable *node = find_table_node(to_send);
	if(!node)
		return;

	pthread_mutex_lock(&send_table_mutex);
		list_del(&node->next);
	pthread_mutex_unlock(&send_table_mutex);


	close(node->connfd);
	free(node);
	
	return ;
}

static void push_fd(int connfd,IH_BROADCAST_CMD_T *to_send)
{
	SendTable *table_node = calloc(1,sizeof(SendTable));
	if(!table_node)
		return ;

	table_node->address_u16 = to_send->from_address_u16;
	table_node->connfd = connfd;
	
	pthread_mutex_lock(&send_table_mutex);
				
		list_add_tail(&table_node->next,&send_table_head);
		
	pthread_mutex_unlock(&send_table_mutex);

	return ;
}

int ih_remove(IH_BROADCAST_CMD_T *to_send)
{
	remove_fd(to_send);
	return 0;
}

int ih_subscriber_create_connect(IH_BROADCAST_CMD_T *to_send)
{	
	if(!send_table_head.next)	
		INIT_LIST_HEAD(&send_table_head);

	SendTable *node = find_table_node(to_send);
	if(node){
		//remove_fd(to_send);
		return 0;
	}

	
	int connfd = create_bind_unix_tcp(ICEHYDRA_ROUTE_NAME,false);
	if(connfd<0)
		return connfd;
	
	PROTOCOL_CMD cmd_head = {0};
	memcpy(cmd_head.szMagic,PROTOCOL_MAGIC,STRING_LEN(PROTOCOL_MAGIC));
	cmd_head.bodylen = sizeof(uint16_t);	
	cmd_head.cmdid = IH_CMD_NODE_ADDRESS_U16;
	void *data = &to_send->from_address_u16;

	if(to_send->from_address_string){
		to_send->from_address_u16 = parse_decimal_dot_string_to_hex(to_send->from_address_string);
	}
	
	int ret = tcp_send_protocol_cmd(connfd,&cmd_head,data);
	if(ret < 0){
		close(connfd);
		return ret;
	}

	push_fd(connfd,to_send);
	return 0;
}

int ih_subscriber_close(uint16_t from_address_u16)
{
	IH_BROADCAST_CMD_T to_send = {0};
	to_send.from_address_u16 = from_address_u16;
	remove_fd(&to_send);
	return 0;
}

void ih_wait_other_process(void)
{
	
	return ;
}

int ih_send_broadcast_data(IH_BROADCAST_CMD_T *to_send)
{
	SendTable *node = find_table_node(to_send);
	if(!node)
		return -1;
	
	
	PROTOCOL_CMD cmd_head = {0};
	memcpy(cmd_head.szMagic,PROTOCOL_MAGIC,STRING_LEN(PROTOCOL_MAGIC));
	cmd_head.broadcast_nums = to_send->br_num;

	void *head_data = NULL;
	int head_data_len = 0;
	if(to_send->address_u16){
		cmd_head.cmdid = IH_CMD_P2P_BY_ADDR_U16;
		head_data = to_send->address_u16;
		head_data_len = to_send->br_num*sizeof(uint16_t);
	}
	else if(to_send->address_string){
		cmd_head.cmdid = IH_CMD_P2P_BY_ADDR_STR;
		head_data = to_send->address_string;
		head_data_len = to_send->br_num*ID_ADDR_LEN;
	}
	
	cmd_head.bodylen = to_send->datalen+head_data_len;
	int ret = tcp_send_protocol(node->connfd,&cmd_head,head_data,head_data_len,to_send->data,to_send->datalen);
	if(ret<0){
		remove_fd(to_send);
	}

	return ret;
}

int ih_send_getShmIDs_cmd(void)
{
	return 0;
}

int ih_recv_data(IH_BROADCAST_CMD_T *to_send)
{	
	SendTable *node = find_table_node(to_send);
	if(!node)
		return -1;
	
	PROTOCOL_CMD cmd_head = {0};
	int ret = tcp_recv_protocol_cmd(node->connfd,&cmd_head,to_send->data);
	if(ret<0){
		fprintf(stderr,"recv error : %s \n",strerror(errno));
		remove_fd(to_send);
		return ret;
	}
	to_send->datalen = GET_PROTOCOL(&cmd_head,bodylen);


	return 0;
}

int ih_create_shm(IH_SHM_T *shm_ih)
{
	icehydra_shm_t shm = {0};

	shm.type = shm_ih->type;
	shm.size = shm_ih->size;
	shm.filename = calloc(1,256);
	if(!shm.filename)
		return -1;
	if(shm.type == SHM_SYSV)
		strcpy(shm.filename,name_fix("/run/shm/",shm_ih->name,"-sysv"));
	else if(shm.type == SHM_HUGEPAGESTLBFS)
		strcpy(shm.filename,name_fix("/run/shm/huge/",shm_ih->name,NULL));
	else
		strcpy(shm.filename,shm_ih->name);
	
	shm_create(&shm);
	shm_ih->mem_ptr = shm.mem_ptr;
	free(shm.filename);
	int ret = !!(shm_ih->mem_ptr)-1;
	return ret;
}

int ih_mmap_shm(IH_SHM_T *shm_ih)
{
	int ret = 0;
	icehydra_shm_t shm = {0};

	shm.type = shm_ih->type;
	shm.size = shm_ih->size;
	shm.filename = calloc(1,256);
	if(!shm.filename)
		return -1;
	/*if(shm.type == SHM_SYSV)
		strcpy(shm.filename,name_fix("/run/shm/",shm_ih->name,"-sysv"));
	else if(shm.type == SHM_HUGEPAGESTLBFS)
		strcpy(shm.filename,name_fix("/run/shm/huge/",shm_ih->name,NULL));
	else*/
		strcpy(shm.filename,shm_ih->name);
	
	shm_mmap(&shm);
	shm_ih->mem_ptr = shm.mem_ptr;
	free(shm.filename);
	ret = !!(shm_ih->mem_ptr)-1;
	return ret;
}

int ih_munmap_shm(IH_SHM_T *shm_ih)
{
	int ret = 0;
	icehydra_shm_t shm = {0};
	shm.type = shm_ih->type;
	shm.size = shm_ih->size;
	shm.mem_ptr = shm_ih->mem_ptr;
	ret =shm_munmap(&shm);
	return ret;
}

int ih_get_shm_by_id(int shm_id,void **ptr)
{
	return 0;
}

int ih_get_shm_by_name(char *name,IH_BROADCAST_CMD_T *to_send_recv)
{
	SendTable *node = find_table_node(to_send_recv);
	if(!node){
		fprintf(stderr,"cant find node \n");
		return -1;
	}
	
	PROTOCOL_CMD cmd_head = {0};
	memcpy(cmd_head.szMagic,PROTOCOL_MAGIC,STRING_LEN(PROTOCOL_MAGIC));
	cmd_head.broadcast_nums = 1;
	cmd_head.cmdid = IH_CMD_GET_SHM_INFO;
	
	void *head_data = &to_send_recv->from_address_u16;
	int head_data_len = sizeof(uint16_t);
	
	cmd_head.bodylen = 1+head_data_len;
	int ret = tcp_send_protocol(node->connfd,&cmd_head,head_data,head_data_len,to_send_recv->data,1);
	if(ret<0){
		fprintf(stderr,"ret %d %s \n",ret,strerror(errno));
		remove_fd(to_send_recv);
		return -1;
	}

	IH_BROADCAST_CMD_T to_recv = {0};
	memcpy(&to_recv,to_send_recv,sizeof(IH_BROADCAST_CMD_T));
	to_recv.data = calloc(1,16*Kib);

	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 100000;
	if((ret=ih_select_recv_ready(&to_recv,&timeout))<0){
		fprintf(stderr,"ih_select_recv_ready : %s \n",strerror(errno));
		free(to_recv.data);
		return -1;
	}
	
	ret = ih_recv_data(&to_recv);
	if(ret < 0){
		free(to_recv.data);
		return -1;
	};
	
	int datalen = to_recv.datalen;
	ICE_INFO_T *ice_info = (ICE_INFO_T*)(to_recv.data);
	IH_SHM_T  *ih_info = (IH_SHM_T*)to_send_recv->data;

	int shm_num  = datalen/sizeof(ICE_INFO_T);
	if(shm_num != ice_info->shm_num){
		fprintf(stderr,"num %d %d \n",shm_num,ice_info->shm_num);
		free(to_recv.data);
		return -1;
	}
	
	to_send_recv->datalen = shm_num*sizeof(IH_SHM_T);
	while(shm_num){
		ih_info->type =ice_info->shm_type;
		ih_info->size = ice_info->shm_size;
		strcpy(ih_info->name,ice_info->shm_name); 
		shm_num--;
		ice_info++;
		ih_info++;
	}

	free(to_recv.data);
	return 0;
}

bool ih_recvIsShmIDs(int ret)
{
	return 0;
}

int ih_get_shm_infos(void *buffer,int buffersize)
{
	
	return 0;
}

int ih_select_recv_ready(IH_BROADCAST_CMD_T *to_send,struct timeval *timeout)
{
	SendTable *node = find_table_node(to_send);
	if(!node)
		return -1;

	int client_connfd = node->connfd;
	
	fd_set rset;
	struct timeval ih_timeout;

	FD_ZERO(&rset);
	FD_SET(client_connfd, &rset);
	
	memcpy(&ih_timeout,timeout,sizeof(struct timeval));
	
	int ret = select(client_connfd + 1, &rset, NULL, NULL, &ih_timeout);
	if(ret < 0){
		remove_fd(to_send);
		return -1;
	}else if(ret == 0)
		return 0;
	
	if(!FD_ISSET(client_connfd, &rset))
		return 0;

	return 1;
}
