/*
 *      Headler file of MIPS16 library
 *
 *      $Id: mips16_lib.h,v 1.2 2009/08/06 11:41:29 yachang Exp $
 */

#ifndef _MIPS16_LIB_H
#define _MIPS16_LIB_H

#include <linux/config.h>
#include <net/tcp.h>
//#define rtattr_strcmp(rta, str)	_rtattr_strcmp(rta, str)
//#define atomic_sub_return(i, v)	_atomic_sub_return(i, v)
//#define test_and_clear_bit(nr, addr)	_test_and_clear_bit(nr, addr)
//#define get_fd_set(nr, ufdset, fdset)	_get_fd_set(nr, ufdset, fdset)
//#define __test_and_change_bit(nr, addr)	__test_and_change_bit__(nr, addr)
//#define test_and_set_bit(nr, addr)	_test_and_set_bit(nr, addr)
//#define clear_bit(nr, addr)	_clear_bit(nr, addr)
//#define activate_mm(prev, next)	_activate_mm(prev, next)
//#define switch_mm(prev, next, tsk)	_switch_mm(prev, next, tsk)
//#define get_cycles()	_get_cycles()
//#define __list_cmp_name(i, name)	__list_cmp_name__(i, name)
//#define tcp_v4_check(th, len, saddr, daddr, base)	_tcp_v4_check(th, len, saddr, daddr, base)
//#define tcp_checksum_complete(skb)	_tcp_checksum_complete(skb)
//#define __tcp_checksum_complete(skb) __tcp_checksum_complete__(skb)
//#define ip_compute_csum(buff, len)	_ip_compute_csum(buff, len)
//#define ip_fast_csum(iph, ihl) _ip_fast_csum(iph, ihl)
//#define csum_tcpudp_nofold(saddr, daddr, len, proto, sum)	_csum_tcpudp_nofold(saddr, daddr, len, proto, sum)
//#define csum_tcpudp_magic(saddr, daddr, en, proto, sum)	_csum_tcpudp_magic(saddr, daddr, en, proto, sum)
//#define csum_fold(sum)	_csum_fold(sum)
//#define atomic_dev(i, v)	_atomic_sub(i, v)

#define atomic_add(i, v)	_atomic_add(i, v)
#define atomic_sub(i, v)	_atomic_sub(i, v)
#define strcpy(dest, src)	_strcpy_(dest, src)
#define strncpy(dest, src, count)	_strncpy_(dest, src, count)
#define strcmp(cs, ct)	_strcmp_(cs, ct)
#define strncmp(cs, ct, len)	_strncmp_(cs, ct, len)

// kernel 2.6
#undef set_bit
#undef test_bit
#undef raw_local_irq_disable
#undef raw_local_irq_enable
#undef raw_local_irq_save
#undef raw_local_irq_restore
#undef netif_queue_stopped
#undef netif_wake_queue
#undef tasklet_hi_schedule
#undef tasklet_schedule
#undef netif_start_queue
#undef netif_stop_queue
#undef copy_from_user
#undef __delay
#undef __cpu_to_le16p

#define set_bit(nr, addr)	_set_bit(nr, addr)
#define test_bit(nr, addr)	_test_bit(nr, addr)
#define raw_local_irq_disable	_raw_local_irq_disable
#define raw_local_irq_enable	_raw_local_irq_enable
#define raw_local_irq_save(x) _raw_local_irq_save(&(x)) 
#define raw_local_irq_restore(x) _raw_local_irq_restore(&(x)) 
#define netif_queue_stopped(x) _netif_queue_stopped(x)
#define netif_wake_queue(x) _netif_wake_queue(x)
#define tasklet_hi_schedule(x) _tasklet_hi_schedule(x)
#define tasklet_schedule(x) _tasklet_schedule(x)
#define netif_start_queue(x) _netif_start_queue(x)
#define netif_stop_queue(x) _netif_stop_queue(x)
#define copy_from_user(x,y,z) _copy_from_user(x,y,z)
#define __delay(x)	___delay_(x)
#define cli()	__cli()

#define __cpu_to_le16p(x) ___cpu_to_le16p(x)
//#define cpu_to_le16(x) __cpu_to_le16_(x)
//#define cpu_to_le32(x) __cpu_to_le32_(x)
//#define __fswab32(x) ___fswab32(x)




extern __attribute__((nomips16)) __u16 ___cpu_to_le16p(const __u16 *p);
//extern __attribute__((nomips16)) u16 __cpu_to_le16_(u16 p);
//extern __attribute__((nomips16)) u32 __cpu_to_le32_(u32 p);

extern __attribute__((nomips16)) void _atomic_sub(int i, atomic_t *v);
extern __attribute__((nomips16)) void _atomic_add(int i, atomic_t *v);
extern __attribute__((nomips16)) char *_strcpy_(char *dest, __const__ char *src);
extern __attribute__((nomips16)) char * _strncpy_(char * dest,const char *src,size_t count);
extern __attribute__((nomips16)) int _strcmp_(__const__ char *__cs, __const__ char *__ct);
extern __attribute__((nomips16)) int _strncmp_(__const__ char *__cs, __const__ char *__ct, size_t __count);

// kernel 2.6 
extern __attribute__((nomips16)) void _set_bit(int nr, volatile unsigned long *addr);
extern __attribute__((nomips16)) int _test_bit(int nr, volatile unsigned long *addr);
extern __attribute__((nomips16)) void _raw_local_irq_disable(void);
extern __attribute__((nomips16)) void _raw_local_irq_enable(void);
extern __attribute__((nomips16)) void _raw_local_irq_save(unsigned long *p);
extern __attribute__((nomips16)) void _raw_local_irq_restore(unsigned long *p);
extern __attribute__((nomips16)) int _netif_queue_stopped(const struct net_device *dev);
extern __attribute__((nomips16)) void _netif_wake_queue(struct net_device *dev);
extern __attribute__((nomips16)) void _tasklet_hi_schedule(struct tasklet_struct *t);
extern __attribute__((nomips16)) void _tasklet_schedule(struct tasklet_struct *t);
extern __attribute__((nomips16)) void _netif_start_queue(struct net_device *dev);
extern __attribute__((nomips16)) void _netif_stop_queue(struct net_device *dev);
extern __attribute__((nomips16)) int _copy_from_user(void *dst,void *src,size_t n);
extern __attribute__((nomips16)) void ___delay_(unsigned long n);

unsigned int  swap32(unsigned int *x);
//extern __attribute__((nomips16)) void __spin_lock_irqsave__(spinlock_t *x,unsigned long y);



#endif
