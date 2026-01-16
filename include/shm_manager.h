#ifndef _SHM_MANGER_H_
#define _SHM_MANGER_H_

#define _GNU_SOURCE

#include <sys/ipc.h>
#include <sys/shm.h>

enum SHM_TYPE
{	
	SHM_POSIX,
	SHM_SYSV,
	SHM_MEMFD,
	SHM_HUGEPAGESTLBFS,
};
	
typedef struct
{
	enum SHM_TYPE type;
	int size;

	int proj_id; //SYSVSHM ftok
	int id;

	int fd;
	
	char *filename;
	void *mem_ptr;
}icehydra_shm_t;

void *shm_create(icehydra_shm_t*);

void *shm_mmap(icehydra_shm_t*);

int  shm_munmap(icehydra_shm_t*);
char *name_fix(char *prefix,char *name,char *suffix);
int make_dir(const char *path);


#endif
