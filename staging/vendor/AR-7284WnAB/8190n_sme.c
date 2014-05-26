/*
 *      Handling routines for 802.11 SME (Station Management Entity)
 *
 *      $Id: 8190n_sme.c,v 1.8 2009/08/06 11:41:29 yachang Exp $
 */

#define _8190N_SME_C_

#ifdef __MIPSEB__
#include <asm/addrspace.h>
#include <linux/module.h>
#endif
#include <linux/list.h>
#include <linux/random.h>
#include "./8190n_cfg.h"
#include "./8190n.h"
#include "./wifi.h"
#include "./8190n_hw.h"
#include "./8190n_headers.h"
#include "./8190n_rx.h"
#include "./8190n_debug.h"
#include "./8190n_psk.h"

#include "./8190n_usb.h"

#ifdef CONFIG_RTK_MESH
#include "./mesh_ext/mesh_util.h"
#include "./mesh_ext/mesh_route.h"
#ifdef MESH_USE_METRICOP
#include "mesh_ext/mesh_11kv.h"
#endif
#endif

#ifdef WIFI_SIMPLE_CONFIG
#define TAG_REQUEST_TYPE	0x103a
#define TAG_RESPONSE_TYPE	0x103b
#define MAX_REQUEST_TYPE_NUM 0x3
UINT8 WSC_IE_OUI[4] = {0x00, 0x50, 0xf2, 0x04};
#endif

#ifdef SEMI_QOS
unsigned char WMM_IE[] = {0x00, 0x50, 0xf2, 0x02, 0x00, 0x01};
unsigned char WMM_PARA_IE[] = {0x00, 0x50, 0xf2, 0x02, 0x01, 0x01};
#endif

unsigned char INTEL_OUI[50][3] =
{{0x00, 0x02, 0xb3}, {0x00, 0x03, 0x47},
{0x00, 0x04, 0x23}, {0x00, 0x07, 0xe9},
{0x00, 0x0c, 0xf1}, {0x00, 0x0e, 0x0c},
{0x00, 0x0e, 0x35}, {0x00, 0x11, 0x11},
{0x00, 0x12, 0xf0}, {0x00, 0x13, 0x02},
{0x00, 0x13, 0x20}, {0x00, 0x13, 0xce},
{0x00, 0x13, 0xe8}, {0x00, 0x15, 0x00},
{0x00, 0x15, 0x17}, {0x00, 0x16, 0x6f},
{0x00, 0x16, 0x76}, {0x00, 0x16, 0xea},
{0x00, 0x16, 0xeb}, {0x00, 0x18, 0xde},
{0x00, 0x19, 0xd1}, {0x00, 0x19, 0xd2},
{0x00, 0x1b, 0x21}, {0x00, 0x1b, 0x77},
{0x00, 0x1c, 0xbf}, {0x00, 0x1c, 0xc0},
{0x00, 0x1d, 0xe0}, {0x00, 0x1d, 0xe1},
{0x00, 0x1e, 0x64}, {0x00, 0x1e, 0x65},
{0x00, 0x1e, 0x67}, {0x00, 0x1f, 0x3b},
{0x00, 0x1f, 0x3c}, {0x00, 0x20, 0x7b},
{0x00, 0x21, 0x5c}, {0x00, 0x21, 0x5d},
{0x00, 0x21, 0x6a}, {0x00, 0x21, 0x6b},
{0x00, 0x22, 0xfa}, {0x00, 0x22, 0xfb},
{0x00, 0x23, 0x14}, {0x00, 0x23, 0x15},
{0x00, 0x24, 0xd6}, {0x00, 0x24, 0xd7},
{0x00, 0x90, 0x27}, {0x00, 0xa0, 0xc9},
{0x00, 0xaa, 0x00}, {0x00, 0xaa, 0x01},
{0x00, 0xaa, 0x02}, {0x00, 0xd0, 0xb7}};

// to avoid client mode assoc fail with TKIP/AES

/* for RTL865x suspend mode, the CPU can be suspended initially. */
int gCpuCanSuspend = 1;

static unsigned int OnAssocReq(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnProbeReq(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnProbeRsp(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnBeacon(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnDisassoc(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnAuth(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnDeAuth(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnWmmAction(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo);


#if defined(CONFIG_RTK_MESH) && defined(_11s_TEST_MODE_)
void clean_for_join(struct rtl8190_priv *priv);
#endif
static unsigned int DoReserved(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo);
#ifdef WDS
static void issue_probereq(struct rtl8190_priv * priv, unsigned char * ssid, int ssid_len, unsigned char * da);
#endif
#ifdef CLIENT_MODE
static unsigned int OnAssocRsp(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnBeaconClnt(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnATIM(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnDisassocClnt(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnAuthClnt(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnDeAuthClnt(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo);
static void start_clnt_assoc(struct rtl8190_priv *priv);
static void calculate_rx_beacon(struct rtl8190_priv *priv);
#endif


struct mlme_handler {
	unsigned int   num;
	char* str;
	unsigned int (*func)(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo);
};

#ifdef CONFIG_RTK_MESH
struct mlme_handler mlme_mp_tbl[]={
	{WIFI_ASSOCREQ,		"OnAssocReq",	OnAssocReq_MP},
	{WIFI_ASSOCRSP,		"OnAssocRsp",	OnAssocRsp_MP},
	{WIFI_REASSOCREQ,	"OnReAssocReq",	OnAssocReq_MP},
	{WIFI_REASSOCRSP,	"OnReAssocRsp",	OnAssocRsp_MP},
	{WIFI_PROBEREQ,		"OnProbeReq",	OnProbeReq},
	{WIFI_PROBERSP,		"OnProbeRsp",	OnProbeRsp},

	/*----------------------------------------------------------
					below 2 are reserved
	-----------------------------------------------------------*/
	{0,					"DoReserved",	DoReserved},
	{0,					"DoReserved",	DoReserved},
	{WIFI_BEACON,		"OnBeacon",		OnBeacon_MP},
	{WIFI_ATIM,			"OnATIM",		DoReserved},
	{WIFI_DISASSOC,		"OnDisassoc",	OnDisassoc_MP},
	{WIFI_AUTH,			"OnAuth",		OnAuth},
	{WIFI_DEAUTH,		"OnDeAuth",		OnDeAuth},
	{WIFI_WMM_ACTION,	"OnWmmAct",		OnWmmAction}
};
#endif	// CONFIG_RTK_MESH

struct mlme_handler mlme_ap_tbl[]={
	{WIFI_ASSOCREQ,		"OnAssocReq",	OnAssocReq},
	{WIFI_ASSOCRSP,		"OnAssocRsp",	DoReserved},
	{WIFI_REASSOCREQ,	"OnReAssocReq",	OnAssocReq},
	{WIFI_REASSOCRSP,	"OnReAssocRsp",	DoReserved},
	{WIFI_PROBEREQ,		"OnProbeReq",	OnProbeReq},
	{WIFI_PROBERSP,		"OnProbeRsp",	OnProbeRsp},

	/*----------------------------------------------------------
					below 2 are reserved
	-----------------------------------------------------------*/
	{0,					"DoReserved",	DoReserved},
	{0,					"DoReserved",	DoReserved},
	{WIFI_BEACON,		"OnBeacon",		OnBeacon},
	{WIFI_ATIM,			"OnATIM",		DoReserved},
	{WIFI_DISASSOC,		"OnDisassoc",	OnDisassoc},
	{WIFI_AUTH,			"OnAuth",		OnAuth},
	{WIFI_DEAUTH,		"OnDeAuth",		OnDeAuth},
	{WIFI_WMM_ACTION,	"OnWmmAct",		OnWmmAction}
};
#ifdef CLIENT_MODE
struct mlme_handler mlme_station_tbl[]={
	{WIFI_ASSOCREQ,		"OnAssocReq",	DoReserved},
	{WIFI_ASSOCRSP,		"OnAssocRsp",	OnAssocRsp},
	{WIFI_REASSOCREQ,	"OnReAssocReq",	DoReserved},
	{WIFI_REASSOCRSP,	"OnReAssocRsp",	OnAssocRsp},
	{WIFI_PROBEREQ,		"OnProbeReq",	OnProbeReq},
	{WIFI_PROBERSP,		"OnProbeRsp",	OnProbeRsp},

	/*----------------------------------------------------------
					below 2 are reserved
	-----------------------------------------------------------*/
	{0,					"DoReserved",	DoReserved},
	{0,					"DoReserved",	DoReserved},
	{WIFI_BEACON,		"OnBeacon",		OnBeaconClnt},
	{WIFI_ATIM,			"OnATIM",		OnATIM},
	{WIFI_DISASSOC,		"OnDisassoc",	OnDisassocClnt},
	{WIFI_AUTH,			"OnAuth",		OnAuthClnt},
	{WIFI_DEAUTH,		"OnDeAuth",		OnDeAuthClnt},
	{WIFI_WMM_ACTION,	"OnWmmAct",		OnWmmAction}
};
#endif




#ifdef WIFI_SIMPLE_CONFIG
static unsigned char *search_wsc_tag(unsigned char *data, unsigned short id, int len, int *out_len)
{
	unsigned short tag, tag_len;
	int size;

	while (len > 0) {
		memcpy(&tag, data, 2);
		memcpy(&tag_len, data+2, 2);
		tag = ntohs(tag);
		tag_len = ntohs(tag_len);

		if (id == tag) {
			if (len >= (4 + tag_len)) {
				*out_len = (int)tag_len;
				return (&data[4]);
			}
			else {
				_DEBUG_ERR("Found tag [0x%x], but invalid length!\n", id);
				break;
			}
		}
		size = 4 + tag_len;
		data += size;
		len -= size;
	}

	return NULL;
}

static struct wsc_probe_request_info *search_wsc_probe_sta(struct rtl8190_priv *priv, unsigned char *addr)
{
	int i, idx=-1;

	for (i=0; i<MAX_WSC_PROBE_STA; i++) {
		if (priv->wsc_sta[i].used == 0) {
			if (idx < 0)
				idx = i;
			continue;
		}
		if (!memcmp(priv->wsc_sta[i].addr, addr, MACADDRLEN))
			break;
	}

	if ( i != MAX_WSC_PROBE_STA)
		return (&priv->wsc_sta[i]); // return sta info for WSC sta

	if (idx >= 0)
		return (&priv->wsc_sta[idx]); // add sta info for WSC sta
	else {
		// sta list full, need to replace sta
		unsigned long oldest_time_stamp=jiffies;

		for (i=0; i<MAX_WSC_PROBE_STA; i++) {
			if (priv->wsc_sta[i].time_stamp < oldest_time_stamp) {
				oldest_time_stamp = priv->wsc_sta[i].time_stamp;
				idx = i;
			}
		}
		memset(&priv->wsc_sta[idx], 0, sizeof(struct wsc_probe_request_info));

		return (&priv->wsc_sta[idx]);
	}
}

static __inline__ void wsc_forward_probe_request(struct rtl8190_priv *priv, unsigned char *pframe, unsigned char *IEaddr, unsigned int IElen)
{
	unsigned char *p=IEaddr;
	unsigned int len=IElen;
	unsigned char forwarding=0;
	struct wsc_probe_request_info *wsc_sta=NULL;
	DOT11_PROBE_REQUEST_IND ProbeReq_Ind;
	unsigned long flags;

	if (IEaddr == NULL || IElen == 0)
		return;

	p = search_wsc_tag(p+2+4, TAG_REQUEST_TYPE, len-4, &len);
	if (p && (*p <= MAX_REQUEST_TYPE_NUM)) { //forward WPS IE to wsc daemon
		SAVE_INT_AND_CLI(flags);
		wsc_sta = search_wsc_probe_sta(priv, (unsigned char *)GetAddr2Ptr(pframe));
		if (wsc_sta->used) {
			if ((wsc_sta->ProbeIELen != IElen) ||
				(memcmp(wsc_sta->ProbeIE, (void *)(IEaddr), IElen) != 0) ||
				((jiffies - wsc_sta->time_stamp) > 300)) {
				memcpy(wsc_sta->ProbeIE, (void *)(IEaddr), IElen);
				wsc_sta->ProbeIELen = IElen;
				wsc_sta->time_stamp = jiffies;
				forwarding = 1;
			}
		}
		else {
			memcpy(wsc_sta->addr, (void *)GetAddr2Ptr(pframe), MACADDRLEN);
			memcpy(wsc_sta->ProbeIE, (void *)(IEaddr), IElen);
			wsc_sta->ProbeIELen = IElen;
			wsc_sta->time_stamp = jiffies;
			wsc_sta->used = 1;
			forwarding = 1;
		}
		RESTORE_INT(flags);

		if (forwarding) {
			memcpy((void *)ProbeReq_Ind.MACAddr, (void *)GetAddr2Ptr(pframe), MACADDRLEN);
			ProbeReq_Ind.EventId = DOT11_EVENT_WSC_PROBE_REQ_IND;
			ProbeReq_Ind.IsMoreEvent = 0;
			ProbeReq_Ind.ProbeIELen = IElen;
			memcpy((void *)ProbeReq_Ind.ProbeIE, (void *)(IEaddr), ProbeReq_Ind.ProbeIELen);
			DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&ProbeReq_Ind,
			sizeof(DOT11_PROBE_REQUEST_IND));
			event_indicate(priv, GetAddr2Ptr(pframe), 1);
		}
	}
}

static __inline__ void wsc_probe_expire(struct rtl8190_priv *priv)
{
	int i;
	unsigned long flags;

	SAVE_INT_AND_CLI(flags);
	for (i=0; i<MAX_WSC_PROBE_STA; i++) {
		if (priv->wsc_sta[i].used == 0)
			continue;
		if ((jiffies - priv->wsc_sta[i].time_stamp) > 18000)
			memset(&priv->wsc_sta[i], 0, sizeof(struct wsc_probe_request_info));
	}
	RESTORE_INT(flags);
}
#endif // WIFI_SIMPLE_CONFIG

static __inline__ UINT8 match_supp_rate(unsigned char *pRate, int len, UINT8 rate)
{
	int idx;
	for (idx=0; idx<len; idx++) {
		if ((pRate[idx] & 0x7f) == rate)
			return 1;
	}

	// TODO: need some more refinement
	if ((rate & 0x80) && ((rate & 0x7f) < 16))
		return 1;

	return 0;
}


// unchainned all the skb chainnned in a given list, like frag_list(type == 0)
static void unchainned_all_frag(struct rtl8190_priv *priv, struct list_head *phead,
				unsigned int list_type)
{
	unsigned long flags;
	struct rx_frinfo *pfrinfo;
	struct list_head *plist;
	struct sk_buff	 *pskb;

	SAVE_INT_AND_CLI(flags);
	while(!list_empty(phead))
	{
		plist = phead->next;
		pfrinfo = list_entry(plist, struct rx_frinfo, mpdu_list);
		pskb = get_pskb(pfrinfo);
		list_del_init(plist);
		rtl_kfree_skb(priv, pskb, _SKB_RX_IRQ_);
	}
	RESTORE_INT(flags);
}


void rtl8190_frag_timer(unsigned long task_priv)
{
	unsigned long flags;
	struct list_head	*phead, *plist;
	struct stat_info	*pstat;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)task_priv;

	priv->frag_to ^= 0x01;

	phead = &priv->defrag_list;
	plist = phead->next;

	SAVE_INT_AND_CLI(flags);

	while(plist != phead)
	{
		pstat = list_entry(plist, struct stat_info, defrag_list);
		plist = plist->next;

		if (pstat->frag_to == priv->frag_to)
		{
			list_del_init(&pstat->defrag_list);
			unchainned_all_frag(priv, &pstat->frag_list, 0);
			pstat->frag_count = 0;
		}
	}

	RESTORE_INT(flags);

	mod_timer(&priv->frag_to_filter, jiffies + FRAG_TO);
}


#ifdef USB_PKT_RATE_CTRL_SUPPORT
usb_pktCnt_fn get_usb_pkt_cnt_hook = NULL;
register_usb_pkt_cnt_fn register_usb_hook = NULL;

void register_usb_pkt_cnt_f(void *usbPktFunc)
{
	get_usb_pkt_cnt_hook = (usb_pktCnt_fn)(usbPktFunc);
}


void usbPkt_timer_handler(struct rtl8190_priv *priv)
{
	unsigned int pkt_cnt, pkt_diff;

	if (!get_usb_pkt_cnt_hook)
		return;

	pkt_cnt = get_usb_pkt_cnt_hook();
	pkt_diff = pkt_cnt - priv->pre_pkt_cnt;

	if (pkt_diff) {
		priv->auto_rate_mask = 0x803fffff;
		priv->change_toggle = ((priv->change_toggle) ? 0 : 1);
	}

	priv->pre_pkt_cnt = pkt_cnt;
	priv->pkt_nsec_diff += pkt_diff;

	if ((priv->poll_usb_cnt++) % 10 == 0) {
		if ((priv->pkt_nsec_diff) < 10 ) {
			priv->auto_rate_mask = 0;
			priv->pkt_nsec_diff = 0;
		}
	}
}
#endif // USB_PKT_RATE_CTRL_SUPPORT


static void auth_expire(struct rtl8190_priv *priv)
{
	struct stat_info	*pstat;
	struct list_head	*phead, *plist;
	unsigned long	flags;

	phead = &priv->auth_list;
	plist = phead->next;

	SAVE_INT_AND_CLI(flags);
	while(plist != phead)
	{
		pstat = list_entry(plist, struct stat_info, auth_list);
		plist = plist->next;

// #if defined(CONFIG_RTK_MESH) && defined(MESH_BOOTSEQ_AUTH) // Skip MP node
#ifdef CONFIG_RTK_MESH // Skip MP node
		if(isPossibleNeighbor(pstat))
			continue;
#endif // CONFIG_RTK_MESH

		pstat->expire_to--;
		if (pstat->expire_to == 0)
		{
			list_del_init(&pstat->auth_list);

			//below should be take care... since auth fail, just free the stat info...
			DEBUG_INFO("auth expire %02X%02X%02X%02X%02X%02X\n",
				pstat->hwaddr[0],pstat->hwaddr[1],pstat->hwaddr[2],pstat->hwaddr[3],pstat->hwaddr[4],pstat->hwaddr[5]);
			free_stainfo(priv, pstat);
		}
	}
	RESTORE_INT(flags);
}


static void check_RA_by_rssi(struct rtl8190_priv *priv, struct stat_info *pstat)
{
	int level = 0;

	switch (pstat->rssi_level) {
		case 1:
			if (pstat->rssi >= priv->pshare->rf_ft_var.raGoDownUpper)
				level = 1;
			else if ((pstat->rssi >= priv->pshare->rf_ft_var.raGoDown20MLower) ||
				((priv->pshare->is_40m_bw) && (pstat->ht_cap_len) &&
				(pstat->rssi >= priv->pshare->rf_ft_var.raGoDown40MLower) &&
				(pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_))))
				level = 2;
			else
				level = 3;
			break;
		case 2:
			if (pstat->rssi > priv->pshare->rf_ft_var.raGoUpUpper)
				level = 1;
			else if ((pstat->rssi < priv->pshare->rf_ft_var.raGoDown40MLower) ||
				((!pstat->ht_cap_len || !priv->pshare->is_40m_bw ||
				!(pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_))) &&
				(pstat->rssi < priv->pshare->rf_ft_var.raGoDown20MLower)))
				level = 3;
			else
				level = 2;
			break;
		case 3:
			if (pstat->rssi > priv->pshare->rf_ft_var.raGoUpUpper)
				level = 1;
			else if ((pstat->rssi > priv->pshare->rf_ft_var.raGoUp20MLower) ||
				((priv->pshare->is_40m_bw) && (pstat->ht_cap_len) &&
				(pstat->rssi > priv->pshare->rf_ft_var.raGoUp40MLower) &&
				(pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_))))
				level = 2;
			else
				level = 3;
			break;
		default:
			if (isErpSta(pstat))
				DEBUG_ERR("wrong rssi level setting\n");
			break;
	}

	if (!pstat->is_2t_mimo_sta) {
		if (pstat->highest_rx_rate >= _MCS8_RATE_) {	// higher than MCS8 (2 spacial streams)
			pstat->is_2t_mimo_sta = TRUE;
			//if (pstat->is_rtl8190_sta)
			//	pstat->rssi_level = 0xff;		// force reassign RA bitmap
		}
	}

	if (level != pstat->rssi_level) {
		pstat->rssi_level = level;
		add_update_RATid(priv, pstat);
	}
}




static void check_txrate_by_reg(struct rtl8190_priv *priv
, struct stat_info *pstat
)
{
	unsigned char initial_rate = 0x3f;
#ifdef STA_EXT
	if (pstat->remapped_aid && (pstat->remapped_aid < FW_NUM_STAT-1)) {
#else
	if (pstat->aid && (pstat->aid < 32)) {
#endif
		if (priv->pmib->dot11StationConfigEntry.autoRate) {
#ifdef STA_EXT
			initial_rate = RTL_R8(INIMCS_SEL + pstat->remapped_aid) & 0x3f;
#else
			initial_rate = RTL_R8(INIMCS_SEL + pstat->aid) & 0x3f;
#endif
			if (initial_rate == 0x3f)
				return;
			if (initial_rate < 12)
				pstat->current_tx_rate = dot11_rate_table[initial_rate];
			else if (initial_rate > 27)
				pstat->current_tx_rate = (pstat->tx_ra_bitmap & 0xff00000) ? 0x8f : 0x87;
			else
				pstat->current_tx_rate = 0x80|(initial_rate -12);

			if (initial_rate == 28)
				pstat->ht_current_tx_info |= TX_USE_SHORT_GI;
			else
				pstat->ht_current_tx_info &= ~TX_USE_SHORT_GI;
		}
		else {
			if (((pstat->tx_bw == HT_CHANNEL_WIDTH_20)
				&& (priv->pmib->dot11nConfigEntry.dot11nShortGIfor20M)
				&& (pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SHORTGI_20M_)))
				|| ((pstat->tx_bw == HT_CHANNEL_WIDTH_20_40)
				&& (priv->pmib->dot11nConfigEntry.dot11nShortGIfor40M)
				&& (pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SHORTGI_40M_))))
				pstat->ht_current_tx_info |= TX_USE_SHORT_GI;
			else
				pstat->ht_current_tx_info &= ~TX_USE_SHORT_GI;
		}

		if (priv->pshare->is_40m_bw && (pstat->tx_bw == HT_CHANNEL_WIDTH_20_40))
			pstat->ht_current_tx_info |= TX_USE_40M_MODE;
		else
			pstat->ht_current_tx_info &= ~TX_USE_40M_MODE;
	}
	else {
		DEBUG_INFO("sta has no aid found to check current tx rate\n");
	}
}


// for simplify, we consider only two stations. Otherwise we may sorting all the stations and
// hard to maintain the code.
// 0 for path A/B selection(bg only or 1ss rate), 1 for TX Diversity (ex: DIR 655 clone)

struct stat_info* switch_ant_enable(struct rtl8190_priv *priv, unsigned char flag)
{
	struct stat_info	*pstat, *pstat_chosen = NULL;
	struct list_head	*phead, *plist;
	unsigned int tp_2nd = 0, maxTP = 0;
	unsigned int rssi_2ndTp = 0, rssi_maxTp = 0;
	unsigned int tx_2s_avg = 0;
	unsigned int rx_2s_avg = 0;
	unsigned long total_sum = (priv->pshare->current_tx_bytes+priv->pshare->current_rx_bytes);
	unsigned char th_rssi = 0;

	phead = &priv->asoc_list;
	plist = phead->next;

	while(plist != phead)
	{
		pstat = list_entry(plist, struct stat_info, asoc_list);
		if((pstat->tx_avarage + pstat->rx_avarage) > maxTP){
			tp_2nd = maxTP;
			rssi_2ndTp = rssi_maxTp;

			maxTP = pstat->tx_avarage + pstat->rx_avarage;
			rssi_maxTp = pstat->rssi;

			pstat_chosen = pstat;
		}
		if (plist == plist->next)
			break;
		plist = plist->next;
	}
	// for debug
//	printk("maxTP: %d, second: %d\n", rssi_maxTp, rssi_2ndTp);

	if(pstat_chosen == NULL){
//		printk("ERROR! NULL pstat_chosen \n");
		return NULL;
	}

	if(total_sum != 0){
		tx_2s_avg = (unsigned int)((pstat_chosen->current_tx_bytes*100) / total_sum);
		rx_2s_avg = (unsigned int)((pstat_chosen->current_rx_bytes*100) / total_sum);
	}

	if( priv->assoc_num > 1 && (tx_2s_avg+rx_2s_avg) < (100/priv->assoc_num)){ // this is not a burst station
		pstat_chosen = NULL;
//		printk("avg is: %d\n", (tx_2s_avg+rx_2s_avg));
		goto out_switch_ant_enable;
	}

	if(flag == 1)
		goto out_switch_ant_enable;

#ifdef STA_EXT
	if(pstat_chosen && (pstat->remapped_aid < FW_NUM_STAT-1) && 
		!(priv->pshare->has_2r_sta & BIT(pstat_chosen->remapped_aid)))// 1r STA
#else
	if(pstat_chosen && !(priv->pshare->has_2r_sta & BIT(pstat_chosen->aid)))// 1r STA
#endif
		th_rssi = 40;
	else
		th_rssi = 63;

	if((maxTP < tp_2nd*2 && (rssi_maxTp < th_rssi || rssi_2ndTp < th_rssi)))
		pstat_chosen = NULL;
	else if(maxTP >= tp_2nd*2 && rssi_maxTp < th_rssi)
		pstat_chosen = NULL;

out_switch_ant_enable:
	return pstat_chosen;
}


static void assoc_expire(struct rtl8190_priv *priv)
{
	struct stat_info	*pstat;
	struct stat_info	*highTP_found_pstat = NULL;	// bcm old 11n chipset iot debug, and TXOP enlarge
	struct list_head	*phead, *plist;
	unsigned long	flags;
	unsigned int	ok_curr, ok_pre, do_addRatid;
	unsigned char rssi_min = 0xff;	// for DIG checking
#ifdef SEMI_QOS
	unsigned int switch_turbo = 0;
#endif

	phead = &priv->asoc_list;
	plist = phead->next;

	SAVE_INT_AND_CLI(flags);
	while(plist != phead)
	{
		pstat = list_entry(plist, struct stat_info, asoc_list);
//		plist = plist->next;
		do_addRatid = 0;
		pstat->link_time++;
		// Check idle using packet transmit....nctu note it
		ok_curr = pstat->tx_pkts - pstat->tx_fail;
		ok_pre = pstat->tx_pkts_pre - pstat->tx_fail_pre;
		if ((ok_curr == ok_pre) &&
			(pstat->rx_pkts == pstat->rx_pkts_pre))
		{
			pstat->active = 0;
			if (pstat->expire_to > 0)
			{
				// free queued skb if sta is idle longer than 5 seconds, and do at most 3 times
				if (((priv->expire_to - pstat->expire_to) >= 5) && ((priv->expire_to - pstat->expire_to) < 8))
					free_sta_skb(priv, pstat);

				// calculate STA number
				if ((pstat->expire_to == 1)
#ifdef WDS
					&& !(pstat->state & WIFI_WDS)
#endif
				) {
					cnt_assoc_num(priv, pstat, DECREASE, (char *)__FUNCTION__);
					check_sta_characteristic(priv, pstat, DECREASE);

					// CAM entry update
					if (!SWCRYPTO && pstat->dot11KeyMapping.keyInCam) {
						if (CamDeleteOneEntry(priv, pstat->hwaddr, 0, 0)) {
							pstat->dot11KeyMapping.keyInCam = -1;
							priv->pshare->CamEntryOccupied--;
						}
					}

#if defined(RTL8192E) || defined(STA_EXT)
					//Release Firmware Rateid table
					remove_RATid(priv, pstat);
#endif

					LOG_MSG("A STA is expired - %02X:%02X:%02X:%02X:%02X:%02X\n",
						pstat->hwaddr[0],pstat->hwaddr[1],pstat->hwaddr[2],pstat->hwaddr[3],pstat->hwaddr[4],pstat->hwaddr[5]);
				}

				pstat->expire_to--;
			}
		}
		else
		{
			// pass rssi info to f/w for aid > 8
			// pass rssi info to f/w for aid > 0 if client mode

#ifdef STA_EXT
			if (((OPMODE & WIFI_AP_STATE) && (pstat->remapped_aid < FW_NUM_STAT-1) && (pstat->remapped_aid > 8))
#ifdef CLIENT_MODE
				|| ((OPMODE & (WIFI_STATION_STATE|WIFI_ADHOC_STATE)) && (pstat->aid > 0))
#endif
				)
				set_fw_reg(priv, 0xfd000012 | pstat->remapped_aid<<16 | pstat->rssi<<8, 0, 0);

#else// STA_EXT

			if (((OPMODE & WIFI_AP_STATE) && (pstat->aid > 8))
#ifdef CLIENT_MODE
				|| ((OPMODE & (WIFI_STATION_STATE|WIFI_ADHOC_STATE)) && (pstat->aid > 0))
#endif
				)
				set_fw_reg(priv, 0xfd000012 | pstat->aid<<16 | pstat->rssi<<8, 0, 0);
#endif// STA_EXT



			if (priv->pshare->rf_ft_var.rssi_dump) {
#ifdef CONFIG_PRINTK_DISABLED
				panic_printk
#else
				printk
#endif
					("[%d] %d%%  tx %s%d  rx %s%d (ss %d %d)(snr %d %d)(sq %d %d)\n",
					pstat->aid, pstat->rssi,
					pstat->current_tx_rate&0x80? "MCS" : "",
					pstat->current_tx_rate&0x80? pstat->current_tx_rate&0x7f : pstat->current_tx_rate/2,
					pstat->rx_rate&0x80? "MCS" : "",
					pstat->rx_rate&0x80? pstat->rx_rate&0x7f : pstat->rx_rate/2,
					pstat->rf_info.mimorssi[0], pstat->rf_info.mimorssi[1],
					pstat->rf_info.RxSNRdB[0], pstat->rf_info.RxSNRdB[1],
					pstat->rf_info.mimosq[0], pstat->rf_info.mimosq[1]);
			}

			pstat->active = 1;
			// calculate STA number
			if ((pstat->expire_to == 0)
#ifdef WDS
				&& !(pstat->state & WIFI_WDS)
#endif
			) {
				cnt_assoc_num(priv, pstat, INCREASE, (char *)__FUNCTION__);
				check_sta_characteristic(priv, pstat, INCREASE);

				// CAM entry update
				if (!SWCRYPTO) {
					if (priv->pmib->dot118021xAuthEntry.dot118021xAlgrthm ||
							pstat->dot11KeyMapping.keyInCam == -1) {
						unsigned int privacy = pstat->dot11KeyMapping.dot11Privacy;

						if (CamAddOneEntry(priv, pstat->hwaddr, 0, privacy<<2, 0,
								pstat->dot11KeyMapping.dot11EncryptKey.dot11TTKey.skey)) {
							pstat->dot11KeyMapping.keyInCam = TRUE;
							priv->pshare->CamEntryOccupied++;
							assign_aggre_mthod(priv, pstat);
						}
						else {
							if (pstat->aggre_mthd != AGGRE_MTHD_NONE)
								pstat->aggre_mthd = AGGRE_MTHD_NONE;
						}
					}
				}

#if defined(RTL8192E) || defined(STA_EXT)
				// Resume Ratid
				add_update_RATid(priv, pstat);
#endif

				//pstat->dwngrade_probation_idx = pstat->upgrade_probation_idx = 0;	// unused
				LOG_MSG("A expired STA is resumed - %02X:%02X:%02X:%02X:%02X:%02X\n",
					pstat->hwaddr[0],pstat->hwaddr[1],pstat->hwaddr[2],pstat->hwaddr[3],pstat->hwaddr[4],pstat->hwaddr[5]);
			}

			pstat->expire_to = priv->expire_to;

		}

#ifdef WDS
		if (pstat->state & WIFI_WDS) {
			if ((pstat->rx_pkts != pstat->rx_pkts_pre) || pstat->beacon_num)
				pstat->idle_time = 0;
			else
				pstat->idle_time++;

			if ((priv->up_time%2) == 0) {
				if ((pstat->beacon_num == 0) && (pstat->state & WIFI_WDS_RX_BEACON))
					pstat->state &= ~WIFI_WDS_RX_BEACON;
				if (pstat->beacon_num)
					pstat->beacon_num = 0;
			}
		}
#endif

		pstat->tx_pkts_pre = pstat->tx_pkts;
		pstat->rx_pkts_pre = pstat->rx_pkts;
		pstat->tx_fail_pre = pstat->tx_fail;

		if ((priv->up_time % 3) == 0) {
			check_RA_by_rssi(priv, pstat);
			check_txrate_by_reg(priv, pstat);
		}

		// bcm old 11n chipset iot debug, and TXOP enlarge
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
		if (IS_ROOT_INTERFACE(priv))
#endif
		{
			{
				if ((priv->up_time % 2) == 0) {
					unsigned int tx_2s_avg = 0;
					unsigned int rx_2s_avg = 0;
					unsigned long total_sum = (priv->pshare->current_tx_bytes+priv->pshare->current_rx_bytes);

					pstat->current_tx_bytes += pstat->tx_byte_cnt;
					pstat->current_rx_bytes += pstat->rx_byte_cnt;

					if (pstat->ht_cap_len && !pstat->is_rtl8190_sta && !pstat->is_broadcom_sta &&
						!pstat->is_marvell_sta && !pstat->is_intel_sta) {
						if ((pstat->rssi > 20) && !pstat->is_forced_rts &&
							(!priv->pmib->dot11OperationEntry.wifi_specific ||
							((pstat->current_tx_bytes + pstat->current_rx_bytes) > 2*1024*1024/8)))
							pstat->is_forced_rts = TRUE;
						else if (pstat->is_forced_rts && ((pstat->rssi < 15) ||
							(priv->pmib->dot11OperationEntry.wifi_specific &&
							((pstat->current_tx_bytes + pstat->current_rx_bytes) < 2*1024*1024/8))))
							pstat->is_forced_rts = FALSE;
					}
					if (pstat->ht_cap_len && pstat->is_intel_sta &&
						(!priv->pshare->is_40m_bw || (pstat->tx_bw == HT_CHANNEL_WIDTH_20))) {
						if ((pstat->rssi > 35) && !pstat->is_forced_rts &&
							(!priv->pmib->dot11OperationEntry.wifi_specific ||
							((pstat->current_tx_bytes + pstat->current_rx_bytes) > 2*1024*1024/8)))
							pstat->is_forced_rts = TRUE;
						else if (pstat->is_forced_rts && ((pstat->rssi < 30) ||
							(priv->pmib->dot11OperationEntry.wifi_specific &&
							((pstat->current_tx_bytes + pstat->current_rx_bytes) < 2*1024*1024/8))))
							pstat->is_forced_rts = FALSE;
					}

					if(total_sum != 0){
						tx_2s_avg = (unsigned int)((pstat->current_tx_bytes*100) / total_sum);
						rx_2s_avg = (unsigned int)((pstat->current_rx_bytes*100) / total_sum);
					}
					if (((!pstat->is_intel_sta && pstat->is_forced_rts) || pstat->is_rtl8190_sta ||
						pstat->is_marvell_sta || (pstat->is_intel_sta && !pstat->is_forced_rts &&
						(!priv->pshare->is_40m_bw || (pstat->tx_bw == HT_CHANNEL_WIDTH_20)) &&
						(pstat->rssi < 35))) && pstat->ht_cap_len)
					if (((unsigned int)(tx_2s_avg+rx_2s_avg)) >= 50)
							highTP_found_pstat = pstat;
					{
						struct stat_info	*pstat_swAnt;

						if((pstat_swAnt = switch_ant_enable(priv, 0))!=NULL && (get_rf_mimo_mode(priv) == MIMO_2T2R)){// shall we enable the mechanism?
						//find an Anttena to switch
							tx_path_by_rssi(priv, pstat_swAnt, 1);// 1 means enable
						}else{
							tx_path_by_rssi(priv, pstat_swAnt, 0);
						}

#ifdef EXT_ANT_DVRY
						if(priv->pshare->rf_ft_var.ExtAntDvry==1&& priv->pshare->EXT_AD_probe == 0 && (pstat_swAnt != NULL || (pstat_swAnt = switch_ant_enable(priv, 1))!=NULL)){// there is a HP station
							int pathB_rssi_current = pstat_swAnt->rf_info.mimorssi[1];
							int pathB_rssi_hold = pstat_swAnt->rf_info.mimorssi_hold[1];
							int cur_hold_diff = (pathB_rssi_hold > pathB_rssi_current)?(pathB_rssi_hold - pathB_rssi_current):0;
							int hold_cur_diff = (pathB_rssi_hold < pathB_rssi_current && pathB_rssi_hold != 0 && pathB_rssi_current!= 0)?(pathB_rssi_current - pathB_rssi_hold):0;
							if(((int)(pstat_swAnt->rf_info.mimorssi[0] - pstat_swAnt->rf_info.mimorssi[1]) > priv->pshare->rf_ft_var.ext_ad_th && priv->pshare->EXT_AD_nextTs == 0) ||
								(cur_hold_diff >  priv->pshare->rf_ft_var.ext_ad_th && hold_cur_diff < priv->pshare->rf_ft_var.ext_ad_th ))	{ // if rssi A-B > th
								//switch to EXT_AD_B'
								SwitchExtAnt(priv, priv->pshare->EXT_AD_selection^1);
								priv->pshare->EXT_AD_pstat = pstat_swAnt;
//								panic_printk("A/B difference: %d\n", (int)(pstat_swAnt->rf_info.mimorssi[0] - pstat_swAnt->rf_info.mimorssi[1]));
//								panic_printk("B'/B difference: %d\n", cur_hold_diff);
								priv->pshare->EXT_AD_probe = 1; // start caculating
								pstat_swAnt->rf_info.mimorssi_hold[0] = pstat_swAnt->rf_info.mimorssi_hold[1] = 0;
							}
						}
#endif
					}

				}
				else {
					pstat->current_tx_bytes = pstat->tx_byte_cnt;
					pstat->current_rx_bytes = pstat->rx_byte_cnt;
				}
			}
		}

#if (defined(RTL8192SE)||defined(RTL8192SU)) && defined(EXT_ANT_DVRY)
	if(priv->pshare->EXT_AD_nextTs > 0 && priv->pshare->rf_ft_var.ExtAntDvry==1)
		priv->pshare->EXT_AD_nextTs --;

#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if(IS_ROOT_INTERFACE(priv) && priv->pshare->EXT_AD_probe == 1 && priv->pshare->rf_ft_var.ExtAntDvry==1) // for test
#else
	if (priv->pshare->EXT_AD_probe == 1 && priv->pshare->rf_ft_var.ExtAntDvry==1)
#endif
	{
		if (priv->pshare->EXT_AD_counter > 0)
			priv->pshare->EXT_AD_counter--;
		else {
			int before_swAnt = 0, after_swAnt= 0;
			before_swAnt = priv->pshare->EXT_AD_pstat->rf_info.mimorssi_hold[0] - priv->pshare->EXT_AD_pstat->rf_info.mimorssi_hold[1];
			after_swAnt = priv->pshare->EXT_AD_pstat->rf_info.mimorssi[0] - priv->pshare->EXT_AD_pstat->rf_info.mimorssi[1];

			if(after_swAnt >= before_swAnt){
				SwitchExtAnt(priv, priv->pshare->EXT_AD_selection^1);// toggle the external ant
				priv->pshare->EXT_AD_pstat->rf_info.mimorssi[0] = priv->pshare->EXT_AD_pstat->rf_info.mimorssi_hold[0];
				priv->pshare->EXT_AD_pstat->rf_info.mimorssi[1] = priv->pshare->EXT_AD_pstat->rf_info.mimorssi_hold[1];
			}
			priv->pshare->EXT_AD_counter = priv->pshare->rf_ft_var.ext_ad_Ttry;
			priv->pshare->EXT_AD_probe = 0;
			if(priv->pshare->EXT_AD_nextTs == 0)
				priv->pshare->EXT_AD_nextTs = priv->pshare->rf_ft_var.ext_ad_Ts;
		}
	}
#endif


		// 11n ap AES debug
		if (!SWCRYPTO) {
			if (((pstat->tx_byte_cnt + pstat->rx_byte_cnt) >= (priv->pshare->rf_ft_var.aes_check_th * 1024))
				&& (!pstat->is_fw_matching_sta)) {
				pstat->is_fw_matching_sta++;
				do_addRatid++;
			}
			else if (((pstat->tx_byte_cnt + pstat->rx_byte_cnt) < (priv->pshare->rf_ft_var.aes_check_th * 1024))
				&& (pstat->is_fw_matching_sta)) {
				pstat->is_fw_matching_sta = 0;
				do_addRatid++;
			}
		}

		// calculate tx/rx throughput
		pstat->tx_avarage = (pstat->tx_avarage/10)*7 + (pstat->tx_byte_cnt/10)*3;
		pstat->tx_byte_cnt = 0;
		pstat->rx_avarage = (pstat->rx_avarage/10)*7 + (pstat->rx_byte_cnt/10)*3;
		pstat->rx_byte_cnt = 0;

		if (((priv->up_time % 3) == 1) && (pstat->rssi < rssi_min) &&
			(pstat->expire_to > (priv->expire_to - priv->pshare->rf_ft_var.rssi_expire_to)))
			rssi_min = pstat->rssi;

		// enhance f/w Rate Adaptive

		if (do_addRatid) {
			add_update_RATid(priv, pstat);
		}

		if (plist == plist->next)
			break;
		plist = plist->next;
	}

	if ((priv->up_time % 3) == 1) {
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
		if (IS_ROOT_INTERFACE(priv))
#endif
			if (rssi_min != 0xff) {
				// for DIG checking
				check_DIG_by_rssi(priv, rssi_min);

				CCK_CCA_dynamic_enhance(priv, rssi_min);
			}
	}

#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
#ifdef SEMI_QOS
		// BE switch to VI checking
		if (QOS_ENABLE) {
			if (!priv->pmib->dot11OperationEntry.wifi_specific
				|| (priv->pmib->dot11OperationEntry.wifi_specific == 2)
				) {
				if (priv->BE_switch_to_VI &&
					((priv->pshare->phw->VO_pkt_count > 50) ||
					 (priv->pshare->phw->VI_pkt_count > 50) ||
					 (priv->pshare->phw->BK_pkt_count > 50))) {
					priv->BE_switch_to_VI = 0;
					switch_turbo++;
				}
				else if ((!priv->BE_switch_to_VI) &&
					((priv->pshare->phw->VO_pkt_count < 50) &&
					 (priv->pshare->phw->VI_pkt_count < 50) &&
					 (priv->pshare->phw->BK_pkt_count < 50))) {
					priv->BE_switch_to_VI++;
					switch_turbo++;
				}
			}

			if (priv->pmib->dot11OperationEntry.wifi_specific) {
				if (priv->BE_wifi_EDCA_enhance && (priv->pshare->phw->VO_pkt_count
					|| priv->pshare->phw->VI_pkt_count)) {
					if (priv->pmib->dot11OperationEntry.wifi_specific == 2)
						switch_turbo++;
					else
						RTL_W32(_ACBE_PARM_, 0x642b);
					priv->BE_wifi_EDCA_enhance = 0;
				}
				else if (!priv->BE_wifi_EDCA_enhance && !(priv->pshare->phw->VO_pkt_count
					|| priv->pshare->phw->VI_pkt_count)) {
					if (priv->pmib->dot11OperationEntry.wifi_specific == 2)
						switch_turbo++;
					else
						RTL_W32(_ACBE_PARM_, 0x632b);
					priv->BE_wifi_EDCA_enhance++;
				}
			}

			priv->pshare->phw->VO_pkt_count = 0;
			priv->pshare->phw->VI_pkt_count = 0;
			priv->pshare->phw->BK_pkt_count = 0;
		}
#endif

		if ((priv->up_time % 2) == 0) {
			// bcm old 11n chipset iot debug

#ifdef SEMI_QOS
			// TXOP enlarge
			if ((!priv->pmib->dot11OperationEntry.wifi_specific
				|| (priv->pmib->dot11OperationEntry.wifi_specific == 2)
				) && QOS_ENABLE) {
				if (highTP_found_pstat && highTP_found_pstat->rssi >= priv->pshare->rf_ft_var.txop_enlarge_upper) {
					if ((highTP_found_pstat->is_rtl8190_sta && priv->pshare->txop_enlarge != 2) ||
						((
						(!highTP_found_pstat->is_intel_sta && highTP_found_pstat->is_forced_rts) ||
						(highTP_found_pstat->is_intel_sta && !highTP_found_pstat->is_forced_rts) ||
						highTP_found_pstat->is_marvell_sta)
						&& priv->pshare->txop_enlarge != 1)) {
						if (highTP_found_pstat->is_rtl8190_sta)
							priv->pshare->txop_enlarge = 2;
						if (highTP_found_pstat->is_marvell_sta ||
							(!highTP_found_pstat->is_intel_sta && highTP_found_pstat->is_forced_rts))
							priv->pshare->txop_enlarge = 1;
						if (highTP_found_pstat->is_intel_sta && !highTP_found_pstat->is_forced_rts)
							priv->pshare->txop_enlarge = 0xe;
						if(highTP_found_pstat->is_rtl8192s_sta)
							priv->pshare->txop_enlarge = 0xf;

						if (priv->BE_switch_to_VI)
							switch_turbo++;
					}
				}
				else if (!highTP_found_pstat || highTP_found_pstat->rssi < priv->pshare->rf_ft_var.txop_enlarge_lower) {
					if (priv->pshare->txop_enlarge) {
						priv->pshare->txop_enlarge = 0;
						if (priv->BE_switch_to_VI)
							switch_turbo++;
					}
				}
			}
#endif

			priv->pshare->current_tx_bytes = 0;
			priv->pshare->current_rx_bytes = 0;
		}

#ifdef SEMI_QOS
		if (switch_turbo)
			BE_switch_to_VI(priv, priv->pmib->dot11BssType.net_work_type, priv->BE_switch_to_VI);
#endif
	}

	RESTORE_INT(flags);
}


#ifdef WDS
static void wds_probe_expire(struct rtl8190_priv *priv)
{
	struct stat_info	*pstat;
	unsigned int i;

	if ((priv->up_time % 30) != 5)
		return;

	for (i = 0; i < priv->pmib->dot11WdsInfo.wdsNum; i++) {
		pstat = get_stainfo(priv, priv->pmib->dot11WdsInfo.entry[i].macAddr);
		if (pstat) {
			if (pstat->wds_probe_done)
				continue;
			issue_probereq(priv, NULL, 0, pstat->hwaddr);
		}
	}
}
#endif


#ifdef CHECK_HANGUP
#ifdef CHECK_BB_HANGUP
int check_bb_hangup(struct rtl8190_priv *priv)
{
	unsigned long ioaddr = priv->pshare->ioaddr;
	unsigned int rx_rpt_OFDM, rx_rpt_CCK, rx_rpt_HT;

	// sel OFDM
	RTL_W8(RXERR_RPT+3, RTL_R8(RXERR_RPT+3) & 0x0F);
	// get counter
	rx_rpt_OFDM = RTL_R32(RXERR_RPT) & 0xfffff;

	// sel CCK
	RTL_W8(RXERR_RPT+3, RTL_R8(RXERR_RPT+3) | 0x40);
	// get counter
	rx_rpt_CCK = RTL_R32(RXERR_RPT) & 0xfffff;

	// sel HT
	RTL_W8(RXERR_RPT+3, RTL_R8(RXERR_RPT+3) & 0x0F);
	RTL_W8(RXERR_RPT+3, RTL_R8(RXERR_RPT+3) | 0x80);
	// get counter
	rx_rpt_HT = RTL_R32(RXERR_RPT) & 0xfffff;

	if ((priv->pshare->rx_rpt_ofdm != rx_rpt_OFDM) ||
		(priv->pshare->rx_rpt_cck != rx_rpt_CCK) ||
		(priv->pshare->rx_rpt_ht != rx_rpt_HT))
	{
		if (priv->pshare->rx_rpt_ofdm != rx_rpt_OFDM)
			priv->pshare->rx_rpt_ofdm = rx_rpt_OFDM;

		if (priv->pshare->rx_rpt_cck != rx_rpt_CCK)
			priv->pshare->rx_rpt_cck = rx_rpt_CCK;

		if (priv->pshare->rx_rpt_ht != rx_rpt_HT)
			priv->pshare->rx_rpt_ht = rx_rpt_HT;

		if (priv->pshare->successive_bb_hang)
			priv->pshare->successive_bb_hang = 0;
	}
	else
	{
		// reset BB
		DEBUG_ERR("BB hang, do reset now!!\n");
		RTL_W8(TXPAUSE, 0xff);
		RTL_W16(CMDR, 0x77fc);
		delay_us(10);
		RTL_W16(CMDR, 0x57fc);
		delay_us(10);
		RTL_W16(CMDR, 0x37fc);
		delay_us(10);
		RTL_W8(TXPAUSE, 0x00);
		priv->reset_cnt_bb++;
		priv->pshare->successive_bb_hang++;
	}

	if (priv->pshare->successive_bb_hang < 10)
		return 0;
	else
	{
		int n;

		// check MAC TX hang
		n = 0;
		RTL_W32(0x908, 0);
		while ((RTL_R32(0xdf4) & 0x00300000) != 0) {
			if ((n++) > 5) {
				RTL_W32(0x2c0, 0xf8000016);		// issue FW reset Tx command
				return 0;
			}
		}

		// check CCK CCA hang condition
		n = 0;
		while ((RTL_R32(0xdf4) & 0x00000004) != 0) {
			if ((n++) > 5)
				return 1;	// whole driver reset
		}

		// check OFDM CCA hang condition
		n = 0;
		while ((RTL_R32(0xdf4) & 0x00000002) != 0) {
			if ((n++) > 5)
				return 1;	// whole driver reset
		}

		return 0;
	}
}
#endif


#ifdef CHECK_TX_HANGUP
static int check_tx_hangup(struct rtl8190_priv *priv, int q_num, int *pTail, int *pIsEmpty)
{
	struct tx_desc	*pdescH, *pdesc;
	volatile int	head, tail;
	struct rtl8190_hw	*phw=GET_HW(priv);

	phw	= GET_HW(priv);
	head	= get_txhead(phw, q_num);
	tail	= get_txtail(phw, q_num);
	pdescH	= get_txdesc(phw, q_num);

	*pTail = tail;

	if (CIRC_CNT_RTK(head, tail, NUM_TX_DESC))
	{
		*pIsEmpty = 0;
		pdesc = pdescH + (tail);

#ifdef CONFIG_RTL865X
		pdesc = (struct tx_desc *)((unsigned int)pdesc|0xa0000000);
#else
#ifdef __MIPSEB__
		pdesc = (struct tx_desc *)KSEG1ADDR(pdesc);
#endif
#endif
		if (pdesc && ((get_desc(pdesc->Dword0)) & TX_OWN)) // pending
			return 1;
	}
	else
		*pIsEmpty = 1;

	return 0;
}
#endif


#ifdef CHECK_RX_HANGUP
static void check_rx_hangup_send_pkt(struct rtl8190_priv *priv)
{
	struct stat_info *pstat;
	struct list_head *phead = &priv->asoc_list;
	struct list_head *plist = phead->next;
	DECLARE_TXINSN(txinsn);

	while(plist != phead)
	{
		pstat = list_entry(plist, struct stat_info, asoc_list);
		plist = plist->next;

		if (pstat->expire_to > 0) {
			txinsn.retry = priv->pmib->dot11OperationEntry.dot11ShortRetryLimit;

			txinsn.q_num = MANAGE_QUE_NUM;
			txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
			txinsn.lowest_tx_rate = txinsn.tx_rate;
			txinsn.fixed_rate = 1;
			txinsn.phdr = get_wlanhdr_from_poll(priv);

			if (txinsn.phdr == NULL)
				goto send_test_pkt_fail;

			memset((void *)(txinsn.phdr), 0, sizeof (struct	wlan_hdr));

			SetFrameSubType(txinsn.phdr, WIFI_DATA_NULL);

			if (OPMODE & WIFI_AP_STATE) {
				memcpy((void *)GetAddr1Ptr((txinsn.phdr)), pstat->hwaddr, MACADDRLEN);
				memcpy((void *)GetAddr2Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
				memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
			}
			else {
				memcpy((void *)GetAddr1Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
				memcpy((void *)GetAddr2Ptr((txinsn.phdr)), pstat->hwaddr, MACADDRLEN);
				memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
			}

			txinsn.hdr_len = WLAN_HDR_A3_LEN;

			if ((rtl8190_firetx(priv, &txinsn)) == SUCCESS)
				return;

send_test_pkt_fail:

			if (txinsn.phdr)
				release_wlanhdr_to_poll(priv, txinsn.phdr);
		}
	}
}


static void check_rx_hangup(struct rtl8190_priv *priv)
{
	if (priv->rx_start_monitor_running == 0) {
		if (UINT32_DIFF(priv->rx_packets_pre1, priv->rx_packets_pre2) > 20) {
			priv->rx_start_monitor_running = 1;
			//printk("start monitoring = %d\n", priv->rx_start_monitor_running);
		}
	}
	else if (priv->rx_start_monitor_running == 1) {
		if (UINT32_DIFF(priv->net_stats.rx_packets, priv->rx_packets_pre1) == 0)
			priv->rx_start_monitor_running = 2;
		else if (UINT32_DIFF(priv->net_stats.rx_packets, priv->rx_packets_pre1) < 2 &&
			UINT32_DIFF(priv->net_stats.rx_packets, priv->rx_packets_pre1) > 0) {
			priv->rx_start_monitor_running = 0;
			//printk("stop monitoring = %d\n", priv->rx_start_monitor_running);
		}
	}

	if (priv->rx_start_monitor_running >= 2) {
		//printk("\n\n%s %d start monitoring = 2 rx_packets_pre1=%lu; rx_packets_pre2=%lu; net_stats.rx_packets=%lu\n",
			//__FUNCTION__, __LINE__,
			//priv->rx_packets_pre1, priv->rx_packets_pre2,
			//priv->net_stats.rx_packets);
		priv->pshare->rx_hang_checking = 1;
		priv->pshare->selected_priv = priv;;
	}
}


static __inline__ void check_rx_hangup_record_rxpkts(struct rtl8190_priv *priv)
{
	priv->rx_packets_pre2 = priv->rx_packets_pre1;
	priv->rx_packets_pre1 = priv->net_stats.rx_packets;
}
#endif // CHECK_RX_HANGUP


int check_hangup(struct rtl8190_priv *priv)
{
// Use static variables for stack size overflow issue, david+2007-09-19
	__DRAM_IN_865X static unsigned long	flags;
#ifdef CHECK_TX_HANGUP
	__DRAM_IN_865X static int tail, q_num, is_empty;
#endif
	__DRAM_IN_865X static int margin, txhangup, rxhangup, beacon_hangup, reset_fail_hangup, cca_hangup;
#if defined(RTL8190) || defined(CHECK_RX_HANGUP)
	unsigned long	ioaddr = priv->pshare->ioaddr;
#endif
#ifdef MBSSID
	__DRAM_IN_865X static int i;
#endif
#ifdef FAST_RECOVERY
	void *info = NULL;
#ifdef MBSSID
	void *vap_info[4] = {NULL, NULL, NULL, NULL};
#endif
#ifdef UNIVERSAL_REPEATER
	void *vxd_info = NULL;
#endif
#endif // FAST_RECOVERY
#ifdef CONFIG_RTL865X_WTDOG
#ifndef CONFIG_RTL8196B
	__DRAM_IN_865X static unsigned long wtval;
#endif
#endif

#ifdef CHECK_RX_HANGUP
	__DRAM_IN_865X static unsigned int	rx_cntreg;
#endif

/*
#if defined(CHECK_BEACON_HANGUP) && defined(RTL8192SE)
	unsigned int BcnQ_Val = 0;
	unsigned long ioaddr = priv->pshare->ioaddr;
#endif
*/

// for debug
//---------------------------------------------------------
	margin = -1;
	txhangup = rxhangup = beacon_hangup = reset_fail_hangup = cca_hangup = 0;


#ifdef CHECK_TX_HANGUP
	// we check Q0, Q4, Q3, Q2, Q1
	q_num = 0;

	while (q_num > margin) {
		if (check_tx_hangup(priv, q_num, &tail, &is_empty)) {
			if (priv->pshare->Q_info[q_num].pending_tick &&
				(tail == priv->pshare->Q_info[q_num].pending_tail) &&
				(UINT32_DIFF(priv->up_time, priv->pshare->Q_info[q_num].pending_tick) >= PENDING_PERIOD)) {
				// the stopping is over the period => hangup!
				txhangup++;
				break;
			}

			if ((priv->pshare->Q_info[q_num].pending_tick == 0) ||
				(tail != priv->pshare->Q_info[q_num].pending_tail)) {
				// the first time stopping or the tail moved
				priv->pshare->Q_info[q_num].pending_tick = priv->up_time;
				priv->pshare->Q_info[q_num].pending_tail = tail;
			}
			priv->pshare->Q_info[q_num].idle_tick = 0;
			break;
		}
		else {
			// empty or own bit is cleared
			priv->pshare->Q_info[q_num].pending_tick = 0;
			if (!is_empty &&
				priv->pshare->Q_info[q_num].idle_tick &&
				(tail == priv->pshare->Q_info[q_num].pending_tail) &&
				(UINT32_DIFF(priv->up_time, priv->pshare->Q_info[q_num].idle_tick) >= PENDING_PERIOD)) {
				// own bit is cleared, but the tail didn't move and is idle over the period => call DSR
				rtl8190_tx_dsr((unsigned long)priv);
				priv->pshare->Q_info[q_num].idle_tick = 0;
				break;
			}
			else {
				if (is_empty)
					priv->pshare->Q_info[q_num].idle_tick = 0;
				else {
					if ((priv->pshare->Q_info[q_num].idle_tick == 0) ||
						(tail != priv->pshare->Q_info[q_num].pending_tail)) {
						// the first time idle, or the own bit is cleared and the tail moved
						priv->pshare->Q_info[q_num].idle_tick = priv->up_time;
						priv->pshare->Q_info[q_num].pending_tail = tail;
						break;
					}
				}
			}
		}

		if(q_num == 0) {
			q_num = 4;
			margin = 0;
		}
		else
			q_num--;
	}
#endif

#ifdef CHECK_RX_HANGUP
	// check for rx stop
	if (txhangup == 0) {
		if ((priv->assoc_num > 0
#ifdef WDS
			|| priv->pmib->dot11WdsInfo.wdsEnabled
#endif
			) && !priv->pshare->rx_hang_checking)
			check_rx_hangup(priv);

#ifdef UNIVERSAL_REPEATER
		if (IS_DRV_OPEN(GET_VXD_PRIV(priv)) &&
			GET_VXD_PRIV(priv)->assoc_num > 0 &&
			!priv->pshare->rx_hang_checking)
			check_rx_hangup(GET_VXD_PRIV(priv));
#endif
#ifdef MBSSID
		if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
		{
			for (i=0; i<RTL8190_NUM_VWLAN; i++) {
				if (IS_DRV_OPEN(priv->pvap_priv[i]) &&
					priv->pvap_priv[i]->assoc_num > 0 &&
					!priv->pshare->rx_hang_checking)
					check_rx_hangup(priv->pvap_priv[i]);
			}
		}
#endif

		if (priv->pshare->rx_hang_checking)
		{
			if (priv->pshare->rx_cntreg_log == 0)
				priv->pshare->rx_cntreg_log = RTL_R32(_RXPKTNUM_);
			else {
				rx_cntreg = RTL_R32(_RXPKTNUM_);
				if (priv->pshare->rx_cntreg_log == rx_cntreg) {
					if (priv->pshare->rx_stop_pending_tick) {
						if (UINT32_DIFF(priv->pshare->selected_priv->up_time, priv->pshare->rx_stop_pending_tick) >= (PENDING_PERIOD-1)) {
							rxhangup++;
							//printk("\n\n%s %d rxhangup++ rx_packets_pre1=%lu; rx_packets_pre2=%lu; net_stats.rx_packets=%lu\n",
									//__FUNCTION__, __LINE__,
									//priv->pshare->selected_priv->rx_packets_pre1, priv->pshare->selected_priv->rx_packets_pre2,
									//priv->pshare->selected_priv->net_stats.rx_packets);
						}
					}
					else {
						priv->pshare->rx_stop_pending_tick = priv->pshare->selected_priv->up_time;
						RTL_W32(_RCR_, RTL_R32(_RCR_) | _ACF_);
						check_rx_hangup_send_pkt(priv->pshare->selected_priv);
						//printk("\n\ncheck_rx_hangup_send_pkt!\n");
						//printk("%s %d rx_packets_pre1=%lu; rx_packets_pre2=%lu; net_stats.rx_packets=%lu\n",
								//__FUNCTION__, __LINE__,
								//priv->pshare->selected_priv->rx_packets_pre1, priv->pshare->selected_priv->rx_packets_pre2,
								//priv->pshare->selected_priv->net_stats.rx_packets);
					}
				}
				else {
					//printk("\n\n%s %d Recovered!\n" ,__FUNCTION__, __LINE__);
					priv->pshare->rx_hang_checking = 0;
					priv->pshare->rx_cntreg_log = 0;
					priv->pshare->selected_priv = NULL;
					priv->rx_start_monitor_running = 0;
#ifdef UNIVERSAL_REPEATER
					if (IS_DRV_OPEN(GET_VXD_PRIV(priv)))
						GET_VXD_PRIV(priv)->rx_start_monitor_running = 0;
#endif
#ifdef MBSSID
					if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
					{
						for (i=0; i<RTL8190_NUM_VWLAN; i++) {
							if (IS_DRV_OPEN(priv->pvap_priv[i]))
								priv->pvap_priv[i]->rx_start_monitor_running = 0;
						}
					}
#endif

					if (priv->pshare->rx_stop_pending_tick) {
						priv->pshare->rx_stop_pending_tick = 0;
						RTL_W32(_RCR_, RTL_R32(_RCR_) & (~_ACF_));
					}
				}
			}
		}

		if (rxhangup == 0) {
			if (priv->assoc_num > 0)
				check_rx_hangup_record_rxpkts(priv);
			else if (priv->rx_start_monitor_running) {
				priv->rx_start_monitor_running = 0;
				//printk("stop monitoring = %d\n", priv->rx_start_monitor_running);
			}
#ifdef UNIVERSAL_REPEATER
			if (IS_DRV_OPEN(GET_VXD_PRIV(priv))) {
				if (GET_VXD_PRIV(priv)->assoc_num > 0)
					check_rx_hangup_record_rxpkts(GET_VXD_PRIV(priv));
				else if (GET_VXD_PRIV(priv)->rx_start_monitor_running) {
					GET_VXD_PRIV(priv)->rx_start_monitor_running = 0;
					//printk("stop monitoring = %d\n", GET_VXD_PRIV(priv)->rx_start_monitor_running);
				}
			}
#endif
#ifdef MBSSID
			if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
			{
				for (i=0; i<RTL8190_NUM_VWLAN; i++) {
					if (IS_DRV_OPEN(priv->pvap_priv[i])) {
						if (priv->pvap_priv[i]->assoc_num > 0)
							check_rx_hangup_record_rxpkts(priv->pvap_priv[i]);
						else if (priv->pvap_priv[i]->rx_start_monitor_running) {
							priv->pvap_priv[i]->rx_start_monitor_running = 0;
							//printk("stop monitoring = %d\n", priv->pvap_priv[i]->rx_start_monitor_running);
						}
					}
				}
			}
#endif
		}
	}
#endif // CHECK_RX_HANGUP

#ifdef CHECK_BEACON_HANGUP
	if (((OPMODE & WIFI_AP_STATE)
			&& !(OPMODE &WIFI_SITE_MONITOR)
			&& priv->pBeaconCapability	// beacon has init
#ifdef WDS
			&& !priv->pmib->dot11WdsInfo.wdsPure
#endif
			&& !priv->pmib->miscEntry.func_off
		   )
#ifdef UNIVERSAL_REPEATER
			|| ((OPMODE & WIFI_STATION_STATE) && GET_VXD_PRIV(priv) &&
						(GET_VXD_PRIV(priv)->drv_state & DRV_STATE_VXD_AP_STARTED))
#endif
		) {
		unsigned long beacon_ok;

#ifdef UNIVERSAL_REPEATER
		if (OPMODE & WIFI_STATION_STATE){
			beacon_ok = GET_VXD_PRIV(priv)->ext_stats.beacon_ok;
/*
#ifdef RTL8192SE
			BcnQ_Val = GET_VXD_PRIV(priv)->ext_stats.beaconQ_sts;
			GET_VXD_PRIV(priv)->ext_stats.beaconQ_sts = RTL_R32(0x120);
			if(BcnQ_Val == GET_VXD_PRIV(priv)->ext_stats.beaconQ_sts)
				beacon_hangup = 1;
#endif
*/
		}
		else
#endif
		{
			beacon_ok = priv->ext_stats.beacon_ok;
/*
#ifdef RTL8192SE
			BcnQ_Val = priv->ext_stats.beaconQ_sts;
			priv->ext_stats.beaconQ_sts = RTL_R32(0x120);// firmware beacon Q stats
			if(BcnQ_Val == priv->ext_stats.beaconQ_sts)
				beacon_hangup = 1;
#endif
*/
		}

		if (priv->pshare->beacon_wait_cnt == 0) {
			if (priv->pshare->beacon_ok_cnt == beacon_ok) {
				int threshold=1;
#if (defined(RTL8192SE)||defined(RTL8192SU)) && defined(MBSSID)
				if (priv->pmib->miscEntry.vap_enable)
					threshold=3;
#endif
				if (priv->pmib->dot11StationConfigEntry.dot11BeaconPeriod < 650)
					threshold = 0;
				if (priv->pshare->beacon_pending_cnt++ >= threshold)
					beacon_hangup = 1;
			}
			else {
				priv->pshare->beacon_ok_cnt =beacon_ok;
				if (priv->pshare->beacon_pending_cnt > 0)
					priv->pshare->beacon_pending_cnt = 0;
			}
		}
		else
			priv->pshare->beacon_wait_cnt--;
	}
#endif

	if (priv->pshare->reset_monitor_cnt_down > 0) {
		priv->pshare->reset_monitor_cnt_down--;
		if (priv->pshare->reset_monitor_rx_pkt_cnt == priv->net_stats.rx_packets)	{
//			if (priv->pshare->reset_monitor_pending++ > 1)
				reset_fail_hangup = 1;
		}
		else {
			priv->pshare->reset_monitor_rx_pkt_cnt = priv->net_stats.rx_packets;
			if (priv->pshare->reset_monitor_pending > 0)
				priv->pshare->reset_monitor_pending = 0;
		}
	}

#ifdef CHECK_BB_HANGUP
	if (((priv->up_time % 5) == 0) &&
		(priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
		priv->pmib->dot11OperationEntry.wifi_specific)
		cca_hangup = check_bb_hangup(priv);
#endif

	if (txhangup || rxhangup || beacon_hangup || reset_fail_hangup || cca_hangup) { // hangup happen
		priv->reset_hangup = 1;
#ifdef UNIVERSAL_REPEATER
		if (IS_DRV_OPEN(GET_VXD_PRIV(priv)))
			GET_VXD_PRIV(priv)->reset_hangup = 1;
#endif

		if (txhangup)
			priv->reset_cnt_tx++;
		else if (rxhangup)
			priv->reset_cnt_rx++;
		else if (beacon_hangup)
			priv->reset_cnt_bcn++;
		else if (reset_fail_hangup)
			priv->reset_cnt_rst++;
		else if (cca_hangup)
			priv->reset_cnt_cca++;

#ifdef HW_QUICK_INIT
		priv->pshare->hw_inited = 0;
#endif

// for debug

// Set flag to re-init WDS key in rtl8190_open()
#ifdef WDS
	if (priv->pmib->dot11WdsInfo.wdsEnabled &&
		(priv->pmib->dot11WdsInfo.wdsPrivacy == _TKIP_PRIVACY_ ||
			priv->pmib->dot11WdsInfo.wdsPrivacy == _CCMP_PRIVACY_) ) {
			int i;
			for (i=0; i<priv->pmib->dot11WdsInfo.wdsNum; i++)
				if (netif_running(priv->wds_dev[i]))
	 				priv->pmib->dot11WdsInfo.wdsMappingKeyLen[i]|=0x80000000;
	}
#endif
//----------------------------- david+2006-06-30

		PRINT_INFO("Tx/Rx hangup! Reset wlan driver Tx[%d] Rx[%d] ISR[%d] Bcnt[%d] Rst[%d] BB[%d]...\n",
			priv->reset_cnt_tx, priv->reset_cnt_rx, priv->reset_cnt_isr, priv->reset_cnt_bcn, priv->reset_cnt_rst, priv->reset_cnt_cca);

#ifdef CONFIG_RTL865X_WTDOG
#ifndef CONFIG_RTL8196B
	wtval = *((volatile unsigned long *)0xB800311C);
	*((volatile unsigned long *)0xB800311C) = 0xA5000000;	// disabe watchdog
#endif
#endif

		SAVE_INT_AND_CLI(flags);

// for debug

#ifdef FAST_RECOVERY
		info = backup_sta(priv);
#ifdef MBSSID
		if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
		{
			for (i=0; i<RTL8190_NUM_VWLAN; i++) {
				if (IS_DRV_OPEN(priv->pvap_priv[i]))
					vap_info[i] = backup_sta(priv->pvap_priv[i]);
				else
					vap_info[i] = NULL;
			}
		}
#endif
#ifdef UNIVERSAL_REPEATER
		if (IS_DRV_OPEN(GET_VXD_PRIV(priv))) {
			vxd_info = backup_sta(GET_VXD_PRIV(priv));
			GET_VXD_PRIV(priv)->pmib->dot11OperationEntry.keep_rsnie = 1;
		}
#endif
#endif // FAST_RECOVERY

		// bcm old 11n chipset iot debug

		priv->pmib->dot11OperationEntry.keep_rsnie = 1;
#ifdef MBSSID
		if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
		{
			for (i=0; i<RTL8190_NUM_VWLAN; i++) {
				if (IS_DRV_OPEN(priv->pvap_priv[i]))
					priv->pvap_priv[i]->pmib->dot11OperationEntry.keep_rsnie = 1;
			}
		}
#endif
		rtl8190_close(priv->dev);
		rtl8190_open(priv->dev);

#ifdef FAST_RECOVERY
		if (info)
			restore_backup_sta(priv, info);

#ifdef MBSSID
		if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
		{
			for (i=0; i<RTL8190_NUM_VWLAN; i++) {
				if (IS_DRV_OPEN(priv->pvap_priv[i]) && vap_info[i])
					restore_backup_sta(priv->pvap_priv[i], vap_info[i]);
			}
		}
#endif
#ifdef UNIVERSAL_REPEATER
		if (IS_DRV_OPEN(GET_VXD_PRIV(priv)) && vxd_info)
			restore_backup_sta(GET_VXD_PRIV(priv), vxd_info);
#endif
#endif // FAST_RECOVERY

// for debug

		priv->pshare->reset_monitor_cnt_down = 3;
		priv->pshare->reset_monitor_pending = 0;
		priv->pshare->reset_monitor_rx_pkt_cnt = priv->net_stats.rx_packets;


		RESTORE_INT(flags);

#ifdef CONFIG_RTL865X_WTDOG
#ifndef CONFIG_RTL8196B
	*((volatile unsigned long *)0xB800311C) |=  1 << 23;
	*((volatile unsigned long *)0xB800311C) = wtval;
#endif
#endif

		return 1;
	}
	else
		return 0;
}
#endif // CHECK_HANGUP


static struct ac_log_info *aclog_lookfor_entry(struct rtl8190_priv *priv, unsigned char *addr)
{
	int i, idx=-1;

	for (i=0; i<MAX_AC_LOG; i++) {
		if (priv->acLog[i].used == 0) {
			if (idx < 0)
				idx = i;
			continue;
		}
		if (!memcmp(priv->acLog[i].addr, addr, MACADDRLEN))
			break;
	}

	if ( i != MAX_AC_LOG)
		return (&priv->acLog[i]);

	if (idx >= 0)
		return (&priv->acLog[idx]);

	return NULL; // table full
}


static void aclog_update_entry(struct ac_log_info *entry, unsigned char *addr)
{
	if (entry->used == 0) {
		memcpy(entry->addr, addr, MACADDRLEN);
		entry->used = 1;
	}
	entry->cur_cnt++;
	entry->last_attack_time = jiffies;
}


static int aclog_check(struct rtl8190_priv *priv)
{
	int i, used=0;

	for (i=0; i<MAX_AC_LOG; i++) {
		if (priv->acLog[i].used) {
			used++;
			if (priv->acLog[i].cur_cnt != priv->acLog[i].last_cnt) {
#if defined(CONFIG_RTL8196B_TR)
				LOG_MSG_DROP("Unauthorized wireless PC try to connect;note:%02X:%02X:%02X:%02X:%02X:%02X;\n",
					priv->acLog[i].addr[0], priv->acLog[i].addr[1], priv->acLog[i].addr[2],
					priv->acLog[i].addr[3], priv->acLog[i].addr[4], priv->acLog[i].addr[5]);
#elif defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
				LOG_MSG_DROP("Unauthorized wireless PC try to connect;note:%02X:%02X:%02X:%02X:%02X:%02X;\n",
					priv->acLog[i].addr[0], priv->acLog[i].addr[1], priv->acLog[i].addr[2],
					priv->acLog[i].addr[3], priv->acLog[i].addr[4], priv->acLog[i].addr[5]);
#elif defined(CONFIG_RTL8196B_TLD)
				LOG_MSG_DEL("[WLAN access denied] from MAC: %02x:%02x:%02x:%02x:%02x:%02x,\n",
					priv->acLog[i].addr[0], priv->acLog[i].addr[1], priv->acLog[i].addr[2],
					priv->acLog[i].addr[3], priv->acLog[i].addr[4], priv->acLog[i].addr[5]);
#else
				LOG_MSG("A wireless client (%02X:%02X:%02X:%02X:%02X:%02X) was rejected due to access control for %d times in 5 minutes\n",
					priv->acLog[i].addr[0], priv->acLog[i].addr[1], priv->acLog[i].addr[2],
					priv->acLog[i].addr[3], priv->acLog[i].addr[4], priv->acLog[i].addr[5],
					priv->acLog[i].cur_cnt - priv->acLog[i].last_cnt);
#endif
				priv->acLog[i].last_cnt = priv->acLog[i].cur_cnt;
			}
			else { // no update, check expired entry
				if ((jiffies - priv->acLog[i].last_attack_time) > AC_LOG_EXPIRE) {
					memset(&priv->acLog[i], '\0', sizeof(struct ac_log_info));
					used--;
				}
			}
		}
	}

	return used;
}


#ifdef SEMI_QOS
static void get_AP_Qos_Info(struct rtl8190_priv *priv, unsigned char *temp)
{
	temp[0] = GET_EDCA_PARA_UPDATE;
	temp[0] &= 0x0f;
	if (APSD_ENABLE)
		temp[0] |= BIT(7);
}


static void get_STA_AC_Para_Record(struct rtl8190_priv *priv, unsigned char *temp)
{
//BE
	temp[0] = GET_STA_AC_BE_PARA.AIFSN;
	temp[0] &= 0x0f;
	if (GET_STA_AC_BE_PARA.ACM)
		temp[0] |= BIT(4);
	temp[1] = GET_STA_AC_BE_PARA.ECWmax;
	temp[1] <<= 4;
	temp[1] |= GET_STA_AC_BE_PARA.ECWmin;
	temp[2] = GET_STA_AC_BE_PARA.TXOPlimit % 256;
	temp[3] = GET_STA_AC_BE_PARA.TXOPlimit / 256; // 2^8 = 256, for one byte's range

//BK
	temp[4] = GET_STA_AC_BK_PARA.AIFSN;
	temp[4] &= 0x0f;
	if (GET_STA_AC_BK_PARA.ACM)
		temp[4] |= BIT(4);
	temp[4] |= BIT(5);
	temp[5] = GET_STA_AC_BK_PARA.ECWmax;
	temp[5] <<= 4;
	temp[5] |= GET_STA_AC_BK_PARA.ECWmin;
	temp[6] = GET_STA_AC_BK_PARA.TXOPlimit % 256;
	temp[7] = GET_STA_AC_BK_PARA.TXOPlimit / 256;

//VI
	temp[8] = GET_STA_AC_VI_PARA.AIFSN;
	temp[8] &= 0x0f;
	if (GET_STA_AC_VI_PARA.ACM)
		temp[8] |= BIT(4);
	temp[8] |= BIT(6);
	temp[9] = GET_STA_AC_VI_PARA.ECWmax;
	temp[9] <<= 4;
	temp[9] |= GET_STA_AC_VI_PARA.ECWmin;
	temp[10] = GET_STA_AC_VI_PARA.TXOPlimit % 256;
	temp[11] = GET_STA_AC_VI_PARA.TXOPlimit / 256;

//VO
	temp[12] = GET_STA_AC_VO_PARA.AIFSN;
	temp[12] &= 0x0f;
	if (GET_STA_AC_VO_PARA.ACM)
		temp[12] |= BIT(4);
	temp[12] |= BIT(5)|BIT(6);
	temp[13] = GET_STA_AC_VO_PARA.ECWmax;
	temp[13] <<= 4;
	temp[13] |= GET_STA_AC_VO_PARA.ECWmin;
	temp[14] = GET_STA_AC_VO_PARA.TXOPlimit % 256;
	temp[15] = GET_STA_AC_VO_PARA.TXOPlimit /256;
}


void init_WMM_Para_Element(struct rtl8190_priv *priv, unsigned char *temp)
{
	if (OPMODE & WIFI_AP_STATE) {
		memcpy(temp, WMM_PARA_IE, 6);
//Qos Info field
		get_AP_Qos_Info(priv, &temp[6]);
//AC Parameters
		get_STA_AC_Para_Record(priv, &temp[8]);
 	}
#ifdef CLIENT_MODE
	else if ((OPMODE & WIFI_STATION_STATE) ||(OPMODE & WIFI_ADHOC_STATE)) {  //  WMM STA
		memcpy(temp, WMM_IE, 6);
		temp[6] = 0x00;  //  set zero to WMM STA Qos Info field
	}
#endif
}


#ifdef CLIENT_MODE
#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
__NOMIPS16
#endif
static void process_WMM_para_ie(struct rtl8190_priv *priv, unsigned char *p)
{
	int ACI = (p[0] >> 5) & 0x03;
	if ((ACI >= 0) && (ACI <= 3)) {
		switch(ACI) {
			case 0:
				GET_STA_AC_BE_PARA.ACM = (p[0] >> 4) & 0x01;
				GET_STA_AC_BE_PARA.AIFSN = p[0] & 0x0f;
				GET_STA_AC_BE_PARA.ECWmin = p[1] & 0x0f;
				GET_STA_AC_BE_PARA.ECWmax = p[1] >> 4;
				GET_STA_AC_BE_PARA.TXOPlimit = le16_to_cpu(*(unsigned short *)(&p[2]));
				DEBUG_INFO("BE: ACM %d, AIFSN %d, ECWmin %d, ECWmax %d, TXOP %d\n",
					GET_STA_AC_BE_PARA.ACM, GET_STA_AC_BE_PARA.AIFSN,
					GET_STA_AC_BE_PARA.ECWmin, GET_STA_AC_BE_PARA.ECWmax,
					GET_STA_AC_BE_PARA.TXOPlimit);
				break;
			case 3:
				GET_STA_AC_VO_PARA.ACM = (p[0] >> 4) & 0x01;
				GET_STA_AC_VO_PARA.AIFSN = p[0] & 0x0f;
				GET_STA_AC_VO_PARA.ECWmin = p[1] & 0x0f;
				GET_STA_AC_VO_PARA.ECWmax = p[1] >> 4;
				GET_STA_AC_VO_PARA.TXOPlimit = le16_to_cpu(*(unsigned short *)(&p[2]));
				DEBUG_INFO("VO: ACM %d, AIFSN %d, ECWmin %d, ECWmax %d, TXOP %d\n",
					GET_STA_AC_VO_PARA.ACM, GET_STA_AC_VO_PARA.AIFSN,
					GET_STA_AC_VO_PARA.ECWmin, GET_STA_AC_VO_PARA.ECWmax,
					GET_STA_AC_VO_PARA.TXOPlimit);
				break;
			case 2:
				GET_STA_AC_VI_PARA.ACM = (p[0] >> 4) & 0x01;
				GET_STA_AC_VI_PARA.AIFSN = p[0] & 0x0f;
				GET_STA_AC_VI_PARA.ECWmin = p[1] & 0x0f;
				GET_STA_AC_VI_PARA.ECWmax = p[1] >> 4;
				GET_STA_AC_VI_PARA.TXOPlimit = le16_to_cpu(*(unsigned short *)(&p[2]));
				DEBUG_INFO("VI: ACM %d, AIFSN %d, ECWmin %d, ECWmax %d, TXOP %d\n",
					GET_STA_AC_VI_PARA.ACM, GET_STA_AC_VI_PARA.AIFSN,
					GET_STA_AC_VI_PARA.ECWmin, GET_STA_AC_VI_PARA.ECWmax,
					GET_STA_AC_VI_PARA.TXOPlimit);
				break;
			default:
				GET_STA_AC_BK_PARA.ACM = (p[0] >> 4) & 0x01;
				GET_STA_AC_BK_PARA.AIFSN = p[0] & 0x0f;
				GET_STA_AC_BK_PARA.ECWmin = p[1] & 0x0f;
				GET_STA_AC_BK_PARA.ECWmax = p[1] >> 4;
				GET_STA_AC_BK_PARA.TXOPlimit = le16_to_cpu(*(unsigned short *)(&p[2]));
				DEBUG_INFO("BK: ACM %d, AIFSN %d, ECWmin %d, ECWmax %d, TXOP %d\n",
					GET_STA_AC_BK_PARA.ACM, GET_STA_AC_BK_PARA.AIFSN,
					GET_STA_AC_BK_PARA.ECWmin, GET_STA_AC_BK_PARA.ECWmax,
					GET_STA_AC_BK_PARA.TXOPlimit);
				break;
		}
	}
	else
		printk("WMM AP EDCA Parameter IE error!\n");
}


static void default_WMM_para(struct rtl8190_priv *priv)
{
	GET_STA_AC_BE_PARA.ACM = 0;
	GET_STA_AC_BE_PARA.AIFSN = 3;
	GET_STA_AC_BE_PARA.ECWmin = 4;
	GET_STA_AC_BE_PARA.ECWmax = 10;
	GET_STA_AC_BE_PARA.TXOPlimit = 0;

	GET_STA_AC_BK_PARA.ACM = 0;
	GET_STA_AC_BK_PARA.AIFSN = 7;
	GET_STA_AC_BK_PARA.ECWmin = 4;
	GET_STA_AC_BK_PARA.ECWmax = 10;
	GET_STA_AC_BK_PARA.TXOPlimit = 0;

	GET_STA_AC_VI_PARA.ACM = 0;
	GET_STA_AC_VI_PARA.AIFSN = 2;
	GET_STA_AC_VI_PARA.ECWmin = 3;
	GET_STA_AC_VI_PARA.ECWmax = 4;
	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) ||
		(priv->pmib->dot11BssType.net_work_type & WIRELESS_11A))
		GET_STA_AC_VI_PARA.TXOPlimit = 94; // 3.008ms
	else
		GET_STA_AC_VI_PARA.TXOPlimit = 188; // 6.016ms

	GET_STA_AC_VO_PARA.ACM = 0;
	GET_STA_AC_VO_PARA.AIFSN = 2;
	GET_STA_AC_VO_PARA.ECWmin = 2;
	GET_STA_AC_VO_PARA.ECWmax = 3;
	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) ||
		(priv->pmib->dot11BssType.net_work_type & WIRELESS_11A))
		GET_STA_AC_VO_PARA.TXOPlimit = 47; // 1.504ms
	else
		GET_STA_AC_VO_PARA.TXOPlimit = 102; // 3.264ms
}


static void sta_config_EDCA_para(struct rtl8190_priv *priv)
{
	unsigned int slot_time = 20, ifs_time = 10;

	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N ) ||
		(priv->pmib->dot11BssType.net_work_type & WIRELESS_11G))
		slot_time = 9;

	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)
		ifs_time = 16;

	if(GET_STA_AC_VO_PARA.AIFSN > 0) {
		RTL_W32(_ACVO_PARM_, (((unsigned short)(GET_STA_AC_VO_PARA.TXOPlimit)) << 16)
			| (((unsigned char)(GET_STA_AC_VO_PARA.ECWmax)) << 12)
			| (((unsigned char)(GET_STA_AC_VO_PARA.ECWmin)) << 8)
			| (ifs_time + GET_STA_AC_VO_PARA.AIFSN * slot_time));
		if(GET_STA_AC_VO_PARA.ACM > 0)
			RTL_W8(_ACM_CTRL_, RTL_R8(_ACM_CTRL_)|BIT(3));
	}

	if(GET_STA_AC_VI_PARA.AIFSN > 0) {
		RTL_W32(_ACVI_PARM_, (((unsigned short)(GET_STA_AC_VI_PARA.TXOPlimit)) << 16)
			| (((unsigned char)(GET_STA_AC_VI_PARA.ECWmax)) << 12)
			| (((unsigned char)(GET_STA_AC_VI_PARA.ECWmin)) << 8)
			| (ifs_time + GET_STA_AC_VI_PARA.AIFSN * slot_time));
		if(GET_STA_AC_VI_PARA.ACM > 0)
			RTL_W8(_ACM_CTRL_, RTL_R8(_ACM_CTRL_)|BIT(2));
	}

	if(GET_STA_AC_BE_PARA.AIFSN > 0) {
		RTL_W32(_ACBE_PARM_, (((unsigned short)(GET_STA_AC_BE_PARA.TXOPlimit)) << 16)
			| (((unsigned char)(GET_STA_AC_BE_PARA.ECWmax)) << 12)
			| (((unsigned char)(GET_STA_AC_BE_PARA.ECWmin)) << 8)
			| (ifs_time + GET_STA_AC_BE_PARA.AIFSN * slot_time));
		if(GET_STA_AC_BE_PARA.ACM > 0)
			RTL_W8(_ACM_CTRL_, RTL_R8(_ACM_CTRL_)|BIT(1));
	}

	if(GET_STA_AC_BK_PARA.AIFSN > 0) {
		RTL_W32(_ACBK_PARM_, (((unsigned short)(GET_STA_AC_BK_PARA.TXOPlimit)) << 16)
			| (((unsigned char)(GET_STA_AC_BK_PARA.ECWmax)) << 12)
			| (((unsigned char)(GET_STA_AC_BK_PARA.ECWmin)) << 8)
			| (ifs_time + GET_STA_AC_BK_PARA.AIFSN * slot_time));
	}

	if ((GET_STA_AC_VO_PARA.ACM > 0) || (GET_STA_AC_VI_PARA.ACM > 0) || (GET_STA_AC_BE_PARA.ACM > 0))
		RTL_W8(_ACM_CTRL_, RTL_R8(_ACM_CTRL_)|BIT(0));

	priv->pmib->dot11QosEntry.EDCA_STA_config = 1;
}


static void reset_EDCA_para(struct rtl8190_priv *priv)
{
	memset((void *)&GET_STA_AC_VO_PARA, 0, sizeof(struct ParaRecord));
	memset((void *)&GET_STA_AC_VI_PARA, 0, sizeof(struct ParaRecord));
	memset((void *)&GET_STA_AC_BE_PARA, 0, sizeof(struct ParaRecord));
	memset((void *)&GET_STA_AC_BK_PARA, 0, sizeof(struct ParaRecord));

	init_EDCA_para(priv, priv->pmib->dot11BssType.net_work_type);

	priv->pmib->dot11QosEntry.EDCA_STA_config = 0;
}
#endif // CLIENT_MODE
#endif // SEMI_QOS


// Realtek proprietary IE
static void process_rtk_ie(struct rtl8190_priv *priv)
{
	struct stat_info *pstat;
	int use_long_slottime=0;
	unsigned int threshold;

	if ((priv->up_time % 3) != 0)
		return;

	if ((get_rf_mimo_mode(priv) == MIMO_1T2R) || (get_rf_mimo_mode(priv) == MIMO_1T1R))
		threshold = 50*1024*1024/8;
	else
		threshold = 100*1024*1024/8;

	if (OPMODE & WIFI_AP_STATE)
	{
		struct list_head *phead = &priv->asoc_list;
		struct list_head *plist = phead->next;

		while(plist != phead)
		{
			pstat = list_entry(plist, struct stat_info, asoc_list);
			plist = plist->next;

			if ((pstat->expire_to > 0) &&
				(/*priv->pshare->is_giga_exist ||*/ !pstat->is_2t_mimo_sta) &&
				 ((pstat->is_rtl8190_sta && !pstat->is_rtl8190_apclient && ((pstat->tx_avarage + pstat->rx_avarage) > threshold))
#ifdef WDS
				  || ((pstat->state & WIFI_WDS) && ((pstat->tx_avarage + pstat->rx_avarage) > (threshold*2/3)))
#endif
				  )) {
				use_long_slottime = 1;
				break;
			}
		}

		if (priv->pshare->use_long_slottime == 0) {
			if (use_long_slottime) {
				priv->pshare->use_long_slottime = 1;
				set_slot_time(priv, 0);
				priv->pmib->dot11ErpInfo.shortSlot = 0;
				RESET_SHORTSLOT_IN_BEACON_CAP;
				priv->pshare->rtk_ie_buf[5] |= RTK_CAP_IE_USE_LONG_SLOT;
			}
		}
		else {
			if (use_long_slottime == 0) {
				priv->pshare->use_long_slottime = 0;
				check_protection_shortslot(priv);
				priv->pshare->rtk_ie_buf[5] &= (~RTK_CAP_IE_USE_LONG_SLOT);
			}
		}
	}
}


void rtl8190_expire_timer(unsigned long task_priv)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)task_priv;
	unsigned long	flags;
	int i;

	// advance driver up timer
	priv->up_time++;

#ifdef CONFIG_RTK_MESH
	if ((GET_MIB(priv)->dot1180211sInfo.mesh_enable == 1 )
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)	// fix: 0000107 2008/10/13
			&& IS_ROOT_INTERFACE(priv)
#endif
	) {
		if( priv->mesh_log )	// advance flow log timer Throughput statistics (sounder)
			priv->log_time++;

		mesh_expire(priv);
	}

#ifdef  _11s_TEST_MODE_
	mesh_debug_sme1(priv);
#endif

#endif	// DOT11_MESH_MODE_

	// check auth_list
	auth_expire(priv);

	// check asoc_list
	assoc_expire(priv);

	// to avoid client mode assoc fail with TKIP/AES

#ifdef WIFI_SIMPLE_CONFIG
	// check wsc probe request list
	if (priv->pmib->wscEntry.wsc_enable & 2) // work as AP (not registrar)
		wsc_probe_expire(priv);
#endif

#ifdef WDS
	if ((OPMODE & WIFI_AP_STATE) && priv->pmib->dot11WdsInfo.wdsEnabled &&
		priv->pmib->dot11WdsInfo.wdsNum)
		wds_probe_expire(priv);
#endif

	// check link status and start/stop net queue
	priv->link_status = chklink_wkstaQ(priv);

	// for SW LED
	if ((LED_TYPE >= LEDTYPE_SW_LINK_TXRX) && (LED_TYPE < LEDTYPE_SW_MAX))
	{
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
		if (IS_ROOT_INTERFACE(priv))
#endif
			calculate_sw_LED_interval(priv);
	}

#ifdef CLIENT_MODE
	if (((OPMODE & WIFI_AP_STATE) ||
		((OPMODE & WIFI_ADHOC_STATE) &&
			((priv->join_res == STATE_Sta_Ibss_Active) || (priv->join_res == STATE_Sta_Ibss_Idle)))) &&
		(priv->pmib->dot11BssType.net_work_type & WIRELESS_11G))
#else
	if ((OPMODE & WIFI_AP_STATE) &&
		(priv->pmib->dot11BssType.net_work_type & WIRELESS_11G))
#endif
	{
		if (priv->pmib->dot11ErpInfo.olbcDetected) {
			if (priv->pmib->dot11ErpInfo.olbcExpired > 0)
				priv->pmib->dot11ErpInfo.olbcExpired--;

			if (priv->pmib->dot11ErpInfo.olbcExpired == 0) {
				priv->pmib->dot11ErpInfo.olbcDetected = 0;
				DEBUG_INFO("OLBC expired\n");
				check_protection_shortslot(priv);
			}
		}
	}

	SAVE_INT_AND_CLI(flags);

#ifdef DFS
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
		if (!priv->pmib->dot11DFSEntry.disable_DFS &&
			(OPMODE & WIFI_AP_STATE) &&
			((priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_MKK) ||
			 (priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_MKK3)) &&
			((priv->pmib->dot11RFEntry.dot11channel >= 52) &&
			 (priv->pmib->dot11RFEntry.dot11channel <= 64)))
		{
			if (!priv->pmib->dot11DFSEntry.disable_DetermineDFSDisable)
				DetermineDFSDisable(priv);
			else if (priv->pmib->dot11DFSEntry.disable_DetermineDFSDisable &&
				priv->pmib->dot11DFSEntry.temply_disable_DFS)
				priv->pmib->dot11DFSEntry.temply_disable_DFS = FALSE;
		}
	}
#endif

	// calculate tx/rx throughput
	priv->ext_stats.tx_avarage = (priv->ext_stats.tx_avarage/10)*7 + (priv->ext_stats.tx_byte_cnt/10)*3;
	priv->ext_stats.tx_byte_cnt = 0;
	priv->ext_stats.rx_avarage = (priv->ext_stats.rx_avarage/10)*7 + (priv->ext_stats.rx_byte_cnt/10)*3;
	priv->ext_stats.rx_byte_cnt = 0;

#ifdef CONFIG_RTL8190_THROUGHPUT
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
		unsigned long throughput;

		throughput = (priv->ext_stats.tx_avarage + priv->ext_stats.rx_avarage) * 8 / 1024 / 1024; /* unit: Mbps */
		if (gCpuCanSuspend) {
			if (throughput > TP_HIGH_WATER_MARK) {
				gCpuCanSuspend = 0;
			}
		}
		else {
			if (throughput < TP_LOW_WATER_MARK) {
				gCpuCanSuspend = 1;
			}
		}
	}
#endif

#if defined(CONFIG_RTL8196B_TR)
	if (priv->ext_stats.tx_avarage > priv->ext_stats.tx_peak)
		priv->ext_stats.tx_peak = priv->ext_stats.tx_avarage;

	if (priv->ext_stats.rx_avarage > priv->ext_stats.rx_peak)
		priv->ext_stats.rx_peak = priv->ext_stats.rx_avarage;
#endif
//#ifdef CONFIG_RTL865X_AC
#if defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
	if (priv->ext_stats.tx_avarage > priv->ext_stats.tx_peak)
		priv->ext_stats.tx_peak = priv->ext_stats.tx_avarage;

	if (priv->ext_stats.rx_avarage > priv->ext_stats.rx_peak)
		priv->ext_stats.rx_peak = priv->ext_stats.rx_avarage;
#endif

	RESTORE_INT(flags);

#ifdef CLIENT_MODE
	if (OPMODE & (WIFI_STATION_STATE | WIFI_ADHOC_STATE))
	{
		SAVE_INT_AND_CLI(flags);
		// calculate how many beacons we received and decide if should roaming
		calculate_rx_beacon(priv);

#ifdef RTK_BR_EXT
		// expire NAT2.5 entry
		nat25_db_expire(priv);

		if (priv->pppoe_connection_in_progress > 0)
			priv->pppoe_connection_in_progress--;
#endif
		RESTORE_INT(flags);
	}
#endif

#ifdef MP_TEST

	if (OPMODE & WIFI_MP_RX) {
		if (priv->pshare->rf_ft_var.rssi_dump) {
			printk("%d%%  (ss %d %d %d %d)(snr %d %d %d %d)(sq %d %d)\n",
				priv->pshare->mp_rssi,
				priv->pshare->mp_rf_info.mimorssi[0], priv->pshare->mp_rf_info.mimorssi[1], priv->pshare->mp_rf_info.mimorssi[2], priv->pshare->mp_rf_info.mimorssi[3],
				priv->pshare->mp_rf_info.RxSNRdB[0], priv->pshare->mp_rf_info.RxSNRdB[1], priv->pshare->mp_rf_info.RxSNRdB[2], priv->pshare->mp_rf_info.RxSNRdB[3],
				priv->pshare->mp_rf_info.mimosq[0], priv->pshare->mp_rf_info.mimosq[1]);
		}
	}
#endif

	// Realtek proprietary IE
	process_rtk_ie(priv);

	// check ACL log event
	if ((OPMODE & WIFI_AP_STATE) && priv->acLogCountdown > 0) {
		if (--priv->acLogCountdown == 0)
			if (aclog_check(priv) > 0) // still have active entry
				priv->acLogCountdown = AC_LOG_TIME;
	}

#ifdef CLIENT_MODE
	if (OPMODE & WIFI_AP_STATE)
#endif
	{
		// 11n protection count down
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
			if (priv->ht_legacy_obss_to > 0)
				priv->ht_legacy_obss_to--;
#ifdef WIFI_11N_2040_COEXIST
			if (priv->pshare->rf_ft_var.coexist_enable && priv->bg_ap_timeout)
				priv->bg_ap_timeout--;
#endif
		}
	}

	// Dump Rx FiFo overflow count
	if (priv->pshare->rf_ft_var.rxfifoO) {
		printk("RxFiFo Overflow: %d\n", (unsigned int)(priv->ext_stats.rx_fifoO - priv->pshare->rxFiFoO_pre));
		priv->pshare->rxFiFoO_pre = priv->ext_stats.rx_fifoO;
	}

#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
#ifdef SEMI_QOS
		if (QOS_ENABLE) {
#ifdef CLIENT_MODE
			if((OPMODE & WIFI_STATION_STATE) && (!priv->link_status) && (priv->pmib->dot11QosEntry.EDCA_STA_config)) {
				SAVE_INT_AND_CLI(flags);
				reset_EDCA_para(priv);
				RESTORE_INT(flags);
			}
#endif
		}
#endif

		// check rate info report by firmware

		// One-to-one dynamic mechanism
#ifdef MP_TEST
		if (!(OPMODE & WIFI_MP_STATE))
#endif
		if (priv->up_time % 2)
		{
			if (priv->assoc_num == 1)
			{
					// Tx power control
					if (priv->pshare->rf_ft_var.tx_pwr_ctrl)
						tx_power_control(priv, NULL, TRUE);
			}
			else
			{
				// Tx power control
				if (priv->pshare->rf_ft_var.tx_pwr_ctrl)
					tx_power_control(priv, NULL, FALSE);
			}
		}

#ifdef MP_TEST
		if (!(OPMODE & WIFI_MP_STATE))
#endif
		{
			if (priv->pmib->dot11RFEntry.ther)
			{
#if !defined(RTL8192SU_USE_FW_TPT)
				if ((priv->up_time % priv->pshare->rf_ft_var.tpt_period) == 0)
					tx_power_tracking(priv);
#endif
			}
		}

#if defined(RTL8192E) || defined(STA_EXT)
		if(fw_was_full(priv) && priv->pshare->fw_free_space > 0){ // there are free space for STA
			// do some algorithms to re-alloc STA into free space
			realloc_RATid(priv);
		}
#endif
	}

#ifdef USB_PKT_RATE_CTRL_SUPPORT
	usbPkt_timer_handler(priv);
#endif

#ifdef UNIVERSAL_REPEATER
	if (IS_ROOT_INTERFACE(priv) && GET_VXD_PRIV(priv) &&
			netif_running(GET_VXD_PRIV(priv)->dev))
		rtl8190_expire_timer((unsigned long)GET_VXD_PRIV(priv));
#endif

#ifdef MBSSID
	if (IS_ROOT_INTERFACE(priv)) {
		if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
		{
			for (i=0; i<RTL8190_NUM_VWLAN; i++) {
				if (IS_DRV_OPEN(priv->pvap_priv[i]))
					rtl8190_expire_timer((unsigned long)priv->pvap_priv[i]);
			}
		}
	}
#endif
}


/*
 *	@brief	System 1 sec timer
 *
 *	@param	task_priv: priv
 *
 *	@retval	void
 */
void rtl8190_1sec_timer(unsigned long task_priv)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)task_priv;

#ifdef CHECK_HANGUP
#ifdef MP_TEST
	if (!(OPMODE & WIFI_MP_STATE))
#endif
		if (check_hangup(priv))
			return;
#endif

	// for Rx dynamic tasklet
	if (priv->pshare->rxInt_data_delta > priv->pmib->miscEntry.rxInt_thrd)
		priv->pshare->rxInt_useTsklt = TRUE;
	else
		priv->pshare->rxInt_useTsklt = FALSE;
	priv->pshare->rxInt_data_delta = 0;

	tasklet_schedule(&priv->pshare->oneSec_tasklet);

	mod_timer(&priv->expire_timer, jiffies + EXPIRE_TO);
}


void pwr_state(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned long	flags;
	struct stat_info *pstat;
	unsigned char	*sa, *pframe;

	pframe = get_pframe(pfrinfo);
	sa = pfrinfo->sa;
	pstat = get_stainfo(priv, sa);

	if (pstat == (struct stat_info *)NULL)
		return;

	if (!(pstat->state & WIFI_ASOC_STATE))
		return;

	if (GetPwrMgt(pframe))
	{
		if ((pstat->state & WIFI_SLEEP_STATE) == 0)
			pstat->state |= WIFI_SLEEP_STATE;
		if (!list_empty(&pstat->wakeup_list))
		{
			SAVE_INT_AND_CLI(flags);
			list_del_init(&pstat->wakeup_list);
			RESTORE_INT(flags);
			DEBUG_INFO("Del fr wakeup_list %02X%02X%02X%02X%02X%02X\n", sa[0],sa[1],sa[2],sa[3],sa[4],sa[5]);
		}
		if (list_empty(&pstat->sleep_list))
		{
			SAVE_INT_AND_CLI(flags);
			list_add_tail(&(pstat->sleep_list), &(priv->sleep_list));
			RESTORE_INT(flags);
			DEBUG_INFO("Add to sleep_list %02X%02X%02X%02X%02X%02X\n", sa[0],sa[1],sa[2],sa[3],sa[4],sa[5]);
		}
	}
	else
	{
		if (pstat->state | WIFI_SLEEP_STATE)
			pstat->state &= ~(WIFI_SLEEP_STATE);
		if (!list_empty(&pstat->sleep_list))
		{
			SAVE_INT_AND_CLI(flags);
			list_del_init(&pstat->sleep_list);
			RESTORE_INT(flags);
			DEBUG_INFO("Del fr sleep_list %02X%02X%02X%02X%02X%02X\n", sa[0],sa[1],sa[2],sa[3],sa[4],sa[5]);
		}
		if ((skb_queue_len(&pstat->dz_queue))
#if defined(SEMI_QOS) && defined(WMM_APSD)
			||((QOS_ENABLE) && (APSD_ENABLE) && (pstat->QosEnabled) && (pstat->apsd_pkt_buffering) &&
				((!isFFempty(pstat->VO_dz_queue->head, pstat->VO_dz_queue->tail)) ||
				 (!isFFempty(pstat->VI_dz_queue->head, pstat->VI_dz_queue->tail)) ||
				 (!isFFempty(pstat->BE_dz_queue->head, pstat->BE_dz_queue->tail)) ||
				 (!isFFempty(pstat->BK_dz_queue->head, pstat->BK_dz_queue->tail))))
#endif
		) {
			if (list_empty(&pstat->wakeup_list))
			{
				SAVE_INT_AND_CLI(flags);
				list_add_tail(&pstat->wakeup_list, &priv->wakeup_list);
				RESTORE_INT(flags);
				DEBUG_INFO("Add to wakeup_list %02X%02X%02X%02X%02X%02X\n", sa[0],sa[1],sa[2],sa[3],sa[4],sa[5]);
			}
		}
	}
	return;
}


void mgt_handler(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo)
{
	struct mlme_handler *ptable;
	unsigned int index;
	unsigned char *pframe = get_pframe(pfrinfo);
	unsigned char *sa = pfrinfo->sa;
	unsigned char *da = pfrinfo->da;
	struct stat_info *pstat = NULL;


	if (OPMODE & WIFI_AP_STATE)
		ptable = mlme_ap_tbl;
#ifdef CLIENT_MODE
	else if (OPMODE & (WIFI_STATION_STATE | WIFI_ADHOC_STATE))
		ptable = mlme_station_tbl;
#endif
	else
	{
		DEBUG_ERR("Currently we do not support opmode=%d\n", OPMODE);
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
		if (IS_ROOT_INTERFACE(priv) || (pfrinfo->is_br_mgnt == 0))
#endif
		rtl_kfree_skb(priv, pfrinfo->pskb, _SKB_RX_);
		return;
	}

	index = GetFrameSubType(pframe) >> 4;
	if (index > 13)
	{
		DEBUG_ERR("Currently we do not support reserved sub-fr-type=%d\n", index);
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
		if (IS_ROOT_INTERFACE(priv) || (pfrinfo->is_br_mgnt == 0))
#endif
		rtl_kfree_skb(priv, pfrinfo->pskb, _SKB_RX_);
		return;
	}
	ptable += index;

#ifdef CONFIG_RTK_MESH
	if( is_11s_mgt_frame(ptable->num, priv, pfrinfo))
	{
		pfrinfo->is_br_mgnt = 0;
		pfrinfo->is_11s = 1;
		ptable = mlme_mp_tbl;
		ptable += index;
#ifdef MBSSID
		if(!IS_VAP_INTERFACE(priv))
#endif
			// An 11s mgt frame will have Addr3 = 00..00, it might be dispatched to vxd in validate_mpdu
			// Hence, we have to "correct" it here.
			priv = GET_ROOT(priv);
	}
#endif // CONFIG_RTK_MESH

	if (!IS_MCAST(da))
	{
		pstat = get_stainfo(priv, sa);

		// only check last cache seq number for management frame, david -------------------------
		if (pstat != NULL) {
			if (GetRetry(pframe)) {
				if (GetTupleCache(pframe) == pstat->tpcache_mgt) {
					priv->ext_stats.rx_decache++;
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
					if (IS_ROOT_INTERFACE(priv) || (pfrinfo->is_br_mgnt == 0))
#endif
					rtl_kfree_skb(priv, pfrinfo->pskb, _SKB_RX_);
					SNMP_MIB_INC(dot11FrameDuplicateCount, 1);
					return;
				}
			}
			pstat->tpcache_mgt = GetTupleCache(pframe);
		}
	}

	// log rx statistics...
#ifdef WDS
	if (pstat && (pstat->state & WIFI_WDS) && (ptable->num == WIFI_BEACON)) {
		rx_sum_up(NULL, pstat, pfrinfo->pktlen, 0);
		update_sta_rssi(priv, pstat, pfrinfo);
	}
	else
#endif
#ifdef CONFIG_RTK_MESH
	if (pstat && pfrinfo->is_11s && (ptable->num == WIFI_BEACON)) {
		// count statistics for mesh points -- chris
		rx_sum_up(NULL, pstat, pfrinfo->pktlen, 0);
		update_sta_rssi(priv, pstat, pfrinfo);
	}
	else
#endif
	if (pstat != NULL)
	{
		// If AP mode and rx is a beacon, do not count in statistics. david
		if (!((OPMODE & WIFI_AP_STATE) && (ptable->num == WIFI_BEACON)))
		{
			rx_sum_up(NULL, pstat, pfrinfo->pktlen, 0);
			update_sta_rssi(priv, pstat, pfrinfo);
		}
	}

	// check power save state
	if ((OPMODE & WIFI_AP_STATE) && (pstat != NULL)) {
		if (IS_BSSID(priv, GetAddr1Ptr(pframe)))
			pwr_state(priv, pfrinfo);
	}

#ifdef MBSSID
	if (
		GET_ROOT(priv)->pmib->miscEntry.vap_enable &&
		IS_VAP_INTERFACE(priv)) {
		if (IS_MCAST(da) || !memcmp(BSSID, da, MACADDRLEN))
			ptable->func(priv, pfrinfo);
	}
	else
#endif
	ptable->func(priv, pfrinfo);

#ifdef MBSSID
	if (IS_VAP_INTERFACE(priv)) {
		if (pfrinfo->is_br_mgnt) {
			rx_sum_up(priv, NULL, pfrinfo->pktlen, GetRetry(pframe));
			return;
		}
	}
	else if (IS_ROOT_INTERFACE(priv) && pfrinfo->is_br_mgnt && (OPMODE & WIFI_AP_STATE)) {
		int i;
		for (i=0; i<RTL8190_NUM_VWLAN; i++) {
			if ((IS_DRV_OPEN(priv->pvap_priv[i])) && ((IS_MCAST(da)) ||
				(!memcmp(priv->pvap_priv[i]->pmib->dot11StationConfigEntry.dot11Bssid, da, MACADDRLEN))))
				mgt_handler(priv->pvap_priv[i], pfrinfo);
		}
	}
#endif

#ifdef UNIVERSAL_REPEATER
	if (pfrinfo->is_br_mgnt) {
		pfrinfo->is_br_mgnt = 0;

// fix hang-up issue when root-ap (A+B) + vxd-client ------------
		if (IS_DRV_OPEN(GET_VXD_PRIV(priv))) {
			if ((OPMODE & WIFI_AP_STATE) ||
				((OPMODE & WIFI_STATION_STATE) &&
				GET_VXD_PRIV(priv) && (GET_VXD_PRIV(priv)->drv_state & DRV_STATE_VXD_AP_STARTED))) {
//--------------------------------------------david+2006-07-17

				mgt_handler(GET_VXD_PRIV(priv), pfrinfo);
				return;
			}
		}
	}
#endif

	rtl_kfree_skb(priv, pfrinfo->pskb, _SKB_RX_);
}


/*----------------------------------------------------------------------------
// the purpose of this sub-routine
Any station has changed from sleep to active state, and has data buffer should
be dequeued here!
-----------------------------------------------------------------------------*/
void process_dzqueue(struct rtl8190_priv *priv)
{
	struct stat_info *pstat;
	struct sk_buff *pskb;
	struct list_head *phead = &priv->wakeup_list;
	struct list_head *plist = phead->next;
	unsigned long flags;

	while(plist != phead)
	{
		pstat = list_entry(plist, struct stat_info, wakeup_list);
		plist = plist->next;

		while(1)
		{
#if defined(SEMI_QOS) && defined(WMM_APSD)
			if ((QOS_ENABLE) && (APSD_ENABLE) && pstat && (pstat->QosEnabled) && (pstat->apsd_pkt_buffering)) {
				pskb = (struct sk_buff *)deque(priv, &(pstat->VO_dz_queue->head), &(pstat->VO_dz_queue->tail),
					(unsigned int)(pstat->VO_dz_queue->pSkb), NUM_APSD_TXPKT_QUEUE);
				if (pskb == NULL) {
					pskb = (struct sk_buff *)deque(priv, &(pstat->VI_dz_queue->head), &(pstat->VI_dz_queue->tail),
						(unsigned int)(pstat->VI_dz_queue->pSkb), NUM_APSD_TXPKT_QUEUE);
					if (pskb == NULL) {
						pskb = (struct sk_buff *)deque(priv, &(pstat->BE_dz_queue->head), &(pstat->BE_dz_queue->tail),
							(unsigned int)(pstat->BE_dz_queue->pSkb), NUM_APSD_TXPKT_QUEUE);
						if (pskb == NULL) {
							pskb = (struct sk_buff *)deque(priv, &(pstat->BK_dz_queue->head), &(pstat->BK_dz_queue->tail),
								(unsigned int)(pstat->BK_dz_queue->pSkb), NUM_APSD_TXPKT_QUEUE);
							if (pskb == NULL) {
								pstat->apsd_pkt_buffering = 0;
								goto legacy_ps;
							}
							DEBUG_INFO("release BK pkt\n");
						}
						else {
							DEBUG_INFO("release BE pkt\n");
						}
					}
					else {
						DEBUG_INFO("release VI pkt\n");
					}
				}
				else {
					DEBUG_INFO("release VO pkt\n");
				}
			}
			else
legacy_ps:
#endif
			pskb = __skb_dequeue(&pstat->dz_queue);

			if (pskb == NULL)
				break;

#ifdef ENABLE_RTL_SKB_STATS
			rtl_atomic_dec(&priv->rtl_tx_skb_cnt);
#endif

#ifdef CONFIG_RTK_MESH
			if (rtl8190_start_xmit(pskb, pskb->dev))
#else
			if (rtl8190_start_xmit(pskb, priv->dev))
#endif

				rtl_kfree_skb(priv, pskb, _SKB_TX_);
		}

		if (!list_empty(&pstat->wakeup_list))
		{
			SAVE_INT_AND_CLI(flags);
			list_del_init(&pstat->wakeup_list);
			RESTORE_INT(flags);
			DEBUG_INFO("Del fr wakeup_list %02X%02X%02X%02X%02X%02X\n",
				pstat->hwaddr[0],pstat->hwaddr[1],pstat->hwaddr[2],pstat->hwaddr[3],pstat->hwaddr[4],pstat->hwaddr[5]);
		}
	}
}


#ifdef SW_MCAST
void process_mcast_dzqueue(struct rtl8190_priv *priv)
{
	struct sk_buff *pskb;

	priv->release_mcast = 1;
	while(1) {
		pskb = (struct sk_buff *)deque(priv, &(priv->dz_queue.head), &(priv->dz_queue.tail),
			(unsigned int)(priv->dz_queue.pSkb), NUM_TXPKT_QUEUE);

		if (pskb == NULL)
			break;
// stanley: I think using pskb->dev is correct IN THE FUTURE, when mesh0 also applies dzqueue
#ifdef CONFIG_RTK_MESH
		if (rtl8190_start_xmit(pskb, pskb->dev))
#else
		if (rtl8190_start_xmit(pskb, priv->dev))
#endif
			rtl_kfree_skb(priv, pskb, _SKB_TX_);
	}
	priv->release_mcast = 0;
}
#endif


#ifndef CONFIG_RTK_MESH
static
#endif
 int check_basic_rate(struct rtl8190_priv *priv, unsigned char *pRate, int pLen)
{
	int i, match, idx;
	UINT8 rate;

	// david, check if is there is any basic rate existed --
	int any_one_basic_rate_found = 0;

	for (i=0; i<pLen; i++) {
		if (pRate[i] & 0x80) {
			any_one_basic_rate_found = 1;
			break;
		}
	}

	for (i=0; i<AP_BSSRATE_LEN; i++) {
		if (AP_BSSRATE[i] & 0x80) {
			rate = AP_BSSRATE[i] & 0x7f;
			match = 0;
			for (idx=0; idx<pLen; idx++) {
				if ((pRate[idx] & 0x7f) == rate) {
					if (pRate[idx] & 0x80)
						match = 1;
					else {
						if (!any_one_basic_rate_found) {
							pRate[idx] |= 0x80;
							match = 1;
						}
					}
				}
			}
			if (match == 0)
				return FAIL;
		}
	}
	return SUCCESS;
}


// which: 0: set basic rates as mine, 1: set basic rates as peer's
#ifndef CONFIG_RTK_MESH
static
#endif
 void get_matched_rate(struct rtl8190_priv *priv, unsigned char *pRate, int *pLen, int which)
{
	int i, j, num=0;
	UINT8 rate;
	UINT8 found_rate[32];

	for (i=0; i<AP_BSSRATE_LEN; i++) {
		// see if supported rate existed and matched
		rate = AP_BSSRATE[i] & 0x7f;
		if (match_supp_rate(pRate, *pLen, rate)) {
			if (!which) {
				if (AP_BSSRATE[i] & 0x80)
					rate |= 0x80;
			}
			else {
				for (j=0; j<*pLen; j++) {
					if (rate == (pRate[j] & 0x7f)) {
						if (pRate[j] & 0x80)
							rate |= 0x80;
						break;
					}
				}
			}
			found_rate[num++] = rate;
		}
	}

	if (which) {
		for (i=0; i<num; i++) {
			if (found_rate[i] & 0x80)
				break;
		}
		if (i == num) { // no any basic rates in found_rate
			j = 0;
			while(pRate[j] & 0x80)
				j++;
			memcpy(&(pRate[j]), found_rate, num);
			*pLen = j + num;
			return;
		}
	}

	memcpy(pRate, found_rate, num);
	*pLen = num;
}


#ifndef CONFIG_RTK_MESH
static
#endif
 void update_support_rate(struct	stat_info *pstat, unsigned char* buf, int len)
{
	memset(pstat->bssrateset, 0, sizeof(pstat->bssrateset));
	pstat->bssratelen=len;
	memcpy(pstat->bssrateset, buf, len);
}


int isErpSta(struct	stat_info *pstat)
{
	int i, len=pstat->bssratelen;
	UINT8 *buf=pstat->bssrateset;

	for (i=0; i<len; i++) {
		if ( ((buf[i] & 0x7f) != 2) &&
				((buf[i] & 0x7f) != 4) &&
				((buf[i] & 0x7f) != 11) &&
				((buf[i] & 0x7f) != 22) )
			return 1;	// ERP sta existed
	}
	return 0;
}


/*----------------------------------------------------------------------------
index: the information element id index, limit is the limit for search
-----------------------------------------------------------------------------*/
/**
 *	@brief	Get Information Element
 *
 *		p (Find ID in limit)		\n
 *	+--- -+------------+-----+---	\n
 *	| ... | element ID | len |...	\n
 *	+--- -+------------+-----+---	\n
 *
 *	@param	pbuf	frame data for search
 *	@param	index	the information element id = index (search target)
 *	@param	limit	limit for search
 *
 *	@retval	p	pointer to element ID
 *	@retval	len	p(IE) len
 */
#ifndef CONFIG_RTK_MESH
static
#endif
 unsigned char *get_ie(unsigned char *pbuf, int index, int *len, int limit)
{
	unsigned int tmp,i;
	unsigned char *p;

	if (limit < 1)
		return NULL;

	p = pbuf;
	i = 0;
	*len = 0;
	while(1)
	{
		if (*p == index)
		{
			*len = *(p + 1);
			return (p);
		}
		else
		{
			tmp = *(p + 1);
			p += (tmp + 2);
			i += (tmp + 2);
		}
		if (i >= limit)
			break;
	}
	return NULL;
}


#ifdef RTL_WPA2
/*----------------------------------------------------------------------------
index: the information element id index, limit is the limit for search
-----------------------------------------------------------------------------*/
static unsigned char *get_rsn_ie(struct rtl8190_priv *priv, unsigned char *pbuf, int *len, int limit)
{
	unsigned char *p = NULL;

	if (priv->pmib->dot11RsnIE.rsnielen == 0)
		return NULL;

	if ((priv->pmib->dot11RsnIE.rsnie[0] == _RSN_IE_2_) ||
		((priv->pmib->dot11RsnIE.rsnielen > priv->pmib->dot11RsnIE.rsnie[1]) &&
			(priv->pmib->dot11RsnIE.rsnie[priv->pmib->dot11RsnIE.rsnie[1]+2] == _RSN_IE_2_))) {
		p = get_ie(pbuf, _RSN_IE_2_, len, limit);
		if (p != NULL)
			return p;
		else
			return get_ie(pbuf, _RSN_IE_1_, len, limit);
	}
	else {
		p = get_ie(pbuf, _RSN_IE_1_, len, limit);
		if (p != NULL)
			return p;
		else
			return get_ie(pbuf, _RSN_IE_2_, len, limit);
	}
}
#endif


/**
 *	@brief	Set Information Element
 *
 *	Difference between set_fixed_ie, reserve 2 Byte, for Element ID & length \n
 *	\n
 *	+-------+       +------------+--------+----------------+	\n
 *	| pbuf. | <---  | element ID | length |     source     |	\n
 *	+-------+       +------------+--------+----------------+	\n
 *
 *	@param pbuf		buffer(frame) for set
 *	@param index	IE element ID
 *	@param len		IE length content & set length
 *	@param source	IE data for buffer set
 *	@param frlen	total frame length
 *
 *	@retval	pbuf+len+2	pointer of buffer tail.(+2 because element ID and length total 2 bytes)
 */
#ifndef CONFIG_RTK_MESH
static
#endif
 unsigned char *set_ie(unsigned char *pbuf, int index, unsigned int len, unsigned char *source,
				unsigned int *frlen)
{
	*pbuf = index;
	*(pbuf + 1) = len;
	if (len > 0)
		memcpy((void *)(pbuf + 2), (void *)source, len);
	*frlen = *frlen + (len + 2);
	return (pbuf + len + 2);
}


static __inline__ int set_virtual_bitmap(unsigned char *pbuf, unsigned int i)
{
	unsigned int r,s,t;

	r = (i >> 3) << 3;
	t = i - r;

	s = BIT(t);

	*(pbuf + (i >> 3)) |= s;

	return	(i >> 3);
}


static __inline__ unsigned char *update_tim(struct rtl8190_priv *priv,
				unsigned char *bcn_buf, unsigned int *frlen)
{
	unsigned int	i, set_pvb;
	unsigned char	val8;
	unsigned long	flags;
	struct list_head	*plist, *phead;
	struct stat_info	*pstat;
	unsigned char	bitmap[(NUM_STAT/8)+1];
	unsigned char	*pbuf= bcn_buf;
	unsigned char	N1, N2, bitmap_offset;

	memset(bitmap, 0, sizeof(bitmap));

#ifdef MBSSID
	if (
		GET_ROOT(priv)->pmib->miscEntry.vap_enable &&
		IS_VAP_INTERFACE(priv))
		priv->dtimcount = GET_ROOT_PRIV(priv)->dtimcount;
	else
#endif
	{
		if (priv->dtimcount == 0)
			priv->dtimcount = (priv->pmib->dot11StationConfigEntry.dot11DTIMPeriod - 1);
		else
			priv->dtimcount--;
	}

	if (priv->pkt_in_dtimQ && (priv->dtimcount == 0))
		val8 = 0x01;
	else
		val8 = 0x00;

	*pbuf = _TIM_IE_;
#if !defined(RTL8192SU_FWBCN)
	*(pbuf + 2) = priv->dtimcount;
#endif
	*(pbuf + 3) = priv->pmib->dot11StationConfigEntry.dot11DTIMPeriod;

	phead = &priv->sleep_list;
	plist = phead->next;

	SAVE_INT_AND_CLI(flags);
	while(plist != phead)
	{
		pstat = list_entry(plist, struct stat_info, sleep_list);
		plist = plist->next;
		set_pvb = 0;
#if defined(SEMI_QOS) && defined(WMM_APSD)
		if ((QOS_ENABLE) && (APSD_ENABLE) && (pstat) && ((pstat->apsd_bitmap & 0x0f) == 0x0f) &&
			((!isFFempty(pstat->VO_dz_queue->head, pstat->VO_dz_queue->tail)) ||
			 (!isFFempty(pstat->VI_dz_queue->head, pstat->VI_dz_queue->tail)) ||
			 (!isFFempty(pstat->BE_dz_queue->head, pstat->BE_dz_queue->tail)) ||
			 (!isFFempty(pstat->BK_dz_queue->head, pstat->BK_dz_queue->tail)))) {
			set_pvb++;
		}
		else
#endif
		if (skb_queue_len(&pstat->dz_queue))
			set_pvb++;

		if (set_pvb) {
			i = pstat->aid;
			i = set_virtual_bitmap(bitmap, i);
		}
	}
	RESTORE_INT(flags);

	N1 = 0;
	for(i=0; i<(NUM_STAT/8)+1; i++) {
		if(bitmap[i] != 0) {
			N1 = i;
			break;
		}
	}
	N2 = N1;
	for(i=(NUM_STAT/8); i>N1; i--) {
		if(bitmap[i] != 0) {
			N2 = i;
			break;
		}
	}

	// N1 should be an even number
	N1 = (N1 & 0x01)? (N1-1) : N1;
	bitmap_offset = N1 >> 1;	// == N1/2
	*(pbuf + 1) = N2 - N1 + 4;
	*(frlen) = *frlen + *(pbuf + 1) + 2;

	*(pbuf + 4) = val8 | (bitmap_offset << 1);
	memcpy((void *)(pbuf + 5), &bitmap[N1], (N2-N1+1));
#ifdef 	RTL8192SU_FWBCN
	if (memcmp(bitmap, priv->bitmap_tmp, (NUM_STAT>>3)+1))
	{
		memcpy((void *)(priv->bitmap_tmp), (void *)(&bitmap), (NUM_STAT>>3)+1);
		priv->update_bcn++;
		//printk("TIM update!!\n");
	}
#endif

	return (bcn_buf + *(pbuf + 1) + 2);
}

/**
 *	@brief	set fixed information element
 *
 *	set_fixed is haven't Element ID & length, Total length is frlen. \n
 *					 len	\n
 *	+-----------+-----------+	\n
 *	|	pbuf	|   source  |	\n
 *	+-----------+-----------+	\n
 *
 *	@param	pbuf	buffer(frame) for set
 *	@param	len		IE set length
 *	@param	source	IE data for buffer set
 *	@param	frlen	total frame length (Note: frlen have side effect??)
 *
 *	@retval	pbuf+len	pointer of buffer tail. \n
 */
#ifndef CONFIG_RTK_MESH
static
#endif
 unsigned char *set_fixed_ie(unsigned char *pbuf, unsigned int len, unsigned char *source,
				unsigned int *frlen)
{
	memcpy((void *)pbuf, (void *)source, len);
	*frlen = *frlen + len;
	return (pbuf + len);
}

void construct_ht_ie(struct rtl8190_priv *priv, int use_40m, int offset)
{
	struct ht_cap_elmt	*ht_cap;
	struct ht_info_elmt	*ht_ie;
	int ch_offset;

	if (priv->ht_cap_len == 0) {
		// construct HT Capabilities element
		priv->ht_cap_len = sizeof(struct ht_cap_elmt);
		ht_cap = &priv->ht_cap_buf;
		memset(ht_cap, 0, sizeof(struct ht_cap_elmt));
		ht_cap->ht_cap_info |= cpu_to_le16(use_40m ? _HTCAP_SUPPORT_CH_WDTH_ : 0);
		ht_cap->ht_cap_info |= cpu_to_le16(_HTCAP_SMPWR_ENABLE_);
		// for test
		ht_cap->ht_cap_info |= cpu_to_le16(priv->pmib->dot11nConfigEntry.dot11nShortGIfor20M? _HTCAP_SHORTGI_20M_ : 0);
		ht_cap->ht_cap_info |= cpu_to_le16(priv->pmib->dot11nConfigEntry.dot11nShortGIfor40M? _HTCAP_SHORTGI_40M_ : 0);
		ht_cap->ht_cap_info |= cpu_to_le16(priv->pmib->dot11nConfigEntry.dot11nTxSTBC? _HTCAP_TX_STBC_ : 0);
		ht_cap->ht_cap_info |= cpu_to_le16(priv->pmib->dot11nConfigEntry.dot11nAMSDURecvMax? _HTCAP_AMSDU_LEN_8K_ : 0);
		ht_cap->ht_cap_info |= cpu_to_le16(_HTCAP_CCK_IN_40M_);
		if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm)
			ht_cap->ampdu_para = ((_HTCAP_AMPDU_SPC_16_US_ << _HTCAP_AMPDU_SPC_SHIFT_) | _HTCAP_AMPDU_FAC_32K_);
		else
			ht_cap->ampdu_para = ((_HTCAP_AMPDU_SPC_8_US_ << _HTCAP_AMPDU_SPC_SHIFT_) | _HTCAP_AMPDU_FAC_32K_);
		ht_cap->support_mcs[0] = (priv->pmib->dot11nConfigEntry.dot11nSupportedMCS & 0xff);
		ht_cap->support_mcs[1] = (priv->pmib->dot11nConfigEntry.dot11nSupportedMCS & 0xff00) >> 8;
		ht_cap->support_mcs[2] = (priv->pmib->dot11nConfigEntry.dot11nSupportedMCS & 0xff0000) >> 16;
		ht_cap->support_mcs[3] = (priv->pmib->dot11nConfigEntry.dot11nSupportedMCS & 0xff000000) >> 24;
		ht_cap->ht_ext_cap = 0;
		ht_cap->txbf_cap = 0;
		ht_cap->asel_cap = 0;

#ifdef CLIENT_MODE
		if ((OPMODE & WIFI_AP_STATE) || (OPMODE & WIFI_ADHOC_STATE))
#endif
		{
			// construct HT Information element
			priv->ht_ie_len = sizeof(struct ht_info_elmt);
			ht_ie = &priv->ht_ie_buf;
			memset(ht_ie, 0, sizeof(struct ht_info_elmt));
			ht_ie->primary_ch = priv->pmib->dot11RFEntry.dot11channel;
			if (use_40m) {
				if (offset == HT_2NDCH_OFFSET_BELOW)
					ch_offset = _HTIE_2NDCH_OFFSET_BL_ | _HTIE_STA_CH_WDTH_;
				else
					ch_offset = _HTIE_2NDCH_OFFSET_AB_ | _HTIE_STA_CH_WDTH_;
			}
			else
				ch_offset = _HTIE_2NDCH_OFFSET_NO_;
			ht_ie->info0 |= ch_offset;
			ht_ie->info1 = 0;
			ht_ie->info2 = 0;
			ht_ie->basic_mcs[0] = (priv->pmib->dot11nConfigEntry.dot11nBasicMCS & 0xff);
			ht_ie->basic_mcs[1] = (priv->pmib->dot11nConfigEntry.dot11nBasicMCS & 0xff00) >> 8;
			ht_ie->basic_mcs[2] = (priv->pmib->dot11nConfigEntry.dot11nBasicMCS & 0xff0000) >> 16;
			ht_ie->basic_mcs[3] = (priv->pmib->dot11nConfigEntry.dot11nBasicMCS & 0xff000000) >> 24;
		}
	}
	else
#ifdef CLIENT_MODE
	if ((OPMODE & WIFI_AP_STATE) || (OPMODE & WIFI_ADHOC_STATE) )
#endif
	{
#ifdef WIFI_11N_2040_COEXIST
		if (priv->pshare->rf_ft_var.coexist_enable && (OPMODE & WIFI_AP_STATE)) {
			ht_ie = &priv->ht_ie_buf;
			ht_ie->info0 &= ~(_HTIE_2NDCH_OFFSET_BL_ | _HTIE_STA_CH_WDTH_);
			if (use_40m && !(priv->bg_ap_timeout || priv->force_20_sta || priv->switch_20_sta)) {
				if (offset == HT_2NDCH_OFFSET_BELOW)
					ch_offset = _HTIE_2NDCH_OFFSET_BL_ | _HTIE_STA_CH_WDTH_;
				else
					ch_offset = _HTIE_2NDCH_OFFSET_AB_ | _HTIE_STA_CH_WDTH_;
			}
			else
				ch_offset = _HTIE_2NDCH_OFFSET_NO_;
			ht_ie->info0 |= ch_offset;
		}
#endif
		if (!priv->pmib->dot11StationConfigEntry.protectionDisabled) {
			if (priv->ht_legacy_obss_to || priv->ht_legacy_sta_num)
				priv->ht_protection = 1;
			else
				priv->ht_protection = 0;;
		}

		if (priv->ht_legacy_sta_num)
			priv->ht_ie_buf.info1 |= cpu_to_le16(_HTIE_OP_MODE3_);
		else if (priv->ht_legacy_obss_to)
			priv->ht_ie_buf.info1 |= cpu_to_le16(_HTIE_OP_MODE1_);
		else
			priv->ht_ie_buf.info1 &= cpu_to_le16(~_HTIE_OP_MODE3_);

		if (priv->ht_protection)
			priv->ht_ie_buf.info1 |= cpu_to_le16(_HTIE_OBSS_NHT_STA_);
		else
			priv->ht_ie_buf.info1 &= cpu_to_le16(~_HTIE_OBSS_NHT_STA_);
	}
}


unsigned char *construct_ht_ie_old_form(struct rtl8190_priv *priv, unsigned char *pbuf, unsigned int *frlen)
{
	unsigned char old_ht_ie_id[] = {0x00, 0x90, 0x4c};

	*pbuf = _RSN_IE_1_;
	*(pbuf + 1) = 3 + 1 + priv->ht_cap_len;
	memcpy((pbuf + 2), old_ht_ie_id, 3);
	*(pbuf + 5) = 0x33;
	memcpy((pbuf + 6), (unsigned char *)&priv->ht_cap_buf, priv->ht_cap_len);
	*frlen += (*(pbuf + 1) + 2);
	pbuf +=(*(pbuf + 1) + 2);

	*pbuf = _RSN_IE_1_;
	*(pbuf + 1) = 3 + 1 + priv->ht_ie_len;
	memcpy((pbuf + 2), old_ht_ie_id, 3);
	*(pbuf + 5) = 0x34;
	memcpy((pbuf + 6), (unsigned char *)&priv->ht_ie_buf, priv->ht_ie_len);
	*frlen += (*(pbuf + 1) + 2);
	pbuf +=(*(pbuf + 1) + 2);

	return pbuf;
}


#ifdef WIFI_11N_2040_COEXIST
void construct_obss_scan_para_ie(struct rtl8190_priv *priv)
{
	struct obss_scan_para_elmt		*obss_scan_para;

	if (priv->obss_scan_para_len == 0) {
		priv->obss_scan_para_len = sizeof(struct obss_scan_para_elmt);
		obss_scan_para = &priv->obss_scan_para_buf;
		memset(obss_scan_para, 0, sizeof(struct obss_scan_para_elmt));

		// except word2, all are default values and meaningless for ap at present
		// by victoryman, 20090521
		obss_scan_para->word0 = cpu_to_le16(0x14);
		obss_scan_para->word1 = cpu_to_le16(0x0a);
		obss_scan_para->word2 = cpu_to_le16(180);	// set as 180 second for 11n test plan
		obss_scan_para->word3 = cpu_to_le16(0xc8);
		obss_scan_para->word4 = cpu_to_le16(0x14);
		obss_scan_para->word5 = cpu_to_le16(5);
		obss_scan_para->word6 = cpu_to_le16(0x19);
	}
}
#endif


/**
 *	@brief	Update beacon content
 *
 *	IBSS parameter set (STA), TIM (AP), ERP & Ext rate not set  \n \n
 *	+----------------------------+-----+--------------------+-----+---------------------+-----+------------+-----+	\n
 *	| DS parameter (init_beacon) | TIM | IBSS parameter set | ERP | EXT supported rates | RSN | Realtek IE | CRC |	\n
 *	+----------------------------+-----+--------------------+-----+---------------------+-----+------------+-----+	\n
 *	\n
 *	set_desc() set data to hardware, 8190n_hw.h define value
 */


void update_beacon(struct rtl8190_priv *priv)
{
#if defined(CONFIG_RTL_WAPI_SUPPORT)
	unsigned long		flags;
#endif
	{
		struct wifi_mib *pmib;
		struct rtl8190_hw	*phw;
		struct tx_desc	*pdesc;
		unsigned int	frlen;
		unsigned char	*pbuf;
		unsigned char	*pbssrate=NULL;
		int				bssrate_len;

#ifdef	CONFIG_RTK_MESH
		UINT8		meshiearray[32];	// mesh IE buffer (Max byte is mesh_ie_MeshID)
#endif

		pmib = GET_MIB(priv);
		phw = GET_HW(priv);
		pdesc = phw->tx_descB;
		frlen = priv->timoffset;
		pbuf = (unsigned char *)priv->beaconbuf + priv->timoffset;

#ifdef DFS
		if (!priv->pmib->dot11DFSEntry.disable_DFS &&
			(timer_pending(&priv->ch_avail_chk_timer) ||
			 priv->pmib->dot11DFSEntry.disable_tx)) {
			return;
		}
#endif

		// prevent DMA right now
		pdesc->Dword0 = 0;

		// setting tim field...
		if (OPMODE & WIFI_AP_STATE)
			pbuf = update_tim(priv, pbuf, &frlen);

		if (OPMODE & WIFI_ADHOC_STATE) {
			unsigned short val16 = 0;
			pbuf = set_ie(pbuf, _IBSS_PARA_IE_, 2, (unsigned char *)&val16, &frlen);
		}

		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) {
			// ERP infomation
			unsigned char val8 = 0;
			if (priv->pmib->dot11ErpInfo.protection)
				val8 |= BIT(1);
			if (priv->pmib->dot11ErpInfo.nonErpStaNum)
				val8 |= BIT(0);

			if (!SHORTPREAMBLE || priv->pmib->dot11ErpInfo.longPreambleStaNum)
				val8 |= BIT(2);

			pbuf = set_ie(pbuf, _ERPINFO_IE_, 1, &val8, &frlen);
		}

		// EXT supported rates
		if (get_bssrate_set(priv, _EXT_SUPPORTEDRATES_IE_, &pbssrate, &bssrate_len))
			pbuf = set_ie(pbuf, _EXT_SUPPORTEDRATES_IE_, bssrate_len, pbssrate, &frlen);

		/*
			2008-12-16, For Buffalo WLI_CB_AG54L 54Mbps NIC interoperability issue.
			This NIC can not connect to our AP when our AP is set to WPA/TKIP encryption.
			This issue can be fixed after move "HT Capability Info" and "Additional HT Info" in front of "WPA" and "WMM".
		 */
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
			construct_ht_ie(priv, priv->pshare->is_40m_bw, priv->pshare->offset_2nd_chan);
			pbuf = set_ie(pbuf, _HT_CAP_, priv->ht_cap_len, (unsigned char *)&priv->ht_cap_buf, &frlen);
			pbuf = set_ie(pbuf, _HT_IE_, priv->ht_ie_len, (unsigned char *)&priv->ht_ie_buf, &frlen);
		}

#ifdef WIFI_11N_2040_COEXIST
		if ((OPMODE & WIFI_AP_STATE) && (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
			priv->pshare->rf_ft_var.coexist_enable && priv->pshare->is_40m_bw) {
			construct_obss_scan_para_ie(priv);
			pbuf = set_ie(pbuf, _OBSS_SCAN_PARA_IE_, priv->obss_scan_para_len,
				(unsigned char *)&priv->obss_scan_para_buf, &frlen);

			unsigned char temp_buf = _2040_COEXIST_SUPPORT_ ;
			pbuf = set_ie(pbuf, _EXTENDED_CAP_IE_, 1, &temp_buf, &frlen);
		}
#endif

		if (pmib->dot11RsnIE.rsnielen) {
			memcpy(pbuf, pmib->dot11RsnIE.rsnie, pmib->dot11RsnIE.rsnielen);
			pbuf += pmib->dot11RsnIE.rsnielen;
			frlen += pmib->dot11RsnIE.rsnielen;
		}

#ifdef SEMI_QOS
		//Set WMM Parameter Element
		if (QOS_ENABLE)
			pbuf = set_ie(pbuf, _RSN_IE_1_, _WMM_Para_Element_Length_, GET_WMM_PARA_IE, &frlen);
#endif

#ifdef WIFI_SIMPLE_CONFIG
		if (pmib->wscEntry.wsc_enable && pmib->wscEntry.beacon_ielen) {
			memcpy(pbuf, pmib->wscEntry.beacon_ie, pmib->wscEntry.beacon_ielen);
			pbuf += pmib->wscEntry.beacon_ielen;
			frlen += pmib->wscEntry.beacon_ielen;
		}
#endif

		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
			/*
				2008-12-16, For Buffalo WLI_CB_AG54L 54Mbps NIC interoperability issue.
				This NIC can not connect to our AP when our AP is set to WPA/TKIP encryption.
				This issue can be fixed after move "HT Capability Info" and "Additional HT Info" in front of "WPA" and "WMM".
			 */
			//construct_ht_ie(priv, priv->pshare->is_40m_bw, priv->pshare->offset_2nd_chan);
			//pbuf = set_ie(pbuf, _HT_CAP_, priv->ht_cap_len, (unsigned char *)&priv->ht_cap_buf, &frlen);
			//pbuf = set_ie(pbuf, _HT_IE_, priv->ht_ie_len, (unsigned char *)&priv->ht_ie_buf, &frlen);
			pbuf = construct_ht_ie_old_form(priv, pbuf, &frlen);
		}

		// Realtek proprietary IE
		if (priv->pshare->rtk_ie_len)
			pbuf = set_ie(pbuf, _RSN_IE_1_, priv->pshare->rtk_ie_len, priv->pshare->rtk_ie_buf, &frlen);

		// Customer proprietary IE
		if (priv->pmib->miscEntry.private_ie_len) {
			memcpy(pbuf, pmib->miscEntry.private_ie, pmib->miscEntry.private_ie_len);
			pbuf += pmib->miscEntry.private_ie_len;
			frlen += pmib->miscEntry.private_ie_len;
		}

#if defined(CONFIG_RTL_WAPI_SUPPORT)
		if (priv->pmib->wapiInfo.wapiType!=wapiDisable)
		{
			SAVE_INT_AND_CLI(flags);
			*priv->pBeaconCapability |= cpu_to_le16(BIT(4));	/* set privacy	*/
			priv->wapiCachedBuf = pbuf+2;
			wapiSetIE(priv);
			pbuf[0] = _EID_WAPI_;
			pbuf[1] = priv->wapiCachedLen;
			pbuf += priv->wapiCachedLen+2;
			frlen += priv->wapiCachedLen+2;
			RESTORE_INT(flags);
		}
#endif

#ifdef CONFIG_RTK_MESH		// Mesh IE
		if (GET_MIB(priv)->dot1180211sInfo.mesh_enable) {
			// OFDM Parameter Set
			pbuf = set_ie(pbuf, _OFDM_PARAMETER_SET_IE_, 1, ((unsigned char *)&(priv->pmib->dot11RFEntry.dot11channel)) + 3, &frlen);

			// Mesh ID
			pbuf = set_ie(pbuf, _MESH_ID_IE_, mesh_ie_MeshID(priv, meshiearray, FALSE), meshiearray, &frlen);

			// WLAN Mesh Capability
			pbuf = set_ie(pbuf, _WLAN_MESH_CAP_IE_, mesh_ie_WLANMeshCAP(priv, meshiearray), meshiearray, &frlen);

			/*
			if (power save mode?? ) {
				// Neighbor  List not use NOW.
				// DTIM not use NOW.
			}
			*/

			// Mesh Portal Reachability not use NOW.
			// Beacon Timing not use NOW.
			// MDAOP Advertisements not use NOW.
			// MDAOP Set Teardown not use NOW.

			// MKD domain information element [MKDDIE]
			pbuf = set_ie(pbuf, _MKDDIE_IE_, mesh_ie_MKDDIE(priv, meshiearray), meshiearray, &frlen);
		}
#endif	// CONFIG_RTK_MESH

		pdesc->Dword0 = set_desc(TX_FirstSeg| TX_LastSeg|  (32)<<TX_OFFSETSHIFT | (frlen) << TX_PktSizeSHIFT);
		pdesc->Dword1 = set_desc(0x12 << TX_QueueSelSHIFT);
		pdesc->Dword4 = set_desc((0x7 << TX_RaBRSRIDSHIFT) | TX_UserRate);
		pdesc->Dword5 = set_desc(TX_DISFB);

#if defined(CONFIG_RTL8196B_TR)
		priv->ext_stats.tx_byte_cnt += frlen;
#endif
	//#ifdef CONFIG_RTL865X_AC
#if defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
		priv->ext_stats.tx_byte_cnt += frlen;
#endif

#ifdef WDS
		// if pure WDS bridge, don't send beacon
		if ((OPMODE & WIFI_AP_STATE) && pmib->dot11WdsInfo.wdsPure)
			return;
#endif
		// if schedule off, don't send beacon
		if (priv->pmib->miscEntry.func_off)
			return;

		if (!IS_DRV_OPEN(priv))
			return;

	}
}

/**
 *	@brief	Initial beacon
 *
  *	Refer wifi.h and 8190mib.h about MIB define.	\n
 *  Refer 802.11 7,3,13 Beacon interval field	\n
 *	- Timestamp \n - Beacon interval \n - Capability \n - SSID \n - Support rate \n - DS Parameter set \n \n
 *	+-------+-------+-------+	\n
 *	| addr1 | addr2 | addr3 |	\n
 *	+-------+-------+-------+	\n
 *
 *	+-----------+-----------------+------------+------+--------------+------------------+	\n
 *	| Timestamp | Beacon interval | Capability | SSID | Support rate | DS Parameter set |	\n
 *	+-----------+-----------------+------------+------+--------------+------------------+	\n
 *	[Note] \n
 *	abridge FH (unused), CF (AP not support PCF), \n
 *	IBSS parameter set (STA), DTIM (AP), ERP »P Ext rate  IE complete in Update beacon.\n
 *	set_desc is important.
 */

void init_beacon(struct rtl8190_priv *priv)
{
	{
		unsigned short	val16;
		unsigned char	val8;
		struct wifi_mib *pmib;
		unsigned char	*bssid;
		struct tx_desc		*pdesc;
		struct rtl8190_hw	*phw=GET_HW(priv);

		unsigned int	frlen=0;
		unsigned char	*pbuf=(unsigned char *)priv->beaconbuf;
		unsigned char	*pbssrate=NULL;
		int	bssrate_len;
		struct FWtemplate *txfw;
		struct FWtemplate Temptxfw;

		unsigned int rate;

		pmib = GET_MIB(priv);
		bssid = pmib->dot11StationConfigEntry.dot11Bssid;

		memset(pbuf, 0, 128*4);
		txfw = &Temptxfw;

		rate = find_rate(priv, NULL, 0, 1);

#ifdef _11s_TEST_MODE_
		mesh_debug_sme2(priv, &rate);
#endif

		if (is_MCS_rate(rate)) {
			// can we use HT rates for beacon?
			txfw->txRate = rate & 0x7f;
			txfw->txHt = 1;
		}
		else {
			txfw->txRate = get_rate_index_from_ieee_value((UINT8)rate);
			if (priv->pshare->is_40m_bw) {
				if (priv->pshare->offset_2nd_chan == HT_2NDCH_OFFSET_BELOW)
					txfw->txSC = 2;
				else
					txfw->txSC = 1;
			}
		}

		SetFrameSubType(pbuf, WIFI_BEACON);

		memset((void *)GetAddr1Ptr(pbuf), 0xff, 6);
		memcpy((void *)GetAddr2Ptr(pbuf), GET_MY_HWADDR, 6);
		memcpy((void *)GetAddr3Ptr(pbuf), bssid, 6); // (Indeterminable set null mac (all zero)) (Refer: Draft 1.06, Page 12, 7.2.3, Line 21~30)

		pbuf += 24;
		frlen += 24;

		frlen += _TIMESTAMP_;	// for timestamp
		pbuf += _TIMESTAMP_;

		//setup BeaconPeriod...
		val16 = cpu_to_le16(pmib->dot11StationConfigEntry.dot11BeaconPeriod);
#ifdef CLIENT_MODE
		if (OPMODE & WIFI_ADHOC_STATE)
			val16 = cpu_to_le16(priv->beacon_period);
#endif
		pbuf = set_fixed_ie(pbuf, _BEACON_ITERVAL_, (unsigned char *)&val16, &frlen);

#ifdef CONFIG_RTK_MESH
		if ((1 == GET_MIB(priv)->dot1180211sInfo.mesh_enable) && (0 == GET_MIB(priv)->dot1180211sInfo.mesh_ap_enable))	// non-AP MP (MAP)	only, popen:802.11s Draft 1.0 P17  7.3.1.4 : ESS & IBSS are "0" (PS:val reset here)
			val16= 0;
		else
#endif	// CONFIG_RTK_MESH
		{
			if (OPMODE & WIFI_AP_STATE)
				val16 = cpu_to_le16(BIT(0)); //ESS
			else
				val16 = cpu_to_le16(BIT(1)); //IBSS
		}

		if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm)
			val16 |= cpu_to_le16(BIT(4));

		if (SHORTPREAMBLE)
			val16 |= cpu_to_le16(BIT(5));

		pbuf = set_fixed_ie(pbuf, _CAPABILITY_, (unsigned char *)&val16, &frlen);
		priv->pBeaconCapability = (unsigned short *)(pbuf - _CAPABILITY_);

		if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) && (priv->pmib->dot11ErpInfo.shortSlot))
			SET_SHORTSLOT_IN_BEACON_CAP;
		else
			RESET_SHORTSLOT_IN_BEACON_CAP;

		//set ssid...
#ifdef WIFI_SIMPLE_CONFIG
		priv->pbeacon_ssid = pbuf;
#endif
#ifdef CONFIG_RTK_MESH
		if ((1 == GET_MIB(priv)->dot1180211sInfo.mesh_enable) && (0 == GET_MIB(priv)->dot1180211sInfo.mesh_ap_enable))	//	non-AP MP (MAP)  only, popen:802.11s Draft 1.0, Page 11, SSID
			pbuf = set_ie(pbuf, _SSID_IE_, 0, 0, &frlen);	// wildcard SSID (len = 0)
		else
#endif
		{
			if (!HIDDEN_AP)
				pbuf = set_ie(pbuf, _SSID_IE_, SSID_LEN, SSID, &frlen);
			else {
#ifdef CONFIG_RTL8196B_TLD
				pbuf = set_ie(pbuf, _SSID_IE_, 0, NULL, &frlen);
#else
				unsigned char ssidbuf[32];
				memset(ssidbuf, 0, 32);
				pbuf = set_ie(pbuf, _SSID_IE_, SSID_LEN, ssidbuf, &frlen);
#endif
			}
		}

		//supported rates...
		get_bssrate_set(priv, _SUPPORTEDRATES_IE_, &pbssrate, &bssrate_len);
		pbuf = set_ie(pbuf, _SUPPORTEDRATES_IE_, bssrate_len, pbssrate, &frlen);

		//ds parameter set...
		val8 = pmib->dot11RFEntry.dot11channel;
		pbuf = set_ie(pbuf, _DSSET_IE_, 1, &val8, &frlen);
		priv->timoffset = frlen;

		pdesc = phw->tx_descB;

		// clear all bit
		memset(pdesc, 0, 32);

		pdesc->Dword4 |= set_desc(0x08 << TX_RTSRateSHIFT);
		update_beacon(priv);


	}
}


static void setChannelScore(int number, unsigned int *val, int min, int max)
{
	int i=0, score;

	if (number > max)
		return;

	*(val + number) += 5;

	if (number > min) {
		for (i=number-1, score=4; i>=min && score; i--, score--) {
			*(val + i) += score;
		}
	}
	if (number < max) {
		for (i=number+1, score=4; i<=max && score; i++, score--) {
			*(val +i) += score;
		}
	}
}


static int selectClearChannel(struct rtl8190_priv *priv)
{
	unsigned int score2G[MAX_CCK_CHANNEL_NUM], score5G[(MAX_OFDM_CHANNEL_NUM-MAX_CCK_CHANNEL_NUM)/2];
	unsigned int score[64];
	unsigned int minScore=0xffffffff;
	int i, idx=0, idx_2G_end=-1, idx_5G_begin=-1, minChan=0;
	struct bss_desc *pBss=NULL;
#ifdef _DEBUG_RTL8190_
	char tmpbuf[200];
	int len=0;
#endif

	memset(score2G, '\0', sizeof(score2G));
	memset(score5G, '\0', sizeof(score5G));

	for (i=0; i<priv->available_chnl_num; i++) {
		if (priv->available_chnl[i] <= 14)
			idx_2G_end = i;
		else
			break;
	}

	for (i=0; i<priv->available_chnl_num; i++) {
		if (priv->available_chnl[i] > 14) {
			idx_5G_begin = i;
			break;
		}
	}

	for (i=0; i<priv->site_survey.count; i++) {
		pBss = &priv->site_survey.bss[i];
		for (idx=0; idx<priv->available_chnl_num; idx++) {
			if (pBss->channel == priv->available_chnl[idx]) {
				if (pBss->channel <= 14)
					setChannelScore(idx, score2G, 0, MAX_CCK_CHANNEL_NUM-1);
				else
					score5G[idx - idx_5G_begin] += 5;
				break;
			}
		}
	}

	if (idx_2G_end >= 0)
		for (i=0; i<=idx_2G_end; i++)
			score[i] = score2G[i];
	if (idx_5G_begin >= 0)
		for (i=idx_5G_begin; i<priv->available_chnl_num; i++)
			score[i] = score5G[i - idx_5G_begin];

//prevent Auto Channel selecting wrong channel in 40M mode-----------------
	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)
		&& priv->pshare->is_40m_bw) {
		score[13] = 0xffffffff;		// mask chan14
	}
//------------------------------------------------------------------

	for (i=0; i<priv->available_chnl_num; i++) {
#ifdef _DEBUG_RTL8190_
		len += sprintf(tmpbuf+len, "ch%d:%d ", priv->available_chnl[i], score[i]);
#endif
		if (score[i] < minScore) {
			minScore = score[i];
			idx = i;
		}
	}
#ifdef _DEBUG_RTL8190_
	strcat(tmpbuf, "\n");
	DEBUG_INFO("%s", tmpbuf);
#endif
	minChan = priv->available_chnl[idx];

	// skip channel 14 if don't support ofdm
	if ((priv->pmib->dot11RFEntry.disable_ch14_ofdm) &&
		(minChan == 14))
		minChan = 13;

// auto adjust contro-sideband
	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)
			&& priv->pshare->is_40m_bw) {
		if (minChan < 5) {
			GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_ABOVE;
			priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_ABOVE;
		}
		else if (minChan > 7) {
			GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_BELOW;
			priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_BELOW;
		}
	}
//-----------------------
	return minChan;
}




/**
 *	@brief	issue de-authenticaion
 *
 *	Defragement fail will be de-authentication or STA issue deauthenticaion request
 *
 *	+---------------+-----+----+----+-------+-----+-------------+ \n
 *	| Frame Control | ... | DA | SA | BSSID | ... | Reason Code | \n
 *	+---------------+-----+----+----+-------+-----+-------------+ \n
 */
void issue_deauth(struct rtl8190_priv *priv,	unsigned char *da, int reason)
{
#ifdef CONFIG_RTK_MESH
	issue_deauth_MP(priv, da, reason, FALSE);
}

void issue_deauth_MP(struct rtl8190_priv *priv,	unsigned char *da, int reason, UINT8 is_11s)
{
#endif
	struct wifi_mib *pmib;
	unsigned char	*bssid, *pbuf;
	unsigned short  val;
	DECLARE_TXINSN(txinsn);

	if (!memcmp(da, "\x0\x0\x0\x0\x0\x0", 6))
		return;

#ifdef RTL8190_FASTEXTDEV_FUNCALL
	rtl865x_extDev_removeHost(da, priv->WLAN_VLAN_ID);
#endif

	txinsn.retry = priv->pmib->dot11OperationEntry.dot11ShortRetryLimit;

	pmib= GET_MIB(priv);

	bssid = pmib->dot11StationConfigEntry.dot11Bssid;

#ifdef CONFIG_RTK_MESH
	txinsn.is_11s = is_11s;
#endif

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.tx_rate  = find_rate(priv, NULL, 0, 1);
	txinsn.lowest_tx_rate = txinsn.tx_rate;
	txinsn.fixed_rate = 1;
	txinsn.fr_type = _PRE_ALLOCMEM_;

	pbuf = txinsn.pframe  = get_mgtbuf_from_poll(priv);

	if (pbuf == NULL)
		goto issue_deauth_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);

	if (txinsn.phdr == NULL)
		goto issue_deauth_fail;

	memset((void *)txinsn.phdr, 0, sizeof(struct  wlan_hdr));

	val = cpu_to_le16(reason);

	pbuf = set_fixed_ie(pbuf, _RSON_CODE_ , (unsigned char *)&val, &txinsn.fr_len);

	SetFrameType((txinsn.phdr),WIFI_MGT_TYPE);
	SetFrameSubType((txinsn.phdr),WIFI_DEAUTH);

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), da, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), bssid, MACADDRLEN);

#ifdef CONFIG_RTK_MESH
	if (TRUE == is_11s)
		// Though spec define management frames Address 3 is "null mac" (all zero), but avoid filter out by other MP, set da) (Refer: Draft 1.06, Page 12, 7.2.3, Line 29~30 2007/08/11 by popen)
		memcpy((void *)GetAddr3Ptr((txinsn.phdr)), da, MACADDRLEN);
	else
#endif
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), bssid, MACADDRLEN);

	SNMP_MIB_ASSIGN(dot11DeauthenticateReason, reason);
	SNMP_MIB_COPY(dot11DeauthenticateStation, da, MACADDRLEN);

	if ((rtl8190_firetx(priv, &txinsn)) == SUCCESS) {
		// stop rssi check
#ifdef STA_EXT
		if ((OPMODE & WIFI_AP_STATE) && get_stainfo(priv, da) && (get_stainfo(priv, da)->remapped_aid <= 8))
			set_fw_reg(priv, 0xfd000013 | get_stainfo(priv, da)->remapped_aid<<16, 0, 0);
#else
		if ((OPMODE & WIFI_AP_STATE) && get_stainfo(priv, da) && (get_stainfo(priv, da)->aid <= 8))
			set_fw_reg(priv, 0xfd000013 | get_stainfo(priv, da)->aid<<16, 0, 0);
#endif
		return;
	}

issue_deauth_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
}


void issue_disassoc(struct rtl8190_priv *priv,	unsigned char *da, int reason)
{
	struct wifi_mib *pmib;
	unsigned char	*bssid, *pbuf;
	unsigned short  val;
	DECLARE_TXINSN(txinsn);

	txinsn.retry = priv->pmib->dot11OperationEntry.dot11ShortRetryLimit;

#ifdef RTL8190_FASTEXTDEV_FUNCALL
	rtl865x_extDev_removeHost(da, priv->WLAN_VLAN_ID);
#endif

	pmib= GET_MIB(priv);

	bssid = pmib->dot11StationConfigEntry.dot11Bssid;

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
	txinsn.tx_rate  = find_rate(priv, NULL, 0, 1);
	txinsn.lowest_tx_rate = txinsn.tx_rate;
	txinsn.fixed_rate = 1;

	pbuf = txinsn.pframe  = get_mgtbuf_from_poll(priv);

	if (pbuf == NULL)
		goto issue_disassoc_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);

	if (txinsn.phdr == NULL)
		goto issue_disassoc_fail;

	memset((void *)txinsn.phdr, 0, sizeof(struct  wlan_hdr));

	val = cpu_to_le16(reason);

	pbuf = set_fixed_ie(pbuf, _RSON_CODE_, (unsigned char *)&val, &txinsn.fr_len);

	SetFrameType((txinsn.phdr), WIFI_MGT_TYPE);
	SetFrameSubType((txinsn.phdr), WIFI_DISASSOC);

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), da, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), bssid, MACADDRLEN);

	SNMP_MIB_ASSIGN(dot11DisassociateReason, reason);
	SNMP_MIB_COPY(dot11DisassociateStation, da, MACADDRLEN);

	if ((rtl8190_firetx(priv, &txinsn)) == SUCCESS) {
		// stop rssi check
#ifdef STA_EXT
		if ((OPMODE & WIFI_AP_STATE) && get_stainfo(priv, da) && (get_stainfo(priv, da)->remapped_aid <= 8))
			set_fw_reg(priv, 0xfd000013 | get_stainfo(priv, da)->remapped_aid<<16, 0, 0);
#else
		if ((OPMODE & WIFI_AP_STATE) && get_stainfo(priv, da) && (get_stainfo(priv, da)->aid <= 8))
			set_fw_reg(priv, 0xfd000013 | get_stainfo(priv, da)->aid<<16, 0, 0);
#endif
		return;
	}

issue_disassoc_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
}


// if pstat == NULL, indiate we are station now...
void issue_auth(struct rtl8190_priv *priv, struct stat_info *pstat, unsigned short status)
{
	struct wifi_mib *pmib;
	unsigned char	*bssid, *pbuf;
	unsigned short  val;
	int use_shared_key=0;

#if defined(CONFIG_RTK_MESH) && defined(MESH_BOOTSEQ_AUTH)
	UINT8	isMeshMP = FALSE;
#endif	// CONFIG_RTK_MESH

	DECLARE_TXINSN(txinsn);

	txinsn.retry = priv->pmib->dot11OperationEntry.dot11ShortRetryLimit;

	pmib= GET_MIB(priv);

#if defined(CONFIG_RTK_MESH) && defined(MESH_BOOTSEQ_AUTH)
	if ((NULL != pstat) && (1 == GET_MIB(priv)->dot1180211sInfo.mesh_enable) && isPossibleNeighbor(pstat))
		isMeshMP = TRUE;

	if ((pstat) || (TRUE == isMeshMP))
#else
	if (pstat)
#endif	//CONFIG_RTK_MESH
		bssid = BSSID;
	else
		bssid = priv->pmib->dot11Bss.bssid;

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
	txinsn.lowest_tx_rate = txinsn.tx_rate;
	txinsn.fixed_rate = 1;
	txinsn.fr_type = _PRE_ALLOCMEM_;

	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);

	if (pbuf == NULL)
		goto issue_auth_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);

	if (txinsn.phdr == NULL)
		goto issue_auth_fail;

	memset((void *)(txinsn.phdr), 0, sizeof (struct	wlan_hdr));

	// setting auth algm number
	/* In AP mode,	if auth is set to shared-key, use shared key
	 *		if auth is set to auto, use shared key if client use shared
	 *		otherwise set to open
	 * In client mode, if auth is set to shared-key or auto, and WEP is used,
	 *		use shared key algorithm
	 */
	val = 0;

#if defined(CONFIG_RTK_MESH) && defined(MESH_BOOTSEQ_AUTH)	// skip SIMPLE_CONFIG and Auth check ,Force use open system auth.
	if (FALSE == isMeshMP)
#endif
	{

#ifdef WIFI_SIMPLE_CONFIG
	if (pmib->wscEntry.wsc_enable) {
		if (pstat && (status == _STATS_SUCCESSFUL_) && (pstat->auth_seq == 2) &&
			(pstat->state & WIFI_AUTH_SUCCESS) && (pstat->AuthAlgrthm == 0))
			goto skip_security_check;
		else if ((pstat == NULL) && (priv->auth_seq == 1))
			goto skip_security_check;
	}
#endif

	if (priv->pmib->dot1180211AuthEntry.dot11AuthAlgrthm > 0) {
		if (pstat) {
			if (priv->pmib->dot1180211AuthEntry.dot11AuthAlgrthm == 1) // shared key
				val = 1;
			else { // auto mode, check client algorithm
				if (pstat && pstat->AuthAlgrthm)
					val = 1;
			}
		}
		else { // client mode, use shared key if WEP is enabled
			if (priv->pmib->dot1180211AuthEntry.dot11AuthAlgrthm == 1) { // shared-key ONLY
				if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_ ||
					priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_)
					val = 1;
			}
			else { // auto-auth
				if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_ ||
					priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_) {
					if (priv->auth_seq == 1)
						priv->authModeToggle = (priv->authModeToggle ? 0 : 1);

					if (priv->authModeToggle)
						val = 1;
				}
			}
		}

		if (val) {
			val = cpu_to_le16(val);
			use_shared_key = 1;
		}
	}

	if (pstat && (status != _STATS_SUCCESSFUL_))
		val = cpu_to_le16(pstat->AuthAlgrthm);
	}

#ifdef WIFI_SIMPLE_CONFIG
skip_security_check:
#endif

	pbuf = set_fixed_ie(pbuf, _AUTH_ALGM_NUM_, (unsigned char *)&val, &txinsn.fr_len);

	// setting transaction sequence number...
#if defined(CONFIG_RTK_MESH) && defined(MESH_BOOTSEQ_AUTH)
	if ((pstat) || (TRUE == isMeshMP))
#else
	if (pstat)
#endif
		val = cpu_to_le16(pstat->auth_seq);	// Mesh only
	else
		val = cpu_to_le16(priv->auth_seq);

	pbuf = set_fixed_ie(pbuf, _AUTH_SEQ_NUM_, (unsigned char *)&val, &txinsn.fr_len);

	// setting status code...
	val = cpu_to_le16(status);
	pbuf = set_fixed_ie(pbuf, _STATUS_CODE_, (unsigned char *)&val, &txinsn.fr_len);

	// then checking to see if sending challenging text... (Mesh skip this section)
	if (pstat)
	{
		if ((pstat->auth_seq == 2) && (pstat->state & WIFI_AUTH_STATE1) && use_shared_key)
			pbuf = set_ie(pbuf, _CHLGETXT_IE_, 128, pstat->chg_txt, &txinsn.fr_len);
	}
	else
	{
		if ((priv->auth_seq == 3) && (OPMODE & WIFI_AUTH_STATE1) && use_shared_key)
		{
			pbuf = set_ie(pbuf, _CHLGETXT_IE_, 128, priv->chg_txt, &txinsn.fr_len);
			SetPrivacy(txinsn.phdr);
			DEBUG_INFO("sending a privacy pkt with auth_seq=%d\n", priv->auth_seq);
		}
	}

	SetFrameSubType((txinsn.phdr), WIFI_AUTH);

#if defined(CONFIG_RTK_MESH) && defined(MESH_BOOTSEQ_AUTH)
	if ((pstat) || (TRUE == isMeshMP))
#else
	if (pstat)	// for AP mode
#endif
	{
		memcpy((void *)GetAddr1Ptr((txinsn.phdr)), pstat->hwaddr, MACADDRLEN);
		memcpy((void *)GetAddr2Ptr((txinsn.phdr)), bssid, MACADDRLEN);
	}
	else
	{
		memcpy((void *)GetAddr1Ptr((txinsn.phdr)), bssid, MACADDRLEN);
		memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	}

#if defined(CONFIG_RTK_MESH) && defined(MESH_BOOTSEQ_AUTH)
	if (TRUE == isMeshMP)	// Though spec define management frames Address 3 is "null mac" (all zero), but avoid filter out by other MP, set "Other MP MAC") (Refer: Draft 1.06, Page 12, 7.2.3, Line 29~30 2007/08/11 by popen)
		memcpy((void *)GetAddr3Ptr((txinsn.phdr)), pstat->hwaddr, MACADDRLEN);
	else
#endif
		memcpy((void *)GetAddr3Ptr((txinsn.phdr)), bssid, MACADDRLEN);

	if ((rtl8190_firetx(priv, &txinsn)) == SUCCESS)
		return;

issue_auth_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
}


void issue_asocrsp(struct rtl8190_priv *priv, unsigned short status, struct stat_info *pstat, int pkt_type)
{
	unsigned short	val;
	struct wifi_mib *pmib;
	unsigned char	*bssid,*pbuf;
	DECLARE_TXINSN(txinsn);

	txinsn.retry = priv->pmib->dot11OperationEntry.dot11ShortRetryLimit;

	pmib= GET_MIB(priv);

	bssid = pmib->dot11StationConfigEntry.dot11Bssid;

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
	txinsn.lowest_tx_rate = txinsn.tx_rate;
	txinsn.fixed_rate = 1;
	pbuf = txinsn.pframe  = get_mgtbuf_from_poll(priv);

	if (pbuf == NULL)
		goto issue_asocrsp_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);

	if (txinsn.phdr == NULL)
		goto issue_asocrsp_fail;

	memset((void *)txinsn.phdr, 0, sizeof(struct  wlan_hdr));

	val = cpu_to_le16(BIT(0));

	if (pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm)
		val |= cpu_to_le16(BIT(4));

	if (SHORTPREAMBLE)
		val |= cpu_to_le16(BIT(5));

	if (priv->pmib->dot11ErpInfo.shortSlot)
		val |= cpu_to_le16(BIT(10));

	pbuf = set_fixed_ie(pbuf, _CAPABILITY_, (unsigned char *)&val, &txinsn.fr_len);

	status = cpu_to_le16(status);
	pbuf = set_fixed_ie(pbuf, _STATUS_CODE_, (unsigned char *)&status, &txinsn.fr_len);

	val = cpu_to_le16(pstat->aid | 0xC000);
	pbuf = set_fixed_ie(pbuf, _ASOC_ID_, (unsigned char *)&val, &txinsn.fr_len);
#ifdef RTL8192SE_ACUT
	pstat->bssrateset[0] = 0x8c; // basic rate
	pstat->bssrateset[1] = 0x92; // basic rate
	pstat->bssrateset[2] = 0x98; // basic rate
	pstat->bssrateset[3] = 0xA4; // basic rate
	pstat->bssrateset[4] = 0x30;
	pstat->bssrateset[5] = 0x48;
	pstat->bssrateset[6] = 0x60;
	pstat->bssrateset[7] = 0x6c;
#endif
	if (STAT_OPRATE_LEN <= 8)
		pbuf = set_ie(pbuf, _SUPPORTEDRATES_IE_, STAT_OPRATE_LEN, STAT_OPRATE, &txinsn.fr_len);
	else {
		pbuf = set_ie(pbuf, _SUPPORTEDRATES_IE_, 8, STAT_OPRATE, &txinsn.fr_len);
		pbuf = set_ie(pbuf, _EXT_SUPPORTEDRATES_IE_, STAT_OPRATE_LEN-8, STAT_OPRATE+8, &txinsn.fr_len);
	}

#ifdef SEMI_QOS
	//Set WMM Parameter Element
	if ((QOS_ENABLE) && (pstat->QosEnabled))
		pbuf = set_ie(pbuf, _RSN_IE_1_, _WMM_Para_Element_Length_, GET_WMM_PARA_IE, &txinsn.fr_len);
#endif

#ifdef WIFI_SIMPLE_CONFIG
	if (pmib->wscEntry.wsc_enable && pmib->wscEntry.assoc_ielen) {
		memcpy(pbuf, pmib->wscEntry.assoc_ie, pmib->wscEntry.assoc_ielen);
		pbuf += pmib->wscEntry.assoc_ielen;
		txinsn.fr_len += pmib->wscEntry.assoc_ielen;
	}
#endif

	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) && (pstat->ht_cap_len > 0))
	{
		{
			pbuf = set_ie(pbuf, _HT_CAP_, priv->ht_cap_len, (unsigned char *)&priv->ht_cap_buf, &txinsn.fr_len);
			pbuf = set_ie(pbuf, _HT_IE_, priv->ht_ie_len, (unsigned char *)&priv->ht_ie_buf, &txinsn.fr_len);
			pbuf = construct_ht_ie_old_form(priv, pbuf, &txinsn.fr_len);
		}

#ifdef WIFI_11N_2040_COEXIST
		if (priv->pshare->rf_ft_var.coexist_enable && priv->pshare->is_40m_bw) {
			construct_obss_scan_para_ie(priv);
			pbuf = set_ie(pbuf, _OBSS_SCAN_PARA_IE_, priv->obss_scan_para_len,
				(unsigned char *)&priv->obss_scan_para_buf, &txinsn.fr_len);

			unsigned char temp_buf = _2040_COEXIST_SUPPORT_ ;
			pbuf = set_ie(pbuf, _EXTENDED_CAP_IE_, 1, &temp_buf, &txinsn.fr_len);
		}
#endif
	}

	// Realtek proprietary IE
	if (priv->pshare->rtk_ie_len)
		pbuf = set_ie(pbuf, _RSN_IE_1_, priv->pshare->rtk_ie_len, priv->pshare->rtk_ie_buf, &txinsn.fr_len);

	if ((pkt_type == WIFI_ASSOCRSP) || (pkt_type == WIFI_REASSOCRSP))
		SetFrameSubType((txinsn.phdr), pkt_type);
	else
		goto issue_asocrsp_fail;

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), pstat->hwaddr, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), bssid, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), bssid, MACADDRLEN);


	if ((rtl8190_firetx(priv, &txinsn)) == SUCCESS) {
//#if !defined(CONFIG_RTL865X_KLD) && !defined(CONFIG_RTL8196B_KLD)
		return;
	}

issue_asocrsp_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
}


/**
 *	@brief	issue probe response
 *
 *	- Timestamp \n - Beacon interval \n - Capability \n - SSID \n - Support rate \n - DS Parameter set \n \n
 *	+-------+-------+----+----+--------+	\n
 *	| Frame control | DA | SA |	BSS ID |	\n
 *	+-------+-------+----+----+--------+	\n
 *	\n
 *	+-----------+-----------------+------------+------+--------------+------------------+-----------+	\n
 *	| Timestamp | Beacon interval | Capability | SSID | Support rate | DS Parameter set | ERP info.	|	\n
 *	+-----------+-----------------+------------+------+--------------+------------------+-----------+	\n
 *
 *	\param	priv	device info.
 *	\param	da	address
 *	\param	sid	SSID
 *	\param	ssid_len	SSID length
 *	\param 	set_privacy	Use Robust security network
 */
static void issue_probersp(struct rtl8190_priv *priv, unsigned char *da,
				UINT8 *ssid, int ssid_len, int set_privacy)
{
#ifdef CONFIG_RTK_MESH
	issue_probersp_MP(priv, da, ssid, ssid_len, set_privacy, FALSE);
}

void issue_probersp_MP(struct rtl8190_priv *priv, unsigned char *da,
				UINT8 *ssid, int ssid_len, int set_privacy, UINT8 is_11s)
{
	UINT8 meshiearray[32];	// mesh IE buffer (Max byte is mesh_ie_MeshID)
#endif

#if defined(CONFIG_RTL_WAPI_SUPPORT)
	unsigned long		flags;
#endif


	unsigned short	val;
	struct wifi_mib *pmib;
	unsigned char	*bssid,*pbuf;
	UINT8	val8;
	unsigned char	*pbssrate=NULL;
	int		bssrate_len;
	DECLARE_TXINSN(txinsn);

	pmib= GET_MIB(priv);

	bssid = pmib->dot11StationConfigEntry.dot11Bssid;
#ifdef CONFIG_RTK_MESH
	txinsn.is_11s = is_11s;
#endif
	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
	txinsn.lowest_tx_rate = txinsn.tx_rate;
	txinsn.fixed_rate = 1;
	pbuf = txinsn.pframe  = get_mgtbuf_from_poll(priv);

	if (pbuf == NULL)
		goto issue_probersp_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);

	if (txinsn.phdr == NULL)
		goto issue_probersp_fail;

	memset((void *)(txinsn.phdr), 0, sizeof (struct	wlan_hdr));

	pbuf += _TIMESTAMP_;
	txinsn.fr_len += _TIMESTAMP_;

    val = cpu_to_le16(pmib->dot11StationConfigEntry.dot11BeaconPeriod);
	pbuf = set_fixed_ie(pbuf,  _BEACON_ITERVAL_ , (unsigned char *)&val, &txinsn.fr_len);

#ifdef CONFIG_RTK_MESH
	if ((1 == GET_MIB(priv)->dot1180211sInfo.mesh_enable) && (0 == GET_MIB(priv)->dot1180211sInfo.mesh_ap_enable))	// non-AP MP (MAP)	only, popen:802.11s Draft 1.0 P17  7.3.1.4 : ESS & IBSS are "0" (PS:val Reset here.)
		val = 0;
	else
#endif
	{
		if (OPMODE & WIFI_AP_STATE)
			val = cpu_to_le16(BIT(0)); //ESS
		else
			val = cpu_to_le16(BIT(1)); //IBSS
	}

	if (pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm && set_privacy)
		val |= cpu_to_le16(BIT(4));

#if defined(CONFIG_RTL_WAPI_SUPPORT)
	if (priv->pmib->wapiInfo.wapiType!=wapiDisable)
	{
		val |= cpu_to_le16(BIT(4));	/* set privacy	*/
	}
#endif

	if (SHORTPREAMBLE)
		val |= cpu_to_le16(BIT(5));

	if (priv->pmib->dot11ErpInfo.shortSlot)
		val |= cpu_to_le16(BIT(10));

	pbuf = set_fixed_ie(pbuf, _CAPABILITY_, (unsigned char *)&val, &txinsn.fr_len);

	pbuf = set_ie(pbuf, _SSID_IE_, ssid_len, ssid, &txinsn.fr_len);

	get_bssrate_set(priv, _SUPPORTEDRATES_IE_, &pbssrate, &bssrate_len);
	pbuf = set_ie(pbuf, _SUPPORTEDRATES_IE_, bssrate_len, pbssrate, &txinsn.fr_len);

	val8 = pmib->dot11RFEntry.dot11channel;

	pbuf = set_ie(pbuf, _DSSET_IE_, 1, &val8 , &txinsn.fr_len);

	if (OPMODE & WIFI_ADHOC_STATE) {
		unsigned short val16 = 0;
		pbuf = set_ie(pbuf, _IBSS_PARA_IE_, 2, (unsigned char *)&val16, &txinsn.fr_len);
	}

	if (OPMODE & WIFI_AP_STATE) {
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) {
		 	//ERP infomation.
			val8=0;
			if (priv->pmib->dot11ErpInfo.protection)
				val8 |= BIT(1);
			if (priv->pmib->dot11ErpInfo.nonErpStaNum)
				val8 |= BIT(0);
			pbuf = set_ie(pbuf, _ERPINFO_IE_ , 1 , &val8, &txinsn.fr_len);
		}
	}

	//EXT supported rates.
	if (get_bssrate_set(priv, _EXT_SUPPORTEDRATES_IE_, &pbssrate, &bssrate_len))
		pbuf = set_ie(pbuf, _EXT_SUPPORTEDRATES_IE_ , bssrate_len , pbssrate, &txinsn.fr_len);

	/*
		2008-12-16, For Buffalo WLI_CB_AG54L 54Mbps NIC interoperability issue.
		This NIC can not connect to our AP when our AP is set to WPA/TKIP encryption.
		This issue can be fixed after move "HT Capability Info" and "Additional HT Info" in front of "WPA" and "WMM".
	 */
	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
		{
			pbuf = set_ie(pbuf, _HT_CAP_, priv->ht_cap_len, (unsigned char *)&priv->ht_cap_buf, &txinsn.fr_len);
			pbuf = set_ie(pbuf, _HT_IE_, priv->ht_ie_len, (unsigned char *)&priv->ht_ie_buf, &txinsn.fr_len);
		}
	}

#ifdef WIFI_11N_2040_COEXIST
		if ((OPMODE & WIFI_AP_STATE) && (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
			priv->pshare->rf_ft_var.coexist_enable && priv->pshare->is_40m_bw) {
			construct_obss_scan_para_ie(priv);
			pbuf = set_ie(pbuf, _OBSS_SCAN_PARA_IE_, priv->obss_scan_para_len,
				(unsigned char *)&priv->obss_scan_para_buf, &txinsn.fr_len);

			unsigned char temp_buf = _2040_COEXIST_SUPPORT_ ;
			pbuf = set_ie(pbuf, _EXTENDED_CAP_IE_, 1, &temp_buf, &txinsn.fr_len);
		}
#endif

	if (pmib->dot11RsnIE.rsnielen && set_privacy)
	{
		memcpy(pbuf, pmib->dot11RsnIE.rsnie, pmib->dot11RsnIE.rsnielen);
		pbuf += pmib->dot11RsnIE.rsnielen;
		txinsn.fr_len += pmib->dot11RsnIE.rsnielen;
	}

#ifdef SEMI_QOS
	//Set WMM Parameter Element
	if (QOS_ENABLE)
		pbuf = set_ie(pbuf, _RSN_IE_1_, _WMM_Para_Element_Length_, GET_WMM_PARA_IE, &txinsn.fr_len);
#endif

#ifdef WIFI_SIMPLE_CONFIG
	if (pmib->wscEntry.wsc_enable && pmib->wscEntry.probe_rsp_ielen) {
		memcpy(pbuf, pmib->wscEntry.probe_rsp_ie, pmib->wscEntry.probe_rsp_ielen);
		pbuf += pmib->wscEntry.probe_rsp_ielen;
		txinsn.fr_len += pmib->wscEntry.probe_rsp_ielen;
	}
#endif

	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
		/*
			2008-12-16, For Buffalo WLI_CB_AG54L 54Mbps NIC interoperability issue.
			This NIC can not connect to our AP when our AP is set to WPA/TKIP encryption.
			This issue can be fixed after move "HT Capability Info" and "Additional HT Info" in front of "WPA" and "WMM".
		 */
		//pbuf = set_ie(pbuf, _HT_CAP_, priv->ht_cap_len, (unsigned char *)&priv->ht_cap_buf, &txinsn.fr_len);
		//pbuf = set_ie(pbuf, _HT_IE_, priv->ht_ie_len, (unsigned char *)&priv->ht_ie_buf, &txinsn.fr_len);
		pbuf = construct_ht_ie_old_form(priv, pbuf, &txinsn.fr_len);
	}

#ifdef CONFIG_RTK_MESH
	if((TRUE == is_11s))
// deleted by GANTOE for the wrong comparison 2008/12/25 ====
//		&& (ssid_len == strlen(GET_MIB(priv)->dot1180211sInfo.mesh_id)) &&
//		!memcmp((const void*)ssid, (const void*)GET_MIB(priv)->dot1180211sInfo.mesh_id, ssid_len))
	{
		pbuf = set_ie(pbuf, _OFDM_PARAMETER_SET_IE_, 1, ((unsigned char *)&(priv->pmib->dot11RFEntry.dot11channel)) + 3, &txinsn.fr_len);
		pbuf = set_ie(pbuf, _MESH_ID_IE_, mesh_ie_MeshID(priv, meshiearray, FALSE), meshiearray, &txinsn.fr_len);
		pbuf = set_ie(pbuf, _WLAN_MESH_CAP_IE_, mesh_ie_WLANMeshCAP(priv, meshiearray), meshiearray, &txinsn.fr_len);
		pbuf = set_ie(pbuf, _DTIM_IE_, mesh_ie_DTIM(priv, meshiearray), meshiearray, &txinsn.fr_len);
		pbuf = set_ie(pbuf, _BEACON_TIMING_IE_, 29, "Beacon Timing info. element!!", &txinsn.fr_len); 	// I can't understand..
		pbuf = set_ie(pbuf, _MKDDIE_IE_, mesh_ie_MKDDIE(priv, meshiearray), meshiearray, &txinsn.fr_len);
	} else
		is_11s = FALSE;
#endif

	// Realtek proprietary IE
	if (priv->pshare->rtk_ie_len)
		pbuf = set_ie(pbuf, _RSN_IE_1_, priv->pshare->rtk_ie_len, priv->pshare->rtk_ie_buf, &txinsn.fr_len);

	// Customer proprietary IE
	if (priv->pmib->miscEntry.private_ie_len) {
		memcpy(pbuf, pmib->miscEntry.private_ie, pmib->miscEntry.private_ie_len);
		pbuf += pmib->miscEntry.private_ie_len;
		txinsn.fr_len += pmib->miscEntry.private_ie_len;
	}

	SetFrameSubType((txinsn.phdr), WIFI_PROBERSP);

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), da, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);

#if defined(CONFIG_RTL_WAPI_SUPPORT)
		if (priv->pmib->wapiInfo.wapiType!=wapiDisable)
		{
			SAVE_INT_AND_CLI(flags);
			priv->wapiCachedBuf = pbuf+2;
			wapiSetIE(priv);
			pbuf[0] = _EID_WAPI_;
			pbuf[1] = priv->wapiCachedLen;
			pbuf += priv->wapiCachedLen+2;
			txinsn.fr_len += priv->wapiCachedLen+2;
			RESTORE_INT(flags);
		}
#endif

#ifdef CONFIG_RTK_MESH
	if(TRUE == is_11s)  // spec define management frames Address 3 is "null mac" (all zero) (Refer: Draft 1.06, Page 12, 7.2.3, Line 29~30 2007/08/11 by popen)
		memset((void *)GetAddr3Ptr((txinsn.phdr)), 0, MACADDRLEN);
	else
#endif
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), bssid, MACADDRLEN);

	if ((rtl8190_firetx(priv, &txinsn)) == SUCCESS)
		return;

issue_probersp_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
}



/**
 *	@brief	STA issue prob request
 *
 *	+---------------+-----+------+-----------------+--------------------------+	\n
 *	| Frame Control | ... | SSID | Supported Rates | Extended Supported Rates |	\n
 *	+---------------+-----+------+-----------------+--------------------------+	\n
 *	@param	priv	device
 *	@param	ssid	ssid name
 *	@param	ssid_len	ssid length
 */
static void issue_probereq(struct rtl8190_priv *priv, unsigned char *ssid, int ssid_len, unsigned char *da)
{
#ifdef CONFIG_RTK_MESH

	issue_probereq_MP(priv, ssid, ssid_len, da, FALSE);
}
void issue_probereq_MP(struct rtl8190_priv *priv, unsigned char *ssid, int ssid_len, unsigned char *da, int is_11s)
{
	UINT8           meshiearray[32];	// mesh IE buffer (Max byte is mesh_ie_MeshID)
#endif // CONFIG_RTK_MESH

	struct wifi_mib *pmib;
	unsigned char	*hwaddr, *pbuf;
	unsigned char	*pbssrate=NULL;
	int		bssrate_len;
	DECLARE_TXINSN(txinsn);

	txinsn.retry = priv->pmib->dot11OperationEntry.dot11ShortRetryLimit;

	pmib = GET_MIB(priv);

	hwaddr = pmib->dot11OperationEntry.hwaddr;
#ifdef CONFIG_RTK_MESH
	txinsn.is_11s = is_11s;
#endif
	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
	txinsn.lowest_tx_rate = txinsn.tx_rate;
	txinsn.fixed_rate = 1;
	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);

	if (pbuf == NULL)
		goto issue_probereq_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);

	if (txinsn.phdr == NULL)
		goto issue_probereq_fail;

	memset((void *)(txinsn.phdr), 0, sizeof (struct	wlan_hdr));

	pbuf = set_ie(pbuf, _SSID_IE_, ssid_len, ssid, &txinsn.fr_len);

	get_bssrate_set(priv, _SUPPORTEDRATES_IE_, &pbssrate, &bssrate_len);
	pbuf = set_ie(pbuf, _SUPPORTEDRATES_IE_ , bssrate_len , pbssrate, &txinsn.fr_len);

	if (get_bssrate_set(priv, _EXT_SUPPORTEDRATES_IE_, &pbssrate, &bssrate_len))
		pbuf = set_ie(pbuf, _EXT_SUPPORTEDRATES_IE_ , bssrate_len , pbssrate, &txinsn.fr_len);

#ifdef WIFI_SIMPLE_CONFIG
	if (pmib->wscEntry.wsc_enable && pmib->wscEntry.probe_req_ielen) {
		memcpy(pbuf, pmib->wscEntry.probe_req_ie, pmib->wscEntry.probe_req_ielen);
		pbuf += pmib->wscEntry.probe_req_ielen;
		txinsn.fr_len += pmib->wscEntry.probe_req_ielen;
	}
#endif

#ifdef CONFIG_RTK_MESH	// mesh_profile Configure by WEB in the future, Maybe delete, Preservation before delete
	if((TRUE == is_11s) && (1 == GET_MIB(priv)->dot1180211sInfo.mesh_enable) && (TRUE == priv->mesh_profile[0].used)
			&& (MESH_PEER_LINK_CAP_NUM(priv) > 0))
	{
// ==== modified by GANTOE for site survey 2008/12/25 ====
		if(priv->auto_channel == 0)
			pbuf = set_ie(pbuf, _MESH_ID_IE_, 0, "", &txinsn.fr_len);
		else
			pbuf = set_ie(pbuf, _MESH_ID_IE_, mesh_ie_MeshID(priv, meshiearray, FALSE), meshiearray, &txinsn.fr_len);
		pbuf = set_ie(pbuf, _WLAN_MESH_CAP_IE_, mesh_ie_WLANMeshCAP(priv, meshiearray), meshiearray, &txinsn.fr_len);
	}
#endif

	SetFrameSubType(txinsn.phdr, WIFI_PROBEREQ);

	if (da)
		memcpy((void *)GetAddr1Ptr((txinsn.phdr)), da, MACADDRLEN); // unicast
	else
		memset((void *)GetAddr1Ptr((txinsn.phdr)), 0xff, MACADDRLEN); // broadcast
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), hwaddr, MACADDRLEN);
	//nctu note
	// spec define ProbeREQ Address 3 is BSSID or wildcard) (Refer: Draft 1.06, Page 12, 7.2.3, Line 27~28)
	memset((void *)GetAddr3Ptr((txinsn.phdr)), 0xff, MACADDRLEN);

	if ((rtl8190_firetx(priv, &txinsn)) == SUCCESS)
		return;

issue_probereq_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
}


#ifdef SEMI_QOS


void issue_ADDBAreq(struct rtl8190_priv *priv, struct stat_info *pstat, unsigned char TID)
{
	unsigned char	*pbuf;
	unsigned short	ba_para = 0;
	int max_size;
	DECLARE_TXINSN(txinsn);

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
	txinsn.lowest_tx_rate = txinsn.tx_rate;
	txinsn.fixed_rate = 1;

	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);
	if (pbuf == NULL)
		goto issue_ADDBArsp_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);
	if (txinsn.phdr == NULL)
		goto issue_ADDBArsp_fail;

	memset((void *)(txinsn.phdr), 0, sizeof(struct wlan_hdr));

	if (!(++pstat->dialog_token))	// dialog token set to a non-zero value
		pstat->dialog_token++;

	pbuf[0] = _BLOCK_ACK_CATEGORY_ID_;
	pbuf[1] = _ADDBA_Req_ACTION_ID_;
	pbuf[2] = pstat->dialog_token;

		max_size = _ADDBA_Maximum_Buffer_Size_;

	ba_para = (max_size<<6) | (TID<<2) | BIT(1);	// assign buffer size | assign TID | set Immediate Block Ack
	pbuf[3] = ba_para & 0x00ff;
	pbuf[4] = (ba_para & 0xff00) >> 8;

	// set Block Ack Timeout value to zero, to disable the timeout
	pbuf[5] = 0;
	pbuf[6] = 0;

	// set the immediate next seq number of the "TID", as Block Ack Starting Seq
	pbuf[7] = ((pstat->AC_seq[TID] & 0xfff) << 4) & 0x00ff;
	pbuf[8] = (((pstat->AC_seq[TID] & 0xfff) << 4) & 0xff00) >> 8;

	txinsn.fr_len = _ADDBA_Req_Frame_Length_;
	SetFrameSubType((txinsn.phdr), WIFI_WMM_ACTION);
	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), pstat->hwaddr, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);

#ifdef CONFIG_RTK_MESH
	if((GET_MIB(priv)->dot1180211sInfo.mesh_enable) && isMeshPoint(pstat))
		memset((void *)GetAddr3Ptr((txinsn.phdr)), 0x00, MACADDRLEN);
	else
		memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
#else
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
#endif

	DEBUG_INFO("ADDBA-req sent to AID %d, token %d TID %d size %d seq %d\n",
		pstat->aid, pstat->dialog_token, TID, max_size, pstat->AC_seq[TID]);

	if ((rtl8190_firetx(priv, &txinsn)) == SUCCESS) {
		pstat->ADDBA_ready++;
		return;
	}

issue_ADDBArsp_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
	return;
}


static int issue_ADDBArsp(struct rtl8190_priv *priv, unsigned char *da, unsigned char dialog_token,
				unsigned char TID, unsigned short status_code, unsigned short timeout)
{
	unsigned char	*pbuf;
	unsigned short	ba_para = 0;
	int max_size;

	DECLARE_TXINSN(txinsn);

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
	txinsn.lowest_tx_rate = txinsn.tx_rate;
	txinsn.fixed_rate = 1;

	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);
	if (pbuf == NULL)
		goto issue_ADDBArsp_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);
	if (txinsn.phdr == NULL)
		goto issue_ADDBArsp_fail;

	memset((void *)(txinsn.phdr), 0, sizeof(struct wlan_hdr));

	pbuf[0] = _BLOCK_ACK_CATEGORY_ID_;
	pbuf[1] = _ADDBA_Rsp_ACTION_ID_;
	pbuf[2] = dialog_token;
	pbuf[3] = status_code & 0x00ff;
	pbuf[4] = (status_code & 0xff00) >> 8;

		max_size = _ADDBA_Maximum_Buffer_Size_;

	ba_para = (max_size<<6) | (TID<<2) | BIT(1);	// assign buffer size | assign TID | set Immediate Block Ack

	pbuf[5] = ba_para & 0x00ff;
	pbuf[6] = (ba_para & 0xff00) >> 8;
	pbuf[7] = timeout & 0x00ff;
	pbuf[8] = (timeout & 0xff00) >> 8;

	txinsn.fr_len += _ADDBA_Rsp_Frame_Length_;

	SetFrameSubType((txinsn.phdr), WIFI_WMM_ACTION);

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), da, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);

#ifdef CONFIG_RTK_MESH
	if((GET_MIB(priv)->dot1180211sInfo.mesh_enable) && isMeshPoint(get_stainfo(priv, da)))
		memset((void *)GetAddr3Ptr((txinsn.phdr)), 0x00, MACADDRLEN);
	else
		memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
#else
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
#endif

	DEBUG_INFO("ADDBA-rsp sent to AID %d, token %d TID %d size %d status %d\n",
		get_stainfo(priv, da)->aid, dialog_token, TID, max_size, status_code);

	if ((rtl8190_firetx(priv, &txinsn)) == SUCCESS)
		return SUCCESS;

issue_ADDBArsp_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
	return FAIL;
}
#endif


#ifdef RTK_WOW
void issue_rtk_wow(struct rtl8190_priv *priv, unsigned char *da)
{
	unsigned char	*pbuf;
	unsigned int i;
	DECLARE_TXINSN(txinsn);

	if (!(OPMODE & WIFI_AP_STATE)) {
		DEBUG_WARN("rtk_wake_up pkt should be sent in AP mode!!\n");
		return;
	}

	txinsn.retry = priv->pmib->dot11OperationEntry.dot11ShortRetryLimit;
	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
	txinsn.lowest_tx_rate = txinsn.tx_rate;
	txinsn.fixed_rate = 1;

	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);
	if (pbuf == NULL)
		goto send_rtk_wake_up_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);
	if (txinsn.phdr == NULL)
		goto send_rtk_wake_up_fail;

	memset((void *)(txinsn.phdr), 0, sizeof (struct	wlan_hdr));

	SetFrameSubType(txinsn.phdr, WIFI_DATA);
	SetFrDs(txinsn.phdr);
	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), da, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);

	// sync stream
	memset((void *)pbuf, 0xff, MACADDRLEN);
	pbuf += MACADDRLEN;
	txinsn.fr_len += MACADDRLEN;

	for(i=0; i<16; i++) {
		memcpy((void *)pbuf, da, MACADDRLEN);
		pbuf += MACADDRLEN;
		txinsn.fr_len += MACADDRLEN;
	}

	if ((rtl8190_firetx(priv, &txinsn)) == SUCCESS) {
		DEBUG_INFO("RTK wake up pkt sent\n");
		return;
	}
	else {
		DEBUG_ERR("Fail to send RTK wake up pkt\n");
	}

send_rtk_wake_up_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
}
#endif

/**
 *	@brief	Process Site Survey
 *
 *	set site survery, reauth. , reassoc, idle_timer and proces Site survey \n
 *	PS: ss_timer is site survey timer	\n
 */
void start_clnt_ss(struct rtl8190_priv *priv)
{
	unsigned long	flags;
#if defined(CLIENT_MODE) && defined(MBSSID)
	unsigned long	ioaddr = priv->pshare->ioaddr;
#endif

#ifdef RTL8192SU_SW_BEACON
	if (timer_pending(&priv->beacon_timer))
		del_timer_sync(&priv->beacon_timer);
#endif
	if (timer_pending(&priv->ss_timer))
		del_timer_sync(&priv->ss_timer);
#ifdef CLIENT_MODE
	if (timer_pending(&priv->reauth_timer))
		del_timer_sync(&priv->reauth_timer);
	if (timer_pending(&priv->reassoc_timer))
		del_timer_sync(&priv->reassoc_timer);
	if (timer_pending(&priv->idle_timer))
		del_timer_sync(&priv->idle_timer);
#endif

	OPMODE &= (~WIFI_SITE_MONITOR);

	SAVE_INT_AND_CLI(flags);
	priv->site_survey_times = 0;

// mark by david ---------------
//--------------------2007-04-14

	priv->site_survey.ss_channel = priv->available_chnl[0];
	priv->site_survey.count = 0;

#ifdef UNIVERSAL_REPEATER
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
#ifdef DFS
		if (!priv->pmib->dot11DFSEntry.disable_DFS &&
			(priv->site_survey.ss_channel >= 52))
			priv->pmib->dot11DFSEntry.disable_tx = 1;
		else
			priv->pmib->dot11DFSEntry.disable_tx = 0;
#endif
		priv->pshare->CurrentChannelBW = HT_CHANNEL_WIDTH_20;
		SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);
		SwChnl(priv, priv->site_survey.ss_channel, priv->pshare->offset_2nd_chan);
	}

	memset((void *)&priv->site_survey.bss, 0, sizeof(struct bss_desc)*MAX_BSS_NUM);
#ifdef WIFI_SIMPLE_CONFIG
	if (priv->ss_req_ongoing == 2)
		memset((void *)&priv->site_survey.ie, 0, MAX_BSS_NUM*MAX_WSC_IE_LEN);
#endif
#if defined(CLIENT_MODE) && defined(MBSSID)
	if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
	{
		if ((OPMODE & WIFI_STATION_STATE) || (OPMODE & WIFI_ADHOC_STATE))
			RTL_W32(_RCR_, RTL_R32(_RCR_) & ~_CBSSID_);
	}
#endif
	DIG_for_site_survey(priv, TRUE);
	OPMODE |= WIFI_SITE_MONITOR;
	RESTORE_INT(flags);

	{
#ifdef UNIVERSAL_REPEATER
	// if request from vxd interface and root interface is not in scanning, send probe-req
		unsigned long ioaddr = priv->pshare->ioaddr;
		unsigned long is_tx_enabled = (RTL_R8(CMDR)&TXDMA_EN) ? 1 : 0;

		if (IS_ROOT_INTERFACE(priv) ||
			(GET_ROOT_PRIV(priv) && is_tx_enabled &&
			!(GET_ROOT_PRIV(priv)->pmib->dot11OperationEntry.opmode & WIFI_SITE_MONITOR))
		  )
#endif
		{
#ifdef DFS
			if (!(!priv->pmib->dot11DFSEntry.disable_DFS &&
				(priv->site_survey.ss_channel >= 52)))
#endif
			{
#ifdef CONFIG_RTK_MESH
		//  GANTOE for site survey 2008/12/25 ====
				if(GET_MIB(priv)->dot1180211sInfo.mesh_enable)
					issue_probereq_MP(priv, NULL, 0, NULL, TRUE);
				else
#endif
				{
				if (priv->ss_ssidlen == 0)
					issue_probereq(priv, NULL, 0, NULL);
				else
					issue_probereq(priv, priv->ss_ssid, priv->ss_ssidlen, NULL);
				}
			}
		}
	}
#ifdef CONFIG_RTK_MESH
	//GANTOE for site survey 2008/12/25 ====
	if(priv->auto_channel & 0x30)
	{
		SET_PSEUDO_RANDOM_NUMBER(flags);
		flags %= SS_RAND_DEFER;
	} else
		flags=0;
#endif
#ifdef DFS
	if (!priv->pmib->dot11DFSEntry.disable_DFS &&
		(priv->site_survey.ss_channel >= 52))
		mod_timer(&priv->ss_timer, jiffies + SS_PSSV_TO
		#ifdef CONFIG_RTK_MESH //GANTOE for site survey 2008/12/25
			+ ( flags ) // for the deafness problem
		#endif
		);
	else
#endif
		mod_timer(&priv->ss_timer, jiffies + SS_TO
		#ifdef CONFIG_RTK_MESH 		//GANTOE for site survey 2008/12/25
			+ ( flags ) // for the deafness problem
		#endif
		);
}


static void qsort (void  *base, int nel, int width,
				int (*comp)(const void *, const void *))
{
	int wgap, i, j, k;
	unsigned char tmp;

	if ((nel > 1) && (width > 0)) {
		//assert( nel <= ((size_t)(-1)) / width ); /* check for overflow */
		wgap = 0;
		do {
			wgap = 3 * wgap + 1;
		} while (wgap < (nel-1)/3);
		/* From the above, we know that either wgap == 1 < nel or */
		/* ((wgap-1)/3 < (int) ((nel-1)/3) <= (nel-1)/3 ==> wgap <  nel. */
		wgap *= width;			/* So this can not overflow if wnel doesn't. */
		nel *= width;			/* Convert nel to 'wnel' */
		do {
			i = wgap;
			do {
				j = i;
				do {
					register unsigned char *a;
					register unsigned char *b;

					j -= wgap;
					a = (unsigned char *)(j + ((char *)base));
					b = a + wgap;
					if ( (*comp)(a, b) <= 0 ) {
						break;
					}
					k = width;
					do {
						tmp = *a;
						*a++ = *b;
						*b++ = tmp;
					} while ( --k );
				} while (j >= wgap);
				i += width;
			} while (i < nel);
			wgap = (wgap - width)/3;
		} while (wgap);
	}
}


static int compareBSS(const void *entry1, const void *entry2)
{
	if (((struct bss_desc *)entry1)->rssi > ((struct bss_desc *)entry2)->rssi)
		return -1;

	if (((struct bss_desc *)entry1)->rssi < ((struct bss_desc *)entry2)->rssi)
		return 1;

	return 0;
}

#ifdef WIFI_SIMPLE_CONFIG
static int compareWpsIE(const void *entry1, const void *entry2)
{
	if (((struct wps_ie_info *)entry1)->rssi > ((struct wps_ie_info *)entry2)->rssi)
		return -1;

	if (((struct wps_ie_info *)entry1)->rssi < ((struct wps_ie_info *)entry2)->rssi)
		return 1;

	return 0;
}
#endif

static void debug_print_bss(struct rtl8190_priv *priv)
{
}


void rtl8190_ss_timer(unsigned long task_priv)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)task_priv;
	int idx, loop_finish=0;
#if defined(CLIENT_MODE) && defined(MBSSID)
	unsigned long ioaddr = priv->pshare->ioaddr;
#endif
#if defined(MBSSID) || defined(SIMPLE_CH_UNI_PROTOCOL) //for mesh
	int i;
#endif
	struct wifi_mib	*pmib = GET_MIB(priv);

	for (idx=0; idx<priv->available_chnl_num; idx++)
		if (priv->site_survey.ss_channel == priv->available_chnl[idx])
			break;

	if (idx == (priv->available_chnl_num - 1))
		loop_finish = 1;
	else {
// mark by david ------------------------
//--------------------------- 2007-04-14

		priv->site_survey.ss_channel = priv->available_chnl[idx+1];
	}

	if (loop_finish)
	{
		priv->site_survey_times++;
#ifdef SIMPLE_CH_UNI_PROTOCOL
		if(GET_MIB(priv)->dot1180211sInfo.mesh_enable && (priv->auto_channel & 0x30) )
		{
			if( priv->auto_channel == 0x10 )
			{
				if( priv->site_survey_times >= _11S_SS_COUNT1 ){
					if( !priv->pmib->dot11RFEntry.dot11channel)
					{
						priv->pmib->dot11RFEntry.dot11channel = selectClearChannel(priv);
						SET_PSEUDO_RANDOM_NUMBER(priv->mesh_ChannelPrecedence);
					}
					priv->auto_channel = 0x20;
				}
			}
			else
			{
				if( priv->site_survey_times >= 	_11S_SS_COUNT1+_11S_SS_COUNT2)
					priv->auto_channel = 1;
			}
			for(i=0; i<priv->available_chnl_num; i++)
			{
				get_random_bytes(&(idx), sizeof(idx));
				idx %= priv->available_chnl_num;
				loop_finish = priv->available_chnl[idx];
				priv->available_chnl[idx] = priv->available_chnl[i];
				priv->available_chnl[i] = loop_finish;
			}
			priv->site_survey.ss_channel = priv->available_chnl[0];
		}
		else
#endif

// only do multiple scan when site-survey request, david+2006-01-25
//		if (priv->site_survey_times < SS_COUNT)
		if (priv->ss_req_ongoing && priv->site_survey_times < SS_COUNT)
		{
// mark by david ---------------------
//------------------------ 2007-04-14
			priv->site_survey.ss_channel = priv->available_chnl[0];

		}
		else
		{
			// scan end
			OPMODE &= ~WIFI_SITE_MONITOR;
			DIG_for_site_survey(priv, FALSE);
#if defined(CLIENT_MODE) && defined(MBSSID)
			if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
			{
				if ((OPMODE & WIFI_STATION_STATE) || (OPMODE & WIFI_ADHOC_STATE))
					RTL_W32(_RCR_, RTL_R32(_RCR_) | _CBSSID_);
			}
#endif
#ifdef UNIVERSAL_REPEATER
			if (IS_ROOT_INTERFACE(priv))
#endif
			{
#ifdef DFS
				if (!priv->pmib->dot11DFSEntry.disable_DFS &&
					(timer_pending(&priv->ch_avail_chk_timer)))
					priv->pmib->dot11DFSEntry.disable_tx = 1;
				else
					priv->pmib->dot11DFSEntry.disable_tx = 0;
#endif
				if (priv->ss_req_ongoing) {
					priv->pshare->CurrentChannelBW = priv->pshare->is_40m_bw;
					SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);
					SwChnl(priv, priv->pmib->dot11RFEntry.dot11channel, priv->pshare->offset_2nd_chan);
				}
			}

			qsort(priv->site_survey.bss, priv->site_survey.count, sizeof(struct bss_desc), compareBSS);
#ifdef WIFI_SIMPLE_CONFIG
			qsort(priv->site_survey.ie, priv->site_survey.count, sizeof(struct wps_ie_info), compareWpsIE);
#endif
			debug_print_bss(priv);

			if (priv->auto_channel == 1) {
#ifdef SIMPLE_CH_UNI_PROTOCOL
				if(!GET_MIB(priv)->dot1180211sInfo.mesh_enable || !priv->pmib->dot11RFEntry.dot11channel)
#endif
				priv->pmib->dot11RFEntry.dot11channel = selectClearChannel(priv);
				DEBUG_INFO("auto channel select ch %d\n", priv->pmib->dot11RFEntry.dot11channel);

#if defined(CONFIG_RTL8196B_TR)
				LOG_START_MSG();
#endif
//#ifdef CONFIG_RTL865X_AC
#if defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
				LOG_START_MSG();
#endif

#ifdef DFS
#ifdef UNIVERSAL_REPEATER
				if (IS_ROOT_INTERFACE(priv))
#endif
				{
				 	if (!priv->pmib->dot11DFSEntry.disable_DFS &&
							(priv->pmib->dot11RFEntry.dot11channel >= 52) &&
							(OPMODE & WIFI_AP_STATE)) {
						priv->auto_channel = 0;

						// to prevent kill tasklet issues
						// please refer to rtl8190_stop_sw()
						priv->pmib->dot11DFSEntry.DFS_detected = 1;
						priv->pmib->dot11OperationEntry.keep_rsnie = 1; // recovery in WPA case, david+2006-01-27
#ifdef MBSSID
						if (priv->pmib->miscEntry.vap_enable)
						{
							for (i=0; i<RTL8190_NUM_VWLAN; i++) {
								if (IS_DRV_OPEN(priv->pvap_priv[i]))
									priv->pvap_priv[i]->pmib->dot11OperationEntry.keep_rsnie = 1;
							}
						}
#endif
						rtl8190_close(priv->dev);
						rtl8190_open(priv->dev);

						return;
					}
				}
#endif

				priv->pshare->CurrentChannelBW = priv->pshare->is_40m_bw;
				SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);
				SwChnl(priv, priv->pmib->dot11RFEntry.dot11channel, priv->pshare->offset_2nd_chan);
				init_beacon(priv);
#ifdef SIMPLE_CH_UNI_PROTOCOL
				printk("scan finish, sw ch to (#%d), init beacon\n", priv->pmib->dot11RFEntry.dot11channel);
#endif
#ifdef MBSSID
				if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
				{
					for (i=0; i<RTL8190_NUM_VWLAN; i++) {
						if (IS_DRV_OPEN(priv->pvap_priv[i])) {
							priv->pvap_priv[i]->pmib->dot11RFEntry.dot11channel = priv->pmib->dot11RFEntry.dot11channel;
							init_beacon(priv->pvap_priv[i]);
						}
					}
				}
#endif
				if (OPMODE & WIFI_AP_STATE)
					priv->auto_channel = 0;
				else
					priv->auto_channel = 2;
#ifdef CLIENT_MODE
				if (priv->join_res == STATE_Sta_Ibss_Idle) {
					unsigned long ioaddr = priv->pshare->ioaddr;
					RTL_W8(_MSR_, _HW_STATE_ADHOC_);
					mod_timer(&priv->idle_timer, jiffies + 500);
				}
#endif
			}
			// backup the bss database
			else if (priv->ss_req_ongoing) {
				priv->site_survey.count_backup = priv->site_survey.count;
				memcpy(priv->site_survey.bss_backup, priv->site_survey.bss, sizeof(struct bss_desc)*priv->site_survey.count);
#ifdef WIFI_SIMPLE_CONFIG
				memcpy(priv->site_survey.ie_backup, priv->site_survey.ie, sizeof(struct wps_ie_info)*priv->site_survey.count);
#endif

#ifdef CLIENT_MODE
				if (priv->join_res == STATE_Sta_Ibss_Idle)
					start_clnt_lookup(priv, 1);
				else if (priv->join_res == STATE_Sta_Auth_Success){
						start_clnt_assoc(priv);
				}
				else if (priv->join_res == STATE_Sta_Roaming_Scan)
					start_clnt_lookup(priv, 1);
				else
					;
#endif
				priv->ss_req_ongoing = 0;
			}
#ifdef CLIENT_MODE
			else if (OPMODE & (WIFI_STATION_STATE | WIFI_ADHOC_STATE)) {
				priv->site_survey.count_target = priv->site_survey.count;
				memcpy(priv->site_survey.bss_target, priv->site_survey.bss, sizeof(struct bss_desc)*priv->site_survey.count);
				priv->join_index = -1;

				if (priv->join_res == STATE_Sta_Roaming_Scan)
					start_clnt_lookup(priv, 0);
				else
					;
			}
#endif
			else {
				DEBUG_ERR("Faulty scanning\n");
			}

#ifdef CHECK_BEACON_HANGUP
			priv->pshare->beacon_wait_cnt = 2;
#endif
#ifdef RTL8192SU_SW_BEACON
			mod_timer(&priv->beacon_timer, jiffies+pmib->dot11StationConfigEntry.dot11BeaconPeriod);
			OPMODE &= (~WIFI_WAIT_FOR_CHANNEL_SELECT);
#ifdef MBSSID
			for (i=0; i<RTL8190_NUM_VWLAN; i++)
			{
				if (IS_DRV_OPEN(priv->pvap_priv[i]))
				{
					struct rtl8190_priv *pvap_priv = priv->pvap_priv[i];
					mod_timer(&pvap_priv->beacon_timer, jiffies+pmib->dot11StationConfigEntry.dot11BeaconPeriod);
				}
			}
#endif
#endif

			return;
		}
	}

#ifdef UNIVERSAL_REPEATER
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
	// now, change RF channel...
		SwChnl(priv, priv->site_survey.ss_channel, priv->pshare->offset_2nd_chan);

#ifdef DFS
		if (!priv->pmib->dot11DFSEntry.disable_DFS) {
			if (priv->site_survey.ss_channel < 52)
				priv->pmib->dot11DFSEntry.disable_tx = 0;
			else
				priv->pmib->dot11DFSEntry.disable_tx = 1;
		}
#endif
	}

	{
#ifdef UNIVERSAL_REPEATER
		// if vxd interface and root interface is not in scanning, send probe-req
		unsigned long ioaddr = priv->pshare->ioaddr;
		unsigned long is_tx_enabled = (RTL_R8(CMDR)&TXDMA_EN) ? 1 : 0;

		if (IS_ROOT_INTERFACE(priv) ||
			(GET_ROOT_PRIV(priv) && is_tx_enabled &&
			!(GET_ROOT_PRIV(priv)->pmib->dot11OperationEntry.opmode & WIFI_SITE_MONITOR))
		  )
#endif
		{
#ifdef DFS
			if (!(!priv->pmib->dot11DFSEntry.disable_DFS &&
				(priv->site_survey.ss_channel >= 52)))
#endif
			{
#ifdef SIMPLE_CH_UNI_PROTOCOL
				if(GET_MIB(priv)->dot1180211sInfo.mesh_enable)
					issue_probereq_MP(priv, "MESH-SCAN", 9, NULL, TRUE);
				else
#endif
				// issue probe_req here...
				if (priv->ss_ssidlen == 0)
					issue_probereq(priv, NULL, 0, NULL);
				else
					issue_probereq(priv, priv->ss_ssid, priv->ss_ssidlen, NULL);
			}
		}
	}

	// now, start another timer again.
#ifdef DFS
	if (!priv->pmib->dot11DFSEntry.disable_DFS &&
		(priv->site_survey.ss_channel >= 52))
		mod_timer(&priv->ss_timer, jiffies + SS_PSSV_TO);
	else
#endif
		mod_timer(&priv->ss_timer, jiffies + SS_TO);
}


/**
 *	@brief  get WPA/WPA2 information
 *
 *	use 1 timestamp (32-bit variable) to carry WPA/WPA2 info \n
 *	1st 16-bit:                 WPA \n
 *  |          auth       |              unicast cipher              |              multicast cipher            |	\n
 *     15    14    13   12      11      10     9     8       7      6      5       4      3     2       1      0	\n
 *	+-----+-----+----+-----+--------+------+-----+------+-------+-----+--------+------+-----+------+-------+-----+	\n
 *	| Rsv | PSK | 1X | Rsv | WEP104 | CCMP | Rsv | TKIP | WEP40 | Grp | WEP104 | CCMP | Rsv | TKIP | WEP40 | Grp |	\n
 *	+-----+-----+----+-----+--------+------+-----+------+-------+-----+--------+------+-----+------+-------+-----+	\n
 *	2nd 16-bit:                 WPA2 \n
 *            auth       |              unicast cipher              |              multicast cipher            |	\n
 *	  15    14    13   12      11      10     9     8       7      6      5       4      3     2       1      0		\n
 *  +-----+-----+----+-----+--------+------+-----+------+-------+-----+--------+------+-----+------+-------+-----+	\n
 *	| Rsv | PSK | 1X | Rsv | WEP104 | CCMP | Rsv | TKIP | WEP40 | Grp | WEP104 | CCMP | Rsv | TKIP | WEP40 | Grp |	\n
 *  +-----+-----+----+-----+--------+------+-----+------+-------+-----+--------+------+-----+------+-------+-----+	\n
 */
static void get_security_info(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo, int index)
{
	int i, len, result;
	unsigned char *p, *pframe, *p_uni, *p_auth, val;
	unsigned short num;
	unsigned char OUI1[] = {0x00, 0x50, 0xf2};
	unsigned char OUI2[] = {0x00, 0x0f, 0xac};

	pframe = get_pframe(pfrinfo);
	priv->site_survey.bss[index].t_stamp[0] = 0;

	p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_;
	len = 0;
	result = 0;
	do {
		p = get_ie(p, _RSN_IE_1_, &len,
			pfrinfo->pktlen - (p - pframe));
		if ((p != NULL) && (len > 18))
		{
			if (memcmp((p + 2), OUI1, 3))
				goto next_tag;
			if (*(p + 5) != 0x01)
				goto next_tag;
			if (memcmp((p + 8), OUI1, 3))
				goto next_tag;
			val = *(p + 11);
			priv->site_survey.bss[index].t_stamp[0] |= BIT(val);
			p_uni = p + 12;
			memcpy(&num, p_uni, 2);
			num = le16_to_cpu(num);
			for (i=0; i<num; i++) {
				if (memcmp((p_uni + 2 + 4 * i), OUI1, 3))
					goto next_tag;
				val = *(p_uni + 2 + 4 * i + 3);
				priv->site_survey.bss[index].t_stamp[0] |= (BIT(val) << 6);
			}
			p_auth = p_uni + 2 + 4 * num;
			memcpy(&num, p_auth, 2);
			num = le16_to_cpu(num);
			for (i=0; i<num; i++) {
				if (memcmp((p_auth + 2 + 4 * i), OUI1, 3))
					goto next_tag;
				val = *(p_auth + 2 + 4 * i + 3);
				priv->site_survey.bss[index].t_stamp[0] |= (BIT(val) << 12);
			}
			result = 1;
		}
next_tag:
		if (p != NULL)
			p = p + 2 + len;
	} while ((p != NULL) && (result != 1));

	if (result != 1)
	{
		priv->site_survey.bss[index].t_stamp[0] = 0;
	}

	p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_;
	len = 0;
	result = 0;
	do {
		p = get_ie(p, _RSN_IE_2_, &len,
			pfrinfo->pktlen - (p - pframe));
		if ((p != NULL) && (len > 12))
		{
			if (memcmp((p + 4), OUI2, 3))
				goto next_id;
			val = *(p + 7);
			priv->site_survey.bss[index].t_stamp[0] |= (BIT(val) << 16);
			p_uni = p + 8;
			memcpy(&num, p_uni, 2);
			num = le16_to_cpu(num);
			for (i=0; i<num; i++) {
				if (memcmp((p_uni + 2 + 4 * i), OUI2, 3))
					goto next_id;
				val = *(p_uni + 2 + 4 * i + 3);
				priv->site_survey.bss[index].t_stamp[0] |= (BIT(val) << 22);
			}
			p_auth = p_uni + 2 + 4 * num;
			memcpy(&num, p_auth, 2);
			num = le16_to_cpu(num);
			for (i=0; i<num; i++) {
				if (memcmp((p_auth + 2 + 4 * i), OUI2, 3))
					goto next_id;
				val = *(p_auth + 2 + 4 * i + 3);
				priv->site_survey.bss[index].t_stamp[0] |= (BIT(val) << 28);
			}
			result = 1;
		}
next_id:
		if (p != NULL)
			p = p + 2 + len;
	} while ((p != NULL) && (result != 1));

	if (result != 1)
	{
		priv->site_survey.bss[index].t_stamp[0] &= 0x0000ffff;
	}

#ifdef WIFI_SIMPLE_CONFIG
	if (priv->ss_req_ongoing == 2) { // simple-config scan-req
		p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_;
		for (;;)
		{
			p = get_ie(p, _WPS_IE_, &len, pfrinfo->pktlen - (p - pframe));
			if (p != NULL) {
				if (!memcmp(p+2, WSC_IE_OUI, 4)) {
					if (len > MAX_WSC_IE_LEN) {
						DEBUG_ERR("WSC IE length too long [%d]!\n", len);
						continue;
					}
					memcpy(&priv->site_survey.ie[index].data[0], p, len+2);
					break;
				}
			}
			else
				break;

			p = p + len + 2;
		}
	}
#endif
}


/**
 *	@brief	After site survey, collect BSS information to site_survey.bss[index]
 *
 *	The function can find site
 *  Later finish site survey, call the function get BSS informat.
 */
#ifndef CONFIG_RTK_MESH
static
#endif
int collect_bss_info(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo)
{
	int i, index, len;
	unsigned char *addr, *p, *pframe, *sa, channel;
	UINT32	basicrate=0, supportrate=0, hiddenAP=0;
	UINT16	val16;
	struct wifi_mib *pmib;
	DOT11_WPA_MULTICAST_CIPHER wpaMulticastCipher;
	unsigned char OUI1[] = {0x00, 0x50, 0xf2};
	DOT11_WPA2_MULTICAST_CIPHER wpa2MulticastCipher;
	unsigned char OUI2[] = {0x00, 0x0f, 0xac};

	pframe = get_pframe(pfrinfo);
#ifdef CONFIG_RTK_MESH
// GANTOE for site survey 2008/12/25 ====
	if(pfrinfo->is_11s)
		addr = GetAddr2Ptr(pframe);
	else
#endif
		addr = GetAddr3Ptr(pframe);

	sa = GetAddr2Ptr(pframe);
	pmib = GET_MIB(priv);

#ifdef WIFI_11N_2040_COEXIST
	if (priv->pshare->rf_ft_var.coexist_enable && (OPMODE & WIFI_AP_STATE) &&
		(priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) && priv->pshare->is_40m_bw) {
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_CAP_,
			&len, pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p == NULL) {
			priv->bg_ap_timeout = 60;
		}
	}
#endif

	if (priv->site_survey.count >= MAX_BSS_NUM)
		return 0;

	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _DSSET_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	if (p != NULL)
		channel = *(p+2);
	else
		channel = priv->site_survey.ss_channel;

	for(i=0; i<priv->site_survey.count; i++)
	{
		if (!memcmp((void *)addr, priv->site_survey.bss[i].bssid, MACADDRLEN)) {
			if (channel == priv->site_survey.bss[i].channel) {
				if ((unsigned char)pfrinfo->rssi > priv->site_survey.bss[i].rssi) {
					priv->site_survey.bss[i].rssi = (unsigned char)pfrinfo->rssi;
#ifdef WIFI_SIMPLE_CONFIG
					priv->site_survey.ie[i].rssi = priv->site_survey.bss[i].rssi;
#endif
				}
				if ((unsigned char)pfrinfo->sq > priv->site_survey.bss[i].sq)
					priv->site_survey.bss[i].sq = (unsigned char)pfrinfo->sq;
				return SUCCESS;
			}
		}
	}

	// checking SSID
	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _SSID_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);

	if ((p == NULL) ||		// NULL AP case 1
		(len == 0) ||		// NULL AP case 2
		(*(p+2) == '\0'))	// NULL AP case 3 (like 8181/8186)
	{
		if (priv->ss_req_ongoing && pmib->miscEntry.show_hidden_bss)
			hiddenAP = 1;
		else if (priv->auto_channel == 1)
			hiddenAP = 1;
		else {
			DEBUG_INFO("drop beacon/probersp due to null ssid\n");
			return 0;
		}
	}

	// if scan specific SSID
	if (priv->ss_ssidlen > 0)
		if ((priv->ss_ssidlen != len) || memcmp(priv->ss_ssid, p+2, len))
			return 0;

	//printk("priv->ss_ssid = %s, priv->ss_ssidlen=%d\n", priv->ss_ssid, priv->ss_ssidlen);

	// if scan specific SSID && WPA2 enabled
	if (priv->ss_ssidlen > 0) {
		// search WPA2 IE
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _RSN_IE_2_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p != NULL) {
			// RSN IE
			// 0	1	23	4567
			// ID	Len	Versin	GroupCipherSuite
			if ((len > 7) && (pmib->dot11RsnIE.rsnie[0] == _RSN_IE_2_) &&
					(pmib->dot11RsnIE.rsnie[7] != *(p+7)) &&
					!memcmp((p + 4), OUI2, 3)) {
				// set WPA2 Multicast Cipher as same as AP's
				//printk("WPA2 Multicast Cipher = %d\n", *(p+7));
				pmib->dot11RsnIE.rsnie[7] = *(p+7);
			}
#ifndef WITHOUT_ENQUEUE
			wpa2MulticastCipher.EventId = DOT11_EVENT_WPA2_MULTICAST_CIPHER;
			wpa2MulticastCipher.IsMoreEvent = 0;
			wpa2MulticastCipher.MulticastCipher = *(p+7);
			DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&wpa2MulticastCipher,
						sizeof(DOT11_WPA2_MULTICAST_CIPHER));
#endif
			event_indicate(priv, NULL, -1);
// button 2009.05.21
#ifdef INCLUDE_WPA_PSK
			psk_indicate_evt(priv, DOT11_EVENT_WPA2_MULTICAST_CIPHER, GetAddr2Ptr(pframe), p+7, 1);
#endif

		}
	}

	// david, reported multicast cipher suite for WPA
	// if scan specific SSID && WPA2 enabled
	if (priv->ss_ssidlen > 0) {
		// search WPA IE, should skip that not real RSNIE (eg. Asus WL500g-Deluxe)
		p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_;
		len = 0;
		do {
			p = get_ie(p, _RSN_IE_1_, &len,
				pfrinfo->pktlen - (p - pframe));
			if (p != NULL) {
				if ((len > 11) && (pmib->dot11RsnIE.rsnie[0] == _RSN_IE_1_) &&
						(pmib->dot11RsnIE.rsnie[11] != *(p+11)) &&
						!memcmp((p + 2), OUI1, 3) &&
						(*(p + 5) == 0x01)) {
					// set WPA Multicast Cipher as same as AP's
					pmib->dot11RsnIE.rsnie[11] = *(p+11);

#ifndef WITHOUT_ENQUEUE
					wpaMulticastCipher.EventId = DOT11_EVENT_WPA_MULTICAST_CIPHER;
					wpaMulticastCipher.IsMoreEvent = 0;
					wpaMulticastCipher.MulticastCipher = *(p+11);
					DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&wpaMulticastCipher,
							sizeof(DOT11_WPA_MULTICAST_CIPHER));
#endif
					event_indicate(priv, NULL, -1);
// button 2009.05.21
#ifdef INCLUDE_WPA_PSK
					psk_indicate_evt(priv, DOT11_EVENT_WPA_MULTICAST_CIPHER, GetAddr2Ptr(pframe), p+11, 1);
#endif
				}
			}
			if (p != NULL)
				p = p + 2 + len;
		} while (p != NULL);
	}

	for(i=0; i<priv->available_chnl_num; i++) {
		if (channel == priv->available_chnl[i])
			break;
	}
	if (i == priv->available_chnl_num)	// receive the adjacent channel that is not our domain
		return 0;

	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _SUPPORTEDRATES_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	if (p != NULL) {
		for(i=0; i<len; i++) {
			if (p[2+i] & 0x80)
				basicrate |= get_bit_value_from_ieee_value(p[2+i] & 0x7f);
			supportrate |= get_bit_value_from_ieee_value(p[2+i] & 0x7f);
		}
	}

	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _EXT_SUPPORTEDRATES_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	if (p != NULL) {
		for(i=0; i<len; i++) {
			if (p[2+i] & 0x80)
				basicrate |= get_bit_value_from_ieee_value(p[2+i] & 0x7f);
			supportrate |= get_bit_value_from_ieee_value(p[2+i] & 0x7f);
		}
	}

	if (channel <= 14)
	{
		if (!(pmib->dot11BssType.net_work_type & WIRELESS_11B))
			if (((basicrate & 0xff0) == 0) && ((supportrate & 0xff0) == 0))
				return 0;

		if (!(pmib->dot11BssType.net_work_type & WIRELESS_11G))
			if (((basicrate & 0xf) == 0) && ((supportrate & 0xf) == 0))
				return 0;
	}

	/*
	 * okay, recording this bss...
	 */
	index = priv->site_survey.count;
	priv->site_survey.count++;

	memcpy(priv->site_survey.bss[index].bssid, addr, MACADDRLEN);

	if (hiddenAP) {
		priv->site_survey.bss[index].ssidlen = 0;
		memset((void *)(priv->site_survey.bss[index].ssid),0, 32);
	}
	else {
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _SSID_IE_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		priv->site_survey.bss[index].ssidlen = len;
		memcpy((void *)(priv->site_survey.bss[index].ssid), (void *)(p+2), len);
	}
#ifdef CONFIG_RTK_MESH
	// GANTOE for site survey 2008/12/25 ====
	//Mesh ID
	p = get_ie(pframe + WLAN_HDR_A3_LEN + _PROBERSP_IE_OFFSET_, _MESH_ID_IE_, (int *)&len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _PROBERSP_IE_OFFSET_);
	if(NULL == p)
	{
		priv->site_survey.bss[index].meshidlen = 0;
		priv->site_survey.bss[index].meshid[0] = '\0';
	}
	else
	{
		priv->site_survey.bss[index].meshidlen = (len > MESH_ID_LEN ? MESH_ID_LEN : len);;
		memcpy((void *)(priv->site_survey.bss[index].meshid), (void *)(p + 2), priv->site_survey.bss[index].meshidlen);
	}
#endif

	// we use t_stamp to carry other info so don't get timestamp here

	memcpy(&val16, (pframe + WLAN_HDR_A3_LEN + 8 ), 2);
	priv->site_survey.bss[index].beacon_prd = le16_to_cpu(val16);

	memcpy(&val16, (pframe + WLAN_HDR_A3_LEN + 8 + 2), 2);
	priv->site_survey.bss[index].capability = le16_to_cpu(val16);

	if ((priv->site_survey.bss[index].capability & BIT(0)) &&
		!(priv->site_survey.bss[index].capability & BIT(1)))
		priv->site_survey.bss[index].bsstype = WIFI_AP_STATE;
	else if (!(priv->site_survey.bss[index].capability & BIT(0)) &&
		(priv->site_survey.bss[index].capability & BIT(1)))
		priv->site_survey.bss[index].bsstype = WIFI_ADHOC_STATE;
	else
		priv->site_survey.bss[index].bsstype = 0;

	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _TIM_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	if (p != NULL)
		priv->site_survey.bss[index].dtim_prd = *(p+3);

	priv->site_survey.bss[index].channel = channel;
	priv->site_survey.bss[index].basicrate = basicrate;
	priv->site_survey.bss[index].supportrate = supportrate;

	memcpy(priv->site_survey.bss[index].bdsa, sa, MACADDRLEN);

	priv->site_survey.bss[index].rssi = (unsigned char)pfrinfo->rssi;
	priv->site_survey.bss[index].sq = (unsigned char)pfrinfo->sq;

#ifdef WIFI_SIMPLE_CONFIG
	priv->site_survey.ie[index].rssi = priv->site_survey.bss[index].rssi;
#endif

	if (channel >= 36)
		priv->site_survey.bss[index].network |= WIRELESS_11A;
	else {
		if ((basicrate & 0xff0) || (supportrate & 0xff0))
			priv->site_survey.bss[index].network |= WIRELESS_11G;
		if ((basicrate & 0xf) || (supportrate & 0xf))
			priv->site_survey.bss[index].network |= WIRELESS_11B;
	}

	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_CAP_, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	if (p !=  NULL) {
		struct ht_cap_elmt *ht_cap=(struct ht_cap_elmt *)(p+2);
		if (cpu_to_le16(ht_cap->ht_cap_info) & _HTCAP_SUPPORT_CH_WDTH_)
			priv->site_survey.bss[index].t_stamp[1] |= BIT(1);
		else
			priv->site_survey.bss[index].t_stamp[1] &= ~(BIT(1));
		priv->site_survey.bss[index].network |= WIRELESS_11N;
	}
	else
		priv->site_survey.bss[index].t_stamp[1] &= ~(BIT(1));

	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_IE_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);

	if (p !=  NULL) {
		struct ht_info_elmt *ht_info=(struct ht_info_elmt *)(p+2);
		if (!(ht_info->info0 & _HTIE_STA_CH_WDTH_))
			priv->site_survey.bss[index].t_stamp[1] &= ~(BIT(1)|BIT(2));
		else {
			if ((ht_info->info0 & _HTIE_2NDCH_OFFSET_BL_) == _HTIE_2NDCH_OFFSET_BL_)
				priv->site_survey.bss[index].t_stamp[1] |= BIT(2);
			else
				priv->site_survey.bss[index].t_stamp[1] &= ~(BIT(2));
		}
	}
	else
		priv->site_survey.bss[index].t_stamp[1] &= ~(BIT(1)|BIT(2));

	// get WPA/WPA2 information
	get_security_info(priv, pfrinfo, index);

#ifdef WDS
	if (priv->pmib->dot11WdsInfo.wdsEnabled && priv->pmib->dot11WdsInfo.wdsNum) {
		// look for ERP rate. if no ERP rate existed, thought it is a legacy AP
		unsigned char supportedRates[32];
		int supplen=0;

		struct stat_info *pstat = get_stainfo(priv, GetAddr2Ptr(pframe));
		if (pstat && (pstat->state & WIFI_WDS)) {
			p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_,
					_SUPPORTEDRATES_IE_, &len,
					pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
			if (p) {
				if (len>8)
					len=8;
				memcpy(&supportedRates[supplen], p+2, len);
				supplen += len;
			}

			p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_,
					_EXT_SUPPORTEDRATES_IE_, &len,
					pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
			if (p) {
				if (len>8)
					len=8;
				memcpy(&supportedRates[supplen], p+2, len);
				supplen += len;
			}

			get_matched_rate(priv, supportedRates, &supplen, 0);
			update_support_rate(pstat, supportedRates, supplen);
			if (supplen == 0)
				pstat->current_tx_rate = 0;
			else {
				if (priv->pmib->dot11StationConfigEntry.autoRate) {
					pstat->current_tx_rate = find_rate(priv, pstat, 1, 0);
					//pstat->upper_tx_rate = 0;	// unused
				}
			}

			// Customer proprietary IE
			if (priv->pmib->miscEntry.private_ie_len) {
				p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_,
					priv->pmib->miscEntry.private_ie[0], &len,
					pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
				if (p) {
					memcpy(pstat->private_ie, p, len + 2);
					pstat->private_ie_len = len + 2;
				}
			}

			// Realtek proprietary IE
			p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_; len = 0;
			for (;;) {
				p = get_ie(p, _RSN_IE_1_, &len,
				pfrinfo->pktlen - (p - pframe));
				if (p != NULL) {
					if (!memcmp(p+2, Realtek_OUI, 3)) {
						if (*(p+2+3) == 2)
							pstat->is_rtl8190_sta = TRUE;
						else
							pstat->is_rtl8190_sta = FALSE;
						break;
					}
				}
				else
					break;
				p = p + len + 2;
			}

#ifdef SEMI_QOS
			if (QOS_ENABLE) {
				p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_;
				for (;;) {
					p = get_ie(p, _RSN_IE_1_, &len,
							pfrinfo->pktlen - (p - pframe));
					if (p != NULL) {
						if (!memcmp(p+2, WMM_PARA_IE, 6)) {
							pstat->QosEnabled = 1;
							break;
						}
					}
					else {
						pstat->QosEnabled = 0;
						break;
					}
					p = p + len + 2;
				}
			}
#endif
			if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
				p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_CAP_, &len,
					pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
				if (p !=  NULL) {
					pstat->ht_cap_len = len;
					memcpy((unsigned char *)&pstat->ht_cap_buf, p+2, len);
					if (cpu_to_le16(pstat->ht_cap_buf.ht_cap_info) & _HTCAP_AMSDU_LEN_8K_) {
						pstat->is_8k_amsdu = 1;
						pstat->amsdu_level = 7935 - sizeof(struct wlan_hdr);
					}
					else {
						pstat->is_8k_amsdu = 0;
						pstat->amsdu_level = 3839 - sizeof(struct wlan_hdr);
					}
				}
				else
					pstat->ht_cap_len = 0;
			}
		}
	}
#endif

#ifdef SEMI_QOS  //  WMM STA
	if (QOS_ENABLE) {  // get WMM IE / WMM Parameter IE
		p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_;
		for (;;) {
			p = get_ie(p, _RSN_IE_1_, &len,
				pfrinfo->pktlen - (p - pframe));
			if (p != NULL) {
				if ((!memcmp(p+2, WMM_IE, 6)) || (!memcmp(p+2, WMM_PARA_IE, 6))) {
					priv->site_survey.bss[index].t_stamp[1] |= BIT(0);  //  set t_stamp[1] bit 0 when AP supports WMM
					break;
				}
			}
			else {
				priv->site_survey.bss[index].t_stamp[1] &= ~(BIT(0));  //  reset t_stamp[1] bit 0 when AP not support WMM
				break;
			}
			p = p + len + 2;
		}
	}
#endif

	return SUCCESS;
}


void assign_tx_rate(struct rtl8190_priv *priv, struct stat_info *pstat, struct rx_frinfo *pfrinfo)
{
	int tx_rate=0;
	UINT8 rate;
	int auto_rate;

#ifdef WDS
	if (pstat->state & WIFI_WDS) {
		auto_rate =	(priv->pmib->dot11WdsInfo.entry[pstat->wds_idx].txRate==0) ? 1: 0;
		tx_rate = priv->pmib->dot11WdsInfo.entry[pstat->wds_idx].txRate;
	}
	else
#endif
	{
		auto_rate = priv->pmib->dot11StationConfigEntry.autoRate;
		tx_rate = priv->pmib->dot11StationConfigEntry.fixedTxRate;
	}

	if (auto_rate) {
		pstat->current_tx_rate = find_rate(priv, pstat, 1, 0);

	}
	else {
		// see if current fixed tx rate of mib is existed in supported rates set
		rate = get_rate_from_bit_value(tx_rate);
		if (match_supp_rate(pstat->bssrateset, pstat->bssratelen, rate))
			tx_rate = (int)rate;
		if (tx_rate == 0) // if not found, use highest supported rate for current tx rate
			tx_rate = find_rate(priv, pstat, 1, 0);
		pstat->current_tx_rate = tx_rate;
	}

	if ((pstat->MIMO_ps & _HT_MIMO_PS_STATIC_) && is_MCS_rate(pstat->current_tx_rate) &&
		((pstat->current_tx_rate & 0x7f) > 7))
		pstat->current_tx_rate = _MCS7_RATE_;	// when HT MIMO Static power save is set and rate > MCS7, fix rate to MCS7

	if (pfrinfo)
		pstat->rssi = pfrinfo->rssi;	// give the initial value to pstat->rssi

	if (priv->pshare->rf_ft_var.rssi_dump && pfrinfo)
		printk("[%d] rssi=%d%% assign rate %s%d\n", pstat->aid, pfrinfo->rssi,
			is_MCS_rate(pstat->current_tx_rate)? "MCS" : "",
			is_MCS_rate(pstat->current_tx_rate)? pstat->current_tx_rate&0x7f : pstat->current_tx_rate/2);
}


// Assign aggregation method automatically.
// We according to the following rule:
// 1. Rtl8190: AMPDU
// 2. Broadcom: AMSDU
// 3. Station who supports only 4K AMSDU receiving: AMPDU
// 4. Others: AMSDU
void assign_aggre_mthod(struct rtl8190_priv *priv, struct stat_info *pstat)
{

	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) && pstat->ht_cap_len) {
		if ((AMPDU_ENABLE == 1) || (AMSDU_ENABLE == 1))		// auto assignment
			pstat->aggre_mthd = AGGRE_MTHD_MPDU;
		else if ((AMPDU_ENABLE >= 2) && (AMSDU_ENABLE == 0))
			pstat->aggre_mthd = AGGRE_MTHD_MPDU;
		else if ((AMPDU_ENABLE == 0) && (AMSDU_ENABLE == 2))
			pstat->aggre_mthd = AGGRE_MTHD_MSDU;
		else
			pstat->aggre_mthd = AGGRE_MTHD_NONE;
	}
	else
		pstat->aggre_mthd = AGGRE_MTHD_NONE;

#ifdef STA_EXT
	if(pstat->sta_in_firmware != 1 && priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _NO_PRIVACY_)
		pstat->aggre_mthd = AGGRE_MTHD_NONE;
#endif

}


#ifndef USE_WEP_DEFAULT_KEY
void set_keymapping_wep(struct rtl8190_priv *priv, struct stat_info *pstat)
{
	struct wifi_mib	*pmib = GET_MIB(priv);

//	if ((GET_ROOT(priv)->pmib->dot11OperationEntry.opmode & WIFI_AP_STATE) &&
	if (!SWCRYPTO && !IEEE8021X_FUN &&
		((pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_) ||
		 (pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_)))
	{
		pstat->dot11KeyMapping.dot11Privacy = pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm;
		pstat->keyid = pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex;
		if (pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_) {
			pstat->dot11KeyMapping.dot11EncryptKey.dot11TTKeyLen = 5;
			memcpy(pstat->dot11KeyMapping.dot11EncryptKey.dot11TTKey.skey,
				   pmib->dot11DefaultKeysTable.keytype[pstat->keyid].skey, 5);
		}
		else {
			pstat->dot11KeyMapping.dot11EncryptKey.dot11TTKeyLen = 13;
			memcpy(pstat->dot11KeyMapping.dot11EncryptKey.dot11TTKey.skey,
				   pmib->dot11DefaultKeysTable.keytype[pstat->keyid].skey, 13);
		}

		DEBUG_INFO("going to set %s unicast key for sta %02X%02X%02X%02X%02X%02X, id=%d\n",
			(pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_)?"WEP40":"WEP104",
			pstat->hwaddr[0], pstat->hwaddr[1], pstat->hwaddr[2],
			pstat->hwaddr[3], pstat->hwaddr[4], pstat->hwaddr[5], pstat->keyid);
		if (!SWCRYPTO) {
			int retVal;
			retVal = CamDeleteOneEntry(priv, pstat->hwaddr, pstat->keyid, 0);
			if (retVal) {
				priv->pshare->CamEntryOccupied--;
				pstat->dot11KeyMapping.keyInCam = FALSE;
			}
			retVal = CamAddOneEntry(priv, pstat->hwaddr, pstat->keyid,
				pstat->dot11KeyMapping.dot11Privacy<<2, 0, pstat->dot11KeyMapping.dot11EncryptKey.dot11TTKey.skey);
			if (retVal) {
				priv->pshare->CamEntryOccupied++;
				pstat->dot11KeyMapping.keyInCam = TRUE;
			}
			else {
				if (pstat->aggre_mthd != AGGRE_MTHD_NONE)
					pstat->aggre_mthd = AGGRE_MTHD_NONE;
			}
		}
	}
}
#endif


/*-----------------------------------------------------------------------------
OnAssocReg:
	--> Reply DeAuth or AssocRsp
Capability Info, Listen Interval, SSID, SupportedRates
------------------------------------------------------------------------------*/
static unsigned int OnAssocReq(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo)
{
	struct wifi_mib		*pmib;
	struct stat_info	*pstat;
	unsigned char		*pframe, *p;
	unsigned char		rsnie_hdr[4]={0x00, 0x50, 0xf2, 0x01};
#ifdef RTL_WPA2
	unsigned char		rsnie_hdr_wpa2[2]={0x01, 0x00};
#endif
	int		len;
	unsigned long		flags;
	DOT11_ASSOCIATION_IND     Association_Ind;
	DOT11_REASSOCIATION_IND   Reassociation_Ind;
	unsigned char		supportRate[32];
	int					supportRateNum;
	unsigned int		status = _STATS_SUCCESSFUL_;
	unsigned short		frame_type, ie_offset=0, val16;

	pmib = GET_MIB(priv);
	pframe = get_pframe(pfrinfo);
	pstat = get_stainfo(priv, GetAddr2Ptr(pframe));

	if (!(OPMODE & WIFI_AP_STATE))
		return FAIL;

#ifdef WDS
	if (pmib->dot11WdsInfo.wdsPure)
		return FAIL;
#endif

	if (pmib->miscEntry.func_off)
		return FAIL;

#ifdef CONFIG_RTK_MESH

// KEY_MAP_KEY_PATCH_0223
	if(	  pmib->dot1180211sInfo.mesh_enable && !(GET_MIB(priv)->dot1180211sInfo.mesh_ap_enable))
// 2008.05.16
//		((pmib->dot11sKeysTable.dot11Privacy && pmib->dot11sKeysTable.keyInCam == FALSE )
//		||( (OPMODE & WIFI_AP_STATE) && !(GET_MIB(priv)->dot1180211sInfo.mesh_ap_enable))))
	{
		return FAIL;
	}
// KEY_MAP_KEY_PATCH_0223
// 2008.05.16

#endif

	frame_type = GetFrameSubType(pframe);
	if (frame_type == WIFI_ASSOCREQ)
		ie_offset = _ASOCREQ_IE_OFFSET_;
	else // WIFI_REASSOCREQ
		ie_offset = _REASOCREQ_IE_OFFSET_;

	if (pstat == (struct stat_info *)NULL)
	{
		status = _RSON_CLS2_;
		goto asoc_class2_error;
	}

	// check if this stat has been successfully authenticated/assocated
	if (!((pstat->state) & WIFI_AUTH_SUCCESS))
	{
		status = _RSON_CLS2_;
		goto asoc_class2_error;
	}

	if (priv->assoc_reject_on)
	{
		status = _STATS_OTHER_;
		goto OnAssocReqFail;
	}

	// now we should check all the fields...

	// checking SSID
	p = get_ie(pframe + WLAN_HDR_A3_LEN + ie_offset, _SSID_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - ie_offset);

	if (p == NULL)
	{
		status = _STATS_FAILURE_;
		goto OnAssocReqFail;
	}

	if (len == 0) // broadcast ssid, however it is not allowed in assocreq
		status = _STATS_FAILURE_;
	else
	{
		// check if ssid match
		if (memcmp((void *)(p+2), SSID, SSID_LEN))
			status = _STATS_FAILURE_;

		if (len != SSID_LEN)
			status = _STATS_FAILURE_;
	}

	// check if the supported is ok
	p = get_ie(pframe + WLAN_HDR_A3_LEN + ie_offset, _SUPPORTEDRATES_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - ie_offset);

	if (p == NULL) {
		DEBUG_WARN("Rx a sta assoc-req which supported rate is empty!\n");
		// use our own rate set as statoin used
		memcpy(supportRate, AP_BSSRATE, AP_BSSRATE_LEN);
		supportRateNum = AP_BSSRATE_LEN;
	}
	else {
		memcpy(supportRate, p+2, len);
		supportRateNum = len;

		p = get_ie(pframe + WLAN_HDR_A3_LEN + ie_offset, _EXT_SUPPORTEDRATES_IE_ , &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - ie_offset);
		if (p !=  NULL) {
			memcpy(supportRate+supportRateNum, p+2, len);
			supportRateNum += len;
		}
	}

#ifdef __DRAYTEK_OS__
	if (status == _STATS_SUCCESSFUL_) {
		status = cb_assoc_request(priv->dev, GetAddr2Ptr(pframe), pframe + WLAN_HDR_A3_LEN + _ASOCREQ_IE_OFFSET_,
				pfrinfo->pktlen-WLAN_HDR_A3_LEN-_ASOCREQ_IE_OFFSET_);
		if (status != _STATS_SUCCESSFUL_) {
			DEBUG_ERR("\rReject association from draytek OS, status=%d!\n", status);
			goto OnAssocReqFail;
		}
	}
#endif

	if (check_basic_rate(priv, supportRate, supportRateNum) == FAIL) {		// check basic rate. jimmylin 2004/12/02
		DEBUG_WARN("Rx a sta assoc-req which basic rates not match! %02X%02X%02X%02X%02X%02X\n",
			pstat->hwaddr[0],pstat->hwaddr[1],pstat->hwaddr[2],pstat->hwaddr[3],pstat->hwaddr[4],pstat->hwaddr[5]);
		if (priv->pmib->dot11OperationEntry.wifi_specific) {
			status = _STATS_RATE_FAIL_;
			goto OnAssocReqFail;
		}
	}

	get_matched_rate(priv, supportRate, &supportRateNum, 0);
	update_support_rate(pstat, supportRate, supportRateNum);

	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) &&
		!isErpSta(pstat) &&
		(priv->pmib->dot11StationConfigEntry.legacySTADeny & WIRELESS_11B)) {
		status = _STATS_RATE_FAIL_;
		goto OnAssocReqFail;
	}

	val16 = cpu_to_le16(*(unsigned short*)((unsigned int)pframe + WLAN_HDR_A3_LEN));
	if (!(val16 & BIT(5))) // NOT use short preamble
		pstat->useShortPreamble = 0;
	else
		pstat->useShortPreamble = 1;

	pstat->state |= WIFI_ASOC_STATE;

#ifdef RTL8190_FASTEXTDEV_FUNCALL
	rtl865x_extDev_addHost(	pstat->hwaddr,
							priv->WLAN_VLAN_ID,
							priv->dev,
							(1 << priv->rtl8650extPortNum),
#ifdef WDS
							priv->rtl8650linkNum[(getWdsIdxByDev(priv, priv->dev) >= 0)?(1+getWdsIdxByDev(priv, priv->dev)):0]
#else
							priv->rtl8650linkNum[0]
#endif
							);
#endif /* RTL8190_FASTEXTDEV_FUNCALL */

	if (status != _STATS_SUCCESSFUL_)
		goto OnAssocReqFail;

	// now the station is qualified to join our BSS...

#ifdef SEMI_QOS
	// check if there is WMM IE
	if (QOS_ENABLE) {
		p = pframe + WLAN_HDR_A3_LEN + ie_offset; len = 0;
		for (;;) {
			p = get_ie(p, _RSN_IE_1_, &len,
				pfrinfo->pktlen - (p - pframe));
			if (p != NULL) {
				if (!memcmp(p+2, WMM_IE, 6)) {
					pstat->QosEnabled = 1;
#ifdef WMM_APSD
					if (APSD_ENABLE)
						pstat->apsd_bitmap = *(p+8) & 0x0f;		// get QSTA APSD bitmap
#endif
					break;
				}
			}
			else {
				pstat->QosEnabled = 0;
#ifdef WMM_APSD
				pstat->apsd_bitmap = 0;
#endif
				break;
			}
			p = p + len + 2;
		}
	}
	else {
		pstat->QosEnabled = 0;
#ifdef WMM_APSD
		pstat->apsd_bitmap = 0;
#endif
	}
#endif

	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
		p = get_ie(pframe + WLAN_HDR_A3_LEN + ie_offset, _HT_CAP_, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - ie_offset);
		if (p !=  NULL) {
			pstat->ht_cap_len = len;
			memcpy((unsigned char *)&pstat->ht_cap_buf, p+2, len);
		}
		else {
			unsigned char old_ht_ie_id[] = {0x00, 0x90, 0x4c};
			p = pframe + WLAN_HDR_A3_LEN + ie_offset; len = 0;
			for (;;)
			{
				p = get_ie(p, _RSN_IE_1_, &len,
					pfrinfo->pktlen - (p - pframe));
				if (p != NULL) {
					if (!memcmp(p+2, old_ht_ie_id, 3) && (*(p+5) == 0x33)) {
						pstat->ht_cap_len = len - 4;
						memcpy((unsigned char *)&pstat->ht_cap_buf, p+6, pstat->ht_cap_len);
						break;
					}
				}
				else
					break;

				p = p + len + 2;
			}
		}

		if (pstat->ht_cap_len) {
			// below is the process to check HT MIMO power save
			unsigned char mimo_ps = ((cpu_to_le16(pstat->ht_cap_buf.ht_cap_info)) >> 2)&0x0003;
			pstat->MIMO_ps = 0;
			if (!mimo_ps)
				pstat->MIMO_ps |= _HT_MIMO_PS_STATIC_;
			else if (mimo_ps == 1)
				pstat->MIMO_ps |= _HT_MIMO_PS_DYNAMIC_;
			if (cpu_to_le16(pstat->ht_cap_buf.ht_cap_info) & _HTCAP_AMSDU_LEN_8K_) {
				pstat->is_8k_amsdu = 1;
				pstat->amsdu_level = 7935 - sizeof(struct wlan_hdr);
			}
			else {
				pstat->is_8k_amsdu = 0;
				pstat->amsdu_level = 3839 - sizeof(struct wlan_hdr);
			}
			if (pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_))
				pstat->tx_bw = HT_CHANNEL_WIDTH_20_40;
		}
		else {
			if (priv->pmib->dot11StationConfigEntry.legacySTADeny & WIRELESS_11G) {
				DEBUG_ERR("Deny legacy STA association!\n");
				status = _STATS_RATE_FAIL_;
				goto OnAssocReqFail;
			}
		}
	}

#ifdef SEMI_QOS
	if (QOS_ENABLE) {
		if ((pstat->QosEnabled == 0) && pstat->ht_cap_len) {
			DEBUG_INFO("STA supports HT but doesn't support WMM, force WMM supported\n");
			pstat->QosEnabled = 1;
		}
	}
#endif

	SAVE_INT_AND_CLI(flags);
	if (!list_empty(&pstat->auth_list))
		list_del_init(&pstat->auth_list);
	if (list_empty(&pstat->asoc_list))
	{
		pstat->expire_to = priv->expire_to;
		list_add_tail(&pstat->asoc_list, &priv->asoc_list);
		cnt_assoc_num(priv, pstat, INCREASE, (char *)__FUNCTION__);
		check_sta_characteristic(priv, pstat, INCREASE);
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)
		  construct_ht_ie(priv, priv->pshare->is_40m_bw, priv->pshare->offset_2nd_chan);
	}
	RESTORE_INT(flags);

//#ifndef RTL8192SE
	// Realtek proprietary IE
	p = pframe + WLAN_HDR_A3_LEN + ie_offset; len = 0;
	for (;;)
	{
		p = get_ie(p, _RSN_IE_1_, &len,
			pfrinfo->pktlen - (p - pframe));
		if (p != NULL) {
			if (!memcmp(p+2, Realtek_OUI, 3)) {
				if (*(p+2+3) == 2) {
					pstat->is_rtl8190_sta = TRUE;
					if (*(p+2+3+2) & RTK_CAP_IE_AP_CLEINT)
						pstat->is_rtl8190_apclient = TRUE;
					else
						pstat->is_rtl8190_apclient = FALSE;
					if(*(p+2+3+2) & RTK_CAP_IE_WLAN_8192SE)
						pstat->is_rtl8192s_sta = TRUE;
					else
						pstat->is_rtl8192s_sta = FALSE;
					if (*(p+2+3+2) & RTK_CAP_IE_USE_AMPDU)
						pstat->is_forced_ampdu = TRUE;
					else
						pstat->is_forced_ampdu = FALSE;
#ifdef RTK_WOW
					if (*(p+2+3+2) & RTK_CAP_IE_USE_WOW)
						pstat->is_rtk_wow_sta = TRUE;
					else
						pstat->is_rtk_wow_sta = FALSE;
#endif
				}
				else
					pstat->is_rtl8190_sta = FALSE;
				break;
			}
		}
		else
			break;

		p = p + len + 2;
	}

	// identify if this is Broadcom sta
	p = pframe + WLAN_HDR_A3_LEN + ie_offset; len = 0;
	pstat->is_broadcom_sta = FALSE;
	for (;;)
	{
		unsigned char Broadcom_OUI1[]={0x00, 0x05, 0xb5};
		unsigned char Broadcom_OUI2[]={0x00, 0x0a, 0xf7};
		unsigned char Broadcom_OUI3[]={0x00, 0x10, 0x18};

		p = get_ie(p, _RSN_IE_1_, &len,
			pfrinfo->pktlen - (p - pframe));
		if (p != NULL) {
			if (!memcmp(p+2, Broadcom_OUI1, 3) ||
				!memcmp(p+2, Broadcom_OUI2, 3) ||
				!memcmp(p+2, Broadcom_OUI3, 3)) {
				pstat->is_broadcom_sta = TRUE;
				break;
			}
		}
		else
			break;

		p = p + len + 2;
	}

	if (!pstat->is_rtl8190_sta && !pstat->is_broadcom_sta) {
		unsigned int z = 0;
		for (z = 0; z < 50; z++) {
			if ((pstat->hwaddr[0] == INTEL_OUI[z][0]) &&
				(pstat->hwaddr[1] == INTEL_OUI[z][1]) &&
				(pstat->hwaddr[2] == INTEL_OUI[z][2])) {
				pstat->is_intel_sta = TRUE;
				break;
			}
		}

		if (z == 50)
			pstat->is_intel_sta = FALSE;
	}

	// Use legacy rate for Wep
	if ((priv->pmib->dot11nConfigEntry.dot11nLgyEncRstrct & BIT(0)) &&
		(!pstat->is_rtl8192s_sta || (priv->pmib->dot11nConfigEntry.dot11nLgyEncRstrct & BIT(2)))) {
		if ((priv->pmib->dot11WdsInfo.wdsPrivacy == _WEP_40_PRIVACY_) ||
			(priv->pmib->dot11WdsInfo.wdsPrivacy == _WEP_104_PRIVACY_)) {
			if (pstat->ht_cap_len) {
				pstat->ht_cap_len = 0;
				pstat->MIMO_ps = 0;
				pstat->is_8k_amsdu = 0;
				pstat->tx_bw = HT_CHANNEL_WIDTH_20;
			}
		}
	}

	assign_tx_rate(priv, pstat, pfrinfo);
	add_update_RATid(priv, pstat);

	// start rssi check
#ifdef STA_EXT
	if (pstat->remapped_aid <= 8)
		set_fw_A2_entry(priv, 0xfd000011 | pstat->remapped_aid<<16, pstat->hwaddr);
#else
	if (pstat->aid <= 8)
		set_fw_A2_entry(priv, 0xfd000011 | pstat->aid<<16, pstat->hwaddr);
#endif

	// Choose aggregation method
	assign_aggre_mthod(priv, pstat);


	// assign aggregation size of this STA
	if ((priv->pmib->dot11nConfigEntry.dot11nAMPDUSendSz == 8) ||
		(priv->pmib->dot11nConfigEntry.dot11nAMPDUSendSz == 16) ||
		(priv->pmib->dot11nConfigEntry.dot11nAMPDUSendSz == 32) ||
		(priv->pmib->dot11nConfigEntry.dot11nAMPDUSendSz == 64)) {
#ifdef STA_EXT
		set_fw_reg(priv, (0xfd0000b4 | pstat->remapped_aid<<8 | priv->pmib->dot11nConfigEntry.dot11nAMPDUSendSz <<16),0,0);
#else
		set_fw_reg(priv, (0xfd0000b4 | pstat->aid<<8 | priv->pmib->dot11nConfigEntry.dot11nAMPDUSendSz <<16),0,0);
#endif
	}
	else {
//	set_fw_reg(priv, (0xfd0000b4 | pstat->aid<<8 | (8<<(pstat->ht_cap_buf.ampdu_para & 0x03))<<16),0,0);
#ifdef STA_EXT
		if (pstat->remapped_aid < FW_NUM_STAT-1) {
			if(pstat->is_rtl8190_sta)
				set_fw_reg(priv, (0xfd0000b4 | pstat->remapped_aid<<8 | (8<<(pstat->ht_cap_buf.ampdu_para & 0x03))<<16),0,0);
			else{
				if ((pstat->ht_cap_buf.ampdu_para & 0x03) > 0)
			 		set_fw_reg(priv, (0xfd0000b4 | pstat->remapped_aid<<8 | 16 <<16),0,0);// default 16K of AMPDU size to other clients support more than 8K
				else
					set_fw_reg(priv, (0xfd0000b4 | pstat->remapped_aid<<8 | 8 <<16),0,0); // default 8K of AMPDU size to other clients support 8K only
			}
		}
		else
			set_fw_reg(priv, (0xfd0000b4 | pstat->remapped_aid<<8 | 8 <<16),0,0);
#else
		if(pstat->is_rtl8190_sta)
			set_fw_reg(priv, (0xfd0000b4 | pstat->aid<<8 | (8<<(pstat->ht_cap_buf.ampdu_para & 0x03))<<16),0,0);
		else{
			if ((pstat->ht_cap_buf.ampdu_para & 0x03) > 0)
		 		set_fw_reg(priv, (0xfd0000b4 | pstat->aid<<8 | 16 <<16),0,0);// default 16K of AMPDU size to other clients support more than 8K
			else
				set_fw_reg(priv, (0xfd0000b4 | pstat->aid<<8 | 8 <<16),0,0); // default 8K of AMPDU size to other clients support 8K only
		}
#endif
	}

 	DEBUG_INFO("assign aggregation size: %x\n", (0xfd0000b4 | pstat->aid<<8 | (8<<(pstat->ht_cap_buf.ampdu_para & 0x03))<<16) );

	// Customer proprietary IE
	if (priv->pmib->miscEntry.private_ie_len) {
		p = get_ie(pframe + WLAN_HDR_A3_LEN + ie_offset, priv->pmib->miscEntry.private_ie[0], &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - ie_offset);
		if (p) {
			memcpy(pstat->private_ie, p, len + 2);
			pstat->private_ie_len = len + 2;
		}
	}

#if defined(CONFIG_RTL_WAPI_SUPPORT)
	if (priv->pmib->wapiInfo.wapiType!=wapiDisable)
	{
		SAVE_INT_AND_CLI(flags);
		wapiAssert(pstat->wapiInfo==NULL);
		if (pstat->wapiInfo==NULL)
		{
			pstat->wapiInfo = (wapiStaInfo*)kmalloc(sizeof(wapiStaInfo), GFP_ATOMIC);
			if (pstat->wapiInfo==NULL)
			{
				pstat->wapiInfo->wapiState = ST_WAPI_AE_IDLE;
				status = _RSON_UNABLE_HANDLE_;
				goto asoc_class2_error;
			}
			pstat->wapiInfo->priv = priv;
			wapiStationInit(pstat);
		}
		
		RESTORE_INT(flags);

		p = get_ie(pframe + WLAN_HDR_A3_LEN + ie_offset, _EID_WAPI_, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - ie_offset);

		if (p==NULL)
		{
			pstat->wapiInfo->wapiState = ST_WAPI_AE_IDLE;
			status = _RSON_IE_NOT_CONSISTENT_;
			goto asoc_class2_error;
		}

		memcpy(pstat->wapiInfo->asueWapiIE, p, len+2);
		pstat->wapiInfo->asueWapiIELength= len+2;

		/*	check for KM	*/
		if ((status=wapiIEInfoInstall(priv, pstat))!=_STATS_SUCCESSFUL_)
		{
			pstat->wapiInfo->wapiState = ST_WAPI_AE_IDLE;
			goto asoc_class2_error;
		}
	}
#endif

	DEBUG_INFO("%s %02X%02X%02X%02X%02X%02X\n",
		(frame_type == WIFI_ASSOCREQ)? "OnAssocReq" : "OnReAssocReq",
		pstat->hwaddr[0],pstat->hwaddr[1],pstat->hwaddr[2],pstat->hwaddr[3],pstat->hwaddr[4],pstat->hwaddr[5]);

	/* 1. If 802.1x enabled, get RSN IE (if exists) and indicate ASSOIC_IND event
	 * 2. Set dot118021xAlgrthm, dot11PrivacyAlgrthm in pstat
	 */
	if (IEEE8021X_FUN || IAPP_ENABLE || priv->pmib->wscEntry.wsc_enable)
	{
		p = pframe + WLAN_HDR_A3_LEN + ie_offset; len = 0;
		for(;;)
		{
#ifdef RTL_WPA2
			char tmpbuf[128];
			int buf_len=0;
			p = get_rsn_ie(priv, p, &len,
				pfrinfo->pktlen - (p - pframe));

			buf_len = sprintf(tmpbuf, "RSNIE len = %d, p = %s", len, (p==NULL? "NULL":"non-NULL"));
			if (p != NULL)
				buf_len += sprintf(tmpbuf+buf_len, ", ID = %02X\n", *(unsigned char *)p);
			else
				buf_len += sprintf(tmpbuf+buf_len, "\n");
			DEBUG_INFO("%s", tmpbuf);
#else
			p = get_ie(p, _RSN_IE_1_, &len,
				pfrinfo->pktlen - (p - pframe));
#endif

			if (p == NULL)
				break;

#ifdef RTL_WPA2
			if ((*(unsigned char *)p == _RSN_IE_1_) && (len >= 4) && (!memcmp((void *)(p + 2), (void *)rsnie_hdr, 4)))
				break;

			if ((*(unsigned char *)p == _RSN_IE_2_) && (len >= 2) && (!memcmp((void *)(p + 2), (void *)rsnie_hdr_wpa2, 2)))
				break;
#else
			if ((len >= 4) && (!memcmp((void *)(p + 2), (void *)rsnie_hdr, 4)))
				break;
#endif

			p = p + len + 2;
		}

#ifdef WIFI_SIMPLE_CONFIG
		if (priv->pmib->wscEntry.wsc_enable & 2) { // work as AP (not registrar)
			unsigned char *ptmp;
			unsigned int lentmp;
			unsigned char passWscIE=0;
			DOT11_WSC_ASSOC_IND wsc_Association_Ind;

			ptmp = pframe + WLAN_HDR_A3_LEN + ie_offset; lentmp = 0;
			for (;;)
			{
				ptmp = get_ie(ptmp, _WPS_IE_, &lentmp,
					pfrinfo->pktlen - (ptmp - pframe));
				if (ptmp != NULL) {
					if (!memcmp(ptmp+2, WSC_IE_OUI, 4)) {
						ptmp = search_wsc_tag(ptmp+2+4, TAG_REQUEST_TYPE, lentmp-4, &lentmp);
						if (ptmp && (*ptmp <= MAX_REQUEST_TYPE_NUM)) {
							DEBUG_INFO("WSC IE TAG_REQUEST_TYPE = %d has been found\n", *ptmp);
							passWscIE = 1;
						}
						break;
					}
				}
				else
					break;

				ptmp = ptmp + lentmp + 2;
			}

			memset(&wsc_Association_Ind, 0, sizeof(DOT11_WSC_ASSOC_IND));
			wsc_Association_Ind.EventId = DOT11_EVENT_WSC_ASSOC_REQ_IE_IND;
			memcpy((void *)wsc_Association_Ind.MACAddr, (void *)GetAddr2Ptr(pframe), MACADDRLEN);
			if (passWscIE) {
				wsc_Association_Ind.wscIE_included = 1;
				wsc_Association_Ind.AssocIELen = lentmp + 2;
				memcpy((void *)wsc_Association_Ind.AssocIE, (void *)(ptmp), wsc_Association_Ind.AssocIELen);
			}
			else {
				if (IEEE8021X_FUN &&
					(pstat->AuthAlgrthm == _NO_PRIVACY_) && // authentication is open
					(p == NULL)) { // No SSN or RSN IE
					wsc_Association_Ind.wscIE_included = 1; //treat this case as WSC IE included
					DEBUG_INFO("Association : auth open; no SSN or RSN IE\n");
				}
			}

			if ((wsc_Association_Ind.wscIE_included == 1) || !IEEE8021X_FUN)
				DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&wsc_Association_Ind,
						sizeof(DOT11_WSC_ASSOC_IND));

			if (wsc_Association_Ind.wscIE_included == 1) {
				pstat->state |= WIFI_WPS_JOIN;
				goto OnAssocReqSuccess;
			}
// Brad add for DWA-652 WPS interoperability 2008/03/13--------
			if ((pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_ ||
     				pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_) &&
     				!IEEE8021X_FUN)
				pstat->state |= WIFI_WPS_JOIN;
//------------------------- end

		}
#endif


#ifndef WITHOUT_ENQUEUE
		if (frame_type == WIFI_ASSOCREQ)
		{
			memcpy((void *)Association_Ind.MACAddr, (void *)GetAddr2Ptr(pframe), MACADDRLEN);
			Association_Ind.EventId = DOT11_EVENT_ASSOCIATION_IND;
			Association_Ind.IsMoreEvent = 0;
			if (p == NULL)
				Association_Ind.RSNIELen = 0;
			else
			{
				DEBUG_INFO("assoc indication rsnie len=%d\n", len);
#ifdef RTL_WPA2
				// inlcude ID and Length
				Association_Ind.RSNIELen = len + 2;
				memcpy((void *)Association_Ind.RSNIE, (void *)(p), Association_Ind.RSNIELen);
#else
				Association_Ind.RSNIELen = len;
				memcpy((void *)Association_Ind.RSNIE, (void *)(p + 2), len);
#endif
			}
			// indicate if 11n sta associated
			Association_Ind.RSNIE[MAXRSNIELEN-1] = ((pstat->ht_cap_len==0) ? 0 : 1);

			DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&Association_Ind,
						sizeof(DOT11_ASSOCIATION_IND));
		}
		else
		{
			memcpy((void *)Reassociation_Ind.MACAddr, (void *)GetAddr2Ptr(pframe), MACADDRLEN);
			Reassociation_Ind.EventId = DOT11_EVENT_REASSOCIATION_IND;
			Reassociation_Ind.IsMoreEvent = 0;
			if (p == NULL)
				Reassociation_Ind.RSNIELen = 0;
			else
			{
				DEBUG_INFO("assoc indication rsnie len=%d\n", len);
#ifdef RTL_WPA2
				// inlcude ID and Length
				Reassociation_Ind.RSNIELen = len + 2;
				memcpy((void *)Reassociation_Ind.RSNIE, (void *)(p), Reassociation_Ind.RSNIELen);
#else
				Reassociation_Ind.RSNIELen = len;
				memcpy((void *)Reassociation_Ind.RSNIE, (void *)(p + 2), len);
#endif
			}
			memcpy((void *)Reassociation_Ind.OldAPaddr,
				(void *)(pframe + WLAN_HDR_A3_LEN + _CAPABILITY_ + _LISTEN_INTERVAL_), MACADDRLEN);

			// indicate if 11n sta associated
			Reassociation_Ind.RSNIE[MAXRSNIELEN-1] = ((pstat->ht_cap_len==0) ? 0 : 1);

			DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&Reassociation_Ind,
						sizeof(DOT11_REASSOCIATION_IND));
		}
#endif // WITHOUT_ENQUEUE

#ifdef INCLUDE_WPA_PSK
		{
			int id;
			unsigned char *pIE;
			int ie_len;

			LOG_MSG("A wireless client is associated - %02X:%02X:%02X:%02X:%02X:%02X\n",
				*GetAddr2Ptr(pframe), *(GetAddr2Ptr(pframe)+1), *(GetAddr2Ptr(pframe)+2),
				*(GetAddr2Ptr(pframe+3)), *(GetAddr2Ptr(pframe)+4), *(GetAddr2Ptr(pframe)+5));

			if (frame_type == WIFI_ASSOCREQ)
				id = DOT11_EVENT_ASSOCIATION_IND;
			else
				id = DOT11_EVENT_REASSOCIATION_IND;

#ifdef RTL_WPA2
			ie_len = len + 2;
			pIE = p;
#else
			ie_len = len;
			pIE = p + 2;
#endif
			psk_indicate_evt(priv, id, GetAddr2Ptr(pframe), pIE, ie_len);
		}
#endif // INCLUDE_WPA_PSK

		event_indicate(priv, GetAddr2Ptr(pframe), 1);
	}

//#ifndef INCLUDE_WPA_PSK
#if defined(CONFIG_RTL8196B_TR)
	if (!IEEE8021X_FUN &&
			!(priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _TKIP_PRIVACY_ ||
			 priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _CCMP_PRIVACY_))
			LOG_MSG_NOTICE("Wireless PC connected;note:%02x-%02x-%02x-%02x-%02x-%02x;\n",
				*GetAddr2Ptr(pframe), *(GetAddr2Ptr(pframe)+1), *(GetAddr2Ptr(pframe)+2),
				*(GetAddr2Ptr(pframe+3)), *(GetAddr2Ptr(pframe)+4), *(GetAddr2Ptr(pframe)+5));
#elif defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
	if (!IEEE8021X_FUN &&
			!(priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _TKIP_PRIVACY_ ||
			 priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _CCMP_PRIVACY_))
			LOG_MSG_NOTICE("Wireless PC connected;note:%02x-%02x-%02x-%02x-%02x-%02x;\n",
				*GetAddr2Ptr(pframe), *(GetAddr2Ptr(pframe)+1), *(GetAddr2Ptr(pframe)+2),
				*(GetAddr2Ptr(pframe+3)), *(GetAddr2Ptr(pframe)+4), *(GetAddr2Ptr(pframe)+5));
#elif defined(CONFIG_RTL8196B_TLD)
	if (!IEEE8021X_FUN &&
			!(priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _TKIP_PRIVACY_ ||
			 priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _CCMP_PRIVACY_)) {
			if (!list_empty(&priv->wlan_acl_list)) {
				LOG_MSG_DEL("[WLAN access allowed] from MAC: %02x:%02x:%02x:%02x:%02x:%02x,\n",
				*GetAddr2Ptr(pframe), *(GetAddr2Ptr(pframe)+1), *(GetAddr2Ptr(pframe)+2),
				*(GetAddr2Ptr(pframe+3)), *(GetAddr2Ptr(pframe)+4), *(GetAddr2Ptr(pframe)+5));
			}
	}
#else
	LOG_MSG("A wireless client is associated - %02X:%02X:%02X:%02X:%02X:%02X\n",
			*GetAddr2Ptr(pframe), *(GetAddr2Ptr(pframe)+1), *(GetAddr2Ptr(pframe)+2),
			*(GetAddr2Ptr(pframe+3)), *(GetAddr2Ptr(pframe)+4), *(GetAddr2Ptr(pframe)+5));
#endif
//#endif

	if (IEEE8021X_FUN || IAPP_ENABLE || priv->pmib->wscEntry.wsc_enable) {
#ifndef __DRAYTEK_OS__
		if (IEEE8021X_FUN &&	// in WPA, let user daemon check RSNIE and decide to accept or not
			(priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _TKIP_PRIVACY_ ||
			 priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _CCMP_PRIVACY_))
			return SUCCESS;
#endif
	}

#ifdef WIFI_SIMPLE_CONFIG
OnAssocReqSuccess:
#endif

	if (frame_type == WIFI_ASSOCREQ)
		issue_asocrsp(priv, status, pstat, WIFI_ASSOCRSP);
	else
		issue_asocrsp(priv, status, pstat, WIFI_REASSOCRSP);

#if defined(CONFIG_RTK_MESH) && defined(PU_STANDARD_SME)
//by pepsi from Draft 1.08
	if ((1 == GET_MIB(priv)->dot1180211sInfo.mesh_enable)	// fix: 0000107 2008/10/13
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID) // Spare for Mesh work with Multiple AP (Please see Mantis 0000107 for detail)
			&& IS_ROOT_INTERFACE(priv)
#endif
	) {
		struct proxyupdate_table_entry puEntry;
		struct proxy_table_entry *pEntry=NULL;

		puEntry.retry = 1U;
		puEntry.STAcount = 0x0001;
		memcpy((void *)puEntry.proxymac ,GET_MY_HWADDR ,MACADDRLEN);
		memcpy((void *)puEntry.proxiedmac ,(void *)GetAddr2Ptr(pframe) ,MACADDRLEN);

		// PROXY DELETE
		if( (pEntry = (struct proxy_table_entry *)HASH_SEARCH(priv->proxy_table,(void *)GetAddr2Ptr(pframe)))
			&& memcmp(GET_MY_HWADDR, pEntry->owner ,MACADDRLEN))	//  2008.05.16
		{
			struct path_sel_entry *pdstEntry=NULL;
			pdstEntry = pathsel_query_table( priv, pEntry->owner);
			if(pdstEntry != (struct path_sel_entry *)-1)
			{
				puEntry.isMultihop = pdstEntry->hopcount;
				memcpy(puEntry.nexthopmac, pdstEntry->nexthopMAC, MACADDRLEN);
			}
			else
			{
				memset((void *)puEntry.nexthopmac, 0xff, MACADDRLEN );
				puEntry.isMultihop = 0;
			}
			memcpy((void *)puEntry.destproxymac ,pEntry->owner ,MACADDRLEN);
//			printk("Old AP:");
//			printMac(pEntry->owner);
//			printk(" is informed to delete.\n");
			puEntry.isMultihop = PU_delete;
			//01 for delete ; 00 for add, follow Draft 1.08
			puEntry.PUflag = 0x01;
			puEntry.PUSN = getPUSeq(priv);
			puEntry.update_time = xtime;
			HASH_INSERT(priv->proxyupdate_table,&puEntry.PUSN,&puEntry);
			issue_proxyupdate_MP(priv, &puEntry);
		}

		// PROXY ADD (broadcast temporary)
		memset((void *)puEntry.destproxymac ,0xff ,MACADDRLEN);
		puEntry.isMultihop = 0x00;
		puEntry.PUflag = PU_add;
		puEntry.PUSN = getPUSeq(priv);
		puEntry.update_time = xtime;
		HASH_INSERT(priv->proxyupdate_table, &puEntry.PUSN, &puEntry);
		issue_proxyupdate_MP(priv, &puEntry);
#ifdef BR_SHORTCUT
		{
			extern unsigned char cached_mesh_mac[6];
			extern struct net_device *cached_mesh_dev;
			if(memcmp(cached_mesh_mac, GetAddr2Ptr(pframe), MACADDRLEN) == 0)
				cached_mesh_dev = NULL;
#ifdef WDS
			extern unsigned char cached_wds_mac[6];
			extern struct net_device *cached_wds_dev;
			if(memcmp(cached_wds_mac, GetAddr2Ptr(pframe), MACADDRLEN) == 0)
				cached_wds_dev = NULL;
#endif
#ifdef CLIENT_MODE
			extern unsigned char cached_sta_mac[6];
			extern struct net_device *cached_sta_dev;
			if(memcmp(cached_sta_mac, GetAddr2Ptr(pframe), MACADDRLEN) == 0)
				cached_sta_dev = NULL;
#endif
			extern unsigned char cached_eth_addr[6];
			extern struct net_device *cached_dev;
			if(memcmp(cached_eth_addr, GetAddr2Ptr(pframe), MACADDRLEN) == 0)
				cached_dev = NULL;
		}
#endif
	}
#endif // CONFIG_RTK_MESH && PU_STANDARD_SME

	update_fwtbl_asoclst(priv, pstat);
	event_indicate(priv, GetAddr2Ptr(pframe), 1);

#ifndef USE_WEP_DEFAULT_KEY
	set_keymapping_wep(priv, pstat);
#endif

#if defined(CONFIG_RTL_WAPI_SUPPORT)
	if (priv->pmib->wapiInfo.wapiType==wapiTypeCert)
	{
		wapiAssert(pstat->wapiInfo->wapiState==ST_WAPI_AE_IDLE);
		wapiReqActiveCA(pstat);
	return SUCCESS;
	}
	else if (priv->pmib->wapiInfo.wapiType==wapiTypePSK)
	{
		wapiAssert(pstat->wapiInfo->wapiState==ST_WAPI_AE_IDLE);
		wapiSetBK(pstat);
		if (wapiSendUnicastKeyAgrementRequeset(priv, pstat)==WAPI_RETURN_SUCCESS)
			return SUCCESS;
		else
			return FAIL;
	}
#endif

	return SUCCESS;

asoc_class2_error:

	issue_deauth(priv,	(void *)GetAddr2Ptr(pframe), status);
	return FAIL;

OnAssocReqFail:

	if (frame_type == WIFI_ASSOCREQ)
		issue_asocrsp(priv, status, pstat, WIFI_ASSOCRSP);
	else
		issue_asocrsp(priv, status, pstat, WIFI_REASSOCRSP);
	return FAIL;
}


static unsigned int OnProbeReq(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo)
{
	struct wifi_mib	*pmib;
	unsigned char	*pframe, *p;
	unsigned int	len;
	unsigned char	*bssid;
#ifdef WDS
	unsigned int i;
#endif

	bssid  = BSSID;
	pmib   = GET_MIB(priv);
	pframe = get_pframe(pfrinfo);

	if (!IS_DRV_OPEN(priv))
		return FAIL;

	if (!((OPMODE & WIFI_AP_STATE) || (OPMODE & WIFI_ADHOC_STATE)))
		return FAIL;

#ifdef WDS
	if (pmib->dot11WdsInfo.wdsEnabled && pmib->dot11WdsInfo.wdsPure) {
		if (pmib->dot11WdsInfo.wdsNum) {
			for (i = 0; i < pmib->dot11WdsInfo.wdsNum; i++) {
				if (!memcmp(pmib->dot11WdsInfo.entry[i].macAddr, (char *)GetAddr2Ptr(pframe), MACADDRLEN)) {
					break;
				}
			}
			if (i == pmib->dot11WdsInfo.wdsNum) {
				return FAIL;
			}
		}
		else
			return FAIL;
	}
#endif

	if (pmib->miscEntry.func_off)
		return FAIL;
#ifdef CLIENT_MODE
	if ((OPMODE & WIFI_ADHOC_STATE) &&
			(!priv->ibss_tx_beacon || (OPMODE & WIFI_SITE_MONITOR)))
		return FAIL;
#endif

#ifdef CONFIG_RTK_MESH
	if(pfrinfo->is_11s)
		return OnProbeReq_MP(priv, pfrinfo);
#endif

	p = get_ie(pframe + WLAN_HDR_A3_LEN + _PROBEREQ_IE_OFFSET_, _SSID_IE_, (int *)&len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _PROBEREQ_IE_OFFSET_);

	if (p == NULL)
		goto OnProbeReqFail;

#ifdef WIFI_SIMPLE_CONFIG
	if (priv->pmib->wscEntry.wsc_enable & 2) { // work as AP (not registrar)
		unsigned char *ptmp;
		unsigned int lentmp;
		ptmp = pframe + WLAN_HDR_A3_LEN + _PROBEREQ_IE_OFFSET_; lentmp = 0;
		for (;;)
		{
			ptmp = get_ie(ptmp, _WPS_IE_, &lentmp,
				pfrinfo->pktlen - (ptmp - pframe));
			if (ptmp != NULL) {
				if (!memcmp(ptmp+2, WSC_IE_OUI, 4)) {
					wsc_forward_probe_request(priv, pframe, ptmp, lentmp+2);
					break;
				}
			}
			else
				break;

			ptmp = ptmp + lentmp + 2;
		}
	}
#endif


	if (len == 0) {
		if (HIDDEN_AP)
			goto OnProbeReqFail;
		else
			goto send_rsp;
	}

	if ((len != SSID_LEN) ||
			memcmp((void *)(p+2), (void *)SSID, SSID_LEN)) {
		if ((len == 3) &&
				((*(p+2) == 'A') || (*(p+2) == 'a')) &&
				((*(p+3) == 'N') || (*(p+3) == 'n')) &&
				((*(p+4) == 'Y') || (*(p+4) == 'y'))) {
			if (pmib->dot11OperationEntry.deny_any)
				goto OnProbeReqFail;
			else
				if (HIDDEN_AP)
					goto OnProbeReqFail;
				else
					goto send_rsp;
		}
		else
			goto OnProbeReqFail;
	}

send_rsp:


	issue_probersp(priv, GetAddr2Ptr(pframe), SSID, SSID_LEN, 1);

	return SUCCESS;

OnProbeReqFail:

	return FAIL;
}


static unsigned int OnProbeRsp(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo)
{
#ifdef WDS
	struct stat_info *pstat = NULL;
	unsigned char *pframe = get_pframe(pfrinfo);
#endif
// ==== modified by GANTOE for site survey 2008/12/25 ====
	if (OPMODE & WIFI_SITE_MONITOR)
		collect_bss_info(priv, pfrinfo);
#ifdef WDS
	else if ((OPMODE & WIFI_AP_STATE) && priv->pmib->dot11WdsInfo.wdsEnabled &&
		priv->pmib->dot11WdsInfo.wdsNum) {
		pstat = get_stainfo(priv, (char *)GetAddr2Ptr(pframe));
		if (pstat && (pstat->state & WIFI_WDS)) {
			collect_bss_info(priv, pfrinfo);
			if (pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_))
				pstat->tx_bw = HT_CHANNEL_WIDTH_20_40;
			else
				pstat->tx_bw = HT_CHANNEL_WIDTH_20;
			assign_aggre_mthod(priv, pstat);
			assign_tx_rate(priv, pstat, pfrinfo);

			if (!pstat->wds_probe_done)
				pstat->wds_probe_done = 1;
		}
	}
#endif
#ifdef CONFIG_RTK_MESH	// ==== GANTOE ====
	if(pfrinfo->is_11s)
		return OnProbeRsp_MP(priv, pfrinfo);
#endif

	return SUCCESS;
}


static unsigned int OnBeacon(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo)
{
	int i, len;
	unsigned char *p, *pframe, channel;

	if (OPMODE & WIFI_SITE_MONITOR) {
		collect_bss_info(priv, pfrinfo);
		return SUCCESS;
	}

	pframe = get_pframe(pfrinfo);

	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _DSSET_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	if (p != NULL)
		channel = *(p+2);
	else
		channel = priv->pmib->dot11RFEntry.dot11channel;

	// If used as AP in G mode, need monitor other 11B AP beacon to enable
	// protection mechanism
#ifdef WDS
	// if WDS is used, need monitor other WDS AP beacon to decide tx rate
	if (priv->pmib->dot11WdsInfo.wdsEnabled ||
		((OPMODE & WIFI_AP_STATE) && (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) &&
		 (channel == priv->pmib->dot11RFEntry.dot11channel)))
#else
	if ((OPMODE & WIFI_AP_STATE) &&
		(priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) &&
		(channel == priv->pmib->dot11RFEntry.dot11channel))
#endif
	{
		// look for ERP rate. if no ERP rate existed, thought it is a legacy AP
		unsigned char supportedRates[32];
		int supplen=0, legacy=1;

		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_,
				_SUPPORTEDRATES_IE_, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p) {
			if (len>8)
				len=8;
			memcpy(&supportedRates[supplen], p+2, len);
			supplen += len;
		}

		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_,
				_EXT_SUPPORTEDRATES_IE_, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p) {
			if (len>8)
				len=8;
			memcpy(&supportedRates[supplen], p+2, len);
			supplen += len;
		}

#ifdef WDS
		if (priv->pmib->dot11WdsInfo.wdsEnabled) {
			struct stat_info *pstat = get_stainfo(priv, GetAddr2Ptr(pframe));
			if (pstat && !(pstat->state & WIFI_WDS_RX_BEACON)) {
				get_matched_rate(priv, supportedRates, &supplen, 0);
				update_support_rate(pstat, supportedRates, supplen);
				if (supplen == 0)
					pstat->current_tx_rate = 0;
				else {
					if (priv->pmib->dot11StationConfigEntry.autoRate) {
						pstat->current_tx_rate = find_rate(priv, pstat, 1, 0);
						//pstat->upper_tx_rate = 0;	// unused
					}
				}

				// Customer proprietary IE
				if (priv->pmib->miscEntry.private_ie_len) {
					p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_,
						priv->pmib->miscEntry.private_ie[0], &len,
						pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
					if (p) {
						memcpy(pstat->private_ie, p, len + 2);
						pstat->private_ie_len = len + 2;
					}
				}

				// Realtek proprietary IE
				p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_; len = 0;
				for (;;)
				{
					p = get_ie(p, _RSN_IE_1_, &len,
					pfrinfo->pktlen - (p - pframe));
					if (p != NULL) {
						if (!memcmp(p+2, Realtek_OUI, 3)) {
							if (*(p+2+3) == 2)
								pstat->is_rtl8190_sta = TRUE;
							else
								pstat->is_rtl8190_sta = FALSE;
							break;
						}
					}
					else
						break;
					p = p + len + 2;
				}

#ifdef SEMI_QOS
				if (QOS_ENABLE) {
					p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_;
					for (;;) {
						p = get_ie(p, _RSN_IE_1_, &len,
								pfrinfo->pktlen - (p - pframe));
						if (p != NULL) {
							if (!memcmp(p+2, WMM_PARA_IE, 6)) {
								pstat->QosEnabled = 1;
								break;
							}
						}
						else {
							pstat->QosEnabled = 0;
							break;
						}
						p = p + len + 2;
					}
				}
#endif
				if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
					p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_CAP_, &len,
						pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
					if (p !=  NULL) {
						pstat->ht_cap_len = len;
						memcpy((unsigned char *)&pstat->ht_cap_buf, p+2, len);
						if (cpu_to_le16(pstat->ht_cap_buf.ht_cap_info) & _HTCAP_AMSDU_LEN_8K_) {
							pstat->is_8k_amsdu = 1;
							pstat->amsdu_level = 7935 - sizeof(struct wlan_hdr);
						}
						else {
							pstat->is_8k_amsdu = 0;
							pstat->amsdu_level = 3839 - sizeof(struct wlan_hdr);
						}
					}
					else
						pstat->ht_cap_len = 0;
				}

				if (pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_))
					pstat->tx_bw = HT_CHANNEL_WIDTH_20_40;
				else
					pstat->tx_bw = HT_CHANNEL_WIDTH_20;
				assign_tx_rate(priv, pstat, pfrinfo);
				assign_aggre_mthod(priv, pstat);
				pstat->state |= WIFI_WDS_RX_BEACON;
			}

			if (pstat && pstat->state & WIFI_WDS) {
				pstat->beacon_num++;
				if (!pstat->wds_probe_done)
					pstat->wds_probe_done = 1;
			}
		}
#endif

		for (i=0; i<supplen; i++) {
			if (!is_CCK_rate(supportedRates[i]&0x7f)) {
				legacy = 0;
				break;
			}
		}

		// look for ERP IE and check non ERP present
		if (legacy == 0) {
			p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _ERPINFO_IE_,
					&len, pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
			if (p && (*(p+2) & BIT(0)))
				legacy = 1;
		}

		if (legacy) {
			if (!priv->pmib->dot11StationConfigEntry.olbcDetectDisabled &&
							priv->pmib->dot11ErpInfo.olbcDetected==0) {
				priv->pmib->dot11ErpInfo.olbcDetected = 1;
				check_protection_shortslot(priv);
				DEBUG_INFO("OLBC detected\n");
			}
			if (priv->pmib->dot11ErpInfo.olbcDetected)
				priv->pmib->dot11ErpInfo.olbcExpired = DEFAULT_OLBC_EXPIRE;
		}
	}

	if ((OPMODE & WIFI_AP_STATE) &&
		(priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
		(channel == priv->pmib->dot11RFEntry.dot11channel)) {
		if (!priv->pmib->dot11StationConfigEntry.protectionDisabled &&
				!priv->pmib->dot11StationConfigEntry.olbcDetectDisabled) {
			p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_CAP_,
				&len, pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
			if (p == NULL)
				priv->ht_legacy_obss_to = 60;
		}
	}

#ifdef WIFI_11N_2040_COEXIST
	if (priv->pshare->rf_ft_var.coexist_enable && (OPMODE & WIFI_AP_STATE) &&
		(priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) && priv->pshare->is_40m_bw) {
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_CAP_,
			&len, pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p == NULL) {
			priv->bg_ap_timeout = 60;
		}
	}
#endif

	return SUCCESS;
}


static unsigned int OnDisassoc(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned char *pframe;
	struct  stat_info   *pstat;
	unsigned char *sa;
	unsigned short reason;
	DOT11_DISASSOCIATION_IND Disassociation_Ind;
	unsigned long flags;

	pframe = get_pframe(pfrinfo);
	sa = GetAddr2Ptr(pframe);
	pstat = get_stainfo(priv, sa);

	if (pstat == NULL)
		return 0;

#ifdef RTK_WOW
	if (pstat->is_rtk_wow_sta)
		return 0;
#endif

	reason = cpu_to_le16(*(unsigned short *)((unsigned int)pframe + WLAN_HDR_A3_LEN ));
	DEBUG_INFO("receiving disassoc from station %02X%02X%02X%02X%02X%02X reason %d\n",
		pstat->hwaddr[0], pstat->hwaddr[1], pstat->hwaddr[2],
		pstat->hwaddr[3], pstat->hwaddr[4], pstat->hwaddr[5], reason);

	SAVE_INT_AND_CLI(flags);

	if (!list_empty(&pstat->asoc_list))
	{
		list_del_init(&pstat->asoc_list);
		if (pstat->expire_to > 0)
		{
			cnt_assoc_num(priv, pstat, DECREASE, (char *)__FUNCTION__);
			check_sta_characteristic(priv, pstat, DECREASE);
		}
	}

#ifdef RTL8190_FASTEXTDEV_FUNCALL
	rtl865x_extDev_removeHost(pstat->hwaddr, priv->WLAN_VLAN_ID);
#endif

#ifdef CONFIG_RTL8186_KB
	if (priv->pmib->dot11OperationEntry.guest_access || (pstat && pstat->ieee8021x_ctrlport == DOT11_PortStatus_Guest))
	{
		if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == 0)
		{
			/* hotel style guest access */
			set_guestmacinvalid(priv, sa);
		}
	}
#endif

	// stop rssi check
#ifdef STA_EXT
	if ((OPMODE & WIFI_AP_STATE) && (pstat->remapped_aid <= 8))
		set_fw_reg(priv, 0xfd000013 | pstat->remapped_aid<<16, 0, 0);
#else
	if ((OPMODE & WIFI_AP_STATE) && (pstat->aid <= 8))
		set_fw_reg(priv, 0xfd000013 | pstat->aid<<16, 0, 0);
#endif

	// Need change state back to autehnticated
	release_stainfo(priv, pstat);
	init_stainfo(priv, pstat);
	pstat->state |= WIFI_AUTH_SUCCESS;
	pstat->expire_to = priv->assoc_to;
	list_add_tail(&(pstat->auth_list), &(priv->auth_list));

	RESTORE_INT(flags);

	LOG_MSG("A wireless client is disassociated - %02X:%02X:%02X:%02X:%02X:%02X\n",
		*sa, *(sa+1), *(sa+2), *(sa+3), *(sa+4), *(sa+5));

	if (IEEE8021X_FUN)
	{
#ifndef WITHOUT_ENQUEUE
		memcpy((void *)Disassociation_Ind.MACAddr, (void *)sa, MACADDRLEN);
		Disassociation_Ind.EventId = DOT11_EVENT_DISASSOCIATION_IND;
		Disassociation_Ind.IsMoreEvent = 0;
		Disassociation_Ind.Reason = reason;
		Disassociation_Ind.tx_packets = pstat->tx_pkts;
		Disassociation_Ind.rx_packets = pstat->rx_pkts;
		Disassociation_Ind.tx_bytes   = pstat->tx_bytes;
		Disassociation_Ind.rx_bytes   = pstat->rx_bytes;
		DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&Disassociation_Ind,
					sizeof(DOT11_DISASSOCIATION_IND));
#endif
#ifdef INCLUDE_WPA_PSK
		psk_indicate_evt(priv, DOT11_EVENT_DISASSOCIATION_IND, sa, NULL, 0);
#endif
	}

	event_indicate(priv, sa, 2);

	return SUCCESS;
}


static unsigned int OnAuth(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned int		privacy,seq, len;
	unsigned long		flags=0;
	struct list_head	*phead, *plist;
	struct wlan_acl_node *paclnode;
	unsigned int		acl_mode;

	struct wifi_mib		*pmib;
	struct stat_info	*pstat=NULL;
	unsigned char		*pframe, *sa, *p;
	unsigned int		res=FAIL;
	UINT16				algorithm;
	int					status, alloc_pstat=0;
	struct ac_log_info	*log_info;

	pmib = GET_MIB(priv);

#if defined(CONFIG_RTK_MESH) && defined(MESH_BOOTSEQ_AUTH)
	UINT8	isMeshMP = FALSE, prevState = MP_UNUSED;
#endif
	acl_mode = priv->pmib->dot11StationConfigEntry.dot11AclMode;
	pframe = get_pframe(pfrinfo);
	sa = GetAddr2Ptr(pframe);
	pstat = get_stainfo(priv, sa);

	if (!IS_DRV_OPEN(priv))
		return FAIL;
	if (!(OPMODE & WIFI_AP_STATE))
		return FAIL;

#if defined(CONFIG_RTK_MESH) && defined(MESH_BOOTSEQ_AUTH) 	// Is Mesh MP?
	if ((1 == GET_MIB(priv)->dot1180211sInfo.mesh_enable) && (NULL != pstat) && isPossibleNeighbor(pstat)) {
		prevState = pstat->mesh_neighbor_TBL.State;
		isMeshMP = TRUE;
	}
#endif
#ifdef WDS
#if defined(CONFIG_RTK_MESH) && defined(MESH_BOOTSEQ_AUTH)
	if ((pmib->dot11WdsInfo.wdsPure) && (isMeshMP==FALSE))
#else
	if (pmib->dot11WdsInfo.wdsPure)
#endif
		return FAIL;
#endif

	if (pmib->miscEntry.func_off)
		return FAIL;

	privacy = priv->pmib->dot1180211AuthEntry.dot11AuthAlgrthm;

	seq = cpu_to_le16(*(unsigned short *)((unsigned int)pframe + WLAN_HDR_A3_LEN + 2));

	algorithm = cpu_to_le16(*(unsigned short *)((unsigned int)pframe + WLAN_HDR_A3_LEN));

#if defined(CONFIG_RTK_MESH) && defined(MESH_BOOTSEQ_AUTH)
	MESH_DEBUG_MSG("\nMesh: Auth START !! seq=%d\n", seq);

	if (FALSE == isMeshMP)
#endif
	{
	if (GetPrivacy(pframe))
	{
		int use_keymapping=0;
#ifdef __ZINWELL__
		pstat = get_stainfo(priv, sa);
		if (pstat && priv->keep_encrypt_key && GET_UNICAST_ENCRYP_KEYLEN) {
			DEBUG_INFO("using key-mapping key!\n");
			use_keymapping = 1;
		}
#endif
		status = wep_decrypt(priv, pfrinfo, pfrinfo->pktlen,
			priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm, use_keymapping);

		if (status == FALSE)
		{
			SAVE_INT_AND_CLI(flags);
#if defined(CONFIG_RTL8196B_TLD)
			LOG_MSG_DEL("[WLAN access rejected: incorrect security] from MAC address: %02x:%02x:%02x:%02x:%02x:%02x,\n",
				sa[0], sa[1], sa[2], sa[3], sa[4], sa[5]);
#endif

			DEBUG_ERR("wep-decrypt a Auth frame error!\n");
			status = _STATS_CHALLENGE_FAIL_;
			goto auth_fail;
		}

		seq = cpu_to_le16(*(unsigned short *)((unsigned int)pframe + WLAN_HDR_A3_LEN + 4 + 2));
		algorithm = cpu_to_le16(*(unsigned short *)((unsigned int)pframe + WLAN_HDR_A3_LEN + 4));
	}
#ifdef WIFI_SIMPLE_CONFIG
#ifndef CONFIG_RTL8196B_TLD
	else {
		if (pmib->wscEntry.wsc_enable && (seq == 1) && (algorithm == 0))
			privacy = 0;
	}
#endif
#endif

	DEBUG_INFO("auth alg=%x, seq=%X\n", algorithm, seq);

	if (privacy == 2 &&
			priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _WEP_40_PRIVACY_ &&
			priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _WEP_104_PRIVACY_)
		privacy = 0;

	if ((algorithm > 0 && privacy == 0) ||	// rx a shared-key auth but shared not enabled
		(algorithm == 0 && privacy == 1) )	// rx a open-system auth but shared-key is enabled
	{
		SAVE_INT_AND_CLI(flags);
		DEBUG_ERR("auth rejected due to bad alg [alg=%d, auth_mib=%d] %02X%02X%02X%02X%02X%02X\n",
			algorithm, privacy, sa[0], sa[1], sa[2], sa[3], sa[4], sa[5]);
#if defined(CONFIG_RTL8196B_TLD)
		LOG_MSG_DEL("[WLAN access rejected: incorrect security] from MAC address: %02x:%02x:%02x:%02x:%02x:%02x,\n",
			sa[0], sa[1], sa[2], sa[3], sa[4], sa[5]);
#endif
		status = _STATS_NO_SUPP_ALG_;
		goto auth_fail;
	}

	// STA ACL check;nctu note
	SAVE_INT_AND_CLI(flags);
	phead = &priv->wlan_acl_list;
	plist = phead->next;
	//check sa
	if (acl_mode == 1)		// 1: positive check, only those on acl_list can be connected.
		res = FAIL;
	else
		res = SUCCESS;

	while(plist != phead)
	{
		paclnode = list_entry(plist, struct wlan_acl_node, list);
		plist = plist->next;
		if (!memcmp((void *)sa, paclnode->addr, 6)) {
			if (paclnode->mode & 2) { // deny
				res = FAIL;
				break;
			}
			else {
				res = SUCCESS;
				break;
			}
		}
	}

	RESTORE_INT(flags);

#if defined(CONFIG_RTK_MESH) && defined(MESH_ESTABLISH_RSSI_THRESHOLD)
		if (pfrinfo->rssi < priv->mesh_fake_mib.establish_rssi_threshold)
			res = FAIL;
#endif

#ifdef __DRAYTEK_OS__
	if (res == SUCCESS) {
		if (cb_auth_request(priv->dev, sa) != 0)
			res = FAIL;
	}
#endif

	if (res != SUCCESS) {
		DEBUG_ERR("auth abort because ACL!\n");

		log_info = aclog_lookfor_entry(priv, sa);
		if (log_info) {
			aclog_update_entry(log_info, sa);

			if (log_info->cur_cnt == 1) { // first time trigger
#if defined(CONFIG_RTL8196B_TR)
				LOG_MSG_DROP("Unauthorized wireless PC try to connect;note:%02x:%02x:%02x:%02x:%02x:%02x;\n",
					*sa, *(sa+1), *(sa+2), *(sa+3), *(sa+4), *(sa+5));
#elif defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
				LOG_MSG_DROP("Unauthorized wireless PC try to connect;note:%02x:%02x:%02x:%02x:%02x:%02x;\n",
					*sa, *(sa+1), *(sa+2), *(sa+3), *(sa+4), *(sa+5));
#elif defined(CONFIG_RTL8196B_TLD)
				LOG_MSG_DEL("[WLAN access denied] from MAC: %02x:%02x:%02x:%02x:%02x:%02x,\n",
					*sa, *(sa+1), *(sa+2), *(sa+3), *(sa+4), *(sa+5));
#else
				LOG_MSG("A wireless client was rejected due to access control - %02X:%02X:%02X:%02X:%02X:%02X\n",
					*sa, *(sa+1), *(sa+2), *(sa+3), *(sa+4), *(sa+5));
#endif
				log_info->last_cnt = log_info->cur_cnt;

				if (priv->acLogCountdown == 0)
					priv->acLogCountdown = AC_LOG_TIME;
			}
		}
		return FAIL;
	}

	if (priv->pmib->dot11StationConfigEntry.supportedStaNum) {
		if (priv->assoc_num >= priv->pmib->dot11StationConfigEntry.supportedStaNum) {
			DEBUG_ERR("Exceed the upper limit of supported clients...\n");
			status = _STATS_UNABLE_HANDLE_STA_;
			goto auth_fail;
		}
	}
	}	//if MESH is enable here is end of (FALSE == isMeshMP)

	// (below, share with Mesh) due to the free_statinfo in AUTH_TO, we should enter critical section here!
	SAVE_INT_AND_CLI(flags);

	if (pstat == NULL)	// STA only, other one, Don't detect peer MP myself, But peer MP detect me and send Auth request.;nctu note
	{
#ifdef CONFIG_RTK_MESH
		// Denied STA auth, When MP configure MAP "OFF"(beacon or ProbeREQ/RSP AP information OFF), and STA ignore these information.
		if ((1 == GET_MIB(priv)->dot1180211sInfo.mesh_enable) && (0 == GET_MIB(priv)->dot1180211sInfo.mesh_ap_enable)) {
			status = _STATS_OTHER_;
			goto auth_fail;
		}
#endif

		// allocate a new one
		DEBUG_INFO("going to alloc stainfo for sa=%02X%02X%02X%02X%02X%02X\n",  sa[0],sa[1],sa[2],sa[3],sa[4],sa[5]);
		pstat = alloc_stainfo(priv, sa, -1);

		if (pstat == NULL)
		{
			DEBUG_ERR("Exceed the upper limit of supported clients...\n");
			status = _STATS_UNABLE_HANDLE_STA_;
			goto auth_fail;
		}
		pstat->state = WIFI_AUTH_NULL;
		pstat->auth_seq = 0;	// clear in alloc_stainfo;nctu note
	}
	else
	{	// close exist connection.;nctu note
		if (!list_empty(&pstat->asoc_list))
		{
			list_del_init(&pstat->asoc_list);

#if defined(CONFIG_RTK_MESH) && defined(MESH_BOOTSEQ_AUTH)
			if ((TRUE == isMeshMP)	// fix: 00000053 2007/12/11  NOTE!! Best solution detail see bug report !!
					&& ((pstat->mesh_neighbor_TBL.State == MP_SUPERORDINATE_LINK_UP) || (pstat->mesh_neighbor_TBL.State == MP_SUBORDINATE_LINK_UP)
					|| (pstat->mesh_neighbor_TBL.State == MP_SUPERORDINATE_LINK_DOWN) || (pstat->mesh_neighbor_TBL.State == MP_SUBORDINATE_LINK_DOWN_E)))
			{
				mesh_cnt_ASSOC_PeerLink_CAP(priv, pstat, DECREASE);
				if (!list_empty(&pstat->mesh_mp_ptr))	// add by Galileo
					list_del_init(&(pstat->mesh_mp_ptr));

			}
#endif

			if (pstat->expire_to > 0)
			{
				cnt_assoc_num(priv, pstat, DECREASE, (char *)__FUNCTION__);
				check_sta_characteristic(priv, pstat, DECREASE);
			}
		}
		if (seq==1) {
#ifdef __ZINWELL__
			unsigned int key_len=0;
			unsigned char tmpbuf[100];
			if (priv->keep_encrypt_key && GET_UNICAST_ENCRYP_KEYLEN) {
				key_len = GET_UNICAST_ENCRYP_KEYLEN;
				memcpy(tmpbuf, GET_UNICAST_ENCRYP_KEY, key_len);
			}
#endif
			release_stainfo(priv, pstat);
			init_stainfo(priv, pstat);
#ifdef __ZINWELL__
			if (key_len) {
				memcpy(GET_UNICAST_ENCRYP_KEY, tmpbuf, key_len);
				GET_UNICAST_ENCRYP_KEYLEN = key_len;
				//debug_out("keep old wep key", GET_UNICAST_ENCRYP_KEY, GET_UNICAST_ENCRYP_KEYLEN);
			}
#endif

#if defined(CONFIG_RTK_MESH) && defined(MESH_BOOTSEQ_AUTH)
			if (TRUE == isMeshMP) {
				if (!list_empty(&pstat->mesh_mp_ptr))
					list_del_init(&pstat->mesh_mp_ptr);

				// Avoid pstat remain in system when can't auth, Not actually use
				pstat->mesh_neighbor_TBL.BSexpire_LLSAperiod = jiffies + MESH_PEER_LINK_LISTEN_TO;
				pstat->mesh_neighbor_TBL.State = MP_LISTEN;
				pstat->state = WIFI_AUTH_NULL;
				SET_PSEUDO_RANDOM_NUMBER(pstat->mesh_neighbor_TBL.LocalLinkID);

				list_add_tail(&(pstat->mesh_mp_ptr), &(priv->mesh_auth_hdr));

				if (!(timer_pending(&priv->mesh_auth_timer)))	// start timer if stop
					mod_timer(&priv->mesh_auth_timer, jiffies + MESH_TIMER_TO);
			}
#endif

		}
	}

	if (list_empty(&(pstat->auth_list)))
		list_add_tail(&(pstat->auth_list), &(priv->auth_list));

	if (pstat->auth_seq == 0)
		pstat->expire_to = priv->auth_to;

	// Authentication Sequence (STA only)
#if defined(CONFIG_RTK_MESH) && defined(MESH_BOOTSEQ_AUTH)	// Mesh can't check, Because recived seq=1 and 2
	if((FALSE == isMeshMP) && ((pstat->auth_seq + 1) != seq))
#else
	if ((pstat->auth_seq + 1) != seq)
#endif
	{
		DEBUG_ERR("(1)auth rejected because out of seq [rx_seq=%d, exp_seq=%d]!\n",
			seq, pstat->auth_seq+1);
		status = _STATS_OUT_OF_AUTH_SEQ_;
		goto auth_fail;
	}

#ifdef __ZINWELL__
#ifndef WITHOUT_ENQUEUE
#if defined(CONFIG_RTK_MESH) && defined(MESH_BOOTSEQ_AUTH)	// No pass message to upper
	if ((priv->indicate_auth && seq == 1) && (FALSE == isMeshMP))
#else
	if (priv->indicate_auth && seq == 1)
#endif
	{
		DOT11_AUTH_IND auth_ind;
		memcpy((void *)auth_ind.MACAddr, (void *)GetAddr2Ptr(pframe), MACADDRLEN);
		auth_ind.EventId = DOT11_EVENT_AUTHENTICATION_IND;
		auth_ind.IsMoreEvent = 0;
		DEBUG_INFO("authentication indication, mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
			auth_ind.MACAddr[0],auth_ind.MACAddr[1],auth_ind.MACAddr[2],
			auth_ind.MACAddr[3],auth_ind.MACAddr[4],auth_ind.MACAddr[5]);

		DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&auth_ind,
						sizeof(DOT11_AUTH_IND));
	}
#endif
#endif

#if defined(CONFIG_RTK_MESH) && defined(MESH_BOOTSEQ_AUTH)
	if (algorithm == 0 && ((FALSE == isMeshMP && (privacy == 0 || privacy == 2)) || (TRUE == isMeshMP))) {	// open auth (STA & mesh)
		if ((1 == seq) || ((2 == seq) && (TRUE == isMeshMP))) {

			if((2 == seq) && ((MP_OPEN_SENT != prevState) ||
				(_STATS_SUCCESSFUL_ != cpu_to_le16(*(unsigned short *)((unsigned int)pframe + WLAN_HDR_A3_LEN + 2 + 2)))))
				return FAIL;
#else
	if (algorithm == 0 && (privacy == 0 || privacy == 2)) {// STA only open auth
		if (seq == 1) {
#endif

			pstat->state &= ~WIFI_AUTH_NULL;
			pstat->state |= WIFI_AUTH_SUCCESS;
			pstat->expire_to = priv->assoc_to;
			pstat->AuthAlgrthm = algorithm;

#if defined(CONFIG_RTK_MESH) && defined(MESH_BOOTSEQ_AUTH)
			if (TRUE == isMeshMP) {	// Authentication Success

				if (!list_empty(&pstat->mesh_mp_ptr))	// mesh_auth_hdr -> mesh_unEstablish_hdr
					list_del_init(&(pstat->mesh_mp_ptr));

				if ((1 == seq) && (MP_OPEN_SENT != prevState)) { // Passive
					pstat->expire_to = priv->assoc_to;
					MESH_DEBUG_MSG("Mesh: Auth Successful... seq = '%d', state = '%d', And 'PASSIVE' connect\n", seq, prevState);
					pstat->mesh_neighbor_TBL.BSexpire_LLSAperiod = jiffies + MESH_PEER_LINK_LISTEN_TO;
					pstat->mesh_neighbor_TBL.State = MP_LISTEN;
				} else { // Active (include associated)
					MESH_DEBUG_MSG("Mesh: Auth Successful... seq = '%d', state = '%d', And 'ACTIVE' connect\n", seq, prevState);
					pstat->mesh_neighbor_TBL.BSexpire_LLSAperiod = jiffies + MESH_PEER_LINK_RETRY_TO;
					pstat->mesh_neighbor_TBL.retry = 0;
					pstat->mesh_neighbor_TBL.State = MP_OPEN_SENT;
				}

				list_add_tail(&(pstat->mesh_mp_ptr), &(priv->mesh_unEstablish_hdr));

				if (!(timer_pending(&priv->mesh_peer_link_timer)))		// start timer if stop
					mod_timer(&priv->mesh_peer_link_timer, jiffies + MESH_TIMER_TO);

			}
#endif

		}
		else
		{
			DEBUG_ERR("(2)auth rejected because out of seq [rx_seq=%d, exp_seq=%d]!\n",
				seq, pstat->auth_seq+1);
			status = _STATS_OUT_OF_AUTH_SEQ_;
			goto auth_fail;
		}
	}
	else // shared system or auto authentication (STA only).
	{
#if defined(CONFIG_RTK_MESH) && defined(MESH_BOOTSEQ_AUTH)
		if (TRUE == isMeshMP) {
			MESH_DEBUG_MSG("Mesh: Auth rejected because You're Mesh MP, but use incorrect algorithm = %d!\n", algorithm);
			status = _STATS_CHALLENGE_FAIL_;
			goto auth_fail;
		}
#endif
		if (seq == 1)
		{
			//prepare for the challenging txt...
			get_random_bytes((void *)pstat->chg_txt, 128);
			pstat->state &= ~WIFI_AUTH_NULL;
			pstat->state |= WIFI_AUTH_STATE1;
			pstat->AuthAlgrthm = algorithm;
			pstat->auth_seq = 2;
		}
		else if (seq == 3)
		{
			//checking for challenging txt...
			p = get_ie(pframe + WLAN_HDR_A3_LEN + 4 + _AUTH_IE_OFFSET_, _CHLGETXT_IE_, (int *)&len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _AUTH_IE_OFFSET_ - 4);
			if ((p != NULL) && !memcmp((void *)(p + 2),pstat->chg_txt, 128))
			{
				pstat->state &= (~WIFI_AUTH_STATE1);
				pstat->state |= WIFI_AUTH_SUCCESS;
				// challenging txt is correct...
				pstat->expire_to = priv->assoc_to;
			}
			else
			{
				DEBUG_ERR("auth rejected because challenge failure!\n");
				status = _STATS_CHALLENGE_FAIL_;
#if defined(CONFIG_RTL8196B_TLD)
				LOG_MSG_DEL("[WLAN access rejected: incorrect security] from MAC address: %02x:%02x:%02x:%02x:%02x:%02x,\n",
					sa[0], sa[1], sa[2], sa[3], sa[4], sa[5]);
#endif
				goto auth_fail;
			}
		}
		else
		{
			DEBUG_ERR("(3)auth rejected because out of seq [rx_seq=%d, exp_seq=%d]!\n",
				seq, pstat->auth_seq+1);
			status = _STATS_OUT_OF_AUTH_SEQ_;
			goto auth_fail;
		}
	}

	// Now, we are going to issue_auth...

#if defined(CONFIG_RTK_MESH) && defined(MESH_BOOTSEQ_AUTH)
	if (TRUE == isMeshMP)
		pstat->auth_seq = 2;		// Mesh send seq=2 (response) only
	else
#endif
	pstat->auth_seq = seq + 1;
#if defined(CONFIG_RTK_MESH) && defined(MESH_BOOTSEQ_AUTH)
	if ((FALSE == isMeshMP) || ((1 == seq) && (TRUE == isMeshMP)))
#endif
	issue_auth(priv, pstat, (unsigned short)(_STATS_SUCCESSFUL_));

	if (pstat->state & WIFI_AUTH_SUCCESS)	// STA valid
		pstat->auth_seq = 0;

	RESTORE_INT(flags);

#if defined(CONFIG_RTK_MESH) && defined(MESH_BOOTSEQ_AUTH)
	if ((TRUE == isMeshMP) && ( MP_OPEN_SENT == pstat->mesh_neighbor_TBL.State) && (2 == seq)) // Active 2==seq , auth success prevState is MP_OPEN_SENT
		issue_assocreq_MP(priv, pstat);
#endif

	return SUCCESS;

auth_fail:

	if ((OPMODE & WIFI_AP_STATE) && (pstat == NULL)) {
		pstat = (struct stat_info *)kmalloc(sizeof(struct stat_info), GFP_ATOMIC);
		if (pstat == NULL) {
			RESTORE_INT(flags);
			return FAIL;
		}

		alloc_pstat = 1;
		memset(pstat, 0, sizeof(struct stat_info));

		pstat->auth_seq = 2;
		memcpy(pstat->hwaddr, sa, 6);
		pstat->AuthAlgrthm = algorithm;
	}
	else {
		alloc_pstat = 0;
		pstat->auth_seq = seq + 1;
	}

	issue_auth(priv, pstat, (unsigned short)status);

	if (alloc_pstat)
		kfree(pstat);

	SNMP_MIB_ASSIGN(dot11AuthenticateFailStatus, status);
	SNMP_MIB_COPY(dot11AuthenticateFailStation, sa, MACADDRLEN);

	RESTORE_INT(flags);

	return FAIL;
}



/**
 *	@brief	AP recived De-Authentication
 *
 */
static unsigned int OnDeAuth(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned char *pframe;
	struct  stat_info   *pstat;
	unsigned char *sa;
	unsigned short reason;
	DOT11_DISASSOCIATION_IND Disassociation_Ind;
	unsigned long flags;

	pframe = get_pframe(pfrinfo);
	sa = GetAddr2Ptr(pframe);
	pstat = get_stainfo(priv, sa);

	if (pstat == NULL)
		return 0;

#ifdef RTK_WOW
	if (pstat->is_rtk_wow_sta)
		return 0;
#endif

	reason = cpu_to_le16(*(unsigned short *)((unsigned int)pframe + WLAN_HDR_A3_LEN ));
	DEBUG_INFO("receiving deauth from station %02X%02X%02X%02X%02X%02X reason %d\n",
		pstat->hwaddr[0], pstat->hwaddr[1], pstat->hwaddr[2],
		pstat->hwaddr[3], pstat->hwaddr[4], pstat->hwaddr[5], reason);

	SAVE_INT_AND_CLI(flags);
	if (!list_empty(&pstat->asoc_list))
	{
		list_del_init(&pstat->asoc_list);
		if (pstat->expire_to > 0)
		{
			cnt_assoc_num(priv, pstat, DECREASE, (char *)__FUNCTION__);
			check_sta_characteristic(priv, pstat, DECREASE);

#ifdef CONFIG_RTK_MESH
			if (isPossibleNeighbor(pstat))
			{
				mesh_cnt_ASSOC_PeerLink_CAP(priv, pstat, DECREASE);
			}
#endif
		}
	}
	RESTORE_INT(flags);

#ifdef RTL8190_FASTEXTDEV_FUNCALL
	rtl865x_extDev_removeHost(pstat->hwaddr, priv->WLAN_VLAN_ID);
#endif

	// stop rssi check
#ifdef STA_EXT
	if ((OPMODE & WIFI_AP_STATE) && (pstat->remapped_aid <= 8))
		set_fw_reg(priv, 0xfd000013 | pstat->remapped_aid<<16, 0, 0);
#else
	if ((OPMODE & WIFI_AP_STATE) && (pstat->aid <= 8))
		set_fw_reg(priv, 0xfd000013 | pstat->aid<<16, 0, 0);
#endif

	free_stainfo(priv, pstat);

	LOG_MSG("A wireless client is deauthenticated - %02X:%02X:%02X:%02X:%02X:%02X\n",
		*sa, *(sa+1), *(sa+2), *(sa+3), *(sa+4), *(sa+5));

	if (IEEE8021X_FUN)
	{
#ifndef WITHOUT_ENQUEUE
#ifdef CONFIG_RTK_MESH
		if (isSTA(pstat))
#endif
		{
		memcpy((void *)Disassociation_Ind.MACAddr, (void *)sa, MACADDRLEN);
		Disassociation_Ind.EventId = DOT11_EVENT_DISASSOCIATION_IND;
		Disassociation_Ind.IsMoreEvent = 0;
		Disassociation_Ind.Reason = reason;
		Disassociation_Ind.tx_packets = pstat->tx_pkts;
		Disassociation_Ind.rx_packets = pstat->rx_pkts;
		Disassociation_Ind.tx_bytes   = pstat->tx_bytes;
		Disassociation_Ind.rx_bytes   = pstat->rx_bytes;
		DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&Disassociation_Ind,
					sizeof(DOT11_DISASSOCIATION_IND));
		}
#endif
#ifdef INCLUDE_WPA_PSK
		psk_indicate_evt(priv, DOT11_EVENT_DISASSOCIATION_IND, sa, NULL, 0);
#endif
	}

	event_indicate(priv, sa, 2);

	return SUCCESS;
}


int should_send_ADDBArsp(struct rtl8190_priv *priv, struct stat_info *pstat)
{

	return TRUE;
}


static unsigned int OnWmmAction(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo)
{
#ifdef CONFIG_RTK_MESH
	// please add the codes to check where the action frame is rreq, rrep or rrer
	// (check  the action field )
	if (GET_MIB(priv)->dot1180211sInfo.mesh_enable) {
		unsigned char  *pframe, *pFrameBody;
		unsigned char action_type = -1, category_type;
		int Is_6Addr = 0;

		pframe = get_pframe(pfrinfo);  // get frame data

		if (pframe!=0) {
			if(is_mesh_6addr_format_without_qos(pframe)) {
				pFrameBody = pframe + WLAN_HDR_A6_MESH_DATA_LEN;
				Is_6Addr = 1;
			} else
				pFrameBody = pframe + WLAN_HDR_A4_MESH_DATA_LEN;

			//pFrameBody = GetMeshMgtPtr(pframe);
			// reason = cpu_to_le16(*(unsigned short *)((unsigned int)pframe + WLAN_HDR_A4_MESH_MGT_LEN )); //len define ref to wifi.h
			category_type = *pFrameBody;
			action_type = *(pFrameBody+1);



#ifdef MESH_USE_METRICOP
			if(category_type == _CATEGORY_11K_ACTION_)
			{
				switch(action_type) {
					case ACTION_FILED_11K_LINKME_REQ:
						return On11kvLinkMeasureReq(priv, pfrinfo);
					case ACTION_FILED_11K_LINKME_REP:
						return On11kvLinkMeasureRep(priv, pfrinfo);
				}
			}
#endif

			if(category_type != _CATEGORY_MESH_ACTION_)
				goto	ACTIVE_NOT_11S;

			//ACTION_FIELD_PU and ACTION_FIELD_PUC by pepsi
			switch(action_type) {
				case ACTION_FIELD_LOCAL_LINK_STATE_ANNOUNCE:
					OnLocalLinkStateANNOU_MP(priv, pfrinfo);
					break;
#ifdef PU_STANDARD
				case ACTION_FIELD_PU:
					OnProxyUpdate_MP(priv,pfrinfo);
					break;
				case ACTION_FIELD_PUC:
					OnProxyUpdateConfirm_MP(priv,pfrinfo);
					break;
#endif
				case ACTION_FIELD_RREQ:
				case ACTION_FIELD_RREP:
				case ACTION_FIELD_RERR:
				case ACTION_FIELD_RREP_ACK:
				case ACTION_FIELD_PANN:
				case ACTION_FIELD_RANN:
					OnPathSelectionManagFrame(priv, pfrinfo, Is_6Addr);
					break;
				case ACTION_FIELD_HELLO:
					break;
				case -1:
					printk("the action_type in OnWmmAction is decoded fail \n");
					goto ACTIVE_NOT_11S;
				default:
					printk("unknow action_type in OnWmmAction\n");
					goto ACTIVE_NOT_11S;
			}
		}
		return SUCCESS;
	}

ACTIVE_NOT_11S:
#endif	// CONFIG_RTK_MESH

	// not support action frame right now
#ifdef SEMI_QOS
	if (QOS_ENABLE) {
		unsigned char *sa = pfrinfo->sa;
		unsigned char *da = pfrinfo->da;
		struct stat_info *pstat = get_stainfo(priv, sa);
		unsigned char *pframe=NULL;
		unsigned char Category_field=0, Action_field=0, TID=0, previous_mimo_ps=0;
		unsigned short blockAck_para=0, status_code=_STATS_SUCCESSFUL_, timeout=0, reason_code, max_size;

		// Reply in B/G mode to fix IOT issue with D-Link DWA-642

		if ((!IS_MCAST(da)) && (pstat)) {
			pframe = get_pframe(pfrinfo) + WLAN_HDR_A3_LEN;	//start of action frame content
			Category_field = pframe[0];
			Action_field = pframe[1];
			switch (Category_field) {
				case _BLOCK_ACK_CATEGORY_ID_:
					switch (Action_field) {
						case _ADDBA_Req_ACTION_ID_:
							blockAck_para = pframe[3] | (pframe[4] << 8);
							timeout = pframe[5] | (pframe[6] << 8);
							TID = (blockAck_para>>2)&0x000f;
							max_size = (blockAck_para&0xffc0)>>6;
							DEBUG_INFO("ADDBA-req recv fr AID %d, token %d TID %d size %d timeout %d\n",
								pstat->aid, pframe[2], TID, max_size, timeout);
							if (!(blockAck_para & BIT(1))
#ifdef STA_EXT
							|| (pstat->sta_in_firmware != 1 && priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _NO_PRIVACY_)
#endif
								)	// 0=delayed BA, 1=immediate BA
								status_code = _STATS_REQ_DECLINED_;
							if (should_send_ADDBArsp(priv, pstat)) {
								if (!issue_ADDBArsp(priv, sa, pframe[2], TID, status_code, timeout))
									DEBUG_ERR("issue ADDBA-rsp failed\n");
							}
							break;
						case _DELBA_ACTION_ID_:
							pstat->ADDBA_ready = 0;
							TID = (pframe[3] & 0xf0) >> 4;
							reason_code = pframe[4] | (pframe[5] << 8);
							DEBUG_INFO("DELBA recv from AID %d, TID %d reason %d\n", pstat->aid, TID, reason_code);
							break;
						case _ADDBA_Rsp_ACTION_ID_:
							blockAck_para = pframe[5] | (pframe[6] << 8);
							status_code = pframe[3] | (pframe[4] << 8);
							TID = (blockAck_para>>2)&0x000f;
							max_size = (blockAck_para&0xffc0)>>6;
							DEBUG_INFO("ADDBA-rsp recv fr AID %d, token %d TID %d size %d status %d\n",
								pstat->aid, pframe[2], TID, max_size, status_code);
							break;
						default:
							DEBUG_ERR("Error BA Action frame is received\n");
							goto error_frame;
							break;
					}
					break;

#ifdef WIFI_11N_2040_COEXIST
				case _PUBLIC_CATEGORY_ID_:
					switch (Action_field) {
						case _2040_COEXIST_ACTION_ID_:
							if (priv->pshare->rf_ft_var.coexist_enable) {
								if (!priv->pshare->is_40m_bw) {
									DEBUG_WARN("Ignored Public Action frame received since AP is 20m mode\n");
									break;
								}

								if (pframe[2] == _2040_BSS_COEXIST_IE_) {
									if (pframe[4] & (_40M_INTOLERANT_ |_20M_BSS_WIDTH_REQ_)) {
										priv->switch_20_sta |= BIT(pstat->aid - 1);
										if (pframe[4] & _40M_INTOLERANT_) {
											DEBUG_INFO("Public Action frame: force 20m by 40m intolerant\n");
										}
										else
											DEBUG_INFO("Public Action frame: force 20m by 20m bss width req\n");
									}
									else {
										if ((pframe[2+pframe[3]+2]) && (pframe[2+pframe[3]+2] == _2040_Intolerant_ChRpt_IE_)) {
											priv->switch_20_sta |= BIT(pstat->aid - 1);
											DEBUG_INFO("Public Action frame: force 20m by channel report\n");
										}
										else {
											priv->switch_20_sta &= ~BIT(pstat->aid - 1);
											DEBUG_INFO("Public Action frame: cancel force 20m\n");
										}
									}

									// let the first tx packet go through normal path and set fw properly
#ifdef TX_SHORTCUT									
									pstat->hwdesc1.Dword7 &= ~(TX_TxBufferSizeMask);
#endif
								}
								else
									DEBUG_ERR("Error Public Action frame received\n");
							}
							else
								DEBUG_WARN("Public Action frame received but func off\n");
							break;
						default:
							DEBUG_INFO("Public Action frame received but not support yet\n");
							goto error_frame;
							break;
					}
					break;
#endif

				case _HT_CATEGORY_ID_:
					if (Action_field == _HT_MIMO_PS_ACTION_ID_) {
						previous_mimo_ps = pstat->MIMO_ps;
						pstat->MIMO_ps = 0;
						if (pframe[2] & BIT(0)) {
							if (pframe[2] & BIT(1))
								pstat->MIMO_ps|=_HT_MIMO_PS_DYNAMIC_;
							else
								pstat->MIMO_ps|=_HT_MIMO_PS_STATIC_;
						}
						if ((previous_mimo_ps|pstat->MIMO_ps)&_HT_MIMO_PS_STATIC_) {
							assign_tx_rate(priv, pstat, pfrinfo);
							add_update_RATid(priv, pstat);
						}
#ifdef TX_SHORTCUT
						if ((previous_mimo_ps|pstat->MIMO_ps)&_HT_MIMO_PS_DYNAMIC_)
							pstat->hwdesc1.Dword7 &= ~(TX_TxBufferSizeMask);	// let the first tx packet go through normal path and set fw properly
#endif
					}
					else
						DEBUG_INFO("HT Action Frame is received but not support yet\n");
					break;

				default:
					DEBUG_INFO("Action Frame is received but not support yet\n");
					break;
			}
		}
		else {
			if (IS_MCAST(da)) {
			}
			else {
				DEBUG_ERR("Action Frame is received from non-associated station\n");
			}
		}
	}
error_frame:
#endif
	return SUCCESS;
}


static unsigned int DoReserved(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo)
{
	return SUCCESS;
}


#ifdef CLIENT_MODE
static void update_bss(struct Dot11StationConfigEntry *dst, struct bss_desc *src)
{
	memcpy((void *)dst->dot11Bssid, (void *)src->bssid, MACADDRLEN);
	memset((void *)dst->dot11DesiredSSID, 0, sizeof(dst->dot11DesiredSSID));
	memcpy((void *)dst->dot11DesiredSSID, (void *)src->ssid, src->ssidlen);
	dst->dot11DesiredSSIDLen = src->ssidlen;
}

/**
 *	@brief	Authenticat success, Join a BSS
 *
 *	Set BSSID to hardware, Join BSS complete
 */
static void join_bss(struct rtl8190_priv *priv)
{
	unsigned long	ioaddr = priv->pshare->ioaddr;
	unsigned short	val16;
	unsigned long	val32;

	memcpy((void *)&val32, BSSID, 4);
	memcpy((void *)&val16, BSSID+4, 2);
	RTL_W32(BSSIDR, cpu_to_le32(val32));
	RTL_W16((BSSIDR + 4), cpu_to_le16(val16));
}


/**
 *	@brief	issue Association Request
 *
 *	STA find compatible network, and authenticate success, use this function send association request. \n
 *	+---------------+----+----+-------+------------+-----------------+------+-----------------+	\n
 *	| Frame Control | DA | SA | BSSID | Capability | Listen Interval | SSID | Supported Rates |	\n
 *	+---------------+----+----+-------+------------+-----------------+------+-----------------+	\n
 *	\n
 *	+--------------------+---------------------+-----------------+	\n
 *  | Ext. Support Rates | Realtek proprietary | RSN information |	\n
 *	+--------------------+---------------------+-----------------+	\n
 *
 *	PS: Reassociation Frame Body have Current AP Address field, But not implement.
 */
static unsigned int issue_assocreq(struct rtl8190_priv *priv)
{
	unsigned short	val;
	struct wifi_mib *pmib;
	unsigned char	*bssid, *pbuf;
	unsigned char	*pbssrate=NULL;
	int		bssrate_len;
	unsigned char	supportRateSet[32];
	int		i, j, idx=0, supportRateSetLen=0, match=0;
	unsigned int	retval=0;
#ifdef SEMI_QOS
	int		k;
#endif
#if defined(CONFIG_RTL_WAPI_SUPPORT)
	unsigned long		flags;
#endif

	DECLARE_TXINSN(txinsn);

	txinsn.retry = priv->pmib->dot11OperationEntry.dot11ShortRetryLimit;

	pmib= GET_MIB(priv);

	bssid = pmib->dot11Bss.bssid;

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
	txinsn.lowest_tx_rate = txinsn.tx_rate;
	txinsn.fixed_rate = 1;
	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);

	if (pbuf == NULL)
		goto issue_assocreq_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);

	if (txinsn.phdr == NULL)
		goto issue_assocreq_fail;
	memset((void *)txinsn.phdr, 0, sizeof(struct  wlan_hdr));

	val = cpu_to_le16(pmib->dot11Bss.capability);

	if (pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm)
		val |= cpu_to_le16(BIT(4));

	if (SHORTPREAMBLE)
		val |= cpu_to_le16(BIT(5));

	pbuf = set_fixed_ie(pbuf, _CAPABILITY_, (unsigned char *)&val, &txinsn.fr_len);

	val	 = cpu_to_le16(3);
	pbuf = set_fixed_ie(pbuf, _LISTEN_INTERVAL_, (unsigned char *)&val, &txinsn.fr_len);

	pbuf = set_ie(pbuf, _SSID_IE_, pmib->dot11Bss.ssidlen, pmib->dot11Bss.ssid, &txinsn.fr_len);

	if (pmib->dot11Bss.supportrate == 0)
	{
		// AP don't contain rate info in beacon/probe response
		// Use our rate in asoc req
		get_bssrate_set(priv, _SUPPORTEDRATES_IE_, &pbssrate, &bssrate_len);
		pbuf = set_ie(pbuf, _SUPPORTEDRATES_IE_, bssrate_len, pbssrate, &txinsn.fr_len);

		//EXT supported rates.
		if (get_bssrate_set(priv, _EXT_SUPPORTEDRATES_IE_, &pbssrate, &bssrate_len))
			pbuf = set_ie(pbuf, _EXT_SUPPORTEDRATES_IE_, bssrate_len, pbssrate, &txinsn.fr_len);
	}
	else
	{
		// See if there is any mutual supported rate
		for (i=0; dot11_rate_table[i]; i++) {
			int bit_mask = 1 << i;
			if (pmib->dot11Bss.supportrate & bit_mask) {
				val = dot11_rate_table[i];
				for (j=0; j<AP_BSSRATE_LEN; j++) {
					if (val == (AP_BSSRATE[j] & 0x7f)) {
						match = 1;
						break;
					}
				}
				if (match)
					break;
			}
		}

		// If no supported rates match, assoc fail!
		if (!match) {
			DEBUG_ERR("Supported rate mismatch!\n");
			retval = 1;
			goto issue_assocreq_fail;
		}

		// Use AP's rate info in asoc req
		for (i=0; dot11_rate_table[i]; i++) {
			int bit_mask = 1 << i;
			if (pmib->dot11Bss.supportrate & bit_mask) {
				val = dot11_rate_table[i];
				if (((pmib->dot11BssType.net_work_type == WIRELESS_11B) && is_CCK_rate(val)) ||
					(pmib->dot11BssType.net_work_type != WIRELESS_11B)) {
					if (pmib->dot11Bss.basicrate & bit_mask)
						val |= 0x80;

					supportRateSet[idx] = val;
					supportRateSetLen++;
					idx++;
				}
			}
		}

		if (supportRateSetLen == 0) {
			retval = 1;
			goto issue_assocreq_fail;
		}
		else if (supportRateSetLen <= 8)
			pbuf = set_ie(pbuf, _SUPPORTEDRATES_IE_ , supportRateSetLen , supportRateSet, &txinsn.fr_len);
		else {
			pbuf = set_ie(pbuf, _SUPPORTEDRATES_IE_, 8, supportRateSet, &txinsn.fr_len);
			pbuf = set_ie(pbuf, _EXT_SUPPORTEDRATES_IE_, supportRateSetLen-8, &supportRateSet[8], &txinsn.fr_len);
		}
	}

	{
#ifdef WIFI_SIMPLE_CONFIG
		if (!(pmib->wscEntry.wsc_enable && pmib->wscEntry.assoc_ielen))
#endif
		{
			if (pmib->dot11RsnIE.rsnielen) {
				memcpy(pbuf, pmib->dot11RsnIE.rsnie, pmib->dot11RsnIE.rsnielen);
				pbuf += pmib->dot11RsnIE.rsnielen;
				txinsn.fr_len += pmib->dot11RsnIE.rsnielen;
			}
		}
	}


	if ((QOS_ENABLE) || (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)) {
		int count=0;
		struct bss_desc	*bss=NULL;

		if (priv->site_survey.count) {
			count = priv->site_survey.count;
			bss = priv->site_survey.bss;
		}
		else if (priv->site_survey.count_backup) {
			count = priv->site_survey.count_backup;
			bss = priv->site_survey.bss_backup;
		}

		for(k=0; k<count; k++) {
			if (!memcmp((void *)bssid, bss[k].bssid, MACADDRLEN)) {

#ifdef SEMI_QOS
				if ((QOS_ENABLE) && (bss[k].t_stamp[1] & BIT(0)))	//  AP supports WMM when t_stamp[1] bit 0 is set
					pbuf = set_ie(pbuf, _RSN_IE_1_, _WMM_IE_Length_, GET_WMM_IE, &txinsn.fr_len);
#endif
				if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
						(bss[k].network & WIRELESS_11N)) {

					int is_40m_bw, offset_chan;
					is_40m_bw = (bss[k].t_stamp[1] & BIT(1)) ? 1 : 0;
					if (is_40m_bw) {
						if (bss[k].t_stamp[1] & BIT(2))
							offset_chan = 1;
						else
							offset_chan = 2;
					}
					else
						offset_chan = 0;

					construct_ht_ie(priv, is_40m_bw, offset_chan);
					pbuf = set_ie(pbuf, _HT_CAP_, priv->ht_cap_len, (unsigned char *)&priv->ht_cap_buf, &txinsn.fr_len);
				}

				break;
			}
		}
	}

#ifdef WIFI_SIMPLE_CONFIG
	if (pmib->wscEntry.wsc_enable && pmib->wscEntry.assoc_ielen) {
		memcpy(pbuf, pmib->wscEntry.assoc_ie, pmib->wscEntry.assoc_ielen);
		pbuf += pmib->wscEntry.assoc_ielen;
		txinsn.fr_len += pmib->wscEntry.assoc_ielen;
	}
#endif

#if defined(CONFIG_RTL_WAPI_SUPPORT)
		if (priv->pmib->wapiInfo.wapiType!=wapiDisable)
		{
			SAVE_INT_AND_CLI(flags);
			priv->wapiCachedBuf = pbuf+2;
			wapiSetIE(priv);
			pbuf[0] = _EID_WAPI_;
			pbuf[1] = priv->wapiCachedLen;
			pbuf += priv->wapiCachedLen+2;
			txinsn.fr_len += priv->wapiCachedLen+2;
			RESTORE_INT(flags);
	}
#endif

	// Realtek proprietary IE
	if (priv->pshare->rtk_ie_len)
		pbuf = set_ie(pbuf, _RSN_IE_1_, priv->pshare->rtk_ie_len, priv->pshare->rtk_ie_buf, &txinsn.fr_len);

	// Customer proprietary IE
	if (priv->pmib->miscEntry.private_ie_len) {
		memcpy(pbuf, pmib->miscEntry.private_ie, pmib->miscEntry.private_ie_len);
		pbuf += pmib->miscEntry.private_ie_len;
		txinsn.fr_len += pmib->miscEntry.private_ie_len;
	}

	SetFrameSubType((txinsn.phdr), WIFI_ASSOCREQ);

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), bssid, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), bssid, MACADDRLEN);

	if ((rtl8190_firetx(priv, &txinsn)) == SUCCESS)
		return retval;

issue_assocreq_fail:

	DEBUG_ERR("sending assoc req fail!\n");

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
	return retval;
}


/**
 *	@brief	STA Authentication
 *
 *	STA process Authentication Request first step.
 */
void start_clnt_auth(struct rtl8190_priv *priv)
{
	unsigned long flags;

	SAVE_INT_AND_CLI(flags);

	OPMODE &= ~(WIFI_AUTH_SUCCESS | WIFI_AUTH_STATE1 | WIFI_ASOC_STATE);
	OPMODE |= WIFI_AUTH_NULL;
	priv->reauth_count = 0;
	priv->reassoc_count = 0;
	priv->auth_seq = 1;

	if (timer_pending(&priv->reauth_timer))
		del_timer_sync(&priv->reauth_timer);

	if (timer_pending(&priv->reassoc_timer))
		del_timer_sync(&priv->reassoc_timer);

#ifdef UNIVERSAL_REPEATER
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
		priv->pshare->CurrentChannelBW = HT_CHANNEL_WIDTH_20;
		SwBWMode(priv, priv->pshare->CurrentChannelBW, 0);
		SwChnl(priv, priv->pmib->dot11Bss.channel, 0);
	}

	mod_timer(&priv->reauth_timer, jiffies + REAUTH_TO);

	DEBUG_INFO("start sending auth req\n");
	issue_auth(priv, NULL, 0);

	RESTORE_INT(flags);
}


/**
 *	@brief	client (STA) association
 *
 *	PS: clnt is client.
 */
void start_clnt_assoc(struct rtl8190_priv *priv)
{
	unsigned long flags;

	// now auth has succedded...let's perform assoc
	SAVE_INT_AND_CLI(flags);

	OPMODE &= (~ (WIFI_AUTH_NULL | WIFI_AUTH_STATE1 | WIFI_ASOC_STATE));
	OPMODE |= (WIFI_AUTH_SUCCESS);
	priv->join_res = STATE_Sta_Auth_Success;
	priv->reauth_count = 0;
	priv->reassoc_count = 0;

	if (timer_pending(&priv->reauth_timer))
		del_timer_sync (&priv->reauth_timer);

	if (timer_pending(&priv->reassoc_timer))
		del_timer_sync (&priv->reassoc_timer);


	DEBUG_INFO("start sending assoc req\n");
	if (issue_assocreq(priv) == 0) {
		mod_timer(&priv->reassoc_timer, jiffies + REASSOC_TO);
		RESTORE_INT(flags);
	}
	else {
		RESTORE_INT(flags);
		start_clnt_lookup(priv, 0);
	}
}




#ifdef _11s_TEST_MODE_
void clean_for_join(struct rtl8190_priv *priv)
#else
static inline void clean_for_join(struct rtl8190_priv *priv)
#endif
{
	int i;
	unsigned long flags;

	SAVE_INT_AND_CLI(flags);
	memset(BSSID, 0, MACADDRLEN);
	OPMODE = OPMODE & (WIFI_STATION_STATE | WIFI_ADHOC_STATE);

	for(i=0; i<NUM_STAT; i++) {
		if (priv->pshare->aidarray[i] && (priv->pshare->aidarray[i]->used == TRUE)) {
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
			if (priv != priv->pshare->aidarray[i]->priv)
				continue;
#endif
			if ((free_stainfo(priv, &(priv->pshare->aidarray[i]->station))) == FALSE)
				DEBUG_ERR("free station %d fails\n", i);
		}
	}

	priv->assoc_num = 0;

	if ((OPMODE & WIFI_STATION_STATE) &&
			((priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _TKIP_PRIVACY_) ||
			 (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _CCMP_PRIVACY_) ||
			 (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_WPA_MIXED_PRIVACY_))) {
		memset(&(priv->pmib->dot11GroupKeysTable), 0, sizeof(struct Dot11KeyMappingsEntry));
#ifdef UNIVERSAL_REPEATER
		if (IS_ROOT_INTERFACE(priv))
#endif
			CamResetAllEntry(priv);
	}

	if (IEEE8021X_FUN)
		priv->pmib->dot118021xAuthEntry.dot118021xcontrolport =
			priv->pmib->dot118021xAuthEntry.dot118021xDefaultPort;
	else
		priv->pmib->dot118021xAuthEntry.dot118021xcontrolport = 1;

	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) {
		if (OPMODE & WIFI_ADHOC_STATE) {
			priv->pmib->dot11ErpInfo.nonErpStaNum = 0;
			check_protection_shortslot(priv);
			priv->pmib->dot11ErpInfo.longPreambleStaNum = 0;
		}
	}

	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)
		priv->ht_legacy_sta_num = 0;

	priv->join_res = STATE_Sta_No_Bss;
	priv->link_status = 0;
	//netif_stop_queue(priv->dev);		// don't start/stop queue dynamically
	priv->rxBeaconNumInPeriod = 0;
	memset(priv->rxBeaconCntArray, 0, sizeof(priv->rxBeaconCntArray));
	priv->rxBeaconCntArrayIdx = 0;
	priv->rxBeaconCntArrayWindow = 0;
	priv->rxBeaconPercentage = 0;
	priv->rxDataNumInPeriod = 0;
	memset(priv->rxDataCntArray, 0, sizeof(priv->rxDataCntArray));
	priv->rxMlcstDataNumInPeriod = 0;
	priv->rxDataNumInPeriod_pre = 0;
	priv->rxMlcstDataNumInPeriod_pre = 0;
	RESTORE_INT(flags);
}


/**
 *	@brief	STA join BSS
 *	Join a BSS, In function, emit join request, Before association must be Authentication. \n
 *	[NOTE] TPT information element
 */
void start_clnt_join(struct rtl8190_priv *priv)
{
	struct wifi_mib *pmib = GET_MIB(priv);
	unsigned long ioaddr = priv->pshare->ioaddr;
	unsigned char null_mac[]={0,0,0,0,0,0};
	unsigned char random;
	int i;

// stop ss_timer before join ------------------------
	if (timer_pending(&priv->ss_timer))
		del_timer_sync(&priv->ss_timer);
//------------------------------- david+2007-03-10

#ifdef WIFI_SIMPLE_CONFIG
	if (priv->pmib->wscEntry.wsc_enable == 1) { //wps client mode
		if (priv->wps_issue_join_req)
			priv->wps_issue_join_req = 0;
		else {
			priv->recover_join_req = 1;
			return;
		}
	}
#endif

	// if found bss
	if (memcmp(pmib->dot11Bss.bssid, null_mac, MACADDRLEN))
	{
		priv->beacon_period = pmib->dot11Bss.beacon_prd;
		if (pmib->dot11Bss.bsstype == WIFI_AP_STATE)
		{
#ifdef WIFI_SIMPLE_CONFIG
			if (priv->pmib->wscEntry.wsc_enable == 1) //wps client mode
				priv->recover_join_req = 1;
#endif
			clean_for_join(priv);
			OPMODE = WIFI_STATION_STATE;
#ifdef UNIVERSAL_REPEATER
			if (IS_ROOT_INTERFACE(priv))
#endif
				RTL_W8(_MSR_, _HW_STATE_STATION_);
			start_clnt_auth(priv);
			return;
		}
		else if (pmib->dot11Bss.bsstype == WIFI_ADHOC_STATE)
		{
			clean_for_join(priv);
			OPMODE = WIFI_ADHOC_STATE;
			update_bss(&pmib->dot11StationConfigEntry, &pmib->dot11Bss);
			pmib->dot11RFEntry.dot11channel = pmib->dot11Bss.channel;

			if (pmib->dot11BssType.net_work_type & WIRELESS_11N) {
				if (priv->pmib->dot11nConfigEntry.dot11nUse40M) {
					if (pmib->dot11Bss.t_stamp[1] & BIT(1))
						priv->pshare->is_40m_bw	= 1;
					else
						priv->pshare->is_40m_bw	= 0;

					if (priv->pshare->is_40m_bw) {
						if (pmib->dot11Bss.t_stamp[1] & BIT(2))
							priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_BELOW;
						else
							priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_ABOVE;
					}
					else
						priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_DONTCARE;
				}
				else
					priv->pshare->is_40m_bw	= 0;

				priv->ht_cap_len = 0;
				priv->ht_ie_len = 0;

				priv->pshare->CurrentChannelBW = priv->pshare->is_40m_bw;
				SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);
			}

			SwChnl(priv, pmib->dot11Bss.channel, priv->pshare->offset_2nd_chan);

			DEBUG_INFO("Join IBSS: chan=%d, 40M=%d, offset=%d\n", pmib->dot11Bss.channel,
				priv->pshare->is_40m_bw, priv->pshare->offset_2nd_chan);

			join_bss(priv);
			RTL_W8(_MSR_, _HW_STATE_ADHOC_);
			priv->join_req_ongoing = 0;
			init_beacon(priv);
			priv->join_res = STATE_Sta_Ibss_Active;

			DEBUG_INFO("Join IBSS - %02X:%02X:%02X:%02X:%02X:%02X\n",
				BSSID[0], BSSID[1], BSSID[2], BSSID[3], BSSID[4], BSSID[5]);
			LOG_MSG("Join IBSS - %02X:%02X:%02X:%02X:%02X:%02X\n",
				BSSID[0], BSSID[1], BSSID[2], BSSID[3], BSSID[4], BSSID[5]);
			return;
		}
		else
			return;
	}

	// not found
	if (OPMODE & WIFI_STATION_STATE)
	{
		clean_for_join(priv);
#ifdef UNIVERSAL_REPEATER
		if (IS_ROOT_INTERFACE(priv))
#endif
			RTL_W8(_MSR_, _HW_STATE_NOLINK_);
		priv->join_res = STATE_Sta_No_Bss;
		priv->join_req_ongoing = 0;
		start_clnt_lookup(priv, 1);
		return;
	}
	else if (OPMODE & WIFI_ADHOC_STATE)
	{
		unsigned char tmpbssid[MACADDRLEN];
		int start_period;

		memset(tmpbssid, 0, MACADDRLEN);
		if (!memcmp(BSSID, tmpbssid, MACADDRLEN)) {
			// generate an unique Ibss ssid
			get_random_bytes(&random, 1);
			tmpbssid[0] = 0x02;
			for (i=1; i<MACADDRLEN; i++)
				tmpbssid[i] = GET_MY_HWADDR[i-1] ^ GET_MY_HWADDR[i] ^ random;
			while(1) {
				for (i=0; i<priv->site_survey.count_target; i++) {
					if (!memcmp(tmpbssid, priv->site_survey.bss_target[i].bssid, MACADDRLEN)) {
						tmpbssid[5]++;
						break;
					}
				}
				if (i == priv->site_survey.count)
					break;
			}

			clean_for_join(priv);
			memcpy(BSSID, tmpbssid, MACADDRLEN);
			if (SSID_LEN == 0) {
				SSID_LEN = pmib->dot11StationConfigEntry.dot11DefaultSSIDLen;
				memcpy(SSID, pmib->dot11StationConfigEntry.dot11DefaultSSID, SSID_LEN);
			}

			pmib->dot11Bss.channel = pmib->dot11RFEntry.dot11channel;

			if (pmib->dot11BssType.net_work_type & WIRELESS_11N) {
				priv->pshare->is_40m_bw = priv->pmib->dot11nConfigEntry.dot11nUse40M;
				if (priv->pshare->is_40m_bw)
					priv->pshare->offset_2nd_chan = priv->pmib->dot11nConfigEntry.dot11n2ndChOffset;
				else
					priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_DONTCARE;

				priv->pshare->CurrentChannelBW = priv->pshare->is_40m_bw;
				SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);
			}
			SwChnl(priv, pmib->dot11Bss.channel, priv->pshare->offset_2nd_chan);

			DEBUG_INFO("Start IBSS: chan=%d, 40M=%d, offset=%d\n", pmib->dot11Bss.channel,
				priv->pshare->is_40m_bw, priv->pshare->offset_2nd_chan);
			DEBUG_INFO("Start IBSS - %02X:%02X:%02X:%02X:%02X:%02X\n",
				BSSID[0], BSSID[1], BSSID[2], BSSID[3], BSSID[4], BSSID[5]);

			join_bss(priv);
			RTL_W8(_MSR_, _HW_STATE_ADHOC_);
			priv->beacon_period = pmib->dot11StationConfigEntry.dot11BeaconPeriod;
			priv->join_res = STATE_Sta_Ibss_Idle;
			priv->join_req_ongoing = 0;
			if (priv->auto_channel) {
				priv->auto_channel = 1;
				priv->ss_ssidlen = 0;
				DEBUG_INFO("start_clnt_ss, trigger by %s, ss_ssidlen=0\n", (char *)__FUNCTION__);
				RTL_W8(_MSR_, _HW_STATE_NOLINK_);
				start_clnt_ss(priv);
				return;
			}
			else
				init_beacon(priv);

			priv->join_res = STATE_Sta_Ibss_Idle;
		}
		else {
			pmib->dot11Bss.channel = pmib->dot11RFEntry.dot11channel;

			if (pmib->dot11BssType.net_work_type & WIRELESS_11N) {
				priv->pshare->CurrentChannelBW = priv->pshare->is_40m_bw;
				SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);
			}
			SwChnl(priv, pmib->dot11Bss.channel, priv->pshare->offset_2nd_chan);
			RTL_W8(_MSR_, _HW_STATE_ADHOC_);

			DEBUG_INFO("Start IBSS: chan=%d, 40M=%d, offset=%d\n", pmib->dot11Bss.channel,
				priv->pshare->is_40m_bw, priv->pshare->offset_2nd_chan);
			DEBUG_INFO("Start IBSS - %02X:%02X:%02X:%02X:%02X:%02X\n",
				BSSID[0], BSSID[1], BSSID[2], BSSID[3], BSSID[4], BSSID[5]);
			priv->join_res = STATE_Sta_Ibss_Idle;
		}

		// start for more than scanning period, including random backoff
		start_period = UINT32_DIFF(jiffies, priv->jiffies_pre) / 100 + 1;
		get_random_bytes(&random, 1);
		start_period += (random % 5);
		mod_timer(&priv->idle_timer, jiffies + start_period * 100);

		LOG_MSG("Start IBSS - %02X:%02X:%02X:%02X:%02X:%02X\n",
			BSSID[0], BSSID[1], BSSID[2], BSSID[3], BSSID[4], BSSID[5]);

		return;
	}
	else
		return;
}


static int check_bss_networktype(struct rtl8190_priv * priv, struct bss_desc *bss_target)
{
	int result;

	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
		(priv->pmib->dot11StationConfigEntry.legacySTADeny & WIRELESS_11G) &&
		!(bss_target->network & WIRELESS_11N))
		result = FAIL;
	else if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
		(priv->pmib->dot11StationConfigEntry.legacySTADeny & WIRELESS_11B) &&
		(bss_target->network == WIRELESS_11B))
		result = FAIL;
	else if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) &&
		(priv->pmib->dot11StationConfigEntry.legacySTADeny & WIRELESS_11B) &&
		(bss_target->network == WIRELESS_11B))
		result = FAIL;
	else
		result = SUCCESS;

	if (result == FAIL) {
		DEBUG_ERR("Deny connect to a legacy AP!\n");
	}

	return result;
}


/**
 *	@brief	STA don't how to do
 *	popen:Maybe process client lookup and auth and assoc by IOCTL trigger
 *
 *	[Important]
 *	Exceed Authentication times, process this function.
 *	@param	rescan	: process rescan.
 */
void start_clnt_lookup(struct rtl8190_priv *priv, int rescan)
{
	struct wifi_mib *pmib = GET_MIB(priv);
	unsigned char null_mac[]={0,0,0,0,0,0};
	char tmpbuf[33];
	int i;

	if (rescan || ((priv->site_survey.count_target > 0) &&
		((priv->join_index+1) >= priv->site_survey.count_target)))
	{
		priv->join_res = STATE_Sta_Roaming_Scan;
		if (OPMODE & WIFI_SITE_MONITOR) // if scanning, scan later
			return;

		priv->ss_ssidlen = SSID2SCAN_LEN;
		memcpy(priv->ss_ssid, SSID2SCAN, SSID2SCAN_LEN);
		DEBUG_INFO("start_clnt_ss, trigger by %s, ss_ssidlen=%d, rescan=%d\n", (char *)__FUNCTION__, priv->ss_ssidlen, rescan);
		priv->jiffies_pre = jiffies;
		start_clnt_ss(priv);
		return;
	}

	memset(&pmib->dot11Bss, 0, sizeof(struct bss_desc));
	if (SSID2SCAN_LEN > 0)
	{
		for (i=priv->join_index+1; i<priv->site_survey.count_target; i++)
		{
			// check SSID
			if ((priv->site_survey.bss_target[i].ssidlen == SSID2SCAN_LEN) &&
				(!memcmp(SSID2SCAN, priv->site_survey.bss_target[i].ssid, SSID2SCAN_LEN)))
			{
				// check BSSID
				if (!memcmp(pmib->dot11StationConfigEntry.dot11DesiredBssid, null_mac, MACADDRLEN) ||
					!memcmp(priv->site_survey.bss_target[i].bssid, pmib->dot11StationConfigEntry.dot11DesiredBssid, MACADDRLEN))
				{
					// check BSS type
					if (((OPMODE & WIFI_STATION_STATE) && (priv->site_survey.bss_target[i].bsstype == WIFI_AP_STATE)) ||
						((OPMODE & WIFI_ADHOC_STATE) && (priv->site_survey.bss_target[i].bsstype == WIFI_ADHOC_STATE)))
					{
						// check encryption
						if (((priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm) && (priv->site_survey.bss_target[i].capability&BIT(4))) ||
							((priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm==0) && ((priv->site_survey.bss_target[i].capability&BIT(4))==0)))
						{
							// check network type
							if (check_bss_networktype(priv, &(priv->site_survey.bss_target[i])))
							{
								memcpy(tmpbuf, SSID2SCAN, SSID2SCAN_LEN);
								tmpbuf[SSID2SCAN_LEN] = '\0';
								DEBUG_INFO("found desired bss [%s], start to join\n", tmpbuf);

								memcpy(&pmib->dot11Bss, &(priv->site_survey.bss_target[i]), sizeof(struct bss_desc));
								break;
							}
						}
					}
				}
			}
		}
		priv->join_index = i;
	}
	else
	{
		for (i=priv->join_index+1; i<priv->site_survey.count_target; i++)
		{
			// check BSSID
			if (!memcmp(pmib->dot11StationConfigEntry.dot11DesiredBssid, null_mac, MACADDRLEN) ||
				!memcmp(priv->site_survey.bss_target[i].bssid, pmib->dot11StationConfigEntry.dot11DesiredBssid, MACADDRLEN))
			{
				// check BSS type
				if (((OPMODE & WIFI_STATION_STATE) && (priv->site_survey.bss_target[i].bsstype == WIFI_AP_STATE)) ||
					((OPMODE & WIFI_ADHOC_STATE) && (priv->site_survey.bss_target[i].bsstype == WIFI_ADHOC_STATE)))
				{
					// check encryption
					if (((priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm) && (priv->site_survey.bss_target[i].capability&BIT(4))) ||
						((priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm==0) && ((priv->site_survey.bss_target[i].capability&BIT(4))==0)))
					{
						// check network type
						if (check_bss_networktype(priv, &(priv->site_survey.bss_target[i])))
						{
#ifdef UNIVERSAL_REPEATER
							// if this is vxd interface, and chan of found AP is
							// different with root interface AP, skip it
							if (IS_VXD_INTERFACE(priv) &&
								(GET_ROOT_PRIV(priv)->pmib->dot11RFEntry.dot11channel
									!= priv->site_survey.bss_target[i].channel))
								continue;
#endif
							memcpy(tmpbuf, priv->site_survey.bss_target[i].ssid, priv->site_survey.bss_target[i].ssidlen);
							tmpbuf[priv->site_survey.bss_target[i].ssidlen] = '\0';
							DEBUG_INFO("found desired bss [%s], start to join\n", tmpbuf);

							memcpy(&pmib->dot11Bss, &(priv->site_survey.bss_target[i]), sizeof(struct bss_desc));
							break;
						}
					}
				}
			}
		}
		priv->join_index = i;
	}

	start_clnt_join(priv);
}


static void calculate_rx_beacon(struct rtl8190_priv *priv)
{
	int window_top;
	unsigned int rx_beacon_delta, expect_num, decision_period, rx_data_delta;

	if ((((OPMODE & (WIFI_STATION_STATE | WIFI_ASOC_STATE)) == (WIFI_STATION_STATE | WIFI_ASOC_STATE)) ||
		((OPMODE & WIFI_ADHOC_STATE) &&
				((priv->join_res == STATE_Sta_Ibss_Active) || (priv->join_res == STATE_Sta_Ibss_Idle)))) &&
		!priv->ss_req_ongoing)
	{
		if (OPMODE & WIFI_ADHOC_STATE)
			decision_period = ROAMING_DECISION_PERIOD_ADHOC;
		else
			decision_period = ROAMING_DECISION_PERIOD_INFRA;

		priv->rxBeaconCntArray[priv->rxBeaconCntArrayIdx] = priv->rxBeaconNumInPeriod;
		priv->rxDataCntArray[priv->rxBeaconCntArrayIdx] = priv->rxDataNumInPeriod;
		if (priv->rxBeaconCntArrayWindow < decision_period)
			priv->rxBeaconCntArrayWindow++;
		else
		{
			if (priv->rxBeaconCntArrayIdx == decision_period)
				window_top = 0;
			else
				window_top = priv->rxBeaconCntArrayIdx + 1;

			rx_beacon_delta = UINT32_DIFF(priv->rxBeaconCntArray[priv->rxBeaconCntArrayIdx],
				priv->rxBeaconCntArray[window_top]);

			rx_data_delta = UINT32_DIFF(priv->rxDataCntArray[priv->rxBeaconCntArrayIdx],
				priv->rxDataCntArray[window_top]);

			expect_num = (decision_period * 1000) / priv->beacon_period;
			priv->rxBeaconPercentage = (rx_beacon_delta * 100) / expect_num;

			//DEBUG_INFO("Rx beacon percentage=%d%%, delta=%d, cnt=%d\n", priv->rxBeaconPercentage,
			//	rx_beacon_delta, priv->rxBeaconCntArray[priv->rxBeaconCntArrayIdx]);
#ifdef CLIENT_MODE
			if (OPMODE & WIFI_STATION_STATE)
			{
				// when fast-roaming is enabled, trigger roaming while (david+2006-01-25):
				//	- no any beacon frame received in last one sec (under beacon interval is <= 200ms)
				//  - rx beacon is less than FAST_ROAMING_THRESHOLD
				int offset, fast_roaming_triggered=0;
				if (priv->pmib->dot11StationConfigEntry.fastRoaming) {
					if (priv->beacon_period <= 200) {
						if (priv->rxBeaconCntArrayIdx == 0)
							offset = priv->rxBeaconNumInPeriod - priv->rxBeaconCntArray[decision_period];
						else
							offset = priv->rxBeaconNumInPeriod - priv->rxBeaconCntArray[priv->rxBeaconCntArrayIdx-1];
						if (offset == 0)
							fast_roaming_triggered = 1;
					}
					if (!fast_roaming_triggered && priv->rxBeaconPercentage < FAST_ROAMING_THRESHOLD)
						fast_roaming_triggered = 1;
				}
				if (priv->rxBeaconPercentage < ROAMING_THRESHOLD || fast_roaming_triggered) {
					DEBUG_INFO("Roaming...\n");
					LOG_MSG("Roaming...\n");
#if defined(CONFIG_RTL8196B_TR) || defined(CONFIG_RTL865X_SC) || defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
					LOG_MSG_NOTICE("Roaming...;note:\n");
#endif
					OPMODE &= ~(WIFI_AUTH_SUCCESS | WIFI_ASOC_STATE);
#ifdef UNIVERSAL_REPEATER
					disable_vxd_ap(GET_VXD_PRIV(priv));
#endif

					priv->join_res = STATE_Sta_No_Bss;
					start_clnt_lookup(priv, 1);
				}
			}
			else
			{
				if ((rx_beacon_delta == 0) && (rx_data_delta == 0)) {
					if (priv->join_res == STATE_Sta_Ibss_Active)
					{
						unsigned long ioaddr = priv->pshare->ioaddr;
						DEBUG_INFO("Searching IBSS...\n");
						LOG_MSG("Searching IBSS...\n");
						RTL_W8(_MSR_, _HW_STATE_NOLINK_);
						priv->join_res = STATE_Sta_Ibss_Idle;
						start_clnt_lookup(priv, 1);
					}
				}
			}
#endif // CLIENT_MODE
		}

		if (priv->rxBeaconCntArrayIdx++ == decision_period)
			priv->rxBeaconCntArrayIdx = 0;
	}
}


void rtl8190_reauth_timer(unsigned long task_priv)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)task_priv;
	unsigned long flags;

	SAVE_INT_AND_CLI(flags);

	if (++priv->reauth_count > REAUTH_LIMIT)
	{
		DEBUG_WARN("Client Auth time-out!\n");
		RESTORE_INT(flags);
		start_clnt_lookup(priv, 0);
		return;
	}

	if (OPMODE & WIFI_AUTH_SUCCESS)
	{
		RESTORE_INT(flags);
		return;
	}

	priv->auth_seq = 1;
	OPMODE &= ~(WIFI_AUTH_STATE1);
	OPMODE |= WIFI_AUTH_NULL;

	DEBUG_INFO("auth timeout, sending auth req again\n");
	issue_auth(priv, NULL, 0);

	mod_timer(&priv->reauth_timer, jiffies + REAUTH_TO);

	RESTORE_INT(flags);
}


void rtl8190_reassoc_timer(unsigned long task_priv)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)task_priv;
	unsigned long flags;

	SAVE_INT_AND_CLI(flags);

	if (++priv->reassoc_count > REASSOC_LIMIT)
	{
		DEBUG_WARN("Client Assoc time-out!\n");
		RESTORE_INT(flags);
		start_clnt_lookup(priv, 0);
		return;
	}

	if (OPMODE & WIFI_ASOC_STATE)
	{
		RESTORE_INT(flags);
		return;
	}

	DEBUG_INFO("assoc timeout, sending assoc req again\n");
	issue_assocreq(priv);

	mod_timer(&priv->reassoc_timer, jiffies + REASSOC_TO);

	RESTORE_INT(flags);
}


void rtl8190_idle_timer(unsigned long task_priv)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)task_priv;
	unsigned long ioaddr = priv->pshare->ioaddr;

	RTL_W8(_MSR_, _HW_STATE_NOLINK_);
	LOG_MSG("Searching IBSS...\n");
	start_clnt_lookup(priv, 1);
}


#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
__NOMIPS16
#endif
static unsigned int OnAssocRsp(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned long	ioaddr = priv->pshare->ioaddr;
	unsigned long	flags;
	struct wifi_mib	*pmib;
	struct stat_info *pstat;
	unsigned char	*pframe, *p;
	DOT11_ASSOCIATION_IND	Association_Ind;
	unsigned char	supportRate[32];
	int		supportRateNum;
	UINT16	val;
	int		len;

	if (!(OPMODE & WIFI_STATION_STATE))
		return SUCCESS;

	if (memcmp(GET_MY_HWADDR, pfrinfo->da, MACADDRLEN))
		return SUCCESS;

	if (OPMODE & WIFI_SITE_MONITOR)
		return SUCCESS;

	DEBUG_INFO("got assoc response\n");
	pmib = GET_MIB(priv);
	pframe = get_pframe(pfrinfo);

	// checking status
	val = cpu_to_le16(*(unsigned short*)((unsigned int)pframe + WLAN_HDR_A3_LEN + 2));

	if (val) {
		DEBUG_ERR("assoc reject, status: %d\n", val);
		goto assoc_rejected;
	}

	priv->aid = cpu_to_le16(*(unsigned short*)((unsigned int)pframe + WLAN_HDR_A3_LEN + 4)) & 0x3fff;

	pstat = get_stainfo(priv, pfrinfo->sa);
	if (pstat == NULL) {
		pstat = alloc_stainfo(priv, pfrinfo->sa, -1);
		if (pstat == NULL) {
			DEBUG_ERR("Exceed the upper limit of supported clients...\n");
			goto assoc_rejected;
		}
	}
	else {
		release_stainfo(priv, pstat);
		cnt_assoc_num(priv, pstat, DECREASE, (char *)__FUNCTION__);
		init_stainfo(priv, pstat);
	}

	// Realtek proprietary IE
	p = pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_; len = 0;
	for (;;) {
		p = get_ie(p, _RSN_IE_1_, &len,
		pfrinfo->pktlen - (p - pframe));
		if (p != NULL) {
			if (!memcmp(p+2, Realtek_OUI, 3)) {
				if (*(p+2+3) == 2)
					pstat->is_rtl8190_sta = TRUE;
				else
					pstat->is_rtl8190_sta = FALSE;
				break;
			}
		}
		else
			break;
		p = p + len + 2;
	}


	// get rates
	p = get_ie(pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_, _SUPPORTEDRATES_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _ASOCRSP_IE_OFFSET_);
	if (p == NULL) {
		free_stainfo(priv, pstat);
		return FAIL;
	}
	memcpy(supportRate, p+2, len);
	supportRateNum = len;
	p = get_ie(pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_, _EXT_SUPPORTEDRATES_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _ASOCRSP_IE_OFFSET_);
	if (p !=  NULL) {
		memcpy(supportRate+supportRateNum, p+2, len);
		supportRateNum += len;
	}

	// other capabilities
	memcpy(&val, (pframe + WLAN_HDR_A3_LEN), 2);
	val = le16_to_cpu(val);
	if (val & BIT(5)) {
		// set preamble according to AP
		RTL_W8(RRSR+2, RTL_R8(RRSR+2) | BIT(7));
		pstat->useShortPreamble = 1;
	}
	else {
		// set preamble according to AP
		RTL_W8(RRSR+2, RTL_R8(RRSR+2) & ~BIT(7));
		pstat->useShortPreamble = 0;
	}

	if ((priv->pshare->curr_band == BAND_2G) && (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G))
	{
		if (val & BIT(10)) {
			priv->pmib->dot11ErpInfo.shortSlot = 1;
			set_slot_time(priv, priv->pmib->dot11ErpInfo.shortSlot);
		}
		else {
			priv->pmib->dot11ErpInfo.shortSlot = 0;
			set_slot_time(priv, priv->pmib->dot11ErpInfo.shortSlot);
		}

		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _ERPINFO_IE_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);

		if (p && (*(p+2) & BIT(1)))	// use Protection
			priv->pmib->dot11ErpInfo.protection = 1;
		else
			priv->pmib->dot11ErpInfo.protection = 0;

		if (p && (*(p+2) & BIT(2)))	// use long preamble
			priv->pmib->dot11ErpInfo.longPreambleStaNum = 1;
		else
			priv->pmib->dot11ErpInfo.longPreambleStaNum = 0;
	}

	// set associated and add to association list
	pstat->state |= (WIFI_ASOC_STATE | WIFI_AUTH_SUCCESS);

#ifdef SEMI_QOS  //  WMM STA
	if (QOS_ENABLE) {
		int i;
		p = pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_;
		for (;;) {
			p = get_ie(p, _RSN_IE_1_, &len,
				pfrinfo->pktlen - (p - pframe));
			if (p != NULL) {
				if (!memcmp(p+2, WMM_PARA_IE, 6)) {
					pstat->QosEnabled = 1;
//capture the EDCA para
					p += 10;  // start of EDCA parameters
					for (i = 0; i <4; i++) {
						process_WMM_para_ie(priv, p);  //get the info
						p += 4;
					}
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
					if (IS_ROOT_INTERFACE(priv))
#endif
					{
						SAVE_INT_AND_CLI(flags);
						sta_config_EDCA_para(priv);
						RESTORE_INT(flags);
					}
					break;
				}
			}
			else {
				pstat->QosEnabled = 0;
				break;
			}
			p = p + len + 2;
		}
	}
	else
		pstat->QosEnabled = 0;
#endif

	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) && priv->ht_cap_len)
	{
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_, _HT_CAP_, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _ASOCRSP_IE_OFFSET_);
		if (p !=  NULL) {
			pstat->ht_cap_len = len;
			memcpy((unsigned char *)&pstat->ht_cap_buf, p+2, len);
		}
		else {
			pstat->ht_cap_len = 0;
			memset((unsigned char *)&pstat->ht_cap_buf, 0, sizeof(struct ht_cap_elmt));
		}

		p = get_ie(pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_, _HT_IE_, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _ASOCRSP_IE_OFFSET_);
		if (p !=  NULL) {
			pstat->ht_ie_len = len;
			memcpy((unsigned char *)&pstat->ht_ie_buf, p+2, len);
		}
		else
			pstat->ht_ie_len = 0;

		if (pstat->ht_cap_len) {
			if (cpu_to_le16(pstat->ht_cap_buf.ht_cap_info) & _HTCAP_AMSDU_LEN_8K_) {
				pstat->is_8k_amsdu = 1;
				pstat->amsdu_level = 7935 - sizeof(struct wlan_hdr);
			}
			else {
				pstat->is_8k_amsdu = 0;
				pstat->amsdu_level = 3839 - sizeof(struct wlan_hdr);
			}
		}
	}

#ifdef SEMI_QOS  //  WMM STA
	if (QOS_ENABLE) {
		if ((pstat->QosEnabled == 0) && pstat->ht_cap_len) {
			DEBUG_INFO("AP supports HT but doesn't support WMM, use default WMM value\n");
			pstat->QosEnabled = 1;
			default_WMM_para(priv);
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
			if (IS_ROOT_INTERFACE(priv))
#endif
			{
				SAVE_INT_AND_CLI(flags);
				sta_config_EDCA_para(priv);
				RESTORE_INT(flags);
			}
		}
	}
#endif

	get_matched_rate(priv, supportRate, &supportRateNum, 1);
	update_support_rate(pstat, supportRate, supportRateNum);
	assign_tx_rate(priv, pstat, pfrinfo);
	assign_aggre_mthod(priv, pstat);

	SAVE_INT_AND_CLI(flags);

	pstat->expire_to = priv->expire_to;
	list_add_tail(&pstat->asoc_list, &priv->asoc_list);
	cnt_assoc_num(priv, pstat, INCREASE, (char *)__FUNCTION__);

	if (!IEEE8021X_FUN &&
			!(priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _TKIP_PRIVACY_ ||
			 priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _CCMP_PRIVACY_)) {
#if defined(CONFIG_RTL8196B_TR) || defined(CONFIG_RTL865X_SC) || defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
		LOG_MSG_NOTICE("Connected to AP;note:%02x-%02x-%02x-%02x-%02x-%02x;\n",
				*GetAddr2Ptr(pframe), *(GetAddr2Ptr(pframe)+1), *(GetAddr2Ptr(pframe)+2),
				*(GetAddr2Ptr(pframe+3)), *(GetAddr2Ptr(pframe)+4), *(GetAddr2Ptr(pframe)+5));
#else
	LOG_MSG("Associate to AP successfully - %02X:%02X:%02X:%02X:%02X:%02X\n",
		*GetAddr2Ptr(pframe), *(GetAddr2Ptr(pframe)+1), *(GetAddr2Ptr(pframe)+2),
		*(GetAddr2Ptr(pframe+3)), *(GetAddr2Ptr(pframe)+4), *(GetAddr2Ptr(pframe)+5));
#endif
	}

	// now we have successfully join the give bss...
	if (timer_pending(&priv->reauth_timer))
		del_timer_sync(&priv->reauth_timer);
	if (timer_pending(&priv->reassoc_timer))
		del_timer_sync(&priv->reassoc_timer);

	RESTORE_INT(flags);

	OPMODE |= WIFI_ASOC_STATE;
	update_bss(&priv->pmib->dot11StationConfigEntry, &priv->pmib->dot11Bss);
	priv->pmib->dot11RFEntry.dot11channel = priv->pmib->dot11Bss.channel;
	join_bss(priv);
	priv->join_res = STATE_Sta_Bss;
	priv->join_req_ongoing = 0;

#ifndef WITHOUT_ENQUEUE
	if (priv->pmib->dot118021xAuthEntry.dot118021xAlgrthm
#ifdef WIFI_SIMPLE_CONFIG
		&& !(priv->pmib->wscEntry.wsc_enable)
#endif
		)
	{
		memcpy((void *)Association_Ind.MACAddr, (void *)GetAddr2Ptr(pframe), MACADDRLEN);
		Association_Ind.EventId = DOT11_EVENT_ASSOCIATION_IND;
		Association_Ind.IsMoreEvent = 0;
		Association_Ind.RSNIELen = 0;
		DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&Association_Ind,
					sizeof(DOT11_ASSOCIATION_IND));
		event_indicate(priv, GetAddr2Ptr(pframe), 1);
	}
#endif // WITHOUT_ENQUEUE

#ifdef WIFI_SIMPLE_CONFIG
	if (priv->pmib->wscEntry.wsc_enable) {
		DOT11_WSC_ASSOC_IND wsc_Association_Ind;

		memset(&wsc_Association_Ind, 0, sizeof(DOT11_WSC_ASSOC_IND));
		wsc_Association_Ind.EventId = DOT11_EVENT_WSC_ASSOC_REQ_IE_IND;
		memcpy((void *)wsc_Association_Ind.MACAddr, (void *)GetAddr2Ptr(pframe), MACADDRLEN);
		DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&wsc_Association_Ind,
			sizeof(DOT11_WSC_ASSOC_IND));
		event_indicate(priv, GetAddr2Ptr(pframe), 1);
		pstat->state |= WIFI_WPS_JOIN;
	}
#endif


	DEBUG_INFO("assoc successful!\n");

#ifdef UNIVERSAL_REPEATER
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
		if ((pstat->ht_cap_len > 0) && (pstat->ht_ie_len > 0) &&
				(pstat->ht_ie_buf.info0 & _HTIE_STA_CH_WDTH_) &&
		(pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_))) {
			priv->pshare->is_40m_bw = 1;
			if ((pstat->ht_ie_buf.info0 & _HTIE_2NDCH_OFFSET_BL_) == _HTIE_2NDCH_OFFSET_BL_)
				priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_BELOW;
			else
				priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_ABOVE;

			priv->pshare->CurrentChannelBW = priv->pshare->is_40m_bw;
			SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);
			SwChnl(priv, priv->pmib->dot11Bss.channel, priv->pshare->offset_2nd_chan);

			DEBUG_INFO("%s: set chan=%d, 40M=%d, offset_2nd_chan=%d\n",
				__FUNCTION__,
				priv->pmib->dot11Bss.channel,
				priv->pshare->is_40m_bw,  priv->pshare->offset_2nd_chan);

		}
		else {
			priv->pshare->is_40m_bw = 0;
			priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_DONTCARE;
		}
	}

	if (pstat->ht_cap_len) {
		if (pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_))
			pstat->tx_bw = HT_CHANNEL_WIDTH_20_40;
		else
			pstat->tx_bw = HT_CHANNEL_WIDTH_20;
	}

#ifdef UNIVERSAL_REPEATER
	if (IS_ROOT_INTERFACE(priv)) {
#ifdef RTK_BR_EXT
		if (!(priv->pmib->ethBrExtInfo.macclone_enable && !priv->macclone_completed))
#endif
		{
			if (netif_running(GET_VXD_PRIV(priv)->dev))
				enable_vxd_ap(GET_VXD_PRIV(priv));
		}
	}
#endif

#ifndef USE_WEP_DEFAULT_KEY
	set_keymapping_wep(priv, pstat);
#endif

	add_update_RATid(priv, pstat);

	return SUCCESS;

assoc_rejected:

	priv->join_res = STATE_Sta_No_Bss;
	priv->join_req_ongoing = 0;

	if (timer_pending(&priv->reassoc_timer))
		del_timer_sync (&priv->reassoc_timer);

	start_clnt_lookup(priv, 0);

#ifdef UNIVERSAL_REPEATER
	disable_vxd_ap(GET_VXD_PRIV(priv));
#endif

	return FAIL;
}


/**
 *	@brief	STA in Inferstructure mode Beacon process.
 */
static unsigned int OnBeaconClnt_Bss(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned char *bssid;
	struct stat_info *pstat;
	unsigned char *p, *pframe;
	int len;
	unsigned short val16;

	pframe = get_pframe(pfrinfo);
	bssid = GetAddr3Ptr(pframe);

	if (!IS_BSSID(priv, bssid))
		return SUCCESS;

	memcpy(&val16, (pframe + WLAN_HDR_A3_LEN + 8 + 2), 2);
	val16 = le16_to_cpu(val16);
	if (!(val16 & BIT(0)) || (val16 & BIT(1)))
		return SUCCESS;

	// this is our AP
	pstat = get_stainfo(priv, bssid);
	if (pstat == NULL) {
		DEBUG_ERR("Can't find our AP\n");
		return FAIL;
	}

	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _DSSET_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	if (p != NULL) {
		if (priv->pmib->dot11Bss.channel	== *(p+2)) {
			priv->rxBeaconNumInPeriod++;
		}
	}

	if (val16 & BIT(5))
		pstat->useShortPreamble = 1;
	else
		pstat->useShortPreamble = 0;

	if ((priv->pshare->curr_band == BAND_2G) && (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G))
	{
		if (val16 & BIT(10)) {
			if (priv->pmib->dot11ErpInfo.shortSlot == 0) {
				priv->pmib->dot11ErpInfo.shortSlot = 1;
				set_slot_time(priv, priv->pmib->dot11ErpInfo.shortSlot);
			}
		}
		else {
			if (priv->pmib->dot11ErpInfo.shortSlot == 1) {
				priv->pmib->dot11ErpInfo.shortSlot = 0;
				set_slot_time(priv, priv->pmib->dot11ErpInfo.shortSlot);
			}
		}

		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _ERPINFO_IE_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);

		if (p && (*(p+2) & BIT(1)))	// use Protection
			priv->pmib->dot11ErpInfo.protection = 1;
		else
			priv->pmib->dot11ErpInfo.protection = 0;

		if (p && (*(p+2) & BIT(2)))	// use long preamble
			priv->pmib->dot11ErpInfo.longPreambleStaNum = 1;
		else
			priv->pmib->dot11ErpInfo.longPreambleStaNum = 0;
	}

	// Realtek proprietary IE
	p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_; len = 0;
	for (;;)
	{
		p = get_ie(p, _RSN_IE_1_, &len,
			pfrinfo->pktlen - (p - pframe));
		if (p != NULL) {
			if (!memcmp(p+2, Realtek_OUI, 3)) {
				if (*(p+2+3) == 2)
					pstat->is_rtl8190_sta = TRUE;
				else
					pstat->is_rtl8190_sta = FALSE;
				break;
			}
		}
		else
			break;

		p = p + len + 2;
	}

	// Customer proprietary IE
	if (priv->pmib->miscEntry.private_ie_len) {
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, priv->pmib->miscEntry.private_ie[0], &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p) {
			memcpy(pstat->private_ie, p, len + 2);
			pstat->private_ie_len = len + 2;
		}
	}

	return SUCCESS;
}


/**
 *	@brief	STA in ad hoc mode Beacon process.
 */
static unsigned int OnBeaconClnt_Ibss(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned char *bssid, *bdsa;
	struct stat_info *pstat;
	unsigned char *p, *pframe, channel;
	int len;
	unsigned char supportRate[32];
	int supportRateNum;
	unsigned short val16;
	unsigned long flags;

	pframe = get_pframe(pfrinfo);

	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _DSSET_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	if (p != NULL)
		channel = *(p+2);
	else
		channel = priv->pmib->dot11RFEntry.dot11channel;

	/*
	 * check if OLBC exist
	 */
	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) &&
		(channel == priv->pmib->dot11RFEntry.dot11channel))
	{
		// look for ERP rate. if no ERP rate existed, thought it is a legacy AP
		unsigned char supportedRates[32];
		int supplen=0, legacy=1, i;

		pframe = get_pframe(pfrinfo);
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _SUPPORTEDRATES_IE_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p) {
			if (len>8)
				len=8;
			memcpy(&supportedRates[supplen], p+2, len);
			supplen += len;
		}

		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _EXT_SUPPORTEDRATES_IE_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p) {
			if (len>8)
				len=8;
			memcpy(&supportedRates[supplen], p+2, len);
			supplen += len;
		}

		for (i=0; i<supplen; i++) {
			if (!is_CCK_rate(supportedRates[i]&0x7f)) {
				legacy = 0;
				break;
			}
		}

		// look for ERP IE and check non ERP present
		if (legacy == 0) {
			p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _ERPINFO_IE_, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
			if (p && (*(p+2) & BIT(0)))
				legacy = 1;
		}

		if (legacy) {
			if (!priv->pmib->dot11StationConfigEntry.olbcDetectDisabled &&
							priv->pmib->dot11ErpInfo.olbcDetected==0) {
				priv->pmib->dot11ErpInfo.olbcDetected = 1;
				check_protection_shortslot(priv);
				DEBUG_INFO("OLBC detected\n");
			}
			if (priv->pmib->dot11ErpInfo.olbcDetected)
				priv->pmib->dot11ErpInfo.olbcExpired = DEFAULT_OLBC_EXPIRE;
		}
	}

	/*
	 * add into sta table and calculate beacon
	 */
	bssid = GetAddr3Ptr(pframe);
	bdsa = GetAddr2Ptr(pframe);

	if (!IS_BSSID(priv, bssid))
		return SUCCESS;

	memcpy(&val16, (pframe + WLAN_HDR_A3_LEN + 8 + 2), 2);
	val16 = le16_to_cpu(val16);
	if ((val16 & BIT(0)) || !(val16 & BIT(1)))
		return SUCCESS;

	// this is our peers
	pstat = get_stainfo(priv, bdsa);

	if (pstat == NULL) {
		DEBUG_INFO("Add IBSS sta, %02x:%02x:%02x:%02x:%02x:%02x!\n",
			bdsa[0],bdsa[1], bdsa[2],bdsa[3],bdsa[4],bdsa[5]);

		pstat = alloc_stainfo(priv, bdsa, -1);
		if (pstat == NULL)
			return SUCCESS;

		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _SUPPORTEDRATES_IE_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p == NULL) {
			free_stainfo(priv, pstat);
			return SUCCESS;
		}
		memcpy(supportRate, p+2, len);
		supportRateNum = len;
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _EXT_SUPPORTEDRATES_IE_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p !=  NULL) {
			memcpy(supportRate+supportRateNum, p+2, len);
			supportRateNum += len;
		}

#ifdef SEMI_QOS
		// check if there is WMM IE
		if (QOS_ENABLE) {
			p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_; len = 0;
			for (;;) {
				p = get_ie(p, _RSN_IE_1_, &len,
					pfrinfo->pktlen - (p - pframe));
				if (p != NULL) {
					if (!memcmp(p+2, WMM_IE, 6)) {
						pstat->QosEnabled = 1;
#ifdef WMM_APSD
						if (APSD_ENABLE)
							pstat->apsd_bitmap = *(p+8) & 0x0f;		// get QSTA APSD bitmap
#endif
						break;
					}
				}
				else {
					pstat->QosEnabled = 0;
#ifdef WMM_APSD
					pstat->apsd_bitmap = 0;
#endif
					break;
				}
				p = p + len + 2;
			}
		}
		else {
			pstat->QosEnabled = 0;
#ifdef WMM_APSD
			pstat->apsd_bitmap = 0;
#endif
		}
#endif

		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
			p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_CAP_, &len,
					pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
			if (p !=  NULL) {
				unsigned char mimo_ps;
				pstat->ht_cap_len = len;
				memcpy((unsigned char *)&pstat->ht_cap_buf, p+2, len);
				// below is the process to check HT MIMO power save
				mimo_ps = ((cpu_to_le16(pstat->ht_cap_buf.ht_cap_info)) >> 2)&0x0003;
				pstat->MIMO_ps = 0;
				if (!mimo_ps)
					pstat->MIMO_ps |= _HT_MIMO_PS_STATIC_;
				else if (mimo_ps == 1)
					pstat->MIMO_ps |= _HT_MIMO_PS_DYNAMIC_;
				if (cpu_to_le16(pstat->ht_cap_buf.ht_cap_info) & _HTCAP_AMSDU_LEN_8K_) {
					pstat->is_8k_amsdu = 1;
					pstat->amsdu_level = 7935 - sizeof(struct wlan_hdr);
 				}
				else {
					pstat->is_8k_amsdu = 0;
					pstat->amsdu_level = 3839 - sizeof(struct wlan_hdr);
				}
				if (pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_))
					pstat->tx_bw = HT_CHANNEL_WIDTH_20_40;
				else
					pstat->tx_bw = HT_CHANNEL_WIDTH_20;
			}
			else {
				pstat->ht_cap_len = 0;
				memset((unsigned char *)&pstat->ht_cap_buf, 0, sizeof(struct ht_cap_elmt));
			}

			p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_IE_, &len,
					pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
			if (p !=  NULL) {
				pstat->ht_ie_len = len;
				memcpy((unsigned char *)&pstat->ht_ie_buf, p+2, len);
			}
			else
				pstat->ht_ie_len = 0;
		}

		// Realtek proprietary IE
		p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_; len = 0;
		for (;;)
		{
			p = get_ie(p, _RSN_IE_1_, &len,
								pfrinfo->pktlen - (p - pframe));
			if (p != NULL) {
				if (!memcmp(p+2, Realtek_OUI, 3)) {
					if (*(p+2+3) == 2)
						pstat->is_rtl8190_sta = TRUE;
					else
						pstat->is_rtl8190_sta = FALSE;
					break;
				}
			}
			else
				break;

			p = p + len + 2;
		}

		if ((priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _TKIP_PRIVACY_) ||
			(priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _CCMP_PRIVACY_) ||
			(priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_) ||
			(priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_) ) {
			DOT11_SET_KEY Set_Key;
			memcpy(Set_Key.MACAddr, pstat->hwaddr, 6);
			Set_Key.KeyType = DOT11_KeyType_Pairwise;
			if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_ ||
					priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_) {
				Set_Key.EncType = priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm;
				Set_Key.KeyIndex = priv->pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex;
				DOT11_Process_Set_Key(priv->dev, NULL, &Set_Key,
				priv->pmib->dot11DefaultKeysTable.keytype[Set_Key.KeyIndex].skey);
			}
			else {
				Set_Key.EncType = (unsigned char)priv->pmib->dot11GroupKeysTable.dot11Privacy;
				Set_Key.KeyIndex = priv->pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex;
				DOT11_Process_Set_Key(priv->dev, NULL, &Set_Key,
				priv->pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TMicKey1.skey);
			}
		}


		get_matched_rate(priv, supportRate, &supportRateNum, 0);
		update_support_rate(pstat, supportRate, supportRateNum);

		assign_tx_rate(priv, pstat, pfrinfo);
		assign_aggre_mthod(priv, pstat);

		val16 = cpu_to_le16(*(unsigned short*)((unsigned int)pframe + WLAN_HDR_A3_LEN + 8 + 2));
		if (!(val16 & BIT(5))) // NOT use short preamble
			pstat->useShortPreamble = 0;
		else
			pstat->useShortPreamble = 1;

		pstat->state |= (WIFI_ASOC_STATE | WIFI_AUTH_SUCCESS);

		SAVE_INT_AND_CLI(flags);
		pstat->expire_to = priv->expire_to;
		list_add_tail(&pstat->asoc_list, &priv->asoc_list);
		cnt_assoc_num(priv, pstat, INCREASE, (char *)__FUNCTION__);
		check_sta_characteristic(priv, pstat, INCREASE);
		RESTORE_INT(flags);

		LOG_MSG("An IBSS client is detected - %02X:%02X:%02X:%02X:%02X:%02X\n",
			*GetAddr2Ptr(pframe), *(GetAddr2Ptr(pframe)+1), *(GetAddr2Ptr(pframe)+2),
			*(GetAddr2Ptr(pframe+3)), *(GetAddr2Ptr(pframe)+4), *(GetAddr2Ptr(pframe)+5));

		add_update_RATid(priv, pstat);
	}

	if (timer_pending(&priv->idle_timer))
		del_timer_sync(&priv->idle_timer);

	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _DSSET_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	if (p != NULL) {
		if (priv->pmib->dot11Bss.channel	== *(p+2)) {
			pstat->beacon_num++;
			priv->rxBeaconNumInPeriod++;
			priv->join_res = STATE_Sta_Ibss_Active;
		}
	}
	return SUCCESS;
}


/**
 *	@brief	STA recived Beacon process
 */
static unsigned int OnBeaconClnt(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo)
{
	int ret = SUCCESS;

	// Site survey and collect information
	if (OPMODE & WIFI_SITE_MONITOR) {
		collect_bss_info(priv, pfrinfo);
		return SUCCESS;
	}

	// Infra client mode, check beacon info
	if ((OPMODE & (WIFI_STATION_STATE | WIFI_ASOC_STATE)) ==
		(WIFI_STATION_STATE | WIFI_ASOC_STATE))
		ret = OnBeaconClnt_Bss(priv, pfrinfo);

	// Ad-hoc client mode, check peer's beacon
	if ((OPMODE & WIFI_ADHOC_STATE) &&
		((priv->join_res == STATE_Sta_Ibss_Active) || (priv->join_res == STATE_Sta_Ibss_Idle)))
		ret = OnBeaconClnt_Ibss(priv, pfrinfo);

	return ret;
}


/**
 *	@brief	STA recived ATIM
 *
 *	STA only.
 */
static unsigned int OnATIM(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo)
{
	return SUCCESS;
}


static unsigned int OnDisassocClnt(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned char *pframe = get_pframe(pfrinfo);
	unsigned char *bssid = GetAddr3Ptr(pframe);
	unsigned short val16;

	if (!(OPMODE & WIFI_STATION_STATE))
		return SUCCESS;

	if (memcmp(GET_MY_HWADDR, pfrinfo->da, MACADDRLEN))
		return SUCCESS;

	if (!memcmp(BSSID, bssid, MACADDRLEN)) {
		memcpy(&val16, (pframe + WLAN_HDR_A3_LEN), 2);
		DEBUG_INFO("recv Disassociation, reason: %d\n", le16_to_cpu(val16));

		OPMODE &= ~(WIFI_AUTH_SUCCESS);
		priv->join_res = STATE_Sta_No_Bss;

		start_clnt_auth(priv);

#if defined(CONFIG_RTL8196B_TR) || defined(CONFIG_RTL865X_SC) || defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
		LOG_MSG_NOTICE("Disassociated by AP;note:%02x-%02x-%02x-%02x-%02x-%02x;\n",
				*GetAddr2Ptr(pframe), *(GetAddr2Ptr(pframe)+1), *(GetAddr2Ptr(pframe)+2),
				*(GetAddr2Ptr(pframe+3)), *(GetAddr2Ptr(pframe)+4), *(GetAddr2Ptr(pframe)+5));
#else
		LOG_MSG("Disassociated by AP - %02X:%02X:%02X:%02X:%02X:%02X\n",
			*GetAddr2Ptr(pframe), *(GetAddr2Ptr(pframe)+1), *(GetAddr2Ptr(pframe)+2),
			*(GetAddr2Ptr(pframe+3)), *(GetAddr2Ptr(pframe)+4), *(GetAddr2Ptr(pframe)+5));
#endif

#ifdef UNIVERSAL_REPEATER
		disable_vxd_ap(GET_VXD_PRIV(priv));
#endif
	}

	return SUCCESS;
}


/**
 *	@brief	STA recived authentication
 *	AP and STA authentication each other.
 */
static unsigned int OnAuthClnt(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned int	privacy, seq, len, status, algthm, offset, go2asoc=0;
	unsigned long	flags;
	struct wifi_mib	*pmib;
	unsigned char	*pframe, *p;

	if (!(OPMODE & WIFI_STATION_STATE))
		return SUCCESS;

	if (memcmp(GET_MY_HWADDR, pfrinfo->da, MACADDRLEN))
		return SUCCESS;

	if (OPMODE & WIFI_SITE_MONITOR)
		return SUCCESS;

	DEBUG_INFO("got auth response\n");
	pmib = GET_MIB(priv);
	pframe = get_pframe(pfrinfo);

	privacy = priv->pmib->dot1180211AuthEntry.dot11AuthAlgrthm;

	if (GetPrivacy(pframe))
		offset = 4;
	else
		offset = 0;

	algthm 	= cpu_to_le16(*(unsigned short *)((unsigned int)pframe + WLAN_HDR_A3_LEN + offset));
	seq 	= cpu_to_le16(*(unsigned short *)((unsigned int)pframe + WLAN_HDR_A3_LEN + offset + 2));
	status 	= cpu_to_le16(*(unsigned short *)((unsigned int)pframe + WLAN_HDR_A3_LEN + offset + 4));

	if (status != 0)
	{
		DEBUG_ERR("clnt auth fail, status: %d\n", status);
		goto authclnt_err_end;
	}

	if (seq == 2)
	{
#ifdef WIFI_SIMPLE_CONFIG
		if (pmib->wscEntry.wsc_enable && algthm == 0)
			privacy = 0;
#endif

		if ((privacy == 1) || // legacy shared system
			((privacy == 2) && (priv->authModeToggle) && // auto and use shared-key currently
			 (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_ ||
			  priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_)))
		{
			p = get_ie(pframe + WLAN_HDR_A3_LEN + _AUTH_IE_OFFSET_, _CHLGETXT_IE_, (int *)&len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _AUTH_IE_OFFSET_);

			if (p == NULL) {
				DEBUG_ERR("no challenge text?\n");
				goto authclnt_fail;
			}

			DEBUG_INFO("auth chlgetxt len =%d\n", len);
			memcpy((void *)priv->chg_txt, (void *)(p+2), len);
			SAVE_INT_AND_CLI(flags);
			priv->auth_seq = 3;
			OPMODE &= (~ WIFI_AUTH_NULL);
			OPMODE |= (WIFI_AUTH_STATE1);
			RESTORE_INT(flags);
			issue_auth(priv, NULL, 0);
			return SUCCESS;
		}
		else // open system
			go2asoc = 1;
	}
	else if (seq == 4)
	{
		if (privacy)
			go2asoc = 1;
		else
		{
			// this is illegal
			DEBUG_ERR("no privacy but auth seq=4?\n");
			goto authclnt_fail;
		}
	}
	else
	{
		// this is also illegal
		DEBUG_ERR("clnt auth failed due to illegal seq=%x\n", seq);
		goto authclnt_fail;
	}

	if (go2asoc)
	{
		DEBUG_INFO("auth successful!\n");
			start_clnt_assoc(priv);
		return SUCCESS;
	}

authclnt_fail:

	if ((++priv->reauth_count) < REAUTH_LIMIT)
		return FAIL;

authclnt_err_end:

	if ((priv->pmib->dot1180211AuthEntry.dot11AuthAlgrthm == 2) &&
		((priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_) ||
		 (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_)) &&
		(priv->authModeRetry == 0)) {
		// auto-auth mode, retry another auth method
		priv->authModeRetry++;

		start_clnt_auth(priv);
		return SUCCESS;
	}
	else {
		priv->join_res = STATE_Sta_No_Bss;
		priv->join_req_ongoing = 0;

		if (timer_pending(&priv->reauth_timer))
			del_timer_sync (&priv->reauth_timer);

		start_clnt_lookup(priv, 0);
		return FAIL;
	}
}


/**
 *	@brief	Client/STA De authentication
 *	First DeAuthClnt, Second OnDeAuthClnt
 */
static unsigned int OnDeAuthClnt(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned long flags, link_time;
	unsigned char *pframe = get_pframe(pfrinfo);
	unsigned char *bssid = GetAddr3Ptr(pframe);
	struct stat_info *pstat;
	unsigned short val16;

	if (!(OPMODE & WIFI_STATION_STATE))
		return SUCCESS;

	if (memcmp(GET_MY_HWADDR, pfrinfo->da, MACADDRLEN))
		return SUCCESS;

	if (!memcmp(priv->pmib->dot11Bss.bssid, bssid, MACADDRLEN)) {
		memcpy(&val16, (pframe + WLAN_HDR_A3_LEN), 2);
		DEBUG_INFO("recv Deauthentication, reason: %d\n", le16_to_cpu(val16));

		pstat = get_stainfo(priv, bssid);
		if (pstat == NULL) { // how come?
// Start scan again ----------------------
//			return FAIL;
			link_time = 0; // get next bss info
			goto do_scan;
//--------------------- david+2007-03-10
		}

		link_time = pstat->link_time;

		SAVE_INT_AND_CLI(flags);
		if (!list_empty(&pstat->asoc_list)) {
			list_del_init(&pstat->asoc_list);
			cnt_assoc_num(priv, pstat, DECREASE, (char *)__FUNCTION__);
		}
		RESTORE_INT(flags);

		free_stainfo(priv, pstat);
do_scan:
		OPMODE &= ~(WIFI_AUTH_SUCCESS | WIFI_ASOC_STATE);
		priv->join_res = STATE_Sta_No_Bss;

// Delete timer --------------------------------------
		if (timer_pending(&priv->reauth_timer))
			del_timer_sync (&priv->reauth_timer);

		if (timer_pending(&priv->reassoc_timer))
			del_timer_sync (&priv->reassoc_timer);
//---------------------------------- david+2007-03-10

		if (link_time > priv->expire_to)	// if link time exceeds timeout, site survey again
			start_clnt_lookup(priv, 1);
		else
			start_clnt_lookup(priv, 0);

#if defined(CONFIG_RTL8196B_TR) || defined(CONFIG_RTL865X_SC) || defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
		LOG_MSG_NOTICE("Deauthenticated by AP;note:%02x-%02x-%02x-%02x-%02x-%02x;\n",
				*GetAddr2Ptr(pframe), *(GetAddr2Ptr(pframe)+1), *(GetAddr2Ptr(pframe)+2),
				*(GetAddr2Ptr(pframe+3)), *(GetAddr2Ptr(pframe)+4), *(GetAddr2Ptr(pframe)+5));
#else
		LOG_MSG("Deauthenticated by AP - %02X:%02X:%02X:%02X:%02X:%02X\n",
			*GetAddr2Ptr(pframe), *(GetAddr2Ptr(pframe)+1), *(GetAddr2Ptr(pframe)+2),
			*(GetAddr2Ptr(pframe+3)), *(GetAddr2Ptr(pframe)+4), *(GetAddr2Ptr(pframe)+5));
#endif

#ifdef UNIVERSAL_REPEATER
		disable_vxd_ap(GET_VXD_PRIV(priv));
#endif
	}

	return SUCCESS;
}
#endif // CLIENT_MODE


// A dedicated function to check link status
int chklink_wkstaQ(struct rtl8190_priv *priv)
{
	int link_status=0;

	if (OPMODE & WIFI_AP_STATE)
	{
		if (priv->assoc_num > 0)
			link_status = 1;
		else
			link_status = 0;
	}
#ifdef CLIENT_MODE
	else if (OPMODE & WIFI_STATION_STATE)
	{
		if (OPMODE & WIFI_ASOC_STATE)
			link_status = 1;
		else
			link_status = 0;
	}
	else if ((OPMODE & WIFI_ADHOC_STATE) &&
		((priv->join_res == STATE_Sta_Ibss_Active) || (priv->join_res == STATE_Sta_Ibss_Idle)))
	{
		if (priv->rxBeaconCntArrayWindow < ROAMING_DECISION_PERIOD_ADHOC) {
			if (priv->rxBeaconCntArrayWindow) {
				if (priv->rxBeaconCntArray[priv->rxBeaconCntArrayIdx-1] > 0) {
					link_status = 1;
				}
			}
		}
		else {
			if (priv->rxBeaconPercentage)
				link_status = 1;
			else
				link_status = 0;
		}
	}
#endif
	else
	{
		link_status = 0;
	}

	return link_status;
}


// for SW LED ----------------------------------------------------
#ifdef RTL8190_SWGPIO_LED
static void set_swGpio_LED(struct rtl8190_priv *priv, unsigned int ledNum, int flag)
{
	unsigned long ioaddr = priv->pshare->ioaddr;
	unsigned int ledItem;	/* parameter to decode GPIO item */

	if (ledNum >= SWLED_GPIORT_CNT)
		return;

	ledItem = SWLED_GPIORT_ITEM(LED_ROUTE, ledNum);

	if (ledItem & SWLED_GPIORT_ENABLEMSK)
	{
		/* get the corresponding information (GPIO number/Active high or low) of LED */
		int gpio;
		int activeMode;	/* !=0 : Active High, ==0 : Active Low */

		gpio = ledItem & SWLED_GPIORT_RTBITMSK;
		activeMode = ledItem & SWLED_GPIORT_HLMSK;

		if (flag) {	/* Turn ON LED */
			if (activeMode)	/* Active High */
				RTL_W8(0x90, RTL_R8(0x90) | BIT(gpio));
			else			/* Active Low */
				RTL_W8(0x90, RTL_R8(0x90) &~ BIT(gpio));
		}
		else {	/* Turn OFF LED */
			if (activeMode)	/* Active High */
				RTL_W8(0x90, RTL_R8(0x90) &~ BIT(gpio));
			else			/* Active Low */
				RTL_W8(0x90, RTL_R8(0x90) | BIT(gpio));
		}
	}
}
#endif // RTL8190_SWGPIO_LED


static void set_sw_LED0(struct rtl8190_priv *priv, int flag)
{
#ifdef RTL8190_SWGPIO_LED
	if (LED_ROUTE)
		set_swGpio_LED(priv, 0, flag);
#else

#if defined(RTL8192SU) && defined(SW_LED)	//cathy, only LED0, low active
			if (flag)
				RTL_W8SU2(priv, LEDCFG, 0x88);
			else
				RTL_W8SU2(priv, LEDCFG, 0x80);
#else

	if (flag)
		RTL_W8(LEDCFG, RTL_R8(LEDCFG) | 0x0f);
	else
		RTL_W8(LEDCFG, RTL_R8(LEDCFG) & 0x00);
#endif	
#endif
}


static void set_sw_LED1(struct rtl8190_priv *priv, int flag)
{
		return;
#ifdef RTL8190_SWGPIO_LED
	if (LED_ROUTE)
		set_swGpio_LED(priv, 1, flag);
#else

//	if (flag)
//		RTL_W8(LEDCFG, RTL_R8(LEDCFG) | 0xf0);
//	else
//		RTL_W8(LEDCFG, RTL_R8(LEDCFG) & 0x0f);
#endif
}


static void LED_Interval_timeout(unsigned long task_priv)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)task_priv;

	if ((LED_TYPE == LEDTYPE_SW_LINKTXRX) ||
		(LED_TYPE == LEDTYPE_SW_LINKTXRXDATA) ||
		(LED_TYPE == LEDTYPE_SW_ENABLETXRXDATA) ||
		((LED_TYPE == LEDTYPE_SW_ADATA_GDATA) && (priv->pshare->curr_band == BAND_5G))) {
		if (!priv->pshare->set_led_in_progress)
			set_sw_LED0(priv, priv->pshare->LED_Toggle);
	}
	else {
		if (!priv->pshare->set_led_in_progress)
			set_sw_LED1(priv, priv->pshare->LED_Toggle);
	}

	if ( priv->pshare->LED_Toggle == priv->pshare->LED_ToggleStart )
		mod_timer(&priv->pshare->LED_Timer, jiffies + priv->pshare->LED_Interval);
	else {
		if 	(LED_TYPE == LEDTYPE_SW_CUSTOM1)
			mod_timer(&priv->pshare->LED_Timer, jiffies + priv->pshare->LED_Interval);
		else
			mod_timer(&priv->pshare->LED_Timer, jiffies + LED_ON_TIME);
	}

	priv->pshare->LED_Toggle = (priv->pshare->LED_Toggle + 1) % 2;
}


void enable_sw_LED(struct rtl8190_priv *priv, int init)
{

	// configure mac to use SW LED
#if defined(RTL8192SU) && defined(SW_LED)
	RTL_W8SU2(priv, LEDCFG, 0x88);
#else
	RTL_W8(LEDCFG, 0x88);
#endif
	priv->pshare->LED_Interval = LED_INTERVAL_TIME;
	priv->pshare->LED_Toggle = 0;
	priv->pshare->LED_ToggleStart = LED_OFF;
	priv->pshare->LED_tx_cnt_log = 0;
	priv->pshare->LED_rx_cnt_log = 0;
	priv->pshare->LED_tx_cnt = 0;
	priv->pshare->LED_rx_cnt = 0;

	if ((LED_TYPE == LEDTYPE_SW_ENABLE_TXRXDATA) ||
		(LED_TYPE == LEDTYPE_SW_ENABLETXRXDATA)) {
		set_sw_LED0(priv, LED_ON);
		set_sw_LED1(priv, LED_OFF);
		if (LED_TYPE == LEDTYPE_SW_ENABLETXRXDATA)
			priv->pshare->LED_ToggleStart = LED_ON;
	}
	else if (LED_TYPE == LEDTYPE_SW_ADATA_GDATA) {
		priv->pshare->LED_ToggleStart = LED_ON;
		if (priv->pshare->curr_band == BAND_5G) {
			set_sw_LED0(priv, LED_ON);
			set_sw_LED1(priv, LED_OFF);
		}
		else {	// 11G
			set_sw_LED0(priv, LED_OFF);
			set_sw_LED1(priv, LED_ON);
		}
	}
	else if (LED_TYPE == LEDTYPE_SW_ENABLETXRXDATA_1) {
		set_sw_LED0(priv, LED_OFF);
		set_sw_LED1(priv, LED_ON);
		priv->pshare->LED_ToggleStart = LED_ON;
	}
	else {
		set_sw_LED0(priv, LED_OFF);
		set_sw_LED1(priv, LED_OFF);
	}

	if (init) {
		init_timer(&priv->pshare->LED_Timer);
		priv->pshare->LED_sw_use = 1;
		priv->pshare->LED_Timer.expires = jiffies + priv->pshare->LED_Interval;
		priv->pshare->LED_Timer.data = (unsigned long) priv;
		priv->pshare->LED_Timer.function = &LED_Interval_timeout;
		mod_timer(&priv->pshare->LED_Timer, jiffies + priv->pshare->LED_Interval);
	}
}


void disable_sw_LED(struct rtl8190_priv *priv)
{
	set_sw_LED0(priv, LED_OFF);
	set_sw_LED1(priv, LED_OFF);

	del_timer_sync(&priv->pshare->LED_Timer);
	priv->pshare->LED_sw_use = 0;
}


void calculate_sw_LED_interval(struct rtl8190_priv *priv)
{
	unsigned int delta = 0;
	int i, scale_num=0;

	if (priv->pshare->set_led_in_progress)
		return;

	// calculate counter delta
	delta += UINT32_DIFF(priv->pshare->LED_tx_cnt, priv->pshare->LED_tx_cnt_log);
	delta += UINT32_DIFF(priv->pshare->LED_rx_cnt, priv->pshare->LED_rx_cnt_log);
	priv->pshare->LED_tx_cnt_log = priv->pshare->LED_tx_cnt;
	priv->pshare->LED_rx_cnt_log = priv->pshare->LED_rx_cnt;

	// update interval according to delta
	if (delta == 0) {
		if (LED_TYPE == LEDTYPE_SW_CUSTOM1) {
			if (priv->pshare->LED_Interval != 100) {
				priv->pshare->LED_Interval = 100;
				mod_timer(&priv->pshare->LED_Timer, jiffies + priv->pshare->LED_Interval);
			}
		}
		else {
			if (priv->pshare->LED_Interval == LED_NOBLINK_TIME)
				mod_timer(&priv->pshare->LED_Timer, jiffies + priv->pshare->LED_Interval);
			else
				priv->pshare->LED_Interval = LED_NOBLINK_TIME;
		}
	}
	else
	{
		if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) ||
			(priv->pmib->dot11BssType.net_work_type & WIRELESS_11A))
			scale_num = LED_MAX_PACKET_CNT_AG / LED_MAX_SCALE;
		else
			scale_num = LED_MAX_PACKET_CNT_B / LED_MAX_SCALE;

		if ((LED_TYPE == LEDTYPE_SW_LINK_TXRX) ||
			(LED_TYPE == LEDTYPE_SW_LINKTXRX) ||
			(LED_TYPE == LEDTYPE_SW_CUSTOM1))
			scale_num = scale_num*2;

		for (i=1; i<=LED_MAX_SCALE; i++) {
			if (delta < i*scale_num)
				break;
		}

		priv->pshare->LED_Interval = ((LED_MAX_SCALE-i+1)*LED_INTERVAL_TIME)/LED_MAX_SCALE;

		if (priv->pshare->LED_Interval < LED_ON_TIME)
			priv->pshare->LED_Interval = LED_ON_TIME;
	}

	if ((LED_TYPE == LEDTYPE_SW_LINKTXRX) ||
		(LED_TYPE == LEDTYPE_SW_LINKTXRXDATA)) {
		if (priv->link_status)
			priv->pshare->LED_ToggleStart = LED_ON;
		else
			priv->pshare->LED_ToggleStart = LED_OFF;
	}
	else {
		if (priv->pshare->set_led_in_progress)
			return;

		if ((LED_TYPE == LEDTYPE_SW_LINK_TXRX) ||
			(LED_TYPE == LEDTYPE_SW_LINK_TXRXDATA)) {
			if (priv->link_status)
				set_sw_LED0(priv, LED_ON);
			else
				set_sw_LED0(priv, LED_OFF);
		}
		else if (LED_TYPE == LEDTYPE_SW_ADATA_GDATA) {
			if (priv->pshare->curr_band == BAND_5G) {
				set_sw_LED0(priv, LED_ON);
				set_sw_LED1(priv, LED_OFF);
			}
			else {	// 11A
				set_sw_LED0(priv, LED_OFF);
				set_sw_LED1(priv, LED_ON);
			}
		}
	}
}


void set_wireless_LED_steady_on(int led_num, struct net_device *dev)
{
	struct rtl8190_priv *priv;

	if (led_num != LED_0 && led_num != LED_1)
		return;

	if (dev == NULL || dev->priv == NULL)
		return;

	priv = (struct rtl8190_priv *)dev->priv;

	if (priv->pshare == NULL)
		return;

	priv->pshare->set_led_in_progress = 1;

	if ((LED_TYPE >= LEDTYPE_HW_TX_RX) && (LED_TYPE <= LEDTYPE_HW_LINKACT_INFRA)) {
		enable_sw_LED(priv, 0);
	}
	else if ((LED_TYPE >= LEDTYPE_SW_LINK_TXRX) && (LED_TYPE < LEDTYPE_SW_MAX)) {
		if (timer_pending(&priv->pshare->LED_Timer))
			del_timer_sync(&priv->pshare->LED_Timer);
	}

	if (led_num == LED_0)
		set_sw_LED0(priv, LED_ON);
	else
		set_sw_LED1(priv, LED_ON);
}


void recover_wireless_LED(struct net_device *dev)
{
	struct rtl8190_priv *priv;

	if (dev == NULL || dev->priv == NULL)
		return;

	priv = (struct rtl8190_priv *)dev->priv;

	if (!priv->pshare->set_led_in_progress)
		return;

	// for HW/SW LED
	if ((LED_TYPE >= LEDTYPE_HW_TX_RX) && (LED_TYPE <= LEDTYPE_HW_LINKACT_INFRA)) {
		set_sw_LED0(priv, LED_OFF);
		set_sw_LED1(priv, LED_OFF);
		enable_hw_LED(priv, LED_TYPE);
	}
	else if ((LED_TYPE >= LEDTYPE_SW_LINK_TXRX) && (LED_TYPE < LEDTYPE_SW_MAX)) {
		enable_sw_LED(priv, 0);
		mod_timer(&priv->pshare->LED_Timer, jiffies + priv->pshare->LED_Interval);
	}

	priv->pshare->set_led_in_progress = 0;
}


void control_wireless_led(struct rtl8190_priv *priv, int enable)
{
	if (enable == 0) {
		priv->pshare->set_led_in_progress = 1;
		set_sw_LED0(priv, LED_OFF);
		set_sw_LED1(priv, LED_OFF);
	}
	else if (enable == 1) {
		priv->pshare->set_led_in_progress = 1;
		set_sw_LED0(priv, LED_ON);
		set_sw_LED1(priv, LED_ON);
	}
	else if (enable == 2) {
		set_sw_LED0(priv, priv->pshare->LED_ToggleStart);
		set_sw_LED1(priv, priv->pshare->LED_ToggleStart);
		priv->pshare->set_led_in_progress = 0;
	}
}


#ifdef DFS
void DetermineDFSDisable(struct rtl8190_priv *priv)
{
	//this function will be called by the one min timer
	struct net_device_stats *pnet_stats;
	unsigned int i, total=0, total_Mbits_persec;

	pnet_stats = &(priv->net_stats);

	// record the throughput (Tx+Rx) in M bits/sec for the past one sec
	priv->TotalTxRx[priv->TotalTxRx_index++] = pnet_stats->tx_bytes + pnet_stats->rx_bytes - priv->PreTxBytes - priv->PreRxBytes;
	if (priv->TotalTxRx_index >=  priv->pmib->dot11DFSEntry.RecordHistory_sec)
		priv->TotalTxRx_index = 0;
	for (i=0; i < priv->pmib->dot11DFSEntry.RecordHistory_sec; i++)
		total += priv->TotalTxRx[i];
	total_Mbits_persec = (total * 8) / (priv->pmib->dot11DFSEntry.RecordHistory_sec * 1024 *1024);
	priv->PreTxBytes = pnet_stats->tx_bytes;
	priv->PreRxBytes = pnet_stats->rx_bytes;

	if (total_Mbits_persec > priv->pmib->dot11DFSEntry.Throughput_Threshold) {
		if (!priv->pmib->dot11DFSEntry.temply_disable_DFS) {
			priv->pmib->dot11DFSEntry.temply_disable_DFS = TRUE;
			WriteBBPortUchar(priv, 0x759b);
		}
	}
	else {
		if (priv->pmib->dot11DFSEntry.temply_disable_DFS) {
			memset(priv->rs1, 0, sizeof(priv->rs1));
			memset(priv->rs2, 0, sizeof(priv->rs2));
			priv->pmib->dot11DFSEntry.temply_disable_DFS = FALSE;
			WriteBBPortUchar(priv, 0x7b9b);
		}
	}
	if (priv->pmib->dot11DFSEntry.Dump_Throughput) {
		printk(" S%d", total_Mbits_persec);
		printk("D%d", priv->pmib->dot11DFSEntry.temply_disable_DFS);
	}
}


void rtl8190_DFS_timer(unsigned long task_priv)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)task_priv;
	unsigned long ioaddr = priv->pshare->ioaddr;
	volatile unsigned int radar_counter;
	unsigned int i;
	unsigned int j1, j2;

	radar_counter = RTL_R32(0x114); //read the number of pulses
	if (priv->pmib->dot11DFSEntry.temply_disable_DFS)
		goto ClearRadarCounter;

	priv->rs1[priv->rs1_index++] = radar_counter;
	priv->rs2[priv->rs2_index++] = radar_counter;
	if (priv->rs1_index == 4)
		priv->rs1_index = 0;
	if (priv->rs2_index == 8)
		priv->rs2_index = 0;

	j1=0;
	for (i=0; i<=3; i++)
		j1 += priv->rs1[i]; //accumulate the pulses for the past 40ms
	j2=0;
	for (i=0; i<=7; i++)
		j2 += priv->rs2[i]; //accumulate the pulses for the past 80ms
	if ((j1 >= priv->pmib->dot11DFSEntry.rs1_threshold) ||
		(j2 >= priv->pmib->dot11DFSEntry.rs1_threshold))
		priv->pmib->dot11DFSEntry.DFS_detected = 1;

	if (!priv->pmib->dot11DFSEntry.disable_DFS && priv->pmib->dot11DFSEntry.DFS_detected) {
		RTL_W8(_CR_, 0); //disable transmitter
		priv->pmib->dot11DFSEntry.disable_tx = 1;
		PRINT_INFO("Radar is detected!\n");

		if (timer_pending(&priv->ch_avail_chk_timer))
			del_timer_sync(&priv->ch_avail_chk_timer);

		switch(priv->pmib->dot11RFEntry.dot11channel)
		{
			case 52:
				mod_timer(&priv->ch52_timer, jiffies + NONE_OCCUPANCY_PERIOD);
				break;
			case 56:
				mod_timer(&priv->ch56_timer, jiffies + NONE_OCCUPANCY_PERIOD);
				break;
			case 60:
				mod_timer(&priv->ch60_timer, jiffies + NONE_OCCUPANCY_PERIOD);
				break;
			case 64:
				mod_timer(&priv->ch64_timer, jiffies + NONE_OCCUPANCY_PERIOD);
				break;
		}

		//add the channel in the blocked-channel list
		InsertChannel(priv->NOP_chnl, &priv->NOP_chnl_num, priv->pmib->dot11RFEntry.dot11channel);

		priv->pmib->dot11RFEntry.dot11channel = DFS_SelectChannel(); //select a non-DFS channel
		PRINT_INFO("Swiching channel to %d!\n", priv->pmib->dot11RFEntry.dot11channel);
		priv->pmib->dot11OperationEntry.keep_rsnie = 1; // recovery in WPA case, david+2006-01-27
#ifdef MBSSID
		if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
		{
			for (i=0; i<RTL8190_NUM_VWLAN; i++) {
				if (IS_DRV_OPEN(priv->pvap_priv[i]))
					priv->pvap_priv[i]->pmib->dot11OperationEntry.keep_rsnie = 1;
			}
		}
#endif
		rtl8190_close(priv->dev);
		rtl8190_open(priv->dev);
		return;
	}

ClearRadarCounter:
	//clear the radar counter
	RTL_W32(0x100, (RTL_R32(0x100) & 0xFFFFFFFE));
	RTL_W32(0x100, (RTL_R32(0x100) | BIT(0)));
	mod_timer(&priv->DFS_timer, jiffies + DFS_TO);
}


void rtl8190_ch_avail_chk_timer(unsigned long task_priv)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)task_priv;

	priv->pmib->dot11DFSEntry.disable_tx = 0;
	PRINT_INFO("Transmitter is enabled!\n");
}


void rtl8190_ch52_timer(unsigned long task_priv)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)task_priv;

	//still block channel 52 if in adhoc mode in Japan
	if (((priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_MKK) ||
		 (priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_MKK3)) &&
		(OPMODE & WIFI_ADHOC_STATE))
		return;

	//remove the channel from NOP_chnl[4] and place it in available_chnl[32]
	if (RemoveChannel(priv->NOP_chnl, &priv->NOP_chnl_num, 52)) {
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11A)
			InsertChannel(priv->available_chnl, &priv->available_chnl_num, 52);
		DEBUG_INFO("Channel 52 is released!\n");
	}
}


void rtl8190_ch56_timer(unsigned long task_priv)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)task_priv;

	if (((priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_MKK) ||
		 (priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_MKK3)) &&
		(OPMODE & WIFI_ADHOC_STATE))
		return;
	if (RemoveChannel(priv->NOP_chnl, &priv->NOP_chnl_num, 56)) {
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11A)
			InsertChannel(priv->available_chnl, &priv->available_chnl_num, 56);
		DEBUG_INFO("Channel 56 is released!\n");
	}
}


void rtl8190_ch60_timer(unsigned long task_priv)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)task_priv;

	if (((priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_MKK) ||
		 (priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_MKK3)) &&
		(OPMODE & WIFI_ADHOC_STATE))
		return;
	if (RemoveChannel(priv->NOP_chnl, &priv->NOP_chnl_num, 60)) {
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11A)
			InsertChannel(priv->available_chnl, &priv->available_chnl_num, 60);
		DEBUG_INFO("Channel 60 is released!\n");
	}
}


void rtl8190_ch64_timer(unsigned long task_priv)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)task_priv;

	if (((priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_MKK) ||
		 (priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_MKK3)) &&
		(OPMODE & WIFI_ADHOC_STATE))
		return;
	if (RemoveChannel(priv->NOP_chnl, &priv->NOP_chnl_num, 64)) {
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11A)
			InsertChannel(priv->available_chnl, &priv->available_chnl_num, 64);
		DEBUG_INFO("Channel 64 is released!\n");
	}
}


unsigned int DFS_SelectChannel(void)
{
	unsigned char random;
	unsigned int num, which_channel=36;

	get_random_bytes(&random, 1);
	num = random % 4;
	switch(num)
	{
		case 0:
			which_channel = 36;
			break;
		case 1:
			which_channel = 40;
			break;
		case 2:
			which_channel = 44;
			break;
		case 3:
			which_channel = 48;
			break;
	}
	return which_channel;
}


//insert the channel into the channel list
//if successful, return 1, else return 0
int InsertChannel(unsigned int chnl_list[], unsigned int *chnl_num, unsigned int channel)
{
	unsigned int i, j;

	if (*chnl_num==0) {
		chnl_list[0] = channel;
		(*chnl_num)++;
		return SUCCESS;
	}
	for (i=0; i < *chnl_num; i++) {
		if (chnl_list[i] == channel) {
			_DEBUG_INFO("Inserting channel failed: channel %d already exists!\n", channel);
			return FAIL;
		}
		else if (chnl_list[i] > channel)
			break;
	}
	if (i == *chnl_num)
		chnl_list[(*chnl_num)++] = channel;
	else {
		for (j=*chnl_num; j > i; j--)
			chnl_list[j] = chnl_list[j-1];
		chnl_list[j] = channel;
		(*chnl_num)++;
	}
	return SUCCESS;
}


//remove the channel from the channel list
//if successful, return 1, else return 0
int RemoveChannel(unsigned int chnl_list[], unsigned int *chnl_num, unsigned int channel)
{
	unsigned int i, j;

	if (*chnl_num) {
		for (i=0; i < *chnl_num; i++)
			if (channel == chnl_list[i])
				break;
		if (i == *chnl_num)  {
			_DEBUG_INFO("Can not remove channel %d!\n", channel);
			return FAIL;
		}
		else {
			for (j=i; j < (*chnl_num-1); j++)
				chnl_list[j] = chnl_list[j+1];
			(*chnl_num)--;
			return SUCCESS;
		}
	}
	else {
		_DEBUG_INFO("Can not remove channel %d!\n", channel);
		return FAIL;
	}
}
#endif

#ifdef RTL8192SU_CHECKTXHANGUP
void send_probereq(struct rtl8190_priv *priv)
{
	issue_probereq(priv, NULL, 0, NULL);
}
#endif

