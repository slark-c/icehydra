#ifndef _ICEHYDRA_H_
#define _ICEHYDRA_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef EXPORT_IH_LIB
#define IH_EXPORT __attribute__((visibility("default")))
#else
#define IH_EXPORT
#endif

typedef struct
{
	void *data;
	uint8_t *br_ids;	
	int br_num;
	int datalen;
}IH_BROADCAST_CMD_T;

int IH_EXPORT
ih_subscriber_create_connect(char *subscriber_name,char *publisher_name);

bool IH_EXPORT
ih_select_recv_ready(struct timeval *timeout);

int IH_EXPORT
ih_send_broadcast_data(IH_BROADCAST_CMD_T *cmd);

int IH_EXPORT
ih_recv_data(void *data,int *datalen);

int IH_EXPORT
ih_get_shm_infos(void *buffer,int buffersize);

int IH_EXPORT
ih_get_shm_by_name(void *infobuf,char *shm_name,void **shm_ptr);

#endif
