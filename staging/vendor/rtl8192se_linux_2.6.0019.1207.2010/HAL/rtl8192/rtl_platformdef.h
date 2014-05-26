#ifndef __RTL_PLATFORMDEF_LINUX_H_
#define __RTL_PLATFORMDEF_LINUX_H_

#define IN
#define OUT

#define VOID				void
#define PVOID				void *
#define TRUE                true
#define FALSE 				false


typedef bool				BOOLEAN, *PBOOLEAN;

typedef unsigned char		UCHAR, *PUCHAR;
typedef unsigned short		USHORT, *PUSHORT;
typedef short				SHORT;
typedef u32					ULONG, *PULONG;
typedef int					LONG;

typedef long long			LONGLONG;
typedef unsigned long long	ULONGLONG;
typedef unsigned long long  LARGE_INTEGER;

typedef UCHAR				u1Byte,*pu1Byte;
typedef USHORT				u2Byte,*pu2Byte;
typedef u32					u4Byte,*pu4Byte;
typedef ULONGLONG			u8Byte,*pu8Byte;
typedef char				s1Byte,*ps1Byte;
typedef short				s2Byte,*ps2Byte;
typedef int					s4Byte,*ps4Byte;
typedef LONGLONG			s8Byte,*ps8Byte;
typedef ULONGLONG			ULONG64,*PULONG64;

typedef u8				UINT8;
typedef unsigned short			UINT16;
typedef unsigned long			UINT32;
typedef signed char			INT8;
typedef signed short			INT16;
typedef signed long			INT32;

typedef unsigned int			UINT;
typedef signed int			INT;

typedef unsigned long long		UINT64;
typedef signed long long		INT64;

typedef const unsigned char cu8;

typedef	struct net_device	ADAPTER, *PADAPTER;
typedef struct r8192_priv	MGNT_INFO, *PMGNT_INFO; 

typedef unsigned long 		PRT_TIMER;
#endif 

