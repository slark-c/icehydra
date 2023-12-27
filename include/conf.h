#ifndef _CONF_H_
#define _CONF_H_

#include "list.h"
#include "ih_types.h"

typedef enum 
{
	CHUNK_TYPE_ERR,
	CHUNK_TYPE_PUB,
	CHUNK_TYPE_SUB,
}CHUNK_TYES;

typedef struct
{
	char shm_name[MAX_FILE_NAME];
	unsigned int shm_id;
    int shm_size;
}SHM_NODE_T;

typedef	struct 
{	
	char name[MAX_FILE_NAME];
	//char sock_name[MAX_PATH];
	CHUNK_TYES type;
	int cpu_bind;
	int id;
	int connfd;
	
	struct list_head node_list; 

	SHM_NODE_T *shm_node_ptr;
	int shm_node_num;
	int pub_links_num;
	//int pub_links[0];
}pub_sub_node_t;

#define PROTOCOL_MAGIC "IH"

enum ICEHYDAR_CMD_SEND_TYPE
{
	IH_CMD_TYPE_SEND_RESERVE = 0,
	IH_CMD_TYPE_SEND_PARALLEL = 1,
	IH_CMD_TYPE_SEND_SERIAL = 2,
};


typedef struct 
{
	char 	   szMagic[2];
	uint16_t   bodylen;

	uint8_t    cmdSendType;
	uint8_t    cmdid;
	uint8_t    broadcast_num;
	uint8_t    pad;

	uint8_t   broadcast_ids[0]; //(broadcast_num+1)/4
} PROTOCOL_CMD;

#define PROTOCOL_CMD_HEAD_LEN  sizeof(PROTOCOL_CMD)
#define GET_PROTOCOL_BODY_LEN(ptr) (((PROTOCOL_CMD*) (ptr))->bodylen)
#define GET_PROTOCOL_CMD_TYPE(ptr) (((PROTOCOL_CMD*) (ptr))->cmdSendType)
#define GET_PROTOCOL_CMD_ID(ptr) (((PROTOCOL_CMD*) (ptr))->cmdid)
#define GET_PROTOCOL_CMD_BROADCAST_NUM(ptr) (((PROTOCOL_CMD*) (ptr))->broadcast_num)
#define GET_PROTOCOL_BROADCAST_LEN(ptr) (4*(GET_PROTOCOL_CMD_BROADCAST_NUM(ptr)/4+!!(0b11&GET_PROTOCOL_CMD_BROADCAST_NUM(ptr))))
#define GET_PROTOCOL_CMD_MSG_BODY(head,body) (((void*)body)+GET_PROTOCOL_BROADCAST_LEN(head))
#define GET_PROTOCOL_LEN(ptr) (GET_PROTOCOL_BODY_LEN(ptr)+PROTOCOL_CMD_HEAD_LEN)


#define NAME_PREFIX "/run/shm/"
#define SOCK_NAME_SUFFIX "-sock"
#define NAME_PREFIX_LEN (sizeof(NAME_PREFIX)-1)
#define SOCK_NAME_SUFFIX_LEN (sizeof(SOCK_NAME_SUFFIX)-1)

pub_sub_node_t *get_node_start(char *file,char *key);
int set_pure_protocol_head(void* protocol,int bodylen,int cmdSendType,int cmdid);
int set_broadcast_cmd_head(void* cmd_head,int bodylen,int cmdid,int br_num);

pub_sub_node_t *find_node_by_id(struct list_head *head,int id);
pub_sub_node_t *find_node_by_name(struct list_head *head,char *name);

#define find_node(x,y) _Generic((y),int:find_node_by_id,char *:find_node_by_name)(x,y)
int broadcast_cmd(struct list_head *head,void* cmd_head,void *data);
int set_pure_broadcast_field(void* cmd_head,void* data,int broadcast_num,uint8_t *broadcast_ids);
void print_cmd_head(PROTOCOL_CMD *head);
void print_data_char(int br_num,uint8_t *data);
int parser_reserve_cmd(pub_sub_node_t *node_start,pub_sub_node_t *current_node,PROTOCOL_CMD* cmd);
int  isShmInfo(PROTOCOL_CMD *cmd_head,uint8_t *data);

#endif
