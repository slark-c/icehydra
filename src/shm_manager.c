#include "shm_manager.h"

#include <sys/mman.h>
#include <stdio.h>
#include <pthread.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>

#include "ih_types.h"
#include "icehydra_util.h"

#if IH_HAVE_MEMFDSHM

void* shm_create(icehydra_shm_t *shm)
{
	int fd = memfd_create(shm->filename,0);
	if(fd < 0 )
	{
		printf("%s line = %d , %s \n",__FILE__,__LINE__,strerror(errno));
		return NULL;
	}
	
	int ret = ftruncate(fd,shm->size);
	if(ret < 0)
	{
		close(fd);
		return NULL;
	}
	
	shm->fd = fd;
	return shm_mmap(shm);
}

void *shm_mmap(icehydra_shm_t *shm)
{
	shm->mem_ptr = mmap(NULL,shm->size,PROT_READ|PROT_READ,MAP_SHARED,shm->fd,0);
	if(shm->mem_ptr == MAP_FAILED)
	{
		shm->mem_ptr = NULL;
		return NULL;
	}
	
	return shm->mem_ptr;
}

int  shm_munmap(icehydra_shm_t *shm)
{
	return munmap(shm->mem_ptr,shm->size);
}

#elif IH_HAVE_POSIXSHM

void* shm_create(icehydra_shm_t *shm)
{
	int fd = shm_open(shm->filename,O_CREAT|O_RDWR,0666);
	if(fd < 0 )
	{
		printf("%s line = %d , %s \n",__FILE__,__LINE__,strerror(errno));
		return NULL;
	}
	
	int ret = ftruncate(fd,shm->size);
	if(ret < 0)
	{
		close(fd);
		return NULL;
	}
	
	shm->fd = fd;
	return  shm_mmap(shm);
}

void *shm_mmap(icehydra_shm_t *shm)
{
	shm->mem_ptr = mmap(NULL,shm->size,PROT_READ|PROT_READ,MAP_SHARED,shm->fd,0);
	if(shm->mem_ptr == MAP_FAILED)
	{
		shm->mem_ptr = NULL;
		return NULL;
	}
	
	return shm->mem_ptr;
}

int  shm_munmap(icehydra_shm_t *shm)
{
	return munmap(shm->mem_ptr,shm->size);
}

#elif (IH_HAVE_SYSVSHM)

void *shm_create(icehydra_shm_t *shm)
{
	make_dir(shm->filename);
	int proj_id = shm->proj_id > 0 ? shm->proj_id : 1;
	key_t key  = ftok(shm->filename,proj_id);
	int id = shmget(key,shm->size,IPC_CREAT|0666);	
	shm->mem_ptr = shmat(id,NULL,0);
	if(shm->mem_ptr == (void*)-1 )
	{
		printf("%s line = %d , %s \n",__FILE__,__LINE__,strerror(errno));
		shm->mem_ptr = NULL;
		return NULL;
	}
	
	memset(shm->mem_ptr,0,shm->size);	
	shm->id = id;
	return shm->mem_ptr;
}

void *shm_mmap(icehydra_shm_t *shm)
{	
	int id = shm->id;
	if(id < 0)
	{
		int proj_id = shm->proj_id > 0 ? shm->proj_id : 1;
		key_t key  = ftok(shm->filename,proj_id);
		id = shmget(key,0,0666);		
		shm->id = id;
	}
	
	shm->mem_ptr = shmat(id,NULL,0);
	if(shm->mem_ptr == (void*)-1 )
	{
		printf("%s line = %d , %s \n",__FILE__,__LINE__,strerror(errno));
		shm->mem_ptr = NULL;
		return NULL;
	}

	struct shmid_ds ds;
	shmctl(id,IPC_STAT,&ds);
	shm->size = ds.shm_segsz;
	return shm->mem_ptr;
}

int  shm_munmap(icehydra_shm_t *shm)
{
	return shmdt(shm->mem_ptr);
}

#endif

