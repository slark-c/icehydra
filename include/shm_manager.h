#ifndef _SHM_MANGER_H_
#define _SHM_MANGER_H_


#define SHM_BLOCK 64 

typedef struct
{
	int size;
	int proj_id; //SYSVSHM ftok
	
	union{
		int fd;
		int id;
	};
	
	char *filename;
	void *mem_ptr;
}icehydra_shm_t;


//#define  IH_HAVE_MEMFDSHM 0
//#define  IH_HAVE_POSIXSHM 0
//#define  IH_HAVE_SYSVSHM  1

#if IH_HAVE_MEMFDSHM

#define _GNU_SOURCE

#elif  IH_HAVE_SYSVSHM

#include <sys/ipc.h>
#include <sys/shm.h>

#endif

void *shm_create(icehydra_shm_t*);

void *shm_mmap(icehydra_shm_t*);

int  shm_munmap(icehydra_shm_t*);


#endif
