#ifndef __INC_WAPI_H
#define __INC_WAPI_H

#include <linux/kernel.h>
#include <linux/list.h>

/* WAPI trace debug */
extern u32 wapi_debug_component;

#define WAPI_TRACE(component, x, args...) \
do { if(wapi_debug_component & (component)) \
	printk(KERN_DEBUG "WAPI" ":" x "" , \
	       ##args);\
}while(0);

#define WAPI_DATA(component, x, buf, len) \
do { if(wapi_debug_component & (component)){ \
	printk("%s:\n", x);\
	dump_buf((buf), (len));}\
}while(0);

enum WAPI_DEBUG {
	WAPI_INIT				=1,
	WAPI_API				= 2,	 
	WAPI_TX				= 4,	 
	WAPI_RX				= 8,
	WAPI_ERR		        	= 1<<31
};

#define			WAPI_MAX_BKID_NUM				64
#define			WAPI_MAX_STAINFO_NUM			64
typedef struct  _RT_WAPI_BKID
{
	struct list_head	list;
	u8				bkid[16];
}RT_WAPI_BKID,*PRT_WAPI_BKID;

typedef struct  _RT_WAPI_KEY
{
	u8			dataKey[16];
	u8			micKey[16];
	u8			keyId;
	bool			bSet;
	bool             bTxEnable;
}RT_WAPI_KEY,*PRT_WAPI_KEY;

typedef enum _RT_WAPI_PACKET_TYPE
{
	WAPI_NONE = 0,
	WAPI_PREAUTHENTICATE=1,
	WAPI_STAKEY_REQUEST=2,
	WAPI_AUTHENTICATE_ACTIVE=3,
	WAPI_ACCESS_AUTHENTICATE_REQUEST=4,
	WAPI_ACCESS_AUTHENTICATE_RESPONSE=5,
	WAPI_CERTIFICATE_AUTHENTICATE_REQUEST=6,
	WAPI_CERTIFICATE_AUTHENTICATE_RESPONSE=7,
	WAPI_USK_REQUEST=8,
	WAPI_USK_RESPONSE=9,
	WAPI_USK_CONFIRM=10,
	WAPI_MSK_NOTIFICATION=11,
	WAPI_MSK_RESPONSE=12
}RT_WAPI_PACKET_TYPE;

typedef struct _RT_CACHE_INFO {
	u8	cache_buffer[2000];
	u16	cache_buffer_len;
	u8 saddr[6];
	bool bAuthenticator;
	u16	recvSeq;
	u8	lastFragNum;
	struct list_head list;
}RT_CACHE_INFO, *PRT_CACHE_INFO;

typedef struct	_RT_WAPI_STA_INFO
{
	struct list_head		list;
	u8					PeerMacAddr[6];	
	RT_WAPI_KEY		      wapiUsk;
	RT_WAPI_KEY		      wapiUskUpdate;
	RT_WAPI_KEY		      wapiMsk;
	RT_WAPI_KEY		      wapiMskUpdate;	
	u8					lastRxUnicastPN[16];
	u8					lastTxUnicastPN[16];
	u8					lastRxMulticastPN[16];
	u8					lastRxUnicastPNBEQueue[16];
	u8					lastRxUnicastPNBKQueue[16];
	u8					lastRxUnicastPNVIQueue[16];
	u8					lastRxUnicastPNVOQueue[16];
	bool					bSetkeyOk;
	bool					bAuthenticateInProgress;
	bool					bAuthenticatorInUpdata;
}RT_WAPI_STA_INFO,*PRT_WAPI_STA_INFO;

typedef struct _RT_WAPI_T
{
   	u8		assoReqWapiIE[256];
	u8		assoReqWapiIELength;
	u8		assoRspWapiIE[256];
	u8		assoRspWapiIELength;
	u8		sendbeaconWapiIE[256];
	u8		sendbeaconWapiIELength;
	RT_WAPI_BKID		wapiBKID[WAPI_MAX_BKID_NUM];
	struct list_head		wapiBKIDIdleList;
	struct list_head  		wapiBKIDStoreList;
	RT_WAPI_KEY		      wapiTxMsk;

	u8				wapiDestMacAddr[6];
	bool				bAuthenticator;
	u8				lastTxMulticastPN[16];
	RT_WAPI_STA_INFO	wapiSta[WAPI_MAX_STAINFO_NUM];
	struct list_head		wapiSTAIdleList;
	struct list_head		wapiSTAUsedList;
	bool				bWapiEnable;
	bool				bUpdateUsk;
	bool				bUpdateMsk;

	u8				wapiIE[256];
	u8				wapiIELength;
	bool				bWapiNotSetEncMacHeader;
	bool				bWapiPSK;
	bool				bFirstAuthentiateInProgress;
	u16				wapiSeqnumAndFragNum;
	int extra_prefix_len;
	int extra_postfix_len;
}RT_WAPI_T,*PRT_WAPI_T;

typedef struct _WLAN_HEADER_WAPI_EXTENSION
{
    u8      KeyIdx;
    u8      Reserved;
    u8      PN[16];
} WLAN_HEADER_WAPI_EXTENSION, *PWLAN_HEADER_WAPI_EXTENSION;

#endif
