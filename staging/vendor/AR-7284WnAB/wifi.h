/*
 *      Header file defines some WiFi (802.11) standard related
 *
 *      $Id: wifi.h,v 1.7 2009/08/06 11:41:29 yachang Exp $
 */

#ifndef _WIFI_H_
#define _WIFI_H_

#ifdef BIT
//#error	"BIT define occurred earlier elsewhere!\n"
#undef BIT
#endif
#define BIT(x)	(1 << (x))


#define WLAN_ETHHDR_LEN		14
#define WLAN_ETHADDR_LEN	6
#define WLAN_IEEE_OUI_LEN	3
#define WLAN_ADDR_LEN		6
#define WLAN_CRC_LEN		4
#define WLAN_BSSID_LEN		6
#define WLAN_BSS_TS_LEN		8
#define WLAN_HDR_A3_LEN		24
#define WLAN_HDR_A4_LEN		30
#define WLAN_HDR_A3_QOS_LEN	26
#define WLAN_HDR_A4_QOS_LEN	32
#define WLAN_SSID_MAXLEN	32
#define WLAN_DATA_MAXLEN	2312

#define WLAN_A3_PN_OFFSET	24
#define WLAN_A4_PN_OFFSET	30

#define WLAN_LLC_HEADER_SIZE	6

#ifdef CONFIG_RTK_MESH
// Define mesh header length, But 11s data 11s mgt frame header length different, So have two type.
#define WLAN_HDR_A4_MESH_DATA_LEN 34		// WLAN_HDR_A4_LEN + MeshHeader_Len(4 bytes)
#define WLAN_HDR_A6_MESH_DATA_LEN 46		// WLAN_HDR_A4_LEN + MeshHeader_Len(16 bytes)
#define WLAN_HDR_A4_MESH_DATA_LEN_QOS 36	// WLAN_HDR_A4_LEN + MeshHeader_Len(4 bytes) + QOS
#define WLAN_HDR_A6_MESH_DATA_LEN_QOS 48	// WLAN_HDR_A4_LEN + MeshHeader_Len(16 bytes) + QOS
// #define WLAN_HDR_A4_MESH_MGT_LEN 34		// always processed by daemon (raw socket)
#endif // CONFIG_RTK_MESH

#define WLAN_MIN_ETHFRM_LEN	60
#define WLAN_MAX_ETHFRM_LEN	1514
#define WLAN_ETHHDR_LEN		14

#define P80211CAPTURE_VERSION	0x80211001

#ifdef GREEN_HILL
#pragma pack(1)
#endif

__PACK struct wlan_ethhdr_t
{
	UINT8	daddr[WLAN_ETHADDR_LEN]		__WLAN_ATTRIB_PACK__;
	UINT8	saddr[WLAN_ETHADDR_LEN]		__WLAN_ATTRIB_PACK__;
	UINT16	type						__WLAN_ATTRIB_PACK__;
} __WLAN_ATTRIB_PACK__;

__PACK struct wlan_llc_t
{
	UINT8	dsap						__WLAN_ATTRIB_PACK__;
	UINT8	ssap						__WLAN_ATTRIB_PACK__;
	UINT8	ctl							__WLAN_ATTRIB_PACK__;
} __WLAN_ATTRIB_PACK__;

/* local snap header type */
__PACK struct wlan_snap_t
{
	UINT8	oui[WLAN_IEEE_OUI_LEN]		__WLAN_ATTRIB_PACK__;
	UINT16	type						__WLAN_ATTRIB_PACK__;
} __WLAN_ATTRIB_PACK__;

__PACK struct llc_snap {
	struct wlan_llc_t	llc_hdr;
	struct wlan_snap_t	snap_hdr;
} __WLAN_ATTRIB_PACK__;

__PACK struct ht_cap_elmt
{
	UINT16	ht_cap_info					__WLAN_ATTRIB_PACK__;
	UINT8	ampdu_para					__WLAN_ATTRIB_PACK__;
	UINT8	support_mcs[16]				__WLAN_ATTRIB_PACK__;
	UINT16	ht_ext_cap					__WLAN_ATTRIB_PACK__;
	UINT32	txbf_cap					__WLAN_ATTRIB_PACK__;
	UINT8	asel_cap					__WLAN_ATTRIB_PACK__;
} __WLAN_ATTRIB_PACK__;

__PACK struct ht_info_elmt
{
	UINT8	primary_ch					__WLAN_ATTRIB_PACK__;
	UINT8	info0						__WLAN_ATTRIB_PACK__;
	UINT16	info1						__WLAN_ATTRIB_PACK__;
	UINT16	info2						__WLAN_ATTRIB_PACK__;
	UINT8	basic_mcs[16]				__WLAN_ATTRIB_PACK__;
} __WLAN_ATTRIB_PACK__;

#ifdef WIFI_11N_2040_COEXIST
__PACK struct obss_scan_para_elmt
{
	UINT16	word0					__WLAN_ATTRIB_PACK__;
	UINT16	word1					__WLAN_ATTRIB_PACK__;
	UINT16	word2					__WLAN_ATTRIB_PACK__;
	UINT16	word3					__WLAN_ATTRIB_PACK__;
	UINT16	word4					__WLAN_ATTRIB_PACK__;
	UINT16	word5					__WLAN_ATTRIB_PACK__;
	UINT16	word6					__WLAN_ATTRIB_PACK__;
} __WLAN_ATTRIB_PACK__;
#endif

#ifdef GREEN_HILL
#pragma pack()
#endif

/**
 *	@brief Frame type value
 *  See 802.11 Table.1 , type value define by bit2 bit3
 */
enum WIFI_FRAME_TYPE {
	WIFI_MGT_TYPE  =	(0),
	WIFI_CTRL_TYPE =	(BIT(2)),
	WIFI_DATA_TYPE =	(BIT(3)),

#ifdef CONFIG_RTK_MESH
	// Hardware of 8186 doesn't support it. Confirm by David, 2007/1/5
	WIFI_EXT_TYPE  =	(BIT(2) | BIT(3))	///< 11 is 802.11S Extended Type
#endif
};

/**
 *	@brief WIFI_FRAME_SUBTYPE
 *  See 802.11 Table.1 valid type and subtype combinations
 *
 */
enum WIFI_FRAME_SUBTYPE {

	// below is for mgt frame
	WIFI_ASSOCREQ       = (0 | WIFI_MGT_TYPE),
    WIFI_ASSOCRSP       = (BIT(4) | WIFI_MGT_TYPE),
    WIFI_REASSOCREQ     = (BIT(5) | WIFI_MGT_TYPE),
    WIFI_REASSOCRSP     = (BIT(5) | BIT(4) | WIFI_MGT_TYPE),
    WIFI_PROBEREQ       = (BIT(6) | WIFI_MGT_TYPE),
    WIFI_PROBERSP       = (BIT(6) | BIT(4) | WIFI_MGT_TYPE),
    WIFI_BEACON         = (BIT(7) | WIFI_MGT_TYPE),
    WIFI_ATIM           = (BIT(7) | BIT(4) | WIFI_MGT_TYPE),
    WIFI_DISASSOC       = (BIT(7) | BIT(5) | WIFI_MGT_TYPE),
    WIFI_AUTH           = (BIT(7) | BIT(5) | BIT(4) | WIFI_MGT_TYPE),
    WIFI_DEAUTH         = (BIT(7) | BIT(6) | WIFI_MGT_TYPE),
    WIFI_WMM_ACTION		= (BIT(7) | BIT(6) | BIT(4) | WIFI_MGT_TYPE),
#ifdef CONFIG_RTK_MESH
	WIFI_MULTIHOP_ACTION 	= (BIT(7) | BIT(6) | BIT(5) |BIT(4) | WIFI_MGT_TYPE),	// (Refer: Draft 1.06, Page 8, 7.1.3.1.2, Table 7-1, 2007/08/13 by popen)
#endif

    // below is for control frame
    WIFI_BLOCKACK_REQ	= (BIT(7) | WIFI_CTRL_TYPE),
    WIFI_BLOCKACK		= (BIT(7) | BIT(4) | WIFI_CTRL_TYPE),
    WIFI_PSPOLL         = (BIT(7) | BIT(5) | WIFI_CTRL_TYPE),
    WIFI_RTS            = (BIT(7) | BIT(5) | BIT(4) | WIFI_CTRL_TYPE),
    WIFI_CTS            = (BIT(7) | BIT(6) | WIFI_CTRL_TYPE),
    WIFI_ACK            = (BIT(7) | BIT(6) | BIT(4) | WIFI_CTRL_TYPE),
    WIFI_CFEND          = (BIT(7) | BIT(6) | BIT(5) | WIFI_CTRL_TYPE),
    WIFI_CFEND_CFACK    = (BIT(7) | BIT(6) | BIT(5) | BIT(4) | WIFI_CTRL_TYPE),

    // below is for data frame
    WIFI_DATA           = (0 | WIFI_DATA_TYPE),
    WIFI_QOS_DATA       = (BIT(7) | WIFI_DATA_TYPE),
    WIFI_DATA_CFACK     = (BIT(4) | WIFI_DATA_TYPE),
    WIFI_DATA_CFPOLL    = (BIT(5) | WIFI_DATA_TYPE),
    WIFI_DATA_CFACKPOLL = (BIT(5) | BIT(4) | WIFI_DATA_TYPE),
    WIFI_DATA_NULL      = (BIT(6) | WIFI_DATA_TYPE),
    WIFI_CF_ACK         = (BIT(6) | BIT(4) | WIFI_DATA_TYPE),
    WIFI_CF_POLL        = (BIT(6) | BIT(5) | WIFI_DATA_TYPE),
    WIFI_CF_ACKPOLL     = (BIT(6) | BIT(5) | BIT(4) | WIFI_DATA_TYPE),

#ifdef CONFIG_RTK_MESH    // (CAUTION!! Below not exist in D1.06!!)
	// Because hardware of RTL8186 doen's support TYPE=11, we use BIT(7) | WIFI_DATA_TYPE to
	// simulate TYPE=11, 2007/1/8
    WIFI_11S_MESH	    = (BIT(7) | WIFI_DATA_TYPE),	// CAUTION!! Below not exist in D1.06!!
    WIFI_11S_MESH_ACTION = (BIT(5) | WIFI_11S_MESH),	///< Mesh Action
#endif

};

#ifdef CONFIG_RTK_MESH 

// mesh action field
enum CATEGORY_FIELD {
  	_CATEGORY_MESH_ACTION_ = 5,

#ifdef MESH_USE_METRICOP
        _CATEGORY_11K_ACTION_ = 5, // oops, 11k also uses '5'
#endif

	// Use these define in the future (Refer: Draft 1.06, Page 18, 7.3.1.11, Table 7-24, 2007/08/13 by popen)
	_CATEGORY_MESH_PEER_LINK_MANAGEMENT_	= 10,
	_CATEGORY_MESH_LINK_METRIC_				= 11,
	_CATEGORY_MESH_PATH_SELECTION_			= 12,
	_CATEGORY_MESH_INTERWORKING_			= 13,
	_CATEGORY_MESH_RESOURCE_COORDINATION_	= 14,
	_CATEGORY_MESH_SECURITY_ARCHITECTURE_	= 15,
};

// mesh action field
enum ACTION_FIELD {
	ACTION_FIELD_LOCAL_LINK_STATE_ANNOUNCE	= 0,
	ACTION_FIELD_PEER_LINK_DISCONNECT		= 1,
	ACTION_FIELD_RREQ		= 2,
	ACTION_FIELD_RREP		= 3,
	ACTION_FIELD_RERR		= 4,
	ACTION_FIELD_RREP_ACK	= 5,
	ACTION_FIELD_HELLO		= 9,
	ACTION_FIELD_PANN		= 17,		//add by mcinnis 20070415
	ACTION_FIELD_RANN		= 18,		//add by chuangch 20070507
	ACTION_FIELD_PU			= 19,		//add by pepsi 20080229
	ACTION_FIELD_PUC		= 20,		//add by pepsi 20080229
#ifdef MESH_USE_METRICOP
        ACTION_FILED_11K_LINKME_REQ     = 52,           // 802.11k specifies '2'
        ACTION_FILED_11K_LINKME_REP     = 53,           // 802.11k specifies '3'
#endif
};
#endif // CONFIG_RTK_MESH

/**
 *	@brief	REASON CODE
 *	16 bit field, See textbook Table.4-5.
 */
enum WIFI_REASON_CODE	{
	_RSON_RESERVED_					= 0,	// Reserved.
	_RSON_UNSPECIFIED_				= 1,	// Unspecified.
	_RSON_AUTH_NO_LONGER_VALID_		= 2,	// Auth invalid.
	_RSON_DEAUTH_STA_LEAVING_		= 3,	// STA leave BSS or ESS, DeAuth.
	_RSON_INACTIVITY_				= 4,	// Exceed idle timer, Disconnect.
	_RSON_UNABLE_HANDLE_			= 5,	// BS resoruce insufficient.
	_RSON_CLS2_						= 6,	// UnAuth STA frame or SubType incorrect.
	_RSON_CLS3_						= 7, 	// UnAuth STA frame or SubType incorrect.
	_RSON_DISAOC_STA_LEAVING_		= 8,	// STA leave BSS or ESS, DeAssoc.
	_RSON_ASOC_NOT_AUTH_			= 9,	// Request assiciate or reassociate, before authenticate
	// 10,11,12 for 802.11h
	// WPA reason
	_RSON_INVALID_IE_				= 13,
	_RSON_MIC_FAILURE_				= 14,
	_RSON_4WAY_HNDSHK_TIMEOUT_		= 15,
	_RSON_GROUP_KEY_UPDATE_TIMEOUT_	= 16,
	_RSON_DIFF_IE_					= 17,
	_RSON_MLTCST_CIPHER_NOT_VALID_	= 18,
	_RSON_UNICST_CIPHER_NOT_VALID_	= 19,
	_RSON_AKMP_NOT_VALID_			= 20,
	_RSON_UNSUPPORT_RSNE_VER_		= 21,
	_RSON_INVALID_RSNE_CAP_			= 22,
	_RSON_IEEE_802DOT1X_AUTH_FAIL_	= 23,

	//belowing are Realtek definition
	_RSON_PMK_NOT_AVAILABLE_		= 24,
/*#if defined(CONFIG_RTL_WAPI_SUPPORT)*/
	_RSON_USK_HANDSHAKE_TIMEOUT_	= 25,	/* handshake timeout for unicast session key*/
	_RSON_MSK_HANDSHAKE_TIMEOUT_	= 26,	/* handshake timeout for multicast session key*/
	_RSON_IE_NOT_CONSISTENT_			= 27,	/* IE was different between USK handshake & assocReq/probeReq/Beacon */
	_RSON_INVALID_USK_				= 28,	/* Invalid unicast key set */
	_RSON_INVALID_MSK_				= 29,	/* Invalid multicast key set */
	_RSON_INVALID_WAPI_VERSION_		= 30,	/* Invalid wapi version */
	_RSON_INVALID_WAPI_CAPABILITY_	= 31,	/* Wapi capability not support */
/*#endif*/

#ifdef CONFIG_RTK_MESH	// CAUTION!! Below undefine and name!! (Refer: Draft 1.06, Page 16, 7.3.1.7, Table 7-22, 2007/08/13 by popen)
	_RSON_MESH_LINK_CANCELLED_		= 46,
	_RSON_MESH_MAX_NEIGHBORS_		= 47,
	_RSON_MESH_CAPABILITY_POLICY_VIOLATION_			= 48,
	_RSON_MESH_CLOSE_RCVD_							= 49,
	_RSON_MESH_MAX_RETRIES_							= 50,
	_RSON_MESH_CONFIRM_TIMEOUT_						= 51,
	_RSON_MESH_SECURITY_ROLE_NEGOTIATION_DIFFERS_	= 52,
	_RSON_MESH_SECURITY_AUTHENTICATION_IMPOSSIBLE_	= 53,
	_RSON_MESH_SECURITY_FAILED_VERIFICATION_		= 54,
#endif	
};

enum WIFI_STATUS_CODE {
	_STATS_SUCCESSFUL_				= 0,	// Success.
	_STATS_FAILURE_					= 1,	// Failure.
	_STATS_CAP_FAIL_				= 10,	// Capability too wide, can't support
	_STATS_NO_ASOC_					= 11,	// Denial reassociate
	_STATS_OTHER_					= 12,	// Denial connect, not 802.11 standard.
	_STATS_NO_SUPP_ALG_				= 13,	// Authenticate algorithm not support .
	_STATS_OUT_OF_AUTH_SEQ_			= 14,	// Out of authenticate sequence number.
	_STATS_CHALLENGE_FAIL_			= 15,	// Denial authenticate, Response message fail.
	_STATS_AUTH_TIMEOUT_			= 16,	// Denial authenticate, timeout.
	_STATS_UNABLE_HANDLE_STA_		= 17,	// Denial authenticate, BS resoruce insufficient.
	_STATS_RATE_FAIL_				= 18,	// Denial authenticate, STA not support BSS request datarate.
	_STATS_REQ_DECLINED_		= 37,
/*#if defined(CONFIG_RTL_WAPI_SUPPORT)*/
	__STATS_INVALID_IE_ = 40,
	__STATS_INVALID_AKMP_ = 43,
	__STATS_CIPER_REJECT_ = 46,
	__STATS_INVALID_USK_ = 47,
	__STATS_INVALID_MSK_ = 48,
	__STATS_INVALID_WAPI_VERSION_ = 49,
	__STATS_INVALID_WAPI_CAPABILITY_ = 50,
/*#endif*/
	
#ifdef CONFIG_RTK_MESH	// CATUTION: below undefine !! (Refer: Draft 1.06, Page 17, 7.3.1.9, Table 7-23, 2007/08/13 by popen)
	_STATS_MESH_LINK_ESTABLISHED_	= 55,	//The mesh peer link has been successfully
	_STATS_MESH_LINK_CLOSED_		= 56,	// The mesh peer link has been closed completely
	_STATS_MESH_UNDEFINE1_			= 57,	// No listed Key Holder Transport type is supported.
	_STATS_MESH_UNDEFINE2_			= 58,	// The Mesh Key Holder Security Handshake message was malformed.
#endif
};

enum WIFI_REG_DOMAIN {
	DOMAIN_FCC		= 1,
	DOMAIN_IC		= 2,
	DOMAIN_ETSI		= 3,
	DOMAIN_SPAIN	= 4,
	DOMAIN_FRANCE	= 5,
	DOMAIN_MKK		= 6,
	DOMAIN_ISRAEL	= 7,
	DOMAIN_MKK1		= 8,
	DOMAIN_MKK2		= 9,
	DOMAIN_MKK3		= 10,
	DOMAIN_MAX
};

#define _TO_DS_		BIT(8)
#define _FROM_DS_	BIT(9)
#define _MORE_FRAG_	BIT(10)
#define _RETRY_		BIT(11)
#define _PWRMGT_	BIT(12)
#define _MORE_DATA_	BIT(13)
#define _PRIVACY_	BIT(14)

#define SetToDs(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_TO_DS_); \
	} while(0)

#define GetToDs(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_TO_DS_)) != 0)

#define ClearToDs(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) &= (~cpu_to_le16(_TO_DS_)); \
	} while(0)

#define SetFrDs(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_FROM_DS_); \
	} while(0)

#define GetFrDs(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_FROM_DS_)) != 0)

#define ClearFrDs(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) &= (~cpu_to_le16(_FROM_DS_)); \
	} while(0)

#define SetMFrag(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_MORE_FRAG_); \
	} while(0)

#define GetMFrag(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_MORE_FRAG_)) != 0)

#define ClearMFrag(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) &= (~cpu_to_le16(_MORE_FRAG_)); \
	} while(0)

#define SetRetry(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_RETRY_); \
	} while(0)

#define GetRetry(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_RETRY_)) != 0)

#define ClearRetry(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) &= (~cpu_to_le16(_RETRY_)); \
	} while(0)

#define SetPwrMgt(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_PWRMGT_); \
	} while(0)

#define GetPwrMgt(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_PWRMGT_)) != 0)

#define ClearPwrMgt(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) &= (~cpu_to_le16(_PWRMGT_)); \
	} while(0)

#define SetMData(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_MORE_DATA_); \
	} while(0)

#define GetMData(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_MORE_DATA_)) != 0)

#define ClearMData(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) &= (~cpu_to_le16(_MORE_DATA_)); \
	} while(0)

#define SetPrivacy(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_PRIVACY_); \
	} while(0)

#define GetPrivacy(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_PRIVACY_)) != 0)

#define ClearPrivacy(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) &= (~cpu_to_le16(_PRIVACY_)); \
	} while(0)

#define GetFrameType(pbuf)	(le16_to_cpu(*(unsigned short *)(pbuf)) & (BIT(3) | BIT(2)))

#define SetFrameType(pbuf,type)	\
	do { 	\
		*(unsigned short *)(pbuf) &= __constant_cpu_to_le16(~(BIT(3) | BIT(2))); \
		*(unsigned short *)(pbuf) |= __constant_cpu_to_le16(type); \
	} while(0)

#define GetFrameSubType(pbuf)	(cpu_to_le16(*(unsigned short *)(pbuf)) & (BIT(7) | BIT(6) | BIT(5) | BIT(4) | BIT(3) | BIT(2)))

#define SetFrameSubType(pbuf,type) \
	do {    \
		*(unsigned short *)(pbuf) &= cpu_to_le16(~(BIT(7) | BIT(6) | BIT(5) | BIT(4) | BIT(3) | BIT(2))); \
		*(unsigned short *)(pbuf) |= cpu_to_le16(type); \
	} while(0)

#define GetSequence(pbuf)	(cpu_to_le16(*(unsigned short *)((unsigned int)(pbuf) + 22)) >> 4)

#define GetFragNum(pbuf)	(cpu_to_le16(*(unsigned short *)((unsigned int)(pbuf) + 22)) & 0x0f)

#define GetTupleCache(pbuf)	(cpu_to_le16(*(unsigned short *)((unsigned int)(pbuf) + 22)))

#define SetFragNum(pbuf, num) \
	do {    \
		*(unsigned short *)((unsigned int)(pbuf) + 22) = \
			((*(unsigned short *)((unsigned int)(pbuf) + 22)) & le16_to_cpu(~(0x000f))) | \
			cpu_to_le16(0x0f & (num));     \
	} while(0)

#define SetSeqNum(pbuf, num) \
	do {    \
		*(unsigned short *)((unsigned int)(pbuf) + 22) = \
			((*(unsigned short *)((unsigned int)(pbuf) + 22)) & le16_to_cpu((unsigned short)~0xfff0)) | \
			le16_to_cpu((unsigned short)(0xfff0 & (num << 4))); \
	} while(0)

#define SetDuration(pbuf, dur) \
	do {    \
		*(unsigned short *)((unsigned int)(pbuf) + 2) |= cpu_to_le16(0xffff & (dur)); \
	} while(0)

#define GetAid(pbuf)	(cpu_to_le16(*(unsigned short *)((unsigned int)(pbuf) + 2)) & 0x3fff)

#define GetTid(pbuf)	(cpu_to_le16(*(unsigned short *)((unsigned int)(pbuf) + (((GetToDs(pbuf)<<1)|GetFrDs(pbuf))==3?30:24))) & 0x000f)

//SEMI_QOS
#define GetQOSackPolicy(pbuf)	((cpu_to_le16(*(unsigned short *)((unsigned int)(pbuf) + (((GetToDs(pbuf)<<1)|GetFrDs(pbuf))==3?30:24))) & 0x0060)>>5)

#define GetAddr1Ptr(pbuf)	((unsigned char *)((unsigned int)(pbuf) + 4))

#define GetAddr2Ptr(pbuf)	((unsigned char *)((unsigned int)(pbuf) + 10))

#define GetAddr3Ptr(pbuf)	((unsigned char *)((unsigned int)(pbuf) + 16))

#define GetAddr4Ptr(pbuf)	((unsigned char *)((unsigned int)(pbuf) + 24))

//SEMI_QOS
#define GetQosControl(pbuf) (unsigned char *)((unsigned int)(pbuf) + (((GetToDs(pbuf)<<1)|GetFrDs(pbuf))==3?30:24))


#ifdef CONFIG_RTK_MESH
#define GetMeshHeaderFlagWithoutQOS(pbuf)	((unsigned char *)(pbuf) + 30)

#define GetMeshHeaderTTLWithOutQOS(pbuf)	((unsigned char *)(pbuf) + 31)  	// mesh header ttl

#define GetMeshHeaderSeqNumWithoutQOS(pbuf)	((unsigned short *)((unsigned int)(pbuf) + 32))	// Don't use cpu_to_le16(Other not use cpu_to_le16)
#define SetMeshHeaderSeqNum(pbuf, num) \
	do {    \
		*(unsigned short *)((unsigned int)(pbuf) + 34) = \
			((*(unsigned short *)((unsigned int)(pbuf) + 34)) & le16_to_cpu((unsigned short)~0xffff)) | \
			le16_to_cpu((unsigned short)(0xffff & num )); \
	} while(0)

#endif // CONFIG_RTK_MESH
/*-----------------------------------------------------------------------------
			Below is for the security related definition
------------------------------------------------------------------------------*/
#define _RESERVED_FRAME_TYPE_	0
#define _SKB_FRAME_TYPE_		2
#define _PRE_ALLOCMEM_			1
#define _PRE_ALLOCHDR_			3
#define _PRE_ALLOCLLCHDR_		4
#define _PRE_ALLOCICVHDR_		5
#define _PRE_ALLOCMICHDR_		6

#define _SIFSTIME_				((priv->pmib->dot11BssType.net_work_type&WIRELESS_11A)?16:10)
#define _ACKCTSLNG_				14	//14 bytes long, including crclng
#define _CRCLNG_				4

#define _ASOCREQ_IE_OFFSET_		4	// excluding wlan_hdr
#define	_ASOCRSP_IE_OFFSET_		6
#define _REASOCREQ_IE_OFFSET_	10
#define _REASOCRSP_IE_OFFSET_	6
#define _PROBEREQ_IE_OFFSET_	0
#define	_PROBERSP_IE_OFFSET_	12
#define _AUTH_IE_OFFSET_		6
#define _DEAUTH_IE_OFFSET_		0
#define _BEACON_IE_OFFSET_		12
#define _BEACON_CAP_OFFSET_	34

#ifdef CONFIG_RTK_MESH
#define	_DISASS_IE_OFFSET_			2	// 2 octets, reason code
#define	_MESH_HEADER_WITH_AE_		16	// mesh header with AE(Address Extension)
#define	_MESH_HEADER_WITHOUT_AE_	4	// mesh header without AE(Address Extension)
#define	_MESH_ACTIVE_FIELD_OFFSET_	2	// mesh active field Category+Action length
#endif

/* information element ID ,See textbook Table 4.7 */
#define _SSID_IE_				0
#define _SUPPORTEDRATES_IE_		1
#define _DSSET_IE_				3
#define _TIM_IE_				5
#define _IBSS_PARA_IE_			6

#define _CHLGETXT_IE_			16
#define _RSN_IE_2_				48
#define _RSN_IE_1_				221	// Error, Shall be Wi-Fi protected access (802.11b)
#define _ERPINFO_IE_			42	// [802.11g 7.3.2] ERP Information
#define _EXT_SUPPORTEDRATES_IE_	50	// [802.11g 7.3.2] Extended supported rates
#if defined(CONFIG_RTL_WAPI_SUPPORT)
#define	_EID_WAPI_				68
#endif
#define _WPS_IE_				221

#ifdef CONFIG_RTK_MESH    // CATUTION: below ALL undefine !!
#define _OFDM_PARAMETER_SET_IE_		200
#define _MESH_ID_IE_				201	// MESH ID infoemation element
#define _ACT_PROFILE_ANNOUN_IE_		202	// Active profile announcement IE
#define _LOCAL_LINK_STATE_ANNOU_IE_	203	// Local link state announcement IE
#define _WLAN_MESH_CAP_IE_			204	// WLAN mesh capability IE
#define _NEIGHBOR_LIST_IE_			205	// neighbor list IE
#define _DTIM_IE_					206	// DTIM IE
#define _MKDDIE_IE_					207
#define _EMSAIE_IE_					208
#define _BEACON_TIMING_IE_			209	
#define _UCG_SWITCH_ANNOU_IE_		210
#define _MDAOP_ADVERTISMENTS_IE_	211
#define _MOAOP_SET_TEARDOWN_IE_		212
#define _PEER_LINK_OPEN_IE_			223
#define _PEER_LINK_CONFIRM_IE_		224
#define _PEER_LINK_CLOSE_IE_		225
#define _PROXY_UPDATE_IE_			226 // add by pepsi 20080229
#define _PROXY_UPDATE_CONFIRM_IE_	227
#endif

/* ---------------------------------------------------------------------------
					Below is the fixed elements...
-----------------------------------------------------------------------------*/
#define _AUTH_ALGM_NUM_			2
#define _AUTH_SEQ_NUM_			2
#define _BEACON_ITERVAL_		2
#define _CAPABILITY_			2
#define _CURRENT_APADDR_		6
#define _LISTEN_INTERVAL_		2
#define _RSON_CODE_				2
#define _ASOC_ID_				2
#define _STATUS_CODE_			2
#define _TIMESTAMP_				8

#define AUTH_ODD_TO				0
#define AUTH_EVEN_TO			1

#define WLAN_ETHCONV_ENCAP		1
#define WLAN_ETHCONV_RFC1042	2
#define WLAN_ETHCONV_8021h		3


/*-----------------------------------------------------------------------------
				Below is the definition for 802.11i / 802.1x
------------------------------------------------------------------------------*/
#define _IEEE8021X_MGT_			1		// WPA
#define _IEEE8021X_PSK_			2		// WPA with pre-shared key

#define _NO_PRIVACY_			0
#define _WEP_40_PRIVACY_		1
#define _TKIP_PRIVACY_			2
#define _WRAP_PRIVACY_			3
#define _CCMP_PRIVACY_			4
#define _WEP_104_PRIVACY_		5
#define _WEP_WPA_MIXED_PRIVACY_ 6	// WEP + WPA
#define _WAPI_SMS4_				7


/*-----------------------------------------------------------------------------
			Below is for QoS related definition
------------------------------------------------------------------------------*/
#define _WMM_IE_Length_				7
#define _WMM_Para_Element_Length_	24
#define _ADDBA_Req_Frame_Length_	9
#define _ADDBA_Rsp_Frame_Length_	9
#define _DELBA_Frame_Length 		6
#define _ADDBA_Maximum_Buffer_Size_	64

#define _BLOCK_ACK_CATEGORY_ID_		3
#define _ADDBA_Req_ACTION_ID_		0
#define _ADDBA_Rsp_ACTION_ID_		1
#define _DELBA_ACTION_ID_			2


/*-----------------------------------------------------------------------------
			Below is for HT related definition
------------------------------------------------------------------------------*/
#define _HT_CAP_					45
#define _HT_IE_						61

#define _HT_MIMO_PS_STATIC_			BIT(0)
#define _HT_MIMO_PS_DYNAMIC_		BIT(1)

#ifdef WIFI_11N_2040_COEXIST
#define _PUBLIC_CATEGORY_ID_		4
#define _2040_COEXIST_ACTION_ID_	0

#define _2040_BSS_COEXIST_IE_		72
#define _40M_INTOLERANT_			BIT(1)
#define _20M_BSS_WIDTH_REQ_		BIT(2)

#define _2040_Intolerant_ChRpt_IE_	73
#define _OBSS_SCAN_PARA_IE_		74

#define _EXTENDED_CAP_IE_			127
#define _2040_COEXIST_SUPPORT_	BIT(0)
#endif
#define _HT_CATEGORY_ID_			7
#define _HT_MIMO_PS_ACTION_ID_		1


/*-----------------------------------------------------------------------------
			Below is the bit definition for HT Capabilities element
------------------------------------------------------------------------------*/
#define _HTCAP_SUPPORT_CH_WDTH_		BIT(1)
#define _HTCAP_SMPWR_STATIC_		0
#define _HTCAP_SMPWR_DYNAMIC_		BIT(2)
#define _HTCAP_SMPWR_ENABLE_		(BIT(2) | BIT(3))
#define _HTCAP_SHORTGI_20M_			BIT(5)
#define _HTCAP_SHORTGI_40M_			BIT(6)
#define _HTCAP_TX_STBC_				BIT(7)
#define _HTCAP_RX_STBC_				(BIT(8) | BIT(9))
#define _HTCAP_AMSDU_LEN_8K_		BIT(11)
#define _HTCAP_CCK_IN_40M_			BIT(12)
#ifdef WIFI_11N_2040_COEXIST
#define _HTCAP_40M_INTOLERANT_		BIT(14)
#endif
#define _HTCAP_AMPDU_FAC_8K_		0
#define _HTCAP_AMPDU_FAC_16K_		BIT(0)
#define _HTCAP_AMPDU_FAC_32K_		BIT(1)
#define _HTCAP_AMPDU_FAC_64K_		(BIT(0) | BIT(1))
#define _HTCAP_AMPDU_SPC_SHIFT_		2
#define _HTCAP_AMPDU_SPC_MASK_		0x1c
#define _HTCAP_AMPDU_SPC_NORES_		0
#define _HTCAP_AMPDU_SPC_QUAR_US_	1
#define _HTCAP_AMPDU_SPC_HALF_US_	2
#define _HTCAP_AMPDU_SPC_1_US_		3
#define _HTCAP_AMPDU_SPC_2_US_		4
#define _HTCAP_AMPDU_SPC_4_US_		5
#define _HTCAP_AMPDU_SPC_8_US_		6
#define _HTCAP_AMPDU_SPC_16_US_		7


/*-----------------------------------------------------------------------------
			Below is the bit definition for HT Information element
------------------------------------------------------------------------------*/
#define _HTIE_2NDCH_OFFSET_NO_		0
#define _HTIE_2NDCH_OFFSET_AB_		BIT(0)
#define _HTIE_2NDCH_OFFSET_BL_		(BIT(0) | BIT(1))
#define	_HTIE_STA_CH_WDTH_			BIT(2)
#define _HTIE_OP_MODE0_				0
#define _HTIE_OP_MODE1_				BIT(0)
#define _HTIE_OP_MODE2_				BIT(1)
#define _HTIE_OP_MODE3_				(BIT(0) | BIT(1))
#define _HTIE_NGF_STA_				BIT(2)
#define	_HTIE_TXBURST_LIMIT_		BIT(3)
#define _HTIE_OBSS_NHT_STA_			BIT(4)


#endif // _WIFI_H_

