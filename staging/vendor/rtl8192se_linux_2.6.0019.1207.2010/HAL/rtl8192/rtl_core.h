/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
 *
 * Based on the r8180 driver, which is:
 * Copyright 2004-2005 Andrea Merello <andreamrl@tiscali.it>, et al.
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
******************************************************************************/

#ifndef _RTL_CORE_H
#define _RTL_CORE_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/netdevice.h>
#include <linux/pci.h>
#include <linux/etherdevice.h>
#include <linux/delay.h>
#include <linux/rtnetlink.h>	
#include <linux/wireless.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>	
#include <linux/if_arp.h>
#include <linux/random.h>
#include <linux/version.h>
#include <asm/io.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
#include <asm/semaphore.h>
#endif

#include "rtl_platformdef.h"

#include "../../rtllib/rtllib.h"
#ifdef CONFIG_BT_30
#include "../../btlib/bt_inc.h"
#endif

#ifdef ENABLE_DOT11D
#include "../../rtllib/dot11d.h"
#endif



#ifdef RTL8192SE
#include "rtl_dm.h"
#include "rtl8192s/r8192S_inc.h"
#elif defined RTL8190P || defined RTL8192E
#include "rtl_dm.h"
#include "rtl8192e/r8192E_inc.h"
#elif defined RTL8192CE
#include "rtl8192c/r8192C_inc.h"
#endif

#if defined CONFIG_CFG_80211 && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)  
#include "rtl_regd.h"
#endif

#ifdef CONFIG_RTL_RFKILL
#include "rtl_rfkill.h"
#endif

#include "rtl_debug.h"
#include "rtl_eeprom.h"
#include "rtl_ps.h"
#include "rtl_pci.h"
#include "rtl_cam.h"

#define DRV_COPYRIGHT  "Copyright(c) 2008 - 2010 Realsil Semiconductor Corporation"
#define DRV_AUTHOR  "<wlanfae@realtek.com>"
#define DRV_VERSION  "0019.1207.2010"

#ifdef RTL8190P
#define DRV_NAME "rtl819xP"
#elif defined RTL8192E
#define DRV_NAME "rtl819xE"
#elif defined RTL8192SE
#define DRV_NAME "rtl819xSE"
#elif defined RTL8192CE
#define DRV_NAME "rtl8192CE"
#endif

#define IS_HARDWARE_TYPE_819xP(dev) 	((((struct r8192_priv*)rtllib_priv(dev))->card_8192==NIC_8190P)||\
										(((struct r8192_priv*)rtllib_priv(dev))->card_8192==NIC_8192E))
#define IS_HARDWARE_TYPE_8192SE(dev)	(((struct r8192_priv*)rtllib_priv(dev))->card_8192==NIC_8192SE)
#define IS_HARDWARE_TYPE_8192SU(dev)	(((struct r8192_priv*)rtllib_priv(dev))->card_8192==NIC_8192SU)
#define IS_HARDWARE_TYPE_8192CE(dev)	(((struct r8192_priv*)rtllib_priv(dev))->card_8192==NIC_8192CE)
#define IS_HARDWARE_TYPE_8192CU(dev)	(((struct r8192_priv*)rtllib_priv(dev))->card_8192==NIC_8192CU)
#define IS_HARDWARE_TYPE_8192DE(dev)	(((struct r8192_priv*)rtllib_priv(dev))->card_8192==NIC_8192DE)
#define IS_HARDWARE_TYPE_8192DU(dev)	(((struct r8192_priv*)rtllib_priv(dev))->card_8192==NIC_8192DU)
#define IS_HARDWARE_TYPE_8723E(dev)		(((struct r8192_priv*)rtllib_priv(dev))->card_8192==NIC_8723E)
#define IS_HARDWARE_TYPE_8723U(dev)		(((struct r8192_priv*)rtllib_priv(dev))->card_8192==NIC_8723U)

#define	IS_HARDWARE_TYPE_8192S(dev)			\
(IS_HARDWARE_TYPE_8192SE(dev) || IS_HARDWARE_TYPE_8192SU(dev))

#define	IS_HARDWARE_TYPE_8192C(dev)			\
(IS_HARDWARE_TYPE_8192CE(dev) || IS_HARDWARE_TYPE_8192CU(dev))

#define	IS_HARDWARE_TYPE_8192D(dev)			\
(IS_HARDWARE_TYPE_8192DE(dev) || IS_HARDWARE_TYPE_8192DU(dev))

#define	IS_HARDWARE_TYPE_8723(dev)			\
(IS_HARDWARE_TYPE_8723E(dev) || IS_HARDWARE_TYPE_8723U(dev))


#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
#define RTL_PCI_DEVICE(vend, dev, cfg) \
	.vendor = (vend), .device = (dev), \
	.subvendor = PCI_ANY_ID, .subdevice =PCI_ANY_ID , \
	.driver_data = (kernel_ulong_t)&(cfg)
#else
#define RTL_PCI_DEVICE(vend, dev, cfg) \
	.vendor = (vend), .device = (dev), \
	.subvendor = PCI_ANY_ID, .subdevice =PCI_ANY_ID
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	typedef void irqreturn_type; 
#else
	typedef irqreturn_t irqreturn_type; 
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10))

#if !defined(PCI_CAP_ID_EXP)
#define PCI_CAP_ID_EXP			    0x10
#endif
#if !defined(PCI_EXP_LNKCTL)
#define PCI_EXP_LNKCTL			    0x10
#endif

typedef int __bitwise pci_power_t; 
#define PCI_D0		((pci_power_t __force) 0)
#define PCI_D1		((pci_power_t __force) 1)
#define PCI_D2		((pci_power_t __force) 2)
#define PCI_D3hot	((pci_power_t __force) 3)
#define PCI_D3cold	((pci_power_t __force) 4)
#define PCI_UNKNOWN	((pci_power_t __force) 5)
#define PCI_POWER_ERROR	((pci_power_t __force) -1)

#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
	#define rtl8192_interrupt(x,y,z) rtl8192_interrupt_rsl(x,y,z)
#else
	#define rtl8192_interrupt(x,y,z) rtl8192_interrupt_rsl(x,y)
#endif

#define RTL_MAX_SCAN_SIZE 128 

#define RTL_RATE_MAX     	30

#define TOTAL_CAM_ENTRY 	32
#define CAM_CONTENT_COUNT 	8

#ifndef BIT
#define BIT(_i)				(1<<(_i))
#endif


#ifdef _RTL8192_EXT_PATCH_	
#ifdef ASL
#define IS_NIC_DOWN(priv)	((!(priv)->up) && (!(priv)->mesh_up) && (!(priv)->apdev_up))
#else
#define IS_NIC_DOWN(priv)	((!(priv)->up) && (!(priv)->mesh_up))
#endif
#else 
#ifdef ASL
#define IS_NIC_DOWN(priv)	((!(priv)->up) && (!(priv)->apdev_up))
#else  
#define IS_NIC_DOWN(priv)	(!(priv)->up)
#endif  
#endif  

#define IS_ADAPTER_SENDS_BEACON(dev) 0

#ifdef _RTL8192_EXT_PATCH_
#define IS_UNDER_11N_AES_MODE(_rtllib)  ((_rtllib->pHTInfo->bCurrentHTSupport == true) &&\
					((_rtllib->pairwise_key_type == KEY_TYPE_CCMP) || \
					 (_rtllib->mesh_pairwise_key_type == KEY_TYPE_CCMP)))
#else
#define IS_UNDER_11N_AES_MODE(_rtllib)  ((_rtllib->pHTInfo->bCurrentHTSupport == true) &&\
					(_rtllib->pairwise_key_type == KEY_TYPE_CCMP))
#endif

#define HAL_MEMORY_MAPPED_IO_RANGE_8190PCI 	0x1000     
#define HAL_HW_PCI_REVISION_ID_8190PCI			0x00
#define HAL_MEMORY_MAPPED_IO_RANGE_8192PCIE	0x4000	
#define HAL_HW_PCI_REVISION_ID_8192PCIE		0x01
#define HAL_MEMORY_MAPPED_IO_RANGE_8192SE	0x4000	
#define HAL_HW_PCI_REVISION_ID_8192SE 	0x10
#define HAL_HW_PCI_REVISION_ID_8192CE			0x1
#define HAL_MEMORY_MAPPED_IO_RANGE_8192CE	0x4000	
#define HAL_HW_PCI_REVISION_ID_8192DE			0x0
#define HAL_MEMORY_MAPPED_IO_RANGE_8192DE	0x4000	

#define HAL_HW_PCI_8180_DEVICE_ID             		0x8180
#define HAL_HW_PCI_8185_DEVICE_ID             		0x8185	
#define HAL_HW_PCI_8188_DEVICE_ID             		0x8188	
#define HAL_HW_PCI_8198_DEVICE_ID             		0x8198	
#define HAL_HW_PCI_8190_DEVICE_ID             		0x8190	
#define HAL_HW_PCI_8192_DEVICE_ID             		0x8192	
#define HAL_HW_PCI_8192SE_DEVICE_ID             		0x8192	
#define HAL_HW_PCI_8174_DEVICE_ID             		0x8174	
#define HAL_HW_PCI_8173_DEVICE_ID             		0x8173	
#define HAL_HW_PCI_8172_DEVICE_ID             		0x8172	
#define HAL_HW_PCI_8171_DEVICE_ID             		0x8171	
#define HAL_HW_PCI_0045_DEVICE_ID				0x0045	
#define HAL_HW_PCI_0046_DEVICE_ID				0x0046	
#define HAL_HW_PCI_0044_DEVICE_ID				0x0044	
#define HAL_HW_PCI_0047_DEVICE_ID				0x0047	
#define HAL_HW_PCI_700F_DEVICE_ID		   		0x700F
#define HAL_HW_PCI_701F_DEVICE_ID		   		0x701F
#define HAL_HW_PCI_DLINK_DEVICE_ID				0x3304
#define HAL_HW_PCI_8192CET_DEVICE_ID             	0x8191	
#define HAL_HW_PCI_8192CE_DEVICE_ID             		0x8178	
#define HAL_HW_PCI_8191CE_DEVICE_ID             		0x8177	
#define HAL_HW_PCI_8188CE_DEVICE_ID             		0x8176	
#define HAL_HW_PCI_8192CU_DEVICE_ID             		0x8191	
#define HAL_HW_PCI_8192DE_DEVICE_ID             		0x092D	
#define HAL_HW_PCI_8192DU_DEVICE_ID             		0x092D	

#ifdef RTL8192CE
#define RTL819X_DEFAULT_RF_TYPE 	RF_2T2R
#else
#define RTL819X_DEFAULT_RF_TYPE 	RF_1T2R
#endif

#define RTLLIB_WATCH_DOG_TIME    	2000

#define MAX_DEV_ADDR_SIZE		8  /* support till 64 bit bus width OS */
#define MAX_FIRMWARE_INFORMATION_SIZE   32 
#define MAX_802_11_HEADER_LENGTH       	(40 + MAX_FIRMWARE_INFORMATION_SIZE)
#define ENCRYPTION_MAX_OVERHEAD		128
#define MAX_FRAGMENT_COUNT		8
#define MAX_TRANSMIT_BUFFER_SIZE  	(1600+(MAX_802_11_HEADER_LENGTH+ENCRYPTION_MAX_OVERHEAD)*MAX_FRAGMENT_COUNT) 

#define scrclng				4		

#define DEFAULT_FRAG_THRESHOLD 	2342U
#define MIN_FRAG_THRESHOLD     	256U
#define DEFAULT_BEACONINTERVAL 	0x64U

#define DEFAULT_SSID 		""
#define DEFAULT_RETRY_RTS	7
#define DEFAULT_RETRY_DATA 	7
#define PRISM_HDR_SIZE 		64

#define	PHY_RSSI_SLID_WIN_MAX		 	100

#ifdef ASL
#define RTL_IOCTL_WPA					SIOCIWFIRSTPRIV+29
#endif
#define RTL_IOCTL_WPA_SUPPLICANT		SIOCIWFIRSTPRIV+30

#define TxBBGainTableLength 		        37
#define CCKTxBBGainTableLength 	                23

#define CHANNEL_PLAN_LEN			10
#define sCrcLng 		                4

#define NIC_SEND_HANG_THRESHOLD_NORMAL		4        
#define NIC_SEND_HANG_THRESHOLD_POWERSAVE 	8

#define MAX_TX_QUEUE				9 

#if defined RTL8192SE || defined RTL8192CE
#define MAX_RX_QUEUE				2
#else 
#define MAX_RX_QUEUE				1
#endif

#define MAX_RX_COUNT                            64
#define MAX_TX_QUEUE_COUNT                      9        

enum RTL819x_PHY_PARAM {
	RTL819X_PHY_MACPHY_REG		= 0,		
	RTL819X_PHY_MACPHY_REG_PG	= 1,		
	RTL8188C_PHY_MACREG			=2,		
	RTL8192C_PHY_MACREG			=3,		
	RTL819X_PHY_REG				= 4,		
	RTL819X_PHY_REG_1T2R			= 5,		
	RTL819X_PHY_REG_to1T1R		= 6,		
	RTL819X_PHY_REG_to1T2R		= 7,		
	RTL819X_PHY_REG_to2T2R		= 8,		
	RTL819X_PHY_REG_PG			= 9,		
	RTL819X_AGC_TAB				= 10,		
	RTL819X_PHY_RADIO_A			=11,		
	RTL819X_PHY_RADIO_A_1T		=12,		
	RTL819X_PHY_RADIO_A_2T		=13,		
	RTL819X_PHY_RADIO_B			=14,		
	RTL819X_PHY_RADIO_B_GM		=15,		
	RTL819X_PHY_RADIO_C			=16,		
	RTL819X_PHY_RADIO_D			=17,		
	RTL819X_EEPROM_MAP			=18,		
	RTL819X_EFUSE_MAP				=19,		
};

enum RTL_DEBUG {
	COMP_TRACE		= BIT0,	 
	COMP_DBG		= BIT1,	 
	COMP_INIT		= BIT2,	 
	COMP_RECV		= BIT3,	 
	COMP_SEND		= BIT4,	 
	COMP_CMD		= BIT5,	 
	COMP_POWER	        = BIT6,	 
	COMP_EPROM	        = BIT7,	 
	COMP_SWBW		= BIT8,	 
	COMP_SEC		= BIT9,	 
	COMP_LPS		= BIT10, 
	COMP_QOS		= BIT11, 
	COMP_RATE		= BIT12, 
	COMP_RXDESC	        = BIT13, 
	COMP_PHY		= BIT14, 
	COMP_DIG		= BIT15, 
	COMP_TXAGC		= BIT16, 
	COMP_HALDM	        = BIT17, 
	COMP_POWER_TRACKING	= BIT18, 
	COMP_CH		        = BIT19, 
	COMP_RF		        = BIT20, 
	COMP_FIRMWARE	        = BIT21, 
	COMP_HT		        = BIT22, 
	COMP_RESET		= BIT23,
	COMP_CMDPKT	        = BIT24,
	COMP_SCAN		= BIT25,
	COMP_PS		        = BIT26,
	COMP_DOWN		= BIT27, 
	COMP_INTR 		= BIT28, 
	COMP_LED		= BIT29, 
	COMP_MLME		= BIT30, 
	COMP_ERR		= BIT31  
};

typedef enum{
	NIC_UNKNOWN     = 0,
	NIC_8192E       = 1,
	NIC_8190P       = 2,
	NIC_8192SE      = 4,
	NIC_8192SU      = 5,
	NIC_8192CE     	= 6,
	NIC_8192CU     	= 7,
	NIC_8192DE     	= 8,
	NIC_8192DU     	= 9,
	NIC_8723E		= 10,
	NIC_8723U		= 11,
} nic_t;

typedef	enum _RT_EEPROM_TYPE{
	EEPROM_93C46,
	EEPROM_93C56,
	EEPROM_BOOT_EFUSE,
}RT_EEPROM_TYPE,*PRT_EEPROM_TYPE;

typedef enum _tag_TxCmd_Config_Index{
	TXCMD_TXRA_HISTORY_CTRL	        = 0xFF900000,
	TXCMD_RESET_TX_PKT_BUFF		= 0xFF900001,
	TXCMD_RESET_RX_PKT_BUFF		= 0xFF900002,
	TXCMD_SET_TX_DURATION		= 0xFF900003,
	TXCMD_SET_RX_RSSI		= 0xFF900004,
	TXCMD_SET_TX_PWR_TRACKING	= 0xFF900005,
	TXCMD_XXXX_CTRL,
}DCMD_TXCMD_OP;

typedef enum _RT_RF_TYPE_819xU{
        RF_TYPE_MIN = 0,
        RF_8225,
        RF_8256,
        RF_8258,
        RF_6052=4,		
        RF_PSEUDO_11N = 5,
}RT_RF_TYPE_819xU, *PRT_RF_TYPE_819xU;

typedef enum tag_Rf_Operatetion_State
{    
    RF_STEP_INIT = 0,
    RF_STEP_NORMAL,   
    RF_STEP_MAX
}RF_STEP_E;

typedef enum _RT_STATUS{
	RT_STATUS_SUCCESS,
	RT_STATUS_FAILURE,
	RT_STATUS_PENDING,
	RT_STATUS_RESOURCE
}RT_STATUS,*PRT_STATUS;

typedef enum _RT_CUSTOMER_ID
{
	RT_CID_DEFAULT          = 0,
	RT_CID_8187_ALPHA0      = 1,
	RT_CID_8187_SERCOMM_PS  = 2,
	RT_CID_8187_HW_LED      = 3,
	RT_CID_8187_NETGEAR     = 4,
	RT_CID_WHQL             = 5,
	RT_CID_819x_CAMEO       = 6, 
	RT_CID_819x_RUNTOP      = 7,
	RT_CID_819x_Senao       = 8,
	RT_CID_TOSHIBA          = 9,	
	RT_CID_819x_Netcore     = 10,
	RT_CID_Nettronix        = 11,
	RT_CID_DLINK            = 12,
	RT_CID_PRONET           = 13,
	RT_CID_COREGA           = 14,
	RT_CID_819x_ALPHA       = 15,
	RT_CID_819x_Sitecom     = 16,
	RT_CID_CCX              = 17, 
	RT_CID_819x_Lenovo      = 18,	
	RT_CID_819x_QMI         = 19,
	RT_CID_819x_Edimax_Belkin = 20,		
	RT_CID_819x_Sercomm_Belkin = 21,			
	RT_CID_819x_CAMEO1 = 22,
	RT_CID_819x_MSI = 23,
	RT_CID_819x_Acer = 24,
	RT_CID_819x_HP	=27,
	RT_CID_819x_CLEVO = 28,
	RT_CID_819x_Arcadyan_Belkin = 29,
	RT_CID_819x_SAMSUNG = 30,
	RT_CID_819x_WNC_COREGA = 31,
	RT_CID_819x_Foxcoon = 32,
	RT_CID_819x_DELL = 33,
}RT_CUSTOMER_ID, *PRT_CUSTOMER_ID;

typedef enum _RESET_TYPE {
	RESET_TYPE_NORESET = 0x00,
	RESET_TYPE_NORMAL = 0x01,
	RESET_TYPE_SILENT = 0x02
} RESET_TYPE;

typedef enum _IC_INFERIORITY_8192S{
	IC_INFERIORITY_A            = 0, 
	IC_INFERIORITY_B            = 1, 
}IC_INFERIORITY_8192S, *PIC_INFERIORITY_8192S;

typedef enum _PCI_BRIDGE_VENDOR {
	PCI_BRIDGE_VENDOR_INTEL = 0x0,
	PCI_BRIDGE_VENDOR_ATI, 
	PCI_BRIDGE_VENDOR_AMD, 
	PCI_BRIDGE_VENDOR_SIS ,
	PCI_BRIDGE_VENDOR_UNKNOWN, 
	PCI_BRIDGE_VENDOR_MAX ,
} PCI_BRIDGE_VENDOR;

typedef struct buffer
{
	struct buffer *next;
	u32 *buf;
	dma_addr_t dma;
	
} buffer;

typedef struct rtl_reg_debug{
        unsigned int  cmd;
        struct {
                unsigned char type;
                unsigned char addr;
                unsigned char page;
                unsigned char length;
        } head;
        unsigned char buf[0xff];
}rtl_reg_debug;

typedef struct _rt_9x_tx_rate_history {
	u32             cck[4];
	u32             ofdm[8];
	u32             ht_mcs[4][16];
}rt_tx_rahis_t, *prt_tx_rahis_t;

typedef	struct _RT_SMOOTH_DATA_4RF {
	char	elements[4][100];
	u32	index;			
	u32	TotalNum;		
	u32	TotalVal[4];		
}RT_SMOOTH_DATA_4RF, *PRT_SMOOTH_DATA_4RF;

typedef	struct _RT_SMOOTH_DATA {
	u32	elements[100];	
	u32	index;			
	u32	TotalNum;		
	u32	TotalVal;		
}RT_SMOOTH_DATA, *PRT_SMOOTH_DATA;

typedef struct Stats
{
	unsigned long txrdu;
	unsigned long rxrdu;
	unsigned long rxok;
	unsigned long rxframgment;
	unsigned long rxcmdpkt[4];		
	unsigned long rxurberr;
	unsigned long rxstaterr;
	unsigned long rxdatacrcerr;
	unsigned long rxmgmtcrcerr;
	unsigned long rxcrcerrmin;
	unsigned long rxcrcerrmid;
	unsigned long rxcrcerrmax;
	unsigned long received_rate_histogram[4][32];	
	unsigned long received_preamble_GI[2][32];		
	unsigned long	rx_AMPDUsize_histogram[5]; 
	unsigned long rx_AMPDUnum_histogram[5]; 
	unsigned long numpacket_matchbssid;	
	unsigned long numpacket_toself;		
	unsigned long num_process_phyinfo;		
	unsigned long numqry_phystatus;
	unsigned long numqry_phystatusCCK;
	unsigned long numqry_phystatusHT;
	unsigned long received_bwtype[5];              
	unsigned long txnperr;
	unsigned long txnpdrop;
	unsigned long txresumed;
	unsigned long rxoverflow;
	unsigned long rxint;
	unsigned long txnpokint;
	unsigned long ints;
	unsigned long shints;
	unsigned long txoverflow;
	unsigned long txlpokint;
	unsigned long txlpdrop;
	unsigned long txlperr;
	unsigned long txbeokint;
	unsigned long txbedrop;
	unsigned long txbeerr;
	unsigned long txbkokint;
	unsigned long txbkdrop;
	unsigned long txbkerr;
	unsigned long txviokint;
	unsigned long txvidrop;
	unsigned long txvierr;
	unsigned long txvookint;
	unsigned long txvodrop;
	unsigned long txvoerr;
	unsigned long txbeaconokint;
	unsigned long txbeacondrop;
	unsigned long txbeaconerr;
	unsigned long txmanageokint;
	unsigned long txmanagedrop;
	unsigned long txmanageerr;
	unsigned long txcmdpktokint;
	unsigned long txdatapkt;
	unsigned long txfeedback;
	unsigned long txfeedbackok;
	unsigned long txoktotal;
	unsigned long txokbytestotal;
	unsigned long txokinperiod;
	unsigned long txmulticast;
	unsigned long txbytesmulticast;
	unsigned long txbroadcast;
	unsigned long txbytesbroadcast;
	unsigned long txunicast;
	unsigned long txbytesunicast;
	unsigned long rxbytesunicast;
	unsigned long txfeedbackfail;
	unsigned long txerrtotal;
	unsigned long txerrbytestotal;
	unsigned long txerrmulticast;
	unsigned long txerrbroadcast;
	unsigned long txerrunicast;
	unsigned long txretrycount;
	unsigned long txfeedbackretry;
	u8			last_packet_rate;
	unsigned long slide_signal_strength[100];
	unsigned long slide_evm[100];
	unsigned long	slide_rssi_total;	
	unsigned long slide_evm_total;	
	long signal_strength; 
	long signal_quality;
	long last_signal_strength_inpercent;
	long	recv_signal_power;	
	u8 rx_rssi_percentage[4];
	u8 rx_evm_percentage[2];
	long rxSNRdB[4];
	rt_tx_rahis_t txrate;
	u32 Slide_Beacon_pwdb[100];	
	u32 Slide_Beacon_Total;		
	RT_SMOOTH_DATA_4RF		cck_adc_pwdb;
	u32	CurrentShowTxate;
	u32 RssiCalculateCnt;
	RT_SMOOTH_DATA	ui_rssi;
	RT_SMOOTH_DATA ui_link_quality;
} Stats;

typedef struct 	ChnlAccessSetting {
	u16 SIFS_Timer;
	u16 DIFS_Timer; 
	u16 SlotTimeTimer;
	u16 EIFS_Timer;
	u16 CWminIndex;
	u16 CWmaxIndex;
}*PCHANNEL_ACCESS_SETTING,CHANNEL_ACCESS_SETTING;

typedef enum _TWO_PORT_STATUS
{
	TWO_PORT_STATUS__DEFAULT_ONLY,				
	TWO_PORT_STATUS__EXTENSION_ONLY,			
	TWO_PORT_STATUS__EXTENSION_FOLLOW_DEFAULT,	
	TWO_PORT_STATUS__DEFAULT_G_EXTENSION_N20,	
	TWO_PORT_STATUS__ADHOC,						
	TWO_PORT_STATUS__WITHOUT_ANY_ASSOCIATE		
}TWO_PORT_STATUS;

typedef enum _RT_MULTI_FUNC{
	RT_MULTI_FUNC_NONE = 0x00,
	RT_MULTI_FUNC_WIFI = 0x01,
	RT_MULTI_FUNC_BT = 0x02,
	RT_MULTI_FUNC_GPS = 0x04,
}RT_MULTI_FUNC,*PRT_MULTI_FUNC;


typedef enum _RT_POLARITY_CTL{
	RT_POLARITY_LOW_ACT = 0,
	RT_POLARITY_HIGH_ACT = 1,	
}RT_POLARITY_CTL,*PRT_POLARITY_CTL;

typedef struct _txbbgain_struct
{
	long	txbb_iq_amplifygain;
	u32	txbbgain_value;
} txbbgain_struct, *ptxbbgain_struct;

typedef struct _ccktxbbgain_struct
{
	u8	ccktxbb_valuearray[8];
} ccktxbbgain_struct,*pccktxbbgain_struct;

typedef struct _init_gain
{
	u8				xaagccore1;
	u8				xbagccore1;
	u8				xcagccore1;
	u8				xdagccore1;
	u8				cca;

} init_gain, *pinit_gain;

typedef struct _tx_ring{
	u32 * desc;
	u8 nStuckCount;
	struct _tx_ring * next;
}__attribute__ ((packed)) tx_ring, * ptx_ring;

struct rtl8192_tx_ring {
    tx_desc *desc;
    dma_addr_t dma;
    unsigned int idx;
    unsigned int entries;
    struct sk_buff_head queue;
};



struct rtl819x_ops{
	nic_t nic_type;
	void (* get_eeprom_size)(struct net_device* dev);
	void (* init_adapter_variable)(struct net_device* dev);
	void (* init_before_adapter_start)(struct net_device* dev);
	bool (* initialize_adapter)(struct net_device* dev);
	void (*link_change)(struct net_device* dev);
#ifdef ASL
	void (*ap_link_change)(struct net_device* dev);
#endif
	void (* tx_fill_descriptor)(struct net_device* dev, tx_desc * tx_desc, cb_desc * cb_desc, struct sk_buff *skb);
	void (* tx_fill_cmd_descriptor)(struct net_device* dev, tx_desc_cmd * entry, cb_desc * cb_desc, struct sk_buff *skb); 
	bool (* rx_query_status_descriptor)(struct net_device* dev, struct rtllib_rx_stats*  stats, rx_desc *pdesc, struct sk_buff* skb);
	bool (* rx_command_packet_handler)(struct net_device *dev, struct sk_buff* skb, rx_desc *pdesc);
	void (* stop_adapter)(struct net_device *dev, bool reset);
#if defined RTL8192SE || defined RTL8192CE
	void (* update_ratr_table)(struct net_device* dev,u8* pMcsRate,struct sta_info* pEntry);
#else
	void (* update_ratr_table)(struct net_device* dev);
#endif
	void (* irq_enable)(struct net_device* dev);
	void (* irq_disable)(struct net_device* dev);
	void (* irq_clear)(struct net_device* dev);
	void (* rx_enable)(struct net_device* dev);
	void (* tx_enable)(struct net_device* dev);	
	void (* interrupt_recognized)(struct net_device *dev, u32 *p_inta, u32 *p_intb);
	bool (* TxCheckStuckHandler)(struct net_device* dev);
	bool (* RxCheckStuckHandler)(struct net_device* dev);
#ifdef ASL
	void (* rtl819x_hwconfig)(struct net_device* dev);
#endif
};

typedef struct r8192_priv
{
	struct pci_dev *pdev;
	struct pci_dev *bridge_pdev;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10))
	u32		pci_state;
#endif
#ifdef ASL
	struct net_device *apdev;
	short	apdev_up;
	struct iw_statistics 			ap_wstats;
#endif
	bool 		bfirst_init;
	bool 		bfirst_after_down;
	bool 		initialized_at_probe;
	bool		being_init_adapter;
	bool		bDriverIsGoingToUnload;
	
	int 		irq;
	short 	irq_enabled;

	short 	up;
	short 	up_first_time;
#ifdef _RTL8192_EXT_PATCH_
	short 	mesh_up;
#endif

	delayed_work_struct_rsl 	update_beacon_wq;
	delayed_work_struct_rsl 	watch_dog_wq;    
	delayed_work_struct_rsl 	txpower_tracking_wq;
	delayed_work_struct_rsl 	rfpath_check_wq;
	delayed_work_struct_rsl 	gpio_change_rf_wq;
	delayed_work_struct_rsl 	initialgain_operate_wq;
	delayed_work_struct_rsl 	check_hw_scan_wq; 
	delayed_work_struct_rsl 	hw_scan_simu_wq; 
	delayed_work_struct_rsl 	start_hw_scan_wq; 	
	
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)  
	struct workqueue_struct 	*priv_wq;
#else
	u32 						*priv_wq;
#endif

#ifdef _RTL8192_EXT_PATCH_
	struct mshclass			*mshobj;
#endif

	CHANNEL_ACCESS_SETTING	ChannelAccessSetting;

	mp_adapter				NdisAdapter;
	
	struct rtl819x_ops 			*ops;
	struct rtllib_device 			*rtllib;
	
#ifdef CONFIG_BT_30
	BT30Info				BtInfo;
	delayed_work_struct_rsl	HCICmdWorkItem;
#ifdef BT_TODO 
	delayed_work_struct_rsl	HCISendACLDataWorkItem;
#endif
	delayed_work_struct_rsl	HCISendBeaconWorkItem;

	int bt_close;
	spinlock_t hci_event_lock[2]; 			/* 0:rx, 1:tx*/
	struct list_head hci_event_queue[2]; 	/* 0:rx, 1:tx*/
	wait_queue_head_t hci_event_wait[2]; 	/* 0:rx, 1:tx*/

	spinlock_t acl_data_lock[2]; 			/* 0:rx, 1:tx*/
	struct list_head acl_data_queue[2]; 	/* 0:rx, 1:tx*/
	wait_queue_head_t acl_data_wait[2]; 	/* 0:rx, 1:tx*/
#endif
	
#ifdef CONFIG_RTLWIFI_DEBUGFS
	rtl_fs_debug 				*debug;
#endif /* CONFIG_IWLWIFI_DEBUGFS */

	work_struct_rsl 			reset_wq;
	
	LOG_INTERRUPT_8190_T 	InterruptLog;

	RT_CUSTOMER_ID 			CustomerID;


	RT_RF_TYPE_819xU 		rf_chip;	
	IC_INFERIORITY_8192S 		IC_Class;
	HT_CHANNEL_WIDTH		CurrentChannelBW;	
	BB_REGISTER_DEFINITION_T	PHYRegDef[4];	
	rate_adaptive 				rate_adaptive;

	ccktxbbgain_struct			cck_txbbgain_table[CCKTxBBGainTableLength];
	ccktxbbgain_struct			cck_txbbgain_ch14_table[CCKTxBBGainTableLength];
	
	txbbgain_struct 			txbbgain_table[TxBBGainTableLength];

	ACM_METHOD 				AcmMethod; 
	
#if defined RTL8192SE || defined RTL8192CE
	LED_STRATEGY_8190		LedStrategy;  
	LED_8190				SwLed0;
	LED_8190				SwLed1;
#endif

	prt_firmware				pFirmware;
	rtl819x_loopback_e			LoopbackMode;
	firmware_source_e			firmware_source;
	
	struct timer_list 			watch_dog_timer;
	struct timer_list 			fsync_timer;
	struct timer_list 			gpio_polling_timer;
	struct timer_list 			SwAntennaSwitchTimer;
	
	spinlock_t 				fw_scan_lock;
	spinlock_t 				irq_lock;
	spinlock_t 				irq_th_lock;
	spinlock_t 				tx_lock;
	spinlock_t 				rf_ps_lock;
	spinlock_t 				rw_lock;
	spinlock_t 				rt_h2c_lock;
#ifdef CONFIG_ASPM_OR_D3
	spinlock_t 				D3_lock;
#endif
	spinlock_t 				rf_lock; 
	spinlock_t 				ps_lock;

	struct sk_buff_head 		rx_queue;
	struct sk_buff_head 		skb_queue;

	struct tasklet_struct 		irq_rx_tasklet;
	struct tasklet_struct 		irq_tx_tasklet;
	struct tasklet_struct 		irq_prepare_beacon_tasklet;

	struct semaphore 			wx_sem;
	struct semaphore 			rf_sem; 
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16))
	struct semaphore 			mutex;
#else
	struct mutex 				mutex;
#endif

	struct Stats 				stats;
	struct iw_statistics 			wstats;
	struct proc_dir_entry 		*dir_dev;
	
	short (*rf_set_sens)(struct net_device *dev,short sens);
	u8 (*rf_set_chan)(struct net_device *dev,u8 ch);
	void (*rf_close)(struct net_device *dev);
	void (*rf_init)(struct net_device *dev);
	
#if defined CONFIG_CFG_80211 && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)  
	struct ieee80211_rate rates[IEEE80211_NUM_BANDS][RTL_RATE_MAX];
	struct ieee80211_supported_band bands[IEEE80211_NUM_BANDS];
#endif	

	rx_desc 		*rx_ring[MAX_RX_QUEUE];
	struct sk_buff 	*rx_buf[MAX_RX_QUEUE][MAX_RX_COUNT];
	dma_addr_t 	rx_ring_dma[MAX_RX_QUEUE];
	unsigned int 	rx_idx[MAX_RX_QUEUE];
	int 		rxringcount;
	u16 		rxbuffersize;

	u32     	LastRxDescTSFHigh;
	u32     	LastRxDescTSFLow;

	u16		EarlyRxThreshold;
	u32		ReceiveConfig;
	u8		AcmControl;
	u8		RFProgType;
	u8 		retry_data;
	u8 		retry_rts;
	u16 		rts;
	
	struct rtl8192_tx_ring tx_ring[MAX_TX_QUEUE_COUNT];	
	int		 txringcount;
	int 		txbuffsize;
	int		txfwbuffersize;
	atomic_t 	tx_pending[0x10];

	u16		ShortRetryLimit;
	u16		LongRetryLimit;
	u32		TransmitConfig;
	u8		RegCWinMin;		
	u8      	keepAliveLevel; 

#ifdef CONFIG_RTL_RFKILL
	bool 		rfkill_off;
#endif	
	bool		sw_radio_on;
	bool 		bHwRadioOff;
	bool 		pwrdown;
	bool 		blinked_ingpio;
	u8    		polling_timer_on;
	
	/**********************************************************/

	enum card_type {PCI,MINIPCI,CARDBUS,USB}card_type;

	work_struct_rsl qos_activate;

	u8 		bIbssCoordinator; 

	short 	promisc;
	short 	crcmon; 

	int 		txbeaconcount;
		
	short 	chan;
	short 	sens;
	short 	max_sens;
	u32 		rx_prevlen;
	
	u8 		ScanDelay;	
	bool 		ps_force;

	u32 		irq_mask[2];

	long		RSSI_sum_A;
	long		RSSI_cnt_A;
	long		RSSI_sum_B;
	long		RSSI_cnt_B;
	
	u8 		Rf_Mode;
	nic_t 	card_8192; 
	u8 		card_8192_version; 
	
	short 	enable_gpio0;

	u8 		rf_type; 
	u8		IC_Cut;
	char 		nick[IW_ESSID_MAX_SIZE + 1];

	u32		RegBcnCtrlVal;
	u8		AntDivCfg;

	bool		bTKIPinNmodeFromReg;
	bool		bWEPinNmodeFromReg;

	bool		bLedOpenDrain; 

	u8 		check_roaming_cnt;

	bool 		bIgnoreSilentReset;
	u32 		SilentResetRxSoltNum;
	u32		SilentResetRxSlotIndex;
	u32		SilentResetRxStuckEvent[MAX_SILENT_RESET_RX_SLOT_NUM];

	void 		*scan_cmd;
	u8    	hwscan_bw_40;
	
	u16 		nrxAMPDU_size;
	u8 		nrxAMPDU_aggr_num;

	u32 		last_rxdesc_tsf_high;
	u32 		last_rxdesc_tsf_low;

	
	u16		basic_rate;
	u8		short_preamble;
	u8		dot11CurrentPreambleMode;
	u8 		slot_time;
	u16 		SifsTime;
	
	u8 		RegWirelessMode;
	
	u8 		firmware_version;
	u16		FirmwareSubVersion;
	u16 		rf_pathmap;
	bool 		AutoloadFailFlag;
	bool		bFWReady;

	u8		RegPciASPM;
	u8		RegAMDPciASPM;
	u8		RegHwSwRfOffD3;
	u8		RegSupportPciASPM;
	bool 		bSupportASPM; 
	bool		bSupportBackDoor;
	
	bool		bCurrentMrcSwitch; 
	u8		CurrentTxRate; 

	u32 		RfRegChnlVal[2];

	u8 		ShowRateMode;
	u8 		RATRTableBitmap;

	u8  		EfuseMap[2][HWSET_MAX_SIZE_92S];
	u16 		EfuseUsedBytes;
	u8  		EfuseUsedPercentage;

#ifdef EFUSE_REPG_WORKAROUND 
	bool 		efuse_RePGSec1Flag;
	u8   		efuse_RePGData[8];
#endif

	short 	epromtype;
	u16 		eeprom_vid;
	u16 		eeprom_did;
	u16 		eeprom_svid;
	u16 		eeprom_smid;
	u8  		eeprom_CustomerID;
	u16  	eeprom_ChannelPlan;
	u8 		eeprom_version;

	u8 		EEPROMRegulatory;
	u8 		EEPROMPwrGroup[2][3];
	u8 		EEPROMOptional;	
	
	u8 		EEPROMTxPowerLevelCCK[14];		
	u8 		EEPROMTxPowerLevelOFDM24G[14];	
	u8 		EEPROMTxPowerLevelOFDM5G[24];	
	u8 		RfCckChnlAreaTxPwr[2][3];	
	u8 		RfOfdmChnlAreaTxPwr1T[2][3];	
	u8 		RfOfdmChnlAreaTxPwr2T[2][3];	
	u8 		EEPROMRfACCKChnl1TxPwLevel[3];	
	u8 		EEPROMRfAOfdmChnlTxPwLevel[3];
	u8 		EEPROMRfCCCKChnl1TxPwLevel[3];	
	u8 		EEPROMRfCOfdmChnlTxPwLevel[3];
#if defined (RTL8192S_WAPI_SUPPORT)
	u8 		EEPROMWapiSupport;
	u8 		WapiSupport;
#endif
	u16 		EEPROMTxPowerDiff;
	u16 		EEPROMAntPwDiff;		
	u8 		EEPROMThermalMeter;
	u8 		EEPROMPwDiff;
	u8 		EEPROMCrystalCap;

	u8 		EEPROMBluetoothCoexist;	
	u8 		EEPROMBluetoothType;
	u8 		EEPROMBluetoothAntNum;
	u8 		EEPROMBluetoothAntIsolation;
	u8 		EEPROMBluetoothRadioShared;

	
	u8 		EEPROMSupportWoWLAN;
	u8 		EEPROMBoardType;
	u8 		EEPROM_Def_Ver;
	u8 		EEPROMHT2T_TxPwr[6];			
	u8 		EEPROMTSSI_A;
	u8 		EEPROMTSSI_B;
	u8 		EEPROMTxPowerLevelCCK_V1[3];
	u8 		EEPROMLegacyHTTxPowerDiff;	

	u8 		BluetoothCoexist;

	u8		CrystalCap;						
	u8		ThermalMeter[2];	

	u16 		FwCmdIOMap;
	u32 		FwCmdIOParam;
	
	u8		SwChnlInProgress;
	u8 		SwChnlStage;
	u8		SwChnlStep;
	u8		SetBWModeInProgress;

	u8		nCur40MhzPrimeSC;	

	u32		RfReg0Value[4];
	u8 		NumTotalRFPath;	
	bool 		brfpath_rxenable[4];

	bool 		bTXPowerDataReadFromEEPORM;

	u16 		RegChannelPlan; 
	u16 		ChannelPlan;
	bool 		bChnlPlanFromHW;

	bool 		RegRfOff;
	bool 		isRFOff;
	bool 		bInPowerSaveMode;
	u8		bHwRfOffAction;	

	bool 		aspm_clkreq_enable;
	u32 		pci_bridge_vendor;
	u8 		RegHostPciASPMSetting;
	u8 		RegDevicePciASPMSetting;

	bool 		RFChangeInProgress; 
	bool 		SetRFPowerStateInProgress;
	bool 		bdisable_nic;
	
	u8		pwrGroupCnt;

	u8		ThermalValue_LCK;
	u8		ThermalValue_IQK;
	u8		ThermalValue_DPK;	
	bool	bRfPiEnable;

	u32		APKoutput[2][2];				
	bool	bAPKdone;	
	bool	bDPdone;
	bool	bDPPathAOK;
	bool	bDPPathBOK;	
	
	long		RegE94;
	long 		RegE9C;
	long		RegEB4;
	long		RegEBC;
	
	u32		RegC04;
	u32		Reg874;
	u32		RegC08;
	
	bool	bIQKInitialized;
	u32		ADDA_backup[16];
#if defined RTL8192CE
	u32		IQK_MAC_backup[IQK_MAC_REG_NUM];
	u32		IQK_BB_backup_recover[10];
	u32		IQK_BB_backup[IQK_BB_REG_NUM];
#endif

	bool 		SetFwCmdInProgress; 
	u8 		CurrentFwCmdIO;

	u8      	rssi_level;

	bool 		bInformFWDriverControlDM;
	u8 		PwrGroupHT20[2][14];
	u8 		PwrGroupHT40[2][14];
	
	u8 		ThermalValue;
	long 		EntryMinUndecoratedSmoothedPWDB;
	long 		EntryMaxUndecoratedSmoothedPWDB;
	u8 		DynamicTxHighPowerLvl;  
	u8 		LastDTPLvl;
	u32 		CurrentRATR0;
	FALSE_ALARM_STATISTICS FalseAlmCnt;
	
	u8 		DMFlag; 
	u8 		DM_Type; 
	
	u8		CckPwEnl;
	u16		TSSI_13dBm;
	u32 		Pwr_Track;
	u8		CCKPresentAttentuation_20Mdefault;
	u8		CCKPresentAttentuation_40Mdefault;
	char		CCKPresentAttentuation_difference;
	char		CCKPresentAttentuation;
	u8		bCckHighPower;
	long		undecorated_smoothed_pwdb;
	long		undecorated_smoothed_cck_adc_pwdb[4];
	
#if  defined RTL8192CE
	u32		MCSTxPowerLevelOriginalOffset[7][16];	
#elif defined RTL8192SE 
	u32		MCSTxPowerLevelOriginalOffset[4][7];
#else
	u32		MCSTxPowerLevelOriginalOffset[6];
#endif
	u32		CCKTxPowerLevelOriginalOffset;
	u8		TxPowerLevelCCK[14];			
	u8		TxPowerLevelCCK_A[14];			
	u8 		TxPowerLevelCCK_C[14];
	u8		TxPowerLevelOFDM24G[14];		
	u8		TxPowerLevelOFDM5G[14];			
	u8		TxPowerLevelOFDM24G_A[14];	
	u8		TxPowerLevelOFDM24G_C[14];	
	u8		LegacyHTTxPowerDiff;			
	u8		TxPowerDiff;
	s8		RF_C_TxPwDiff;					
	s8		RF_B_TxPwDiff;
	u8 		RfTxPwrLevelCck[2][14];
	u8		RfTxPwrLevelOfdm1T[2][14];
	u8		RfTxPwrLevelOfdm2T[2][14];
	u8		AntennaTxPwDiff[3];				
	char		TxPwrHt20Diff[2][14];				
	u8		TxPwrLegacyHtDiff[2][14];		
	u8		TxPwrSafetyFlag;				
	u8		HT2T_TxPwr_A[14]; 				
	u8		HT2T_TxPwr_B[14]; 				
	u8		CurrentCckTxPwrIdx;
	u8 		CurrentOfdm24GTxPwrIdx;

	bool		bdynamic_txpower;  
	bool		bDynamicTxHighPower;  
	bool		bDynamicTxLowPower;  
	bool		bLastDTPFlag_High;
	bool		bLastDTPFlag_Low;

	bool		bstore_last_dtpflag;
	bool		bstart_txctrl_bydtp;   

	u8 		rfa_txpowertrackingindex;
	u8 		rfa_txpowertrackingindex_real;
	u8 		rfa_txpowertracking_default;
	u8 		rfc_txpowertrackingindex;
	u8 		rfc_txpowertrackingindex_real;
	u8 		rfc_txpowertracking_default;
	bool 		btxpower_tracking;
	bool 		bcck_in_ch14;
	
	u8		TxPowerTrackControl;	
	u8   		txpower_count;
	bool 		btxpower_trackingInit;
	
#ifdef RTL8192CE
	char		OFDM_index[2];
	char		CCK_index;
#else
	u8		OFDM_index[2];
	u8   		CCK_index;
#endif	

	u8   		Record_CCK_20Mindex;
	u8   		Record_CCK_40Mindex;
	
	init_gain 	initgain_backup;
	u8   		DefaultInitialGain[4];
	bool 		bis_any_nonbepkts;
	bool 		bcurrent_turbo_EDCA;
	bool		bis_cur_rdlstate;	
	
	bool		bCCKinCH14;

	u8 		MidHighPwrTHR_L1;
	u8 		MidHighPwrTHR_L2;

	bool 		bfsync_processing;	
	u32 		rate_record;
	u32 		rateCountDiffRecord;
	u32		ContiuneDiffCount;
	bool 		bswitch_fsync;
	u8		framesync;
	u32 		framesyncC34;
	u8   		framesyncMonitor;

	bool		bDMInitialGainEnable;
        bool 		MutualAuthenticationFail;
		
	bool		bDisableFrameBursting;
	
	u32 		reset_count;
	bool 		bpbc_pressed;
	
	u32 		txpower_checkcnt;
	u32 		txpower_tracking_callback_cnt;
	u8 		thermal_read_val[40];
	u8 		thermal_readback_index;
	u32 		ccktxpower_adjustcnt_not_ch14;
	u32		ccktxpower_adjustcnt_ch14;

	RESET_TYPE	ResetProgress;
	bool		bForcedSilentReset;
	bool		bDisableNormalResetCheck;
	u16		TxCounter;
	u16		RxCounter;
	int		IrpPendingCount;
	bool		bResetInProgress;
	bool		force_reset;
	bool		force_lps;
	u8		InitialGainOperateType;

	bool 		chan_forced;
	bool 		bSingleCarrier;
	bool 		RegBoard;
	bool 		bCckContTx; 
 	bool 		bOfdmContTx; 
	bool 		bStartContTx; 
	u8   		RegPaModel; 
	u8   		btMpCckTxPower; 
	u8   		btMpOfdmTxPower; 

	u32		MptActType; 	
	u32		MptIoOffset; 
	u32		MptIoValue; 
	u32		MptRfPath;

	u32		MptBandWidth;			
	u32		MptRateIndex;			
	u8		MptChannelToSw; 		
	u32  	MptRCR;

	u8		PwrDomainProtect;	
	u8		H2CTxCmdSeq; 

#ifdef RTL8192CE
	u8		EEPROMTSSI[2];
	u8		EEPROMPwrLimitHT20[3];
	u8		EEPROMPwrLimitHT40[3];
	u8		EEPROMChnlAreaTxPwrCCK[2][3];	
	u8		EEPROMChnlAreaTxPwrHT40_1S[2][3];	
	u8		EEPROMChnlAreaTxPwrHT40_2SDiff[2][3];	
	u8		TxPwrLevelCck[2][14];
	u8		TxPwrLevelHT40_1S[2][14];		
	u8		TxPwrLevelHT40_2S[2][14];		
	bool		bRPDownloadFinished;
	bool		bH2CSetInProgress;
	u8		SetIOInProgress;
	IO_TYPE	CurrentIOType;
	
	u8		CurFwCmdRegSet;
	u8		LastHMEBoxNum;
	
	H2C_CMD_8192C	H2CCmd;
	
	BT_COEXIST_STR	bt_coexist;
	u8		bRegBT_Iso;
	u8		bRegBT_Sco;
	u8		bBT_Ampdu;

	bool		bAPKThermalMeterIgnore;
	bool		bInterruptMigration;
	bool		bDisableTxInt;

	u8		bDefaultAntenna;

	INTERFACE_SELECT_8192CPCIe	InterfaceSel;

	RT_MULTI_FUNC			MultiFunc; 
	RT_POLARITY_CTL			PolarityCtl; 
#endif

}r8192_priv;


#ifdef _RTL8192_EXT_PATCH_
struct meshdev_priv {
	struct net_device_stats stats;
	struct rtllib_device *rtllib;
	struct r8192_priv * priv;
};
#endif
#ifdef ASL
struct apdev_priv {
	struct net_device_stats stats;
	struct rtllib_device *rtllib;
	struct r8192_priv * priv;
	int status;
	int rx_int_enabled;
	int tx_packetlen;
	u8 *tx_packetdata;
	struct sk_buff *skb;
	spinlock_t lock;
};
#endif
extern const struct ethtool_ops rtl819x_ethtool_ops;

#ifdef RTL8192CE
#define Rtl819XFwImageArray				Rtl8192CEFwImgArray
#define Rtl819XMAC_Array					Rtl8192CEMAC_2T_Array
#define Rtl819XAGCTAB_2TArray			Rtl8192CEAGCTAB_2TArray
#define Rtl819XAGCTAB_1TArray			Rtl8192CEAGCTAB_1TArray
#define Rtl819XPHY_REG_2TArray			Rtl8192CEPHY_REG_2TArray			
#define Rtl819XPHY_REG_1TArray			Rtl8192CEPHY_REG_1TArray
#define Rtl819XRadioA_2TArray				Rtl8192CERadioA_2TArray
#define Rtl819XRadioA_1TArray				Rtl8192CERadioA_1TArray
#define Rtl819XRadioB_2TArray				Rtl8192CERadioB_2TArray
#define Rtl819XRadioB_1TArray				Rtl8192CERadioB_1TArray
#define Rtl819XPHY_REG_Array_PG 			Rtl8192CEPHY_REG_Array_PG
#endif

void rtl8192_tx_cmd(struct net_device *dev, struct sk_buff *skb);
short rtl8192_tx(struct net_device *dev, struct sk_buff* skb);

u8 read_nic_io_byte(struct net_device *dev, int x);
u32 read_nic_io_dword(struct net_device *dev, int x);
u16 read_nic_io_word(struct net_device *dev, int x) ;
void write_nic_io_byte(struct net_device *dev, int x,u8 y);
void write_nic_io_word(struct net_device *dev, int x,u16 y);
void write_nic_io_dword(struct net_device *dev, int x,u32 y);

u8 read_nic_byte(struct net_device *dev, int x);
u32 read_nic_dword(struct net_device *dev, int x);
u16 read_nic_word(struct net_device *dev, int x) ;
void write_nic_byte(struct net_device *dev, int x,u8 y);
void write_nic_word(struct net_device *dev, int x,u16 y);
void write_nic_dword(struct net_device *dev, int x,u32 y);

void force_pci_posting(struct net_device *dev);

void rtl8192_rx_enable(struct net_device *);
void rtl8192_tx_enable(struct net_device *);

int rtl8192_hard_start_xmit(struct sk_buff *skb,struct net_device *dev);
void rtl8192_hard_data_xmit(struct sk_buff *skb, struct net_device *dev, int rate);
void rtl8192_data_hard_stop(struct net_device *dev);
void rtl8192_data_hard_resume(struct net_device *dev);
void rtl8192_restart(void *data);
void rtl819x_watchdog_wqcallback(void *data);
void rtl8192_hw_sleep_wq (void *data);
void watch_dog_timer_callback(unsigned long data);
void rtl8192_irq_rx_tasklet(struct r8192_priv *priv);
void rtl8192_irq_tx_tasklet(struct r8192_priv *priv);
int rtl8192_down(struct net_device *dev,bool shutdownrf);
int rtl8192_up(struct net_device *dev);
void rtl8192_commit(struct net_device *dev);
void rtl8192_set_chan(struct net_device *dev,short ch);

void check_rfctrl_gpio_timer(unsigned long data);

void rtl8192_hw_wakeup_wq(void *data);
irqreturn_type rtl8192_interrupt(int irq, void *netdev, struct pt_regs *regs);

short rtl8192_pci_initdescring(struct net_device *dev);

void rtl8192_cancel_deferred_work(struct r8192_priv* priv);

int _rtl8192_up(struct net_device *dev,bool is_silent_reset);

short rtl8192_is_tx_queue_empty(struct net_device *dev);
void rtl8192_irq_disable(struct net_device *dev);

#ifdef _RTL8192_EXT_PATCH_
extern int r8192_mesh_set_enc_ext(struct net_device *dev, struct iw_point *encoding, struct iw_encode_ext *ext, u8 *addr);
#ifdef BUILT_IN_MSHCLASS
extern int msh_init(void);
extern void msh_exit(void);
#endif
#endif


void rtl8192_tx_timeout(struct net_device *dev);
void rtl8192_pci_resetdescring(struct net_device *dev);
#ifdef ASL
void rtl8192_SetWirelessMode(struct net_device* dev, u8 wireless_mode, bool is_ap);
#else
void rtl8192_SetWirelessMode(struct net_device* dev, u8 wireless_mode);
#endif
void rtl8192_irq_enable(struct net_device *dev);
#ifdef ASL
void rtl8192_config_rate(struct net_device* dev, u16* rate_config, bool bap);
#else
void rtl8192_config_rate(struct net_device* dev, u16* rate_config);
#endif
#ifdef ASL
void rtl8192_update_cap(struct net_device* dev, u16 cap, bool is_ap);
#else
void rtl8192_update_cap(struct net_device* dev, u16 cap);
#endif
void rtl8192_irq_disable(struct net_device *dev);

void rtl819x_UpdateRxPktTimeStamp (struct net_device *dev, struct rtllib_rx_stats *stats);
long rtl819x_translate_todbm(struct r8192_priv * priv, u8 signal_strength_index	);
void rtl819x_update_rxsignalstatistics8190pci(struct r8192_priv * priv,struct rtllib_rx_stats * pprevious_stats);
u8 rtl819x_evm_dbtopercentage(char value);
void rtl819x_process_cck_rxpathsel(struct r8192_priv * priv,struct rtllib_rx_stats * pprevious_stats);
u8 rtl819x_query_rxpwrpercentage(	char		antpower	);
void rtl8192_record_rxdesc_forlateruse(struct rtllib_rx_stats * psrc_stats,struct rtllib_rx_stats * ptarget_stats);

bool NicIFEnableNIC(struct net_device* dev);
bool NicIFDisableNIC(struct net_device* dev);

bool
MgntActSet_RF_State(
	struct net_device* dev,
	RT_RF_POWER_STATE	StateToSet,
	RT_RF_CHANGE_SOURCE ChangeSource,
	bool	ProtectOrNot
	);
void
ActUpdateChannelAccessSetting(
	struct net_device* 			dev,
	WIRELESS_MODE			WirelessMode,
	PCHANNEL_ACCESS_SETTING	ChnlAccessSetting
	);

#if defined CONFIG_CFG_80211 && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)  
struct net_device *wiphy_to_net_device(struct wiphy *wiphy);
#endif

#endif


