#ifndef _CONF_H_
#define _CONF_H_

#include <stdint.h>
#include <pthread.h>
#include "list.h"

#define Kib 1024
#define Mib (Kib*Kib)
#define Gib (Mib*Mib)

#define MAX_PATH 256
#define MAX_P2P_NODE 64
#define DATA_SHORT_LEN 512
#define ADDR_LEN  8
#define MAX_NAME 64

#define MAX_FILE_LEN (1*Kib)
#define MAX_CMD_LEN (32*Kib)
#define IH_MAGIC "IH"
#define ICEHYDRA_ROUTE_NAME "/run/shm/icehydra_router"
#define STRING_LEN(a) ({sizeof(a)-1;})

#define memzero(buf, n)       (void) memset(buf, 0, n)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define memsame(x,y,sz)   (memcmp(x,y,sz) == 0)
#define strsame(x,y)   (memcmp(x,y,STRING_LEN(y)) == 0)

typedef struct
{
    int conn_fd; 
   	struct sockaddr address;
	socklen_t addr_len;
    pthread_t thread_id; 
}SockInfo;

typedef struct
{
    int conn_fd; 
   	uint16_t address_u16;
	uint16_t name_u16;
	
	struct list_head  next;
	
	char address_string[ADDR_LEN]; //"like 10.12 "	
	
	pthread_mutex_t  client_node_mutex;

	char name[MAX_NAME];
		
}ClientNode;

typedef struct
{
   	uint16_t address_u16[MAX_P2P_NODE];		
	//uint8_t  short_data[DATA_SHORT_LEN];	
	void* long_data;

	void* data;
	int datalen;
	uint16_t from_address_u16;
	
	struct list_head  next;
}DataNode;

typedef struct
{
   ClientNode *client_node;
	
   DataNode *data_node;
   
   int node_count;
   int padd;
   
   pthread_mutex_t	route_mutex;

   pthread_mutex_t	data_mutex;
   
   struct list_head  client_node_head;
   
   struct list_head  data_node_head;
   	
}RouteInfo;


typedef enum 
{
	CHUNK_TYPE_ERR,
	CHUNK_TYPE_PUB,
	CHUNK_TYPE_SUB,
}CHUNK_TYES;

typedef struct
{
	char shm_name[MAX_FILE_LEN];
	unsigned int shm_id;
    int shm_size;
}SHM_NODE_T;

typedef	struct 
{	
	char name[MAX_FILE_LEN];
	CHUNK_TYES type;
	int cpu_bind;
	int id;
	int connfd;
	

	SHM_NODE_T *shm_node_ptr;
	int shm_node_num;
	int pub_links_num;
	//int pub_links[0];
}pub_sub_node_t;

#define PROTOCOL_MAGIC "IH"

enum ICEHYDAR_CMD_ID
{
	IH_CMD_NODE_ADDRESS_U16 = 10,
	IH_CMD_NODE_ADDRESS_STR ,
	IH_CMD_NODE_NAME_U16 ,
	IH_CMD_NODE_NAME_STR ,
	
	IH_CMD_P2P_BY_ADDR_U16,
	IH_CMD_P2P_BY_ADDR_STR,
	IH_CMD_P2P_BY_NAME_STR,

	IH_CMD_P2P_DATA,
	/***************/	
	IH_CMD_GET_SHM_INFO,
};
typedef struct
{
	int shm_type;
	int shm_size;
	int shm_num;
	char shm_name[28];
}ICE_INFO_T;


typedef struct 
{
	char 	   szMagic[2];
	uint16_t   bodylen;

	uint8_t    cmdid;
	uint8_t    broadcast_nums;
	uint8_t    padd1;
	uint8_t    pad;

	uint8_t   broadcast_ids[0]; //(broadcast_num+1)/4
} PROTOCOL_CMD;


#define PROTOCOL_CMD_HEAD_LEN  sizeof(PROTOCOL_CMD)
#define GET_PROTOCOL(ptr,member) (((PROTOCOL_CMD*) (ptr))->member)
#define CHECK_MAGIC(ptr) (memcmp(GET_PROTOCOL(ptr,szMagic),IH_MAGIC,STRING_LEN(IH_MAGIC))==0)


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
