/*
 *      Header file defines some private structures and macro
 *
 *      $Id: 8190n.h,v 1.11 2009/09/28 13:22:10 cathy Exp $
 */

#ifndef	_8190N_H_
#define _8190N_H_

#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>

#ifdef __DRAYTEK_OS__
#include <draytek/softimer.h>
#include <draytek/skbuff.h>
#include <draytek/wl_dev.h>
#endif

#include "./8190n_cfg.h"


#ifdef SUPPORT_SNMP_MIB
#include "./8190n_mib.h"
#endif

#define TRUE		1
#define FALSE		0

#define CONGESTED	2
#ifdef SUCCESS
#undef SUCCESS
#endif
#define SUCCESS		1
#define FAIL		0
#define LOADFW_FAIL (-2)

#if defined(USE_RTL8186_SDK) || !defined(PKT_PROCESSOR)
typedef unsigned char	UINT8;
typedef unsigned short	UINT16;
typedef unsigned long	UINT32;

typedef signed char		INT8;
typedef signed short	INT16;
typedef signed long		INT32;

typedef unsigned long long	UINT64;
typedef signed long long	INT64;
#endif
typedef unsigned int	UINT;
typedef signed int		INT;

#include "./ieee802_mib.h"
#include "./wifi.h"
#include "./8190n_security.h"

#ifdef RTK_BR_EXT
#include "./8190n_br_ext.h"
#endif

#include "./8190n_hw.h"

#ifdef INCLUDE_WPA_PSK
#include "./8190n_psk.h"
#endif

#ifdef WIFI_SIMPLE_CONFIG
#define MAX_WSC_IE_LEN		256
#define MAX_WSC_PROBE_STA	10
#endif

#if defined(CONFIG_RTL_WAPI_SUPPORT)
#include "wapi_wai.h"
#endif

#ifdef CONFIG_RTK_MESH
#include "./mesh_ext/mesh.h"
#include "./mesh_ext/hash_table.h"
#include "./mesh_ext/mesh_route.h"
#include "./mesh_ext/mesh_security.h"
#endif
#define DWNGRADE_PROBATION_TIME		3
#define UPGRADE_PROBATION_TIME		3
#define TRY_RATE_FREQ				6

#ifdef CONFIG_RTL8190_PRIV_SKB
	#define MAX_PRE_ALLOC_SKB_NUM	160
#else
	#define MAX_PRE_ALLOC_SKB_NUM	32
#endif

// for packet aggregation
#define FG_AGGRE_MPDU				1
#define FG_AGGRE_MPDU_BUFFER_FIRST	2
#define FG_AGGRE_MPDU_BUFFER_MID	3
#define FG_AGGRE_MPDU_BUFFER_LAST	4
#define FG_AGGRE_MSDU_FIRST			5
#define FG_AGGRE_MSDU_MIDDLE		6
#define FG_AGGRE_MSDU_LAST			7

	#define MANAGEMENT_AID			0	//for 8192 AID reducing issue

enum wifi_state {
	WIFI_NULL_STATE		=	0x00000000,
	WIFI_ASOC_STATE		=	0x00000001,
	WIFI_REASOC_STATE	=	0x00000002,
	WIFI_SLEEP_STATE	=	0x00000004,
	WIFI_STATION_STATE	=	0x00000008,
	WIFI_AP_STATE		=	0x00000010,
	WIFI_ADHOC_STATE	=	0x00000020,
	WIFI_AUTH_NULL		=	0x00000100,
	WIFI_AUTH_STATE1	= 	0x00000200,
	WIFI_AUTH_SUCCESS	=	0x00000400,
	WIFI_SITE_MONITOR	=	0x00000800,		//to indicate the station is under site surveying
#ifdef WDS
	WIFI_WDS			=	0x00001000,
	WIFI_WDS_RX_BEACON	=	0x00002000,		// already rx WDS AP beacon
#endif

#ifdef MP_TEST
	WIFI_MP_STATE					= 0x00010000,
	WIFI_MP_CTX_BACKGROUND			= 0x00020000,	// in continuous tx background
	WIFI_MP_CTX_BACKGROUND_PENDING	= 0x00040000,	// pending in continuous tx background due to out of skb
	WIFI_MP_CTX_PACKET				= 0x00080000,	// in packet mode
	WIFI_MP_CTX_ST					= 0x00100000,	// in continuous tx with single-tone
	WIFI_MP_CTX_CCK_HW				= 0x00200000,	// in cck continuous tx
	WIFI_MP_CTX_CCK_CS				= 0x00400000,	// in cck continuous tx with carrier suppression
	WIFI_MP_CTX_OFDM_HW				= 0x00800000,	// in ofdm continuous tx
	WIFI_MP_RX						= 0x01000000,
#endif

#ifdef WIFI_SIMPLE_CONFIG
	WIFI_WPS			= 0x01000000,
	WIFI_WPS_JOIN		= 0x02000000,
#endif

#ifdef RTL8192SU_SW_BEACON
	WIFI_WAIT_FOR_CHANNEL_SELECT	= 0x04000000,
#endif

};

enum frag_chk_state {
	NO_FRAG		= 0x0,
	UNDER_FRAG	= 0x1,
	CHECK_FRAG	= 0x2,
};

enum led_type {
	LEDTYPE_HW_TX_RX,
	LEDTYPE_HW_LINKACT_INFRA,
	LEDTYPE_SW_LINK_TXRX,
	LEDTYPE_SW_LINKTXRX,
	LEDTYPE_SW_LINK_TXRXDATA,
	LEDTYPE_SW_LINKTXRXDATA,
	LEDTYPE_SW_ENABLE_TXRXDATA,
	LEDTYPE_SW_ENABLETXRXDATA,
	LEDTYPE_SW_ADATA_GDATA,
	LEDTYPE_SW_ENABLETXRXDATA_1,
	LEDTYPE_SW_CUSTOM1,
	LEDTYPE_SW_MAX,
};

enum Synchronization_Sta_State {
	STATE_Sta_Min				= 0,
	STATE_Sta_No_Bss			= 1,
	STATE_Sta_Bss				= 2,
	STATE_Sta_Ibss_Active		= 3,
	STATE_Sta_Ibss_Idle			= 4,
	STATE_Sta_Auth_Success		= 5,
	STATE_Sta_Roaming_Scan		= 6,
};

// Realtek proprietary IE
enum Realtek_capability_IE_bitmap {
	RTK_CAP_IE_turbomode		= 0x01,
	RTK_CAP_IE_USE_LONG_SLOT	= 0x02,
	RTK_CAP_IE_USE_AMPDU		= 0x04,
#ifdef RTK_WOW
	RTK_CAP_IE_USE_WOW		= 0x08,
#endif
	RTK_CAP_IE_WLAN_8192SE	= 0x20,
	RTK_CAP_IE_AP_CLEINT		= 0x80,
};

enum CW_STATE {
	CW_STATE_NORMAL				= 0x00000000,
	CW_STATE_AGGRESSIVE			= 0x00010000,
	CW_STATE_DIFSEXT			= 0x00020000,
	CW_STATE_AUTO_TRUBO			= 0x01000000,
};

enum {TURBO_AUTO=0, TURBO_ON=1, TURBO_OFF=2};

enum NETWORK_TYPE {
	WIRELESS_11B = 1,
	WIRELESS_11G = 2,
	WIRELESS_11A = 4,
	WIRELESS_11N = 8
};

enum FREQUENCY_BAND {
	BAND_2G,
	BAND_5G
};

enum _HT_CHANNEL_WIDTH {
	HT_CHANNEL_WIDTH_20		= 0,
	HT_CHANNEL_WIDTH_20_40	= 1
};

enum SECONDARY_CHANNEL_OFFSET {
	HT_2NDCH_OFFSET_DONTCARE = 0,
	HT_2NDCH_OFFSET_BELOW    = 1,	// secondary channel is below primary channel, ex. primary:5 2nd:1
	HT_2NDCH_OFFSET_ABOVE    = 2	// secondary channel is above primary channel, ex. primary:5 2nd:9
};

enum AGGREGATION_METHOD {
	AGGRE_MTHD_NONE = 0,
	AGGRE_MTHD_MPDU = 1,
	AGGRE_MTHD_MSDU = 2
};

enum _HT_CURRENT_TX_INFO_ {
	TX_USE_40M_MODE		= BIT(0),
	TX_USE_SHORT_GI		= BIT(1)
};

enum _ADD_RATID_UPDATE_CONTENT_ {
	RATID_NONE_UPDATE = 0,
	RATID_GENERAL_UPDATE = 1,
	RATID_INFO_UPDATE = 2
};

enum _DC_TH_CURRENT_STATE_ {
	DC_TH_USE_NONE	= 0,
	DC_TH_USE_UPPER	= 1,
	DC_TH_USE_LOWER	= 2
};

enum _ANTENNA_ {
	ANTENNA_A		= 0x1,
	ANTENNA_B		= 0x2,
	ANTENNA_C		= 0x4,
	ANTENNA_D		= 0x8,
	ANTENNA_AC		= 0x5,
	ANTENNA_AB		= 0x3,
	ANTENNA_BD		= 0xA,
	ANTENNA_CD		= 0xC,
	ANTENNA_ABCD	= 0xF
};


#ifdef USE_RTL8186_SDK
	typedef int bool;
#endif

typedef enum _RT_STATUS{
	RT_STATUS_SUCCESS,
	RT_STATUS_FAILURE,
	RT_STATUS_PENDING,
	RT_STATUS_RESOURCE
}RT_STATUS,*PRT_STATUS;

#define PVOID void *

#define ETHERNET_ADDRESS_LENGTH			6
#define MAX_FRAGMENT_COUNT					8	// Max number of MPDU per MSDU
#define AMSDU_MAX_NUM							16 // Test A-MSDU

#define MAX_PER_PACKET_VIRTUAL_BUF_NUM		24
#define MAX_PER_PACKET_PHYSICAL_BUF_NUM		MAX_PER_PACKET_VIRTUAL_BUF_NUM
#define MAX_PER_PACKET_BUFFER_LIST_LENGTH	(MAX_PER_PACKET_PHYSICAL_BUF_NUM+MAX_FRAGMENT_COUNT+4+16)		// +4 is for FirmwareInfo, Header, LLC and trailer, MAX_FRAGMENT_COUNT for max fragment overhead
														// +16 is for A-MSDU aggregation. Add this to aggregate more frames.
#define UNSPECIFIED_DATA_RATE				0xffff														


#define RTInsertTailList(ListHead,Entry)			\
	{											\
		PRT_LIST_ENTRY _EX_Blink;					\
		PRT_LIST_ENTRY _EX_ListHead;				\
		_EX_ListHead = (ListHead);					\
		_EX_Blink = _EX_ListHead->Blink;			\
		(Entry)->Flink = _EX_ListHead;				\
		(Entry)->Blink = _EX_Blink;				\
		_EX_Blink->Flink = (Entry);					\
		_EX_ListHead->Blink = (Entry);				\
	}

#define RTRemoveEntryList(Entry)					\
	{											\
		PRT_LIST_ENTRY _EX_Blink;					\
		PRT_LIST_ENTRY _EX_Flink;					\
		_EX_Flink = (Entry)->Flink;					\
		_EX_Blink = (Entry)->Blink;				\
		_EX_Blink->Flink = _EX_Flink;				\
		_EX_Flink->Blink = _EX_Blink;				\
	}

#define RTIsListEmpty(ListHead)					\
	((ListHead)->Flink == (ListHead))
#define RTRemoveHeadList(ListHead)				\
	(ListHead)->Flink;							\
	{RTRemoveEntryList((ListHead)->Flink)}

#define RTRemoveHeadListWithCnt(ListHead,Cnt)				\
	(ListHead)->Flink;										\
	RTRemoveEntryListWithCnt((ListHead)->Flink, Cnt)
#define RTRemoveEntryListWithCnt(Entry,Cnt)					\
	RTRemoveEntryList(Entry)								\
	(*Cnt)--;	
	

typedef enum{
	NO_Encryption			= 0,
	WEP40_Encryption		= 1,
	TKIP_Encryption			= 2,
	Reserved_Encryption		= 3,
	AESCCMP_Encryption 	= 4,
	WEP104_Encryption		= 5,
}RT_ENC_ALG, *PRT_ENC_ALG;

typedef struct _RT_LIST_ENTRY {
	struct _RT_LIST_ENTRY *Flink;
	struct _RT_LIST_ENTRY *Blink;
} RT_LIST_ENTRY, *PRT_LIST_ENTRY;
typedef enum _RT_PROTOCOL_TYPE{
	RT_PROTOCOL_802_3,
	RT_PROTOCOL_802_11,
	RT_PROTOCOL_COMMAND
}PROTOCOL_TYPE, *PPROTOCOL_TYPE;
typedef enum _RT_TCB_BUFFER_TYPE{
	RT_TCB_BUFFER_TYPE_SYSTEM,				// Packet is from upper layer
	RT_TCB_BUFFER_TYPE_RFD,					// Receive forwarding packet
	RT_TCB_BUFFER_TYPE_LOCAL,				// Management
	RT_TCB_BUFFER_TYPE_FW_LOCAL,			// Firmware download buffer
}RT_TCB_BUFFER_TYPE,*PRT_TCB_BUFFER_TYPE;
typedef struct _SHARED_MEMORY{
	unsigned char*	VirtualAddress;
	unsigned int	PhysicalAddressHigh;
	unsigned int	PhysicalAddressLow;
	unsigned int	Length;
}SHARED_MEMORY,*PSHARED_MEMORY;
typedef struct _RT_TCB_STATUS{
	unsigned short	DataRetryCount:8;
	unsigned short	TOK:1;
	unsigned int	FrameLength:12;
}RT_TCB_STATUS,*PRT_TCB_STATUS;

typedef struct _STA_ENC_INFO_T{
	RT_ENC_ALG			EncAlg;
	bool				IsEncByHW;		// FALSE if this packet SHOULD NOT be encrypted by HW, TRUE otherwise. Note that, it is used in conjuntion with pMgntInfo->SecurityInfo.SWTxEncryptFlag to determine if we shall encrypt the packet by HW or SW.
	unsigned char		keyId;
	unsigned short      ExemptionActionType; //0:Auto 1:encrypt 2:no encrypt 3: key UNAVAILABLE
	unsigned int		keylen;
	unsigned char*		keybuf;
	//RT_SEC_PROT_INFO	SecProtInfo;	// Keep higher layer meaning and context information about this packet, see definition of RT_SEC_PROT_INFO for details.
	unsigned int	SecProtInfo;	// Keep higher layer meaning and context information about this packet, see definition of RT_SEC_PROT_INFO for details.
}STA_ENC_INFO_T, *PSTA_ENC_INFO_T;
typedef struct _RT_TX_LOCAL_BUFFER{
	RT_LIST_ENTRY		List;
	SHARED_MEMORY		Buffer;
}RT_TX_LOCAL_BUFFER, *PRT_TX_LOCAL_BUFFER;

struct pkt_queue {
	struct sk_buff	*pSkb[NUM_TXPKT_QUEUE];
	int	head;
	int	tail;
};

#if defined(SEMI_QOS) && defined(WMM_APSD)
struct apsd_pkt_queue {
	struct sk_buff	*pSkb[NUM_APSD_TXPKT_QUEUE];
	int				head;
	int				tail;
};
#endif

#ifdef GREEN_HILL
#pragma pack(1)
#endif


/**
 *	@brief MAC Frame format - wlan_hdr (wireless LAN header)
 *
 *	Dcscription: 802.11 MAC Frame (header). See textbook P.46
 *	p.s : memory aligment by BYTE,
 *	      __PACK : not need 4 bytes aligment
 */
__PACK struct wlan_hdr {
	unsigned short	fmctrl;
	unsigned short	duration;
	unsigned char	addr1[MACADDRLEN];
	unsigned char	addr2[MACADDRLEN];
	unsigned char	addr3[MACADDRLEN];
	unsigned short	sequence;
	unsigned char	addr4[MACADDRLEN];
	unsigned short	qosctrl;
#ifdef CONFIG_RTK_MESH
	struct lls_mesh_header	meshhdr;	// 11s, mesh header, 4~16 bytes
#endif
	unsigned char	iv[8];
} __WLAN_ATTRIB_PACK__;

#ifdef GREEN_HILL
#pragma pack()
#endif

struct wlan_hdrnode {
	struct list_head	list;
	struct wlan_hdr		hdr;
};

struct wlan_hdr_poll {
	struct wlan_hdrnode hdrnode[PRE_ALLOCATED_HDR];
	int					count;
};

#ifdef GREEN_HILL
#pragma pack(1)
#endif

__PACK struct wlanllc_hdr {
	struct wlan_hdr		wlanhdr;
	struct llc_snap		llcsnaphdr;
} __WLAN_ATTRIB_PACK__;

#ifdef GREEN_HILL
#pragma pack()
#endif

struct wlanllc_node {
	struct list_head	list;
	struct wlanllc_hdr	hdr;

#ifdef CONFIG_RTK_MESH
	unsigned char amsdu_header[30];
#else
    unsigned char amsdu_header[14];
#endif

};

struct wlanllc_hdr_poll {
	struct wlanllc_node	hdrnode[PRE_ALLOCATED_HDR];
	int					count;
};

struct wlanbuf_node {
	struct list_head	list;
	unsigned int		buf[PRE_ALLOCATED_BUFSIZE]; // 4 bytes alignment!
};

struct wlanbuf_poll {
	struct wlanbuf_node	hdrnode[PRE_ALLOCATED_MMPDU];
	int					count;
};

struct wlanicv_node {
	struct list_head	list;
	unsigned int		icv[2];
};

struct wlanicv_poll {
	struct wlanicv_node	hdrnode[PRE_ALLOCATED_HDR];
	int					count;
};

struct wlanmic_node {
	struct list_head	list;
	unsigned int		mic[2];
};

struct wlanmic_poll {
	struct wlanmic_node	hdrnode[PRE_ALLOCATED_HDR];
	int					count;
};

struct wlan_acl_node {
	struct list_head	list;
	unsigned char		addr[MACADDRLEN];
	unsigned char		mode;
};

struct wlan_acl_poll {
	struct wlan_acl_node aclnode[NUM_ACL];
};

#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_)
struct mesh_acl_poll {
	struct wlan_acl_node meshaclnode[NUM_MESH_ACL];
};
#endif

#ifdef RTL8192SU_TX_FEW_INT
struct tx_pool {
	struct urb *urb[MAX_TX_URB];
	struct tx_desc_info *pdescinfo[MAX_TX_URB];
	unsigned char		own[MAX_TX_URB]; //0:SW(free), 1:HW(not free)
};
#endif

struct tx_insn	{
	unsigned int		q_num;
	void				*pframe;
	unsigned char		*phdr;			//in case of mgt frame, phdr is wlan_hdr,
										//in case of data, phdr = wlan + llc
	unsigned int		hdr_len;
	unsigned int		fr_type;
	unsigned int		fr_len;
	unsigned int		frg_num;
	unsigned int		need_ack;
	unsigned int		frag_thrshld;
	unsigned int		rts_thrshld;
	unsigned int		privacy;
	unsigned int		iv;
	unsigned int		icv;
	unsigned int		mic;
	unsigned char		llc;
	unsigned char		tx_rate;
	unsigned char		lowest_tx_rate;
	unsigned char		fixed_rate;
	unsigned char		retry;
	unsigned char		aggre_en;
	unsigned char		tpt_pkt;
	unsigned char		one_txdesc;
#ifdef WDS
	int					wdsIdx;
#endif
	struct stat_info	*pstat;

#ifdef CONFIG_RTK_MESH
	unsigned char		is_11s;			// for transmitting 11s data frame (to rewrite 4 addresses)
	unsigned char		nhop_11s[MACADDRLEN]; // to record "da" in start_xmit
	unsigned char		prehop_11s[MACADDRLEN];
	struct  lls_mesh_header mesh_header;
#endif

};


#ifdef A4_TUNNEL
struct wlan_a4tnl_node {
	struct list_head	list;
	unsigned char		addr[MACADDRLEN];
	struct stat_info	*pstat;
};
#endif


struct reorder_ctrl_entry
{
	struct sk_buff		*packet_q[RC_ENTRY_NUM];
	unsigned char		start_rcv;
	short				rc_timer_id;
	unsigned short		win_start;
	unsigned short		last_seq;
};

#ifdef SUPPORT_TX_MCAST2UNI
struct ip_mcast_info {
	int					used;
	unsigned char		mcmac[MACADDRLEN];
};
#endif

struct stat_info {
	struct list_head	hash_list;	// always keep the has_list as the first item, to accelerat searching
	struct list_head	asoc_list;
	struct list_head	auth_list;
	struct list_head	sleep_list;
	struct list_head	defrag_list;
	struct list_head	wakeup_list;
	struct list_head	frag_list;
	struct list_head	addRAtid_list;	// to avoid add RAtid fail

	struct sk_buff_head	dz_queue;	// Queue for sleeping mode

#ifdef CONFIG_RTK_MESH
	struct list_head	mesh_mp_ptr;	// MESH MP list
#endif


#ifdef INCLUDE_WPA_PSK
	WPA_STA_INFO		*wpa_sta_info;
#endif

#if defined(SEMI_QOS) && defined(WMM_APSD)
	unsigned char		apsd_bitmap;	// bit 0: VO, bit 1: VI, bit 2: BK, bit 3: BE
	unsigned int		apsd_pkt_buffering;
	struct apsd_pkt_queue	*VO_dz_queue;
	struct apsd_pkt_queue	*VI_dz_queue;
	struct apsd_pkt_queue	*BE_dz_queue;
	struct apsd_pkt_queue	*BK_dz_queue;
#endif

	/****************************************************************
	 * from here on, data will be clear in init_stainfo() except "aid" and "hwaddr"  *
	 ****************************************************************/
	unsigned int		auth_seq;
	unsigned char		chg_txt[128];

	unsigned int		frag_to;
	unsigned int		frag_count;
	unsigned int		sleep_to;
	unsigned short		tpcache[8][TUPLE_WINDOW];
	unsigned short		tpcache_mgt;	// mgt cache number
#ifdef _DEBUG_RTL8190_
	unsigned int		rx_amsdu_err;
	unsigned int		rx_amsdu_1pkt;
	unsigned int		rx_amsdu_2pkt;
	unsigned int		rx_amsdu_3pkt;
	unsigned int		rx_amsdu_4pkt;
	unsigned int		rx_amsdu_5pkt;
	unsigned int		rx_amsdu_gt5pkt;

	unsigned int		rx_rc_drop1;
	unsigned int		rx_rc_drop3;
	unsigned int		rx_rc_reorder3;
	unsigned int		rx_rc_reorder4;
	unsigned int		rx_rc_passup2;
	unsigned int		rx_rc_passup3;
	unsigned int		rx_rc_passup4;
	unsigned int		rx_rc_passupi;
#endif

#if defined(CLIENT_MODE) || defined(WDS)
	unsigned int		beacon_num;
#endif


#ifdef TX_SHORTCUT
	struct tx_insn		txcfg;
	struct wlanllc_hdr 	wlanhdr;
	struct tx_desc		hwdesc1;
	struct tx_desc		hwdesc2;
	struct tx_desc_info	swdesc1;
	struct tx_desc_info	swdesc2;
	struct tx_insn		txcfg_short;
	struct tx_desc		hwdesc3;
	struct tx_desc		hwdesc4;
	struct tx_desc_info	swdesc3;
	int 				protection;
	int					sc_keyid;
	struct wlan_ethhdr_t	ethhdr;
	unsigned char		pktpri;
#endif
	unsigned long setCamTime;	  //cathy, to resolve that hw setting is too slow in tkip mode

#ifdef RX_SHORTCUT
	int					rx_payload_offset;
	int					rx_trim_pad;
	struct wlan_ethhdr_t	rx_ethhdr;
	struct wlanllc_hdr 	rx_wlanhdr;
	int					rx_privacy;
#endif

#ifdef SUPPORT_TX_AMSDU
	struct sk_buff_head	amsdu_tx_que[8];
	int					amsdu_timer_id[8];
	int					amsdu_size[8];
#endif

#ifdef BUFFER_TX_AMPDU
	struct sk_buff_head	ampdu_tx_que[8];
	int					ampdu_timer_id[8];
	int					ampdu_size[8];
#endif

	/******************************************************************
	 * from here to end, data will be backup when doing FAST_RECOVERY *
	 ******************************************************************/
	unsigned short		aid;
#ifdef STA_EXT
	unsigned short		remapped_aid;// support up to 64 clients
#endif
	unsigned char		hwaddr[MACADDRLEN];
#ifdef SEMI_QOS
	unsigned int 		QosEnabled;
	unsigned short		AC_seq[8];
#endif
	enum wifi_state		state;
	unsigned int		AuthAlgrthm;		// could be open/shared key
	unsigned int		ieee8021x_ctrlport;	// 0 for blocked, 1 for open
	unsigned int		keyid;				// this could only be valid in legacy wep
	unsigned int		keylen;
	struct Dot11KeyMappingsEntry	dot11KeyMapping;
	unsigned char		bssrateset[32];
	unsigned int		bssratelen;
	unsigned int		useShortPreamble;
	unsigned int		expire_to;
	unsigned char		rssi;
	unsigned char		sq;
	unsigned char		rx_rate;
	unsigned char		rx_bw;
	unsigned char		rx_splcp;
	struct rf_misc_info	rf_info;
	unsigned short		seq_backup;
	unsigned char		rssi_backup;
	unsigned char		sq_backup;
	unsigned char		rx_rate_backup;
	unsigned char		rx_bw_backup;
	unsigned char		rx_splcp_backup;
	struct rf_misc_info rf_info_backup;
	int					cck_mimorssi_total[4];
	unsigned char		cck_rssi_num;
	unsigned char		highest_rx_rate;
	unsigned char		active;
	unsigned char		rssi_level;
	unsigned char		is_rtl8190_sta;
	unsigned char		is_rtl8190_apclient;
	unsigned char		is_rtl8192s_sta;
	unsigned char		is_broadcom_sta;
	unsigned char		is_marvell_sta;
	unsigned char		is_intel_sta;
	unsigned char		is_2t_mimo_sta;
#ifdef RTK_WOW
	unsigned char		is_rtk_wow_sta;
#endif
	unsigned char		is_forced_ampdu;
	unsigned char		is_forced_rts;
	unsigned char		aggre_mthd;
	unsigned char		tx_bw;
	unsigned char		ht_current_tx_info;	// bit0: 0=20M mode, 1=40M mode; bit1: 0=longGI, 1=shortGI
	unsigned long		link_time;
	unsigned char		private_ie[32];
	unsigned int		private_ie_len;
	unsigned int		tx_ra_bitmap;

	unsigned int		tx_bytes;
	unsigned int		rx_bytes;
	unsigned int		tx_pkts;
	unsigned int		rx_pkts;
	unsigned int		tx_fail;
	unsigned int		tx_pkts_pre;
	unsigned int		rx_pkts_pre;
	unsigned int		tx_fail_pre;
	unsigned int		current_tx_rate;
	unsigned int		tx_byte_cnt;
	unsigned int		tx_avarage;
	unsigned int		rx_byte_cnt;
	unsigned int		rx_avarage;

	// bcm old 11n chipset iot debug, and TXOP enlarge
	unsigned int		current_tx_bytes;
	unsigned int		current_rx_bytes;

	// 11n ap AES debug
	unsigned char		is_fw_matching_sta;

	// enhance f/w Rate Adaptive
	unsigned int		rssi_tick;

	unsigned int		ratid_update_content;

#ifdef WDS
	int					wds_idx;
	unsigned int		wds_probe_done;
	unsigned int		idle_time;
#endif

	struct ht_cap_elmt	ht_cap_buf;
	unsigned int		ht_cap_len;
	struct ht_info_elmt	ht_ie_buf;
	unsigned int		ht_ie_len;
	unsigned char		cam_id;
	unsigned char		MIMO_ps;
	unsigned char		dialog_token;
	unsigned char		is_8k_amsdu;
	unsigned char		ADDBA_ready;
	unsigned int		amsdu_level;
	unsigned char		tmp_mic_key[8];

#ifdef GBWC
	unsigned char		GBWC_in_group;
#endif

	struct reorder_ctrl_entry	rc_entry[8];

#ifdef  SUPPORT_TX_MCAST2UNI
	int					ipmc_num;
	struct ip_mcast_info	ipmc[MAX_IP_MC_ENTRY];
#endif

#ifdef USB_PKT_RATE_CTRL_SUPPORT
	unsigned int		change_toggle;
#endif

#if defined(RTL8192E) || defined(STA_EXT)
	int					sta_in_firmware;
#endif
#ifdef CONFIG_RTK_MESH
	struct MESH_Neighbor_Entry	mesh_neighbor_TBL;	//mesh_neighbor

	// Throughput statistics (sounder)
	//	struct flow_stats		f_stats;
#endif
#if defined(CONFIG_RTL_WAPI_SUPPORT)
	wapiStaInfo		*wapiInfo;
#endif
};


/*
 *	Driver open, alloc whole amount of aid_obj, avoid memory fragmentation
 *	If STA Association , The aid_obj will chain to stat_info.
 */
struct aid_obj {
	struct stat_info	station;
	unsigned int		used;	// used == TRUE => has been allocated, used == FALSE => can be allocated
	struct rtl8190_priv *priv;
};

/* Note: always calculate the WLAN average throughput, if the throughput is larger than TP_HIGH_WATER_MARK,
             gCpuCanSuspend will be FALSE. If the throughput is smaller than TP_LOW_WATER_MARK,
             gCpuCanSuspend will be TRUE.
             However, you can undefine the CONFIG_RTL8190_THROUGHPUT. The gCpuCanSuspend will always be
             TRUE in this case.
 */
#define CONFIG_RTL8190_THROUGHPUT

struct extra_stats {
	unsigned long		tx_retrys;
	unsigned long		tx_drops;
	unsigned long		rx_retrys;
	unsigned long		rx_decache;
	unsigned long		rx_data_drops;
	unsigned long		beacon_ok;
	unsigned long		beacon_er;
	unsigned long 		beaconQ_sts;
#if defined(CONFIG_RTL8196B_TR) || defined(CONFIG_RTL8190_THROUGHPUT) || defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
	unsigned long		tx_peak;
	unsigned long		rx_peak;
#endif
	unsigned long		tx_byte_cnt;
	unsigned long		tx_avarage;
	unsigned long		rx_byte_cnt;
	unsigned long		rx_avarage;
	unsigned long		rx_fifoO;
	unsigned long		rx_rdu;
	unsigned long		freeskb_err;
	unsigned long		reused_skb;
#ifdef 	LOOPBACK_NORMAL_TX_MODE	
	unsigned long		loopback_TX_cnt;
#endif
#if defined(TX_SHORTCUT)&&defined(RTL8192SU_TXSC_DBG)
	unsigned long		tx_shortcut_cnt;
	unsigned long		mgt_pkt_cnt;
	unsigned long		normal_pkt_cnt;
	unsigned long		nonTxSC_pkt_cnt;
#endif
};

#ifdef WIFI_SIMPLE_CONFIG
struct wps_ie_info {
	unsigned char rssi;
	unsigned char data[MAX_WSC_IE_LEN];
};
#endif

struct ss_res {
	unsigned int		ss_channel;
	unsigned int		count;
	struct bss_desc		bss[MAX_BSS_NUM];
	unsigned int		count_backup;
	struct bss_desc		bss_backup[MAX_BSS_NUM];
	unsigned int		count_target;
	struct bss_desc		bss_target[MAX_BSS_NUM];
#ifdef WIFI_SIMPLE_CONFIG
	struct wps_ie_info	ie[MAX_BSS_NUM];
	struct wps_ie_info	ie_backup[MAX_BSS_NUM];
#endif
};

struct stat_info_cache {
	struct stat_info	*pstat;
	unsigned char		hwaddr[6];
};

struct rf_finetune_var {
	unsigned char		ofdm_1ss_oneAnt;// for 2T2R
	unsigned char		pathB_1T; // for 1T2R, 1T1R
#ifdef EXT_ANT_DVRY
	unsigned char 		ext_ad_th;
	unsigned char		ext_ad_Ttry;
	unsigned char		ext_ad_Ts;
	unsigned char		ExtAntDvry;
#endif
	unsigned char		rssi_dump;
	unsigned char		rxfifoO;
	unsigned char		raGoDownUpper;
	unsigned char		raGoDown20MLower;
	unsigned char		raGoDown40MLower;
	unsigned char		raGoUpUpper;
	unsigned char		raGoUp20MLower;
	unsigned char		raGoUp40MLower;
	unsigned char		digGoLowerLevel;
	unsigned char		digGoUpperLevel;
	unsigned char		dcThUpper;
	unsigned char		dcThLower;
	unsigned char		rssiTx20MUpper;
	unsigned char		rssiTx20MLower;
	unsigned char		rssi_expire_to;

	// bcm old 11n chipset iot debug

	unsigned char		mlcstRxIgUpperBound;

	// dynamic Rx path selection by signal strength
	unsigned char		ss_th_low;
	unsigned char		diff_th;
	unsigned char		cck_sel_ver;
	unsigned char		cck_accu_num;

	// dynamic Tx power control
	unsigned char		tx_pwr_ctrl;

	// 11n ap AES debug
	unsigned char		aes_check_th;

	// dynamic CCK Tx power by rssi
	unsigned char		cck_enhance;

	// Tx power tracking
	unsigned int		tpt_period;

	// TXOP enlarge
	unsigned char		txop_enlarge_upper;
	unsigned char		txop_enlarge_lower;

#ifdef BUFFER_TX_AMPDU
	unsigned int		dot11nAMPDUBufferMax;	// max size of buffered Tx AMPDU
	unsigned int		dot11nAMPDUBufferTimeout;	// timeout value for buffered Tx AMPDU
	unsigned int		dot11nAMPDUBufferNum;	// max num of buffered Tx AMPDU
#endif

	// 2.3G support
	unsigned char		use_frq_2_3G;

	// for mp test
#ifdef MP_TEST
	unsigned char		mp_specific;
#endif

	//Support IP multicast->unicast
#ifdef SUPPORT_TX_MCAST2UNI
	unsigned char		mc2u_disable;
#ifdef IGMP_FILTER_CMO
	unsigned char		igmp_deny;
#endif
#endif

#ifdef WIFI_11N_2040_COEXIST
	unsigned char		coexist_enable;
#endif

#ifdef	HIGH_POWER_EXT_PA
	unsigned char		use_ext_pa;
#endif

#ifdef ADD_TX_POWER_BY_CMD
	unsigned char		txPowerPlus_cck;
	unsigned char		txPowerPlus_ofdm_6;
	unsigned char		txPowerPlus_ofdm_9;
	unsigned char		txPowerPlus_ofdm_12;
	unsigned char		txPowerPlus_ofdm_18;
	unsigned char		txPowerPlus_ofdm_24;
	unsigned char		txPowerPlus_ofdm_36;
	unsigned char		txPowerPlus_ofdm_48;
	unsigned char		txPowerPlus_ofdm_54;
	unsigned char		txPowerPlus_mcs_0;
	unsigned char		txPowerPlus_mcs_1;
	unsigned char		txPowerPlus_mcs_2;
	unsigned char		txPowerPlus_mcs_3;
	unsigned char		txPowerPlus_mcs_4;
	unsigned char		txPowerPlus_mcs_5;
	unsigned char		txPowerPlus_mcs_6;
	unsigned char		txPowerPlus_mcs_7;
	unsigned char		txPowerPlus_mcs_8;
	unsigned char		txPowerPlus_mcs_9;
	unsigned char		txPowerPlus_mcs_10;
	unsigned char		txPowerPlus_mcs_11;
	unsigned char		txPowerPlus_mcs_12;
	unsigned char		txPowerPlus_mcs_13;
	unsigned char		txPowerPlus_mcs_14;
	unsigned char		txPowerPlus_mcs_15;
#endif
};


/* Bit mask value of type */
#define ACCESS_SWAP_IO		0x01	/* Do bye-swap in access IO register */
#define ACCESS_SWAP_MEM		0x02	/* Do byte-swap in access memory space */

#define ACCESS_MASK			0x3
#define TYPE_SHIFT			2
#define TYPE_MASK			0x3

#ifdef WDS
#define WDS_SHIFT			4
#define WDS_MASK			0xf
#define WDS_NUM_CFG			NUM_WDS
#else
#define WDS_SHIFT			0
#define WDS_NUM_CFG			0
#endif


enum {
	TYPE_EMBEDDED	= 0,	/* embedded wlan controller */
	TYPE_PCI_DIRECT	= 1,	/* PCI wlan controller and enable PCI bridge directly */
	TYPE_PCI_BIOS	= 2		/* PCI wlan controller and enable PCI by BIOS */
};


enum {
	DRV_STATE_INIT	 = 1,	/* driver has been init */
	DRV_STATE_OPEN	= 2,	/* driver is opened */
	DRV_STATE_CLOSE	= 4,	/* driver is closing */
#ifdef UNIVERSAL_REPEATER
	DRV_STATE_VXD_INIT = 4,	/* vxd driver has been opened */
	DRV_STATE_VXD_AP_STARTED	= 8, /* vxd ap has been started */
#endif
};


#ifdef CHECK_TX_HANGUP
#define PENDING_PERIOD		2	// max time of pending period


struct desc_check_info {
	int pending_tick;	// tick value when pending is detected
	int pending_tail;	// descriptor tail number when pending is detected
	int idle_tick;		// tick value when detect idle (tx desc is not free)
};
#endif


// MAC access control log definition
#define MAX_AC_LOG		32	// max number of log entry
#define AC_LOG_TIME		300	// log time in sec
#define AC_LOG_EXPIRE	300	// entry expire time

struct ac_log_info {
	int	used;		// <>0: in use
	int	cur_cnt;	// current attack counter
	int	last_cnt;	// counter of last time log
	unsigned long last_attack_time; // jiffies time of last attack
	unsigned char addr[MACADDRLEN];	// mac address
};

#ifdef WIFI_SIMPLE_CONFIG
struct wsc_probe_request_info {
	unsigned char addr[MACADDRLEN]; // mac address
	char ProbeIE[PROBEIELEN];
	unsigned short ProbeIELen;
	unsigned long time_stamp; // jiffies time of last probe request
	unsigned char used;
};
#endif

struct reorder_ctrl_timer
{
	struct rtl8190_priv	*priv;
	struct stat_info	*pstat;
	unsigned char		tid;
	unsigned int		timeout;
};

#ifdef RTK_QUE
struct ring_que {
	int qlen;
	int qmax;
	int head;
	int tail;
	struct sk_buff *ring[MAX_PRE_ALLOC_SKB_NUM+1];
};
#endif


// common private structure which info are shared between root interface and virtual interface
struct priv_shared_info {
	unsigned int			type;
	unsigned int			ioaddr;
	unsigned int			VersionID;
#ifdef IO_MAPPING
	unsigned int			io_mapping;
#endif

#ifdef USE_CHAR_DEV
	struct rtl8190_chr_priv	*chr_priv;
#endif
#ifdef USE_PID_NOTIFY
	pid_t					wlanapp_pid;
#endif

#ifdef CONFIG_RTL_WAPI_SUPPORT
	pid_t					wlanwapi_pid;
#endif


	struct tasklet_struct	rx_tasklet;
	struct tasklet_struct	tx_tasklet;
	struct tasklet_struct	oneSec_tasklet;

	struct wlan_hdr_poll	*pwlan_hdr_poll;
	struct list_head		wlan_hdrlist;

	struct wlanllc_hdr_poll	*pwlanllc_hdr_poll;
	struct list_head		wlanllc_hdrlist;

	struct wlanbuf_poll		*pwlanbuf_poll;
	struct list_head		wlanbuf_list;

	struct wlanicv_poll		*pwlanicv_poll;
	struct list_head		wlanicv_list;

	struct wlanmic_poll		*pwlanmic_poll;
	struct list_head		wlanmic_list;

	struct rtl8190_hw		*phw;
	unsigned int			have_hw_mic;

	struct aid_obj			*aidarray[NUM_STAT];

#ifdef _11s_TEST_MODE_
	struct Galileo_poll 	*galileo_poll;
	struct list_head		galileo_list;
#endif


#ifdef RTL867X_DMEM_ENABLE
	unsigned char			*fw_IMEM_buf;
	unsigned char			*fw_EMEM_buf;
	unsigned char			*fw_DMEM_buf;
	unsigned char			*agc_tab_buf;
	unsigned char			*mac_reg_buf;
	unsigned char			*phy_reg_buf;
#else

#if defined(MERGE_FW) ||defined(DW_FW_BY_MALLOC_BUF)
	unsigned char			*fw_IMEM_buf;
	unsigned char			*fw_EMEM_buf;
	unsigned char			*fw_DMEM_buf;
#else
	unsigned char			fw_IMEM_buf[FW_IMEM_SIZE];
	unsigned char			fw_EMEM_buf[FW_EMEM_SIZE];
	unsigned char			fw_DMEM_buf[FW_DMEM_SIZE];
#endif
	unsigned char			agc_tab_buf[AGC_TAB_SIZE];
	unsigned char			mac_reg_buf[MAC_REG_SIZE];
	unsigned char			phy_reg_buf[PHY_REG_SIZE];
#endif //RTL867X_DMEM_ENABLE
#ifdef MP_TEST
	unsigned char			phy_reg_mp_buf[PHY_REG_SIZE];
#endif
	unsigned char			phy_reg_pg_buf[PHY_REG_PG_SIZE];
	unsigned char			phy_reg_2to1[PHY_REG_1T2R];
	unsigned short			fw_IMEM_len;
	unsigned short			fw_EMEM_len;
	unsigned short			fw_DMEM_len;

#ifdef HW_QUICK_INIT
	unsigned short			hw_inited;
	unsigned short			hw_init_num;
	unsigned char			last_reinit;
#endif

	spinlock_t				lock;

	// for RF fine tune
	struct rf_finetune_var	rf_ft_var;

	// bcm old 11n chipset iot debug

	// TXOP enlarge
	unsigned int			txop_enlarge;	// 0=no txop, 1=half txop enlarged, 2=full txop enlarged

	int						is_40m_bw;
	int						offset_2nd_chan;
//	int						is_giga_exist;

#ifdef CONFIG_RTK_MESH
	struct MESH_Share		meshare; 	// mesh share data
	spinlock_t				lock_queue;	// lock for DOT11_EnQueue2/DOT11_DeQueue2
	spinlock_t				lock_Rreq;	// lock for rreq_retry. Some function like aodv_expire/tx use lock_queue simultaneously
#endif

#ifdef RTL8192SU_TX_FEW_INT
	struct tx_pool		tx_pool;
	int					tx_pool_idx;
#endif

	unsigned int			curr_band;				// remember the current band to save switching time
	unsigned short			fw_version;
	unsigned short			fw_src_version;
	unsigned short			fw_sub_version;
	unsigned int			CamEntryOccupied;		// how many entries in CAM?

	unsigned char			rtk_ie_buf[16];
	unsigned int			rtk_ie_len;
	unsigned char			*rtk_cap_ptr;
	unsigned short			rtk_cap_val;
	unsigned char			use_long_slottime;

	// for Tx power control
	unsigned char			working_channel;
	unsigned char			use_default_para;
	unsigned char			legacyOFDM_pwrdiff_A;
	unsigned char			legacyOFDM_pwrdiff_B;
	signed char				channelAB_pwrdiff;
	unsigned char			min_ampdu_spacing;

	/*********************************************************
	 * from here on, data will be clear in rtl8190_init_sw() *
	 *********************************************************/

	// for SW LED
	struct timer_list		LED_Timer;
	unsigned int			LED_Interval;
	unsigned char			LED_Toggle;
	unsigned char			LED_ToggleStart;
	unsigned char 			LED_sw_use;
	unsigned int			LED_tx_cnt_log;
	unsigned int			LED_rx_cnt_log;
	unsigned int			LED_tx_cnt;
	unsigned int			LED_rx_cnt;

	// for Rx dynamic tasklet
	unsigned int			rxInt_useTsklt;
	unsigned int			rxInt_data_delta;

#ifdef CHECK_HANGUP
#ifdef CHECK_TX_HANGUP
	struct desc_check_info	Q_info[5];
#endif
#ifdef CHECK_RX_HANGUP
	unsigned int			rx_hang_checking;
	unsigned int			rx_cntreg_log;
	unsigned int			rx_stop_pending_tick;
	struct rtl8190_priv		*selected_priv;
#endif
#ifdef CHECK_BEACON_HANGUP
	unsigned int			beacon_ok_cnt;
	unsigned int			beacon_pending_cnt;
	unsigned int			beacon_wait_cnt;
#endif
	unsigned int			reset_monitor_cnt_down;
	unsigned int			reset_monitor_pending;
	unsigned int			reset_monitor_rx_pkt_cnt;
#endif

#ifdef MP_TEST
	unsigned char			mp_datarate;
	unsigned char			mp_antenna_tx;
	unsigned char			mp_antenna_rx;
	unsigned char			mp_txpwr_cck;
	unsigned char			mp_txpwr_ofdm;
	unsigned char			mp_txpwr_mcs_1ss;
	unsigned char			mp_txpwr_mcs_2ss;
	void 					*skb_pool_ptr;
	struct sk_buff 			*skb_pool[NUM_MP_SKB];
	int						skb_head;
	int						skb_tail;

	unsigned char			mp_rssi;
	unsigned char			mp_sq;
	struct rf_misc_info		mp_rf_info;

	unsigned char			mp_ofdm_swing_idx;
	unsigned char			mp_cck_swing_idx;
	unsigned char			mp_txpwr_tracking;

#ifdef B2B_TEST
	volatile unsigned long	mp_rx_ok, mp_rx_sequence, mp_rx_lost_packet, mp_rx_dup;
	volatile unsigned short	mp_cached_seq;
	int 					mp_rx_waiting;
	volatile unsigned int	mp_mac_changed;
	unsigned long			txrx_elapsed_time;
	unsigned long			txrx_start_time;
#endif
#endif // MP_TEST

	// monitor Tx and Rx
	unsigned long			tx_packets_pre;
	unsigned long			rx_packets_pre;

	// bcm old 11n chipset iot debug, and TXOP enlarge
	unsigned long			current_tx_bytes;
	unsigned long			current_rx_bytes;

	unsigned int			CurrentChannelBW;
	unsigned char			*txcmd_buf;
	unsigned long			cmdbuf_phyaddr;
	unsigned long			InterruptMask;
	unsigned long			InterruptMaskExt;
	unsigned int			rx_rpt_ofdm;
	unsigned int			rx_rpt_cck;
	unsigned int			rx_rpt_ht;
	unsigned int			successive_bb_hang;

	unsigned long			rxFiFoO_pre;

#ifdef RTK_QUE
	struct ring_que 		skb_queue;
#else
	struct sk_buff_head		skb_queue;
#endif

	struct timer_list			rc_sys_timer;
	struct reorder_ctrl_timer	rc_timer[64];
	unsigned short				rc_timer_head;
	unsigned short				rc_timer_tail;

	struct reorder_ctrl_timer	amsdu_timer[64];
	unsigned short				amsdu_timer_head;
	unsigned short				amsdu_timer_tail;

#ifdef BUFFER_TX_AMPDU
	struct reorder_ctrl_timer	ampdu_timer[64];
	unsigned short				ampdu_timer_head;
	unsigned short				ampdu_timer_tail;
#endif

	// ht associated client statistic
#ifdef SEMI_QOS
	unsigned int			ht_sta_num;
#endif

	unsigned int			set_led_in_progress;

	struct stat_info		*CurPstat; // for tx desc break field
	unsigned int			has_2r_sta; // Used when AP is 2T2R. bitmap of 2R aid
	int						has_triggered_rx_tasklet;
	int						has_triggered_tx_tasklet;

#if defined(RTL8192E) || defined(STA_EXT)
	unsigned char			fw_free_space;
#endif

#if (defined(RTL8192SE)||defined(RTL8192SU)) && defined(EXT_ANT_DVRY)
	unsigned char 			EXT_AD_selection;// 0 is default, 1 is extra
	unsigned int			EXT_AD_nextTs;
	unsigned int			EXT_AD_counter;
	unsigned int			EXT_AD_probe;
	struct stat_info		*EXT_AD_pstat;
#endif

	// RF and BB access related synchronization flags.
	bool			bChangeBBInProgress; // BaseBand RW is still in progress.
	bool			bChangeRFInProgress; // RF RW is still in progress.
	unsigned int	RegAddr;
#ifdef RTL8192SU_FWBCN
	short			max_vapid;
	short			intf_map;
#endif
	unsigned short	rx_schedule_index;
	unsigned short	rx_hw_index;
	unsigned char	rx_ring[MAX_RX_URB<<1];
	unsigned char   root_bcn;
	//unsigned char	mgt_poll_idx[10];
#ifdef RTL8192SU_CHECKTXHANGUP
	struct urb 			*ptx_urb;
	unsigned int		init_success;
	unsigned int		txpkt_count;
#endif
#ifdef MP_TEST
	unsigned short		multibss_sg;
	unsigned long       tmpTCR;
	unsigned char		drop_pkt;
	//unsigned long       mp_pkt_txcnt;
#endif
	unsigned char 	IC_Class;
};

#ifdef CONFIG_RTL8186_KB
typedef struct guestmac {
	unsigned char			macaddr[6];
	unsigned char			valid;
} GUESTMAC_T;
#endif


struct rtl8190_priv {
	int						drv_state;		// bit0 - init, bit1 - open/close
	struct net_device		*dev;
	struct wifi_mib 		*pmib;

	struct wlan_acl_poll	*pwlan_acl_poll;
	struct list_head		wlan_aclpolllist;	// this is for poll management
	struct list_head		wlan_acl_list;		// this is for auth checking

	DOT11_QUEUE				*pevent_queue;
#ifdef CONFIG_RTL_WAPI_SUPPORT
	DOT11_QUEUE				*wapiEvent_queue;
#endif
	DOT11_EAP_PACKET		*Eap_packet;

	struct proc_dir_entry	*proc_root;

#ifdef CONFIG_RTK_MESH
	DOT11_QUEUE2			*pathsel_queue;		// pathselection QUEUE
#ifdef _11s_TEST_MODE_
	DOT11_QUEUE2			*receiver_queue;	// pathselection QUEUE
	struct list_head		mtb_list;
#endif
#endif

#ifdef ENABLE_RTL_SKB_STATS
	atomic_t 				rtl_tx_skb_cnt;
	atomic_t				rtl_rx_skb_cnt;
#endif

	struct priv_shared_info	*pshare;		// pointer of shared info, david

#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	struct rtl8190_priv		*proot_priv;	// ptr of private structure of root interface
#endif
#ifdef UNIVERSAL_REPEATER
	struct rtl8190_priv		*pvxd_priv;		// ptr of private structure of virtual interface
#endif
#ifdef MBSSID
	struct rtl8190_priv		*pvap_priv[4];	// ptr of private structure of vap interface
	struct sk_buff			*amsdu_skb;
	unsigned int			amsdu_pkt_remain_size;
	short					vap_id;
	short					vap_init_seq;
#endif

#ifdef DFS
	struct timer_list		ch52_timer;		// timer for the blocked channel if radar detected
	struct timer_list		ch56_timer;		// timer for the blocked channel if radar detected
	struct timer_list		ch60_timer;		// timer for the blocked channel if radar detected
	struct timer_list		ch64_timer;		// timer for the blocked channel if radar detected
	unsigned int			NOP_chnl[4];	// blocked channel will be removed from available_chnl[32]
											// and placed in this list;
	unsigned int			NOP_chnl_num;	// record the number of channels in NOP_chnl[4]
#endif

#ifdef INCLUDE_WPA_PSK
	WPA_GLOBAL_INFO			*wpa_global_info;
#endif

#ifdef CHECK_HANGUP
	int						reset_hangup;
	unsigned int			reset_cnt_tx;
	unsigned int			reset_cnt_rx;
	unsigned int			reset_cnt_isr;
	unsigned int			reset_cnt_bcn;
	unsigned int			reset_cnt_rst;
	unsigned int			reset_cnt_bb;
	unsigned int			reset_cnt_cca;
#endif

#ifdef WDS
	struct net_device_stats	wds_stats[NUM_WDS];
	struct net_device		*wds_dev[NUM_WDS];
#endif
	unsigned int			auto_channel;			// 0: not auto, 1: auto select this time, 2: auto select next time
	unsigned int			auto_channel_backup;
	unsigned long			up_time;

#ifdef CONFIG_RTK_MESH
	unsigned char			RreqMAC[AODV_RREQ_TABLE_SIZE][MACADDRLEN];
	unsigned int			RreqBegin;
	unsigned int			RreqEnd;

	struct hash_table		*proxy_table, *mesh_rreq_retry_queue;
#ifdef PU_STANDARD
	struct hash_table		*proxyupdate_table;
#endif
	struct hash_table		*pathsel_table; // add by chuangch 2007.09.13
	struct mpp_tb			*pann_mpp_tb;

	struct MESH_FAKE_MIB_T	mesh_fake_mib;
	unsigned char			root_mac[MACADDRLEN];		// Tree Base root MAC

/*
	dev->priv->base_addr = 0 is wds
	dev->priv->base_addr = 1 is mesh
	We provide only one mesh device now. Although it is possible that more than one
	mesh devices bind with one physical interface simultaneously. RTL8186 shares the
	same MAC address with multiple virtual devices. Hence, the mesh data frame can't
	be handled (rx) by mesh devices correctly.
*/

	struct net_device		*mesh_dev;

#ifdef _MESH_ACL_ENABLE_
	struct mesh_acl_poll	*pmesh_acl_poll;
	struct list_head		mesh_aclpolllist;	// this is for poll management
	struct list_head		mesh_acl_list;		// this is for auth checking
#endif

#endif // CONFIG_RTK_MESH

#ifdef MESH_USE_METRICOP
	UINT32                          toMeshMetricAuto; // timeout, check mesh_fake_mib for further description
#endif

#if defined(CONFIG_RTL_WAPI_SUPPORT)
	unsigned char			*wapiCachedBuf;
	unsigned char			wapiCachedLen;
	uint8				wapiNMK[WAPI_KEY_LEN];
	unsigned char			txMCast[WAPI_PN_LEN];
	unsigned char			rxMCast[WAPI_PN_LEN];
	unsigned char			keyNotify[WAPI_PN_LEN];
	uint8				wapiMCastKeyId:1;
	uint8				wapiMCastKeyUpdateAllDone:1;
	uint8				wapiMCastKeyUpdate:1;
	wapiKey				wapiMCastKey[2];
	unsigned long			wapiMCastKeyUpdateCnt;
#endif

	struct usb_device		*udev;
	struct urb				**rx_urb;
	struct semaphore		wx_sem;
	struct tasklet_struct	irq_rx_tasklet;

	short					tx_urb_index;

	atomic_t irt_counter;//count for irq_rx_tasklet
	struct sk_buff			**pp_rxskb;
	unsigned int			*oldaddr;

#ifdef RTL8192SU_EFUSE
	unsigned char			EepromOrEfuse;
	unsigned char			AutoloadFailFlag;
	unsigned char			EfuseMap[2][HWSET_MAX_SIZE_92S];
	unsigned short			EfuseUsedBytes;
#endif
	struct timer_list	poll_usb_timer;
#ifdef SW_LED
	struct usb_ctrlrequest *led_dr;
	u8 *led_data;
	struct urb *led_urb;
#endif


	/*********************************************************
	 * from here on, data will be clear in rtl8190_init_sw() *
	 *********************************************************/

	struct net_device_stats	net_stats;
	struct extra_stats		ext_stats;

	struct timer_list		frag_to_filter;
	unsigned int			frag_to;

	struct timer_list		expire_timer;
	unsigned int			auth_to;	// second. time to expire in authenticating
	unsigned int			assoc_to;	// second. time to expire before associating
	unsigned int			expire_to;	// second. time to expire after associating

	struct timer_list		ss_timer;	//ss_timer: site_survey timer
	struct ss_res			site_survey;
	unsigned int			site_survey_times;
	unsigned char			ss_ssid[32];
	unsigned int			ss_ssidlen;
	unsigned int			ss_req_ongoing;

#ifdef RTL8192SU_SW_BEACON
	struct timer_list		beacon_timer;
#endif

	unsigned int			auth_seq;
	unsigned char			chg_txt[128];

	struct list_head		stat_hash[NUM_STAT];
	struct list_head		asoc_list;
	struct list_head		auth_list;
	struct list_head		defrag_list;
	struct list_head		sleep_list;
	struct list_head		wakeup_list;
	struct list_head		addRAtid_list;	// to avoid add RAtid fail

#ifdef WIFI_SIMPLE_CONFIG
	unsigned int			beaconbuf[MAX_WSC_IE_LEN];
	struct wsc_probe_request_info wsc_sta[MAX_WSC_PROBE_STA];
	unsigned int 			wps_issue_join_req;
	unsigned int			recover_join_req;
	struct bss_desc			dot11Bss_original;
	int						hidden_ap_mib_backup;
	unsigned	char		*pbeacon_ssid;
#else
	unsigned int			beaconbuf[128];
#endif

	struct ht_cap_elmt		ht_cap_buf;
	unsigned int			ht_cap_len;
	struct ht_info_elmt		ht_ie_buf;
	unsigned int			ht_ie_len;
	unsigned int			ht_legacy_obss_to;
	unsigned int			ht_legacy_sta_num;
	unsigned int			ht_protection;
	unsigned int			dc_th_current_state;

	// to avoid add RAtid fail
	struct timer_list		add_RATid_timer;

	// to avoid client mode assoc fail with TKIP/AES

	unsigned short			timoffset;
	unsigned char			dtimcount;
	unsigned char			pkt_in_dtimQ;
#ifdef RTL8192SU_FWBCN
	unsigned char			update_bcn;
	unsigned char			bitmap_tmp[(NUM_STAT>>3)+1];
#endif

	struct stat_info_cache	stainfo_cache;

	struct list_head		rx_datalist;
	struct list_head		rx_mgtlist;
	struct list_head		rx_ctrllist;

	int						assoc_num;		// association client number
	int						link_status;

	unsigned short			*pBeaconCapability;		// ptr of capability field in beacon buf
	unsigned char			*pBeaconErp;			// ptr of ERP field in beacon buf

	unsigned int			available_chnl[76];		// all available channel we can use
	unsigned int			available_chnl_num;		// record the number
	//unsigned int			oper_band;				// remember the operating bands
	unsigned int			supported_rates;
	unsigned int			basic_rates;

	// for MIC check
	struct timer_list		MIC_check_timer;
	struct timer_list		assoc_reject_timer;
	unsigned int			MIC_timer_on;
	unsigned int			assoc_reject_on;

#ifdef CLIENT_MODE
	struct timer_list		reauth_timer;
	unsigned int			reauth_count;

	struct timer_list		reassoc_timer;
	unsigned int			reassoc_count;

	struct timer_list		idle_timer;

	unsigned int			join_req_ongoing;
	int						join_index;
	unsigned long			jiffies_pre;
	unsigned int			ibss_tx_beacon;
	unsigned int			rxBeaconNumInPeriod;
	unsigned int			rxBeaconCntArray[ROAMING_DECISION_PERIOD_ARRAY];
	unsigned int			rxBeaconCntArrayIdx;	// current index of array
	unsigned int			rxBeaconCntArrayWindow;	// Is slide windows full
	unsigned int			rxBeaconPercentage;
	unsigned int			rxDataNumInPeriod;
	unsigned int			rxDataCntArray[ROAMING_DECISION_PERIOD_ARRAY];
	unsigned int			rxMlcstDataNumInPeriod;
	unsigned int			rxDataNumInPeriod_pre;
	unsigned int			rxMlcstDataNumInPeriod_pre;
	unsigned int			dual_band;
	unsigned int			supported_rates_alt;
	unsigned int			basic_rates_alt;
#endif

	int						authModeToggle;		// auth mode toggle referred when auto-auth mode is set under client mode, david
	int						authModeRetry;		// auth mode retry sequence when auto-auth mode is set under client mode

	int						acLogCountdown;		// log count-down time
	struct ac_log_info		acLog[MAX_AC_LOG];

	struct tx_desc			*amsdu_first_desc;

#ifdef BUFFER_TX_AMPDU
	struct tx_desc			*ampdu_first_desc;
#endif

#ifdef SEMI_QOS
	unsigned char			BE_switch_to_VI;
	unsigned int			BE_wifi_EDCA_enhance;
#endif

#ifdef WIFI_11N_2040_COEXIST
	struct obss_scan_para_elmt		obss_scan_para_buf;
	unsigned int					obss_scan_para_len;
	unsigned int					bg_ap_timeout;
	unsigned int					force_20_sta;
	unsigned int					switch_20_sta;
#endif

	/*********************************************************************
	 * from here on till EE_Cached will be backup during hang up reset   *
	 *********************************************************************/
#ifdef CLIENT_MODE
	unsigned int			join_res;
	unsigned int			beacon_period;
	unsigned short			aid;
#ifdef RTK_BR_EXT
	unsigned int			macclone_completed;
	struct nat25_network_db_entry	*nethash[NAT25_HASH_SIZE];
	int						pppoe_connection_in_progress;
	unsigned char			pppoe_addr[MACADDRLEN];
	unsigned char			scdb_mac[MACADDRLEN];
	unsigned char			scdb_ip[4];
	struct nat25_network_db_entry	*scdb_entry;
	unsigned char			br_mac[MACADDRLEN];
	unsigned char			br_ip[4];
#endif
#endif

	// cache of EEPROM
	unsigned int			EE_Cached;
	unsigned int			EE_AutoloadFail;
	unsigned int			EE_ID;
	unsigned int			EE_Version;
	unsigned int			EE_RFTypeID;
	unsigned int			EE_AnaParm;
	unsigned int			EE_AnaParm2;
	unsigned char			EE_Mac[6];
	unsigned char			EE_TxPower_CCK[MAX_CCK_CHANNEL_NUM];
	unsigned char			EE_TxPower_OFDM[MAX_OFDM_CHANNEL_NUM];
	unsigned char			EE_CrystalCap;		// added for initial 8192

#ifdef CONFIG_RTL865X
	unsigned char			rtl8650extPortNum;	// loopback/extension port used
	unsigned int			rtl8650linkNum[16];	// to save the link ID allocated from glue interface
#endif

#ifdef MICERR_TEST
	unsigned int			micerr_flag;
#endif

#ifdef DFS
	struct timer_list		DFS_timer;			// 10 ms timer for radar detection
	struct timer_list		ch_avail_chk_timer;	// 60 secs timer for channel availability check
	unsigned int			rs1[4];				// rs1[i] records the radar pulses during the past 10 ms; radar type 18pulses/26ms
	unsigned int			rs1_index;			// index to rs1[i]
	unsigned int			rs2[8];				// rs2[i] records the radar pulses during the past 10 ms; radar type 18pulses/70ms
	unsigned int			rs2_index;			// index to rs2[i]
	unsigned int			TotalTxRx[10];		// TotalTxRx[i] : Total bytes of Tx+Rx during the past 1 sec
	unsigned int			PreTxBytes;			// store the previous pnet_stats->tx_bytes
	unsigned int			PreRxBytes;			// store the previous pnet_stats->rx_bytes
	unsigned int			TotalTxRx_index;
#endif

#ifdef A4_TUNNEL
	struct list_head		a4tnl_table[NUM_STAT];
#endif

#ifdef CONFIG_RTL865X
	unsigned int			WLAN_VLAN_ID;
#endif

#ifdef __ZINWELL__
	int						indicate_auth;		// send authentication event to daemon
	int						keep_encrypt_key;	// keep encrypt key when release_stainfo
#endif

#ifdef GBWC
	struct timer_list		GBWC_timer;
	struct pkt_queue		GBWC_tx_queue;
	struct pkt_queue		GBWC_rx_queue;
	unsigned int			GBWC_tx_count;
	unsigned int			GBWC_rx_count;
	unsigned int			GBWC_consuming_Q;
#endif

#ifdef SUPPORT_SNMP_MIB
	struct mib_snmp			snmp_mib;
#endif

#ifdef SW_MCAST
	struct pkt_queue		dz_queue;	// Queue for multicast pkts when there is sleeping sta
	unsigned char			release_mcast;
	unsigned char			queued_mcast;
#endif

#ifndef USE_RTL8186_SDK
	unsigned int			amsdu_first_dma_desc;
	unsigned int			ampdu_first_dma_desc;
#endif
	int						amsdu_len;
	int						ampdu_len;

#ifdef CHECK_RX_HANGUP
	unsigned long			rx_packets_pre1;
	unsigned long			rx_packets_pre2;
	unsigned int 			rx_start_monitor_running;
#endif

#ifdef USB_PKT_RATE_CTRL_SUPPORT
	unsigned int			change_toggle;
	unsigned int			pre_pkt_cnt;
	unsigned int			pkt_nsec_diff;
	unsigned int			poll_usb_cnt;
	unsigned int			auto_rate_mask;
#endif

#ifdef CONFIG_RTL8186_KB
	GUESTMAC_T				guestMac[MAX_GUEST_NUM];
#endif

#ifdef CONFIG_RTK_VLAN_SUPPORT
	int						global_vlan_enable;
	struct vlan_info	vlan_setting;
#endif

#ifdef CONFIG_RTK_MESH
	struct net_device_stats	mesh_stats;

#ifdef _11s_TEST_MODE_
	char 					rvTestPacket[3000];
#endif

	UINT8					mesh_Version;
	// WLAN Mesh Capability
	INT16					mesh_PeerCAP_cap;		// peer capability-Cap number (Signed!)
	UINT8					mesh_PeerCAP_flags;		// peer capability-flags
	UINT8					mesh_PowerSaveCAP;		// Power Save capability
	UINT8					mesh_SyncCAP;			// Synchronization capability
	UINT8					mesh_MDA_CAP;			// MDA capability
	UINT32					mesh_ChannelPrecedence;	// Channel Precedence

	UINT8					mesh_HeaderFlags;		// mesh header in mesh flags field

	// MKD domain element [MKDDIE]
	UINT8					mesh_MKD_DomainID[6];
	UINT8					mesh_MKDDIE_SecurityConfiguration;

	// EMSA Handshake element [EMSAIE]
	UINT8					mesh_EMSAIE_ANonce[32];
	UINT8					mesh_EMSAIE_SNonce[32];
	UINT8					mesh_EMSAIE_MA_ID[6];
	UINT16					mesh_EMSAIE_MIC_Control;
	UINT8					mesh_EMSAIE_MIC[16];

#ifdef MESH_BOOTSEQ_AUTH
	struct timer_list		mesh_auth_timer;		///< for unestablish (And establish to unestablish) MP mesh_auth_hdr

	// mesh_auth_hdr:
	//  It is a list structure, only stores unAuth MP entry
	//  Each entry is a pointer pointing to an entry in "stat_info->mesh_mp_ptr"
	//  and removed by successful "Auth" or "Expired"
	struct list_head		mesh_auth_hdr;
#endif

	struct timer_list		mesh_peer_link_timer;	///< for unestablish (And establish to unestablish) MP mesh_unEstablish_hdr

	// mesh_unEstablish_hdr:
	//  It is a list structure, only stores unEstablish (or Establish -> unEstablish [MP_HOLDING])MP entry
	//  Each entry is a pointer pointing to an entry in "stat_info->mesh_mp_ptr"
	//  and removed by successful "Peer link setup" or "Expired"
	struct list_head		mesh_unEstablish_hdr;

	// mesh_mp_hdr:
	//  It is a list of MP/MAP/MPP who has already passed "Peer link setup"
	//  Each entry is a pointer pointing to an entry in "stat_info->mesh_mp_ptr"
	//  Every entry is inserted by "successful peer link setup"
	//  and removed by "Expired"
	struct list_head		mesh_mp_hdr;

	struct MESH_Profile		mesh_profile[1];	// Configure by WEB in the future, Maybe delete, Preservation before delete


#ifdef MESH_BOOTSEQ_STRESS_TEST
	unsigned long			mesh_stressTestCounter;
#endif	// MESH_BOOTSEQ_STRESS_TEST

	// Throughput statistics (sounder)
	unsigned int			mesh_log;
	unsigned long			log_time;

#ifdef _MESH_ACL_ENABLE_
	unsigned char			meshAclCacheAddr[MACADDRLEN];
	unsigned char			meshAclCacheMode;
#endif
#endif // CONFIG_RTK_MESH

#ifdef SUPPORT_TX_MCAST2UNI
	int 							stop_tx_mcast2uni;
#endif

	atomic_t				tx_pending[0x10];//UART_PRIORITY+1
};

struct rtl8190_chr_priv {
	unsigned int			major;
	unsigned int			minor;
	struct rtl8190_priv*	wlan_priv;
	struct fasync_struct*	asoc_fasync;	// asynch notification
};


// Macros
#define GET_MIB(priv)		(priv->pmib)
#define GET_HW(priv)		(priv->pshare->phw)

#define AP_BSSRATE			((GET_MIB(priv))->dot11StationConfigEntry.dot11OperationalRateSet)
#define AP_BSSRATE_LEN		((GET_MIB(priv))->dot11StationConfigEntry.dot11OperationalRateSetLen)

#define STAT_OPRATE			(pstat->bssrateset)
#define STAT_OPRATE_LEN		(pstat->bssratelen)

#define BSSID			((GET_MIB(priv))->dot11StationConfigEntry.dot11Bssid)

#define SSID			((GET_MIB(priv))->dot11StationConfigEntry.dot11DesiredSSID)

#define SSID_LEN		((GET_MIB(priv))->dot11StationConfigEntry.dot11DesiredSSIDLen)

#define OPMODE			((GET_MIB(priv))->dot11OperationEntry.opmode)

#define HIDDEN_AP		((GET_MIB(priv))->dot11OperationEntry.hiddenAP)

#define RTSTHRSLD		((GET_MIB(priv))->dot11OperationEntry.dot11RTSThreshold)

#define FRAGTHRSLD		((GET_MIB(priv))->dot11OperationEntry.dot11FragmentationThreshold)

#define EXPIRETIME		((GET_MIB(priv))->dot11OperationEntry.expiretime)

#define LED_TYPE		((GET_MIB(priv))->dot11OperationEntry.ledtype)

#ifdef RTL8190_SWGPIO_LED
#define LED_ROUTE		((GET_MIB(priv))->dot11OperationEntry.ledroute)
#endif

#define IAPP_ENABLE		((GET_MIB(priv))->dot11OperationEntry.iapp_enable)

#define SWCRYPTO		((GET_MIB(priv))->dot11StationConfigEntry.dot11swcrypto)

#define IEEE8021X_FUN	((GET_MIB(priv))->dot118021xAuthEntry.dot118021xAlgrthm)

#define SHORTPREAMBLE	((GET_MIB(priv))->dot11RFEntry.shortpreamble)

#define SSID2SCAN		((GET_MIB(priv))->dot11StationConfigEntry.dot11SSIDtoScan)

#define SSID2SCAN_LEN	((GET_MIB(priv))->dot11StationConfigEntry.dot11SSIDtoScanLen)

#define RX_BUF_LEN		RX_ALLOC_SIZE


#ifdef SEMI_QOS
#define QOS_ENABLE 		((GET_MIB(priv))->dot11QosEntry.dot11QosEnable)

#define APSD_ENABLE 	((GET_MIB(priv))->dot11QosEntry.dot11QosAPSD)

#define GET_WMM_IE		((GET_MIB(priv))->dot11QosEntry.WMM_IE)

#define GET_WMM_PARA_IE		((GET_MIB(priv))->dot11QosEntry.WMM_PARA_IE)

#define GET_EDCA_PARA_UPDATE 	((GET_MIB(priv))->dot11QosEntry.EDCAparaUpdateCount)

#define GET_STA_AC_BE_PARA	((GET_MIB(priv))->dot11QosEntry.STA_AC_BE_paraRecord)

#define GET_STA_AC_BK_PARA	((GET_MIB(priv))->dot11QosEntry.STA_AC_BK_paraRecord)

#define GET_STA_AC_VI_PARA	((GET_MIB(priv))->dot11QosEntry.STA_AC_VI_paraRecord)

#define GET_STA_AC_VO_PARA	((GET_MIB(priv))->dot11QosEntry.STA_AC_VO_paraRecord)
#endif

#define AMPDU_ENABLE	((GET_MIB(priv))->dot11nConfigEntry.dot11nAMPDU)
#define AMSDU_ENABLE	((GET_MIB(priv))->dot11nConfigEntry.dot11nAMSDU)

#define TSF_LESS(a, b)	(((a - b) & 0x80000000) != 0)
#define TSF_DIFF(a, b)	((a >= b)? (a - b):(0xffffffff - b + a + 1))

#define GET_GROUP_MIC_KEYLEN	((GET_MIB(priv))->dot11GroupKeysTable.dot11EncryptKey.dot11TMicKeyLen)
#define GET_GROUP_TKIP_MIC1_KEY	((GET_MIB(priv))->dot11GroupKeysTable.dot11EncryptKey.dot11TMicKey1.skey)
#define GET_GROUP_TKIP_MIC2_KEY	((GET_MIB(priv))->dot11GroupKeysTable.dot11EncryptKey.dot11TMicKey2.skey)

#define GET_UNICAST_MIC_KEYLEN		(pstat->dot11KeyMapping.dot11EncryptKey.dot11TMicKeyLen)
#define GET_UNICAST_TKIP_MIC1_KEY	(pstat->dot11KeyMapping.dot11EncryptKey.dot11TMicKey1.skey)
#define GET_UNICAST_TKIP_MIC2_KEY	(pstat->dot11KeyMapping.dot11EncryptKey.dot11TMicKey2.skey)

#define GET_GROUP_ENCRYP_KEY		((GET_MIB(priv))->dot11GroupKeysTable.dot11EncryptKey.dot11TTKey.skey)
#define GET_UNICAST_ENCRYP_KEY		(pstat->dot11KeyMapping.dot11EncryptKey.dot11TTKey.skey)

#define GET_GROUP_ENCRYP_KEYLEN			((GET_MIB(priv))->dot11GroupKeysTable.dot11EncryptKey.dot11TTKeyLen)
#define GET_UNICAST_ENCRYP_KEYLEN		(pstat->dot11KeyMapping.dot11EncryptKey.dot11TTKeyLen)

#define GET_MY_HWADDR		((GET_MIB(priv))->dot11OperationEntry.hwaddr)

#define GET_GROUP_ENCRYP_PN			(&((GET_MIB(priv))->dot11GroupKeysTable.dot11EncryptKey.dot11TXPN48))
#define GET_UNICAST_ENCRYP_PN		(&(pstat->dot11KeyMapping.dot11EncryptKey.dot11TXPN48))

#define SET_SHORTSLOT_IN_BEACON_CAP		\
	do {	\
		if (priv->pBeaconCapability != NULL)	\
			*priv->pBeaconCapability |= cpu_to_le16(BIT(10));	\
	} while(0)

#define RESET_SHORTSLOT_IN_BEACON_CAP	\
	do {	\
		if (priv->pBeaconCapability != NULL)	\
			*priv->pBeaconCapability &= ~cpu_to_le16(BIT(10));	\
	} while(0)

#define IS_DRV_OPEN(priv) ((priv==NULL) ? 0 : ((priv->drv_state & DRV_STATE_OPEN) ? 1 : 0))
#define IS_DRV_CLOSE(priv) ((priv==NULL) ? 0 : ((priv->drv_state & DRV_STATE_CLOSE) ? 1 : 0))

#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
#define GET_ROOT_PRIV(priv)			(priv->proot_priv)
#define IS_ROOT_INTERFACE(priv)	 	(GET_ROOT_PRIV(priv) ? 0 : 1)
#define GET_ROOT(priv)				((priv->proot_priv) ? priv->proot_priv : priv)
#else
#define GET_ROOT(priv)		(priv)
#endif
#ifdef UNIVERSAL_REPEATER
#define GET_VXD_PRIV(priv)			(priv->pvxd_priv)
#ifdef MBSSID
#define IS_VXD_INTERFACE(priv)		((GET_ROOT_PRIV(priv) ? 1 : 0) && (priv->vap_id < 0))
#else
#define IS_VXD_INTERFACE(priv)		(GET_ROOT_PRIV(priv) ? 1 : 0)
#endif
#endif // UNIVERSAL_REPEATER
#ifdef MBSSID
#define IS_VAP_INTERFACE(priv)		(!IS_ROOT_INTERFACE(priv) && (priv->vap_id >= 0))
#endif

#define MANAGE_QUE_NUM		MGNT_QUEUE

#ifdef DFS
#define DFS_TO					(priv->pmib->dot11DFSEntry.DFS_timeout)
#define NONE_OCCUPANCY_PERIOD	(priv->pmib->dot11DFSEntry.NOP_timeout)
#endif

#ifdef _DEBUG_RTL8190_
#define ASSERT(expr) \
        if(!(expr)) {					\
  			printk( "\033[33;41m%s:%d: assert(%s)\033[m\n",	\
	        __FILE__,__LINE__,#expr);		\
        }
#else
	#define ASSERT(expr)
#endif

#ifdef RTL8190_SWGPIO_LED
/* =====================================================================
		LED route configuration:

			Currently, LOW 10 bits of this MIB are used.

			+---+---+---+---+---+---+---+---+---+---+
			| E1| H1|  Route 1  | E0| H0|  Route 0  |
			+---+---+---+---+---+---+---+---+---+---+

			E0		: Indicates if the field route 0 is valid or not.
			E1		: Indicates if the field route 1 is valid or not.
			H0		: Indicate the GPIO indicated by route 0 is Active HIGH or Active LOW. ( 0: Active LOW, 1: Active HIGH)
			H1		: Indicate the GPIO indicated by route 1 is Active HIGH or Active LOW. ( 0: Active LOW, 1: Active HIGH)
			Route0	: The GPIO number (0~6) which used by LED0. Only used when E0=0b'1
			Route1	: The GPIO number (0~6) which used by LED1. Only used when E1=0b'1

			Unused bits	: reserved for further extension, must set to 0.

			Currently RTL8185 AP driver supports LED0/LED1, and RTL8185 has 7 GPIOs in it.
			So we need a routing-mechanism to decide what GPIO is used for LED0/LED1.
			The bit-field pairs {E0, H0, Route0}, {E1, H0, Route1} is used to set it.
			Ex.
				One customer only use GPIO0 for LED1 (Active LOW) and don't need LED0,
				he can set the route being:
				----------------------------------
				E0 = 0
				E1 = 1
				H0 = 0 ( Driver would ignore it )
				H1 = 0  ( Driver would ignore it )
				Route 0 = 0 ( Driver would ignore it )
				Route 1 = 0 ( GPIO0 )

				ledroute = 0x10 << 5;		: LED1 -Active LOW, GPIO0
				ledroute |= 0;				: LED0 -Disabled
				----------------------------------
     ===================================================================== */
#define	SWLED_GPIORT_CNT			2					/* totally we have max 3 GPIOs reserved for LED route usage */
#define	SWLED_GPIORT_RTBITMSK		0x07				/* bit mask of routing field = 0b'111 */
#define	SWLED_GPIORT_HLMSK			0x08				/* bit mask of Active high/low field = 0b'1000 */
#define	SWLED_GPIORT_ENABLEMSK		0x10				/* bit mask of enable filed = 0b'10000 */
#define	SWLED_GPIORT_ITEMBITCNT		5					/* total bit count of each item */
#define	SWLED_GPIORT_ITEMBITMASK	(	SWLED_GPIORT_RTBITMSK |\
										SWLED_GPIORT_HLMSK |\
										SWLED_GPIORT_ENABLEMSK)	/* bit mask of each item */
#define	SWLED_GPIORT_ITEM(ledroute, x)	(((ledroute) >> ((x)*SWLED_GPIORT_ITEMBITCNT)) & SWLED_GPIORT_ITEMBITMASK)
#endif // RTL8190_SWGPIO_LED

#ifdef SUPPORT_SNMP_MIB
#define SNMP_MIB(f)					(priv->snmp_mib.f)
#define SNMP_MIB_ASSIGN(f,v)		(SNMP_MIB(f)=v)
#define SNMP_MIB_COPY(f,v,len)		(memcpy(&SNMP_MIB(f), v, len))
#define SNMP_MIB_INC(f,v)			(SNMP_MIB(f)+=v)
#define SNMP_MIB_DEC(f,v)			(SNMP_MIB(f)-=v)

#else

#define SNMP_MIB(f)
#define SNMP_MIB_ASSIGN(f,v)
#define SNMP_MIB_COPY(f,v,len)
#define SNMP_MIB_INC(f,v)
#define SNMP_MIB_DEC(f,v)
#endif //SUPPORT_SNMP_MIB


#ifdef USB_PKT_RATE_CTRL_SUPPORT
	typedef unsigned int (*usb_pktCnt_fn)(void);
	typedef unsigned int (*register_usb_pkt_cnt_fn)(void *);
	extern register_usb_pkt_cnt_fn register_usb_hook;
#endif


#ifdef CONFIG_RTK_VLAN_SUPPORT
extern int  rx_vlan_process(struct net_device *dev, struct VlanConfig *info, struct sk_buff *skb);
extern int  tx_vlan_process(struct net_device *dev, struct VlanConfig *info, struct sk_buff *skb, int wlan_pri);
#endif

typedef enum{
	BULK_PRIORITY = 0x01,
	//RSVD0,
	//RSVD1,
	LOW_PRIORITY,
	NORM_PRIORITY, 
	VO_PRIORITY,
	VI_PRIORITY, //0x05
	BE_PRIORITY,
	BK_PRIORITY,
	RSVD2,
	RSVD3,
	BEACON_PRIORITY, //0x0A
	HIGH_PRIORITY,
	MANAGE_PRIORITY,
	TxCmd_Bcn_Mgt_Hiht_PRIORITY,
	RSVD5,
	UART_PRIORITY //0x0F
} priority_t;

struct usb_priv
{
	struct net_device *netdev;
	struct usb_device *dev;
	unsigned int pipe;
	void *transfer_buffer;
	int buffer_length;
	int index;
};

#endif // _8190N_H_

