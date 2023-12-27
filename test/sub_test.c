#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>

#include "icehydra.h"

#define strLen(str)        ((!str)?0:strlen(str))
#define memzero(buf, n)       (void) memset(buf, 0, n)

void help(void)
{
	printf(
	
		"    v (show version )\n"
		"    h (show help info)\n"		
		"    D (run in Daemon.)\n"		
		"    s (need send msg string.)\n"
		"    S (the name of Subscriber)\n"
		"    P (the name of Publisher)\n"
		"    i (broadcast ids. -i 11,22,33 )\n"
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
	char name[128] = {0},pub_name[128] = {0};
	int ret,ch,deamon = 0,needsend = 0;
	uint8_t ids_num=0,ids[128] = {0};
	char str[128] = {0};
	while((ch = getopt(argc,argv,"hvDi:s:P:S:"))!= EOF)
	{
		switch(ch){
			case 'i':
				find_atoi(optarg,ids,&ids_num);
				break;
			case 'S':
				strcpy(name,optarg);
				break;
			case 'v':
			case 'h':
				help();
				return 0;
				break;
			case 'D':
				deamon = 1;
				break;
			case 's':
				needsend = 1;
				strcpy(str,optarg);
				break;
			case 'P':
				strcpy(pub_name,optarg);
				break;
			default:
				break;
		}
	}

	if(deamon)
		ret = daemon(1,1);
	assert(strLen(name));
	
	ret = ih_subscriber_create_connect(name,pub_name);
	if(ret < 0)
		return -1;
	
	int datalen = strlen(str);

	ih_wait_other_process();

	if(needsend){
		
		IH_BROADCAST_CMD_T cmdtest = {
						.br_ids = ids,
						.br_num = ids_num,

						.data = str,
						.datalen = datalen,
					
		};
						
		ret = ih_send_broadcast_data(&cmdtest);	
		if(ret < 0){
			printf("client send %d bytes %s \n",datalen,ret?"Fail":"SUCCESS");
			return -1;
		}
	}

	ret = ih_send_getShmIDs_cmd();
	if(ret <0)
		return -1;	
	
	uint8_t rbuf[128] = {0};
	int recvlen;
	struct timeval timeout;
	while(1) 
	{
		
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if(!ih_select_recv_ready(&timeout))
			continue;
		
		memzero(rbuf, sizeof(rbuf));
		
		ret = ih_recv_data(rbuf,&recvlen);
		if(ret < 0){			
			printf("error recv \n");
			exit(-1);
		}

		if(ih_recvIsShmIDs(ret)){
			void *shm_ptr = NULL;
			int shm_size = 0;

			for(int i=0;i<4;i++){
				char shm_name[16] = {0};
				sprintf(shm_name,"testname%d",i);
				shm_size = ih_get_shm_by_name(rbuf,shm_name,&shm_ptr);
				//if(shm_size < 0)
				//	break;
				printf("%d size %d %p \n",i,shm_size,shm_ptr);
			}
			continue;
		}

		printf("%s client recv : %s \n",name,rbuf);
			
	}
	return 0;
}
