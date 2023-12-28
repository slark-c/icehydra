#ifndef _ICEHYDRA_UTIL_H_
#define _ICEHYDRA_UTIL_H_

#include <stdbool.h>
#include <pthread.h>

#define BIT_GET(value, offset)	({		\
	((value) >> (offset)) & 0x01; \
})

#define BIT_SET(value, offset)	({		\
	(value) = (value) | (1 << (offset)); \
})

#define BIT_CLEAR(value, offset)	({		\
	(value) = (value) & ~(1 << (offset)); \
})

#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))

#define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))

#define __must_be_array(a)  BUILD_BUG_ON_ZERO(__same_type((a), &(a)[0]))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0])+__must_be_array(arr))

#define set_nonblock(fd)  ({		\
	int val = fcntl(fd, F_GETFL); \
	fcntl(fd, F_SETFL,val | O_NONBLOCK); \
})

#define memzero(buf, n)       (void) memset(buf, 0, n)


#define close_s(fd) do{ \
						if(fd > 0) close(fd);\
						fd = -1;\
					}while(0)
					
#define free_s(ptr)  do{ \
						 if(ptr)\
						 	free(ptr);\
						 ptr =NULL;\
					 	}while(0)

#define min(x,y)	({\
						typeof(x) _x = (x);\
        				typeof(y) _y = (y);\
        				(void) (&_x == &_y);\
        				_x < _y ? _x : _y;\
					})

#define max(x,y)	({\
						typeof(x) _x = (x);\
        				typeof(y) _y = (y);\
        				(void) (&_x == &_y);\
        				_x > _y ? _x : _y;\
        			})

#define isStrSame(a,b)     (!strcmp(a,b))
#define isStrSameN(a,b,N)  (!strncmp(a,b,N))
#define strLen(str)        ((!str)?0:strlen(str))
#define isMemSameN(a,b,N)  (!memcmp(a,b,N))

#define round4(x) (4*((x>>2)+!!(0b11&x)))

int  bind_cpu(int cpuid);
int file_read_lock(int fd);
int file_write_lock(int fd);
int file_unlock(int fd);

int icehydra_system(const char *cmd,char *resp);
bool isDirExists(const char *path);
int make_dir(const char *path);
int mutexattr_init(pthread_mutex_t *mutex,pthread_mutexattr_t *mutexattr);
char *name_fix(char *prefix,char *name,char *suffix);
int read_file(char* filename , char **data);
int tcp_send_noblock(int sockfd,void *data,int len);
int tcp_recv_noblock(int sockfd,void *data,int len);

#endif
