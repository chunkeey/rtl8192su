/*
 *      io-control handling routines
 *
 *      $Id: 8190n_ioctl.c,v 1.9 2009/08/06 11:41:28 yachang Exp $
 */

#define _8190N_IOCTL_C_

#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/unistd.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/delay.h>
#endif

#include "./8190n_cfg.h"

#ifdef __LINUX_2_6__
#include <linux/initrd.h>
#include <linux/syscalls.h>
#endif

#ifndef __KERNEL__
#include "./sys-support.h"
#endif

#include "./8190n_headers.h"
#include "./8190n_debug.h"
#ifdef RTL8192SU
#include "./8190n_usb.h"
#ifdef RTL867X_CP3
#include "romeperf.h"
#endif

#ifndef REG32
/* Register access macro (REG*()). */
#define REG32(reg) 			(*((volatile unsigned int *)(reg)))
#define REG16(reg) 			(*((volatile unsigned short *)(reg)))
#define REG8(reg) 				(*((volatile unsigned char *)(reg)))
#endif
#endif

#ifdef CONFIG_RTK_MESH
// for commu with  802.11s path selection deamon;plus note
#include "./mesh_ext/mesh_route.h"
#endif

// dump 8190 registers
#define DEBUG_8190

#define RTL8190_IOCTL_SET_MIB				(SIOCDEVPRIVATE + 0x1)	// 0x89f1
#define RTL8190_IOCTL_GET_MIB				(SIOCDEVPRIVATE + 0x2)	// 0x89f2
#define RTL8190_IOCTL_WRITE_REG				(SIOCDEVPRIVATE + 0x3)	// 0x89f3
#define RTL8190_IOCTL_READ_REG				(SIOCDEVPRIVATE + 0x4)	// 0x89f4
#define RTL8190_IOCTL_WRITE_MEM				(SIOCDEVPRIVATE + 0x5)	// 0x89f5
#define RTL8190_IOCTL_READ_MEM				(SIOCDEVPRIVATE + 0x6)	// 0x89f6
#define RTL8190_IOCTL_DEL_STA				(SIOCDEVPRIVATE + 0x7)	// 0x89f7
#define RTL8190_IOCTL_WRITE_EEPROM			(SIOCDEVPRIVATE + 0x8)	// 0x89f8
#define RTL8190_IOCTL_READ_EEPROM			(SIOCDEVPRIVATE + 0x9)	// 0x89f9
#define RTL8190_IOCTL_WRITE_BB_REG			(SIOCDEVPRIVATE + 0xa)	// 0x89fa
#define RTL8190_IOCTL_READ_BB_REG			(SIOCDEVPRIVATE + 0xb)	// 0x89fb
#define RTL8190_IOCTL_WRITE_RF_REG			(SIOCDEVPRIVATE + 0xc)	// 0x89fc
#define RTL8190_IOCTL_READ_RF_REG			(SIOCDEVPRIVATE + 0xd)	// 0x89fd
#define RTL8190_IOCTL_USER_DAEMON_REQUEST	(SIOCDEVPRIVATE + 0xf)	// 0x89ff

#ifdef	CONFIG_RTK_MESH
#define RTL8190_IOCTL_STATIC_ROUTE			(SIOCDEVPRIVATE + 0xe)
#endif

#define SIOCGIWRTLSTAINFO		0x8B30
#define SIOCGIWRTLSTANUM		0x8B31
#define SIOCGIWRTLDRVVERSION	0x8B32
#define SIOCGIWRTLSCANREQ		0x8B33
#define SIOCGIWRTLGETBSSDB		0x8B34
#define SIOCGIWRTLJOINREQ		0x8B35
#define SIOCGIWRTLJOINREQSTATUS	0x8B36
#define SIOCGIWRTLGETBSSINFO	0x8B37
#ifdef WDS
#define SIOCGIWRTLGETWDSINFO	0x8B38
#endif
#define SIOCSIWRTLSTATXRATE		0x8B39
#ifdef MICERR_TEST
#define SIOCSIWRTLMICERROR		0x8B3A
#define SIOCSIWRTLMICREPORT		0x8B3B
#endif
#ifdef SUPPORT_SNMP_MIB
#define SIOCGSNMPMIB			0x8B3D
#endif
#ifdef USE_PID_NOTIFY
#define SIOCSIWRTLSETPID		0x8B3E
#endif
#ifdef CONFIG_RTL_WAPI_SUPPORT
#define SIOCSIWRTLSETWAPIPID		0x8B3F
#endif
#define SIOCSMIBDATA	0x8B41
#define SIOCMIBINIT		0x8B42
#define SIOCMIBSYNC		0x8B43
#define SIOCGMIBDATA	0x8B44
#define SIOCSACLADD		0x8B45
#define SIOCSACLDEL		0x8B46
#define SIOCSACLQUERY	0x8B47

#define SIOCGMISCDATA	0x8B48

#ifdef RTK_WOW
#define SIOCGRTKWOW		0x8B49
#define SIOCGRTKWOWSTAINFO		0x8B5A
#endif

#define SIOCSRFPWRADJ	0x8B5B

#ifdef MP_TEST
#define MP_START_TEST	0x8B61
#define MP_STOP_TEST	0x8B62
#define MP_SET_RATE		0x8B63
#define MP_SET_CHANNEL	0x8B64
#define MP_SET_TXPOWER	0x8B65
#define MP_CONTIOUS_TX	0x8B66
#define MP_ARX			0x8B67
#define MP_SET_BSSID	0x8B68
#define MP_ANTENNA_TX	0x8B69
#define MP_ANTENNA_RX	0x8B6A
#define MP_SET_BANDWIDTH 0x8B6B
#define MP_SET_PHYPARA	0x8B6C
#define MP_QUERY_STATS 	0x8B6D
#define MP_TXPWR_TRACK	0x8B6E
#define MP_QUERY_TSSI	0x8B6F
#if defined(RTL8192SE) || defined(RTL8192SU)
#define MP_QUERY_THER	0x8B77
#endif

#ifdef B2B_TEST
// set/get convention: set(even number) get (odd number)
#define MP_TX_PACKET	0x8B71
#define MP_RX_PACKET	0x8B70
#define MP_BRX_PACKET	0x8B73
#endif


#if defined(CONFIG_RTL865X) && defined(CONFIG_RTL865X_CLE)
#define MP_FLASH_CFG    0x8B80
#endif

#endif // MP_TEST

#ifdef DEBUG_8190
#define SIOCGIWRTLREGDUMP		0x8B78
#endif

#ifdef MBSSID
#define SIOCSICOPYMIB			0x8B79
#endif

#ifdef SUPPORT_TX_MCAST2UNI
#define SIOCGIMCAST_ADD			0x8B80
#define SIOCGIMCAST_DEL			0x8B81
#endif

#ifdef CONFIG_RTL8186_KB
#define SIOCGIREADGUESTMAC		0x8B82
#define SIOCSIWRTGUESTMAC		0x8B83
#endif

#if defined(CONFIG_RTL8186_KB_N)
#define SIOCGIWRTLAUTH			0x8B84//To get wireless auth result
#endif

#ifdef CONFIG_RTL8671
// MBSSID Port Mapping
#define SIOSIWRTLITFGROUP		0x8B90
extern int bitmap_virt2phy(int mbr);
extern struct port_map wlanDev[5];
int g_port_mapping=FALSE;
#endif

#ifdef	CONFIG_RTK_MESH

#ifdef _11s_TEST_MODE_
#define SAVE_RECEIBVER_PID 			0x8B92 //PID ioctl
#define DEQUEUE_RECEIBVER_IOCTL 	0x8B93 //DEQUEUE ioctl
#endif
// ==== inserted by GANTOE for manual site survey 2008/12/25 ====
#define SIOCJOINMESH 				0x8B94
#define SIOCCHECKMESHLINK			0x8B95	// This OID might be removed when the mesh peerlink precedure has been completed
// GANTOE
#ifdef D_ACL//tsananiu
#define RTL8190_IOCTL_ADD_ACL_TABLE		0x8B96
#define RTL8190_IOCTL_REMOVE_ACL_TABLE	0x8B97
#define RTL8190_IOCTL_GET_ACL_TABLE		0x8B98
#define ACL_allow 1
#define ACL_deny 2
#endif//tsananiu//

#define SIOCQPATHTABLE  0x8BA0  // query pathselection table
#define SIOCUPATHTABLE  0x8BA1  // update  existing entry's date in pathselection table
#define SIOCAPATHTABLE  0x8BA2  // add a new entry into pathselection table

#ifdef FREDDY	//mesh related
#define RTL8190_IOCTL_DEL_STA_ENC 0x8BA5 //for kick STA function by freddy 2008/12/2
#endif

#define GET_STA_LIST 			0x8BA6
#define SET_PORTAL_POOL 		0x8BA8
#define SIOC_NOTIFY_PATH_CREATE 0x8BA9 // path selection daemon notify dirver that the path to des mac has created
#define SIOC_UPDATE_ROOT_INFO 	0x8BAA // update root mac into driver
#define SIOC_GET_ROUTING_INFO	0x8BAB // send routing info to user space
#define REMOVE_PATH_ENTRY		0x8BAC // remove specified path entry
#define SIOC_SET_ROUTING_INFO	0x8BAD // set MESH routing info from user space

#define SAVEPID_IOCTL			0x8BB0   //PID ioctl
#define DEQUEUEDATA_IOCTL		0x8BB1   //DEQUEUE ioctl

#ifdef _MESH_ACL_ENABLE_
#define SIOCSMESHACLADD		0x8BB5
#define SIOCSMESHACLDEL		0x8BB6
#define SIOCSMESHACLQUERY	0x8BB7
#endif

#endif // CONFIG_RTK_MESH

#define _OFFSET(field)	((int)(long *)&(((struct wifi_mib *)0)->field))
#define _SIZE(field)	sizeof(((struct wifi_mib *)0)->field)

#define _OFFSET_RFFT(field)	((int)(long *)&(((struct rf_finetune_var *)0)->field))
#define _SIZE_RFFT(field)	sizeof(((struct rf_finetune_var *)0)->field)


typedef enum {BYTE_T, INT_T, SSID_STRING_T, BYTE_ARRAY_T, ACL_T, IDX_BYTE_ARRAY_T, MULTI_BYTE_T,
#ifdef _DEBUG_RTL8190_
		DEBUG_T,
#endif
	DEF_SSID_STRING_T, STRING_T, RFFT_T, VARLEN_BYTE_T,
#ifdef WIFI_SIMPLE_CONFIG
	PIN_IND_T,
#endif
#ifdef 	CONFIG_RTK_MESH
	WORD_T,
#endif
	ACL_INT_T,	// mac address + 1 int
#ifdef CONFIG_RTL_WAPI_SUPPORT	
	INT_ARRAY_T,
	WAPI_KEY_T,
#endif	
} TYPE_T;

struct iwpriv_arg {
	char name[20];	/* mib name */
	TYPE_T type;	/* Type and number of args */
	int offset;		/* mib offset */
	int len;		/* mib byte len */
	int Default;	/* mib default value */
};

/* station info, reported to web server */
typedef struct _sta_info_2_web {
	unsigned short	aid;
	unsigned char	addr[6];
	unsigned long	tx_packets;
	unsigned long	rx_packets;
	unsigned long	expired_time;	// 10 msec unit
	unsigned short	flags;
	unsigned char	TxOperaRate;
	unsigned char	rssi;
	unsigned long	link_time;		// 1 sec unit
	unsigned long	tx_fail;
	unsigned long	tx_bytes;
	unsigned long	rx_bytes;
	unsigned char	network;
	unsigned char ht_info;	// bit0: 0=20M mode, 1=40M mode; bit1: 0=longGI, 1=shortGI
	unsigned char 	resv[6];
} sta_info_2_web;

/* Bit mask value for flags, compatiable with old driver */
#define STA_INFO_FLAG_AUTH_OPEN     	0x01
#define STA_INFO_FLAG_AUTH_WEP      	0x02
#define STA_INFO_FLAG_ASOC          	0x04
#define STA_INFO_FLAG_ASLEEP        	0x08
#if defined(CONFIG_RTL865X) && defined(RTL865X_EXTRA_STAINFO)
#define STA_INFO_FLAG_80211A        	0x10
#define STA_INFO_FLAG_80211B        	0x20   //if this bit is not set, it is a 11G station
#endif

/* BSS info, reported to web server */
typedef struct _bss_info_2_web {
    unsigned char state;
    unsigned char channel;
    unsigned char txRate;
    unsigned char bssid[6];
    unsigned char rssi, sq;
    unsigned char ssid[33];
} bss_info_2_web;

typedef enum _wlan_mac_state {
    STATE_DISABLED=0, STATE_IDLE, STATE_SCANNING, STATE_STARTED, STATE_CONNECTED, STATE_WAITFORKEY
} wlan_mac_state;

#ifdef WDS
typedef enum _wlan_wds_state {
	STATE_WDS_EMPTY=0, STATE_WDS_DISABLED, STATE_WDS_ACTIVE
} wlan_wds_state;

typedef struct _wds_info {
	unsigned char	state;
	unsigned char	addr[6];
	unsigned long	tx_packets;
	unsigned long	rx_packets;
	unsigned long	tx_errors;
	unsigned char	TxOperaRate;
} web_wds_info;
#endif

struct _wlan_sta_rateset {
	unsigned char	mac[6];
	unsigned char	txrate;
};

struct _misc_data_ {
	unsigned char	mimo_tr_hw_support;
	unsigned char	mimo_tr_used;
	unsigned char	resv[30];
};


/* MIB table */
static struct iwpriv_arg mib_table[] = {
	// struct Dot11RFEntry
	{"RFChipID",	INT_T,		_OFFSET(dot11RFEntry.dot11RFType), _SIZE(dot11RFEntry.dot11RFType), 10},
	{"channel",		INT_T,		_OFFSET(dot11RFEntry.dot11channel), _SIZE(dot11RFEntry.dot11channel), 0},
	{"ch_low",		INT_T,		_OFFSET(dot11RFEntry.dot11ch_low), _SIZE(dot11RFEntry.dot11ch_low), 0},
	{"ch_hi",		INT_T,		_OFFSET(dot11RFEntry.dot11ch_hi), _SIZE(dot11RFEntry.dot11ch_hi), 0},
#ifdef CONFIG_RTL865X
	{"TxPowerCCK",	IDX_BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrlevelCCK), _SIZE(dot11RFEntry.pwrlevelCCK), 0},
#if defined(RTL8190) || defined(RTL8192E)
	{"TxPowerOFDM",	IDX_BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrlevelOFDM), _SIZE(dot11RFEntry.pwrlevelOFDM), 0},
#else	// RTL8192SE
	{"TxPowerOFDM_1SS",	IDX_BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrlevelOFDM_1SS), _SIZE(dot11RFEntry.pwrlevelOFDM_1SS), 0},
	{"TxPowerOFDM_2SS",	IDX_BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrlevelOFDM_2SS), _SIZE(dot11RFEntry.pwrlevelOFDM_2SS), 0},
#endif
#else
	{"TxPowerCCK",	BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrlevelCCK), _SIZE(dot11RFEntry.pwrlevelCCK), 0},
#if defined(RTL8190) || defined(RTL8192E)
	{"TxPowerOFDM",	BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrlevelOFDM), _SIZE(dot11RFEntry.pwrlevelOFDM), 0},
#else	// RTL8192SE
	{"TxPowerOFDM_1SS",	BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrlevelOFDM_1SS), _SIZE(dot11RFEntry.pwrlevelOFDM_1SS), 0},
	{"TxPowerOFDM_2SS",	BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrlevelOFDM_2SS), _SIZE(dot11RFEntry.pwrlevelOFDM_2SS), 0},
#endif
#endif
	{"preamble",	INT_T,		_OFFSET(dot11RFEntry.shortpreamble), _SIZE(dot11RFEntry.shortpreamble), 0},
	{"disable_ch14_ofdm",	INT_T,	_OFFSET(dot11RFEntry.disable_ch14_ofdm), _SIZE(dot11RFEntry.disable_ch14_ofdm), 0},
#if defined(RTL8190)
	{"LOFDM_pwrdiff",	INT_T,	_OFFSET(dot11RFEntry.legacyOFDM_pwrdiff), _SIZE(dot11RFEntry.legacyOFDM_pwrdiff), 0},
	{"antC_pwrdiff",INT_T,	_OFFSET(dot11RFEntry.antC_pwrdiff), _SIZE(dot11RFEntry.antC_pwrdiff), 0},
	{"ther_rfic",	INT_T,	_OFFSET(dot11RFEntry.ther_rfic), _SIZE(dot11RFEntry.ther_rfic), 0},
	{"crystalCap",	INT_T,	_OFFSET(dot11RFEntry.crystalCap), _SIZE(dot11RFEntry.crystalCap), 0},
	{"bw_pwrdiff",	INT_T,	_OFFSET(dot11RFEntry.bw_pwrdiff), _SIZE(dot11RFEntry.bw_pwrdiff), 0},
#elif defined(RTL8192SE) || defined(RTL8192SU)
	{"LOFDM_pwd_A",	INT_T,	_OFFSET(dot11RFEntry.LOFDM_pwd_A), _SIZE(dot11RFEntry.LOFDM_pwd_A), 0},
	{"LOFDM_pwd_B",	INT_T,	_OFFSET(dot11RFEntry.LOFDM_pwd_B), _SIZE(dot11RFEntry.LOFDM_pwd_B), 0},
	{"tssi1",		INT_T,	_OFFSET(dot11RFEntry.tssi1), _SIZE(dot11RFEntry.tssi1), 0},
	{"tssi2",		INT_T,	_OFFSET(dot11RFEntry.tssi2), _SIZE(dot11RFEntry.tssi2), 0},
	{"ther",		INT_T,	_OFFSET(dot11RFEntry.ther), _SIZE(dot11RFEntry.ther), 0},
#endif
#if defined(RTL8192SE)||defined(RTL8192SU)
	{"MIMO_TR_mode",	INT_T,	_OFFSET(dot11RFEntry.MIMO_TR_mode), _SIZE(dot11RFEntry.MIMO_TR_mode), MIMO_1T2R},
#else
	{"MIMO_TR_mode",	INT_T,	_OFFSET(dot11RFEntry.MIMO_TR_mode), _SIZE(dot11RFEntry.MIMO_TR_mode), MIMO_2T4R},
#endif

	// struct Dot11StationConfigEntry
	{"ssid",		SSID_STRING_T,	_OFFSET(dot11StationConfigEntry.dot11DesiredSSID), _SIZE(dot11StationConfigEntry.dot11DesiredSSID), 0},
	{"defssid",		DEF_SSID_STRING_T,	_OFFSET(dot11StationConfigEntry.dot11DefaultSSID), _SIZE(dot11StationConfigEntry.dot11DefaultSSID), 0},
	{"bssid2join",	BYTE_ARRAY_T,	_OFFSET(dot11StationConfigEntry.dot11DesiredBssid), _SIZE(dot11StationConfigEntry.dot11DesiredBssid), 0},
	{"bcnint",		INT_T,		_OFFSET(dot11StationConfigEntry.dot11BeaconPeriod), _SIZE(dot11StationConfigEntry.dot11BeaconPeriod), 100},
	{"dtimperiod",	INT_T,		_OFFSET(dot11StationConfigEntry.dot11DTIMPeriod), _SIZE(dot11StationConfigEntry.dot11DTIMPeriod), 1},
	{"swcrypto",	INT_T,		_OFFSET(dot11StationConfigEntry.dot11swcrypto), _SIZE(dot11StationConfigEntry.dot11swcrypto), 0},
	{"aclmode",		INT_T,		_OFFSET(dot11StationConfigEntry.dot11AclMode), _SIZE(dot11StationConfigEntry.dot11AclMode), 0},
	{"aclnum",		INT_T,		_OFFSET(dot11StationConfigEntry.dot11AclNum), _SIZE(dot11StationConfigEntry.dot11AclNum), 0},
	{"acladdr",		ACL_T,		_OFFSET(dot11StationConfigEntry.dot11AclAddr), _SIZE(dot11StationConfigEntry.dot11AclAddr), 0},
	{"oprates",		INT_T,		_OFFSET(dot11StationConfigEntry.dot11SupportedRates), _SIZE(dot11StationConfigEntry.dot11SupportedRates), 0xfff},
	{"basicrates",	INT_T,		_OFFSET(dot11StationConfigEntry.dot11BasicRates), _SIZE(dot11StationConfigEntry.dot11BasicRates), 0xf},
	{"regdomain",	INT_T,		_OFFSET(dot11StationConfigEntry.dot11RegDomain), _SIZE(dot11StationConfigEntry.dot11RegDomain), 1},
	{"autorate",	INT_T,		_OFFSET(dot11StationConfigEntry.autoRate), _SIZE(dot11StationConfigEntry.autoRate), 1},
	{"fixrate",		INT_T,		_OFFSET(dot11StationConfigEntry.fixedTxRate), _SIZE(dot11StationConfigEntry.fixedTxRate), 0},
#ifdef CONFIG_RTL8671
	{"swTkipMic",	INT_T,		_OFFSET(dot11StationConfigEntry.swTkipMic), _SIZE(dot11StationConfigEntry.swTkipMic), 0},
#else
	{"swTkipMic",	INT_T,		_OFFSET(dot11StationConfigEntry.swTkipMic), _SIZE(dot11StationConfigEntry.swTkipMic), 1},
#endif
	{"disable_protection", INT_T,	_OFFSET(dot11StationConfigEntry.protectionDisabled), _SIZE(dot11StationConfigEntry.protectionDisabled), 0},
	{"disable_olbc", INT_T,		_OFFSET(dot11StationConfigEntry.olbcDetectDisabled), _SIZE(dot11StationConfigEntry.olbcDetectDisabled), 0},
	{"deny_legacy",	INT_T,		_OFFSET(dot11StationConfigEntry.legacySTADeny), _SIZE(dot11StationConfigEntry.legacySTADeny), 0},
#ifdef CLIENT_MODE
	{"fast_roaming", INT_T,		_OFFSET(dot11StationConfigEntry.fastRoaming), _SIZE(dot11StationConfigEntry.fastRoaming), 0},
#endif
	{"lowestMlcstRate", INT_T,	_OFFSET(dot11StationConfigEntry.lowestMlcstRate), _SIZE(dot11StationConfigEntry.lowestMlcstRate), 0},
	{"stanum",		INT_T,		_OFFSET(dot11StationConfigEntry.supportedStaNum), _SIZE(dot11StationConfigEntry.supportedStaNum), 0},

#ifdef	CONFIG_RTK_MESH
	{"mesh_enable",			BYTE_T,			_OFFSET(dot1180211sInfo.mesh_enable),			_SIZE(dot1180211sInfo.mesh_enable),			0},
	{"mesh_root_enable",	BYTE_T,			_OFFSET(dot1180211sInfo.mesh_root_enable),		_SIZE(dot1180211sInfo.mesh_root_enable),	0},
	{"mesh_ap_enable",		BYTE_T,			_OFFSET(dot1180211sInfo.mesh_ap_enable),		_SIZE(dot1180211sInfo.mesh_ap_enable),		0},
	{"mesh_portal_enable",	BYTE_T,			_OFFSET(dot1180211sInfo.mesh_portal_enable),	_SIZE(dot1180211sInfo.mesh_portal_enable),	0},
	{"mesh_id",				STRING_T,		_OFFSET(dot1180211sInfo.mesh_id),				_SIZE(dot1180211sInfo.mesh_id),				0},
	{"mesh_max_neightbor",	WORD_T,			_OFFSET(dot1180211sInfo.mesh_max_neightbor),	_SIZE(dot1180211sInfo.mesh_max_neightbor),	0},

	{"log_enabled",			BYTE_T,			_OFFSET(dot1180211sInfo.log_enabled),		_SIZE(dot1180211sInfo.log_enabled),		0},
	{"mesh_privacy",		INT_T,		_OFFSET(dot11sKeysTable.dot11Privacy),	_SIZE(dot11sKeysTable.dot11Privacy),		0},

#ifdef _MESH_ACL_ENABLE_
	{"meshaclmode", 		INT_T,			_OFFSET(dot1180211sInfo.mesh_acl_mode), 		_SIZE(dot1180211sInfo.mesh_acl_mode),		0},
	{"meshaclnum",			INT_T,			_OFFSET(dot1180211sInfo.mesh_acl_num), 			_SIZE(dot1180211sInfo.mesh_acl_num),		0},
	{"meshacladdr", 		ACL_T,			_OFFSET(dot1180211sInfo.mesh_acl_addr), 		_SIZE(dot1180211sInfo.mesh_acl_addr),		0},
#endif

#ifdef 	_11s_TEST_MODE_	
	{"mesh_reserved1",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reserved1),	_SIZE(dot1180211sInfo.mesh_reserved1),		0},
	{"mesh_reserved2",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reserved2),	_SIZE(dot1180211sInfo.mesh_reserved2),		0},
	{"mesh_reserved3",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reserved3),	_SIZE(dot1180211sInfo.mesh_reserved3),		0},
	{"mesh_reserved4",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reserved4),	_SIZE(dot1180211sInfo.mesh_reserved4),		0},
	{"mesh_reserved5",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reserved5),	_SIZE(dot1180211sInfo.mesh_reserved5),		0},
	{"mesh_reserved6",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reserved6),	_SIZE(dot1180211sInfo.mesh_reserved6),		0},
	{"mesh_reserved7",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reserved7),	_SIZE(dot1180211sInfo.mesh_reserved7),		0},
	{"mesh_reserved8",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reserved8),	_SIZE(dot1180211sInfo.mesh_reserved8),		0},
	{"mesh_reserved9",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reserved9),	_SIZE(dot1180211sInfo.mesh_reserved9),		0},
	{"mesh_reserveda",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reserveda),	_SIZE(dot1180211sInfo.mesh_reserveda),		0},
	{"mesh_reservedb",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reservedb),	_SIZE(dot1180211sInfo.mesh_reservedb),		0},
	{"mesh_reservedc",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reservedc),	_SIZE(dot1180211sInfo.mesh_reservedc),		0},
	{"mesh_reservedd",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reservedd),	_SIZE(dot1180211sInfo.mesh_reservedd),		0},
	{"mesh_reservede",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reservede),	_SIZE(dot1180211sInfo.mesh_reservede),		0},
	{"mesh_reservedf",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reservedf),	_SIZE(dot1180211sInfo.mesh_reservedf),		0},		
	{"mesh_reservedstr1",		STRING_T,		_OFFSET(dot1180211sInfo.mesh_reservedstr1),	_SIZE(dot1180211sInfo.mesh_reservedstr1),	0},
#endif

#endif

	// struct Dot1180211AuthEntry
#if defined(RTL8192SE) || defined(RTL8192SU) // temporary solution
	{"authtype",	INT_T,		_OFFSET(dot1180211AuthEntry.dot11AuthAlgrthm), _SIZE(dot1180211AuthEntry.dot11AuthAlgrthm), 0},
#else
	{"authtype",	INT_T,		_OFFSET(dot1180211AuthEntry.dot11AuthAlgrthm), _SIZE(dot1180211AuthEntry.dot11AuthAlgrthm), 2},
#endif
	{"encmode",		INT_T,		_OFFSET(dot1180211AuthEntry.dot11PrivacyAlgrthm), _SIZE(dot1180211AuthEntry.dot11PrivacyAlgrthm), 0},
	{"wepdkeyid",	INT_T,		_OFFSET(dot1180211AuthEntry.dot11PrivacyKeyIndex), _SIZE(dot1180211AuthEntry.dot11PrivacyKeyIndex), 0},
#ifdef INCLUDE_WPA_PSK
	{"psk_enable",	INT_T,		_OFFSET(dot1180211AuthEntry.dot11EnablePSK), _SIZE(dot1180211AuthEntry.dot11EnablePSK), 0},
	{"wpa_cipher",	INT_T,		_OFFSET(dot1180211AuthEntry.dot11WPACipher), _SIZE(dot1180211AuthEntry.dot11WPACipher), 0},
#ifdef RTL_WPA2
	{"wpa2_cipher",	INT_T,		_OFFSET(dot1180211AuthEntry.dot11WPA2Cipher), _SIZE(dot1180211AuthEntry.dot11WPA2Cipher), 0},
#endif
	{"passphrase",	STRING_T,	_OFFSET(dot1180211AuthEntry.dot11PassPhrase), _SIZE(dot1180211AuthEntry.dot11PassPhrase), 0},
#ifdef CONFIG_RTL8186_KB
	{"passphrase_guest", STRING_T, _OFFSET(dot1180211AuthEntry.dot11PassPhraseGuest), _SIZE(dot1180211AuthEntry.dot11PassPhraseGuest), 0},
#endif
	{"gk_rekey",	INT_T,		_OFFSET(dot1180211AuthEntry.dot11GKRekeyTime), _SIZE(dot1180211AuthEntry.dot11GKRekeyTime), 0},
#endif

	// struct Dot118021xAuthEntry
	{"802_1x",		INT_T,		_OFFSET(dot118021xAuthEntry.dot118021xAlgrthm), _SIZE(dot118021xAuthEntry.dot118021xAlgrthm), 0},
	{"default_port",INT_T,		_OFFSET(dot118021xAuthEntry.dot118021xDefaultPort), _SIZE(dot118021xAuthEntry.dot118021xDefaultPort), 0},

	// struct Dot11DefaultKeysTable
	{"wepkey1",		BYTE_ARRAY_T,	_OFFSET(dot11DefaultKeysTable.keytype[0]), _SIZE(dot11DefaultKeysTable.keytype[0]), 0},
	{"wepkey2",		BYTE_ARRAY_T,	_OFFSET(dot11DefaultKeysTable.keytype[1]), _SIZE(dot11DefaultKeysTable.keytype[1]), 0},
	{"wepkey3",		BYTE_ARRAY_T,	_OFFSET(dot11DefaultKeysTable.keytype[2]), _SIZE(dot11DefaultKeysTable.keytype[2]), 0},
	{"wepkey4",		BYTE_ARRAY_T,	_OFFSET(dot11DefaultKeysTable.keytype[3]), _SIZE(dot11DefaultKeysTable.keytype[3]), 0},

	// struct Dot11OperationEntry
	{"opmode",		INT_T,		_OFFSET(dot11OperationEntry.opmode), _SIZE(dot11OperationEntry.opmode), 0x10},
	{"hiddenAP",	INT_T,		_OFFSET(dot11OperationEntry.hiddenAP), _SIZE(dot11OperationEntry.hiddenAP), 0},
	{"rtsthres",	INT_T,		_OFFSET(dot11OperationEntry.dot11RTSThreshold), _SIZE(dot11OperationEntry.dot11RTSThreshold), 2347},
	{"fragthres",	INT_T,		_OFFSET(dot11OperationEntry.dot11FragmentationThreshold), _SIZE(dot11OperationEntry.dot11FragmentationThreshold), 2347},
	{"shortretry",	INT_T,		_OFFSET(dot11OperationEntry.dot11ShortRetryLimit), _SIZE(dot11OperationEntry.dot11ShortRetryLimit), 0},
	{"longretry",	INT_T,		_OFFSET(dot11OperationEntry.dot11LongRetryLimit), _SIZE(dot11OperationEntry.dot11LongRetryLimit), 0},
	{"expired_time",INT_T,		_OFFSET(dot11OperationEntry.expiretime), _SIZE(dot11OperationEntry.expiretime), 30000},
	{"led_type",	INT_T,		_OFFSET(dot11OperationEntry.ledtype), _SIZE(dot11OperationEntry.ledtype), 0},
#ifdef RTL8190_SWGPIO_LED
	{"led_route",	INT_T,		_OFFSET(dot11OperationEntry.ledroute), _SIZE(dot11OperationEntry.ledroute), 0},
#endif
	{"iapp_enable",	INT_T,		_OFFSET(dot11OperationEntry.iapp_enable), _SIZE(dot11OperationEntry.iapp_enable), 0},
	{"block_relay",	INT_T,		_OFFSET(dot11OperationEntry.block_relay), _SIZE(dot11OperationEntry.block_relay), 0},
	{"deny_any",	INT_T,		_OFFSET(dot11OperationEntry.deny_any), _SIZE(dot11OperationEntry.deny_any), 0},
	{"crc_log",		INT_T,		_OFFSET(dot11OperationEntry.crc_log), _SIZE(dot11OperationEntry.crc_log), 0},
	{"wifi_specific",INT_T,		_OFFSET(dot11OperationEntry.wifi_specific), _SIZE(dot11OperationEntry.wifi_specific), 0},
#ifdef TX_SHORTCUT
	{"disable_txsc",INT_T,		_OFFSET(dot11OperationEntry.disable_txsc), _SIZE(dot11OperationEntry.disable_txsc), 0},
#endif
#ifdef RX_SHORTCUT
	{"disable_rxsc",INT_T,		_OFFSET(dot11OperationEntry.disable_rxsc), _SIZE(dot11OperationEntry.disable_rxsc), 0},
#endif
#ifdef BR_SHORTCUT
	{"disable_brsc",INT_T,		_OFFSET(dot11OperationEntry.disable_brsc), _SIZE(dot11OperationEntry.disable_brsc), 0},
#endif
	{"keep_rsnie",	INT_T,		_OFFSET(dot11OperationEntry.keep_rsnie), _SIZE(dot11OperationEntry.keep_rsnie), 0},
	{"guest_access",INT_T,		_OFFSET(dot11OperationEntry.guest_access), _SIZE(dot11OperationEntry.guest_access), 0},

	// struct bss_type
#ifdef CONFIG_RTL8671
	{"band",		BYTE_T,		_OFFSET(dot11BssType.net_work_type), _SIZE(dot11BssType.net_work_type), 11},
#else
	{"band",		BYTE_T,		_OFFSET(dot11BssType.net_work_type), _SIZE(dot11BssType.net_work_type), 3},
#endif

	// struct erp_mib
	{"cts2self",	INT_T,		_OFFSET(dot11ErpInfo.ctsToSelf), _SIZE(dot11ErpInfo.ctsToSelf), 1},

#ifdef WDS
	// struct wds_info
	{"wds_enable",	INT_T,		_OFFSET(dot11WdsInfo.wdsEnabled), _SIZE(dot11WdsInfo.wdsEnabled), 0},
	{"wds_pure",	INT_T,		_OFFSET(dot11WdsInfo.wdsPure), _SIZE(dot11WdsInfo.wdsPure), 0},
	{"wds_priority",INT_T,		_OFFSET(dot11WdsInfo.wdsPriority), _SIZE(dot11WdsInfo.wdsPriority), 0},
	{"wds_num",		INT_T,		_OFFSET(dot11WdsInfo.wdsNum), _SIZE(dot11WdsInfo.wdsNum), 0},
	{"wds_add",		ACL_INT_T,		_OFFSET(dot11WdsInfo.entry), _SIZE(dot11WdsInfo.entry), 0},
	{"wds_encrypt",	INT_T,		_OFFSET(dot11WdsInfo.wdsPrivacy), _SIZE(dot11WdsInfo.wdsPrivacy), 0},
	{"wds_wepkey",	BYTE_ARRAY_T, _OFFSET(dot11WdsInfo.wdsWepKey), _SIZE(dot11WdsInfo.wdsWepKey), 0},
#ifdef INCLUDE_WPA_PSK
	{"wds_passphrase",	STRING_T, _OFFSET(dot11WdsInfo.wdsPskPassPhrase), _SIZE(dot11WdsInfo.wdsPskPassPhrase), 0},
#endif
#endif

#ifdef RTK_BR_EXT
	// struct br_ext_info
	{"nat25_disable",		INT_T,	_OFFSET(ethBrExtInfo.nat25_disable), _SIZE(ethBrExtInfo.nat25_disable), 0},
	{"macclone_enable",		INT_T,	_OFFSET(ethBrExtInfo.macclone_enable), _SIZE(ethBrExtInfo.macclone_enable), 0},
	{"dhcp_bcst_disable",	INT_T,	_OFFSET(ethBrExtInfo.dhcp_bcst_disable), _SIZE(ethBrExtInfo.dhcp_bcst_disable), 0},
	{"add_pppoe_tag",		INT_T,	_OFFSET(ethBrExtInfo.addPPPoETag), _SIZE(ethBrExtInfo.addPPPoETag), 1},
#ifdef CONFIG_RTL865X
	{"nat25_dmzMac",		BYTE_ARRAY_T,	_OFFSET(ethBrExtInfo.nat25_dmzMac), _SIZE(ethBrExtInfo.nat25_dmzMac), 0},
#endif
	{"clone_mac_addr",		BYTE_ARRAY_T,	_OFFSET(ethBrExtInfo.nat25_dmzMac), _SIZE(ethBrExtInfo.nat25_dmzMac), 0},
	{"nat25sc_disable",		INT_T,	_OFFSET(ethBrExtInfo.nat25sc_disable), _SIZE(ethBrExtInfo.nat25sc_disable), 0},
#endif

#ifdef DFS
	//struct Dot11DFSEntry
	{"disable_DFS",	INT_T,		_OFFSET(dot11DFSEntry.disable_DFS), _SIZE(dot11DFSEntry.disable_DFS), 0},
	{"disable_tx",	INT_T,		_OFFSET(dot11DFSEntry.disable_tx), _SIZE(dot11DFSEntry.disable_tx), 0},
	{"DFS_timeout",	INT_T,		_OFFSET(dot11DFSEntry.DFS_timeout), _SIZE(dot11DFSEntry.DFS_timeout), 1},
	{"DFS_detected",INT_T,		_OFFSET(dot11DFSEntry.DFS_detected), _SIZE(dot11DFSEntry.DFS_detected), 0},
	{"NOP_timeout",	INT_T,		_OFFSET(dot11DFSEntry.NOP_timeout), _SIZE(dot11DFSEntry.NOP_timeout), 180500},
	{"rs1_threshold",INT_T,		_OFFSET(dot11DFSEntry.rs1_threshold), _SIZE(dot11DFSEntry.rs1_threshold), 8},
	{"Throughput_Threshold",INT_T,	_OFFSET(dot11DFSEntry.Throughput_Threshold), _SIZE(dot11DFSEntry.Throughput_Threshold), 15},
	{"RecordHistory_sec",	INT_T,	_OFFSET(dot11DFSEntry.RecordHistory_sec), _SIZE(dot11DFSEntry.RecordHistory_sec), 1},
	{"temply_disable_DFS",	INT_T,	_OFFSET(dot11DFSEntry.temply_disable_DFS), _SIZE(dot11DFSEntry.temply_disable_DFS), 0},
	{"Dump_Throughput",		INT_T,	_OFFSET(dot11DFSEntry.Dump_Throughput), _SIZE(dot11DFSEntry.Dump_Throughput), 0},
	{"disable_DtmDFS",		INT_T,	_OFFSET(dot11DFSEntry.disable_DetermineDFSDisable), _SIZE(dot11DFSEntry.disable_DetermineDFSDisable), 0},
#endif

	//struct miscEntry
	{"show_hidden_bss",INT_T,	_OFFSET(miscEntry.show_hidden_bss), _SIZE(miscEntry.show_hidden_bss), 0},
	{"ack_timeout",	INT_T,		_OFFSET(miscEntry.ack_timeout), _SIZE(miscEntry.ack_timeout), 0},
	{"tx_priority",	INT_T,		_OFFSET(miscEntry.tx_priority), _SIZE(miscEntry.tx_priority), 0},
#ifdef A4_TUNNEL
	{"a4tnl_enable",INT_T,		_OFFSET(miscEntry.a4tnl_enable), _SIZE(miscEntry.a4tnl_enable), 0},
#endif
	{"private_ie",	VARLEN_BYTE_T,	_OFFSET(miscEntry.private_ie), _SIZE(miscEntry.private_ie), 0},
#ifdef CONFIG_RTL865X
	{"set_vlanid",			INT_T,	_OFFSET(miscEntry.set_vlanid), _SIZE(miscEntry.set_vlanid), 9},
#endif
	{"rxInt",		INT_T,		_OFFSET(miscEntry.rxInt_thrd), _SIZE(miscEntry.rxInt_thrd), 300},
#ifdef DRVMAC_LB
	{"dmlb",		INT_T,		_OFFSET(miscEntry.drvmac_lb), _SIZE(miscEntry.drvmac_lb), 0},
	{"lb_da",		BYTE_ARRAY_T,	_OFFSET(miscEntry.lb_da), _SIZE(miscEntry.lb_da), 0},
#endif
	{"groupID",		INT_T,		_OFFSET(miscEntry.groupID), _SIZE(miscEntry.groupID), 0},
#if (defined(RTL8192SE)||defined(RTL8192SU)) && defined(MBSSID)
	{"vap_enable",	INT_T,		_OFFSET(miscEntry.vap_enable), _SIZE(miscEntry.vap_enable), 0},
#endif
#ifdef RTL8192SU	
	{"num_sta",	INT_T,		_OFFSET(miscEntry.num_sta), _SIZE(miscEntry.num_sta), NUM_STAT},
#ifdef RTL8192SU_FWBCN
	{"intf_map",	INT_T,		_OFFSET(miscEntry.intf_map), _SIZE(miscEntry.intf_map), 1},
#endif
#endif

	{"func_off",	INT_T,		_OFFSET(miscEntry.func_off), _SIZE(miscEntry.func_off), 0},

	//struct Dot11QosEntry
#ifdef SEMI_QOS
	{"qos_enable",	INT_T,		_OFFSET(dot11QosEntry.dot11QosEnable), _SIZE(dot11QosEntry.dot11QosEnable), 1},	//default enable for AMPDU
#ifdef WMM_APSD
	{"apsd_enable",	INT_T,		_OFFSET(dot11QosEntry.dot11QosAPSD), _SIZE(dot11QosEntry.dot11QosAPSD), 0},
#endif
#endif

#ifdef WIFI_SIMPLE_CONFIG
	// struct WifiSimpleConfigEntry
	{"wsc_enable",	INT_T,	_OFFSET(wscEntry.wsc_enable), _SIZE(wscEntry.wsc_enable), 0},
	{"pin",			PIN_IND_T, 0, 0},
#endif

#ifdef GBWC
	// struct GroupBandWidthControl
	{"gbwcmode",	INT_T,		_OFFSET(gbwcEntry.GBWCMode), _SIZE(gbwcEntry.GBWCMode), 0},
	{"gbwcnum",		INT_T,		_OFFSET(gbwcEntry.GBWCNum), _SIZE(gbwcEntry.GBWCNum), 0},
	{"gbwcaddr",	ACL_T,		_OFFSET(gbwcEntry.GBWCAddr), _SIZE(gbwcEntry.GBWCAddr), 0},
	{"gbwcthrd",	INT_T,		_OFFSET(gbwcEntry.GBWCThrd), _SIZE(gbwcEntry.GBWCThrd), 30000},
#endif

	// struct Dot11nConfigEntry
	{"supportedmcs",INT_T,		_OFFSET(dot11nConfigEntry.dot11nSupportedMCS), _SIZE(dot11nConfigEntry.dot11nSupportedMCS), 0xffff},
	{"basicmcs",	INT_T,		_OFFSET(dot11nConfigEntry.dot11nBasicMCS), _SIZE(dot11nConfigEntry.dot11nBasicMCS), 0},
	{"use40M",		INT_T,		_OFFSET(dot11nConfigEntry.dot11nUse40M), _SIZE(dot11nConfigEntry.dot11nUse40M), 1},
	{"2ndchoffset",	INT_T,		_OFFSET(dot11nConfigEntry.dot11n2ndChOffset), _SIZE(dot11nConfigEntry.dot11n2ndChOffset), 2},
	{"shortGI20M",	INT_T,		_OFFSET(dot11nConfigEntry.dot11nShortGIfor20M), _SIZE(dot11nConfigEntry.dot11nShortGIfor20M), 0},
	{"shortGI40M",	INT_T,		_OFFSET(dot11nConfigEntry.dot11nShortGIfor40M), _SIZE(dot11nConfigEntry.dot11nShortGIfor40M), 0},
	{"txStbc",		INT_T,		_OFFSET(dot11nConfigEntry.dot11nTxSTBC), _SIZE(dot11nConfigEntry.dot11nTxSTBC), 0},
	{"ampdu",		INT_T,		_OFFSET(dot11nConfigEntry.dot11nAMPDU), _SIZE(dot11nConfigEntry.dot11nAMPDU), 1},
	{"amsdu",		INT_T,		_OFFSET(dot11nConfigEntry.dot11nAMSDU), _SIZE(dot11nConfigEntry.dot11nAMSDU), 0},
	{"ampduSndSz",	INT_T,		_OFFSET(dot11nConfigEntry.dot11nAMPDUSendSz), _SIZE(dot11nConfigEntry.dot11nAMPDUSendSz), 0},
#if defined(CONFIG_RTL8196B_GW_8M) || defined(RTL8192SU) /*RTL8192SE_ACUT*/ // 8196B patch need lots of spaces
	{"amsduMax",	INT_T,		_OFFSET(dot11nConfigEntry.dot11nAMSDURecvMax), _SIZE(dot11nConfigEntry.dot11nAMSDURecvMax), 0},
#else
	{"amsduMax",	INT_T,		_OFFSET(dot11nConfigEntry.dot11nAMSDURecvMax), _SIZE(dot11nConfigEntry.dot11nAMSDURecvMax), 1},
#endif
	{"amsduTimeout",INT_T,		_OFFSET(dot11nConfigEntry.dot11nAMSDUSendTimeout), _SIZE(dot11nConfigEntry.dot11nAMSDUSendTimeout), 400},
	{"amsduNum",	INT_T,		_OFFSET(dot11nConfigEntry.dot11nAMSDUSendNum), _SIZE(dot11nConfigEntry.dot11nAMSDUSendNum), 15},
	{"lgyEncRstrct",INT_T,		_OFFSET(dot11nConfigEntry.dot11nLgyEncRstrct), _SIZE(dot11nConfigEntry.dot11nLgyEncRstrct), 0},

	// struct ReorderControlEntry
	{"rc_enable",	INT_T,		_OFFSET(reorderCtrlEntry.ReorderCtrlEnable), _SIZE(reorderCtrlEntry.ReorderCtrlEnable), 1},
	{"rc_winsz",	INT_T,		_OFFSET(reorderCtrlEntry.ReorderCtrlWinSz), _SIZE(reorderCtrlEntry.ReorderCtrlWinSz), RC_ENTRY_NUM},
	{"rc_timeout",	INT_T,		_OFFSET(reorderCtrlEntry.ReorderCtrlTimeout), _SIZE(reorderCtrlEntry.ReorderCtrlTimeout), 30000},

#ifdef CONFIG_RTK_VLAN_SUPPORT
	// struct VlanConfig
	{"global_vlan",	INT_T,		_OFFSET(vlan.global_vlan), _SIZE(vlan.global_vlan), 0},
	{"is_lan",	INT_T,		_OFFSET(vlan.is_lan), _SIZE(vlan.is_lan), 1},
	{"vlan_enable",	INT_T,		_OFFSET(vlan.vlan_enable), _SIZE(vlan.vlan_enable), 0},
	{"vlan_tag",	INT_T,		_OFFSET(vlan.vlan_tag), _SIZE(vlan.vlan_tag), 0},
	{"vlan_id",	INT_T,		_OFFSET(vlan.vlan_id), _SIZE(vlan.vlan_id), 0},
	{"vlan_pri",	INT_T,		_OFFSET(vlan.vlan_pri), _SIZE(vlan.vlan_pri), 0},
	{"vlan_cfi",	INT_T,		_OFFSET(vlan.vlan_cfi), _SIZE(vlan.vlan_cfi), 0},
#endif

#ifdef  CONFIG_RTL_WAPI_SUPPORT
	{"wapiType",		INT_T,	_OFFSET(wapiInfo.wapiType), _SIZE(wapiInfo.wapiType), 0},
#ifdef  WAPI_SUPPORT_MULTI_ENCRYPT
	{"wapiUCastEncodeType",		INT_T,	_OFFSET(wapiInfo.wapiUCastEncodeType), _SIZE(wapiInfo.wapiUCastEncodeType), 0},
	{"wapiMCastEncodeType",		INT_T,	_OFFSET(wapiInfo.wapiMCastEncodeType), _SIZE(wapiInfo.wapiMCastEncodeType), 0},
#endif
	{"wapiPsk",	WAPI_KEY_T,	_OFFSET(wapiInfo.wapiPsk), _SIZE(wapiInfo.wapiPsk), 0},
	{"wapiPsklen",	INT_T,	_OFFSET(wapiInfo.wapiPsk.len), _SIZE(wapiInfo.wapiPsk.len), 0},
	{"wapiUCastKeyType",		INT_T,	_OFFSET(wapiInfo.wapiUpdateUCastKeyType), _SIZE(wapiInfo.wapiUpdateUCastKeyType), 0},
	{"wapiUCastKeyTimeout",		INT_T,	_OFFSET(wapiInfo.wapiUpdateUCastKeyTimeout), _SIZE(wapiInfo.wapiUpdateUCastKeyTimeout), 0},
	{"wapiUCastKeyPktNum",		INT_T,	_OFFSET(wapiInfo.wapiUpdateUCastKeyPktNum), _SIZE(wapiInfo.wapiUpdateUCastKeyPktNum), 0},
	{"wapiMCastKeyType",		INT_T,	_OFFSET(wapiInfo.wapiUpdateMCastKeyType), _SIZE(wapiInfo.wapiUpdateMCastKeyType), 0},
	{"wapiMCastKeyTimeout",		INT_T,	_OFFSET(wapiInfo.wapiUpdateMCastKeyTimeout), _SIZE(wapiInfo.wapiUpdateMCastKeyTimeout), 0},
	{"wapiMCastKeyPktNum",		INT_T,	_OFFSET(wapiInfo.wapiUpdateMCastKeyPktNum), _SIZE(wapiInfo.wapiUpdateMCastKeyPktNum), 0},
	{"wapiTimeout",		INT_ARRAY_T,	_OFFSET(wapiInfo.wapiTimeout), _SIZE(wapiInfo.wapiTimeout), 0},
#endif

#ifdef _DEBUG_RTL8190_
	// debug flag
	{"debug_err",	DEBUG_T,	1, sizeof(rtl8190_debug_err), 0},
	{"debug_info",	DEBUG_T,	2, sizeof(rtl8190_debug_info), 0},
	{"debug_trace",	DEBUG_T,	3, sizeof(rtl8190_debug_trace), 0},
	{"debug_warn",	DEBUG_T,	4, sizeof(rtl8190_debug_warn), 0},
#endif

	// for RF debug
#if defined(RTL8192SE) || defined(RTL8192SU)
	{"ofdm_1ss_oneAnt",	RFFT_T,		_OFFSET_RFFT(ofdm_1ss_oneAnt), _SIZE_RFFT(ofdm_1ss_oneAnt), 0},// 1ss and ofdm rate using one ant
	{"pathB_1T", RFFT_T, _OFFSET_RFFT(pathB_1T), _SIZE_RFFT(pathB_1T), 0},// using pathB as 1T2R/1T1R tx path
#ifdef EXT_ANT_DVRY
	{"ext_ad_th",		RFFT_T,		_OFFSET_RFFT(ext_ad_th), _SIZE_RFFT(ext_ad_th), 5},
	{"ext_ad_Ttry",		RFFT_T,		_OFFSET_RFFT(ext_ad_Ttry), _SIZE_RFFT(ext_ad_Ttry), 10},
	{"ext_ad_Ts",		RFFT_T,		_OFFSET_RFFT(ext_ad_Ts), _SIZE_RFFT(ext_ad_Ts), 30},
	{"ExtAntDvry",		RFFT_T,		_OFFSET_RFFT(ExtAntDvry), _SIZE_RFFT(ExtAntDvry), 1},
#endif
#endif
	{"rssi_dump",		RFFT_T,		_OFFSET_RFFT(rssi_dump), _SIZE_RFFT(rssi_dump), 0},
	{"rxfifoO",			RFFT_T,		_OFFSET_RFFT(rxfifoO), _SIZE_RFFT(rxfifoO), 0},
	{"raGoDownUpper",	RFFT_T,	_OFFSET_RFFT(raGoDownUpper), _SIZE_RFFT(raGoDownUpper), 50},
#if defined(RTL8190) || defined(RTL8192E)
	{"raGoDown20MLower",RFFT_T,	_OFFSET_RFFT(raGoDown20MLower), _SIZE_RFFT(raGoDown20MLower), 30},
#else // RTL8192SE, RTL8192SU
	{"raGoDown20MLower",RFFT_T,	_OFFSET_RFFT(raGoDown20MLower), _SIZE_RFFT(raGoDown20MLower), 18},
#endif
	{"raGoDown40MLower",RFFT_T,	_OFFSET_RFFT(raGoDown40MLower), _SIZE_RFFT(raGoDown40MLower), 10},
	{"raGoUpUpper",		RFFT_T,	_OFFSET_RFFT(raGoUpUpper), _SIZE_RFFT(raGoUpUpper), 55},
#if defined(RTL8190) || defined(RTL8192E)
	{"raGoUp20MLower",	RFFT_T,	_OFFSET_RFFT(raGoUp20MLower), _SIZE_RFFT(raGoUp20MLower), 35},
#else // RTL8192SE, RTL8192SU
	{"raGoUp20MLower",	RFFT_T,	_OFFSET_RFFT(raGoUp20MLower), _SIZE_RFFT(raGoUp20MLower), 23},
#endif
	{"raGoUp40MLower",	RFFT_T,	_OFFSET_RFFT(raGoUp40MLower), _SIZE_RFFT(raGoUp40MLower), 15},
	{"digGoLowerLevel",	RFFT_T,	_OFFSET_RFFT(digGoLowerLevel), _SIZE_RFFT(digGoLowerLevel), 35},
	{"digGoUpperLevel",	RFFT_T,	_OFFSET_RFFT(digGoUpperLevel), _SIZE_RFFT(digGoUpperLevel), 40},
#if defined(RTL8190) || defined(RTL8192E)
	{"dcThUpper",		RFFT_T,	_OFFSET_RFFT(dcThUpper), _SIZE_RFFT(dcThUpper), 25},
	{"dcThLower",		RFFT_T,	_OFFSET_RFFT(dcThLower), _SIZE_RFFT(dcThLower), 20},
#elif defined(RTL8192SE) || defined(RTL8192SU)
	{"dcThUpper",		RFFT_T,	_OFFSET_RFFT(dcThUpper), _SIZE_RFFT(dcThUpper), 30},
	{"dcThLower",		RFFT_T,	_OFFSET_RFFT(dcThLower), _SIZE_RFFT(dcThLower), 25},
#endif
	{"rssiTx20MUpper",	RFFT_T,	_OFFSET_RFFT(rssiTx20MUpper), _SIZE_RFFT(rssiTx20MUpper), 20},
	{"rssiTx20MLower",	RFFT_T,	_OFFSET_RFFT(rssiTx20MLower), _SIZE_RFFT(rssiTx20MLower), 15},
	{"ss_th_low",		RFFT_T,	_OFFSET_RFFT(ss_th_low), _SIZE_RFFT(ss_th_low), 30},
	{"diff_th",			RFFT_T,	_OFFSET_RFFT(diff_th), _SIZE_RFFT(diff_th), 18},
	{"cck_sel_ver",		RFFT_T,	_OFFSET_RFFT(cck_sel_ver), _SIZE_RFFT(cck_sel_ver), 1},
	{"cck_accu_num",	RFFT_T,	_OFFSET_RFFT(cck_accu_num), _SIZE_RFFT(cck_accu_num), 100},
	{"rssi_expire_to",	RFFT_T,	_OFFSET_RFFT(rssi_expire_to), _SIZE_RFFT(rssi_expire_to), 60},

	// bcm old 11n chipset iot debug
#if defined(RTL8190) || defined(RTL8192E)
	{"fsync_func_on",	RFFT_T,	_OFFSET_RFFT(fsync_func_on), _SIZE_RFFT(fsync_func_on), 1},
	{"fsync_mcs_th",	RFFT_T,	_OFFSET_RFFT(fsync_mcs_th), _SIZE_RFFT(fsync_mcs_th), 5},
	{"fsync_rssi_th",	RFFT_T,	_OFFSET_RFFT(fsync_rssi_th), _SIZE_RFFT(fsync_rssi_th), 30},
	{"mcs_ignore_upper",RFFT_T,	_OFFSET_RFFT(mcs_ignore_upper), _SIZE_RFFT(mcs_ignore_upper), 11},
	{"mcs_ignore_lower",RFFT_T,	_OFFSET_RFFT(mcs_ignore_lower), _SIZE_RFFT(mcs_ignore_lower), 8},
#endif

	{"igUpperBound",	RFFT_T,	_OFFSET_RFFT(mlcstRxIgUpperBound), _SIZE_RFFT(mlcstRxIgUpperBound), 0x42},
	{"tx_pwr_ctrl",		RFFT_T,	_OFFSET_RFFT(tx_pwr_ctrl), _SIZE_RFFT(tx_pwr_ctrl), 1},

	// 11n ap AES debug
	{"aes_check_th",	RFFT_T,	_OFFSET_RFFT(aes_check_th), _SIZE_RFFT(aes_check_th), 2},

	// dynamic CCK Tx power by rssi
	{"cck_enhance",		RFFT_T,	_OFFSET_RFFT(cck_enhance), _SIZE_RFFT(cck_enhance), 1},

	// Tx power tracking
	{"tpt_period",		RFFT_T,	_OFFSET_RFFT(tpt_period), _SIZE_RFFT(tpt_period), 30},

	// TXOP enlarge
	{"txop_enlarge_upper",		RFFT_T,	_OFFSET_RFFT(txop_enlarge_upper), _SIZE_RFFT(txop_enlarge_upper), 20},
	{"txop_enlarge_lower",		RFFT_T,	_OFFSET_RFFT(txop_enlarge_lower), _SIZE_RFFT(txop_enlarge_lower), 15},

	// 2.3G support
	{"frq_2_3G",		RFFT_T,	_OFFSET_RFFT(use_frq_2_3G), _SIZE_RFFT(use_frq_2_3G), 0},

	// for mp test
#ifdef MP_TEST
	{"mp_specific",		RFFT_T,	_OFFSET_RFFT(mp_specific), _SIZE_RFFT(mp_specific), 0},
#endif

#ifdef IGMP_FILTER_CMO
	{"igmp_deny",		RFFT_T,	_OFFSET_RFFT(igmp_deny), _SIZE_RFFT(igmp_deny), 0},
#endif
	//Support IP multicast->unicast
#ifdef SUPPORT_TX_MCAST2UNI
	{"mc2u_disable",	RFFT_T,	_OFFSET_RFFT(mc2u_disable), _SIZE_RFFT(mc2u_disable), 0},
#endif

#ifdef BUFFER_TX_AMPDU
	{"ampduMax",		RFFT_T,	_OFFSET_RFFT(dot11nAMPDUBufferMax), _SIZE_RFFT(dot11nAMPDUBufferMax), 12000},
	{"ampduTimeout",	RFFT_T,	_OFFSET_RFFT(dot11nAMPDUBufferTimeout), _SIZE_RFFT(dot11nAMPDUBufferTimeout), 400},
	{"ampduNum",		RFFT_T,	_OFFSET_RFFT(dot11nAMPDUBufferNum), _SIZE_RFFT(dot11nAMPDUBufferNum), 24},
#endif

#ifdef WIFI_11N_2040_COEXIST
	{"coexist_enable",	RFFT_T,	_OFFSET_RFFT(coexist_enable), _SIZE_RFFT(coexist_enable), 0},
#endif

#ifdef HIGH_POWER_EXT_PA
	{"use_ext_pa",		RFFT_T,	_OFFSET_RFFT(use_ext_pa), _SIZE_RFFT(use_ext_pa), 0},
#endif

#ifdef ADD_TX_POWER_BY_CMD
	{"txPowerPlus_cck",		RFFT_T,	_OFFSET_RFFT(txPowerPlus_cck), _SIZE_RFFT(txPowerPlus_cck), 0xff},
	{"txPowerPlus_ofdm_6",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_ofdm_6), _SIZE_RFFT(txPowerPlus_ofdm_6), 0xff},
	{"txPowerPlus_ofdm_9",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_ofdm_9), _SIZE_RFFT(txPowerPlus_ofdm_9), 0xff},
	{"txPowerPlus_ofdm_12",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_ofdm_12), _SIZE_RFFT(txPowerPlus_ofdm_12), 0xff},
	{"txPowerPlus_ofdm_18",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_ofdm_18), _SIZE_RFFT(txPowerPlus_ofdm_18), 0xff},
	{"txPowerPlus_ofdm_24",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_ofdm_24), _SIZE_RFFT(txPowerPlus_ofdm_24), 0xff},
	{"txPowerPlus_ofdm_36",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_ofdm_36), _SIZE_RFFT(txPowerPlus_ofdm_36), 0xff},
	{"txPowerPlus_ofdm_48",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_ofdm_48), _SIZE_RFFT(txPowerPlus_ofdm_48), 0xff},
	{"txPowerPlus_ofdm_54",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_ofdm_54), _SIZE_RFFT(txPowerPlus_ofdm_54), 0xff},
	{"txPowerPlus_mcs_0",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_0), _SIZE_RFFT(txPowerPlus_mcs_0), 0xff},
	{"txPowerPlus_mcs_1",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_1), _SIZE_RFFT(txPowerPlus_mcs_1), 0xff},
	{"txPowerPlus_mcs_2",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_2), _SIZE_RFFT(txPowerPlus_mcs_2), 0xff},
	{"txPowerPlus_mcs_3",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_3), _SIZE_RFFT(txPowerPlus_mcs_3), 0xff},
	{"txPowerPlus_mcs_4",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_4), _SIZE_RFFT(txPowerPlus_mcs_4), 0xff},
	{"txPowerPlus_mcs_5",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_5), _SIZE_RFFT(txPowerPlus_mcs_5), 0xff},
	{"txPowerPlus_mcs_6",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_6), _SIZE_RFFT(txPowerPlus_mcs_6), 0xff},
	{"txPowerPlus_mcs_7",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_7), _SIZE_RFFT(txPowerPlus_mcs_7), 0xff},
	{"txPowerPlus_mcs_8",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_8), _SIZE_RFFT(txPowerPlus_mcs_8), 0xff},
	{"txPowerPlus_mcs_9",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_9), _SIZE_RFFT(txPowerPlus_mcs_9), 0xff},
	{"txPowerPlus_mcs_10",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_10), _SIZE_RFFT(txPowerPlus_mcs_10), 0xff},
	{"txPowerPlus_mcs_11",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_11), _SIZE_RFFT(txPowerPlus_mcs_11), 0xff},
	{"txPowerPlus_mcs_12",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_12), _SIZE_RFFT(txPowerPlus_mcs_12), 0xff},
	{"txPowerPlus_mcs_13",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_13), _SIZE_RFFT(txPowerPlus_mcs_13), 0xff},
	{"txPowerPlus_mcs_14",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_14), _SIZE_RFFT(txPowerPlus_mcs_14), 0xff},
	{"txPowerPlus_mcs_15",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_15), _SIZE_RFFT(txPowerPlus_mcs_15), 0xff},
#endif

#if !defined(RTL8192SU)
	{"rootFwBeacon",		RFFT_T,	_OFFSET_RFFT(rootFwBeacon), _SIZE_RFFT(rootFwBeacon), 1},
#endif

#ifdef RTL8192SU_EFUSE
	{"use_efuse",		INT_T,	_OFFSET(efuseEntry.enable_efuse), _SIZE(efuseEntry.enable_efuse), 1},
	{"usb_epnum",		INT_T,	_OFFSET(efuseEntry.usb_epnum), _SIZE(efuseEntry.usb_epnum), USBEP_FOUR},
#endif

#ifdef RTL8192SU_TXPKT
	{"use_queue",		INT_T,	_OFFSET(txpktEntry.use_queue), _SIZE(txpktEntry.use_queue), 0},
#endif
};

#if !defined(RTL8192SE) && !defined(RTL8192SU)
static struct iwpriv_arg eeprom_table[] = {
	{"RFChipID",	BYTE_T,			EEPROM_RF_CHIP_ID, 1},
	{"Mac",			BYTE_ARRAY_T,	EEPROM_NODE_ADDRESS_BYTE_0, MACADDRLEN},
	{"TxPowerCCK",	IDX_BYTE_ARRAY_T,	EEPROM_TX_POWER_LEVEL_0, MAX_CCK_CHANNEL_NUM},
	{"TxPowerOFDM",	IDX_BYTE_ARRAY_T,	EEPROM_11G_CHANNEL_OFDM_TX_POWER_LEVEL_OFFSET, MAX_OFDM_CHANNEL_NUM},
};
#endif

#ifdef _DEBUG_RTL8190_
unsigned long rtl8190_debug_err=0xffffffff;
unsigned long rtl8190_debug_info=0;
unsigned long rtl8190_debug_trace=0;
unsigned long rtl8190_debug_warn=0;
#endif

void MDL_DEVINIT set_mib_default_tbl(struct rtl8190_priv *priv)
{
	int i;
	int arg_num = sizeof(mib_table)/sizeof(struct iwpriv_arg);

	for (i=0; i<arg_num; i++) {
		if (mib_table[i].Default) {
			if (mib_table[i].type == BYTE_T)
				*(((unsigned char *)priv->pmib)+mib_table[i].offset) = (unsigned char)mib_table[i].Default;
			else if (mib_table[i].type == INT_T)
				memcpy(((unsigned char *)priv->pmib)+mib_table[i].offset, (unsigned char *)&mib_table[i].Default, sizeof(int));
			else if (mib_table[i].type == RFFT_T && mib_table[i].len == 1)
				*(((unsigned char *)&(priv->pshare->rf_ft_var))+mib_table[i].offset) = (unsigned char)mib_table[i].Default;
			else if (mib_table[i].type == RFFT_T && mib_table[i].len == 4)
				memcpy(((unsigned char *)&(priv->pshare->rf_ft_var))+mib_table[i].offset, (unsigned char *)&mib_table[i].Default, sizeof(int));
			else {
				// We only give default value of types of BYTE_T and INT_T here.
				// Some others are gave in set_mib_default().
			}
		}
	}
}


int _atoi(char *s, int base)
{
	int k = 0;

	k = 0;
	if (base == 10) {
		while (*s != '\0' && *s >= '0' && *s <= '9') {
			k = 10 * k + (*s - '0');
			s++;
		}
	}
	else {
		while (*s != '\0') {
			int v;
			if ( *s >= '0' && *s <= '9')
				v = *s - '0';
			else if ( *s >= 'a' && *s <= 'f')
				v = *s - 'a' + 10;
			else if ( *s >= 'A' && *s <= 'F')
				v = *s - 'A' + 10;
			else {
				_DEBUG_ERR("error hex format!\n");
				return 0;
			}
			k = 16 * k + v;
			s++;
		}
	}
	return k;
}


static struct iwpriv_arg *get_tbl_entry(char *pstr)
{
	int i=0;
	int arg_num = sizeof(mib_table)/sizeof(struct iwpriv_arg);
	char name[128];

	while (*pstr && *pstr != '=')
		name[i++] = *pstr++;
	name[i] = '\0';

	for (i=0; i<arg_num; i++) {
		if (!strcmp(name, mib_table[i].name)) {
			return &mib_table[i];
		}
	}
	return NULL;
}


#if !defined(RTL8192SE) && !defined(RTL8192SU)
static struct iwpriv_arg *get_eeprom_tbl_entry(char *pstr)
{
	int i=0;
	int arg_num = sizeof(eeprom_table)/sizeof(struct iwpriv_arg);
	char name[128];

	while (*pstr && *pstr != '=')
		name[i++] = *pstr++;
	name[i] = '\0';

	for (i=0; i<arg_num; i++) {
		if (!strcmp(name, eeprom_table[i].name)) {
			return &eeprom_table[i];
		}
	}
	return NULL;
}
#endif


int get_array_val(unsigned char *dst, char *src, int len)
{
	char tmpbuf[4];
	int num=0;

	while (len > 0) {
		memcpy(tmpbuf, src, 2);
		tmpbuf[2]='\0';
		*dst++ = (unsigned char)_atoi(tmpbuf, 16);
		len-=2;
		src+=2;
		num++;
	}
	return num;
}


char *get_arg(char *src, char *val)
{
	int len=0;

	while (*src && *src!=',') {
		*val++ = *src++;
		len++;
	}
	if (len == 0)
		return NULL;

	*val = '\0';

	if (*src==',')
		src++;

	return src;
}


static int set_mib(struct rtl8190_priv *priv, unsigned char *data)
{
	struct iwpriv_arg *entry;
	int int_val, int_idx, len, *int_ptr;
	unsigned char byte_val;
	char *arg_val, tmpbuf[100];
#ifdef 	CONFIG_RTK_MESH
	unsigned short word;
#endif
	DEBUG_TRACE;

	DEBUG_INFO("set_mib %s\n", data);

	entry = get_tbl_entry((char *)data);
	if (entry == NULL) {
		DEBUG_ERR("invalid mib name [%s] !\n", data);
		return -1;
	}

	// search value
	arg_val = (char *)data;
	while (*arg_val && *arg_val != '=')
		arg_val++;

	if (!*arg_val) {
		DEBUG_ERR("mib value empty [%s] !\n", data);
		return -1;
	}
	arg_val++;

	switch (entry->type) {
	case BYTE_T:
		byte_val = (unsigned char)_atoi(arg_val, 10);
		memcpy(((unsigned char *)priv->pmib)+entry->offset, &byte_val,  1);
		break;
#ifdef 	CONFIG_RTK_MESH
	case WORD_T:
		word = (unsigned short)_atoi(arg_val, 10);
		memcpy(((unsigned char *)priv->pmib)+entry->offset, &word,  2);
		break;
#endif
	case INT_T:
		int_val = _atoi(arg_val, 10);
#ifdef WIFI_SIMPLE_CONFIG
		if (strcmp(entry->name, "wsc_enable") == 0) {
			if (int_val == 4) { // disable hidden AP
				if (HIDDEN_AP && priv->pbeacon_ssid) {
					memcpy(priv->pbeacon_ssid, SSID, SSID_LEN);
					priv->hidden_ap_mib_backup = HIDDEN_AP;
					HIDDEN_AP = 0;
				}
				break;
			}
			if (int_val == 5) { // restore hidden AP
				if (priv->pbeacon_ssid && !HIDDEN_AP && priv->hidden_ap_mib_backup) {
					memset(priv->pbeacon_ssid, '\0', 32);
					HIDDEN_AP = priv->hidden_ap_mib_backup;
				}
				break;
			}
#ifdef CLIENT_MODE
			if ((priv->pmib->wscEntry.wsc_enable == 1) && (int_val == 0)) { // for client mode
				if (priv->recover_join_req) {
					priv->recover_join_req = 0;
					priv->pmib->wscEntry.wsc_enable = 0;
					memcpy(&priv->pmib->dot11Bss, &priv->dot11Bss_original, sizeof(struct bss_desc));
					memcpy(SSID2SCAN, priv->pmib->dot11Bss.ssid, priv->pmib->dot11Bss.ssidlen);
					SSID2SCAN_LEN = priv->pmib->dot11Bss.ssidlen;
					SSID_LEN = SSID2SCAN_LEN;
					memcpy(SSID, SSID2SCAN, SSID_LEN);
					memset(BSSID, 0, MACADDRLEN);
					priv->join_req_ongoing = 1;
					priv->authModeRetry = 0;
					start_clnt_join(priv);
					break;
				}
			}
			else if ((priv->pmib->wscEntry.wsc_enable == 0) && (int_val == 1))
				memcpy(&priv->dot11Bss_original, &priv->pmib->dot11Bss, sizeof(struct bss_desc));

#endif
		}
#endif
		memcpy(((unsigned char *)priv->pmib)+entry->offset, (unsigned char *)&int_val, sizeof(int));
		break;

	case SSID_STRING_T:
		if (strlen(arg_val) > entry->len)
			arg_val[entry->len] = '\0';
		memset(priv->pmib->dot11StationConfigEntry.dot11DesiredSSID, 0, sizeof(priv->pmib->dot11StationConfigEntry.dot11DesiredSSID));
		memcpy(priv->pmib->dot11StationConfigEntry.dot11DesiredSSID, arg_val, strlen(arg_val));
		priv->pmib->dot11StationConfigEntry.dot11DesiredSSIDLen = strlen(arg_val);
		if ((SSID_LEN == 3) &&
			((SSID[0] == 'A') || (SSID[0] == 'a')) &&
			((SSID[1] == 'N') || (SSID[1] == 'n')) &&
			((SSID[2] == 'Y') || (SSID[2] == 'y'))) {
			SSID2SCAN_LEN = 0;
			memset(SSID2SCAN, 0, 32);
		}
		else {
			SSID2SCAN_LEN = SSID_LEN;
			memcpy(SSID2SCAN, SSID, SSID_LEN);
		}
		break;

	case BYTE_ARRAY_T:
		len = strlen(arg_val);
		if (len/2 > entry->len) {
			DEBUG_ERR("invalid len of BYTE_ARRAY_T mib [%s] !\n", entry->name);
			return -1;
		}
		if (len%2) {
			DEBUG_ERR("invalid len of BYTE_ARRAY_T mib [%s] !\n", entry->name);
			return -1;
		}
		get_array_val(((unsigned char *)priv->pmib)+entry->offset, arg_val, strlen(arg_val));
		break;

	case ACL_T:
	case ACL_INT_T:
		arg_val = get_arg(arg_val, tmpbuf);
		if (arg_val == NULL) {
			DEBUG_ERR("invalid ACL_T addr [%s] !\n", entry->name);
			return -1;
		}
		if (entry->type == ACL_T && strlen(tmpbuf)!=12) {
			DEBUG_ERR("invalid len of ACL_T mib [%s] !\n", entry->name);
			return -1;
		}
		int_ptr = (int *)(((unsigned char *)priv->pmib)+entry->offset+entry->len);
		int_idx = *int_ptr;
		if (entry->type == ACL_T)
			get_array_val(((unsigned char *)priv->pmib)+entry->offset+int_idx*6, tmpbuf, 12);
		else {
			get_array_val(((unsigned char *)priv->pmib)+entry->offset+int_idx*(6+4), tmpbuf, 12);
			if (strlen(arg_val) > 0) {
				int_val = _atoi(arg_val, 10);
				memcpy(((unsigned char *)priv->pmib)+entry->offset+int_idx*(6+4)+6, &int_val, 4);
			}
		}
		*int_ptr = *int_ptr + 1;
		break;

	case IDX_BYTE_ARRAY_T:
		arg_val = get_arg(arg_val, tmpbuf);
		if (arg_val == NULL) {
			DEBUG_ERR("invalid BYTE_ARRAY mib [%s] !\n", entry->name);
			return -1;
		}
		int_idx = _atoi(tmpbuf, 10);
		if (int_idx+1 > entry->len) {
			DEBUG_ERR("invalid BYTE_ARRAY mib index [%s, %d] !\n", entry->name, int_idx);
			return -1;
		}
		arg_val = get_arg(arg_val, tmpbuf);
		if (arg_val == NULL) {
			DEBUG_ERR("invalid BYTE_ARRAY mib [%s] !\n", entry->name);
			return -1;
		}
		byte_val = (unsigned char)_atoi(tmpbuf, 10);
		memcpy(((unsigned char *)priv->pmib)+entry->offset+int_idx, (unsigned char *)&byte_val, sizeof(byte_val));
		break;

	case MULTI_BYTE_T:
		int_idx=0;
		while (1) {
			arg_val = get_arg(arg_val, tmpbuf);
			if (arg_val == NULL)
				break;
			if (int_idx+1 > entry->len) {
				DEBUG_ERR("invalid MULTI_BYTE_T mib index [%s, %d] !\n", entry->name, int_idx);
				return -1;
			}
			byte_val = (unsigned char)_atoi(tmpbuf, 16);
			memcpy(((unsigned char *)priv->pmib)+entry->offset+int_idx++, (unsigned char *)&byte_val, sizeof(byte_val));
		}
		// copy length to next parameter
		memcpy( ((unsigned char *)priv->pmib)+entry->offset+entry->len, (unsigned char *)&int_idx, sizeof(int));
		break;

#ifdef _DEBUG_RTL8190_
	case DEBUG_T:
		int_val = _atoi(arg_val, 16);
		if (entry->offset==1)
			rtl8190_debug_err = int_val;
		else if (entry->offset==2)
			rtl8190_debug_info = int_val;
		else if (entry->offset==3)
			rtl8190_debug_trace = int_val;
		else if (entry->offset==4)
			rtl8190_debug_warn = int_val;
		else {
			DEBUG_ERR("invalid debug index\n");
		}
		break;
#endif // _DEBUG_RTL8190_

	case DEF_SSID_STRING_T:
		if (strlen(arg_val) > entry->len)
			arg_val[entry->len] = '\0';
		memset(priv->pmib->dot11StationConfigEntry.dot11DefaultSSID, 0, sizeof(priv->pmib->dot11StationConfigEntry.dot11DefaultSSID));
		memcpy(priv->pmib->dot11StationConfigEntry.dot11DefaultSSID, arg_val, strlen(arg_val));
		priv->pmib->dot11StationConfigEntry.dot11DefaultSSIDLen = strlen(arg_val);
		break;

	case STRING_T:
		if (strlen(arg_val) > entry->len)
			arg_val[entry->len] = '\0';
		strcpy(((unsigned char *)priv->pmib)+entry->offset, arg_val);
		break;

	case RFFT_T:
		if (entry->len == 1) {
			byte_val = _atoi(arg_val, 10);
			memcpy(((unsigned char *)&priv->pshare->rf_ft_var)+entry->offset, (unsigned char *)&byte_val, entry->len);
		}
		else if (entry->len == 4) {
			int_val = _atoi(arg_val, 10);
			memcpy(((unsigned char *)&priv->pshare->rf_ft_var)+entry->offset, (unsigned char *)&int_val, entry->len);
		}
		break;

	case VARLEN_BYTE_T:
		len = strlen(arg_val);
		if (len/2 > entry->len) {
			DEBUG_ERR("invalid len of VARLEN_BYTE_T mib [%s] !\n", entry->name);
			return -1;
		}
		if (len%2) {
			DEBUG_ERR("invalid len of VARLEN_BYTE_T mib [%s] !\n", entry->name);
			return -1;
		}
		memset(((unsigned char *)priv->pmib)+entry->offset, 0, entry->len);
		len = get_array_val(((unsigned char *)priv->pmib)+entry->offset, arg_val, strlen(arg_val));
		*(unsigned int *)(((unsigned char *)priv->pmib)+entry->offset+entry->len) = len;
		break;

#ifdef WIFI_SIMPLE_CONFIG
	case PIN_IND_T:
		if (strlen(arg_val) > entry->len) {
			DOT11_WSC_PIN_IND wsc_ind;

			wsc_ind.EventId = DOT11_EVENT_WSC_PIN_IND;
			wsc_ind.IsMoreEvent = 0;
			strcpy(wsc_ind.code, arg_val);
			DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (unsigned char*)&wsc_ind, sizeof(DOT11_WSC_PIN_IND));
			event_indicate(priv, NULL, -1);
		}
		break;

#endif

#ifdef CONFIG_RTL_WAPI_SUPPORT
	case INT_ARRAY_T:
		int_idx=0;
		while (1) {
			arg_val = get_arg(arg_val, tmpbuf);
			if (arg_val == NULL)
				break;
			if (int_idx+1 > (entry->len)/sizeof(int)) {
				DEBUG_ERR("invalid MULTI_BYTE_T mib index [%s, %d] !\n", entry->name, int_idx);
				return -1;
			}
			int_val = _atoi(tmpbuf, 16);
			memcpy(((unsigned char *)priv->pmib)+entry->offset+int_idx++, (void *)&int_val, sizeof(int_val));
		}
		break;
	case WAPI_KEY_T:
		{
			char tmppasswd[100]={0};
			wapiMibPSK *wapipsk=NULL;
			int_idx=0;

			/*Get Password*/	
			arg_val = get_arg(arg_val, tmpbuf);
			if (arg_val == NULL)
				break;
			memcpy(tmppasswd, tmpbuf, strlen(tmpbuf));
			
			/*Get Password length*/
			arg_val=get_arg(arg_val, tmpbuf);
			int_val = _atoi(tmpbuf, 16);

			wapipsk=(wapiMibPSK *)((unsigned char *)(priv->pmib)+entry->offset);
			
			/*Hex or passthru*/
			if((0==(strlen(tmppasswd) % 2))  && (int_val < strlen(tmppasswd)) &&
				(int_val == (strlen(tmppasswd)/2)))
			{
				/*Hex mode*/
				string_to_hex(tmppasswd,wapipsk->octet,strlen(tmppasswd));
			}
			else
			{
				strncpy(wapipsk->octet,tmppasswd,strlen(tmppasswd));
			}
			wapipsk->len = int_val;
			break;
	      }
#endif
	default:
		DEBUG_ERR("invalid mib type!\n");
		break;
	}

	return 0;
}


static int get_mib(struct rtl8190_priv *priv, unsigned char *data)
{
	struct iwpriv_arg *entry;
	int i, len, *int_ptr, copy_len;
	char tmpbuf[40];

	DEBUG_TRACE;

	DEBUG_INFO("get_mib %s\n", data);

	entry = get_tbl_entry((char *)data);
	if (entry == NULL) {
		DEBUG_ERR("invalid mib name [%s] !\n", data);
		return -1;
	}
	copy_len = entry->len;

	switch (entry->type) {
	case BYTE_T:
		memcpy(data, ((unsigned char *)priv->pmib)+entry->offset,  1);
		PRINT_INFO("byte data: %d\n", *data);
		break;
#ifdef 	CONFIG_RTK_MESH
	case WORD_T:
		memcpy(data, ((unsigned char *)priv->pmib)+entry->offset,  2);
		PRINT_INFO("word data: %d\n", *data);
		break;
#endif
	case INT_T:
		memcpy(data, ((unsigned char *)priv->pmib)+entry->offset, sizeof(int));
		PRINT_INFO("int data: %d\n", *((int *)data));
		break;

	case SSID_STRING_T:
		memcpy(tmpbuf, priv->pmib->dot11StationConfigEntry.dot11DesiredSSID, priv->pmib->dot11StationConfigEntry.dot11DesiredSSIDLen);
		tmpbuf[priv->pmib->dot11StationConfigEntry.dot11DesiredSSIDLen] = '\0';
		strcpy(data, tmpbuf);
		PRINT_INFO("ssid: %s\n", tmpbuf);
		break;

	case BYTE_ARRAY_T:
		memcpy(data, ((unsigned char *)priv->pmib)+entry->offset, entry->len);
		PRINT_INFO("data (hex): ");
		for (i=0; i<entry->len; i++)
			PRINT_INFO("%02x", *((unsigned char *)((unsigned char *)priv->pmib)+entry->offset+i));
		PRINT_INFO("\n");
		break;

	case ACL_T:
		int_ptr = (int *)(((unsigned char *)priv->pmib)+entry->offset+entry->len);
		PRINT_INFO("ACL table (%d):\n", *int_ptr);
		copy_len = 0;
		for (i=0; i<*int_ptr; i++) {
			memcpy(data, ((unsigned char *)priv->pmib)+entry->offset+i*6, 6);
			PRINT_INFO("mac-addr: %02x-%02x-%02x-%02x-%02x-%02x\n",
				data[0],data[1],data[2],data[3],data[4],data[5]);
			data += 6;
			copy_len += 6;
		}
		DEBUG_INFO("\n");
		break;

	case IDX_BYTE_ARRAY_T:
		memcpy(data, ((unsigned char *)priv->pmib)+entry->offset, entry->len);
		PRINT_INFO("data (dec): ");
		for (i=0; i<entry->len; i++)
			PRINT_INFO("%d ", *((unsigned char *)((unsigned char *)priv->pmib)+entry->offset+i));
		PRINT_INFO("\n");
		break;

	case MULTI_BYTE_T:
		memcpy(&len, ((unsigned char *)priv->pmib)+entry->offset+entry->len, sizeof(int));
		memcpy(data, ((unsigned char *)priv->pmib)+entry->offset, len);
		PRINT_INFO("data (hex): ");
		for (i=0; i<len; i++)
			PRINT_INFO("%02x ", *((unsigned char *)((unsigned char *)priv->pmib)+entry->offset+i));
		PRINT_INFO("\n");
		break;

#ifdef _DEBUG_RTL8190_
	case DEBUG_T:
		if (entry->offset==1)
			memcpy(data, (unsigned char *)&rtl8190_debug_err, sizeof(rtl8190_debug_err));
		else if (entry->offset==2)
			memcpy(data, (unsigned char *)&rtl8190_debug_info, sizeof(rtl8190_debug_info));
		else if (entry->offset==3)
			memcpy(data, (unsigned char *)&rtl8190_debug_trace, sizeof(rtl8190_debug_trace));
		else if (entry->offset==4)
			memcpy(data, (unsigned char *)&rtl8190_debug_warn, sizeof(rtl8190_debug_warn));
		else {
			DEBUG_ERR("invalid debug index\n");
		}
		PRINT_INFO("debug flag(hex): %08lx\n", *((unsigned long *)data));
		break;
#endif // _DEBUG_RTL8190_

	case DEF_SSID_STRING_T:
		memcpy(tmpbuf, priv->pmib->dot11StationConfigEntry.dot11DefaultSSID, priv->pmib->dot11StationConfigEntry.dot11DefaultSSIDLen);
		tmpbuf[priv->pmib->dot11StationConfigEntry.dot11DefaultSSIDLen] = '\0';
		strcpy(data, tmpbuf);
		PRINT_INFO("defssid: %s\n", tmpbuf);
		break;

	case STRING_T:
		strcpy(data, ((unsigned char *)priv->pmib)+entry->offset);
		PRINT_INFO("string data: %s\n", data);
		break;

	case RFFT_T:
		memcpy(data, ((unsigned char *)&priv->pshare->rf_ft_var)+entry->offset, sizeof(int));
		PRINT_INFO("int data: %d\n", *((int *)data));
		break;

	case VARLEN_BYTE_T:
		copy_len = *(unsigned int *)(((unsigned char *)priv->pmib)+entry->offset+entry->len);
		memcpy(data, ((unsigned char *)priv->pmib)+entry->offset, copy_len);
		PRINT_INFO("data (hex): ");
		for (i=0; i<copy_len; i++)
			PRINT_INFO("%02x", *((unsigned char *)((unsigned char *)priv->pmib)+entry->offset+i));
		PRINT_INFO("\n");
		break;

	default:
		DEBUG_ERR("invalid mib type!\n");
		return 0;
	}

	return copy_len;
}


#ifdef _IOCTL_DEBUG_CMD_
/*
 * Write register, command: "iwpriv wlanX write_reg,type,offset,value"
 * 	where: type may be: "b" - byte, "w" - word, "dw" - "dw" (based on wlan register offset)
 *			    "_b" - byte, "_w" - word, "_dw" - "dw" (based on register offset 0)
 *		offset and value should be input in hex
 */
static int write_reg(struct rtl8190_priv *priv, unsigned char *data)
{
	char name[100];
	int i=0, op=0, offset;
	unsigned long ioaddr, val;
#ifdef RTL8192SU
	unsigned char get_reg=0;
#endif

	DEBUG_TRACE;

	// get access type
	while (*data && *data != ',')
		name[i++] = *data++;
	name[i] = '\0';

	if (!strcmp(name, "b"))
		op = 1;
	else if (!strcmp(name, "w"))
		op = 2;
	else if (!strcmp(name, "dw"))
		op = 3;
	else if (!strcmp(name, "_b"))
		op = 0x81;
	else if (!strcmp(name, "_w"))
		op = 0x82;
	else if (!strcmp(name, "_dw"))
		op = 0x83;

	if (op == 0 || !*data++) {
		DEBUG_ERR("invalid type!\n");
		return -1;
	}

	if ( !(op&0x80))  // wlan register
		ioaddr = priv->pshare->ioaddr;
	else
		ioaddr = 0;

	// get offset and value
	i=0;
	while (*data && *data != ',')
		name[i++] = *data++;
	name[i] = '\0';
	if (!*data++) {
		DEBUG_ERR("invalid offset!\n");
		return -1;
	}
	offset = _atoi(name, 16);
	val = (unsigned long)_atoi((char *)data, 16);

	DEBUG_INFO("write reg in %s: addr=%08x, val=0x%x\n",
			(op == 1 ? "byte" : (op == 2 ? "word" : "dword")),
			offset, (int)val);

#ifdef RTL8192SU
	if (offset>=0x800 && offset <=0xefff)
		get_reg=1;
	else if(offset>=0x80000000)
		get_reg=2;
	
	if (get_reg==0)
#endif
	switch (op&0x7f) {
	case 1:
		RTL_W8(offset, ((unsigned char)val));
		break;
	case 2:
		RTL_W16(offset, ((unsigned short)val));
		break;
	case 3:
		RTL_W32(offset, ((unsigned long)val));
#ifdef RTL8192SU
		if (offset==WFM4)
		{
			delay_ms(1000);
			printk("read dword reg 0x2b4=0x%08x\n", RTL_R32(WFM4+4));
		}
		if (offset==RF_BB_CMD_ADDR)
		{
			delay_ms(1000);
			printk("read dword reg 0x2c4=0x%08x\n", RTL_R32(RF_BB_CMD_DATA));
		}
#endif
		break;
	}
#ifdef RTL8192SU
	else if(get_reg==1)
	{
		switch (op&0x7f)
		{
			case 1:
				PHY_SetBBReg(priv, offset, bMaskByte0,val);
				break;
			case 2:
				PHY_SetBBReg(priv, offset, bMaskLWord,val);
				break;
			case 3:
				PHY_SetBBReg(priv, offset, bMaskDWord,val);
				break;
		}
	}
	else if(get_reg==2)
	{
		switch (op&0x7f)
		{
			case 1:
				REG8(offset)=val;
				break;
			case 2:
				REG16(offset)=val;
				break;
			case 3:
				REG32(offset)=val;
				break;
		}	
	}
#endif	
	return 0;
}


/*
 * Read register, command: "iwpriv wlanX read_reg,type,offset"
 * 	where: type may be: "b" - byte, "w" - word, "dw" - "dw" (based on wlan register offset)
 *			    "_b" - byte, "_w" - word, "_dw" - "dw" (based on register offset 0)
 *		offset should be input in hex
 */
static int read_reg(struct rtl8190_priv *priv, unsigned char *data)
{
	char name[100];
	int i=0, op=0, offset, len=0;
	unsigned long ioaddr, dw_val;
	unsigned char *org_ptr=data, b_val;
	unsigned short w_val;
#ifdef RTL8192SU
	unsigned char bb_reg=0;
#endif

	DEBUG_TRACE;

	// get access type
	while (*data && *data != ',')
		name[i++] = *data++;
	name[i] = '\0';

	if (!strcmp(name, "b"))
		op = 1;
	else if (!strcmp(name, "w"))
		op = 2;
	else if (!strcmp(name, "dw"))
		op = 3;
	//else if (!strcmp(name, "dump"))
	//	op = 4;		
	else if (!strcmp(name, "_b"))
		op = 0x81;
	else if (!strcmp(name, "_w"))
		op = 0x82;
	else if (!strcmp(name, "_dw"))
		op = 0x83;

	if (op == 0 || !*data++) {
		DEBUG_ERR("invalid type!\n");
		return -1;
	}

	if ( !(op&0x80))  // wlan register
		ioaddr = priv->pshare->ioaddr;
	else
		ioaddr = 0;

	// get offset
	offset = _atoi((char *)data, 16);

#ifdef RTL8192SU
	if (offset>=0x800 && offset <=0xefff)
		bb_reg=1;
	//else if(offset>=0x80000000)
		//get_bb=2;
	
	if (bb_reg==0)
#endif
	switch (op&0x7f) {
	case 1:
		b_val = (unsigned char)RTL_R8(offset);
#ifdef CONFIG_PRINTK_DISABLED
		panic_printk
#else
		printk
#endif
			("read byte reg %x=0x%02x\n", offset, b_val);
		len = 1;
		memcpy(org_ptr, &b_val, len);
		break;
	case 2:
		w_val = (unsigned short)RTL_R16(offset);
#ifdef CONFIG_PRINTK_DISABLED
		panic_printk
#else
		printk
#endif
			("read word reg %x=0x%04x\n", offset, w_val);
		len = 2;
		memcpy(org_ptr, (char *)&w_val, len);
		break;
	case 3:
		dw_val = (unsigned long)RTL_R32(offset);
#ifdef CONFIG_PRINTK_DISABLED
		panic_printk
#else
		printk
#endif
			("read dword reg %x=0x%08lx\n", offset, dw_val);
		len = 4;
		memcpy(org_ptr, (char *)&dw_val, len);
		break;
	}
#ifdef RTL8192SU
	else if(bb_reg==1)
	{
		switch (op&0x7f)
		{
			case 1:
				b_val = PHY_QueryBBReg(priv, offset, bMaskByte0);
#ifdef CONFIG_PRINTK_DISABLED
				panic_printk
#else
				printk
#endif
					("read byte reg %x=0x%02x\n", offset, b_val);
				len = 1;
				memcpy(org_ptr, &b_val, len);
				break;
			case 2:
				w_val = PHY_QueryBBReg(priv, offset, bMaskLWord);
#ifdef CONFIG_PRINTK_DISABLED
				panic_printk
#else
				printk
#endif
					("read word reg %x=0x%04x\n", offset, w_val);
				len = 2;
				memcpy(org_ptr, (char *)&w_val, len);
				break;
			case 3:
				dw_val = PHY_QueryBBReg(priv, offset, bMaskDWord);
#ifdef CONFIG_PRINTK_DISABLED
				panic_printk
#else
				printk
#endif
					("read dword reg %x=0x%08lx\n", offset, dw_val);
				len = 4;
				memcpy(org_ptr, (char *)&dw_val, len);
				break;
		}
	}
#if 0
	else if(get_bb==2)  //read / write regs/mem
	{
		switch (op&0x7f)
		{
			case 1:
				b_val = REG8(offset);
				printk("read byte reg %x=0x%02x\n", offset, b_val);
				len = 1;
				memcpy(org_ptr, &b_val, len);
				break;
			case 2:
				w_val = REG16(offset);
				printk("read word reg %x=0x%04x\n", offset, w_val);
				len = 2;
				memcpy(org_ptr, (char *)&w_val, len);
				break;
			case 3:
				dw_val = REG32(offset);
				printk("read dword reg %x=0x%08lx\n", offset, dw_val);
				len = 4;
				memcpy(org_ptr, (char *)&dw_val, len);
				break;
			case 4:
				memDump(offset,128,"dump");
				break;
		}	
		
	}	
#endif	
#endif

	return len;
}


/*
 * Write memory, command: "iwpriv wlanX write_mem,type,start,len,value"
 * 	where: type may be: "b" - byte, "w" - word, "dw" - "dw"
 *		start, len and value should be input in hex
 */
static int write_mem(struct rtl8190_priv *priv, unsigned char *data)
{
	char tmpbuf[100];
	int i=0, size=0, len;
	unsigned long val, start;

	DEBUG_TRACE;

	// get access type
	while (*data && *data != ',')
		tmpbuf[i++] = *data++;
	tmpbuf[i] = '\0';

	if (!strcmp(tmpbuf, "b"))
		size = 1;
	else if (!strcmp(tmpbuf, "w"))
		size = 2;
	else if (!strcmp(tmpbuf, "dw"))
		size = 4;

	if (size == 0 || !*data++) {
		DEBUG_ERR("invalid command!\n");
		return -1;
	}

	// get start, len, and value
	i=0;
	while (*data && *data != ',')
		tmpbuf[i++] = *data++;
	tmpbuf[i] = '\0';
	if (i==0 || !*data++) {
		DEBUG_ERR("invalid start!\n");
		return -1;
	}
	start = (unsigned long)_atoi(tmpbuf, 16);

	i=0;
	while (*data && *data != ',')
		tmpbuf[i++] = *data++;
	tmpbuf[i] = '\0';
	if (i==0 || !*data++) {
		DEBUG_ERR("invalid len!\n");
		return -1;
	}
	len = _atoi(tmpbuf, 16);
	val = (unsigned long)_atoi((char *)data, 16);

	DEBUG_INFO("write memory: start=%08lx, len=%x, data=0x%x (%s)\n",
		start,	len, (int)val,
		(size == 1 ? "byte" : (size == 2 ? "word" : "dword")));

	for (i=0; i<len; i++) {
		memcpy((char *)start, (char *)&val, size);
		start += size;
	}
	return 0;
}


/*
 * Read memory, command: "iwpriv wlanX read_mem,type,start,len"
 * 	where: type may be: "b" - byte, "w" - word, "dw" - "dw"
 *		start, and len should be input in hex
 */
static int read_mem(struct rtl8190_priv *priv, unsigned char *data)
{
	char tmpbuf[100];
//#ifndef CONFIG_RTL8186_TR	 //brad add for tr 11n
#if !(defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD) || defined(CONFIG_RTL865X_SC))
#ifndef __LINUX_2_6__
//#ifdef _DEBUG_RTL8190_
	char tmp1[2048];
#endif
#endif// !define CONFIG_RTL8186_TR
	int i=0, size=0, len, copy_len;
	unsigned long start, dw_val;
	unsigned short w_val;
	unsigned char b_val, *pVal=NULL, *org_ptr=data;

	DEBUG_TRACE;

	// get access type
	while (*data && *data != ',')
		tmpbuf[i++] = *data++;
	tmpbuf[i] = '\0';

	if (!strcmp(tmpbuf, "b")) {
		size = 1;
		pVal = &b_val;
	}
	else if (!strcmp(tmpbuf, "w")) {
		size = 2;
		pVal = (unsigned char *)&w_val;
	}
	else if (!strcmp(tmpbuf, "dw")) {
		size = 4;
		pVal = (unsigned char *)&dw_val;
	}

	if (size == 0 || !*data++) {
		DEBUG_ERR("invalid type!\n");
		return -1;
	}

	// get start and len
	i=0;
	while (*data && *data != ',')
		tmpbuf[i++] = *data++;
	tmpbuf[i] = '\0';
	if (i==0 || !*data++) {
		DEBUG_ERR("invalid start!\n");
		return -1;
	}
	start = (unsigned long)_atoi(tmpbuf, 16);
	len = _atoi((char *)data, 16);
#if !(defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD) || defined(CONFIG_RTL865X_SC))
#ifndef __LINUX_2_6__
//#ifdef _DEBUG_RTL8190_
	sprintf(tmp1, "read memory: from=%lx, len=0x%x (%s)\n",
		start, len, (size == 1 ? "byte" : (size == 2 ? "word" : "dword")));

	for (i=0; i<len; i++) {
		char tmp2[10];
		memcpy(pVal, (char *)start+i*size, size);
		if (size == 1) {
			sprintf(tmp2, "%02x ", b_val);
			if ((i>0) && ((i%16)==0))
				strcat(tmp1, "\n");
		}
		else if (size == 2) {
			sprintf(tmp2, "%04x ", w_val);
			if ((i>0) && ((i%8)==0))
				strcat(tmp1, "\n");
		}
		else if (size == 4) {
			sprintf(tmp2, "%08lx ", dw_val);
			if ((i>0) && ((i%8)==0))
				strcat(tmp1, "\n");
		}
		strcat(tmp1, tmp2);
	}
	strcat(tmp1, "\n");

#ifdef CONFIG_PRINTK_DISABLED
	panic_printk
#else
	printk
#endif
		("%s", tmp1);
#endif // _DEBUG_RTL8190_
#endif // !define CONFIG_RTL8186_TR
	if (size*len > 128)
		copy_len = 128;
	else
		copy_len = size*len;
	memcpy(org_ptr,  (char *)start, copy_len);

	return copy_len;
}


static int write_bb_reg(struct rtl8190_priv *priv, unsigned char *data)
{
	return 0;
}


static int read_bb_reg(struct rtl8190_priv *priv, unsigned char *data)
{
	return 0;
}


static int write_rf_reg(struct rtl8190_priv *priv, unsigned char *data)
{
	char tmpbuf[32];
	unsigned int path, offset, val, val_read;
	int i;

	DEBUG_TRACE;

	if (strlen(data) != 0) {
		i = 0;
		while (*data && *data != ',')
			tmpbuf[i++] = *data++;
		tmpbuf[i] = '\0';
		if (i==0 || !*data++) {
			DEBUG_ERR("invalid path!\n");
			return -1;
		}
		path = _atoi(tmpbuf, 16);

		i = 0;
		while (*data && *data != ',')
			tmpbuf[i++] = *data++;
		tmpbuf[i] = '\0';
		if (i==0 || !*data++) {
			DEBUG_ERR("invalid offset!\n");
			return -1;
		}
		offset = _atoi(tmpbuf, 16);

		val = (unsigned long)_atoi((char *)data, 16);

		PHY_SetRFReg(priv, path, offset, bMask12Bits, val);
		val_read = PHY_QueryRFReg(priv, path, offset, bMask12Bits, 1);
		printk("write RF %d offset 0x%02x val [0x%04x],  read back [0x%04x]\n",
#if !defined(RTL8192SU)
			path, offset, (unsigned short)val, (unsigned short)val_read);
#else
			path, offset, (unsigned int)val, (unsigned int)val_read);
#endif			
	}

	return 0;
}


static int read_rf_reg(struct rtl8190_priv *priv, unsigned char *data)
{
	char tmpbuf[32];
	unsigned char *arg = data;
	unsigned int path, offset, val;
#if defined(RTL8190) || defined(RTL8192E)
	unsigned short val16;
#endif
	int i;

	DEBUG_TRACE;

	if (strlen(arg) != 0) {
		i = 0;
		while (*arg && *arg != ',')
			tmpbuf[i++] = *arg++;
		tmpbuf[i] = '\0';
		if (i==0 || !*arg++) {
			DEBUG_ERR("invalid path!\n");
			return -1;
		}
		path = _atoi(tmpbuf, 16);

		offset = (unsigned char)_atoi((char *)arg, 16);
#ifdef RTL8192SU		
		if (path==0)printk("radio_a, ");
		else printk("radio_b, ");
		printk("offset=%x\n", offset);
#endif		

#if defined(RTL8190) || defined(RTL8192E)
		val = PHY_QueryRFReg(priv, path, offset, bMask12Bits, 1);
#ifdef CONFIG_PRINTK_DISABLED
		panic_printk
#else
		printk
#endif
			("read RF %d reg %02x=0x%04x\n", path, offset, val);
		val16 = (unsigned short)val;
		memcpy(data, (char *)&val16, 2);
		return 2;
#elif defined(RTL8192SE)|| defined(RTL8192SU)
		val = PHY_QueryRFReg(priv, path, offset, bMask20Bits, 1);
#ifdef CONFIG_PRINTK_DISABLED
		panic_printk
#else
		printk
#endif
			("read RF %d reg %02x=0x%08x\n", path, offset, val);
		memcpy(data, (char *)&val, 4);
		return 4;
#endif
	}
	return 1;
}


#ifdef CONFIG_RTL8186_KB
int get_guestmac(struct rtl8190_priv *priv, GUESTMAC_T *macdata)
{
	int i=0;

	for (i=0; i<MAX_GUEST_NUM; i++)
	{
		if (priv->guestMac[i].valid)
		{
			memcpy(macdata->macaddr, priv->guestMac[i].macaddr, 6);
			macdata->valid = priv->guestMac[i].valid;
		}
		else
			break;
		macdata++;
	}
	return sizeof(GUESTMAC_T)*i;
}


int set_guestmacvalid(struct rtl8190_priv *priv, char *buf)
{
	int i=0;

	for (i=0; i<MAX_GUEST_NUM; i++)
	{
		if (priv->guestMac[i].valid)
		{
			continue;
		}
		memcpy(priv->guestMac[i].macaddr, buf, 6);
		priv->guestMac[i].valid = 1;
		return 0;
	}
	/*No slot avaible*/
	return -1;
}


int set_guestmacinvalid(struct rtl8190_priv *priv, char *buf)
{
	int i=0;

	for (i=0; i<MAX_GUEST_NUM; i++)
	{
		if (priv->guestMac[i].valid && !memcmp(priv->guestMac[i].macaddr, buf, 6))
		{
			priv->guestMac[i].valid = 0;
			return 0;
		}
	}
	/*No such slot*/
	return -1;
}
#endif // CONFIG_RTL8186_KB


#ifdef DEBUG_8190
static void reg_dump(struct rtl8190_priv *priv)
{
	int i, j, len;
#if !defined(RTL8192SU)	
	unsigned long ioaddr = priv->pshare->ioaddr;
#endif	
	unsigned char tmpbuf[100];

#ifdef CONFIG_RTL865X_WTDOG
	static unsigned long wtval;
	wtval = *((volatile unsigned long *)0xB800311C);
	*((volatile unsigned long *)0xB800311C) = 0xA5000000;	// disabe watchdog
#endif

#ifdef CONFIG_PRINTK_DISABLED
	panic_printk
#else
	printk
#endif
	("\nMAC Registers:\n");
	for (i=0; i<0x1000; i+=0x10) {
		len = sprintf(tmpbuf, "%03X\t", i);
		for (j=i; j<i+0x10; j+=4)
			len += sprintf(tmpbuf+len, "%08X ", (unsigned int)RTL_R32(j));
		len += sprintf(tmpbuf+len, "\n");
#ifdef CONFIG_PRINTK_DISABLED
		panic_printk
#else
		printk
#endif
		(tmpbuf);
	}
#ifdef CONFIG_PRINTK_DISABLED
	panic_printk
#else
	printk
#endif
	("\n");

#if defined(RTL8192SE) || defined(RTL8192SU)
#ifdef CONFIG_PRINTK_DISABLED
	panic_printk
#else
	printk
#endif
	("\nRF Registers:\n");
	len = 0;
	for (i = RF90_PATH_A; i < priv->pshare->phw->NumTotalRFPath; i++) {
		for (j = 0; j < 0x34; j++) {
			len += sprintf(tmpbuf+len, "%d%02x  %05x",
				i, j, PHY_QueryRFReg(priv, i, j, bMask20Bits, 1));

			if (j && !((j+1)%4))
				len += sprintf(tmpbuf+len, "\n");
			else
				len += sprintf(tmpbuf+len, "     ");
#ifdef CONFIG_PRINTK_DISABLED
			panic_printk
#else
			printk
#endif
			(tmpbuf);
			len = 0;
		}
#ifdef CONFIG_PRINTK_DISABLED
		panic_printk
#else
		printk
#endif
		("\n");
	}
#endif

/*
	printk("CCK Registers:\n");
	for (i=0; i<0x5f; i+=25) {
		len = sprintf(tmpbuf, "%03X  ", i);
		for (j=i; j<i+25; j++) {
			len += sprintf(tmpbuf+len, "%02X ", ReadBBPortUchar(priv, j+0x01000000));
			if (j == 0x5e)
				break;
 		}
		len += sprintf(tmpbuf+len, "\n");
		printk(tmpbuf);
	}
	printk("\n");

	printk("OFDM Registers:\n");
	for (i=0; i<0x40; i+=25) {
		len = sprintf(tmpbuf, "%03X  ", i);
		for (j=i; j<i+25; j++) {
			len += sprintf(tmpbuf+len, "%02X ", ReadBBPortUchar(priv, j));
			if (j == 0x3f)
				break;
		}
		len += sprintf(tmpbuf+len, "\n");
		printk(tmpbuf);
	}
	printk("\n");

	printk("RF Registers:\n");
	len = 0;
	for (i=0; i<16; i++) {
		len += sprintf(tmpbuf+len, "%08X ", (unsigned int)RF_ReadReg(priv, i));
		if ((i == 7) || (i == 15)) {
			len += sprintf(tmpbuf+len, "\n");
			printk(tmpbuf);
			len = 0;
		}
	}
	printk("\n");
*/

#ifdef CONFIG_RTL865X_WTDOG
	*((volatile unsigned long *)0xB800311C) |=  1 << 23;
	*((volatile unsigned long *)0xB800311C) = wtval;
#endif
}
#endif
#endif // _IOCTL_DEBUG_CMD_


int del_sta(struct rtl8190_priv *priv, unsigned char *data)
{
	struct stat_info *pstat;
	unsigned char macaddr[MACADDRLEN], tmpbuf[3];
	unsigned long flags;
	DOT11_DISASSOCIATION_IND Disassociation_Ind;
	int i;

	if (!netif_running(priv->dev))
		return 0;

	for(i=0; i<MACADDRLEN; i++)
	{
		tmpbuf[0] = data[2*i];
		tmpbuf[1] = data[2*i+1];
		tmpbuf[2] = 0;
		macaddr[i] = (unsigned char)_atoi((char *)tmpbuf, 16);
	}

#ifdef CONFIG_RTK_MESH
	// following add by chuangch 2007/08/30 (IAPP update proxy table)
	//by pepsi 08/03/12 for using PU
	if (( GET_MIB(priv)->dot1180211sInfo.mesh_enable == 1)	// fix: 0000107 2008/10/13
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID) // Spare for Mesh work with Multiple AP (Please see Mantis 0000107 for detail)
			&& IS_ROOT_INTERFACE(priv)
#endif
	)
		HASH_DELETE(priv->proxy_table, macaddr);

#endif	// CONFIG_RTK_MESH

	DEBUG_INFO("del_sta %02X%02X%02X%02X%02X%02X\n",
		macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);

	pstat = get_stainfo(priv, macaddr);

	if (pstat == NULL)
		return 0;

	if (!list_empty(&pstat->asoc_list))
	{
		if (IEEE8021X_FUN)
		{
#ifndef WITHOUT_ENQUEUE
			memcpy((void *)Disassociation_Ind.MACAddr, (void *)macaddr, MACADDRLEN);
			Disassociation_Ind.EventId = DOT11_EVENT_DISASSOCIATION_IND;
			Disassociation_Ind.IsMoreEvent = 0;
			Disassociation_Ind.Reason = _STATS_OTHER_;
			Disassociation_Ind.tx_packets = pstat->tx_pkts;
			Disassociation_Ind.rx_packets = pstat->rx_pkts;
			Disassociation_Ind.tx_bytes   = pstat->tx_bytes;
			Disassociation_Ind.rx_bytes   = pstat->rx_bytes;
			DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&Disassociation_Ind,
						sizeof(DOT11_DISASSOCIATION_IND));
#endif
#ifdef INCLUDE_WPA_PSK
			psk_indicate_evt(priv, DOT11_EVENT_DISASSOCIATION_IND, macaddr, NULL, 0);
#endif
			event_indicate(priv, macaddr, 2);
		}

		issue_disassoc(priv, macaddr, _RSON_UNSPECIFIED_);

		if (pstat->expire_to > 0)
		{
			SAVE_INT_AND_CLI(flags);
			cnt_assoc_num(priv, pstat, DECREASE, (char *)__FUNCTION__);
			check_sta_characteristic(priv, pstat, DECREASE);
			RESTORE_INT(flags);

			LOG_MSG("A STA is deleted by application program - %02X:%02X:%02X:%02X:%02X:%02X\n",
				macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
		}
	}

	free_stainfo(priv, pstat);

#ifdef CLIENT_MODE
	if (OPMODE & WIFI_STATION_STATE) {
		OPMODE &= ~(WIFI_AUTH_SUCCESS | WIFI_ASOC_STATE);
		start_clnt_lookup(priv, 0);
	}
#endif

	return 1;
}

#ifdef FREDDY	//mesh related
int del_sta_enc(struct rtl8190_priv *priv, unsigned char *data,int iStatusCode)
{
	struct stat_info *pstat;
	unsigned char macaddr[MACADDRLEN], tmpbuf[3];
	unsigned long flags;
	DOT11_DEAUTHENTICATION_IND Deauthentication_Ind;
	int i;

	if (!netif_running(priv->dev))
		return 0;

	for(i=0; i<MACADDRLEN; i++)
	{
		tmpbuf[0] = data[2*i];
		tmpbuf[1] = data[2*i+1];
		tmpbuf[2] = 0;
		macaddr[i] = (unsigned char)_atoi((char *)tmpbuf, 16);
	}

#ifdef CONFIG_RTK_MESH
	// following add by chuangch 2007/08/30 (IAPP update proxy table)
	//by pepsi 08/03/12 for using PU
	if ((1 == GET_MIB(priv)->dot1180211sInfo.mesh_enable)	// fix: 0000107 2008/10/13
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID) // Spare for Mesh work with Multiple AP (Please see Mantis 0000107 for detail)
			&& IS_ROOT_INTERFACE(priv)
#endif
	)
		HASH_DELETE(priv->proxy_table, macaddr);

#endif	// CONFIG_RTK_MESH

	DEBUG_INFO("del_sta %02X%02X%02X%02X%02X%02X\n",
		macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
	pstat = get_stainfo(priv, macaddr);
	if (pstat == NULL)
		return 0;

	if (!list_empty(&pstat->asoc_list))
	{
		if (IEEE8021X_FUN)
		{
#ifndef WITHOUT_ENQUEUE
			memcpy((void *)Deauthentication_Ind.MACAddr, (void *)macaddr, MACADDRLEN);
			Deauthentication_Ind.EventId = DOT11_EVENT_DEAUTHENTICATION_IND;
			Deauthentication_Ind.IsMoreEvent = 0;
			Deauthentication_Ind.Reason = iStatusCode;
			Deauthentication_Ind.tx_packets = pstat->tx_pkts;
			Deauthentication_Ind.rx_packets = pstat->rx_pkts;
			Deauthentication_Ind.tx_bytes   = pstat->tx_bytes;
			Deauthentication_Ind.rx_bytes   = pstat->rx_bytes;
			DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&Deauthentication_Ind,
						sizeof(DOT11_DEAUTHENTICATION_IND));
#endif
#ifdef INCLUDE_WPA_PSK
			psk_indicate_evt(priv, DOT11_EVENT_DEAUTHENTICATION_IND, macaddr, NULL, 0);
#endif
			event_indicate(priv, macaddr, 2);
		}

		issue_deauth(priv, macaddr, iStatusCode);
		if (pstat->expire_to > 0)
		{
			printk("A STA is deleted by application program - %02X:%02X:%02X:%02X:%02X:%02X\n",macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
			SAVE_INT_AND_CLI(flags);
			cnt_assoc_num(priv, pstat, DECREASE, (char *)__FUNCTION__);
			check_sta_characteristic(priv, pstat, DECREASE);
			RESTORE_INT(flags);

			LOG_MSG("A STA is deleted by application program - %02X:%02X:%02X:%02X:%02X:%02X\n",
				macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
		}
	}
	free_stainfo(priv, pstat);

#ifdef CLIENT_MODE
	if (OPMODE & WIFI_STATION_STATE) {
		OPMODE &= ~(WIFI_AUTH_SUCCESS | WIFI_ASOC_STATE);
		start_clnt_lookup(priv, 0);
	}
#endif

	return 1;
}
#endif

static int write_eeprom(struct rtl8190_priv *priv, unsigned char *data)
{
#if defined(RTL8192SE) || defined(RTL8192SU)
	return -1;
#else
	struct iwpriv_arg *entry;
	int int_idx, retval;
	unsigned char byte_val;
	char *arg_val, tmpbuf[100];

	DEBUG_TRACE;

	DEBUG_INFO("write_eeprom %s\n", data);

	entry = get_eeprom_tbl_entry((char *)data);
	if (entry == NULL) {
		DEBUG_ERR("invalid entry name [%s] !\n", data);
		return -1;
	}

	// search value
	arg_val = (char *)data;
	while (*arg_val && *arg_val != '=')
		arg_val++;

	if (!*arg_val) {
		DEBUG_ERR("entry value empty [%s] !\n", data);
		return -1;
	}
	arg_val++;

	switch (entry->type) {
	case IDX_BYTE_ARRAY_T:
		arg_val = get_arg(arg_val, tmpbuf);
		if (arg_val == NULL) {
			DEBUG_ERR("invalid BYTE_ARRAY entry [%s] !\n", entry->name);
			return -1;
		}
		int_idx = _atoi(tmpbuf, 10);
		if (int_idx+1 > entry->len) {
			DEBUG_ERR("invalid BYTE_ARRAY entry index [%s, %d] !\n", entry->name, int_idx);
			return -1;
		}
		arg_val = get_arg(arg_val, tmpbuf);
		if (arg_val == NULL) {
			DEBUG_ERR("invalid BYTE_ARRAY entry [%s] !\n", entry->name);
			return -1;
		}
		byte_val = (unsigned char)_atoi(tmpbuf, 10);
		ReadAdapterInfo(priv, entry->offset, NULL);
		if (entry->offset == EEPROM_TX_POWER_LEVEL_0)
			priv->EE_TxPower_CCK[int_idx] = byte_val;
		else if (entry->offset == EEPROM_11G_CHANNEL_OFDM_TX_POWER_LEVEL_OFFSET)
			priv->EE_TxPower_OFDM[int_idx] = byte_val;
		retval = WriteAdapterInfo(priv, entry->offset, (void *)int_idx);
		if (retval == 0)
			DEBUG_ERR("write eeprom fail\n");
		break;

	default:
		DEBUG_ERR("invalid entry name [%s] !\n", data);
		break;
	}

	return 0;
#endif
}


static int read_eeprom(struct rtl8190_priv *priv, unsigned char *data)
{
#if defined(RTL8192SE) || defined(RTL8192SU)
	return -1;
#else
	struct iwpriv_arg *entry;
	int i, copy_len, retval;

	DEBUG_TRACE;

	DEBUG_INFO("read_eeprom %s\n", data);

	entry = get_eeprom_tbl_entry((char *)data);
	if (entry == NULL) {
		DEBUG_ERR("invalid entry name [%s] !\n", data);
		return -1;
	}
	copy_len = entry->len;

	switch (entry->type) {
	case BYTE_T:
		retval = ReadAdapterInfo(priv, entry->offset, data);
		if (retval == 0) {
			copy_len = 0;
			PRINT_INFO("read eeprom fail\n");
		}
		else
			PRINT_INFO("byte data: %d\n", *data);
		break;

	case BYTE_ARRAY_T:
	case IDX_BYTE_ARRAY_T:
		retval = ReadAdapterInfo(priv, entry->offset, data);
		if (retval == 0) {
			copy_len = 0;
			PRINT_INFO("read eeprom fail\n");
		}
		else {
			PRINT_INFO("data (hex): ");
			for (i=0; i<entry->len; i++)
				PRINT_INFO("%02x", data[i]);
			PRINT_INFO("\n");
		}
		break;

	default:
		DEBUG_ERR("invalid entry type!\n");
		return 0;
	}

	return copy_len;
#endif
}


#ifdef CONFIG_RTL865X
static int read_eeprom2(struct rtl8190_priv *priv, struct iwpriv_arg *entry, unsigned char *data)
{
	int copy_len, retval;

	if (entry == NULL) {
		return -1;
	}
	copy_len = entry->len;

	switch (entry->type) {
	case BYTE_T:
		retval = ReadAdapterInfo(priv, entry->offset, data);
		if (retval == 0) {
			copy_len = 0;
		}
		break;

	case BYTE_ARRAY_T:
	case IDX_BYTE_ARRAY_T:
		retval = ReadAdapterInfo(priv, entry->offset, data);
		if (retval == 0) {
			copy_len = 0;
		}
		break;

	default:
		copy_len = -1;
		return 0;
	}

	return copy_len;
}
#endif


#if !defined(CONFIG_RTL865X) || !defined(RTL865X_GETSTAINFO_PATCH)
static void get_sta_info(struct rtl8190_priv *priv, sta_info_2_web *pInfo, int size
#ifdef RTK_WOW
				,unsigned int wakeup_on_wlan
#endif
				)
{
	struct list_head *phead, *plist;
	struct stat_info *pstat;
	struct net_device *dev = priv->dev;
	int i;

	memset((char *)pInfo, '\0', sizeof(sta_info_2_web)*size);

	phead = &priv->asoc_list;
	if (!netif_running(dev) || list_empty(phead))
		return;

	plist = phead->next;
	while ((plist != phead) && (size > 0))
	{
		pstat = list_entry(plist, struct stat_info, asoc_list);
		plist = plist->next;
		if ((pstat->state & WIFI_ASOC_STATE) &&
			((pstat->expire_to > 0)
#ifdef RTK_WOW
			|| wakeup_on_wlan
#endif
			)
#ifdef CONFIG_RTK_MESH
			&& (!(GET_MIB(priv)->dot1180211sInfo.mesh_enable) || isSTA(pstat))	// STA ONLY
#endif
#ifdef RTK_WOW
			&& (!wakeup_on_wlan || pstat->is_rtk_wow_sta)
#endif
#ifdef WDS
			&& !(pstat->state & WIFI_WDS)
#endif
		) {
			pInfo->aid = pstat->aid;
			memcpy(pInfo->addr, pstat->hwaddr, 6);
			pInfo->tx_packets = pstat->tx_pkts;
			pInfo->rx_packets = pstat->rx_pkts;
			pInfo->expired_time = pstat->expire_to * 100;
			pInfo->flags = STA_INFO_FLAG_ASOC;
			if (!list_empty(&pstat->sleep_list))
				pInfo->flags |= STA_INFO_FLAG_ASLEEP;
			pInfo->TxOperaRate = pstat->current_tx_rate;
			pInfo->rssi = pstat->rssi;
			pInfo->link_time = pstat->link_time;
			pInfo->tx_fail = pstat->tx_fail;
			pInfo->tx_bytes = pstat->tx_bytes;
			pInfo->rx_bytes = pstat->rx_bytes;

			if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11A)
				pInfo->network = WIRELESS_11A;
			else if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) {
				if (!isErpSta(pstat))
					pInfo->network = WIRELESS_11B;
				else {
					pInfo->network = WIRELESS_11G;
					for (i=0; i<STAT_OPRATE_LEN; i++) {
						if (is_CCK_rate(STAT_OPRATE[i])) {
							pInfo->network |= WIRELESS_11B;
							break;
						}
					}
				}
			}
			else // 11B only
				pInfo->network = WIRELESS_11B;
			if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
				if (pstat->ht_cap_len) {
					pInfo->network |= WIRELESS_11N;
					if (pstat->ht_current_tx_info & TX_USE_40M_MODE)
						pInfo->ht_info |= TX_USE_40M_MODE;
					if (pstat->ht_current_tx_info & TX_USE_SHORT_GI)
						pInfo->ht_info |= TX_USE_SHORT_GI;
				}
			}

			pInfo++;
			size--;
		}
	}
}
#else /* !defined(CONFIG_RTL865X) || !defined(RTL865X_GETSTAINFO_PATCH) */
static int get_sta_info(struct rtl8190_priv *priv, sta_info_2_web *pInfo, int size
#ifdef RTK_WOW
				,unsigned int wakeup_on_wlan
#endif
				)
{
	struct list_head *phead, *plist;
	struct stat_info *pstat;
	struct net_device *dev = priv->dev;
	int returned=0;

	memset((char *)pInfo, '\0', sizeof(sta_info_2_web)*size);
	phead = &priv->asoc_list;
	if (!netif_running(dev) || list_empty(phead))
	{
		if(list_empty(phead)) {
			DEBUG_INFO("%s List empty!!\n", dev->name);
		}
		return 0;
	}

	plist = phead->next;
	while ((plist != phead) && (size > 0))
	{
		pstat = list_entry(plist, struct stat_info, asoc_list);
		plist = plist->next;
		if ((pstat->state & WIFI_ASOC_STATE) &&
			((pstat->expire_to > 0)
#ifdef RTK_WOW
			|| wakeup_on_wlan
#endif
			)
#ifdef CONFIG_RTK_MESH
			&& (!(GET_MIB(priv)->dot1180211sInfo.mesh_enable) || isSTA(pstat))	// STA ONLY
#endif
#ifdef RTK_WOW
			&& (!wakeup_on_wlan || pstat->is_rtk_wow_sta)
#endif
#ifdef WDS
			&& !(pstat->state & WIFI_WDS)
#endif
		) {
			pInfo->aid = pstat->aid;
			memcpy(pInfo->addr, pstat->hwaddr, 6);
			pInfo->tx_packets = pstat->tx_pkts;
			pInfo->rx_packets = pstat->rx_pkts;
			pInfo->expired_time = pstat->expire_to * 100;
			pInfo->flags = STA_INFO_FLAG_ASOC;
			if (!list_empty(&pstat->sleep_list))
				pInfo->flags |= STA_INFO_FLAG_ASLEEP;
#ifdef RTL865X_EXTRA_STAINFO
			if(priv->pmib->dot11BssType.net_work_type & WIRELESS_11A) {
				pInfo->flags |= STA_INFO_FLAG_80211A;
				pInfo->flags &= ~STA_INFO_FLAG_80211B;
			}
			else if(!isErpSta(pstat)) {
				pInfo->flags &= ~STA_INFO_FLAG_80211A;
				pInfo->flags |= STA_INFO_FLAG_80211B;
			}
			else {
				pInfo->flags &= ~STA_INFO_FLAG_80211A;
				pInfo->flags &= ~STA_INFO_FLAG_80211B;//it supports extended rateset, 11g station
			}
#endif
			pInfo->TxOperaRate = pstat->current_tx_rate;
			pInfo->rssi = pstat->rssi;
			pInfo->link_time = pstat->link_time;
			pInfo->tx_fail = pstat->tx_fail;

			pInfo++;
			size--;
			returned++;
		}
	}

	return returned;
}
#endif /* !defined(CONFIG_RTL865X) || !defined(RTL865X_GETSTAINFO_PATCH) */


#ifdef CONFIG_RTL865X
int get_eeprom_info(struct rtl8190_priv *priv, unsigned char *data)
{
	int i=0, pos=0;
	int arg_num = sizeof(eeprom_table)/sizeof(struct iwpriv_arg);

	for (i=0; i<arg_num; i++) {
		memset(&data[pos], 0xff, eeprom_table[i].len);
		pos += read_eeprom2(priv, &eeprom_table[i], &data[pos]);
	}
	return pos;
}
#endif


static void get_bss_info(struct rtl8190_priv *priv, bss_info_2_web *pBss)
{
	struct net_device *dev = priv->dev;
#ifdef CLIENT_MODE
	struct stat_info *pstat;
#endif

	memset(pBss, '\0', sizeof(bss_info_2_web));

	if (!netif_running(dev)) {
		pBss->state = STATE_DISABLED;
		return;
	}

	if (priv->pmib->miscEntry.func_off) {
		pBss->state = STATE_DISABLED;
		return;
	}

	if (OPMODE & WIFI_AP_STATE)
		pBss->state = STATE_STARTED;
#ifdef CLIENT_MODE
	else {
		switch (priv->join_res) {
		case STATE_Sta_No_Bss:
			pBss->state = STATE_SCANNING;
			break;
		case STATE_Sta_Bss:
			if (IEEE8021X_FUN) {
				pstat = get_stainfo(priv, BSSID);
				if (pstat == NULL)
					return;
				if (pstat->ieee8021x_ctrlport)
					pBss->state = STATE_CONNECTED;
				else
					pBss->state = STATE_WAITFORKEY;
			}
			else
				pBss->state = STATE_CONNECTED;
			break;
		case STATE_Sta_Ibss_Active:
			pBss->state = STATE_CONNECTED;
			break;
		case STATE_Sta_Ibss_Idle:
			pBss->state = STATE_STARTED;
			break;
		default:
			pBss->state = STATE_SCANNING;
			break;
		}
	}
#endif

	if (priv->pmib->dot11StationConfigEntry.autoRate)
		pBss->txRate = find_rate(priv, NULL, 1, 0);
	else
		pBss->txRate = get_rate_from_bit_value(priv->pmib->dot11StationConfigEntry.fixedTxRate);
	memcpy(pBss->ssid, SSID, SSID_LEN);
	pBss->ssid[SSID_LEN] = '\0';
	if (OPMODE & WIFI_SITE_MONITOR)
		pBss->channel = priv->site_survey.ss_channel;
	else
		pBss->channel = priv->pmib->dot11RFEntry.dot11channel;

	if (pBss->state == STATE_STARTED || pBss->state == STATE_CONNECTED) {
#ifdef UNIVERSAL_REPEATER
		if (IS_VXD_INTERFACE(priv) && (OPMODE & WIFI_AP_STATE))
			if (IS_DRV_OPEN(priv))
				memcpy(pBss->bssid, priv->pmib->dot11Bss.bssid, MACADDRLEN);
			else
				memset(pBss->bssid, '\0', MACADDRLEN);
		else
#endif
		memcpy(pBss->bssid, BSSID, MACADDRLEN);
#ifdef CLIENT_MODE
		if (priv->join_res == STATE_Sta_Bss) {
			pstat = get_stainfo(priv, BSSID);
			if (pstat) {
				pBss->rssi = pstat->rssi;
				pBss->sq = pstat->sq;
			}
		}
#endif
	}
	else {
		memset(pBss->bssid, '\0', MACADDRLEN);
		if (pBss->state == STATE_DISABLED)
			pBss->channel = 0;
	}
}


#ifdef WDS
static int get_wds_info(struct rtl8190_priv *priv, web_wds_info *pWds)
{
	int i, j=0;
	struct stat_info *pstat;

	memset(pWds, '\0', NUM_WDS*sizeof(web_wds_info));

	for (j=0, i=0; i<NUM_WDS && i<priv->pmib->dot11WdsInfo.wdsNum; i++) {
		if (!netif_running(priv->wds_dev[i]))
			continue;

		memcpy(pWds[j].addr, priv->pmib->dot11WdsInfo.entry[i].macAddr, 6);

		pstat = get_stainfo(priv, pWds[j].addr);

		pWds[j].state = STATE_WDS_ACTIVE;
		pWds[j].tx_packets = pstat->tx_pkts;
		pWds[j].rx_packets = pstat->rx_pkts;
		pWds[j].tx_errors = pstat->tx_fail;
		pWds[j].TxOperaRate = pstat->current_tx_rate;
		j++;
	}

	return (sizeof(web_wds_info)*j);
}
#endif // WDS


static int set_sta_txrate(struct rtl8190_priv *priv, struct _wlan_sta_rateset *rate_set)
{
	struct stat_info *pstat;

	if (!netif_running(priv->dev))
		return 0;

	pstat = get_stainfo(priv, rate_set->mac);
	if (pstat == NULL)
		return 0;
	if (!(pstat->state & WIFI_ASOC_STATE))
		return 0;
	if (priv->pmib->dot11StationConfigEntry.autoRate) {
		DEBUG_INFO("Auto rate turned on. Can't set rate\n");
		return 0;
	}

	pstat->current_tx_rate = rate_set->txrate;
	return 1;
}


#ifdef MICERR_TEST
static int issue_mic_err_pkt(struct rtl8190_priv *priv)
{
	struct sk_buff *skb;
	struct wlan_ethhdr_t *pethhdr;

	if (!(OPMODE & WIFI_STATION_STATE)) {
		printk("Fail: not in client mode\n");
		return 0;
	}

	printk("Send MIC error packet to AP...\n");
	skb = dev_alloc_skb(64);
	if (skb != NULL)
	{
		skb->dev = priv->dev;
		pethhdr = (struct wlan_ethhdr_t *)(skb->data);
		memcpy(pethhdr->daddr, BSSID, MACADDRLEN);
		memcpy(pethhdr->saddr, GET_MY_HWADDR, MACADDRLEN);
		pethhdr->type = 0x888e;

		memset(skb->data+WLAN_ETHHDR_LEN, 0xa5, 32);
		skb_put(skb, WLAN_ETHHDR_LEN+32);

		priv->micerr_flag = 1;
		if (rtl8190_start_xmit(skb, priv->dev))
			rtl_kfree_skb(priv, skb, _SKB_TX_);
	}
	else
	{
		printk("Can't allocate sk_buff\n");
	}
	return 0;
}


static int issue_mic_rpt_pkt(struct rtl8190_priv *priv, unsigned char *data)
{
	struct sk_buff *skb;
	struct wlan_ethhdr_t *pethhdr;
	int format;
	unsigned char pattern[] = {0x01, 0x03, 0x00, 0x5f, 0xfe, 0x0d, 0x00, 0x00};

	if (!(OPMODE & WIFI_STATION_STATE)) {
		printk("Fail: not in client mode\n");
		return 0;
	}

	if (!strcmp(data, "xp"))
		format = 1;
	else if (!strcmp(data, "funk"))
		format = 2;
	else {
		printk("Usage: iwpriv wlanx mic_report {xp | funk}\n");
		return 0;
	}

	printk("Send MIC report (%s format) to AP...\n", (format==1)? "XP":"Funk");
	skb = dev_alloc_skb(64);
	if (skb != NULL)
	{
		skb->dev = priv->dev;
		pethhdr = (struct wlan_ethhdr_t *)(skb->data);
		memcpy(pethhdr->daddr, BSSID, MACADDRLEN);
		memcpy(pethhdr->saddr, GET_MY_HWADDR, MACADDRLEN);
		pethhdr->type = 0x888e;

		if (format == 2)
			pattern[5] = 0x0f;
		memcpy(skb->data+WLAN_ETHHDR_LEN, pattern, sizeof(pattern));
		skb_put(skb, WLAN_ETHHDR_LEN+sizeof(pattern));

		if (rtl8190_start_xmit(skb, priv->dev))
			rtl_kfree_skb(priv, skb, _SKB_TX_);
	}
	else
	{
		printk("Can't allocate sk_buff\n");
	}
	return 0;
}
#endif // MICERR_TEST


#ifdef D_ACL	//mesh related

static int iwpriv_atoi(unsigned char *data, unsigned char *buf, int len)
{
	unsigned char tmpbuf[20] = {'\0'};
	unsigned char ascii_addr[12] = {'\0'};
	unsigned char hex_addr[6] = {'\0'};
	int i=0;

	if( (len - 1) == MACADDRLEN ){	//user send 6 byte mac address
		if(copy_from_user(buf, (void *)data, len))
			return -1;
		return 0;
	}
	else if( (len - 1) == MACADDRLEN*2 ){ //user send 12 byte mac string
		if(copy_from_user(tmpbuf, (void *)data, len))
			return -1;

		strcpy(ascii_addr, tmpbuf);
		strcpy(buf+MACADDRLEN,tmpbuf);

	  	for(i = 0; i < MACADDRLEN*2; i++){
        	   	if( '0' <= ascii_addr[i]  && ascii_addr[i] <= '9')
        			ascii_addr[i] -= 48;
               		else if( 'A' <= ascii_addr[i] && ascii_addr[i] <= 'F' )
                		ascii_addr[i] -= 55;
			else if( 'a' <= ascii_addr[i] && ascii_addr[i] <= 'f' )
				ascii_addr[i] -= 87;
	                printk("%d", ascii_addr[i]);
		}

                for(i = 0; i < MACADDRLEN*2; i+=2)
                	hex_addr[i>>1] = (ascii_addr[i] << 4) | (ascii_addr[i+1]);

		memcpy(buf,hex_addr,MACADDRLEN);
		DEBUG_INFO("in iwpriv_atoi function\n");
		return 0;
	}
	else{
		DEBUG_ERR("Wrong input format\n");
		return -1;
	}

}
#endif

static int acl_add_cmd(struct rtl8190_priv *priv, unsigned char *data, int len)
{
	struct list_head		*phead, *plist, *pnewlist;
	struct wlan_acl_node	*paclnode;
	unsigned char macaddr[6];

	if (copy_from_user((void *)macaddr, (void *)data, 6))
		return -1;

#ifdef D_ACL	//tsananiu	(mesh related)
	unsigned char tmpbuf[20] = {'\0'};
	unsigned char tmp_add[12] = {'\0'};
	int i;
	if(copy_from_user(tmpbuf, (void *)data, len))
		return -1;

	if( (len - 1) == 6 ){	//user send 6 byte mac address
		for(i = 0; i < 6; i++)
			macaddr[i] = tmpbuf[i];
	}
	else if( (len - 1) == 12 ){ //user send 12 byte mac string

		strcpy(tmp_add, tmpbuf);

	  	for(i = 0; i < 12; i++){
        	   	if( '0' <= tmp_add[i]  && tmp_add[i] <= '9')
        			tmp_add[i] -= 48;
               		else if( 'A' <= tmp_add[i] && tmp_add[i] <= 'F' )
                		tmp_add[i] -= 55;
			else if( 'a' <= tmp_add[i] && tmp_add[i] <= 'f' )
				tmp_add[i] -= 87;
	                printk("%d", tmp_add[i]);
		}

                for(i = 0; i < 12; i+=2){
                	macaddr[i>>1] = (tmp_add[i] << 4) | (tmp_add[i+1]);
                }
	}

	else{
		printk("Wrong input format\n");
	}
	DEBUG_INFO("in add function\n");
	//len = 6;
#else
	if (copy_from_user((void *)macaddr, (void *)data, 6))
		return -1;
#endif//tsananiu//

	// first of all, check if this address has been in acl_list;
	phead = &priv->wlan_acl_list;
	plist = phead->next;

	DEBUG_INFO("Adding %X:%X:%X:%X:%X:%X to acl_table\n",
				macaddr[0],macaddr[1],macaddr[2],
				macaddr[3],macaddr[4],macaddr[5]);

	while(plist != phead)
	{
		paclnode = list_entry(plist, struct wlan_acl_node, list);
		plist = plist->next;

		if (!(memcmp((void *)macaddr, paclnode->addr, 6)))
		{
			DEBUG_INFO("mac-addr %02X%02X%02X%02X%02X%02X has been in acl_list\n",
					macaddr[0], macaddr[1], macaddr[2],
					macaddr[3], macaddr[4], macaddr[5]);
			return 0;
		}
	}

	if (list_empty(&priv->wlan_aclpolllist))
	{
		DEBUG_INFO("acl_poll is full!\n");
		return 0;
	}

	pnewlist = (priv->wlan_aclpolllist.next);
	list_del_init(pnewlist);

	paclnode = list_entry(pnewlist, struct wlan_acl_node, list);

	memcpy((void *)paclnode->addr, macaddr, 6);

	if (len == 6)
		paclnode->mode = (unsigned char)priv->pmib->dot11StationConfigEntry.dot11AclMode;
	else
		paclnode->mode = data[6];

	list_add_tail(pnewlist, phead);

	return 0;
}


static int acl_remove_cmd(struct rtl8190_priv *priv, unsigned char *data, int len)
{
	struct list_head *phead, *plist;
	struct wlan_acl_node *paclnode;
	unsigned char macaddr[6];

	if (data) {
#ifdef D_ACL	//tsananiu	(mesh related)
		int i;
		unsigned char tmpbuf[20] = {'\0'};
		unsigned char tmp_add[12] = {'\0'};

		if(copy_from_user(tmpbuf, (void *)data, len))
			return -1;

		if( (len - 1) == 6 ){	//user send 6 byte mac address
			for(i = 0; i < 6; i++)
				macaddr[i] = tmpbuf[i];
		}
		else if( (len - 1) == 12 ){ //user send 12 byte mac string

			strcpy(tmp_add, tmpbuf);

		  	for(i = 0; i < 12; i++){
	        	   	if( '0' <= tmp_add[i]  && tmp_add[i] <= '9')
	        			tmp_add[i] -= 48;
	               		else if( 'A' <= tmp_add[i] && tmp_add[i] <= 'F' )
	                		tmp_add[i] -= 55;
				else if( 'a' <= tmp_add[i] && tmp_add[i] <= 'f' )
					tmp_add[i] -= 87;
		                DEBUG_INFO("%d", tmp_add[i]);
			}

	                for(i = 0; i < 12; i+=2){
	                	macaddr[i>>1] = (tmp_add[i] << 4) | (tmp_add[i+1]);
	                }
		}
		else{
			DEBUG_ERR("Wrong input format\n");
			return -1;
		}
		DEBUG_INFO("in remove function\n");
		DEBUG_INFO("%X:%X:%X:%X:%X:%X\n",
	                macaddr[0],macaddr[1],macaddr[2],
	                macaddr[3],macaddr[4],macaddr[5]);
	//len = 6;
#else
		if (copy_from_user((void *)macaddr, (void *)data, 6))
			return -1;

		DEBUG_INFO("Delete %X:%X:%X:%X:%X:%X to acl_table\n",
				macaddr[0],macaddr[1],macaddr[2],
				macaddr[3],macaddr[4],macaddr[5]);
#endif//tsananiu//
	}

	phead = &priv->wlan_acl_list;

	if (list_empty(phead)) // nothing to remove
		return 0;

	plist = phead->next;

	while(plist != phead)
	{
		paclnode = list_entry(plist, struct wlan_acl_node, list);
		plist = plist->next;

		if (!(memcmp((void *)macaddr, paclnode->addr, 6)))
		{
			list_del_init(&paclnode->list);
			list_add_tail(&paclnode->list, &priv->wlan_aclpolllist);
			return 0;
		}
	}

	if (data) {
		DEBUG_INFO("Delete %X:%X:%X:%X:%X:%X is not in acl_table\n",
                macaddr[0],macaddr[1],macaddr[2],
                macaddr[3],macaddr[4],macaddr[5]);
	}
	return 0;
}


static int acl_query_cmd(struct rtl8190_priv *priv, unsigned char *data)
{
	struct list_head	*phead, *plist;
	struct wlan_acl_node	*paclnode;

	phead = &priv->wlan_acl_list;

	if (list_empty(phead)) // nothing to remove
		return 0;

	plist = phead->next;

	while(plist != phead)
	{
		paclnode = list_entry(plist, struct wlan_acl_node, list);
		plist = plist->next;

		if (copy_to_user((void *)data, paclnode->addr, 6))
			return -1;
		data += 6;
	}
	return 0;
}


#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_)
// Copy from acl_add_cmd
static int mesh_acl_add_cmd(struct rtl8190_priv *priv, unsigned char *data, int len)
{
	struct list_head		*phead, *plist, *pnewlist;
	struct wlan_acl_node	*paclnode;
	unsigned char macaddr[MACADDRLEN];

	if (copy_from_user((void *)macaddr, (void *)data, MACADDRLEN))
		return -1;

	// first of all, check if this address has been in acl_list;
	phead = &priv->mesh_acl_list;
	plist = phead->next;

	DEBUG_INFO("Adding %X:%X:%X:%X:%X:%X to mesh_acl_table\n",
				macaddr[0],macaddr[1],macaddr[2],
				macaddr[3],macaddr[4],macaddr[5]);

	while(plist != phead)
	{
		paclnode = list_entry(plist, struct wlan_acl_node, list);
		plist = plist->next;

		if (!(memcmp((void *)macaddr, paclnode->addr, MACADDRLEN)))
		{
			DEBUG_INFO("mac-addr %02X%02X%02X%02X%02X%02X has been in mesh_acl_list\n",
					macaddr[0], macaddr[1], macaddr[2],
					macaddr[3], macaddr[4], macaddr[5]);
			return 0;
		}
	}

	if (list_empty(&priv->mesh_aclpolllist))
	{
		DEBUG_INFO("mesh_acl_poll is full!\n");
		return 0;
	}

	pnewlist = (priv->mesh_aclpolllist.next);
	list_del_init(pnewlist);

	paclnode = list_entry(pnewlist, struct wlan_acl_node, list);

	memcpy((void *)paclnode->addr, macaddr, MACADDRLEN);

	if (len == 6)
		paclnode->mode = (unsigned char)priv->pmib->dot1180211sInfo.mesh_acl_mode;
	else
		paclnode->mode = data[6];

	list_add_tail(pnewlist, phead);

	return 0;
}


// Copy from acl_remove_cmd
static int mesh_acl_remove_cmd(struct rtl8190_priv *priv, unsigned char *data, int len)
{
	struct list_head *phead, *plist;
	struct wlan_acl_node *paclnode;
	unsigned char macaddr[MACADDRLEN];

	if (data) {
		if (copy_from_user((void *)macaddr, (void *)data, 6))
			return -1;

		DEBUG_INFO("Delete %X:%X:%X:%X:%X:%X to mesh_acl_table\n",
				macaddr[0],macaddr[1],macaddr[2],
				macaddr[3],macaddr[4],macaddr[5]);
	}

	phead = &priv->mesh_acl_list;

	if (list_empty(phead)) // nothing to remove
		return 0;

	plist = phead->next;

	while(plist != phead)
	{
		paclnode = list_entry(plist, struct wlan_acl_node, list);
		plist = plist->next;

		if (!(memcmp((void *)macaddr, paclnode->addr, MACADDRLEN)))
		{
			list_del_init(&paclnode->list);
			list_add_tail(&paclnode->list, &priv->mesh_aclpolllist);
			return 0;
		}
	}

	if (data) {
		DEBUG_INFO("Delete %X:%X:%X:%X:%X:%X is not in mesh_acl_table\n",
                macaddr[0],macaddr[1],macaddr[2],
                macaddr[3],macaddr[4],macaddr[5]);
	}
	return 0;
}


// Copy from acl_query_cmd
static int mesh_acl_query_cmd(struct rtl8190_priv *priv, unsigned char *data)
{
	struct list_head	*phead, *plist;
	struct wlan_acl_node	*paclnode;

	phead = &priv->mesh_acl_list;

	if (list_empty(phead)) // nothing to remove
		return 0;

	plist = phead->next;

	while(plist != phead)
	{
		paclnode = list_entry(plist, struct wlan_acl_node, list);
		plist = plist->next;

		if (copy_to_user((void *)data, paclnode->addr, 6))
			return -1;
		data += 6;
	}
	return 0;
}
#endif	// CONFIG_RTK_MESH && _MESH_ACL_ENABLE_


static void get_misc_data(struct rtl8190_priv *priv, struct _misc_data_ *pdata)
{
	memset(pdata, '\0', sizeof(struct _misc_data_));

	pdata->mimo_tr_hw_support = GET_HW(priv)->MIMO_TR_hw_support;

	// get number of tx path
	if (get_rf_mimo_mode(priv) == MIMO_1T2R)
		pdata->mimo_tr_used = 1;
	else if (get_rf_mimo_mode(priv) == MIMO_1T1R)
		pdata->mimo_tr_used = 1;
	else if (get_rf_mimo_mode(priv) == MIMO_2T2R)
		pdata->mimo_tr_used = 2;
	else	// MIMO_2T4R
		pdata->mimo_tr_used = 2;

	return;
}


static int rtl8190_ss_req(struct rtl8190_priv *priv, unsigned char *data, int len)
{
	INT8 ret = 0;
#ifdef CONFIG_RTK_MESH
	// by GANTOE for manual site survey 2008/12/25
	// inserted by Joule for simple channel unification protocol 2009/01/06
	if((priv->auto_channel &0x30) && timer_pending(&priv->ss_timer))
		ret = -2;
	else
#endif
	if (!netif_running(priv->dev) || priv->ss_req_ongoing)
		ret = -1;
	else
		ret = 0;

	if (!ret)	// now, let's start site survey
	{
		priv->ss_ssidlen = 0;
		DEBUG_INFO("start_clnt_ss, trigger by %s, ss_ssidlen=0\n", (char *)__FUNCTION__);

#ifdef WIFI_SIMPLE_CONFIG
		if (len == 2)
			priv->ss_req_ongoing = 2;	// WiFi-Simple-Config scan-req
		else
#endif
			priv->ss_req_ongoing = 1;
		start_clnt_ss(priv);
	}

	if (copy_to_user((void *)data, (void *)&ret, 1))
		return -1;

	return 0;
}


static int rtl8190_get_ss_status(struct rtl8190_priv *priv, unsigned char *data)
{
	UINT8 flags;
	INT8 ret = 0;

	if (copy_from_user((void *)&flags, (void *)(data), 1))
		return -1;

	if (!netif_running(priv->dev) || priv->ss_req_ongoing)
	{
		ret = -1;
		if (copy_to_user((void *)(data), (void *)&ret, 1))
			return -1;
	}
	else if (flags == 1)
	{
		ret = priv->site_survey.count_backup;
		if (copy_to_user((void *)(data), (void *)&ret, 1))
			return -1;
	}
	else if (flags == 0)
	{
		ret = priv->site_survey.count_backup;
		if (copy_to_user((void *)data, (void *)&ret, 1))
			return -1;
		// now we should report data base.
		if (copy_to_user((void *)(data+4), priv->site_survey.bss_backup,
				sizeof(struct bss_desc)*priv->site_survey.count_backup))
			return -1;
	}

#ifdef WIFI_SIMPLE_CONFIG
	else if (flags == 2) { // get simple-config scan result, append WSC IE
		ret = priv->site_survey.count_backup;
		if (copy_to_user((void *)data, (void *)&ret, 1))
			return -1;
		// now we should report data base.
		if (copy_to_user((void *)(data+4), priv->site_survey.ie_backup,
				sizeof(struct wps_ie_info)*priv->site_survey.count_backup))
			return -1;
	}
#endif

	return 0;
}


#ifdef CLIENT_MODE
static int check_bss_encrypt(struct rtl8190_priv *priv)
{
	// no encryption
	if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == 0)
	{
		if (priv->pmib->dot11Bss.capability & BIT(4))
			return FAIL;
		else
			return SUCCESS;
	}
	// legacy encryption
	else if (!IEEE8021X_FUN &&
		((priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_) ||
		 (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_)))
	{
		if ((priv->pmib->dot11Bss.capability & BIT(4)) == 0)
			return FAIL;
		else if (priv->pmib->dot11Bss.t_stamp[0] != 0)
			return FAIL;
		else
			return SUCCESS;
	}
	// WPA/WPA2
	else
	{
		if ((priv->pmib->dot11Bss.capability & BIT(4)) == 0)
			return FAIL;
		else if (priv->pmib->dot11Bss.t_stamp[0] == 0)
			return FAIL;
		else if ((priv->pmib->dot11RsnIE.rsnie[0] == _RSN_IE_1_) &&
			((priv->pmib->dot11Bss.t_stamp[0] & 0x0000ffff) == 0))
			return FAIL;
		else if ((priv->pmib->dot11RsnIE.rsnie[0] == _RSN_IE_2_) &&
			((priv->pmib->dot11Bss.t_stamp[0] & 0xffff0000) == 0))
			return FAIL;
		else
			return SUCCESS;
	}
}


static int	rtl8190_join(struct rtl8190_priv *priv, unsigned char *data)
{
	INT8 ret = 0;
	char tmpbuf[33];

	if (!netif_running(priv->dev))
		ret = 2;
	else if (priv->ss_req_ongoing)
		ret = 1;
	else
		ret = 0;

	if (!ret)	// now, let's start site survey and join
	{
		if (copy_from_user((void *)&(priv->pmib->dot11Bss), (void *)data, sizeof(struct bss_desc)))
			return -1;

// button 2009.05.21
// derive  PMK with slelected SSID
#ifdef INCLUDE_WPA_PSK
		if (priv->pmib->dot1180211AuthEntry.dot11EnablePSK)
		{
			strcpy(priv->pmib->dot11StationConfigEntry.dot11DesiredSSID, priv->pmib->dot11Bss.ssid);
			psk_init(priv);
		}
#endif

#ifdef WIFI_SIMPLE_CONFIG
		if (priv->pmib->wscEntry.wsc_enable && (priv->pmib->dot11Bss.bsstype&WIFI_WPS)) {
			priv->pmib->dot11Bss.bsstype &= ~WIFI_WPS;
			priv->wps_issue_join_req = 1;
		}
		else
#endif
		{
			if (check_bss_encrypt(priv) == FAIL) {
				DEBUG_INFO("Encryption mismatch!\n");
				ret = 2;
				if (copy_to_user((void *)data, (void *)&ret, 1))
					return -1;
				else
					return 0;
			}
		}

		if ((priv->pmib->dot11Bss.ssidlen == 0) || (priv->pmib->dot11Bss.ssid[0] == '\0')) {
			DEBUG_INFO("Join to a hidden AP!\n");
			ret = 2;
			if (copy_to_user((void *)data, (void *)&ret, 1))
				return -1;
			else
				return 0;
		}

#ifdef UNIVERSAL_REPEATER
		disable_vxd_ap(GET_VXD_PRIV(priv));
#endif

		memcpy(tmpbuf, priv->pmib->dot11Bss.ssid, priv->pmib->dot11Bss.ssidlen);
		tmpbuf[priv->pmib->dot11Bss.ssidlen] = '\0';
		DEBUG_INFO("going to join bss: %s\n", tmpbuf);

		memcpy(SSID2SCAN, priv->pmib->dot11Bss.ssid, priv->pmib->dot11Bss.ssidlen);
		SSID2SCAN_LEN = priv->pmib->dot11Bss.ssidlen;

		SSID_LEN = SSID2SCAN_LEN;
		memcpy(SSID, SSID2SCAN, SSID_LEN);

		memset(BSSID, 0, MACADDRLEN);

		priv->join_req_ongoing = 1;
		priv->authModeRetry = 0;
		start_clnt_join(priv);
	}

	if (copy_to_user((void *)data, (void *)&ret, 1))
		return -1;

	return 0;
}

#ifdef CONFIG_RTK_MESH
// ==== inserted by GANTOE for site survey 2008/12/25 ====
// This function might be modifed when the mesh peerlink precedure has been completed
static int rtl8190_join_mesh (struct rtl8190_priv *priv, unsigned char* meshid, int meshid_len, int channel, int reset)
{
	int i, ret = -1;
	unsigned long flags;

	SAVE_INT_AND_CLI(flags);
	for(i = 0; i < priv->site_survey.count; i++)
	{
		if(reset || (!memcmp(meshid, priv->site_survey.bss[i].meshid, meshid_len) && priv->site_survey.bss[i].channel == channel))
		{
			SwChnl(priv, priv->site_survey.bss[i].channel, 0); // in this version, automatically establishing link
			priv->pmib->dot11RFEntry.dot11channel = priv->site_survey.bss[i].channel;
			memcpy(GET_MIB(priv)->dot1180211sInfo.mesh_id, meshid, meshid_len);
// fixed by Joule 2009.01.10
			GET_MIB(priv)->dot1180211sInfo.mesh_id[meshid_len]=0;
			update_beacon(priv);
			ret = 0;
		}
	}
	RESTORE_INT(flags);
	return ret;
}

// This function might be removed when the mesh peerlink precedure has been completed
static int rtl8190_check_mesh_link (struct rtl8190_priv *priv, unsigned char* macaddr)
{
	int ret = -1;
	unsigned long flags;
	struct stat_info *pstat;
	struct list_head *phead, *plist, *pprevlist;

	SAVE_INT_AND_CLI(flags);
	phead= &priv->mesh_mp_hdr;
	plist = phead->next;
	pprevlist = phead;

	while(plist != phead)
	{
		pstat = list_entry(plist, struct stat_info, mesh_mp_ptr);
		if(!memcmp(pstat->hwaddr, macaddr, 6))
		{
			ret = 0;
			break;
		}
		plist = plist->next;
		pprevlist = plist->prev;
	}
	RESTORE_INT(flags);
	return ret;
}
#endif

static int rtl8190_join_status(struct rtl8190_priv *priv, unsigned char *data)
{
	INT8 ret = 0;

	if (!netif_running(priv->dev) || priv->join_req_ongoing)
		ret = -1;	// pending
	else
		ret = priv->join_res;

	if (copy_to_user((void *)data, (void *)&ret, 1))
		return -1;

	return 0;
}
#endif // CLIENT_MODE

#ifdef	SUPPORT_TX_MCAST2UNI
static void AddDelMCASTGroup2STA(struct rtl8190_priv *priv, unsigned char *mac2addr, int add)
{
	int i, free=-1, found=0;
	struct stat_info	*pstat;
	struct list_head	*phead, *plist;
	phead = &priv->asoc_list;
	plist = phead->next;

	while (plist != phead) {
		pstat = list_entry(plist, struct stat_info, asoc_list);
		plist = plist->next;

		// Search from SA stat list. If found check if mc entry is existed in table
		if (((OPMODE & WIFI_AP_STATE) && !memcmp(pstat->hwaddr, mac2addr+6 , 6))
#ifdef CLIENT_MODE
				|| !(OPMODE & WIFI_AP_STATE)
#endif
			) {
#ifdef WDS
			if ((OPMODE & WIFI_AP_STATE) && (pstat->state & WIFI_WDS))
				continue;	// Do not need to mc2uni coNversion in WDS
#endif
			for (i=0; i<MAX_IP_MC_ENTRY; i++) {
				if (pstat->ipmc[i].used && !memcmp(pstat->ipmc[i].mcmac, mac2addr, 6)) {
					found = 1;
					break;
				}
				if (free == -1 && !pstat->ipmc[i].used)
					free = i;
			}

			if (found) {
				if (!add) { // delete entry
					pstat->ipmc[i].used = 0;
					pstat->ipmc_num--;
					if (pstat->ipmc_num == 1) { // delete igmp-query entry
						memcpy(&mac2addr[3], "\x00\x00\x01", 3);
						AddDelMCASTGroup2STA(priv, mac2addr, 0);
					}
				}
			}
			else { // not found
				if (!add) {
					printk("%s: Delete MC entry not found!\n", priv->dev->name);
				}
				else { // add entry
					if (free == -1)  { // no free entry
						printk("%s: MC entry full!\n", priv->dev->name);
					}
					else {
						memcpy(pstat->ipmc[free].mcmac, mac2addr, 6);
						pstat->ipmc[free].used	= 1;
						pstat->ipmc_num++;
						if (pstat->ipmc_num == 1) { // add igmp-query entry
							memcpy(&mac2addr[3], "\x00\x00\x01", 3);
							AddDelMCASTGroup2STA(priv, mac2addr, 1);
						}
					}
				}
			}
		}
	}
}
#endif // SUPPORT_TX_MCAST2UNI


#ifdef DRVMAC_LB
void drvmac_loopback(struct rtl8190_priv *priv)
{
	unsigned long ioaddr = priv->pshare->ioaddr;
	struct stat_info *pstat;
	unsigned char *da = priv->pmib->miscEntry.lb_da;

	// prepare station info
	if (memcmp(da, "\x0\x0\x0\x0\x0\x0", 6) && !IS_MCAST(da))
	{
		pstat = get_stainfo(priv, da);
		if (pstat == NULL)
		{
			pstat = alloc_stainfo(priv, da, -1);
			pstat->state = WIFI_AUTH_SUCCESS | WIFI_ASOC_STATE;
			memcpy(pstat->bssrateset, AP_BSSRATE, AP_BSSRATE_LEN);
			pstat->bssratelen = AP_BSSRATE_LEN;
			pstat->expire_to = 30000;
			list_add_tail(&pstat->asoc_list, &priv->asoc_list);
			cnt_assoc_num(priv, pstat, INCREASE, (char *)__FUNCTION__);
			if (QOS_ENABLE)
				pstat->QosEnabled = 1;
			if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
				pstat->ht_cap_len = priv->ht_cap_len;
				memcpy(&pstat->ht_cap_buf, &priv->ht_cap_buf, priv->ht_cap_len);
			}
			pstat->current_tx_rate = find_rate(priv, pstat, 1, 0);
			update_fwtbl_asoclst(priv, pstat);
			add_update_RATid(priv, pstat);
		}
	}

	// accept all packets
	RTL_W32(_RCR_, RTL_R32(_RCR_) | _AAP_);

	// enable MAC loopback
	RTL_W32(_CPURST_, RTL_R32(_CPURST_) | BIT(16) | BIT(17));
}
#endif // DRVMAC_LB


int dynamic_RF_pwr_adj(struct rtl8190_priv *priv, unsigned char *data)
{
	unsigned char *ptr = data;
	int index, minus_sign, adj_value;
	unsigned int writeVal, readVal;
	unsigned char byte0, byte1, byte2, byte3;
	unsigned char RF6052_MAX_TX_PWR = 0x3F;

	if (*ptr == '-') {
		minus_sign = 1;
		ptr++;
	}
	else if (*ptr == '+') {
		minus_sign = 0;
		ptr++;
	}
	else {
		sprintf(data, "[FAIL] No sign to know to add or subtract\n");
		return strlen(data)+1;
	}

	adj_value = _atoi(ptr, 10);
	if (adj_value >= 64) {
		sprintf(data, "[FAIL] Adjust value too large\n");
		return strlen(data)+1;
	}

	for (index=0; index<8; index++)
	{
		if ((index == 2) || (index == 3))
			continue;

		readVal = PHY_QueryBBReg(priv, rTxAGC_Rate18_06+index*4, 0x7f7f7f7f);
		byte0 = (readVal & 0xff000000) >> 24;
		byte1 = (readVal & 0x00ff0000) >> 16;
		byte2 = (readVal & 0x0000ff00) >> 8;
		byte3 = (readVal & 0x000000ff);

		if (minus_sign) {
			if (byte0 >= adj_value)
				byte0 -= adj_value;
			else
				byte0 = 0;
			if (byte1 >= adj_value)
				byte1 -= adj_value;
			else
				byte1 = 0;
			if (byte2 >= adj_value)
				byte2 -= adj_value;
			else
				byte2 = 0;
			if (byte3 >= adj_value)
				byte3 -= adj_value;
			else
				byte3 = 0;
		}
		else {
			byte0 += adj_value;
			byte1 += adj_value;
			byte2 += adj_value;
			byte3 += adj_value;
		}

		// Max power index = 0x3F Range = 0-0x3F
		if (byte0 > RF6052_MAX_TX_PWR)
			byte0 = RF6052_MAX_TX_PWR;
		if (byte1 > RF6052_MAX_TX_PWR)
			byte1 = RF6052_MAX_TX_PWR;
		if (byte2 > RF6052_MAX_TX_PWR)
			byte2 = RF6052_MAX_TX_PWR;
		if (byte3 > RF6052_MAX_TX_PWR)
			byte3 = RF6052_MAX_TX_PWR;

		writeVal = (byte0<<24) | (byte1<<16) |(byte2<<8) | byte3;
		PHY_SetBBReg(priv, rTxAGC_Rate18_06+index*4, 0x7f7f7f7f, writeVal);
	}

	byte0 = PHY_QueryBBReg(priv, rTxAGC_CCK_Mcs32, bTxAGCRateCCK);
	if (minus_sign)
		byte0 -= adj_value;
	else
		byte0 += adj_value;
	if (byte0 > RF6052_MAX_TX_PWR)
		byte0 = RF6052_MAX_TX_PWR;
	PHY_SetBBReg(priv, rTxAGC_CCK_Mcs32, bTxAGCRateCCK, byte0);

	sprintf(data, "[SUCCESS] %s %d level RF power\n", minus_sign?"Subtract":"Add", adj_value);
	return strlen(data)+1;
}


int rtl8190_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	struct rtl8190_priv	*priv = (struct rtl8190_priv *)dev->priv;
	struct wifi_mib *pmib = priv->pmib;
	unsigned long flags;
	struct iwreq *wrq = (struct iwreq *) ifr;
	unsigned char *tmpbuf, *tmp1;
	UINT16 sta_num;
	int	i = 0, ret = -1, sizeof_tmpbuf;
#ifdef CONFIG_RTL8671
	// MBSSID Port Mapping
	int ifgrp_member_tmp;
#endif
	static unsigned char tmpbuf1[1024];
#ifdef RTK_WOW
	unsigned int wakeup_on_wlan = 0;
#endif

#ifdef CONFIG_RTK_MESH
	unsigned char strPID[10];
	int len;
	static UINT8 QueueData[MAXDATALEN2];
	int		QueueDataLen;
	// UINT8	val8;

	// int j;
	#define DATAQUEUE_EMPTY "Queue is empty"
#endif

	DEBUG_TRACE;

	sizeof_tmpbuf = sizeof(tmpbuf1);
	tmpbuf = tmpbuf1;
	memset(tmpbuf, '\0', sizeof_tmpbuf);

	SAVE_INT_AND_CLI(flags);

	switch ( cmd )
	{

	case SIOCGIWNAME:
		strcpy(wrq->u.name, "IEEE 802.11-DS");
		ret = 0;
		break;

	case SIOCMIBINIT:	//-- copy kernel data to user data --//
		if (copy_to_user((void *)wrq->u.data.pointer, (void *)pmib, wrq->u.data.length) == 0)
			ret = 0;
		break;

	case SIOCMIBSYNC:	//-- sync user data to kernel data  --//
		if (copy_from_user((void *)pmib, (void *)wrq->u.data.pointer, wrq->u.data.length) == 0)
			ret = 0;
		break;

	case SIOCGIWPRIV:	//-- get private ioctls for iwpriv --//
		if (wrq->u.data.pointer) {
			struct iw_priv_args privtab[] = {
				{ RTL8190_IOCTL_SET_MIB, IW_PRIV_TYPE_CHAR | 450, 0, "set_mib" },
				{ RTL8190_IOCTL_GET_MIB, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_BYTE | 128, "get_mib" },
#ifdef _IOCTL_DEBUG_CMD_
				{ RTL8190_IOCTL_WRITE_REG, IW_PRIV_TYPE_CHAR | 128, 0, "write_reg" },
				{ RTL8190_IOCTL_READ_REG, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_BYTE | 128, "read_reg" },
				{ RTL8190_IOCTL_WRITE_MEM, IW_PRIV_TYPE_CHAR | 128, 0, "write_mem" },
				{ RTL8190_IOCTL_READ_MEM, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_BYTE | 128, "read_mem" },
				{ RTL8190_IOCTL_WRITE_BB_REG, IW_PRIV_TYPE_CHAR | 128, 0, "write_bb" },
				{ RTL8190_IOCTL_READ_BB_REG, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_BYTE | 128, "read_bb" },
				{ RTL8190_IOCTL_WRITE_RF_REG, IW_PRIV_TYPE_CHAR | 128, 0, "write_rf" },
				{ RTL8190_IOCTL_READ_RF_REG, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_BYTE | 128, "read_rf" },
#endif
				{ RTL8190_IOCTL_DEL_STA, IW_PRIV_TYPE_CHAR | 128, 0, "del_sta" },
				{ RTL8190_IOCTL_WRITE_EEPROM, IW_PRIV_TYPE_CHAR | 128, 0, "write_eeprom" },
				{ RTL8190_IOCTL_READ_EEPROM, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_BYTE | 128, "read_eeprom" },

#ifdef SUPPORT_SNMP_MIB
				{ SIOCGSNMPMIB, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_BYTE | 128, "get_snmp_mib" },
#endif

				{ SIOCSRFPWRADJ, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "rf_pwr" },

#ifdef MP_TEST
				{ MP_START_TEST, IW_PRIV_TYPE_NONE, 0, "mp_start" },
				{ MP_STOP_TEST, IW_PRIV_TYPE_NONE, 0, "mp_stop" },
				{ MP_SET_RATE, IW_PRIV_TYPE_CHAR | 40, 0, "mp_rate" },
				{ MP_SET_CHANNEL, IW_PRIV_TYPE_CHAR | 40, 0, "mp_channel" },
				{ MP_SET_TXPOWER, IW_PRIV_TYPE_CHAR | 40, 0, "mp_txpower" },
				{ MP_CONTIOUS_TX, IW_PRIV_TYPE_CHAR | 128, 0, "mp_ctx" },
				{ MP_ARX, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_arx" },
				{ MP_SET_BSSID, IW_PRIV_TYPE_CHAR | 40, 0, "mp_bssid" },
				{ MP_ANTENNA_TX, IW_PRIV_TYPE_CHAR | 40, 0, "mp_ant_tx" },
				{ MP_ANTENNA_RX, IW_PRIV_TYPE_CHAR | 40, 0, "mp_ant_rx" },
				{ MP_SET_BANDWIDTH, IW_PRIV_TYPE_CHAR | 40, 0, "mp_bandwidth" },
				{ MP_SET_PHYPARA, IW_PRIV_TYPE_CHAR | 40, 0, "mp_phypara" },
#ifdef B2B_TEST
				{ MP_TX_PACKET, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | 128, "mp_tx" },
				{ MP_BRX_PACKET, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_brx" },
#if 0
				{ MP_RX_PACKET, IW_PRIV_TYPE_CHAR | 40, 0, "mp_rx" },
#endif
#endif
				{ MP_QUERY_STATS, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_query" },
				{ MP_TXPWR_TRACK, IW_PRIV_TYPE_CHAR | 40, 0, "mp_pwrtrk" },
				{ MP_QUERY_TSSI, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_tssi" },
#if defined(RTL8192SE) || defined(RTL8192SU)
				{ MP_QUERY_THER, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_ther" },
#endif
#endif // MP_TEST
#if (defined(CONFIG_RTL865X) && defined(CONFIG_RTL865X_CLE) && defined(MP_TEST)) || defined(MP_TEST_CFG)
				{ MP_FLASH_CFG, IW_PRIV_TYPE_CHAR | 128, 0, "mp_cfg" },
#endif
#ifdef MICERR_TEST
				{ SIOCSIWRTLMICERROR, IW_PRIV_TYPE_CHAR | 40, 0, "mic_error" },
				{ SIOCSIWRTLMICREPORT, IW_PRIV_TYPE_CHAR | 40, 0, "mic_report" },
#endif
#ifdef DEBUG_8190
				{ SIOCGIWRTLREGDUMP, IW_PRIV_TYPE_CHAR | 40, 0, "reg_dump" },
#endif

#ifdef MBSSID
				{ SIOCSICOPYMIB, IW_PRIV_TYPE_CHAR | 40, 0, "copy_mib" },
#endif

#ifdef CONFIG_RTL8186_KB
				{ SIOCGIREADGUESTMAC, IW_PRIV_TYPE_CHAR | 40, 0, "read_guestmac" },
				{ SIOCSIWRTGUESTMAC, IW_PRIV_TYPE_CHAR | 40, 0, "write_guestmac" },
#endif

#ifdef	CONFIG_RTK_MESH
				{ RTL8190_IOCTL_STATIC_ROUTE, IW_PRIV_TYPE_CHAR | 40, 0, "strt" },
#ifdef D_ACL//tsananiu
                { RTL8190_IOCTL_ADD_ACL_TABLE, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_BYTE | 128, "add_acl_table" },
                { RTL8190_IOCTL_REMOVE_ACL_TABLE, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_BYTE | 128, "remove_acl_table" },
                { RTL8190_IOCTL_GET_ACL_TABLE, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_BYTE | 128, "get_acl_table" },
#endif//tsananiu//
#endif
			};
#ifdef __LINUX_2_6__
			ret = access_ok(VERIFY_WRITE, (const void *)wrq->u.data.pointer, sizeof(privtab));
			if (!ret) {
				ret = -EFAULT;
				DEBUG_ERR("user space valid check error!\n");
				break;
			}
#else
			ret = verify_area(VERIFY_WRITE, (const void *)wrq->u.data.pointer, sizeof(privtab));
			if (ret) {
				DEBUG_ERR("verify_area() error!\n");
				break;
			}
#endif

			wrq->u.data.length = sizeof(privtab) / sizeof(privtab[0]);
			if (copy_to_user((void *)wrq->u.data.pointer, privtab, sizeof(privtab)))
				ret = -EFAULT;
		}
		break;


#ifdef D_ACL	//tsananiu ; mesh related
	case RTL8190_IOCTL_ADD_ACL_TABLE:
		if( (ret = iwpriv_atoi((unsigned char *)(wrq->u.data.pointer),tmpbuf,wrq->u.data.length)) )
		{
			DEBUG_ERR("Trasnslate MAC address from user space error\n");
			break;
		}
		ret = acl_add_cmd(priv, (unsigned char *)(wrq->u.data.pointer), wrq->u.data.length);
                pstat = get_stainfo(priv, tmpbuf);


                DEBUG_INFO("mode %d\n", priv->pmib->dot11StationConfigEntry.dot11AclMode);

		if(priv->pmib->dot11StationConfigEntry.dot11AclMode == ACL_allow)
        		memset(priv->aclCache, 0, sizeof(priv->aclCache));

		if(priv->pmib->dot11StationConfigEntry.dot11AclMode == ACL_deny){
                	if(NULL != pstat){
	                	if(isSTA(pstat)){	//if station
        	              		DEBUG_INFO("I am a STA\n");
                	      		ret = del_sta(priv, tmpbuf+MACADDRLEN);
                	      	}

                      		else{
                      			DEBUG_INFO("I am a mesh node\n");
                      			issue_disassoc_MP(priv, pstat, 0, 0);
				}
                 	}
                 }

                break;

	case RTL8190_IOCTL_REMOVE_ACL_TABLE:
		if( (ret = iwpriv_atoi((unsigned char *)(wrq->u.data.pointer),tmpbuf,wrq->u.data.length)) )
		{
			DEBUG_ERR("Trasnslate MAC address from user space error\n");
			break;
		}
		ret = acl_remove_cmd(priv, (unsigned char *)(wrq->u.data.pointer), wrq->u.data.length);
		pstat = get_stainfo(priv, tmpbuf);


		if(priv->pmib->dot11StationConfigEntry.dot11AclMode == ACL_deny)
        		memset(priv->aclCache, 0, sizeof(priv->aclCache));


		if(priv->pmib->dot11StationConfigEntry.dot11AclMode == ACL_allow){
			DEBUG_INFO("in allow mode\n");
			if(NULL != pstat){
				DEBUG_INFO("pstat != NULL\n");
				if(isSTA(pstat)){	//if station
        	              		DEBUG_INFO("I am a STA\n");
                	      		ret = del_sta(priv, tmpbuf+MACADDRLEN);
                	      	}
				else{
                      			DEBUG_INFO("I am a mesh node\n");
                      			issue_disassoc_MP(priv, pstat, 0, 0);
                      		}
                      		memset(priv->aclCache, 0, sizeof(priv->aclCache));
                      	}
		}

		break;

        case RTL8190_IOCTL_GET_ACL_TABLE:

        	ret = acl_query_cmd(priv, (unsigned char *)(wrq->u.data.pointer));

        	break;
#endif//tsananiu//

	case RTL8190_IOCTL_SET_MIB:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		ret = set_mib(priv, tmpbuf);
		break;

	case RTL8190_IOCTL_GET_MIB:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		i = get_mib(priv, tmpbuf);
		if (i >= 0) {
			if ((i > 0) && copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
			wrq->u.data.length = i;
			ret = 0;
		}
		break;

#ifdef _IOCTL_DEBUG_CMD_
	case RTL8190_IOCTL_WRITE_REG:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		ret = write_reg(priv, tmpbuf);
		break;

	case RTL8190_IOCTL_READ_REG:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		i = read_reg(priv, tmpbuf);
		if (i >= 0) {
			if ((i > 0) && copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
			wrq->u.data.length = i;
			ret = 0;
		}
		break;

	case RTL8190_IOCTL_WRITE_MEM:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		ret = write_mem(priv, tmpbuf);
		break;

	case RTL8190_IOCTL_READ_MEM:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
				break;
		i = read_mem(priv, tmpbuf);
		if (i >= 0) {
			if ((i > 0) && copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
			wrq->u.data.length = i;
			ret = 0;
		}
		break;

	case RTL8190_IOCTL_WRITE_BB_REG:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		ret = write_bb_reg(priv, tmpbuf);
		break;

	case RTL8190_IOCTL_READ_BB_REG:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		i = read_bb_reg(priv, tmpbuf);
		if (i >= 0) {
			if ((i > 0) && copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
			wrq->u.data.length = i;
			ret = 0;
		}
		break;

	case RTL8190_IOCTL_WRITE_RF_REG:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		ret = write_rf_reg(priv, tmpbuf);
		break;

	case RTL8190_IOCTL_READ_RF_REG:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		i = read_rf_reg(priv, tmpbuf);
		if (i >= 0) {
			if ((i > 0) && copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
			wrq->u.data.length = i;
			ret = 0;
		}
		break;
#endif // _IOCTL_DEBUG_CMD_


	case RTL8190_IOCTL_DEL_STA:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		ret = del_sta(priv, tmpbuf);
		break;


	case RTL8190_IOCTL_WRITE_EEPROM:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		ret = write_eeprom(priv, tmpbuf);
		break;

	case RTL8190_IOCTL_READ_EEPROM:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		i = read_eeprom(priv, tmpbuf);
		if (i >= 0) {
			if ((i > 0) && copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
			wrq->u.data.length = i;
			ret = 0;
		}
		break;

#ifdef RTK_WOW
	case SIOCGRTKWOWSTAINFO:	//-- get station info for Realtek proprietary wake up on wlan mode--//
		wakeup_on_wlan = 1;
#endif
	case SIOCGIWRTLSTAINFO:	//-- get station table information --//
		sizeof_tmpbuf = sizeof(sta_info_2_web) * (NUM_STAT + 1); // for the max of all sta info
		tmp1 = (unsigned char *)kmalloc(sizeof_tmpbuf, GFP_KERNEL);
		if (!tmp1) {
			printk("Unable to allocate temp buffer for ioctl (SIOCGIWRTLSTAINFO)!\n");
			return -1;
		}
		memset(tmp1, '\0', sizeof(sta_info_2_web));

#if !defined(CONFIG_RTL865X) || !defined(RTL865X_GETSTAINFO_PATCH)
		if (copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, 1))
			break;
		if ((tmpbuf[0] == 0) || (tmpbuf[0] > NUM_STAT))
			sta_num = NUM_STAT;
		else
			sta_num = tmpbuf[0];
#ifdef RTK_WOW
		get_sta_info(priv, (sta_info_2_web *)(tmp1 + sizeof(sta_info_2_web)), sta_num, wakeup_on_wlan);
#else
		get_sta_info(priv, (sta_info_2_web *)(tmp1 + sizeof(sta_info_2_web)), sta_num);
#endif
		if (copy_to_user((void *)wrq->u.data.pointer, tmp1, sizeof(sta_info_2_web)*(sta_num+1)))
			break;
		wrq->u.data.length = sizeof(sta_info_2_web)*(sta_num+1);
		ret = 0;
#else
		{
			unsigned short num=0, returned=0;
			num = wrq->u.data.length/sizeof(sta_info_2_web);
			if ((num == 0) || (num > NUM_STAT))
				sta_num = NUM_STAT;
			else
				sta_num = num;
#ifdef RTK_WOW
			returned = get_sta_info(priv, (sta_info_2_web *)tmp1, sta_num, wakeup_on_wlan);
#else
			returned = get_sta_info(priv, (sta_info_2_web *)tmp1, sta_num);
#endif
			printk("get_sta_info: %d\n", returned);
			if (returned > 0) {
				if (copy_to_user(wrq->u.data.pointer, tmp1, sizeof(sta_info_2_web)*(returned)))
					break;
				ret = sizeof(sta_info_2_web)*returned;
			}
			else
				ret = 0;
			wrq->u.data.length = sizeof(sta_info_2_web)*(sta_num+1);
		}
#endif /* !defined(CONFIG_RTL865X) || !defined(RTL865X_GETSTAINFO_PATCH) */

		kfree(tmp1);
		break;

	case SIOCGIWRTLSTANUM:	//-- get the number of stations in table --//
#ifdef UNIVERSAL_REPEATER
		if (IS_VXD_INTERFACE(priv) && (OPMODE & WIFI_AP_STATE) &&
				!IS_DRV_OPEN(priv))
			sta_num = 0;
		else
#endif
		{
//			sta_num = priv->assoc_num;
			struct list_head *phead, *plist;
			struct stat_info *pstat;
			sta_num = 0;
			phead = &priv->asoc_list;
			plist = phead->next;
			while (plist != phead) {
				pstat = list_entry(plist, struct stat_info, asoc_list);
				if (pstat->state & WIFI_ASOC_STATE)
					sta_num++;
				plist = plist->next;
			}
		}

		if (copy_to_user((void *)wrq->u.data.pointer, &sta_num, sizeof(sta_num)))
			break;
		wrq->u.data.length = sizeof(sta_num);
		ret = 0;
		break;

	case SIOCGIWRTLDRVVERSION:
		tmpbuf[0] = DRV_VERSION_H;
		tmpbuf[1] = DRV_VERSION_L;
		if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, 2))
			break;
		wrq->u.data.length = 2;
		ret = 0;
		break;

	case SIOCGIWRTLGETBSSINFO: //-- get BSS info --//
		get_bss_info(priv, (bss_info_2_web *)tmpbuf);
		if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, sizeof(bss_info_2_web)))
			break;
		wrq->u.data.length = sizeof(bss_info_2_web);
		ret = 0;
		break;

#if defined(CONFIG_RTL8186_KB_N)//To get auth result
	case SIOCGIWRTLAUTH:
		if (copy_to_user((void *)wrq->u.data.pointer, &authRes, sizeof(authRes)))
			break;
		wrq->u.data.length = sizeof(authRes);
		ret = 0;
		authRes = 0;//To init authRes
		break;
#endif

#ifdef WDS
	case SIOCGIWRTLGETWDSINFO: //-- get WDS table information --//
		ret = get_wds_info(priv, (web_wds_info *)tmpbuf);
		if ((ret > 0) && copy_to_user((void *)wrq->u.data.pointer, tmpbuf, ret))
			break;
		wrq->u.data.length = ret;
		break;
#endif

	case SIOCSIWRTLSTATXRATE:	//-- set station tx rate --//
		if (wrq->u.data.length != sizeof(struct _wlan_sta_rateset) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, sizeof(struct _wlan_sta_rateset)))
			break;
		ret = set_sta_txrate(priv, (struct _wlan_sta_rateset *)tmpbuf);
		break;


#ifdef MICERR_TEST
	case SIOCSIWRTLMICERROR:
		ret = issue_mic_err_pkt(priv);
		break;

	case SIOCSIWRTLMICREPORT:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, wrq->u.data.pointer, wrq->u.data.length))
			break;
		ret = issue_mic_rpt_pkt(priv, tmpbuf);
		break;
#endif


	case SIOCSACLADD:
		ret = acl_add_cmd(priv, (unsigned char *)(wrq->u.data.pointer), wrq->u.data.length);
		break;

	case SIOCSACLDEL:
		ret = acl_remove_cmd(priv, (unsigned char *)(wrq->u.data.pointer), wrq->u.data.length);
		break;

	case SIOCSACLQUERY:
		ret = acl_query_cmd(priv, (unsigned char *)(wrq->u.data.pointer));
		break;

#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_)
	case SIOCSMESHACLADD:
		ret = mesh_acl_add_cmd(priv, (unsigned char *)(wrq->u.data.pointer), wrq->u.data.length);
		break;

	case SIOCSMESHACLDEL:
		ret = mesh_acl_remove_cmd(priv, (unsigned char *)(wrq->u.data.pointer), wrq->u.data.length);
		break;

	case SIOCSMESHACLQUERY:
		ret = mesh_acl_query_cmd(priv, (unsigned char *)(wrq->u.data.pointer));
		break;
#endif

	case SIOCGMIBDATA:
		CamDumpAll(priv);
		ret = 0;
		break;

	case SIOCGMISCDATA:
		get_misc_data(priv, (struct _misc_data_ *)tmpbuf);
		if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, sizeof(struct _misc_data_)))
			break;
		wrq->u.data.length = sizeof(struct _misc_data_);
		ret = 0;
		break;


	case RTL8190_IOCTL_USER_DAEMON_REQUEST:
		ret = rtl8190_ioctl_priv_daemonreq(dev, &wrq->u.data);
		break;


#ifdef USE_PID_NOTIFY
	case SIOCSIWRTLSETPID:
		priv->pshare->wlanapp_pid = -1;
		if (wrq->u.data.length != sizeof(pid_t) ||
			copy_from_user(&priv->pshare->wlanapp_pid, (void *)wrq->u.data.pointer, sizeof(pid_t)))
			break;
		ret = 0;
		break;
#endif

#ifdef CONFIG_RTL_WAPI_SUPPORT		
	case SIOCSIWRTLSETWAPIPID:
		priv->pshare->wlanwapi_pid= -1;
		if (wrq->u.data.length != sizeof(pid_t) ||
			copy_from_user(&priv->pshare->wlanwapi_pid, (void *)wrq->u.data.pointer, sizeof(pid_t)))
			break;
		ret = 0;
		break;
#endif


#ifdef	CONFIG_RTK_MESH
	case RTL8190_IOCTL_STATIC_ROUTE:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		if( memcmp(tmpbuf, "del", 3)==0 )
		{
			mac12_to_6(tmpbuf+4, tmpbuf+0x100);
			ret = remove_path_entry(priv, tmpbuf+0x100);
		}
		else if((memcmp(tmpbuf, "add", 3)==0) && (wrq->u.data.length>28) )
		{
			struct path_sel_entry Entry;
			memset((void*)&Entry, 0, sizeof(struct path_sel_entry));
			mac12_to_6(tmpbuf+4, Entry.destMAC);
			mac12_to_6(tmpbuf+4+13, Entry.nexthopMAC);
			Entry.flag=1;
			ret = pathsel_table_entry_insert_tail( priv, &Entry);
		}else
			ret =0;
		break;
// ==== inserted by GANTOE for manual site survey 2008/12/25 ====
	case SIOCJOINMESH:
		{
			struct
			{
				unsigned char *meshid;
				int meshid_len, channel, reset;
			}mesh_identifier;
			if(wrq->u.data.length > 0)
			{
				memcpy(&mesh_identifier, wrq->u.data.pointer, wrq->u.data.length);
				ret = rtl8190_join_mesh(priv, mesh_identifier.meshid, mesh_identifier.meshid_len, mesh_identifier.channel, mesh_identifier.reset);
			}
			else
				ret = -1;
		}
		break;
	case SIOCCHECKMESHLINK:	// This case might be removed when the mesh peerlink precedure has been completed
		{
			if(wrq->u.data.length == 6)
				ret = rtl8190_check_mesh_link(priv, wrq->u.data.pointer);
			else
				ret = -1;
		}
		break;
// ==== GANTOE ====
#endif
	case SIOCGIWRTLSCANREQ:		//-- Issue SS request --//
#ifdef UNIVERSAL_REPEATER
		if (IS_VXD_INTERFACE(priv)) {
			DEBUG_ERR("can't do site-survey for vxd!\n");
			break;
		}
#endif
#ifdef MBSSID
		if (
#if defined(RTL8192SE) || defined(RTL8192SU)
			GET_ROOT(priv)->pmib->miscEntry.vap_enable && 
#endif
			IS_VAP_INTERFACE(priv)) {
			DEBUG_ERR("can't do site-survey for vap!\n");
			break;
		}
#endif
		ret = rtl8190_ss_req(priv, (unsigned char *)(wrq->u.data.pointer), wrq->u.data.length);
		break;

	case SIOCGIWRTLGETBSSDB:	//-- Get SS Status --//
#ifdef UNIVERSAL_REPEATER
		if (IS_VXD_INTERFACE(priv)) {
			DEBUG_ERR("can't get site-survey status for vxd!\n");
			break;
		}
#endif
#ifdef MBSSID
		if (
#if defined(RTL8192SE) || defined(RTL8192SU)
			GET_ROOT(priv)->pmib->miscEntry.vap_enable && 
#endif
			IS_VAP_INTERFACE(priv)) {
			DEBUG_ERR("can't get site-survey status for vap!\n");
			break;
		}
#endif
		ret	= rtl8190_get_ss_status(priv, (unsigned char *)(wrq->u.data.pointer));
		break;

#ifdef CLIENT_MODE
	case SIOCGIWRTLJOINREQ:		//-- Issue Join Request --//
		ret = rtl8190_join(priv, (unsigned char *)(wrq->u.data.pointer));
		break;

	case SIOCGIWRTLJOINREQSTATUS:	//-- Get Join Status --//
		ret = rtl8190_join_status(priv, (unsigned char *)(wrq->u.data.pointer));
		break;
#endif


#ifdef RTK_WOW
	case SIOCGRTKWOW:	//-- issue Realtek proprietary wake up on wlan mode --//
		ret = 5;
		do {
			issue_rtk_wow(priv,  (unsigned char *)(wrq->u.data.pointer));
		} while(--ret > 0);
		break;
#endif

	case SIOCSRFPWRADJ:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		i = dynamic_RF_pwr_adj(priv, tmpbuf);
		if (i > 0) {
			if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
		}
		wrq->u.data.length = i;
		ret = 0;
		break;

#ifdef MP_TEST
	case MP_START_TEST:
		mp_start_test(priv);
		ret = 0;
		break;

	case MP_STOP_TEST:
		mp_stop_test(priv);
		ret = 0;
		break;

	case MP_SET_RATE:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		mp_set_datarate(priv, tmpbuf);
		ret = 0;
		break;

	case MP_SET_CHANNEL:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		mp_set_channel(priv, tmpbuf);
		ret = 0;
		break;

	case MP_SET_BANDWIDTH:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		mp_set_bandwidth(priv, tmpbuf);
		ret = 0;
		break;

	case MP_SET_TXPOWER:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		mp_set_tx_power(priv, tmpbuf);
		ret = 0;
		break;

	case MP_CONTIOUS_TX:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		RESTORE_INT(flags);
		mp_ctx(priv, tmpbuf);
		SAVE_INT_AND_CLI(flags);
		ret = 0;
		break;

	case MP_ARX:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		i = mp_arx(priv, tmpbuf);
		if (i > 0) {
			if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
		}
		wrq->u.data.length = i;
		ret = 0;
		break;

#if defined(CONFIG_RTL865X) && defined(CONFIG_RTL865X_CLE) && defined(MP_TEST)
	case MP_FLASH_CFG:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		RESTORE_INT(flags);
		mp_flash_cfg(priv, tmpbuf);
		SAVE_INT_AND_CLI(flags);
		ret = 0;
		break;
#endif

	case MP_SET_BSSID:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		mp_set_bssid(priv, tmpbuf);
		ret = 0;
		break;

	case MP_ANTENNA_TX:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		mp_set_ant_tx(priv, tmpbuf);
		ret = 0;
		break;

	case MP_ANTENNA_RX:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		mp_set_ant_rx(priv, tmpbuf);
		ret = 0;
		break;

	case MP_SET_PHYPARA:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		mp_set_phypara(priv, tmpbuf);
		ret = 0;
		break;

#ifdef B2B_TEST
	case MP_TX_PACKET:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		RESTORE_INT(flags);
		i = mp_tx(priv, tmpbuf);
		SAVE_INT_AND_CLI(flags);
		if (i > 0) {
			if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
		}
		wrq->u.data.length = i;
		ret = 0;
		break;

#if 0
	case MP_RX_PACKET:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
#ifndef __LINUX_2_6__
		RESTORE_INT(flags);
#endif
		mp_rx(priv, tmpbuf);
#ifndef __LINUX_2_6__
		SAVE_INT_AND_CLI(flags);
#endif
		ret = 0;
		break;
#endif

	case MP_BRX_PACKET:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		RESTORE_INT(flags);
		i = mp_brx(priv, tmpbuf);
		SAVE_INT_AND_CLI(flags);
		if (i > 0) {
			if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
		}
		wrq->u.data.length = i;
		ret = 0;
		break;
#endif // B2B_TEST

	case MP_QUERY_STATS:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		i = mp_query_stats(priv, tmpbuf);
		if (i > 0) {
			if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
		}
		wrq->u.data.length = i;
		ret = 0;
		break;

	case MP_TXPWR_TRACK:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		RESTORE_INT(flags);
		mp_txpower_tracking(priv, tmpbuf);
		SAVE_INT_AND_CLI(flags);
		ret = 0;
		break;

	case MP_QUERY_TSSI:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		RESTORE_INT(flags);
		i = mp_query_tssi(priv, tmpbuf);
		SAVE_INT_AND_CLI(flags);
		if (i > 0) {
			if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
			wrq->u.data.length = i;
			ret = 0;
		}
		break;

#if defined(RTL8192SE) || defined(RTL8192SU)
	case MP_QUERY_THER:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		RESTORE_INT(flags);
		i = mp_query_ther(priv, tmpbuf);
		SAVE_INT_AND_CLI(flags);
		if (i > 0) {
			if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
			wrq->u.data.length = i;
			ret = 0;
		}
		break;
#endif
#endif	// MP_TEST

#ifdef DEBUG_8190
	case SIOCGIWRTLREGDUMP:
		reg_dump(priv);
		ret = 0;
		break;
#endif

#ifdef MBSSID
	case SIOCSICOPYMIB:
		memcpy(priv->pmib, GET_ROOT_PRIV(priv)->pmib, sizeof(struct wifi_mib));
		ret = 0;
		break;
#endif

#ifdef CONFIG_RTL8186_KB
	case SIOCGIREADGUESTMAC:
		i = get_guestmac(priv, (GUESTMAC_T *)tmpbuf);
		if (i >= 0) {
			if ((i > 0) && copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
			wrq->u.data.length = i;
			ret = 0;
		}
		break;

	case SIOCSIWRTGUESTMAC:
		if (copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		set_guestmacvalid(priv, tmpbuf);
		ret = 0;
		break;
#endif

#ifdef CONFIG_RTL8671
	// MBSSID Port Mapping
	case SIOSIWRTLITFGROUP:
	{
#if 1
		printk("TBS:not support.\n");
#else
		if (copy_from_user((char *)&ifgrp_member_tmp, wrq->u.data.pointer, 4))
			break;

		for (i=0; i<5; i++) {
			if ( wrq->u.data.flags == i ) {
				wlanDev[i].dev_ifgrp_member = bitmap_virt2phy(ifgrp_member_tmp);
				g_port_mapping = TRUE;
				break;
			}
		}
		break;
#endif
	}
#endif

#ifdef SUPPORT_SNMP_MIB
	case SIOCGSNMPMIB:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;

		if (mib_get(priv, tmpbuf, tmpbuf, &i)) {
			if ((i > 0) && copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
			wrq->u.data.length = i;
			ret = 0;
		}
		break;
#endif

#ifdef	SUPPORT_TX_MCAST2UNI
	case SIOCGIMCAST_ADD:
	case SIOCGIMCAST_DEL:

		if (!priv->pshare->rf_ft_var.mc2u_disable) {
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
			struct net_device *dev_vap;
			struct rtl8190_priv	*priv_vap;

			if (IS_ROOT_INTERFACE(priv))
#endif
			{
#ifdef MBSSID
#if defined(RTL8192SE) || defined(RTL8192SU)
				if (priv->pmib->miscEntry.vap_enable)
#endif
				{
					for (i=0; i<RTL8190_NUM_VWLAN; i++) {
						if (IS_DRV_OPEN(priv->pvap_priv[i])) {
//							rtl8190_ioctl(priv->pvap_priv[i]->dev, ifr, cmd);
							dev_vap = priv->pvap_priv[i]->dev;
							priv_vap = priv->pvap_priv[i];
							if (netif_running(dev_vap)) {
								if ((((GET_MIB(priv_vap))->dot11OperationEntry.opmode) & WIFI_AP_STATE)
#ifdef CLIENT_MODE
									|| ((((GET_MIB(priv_vap))->dot11OperationEntry.opmode) & (WIFI_STATION_STATE|WIFI_ASOC_STATE))==(WIFI_STATION_STATE|WIFI_ASOC_STATE))
#endif
									)
									AddDelMCASTGroup2STA(priv_vap,(unsigned char *)ifr, (cmd == SIOCGIMCAST_ADD) ? 1 : 0);
							}
						}
					}
				}
#endif
#ifdef UNIVERSAL_REPEATER
				if (IS_DRV_OPEN(GET_VXD_PRIV(priv))) {
					dev_vap = (GET_VXD_PRIV(priv))->dev;
					priv_vap = (struct rtl8190_priv *)dev_vap->priv;
//					rtl8190_ioctl((GET_VXD_PRIV(priv))->dev, ifr, cmd);
					if (netif_running(dev_vap)) {
						if ((((GET_MIB(priv_vap))->dot11OperationEntry.opmode) & WIFI_AP_STATE)
#ifdef CLIENT_MODE
							|| ((((GET_MIB(priv_vap))->dot11OperationEntry.opmode) & (WIFI_STATION_STATE|WIFI_ASOC_STATE))==(WIFI_STATION_STATE|WIFI_ASOC_STATE))
#endif
							)
							AddDelMCASTGroup2STA(priv_vap,(unsigned char *)ifr, (cmd == SIOCGIMCAST_ADD) ? 1 : 0);
					}
				}
#endif
			}
			if (netif_running(priv->dev)) {
				DEBUG_INFO("%s: %s MCAST Group mac %02x%02x%02x%02x%02x%02x\n",  priv->dev->name,
					((cmd == SIOCGIMCAST_ADD) ? "Add" : "Del"),
					((unsigned char *)ifr)[0],((unsigned char *)ifr)[1],
					((unsigned char *)ifr)[2],((unsigned char *)ifr)[3],((unsigned char *)ifr)[4],((unsigned char *)ifr)[5]);
				DEBUG_INFO("STA mac %02x%02x%02x%02x%02x%02x\n", ((unsigned char *)ifr)[6],((unsigned char *)ifr)[7],
					((unsigned char *)ifr)[8],((unsigned char *)ifr)[9],((unsigned char *)ifr)[10],((unsigned char *)ifr)[11]);

				if ((OPMODE & WIFI_AP_STATE)
#ifdef CLIENT_MODE
						|| ((OPMODE & (WIFI_STATION_STATE|WIFI_ASOC_STATE))==(WIFI_STATION_STATE|WIFI_ASOC_STATE))
#endif
					)
					AddDelMCASTGroup2STA(priv,(unsigned char *)ifr, (cmd == SIOCGIMCAST_ADD) ? 1 : 0);
			}
		}
		ret = 0;
		break;
#endif	// SUPPORT_TX_MCAST2UNI
#ifdef	CONFIG_RTK_MESH
	case SIOCQPATHTABLE:
   	{
		unsigned char destaddr[MACADDRLEN] = {0};

		copy_from_user(destaddr, (void *)(wrq->u.data.pointer), MACADDRLEN);
		//MESH_DEBUG_MSG("kernel destaddr = %s\n",destaddr);
		struct path_sel_entry *pEntry = pathsel_query_table( priv, destaddr); // modified by chuangch 2007.09.14
		ret = -1;
		if(pEntry!= (struct path_sel_entry *)-1)
		{
			if (copy_to_user((void *)wrq->u.data.pointer, (void *)pEntry, (int)&((struct path_sel_entry*)0)->start) == 0)
			{
				ret = 0;
				wrq->u.data.length = sizeof(struct path_sel_entry);
			}
		}
		break;
	}

	case SIOCUPATHTABLE:
	{
		struct path_sel_entry Entry;
		copy_from_user((struct path_sel_entry *)&Entry, (void *)(wrq->u.data.pointer), (int)&((struct path_sel_entry*)0)->start);
		ret = pathsel_modify_table_entry(priv, &Entry); // chuangch 2007.09.14
		break;
	}
	case SIOCAPATHTABLE:
	{
		struct path_sel_entry Entry, *pEntry;
		memset((void*)&Entry, 0, sizeof(Entry));
		copy_from_user((struct path_sel_entry *)&Entry, (void *)(wrq->u.data.pointer), (int)&((struct path_sel_entry*)0)->start);
		ret = 0;
		pEntry = pathsel_query_table( priv, Entry.destMAC );

		if( pEntry == (struct path_sel_entry *)-1 || pEntry->flag==0)
		{
			Entry.update_time = xtime; // chuangch 2007.09.19
			Entry.routeMaintain = xtime; // chuangch 10.19
			ret = pathsel_table_entry_insert_tail( priv, &Entry); //chuangch 2007.09.14

			MESH_DEBUG_MSG("create path to:%02X:%02X:%02X:%02X:%02X:%02X, Nexthop=%02X:%02X:%02X:%02X:%02X:%02X, Hop count=%d\n",
				Entry.destMAC[0], Entry.destMAC[1], Entry.destMAC[2], Entry.destMAC[3], Entry.destMAC[4], Entry.destMAC[5],
				Entry.nexthopMAC[0],  Entry.nexthopMAC[1], Entry.nexthopMAC[2], Entry.nexthopMAC[3], Entry.nexthopMAC[4], Entry.nexthopMAC[5],
				Entry.hopcount);
		}

		break;
	}

	//modify by Jason 2007.11.26
	case REMOVE_PATH_ENTRY:
	{
		unsigned char invalid_node_addr[MACADDRLEN] = {0};
		unsigned long flags;
		struct path_sel_entry *pEntry;

		if ( copy_from_user((void *)invalid_node_addr, (void *)(wrq->u.data.pointer), MACADDRLEN) ) {
			ret = -1;
			break;
		}

		MESH_DEBUG_MSG("REMOVE_PATH_ENTRY\n");
		MESH_DEBUG_MSG("invalid_node_addr =%2X-%2X-%2X-%2X-%2X-%2X-\n",invalid_node_addr[0],invalid_node_addr[1],invalid_node_addr[2],invalid_node_addr[3],invalid_node_addr[4],invalid_node_addr[5]);	

		pEntry = pathsel_query_table( priv, invalid_node_addr );
		if(pEntry != (struct path_sel_entry *)-1 && pEntry->flag==0)  
		{	
#ifdef __LINUX_2_6__
/*by qjj_qin and hf_shi*/
#else
			SAVE_INT_AND_CLI(flags);
#endif
			ret = remove_path_entry(priv,invalid_node_addr);
#ifdef __LINUX_2_6__
/*by qjj_qin and hf_shi*/
#else
			RESTORE_INT(flags);
#endif
		}
		break;
	}

#ifdef FREDDY	//mesh related
	//modified for kick STA function by freddy 2008/12/2
	case RTL8190_IOCTL_DEL_STA_ENC:
		{
			int iReasonCode = _STATS_FAILURE_;
			char MacBuf[256];
			char *pStr = NULL;
			int iArg = 0;
			if ((wrq->u.data.length > sizeof_tmpbuf) ||
				copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
				break;

			pStr = strtok(tmpbuf,",");
			while(pStr != 0)
			{
			   switch(iArg)
			   {
			   		case 0://MAC address

						strncpy(MacBuf,pStr,256);

			   		break;

					case 1://Reason code

						sscanf(pStr,"%d",&iReasonCode);

			   		break;
			   }
			   iArg++;
			   pStr = strtok(NULL,",");
			}

			ret = del_sta_enc(priv, MacBuf,iReasonCode);
		}
		break;
#endif
	case GET_STA_LIST:
	{
		struct stat_info	*pstat;

		struct proxy_table_entry *pEntry=NULL;
		static unsigned char node[MACADDRLEN] = {0};

		if ( copy_from_user((void *)node, (void *)(wrq->u.data.pointer), MACADDRLEN) ) {
			ret = -1;
			break;
		}

		// my station
		pstat = get_stainfo(priv, node);

		if(pstat != 0)
		{
			if(isSTA(pstat) && (pstat->state & WIFI_ASOC_STATE)) {
				ret = 0;
				break;
			}
			else // a neighbor
				goto ret_nothing;
		}

		pEntry = HASH_SEARCH(priv->proxy_table,node);

		// my proxied entry
		if(pEntry && (memcmp(GET_MY_HWADDR, pEntry->owner, MACADDRLEN)==0))
		{
			ret = 0;
			break;
		}

ret_nothing:
		// if not my station or not my proxied entry, then just fill garbage(0x0b) and return normally
		memset(node, 0x0b, sizeof(node));
	       	if (copy_to_user((void *)wrq->u.data.pointer, (void *)node, MACADDRLEN) != 0)
		{
			ret = -1;
			break;
		}
		ret = 0;
		break;
	}

	case SET_PORTAL_POOL:
	{
		if ( copy_from_user( (priv->pann_mpp_tb->pann_mpp_pool), (void *)(wrq->u.data.pointer), sizeof(struct pann_mpp_tb_entry) * MAX_MPP_NUM ) ) {
			ret = -1;
			break;
		}
		ret = 0;
  		break;
	}

	case SIOC_NOTIFY_PATH_CREATE:
	{
		unsigned char destaddr[MACADDRLEN] = {0};

		if ( copy_from_user((void *)destaddr, (void *)(wrq->u.data.pointer), MACADDRLEN) ) {
			ret = -1;
			break;
		}
		notify_path_found(destaddr,priv);
		// MESH_DEBUG_MSG("destaddr =%2X-%2X-%2X-%2X-%2X-%2X-\n",destaddr[0],destaddr[1],destaddr[2],destaddr[3],destaddr[4],destaddr[5]);
		ret = 0;
  		break;
	}

	case SIOC_UPDATE_ROOT_INFO:
	{
		if ( copy_from_user((void *)priv->root_mac, (void *)(wrq->u.data.pointer), MACADDRLEN) ) {
			ret = -1;
			break;
		}
		ret = 0;
#if 0
		LOG_MESH_MSG("Root MAC = %02X:%02X:%02X:%02X:%02X:%02X\n",
			priv->root_mac[0],priv->root_mac[1],priv->root_mac[2],priv->root_mac[3],priv->root_mac[4],priv->root_mac[5]);
#endif
  		break;
	}

	case SIOC_GET_ROUTING_INFO:
	{
		unsigned char buffer[128] = {0};
		buffer[0] = (unsigned short)(priv->pmib->dot1180211sInfo.mesh_root_enable);
		buffer[1] = (unsigned short)(priv->pmib->dot1180211sInfo.mesh_portal_enable);
#ifdef	_11s_TEST_MODE_
		buffer[2] = (unsigned short)(priv->pmib->dot1180211sInfo.mesh_reserved1);
#endif
		if (copy_to_user((void *)wrq->u.data.pointer, (void *)buffer, 128) == 0)
			ret = 0;
		else
			ret = -1;

  		break;
	}

case SIOC_SET_ROUTING_INFO:
	{
		unsigned char buffer[128] = {0};

		if ( !wrq->u.data.pointer ){
				ret = -1;
				break;
		}

		if (copy_from_user((void *)buffer, (void *)wrq->u.data.pointer, 128) == 0)
			ret = 0;
		else
			ret = -1;

		(unsigned short)(priv->pmib->dot1180211sInfo.mesh_root_enable) = buffer[0];
		(unsigned short)(priv->pmib->dot1180211sInfo.mesh_portal_enable) = buffer[1];

  		break;
	}


#ifdef  _11s_TEST_MODE_
	case SAVE_RECEIBVER_PID:
	{
		if ( !wrq->u.data.pointer ){
				ret = -1;
				break;
		}

		len = wrq->u.data.length;
		memset(strPID, 0, sizeof(strPID));
		copy_from_user(strPID, (void *)wrq->u.data.pointer, len);

		pid_receiver = 0;
		for(i = 0; i < len; i++) //char -> int
		{
			pid_receiver = pid_receiver * 10 + (strPID[i] - 48);
		}
		ret = 0;
		break;
	}

	case DEQUEUE_RECEIBVER_IOCTL:
	{
		if((ret = DOT11_DeQueue2((unsigned long)priv, priv->receiver_queue, QueueData, &QueueDataLen)) != 0) {
			copy_to_user((void *)(wrq->u.data.pointer), DATAQUEUE_EMPTY, sizeof(DATAQUEUE_EMPTY));
			wrq->u.data.length = sizeof(DATAQUEUE_EMPTY);
		} else {
			copy_to_user((void *)wrq->u.data.pointer, (void *)QueueData, QueueDataLen);
			wrq->u.data.length = QueueDataLen;
		}
		break;
	}
#endif

	case SAVEPID_IOCTL:
	{
		if ( !wrq->u.data.pointer ){
			ret = -1;
			break;
		}

		len = wrq->u.data.length;
		memset(strPID, 0, sizeof(strPID));
		copy_from_user(strPID, (void *)wrq->u.data.pointer, len);

		pid_pathsel = 0;
		for(i = 0; i < len; i++) //char -> int
		{
			pid_pathsel = pid_pathsel * 10 + (strPID[i] - 48);
		}

		ret = 0;
		break;
	}

	case DEQUEUEDATA_IOCTL:
	{
		if((ret = DOT11_DeQueue2((unsigned long)priv, priv->pathsel_queue, QueueData, &QueueDataLen)) != 0) {
			copy_to_user((void *)(wrq->u.data.pointer), DATAQUEUE_EMPTY, sizeof(DATAQUEUE_EMPTY));
			wrq->u.data.length = sizeof(DATAQUEUE_EMPTY);
		} else {
			copy_to_user((void *)wrq->u.data.pointer, (void *)QueueData, QueueDataLen);
			wrq->u.data.length = QueueDataLen;
		}

		break;
	}
#endif // CONFIG_RTK_MESH
	}

	RESTORE_INT(flags);
	return ret;
}

#ifdef CONFIG_ENABLE_MIPS16
__attribute__((nomips16))
#endif
void delay_us(unsigned int t)
{
	__udelay(t, __udelay_val);
}

#ifdef CONFIG_ENABLE_MIPS16
__attribute__((nomips16))
#endif
void delay_ms(unsigned int t)
{
	mdelay(t);
}

