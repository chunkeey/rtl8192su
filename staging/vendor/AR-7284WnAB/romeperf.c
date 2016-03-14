/*
* Copyright c                  Realtek Semiconductor Corporation, 2002  
* All rights reserved.
* 
* Program : Performance Profiling for ROME Driver
* Abstract : 
* Author : Yung-Chieh Lo (yjlou@realtek.com.tw)               
* $Id: romeperf.c,v 1.6 2009/08/06 11:41:29 yachang Exp $
*/

#include "8190n_cfg.h"

#if defined(PERF_DUMP) || defined(CP3)

#include "romeperf.h"
//#include <asm/rtl865x/rtl_glue.h>
#define KERNEL_SYSeALLS 
#include <asm/unistd.h>
#include <asm/processor.h>
#include <asm/uaccess.h>
#include <asm/mipsregs.h>
#ifdef CONFIG_WIRELESS_LAN_MODULE
#define __IRAM
#else
#define __IRAM		__attribute__ ((section(".iram-gen")))
#endif
#define rtlglue_malloc(size)	kmalloc(size, 0x1f0)
#define rtlglue_free(p)	kfree(p)
#define rtlglue_printf printk


enum CP3_COUNTER
{
	CP3CNT_CYCLES = 0,
	CP3CNT_NEW_INST_FECTH,
	CP3CNT_NEW_INST_FETCH_CACHE_MISS,
	CP3CNT_NEW_INST_MISS_BUSY_CYCLE,
	CP3CNT_DATA_STORE_INST,
	CP3CNT_DATA_LOAD_INST,
	CP3CNT_DATA_LOAD_OR_STORE_INST,
	CP3CNT_EXACT_RETIRED_INST,
	CP3CNT_RETIRED_INST_FOR_PIPE_A,
	CP3CNT_RETIRED_INST_FOR_PIPE_B,
	CP3CNT_DATA_LOAD_OR_STORE_CACHE_MISS,
	CP3CNT_DATA_LOAD_OR_STORE_MISS_BUSY_CYCLE,
	CP3CNT_RESERVED12,
	CP3CNT_RESERVED13,
	CP3CNT_RESERVED14,
	CP3CNT_RESERVED15,
};

/* Local variables */
static uint64 tempVariable64;
static uint32 tempVariable32;
static uint64 currCnt[4];

/* Global variables */
#ifdef CONFIG_WIRELESS_LAN_MODULE
static uint64 cnt1, cnt2;
static rtl8651_romeperf_stat_t romePerfStat[ROMEPERF_INDEX_MAX];
static uint32 rtl8651_romeperf_inited = 0;
static uint32 rtl8651_romeperf_enable = TRUE;
#else
uint64 cnt1, cnt2;
rtl8651_romeperf_stat_t romePerfStat[ROMEPERF_INDEX_MAX];
uint32 rtl8651_romeperf_inited = 0;
uint32 rtl8651_romeperf_enable = TRUE;
#endif

#if !(defined(RTL8192SU)&&defined(CONFIG_RTL8671))
__IRAM 
#else
__IRAM_IN_8672
#endif
void CP3_COUNTER0_INIT( void )
{
__asm__ __volatile__ \
("  ;\
	mfc0	$8, $12			;\
	la		$9, 0x80000000	;\
	or		$8, $9			;\
	mtc0	$8, $12			;\
");
}

#if !(defined(RTL8192SU)&&defined(CONFIG_RTL8671))
__IRAM 
#else
__IRAM_IN_8672
#endif 
uint32 CP3_COUNTER0_IS_INITED( void )
{
__asm__ __volatile__ \
("  ;\
	mfc0	$8, $12			;\
	la		$9, tempVariable32;\
	sw		$8, 0($9)		;\
");
	return tempVariable32;
}

#if !(defined(RTL8192SU)&&defined(CONFIG_RTL8671))
__IRAM 
#else
__IRAM_IN_8672
#endif
void CP3_COUNTER0_START( void )
{
#if 1/* Inst */
	tempVariable32 = /* Counter0 */((0x10|CP3CNT_CYCLES)<< 0) |
	                 /* Counter1 */((0x10|CP3CNT_NEW_INST_FECTH)<< 8) |
	                 /* Counter2 */((0x10|CP3CNT_NEW_INST_FETCH_CACHE_MISS)<<16) |
	                 /* Counter3 */((0x10|CP3CNT_NEW_INST_MISS_BUSY_CYCLE)<<24);
#elif 0 /* Data (LOAD+STORE) */
	tempVariable32 = /* Counter0 */((0x10|CP3CNT_CYCLES)<< 0) |
	                 /* Counter1 */((0x10|CP3CNT_DATA_LOAD_OR_STORE_INST)<< 8) |
	                 /* Counter2 */((0x10|CP3CNT_DATA_LOAD_OR_STORE_CACHE_MISS)<<16) |
	                 /* Counter3 */((0x10|CP3CNT_DATA_LOAD_OR_STORE_MISS_BUSY_CYCLE)<<24);
#elif 0 /* Data (STORE) */
	tempVariable32 = /* Counter0 */((0x10|CP3CNT_DATA_LOAD_INST)<< 0) |
	                 /* Counter1 */((0x10|CP3CNT_DATA_STORE_INST)<< 8) |
	                 /* Counter2 */((0x10|CP3CNT_DATA_LOAD_OR_STORE_CACHE_MISS)<<16) |
	                 /* Counter3 */((0x10|CP3CNT_DATA_LOAD_OR_STORE_MISS_BUSY_CYCLE)<<24);
#elif 0
	tempVariable32 = /* Counter0 */((0x10|CP3CNT_CYCLES)<< 0) |
	                 /* Counter1 */((0x10|CP3CNT_NEW_INST_FECTH)<< 8) |
	                 /* Counter2 */((0x10|CP3CNT_DATA_LOAD_OR_STORE_INST)<<16) |
	                 /* Counter3 */((0x10|CP3CNT_DATA_LOAD_OR_STORE_MISS_BUSY_CYCLE)<<24);
#else
#error
#endif

__asm__ __volatile__ \
("  ;\
	la		$8, tempVariable32	;\
	lw		$8, 0($8)			;\
	ctc3 	$8, $0				;\
");
}

#if !(defined(RTL8192SU)&&defined(CONFIG_RTL8671))
__IRAM 
#else
__IRAM_IN_8672
#endif
void CP3_COUNTER0_STOP( void )
{
__asm__ __volatile__ \
("	;\
	ctc3 	$0, $0			;\
");
}

#if !(defined(RTL8192SU)&&defined(CONFIG_RTL8671))
__IRAM 
#else
__IRAM_IN_8672
#endif
uint64 CP3_COUNTER0_GET( void )
{
__asm__ __volatile__ \
("	;\
	la		$8, tempVariable64;\
	mfc3	$9, $9			;\
	sw		$9, 0($8)		;\
	mfc3	$9, $8			;\
	sw		$9, 4($8)		;\
");
	return tempVariable64;
}

#if !(defined(RTL8192SU)&&defined(CONFIG_RTL8671))
__IRAM 
#else
__IRAM_IN_8672
#endif
void CP3_COUNTER0_GET_ALL( void )
{
__asm__ __volatile__ \
("	;\
	la		$4, currCnt		;\
	mfc3	$9, $9			;\
	sw		$9, 0x00($4)	;\
	mfc3	$9, $8			;\
	sw		$9, 0x04($4)	;\
	mfc3	$9, $11			;\
	sw		$9, 0x08($4)	;\
	mfc3	$9, $10			;\
	sw		$9, 0x0C($4)	;\
	mfc3	$9, $13			;\
	sw		$9, 0x10($4)	;\
	mfc3	$9, $12			;\
	sw		$9, 0x14($4)	;\
	mfc3	$9, $15			;\
	sw		$9, 0x18($4)	;\
	mfc3	$9, $14			;\
	sw		$9, 0x1C($4)	;\
");
}

int32 rtl8651_romeperfInit()
{
	CP3_COUNTER0_INIT();
	CP3_COUNTER0_START();

	rtl8651_romeperf_inited = TRUE;
	rtl8651_romeperf_enable = TRUE;
	memset( &romePerfStat, 0, sizeof( romePerfStat ) );
#if !(defined(RTL8192SU)&&defined(CONFIG_RTL8671))
	romePerfStat[ROMEPERF_INDEX_NAPT_ADD].desc = "NAPT add_all";
	romePerfStat[ROMEPERF_INDEX_NAPT_ADD_1].desc = "NAPT add_checkIntIP";
	romePerfStat[ROMEPERF_INDEX_NAPT_ADD_2].desc = "NAPT add_localServer";
	romePerfStat[ROMEPERF_INDEX_NAPT_ADD_3].desc = "NAPT add_checkExtIp";
	romePerfStat[ROMEPERF_INDEX_NAPT_ADD_4].desc = "NAPT add_dupCheck1";
	romePerfStat[ROMEPERF_INDEX_NAPT_ADD_5].desc = "NAPT add_dupCheck2";
	romePerfStat[ROMEPERF_INDEX_NAPT_ADD_6].desc = "NAPT add_bPortReused";
	romePerfStat[ROMEPERF_INDEX_NAPT_ADD_7].desc = "NAPT add_routeCache";
	romePerfStat[ROMEPERF_INDEX_NAPT_ADD_8].desc = "NAPT add_tooManyConn";
	romePerfStat[ROMEPERF_INDEX_NAPT_ADD_9].desc = "NAPT add_initConn";
	romePerfStat[ROMEPERF_INDEX_NAPT_ADD_10].desc = "NAPT add_decisionFlo";
	romePerfStat[ROMEPERF_INDEX_NAPT_ADD_11].desc = "NAPT add_ambiguous";
	romePerfStat[ROMEPERF_INDEX_NAPT_DEL].desc = "NAPT del";
	romePerfStat[ROMEPERF_INDEX_NAPT_FIND_OUTBOUND].desc = "NATP outbound";
	romePerfStat[ROMEPERF_INDEX_NAPT_FIND_INBOUND].desc = "NAPT inbound";
	romePerfStat[ROMEPERF_INDEX_NAPT_UPDATE].desc = "NAPT update";
	romePerfStat[ROMEPERF_INDEX_UNTIL_RXTHREAD].desc = "IntDispatch-RxThread";
	romePerfStat[ROMEPERF_INDEX_RECVLOOP].desc = "RecvLoop-FwdInput";
	romePerfStat[ROMEPERF_INDEX_FWDENG_INPUT].desc = "FwdEng_Input()";
	romePerfStat[ROMEPERF_INDEX_BEFORE_CRYPTO_ENCAP].desc = "FwdInput-Crypto(En)";
	romePerfStat[ROMEPERF_INDEX_ENCAP].desc = "IPSEC Encap";
	romePerfStat[ROMEPERF_INDEX_ENCAP_CRYPTO_ENGINE].desc = "Encap Crypto";
	romePerfStat[ROMEPERF_INDEX_ENCAP_AUTH_ENGINE].desc = "Encap Authtication";
	romePerfStat[ROMEPERF_INDEX_BEFORE_CRYPTO_DECAP].desc = "FwdInput-Crypto(De)";
	romePerfStat[ROMEPERF_INDEX_DECAP].desc = "IPSEC Decap";
	romePerfStat[ROMEPERF_INDEX_DECAP_CRYPTO_ENGINE].desc = "Decap Crypto";
	romePerfStat[ROMEPERF_INDEX_DECAP_AUTH_ENGINE].desc = "Decap Authtication";
	romePerfStat[ROMEPERF_INDEX_FASTPATH].desc = "Fast Path";
	romePerfStat[ROMEPERF_INDEX_SLOWPATH].desc = "Slow Path";
	romePerfStat[ROMEPERF_INDEX_FWDENG_SEND].desc = "FwdEngSend()";
	romePerfStat[ROMEPERF_INDEX_UNTIL_ACLDB].desc = "FwdInput() Until ACLDB";
	romePerfStat[ROMEPERF_INDEX_GET_MTU_AND_SOURCE_MAC].desc = "L3Route_MTU_srcMAC";
	romePerfStat[ROMEPERF_INDEX_PPTPL2TP_1].desc = "L3Route_PPTPL2TP_1";
	romePerfStat[ROMEPERF_INDEX_PPPOE_ARP_CACHE].desc = "L3Route_PPPoE_ArpCache";
	romePerfStat[ROMEPERF_INDEX_PPTPL2TP_SEND].desc = "L3Route_PptpL2tpSend()";
	romePerfStat[ROMEPERF_INDEX_FRAG].desc = "L3Route_Fragment";
	romePerfStat[ROMEPERF_INDEX_EGRESS_ACL].desc = "FwdSend_EgressACL";
	romePerfStat[ROMEPERF_INDEX_PPTPL2TP_ENCAP].desc = "FwdSend_PPTP/L2TP_Encap";
	romePerfStat[ROMEPERF_INDEX_FROM_PS].desc = "FwdSend_FromPS";
	romePerfStat[ROMEPERF_INDEX_EXTDEV_SEND].desc = "FwdSend_ExtDevSend()";
	romePerfStat[ROMEPERF_INDEX_FRAG_2ND_HALF].desc = "FwdSend_Frag_2ndHalf()";
	romePerfStat[ROMEPERF_INDEX_TXPKTPOST].desc = "rtl8651_txPktPostProcessing()";
	romePerfStat[ROMEPERF_INDEX_MBUFPAD].desc = "mBuf_padding()";
	romePerfStat[ROMEPERF_INDEX_TXALIGN].desc = "_swNic_txAlign";
	romePerfStat[ROMEPERF_INDEX_ISRTXRECYCLE].desc = "_swNic_isrTxRecycle";
	romePerfStat[ROMEPERF_INDEX_16].desc = "FwdEng_temp_16";
	romePerfStat[ROMEPERF_INDEX_17].desc = "FwdEng_temp_17";
	romePerfStat[ROMEPERF_INDEX_18].desc = "FwdEng_temp_18";
	romePerfStat[ROMEPERF_INDEX_19].desc = "FwdEng_temp_19";
	romePerfStat[ROMEPERF_INDEX_20].desc = "FwdEng_temp_20";
	romePerfStat[ROMEPERF_INDEX_21].desc = "FwdEng_temp_21";
	romePerfStat[ROMEPERF_INDEX_22].desc = "FwdEng_temp_22";
	romePerfStat[ROMEPERF_INDEX_23].desc = "FwdEng_temp_23";
	romePerfStat[ROMEPERF_INDEX_24].desc = "FwdEng_temp_24";
	romePerfStat[ROMEPERF_INDEX_25].desc = "FwdEng_temp_25";
	romePerfStat[ROMEPERF_INDEX_FLUSHDCACHE].desc = "rtlglue_flushDCache";
	romePerfStat[ROMEPERF_INDEX_IRAM_1].desc = "IRAM Cached within IRAM";
	romePerfStat[ROMEPERF_INDEX_IRAM_2].desc = "IRAM Uncached within IRAM";
	romePerfStat[ROMEPERF_INDEX_IRAM_3].desc = "test ICACHE  (1024*100)";
	romePerfStat[ROMEPERF_INDEX_IRAM_4].desc = "test Uncached (1024*10)";
	romePerfStat[ROMEPERF_INDEX_DRAM_1].desc = "DRAM Cached within DRAM";
	romePerfStat[ROMEPERF_INDEX_DRAM_2].desc = "DRAM Uncached within DRAM";
	romePerfStat[ROMEPERF_INDEX_DRAM_3].desc = "test DCACHE  (1024*100)";
	romePerfStat[ROMEPERF_INDEX_DRAM_4].desc = "test Uncached (1024*10)";
	romePerfStat[ROMEPERF_INDEX_BMP].desc = "KMP Algorithm";
      romePerfStat[ROMEPERF_INDEX_MDCMDIO].desc = "MDCMDIO PHY Register ACCESS";
#else
	romePerfStat[ROMEPERF_INDEX_8187_RX_ISR].desc = "RX_ISR+[2]+[3]";
	romePerfStat[ROMEPERF_INDEX_USB_SUBMIT_URB_RX].desc = "SUBMIT_URB_RX";
	romePerfStat[ROMEPERF_INDEX_8187_RX_PROCESS].desc = "RX_PROCESS+netif_rx";
	romePerfStat[ROMEPERF_INDEX_8187_NETIF_RX].desc = "netif_rx";
	romePerfStat[ROMEPERF_INDEX_8187_TX_XMIT].desc = "HARD_START_XMIT+SUBMIT_URB_TX";
	romePerfStat[ROMEPERF_INDEX_USB_SUBMIT_URB_TX].desc = "SUBMIT_URB_TX";
	romePerfStat[ROMEPERF_INDEX_USB_SUBMIT_URB_TX_SC].desc = "SUBMIT_URB_TX_SC";	
	romePerfStat[ROMEPERF_INDEX_8187_TX_ISR].desc = "TX_ISR";
	romePerfStat[ROMEPERF_INDEX_USB_ISR].desc = "USB_ISR";	
	romePerfStat[ROMEPERF_INDEX_USB_TEST].desc = "USB_TEST";
	romePerfStat[ROMEPERF_INDEX_USB_TEST2].desc = "USB_TEST2";
	romePerfStat[ROMEPERF_INDEX_USB_TEST3].desc = "USB_TEST3";	
	romePerfStat[ROMEPERF_INDEX_USB_TEST4].desc = "USB_TEST4";		
	romePerfStat[ROMEPERF_INDEX_USB_TEST5].desc = "USB_TEST5";
#if 0	
	romePerfStat[ROMEPERF_INDEX_TX_SHORTCUT].desc = "TX_SHORTCUT";
	romePerfStat[ROMEPERF_INDEX_STARTXMIT1].desc = "STARTXMIT1";
	romePerfStat[ROMEPERF_INDEX_SLOWSTART].desc = "SLOWSTART";
	romePerfStat[ROMEPERF_INDEX_TX_SHORTCUT1].desc = "TX_SHORTCUT1";
	romePerfStat[ROMEPERF_INDEX_TX_SHORTCUT2].desc = "TX_SHORTCUT2";
#endif
#endif


	return SUCCESS;
}

int32 rtl8651_romeperfReset()
{
	rtl8651_romeperfInit();
	
	return SUCCESS;
}

#if 0/* old fashion function, for reference only. */
int32 rtl8651_romeperfStart()
{
	if ( rtl8651_romeperf_inited == FALSE ) rtl8651_romeperfInit();
	
	START_AND_GET_CP3_COUNTER0( cnt1 );

	return SUCCESS;
}

int32 rtl8651_romeperfStop( uint64 *pDiff )
{
	if ( rtl8651_romeperf_inited == FALSE ) rtl8651_romeperfInit();
	
	STOP_AND_GET_CP3_COUNTER0( cnt2 );

	*pDiff = cnt2 - cnt1;
	return SUCCESS;
}
#endif

int32 rtl8651_romeperfGet( uint64 *pGet )
{
	if ( rtl8651_romeperf_inited == FALSE ) return FAILED;

	/* Louis patch: someone will disable CP3 in somewhere. */
	CP3_COUNTER0_INIT();

	CP3_COUNTER0_STOP();
	*pGet = CP3_COUNTER0_GET();
	CP3_COUNTER0_START();
	
	return SUCCESS;
}

int32 rtl8651_romeperfPause( void )
{
	if ( rtl8651_romeperf_inited == FALSE ) return FAILED;

	rtl8651_romeperf_enable = FALSE;
	
	/* Louis patch: someone will disable CP3 in somewhere. */
	CP3_COUNTER0_INIT();

	CP3_COUNTER0_STOP();
	
	return SUCCESS;
}

int32 rtl8651_romeperfResume( void )
{
	if ( rtl8651_romeperf_inited == FALSE ) return FAILED;

	rtl8651_romeperf_enable = TRUE;
	
	/* Louis patch: someone will disable CP3 in somewhere. */
	CP3_COUNTER0_INIT();
 	
	CP3_COUNTER0_START();
	
	return SUCCESS;
}

#if !(defined(RTL8192SU)&&defined(CONFIG_RTL8671))
__IRAM 
#else
__IRAM_IN_8672
#endif
int32 rtl8651_romeperfEnterPoint( uint32 index )
{
	if ( rtl8651_romeperf_inited == FALSE ||
	     rtl8651_romeperf_enable == FALSE ) return FAILED;
	if ( index >= (sizeof(romePerfStat)/sizeof(rtl8651_romeperf_stat_t)) )
		return FAILED;

	/* Louis patch: someone will disable CP3 in somewhere. */
	CP3_COUNTER0_INIT();

	CP3_COUNTER0_STOP();
	CP3_COUNTER0_GET_ALL();
	romePerfStat[index].tempCycle[0] = currCnt[0];
	romePerfStat[index].tempCycle[1] = currCnt[1];
	romePerfStat[index].tempCycle[2] = currCnt[2];
	romePerfStat[index].tempCycle[3] = currCnt[3];
	romePerfStat[index].hasTempCycle = TRUE;
	CP3_COUNTER0_START();
	
	return SUCCESS;
}

#if !(defined(RTL8192SU)&&defined(CONFIG_RTL8671))
__IRAM 
#else
__IRAM_IN_8672
#endif
int32 rtl8651_romeperfExitPoint( uint32 index )
{
	if ( rtl8651_romeperf_inited == FALSE ||
	     rtl8651_romeperf_enable == FALSE ) return FAILED;
	if ( index >= (sizeof(romePerfStat)/sizeof(rtl8651_romeperf_stat_t)) )
		return FAILED;
	if ( romePerfStat[index].hasTempCycle == FALSE )
		return FAILED;

	/* Louis patch: someone will disable CP3 in somewhere. */
	CP3_COUNTER0_INIT();
	
	CP3_COUNTER0_STOP();
	CP3_COUNTER0_GET_ALL();
	romePerfStat[index].accCycle[0] += currCnt[0]-romePerfStat[index].tempCycle[0];
	romePerfStat[index].accCycle[1] += currCnt[1]-romePerfStat[index].tempCycle[1];
	romePerfStat[index].accCycle[2] += currCnt[2]-romePerfStat[index].tempCycle[2];
	romePerfStat[index].accCycle[3] += currCnt[3]-romePerfStat[index].tempCycle[3];
	romePerfStat[index].hasTempCycle = FALSE;
	romePerfStat[index].executedNum++;
	CP3_COUNTER0_START();
	
	return SUCCESS;
}

int32 rtl8651_romeperfDump( int start, int end )
{
#if 0
	int i;

	rtlglue_printf( "index %30s %12s %8s %10s\n", "description", "accCycle", "totalNum", "Average" );
	for( i = start; i <= end; i++ )
	{
		if ( romePerfStat[i].executedNum == 0 )
		{
			rtlglue_printf( "[%3d] %30s %12s %8s %10s\n", i, romePerfStat[i].desc, "--", "--", "--" );
		}
		else
		{
			int j;
			rtlglue_printf( "[%3d] %30s ", 
			                i, romePerfStat[i].desc );
			for( j =0; j < sizeof(romePerfStat[i].accCycle)/sizeof(romePerfStat[i].accCycle[0]);
			     j++ )
			{
				uint32 *pAccCycle = (uint32*)&romePerfStat[i].accCycle[j];
				uint32 avrgCycle = /* Hi-word */ (pAccCycle[0]*(0xffffffff/romePerfStat[i].executedNum)) +
				                   /* Low-word */(pAccCycle[1]/romePerfStat[i].executedNum);

				rtlglue_printf( "%12llu %8u %10u\n",
				                romePerfStat[i].accCycle[j],
				                romePerfStat[i].executedNum, 
				                avrgCycle
				                );
				rtlglue_printf( " %3s  %30s ", "", "" );
			}
			rtlglue_printf( "\r" );
		}
	}
	
	return SUCCESS;
#else
	int i;

	rtl8651_romeperf_stat_t* statSnapShot = rtlglue_malloc(sizeof(rtl8651_romeperf_stat_t) * (end - start + 1) );
	if( statSnapShot == NULL )
	{
		rtlglue_printf("statSnapShot mem alloc failed\n");
		return FAILED;
	}

	rtlglue_printf( "index %30s %12s %8s %10s\n", "description", "accCycle", "totalNum", "Average" );

	for( i = start; i <= end; i++ )
	{
		int j;
		for( j =0; j < sizeof(romePerfStat[i].accCycle)/sizeof(romePerfStat[i].accCycle[0]); j++ )
		{
			statSnapShot[i].accCycle[j]  = romePerfStat[i].accCycle[j];
			statSnapShot[i].tempCycle[j] = romePerfStat[i].tempCycle[j];
		}
		statSnapShot[i].executedNum  = romePerfStat[i].executedNum;
		statSnapShot[i].hasTempCycle = romePerfStat[i].hasTempCycle;
	}

	for( i = start; i <= end; i++ )
	{
		if ( statSnapShot[i].executedNum == 0 )
		{
			rtlglue_printf( "[%3d] %30s %12s %8s %10s\n", i, romePerfStat[i].desc, "--", "--", "--" );
		}
		else
		{
			int j;
			rtlglue_printf( "[%3d] %30s ", i, romePerfStat[i].desc );
			for( j =0; j < sizeof(statSnapShot[i].accCycle)/sizeof(statSnapShot[i].accCycle[0]); j++ )
			{
				uint32 *pAccCycle = (uint32*)&statSnapShot[i].accCycle[j];
				uint32 avrgCycle = /* Hi-word */ (pAccCycle[0]*(0xffffffff/statSnapShot[i].executedNum)) +
				                   /* Low-word */(pAccCycle[1]/statSnapShot[i].executedNum);
				rtlglue_printf( "%12llu %8u %10u\n",
				statSnapShot[i].accCycle[j], 
				statSnapShot[i].executedNum, 
				avrgCycle );
				rtlglue_printf( " %3s  %30s ", "", "" );
			}
			rtlglue_printf( "\r" );
		}
	}

	rtlglue_free(statSnapShot);
	
	return SUCCESS;
#endif
}#endif // PERF_DUMP

