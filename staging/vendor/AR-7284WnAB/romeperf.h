/*
* Copyright c                  Realtek Semiconductor Corporation, 2002  
* All rights reserved.
* 
* Program : Performance Profiling Header File for ROME Driver
* Abstract : 
* Author : Yung-Chieh Lo (yjlou@realtek.com.tw)               
* $Id: romeperf.h,v 1.6 2009/08/06 11:41:29 yachang Exp $
*/

#ifndef _ROMEPERF_H_
#define _ROMEPERF_H_

//#include <asm/rtl865x/rtl_types.h>
#include <asm/mipsregs.h>


typedef unsigned long long	uint64;
typedef long long		int64;
typedef unsigned int	uint32;
typedef int			int32;
typedef unsigned short	uint16;
typedef short			int16;
typedef unsigned char	uint8;
typedef char			int8;

#ifndef NULL
#define NULL 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef SUCCESS
#define SUCCESS 	0
#endif
#ifndef FAILED
#define FAILED -1
#endif

#ifdef PKT_PROCESSOR//CONFIG_RTL8672
#include "../../../net/packet_processor/icModel_ringController.h" 
#include "../../../net/packet_processor/icTest_ringController.h"
#endif


#if defined(CONFIG_RTL865X_MODULE_ROMEDRV)
#define	rtl8651_romeperfEnterPoint				module_internal_rtl8651_romeperfEnterPoint
#define	rtl8651_romeperfExitPoint					module_internal_rtl8651_romeperfExitPoint
#endif

struct rtl8651_romeperf_stat_s {
	char *desc;
	uint64 accCycle[4];
	uint64 tempCycle[4];
	uint32 executedNum;
	uint32 hasTempCycle:1; /* true if tempCycle is valid. */
};
typedef struct rtl8651_romeperf_stat_s rtl8651_romeperf_stat_t;


#if !(defined(RTL8192SU)&&defined(CONFIG_RTL8671))
/* for rtl8651_romeperfEnterPoint() and rtl8651_romeperfExitPoint() */
#define ROMEPERF_INDEX_MIN 0
#define ROMEPERF_INDEX_NAPT_ADD 0
#define ROMEPERF_INDEX_NAPT_ADD_1 1
#define ROMEPERF_INDEX_NAPT_ADD_2 2
#define ROMEPERF_INDEX_NAPT_ADD_3 3
#define ROMEPERF_INDEX_NAPT_ADD_4 4
#define ROMEPERF_INDEX_NAPT_ADD_5 5
#define ROMEPERF_INDEX_NAPT_ADD_6 6
#define ROMEPERF_INDEX_NAPT_ADD_7 7
#define ROMEPERF_INDEX_NAPT_ADD_8 8
#define ROMEPERF_INDEX_NAPT_ADD_9 9
#define ROMEPERF_INDEX_NAPT_ADD_10 10
#define ROMEPERF_INDEX_NAPT_ADD_11 11
#define ROMEPERF_INDEX_NAPT_DEL 12
#define ROMEPERF_INDEX_NAPT_FIND_OUTBOUND 13
#define ROMEPERF_INDEX_NAPT_FIND_INBOUND 14
#define ROMEPERF_INDEX_NAPT_UPDATE 15
#define ROMEPERF_INDEX_UNTIL_RXTHREAD 20
#define ROMEPERF_INDEX_RECVLOOP 21
#define ROMEPERF_INDEX_FWDENG_INPUT 22
#define ROMEPERF_INDEX_BEFORE_CRYPTO_ENCAP 23
#define ROMEPERF_INDEX_ENCAP 24
#define ROMEPERF_INDEX_ENCAP_CRYPTO_ENGINE 25
#define ROMEPERF_INDEX_ENCAP_AUTH_ENGINE 26
#define ROMEPERF_INDEX_BEFORE_CRYPTO_DECAP 27
#define ROMEPERF_INDEX_DECAP 28
#define ROMEPERF_INDEX_DECAP_CRYPTO_ENGINE 29
#define ROMEPERF_INDEX_DECAP_AUTH_ENGINE 30
#define ROMEPERF_INDEX_FASTPATH 31
#define ROMEPERF_INDEX_SLOWPATH 32
#define ROMEPERF_INDEX_UNTIL_ACLDB 33
#define ROMEPERF_INDEX_GET_MTU_AND_SOURCE_MAC 34
#define ROMEPERF_INDEX_PPTPL2TP_1 35
#define ROMEPERF_INDEX_PPPOE_ARP_CACHE 36
#define ROMEPERF_INDEX_PPTPL2TP_SEND 37
#define ROMEPERF_INDEX_FRAG 38
#define ROMEPERF_INDEX_FWDENG_SEND 39
#define ROMEPERF_INDEX_EGRESS_ACL 40
#define ROMEPERF_INDEX_PPTPL2TP_ENCAP 41
#define ROMEPERF_INDEX_FROM_PS 42
#define ROMEPERF_INDEX_EXTDEV_SEND 43
#define ROMEPERF_INDEX_FRAG_2ND_HALF 44
#define ROMEPERF_INDEX_TXPKTPOST 45
#define ROMEPERF_INDEX_MBUFPAD 46
#define ROMEPERF_INDEX_TXALIGN 47
#define ROMEPERF_INDEX_ISRTXRECYCLE 48
#define ROMEPERF_INDEX_16 49
#define ROMEPERF_INDEX_17 50
#define ROMEPERF_INDEX_18 51
#define ROMEPERF_INDEX_19 52
#define ROMEPERF_INDEX_20 53
#define ROMEPERF_INDEX_21 54
#define ROMEPERF_INDEX_22 55
#define ROMEPERF_INDEX_23 56
#define ROMEPERF_INDEX_24 57
#define ROMEPERF_INDEX_25 58
#define ROMEPERF_INDEX_FLUSHDCACHE 59
#define ROMEPERF_INDEX_IRAM_1 60
#define ROMEPERF_INDEX_IRAM_2 61
#define ROMEPERF_INDEX_IRAM_3 62
#define ROMEPERF_INDEX_IRAM_4 63
#define ROMEPERF_INDEX_DRAM_1 64
#define ROMEPERF_INDEX_DRAM_2 65
#define ROMEPERF_INDEX_DRAM_3 66
#define ROMEPERF_INDEX_DRAM_4 67
#define ROMEPERF_INDEX_BMP 68
#define ROMEPERF_INDEX_MDCMDIO 69
#define ROMEPERF_INDEX_MAX 70
#else
#define ROMEPERF_INDEX_MIN 0
#define ROMEPERF_INDEX_USB_ISR 0
#define ROMEPERF_INDEX_8187_RX_ISR 1
#define ROMEPERF_INDEX_USB_SUBMIT_URB_RX 2
#define ROMEPERF_INDEX_8187_RX_PROCESS 3
#define ROMEPERF_INDEX_8187_NETIF_RX 4
#define ROMEPERF_INDEX_8187_TX_XMIT 5
#define ROMEPERF_INDEX_8187_TX_XMIT2 6
#define ROMEPERF_INDEX_USB_SUBMIT_URB_TX 7
#define ROMEPERF_INDEX_USB_SUBMIT_URB_TX_SC 8
#define ROMEPERF_INDEX_8187_TX_ISR 9
#define ROMEPERF_INDEX_USB_TEST 10
#define ROMEPERF_INDEX_USB_TEST2 11
#define ROMEPERF_INDEX_USB_TEST3 12
#define ROMEPERF_INDEX_USB_TEST4 13
#define ROMEPERF_INDEX_USB_TEST5 14
#define ROMEPERF_INDEX_MAX 15
#endif



int32 rtl8651_romeperfInit( void );
inline int32 rtl8651_romeperfStart(void);
inline int32 rtl8651_romeperfStop(void);
int32 rtl8651_romeperfReset( void );
int32 rtl8651_romeperfEnterPoint( uint32 index );
int32 rtl8651_romeperfExitPoint( uint32 index );
int32 rtl8651_romeperfDump( int start, int end );
int32 rtl8651_romeperfPause( void );
int32 rtl8651_romeperfResume( void );
int32 rtl8651_romeperfGet( uint64 *pGet );

extern rtl8651_romeperf_stat_t romePerfStat[ROMEPERF_INDEX_MAX];



#endif/* _ROMEPERF_H_ */
