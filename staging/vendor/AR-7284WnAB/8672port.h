#ifndef PORT_H_
#define PORT_H_

#include <linux/spinlock.h>

#define UINT8	unsigned char
#define UINT16	unsigned short
#define UINT32	unsigned int

#define INT8 	char

#define DRV_RELDATE_SUB	"2009-10-21"

extern spinlock_t gdma_lock;
extern void gdma_memcpy(UINT32* d, UINT32* s, UINT32 len);
extern struct rtl8190_priv *global_rtl8190_priv;
extern void usb_poll_func(unsigned long task_priv);


#endif
