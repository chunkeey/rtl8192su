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
