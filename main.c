#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>
#include <sys/stat.h>

#include "socket99.h"
#include "cJson.h"

#include "conf.h"
#include "unix_socket.h"
#include "shm_manager.h"

static struct option long_options[] = {
	{"daemon", no_argument, NULL, 'D'},
	{"version ", no_argument, NULL, 'v'},
	{"help ", no_argument, NULL, 'h'},	
	{"file ", no_argument, NULL, 'f'},
	{NULL, 0, NULL, 0}
};

static ICE_INFO_T icehydra_info[128] = {0};

static char config_file[MAX_FILE_LEN] = {0};
static void* route_thread(void *args);
static void* transfer_msg_thread(void *args);
static int parse_msg(ClientNode *client_info,void* cmd_head,void *data);
static void parse_conf(char *file);

RouteInfo routing_table = { \
	.route_mutex = PTHREAD_MUTEX_INITIALIZER,\
	.data_mutex = PTHREAD_MUTEX_INITIALIZER,\
};

int main(int argc ,char *argv[])
{ 	
	int ret;
	bool deamon = false;
	while ((ret = getopt_long(argc, argv, "f:Dvh", long_options, NULL)) != EOF){
		switch (ret){
			case 'v':
			{
				printf("Version : %s \n","1.3");
				return 0;
			}
				break;
			case 'h':
			{
				printf("Usage: %s [options]\n", argv[0]);
                printf("Options:\n");
                printf("  -h, --help       Display this help message\n");
                printf("  -v, --version    Display version information\n");
                printf("  -f, --file       Specify the output file\n");
				printf("  -D, --daemon     Run in Daemon\n");
               	return 0;
			}
				break;
			case 'D':
				deamon = true;
				break;
			case 'f':
			{
				if(access(optarg, F_OK | R_OK)){
					fprintf(stderr, "[%s] can not access,use default config\n", optarg);
				}else{
					strcpy(config_file,optarg);		
					parse_conf(config_file);
					for(int i=0;i<icehydra_info[0].shm_num;i++){
						printf("shm %d:%s \n",i,icehydra_info[i].shm_name);
					}
				}
			}
				break;
			default:
			{
				fprintf(stderr, "Unexpected option: Use -h for help \n");
                return -1;
			}
				break;
		}
	}

	if (deamon){
		ret = daemon(1,0);
		if (ret < 0){
			fprintf(stderr, "daemon() error \n");
			return -1;
		}
	}
	
	signal(SIGPIPE, SIG_IGN);
	INIT_LIST_HEAD(&routing_table.client_node_head);	
	INIT_LIST_HEAD(&routing_table.data_node_head);
	
	int listen_fd = create_bind_unix_tcp(ICEHYDRA_ROUTE_NAME,true);
	if(listen_fd < 0){
		fprintf(stderr,"creat_bind_unix_tcp: %s\n", strerror(errno));
		return -1;
	}

	pthread_t thread_id; 
	ret = pthread_create(&thread_id, NULL,transfer_msg_thread,NULL);
	if(ret) {
		fprintf(stderr,"pthread_create: %s\n", strerror(errno));
		return 0;
	}
	pthread_detach(thread_id);
	
	while(1){
		SockInfo *sock_info = calloc(1,sizeof(SockInfo));
		if(!sock_info){
			fprintf(stderr,"calloc: %s\n", strerror(errno));
			usleep(1000);
			continue;
		}
		sock_info->addr_len = sizeof(sock_info->address);
		
		sock_info->conn_fd =  accept(listen_fd, &sock_info->address, &sock_info->addr_len);
		if (sock_info->conn_fd == -1) {
			free(sock_info);
			fprintf(stderr,"accept: %s\n", strerror(errno));
			continue;
		}

		ret = pthread_create(&sock_info->thread_id, NULL, route_thread, (void *)sock_info);
		if (ret){
			close(sock_info->conn_fd);
			free(sock_info);
			fprintf(stderr,"thread create error: %s\n", strerror(errno));
			continue;
		}
		
		pthread_detach(sock_info->thread_id);
		usleep(1000);
	}

    return 0;
}

static void* transfer_msg_thread(void *args)
{	
	struct list_head *current_list = NULL;
	//int ret = 0 ;
	PROTOCOL_CMD cmd_head = {0};
	memcpy(cmd_head.szMagic,PROTOCOL_MAGIC,STRING_LEN(PROTOCOL_MAGIC));
	
	while(1){
		
		if(list_empty(&routing_table.data_node_head)){
			usleep(400);
			continue;
		}

	pthread_mutex_lock(&routing_table.data_mutex);
		
		DataNode *data_node = list_entry(routing_table.data_node_head.next,DataNode,next);

		list_del(&data_node->next);

	pthread_mutex_unlock(&routing_table.data_mutex);

		
	pthread_mutex_lock(&routing_table.route_mutex);
	
		if(list_empty(&routing_table.client_node_head)){
			pthread_mutex_unlock(&routing_table.route_mutex);
			free(data_node);
			continue;
		}

		for(int i=0;i<MAX_P2P_NODE;i++){
			uint16_t address_u16 = data_node->address_u16[i];
			if(!address_u16)
				break;
			
			list_for_each(current_list,&routing_table.client_node_head){
				ClientNode *client_node = list_entry(current_list,ClientNode, next);
				if(address_u16 != client_node->address_u16)
					continue;

				cmd_head.bodylen = data_node->datalen;
				cmd_head.cmdid = IH_CMD_P2P_DATA;
				cmd_head.broadcast_nums = 1;
				tcp_send_protocol_cmd(client_node->conn_fd,&cmd_head,data_node->data);
				break;
			}
		}
	pthread_mutex_unlock(&routing_table.route_mutex);

		free(data_node->data);
		free(data_node);
		
	}

	return NULL;
}

static ClientNode *new_client_node(int connfd)
{
	if(connfd <= 0)
		return NULL;

	ClientNode *client_node = calloc(1,sizeof(ClientNode));
	if(!client_node)
		return NULL;

	client_node->conn_fd = connfd;
	
	
	pthread_mutex_lock(&routing_table.route_mutex);
	
		list_add_tail(&client_node->next,&routing_table.client_node_head);

	pthread_mutex_unlock(&routing_table.route_mutex);

	return client_node;
}
static void delete_client_node(ClientNode *client_info)
{

pthread_mutex_lock(&routing_table.route_mutex);

	if(!client_info){	
		pthread_mutex_unlock(&routing_table.route_mutex);
		return ;
	}

	list_del(&client_info->next);
	if(client_info->conn_fd > 0){
		shutdown(client_info->conn_fd, SHUT_RDWR);
		close(client_info->conn_fd);
	}

pthread_mutex_unlock(&routing_table.route_mutex);

	printf("Delete conn fd %d addr 0x%x \n",client_info->conn_fd,client_info->address_u16);
	free(client_info);
}

static void* route_thread(void *args)
{
	SockInfo *sock_info = (SockInfo*)args;
	fd_set readfds;
	struct timeval timeout;
	int ret ;	
	uint8_t *rbuf  = calloc(1,MAX_CMD_LEN);

	ClientNode *client_node = new_client_node(sock_info->conn_fd);
	if(!client_node)
		goto exit_thread;
	
	while(1){

		FD_ZERO(&readfds);
		FD_SET(sock_info->conn_fd, &readfds);		
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;
		ret = select(sock_info->conn_fd + 1, &readfds,NULL, NULL, &timeout);
		if(ret < 0)
			goto exit_thread;
		else if(ret == 0)
			continue;
	
		if(!FD_ISSET(sock_info->conn_fd, &readfds))
			continue;

		memzero(rbuf,MAX_CMD_LEN);
		ret = tcp_recv_protocol_cmd(sock_info->conn_fd,rbuf,rbuf+PROTOCOL_CMD_HEAD_LEN);
		if(ret < 0){
			//fprintf(stderr,"recv error %d \n",client_node->conn_fd);
			goto exit_thread;
		}
		
		ret = parse_msg(client_node,rbuf,rbuf+PROTOCOL_CMD_HEAD_LEN);
		if(ret < 0){
			goto exit_thread;
		}	
	}

exit_thread:
	free(sock_info);
	free(rbuf);
	delete_client_node(client_node);
	pthread_exit(NULL);
	return NULL;
}

static int push_node_data(ClientNode *client_node,void *address_u16,int address_len,void *data,int datalen)
{
	if(!datalen || !address_len|| !data || !address_u16 || !client_node)
		return 0;

	DataNode *data_node = calloc(1,sizeof(DataNode));
	if(!data_node)
		return 0;

	//data_node->data = data_node->short_data;
	data_node->data = calloc(1,datalen);
	data_node->datalen = datalen;
	memcpy(data_node->address_u16,address_u16,address_len);
	memcpy(data_node->data,data,datalen);
	
	pthread_mutex_lock(&routing_table.data_mutex);
	
		list_add_tail(&data_node->next,&routing_table.data_node_head);

	pthread_mutex_unlock(&routing_table.data_mutex);
		
	return 0;
}
static int parse_msg(ClientNode *client_node,void* cmd_head,void *data)
{
	
	int cmdid = GET_PROTOCOL(cmd_head,cmdid);
	switch(cmdid){
		case IH_CMD_NODE_ADDRESS_U16:{
			memcpy(&client_node->address_u16,data,sizeof(client_node->address_u16));
			printf("Add fd %d addr 0x%x \n",client_node->conn_fd,client_node->address_u16);
			return 0;
		}
			break;
		case IH_CMD_NODE_ADDRESS_STR:{
			strncpy(client_node->address_string,data,ADDR_LEN-1);
			client_node->address_u16 = parse_decimal_dot_string_to_hex(client_node->address_string);
			if(!client_node->address_u16)
				return -1;
			else
				printf("Add fd %d addr 0x%x \n",client_node->conn_fd,client_node->address_u16);
			return 0;
		}
			break;
		case IH_CMD_NODE_NAME_U16:
			break;
		case IH_CMD_NODE_NAME_STR:{
			strcpy(client_node->name,data);
			return 0;
		}
			break;
		case IH_CMD_P2P_BY_ADDR_U16:{
			int address_len = GET_PROTOCOL(cmd_head,broadcast_nums)*sizeof(uint16_t);
			int datalen = GET_PROTOCOL(cmd_head,bodylen)-address_len;
			push_node_data(client_node,data,address_len,data+address_len,datalen);
		}
			break;
		case IH_CMD_P2P_BY_ADDR_STR:{
			int broadcast_nums = GET_PROTOCOL(cmd_head,broadcast_nums);			
			int datalen = GET_PROTOCOL(cmd_head,bodylen)-broadcast_nums*ADDR_LEN;
			uint16_t address_u16[MAX_P2P_NODE] = {0};		
			for(int i=0;i<broadcast_nums;i++){
				address_u16[i] = parse_decimal_dot_string_to_hex(data+i*ADDR_LEN);
			}
			push_node_data(client_node,address_u16,broadcast_nums*sizeof(uint16_t),data+broadcast_nums*ADDR_LEN,datalen);
		}
			break;
		case IH_CMD_P2P_BY_NAME_STR:
			break;
		case IH_CMD_GET_SHM_INFO:{
			int datalen = sizeof(ICE_INFO_T)*icehydra_info[0].shm_num;
			if(!datalen)
				datalen = sizeof(ICE_INFO_T);
			push_node_data(client_node,&client_node->address_u16,sizeof(uint16_t),icehydra_info,datalen);
		}
			break;
		default:
			break;
	}	

	return 0;
}

typedef struct 
{
	char *key;
	//uint32_t value_type;
}json_item;

static json_item item_list[] = {

	{ .key = "ih-name",},

	{ .key = "ih-pid", },

	{ .key = "ih-cpu_bind",},

	{ .key = "ih-shm", },
};

void parse_conf(char *file)
{
	char *json_string = NULL;
	struct stat sstat; 
  	stat(file, &sstat); 
  	size_t size=sstat.st_size; 
	if(!size)
		return ;

	json_string = (char *)calloc(1,size+10);
	if(!json_string)
		return;
	
	FILE *fp = fopen(file,"rb");
	if (fp == NULL){
		free(json_string);
		return ;
	}

	fread(json_string,size,1,fp);
	fclose(fp);

	make_dir("/run/shm/huge");
	const char *next_obj;
	cJSON* json = cJSON_ParseWithOpts(json_string,&next_obj,0);	
	while(json){
		
		for(int i = 0;i<ARRAY_SIZE(item_list);i++){
			cJSON * object = cJSON_GetObjectItem(json,item_list[i].key);
			if(!object)
				continue;
			if(strsame(object->string,"ih-shm")){
				cJSON * shm_json_obj = NULL;				
				icehydra_shm_t shm = {.type = SHM_POSIX};
				ICE_INFO_T *ice_info = icehydra_info;
				cJSON_ArrayForEach(shm_json_obj, object){
					cJSON *shm_type = cJSON_GetObjectItemCaseSensitive(shm_json_obj, "shm-type");
    				cJSON *shm_size = cJSON_GetObjectItemCaseSensitive(shm_json_obj, "shm-size");
					cJSON *shm_name = cJSON_GetObjectItemCaseSensitive(shm_json_obj, "shm-name");
					if(shm_type && shm_type->valuestring){
						if(strsame(shm_type->valuestring,"posix"))
							shm.type = SHM_POSIX;
						else if(strsame(shm_type->valuestring,"sysv"))
							shm.type = SHM_SYSV;
						else if(strsame(shm_type->valuestring,"memfd"))
							shm.type = SHM_MEMFD;
						else if(strsame(shm_type->valuestring,"hugetlbfs"))
							shm.type = SHM_HUGEPAGESTLBFS;
					}
					if(shm_name && shm_name->valuestring){
						shm.filename = calloc(1,256);
						if(shm.type == SHM_SYSV)
							strcpy(shm.filename,name_fix("/run/shm/",shm_name->valuestring,"-sysv"));
						else if(shm.type == SHM_HUGEPAGESTLBFS)
							strcpy(shm.filename,name_fix("/run/shm/huge/",shm_name->valuestring,NULL));
						else
							strcpy(shm.filename,shm_name->valuestring);
					}
					shm.size = shm_size->valueint;
					shm_create(&shm);
					shm_munmap(&shm);
					ice_info->shm_type = shm.type;
					ice_info->shm_size = shm.size;
					strcpy(ice_info->shm_name,shm.filename);
					icehydra_info[0].shm_num += 1;
					ice_info++;
					if(shm.filename)
						free(shm.filename);
				}
				break;
			}
		}
	
		cJSON_Delete(json);		
		json = cJSON_ParseWithOpts(next_obj,&next_obj,0);
	}

	free(json_string);

	return;
}
