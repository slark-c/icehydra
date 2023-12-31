#include <string.h>
#include <stdint.h>
#include <iso646.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "icehydra_util.h"
#include "cJSON.h"
#include "ih_types.h"
#include "conf.h"
#include "shm_manager.h"
#include "unix_socket.h"
#include "icehydra_cmd.h"

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

static pub_sub_node_t *create_pub_sub_object()
{
	pub_sub_node_t *obj = calloc(1,sizeof(pub_sub_node_t));
	assert(obj != NULL);
	return obj;
}

static int init_shm(pub_sub_node_t* node_start,cJSON* json)
{
	char filename[MAX_PATH] = {0};
	cJSON * shm_json_obj = NULL;
	int shm_node_num = cJSON_GetArraySize(json);

	node_start->shm_node_ptr = calloc(shm_node_num,sizeof(SHM_NODE_T));
	if(!node_start->shm_node_ptr )
		return -1;
	
	sprintf(filename,"%s%s",NAME_PREFIX,node_start->name);

	SHM_NODE_T *shm_node_ptr = node_start->shm_node_ptr;
	uint8_t shm_proj_id = 0;
	cJSON_ArrayForEach(shm_json_obj, json){

		shm_proj_id++;
		
		cJSON *shm_name = cJSON_GetObjectItemCaseSensitive(shm_json_obj, "shm_name");
    	cJSON *shm_size = cJSON_GetObjectItemCaseSensitive(shm_json_obj, "shm_size");		
		
		//node_start->shm_node_ptr->shm_id = shm_proj_id;

		icehydra_shm_t shm = {.filename = filename,
						.proj_id = shm_proj_id,
						.size = shm_size->valueint};

		shm_create(&shm);
		assert(shm.mem_ptr != NULL);

		shm_node_ptr->shm_id = shm.id;
		shm_node_ptr->shm_size = shm.size;
		strcpy(shm_node_ptr->shm_name,shm_name->valuestring);
		shm_node_ptr++;
		shm_munmap(&shm);			
		//printf("%p %s size = %d %d\n",shm_node_ptr,shm_name->valuestring,shm_size->valueint,shm_node_num);
	}	

	node_start->shm_node_num = shm_proj_id;
	return 0;
}

static pub_sub_node_t *get_icehydra_json_node(cJSON* json)
{
#define Publisher  "Publisher"
#define Subscriber "Subscriber"

	pub_sub_node_t* sub_pub_obj = create_pub_sub_object();
	
	for(int i = 0;i<ARRAY_SIZE(item_list);i++)
	{
		cJSON * object = cJSON_GetObjectItem(json,item_list[i].key);
		if(!object)
			continue;

		if(isStrSame(object->string,"ih-name")){
			strcpy(sub_pub_obj->name,object->valuestring);
		}
		
		if(isStrSame(object->string,"ih-pid")){			
			sub_pub_obj->id = object->valueint;
			if(object->valueint == 0)
				sub_pub_obj->type = CHUNK_TYPE_PUB;
			else
				sub_pub_obj->type = CHUNK_TYPE_SUB;
		}
		
		if(isStrSame(object->string,"ih-cpu_bind")){
			sub_pub_obj->cpu_bind = object->valueint;
		}

		if(isStrSame(object->string,"ih-shm")){
			init_shm(sub_pub_obj,object);
		}
	}

#undef Publisher
#undef Subscriber

	return sub_pub_obj;
}

pub_sub_node_t *get_node_start(char *file,char *key)
{
	char *json_string = NULL;

	int size = read_file(file,&json_string);
	if(size < 0)
		return NULL;

	const char *next_obj;
	cJSON* json = cJSON_ParseWithOpts(json_string,&next_obj,0);
	int node_count = 0;
	pub_sub_node_t *result_node = NULL;	
	pub_sub_node_t *head_node = NULL;
	while(json)
	{
		pub_sub_node_t *node = get_icehydra_json_node(json);
		if(!node_count and node->id){
			fprintf(stderr,"[Error] : First node id should be 0 \n");
			exit(-1);
		}

		if(node->id == 0){
			INIT_LIST_HEAD(&(node->node_list));
			head_node = node;
		    result_node = node;
		}else
			list_add_tail(&(node->node_list), &(head_node->node_list));
	
		cJSON_Delete(json);		
		json = cJSON_ParseWithOpts(next_obj,&next_obj,0);
		node_count++;
	}

	result_node->pub_links_num = node_count-1;
	free(json_string);
	return result_node;	
}
pub_sub_node_t *find_node_by_id(struct list_head *head,int id)
{	
	struct list_head * current_node_list = NULL;
	pub_sub_node_t *current_node = NULL;

	list_for_each(current_node_list,head){
		current_node = list_entry(current_node_list, pub_sub_node_t, node_list);
		if(current_node->id == id)
			break;
		current_node = NULL;
	}
	
	return current_node;
}

pub_sub_node_t *find_node_by_name(struct list_head *head,char *name)
{	
	struct list_head * current_node_list = NULL;
	pub_sub_node_t *current_node = NULL;

	list_for_each(current_node_list,head){
		current_node = list_entry(current_node_list, pub_sub_node_t, node_list);
		if(isStrSameN(current_node->name,name,strLen(current_node->name)))
			break;
		current_node = NULL;
	}
	
	return current_node;
}

int set_pure_protocol_head(void* protocol,int bodylen,int cmdSendType,int cmdid)
{
	PROTOCOL_CMD *head = (PROTOCOL_CMD *)protocol;

	if(bodylen>=0 and cmdSendType>=0 and cmdid>=0)		
		memzero(head, PROTOCOL_CMD_HEAD_LEN);
	
	memcpy(head->szMagic,PROTOCOL_MAGIC, 2);
	
	if(bodylen >= 0)
		memcpy(&head->bodylen, &bodylen, sizeof(uint16_t));

	if(cmdSendType >= 0)
		head->cmdSendType = cmdSendType;

	if(cmdid >= 0)
		head->cmdid = cmdid;

	return 0;
}

int set_broadcast_cmd_head(void* cmd_head,int bodylen,int cmdid,int br_num)
{
	int ret = set_pure_protocol_head(cmd_head,bodylen+round4(br_num),IH_CMD_TYPE_SEND_PARALLEL,cmdid);
	((PROTOCOL_CMD *)cmd_head)->broadcast_num = br_num;
	return ret;
}

int set_pure_broadcast_field(void* cmd_head,void* data,int broadcast_num,uint8_t *broadcast_ids)
{
	if(cmd_head){		
		PROTOCOL_CMD *head = (PROTOCOL_CMD *)cmd_head;

		if(broadcast_num > 0)
			head->broadcast_num = broadcast_num;
		else
			head->broadcast_num = 0;
		
		uint8_t *broadcast_ids_ptr = head->broadcast_ids;
		
		if(data)
			broadcast_ids_ptr = (uint8_t *)data;
		
		for(int i=0;broadcast_ids and i<broadcast_num;i++)
			broadcast_ids_ptr[i] = broadcast_ids[i];
	}
	else if(data){
		for(int i=0;broadcast_ids and i<broadcast_num;i++)
			*((uint8_t *)data+i) = broadcast_ids[i];
	}
	
	return 0;
}

void print_cmd_head(PROTOCOL_CMD *head)
{
	printf("\n");
	printf("magic: %c %c \n",head->szMagic[0],head->szMagic[1]);
	printf("bodylen: %d,sendType: %d,id: %d,br_num: %d \n",head->bodylen,head->cmdSendType,
		head->cmdid,head->broadcast_num);
	
	return;
}

void print_data_char(int br_num,uint8_t *data)
{
	printf("\n");
	printf("ids:");
	for(int i=0;i<br_num;i++){
		printf(" %d",data[i]);
	}
	printf("\n");
	
	printf("body: %s \n",data+round4(br_num));
	
	return;
}

int broadcast_cmd(struct list_head *head,void* cmd_head,void *data)
{
	if(data ==NULL || cmd_head == NULL || head == NULL)
		return 0;
	
	int broadcast_num = GET_PROTOCOL_CMD_BROADCAST_NUM(cmd_head);
	int bodylen	= GET_PROTOCOL_BODY_LEN(cmd_head) - GET_PROTOCOL_BROADCAST_LEN(cmd_head);

	data += GET_PROTOCOL_BROADCAST_LEN(cmd_head);
	set_broadcast_cmd_head(cmd_head,bodylen,-1,0);
	set_pure_broadcast_field(cmd_head,NULL,0,NULL);
	
	//print_cmd_head(cmd_head);
	//print_data_char(0,data);
	
	struct list_head * current_node_list = NULL;
	pub_sub_node_t *current_node = NULL;

	list_for_each(current_node_list,head){
		current_node = list_entry(current_node_list, pub_sub_node_t, node_list);
		for(int i=0;i<broadcast_num;i++){
			if(((PROTOCOL_CMD *)cmd_head)->broadcast_ids[i] == current_node->id){
				int ret =tcp_send_protocol_cmd(current_node->connfd,cmd_head,data);
				if(ret < 0)
					return -current_node->id;	
			}
		}
	}

	return 0;
}

int parser_reserve_cmd(pub_sub_node_t *node_start,pub_sub_node_t *current_node,PROTOCOL_CMD* cmd)
{
	uint8_t buf[2000] = {0};
	int need_send = 0;
	void *sendbuf = NULL;
	switch(GET_PROTOCOL_CMD_ID(cmd)){
		case IH_RESERVE_CMD_ID_GET_SHM_INFO:
		{
			int bodylen = sizeof(SHM_NODE_T)*node_start->shm_node_num;
			set_pure_protocol_head(buf,bodylen,0,IH_RESERVE_CMD_ID_GET_SHM_INFO);
			sendbuf = node_start->shm_node_ptr;
			need_send = 1;
		}
			break;
		case IH_RESERVE_CMD_ID_GET_SHM_NUM:
		{
			set_pure_protocol_head(buf,1,0,IH_RESERVE_CMD_ID_GET_SHM_NUM);
			sendbuf = buf+PROTOCOL_CMD_HEAD_LEN;
			buf[PROTOCOL_CMD_HEAD_LEN] = node_start->shm_node_num;
			need_send = 1;
		}
		default:
			break;
	}

	if(!need_send)
		return 0;

	int ret = tcp_send_protocol_cmd(current_node->connfd,buf,sendbuf);
	
	return ret;
}

int  isShmInfo(PROTOCOL_CMD *cmd_head,uint8_t *data)
{	
	if(cmd_head->cmdid != IH_RESERVE_CMD_ID_GET_SHM_INFO)
		return false;
	
	return true;
}
