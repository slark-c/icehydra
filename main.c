#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdlib.h>
#include <argp.h>
#include <stdbool.h>
#include <assert.h>

#include "icehydra_util.h"
#include "icehydra_cmd.h"
#include "conf.h"
#include "unix_socket.h"


struct arguments
{
  char *version;                   
  char *date;      
  char  file[256]; 
  int daemon;
  int head;
};

struct unix_client_t
{
	int connfd;

	char name[64];

	struct list_head un_list;
};

static int parse_opt (int key, char *arg, struct argp_state *state){

	struct arguments *arguments = state->input;
	
	switch (key){
	    case 'v': 
			printf ("Version : %s \n",arguments->version);
			//printf ("Date    : %s \n",arguments->date); 
			break;
		case 'D':
			arguments->daemon = true;
			break;
		case 'f':
			if(!access(arg, F_OK | R_OK))
				strcpy(arguments->file,arg);
			break;
		case 'H':
			arguments->head = atoi(arg);
			break;
		case 'h':
			break;
		case ARGP_KEY_ARG:
			 //if (state->arg_num >= 2)
			 //  argp_usage (state);		
			 //arguments->args[state->arg_num] = arg;		
			 break;
		 case ARGP_KEY_END:
			 //if (state->arg_num < 2)
			  // argp_usage (state);
			 break;
		default:
			return ARGP_ERR_UNKNOWN;
			break;
	}

 	return 0;
}

static struct argp_option options[] = {
  {"version",  'v', 0,      0,  "Produce verbose output" },
  {"Daemon",   'D', 0,      0,  "Run in Daemon" },
  //{"help",     'h', 0, 0,   " Give this help list" },
  {"file",     'f', "FILE", 0,"Config json file to input" },  
  //{"head",     'H', "NUM", 0,"Max num of Heads" },
  { 0 }
};
  
static char doc[] ="icehydra -- a mulit-process mamager program \v";

static struct argp argp = {options,parse_opt,0,doc};

struct arguments arguments = {
		.version = "0.4.2",
};

#if 0
void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
	char buf[100] = {0};
	tcp_recv_noblock(watcher->fd,buf,10);

	printf("buf %s \n",buf);
}
static void un_accept_cb(struct ev_loop* loop, ev_io  *w, int revents) {
	
	int connfd = -1;
	char un[128] = {0};
	memset(un,0,sizeof(un));
	connfd = accept_unix_tcp(w->fd,un);

	if(connfd > 0)
	{
		printf("new conn accepted %d \n",connfd);
		set_nonblock(connfd);

		struct ev_io *w_client = (struct ev_io*) malloc (sizeof(struct ev_io));
	    ev_io_init(w_client, read_cb, connfd, EV_READ);
	    ev_io_start(loop, w_client);
	}
}
#endif
#define remove_node() 	({	close_s(current_node->connfd);	\
							link_count -= 1;\
						})
int main(int argc ,char *argv[])
{ 	
	int ret;
	struct timeval timeout;
	ret = argp_parse (&argp, argc, argv, 0, 0, &arguments);

	if(!strlen(arguments.file)){
		//fprintf(stderr,"[Error] : Config File not Found \n");
		return -1;
	}
	
	if(arguments.daemon)
		ret = daemon(1,0);
	
	pub_sub_node_t *node_start = get_node_start(arguments.file,"id:0");
	assert(node_start != NULL);
	
	bind_cpu(node_start->cpu_bind);
	
	char *un_server_name = name_fix(NAME_PREFIX,node_start->name,SOCK_NAME_SUFFIX);	
	int sockfd = creat_bind_unix_tcp(un_server_name);
	assert(sockfd > 0);
	listen_unix_tcp(sockfd,32);
	free(un_server_name);
	
	uint8_t *rbuf  = calloc(1,MAX_CMD_LEN);
	assert(rbuf != NULL);

	struct list_head * current_node_list = NULL;
	pub_sub_node_t *current_node = NULL;
	
	char un[MAX_PATH] = {0};
	int connfd = -1,maxfd = -1,max_conn_fd = -1,link_count=0;
	fd_set rset;
	while(1){

		FD_ZERO(&rset);
		FD_SET(sockfd,&rset);
		list_for_each(current_node_list,&node_start->node_list){
			current_node = list_entry(current_node_list, pub_sub_node_t, node_list);
			max_conn_fd = max(current_node->connfd,max_conn_fd);
			if(current_node->connfd > 0)
				FD_SET(current_node->connfd,&rset);
		}
		
		maxfd = max(sockfd,max_conn_fd);
	
		timeout.tv_sec  = 1;
		timeout.tv_usec = 0;
		ret = select(maxfd + 1, &rset, NULL, NULL, &timeout);
		if(ret <= 0)
			continue;

		if(FD_ISSET(sockfd, &rset)){
			memzero(un, MAX_PATH);
			connfd = accept_unix_tcp(sockfd,un);
			if(connfd < 0) 
				continue;
			
		#ifndef NDEBUG
			printf("client connect %s \n",un);
		#endif
		
			list_for_each(current_node_list,&node_start->node_list){
				current_node = list_entry(current_node_list, pub_sub_node_t, node_list);
				if(isStrSameN(current_node->name,un+NAME_PREFIX_LEN,strLen(current_node->name))){
					current_node->connfd = connfd;
					link_count++;
				}
				else{
					close_s(connfd);
				}
			}
			continue;
		}

		list_for_each(current_node_list,&node_start->node_list){
			current_node = list_entry(current_node_list, pub_sub_node_t, node_list);
			if(current_node->connfd < 0)
				continue;

			if(!FD_ISSET(current_node->connfd,&rset))
				continue;
			
			memzero(rbuf,MAX_CMD_LEN);
						
			ret = tcp_recv_protocol_cmd(current_node->connfd,rbuf,rbuf+PROTOCOL_CMD_HEAD_LEN);
			if(ret < 0){
				remove_node();
				continue;
			}
		#ifndef NDEBUG
			print_cmd_head((PROTOCOL_CMD*)rbuf);
			print_data_char(GET_PROTOCOL_CMD_BROADCAST_NUM(rbuf),rbuf+PROTOCOL_CMD_HEAD_LEN);
		#endif
		
			if(GET_PROTOCOL_CMD_TYPE(rbuf) == IH_CMD_TYPE_SEND_RESERVE){
				parser_reserve_cmd(node_start,current_node,(PROTOCOL_CMD *)rbuf);
				continue;
			}
			
			if(GET_PROTOCOL_CMD_TYPE(rbuf) == IH_CMD_TYPE_SEND_SERIAL)
				continue;			
			if(GET_PROTOCOL_CMD_TYPE(rbuf) == IH_CMD_TYPE_SEND_PARALLEL);

			ret = broadcast_cmd(&node_start->node_list,rbuf,rbuf+PROTOCOL_CMD_HEAD_LEN);
			if(ret < 0){
				pub_sub_node_t *errnode = find_node(&node_start->node_list,-ret);
				if(errnode){
					close_s(errnode->connfd);
				}
			}	
		}
	}

    return 0;
}
