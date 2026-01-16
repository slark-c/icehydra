#ifndef _UNIX_SOCKET_H_
#define _UNIX_SOCKET_H_

int create_bind_unix_tcp(const char *path,bool isServer);

int multi_client_conn_thread(void *args,void *(*start_routine) (void *));

int connect_unix_tcp(int sockfd,const char *server_name);

int send_fd_unix_domain(int sockfd , int fd_to_send);
int send_errno_unix_domain(int sockfd , int errcode , const char *msg);
int recv_fd_unix_domain(int sockfd);

int tcp_recv_protocol_cmd(int sockfd,void *cmd_head,void *data);
int tcp_send_protocol_cmd(int sockfd,void* cmd_head,void *data);
int tcp_send_protocol(int sockfd,void* cmd_head,void *cmd_head_data,int cmd_head_data_len,void *data,int datelen);
uint16_t parse_decimal_dot_string_to_hex(const char *input) ;

int wait_all_unix_connet(int sockfd);


#endif
