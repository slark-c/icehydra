#ifndef _ICEHYDRA_H_
#define _ICEHYDRA_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef EXPORT_IH_LIB
#define IH_EXPORT __attribute__((visibility("default")))
#else
#define IH_EXPORT
#endif

#define ID_ADDR_LEN  8
enum IH_SHM_TYPE
{	
	IH_SHM_POSIX,
	IH_SHM_SYSV,
	IH_SHM_MEMFD,
	IH_SHM_HUGETLBFS,
};

typedef struct
{
	enum IH_SHM_TYPE type;
	int size;

	char name[128];
	void *mem_ptr;
}IH_SHM_T;

typedef struct
{
	void *data;

	int br_num;
	int datalen;

	uint16_t *address_u16;
	char *address_string;

	char *from_address_string;
	uint16_t from_address_u16;
}IH_BROADCAST_CMD_T;


int IH_EXPORT
ih_subscriber_create_connect(IH_BROADCAST_CMD_T*);

int IH_EXPORT
ih_subscriber_close(uint16_t from_address_u16);

int IH_EXPORT
ih_select_recv_ready(IH_BROADCAST_CMD_T *to_send,struct timeval *timeout);

int IH_EXPORT
ih_send_broadcast_data(IH_BROADCAST_CMD_T *cmd);

int IH_EXPORT
ih_recv_data(IH_BROADCAST_CMD_T *to_send);

int IH_EXPORT
ih_create_shm(IH_SHM_T *shm);

int IH_EXPORT
ih_mmap_shm(IH_SHM_T *shm);

int IH_EXPORT
ih_munmap_shm(IH_SHM_T *shm);

int IH_EXPORT
ih_get_shm_infos(void *buffer,int buffersize);

int IH_EXPORT
ih_get_shm_by_name(char *name,IH_BROADCAST_CMD_T *to_send_recv);


int IH_EXPORT
ih_remove(IH_BROADCAST_CMD_T *to_send);

#endif
