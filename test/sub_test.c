#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>

#include "../icehydra.h"

#define strLen(str)        ((!str)?0:strlen(str))
#define memzero(buf, n)       (void) memset(buf, 0, n)

void help(void)
{
	printf(
	
		"    v (show version )\n"
		"    h (show help info)\n"		
		"    D (run in Daemon.)\n"		
		"    s (source id.)\n"
		"    S (send string)\n"
		"    d (dst ip)\n"
		"    f (file conf)\n"
	);
}

static void find_atoi(char *string,uint8_t *ids,uint8_t *ids_num)
{
	if(string == NULL) return ;

	char *p = strtok(string,",");
	int count = 0;
	while(p != NULL)
	{
		ids[count++] = atoi(p);		
		p = strtok(NULL,",");
	}
	*ids_num = count ;
	return ;
}

int main(int argc,char *argv[])	
{
	char name[128] = {0};
	int ret,ch,deamon = 0,needsend = 0;
	uint8_t ids_num=0,ids[128] = {0};
	char str[128] = {0};	
	IH_BROADCAST_CMD_T to_send = {0};
	to_send.br_num = 1;
	while((ch = getopt(argc,argv,"hvDi:s:P:S:d:"))!= EOF)
	{
		switch(ch){
			case 'i':
				find_atoi(optarg,ids,&ids_num);
				break;
			case 's':
				to_send.from_address_string = calloc(1,8);
				strcpy(to_send.from_address_string,optarg);
				break;
			case 'd':
				to_send.address_string = calloc(1,8);
				strcpy(to_send.address_string,optarg);
				break;
			case 'S':
				strcpy(str,optarg);
				needsend = 1;
				break;
			case 'v':
			case 'h':
				help();
				return 0;
				break;
			case 'D':
				deamon = 1;
				break;
			case 'P':
				break;
			default:
				break;
		}
	}

	if(deamon)
		ret = daemon(1,1);


	
	ret = ih_subscriber_create_connect(&to_send);
	if(ret < 0)
		return -1;
	
	to_send.data = calloc(1,1024);
	
	if(!needsend){
		struct timeval timeout;
		while(1) 
		{
			
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;
			if(!ih_select_recv_ready(&to_send,&timeout))
				continue;
			
			memzero(to_send.data, 1024);
			
			ret = ih_recv_data(&to_send);
			if(ret < 0){			
				printf("error recv \n");
				exit(-1);
			}

			printf("%x ,from 0x%x recv : %s ,len %d\n",to_send.address_u16,to_send.from_address_u16,(char*)to_send.data,to_send.datalen);
				
		}

		return 0;
	}

	int count = 0;
	while(1){
		memzero(to_send.data, 1024);
		sprintf(to_send.data,"%s%d",str,count);
		to_send.datalen = strlen(to_send.data);
		printf("send %s,len %d \n",(char*)to_send.data,to_send.datalen);
		ret = ih_send_broadcast_data(&to_send);
		if(ret < 0){			
			printf("error send \n");
			exit(-1);
		}
		count++;

		sleep(2);
	}
	return 0;
}
