#ifndef _IH_TYPES_H_
#define _IH_TYPES_H_

#include <stdint.h>

#define Kib 1024
#define Mib (Kib*Kib)
#define Gib (Mib*Mib)

#define MAX_SHELL_CMD  256
#define MAX_PATH 128
#define MAX_CONNS 64

#define MAX_FILE_NAME 32
#define MAX_CMD_LEN (4*Kib)

enum IH_TYPES
{
	IH_CHAR=1<<0,
	IH_UCHAR=1<<1,
	IH_SCHAR=1<<2,
	IH_INT=1<<3,
	IH_UINT=1<<4,
	IH_SHORT=1<<5,
	IH_USHORT=1<<6,
	IH_LONG=1<<7,
	IH_ULONGLONG=1<<8,
	IH_STRING=1<<9,
	IH_BOOL=1<<10,
	IH_ARRAY=1<<11,
	IH_POINTER=1<<12,
	IH_FUNCTION=1<<13,
};

#endif
