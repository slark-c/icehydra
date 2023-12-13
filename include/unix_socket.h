#ifndef _UNIX_SOCKET_H_
#define _UNIX_SOCKET_H_

int creat_bind_unix_tcp(const char *name);

int listen_unix_tcp(int sockfd , int backlog);

int accept_unix_tcp(int sockfd,char * cli_name);

int multi_client_conn_thread(void *args,void *(*start_routine) (void *));

int connect_unix_tcp(int sockfd,const char *server_name);

int send_fd_unix_domain(int sockfd , int fd_to_send);
int send_errno_unix_domain(int sockfd , int errcode , const char *msg);
int recv_fd_unix_domain(int sockfd);

int tcp_recv_protocol_cmd(int sockfd,void *cmd_head,void *data);
int tcp_send_protocol_cmd(int sockfd,void* cmd_head,void *data);

int wait_all_unix_connet(int sockfd);


#endif
