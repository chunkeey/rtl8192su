/*
 *      TX handle routines
 *
 *      $Id: 8190n_tx.c,v 1.21 2009/09/28 13:26:29 cathy Exp $
 */

#define _8190N_TX_C_

#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/netdevice.h>
#endif

#ifdef __DRAYTEK_OS__
#include <draytek/wl_dev.h>
#endif

#include "./8190n_cfg.h"
#include "./8190n.h"
#include "./8190n_hw.h"
#include "./8190n_headers.h"
#include "./8190n_debug.h"

#ifdef RTL867X_CP3
#include "./romeperf.h"
#endif

#ifndef __KERNEL__
#include "./sys-support.h"
#endif

#ifdef RTL8190_VARIABLE_USED_DMEM
#include "./8190n_dmem.h"
#endif

#if defined(CONFIG_RTL_WAPI_SUPPORT)
#include "wapiCrypto.h"
#endif

#define AMSDU_TX_DESC_TH		2	// A-MSDU tx desc threshold, A-MSDU will be
									// triggered when more than this threshold packet in hw queue

#define RET_AGGRE_BYPASS		0
#define RET_AGGRE_ENQUE			1
#define RET_AGGRE_DESC_FULL		2

#define TX_NORMAL				0
#define TX_NO_MUL2UNI			1
#define TX_AMPDU_BUFFER_SIG		2
#define TX_AMPDU_BUFFER_FIRST	3
#define TX_AMPDU_BUFFER_MID		4
#define TX_AMPDU_BUFFER_LAST	5

#if	defined(RTL8190) || defined(RTL8192E)
#define PRI_TO_QNUM(priority, q_num, wifi_specific) { \
	if (wifi_specific) { \
		if ((priority == 0) || (priority == 3)) \
			q_num = BE_QUEUE; \
		else if ((priority == 7) || (priority == 6)) \
			q_num = VO_QUEUE; \
		else if ((priority == 5) || (priority == 4)) \
			q_num = VI_QUEUE; \
		else  \
			q_num = BK_QUEUE; \
	} \
	else { \
		if (priority == 4 || priority == 5 || priority == 6 || priority == 7) \
			q_num = VI_QUEUE; \
		else \
			q_num = BE_QUEUE; \
	} \
}
#elif	defined(RTL8192SE) || defined(RTL8192SU)
#define PRI_TO_QNUM(priority, q_num, wifi_specific) { \
		if ((priority == 0) || (priority == 3)) \
			q_num = BE_QUEUE; \
		else if ((priority == 7) || (priority == 6)) \
			q_num = VO_QUEUE; \
		else if ((priority == 5) || (priority == 4)) \
			q_num = VI_QUEUE; \
		else  \
			q_num = BK_QUEUE; \
}
#endif

#ifdef  SUPPORT_TX_MCAST2UNI
#define IP_MCAST_MAC(mac)	((mac[0]==0x01)&&(mac[1]==0x00)&&(mac[2]==0x5e))

#ifdef	IGMP_FILTER_CMO
#define IS_IGMP_PROTO(mac)	((mac[23]==0x02))
#endif

#ifdef	TX_SUPPORT_IPV6_MCAST2UNI
#define ICMPV6_MCAST_MAC(mac) 	   ((mac[0]==0x33)&&(mac[1]==0x33)&&(mac[2]!=0xff))
#endif

#endif

#ifdef CONFIG_RTL8671
extern int enable_IGMP_SNP;
extern int check_IGMP_report(struct sk_buff *skb);
extern int check_wlan_mcast_tx(struct sk_buff *skb);
// MBSSID Port Mapping
extern struct port_map wlanDev[5];
extern int g_port_mapping;
#endif

#if defined(RTL8192E) || defined(STA_EXT)
extern unsigned short MCS_DATA_RATE[2][2][16];

unsigned short BG_TABLE[2][21] = {{73,68,63,57,52,47,42,37,31,30,27,25,23,22,20,18,16,14,11,9,8},
				{108,108,108,108,108,108,108,108,108,108,108,108,72,72,48,48,36,12,11,11,4}};
unsigned short MCS_40M_TABLE[2][18] = {{73,68,63,57,53,45,39,37,31,29,28,26,25,23,21,19,17,12},
					{7,7,7,7,7,7,7,7,5,4,4,4,2,2,2,0,0,0}};
unsigned short MCS_20M_TABLE[2][22]={{73,68,65,60,55,50,45,40,31,32,27,26,24,22,21,19,18,16,14,11,9,8},
					{7,7,7,7,7,7,7,7,7,6,5,5,5,4,3,3,3,2,3,1,0,0}};

#define BG_TABLE_SIZE 21
#define MCS_40M_TABLE_SIZE 18
#define MCS_20M_TABLE_SIZE 22
#endif

static int tkip_mic_padding(struct rtl8190_priv *priv,
			unsigned char *da, unsigned char *sa, unsigned char priority,
			unsigned char *llc,struct sk_buff *pskb, struct tx_insn* txcfg);

static void wep_fill_iv(struct rtl8190_priv *priv,
			unsigned char *pwlhdr, unsigned int hdrlen, unsigned long keyid);

static void tkip_fill_encheader(struct rtl8190_priv *priv,
			unsigned char *pwlhdr, unsigned int hdrlen, unsigned long keyid_out);

static void aes_fill_encheader(struct rtl8190_priv *priv,
			unsigned char *pwlhdr, unsigned int hdrlen, unsigned long keyid);

#if !defined(RTL8192SU)
static int rtl8190_tx_queueDsr(struct rtl8190_priv *priv, unsigned int txRingIdx);
#endif

static void rtl8190_tx_restartQueue(struct rtl8190_priv *priv);

#if !defined(RTL8192SU)
static __inline__ unsigned int GetTxDuration(struct rtl8190_priv *priv, unsigned int tx_rate,
				unsigned int len, unsigned int shrt)
{
	unsigned int dur = 0, Ceiling;

	if (tx_rate==0) {
		DEBUG_ERR("invalid tx rate. use 1M as default!\n");
		tx_rate = 2;
	}

	switch(tx_rate)
	{
	case 108:
	case 96:
	case 72:
	case 66:
	case 48:
	case 44:
	case 36:
	case 24:
	case 18:
	case 12:
		Ceiling = ((16 + (len << 3) + 6) + (tx_rate << 1) - 1) / (tx_rate << 1);
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G)
			dur = 16 + 4 + 4 * Ceiling + 4 + 6;
		else // 11a
			dur = 16 + 4 + 4 * Ceiling + 4;
		break;
	case 22:
	case 11:
	case 4:
		if (shrt)
			dur = 72 + 24 + ((len << 4) + tx_rate - 1) / tx_rate;
		else
			dur = 144 + 48 + ((len << 4) + tx_rate - 1) / tx_rate;
		break;
	case 2:
		dur = 144 + 48 + ((len << 4) + tx_rate - 1) / tx_rate;
		break;
	}

	return dur;
}
#endif


#ifndef CONFIG_RTK_MESH
static __inline__
#endif
unsigned int get_tx_rate(struct rtl8190_priv *priv, struct stat_info *pstat)
{
#if defined(RTL8192E) || defined(STA_EXT)
	if (priv->pmib->dot11StationConfigEntry.autoRate 
#ifdef RTL8192E
		&& ((++pstat->try_rate_idx) == TRY_RATE_FREQ)
#endif
		&& pstat->remapped_aid < FW_NUM_STAT-1/*!(priv->STA_map & BIT(pstat->aid)*/
		) { 
#ifdef RTL8192E
		pstat->try_rate_idx = 0;
		if (pstat->upper_tx_rate)
			return pstat->upper_tx_rate;
		else
#endif
			return pstat->current_tx_rate;
	}
	else
	{
		if (pstat->remapped_aid == FW_NUM_STAT-1/*(priv->STA_map & BIT(pstat->aid)) != 0*/) {
			// firmware does not keep the aid ...
			//use default rate instead
			if (pstat->ht_cap_len) {	// is N client
				if (pstat->tx_bw == HT_CHANNEL_WIDTH_20_40) {//(pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_))){ //40Mhz
					int i = 0;
					pstat->current_tx_rate = MCS_40M_TABLE[1][MCS_40M_TABLE_SIZE-1] | 0x80;
					for (i=1; i < MCS_40M_TABLE_SIZE; i++) {
						if (pstat->rssi > MCS_40M_TABLE[0][i]){
							pstat->current_tx_rate = MCS_40M_TABLE[1][i-1] | 0x80;
							break;
						}
					}
					return pstat->current_tx_rate;
				} else { // 20Mhz
					int i = 0;
					pstat->current_tx_rate = MCS_20M_TABLE[1][MCS_20M_TABLE_SIZE-1] | 0x80;
					for (i=1; i < MCS_20M_TABLE_SIZE; i++) {
						if (pstat->rssi > MCS_20M_TABLE[0][i]){
							pstat->current_tx_rate = MCS_20M_TABLE[1][i-1] | 0x80;
							break;
						}
					}
					return pstat->current_tx_rate;
				}
				return pstat->current_tx_rate;

			} else { // is BG client
				int i = 0;
				pstat->current_tx_rate = BG_TABLE[1][BG_TABLE_SIZE-1] | 0x80;
				for (i = 0; i < BG_TABLE_SIZE; i++) {
					if (pstat->rssi > BG_TABLE[0][i]){
						pstat->current_tx_rate = BG_TABLE[1][i-1];
						break;
				        }
				}
				return pstat->current_tx_rate;
			}
		} else
			return pstat->current_tx_rate;
	}
#elif defined(RTL8190) || (defined(RTL8192SE)||defined(RTL8192SU)) && !defined(STA_EXT)
	return pstat->current_tx_rate;
#endif
}


#ifndef CONFIG_RTK_MESH
static __inline__
#endif
unsigned int get_lowest_tx_rate(struct rtl8190_priv *priv, struct stat_info *pstat,
				unsigned int tx_rate)
{
	unsigned int lowest_tx_rate;

	if (priv->pmib->dot11StationConfigEntry.autoRate) {
#if 0	// unused
		if ((pstat->high_rssi_state) && is_CCK_rate(tx_rate) && !pstat->not_ch_rate_by_rssi)
			lowest_tx_rate = tx_rate;
		else
#endif
			lowest_tx_rate = find_rate(priv, pstat, 0, 0);
	}
	else
		lowest_tx_rate = tx_rate;

	return lowest_tx_rate;
}


#ifdef RTL8192SU
__IRAM_WIFI_PRI5
#endif
void assign_wlanseq(struct rtl8190_hw *phw, unsigned char *pframe, struct stat_info *pstat, struct wifi_mib *pmib
#ifdef CONFIG_RTK_MESH	// For broadcast data frame via mesh (ex:ARP requst)
	, unsigned char is_11s
#endif
	)
{
#ifdef SEMI_QOS
	unsigned char qosControl[2];
	int tid;

	if (is_qos_data(pframe)) {
		memcpy(qosControl, GetQosControl(pframe), 2);
		tid = qosControl[0] & 0x07;

		if (pstat) {
			SetSeqNum(pframe, pstat->AC_seq[tid]);
			pstat->AC_seq[tid] = (pstat->AC_seq[tid] + 1) & 0xfff;
		}
		else {
//			SetSeqNum(pframe, phw->AC_seq[tid]);
//			phw->AC_seq[tid] = (phw->AC_seq[tid] + 1) & 0xfff;
#ifdef CONFIG_RTK_MESH	// For broadcast data frame via mesh (ex:ARP requst)
			if (is_11s)
			{
				SetSeqNum(pframe, phw->seq);
				phw->seq = (phw->seq + 1) & 0xfff;
			}
			else
#endif
				printk("Invalid seq num setting for Multicast or Broadcast pkt!!\n");
		}

#if defined(RTL8190) || defined(RTL8192E)
		if (!pmib->dot11OperationEntry.wifi_specific)
#endif
		{
			if ((tid == 7) || (tid == 6))
				phw->VO_pkt_count++;
			else if ((tid == 5) || (tid == 4))
				phw->VI_pkt_count++;
			else if ((tid == 2) || (tid == 1))
				phw->BK_pkt_count++;
		}
	}
	else
#endif
	{
		SetSeqNum(pframe, phw->seq);
		phw->seq = (phw->seq + 1) & 0xfff;
	}
}


#ifdef CONFIG_RTK_MESH
static unsigned int get_skb_priority3(struct rtl8190_priv *priv, struct sk_buff *skb, int is_11s)
#else
static unsigned int get_skb_priority(struct rtl8190_priv *priv, struct sk_buff *skb)
#endif
{
	unsigned int pri=0, parsing=0;
	unsigned char protocol[2];

#ifdef SEMI_QOS
	if (QOS_ENABLE)
		parsing = 1;
#endif

	if (parsing) {
#ifdef CONFIG_RTK_VLAN_SUPPORT
		if (skb->cb[0]) 
			pri =  skb->cb[0];
		else
#endif
		{		
			protocol[0] = skb->data[12];
			protocol[1] = skb->data[13];

			if ((protocol[0] == 0x08) && (protocol[1] == 0x00))
			{
#ifdef CONFIG_RTK_MESH
				if(is_11s & 8)
				{
					if(skb->data[14]&1) // 6 addr
						pri = (skb->data[31] & 0xe0) >> 5;
					else
						pri = (skb->data[19] & 0xe0) >> 5;
				}
				else
#endif
					pri = (skb->data[15] & 0xe0) >> 5;
			}
			else if ((skb->cb[0]>0) && (skb->cb[0]<8))	// Ethernet driver will parse priority and put in cb[0]
				pri = skb->cb[0];
			else
				pri = 0;
		}
#ifdef RTL8192SU
		if (pri!=0x7 && pri!=0x5 && pri!=0x1 && pri!=0x0)
			pri = 0;
#endif
		skb->cb[1] = pri;

		return pri;
	}
	else {
		// default is no priority
		skb->cb[1] = 0;
		return 0;
	}
}

#ifdef CONFIG_RTK_MESH
#define get_skb_priority(priv, skb)  get_skb_priority3(priv, skb, 0)
#endif

static int dz_queue(struct rtl8190_priv *priv, struct stat_info *pstat, struct sk_buff *pskb)
{
#ifdef SW_MCAST
	unsigned int ret;
#endif

	if (pstat)
	{
#if defined(PKT_PROCESSOR) || defined(PRE_ALLOCATE_SKB)
		//swap skb
		if(pskb->fcpu==0)
		{
			unsigned char *head,*data,*tail,*end;
			struct sk_buff* pskb2;
			pskb2=(struct sk_buff *)(((unsigned int)dev_alloc_skb(BUFFER_SIZE)));
			if(pskb2==NULL)
			{
				printk("allocate failed at: %s %d\n",__FUNCTION__,__LINE__);
				return FALSE;
			}
			init_skb(pskb2,BUFFER_SIZE);
			head=pskb2->head;
			data=pskb2->data;
			tail=pskb2->tail;
			end=pskb2->end;
			pskb2->len=pskb->len;
			pskb2->dev=pskb->dev;
			pskb2->head=(unsigned char *)(((unsigned int)pskb->head)&(~0x20000000));
			pskb2->data=(unsigned char *)(((unsigned int)pskb->data)&(~0x20000000));
			pskb2->tail=(unsigned char *)(((unsigned int)pskb->tail)&(~0x20000000));
			pskb2->end=(unsigned char *)(((unsigned int)pskb->end)&(~0x20000000));
			pskb->head=head;
			pskb->data=data;
			pskb->tail=tail;
			pskb->end=end;
			pskb->len=0;
			skb_reserve(pskb,NET_SKB_PAD+2); // must sync with packet_prcoessor driver
			rtl_kfree_skb(priv,pskb,_SKB_TX_|_SKB_TX_SWAP_);
			pskb2->fcpu=1; // free by dev_kfree_skb
			pskb=pskb2;
			//printk("queue swap pre_allocate_skb\n");
		}

#endif

#if defined(SEMI_QOS) && defined(WMM_APSD)
		if ((QOS_ENABLE) && (APSD_ENABLE) && (pstat->QosEnabled) && (pstat->apsd_bitmap & 0x0f)) {
			int pri = 0;

			pri = get_skb_priority(priv, pskb);

			if (((pri == 7) || (pri == 6)) && (pstat->apsd_bitmap & 0x01)) {
				ret = enque(priv, &(pstat->VO_dz_queue->head), &(pstat->VO_dz_queue->tail),
					(unsigned int)(pstat->VO_dz_queue->pSkb), NUM_APSD_TXPKT_QUEUE, (void *)pskb);
				if (ret==1)
					DEBUG_INFO("enque VO pkt\n");
			}
			else if (((pri == 5) || (pri == 4)) && (pstat->apsd_bitmap & 0x02)) {
				ret = enque(priv, &(pstat->VI_dz_queue->head), &(pstat->VI_dz_queue->tail),
					(unsigned int)(pstat->VI_dz_queue->pSkb), NUM_APSD_TXPKT_QUEUE, (void *)pskb);
				if(ret==1)
					DEBUG_INFO("enque VI pkt\n");
			}
			else if (((pri == 0) || (pri == 3)) && (pstat->apsd_bitmap & 0x08)) {
				ret = enque(priv, &(pstat->BE_dz_queue->head), &(pstat->BE_dz_queue->tail),
					(unsigned int)(pstat->BE_dz_queue->pSkb), NUM_APSD_TXPKT_QUEUE, (void *)pskb);
				if(ret==1)
					DEBUG_INFO("enque BE pkt\n");
			}
			else if (pstat->apsd_bitmap & 0x04) {
				ret = enque(priv, &(pstat->BK_dz_queue->head), &(pstat->BK_dz_queue->tail),
					(unsigned int)(pstat->BK_dz_queue->pSkb), NUM_APSD_TXPKT_QUEUE, (void *)pskb);
				if(ret==1)
					DEBUG_INFO("enque BK pkt\n");
			}
			else
				goto legacy_ps;

			if (!pstat->apsd_pkt_buffering)
				pstat->apsd_pkt_buffering = 1;

			if (ret == FALSE) {
				DEBUG_ERR("sleep Q full for priority = %d!\n", pri);
				return CONGESTED;
			}
			return TRUE;
		}
		else
legacy_ps:
#endif
		skb_queue_tail(&pstat->dz_queue, pskb);
		return TRUE;
	}
#ifdef SW_MCAST
	else {	// Multicast or Broadcast
		ret = enque(priv, &(priv->dz_queue.head), &(priv->dz_queue.tail),
			(unsigned int)(priv->dz_queue.pSkb), NUM_TXPKT_QUEUE, (void *)pskb);
		if (ret == TRUE) {
		        priv->queued_mcast++;
		        if (!priv->pkt_in_dtimQ)
			        priv->pkt_in_dtimQ = 1;
		        return TRUE;
                }
	}
#endif
	return FALSE;
}

#ifdef RTL8192SU
__IRAM_WIFI_PRI5 unsigned int get_TxUrb_Pending_num(struct rtl8190_priv *priv)
{
	unsigned int i, tx_pending;

	tx_pending=0;
	for (i=MGNT_QUEUE; i<=VO_QUEUE; i++)
	{
		tx_pending += atomic_read(&priv->tx_pending[i]);
	}
	return tx_pending;
}
#endif


/*        Function to process different situations in TX flow             */
/* ====================================================================== */
#define TX_PROCEDURE_CTRL_STOP			0
#define TX_PROCEDURE_CTRL_CONTINUE		1
#define TX_PROCEDURE_CTRL_SUCCESS		2

#ifdef WDS
static int rtl8190_tx_wdsDevProc(struct rtl8190_priv *priv, struct sk_buff *skb, struct net_device **dev_p,
				struct net_device **wdsDev_p, struct tx_insn *txcfg)
{
	struct stat_info *pstat;

	txcfg->wdsIdx = getWdsIdxByDev(priv, *dev_p);
	if (txcfg->wdsIdx < 0) {
		priv->ext_stats.tx_drops++;
		DEBUG_ERR("TX DROP: getWdsIdxByDev() fail!\n");
		goto free_and_stop;
	}

	if (!netif_running(priv->dev)) {
		priv->ext_stats.tx_drops++;
		DEBUG_ERR("TX DROP: Can't send WDS packet due to wlan interface is down!\n");
		goto free_and_stop;
	}
	pstat = get_stainfo(priv, priv->pmib->dot11WdsInfo.entry[txcfg->wdsIdx].macAddr);
	if (pstat && pstat->current_tx_rate==0) {
		priv->ext_stats.tx_drops++;
		DEBUG_ERR("TX DROP: Can't send packet due to tx rate is not supported in peer WDS AP!\n");
		goto free_and_stop;
	}
	*wdsDev_p = *dev_p;
	*dev_p = priv->dev;

	/* Reply caller function : Continue process */
	return TX_PROCEDURE_CTRL_CONTINUE;

free_and_stop:		/* Free current packet and stop TX process */

#ifdef RTL8190_FASTEXTDEV_FUNCALL
	rtl865x_extDev_kfree_skb(skb, FALSE);
#else
#ifdef DEBUG_MEMORY_LEAK
	printk("%s %d freeskb=%x\n",__FUNCTION__,__LINE__,(unsigned int)skb);
#endif
#if defined(PKT_PROCESSOR) || defined(PRE_ALLOCATE_SKB)
	rtl_kfree_skb(priv,skb,_SKB_TX_);
#else
	dev_kfree_skb_any(skb);
#endif
#endif //RTL8190_FASTEXTDEV_FUNCALL

	/* Reply caller function : STOP process */
	return TX_PROCEDURE_CTRL_STOP;
}
#endif


#ifdef CLIENT_MODE
static int rtl8190_tx_clientMode(struct rtl8190_priv *priv, struct sk_buff **pskb)
{
	struct sk_buff *skb=*pskb;

#ifdef A4_TUNNEL
	if (priv->pmib->miscEntry.a4tnl_enable)
		A4_tunnel_insert(priv, get_stainfo(priv, BSSID), &(skb->data[6]));
	else
#endif
	{
#ifdef RTK_BR_EXT
		int res, is_vlan_tag=0, i, do_nat25=1;
		unsigned short vlan_hdr=0;

		mac_clone_handle_frame(priv, skb);

		if (!priv->pmib->ethBrExtInfo.nat25_disable) {
//			if (priv->dev->br_port &&
//				 !memcmp(skb->data+MACADDRLEN, priv->br_mac, MACADDRLEN)) {
			if (*((unsigned short *)(skb->data+MACADDRLEN*2)) == __constant_htons(ETH_P_8021Q)) {
				is_vlan_tag = 1;
				vlan_hdr = *((unsigned short *)(skb->data+MACADDRLEN*2+2));
				for (i=0; i<6; i++)
					*((unsigned short *)(skb->data+MACADDRLEN*2+2-i*2)) = *((unsigned short *)(skb->data+MACADDRLEN*2-2-i*2));
				skb_pull(skb, 4);
//			}

			if (!memcmp(skb->data+MACADDRLEN, priv->br_mac, MACADDRLEN) &&
				(*((unsigned short *)(skb->data+MACADDRLEN*2)) == __constant_htons(ETH_P_IP)))
				memcpy(priv->br_ip, skb->data+WLAN_ETHHDR_LEN+12, 4);

			if (*((unsigned short *)(skb->data+MACADDRLEN*2)) == __constant_htons(ETH_P_IP)) {
				if (memcmp(priv->scdb_mac, skb->data+MACADDRLEN, MACADDRLEN)) {
					if ((priv->scdb_entry = (struct nat25_network_db_entry *)scdb_findEntry(priv,
								skb->data+MACADDRLEN, skb->data+WLAN_ETHHDR_LEN+12)) != NULL) {
						memcpy(priv->scdb_mac, skb->data+MACADDRLEN, MACADDRLEN);
						memcpy(priv->scdb_ip, skb->data+WLAN_ETHHDR_LEN+12, 4);
						priv->scdb_entry->ageing_timer = jiffies;
						do_nat25 = 0;
					}
				}
				else {
					if (priv->scdb_entry) {
						priv->scdb_entry->ageing_timer = jiffies;
						do_nat25 = 0;
					}
					else {
						memset(priv->scdb_mac, 0, MACADDRLEN);
						memset(priv->scdb_ip, 0, 4);
						}
					}
				}
			}

			if (do_nat25)
			{
				if (nat25_db_handle(priv, skb, NAT25_CHECK) == 0) {
					struct sk_buff *newskb;

					if (is_vlan_tag) {
						skb_push(skb, 4);
						for (i=0; i<6; i++)
							*((unsigned short *)(skb->data+i*2)) = *((unsigned short *)(skb->data+4+i*2));
						*((unsigned short *)(skb->data+MACADDRLEN*2)) = __constant_htons(ETH_P_8021Q);
						*((unsigned short *)(skb->data+MACADDRLEN*2+2)) = vlan_hdr;
					}

					newskb = skb_copy(skb, GFP_ATOMIC);
#ifdef RTL8190_FASTEXTDEV_FUNCALL
					rtl865x_extDev_pktFromProtocolStack(skb);
					rtl865x_extDev_kfree_skb(skb, FALSE);
#else
					dev_kfree_skb_any(skb);
#endif
					if (newskb == NULL) {
						priv->ext_stats.tx_drops++;
						DEBUG_ERR("TX DROP: skb_copy fail!\n");
						goto stop_proc;
					}
					*pskb = skb = newskb;
					if (is_vlan_tag) {
						vlan_hdr = *((unsigned short *)(skb->data+MACADDRLEN*2+2));
						for (i=0; i<6; i++)
							*((unsigned short *)(skb->data+MACADDRLEN*2+2-i*2)) = *((unsigned short *)(skb->data+MACADDRLEN*2-2-i*2));
						skb_pull(skb, 4);
					}
				}

				res = nat25_db_handle(priv, skb, NAT25_INSERT);
				if (res < 0) {
					if (res == -2) {
						priv->ext_stats.tx_drops++;
						DEBUG_ERR("TX DROP: nat25_db_handle fail!\n");
						goto free_and_stop;
					}
					// we just print warning message and let it go
					DEBUG_WARN("nat25_db_handle INSERT fail!\n");
				}
			}

			memcpy(skb->data+MACADDRLEN, GET_MY_HWADDR, MACADDRLEN);

			dhcp_flag_bcast(priv, skb);

			if (is_vlan_tag) {
				skb_push(skb, 4);
				for (i=0; i<6; i++)
					*((unsigned short *)(skb->data+i*2)) = *((unsigned short *)(skb->data+4+i*2));
				*((unsigned short *)(skb->data+MACADDRLEN*2)) = __constant_htons(ETH_P_8021Q);
				*((unsigned short *)(skb->data+MACADDRLEN*2+2)) = vlan_hdr;
			}
		}

		// check if SA is equal to our MAC
		if (memcmp(skb->data+MACADDRLEN, GET_MY_HWADDR, MACADDRLEN)) {
			priv->ext_stats.tx_drops++;
			DEBUG_ERR("TX DROP: untransformed frame SA:%02X%02X%02X%02X%02X%02X!\n",
				skb->data[6],skb->data[7],skb->data[8],skb->data[9],skb->data[10],skb->data[11]);
			goto free_and_stop;
		}
#endif // RTK_BR_EXT
	}

	/* Reply caller function : Continue process */
	return TX_PROCEDURE_CTRL_CONTINUE;

#ifdef RTK_BR_EXT
free_and_stop:		/* Free current packet and stop TX process */

#ifdef RTL8190_FASTEXTDEV_FUNCALL
	rtl865x_extDev_kfree_skb(skb, FALSE);
#else
	dev_kfree_skb_any(skb);
#endif

stop_proc:
	/* Reply caller function : STOP process */
	return TX_PROCEDURE_CTRL_STOP;
#endif

}
#endif // CLIENT_MODE


#ifdef GBWC
static int rtl8190_tx_gbwc(struct rtl8190_priv *priv, struct stat_info	*pstat, struct sk_buff *skb)
{
	if (((priv->pmib->gbwcEntry.GBWCMode == 1) && (pstat->GBWC_in_group)) ||
		((priv->pmib->gbwcEntry.GBWCMode == 2) && !(pstat->GBWC_in_group))) {
		if ((priv->GBWC_tx_count + skb->len) > ((priv->pmib->gbwcEntry.GBWCThrd * 1024 / 8) / (100 / GBWC_TO))) {
			// over the bandwidth
			if (priv->GBWC_consuming_Q) {
				// in rtl8190_GBWC_timer context
				priv->ext_stats.tx_drops++;
				DEBUG_ERR("TX DROP: BWC bandwidth over!\n");
				rtl_kfree_skb(priv, skb, _SKB_TX_);
			}
			else {
				// normal Tx path
				int ret = enque(priv, &(priv->GBWC_tx_queue.head), &(priv->GBWC_tx_queue.tail),
						(unsigned int)(priv->GBWC_tx_queue.pSkb), NUM_TXPKT_QUEUE, (void *)skb);
				if (ret == FALSE) {
					priv->ext_stats.tx_drops++;
					DEBUG_ERR("TX DROP: BWC tx queue full!\n");
					rtl_kfree_skb(priv, skb, _SKB_TX_);
				}
			}
			goto stop_proc;
		}
		else {
			// not over the bandwidth
			if (CIRC_CNT(priv->GBWC_tx_queue.head, priv->GBWC_tx_queue.tail, NUM_TXPKT_QUEUE) &&
					!priv->GBWC_consuming_Q) {
				// there are already packets in queue, put in queue too for order
				int ret = enque(priv, &(priv->GBWC_tx_queue.head), &(priv->GBWC_tx_queue.tail),
						(unsigned int)(priv->GBWC_tx_queue.pSkb), NUM_TXPKT_QUEUE, (void *)skb);
				if (ret == FALSE) {
					priv->ext_stats.tx_drops++;
					DEBUG_ERR("TX DROP: BWC tx queue full!\n");
					rtl_kfree_skb(priv, skb, _SKB_TX_);
				}
				goto stop_proc;
			}
			else {
				// can transmit directly
				priv->GBWC_tx_count += skb->len;
			}
		}
	}

	/* Reply caller function : Continue process */
	return TX_PROCEDURE_CTRL_CONTINUE;

stop_proc:
	/* Reply caller function : STOP process */
	return TX_PROCEDURE_CTRL_STOP;
}
#endif

#ifdef TX_SHORTCUT
static int rtl8190_tx_tkip(struct rtl8190_priv *priv, struct sk_buff *skb, struct stat_info*pstat, struct tx_insn *txcfg)
{
	unsigned long	flags = 0;
	struct wlan_ethhdr_t *pethhdr;
	struct llc_snap	*pllc_snap;

	pethhdr = (struct wlan_ethhdr_t *)(skb->data - WLAN_ETHHDR_LEN);
	pllc_snap = (struct llc_snap *)((UINT8 *)(txcfg->phdr) + txcfg->hdr_len + txcfg->iv);

	SAVE_INT_AND_CLI(flags);
#ifdef SEMI_QOS
	if ((tkip_mic_padding(priv, pethhdr->daddr, pethhdr->saddr, ((QOS_ENABLE) && (pstat) && (pstat->QosEnabled))?skb->cb[1]:0, (UINT8 *)pllc_snap,
				skb, txcfg)) == FALSE)
#else
	if ((tkip_mic_padding(priv, pethhdr->daddr, pethhdr->saddr, 0, (UINT8 *)pllc_snap,
				skb, txcfg)) == FALSE)
#endif
	{
		priv->ext_stats.tx_drops++;
		DEBUG_ERR("TX DROP: Tkip mic padding fail!\n");
#ifdef DEBUG_MEMORY_LEAK
		printk("%s %d freeskb=%x\n",__FUNCTION__,__LINE__,(unsigned int)skb);	
#endif
		rtl_kfree_skb(priv, skb, _SKB_TX_);
		release_wlanllchdr_to_poll(priv, txcfg->phdr);
		RESTORE_INT(flags);
		goto stop_proc;
	}
	skb_put((struct sk_buff *)txcfg->pframe, 8);
	txcfg->fr_len += 8;	// for Michael padding.

	RESTORE_INT(flags);
	/* Reply caller function : Continue process */
	return TX_PROCEDURE_CTRL_CONTINUE;

stop_proc:
	/* Reply caller function : STOP process */
	return TX_PROCEDURE_CTRL_STOP;
}
#endif

/*
	Always STOP process after calling this Procedure.
*/
#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
__NOMIPS16
#endif
static void rtl8190_tx_xmitSkbFail(struct rtl8190_priv *priv, struct sk_buff *skb, struct net_device *dev,
				struct net_device *wdsDev, struct tx_insn *txcfg)
{
#ifdef WDS
	if (wdsDev)
	{
#if defined(RTL8192SU) && defined(CONFIG_ENABLE_MIPS16) //for mips 16	
#ifdef CONFIG_NETPOLL_TRAP
		if (!netpoll_trap())
#endif
		set_bit(__LINK_STATE_XOFF, &wdsDev->state);
#else
		netif_stop_queue(wdsDev);
#endif

	}
	else
#endif
	{
#ifdef SEMI_QOS
		if (!QOS_ENABLE)
#endif
		{
#if defined(RTL8192SU) && defined(CONFIG_ENABLE_MIPS16) //for mips 16
#ifdef CONFIG_NETPOLL_TRAP
			if (!netpoll_trap())
#endif
			set_bit(__LINK_STATE_XOFF, &dev->state);
#else
			netif_stop_queue(dev);
#endif
		}
	}

	priv->ext_stats.tx_drops++;
	DEBUG_WARN("TX DROP: Congested!\n");
	if (txcfg->phdr)
		release_wlanllchdr_to_poll(priv, txcfg->phdr);
	if (skb)
	{
#ifdef DEBUG_MEMORY_LEAK
		printk("%s %d freeskb=%x\n",__FUNCTION__,__LINE__,(unsigned int)skb);
#endif	
		rtl_kfree_skb(priv, skb, _SKB_TX_);
	}

	return;
}


static int rtl8190_tx_slowPath(struct rtl8190_priv *priv, struct sk_buff *skb, struct stat_info *pstat,
				struct net_device *dev, struct net_device *wdsDev, struct tx_insn *txcfg)
{
#ifdef RTL867X_CP3
	//rtl8651_romeperfEnterPoint(ROMEPERF_INDEX_SLOWSTART );
#endif

#if defined(CONFIG_RTL_WAPI_SUPPORT)
	if ((pstat && pstat->wapiInfo
		&& (pstat->wapiInfo->wapiType!=wapiDisable)
		&& skb->protocol!=ETH_P_WAPI
		&& (!pstat->wapiInfo->wapiUCastTxEnable)))
	{
		{
			rtl8190_tx_xmitSkbFail(priv, skb, dev, wdsDev, txcfg);
			goto stop_proc;
		}
	}
#endif

	if (txcfg->aggre_en < FG_AGGRE_MSDU_FIRST) {
		txcfg->q_num = BE_QUEUE;	// using low queue for data queue
		skb->cb[1] = 0;
	}
	txcfg->fr_type = _SKB_FRAME_TYPE_;
	txcfg->pframe = skb;

	if ((txcfg->aggre_en == FG_AGGRE_MSDU_MIDDLE) || (txcfg->aggre_en == FG_AGGRE_MSDU_LAST))
		txcfg->phdr = NULL;
	else
	{
		txcfg->phdr = (UINT8 *)get_wlanllchdr_from_poll(priv);

		if (txcfg->phdr == NULL) {
			DEBUG_ERR("Can't alloc wlan header!\n");
			rtl8190_tx_xmitSkbFail(priv, skb, dev, wdsDev, txcfg);
			goto stop_proc;
		}

		if (skb->len > priv->pmib->dot11OperationEntry.dot11RTSThreshold)
			txcfg->retry = priv->pmib->dot11OperationEntry.dot11LongRetryLimit;
		else
			txcfg->retry = priv->pmib->dot11OperationEntry.dot11ShortRetryLimit;

		memset((void *)txcfg->phdr, 0, sizeof(struct wlanllc_hdr));

#ifdef CONFIG_RTK_MESH
		if(txcfg->is_11s)
		{
			SetFrameSubType(txcfg->phdr, WIFI_11S_MESH);
			SetToDs(txcfg->phdr);
		}
		else
#endif
#ifdef SEMI_QOS
		if ((pstat) && (QOS_ENABLE) && (pstat->QosEnabled))
			SetFrameSubType(txcfg->phdr, WIFI_QOS_DATA);
		else
#endif
		SetFrameType(txcfg->phdr, WIFI_DATA);

		if (OPMODE & WIFI_AP_STATE) {
			SetFrDs(txcfg->phdr);
#ifdef WDS
			if (wdsDev)
				SetToDs(txcfg->phdr);
#endif
		}
#ifdef CLIENT_MODE
		else if (OPMODE & WIFI_STATION_STATE)
			SetToDs(txcfg->phdr);
		else if (OPMODE & WIFI_ADHOC_STATE)
			/* toDS=0, frDS=0 */;
#endif
		else
			DEBUG_WARN("non supported mode yet!\n");
	}

	if (rtl8190_wlantx(priv, txcfg) == CONGESTED)
	{
		rtl8190_tx_xmitSkbFail(priv, skb, dev, wdsDev, txcfg);
		goto stop_proc;
	}

#ifdef RTL867X_CP3
	//rtl8651_romeperfExitPoint(ROMEPERF_INDEX_SLOWSTART );
#endif
	/* Reply caller function : process done successfully */
	return TX_PROCEDURE_CTRL_SUCCESS;

stop_proc:

#ifdef RTL867X_CP3
	//rtl8651_romeperfExitPoint(ROMEPERF_INDEX_SLOWSTART );
#endif
	/* Reply caller function : STOP process */
	return TX_PROCEDURE_CTRL_STOP;
}


// for data xmit, use DRAM to prepare xmiting config
//__DRAM_FASTEXTDEV struct tx_insn static_tx_insn;
int __rtl8190_start_xmit(struct sk_buff*skb, struct net_device *dev, int tx_fg);

#ifdef PKT_PROCESSOR
int rtl8190_start_xmit_from_cpu(struct sk_buff *skb, struct net_device *dev)
{
	skb->fcpu=1;
	return rtl8190_start_xmit(skb,dev);
}
#endif

#if !defined(RTL8192SU)
#ifndef __LINUX_2_6__
__IRAM_IN_865X_HI
#endif
#else
__IRAM_WIFI_PRI5
#endif
int rtl8190_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	unsigned long x;
	int ret;
#if 0 //defined PKT_PROCESSOR
	if((skb->fcpu==0)&&((unsigned int)skb->pptx==0))
	{
		printk("nerver happen...\n");
		dump_all_msg();
		BUG();
	}
#endif

#ifdef RTL8190_FASTEXTDEV_FUNCALL
	rtl865x_extDev_pktFromProtocolStack(skb);
#endif

	SAVE_INT_AND_CLI(x);

	ret = __rtl8190_start_xmit(skb, dev, TX_NORMAL);

	RESTORE_INT(x);
	return ret;
}


#ifdef SUPPORT_TX_AMSDU
#if defined(RTL8190) || defined(RTL8192E)
__IRAM_FASTEXTDEV
#endif
static int amsdu_xmit(struct rtl8190_priv *priv, struct stat_info *pstat, struct tx_insn *txcfg, int tid,
				int from_isr, struct net_device *wdsDev, struct net_device *dev)
{
	int q_num, max_size, is_first=1, total_len=0, total_num=0;
	struct sk_buff *pskb;
	unsigned long	flags;
	int *tx_head, *tx_tail, space=0;
	struct rtl8190_hw	*phw = GET_HW(priv);

	txcfg->pstat = pstat;
	q_num = txcfg->q_num;

#if !defined(RTL8192SU)
	tx_head = get_txhead_addr(phw, q_num);
	tx_tail = get_txtail_addr(phw, q_num);
#else
	//FIX ME!!
#endif	

	max_size = pstat->amsdu_level;

	// start to transmit queued skb
	SAVE_INT_AND_CLI(flags);
	while (skb_queue_len(&pstat->amsdu_tx_que[tid]) > 0) {
		pskb = __skb_dequeue(&pstat->amsdu_tx_que[tid]);
		if (pskb == NULL)
			break;
		total_len += (pskb->len + sizeof(struct llc_snap) + 3);

		if (is_first) {
			if (skb_queue_len(&pstat->amsdu_tx_que[tid]) > 0) {
				space = CIRC_SPACE_RTK(*tx_head, *tx_tail, NUM_TX_DESC);
				if (space < 10) {
#if !defined(RTL8192SU)
					rtl8190_tx_dsr((unsigned long)priv);
					space = CIRC_SPACE_RTK(*tx_head, *tx_tail, NUM_TX_DESC);
					if (space < 10) {
						// printk("Tx desc not enough for A-MSDU!\n");
						__skb_queue_head(&pstat->amsdu_tx_que[tid], pskb);
						RESTORE_INT(flags);
						return 0;
					}
#else
					//FIX ME!!
#endif
				}
				txcfg->aggre_en = FG_AGGRE_MSDU_FIRST;
				is_first = 0;
				total_num++;
			}
			else {
				if (!from_isr) {
					__skb_queue_head(&pstat->amsdu_tx_que[tid], pskb);
					RESTORE_INT(flags);
					return 0;
				}
				txcfg->aggre_en = 0;
			}
		}
		else if ((skb_queue_len(&pstat->amsdu_tx_que[tid]) == 0) ||
				((total_len + pstat->amsdu_tx_que[tid].next->len + sizeof(struct llc_snap) + 3) > max_size) ||
				(total_num >= (space - 4)) || // 1 for header, 1 for ICV when sw encrypt, 2 for spare
				(!pstat->is_rtl8190_sta && (total_num >= (priv->pmib->dot11nConfigEntry.dot11nAMSDUSendNum-1)))) {
			txcfg->aggre_en = FG_AGGRE_MSDU_LAST;
			total_len = 0;
			is_first = 1;
			total_num = 0;
		}
		else {
			txcfg->aggre_en = FG_AGGRE_MSDU_MIDDLE;
			total_num++;
		}

		pstat->amsdu_size[tid] -= (pskb->len + sizeof(struct llc_snap));
#ifdef MESH_AMSDU
		// totti ...// Gakki
		txcfg->llc = 0;
		if( isMeshPoint(pstat))
		{
			txcfg->is_11s = 8;
			dev = priv->mesh_dev;
			memcpy(txcfg->nhop_11s, pstat->hwaddr, MACADDRLEN);
		}
		else
			txcfg->is_11s = 0;

#endif
		rtl8190_tx_slowPath(priv, pskb, pstat, dev, wdsDev, txcfg);

	}
	RESTORE_INT(flags);

	return 1;
}


static int amsdu_timer_add(struct rtl8190_priv *priv, struct stat_info *pstat, int tid, int from_timeout)
{
	unsigned long ioaddr = priv->pshare->ioaddr;
	unsigned int now, timeout, new_timer=0;
	int setup_timer;
	int current_idx, next_idx;

	current_idx = priv->pshare->amsdu_timer_head;

	while (CIRC_CNT(current_idx, priv->pshare->amsdu_timer_tail, AMSDU_TIMER_NUM)) {
		if (priv->pshare->amsdu_timer[priv->pshare->amsdu_timer_tail].pstat == NULL) {
			priv->pshare->amsdu_timer_tail = (priv->pshare->amsdu_timer_tail + 1) & (AMSDU_TIMER_NUM - 1);
			new_timer = 1;
		}
		else
			break;
	}

	if (CIRC_CNT(current_idx, priv->pshare->amsdu_timer_tail, AMSDU_TIMER_NUM) == 0) {
		cancel_timer2(priv);
		setup_timer = 1;
	}
	else if (CIRC_SPACE(current_idx, priv->pshare->amsdu_timer_tail, AMSDU_TIMER_NUM) == 0) {
		printk("%s: %s, amsdu timer overflow!\n", priv->dev->name, __FUNCTION__ );
		return -1;
	}
	else {	// some items in timer queue
		setup_timer = 0;
		if (new_timer)
			new_timer = priv->pshare->amsdu_timer[priv->pshare->amsdu_timer_tail].timeout;
	}

	next_idx = (current_idx + 1) & (AMSDU_TIMER_NUM - 1);

	priv->pshare->amsdu_timer[current_idx].priv = priv;
	priv->pshare->amsdu_timer[current_idx].pstat = pstat;
	priv->pshare->amsdu_timer[current_idx].tid = (unsigned char)tid;
	priv->pshare->amsdu_timer_head = next_idx;

	now = RTL_R32(_TSFTR_L_);
	timeout = now + priv->pmib->dot11nConfigEntry.dot11nAMSDUSendTimeout;
	priv->pshare->amsdu_timer[current_idx].timeout = timeout;

	if (!from_timeout) {
		if (setup_timer)
			setup_timer2(priv, timeout);
		else if (new_timer) {
			if (TSF_LESS(new_timer, now))
				setup_timer2(priv, timeout);
			else
				setup_timer2(priv, new_timer);
		}
	}

	return current_idx;
}


void amsdu_timeout(struct rtl8190_priv *priv, unsigned int current_time)
{
	__DRAM_IN_865X static struct tx_insn tx_insn;
	struct stat_info *pstat;
	struct net_device *wdsDev=NULL;
	struct rtl8190_priv *priv_this=NULL;
	int tid=0, head;
	//DECLARE_TXCFG(txcfg, tx_insn);

	head = priv->pshare->amsdu_timer_head;
	while (CIRC_CNT(head, priv->pshare->amsdu_timer_tail, AMSDU_TIMER_NUM))
	{
		DECLARE_TXCFG(txcfg, tx_insn);	// will be reused in this while loop

		pstat = priv->pshare->amsdu_timer[priv->pshare->amsdu_timer_tail].pstat;
		if (pstat) {
			tid = priv->pshare->amsdu_timer[priv->pshare->amsdu_timer_tail].tid;
			priv_this = priv->pshare->amsdu_timer[priv->pshare->amsdu_timer_tail].priv;
			priv->pshare->amsdu_timer[priv->pshare->amsdu_timer_tail].pstat = NULL;
		}

		priv->pshare->amsdu_timer_tail = (priv->pshare->amsdu_timer_tail + 1) & (AMSDU_TIMER_NUM - 1);

		if (pstat) {
#ifdef WDS
			wdsDev = NULL;
			if (pstat->state & WIFI_WDS) {
				wdsDev = getWdsDevByAddr(priv, pstat->hwaddr);
				txcfg->wdsIdx = getWdsIdxByDev(priv, wdsDev);
			}
#endif
			PRI_TO_QNUM(tid, txcfg->q_num, priv->pmib->dot11OperationEntry.wifi_specific);

			if (pstat->state & WIFI_SLEEP_STATE)
				pstat->amsdu_timer_id[tid] = amsdu_timer_add(priv_this, pstat, tid, 1) + 1;
			else if (amsdu_xmit(priv_this, pstat, txcfg, tid, 1, wdsDev, priv->dev) == 0) // not finish
				pstat->amsdu_timer_id[tid] = amsdu_timer_add(priv_this, pstat, tid, 1) + 1;
			else
				pstat->amsdu_timer_id[tid] = 0;
		}
	}

	if (CIRC_CNT(priv->pshare->amsdu_timer_head, priv->pshare->amsdu_timer_tail, AMSDU_TIMER_NUM)) {
		setup_timer2(priv, priv->pshare->amsdu_timer[priv->pshare->amsdu_timer_tail].timeout);
		if (TSF_LESS(priv->pshare->amsdu_timer[priv->pshare->amsdu_timer_tail].timeout, current_time))
			printk("Setup timer2 %d too late (now %d)\n", priv->pshare->amsdu_timer[priv->pshare->amsdu_timer_tail].timeout, current_time);
	}
}


#ifndef CONFIG_RTK_MESH
static
#endif
 int amsdu_check(struct rtl8190_priv *priv, struct sk_buff *skb, struct stat_info *pstat, struct tx_insn *txcfg)
{
	int q_num;
	unsigned int priority;
	unsigned short protocol;
	int *tx_head, *tx_tail, cnt, add_timer=1;
	struct rtl8190_hw	*phw;
	unsigned long flags;
	struct net_device *wdsDev=NULL;

	protocol = ntohs(*((UINT16 *)((UINT8 *)skb->data + ETH_ALEN*2)));
#if defined(CONFIG_RTL_WAPI_SUPPORT)
	if ((protocol == 0x888e)||(protocol == ETH_P_WAPI))
#else
	if (protocol == 0x888e)
#endif
	{
		return RET_AGGRE_BYPASS;
	}


#ifdef CONFIG_RTK_MESH
	priority = get_skb_priority3(priv, skb, txcfg->is_11s);
#else
	priority = get_skb_priority(priv, skb);
#endif
	PRI_TO_QNUM(priority, q_num, priv->pmib->dot11OperationEntry.wifi_specific);

	phw = GET_HW(priv);
#if !defined(RTL8192SU)
	tx_head = get_txhead_addr(phw, q_num);
	tx_tail = get_txtail_addr(phw, q_num);

	cnt = CIRC_CNT_RTK(*tx_head, *tx_tail, NUM_TX_DESC);

#if 0
	if (cnt <= AMSDU_TX_DESC_TH)
		return RET_AGGRE_BYPASS;
#endif

	if (cnt == (NUM_TX_DESC - 1))
		return RET_AGGRE_DESC_FULL;
#else
	//FIX ME!!
#endif		

#ifdef MESH_AMSDU
	// Gakki
	if(txcfg->is_11s==0 && isMeshPoint(pstat))
	{
		return RET_AGGRE_DESC_FULL;
	}
	if (txcfg->is_11s&1)
	{
		short j, popen =  ((txcfg->mesh_header.mesh_flag &1) ? 16 : 4);
		if (skb_headroom(skb) < popen || skb_cloned(skb)) {
			struct sk_buff *skb2 = dev_alloc_skb(skb->len);
			if (skb2 == NULL) {
				printk("%s: %s, dev_alloc_skb() failed!\n", priv->mesh_dev->name, __FUNCTION__);
				return RET_AGGRE_BYPASS;
			}
         	memcpy(skb_put(skb2, skb->len), skb->data, skb->len);
#ifdef RTL8190_FASTEXTDEV_FUNCALL
			rtl865x_extDev_kfree_skb(skb, FALSE);
#else
			dev_kfree_skb_any(skb);
#endif
			skb = skb2;
			txcfg->pframe = (void *)skb;
		}
		skb_push(skb, popen);
		for(j=0; j<sizeof(struct wlan_ethhdr_t); j++)
			skb->data[j]= skb->data[j+popen];
		memcpy(skb->data+j, &(txcfg->mesh_header), popen);
	}
#endif // MESH_AMSDU


	SAVE_INT_AND_CLI(flags);
	__skb_queue_tail(&pstat->amsdu_tx_que[priority], skb);
	pstat->amsdu_size[priority] += (skb->len + sizeof(struct llc_snap));

	if ((pstat->amsdu_size[priority] >= pstat->amsdu_level) ||
			(!pstat->is_rtl8190_sta && (skb_queue_len(&pstat->amsdu_tx_que[priority]) >= priv->pmib->dot11nConfigEntry.dot11nAMSDUSendNum)))
	{
#ifdef WDS
		wdsDev = NULL;
		if (pstat->state & WIFI_WDS) {
			wdsDev = getWdsDevByAddr(priv, pstat->hwaddr);
			txcfg->wdsIdx = getWdsIdxByDev(priv, wdsDev);
		}
#endif
		// delete timer entry
		if (pstat->amsdu_timer_id[priority] > 0) {
			priv->pshare->amsdu_timer[pstat->amsdu_timer_id[priority] - 1].pstat = NULL;
			pstat->amsdu_timer_id[priority] = 0;
		}
		txcfg->q_num = q_num;
		if (amsdu_xmit(priv, pstat, txcfg, priority, 0, wdsDev, priv->dev) == 0) // not finish
			pstat->amsdu_timer_id[priority] = amsdu_timer_add(priv, pstat, priority, 0) + 1;
		else
			add_timer = 0;
	}

	if (add_timer) {
		if (pstat->amsdu_timer_id[priority] == 0)
			pstat->amsdu_timer_id[priority] = amsdu_timer_add(priv, pstat, priority, 0) + 1;
	}

	RESTORE_INT(flags);

	return RET_AGGRE_ENQUE;
}
#endif // SUPPORT_TX_AMSDU


#ifdef BUFFER_TX_AMPDU
static int ampdu_xmit(struct rtl8190_priv *priv, struct stat_info *pstat, struct net_device *dev, int tid, int from_timeout)
{
	int q_num, is_first=1, total_len=0, total_num=0;
	struct sk_buff *pskb;
	unsigned long	flags;
	int *tx_head, *tx_tail, space=0, tx_flag;
	struct rtl8190_hw	*phw = GET_HW(priv);

	PRI_TO_QNUM(tid, q_num, priv->pmib->dot11OperationEntry.wifi_specific);

	tx_head = get_txhead_addr(phw, q_num);
	tx_tail = get_txtail_addr(phw, q_num);

	// start to transmit queued skb
	SAVE_INT_AND_CLI(flags);

	while (skb_queue_len(&pstat->ampdu_tx_que[tid]) > 0) {
		pskb = __skb_dequeue(&pstat->ampdu_tx_que[tid]);
		if (pskb == NULL)
			break;

		total_len += pskb->len;

		if (is_first) {
			if (skb_queue_len(&pstat->ampdu_tx_que[tid]) > 0) { // next one is avaiable
				space = CIRC_SPACE_RTK(*tx_head, *tx_tail, NUM_TX_DESC);
				if (space < 10) {

					rtl8190_tx_dsr((unsigned long)priv);
					space = CIRC_SPACE_RTK(*tx_head, *tx_tail, NUM_TX_DESC);
					if (space < 10) {
						// printk("Tx desc not enough for A-MPDU!\n");
						__skb_queue_head(&pstat->ampdu_tx_que[tid], pskb);
						RESTORE_INT(flags);
						return 0;
					}
				}

				tx_flag = TX_AMPDU_BUFFER_FIRST;
				is_first = 0;
				total_num = 1;
			}
			else { // next one is empty
				if (!from_timeout) {
					__skb_queue_head(&pstat->ampdu_tx_que[tid], pskb);
					RESTORE_INT(flags);
					return 0;
				}
				// timeout, and only one skb, tx it as normal pkt
				tx_flag = TX_AMPDU_BUFFER_SIG;
			}
		}
		else if ((skb_queue_len(&pstat->ampdu_tx_que[tid]) == 0) ||
						(total_len > priv->pshare->rf_ft_var.dot11nAMPDUBufferMax) ||
							(space < ((total_num+1)*2 +4)) ||
							((total_num+1) >= priv->pshare->rf_ft_var.dot11nAMPDUBufferNum)) {
			tx_flag = TX_AMPDU_BUFFER_LAST;
			total_len = 0;
			is_first = 1;
			total_num++;
		}
		else {
			tx_flag = TX_AMPDU_BUFFER_MID;
			total_num++;
		}

		pstat->ampdu_size[tid] -= pskb->len;

		__rtl8190_start_xmit(pskb, dev, tx_flag);
	}
	RESTORE_INT(flags);

	return 1;
}


static int ampdu_timer_add(struct rtl8190_priv *priv, struct stat_info *pstat, int tid, int from_timeout)
{
	unsigned long ioaddr = priv->pshare->ioaddr;
	unsigned int now, timeout, new_timer=0;
	int setup_timer;
	int current_idx, next_idx;

	current_idx = priv->pshare->ampdu_timer_head;

	while (CIRC_CNT(current_idx, priv->pshare->ampdu_timer_tail, AMPDU_TIMER_NUM)) {
		if (priv->pshare->ampdu_timer[priv->pshare->ampdu_timer_tail].pstat == NULL) {
			priv->pshare->ampdu_timer_tail = (priv->pshare->ampdu_timer_tail + 1) & (AMPDU_TIMER_NUM - 1);
			new_timer = 1;
		}
		else
			break;
	}

	if (CIRC_CNT(current_idx, priv->pshare->ampdu_timer_tail, AMPDU_TIMER_NUM) == 0) {
		cancel_timer1(priv);
		setup_timer = 1;
	}
	else if (CIRC_SPACE(current_idx, priv->pshare->ampdu_timer_tail, AMPDU_TIMER_NUM) == 0) {
		printk("%s: %s, ampdu timer overflow!\n", priv->dev->name, __FUNCTION__ );
		return -1;
	}
	else {	// some items in timer queue
		setup_timer = 0;
		if (new_timer)
			new_timer = priv->pshare->ampdu_timer[priv->pshare->ampdu_timer_tail].timeout;
	}

	next_idx = (current_idx + 1) & (AMPDU_TIMER_NUM - 1);

	priv->pshare->ampdu_timer[current_idx].priv = priv;
	priv->pshare->ampdu_timer[current_idx].pstat = pstat;
	priv->pshare->ampdu_timer[current_idx].tid = (unsigned char)tid;
	priv->pshare->ampdu_timer_head = next_idx;

	now = RTL_R32(_TSFTR_L_);
	timeout = now + priv->pshare->rf_ft_var.dot11nAMPDUBufferTimeout;
	priv->pshare->ampdu_timer[current_idx].timeout = timeout;

	if (!from_timeout) {
		if (setup_timer)
			setup_timer1(priv, timeout);
		else if (new_timer) {
			if (TSF_LESS(new_timer, now))
				setup_timer1(priv, timeout);
			else
				setup_timer1(priv, new_timer);
		}
	}

	return current_idx;
}


void ampdu_timeout(struct rtl8190_priv *priv, unsigned int current_time)
{
	struct stat_info *pstat;
	struct net_device *dev_this;
	struct rtl8190_priv *priv_this=NULL;
	int tid=0, head;

	head = priv->pshare->ampdu_timer_head;
	while (CIRC_CNT(head, priv->pshare->ampdu_timer_tail, AMPDU_TIMER_NUM))
	{
		pstat = priv->pshare->ampdu_timer[priv->pshare->ampdu_timer_tail].pstat;
		if (pstat) {
			tid = priv->pshare->ampdu_timer[priv->pshare->ampdu_timer_tail].tid;
			priv_this = priv->pshare->ampdu_timer[priv->pshare->ampdu_timer_tail].priv;
			priv->pshare->ampdu_timer[priv->pshare->ampdu_timer_tail].pstat = NULL;
		}

		priv->pshare->ampdu_timer_tail = (priv->pshare->ampdu_timer_tail + 1) & (AMPDU_TIMER_NUM - 1);

		if (pstat) {
			dev_this = priv_this->dev;
#ifdef WDS
			if (pstat->state & WIFI_WDS)
				dev_this = getWdsDevByAddr(priv, pstat->hwaddr);
#endif

			if (pstat->state & WIFI_SLEEP_STATE)
				pstat->ampdu_timer_id[tid] = ampdu_timer_add(priv_this, pstat, tid, 1) + 1;
			else if (ampdu_xmit(priv_this, pstat, dev_this, tid, 1) == 0) // not finish
				pstat->ampdu_timer_id[tid] = ampdu_timer_add(priv_this, pstat, tid, 1) + 1;
			else
				pstat->ampdu_timer_id[tid] = 0;
		}
	}

	if (CIRC_CNT(priv->pshare->ampdu_timer_head, priv->pshare->ampdu_timer_tail, AMPDU_TIMER_NUM)) {
		setup_timer1(priv, priv->pshare->ampdu_timer[priv->pshare->ampdu_timer_tail].timeout);
		if (TSF_LESS(priv->pshare->ampdu_timer[priv->pshare->ampdu_timer_tail].timeout, current_time))
			printk("Setup timer2 %d too late (now %d)\n", priv->pshare->ampdu_timer[priv->pshare->ampdu_timer_tail].timeout, current_time);
	}
}


static int ampdu_check(struct rtl8190_priv *priv,  struct sk_buff *skb, struct stat_info *pstat)
{
	int q_num;
	unsigned int priority;
	unsigned short protocol;
	int *tx_head, *tx_tail, cnt, add_timer=1;
	struct rtl8190_hw	*phw;
	unsigned long flags;
	struct net_device *dev_this=priv->dev;

//DBFENTER;
	protocol = ntohs(*((UINT16 *)((UINT8 *)skb->data + ETH_ALEN*2)));
#if defined(CONFIG_RTL_WAPI_SUPPORT)
	if ((protocol == 0x888e)||(protocol == ETH_P_WAPI))
#else
	if (protocol == 0x888e)
#endif
	{
		return RET_AGGRE_BYPASS;
	}

	priority = get_skb_priority(priv, skb);
	PRI_TO_QNUM(priority, q_num, priv->pmib->dot11OperationEntry.wifi_specific);

	phw = GET_HW(priv);
	tx_head = get_txhead_addr(phw, q_num);
	tx_tail = get_txtail_addr(phw, q_num);

	cnt = CIRC_CNT_RTK(*tx_head, *tx_tail, NUM_TX_DESC);

	if (cnt == (NUM_TX_DESC - 1))
		return RET_AGGRE_DESC_FULL;

	SAVE_INT_AND_CLI(flags);

	__skb_queue_tail(&pstat->ampdu_tx_que[priority], skb);
	pstat->ampdu_size[priority] += skb->len;

	if (pstat->ampdu_size[priority] > priv->pshare->rf_ft_var.dot11nAMPDUBufferMax) {
		// delete timer entry
		if (pstat->ampdu_timer_id[priority] > 0) {
			priv->pshare->ampdu_timer[pstat->ampdu_timer_id[priority] - 1].pstat = NULL;
			pstat->ampdu_timer_id[priority] = 0;
		}

#ifdef WDS
		if (pstat->state & WIFI_WDS)
			dev_this = getWdsDevByAddr(priv, pstat->hwaddr);
#endif

		if (ampdu_xmit(priv, pstat, dev_this, priority, 0) == 0) // not finish
			pstat->ampdu_timer_id[priority] = ampdu_timer_add(priv, pstat,  priority, 0) + 1;
		else
			add_timer = 0;
	}

	if (add_timer) {
		if (pstat->ampdu_timer_id[priority] == 0)
			pstat->ampdu_timer_id[priority] = ampdu_timer_add(priv, pstat, priority, 0) + 1;
	}

	RESTORE_INT(flags);

	return RET_AGGRE_ENQUE;
}
#endif // BUFFER_TX_AMPDU


#ifdef SUPPORT_TX_MCAST2UNI
int mlcst2unicst(struct rtl8190_priv *priv, struct sk_buff *skb)
{
	struct stat_info *pstat;
	struct list_head *phead, *plist;
	struct sk_buff *newskb;
	int i, iffind = 0;
	unsigned char origDest[6];

	memcpy(origDest, skb->data, 6);

	// Filter out the packet come from ourself
	phead = &priv->asoc_list;
	plist = phead->next;
	while (phead && (plist != phead)) {
		pstat = list_entry(plist, struct stat_info, asoc_list);
		plist = plist->next;

		if (!memcmp(pstat->hwaddr, &skb->data[6], 6))
			return FALSE;
	}

	// Do multicast to unicast conversion
	phead = &priv->asoc_list;
	plist = phead->next;
	while (phead && (plist != phead)) {
		pstat = list_entry(plist, struct stat_info, asoc_list);
		plist = plist->next;

		{
#if !defined(RTL8192SU)		
			int *tx_head, *tx_tail, q_num;
			struct rtl8190_hw	*phw = GET_HW(priv);
			q_num = pstat->txcfg.q_num;
			tx_head = get_txhead_addr(phw, q_num);
			tx_tail = get_txtail_addr(phw, q_num);
#endif			
#ifdef CONFIG_RTL865X_ETH_PRIV_SKB			
			extern	int eth_skb_free_num;
#endif
			if (priv->stop_tx_mcast2uni) {
#if !defined(RTL8192SU)			
				rtl8190_tx_queueDsr(priv, q_num);
#endif				
			
				if (priv->stop_tx_mcast2uni  == 1 &&
#if !defined(RTL8192SU)
						CIRC_SPACE_RTK(*tx_head, *tx_tail, NUM_TX_DESC) > (NUM_TX_DESC*1)/4
#else
						(get_TxUrb_Pending_num(priv) <= ((NUM_TX_DESC>>2)+(NUM_TX_DESC>>3)))
#endif
				)
				{
					priv->stop_tx_mcast2uni = 0;
				}
#ifdef CONFIG_RTL865X_ETH_PRIV_SKB			
#ifdef CONFIG_RTL8196B_GW_8M
				else if (priv->stop_tx_mcast2uni  == 2 &&  eth_skb_free_num > 50) {
#else
				else if (priv->stop_tx_mcast2uni  == 2 &&  eth_skb_free_num > 100) {
#endif
					priv->stop_tx_mcast2uni = 0;
				}			
#endif			
				else {
#if defined(PKT_PROCESSOR) || defined(PRE_ALLOCATE_SKB)
					rtl_kfree_skb(priv,skb,_SKB_TX_);
#else				
					dev_kfree_skb_any(skb);
#endif
					priv->ext_stats.tx_drops++;
					DEBUG_ERR("TX DROP: %s: run out ether buffer!\n", __FUNCTION__);
					return TRUE;
				}				
			}
			else {
#if !defined(RTL8192SU)			
				if (CIRC_SPACE_RTK(*tx_head, *tx_tail, NUM_TX_DESC) < 20)
#else
				if (get_TxUrb_Pending_num(priv) > ((NUM_TX_DESC>>1)+(NUM_TX_DESC>>2)))
#endif
				{					
					priv->stop_tx_mcast2uni = 1;
#if defined(PKT_PROCESSOR) || defined(PRE_ALLOCATE_SKB)
					rtl_kfree_skb(priv,skb,_SKB_TX_);
#else
					dev_kfree_skb_any(skb);
#endif					
					priv->ext_stats.tx_drops++;
					DEBUG_ERR("TX DROP: %s: txdesc full!\n", __FUNCTION__);					
					return TRUE;
				}
			}
		}

		for (i=0; i<MAX_IP_MC_ENTRY; i++) {
			if (pstat->ipmc[i].used && !memcmp(&pstat->ipmc[i].mcmac[3], origDest+3, 3)) {
#ifdef CONFIG_RTL865X_ETH_PRIV_SKB
				extern struct sk_buff *priv_skb_copy(struct sk_buff *skb);
				newskb = priv_skb_copy(skb);
#else			
				newskb = skb_copy(skb, GFP_ATOMIC);
#endif
				if (newskb) {
					memcpy(newskb->data, pstat->hwaddr, 6);
					newskb->cb[2] = (char)0xff;			// not do aggregation
#if defined(PKT_PROCESSOR) || defined(PRE_ALLOCATE_SKB)
					newskb->fcpu=1;
#endif					
					__rtl8190_start_xmit(newskb, priv->dev, TX_NO_MUL2UNI);
				}
				else {
					DEBUG_ERR("%s: muti2unit skb_copy() failed!\n", priv->dev->name);					
					priv->stop_tx_mcast2uni = 2;	
					priv->ext_stats.tx_drops++;
					DEBUG_ERR("TX DROP: %s: run out ether buffer!\n", __FUNCTION__);
#if defined(PKT_PROCESSOR) || defined(PRE_ALLOCATE_SKB)
					rtl_kfree_skb(priv,skb,_SKB_TX_);
#else
					dev_kfree_skb_any(skb);
#endif
					return TRUE;					
				}
				iffind=1;
			}
		}
	}

	if (iffind) {
#if defined(PKT_PROCESSOR) || defined(PRE_ALLOCATE_SKB)
		rtl_kfree_skb(priv,skb,_SKB_TX_);
#else
		dev_kfree_skb_any(skb);
#endif
		return TRUE;
	}
	else
		return FALSE;
}
#endif // TX_SUPPORT_MCAST2U

#ifdef RTL8192SU
__IRAM_WIFI_PRI5 void rtk8192su_tx_dsr(struct urb *tx_urb)
{
	struct tx_desc_info	*pdescinfo = (struct tx_desc_info*)tx_urb->context;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)pdescinfo->priv;
	unsigned long	flags;
	unsigned int	restart_queue=0;
	struct rtl8190_hw	*phw=GET_HW(priv);
//	unsigned int q_num = pdescinfo->q_num;

#if 0 //verify only: show the descriptor length is different with urb transfer buffer length
	//if((usb_pipeout(tx_urb->pipe))&&(tx_urb->transfer_buffer_length>32)&&((usb_pipeendpoint(tx_urb->pipe)==6)||(usb_pipeendpoint(tx_urb->pipe)==13)))
	if(tx_urb->transfer_buffer_length>32)
	{
		unsigned char *addr=(unsigned int *)(((unsigned int)tx_urb->transfer_buffer)|0xa0000000);
		unsigned int len=(*(unsigned char*)(addr+1))*256+(*(unsigned char*)(addr));
#ifdef RTL8192SU_DBG
		unsigned char offset=(*(unsigned char*)(addr+2));
		unsigned char padding=((*(unsigned char*)(addr+7))>>2)&0x7f;
		unsigned short usePage=((len+32)>>8);
		if (((len+32)&0xff) !=0 )
			usePage++;
		priv->pshare->Use_PageNum -= usePage;
#endif

		//if((len+32!=tx_urb->transfer_buffer_length)&&(len+40 != tx_urb->transfer_buffer_length))
		if(len+offset+(padding*8)!=tx_urb->transfer_buffer_length)
		{
			printk("[cb]buf_len=%x actual=%x iplen=%x_%x ipid=%x_%x urb=%x buf=%x\n",tx_urb->transfer_buffer_length,tx_urb->actual_length,addr[68],addr[69],addr[70],addr[71],(unsigned int)tx_urb,(unsigned int)addr);
			memDump(tx_urb->transfer_buffer,128,"data");
			BUG();
		}
	}
#endif 

#ifdef RTL867X_CP3
rtl8651_romeperfEnterPoint(ROMEPERF_INDEX_8187_TX_ISR );
#endif

#ifdef WDS
	//int toWds=0;
#endif
	if (!phw)
	{
#ifdef RTL867X_CP3
		rtl8651_romeperfExitPoint(ROMEPERF_INDEX_8187_TX_ISR );
#endif	
		return;
	}

	{
		//SAVE_INT_AND_CLI(flags);
		{
			{
#if 0
				// 1. free header
				{
					if (pdescinfo->hdr_type == _SKB_FRAME_TYPE_)
					{
						//restart_queue = 1;
						printk("Bug: header type == _SKB_FRAME_TYPE_\n");
					}
					else if (pdescinfo->hdr_type == _PRE_ALLOCMEM_)
					{
						//printk("(before)mgtbuf_count=%d\n", priv->pshare->pwlanbuf_poll->count);
						//release_mgtbuf_to_poll(priv, (UINT8 *)(pdescinfo->phdr));
						//printk("(after)mgtbuf_count=%d\n", priv->pshare->pwlanbuf_poll->count);
					}
					else if (pdescinfo->hdr_type == _PRE_ALLOCHDR_)
					{
						//printk("(before)wlan_hdr_poll=%d\n", priv->pshare->pwlan_hdr_poll->count);
						release_wlanhdr_to_poll(priv, (UINT8 *)(pdescinfo->phdr));
						//printk("(after)wlan_hdr_poll=%d\n", priv->pshare->pwlan_hdr_poll->count);
					}
					else if (pdescinfo->hdr_type == _PRE_ALLOCLLCHDR_)
					{
						//printk("(before)wlanllc_hdr_count=%d\n", priv->pshare->pwlanllc_hdr_poll->count);
						release_wlanllchdr_to_poll(priv, (UINT8 *)(pdescinfo->phdr));
//						printk("(after)wlanllc_hdr_count=%d\n", priv->pshare->pwlanllc_hdr_poll->count);
					}
	 				else if (pdescinfo->hdr_type == _RESERVED_FRAME_TYPE_)
					{
						// the chained skb, no need to release memory
					}
					else
					{

					}
					// for skb buffer free
					pdescinfo->phdr = NULL;
				}
#endif
				// 2. free frame
				{
					if (pdescinfo->type == _SKB_FRAME_TYPE_)
					{
#ifdef MP_TEST
						if (OPMODE & WIFI_MP_CTX_BACKGROUND && priv->pshare->rf_ft_var.mp_specific==1) {
							struct sk_buff *skb = (struct sk_buff *)(pdescinfo->pframe);
							skb->data = skb->head;
							skb->tail = skb->data;
							skb->len = 0;
							priv->pshare->skb_tail = (priv->pshare->skb_tail + 1) & (NUM_MP_SKB - 1);
							//priv->pshare->mp_pkt_txcnt++;							
						}
						else
#endif					
						{
							struct sk_buff * skb = (struct sk_buff *)(pdescinfo->pframe);
							//msg_queue("rtk8192su_tx_dsr skb=%x data=%x urb=%x\n",(unsigned int)skb,(unsigned int)skb->data,(unsigned int)tx_urb);
							//if (pdescinfo->enc_type == _PRE_ALLOCICVHDR_)
							//{
							//	struct sk_buff * skb = (struct sk_buff *)(pdescinfo->pframe);
								//memDump(skb->data, skb->len, "TX_DSR");
							//}
#ifdef DEBUG_MEMORY_LEAK
							printk("%s %d freeskb=%x\n",__FUNCTION__,__LINE__,(unsigned int)skb);	
#endif
							if (pdescinfo->last_seg==1)
							{
								rtl_kfree_skb(pdescinfo->priv, skb, _SKB_TX_IRQ_);
								restart_queue = 1;
							}
						}

						if (pdescinfo->hdr_type == _PRE_ALLOCLLCHDR_)
						{
							//printk("(before)wlanllc_hdr_count=%d\n", priv->pshare->pwlanllc_hdr_poll->count);
							release_wlanllchdr_to_poll(priv, (UINT8 *)(pdescinfo->phdr));
	//						printk("(after)wlanllc_hdr_count=%d\n", priv->pshare->pwlanllc_hdr_poll->count);
						}
						else
						{
							printk("[%s, %d]BUG!\n", __FUNCTION__, __LINE__);
						}
					}
					else if (pdescinfo->type == _PRE_ALLOCMEM_)
					{
						//printk("(before)mgtbuf_count=%d\n", priv->pshare->pwlanbuf_poll->count);
						release_mgtbuf_to_poll(priv, (UINT8 *)(pdescinfo->pframe));
						//printk("(after)mgtbuf_count=%d\n", priv->pshare->pwlanbuf_poll->count);
						if (pdescinfo->hdr_type == _PRE_ALLOCHDR_)
						{
							//printk("(before)wlan_hdr_poll=%d\n", priv->pshare->pwlan_hdr_poll->count);
							release_wlanhdr_to_poll(priv, (UINT8 *)(pdescinfo->phdr));
							//printk("(after)wlan_hdr_poll=%d\n", priv->pshare->pwlan_hdr_poll->count);
						}
						else
						{
							printk("[%s, %d]BUG!\n", __FUNCTION__, __LINE__);
						}
					}
	 				else if (pdescinfo->type == _RESERVED_FRAME_TYPE_)
					{
						// the chained skb, no need to release memory
					}
					else
					{

					}
					// for skb buffer free
					pdescinfo->pframe = NULL;
				}
				// 3. free icv/mic
				{
					if (pdescinfo->enc_type == _PRE_ALLOCICVHDR_)
					{
						release_icv_to_poll(priv, (UINT8 *)(pdescinfo->penc));
					}
					else if (pdescinfo->enc_type == _PRE_ALLOCMICHDR_)
					{
						release_mic_to_poll(priv, (UINT8 *)(pdescinfo->penc));
					}
					else if (pdescinfo->enc_type == _RESERVED_FRAME_TYPE_)
					{
						// the chained skb, no need to release memory
					}
					else
					{

					}
					pdescinfo->penc = NULL;
				}
			}
		}
		//RESTORE_INT(flags);
	}

	if (restart_queue)
		rtl8190_tx_restartQueue(priv);

#ifdef MP_TEST
	if (((OPMODE & (WIFI_MP_STATE|WIFI_MP_CTX_BACKGROUND))==(WIFI_MP_STATE|WIFI_MP_CTX_BACKGROUND))
		&&(priv->pshare->rf_ft_var.mp_specific)) {
		if (atomic_read(&priv->tx_pending[BE_QUEUE]) < (NUM_MP_SKB>>1))
			mp_ctx(priv, (unsigned char *)"tx-isr");
	}
#endif

#ifdef RTL867X_CP3
rtl8651_romeperfExitPoint(ROMEPERF_INDEX_8187_TX_ISR );
#endif

}

void rtl8192su_tx_cmdPkt(struct urb *tx_urb, struct pt_regs *regs)
{
	struct tx_desc_info	*pdescinfo = (struct tx_desc_info*)tx_urb->context;
//	struct rtl8190_priv *priv = (struct rtl8190_priv *)pdescinfo->priv;
	//unsigned int q_num = pdescinfo->q_num;

	if(tx_urb->status != 0)
	{
	#ifdef CONFIG_RTK_VOIP
		// rock: reduce debug message
		struct rtl8190_priv *priv = (struct rtl8190_priv *)pdescinfo->priv;
		if (!IS_DRV_CLOSE(priv))
			printk("tx errors!! tx_urb->status=%d\n", tx_urb->status);		
	#else
		printk("tx errors!! tx_urb->status=%d\n", tx_urb->status);		
	#endif
	}

	kfree(pdescinfo->pframe);
	

	kfree(tx_urb->context);
	usb_free_urb(tx_urb);
}

__IRAM_WIFI_PRI5 void rtl8192su_tx_isr(struct urb *tx_urb, struct pt_regs *regs)
{
	struct tx_desc_info	*pdescinfo = (struct tx_desc_info*)tx_urb->context;
	struct rtl8190_priv *priv;
	unsigned int q_num;

	if(pdescinfo==NULL) return;

	priv = (struct rtl8190_priv *)pdescinfo->priv;
#ifdef RTL8192SU_CHECKTXHANGUP
	if (priv->pshare->init_success==0)
	{
		if ((priv->pshare->ptx_urb!=NULL)&&(pdescinfo->q_num!=BEACON_QUEUE))
		{
			if ((priv->pshare->ptx_urb!=tx_urb))
				priv->pshare->init_success=0;
			else
				priv->pshare->init_success=1;
		}
	}
#endif
	q_num = pdescinfo->q_num;
#ifdef RTL8192SU_TX_FEW_INT
	if(priv->pshare->tx_pool.own[pdescinfo->tx_pool_idx]!=1)
	{
		// 2009/5/5: patch for unlink urb
		//printk("tx urb already free %d\n",pdescinfo->tx_pool_idx);
		if(priv->pshare->tx_pool.own[pdescinfo->tx_pool_idx]==2)
			printk("reentry return [%d]\n",pdescinfo->tx_pool_idx);
		return;
	}
	priv->pshare->tx_pool.own[pdescinfo->tx_pool_idx]=2;
#endif

#ifdef RTL8192SU_FWBCN
	if (q_num!=TXCMD_QUEUE)
#endif	
		rtk8192su_tx_dsr(tx_urb);
	
	if(tx_urb->status != 0)
	{
		priv->net_stats.tx_errors++;   		//priv->stats.txmanageerr++;
		printk("tx errors!! tx_urb->status=%d\n", tx_urb->status);
#ifdef RTL8192SU_TX_FEW_INT
		if(tx_urb->status== -2)
			printk("Timeout! kill tx urb!! tx_urb->status=%d ,idx=[%d]\n",tx_urb->status,pdescinfo->tx_pool_idx);
#endif		
	}

#ifdef RTL8192SU_TX_ZEROCOPY
	if ((pdescinfo->type != _SKB_FRAME_TYPE_) || (pdescinfo->non_zcpy) )
	{
		kfree(tx_urb->transfer_buffer);
		//kfree(pdescinfo->purb);
		//if(pdescinfo->multicast) printk("free multicast tx buff: %s %d\n",__FUNCTION__,__LINE__);
		//else printk("free manager tx buff: %s %d\n",__FUNCTION__,__LINE__);
	}
#else
	kfree(pdescinfo->purb);
#endif		

#ifdef RTL8192SU_TX_FEW_INT
#else
	//TxCallBack++;
	kfree(tx_urb->context);
	//printk("%s, %d(TxCallBack=%d)\n", __FUNCTION__, __LINE__, TxCallBack);
	usb_free_urb(tx_urb);
#endif

	if (q_num==BEACON_QUEUE)
	{
		priv->ext_stats.beacon_ok++;
#ifdef RTL8192SU_FWBCN
		if (priv->pshare->intf_map!=(0x01<<7))
		{
#ifdef MBSSID
			if (IS_VAP_INTERFACE(priv))
				priv->pshare->intf_map &= ~(0x01<<(priv->vap_id+1));
			else
#endif
				priv->pshare->intf_map &= ~0x01;
		}
#endif
	}
#ifdef RTL8192SU_FWBCN
	if (q_num!=TXCMD_QUEUE)
#endif
	atomic_dec(&priv->tx_pending[q_num]);

#ifdef RTL8192SU_TX_FEW_INT
	//msg_queue("free idx[%d] urb=%x type=%d\n",pdescinfo->tx_pool_idx,(u32)tx_urb,pdescinfo->type);
	priv->pshare->tx_pool.own[pdescinfo->tx_pool_idx]=0;
#endif

}

#ifdef RTL8192SU_TX_FEW_INT
void dump_tx_urb_pool(struct rtl8190_priv *priv)
{
	int i;
	printk("TX_URB_POOL=====================================================\n");
	for(i=0;i<MAX_TX_URB;i++)
	{
		if((priv->pshare->tx_pool.own[i]!=0)||((unsigned int)priv->pshare->tx_pool.pdescinfo[i]->pframe!=0)) printk("[%d] own=%d skb=%x\n",i,priv->pshare->tx_pool.own[i],(unsigned int)priv->pshare->tx_pool.pdescinfo[i]->pframe);
	}
}

__IRAM_WIFI_PRI5 int get_tx_urb_from_pool(struct rtl8190_priv *priv,struct urb **ptx_urb,struct tx_desc_info **ppdescinfo)
{
	int j=0,skip=0;
	unsigned long	flags;

	SAVE_INT_AND_CLI(flags);
//	printk("%d %d\n",priv->pshare->tx_pool_idx,priv->pshare->tx_pool.own[priv->pshare->tx_pool_idx]);
	while(1)
	{
		j++;
		if(priv->pshare->tx_pool.own[priv->pshare->tx_pool_idx]==0)
		{
			priv->pshare->tx_pool.own[priv->pshare->tx_pool_idx]=1;
			*ptx_urb = priv->pshare->tx_pool.urb[priv->pshare->tx_pool_idx];
			*ppdescinfo =  priv->pshare->tx_pool.pdescinfo[priv->pshare->tx_pool_idx];
			memset(*ppdescinfo, 0, sizeof(struct tx_desc_info));
			((struct tx_desc_info *)(*ppdescinfo))->tx_pool_idx=priv->pshare->tx_pool_idx;
			//msg_queue("using idx[%d] urb=%x dev=%s skip=%d\n",(struct tx_desc_info *)(*ppdescinfo)->tx_pool_idx,(u32)(*ptx_urb),priv->dev->name,skip);


			priv->pshare->tx_pool_idx++;
			if(priv->pshare->tx_pool_idx==MAX_TX_URB)
				priv->pshare->tx_pool_idx=0;

#ifdef MP_TEST
			if ((OPMODE & WIFI_MP_STATE)&&(priv->pshare->rf_ft_var.mp_specific))
			{
				//(((struct urb *)(*ptx_urb))->transfer_flags) &= (~URB_NO_INTERRUPT);
			}
			else
#endif
			{
				if((priv->pshare->tx_pool_idx==0)||((priv->pshare->tx_pool_idx&0x07)==0)||(skip==1))
				{
					(((struct urb *)(*ptx_urb))->transfer_flags) &= (~URB_NO_INTERRUPT);
					skip=0;
				}
				else
				{
					(((struct urb *)(*ptx_urb))->transfer_flags) |= URB_NO_INTERRUPT;
				}
			}

			(((struct urb *)(*ptx_urb))->transfer_buffer) = NULL;
			
			break;
		}


		{
			int ret;
			struct urb *tx_urb;
			// tysu: 2009/5/5 sometimes urb nerver callback, when urb ring is wrap again, kill the non-callback urb.

			ret=usb_unlink_urb(priv->pshare->tx_pool.urb[priv->pshare->tx_pool_idx]);
			if(ret<0)
			{
				printk("unlink urb=%x idx=%d ret=%d[pending=%d]\n",(unsigned int)priv->pshare->tx_pool.urb[priv->pshare->tx_pool_idx],priv->pshare->tx_pool_idx,ret, get_TxUrb_Pending_num(priv));
				priv->pshare->tx_pool.urb[priv->pshare->tx_pool_idx]->status=0;
				rtl8192su_tx_isr(priv->pshare->tx_pool.urb[priv->pshare->tx_pool_idx],NULL);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
			 	tx_urb = usb_alloc_urb(0,GFP_ATOMIC);
#else
			 	tx_urb = usb_alloc_urb(0);
#endif
				usb_free_urb(priv->pshare->tx_pool.urb[priv->pshare->tx_pool_idx]);
				priv->pshare->tx_pool.urb[priv->pshare->tx_pool_idx]->context=NULL;
				priv->pshare->tx_pool.urb[priv->pshare->tx_pool_idx]=tx_urb;
				
			}
		}
		
		//msg_queue("skip %d skb=%x\n",priv->pshare->tx_pool_idx,priv->pshare->tx_pool.pdescinfo[priv->pshare->tx_pool_idx]->pframe);
		skip=1;
		priv->pshare->tx_pool_idx++;
		if(priv->pshare->tx_pool_idx==MAX_TX_URB)
			priv->pshare->tx_pool_idx=0;
		
		if(j>=MAX_TX_URB)
		{
			printk("tx urb full\n");
			RESTORE_INT(flags);
			return FAIL;		
		}
		
	}
	RESTORE_INT(flags);
	return SUCCESS;
}
#endif

__IRAM_WIFI_PRI5 unsigned char get_endpoint(struct rtl8190_priv *priv, unsigned char q_num)
{
	switch(q_num)
	{
		case MGNT_QUEUE:	return TxCmd_Bcn_Mgt_Hiht_PRIORITY; break;
#if 1
		case BK_QUEUE:
			return ((priv->pmib->efuseEntry.usb_epnum==USBEP_SIX) ? BK_PRIORITY:BE_PRIORITY);
			break;
		case BE_QUEUE:
			return BE_PRIORITY; 
			break;
		case VI_QUEUE:
			return ((priv->pmib->efuseEntry.usb_epnum==USBEP_SIX) ? VI_PRIORITY:VO_PRIORITY);
			break;
		case VO_QUEUE:
			return VO_PRIORITY;
			break;
#else
		case BK_QUEUE:
		case BE_QUEUE:
		case VI_QUEUE:
		case VO_QUEUE:		priv->pshare->EP_be_pkt++; return BE_PRIORITY; break;
#endif
		case HIGH_QUEUE:	return TxCmd_Bcn_Mgt_Hiht_PRIORITY; break;
		case BEACON_QUEUE:	return TxCmd_Bcn_Mgt_Hiht_PRIORITY; break;
#ifdef RTL8192SU_FWBCN
		case TXCMD_QUEUE:	return TxCmd_Bcn_Mgt_Hiht_PRIORITY; break;
#endif
		default: printk("error priority!!!\n");
				return BE_PRIORITY;
	}
}

extern int check_align(u32* tx);
extern int Check_USBPatch(unsigned int* tx, unsigned int urb_len, unsigned char en_pktoffset);
extern int Check1kboundary(unsigned int* tx, unsigned int urb_len);
extern int USB_PATCH(unsigned int* tx, unsigned int* urb_len, unsigned char* en_pktoffset);

__IRAM_WIFI_PRI5 int RTL8192SU_submit_urb(struct rtl8190_priv *priv, struct urb *tx_urb)
{
	int 			status;
	unsigned long	flags;

#if 0
	unsigned char *addr;
	addr=(unsigned char *)((unsigned int)tx_urb->transfer_buffer|0xa0000000);
	if(((int)*(unsigned char*)addr+(int)(*(unsigned char*)(addr+1))*256)!=tx_urb->transfer_buffer_length-32)
	{
		printk("length is diff , submit len=0x%x\n",tx_urb->transfer_buffer_length-32);
		memDump(tx_urb->transfer_buffer,64,"pkt");
	}
	dma_cache_wback_inv((unsigned long)tx_urb->transfer_buffer,tx_urb->transfer_buffer_length);
#endif

#ifdef RTL867X_CP3
rtl8651_romeperfEnterPoint(ROMEPERF_INDEX_USB_SUBMIT_URB_TX );
#endif
	SAVE_INT_AND_CLI(flags);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	status = usb_submit_urb(tx_urb, GFP_ATOMIC);
#else
	status = usb_submit_urb(tx_urb);
#endif
	RESTORE_INT(flags);
#ifdef 	LOOPBACK_NORMAL_TX_MODE
	priv->ext_stats.loopback_TX_cnt++;
#endif
#ifdef RTL867X_CP3
rtl8651_romeperfExitPoint(ROMEPERF_INDEX_USB_SUBMIT_URB_TX );
#endif

	return status;
}

void rtl8192su_siginTx_fail(struct rtl8190_priv *priv, struct tx_insn* txcfg, int wlanhdr)
{
	if (txcfg->fr_type == _SKB_FRAME_TYPE_)
	{
#ifdef DEBUG_MEMORY_LEAK
		struct sk_buff *skb;
		skb=(struct sk_buff *)txcfg->pframe;
		printk("%s %d freeskb=%x\n",__FUNCTION__,__LINE__,(unsigned int)txcfg->pframe);	
#endif
		rtl_kfree_skb(priv, (struct sk_buff *)txcfg->pframe, _SKB_TX_);

		if (wlanhdr==0)
			release_wlanllchdr_to_poll(priv, txcfg->phdr);
		else
			release_wlanhdr_to_poll(priv, txcfg->phdr);
	}
	else if (txcfg->fr_type == _PRE_ALLOCMEM_)
	{
		release_mgtbuf_to_poll(priv, txcfg->pframe);
		release_wlanhdr_to_poll(priv, txcfg->phdr);
	}
}

#ifdef RTL8192SU_TXPKT
void rtl8192SU_one_txdesc(struct rtl8190_priv *priv, unsigned int offset, unsigned int frLen)//, struct tx_insn* txcfg)
{
	//struct _TX_DESC_8192SE *phdesc, *pdesc, *pndesc, *picvdesc=NULL, *pmicdesc, *pfrstdesc;
	unsigned int *tx;
	struct tx_desc		*pdesc=NULL;
	struct tx_desc_info			*pdescinfo=NULL;
	unsigned int 		fr_len=frLen-32, tx_len, i, div, mod, duration, keyid=0;
	int					q_num;
	unsigned int		short_preamble, usbTxBugoffset=offset;
	unsigned char		*da, *pbuf, *pwlhdr, *pmic=NULL, *picv=NULL, q_select = priv->pmib->txpktEntry.use_queue;
	priority_t			priority=BULK_PRIORITY;
	
	int status;
	struct urb *tx_urb=NULL;
	int urb_len;

allocate_urb:
	urb_len = (4*8)+fr_len+usbTxBugoffset;

	if(0 == urb_len%512)
	{
		urb_len += 1;
	}
	{
		tx = (unsigned int*)kmalloc(urb_len, GFP_KERNEL);
		if(!tx) return;
	}

	printk("tx address:%x\n", tx);
	memset(tx, 0, urb_len);


#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
 	tx_urb = usb_alloc_urb(0,GFP_ATOMIC);
#else
 	tx_urb = usb_alloc_urb(0);
#endif

	if(!tx_urb){
		kfree(tx);
		return -ENOMEM;
	}

	//for(i=0/*, pfrstdesc= phdesc + (*tx_head)*/; i < txcfg->frg_num; i++)
	{
		/*------------------------------------------------------------*/
		/*           fill descriptor of header + iv + llc             */
		/*------------------------------------------------------------*/
		//pdesc   	= (struct tx_desc*)(((u8*)tx)+usbTxBugoffset);
		pdesc   	= (((unsigned char*)tx)+usbTxBugoffset);
#if 0
		//printk("[%x][%x]\n", ((unsigned int)pdesc), ((unsigned int)pdesc)&0x3ff);
		if ((((unsigned int)pdesc)&0x3ff) != 0)
		{
			unsigned int pkt_len=(((unsigned int)pdesc)+frLen)-((unsigned int)pdesc);
			unsigned int over_size=0;
			//printk("%s, %d\n", __FUNCTION__, __LINE__);
			//printk("urb_len=%d, usbTxBugoffset=%d, fr_len=%d, pkt_len=%d, [%x]\n", urb_len, usbTxBugoffset, fr_len, pkt_len, (((unsigned int)pdesc)+frLen));
			//printk("[%x][%x]\n", ((((unsigned int)pdesc)+frLen)&0x00000f00), (((unsigned int)pdesc)&0x00000f00));
			if ((((((unsigned int)pdesc)+frLen)&0x00000f00)-(((unsigned int)pdesc)&0x00000f00))>= 1024) // over 1k boundary
			{
				over_size=(((unsigned int)pdesc)+frLen)&0x3ff;
				//printk("%s, %d[over_size=%d]\n", __FUNCTION__, __LINE__, over_size);
				//printk("%s, %d[%d], over_size=%x[%x][%x]\n", __FUNCTION__, __LINE__, pkt_len % 64, over_size, (((unsigned int)pdesc)+frLen),((((unsigned int)pdesc)+frLen)&0x00000f00));
				if (over_size>=5&&over_size<=7)
				{
					//printk("====================\nover 1k boundary!\n");
					usbTxBugoffset=0;
					kfree(tx);
					goto allocate_urb;
				}
			}

		}
		else if ((((unsigned int)(tx))&3) != 0) // 4N
		{
			//printk("====================\n4N error!\n");
			usbTxBugoffset=0;
			kfree(tx);
			goto allocate_urb;
		}
#endif
		pdescinfo = kmalloc(sizeof(struct tx_desc_info), GFP_ATOMIC);
		if(!pdescinfo) return -ENOMEM;
		memset(pdescinfo, 0, sizeof(struct tx_desc_info));

		pdescinfo->pframe = tx;

#ifndef DISABLE_UNALIGN_TRAP
		pdesc->Dword0 |= set_desc(32 << TX_OFFSETSHIFT); // tx_desc size
		//pdesc->Dword0 |= set_desc((fr_len + (get_desc(pdesc->Dword7) & TX_TxBufferSizeMask)) << TX_PktSizeSHIFT);
		pdesc->Dword0 |= set_desc(fr_len << TX_PktSizeSHIFT);
		pdesc->Dword0 |= set_desc(TX_FirstSeg);
		pdesc->Dword0 |= set_desc(TX_LastSeg);
		pdesc->Dword0 |= set_desc(TX_OWN);

		pdesc->Dword1 |= set_desc((q_select & TX_QueueSelMask)<< TX_QueueSelSHIFT);

		//pdesc->Dword4 |= set_desc(TX_UserRate);
		pdesc->Dword4 |= set_desc(TX_TXHT);
		//pdesc->Dword5 |= set_desc(TX_DISFB);
		pdesc->Dword5 |= set_desc((0x13 & TX_TxRateMask) << TX_TxRateSHIFT);
		pdesc->Dword5 |= set_desc((0x1f) << TX_DataRateFBLmtSHIFT);
#else
		OR_EQUAL(&pdesc->Dword0,set_desc(32 << TX_OFFSETSHIFT)); // tx_desc size
		//pdesc->Dword0 |= set_desc((fr_len + (get_desc(pdesc->Dword7) & TX_TxBufferSizeMask)) << TX_PktSizeSHIFT);
		OR_EQUAL(&pdesc->Dword0,set_desc(fr_len << TX_PktSizeSHIFT));
		OR_EQUAL(&pdesc->Dword0,set_desc(TX_FirstSeg));
		OR_EQUAL(&pdesc->Dword0,set_desc(TX_LastSeg));
		OR_EQUAL(&pdesc->Dword0,set_desc(TX_OWN));

		OR_EQUAL(&pdesc->Dword1,set_desc((q_select & TX_QueueSelMask)<< TX_QueueSelSHIFT));

		//pdesc->Dword4 |= set_desc(TX_UserRate);
		OR_EQUAL(&pdesc->Dword4,set_desc(TX_TXHT));
		//pdesc->Dword5 |= set_desc(TX_DISFB);
		OR_EQUAL(&pdesc->Dword5,set_desc((0x13 & TX_TxRateMask) << TX_TxRateSHIFT));
		OR_EQUAL(&pdesc->Dword5,set_desc((0x1f) << TX_DataRateFBLmtSHIFT));
#endif
	}
	{
		unsigned char *txbuff=((unsigned char *)pdesc)+32;
		int count=0;
		//memDump(tx, 32, "tx");
		memset(txbuff, 0, fr_len);
		txbuff[0]=0x08;
		txbuff[1]=0x02;
		txbuff[2]=0x3A;
		txbuff[3]=0x00;

		txbuff[4]=0xff;
		txbuff[5]=0xff;
		txbuff[6]=0xff;
		txbuff[7]=0xff;
		txbuff[8]=0xff;
		txbuff[9]=0xff;

#if 0
		txbuff[10]=0xff;
		txbuff[11]=0xff;
		txbuff[12]=0xff;
		txbuff[13]=0xff;
		txbuff[14]=0xff;
		txbuff[15]=0xff;

		txbuff[16]=0xff;
		txbuff[17]=0xff;
		txbuff[18]=0xff;
		txbuff[19]=0xff;
		txbuff[20]=0xff;
		txbuff[21]=0xff;
#else
		txbuff[10]=priv->pmib->dot11StationConfigEntry.dot11Bssid[0];
		txbuff[11]=priv->pmib->dot11StationConfigEntry.dot11Bssid[1];
		txbuff[12]=priv->pmib->dot11StationConfigEntry.dot11Bssid[2];
		txbuff[13]=priv->pmib->dot11StationConfigEntry.dot11Bssid[3];
		txbuff[14]=priv->pmib->dot11StationConfigEntry.dot11Bssid[4];
		txbuff[15]=priv->pmib->dot11StationConfigEntry.dot11Bssid[5];

		txbuff[16]=priv->pmib->dot11StationConfigEntry.dot11Bssid[0];
		txbuff[17]=priv->pmib->dot11StationConfigEntry.dot11Bssid[1];
		txbuff[18]=priv->pmib->dot11StationConfigEntry.dot11Bssid[2];
		txbuff[19]=priv->pmib->dot11StationConfigEntry.dot11Bssid[3];
		txbuff[20]=priv->pmib->dot11StationConfigEntry.dot11Bssid[4];
		txbuff[21]=priv->pmib->dot11StationConfigEntry.dot11Bssid[5];
#endif

		txbuff[22]=0x00;
		txbuff[23]=0x00;

		txbuff[24]=0xaa;
		txbuff[25]=0xaa;
		txbuff[26]=0x03;
		txbuff[27]=0x00;
		txbuff[28]=0x00;
		txbuff[29]=0x00;
		txbuff[30]=0x33;
		txbuff[31]=0x34;

		txbuff[32]=0x35;
		txbuff[33]=0x36;
		txbuff[34]=0x37;
		txbuff[35]=0x38;
		txbuff[36]=0x39;
		txbuff[37]=0x3a;
		txbuff[38]=0x3b;
		txbuff[39]=0x3c;

		txbuff[40]=0x3d;
		txbuff[41]=0x3e;
		txbuff[42]=0x3f;
		txbuff[43]=0x5f;
		txbuff[44]=0x5f;
		txbuff[45]=0x5f;
		txbuff[46]=0x5f;
		txbuff[47]=0x5f;

		txbuff[48]=0x5f;
		txbuff[49]=0x5f;
		txbuff[50]=0x5f;
		txbuff[51]=0x5f;
		txbuff[52]=0x5f;
		txbuff[53]=0x5f;
		txbuff[54]=0x5f;
		txbuff[55]=0x5f;

		txbuff[56]=0x5f;
		txbuff[57]=0x5f;
		txbuff[58]=0x5f;
		txbuff[59]=0x5f;
		txbuff[60]=0x5f;
		txbuff[61]=0x5f;
		txbuff[62]=0x5f;
		txbuff[63]=0x5f;

		txbuff[64]=0x5f;
		txbuff[65]=0x5f;
		txbuff[66]=0x5f;
		txbuff[67]=0x5f;
		txbuff[68]=0x5f;
		txbuff[69]=0x5f;
		txbuff[70]=0x5f;
		txbuff[71]=0x5f;
		if (fr_len>71)
		{
			int i;
			for (i=72; i<fr_len; i++)
			{
				txbuff[i]=i%0xff;
			}
		}

#if 1
		txbuff[fr_len-15]=0x11;
		txbuff[fr_len-14]=0x22;
		txbuff[fr_len-13]=0x33;
		txbuff[fr_len-12]=0x44;
		txbuff[fr_len-11]=0x55;
		txbuff[fr_len-10]=0x66;
		txbuff[fr_len-9]=0x77;
		txbuff[fr_len-8]=0x88;
		txbuff[fr_len-7]=0x99;
		txbuff[fr_len-6]=0xaa;
		txbuff[fr_len-5]=0xbb;
		txbuff[fr_len-4]=0xcc;
		txbuff[fr_len-3]=0xdd;
		txbuff[fr_len-2]=0xee;
		txbuff[fr_len-1]=0xff;
#endif
		memDump(pdesc, 32, "pdesc");
		memDump(txbuff, fr_len, "txbuff");
	}
	delay_ms(100);
	priority=get_endpoint(priv, priv->pmib->txpktEntry.use_queue);
	{
		unsigned int submit_size=urb_len-usbTxBugoffset;
		//printk(KERN_WARNING "Tx packet use by submit urb!\n");
		/* FIXME check what EP is for low/norm PRI */
		if((0 == submit_size%512))
		{
			printk("%s, %d\n", __FUNCTION__, __LINE__);
			submit_size += 1;
		}
		usb_fill_bulk_urb(tx_urb,priv->udev,
				usb_sndbulkpipe(priv->udev,priority), (((unsigned char*)tx)+usbTxBugoffset),
				submit_size, (usb_complete_t)rtl8192su_tx_cmdPkt, (void *)pdescinfo);
		printk("urb_len=%d, urb_len-usbTxBugoffset=%d\n", urb_len, urb_len-usbTxBugoffset);

		//if (txcfg->fr_type == _SKB_FRAME_TYPE_)
		{
			//memDump(((struct sk_buff *)(pdescinfo->pframe))->data, 32, "pdescinfo->pframe");
			//memDump(tx,fr_len+32,"data");
		}
	}

	//dma_cache_wback_inv((unsigned long)tx,urb_len);
	rtl_cache_sync_wback(priv, (unsigned int)tx, urb_len, 0);

	status = RTL8192SU_submit_urb(priv, tx_urb);

	if (!status){
	}else{
		printk("Error TX URB ,error %d\n", status);
	}

}
#endif
#endif	//RTL8192SU

#if	defined(RTL8192SE) || defined(RTL8192SU)
#if !defined(RTL8192SU)
#ifndef __LINUX_2_6__
__MIPS16
#endif
__IRAM_FASTEXTDEV
#endif
static void rtl8192SE_fill_fwinfo(struct rtl8190_priv *priv, struct tx_insn* txcfg, struct tx_desc  *pdesc, unsigned int frag_idx)
{
//	struct FWtemplate *txfw = (struct FWtemplate *)ptxfw;
	char n_txshort = 0, bg_txshort = 0;
	int erp_protection = 0, n_protection = 0;
	unsigned char rate;
//	unsigned char ampdu_des;
//	static unsigned short multibss_sg_old = 0x1234;
	unsigned char txRate = 0;

//	memset(txfw, 0, sizeof(struct FWtemplate)); // initialize to zero

#ifdef MP_TEST
#if !defined(RTL8192SU)
	unsigned long ioaddr = priv->pshare->ioaddr;
#endif
	if (OPMODE & WIFI_MP_STATE) {
		if (is_MCS_rate(txcfg->tx_rate)) {	// HT rates
			txRate = txcfg->tx_rate & 0x7f;
			txRate += 12; // for 92SE
			pdesc->Dword4 |= set_desc(TX_TXHT);
		}
		else{
			txRate = get_rate_index_from_ieee_value((UINT8)txcfg->tx_rate);
		}

		if (priv->pshare->CurrentChannelBW) {
			pdesc->Dword4 |= set_desc(TX_TxBw);
			pdesc->Dword4 |= set_desc(3 << TX_TXSCSHIFT);
			if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor40M && IS_ROOT_INTERFACE(priv))
				n_txshort = 1;
		}
		else {
			if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor20M && IS_ROOT_INTERFACE(priv))
				n_txshort = 1;
		}

		if (txcfg->retry) {
			pdesc->Dword2 |= set_desc(TX_RetryLmtEn);
			pdesc->Dword2 |= set_desc((txcfg->retry & TX_DataRetryLmtMask) << TX_DataRetryLmtSHIFT);
		}

		if(n_txshort == 1){ // short GI, modify the rate
			unsigned short max_sg_rate, multibss_sg;
			max_sg_rate = (txRate-12) & 0xf;
			txRate = 0x1c; // MAX is MSC 15
			multibss_sg = max_sg_rate | (max_sg_rate<<4) | (max_sg_rate<<8) | (max_sg_rate<<12);
#if defined(RTL8192SU)&&defined(MP_TEST)
			if (priv->pshare->rf_ft_var.mp_specific)
			{
				if (priv->pshare->multibss_sg != multibss_sg)
				{
					printk("Short GI, rate: %x\n", multibss_sg);
					RTL_W16(SG_RATE, multibss_sg); // real rate we want to send
					if (priv->pshare->rf_ft_var.mp_specific)
						priv->pshare->multibss_sg=multibss_sg;
				}
			}
			else
#endif
			if (RTL_R16(SG_RATE) != multibss_sg)
			{
				printk("Short GI, rate: %x\n", multibss_sg);
				RTL_W16(SG_RATE, multibss_sg); // real rate we want to send
			}
		}

		txcfg->tx_rate = txRate;
		pdesc->Dword5 |= set_desc((txRate & TX_TxRateMask) << TX_TxRateSHIFT);

		return;
	}
#endif

	if (is_MCS_rate(txcfg->tx_rate))	// HT rates
	{
		txRate = txcfg->tx_rate & 0x7f;
		txRate += 12; // for 92SE
#ifndef DISABLE_UNALIGN_TRAP
		pdesc->Dword4 |= set_desc(TX_TXHT);
#else
		OR_EQUAL(&pdesc->Dword4 , set_desc(TX_TXHT));
#endif

		if (priv->pshare->is_40m_bw) {
//			if (txcfg->pstat && (txcfg->pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_))) {
			if (txcfg->pstat && (txcfg->pstat->tx_bw == HT_CHANNEL_WIDTH_20_40)
#ifdef WIFI_11N_2040_COEXIST
				&& !((OPMODE & WIFI_AP_STATE) && priv->pshare->rf_ft_var.coexist_enable && 
				(priv->bg_ap_timeout || priv->force_20_sta || priv->switch_20_sta))
#endif
				) {
#ifndef DISABLE_UNALIGN_TRAP
				pdesc->Dword4 |= set_desc(TX_TxBw);
				pdesc->Dword4 |= set_desc(0 << TX_TXSCSHIFT);
#else
				OR_EQUAL(&pdesc->Dword4 , set_desc(TX_TxBw));
				OR_EQUAL(&pdesc->Dword4 , set_desc(0 << TX_TXSCSHIFT));
#endif

				if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor40M &&
					txcfg->pstat && (txcfg->pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SHORTGI_40M_))
					&& IS_ROOT_INTERFACE(priv))
					n_txshort = 1;
			}
			else {
				if (priv->pshare->offset_2nd_chan == HT_2NDCH_OFFSET_BELOW)
#ifndef DISABLE_UNALIGN_TRAP
					pdesc->Dword4 |= set_desc(2 << TX_TXSCSHIFT);
#else
					OR_EQUAL(&pdesc->Dword4 , set_desc(2 << TX_TXSCSHIFT));
#endif
				else
#ifndef DISABLE_UNALIGN_TRAP
					pdesc->Dword4 |= set_desc(1 << TX_TXSCSHIFT);
#else
					OR_EQUAL(&pdesc->Dword4 , set_desc(1 << TX_TXSCSHIFT));
#endif

				if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor20M &&
					txcfg->pstat && (txcfg->pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SHORTGI_20M_))
				 && IS_ROOT_INTERFACE(priv))
					n_txshort = 1;
			}
		}
		else {
			if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor20M &&
				txcfg->pstat && (txcfg->pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SHORTGI_20M_))
				&& IS_ROOT_INTERFACE(priv))
				n_txshort = 1;
		}

		if ((txcfg->aggre_en >= FG_AGGRE_MPDU) && (txcfg->aggre_en <= FG_AGGRE_MPDU_BUFFER_LAST)) {

#ifndef DISABLE_UNALIGN_TRAP
			pdesc->Dword2 |= set_desc(TX_AggEn);
#else
			OR_EQUAL(&pdesc->Dword2 , set_desc(TX_AggEn));
#endif
			//set Break
			if(txcfg->pstat != priv->pshare->CurPstat){
#ifndef DISABLE_UNALIGN_TRAP
				pdesc->Dword2 |= set_desc(TX_BK);
#else
				OR_EQUAL(&pdesc->Dword2 , set_desc(TX_BK));
#endif
				priv->pshare->CurPstat = txcfg->pstat;
			}
			/*
			switch (priv->pmib->dot11nConfigEntry.dot11nAMPDUSendSz) {
			case 8:
				txfw->rxMF = 0;
				break;
			case 16:
				txfw->rxMF = 1;
				break;
			case 32:
				txfw->rxMF = 2;
				break;
			case 64:
				txfw->rxMF = 3;
				break;
			default:
				if (txcfg->pstat->is_rtl8190_sta) {
					txfw->rxMF = 3;
				}
				else {
					//txfw->rxMF = txcfg->pstat->ht_cap_buf.ampdu_para & 0x03;
					if ((txcfg->pstat->ht_cap_buf.ampdu_para & 0x03) > 0)
						txfw->rxMF = 1;		// default 16K of AMPDU size to other clients support more than 8K
					else
						txfw->rxMF = 0;		// default 8K of AMPDU size to other clients support 8K only
				}
				break;
			}
			ampdu_des = (txcfg->pstat->ht_cap_buf.ampdu_para & _HTCAP_AMPDU_SPC_MASK_) >> _HTCAP_AMPDU_SPC_SHIFT_;
			if ((ampdu_des > 0) && (ampdu_des < 7))
				ampdu_des++; // 8190 Spec doesn't fit to 802.11n
			if (priv->pshare->is_40m_bw && txcfg->pstat
				&& (txcfg->pstat->tx_bw == HT_CHANNEL_WIDTH_20_40)
				&& (get_sta_encrypt_algthm(priv, txcfg->pstat) == _CCMP_PRIVACY_)
				&& txcfg->pstat->dot11KeyMapping.keyInCam == TRUE)
				txfw->rxAMD = 7;
			else
				txfw->rxAMD = ampdu_des;
			*/
		}

		// for STBC
		if (priv->pmib->dot11nConfigEntry.dot11nTxSTBC &&
			txcfg->pstat && (txcfg->pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_RX_STBC_)))
#ifndef DISABLE_UNALIGN_TRAP
			pdesc->Dword4 |= set_desc(1 << TX_STBCSHIFT);
#else
			OR_EQUAL(&pdesc->Dword4 , set_desc(1 << TX_STBCSHIFT));
#endif
	}
	else	// legacy rate
	{
		txRate = get_rate_index_from_ieee_value((UINT8)txcfg->tx_rate);
		if (is_CCK_rate(txcfg->tx_rate) && (txcfg->tx_rate != 2)) {
			if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) &&
					(priv->pmib->dot11ErpInfo.longPreambleStaNum > 0))
				; // txfw->txshort = 0
			else {
				if (txcfg->pstat)
					bg_txshort = (priv->pmib->dot11RFEntry.shortpreamble) &&
									(txcfg->pstat->useShortPreamble);
				else
					bg_txshort = priv->pmib->dot11RFEntry.shortpreamble;
			}
		}

		if (priv->pshare->is_40m_bw) {
			if (priv->pshare->offset_2nd_chan == HT_2NDCH_OFFSET_BELOW)
#ifndef DISABLE_UNALIGN_TRAP
				pdesc->Dword4 |= set_desc(2 << TX_TXSCSHIFT);
#else
				OR_EQUAL(&pdesc->Dword4, set_desc(2 << TX_TXSCSHIFT));
#endif
			else
#ifndef DISABLE_UNALIGN_TRAP
				pdesc->Dword4 |= set_desc(1 << TX_TXSCSHIFT);
#else
				OR_EQUAL(&pdesc->Dword4, set_desc(1 << TX_TXSCSHIFT));
#endif
		}

		// 20080311 Bryant modified Tx power differences and omitted L-OFDM setting
		//if (!is_CCK_rate(txcfg->tx_rate) && !priv->pshare->use_default_para) {
		//	txfw->txAGCOffset = priv->pshare->legacyOFDM_pwrdiff & 0x0f;
		//	txfw->txAGCSign = (priv->pshare->legacyOFDM_pwrdiff & 0x80)? 1:0;
		//}

		if (bg_txshort)
#ifndef DISABLE_UNALIGN_TRAP
			pdesc->Dword4 |= set_desc(TX_TxShort);
#else
			OR_EQUAL(&pdesc->Dword4, set_desc(TX_TxShort));
#endif
	}

	if (txcfg->need_ack) { // unicast
		if (frag_idx == 0) {
			if ((txcfg->rts_thrshld <= get_mpdu_len(txcfg, txcfg->fr_len)) || 
				(txcfg->pstat && txcfg->pstat->is_forced_rts))
#ifndef DISABLE_UNALIGN_TRAP
				pdesc->Dword4 |=set_desc(TX_RTSEn);
#else
				OR_EQUAL(&pdesc->Dword4 , set_desc(TX_RTSEn));
#endif
			else {
				if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
					is_MCS_rate(txcfg->tx_rate) &&
					(priv->ht_protection /*|| txcfg->pstat->is_rtl8190_sta*/))
				{
					n_protection = 1;
#ifndef DISABLE_UNALIGN_TRAP
					pdesc->Dword4 |=set_desc(TX_RTSEn);
#else
					OR_EQUAL(&pdesc->Dword4 , set_desc(TX_RTSEn));
#endif
					if (priv->pmib->dot11ErpInfo.ctsToSelf)
#ifndef DISABLE_UNALIGN_TRAP
						pdesc->Dword4 |= set_desc(TX_CTS2Self);
#else
						OR_EQUAL(&pdesc->Dword4 , set_desc(TX_CTS2Self));
#endif
				}
				else if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) &&
					(!is_CCK_rate(txcfg->tx_rate)) && // OFDM mode
					priv->pmib->dot11ErpInfo.protection)
				{
					erp_protection = 1;
#ifndef DISABLE_UNALIGN_TRAP
					pdesc->Dword4 |=set_desc(TX_RTSEn);
#else
					OR_EQUAL(&pdesc->Dword4 ,set_desc(TX_RTSEn));
#endif
					if (priv->pmib->dot11ErpInfo.ctsToSelf)
#ifndef DISABLE_UNALIGN_TRAP
						pdesc->Dword4 |= set_desc(TX_CTS2Self);
#else
						OR_EQUAL(&pdesc->Dword4 , set_desc(TX_CTS2Self));
#endif
				}
				else if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
					(txcfg->pstat) && (txcfg->pstat->MIMO_ps & _HT_MIMO_PS_DYNAMIC_) &&
					is_MCS_rate(txcfg->tx_rate) && ((txcfg->tx_rate & 0x7f)>7))
				{	// when HT MIMO Dynamic power save is set and rate > MCS7, RTS is needed
#ifndef DISABLE_UNALIGN_TRAP
					pdesc->Dword4 |=set_desc(TX_RTSEn);
#else
					OR_EQUAL(&pdesc->Dword4 ,set_desc(TX_RTSEn));
#endif
				}
			}
		}
	}

	if (get_desc(pdesc->Dword4 ) & TX_RTSEn) {
		if (erp_protection)
			rate = (unsigned char)find_rate(priv, NULL, 1, 3);
		else
			rate = (unsigned char)find_rate(priv, NULL, 1, 1);

		if (is_MCS_rate(rate)) {	// HT rates
			// can we use HT rates for RTS?
		}
		else {
			unsigned int rtsTxRate  = 0;
			rtsTxRate = get_rate_index_from_ieee_value(rate);
			if (erp_protection) {
				unsigned char  rtsShort = 0;
				if (is_CCK_rate(rate) && (rate != 2)) {
					if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) &&
							(priv->pmib->dot11ErpInfo.longPreambleStaNum > 0))
						rtsShort = 0; // do nothing
					else {
						if (txcfg->pstat)
							rtsShort = (priv->pmib->dot11RFEntry.shortpreamble) &&
											(txcfg->pstat->useShortPreamble);
						else
							rtsShort = priv->pmib->dot11RFEntry.shortpreamble;
					}
				}
#ifndef DISABLE_UNALIGN_TRAP
				pdesc->Dword4 |= (rtsShort == 1)? set_desc(TX_RTSShort): 0;
#else
				OR_EQUAL(&pdesc->Dword4 , (rtsShort == 1)? set_desc(TX_RTSShort): 0);
#endif
			}
			else if (n_protection)
				rtsTxRate = get_rate_index_from_ieee_value(48);
			else {	// > RTS threshold
			}
#ifndef DISABLE_UNALIGN_TRAP
			pdesc->Dword4 |= set_desc((rtsTxRate & TX_RTSRateMask) << TX_RTSRateSHIFT);
			pdesc->Dword4 |= set_desc((0xf & TX_RTSRateFBLmtMask) << TX_RTSRateFBLmtSHIFT);
#else
			OR_EQUAL(&pdesc->Dword4 , set_desc((rtsTxRate & TX_RTSRateMask) << TX_RTSRateSHIFT));
			OR_EQUAL(&pdesc->Dword4 , set_desc((0xf & TX_RTSRateFBLmtMask) << TX_RTSRateFBLmtSHIFT));
#endif
			//8192SE Must specified BW mode while sending RTS ...
			if (priv->pshare->is_40m_bw)
#ifndef DISABLE_UNALIGN_TRAP
				pdesc->Dword4 |= set_desc(TX_RTSBW);
#else
				OR_EQUAL(&pdesc->Dword4 , set_desc(TX_RTSBW));
#endif
		}
	}

	// set TxRate
#ifdef RTL8192SE_ACUT
	if(txRate < 0x4)
			txRate = 0x4; // 6Mhz
#endif

/*
	if(txshort == 1){ // short GI, modify the rate
		unsigned int mbssidRate = 0;
		unsigned short max_sg_rate, multibss_sg;
		max_sg_rate = (txRate-12) & 0xf;
		txRate = 0x1c; // MAX is MSC 15
		multibss_sg = max_sg_rate | (max_sg_rate<<4) | (max_sg_rate<<8) | (max_sg_rate<<12);
		if (multibss_sg_old != multibss_sg)
		{
			printk("Short GI, rate: %x\n", multibss_sg);
			RTL_W16(SG_RATE, multibss_sg); // real rate we want to send
			multibss_sg_old = multibss_sg;
		}
	}
*/
	if(n_txshort == 1
#ifdef STA_EXT
		&& txcfg->pstat && txcfg->pstat->remapped_aid < FW_NUM_STAT -1
#endif
)
		txRate = 0x1c;
#ifndef DISABLE_UNALIGN_TRAP
	pdesc->Dword5 |= set_desc((txRate & TX_TxRateMask) << TX_TxRateSHIFT);
#else
	OR_EQUAL(&pdesc->Dword5 , set_desc((txRate & TX_TxRateMask) << TX_TxRateSHIFT));
#endif

	if (txcfg->need_ack) {
		//txfw->enCPUDur = 1;	// no need to count duration for broadcast & multicast pkts

		// give retry limit to management frame
		if (txcfg->q_num == MANAGE_QUE_NUM) {
#ifndef DISABLE_UNALIGN_TRAP
			pdesc->Dword2 |= set_desc(TX_RetryLmtEn);
#else
			OR_EQUAL(&pdesc->Dword2 , set_desc(TX_RetryLmtEn));
#endif
			if (GetFrameSubType(txcfg->phdr) == WIFI_PROBERSP) {
				;	// 0 no need to set
			}
#ifdef WDS
			else if ((GetFrameSubType(txcfg->phdr) == WIFI_PROBEREQ) && (txcfg->pstat->state & WIFI_WDS)) {
#ifndef DISABLE_UNALIGN_TRAP
				pdesc->Dword2 |= set_desc((2 & TX_DataRetryLmtMask) << TX_DataRetryLmtSHIFT);
#else
				OR_EQUAL(&pdesc->Dword2 , set_desc((2 & TX_DataRetryLmtMask) << TX_DataRetryLmtSHIFT));
#endif
			}
#endif
			else {
#ifndef DISABLE_UNALIGN_TRAP
				pdesc->Dword2 |= set_desc((6 & TX_DataRetryLmtMask) << TX_DataRetryLmtSHIFT);
#else
				OR_EQUAL(&pdesc->Dword2, set_desc((6 & TX_DataRetryLmtMask) << TX_DataRetryLmtSHIFT));
#endif
			}
		}
#ifdef WDS
		else if (txcfg->wdsIdx >= 0) {
			if (txcfg->pstat->rx_avarage == 0) {
#ifndef DISABLE_UNALIGN_TRAP
				pdesc->Dword2 |= set_desc(TX_RetryLmtEn);
				pdesc->Dword2 |= set_desc((3 & TX_DataRetryLmtMask) << TX_DataRetryLmtSHIFT);
#else
				OR_EQUAL(&pdesc->Dword2 , set_desc(TX_RetryLmtEn));
				OR_EQUAL(&pdesc->Dword2 , set_desc((3 & TX_DataRetryLmtMask) << TX_DataRetryLmtSHIFT));
#endif
			}
		}
#endif
	}
}

#ifdef RTL8192SU_FWBCN
//#define RTL8192SU_FWBCN_DBGMSG
static void rtl8192SU_fill_TxCmd(struct rtl8190_priv *priv, struct tx_insn* txcfg, struct tx_desc *pdesc_txcmd,unsigned char en_pktoffset)
{
	struct txcmd_hdr *pcmd_hdr=(struct txcmd_hdr*)(((unsigned char*)pdesc_txcmd)+sizeof(struct tx_desc)+(en_pktoffset<<3));
	struct tx_desc *pdesc=NULL;

	// 1. fill Host to CPU Command Header
	memset(pdesc_txcmd, 0, H2CTXCMD_DESC_LEN+H2CTXCMD_HDR_LEN+(en_pktoffset<<3));
	if (txcfg->q_num==BEACON_QUEUE)
	{
		pdesc=(struct tx_desc*)((unsigned int)pdesc_txcmd+H2CTXCMD_DESC_LEN+H2CTXCMD_HDR_LEN+(en_pktoffset<<3));
#ifdef RTL8192SU_FWBCN_DBGMSG		
		memDump(pdesc, 64, "pdesc");
#endif	
		//memset(pcmd_hdr,0,sizeof(*pcmd_hdr));
		
		//   - fill CMD LEN
		pcmd_hdr->CMD |= set_desc((get_desc(pdesc->Dword0)&TX_PktSizeMask)+sizeof(struct tx_desc));
		//   - fill Element ID
		pcmd_hdr->CMD |= set_desc((H2CTXCMD_UPDATABCN&H2CTXHDR_EIDMask)<<H2CTXHDR_EIDSHIFT);
		//   - fill MBSSID BITMAP
#ifdef MBSSID	
		if (IS_VAP_INTERFACE(priv))
		{
			pcmd_hdr->RSVD |= set_desc(((priv->vap_id+1)&H2CTXHDR_BITMAPMask)<<H2CTXHDR_BITMAPSHIFT);
		}
		else //ROOT interface
#endif	
			pcmd_hdr->RSVD |= set_desc((0&H2CTXHDR_BITMAPMask)<<H2CTXHDR_BITMAPSHIFT);
		//   - fill DTIM OFFSET
		pcmd_hdr->RSVD |= set_desc((priv->timoffset&H2CTXHDR_DTIMOffsetMask)<<H2CTXHDR_DTIMOffsetSHIFT);
	}
	else
	{
		//   - fill Element ID
		pcmd_hdr->CMD |= set_desc((H2CTXCMD_RESETMBSSID&H2CTXHDR_EIDMask)<<H2CTXHDR_EIDSHIFT);
	}

#ifdef RTL8192SU_FWBCN_DBGMSG		
	memDump(pcmd_hdr, H2CTXCMD_HDR_LEN, "pcmd_hdr");
#endif	

	// 2. fill Host to CPU Command DESC
	memset(pdesc_txcmd, 0, sizeof(struct tx_desc));
	if (txcfg->q_num==BEACON_QUEUE)
	{
		//   - fill TXPKTSIZE
		//pdesc_txcmd->Dword0 = set_desc(((sizeof(struct tx_desc))+H2CTXCMD_HDR_LEN)<<H2CTX_PktSizeSHIFT);
#ifndef DISABLE_UNALIGN_TRAP
		pdesc_txcmd->Dword0 = set_desc(((get_desc(pcmd_hdr->CMD)&H2CTX_PktSizeMask)+H2CTXCMD_HDR_LEN)<<H2CTX_PktSizeSHIFT);
#else
		SET_EQUAL(&pdesc_txcmd->Dword0 , set_desc(((get_desc(pcmd_hdr->CMD)&H2CTX_PktSizeMask)+H2CTXCMD_HDR_LEN)<<H2CTX_PktSizeSHIFT));
#endif

	}
	else
	{
		//   - fill TXPKTSIZE
#ifndef DISABLE_UNALIGN_TRAP
		pdesc_txcmd->Dword0 = set_desc(H2CTXCMD_HDR_LEN<<H2CTX_PktSizeSHIFT);
#else
		SET_EQUAL(&pdesc_txcmd->Dword0 , set_desc(H2CTXCMD_HDR_LEN<<H2CTX_PktSizeSHIFT));
#endif
	}

#ifndef DISABLE_UNALIGN_TRAP
	//   - fill Offset
	pdesc_txcmd->Dword0 |= set_desc(32 << H2CTX_OffseSHIFT); // tx_desc size
	//   - fill Queue Select
	pdesc_txcmd->Dword1 = set_desc((0x13 & H2CTX_QSELMask)<<H2CTX_QSELSHIFT);

	pdesc_txcmd->Dword0 |= set_desc(TX_FirstSeg);
	pdesc_txcmd->Dword0 |= set_desc(TX_LastSeg);
	pdesc_txcmd->Dword0 = set_desc((get_desc(pdesc_txcmd->Dword0)) | TX_OWN);
	pdesc_txcmd->Dword1 |= set_desc((en_pktoffset&TX_PktOffsetMask)<<TX_PktOffsetSHIFT);
#else
	//   - fill Offset
	OR_EQUAL(&pdesc_txcmd->Dword0 , set_desc(32 << H2CTX_OffseSHIFT)); // tx_desc size
	//   - fill Queue Select
	SET_EQUAL(&pdesc_txcmd->Dword1 , set_desc((0x13 & H2CTX_QSELMask)<<H2CTX_QSELSHIFT));
	OR_EQUAL(&pdesc_txcmd->Dword0 , set_desc(TX_FirstSeg));
	OR_EQUAL(&pdesc_txcmd->Dword0 , set_desc(TX_LastSeg));
	SET_EQUAL(&pdesc_txcmd->Dword0 , set_desc((get_desc(pdesc_txcmd->Dword0)) | TX_OWN));
	OR_EQUAL(&pdesc_txcmd->Dword1 , set_desc((en_pktoffset&TX_PktOffsetMask)<<TX_PktOffsetSHIFT));
#endif

#ifdef RTL8192SU_FWBCN_DBGMSG			
	memDump(pdesc_txcmd, H2CTXCMD_DESC_LEN, "pdesc_txcmd");	
#endif	
}


#if 0
void rtl8192SU_signin_fwtxcmd(struct rtl8190_priv *priv, struct tx_insn* txcfg)
{
	struct tx_desc  	*pdesc_txcmd;
	struct tx_desc_info	*pdescinfo;
	unsigned int 	*tx;
	int 			status;
	int				urb_len;
	priority_t		priority=BULK_PRIORITY;
	struct urb 		*tx_urb=NULL;

	urb_len = H2CTXCMD_DESC_LEN+H2CTXCMD_HDR_LEN;

	tx = kmalloc(urb_len, GFP_ATOMIC);
	if(!tx)
	{
		printk("%s, %d\n", __FUNCTION__, __LINE__);
		goto fwbcn_fail;
	}
#ifdef RTL8192SU_TX_FEW_INT
#else
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
 	tx_urb = usb_alloc_urb(0,GFP_ATOMIC);
#else
 	tx_urb = usb_alloc_urb(0);
#endif

	if(!tx_urb)
	{
		kfree(tx);
		printk("%s, %d\n", __FUNCTION__, __LINE__);
		goto fwbcn_fail;
	}
#endif
	pdesc_txcmd = (struct tx_desc*)tx;
	//clear all bits
	memset(pdesc_txcmd, 0, H2CTXCMD_DESC_LEN+H2CTXCMD_HDR_LEN);
#ifdef RTL8192SU_TX_FEW_INT
	//get_tx_urb_from_pool(priv,&tx_urb,&pdescinfo);
	if(get_tx_urb_from_pool(priv,&tx_urb,&pdescinfo) != SUCCESS)
	{
		kfree(tx);
		goto fwbcn_fail;
	}
	//memset(pdescinfo, 0, sizeof(struct tx_desc_info));
#else
	pdescinfo = kmalloc(sizeof(struct tx_desc_info), GFP_ATOMIC);
	if(!pdescinfo)
		goto fwbcn_fail;
	memset(pdescinfo, 0, sizeof(struct tx_desc_info));
#endif

	rtl8192SU_fill_TxCmd(priv, txcfg, pdesc_txcmd);

	pdescinfo->purb= tx;
	priority=get_endpoint(priv, txcfg->q_num);
	pdescinfo->q_num = txcfg->q_num;
	pdescinfo->type = _RESERVED_FRAME_TYPE_;
	pdescinfo->priv = priv;
	{
		usb_fill_bulk_urb(tx_urb,priv->udev,
				usb_sndbulkpipe(priv->udev,priority), tx,
				urb_len, (usb_complete_t)rtl8192su_tx_isr, (void *)pdescinfo);

#ifdef RTL8192SU_FWBCN_DBGMSG
			memDump(tx,urb_len,"reset beacon");
#endif
	}
	dma_cache_wback_inv((unsigned long)tx,urb_len);
	//rtl_cache_sync_wback(priv, (unsigned int)tx, urb_len, 0);

	status = RTL8192SU_submit_urb(priv, tx_urb);

	if (!status){
		//atomic_inc((priority == NORM_PRIORITY)? &priv->tx_np_pending : &priv->tx_lp_pending);
	}else{
		printk("Error TXCMD reset beacon URB ,error %d\n", status);
	}
	return;
fwbcn_fail:
}
#endif

#endif

//#endif
// I AM not sure that if our Buffersize and PKTSize is right,
// If there are any problem, fix this first
//__IRAM_IN_865X
void rtl8192SE_signin_txdesc(struct rtl8190_priv *priv, struct tx_insn* txcfg)
{
#if !defined(RTL8192SU)
	static struct tx_desc  *phdesc, *pdesc, *pndesc, *picvdesc, *pmicdesc, *pfrstdesc;
#else //RTl8192SU
	struct tx_desc  *pdesc=NULL, *pndesc, *pfrstdesc=NULL;
#ifdef RTL8192SU_FWBCN
	struct tx_desc	*pdesc_txcmd=NULL;
#endif
#endif

#if !defined(RTL8192SU)
	static struct tx_desc_info	*pswdescinfo, *pdescinfo, *pndescinfo, *picvdescinfo, *pmicdescinfo;
	static int					*tx_head, q_num;
	static unsigned long		tmpphyaddr;
	static struct rtl8190_hw	*phw;
#else //RTL8192SU
	struct tx_desc_info	*pdescinfo, *pndescinfo;
	unsigned int 		*tx;
	int		q_num, wlanhdr;
	unsigned char*		pskb_data;

#ifdef RTL8192SU_TX_ZEROCOPY
	struct sk_buff *skb=NULL;
#endif

	unsigned char	en_pktoffset=0;
	priority_t		priority=BULK_PRIORITY;
	int 			status;
	struct urb 		*tx_urb=NULL;
	unsigned int	urb_len;

#endif
	unsigned int 		fr_len, tx_len, i, keyid=0;
	unsigned char		*da, *pbuf, *pwlhdr, *pmic=NULL, *picv=NULL;
	unsigned char		 q_select=0;
#ifdef TX_SHORTCUT
	int	fit_shortcut = 0;
#endif
#ifdef RTL867X_USBPATCH
	int ref_txsc = 1;
#endif
	unsigned int		pfrst_dma_desc=0;

#if !defined(RTL8192SU)	
	unsigned int		*dma_txhead;

	static unsigned long flush_addr[20];
	static int flush_len[20];
	int flush_num=0;
	picvdesc=NULL;
#endif
	keyid=0;
	pmic=NULL;
	picv=NULL;
	q_select=0;

#ifdef RTL8192SU
	wlanhdr = 0;
	if (!IS_DRV_OPEN(priv))
	{
		printk("%s %d: free packet when intf down!\n",__FUNCTION__,__LINE__);
		if (txcfg->privacy==_TKIP_PRIVACY_)
		{
			((struct sk_buff *)txcfg->pframe)->tail-=8;
			((struct sk_buff *)txcfg->pframe)->len-=8;
		}
		goto signin_tx_fail;
	}
#endif //RTL8192SU

	if (txcfg->tx_rate == 0) {
#if !defined(LOOPBACK_MODE)
		DEBUG_ERR("tx_rate=0!\n");
#endif
		txcfg->tx_rate = find_rate(priv, NULL, 0, 1);
	}

	q_num = txcfg->q_num;

#if !defined(RTL8192SU)
	phw	= GET_HW(priv);

	dma_txhead	= get_txdma_addr(phw, q_num);
	tx_head		= get_txhead_addr(phw, q_num);
	phdesc   	= get_txdesc(phw, q_num);
	pswdescinfo = get_txdesc_info(priv->pshare->pdesc_info, q_num);
#endif

	tx_len = txcfg->fr_len;

	if (txcfg->fr_type == _SKB_FRAME_TYPE_)
		pbuf = ((struct sk_buff *)txcfg->pframe)->data;
	else
		pbuf = (unsigned char*)txcfg->pframe;

	da = get_da((unsigned char *)txcfg->phdr);

#if !defined(RTL8192SU)
	tmpphyaddr = get_physical_addr(priv, pbuf, tx_len, PCI_DMA_TODEVICE);
#endif	
#ifdef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, (unsigned int)pbuf, tx_len, PCI_DMA_TODEVICE);
#endif

	// in case of default key, then find the key id
	if (GetPrivacy((txcfg->phdr)))
	{
#ifdef WDS
		if (txcfg->wdsIdx >= 0) {
			if (txcfg->pstat)
				keyid = txcfg->pstat->keyid;
			else
				keyid = 0;
		}
		else
#endif

#ifdef __DRAYTEK_OS__
		if (!IEEE8021X_FUN)
			keyid = priv->pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex;
		else {
			if (IS_MCAST(GetAddr1Ptr ((unsigned char *)txcfg->phdr)) || !txcfg->pstat)
				keyid = priv->pmib->dot11GroupKeysTable.keyid;
			else
				keyid = txcfg->pstat->keyid;
		}
#else

		if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm==_WEP_40_PRIVACY_ ||
			priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm==_WEP_104_PRIVACY_) {
			if(IEEE8021X_FUN && txcfg->pstat) {
				if(IS_MCAST(da))
					keyid = 0;
				else
					keyid = txcfg->pstat->keyid;
			}
			else
				keyid = priv->pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex;
		}
#endif

#ifdef CONFIG_RTK_MESH
		if( txcfg->is_11s )
			keyid = priv->pmib->dot11sKeysTable.keyid;
#endif
	}
#if !defined(RTL8192SU)
	for(i=0, pfrstdesc= phdesc + (*tx_head); i < txcfg->frg_num; i++)
#else
	for(i=0; i < txcfg->frg_num; i++)
#endif
	{
		/*------------------------------------------------------------*/
		/*           fill descriptor of header + iv + llc             */
		/*------------------------------------------------------------*/
#if !defined(RTL8192SU)
		pdesc     = phdesc + (*tx_head);
		pdescinfo = pswdescinfo + *tx_head;
#else //RTl8192SU
		en_pktoffset=0;
		wlanhdr=i;
		if (txcfg->frg_num==1)
		{
			urb_len = txcfg->hdr_len+txcfg->llc+txcfg->iv+txcfg->fr_len+(4*8);
		}
		else
		{
			if (i != (txcfg->frg_num - 1))
			{
				if (i == 0) {
					urb_len = txcfg->hdr_len+txcfg->llc+txcfg->iv+(txcfg->frag_thrshld - txcfg->llc)+(4*8);
				}
				else {
					urb_len = txcfg->hdr_len+txcfg->iv+txcfg->frag_thrshld+(4*8);
				}
			}
			else	// last seg
			{
				urb_len = txcfg->hdr_len+txcfg->iv+tx_len+(4*8);
			}
		}
#ifdef 	RTL8192SU_FWBCN
		if (txcfg->q_num==BEACON_QUEUE)
			urb_len += (H2CTXCMD_DESC_LEN+H2CTXCMD_HDR_LEN);
#endif

		if (txcfg->privacy)
		{
			if (UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE)))
			{
				urb_len += txcfg->mic+txcfg->icv;
			}
			//else if (txcfg->privacy == _CCMP_PRIVACY_)
				//urb_len += txcfg->mic;
		}

		//if(txcfg->fr_type == _SKB_FRAME_TYPE_)
		//	printk("hdr_len=%d,llc=%d,iv=%d,fr_len=%d,icv=%d,mic=%d, frag_thrshld=%d, urb_len=%d\n", 
		//	txcfg->hdr_len,txcfg->llc,txcfg->iv,txcfg->fr_len,txcfg->icv,txcfg->mic, txcfg->frag_thrshld, urb_len);

#if 0// RTL8672_USB_PATCH
		{
			int rest;
			rest = urb_len&0x3f;
			if((0 == (urb_len & 0x1ff))||((rest>=3) && (rest<=5)))
			{
				en_pktoffset=1;
				urb_len += 8;
			}
		}
#endif	

#ifdef RTL8192SU_TX_ZEROCOPY
		if(txcfg->frg_num==1 && (txcfg->fr_type == _SKB_FRAME_TYPE_) && !skb_cloned((struct sk_buff *)txcfg->pframe) )
		{
			skb=((struct sk_buff *)txcfg->pframe);
			if(*((unsigned char*)skb->data-WLAN_ETHHDR_LEN)&1)  // multicast or broadcast
			{
				skb=NULL;
#ifdef RTL867X_USBPATCH
				ref_txsc=0;
#endif
				goto ALLOCTXBUFF;
			}
			else
			{
				tx = (unsigned int *)(skb->data - sizeof(struct tx_desc) - txcfg->llc - txcfg->iv - txcfg->hdr_len);
#ifdef RTL867X_USBPATCH
				if (check_align(tx) || Check1kboundary(tx, urb_len))
				{
					skb=NULL;
					ref_txsc=0;
					goto ALLOCTXBUFF;
				}
				else
#endif
				{
					en_pktoffset = Check_USBPatch(tx, urb_len, en_pktoffset);
					//if (en_pktoffset!=0)
					{
						if (skb_headroom((struct sk_buff *)txcfg->pframe) >= (txcfg->hdr_len + txcfg->llc + txcfg->iv + sizeof(struct tx_desc) + (en_pktoffset<<3)))
						{
							urb_len += (en_pktoffset<<3);
							tx = (unsigned int *)(((unsigned int)(tx))-(en_pktoffset<<3));
						}
						else
						{
							skb=NULL;
#ifdef RTL867X_USBPATCH
							ref_txsc=0;
#endif
							goto ALLOCTXBUFF;
						}
					}
				}
				// printk("skb->head=%x skb->data=%x skb->tail=%x skb->end=%x skb->len=%d\n",(unsigned int)skb->head,(unsigned int)skb->data,(unsigned int)skb->tail,(unsigned int)skb->end,(unsigned int)skb->len);
			}	
		}
		else
#endif	
		{
ALLOCTXBUFF:
			en_pktoffset=0;
			tx = NULL;
			if (0==(urb_len & 0x1ff))
			{
				en_pktoffset++;
				urb_len+=(en_pktoffset<<3);
			}
			tx = kmalloc(urb_len, GFP_ATOMIC);
			if(!tx)
			{
				DEBUG_INFO("%s, %d\n", __FUNCTION__, __LINE__);
				goto signin_tx_fail;
			}
#ifdef RTL867X_USBPATCH
			if (USB_PATCH(&tx, &urb_len, &en_pktoffset)==FAIL)
				goto signin_tx_fail;
#endif
		}
#if !defined(RTL8192SU_TX_FEW_INT)
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	 	tx_urb = usb_alloc_urb(0,GFP_ATOMIC);
#else
	 	tx_urb = usb_alloc_urb(0);
#endif

		if(!tx_urb){
#ifdef RTL8192SU_TX_ZEROCOPY
			if(txcfg->fr_type != _SKB_FRAME_TYPE_ || skb==NULL)
			{
				kfree(tx);
			}
#else
			kfree(tx);
#endif
			printk("%s, %d\n", __FUNCTION__, __LINE__);
			goto signin_tx_fail;
		}
#endif
#ifdef 	RTL8192SU_FWBCN
		if (txcfg->q_num==BEACON_QUEUE)
		{
			pdesc_txcmd = (struct tx_desc*)tx;
//			memset(pdesc_txcmd, 0, H2CTXCMD_DESC_LEN+H2CTXCMD_HDR_LEN);
			pdesc = (struct tx_desc*)(tx+((H2CTXCMD_DESC_LEN+H2CTXCMD_HDR_LEN)>>2));
			if(en_pktoffset) pdesc=(struct tx_desc*)((unsigned int)pdesc+(en_pktoffset<<3));
		}
		else
#endif
		pdesc = (struct tx_desc*)tx;
		memset(pdesc, 0, sizeof(struct tx_desc));
#ifdef RTL8192SU_TX_FEW_INT
		if(get_tx_urb_from_pool(priv,&tx_urb,&pdescinfo) != SUCCESS)
		{
#ifdef RTL8192SU_TX_ZEROCOPY
			if((txcfg->fr_type != _SKB_FRAME_TYPE_) || (skb==NULL))
			{
				kfree(tx);
			}
#else
			kfree(tx);
#endif
			goto signin_tx_fail;
		}
		//memset(pdescinfo, 0, sizeof(struct tx_desc_info));
#else
		pdescinfo = kmalloc(sizeof(struct tx_desc_info), GFP_ATOMIC);
		if(!pdescinfo){printk("%s, %d\n", __FUNCTION__, __LINE__); goto signin_tx_fail;}
		memset(pdescinfo, 0, sizeof(struct tx_desc_info));
#endif
#ifdef RTL8192SU_TX_ZEROCOPY
		if((skb==NULL)&&(txcfg->fr_type == _SKB_FRAME_TYPE_))
			pdescinfo->non_zcpy=1;
		else
		{
			pdescinfo->non_zcpy=0;
			//pbuf = pbuf+txcfg->llc+txcfg->iv+txcfg->hdr_len;
		}
#endif
#endif //RTL8192SU

		//clear all bits
		memset(pdesc, 0, 32);

		if (i != 0)
		{
			// we have to allocate wlan_hdr
			pwlhdr = (UINT8 *)get_wlanhdr_from_poll(priv);
			if (pwlhdr == (UINT8 *)NULL)
			{
				DEBUG_ERR("System-bug... should have enough wlan_hdr\n");
#if !defined(RTL8192SU)
				return;
#else
				goto signin_tx_fail;
#endif
			}
			// other MPDU will share the same seq with the first MPDU
			memcpy((void *)pwlhdr, (void *)(txcfg->phdr), txcfg->hdr_len); // data pkt has 24 bytes wlan_hdr

		}
		else
		{
#ifdef SEMI_QOS
			if (txcfg->pstat && (is_qos_data(txcfg->phdr))) {
				if ((GetFrameSubType(txcfg->phdr) & (WIFI_DATA_TYPE | BIT(6) | BIT(7))) == (WIFI_DATA_TYPE | BIT(7))) {
					unsigned char tempQosControl[2];
					memset(tempQosControl, 0, 2);
					tempQosControl[0] = ((struct sk_buff *)txcfg->pframe)->cb[1];
#ifdef WMM_APSD
					if ((APSD_ENABLE) && (txcfg->pstat) && (txcfg->pstat->state & WIFI_SLEEP_STATE) &&
						(!GetMData(txcfg->phdr)) &&
						((((tempQosControl[0] == 7) || (tempQosControl[0] == 6)) && (txcfg->pstat->apsd_bitmap & 0x01)) ||
						 (((tempQosControl[0] == 5) || (tempQosControl[0] == 4)) && (txcfg->pstat->apsd_bitmap & 0x02)) ||
						 (((tempQosControl[0] == 3) || (tempQosControl[0] == 0)) && (txcfg->pstat->apsd_bitmap & 0x08)) ||
						 (((tempQosControl[0] == 2) || (tempQosControl[0] == 1)) && (txcfg->pstat->apsd_bitmap & 0x04))))
						tempQosControl[0] |= BIT(4);
#endif
					if (txcfg->aggre_en == FG_AGGRE_MSDU_FIRST)
						tempQosControl[0] |= BIT(7);

					memcpy((void *)GetQosControl((txcfg->phdr)), tempQosControl, 2);
				}
			}
#endif

			assign_wlanseq(GET_HW(priv), txcfg->phdr, txcfg->pstat, GET_MIB(priv)
#ifdef CONFIG_RTK_MESH	// For broadcast data frame via mesh (ex:ARP requst)
				, txcfg->is_11s
#endif
				);

#ifdef RTL8192SU
#ifdef SEMI_QOS
	if ((txcfg->hdr_len==WLAN_HDR_A3_QOS_LEN || txcfg->hdr_len==WLAN_HDR_A4_QOS_LEN))
#ifndef DISABLE_UNALIGN_TRAP
		pdesc->Dword1 &= set_desc(~TX_NonQos);
#else
		AND_EQUAL(&pdesc->Dword1, set_desc(~TX_NonQos));
#endif
	else
#endif
#ifndef DISABLE_UNALIGN_TRAP
	pdesc->Dword1 |= set_desc(TX_NonQos);
#else
	OR_EQUAL(&pdesc->Dword1, set_desc(TX_NonQos));
#endif

#endif

#ifndef DISABLE_UNALIGN_TRAP
			pdesc->Dword3 |= set_desc( (GetSequence(txcfg->phdr) & TX_SeqMask)  << TX_SeqSHIFT );
#else
			OR_EQUAL(&pdesc->Dword3 , set_desc( (GetSequence(txcfg->phdr) & TX_SeqMask)  << TX_SeqSHIFT ));
#endif
//			pdesc->Dword3 |= ( (GetSequence(txcfg->phdr) & TX_SeqMask));
/*
			{
				unsigned short seqnum = GetSequence(txcfg->phdr);
				unsigned int *ppdesc = (unsigned int *)pdesc;
				printk("seqnum: %x\n", seqnum);
				pdesc->Dword3 |= set_desc((seqnum&TX_SeqMask) << TX_SeqSHIFT);
				printk("Dword3 in singin: %x\n", get_desc(*(ppdesc+3)));
				printk("Dword3 in singin, no get_desc: %x\n",*(ppdesc+3));
			}
*/

//			printk("Dword3 in singin: %x\n", get_desc(*(unsigned int *)(&pdesc->Dword3)));
			pwlhdr = txcfg->phdr;
		}
		SetDuration(pwlhdr, 0);

#ifdef RTL8192SU

#ifdef RTL8192SU_FWBCN
		if (txcfg->q_num==BEACON_QUEUE)
		{
			pskb_data=(char*)(((unsigned int)pdesc)+32);
		}
		else
#endif			
		{
			if (en_pktoffset)
			{
#ifndef DISABLE_UNALIGN_TRAP
				pdesc->Dword1 |= set_desc((en_pktoffset&TX_PktOffsetMask)<<TX_PktOffsetSHIFT);
#else
				OR_EQUAL(&pdesc->Dword1 , set_desc((en_pktoffset&TX_PktOffsetMask)<<TX_PktOffsetSHIFT));
#endif
			}		
			pskb_data=(char*)(((unsigned int)pdesc)+32+(en_pktoffset<<3));
		}

#endif //RTL8192SU

		rtl8192SE_fill_fwinfo(priv, txcfg, pdesc, i);
#if 0
		// There is no fwinfo in 8192SE desgin, use TX desc instead
		{
			struct FWtemplate *ppfwinfo =  (struct FWtemplate *)pfwinfo;
			// so far, we cannot send cck rate, use 6Mhz instead.
			// for test, use fix rate
			// 92SE 1st cut can't use CCK rate, avoid.
//			printk("txRate: %d\n", ppfwinfo->txRate);

#ifdef RTL8192SE_ACUT
			if(ppfwinfo->txRate < 0x4)
				ppfwinfo->txRate = 0x4; // 6Mhz
#endif

//			if(txcfg->pstat)
//				pdesc->Dword1 |= set_desc(txcfg->pstat->aid && TX_MACIDMask);

			pdesc->Dword5 |= set_desc((ppfwinfo->txRate & TX_TxRateMask) << TX_TxRateSHIFT);
			pdesc->Dword4 |= (ppfwinfo->txHt == 1)? set_desc(TX_TXHT): 0;
			pdesc->Dword4 |= (ppfwinfo->txbw == 1)? set_desc(TX_TxBw): 0;
			pdesc->Dword4 |= set_desc((ppfwinfo->txSC & TX_TXSCMask) << TX_TXSCSHIFT);

//			printk("txHt: %x, txBW: %x, TxSC: %x, TxRate: %x\n",ppfwinfo->txHt, ppfwinfo->txbw, ppfwinfo->txSC, ppfwinfo->txRate);

//			pdesc->Dword4 |= (ppfwinfo->txshort == 1)?set_desc(TX_TxShort): 0;
//			pdesc->Dword4 |= set_desc(TX_TxShort);
//			pdesc->Dword2 |= (ppfwinfo->aggren == 1)?set_desc(TX_AggEn): 0;
			// force aggren
			pdesc->Dword2 |= set_desc(TX_AggEn);

			//set Break
			if(txcfg->pstat != priv->pshare->CurPstat){
				pdesc->Dword2 |= set_desc(TX_BK);
				priv->pshare->CurPstat = txcfg->pstat;
			}


//			pdesc->xxx = ppfwinfo->rxMF;
//			pdesc->xxx = ppfwinfo->rxAMD;
//			pdesc->xxx = ppfwinfo->retryLimit1;
//			pdesc->xxx = ppfwinfo->retryLimit2;
			//protection related
			pdesc->Dword4 |= (ppfwinfo->rtsEn == 1)? set_desc(TX_RTSEn): 0;
			pdesc->Dword4 |= (ppfwinfo->ctsEn == 1)? set_desc(TX_CTS2Self): 0;
			pdesc->Dword4 |= (ppfwinfo->rtsShort == 1)? set_desc(TX_RTSShort): 0;
//			pdesc->RTSSTBC = 0;	//RTS STBC mode... what is that ...?
			pdesc->Dword4 |= (ppfwinfo->rtsHt == 1)? set_desc(TX_RTSHT): 0;
			pdesc->Dword4 |= set_desc((ppfwinfo->rtsTxRate & TX_RTSRateMask) << TX_RTSRateSHIFT);//1 rate table is different from 8190, need to modify later!!
//			pdesc->RTSRate	= MRateToHwRate8192SE(Adapter, MGN_24M);
			pdesc->Dword4 |= (ppfwinfo->rtsbw == 1)? set_desc(TX_RTSBW): 0;
			pdesc->Dword4 |= set_desc((ppfwinfo->rtsSC & TX_RTSSCMask) << TX_RTSSCSHIFT);
			pdesc->Dword4 |= (ppfwinfo->rtsShort == 1)? set_desc(TX_RTSShort): 0;

			//fill necessary field in First Descriptor
//			pdesc->LINIP = 0;
			pdesc->Dword0 |= set_desc(32 << TX_OFFSETSHIFT); // tx_desc size
//			pdesc->Offset = USB_HWDESC_HEADER_LEN;
//			pdesc->PktSize = (u2Byte)PktLen;
		}

#endif
#if !defined(RTL8192SU) || !defined(DISABLE_UNALIGN_TRAP)
		pdesc->Dword0 |= set_desc(32 << TX_OFFSETSHIFT); // tx_desc size
#else
		SET_EQUAL(&pdesc->Dword0 , set_desc(sizeof(struct tx_desc) << TX_OFFSETSHIFT)); // tx_desc size
#endif
		if (i != (txcfg->frg_num - 1))
		{
			SetMFrag(pwlhdr);
			if (i == 0) {
				fr_len = (txcfg->frag_thrshld - txcfg->llc);
				tx_len -= (txcfg->frag_thrshld - txcfg->llc);
			}
			else {
				fr_len = txcfg->frag_thrshld;
				tx_len -= txcfg->frag_thrshld;
			}
		}
		else	// last seg, or the only seg (frg_num == 1)
		{
			fr_len = tx_len;
			ClearMFrag(pwlhdr);
#ifdef RTL8192SU
			if (txcfg->fr_type == _SKB_FRAME_TYPE_)
			{
				pdescinfo->last_seg=1;
			}
#endif
		}
		SetFragNum((pwlhdr), i);

		if ((i == 0) && (txcfg->fr_type == _SKB_FRAME_TYPE_))
		{
#if !defined(RTL8192SU) || !defined(DISABLE_UNALIGN_TRAP)
			pdesc->Dword7 |= set_desc((txcfg->hdr_len + txcfg->llc) << TX_TxBufferSizeSHIFT);
			pdesc->Dword0 |= set_desc((fr_len + (get_desc(pdesc->Dword7) & TX_TxBufferSizeMask)) << TX_PktSizeSHIFT);
			pdesc->Dword0 |= set_desc(TX_FirstSeg);
#else
			OR_EQUAL(&pdesc->Dword7 , set_desc((txcfg->hdr_len + txcfg->llc) << TX_TxBufferSizeSHIFT));
			OR_EQUAL(&pdesc->Dword0 , set_desc((fr_len + (get_desc(pdesc->Dword7) & TX_TxBufferSizeMask)) << TX_PktSizeSHIFT));
			OR_EQUAL(&pdesc->Dword0 , set_desc(TX_FirstSeg));
#endif
#if !defined(RTL8192SU)
			pdescinfo->type = _PRE_ALLOCLLCHDR_;
#else
			pdescinfo->hdr_type = _PRE_ALLOCLLCHDR_;
#endif
		}
		else
		{
#if !defined(RTL8192SU) || !defined(DISABLE_UNALIGN_TRAP)
			pdesc->Dword7 |= set_desc(txcfg->hdr_len << TX_TxBufferSizeSHIFT);
			pdesc->Dword0 |= set_desc((fr_len + (get_desc(pdesc->Dword7) & TX_TxBufferSizeMask)) << TX_PktSizeSHIFT);
			pdesc->Dword0 |= set_desc(TX_FirstSeg);
#else
			OR_EQUAL(&pdesc->Dword7 , set_desc(txcfg->hdr_len << TX_TxBufferSizeSHIFT));
			OR_EQUAL(&pdesc->Dword0 , set_desc((fr_len + (get_desc(pdesc->Dword7) & TX_TxBufferSizeMask)) << TX_PktSizeSHIFT));
			OR_EQUAL(&pdesc->Dword0 , set_desc(TX_FirstSeg));
#endif
#if !defined(RTL8192SU)
			pdescinfo->type = _PRE_ALLOCHDR_;
#else
			pdescinfo->hdr_type = _PRE_ALLOCHDR_;
#endif
		}


#ifdef _11s_TEST_MODE_
		mesh_debug_tx9(txcfg, pdescinfo);
#endif

//		pdesc->opt = set_desc(sizeof(struct FWtemplate));
//		pdesc->cmd |= set_desc((sizeof(struct FWtemplate) + 8) << _OFFSETSHIFT_);
		switch (q_num) {
#ifdef SW_MCAST
		case VO_QUEUE:
			q_select = 6;
			break;

		case VI_QUEUE:
			q_select = 5;
			break;
#else
		case HIGH_QUEUE:
			q_select = 0x11;// High Queue
			break;
#endif
#ifdef RTL8192SU
		case BEACON_QUEUE:
#ifdef RTL8192SU_FWBCN
			q_select = 0x10;
			break;
#endif
#endif
		case MGNT_QUEUE:
			q_select = 0x12;
			break;
		default:
			// data packet
			q_select = ((struct sk_buff *)txcfg->pframe)->cb[1];
#ifdef RTL8192SU
			if (q_select==3||q_select==0)
				q_num=BE_QUEUE;
			else if (q_select==2||q_select==1)
				q_num=BK_QUEUE;
			else if(q_select==4||q_select==5)
				q_num=VI_QUEUE;
			else if(q_select==6||q_select==7)
				q_num=VO_QUEUE;
			else
			{
				//printk("[%s,%d]q_select=%d??\n", __FUNCTION__, __LINE__, q_select);
				goto signin_tx_fail;
				//q_select=0;
				//q_num=BE_QUEUE;
			}
#endif
			break;
		}
#ifndef DISABLE_UNALIGN_TRAP
		pdesc->Dword1 |= set_desc((q_select & TX_QueueSelMask)<< TX_QueueSelSHIFT);
#else
		OR_EQUAL(&pdesc->Dword1 , set_desc((q_select & TX_QueueSelMask)<< TX_QueueSelSHIFT));
#endif

		if (i != (txcfg->frg_num - 1))
#ifndef DISABLE_UNALIGN_TRAP
			pdesc->Dword1 |= set_desc(TX_MoreFrag);
#else
			OR_EQUAL(&pdesc->Dword1 , set_desc(TX_MoreFrag));
#endif
//			pdesc->opt |= set_desc(_MFRAG_);

		// Set MacID
		if (txcfg->pstat) {
			if (txcfg->pstat->aid != MANAGEMENT_AID)	{
//				pdesc->opt |= set_desc((txcfg->pstat->aid & 0x0007) << _RATIDSHIFT_);
//				((struct FWtemplate *)pfwinfo)->rsvd0 = (txcfg->pstat->aid & 0x0038) >> 3;
#ifdef STA_EXT
#ifndef DISABLE_UNALIGN_TRAP
				pdesc->Dword1 |= set_desc(txcfg->pstat->remapped_aid & TX_MACIDMask);
#else
				OR_EQUAL(&pdesc->Dword1, set_desc(txcfg->pstat->remapped_aid & TX_MACIDMask));
#endif
#else
#ifndef DISABLE_UNALIGN_TRAP
				pdesc->Dword1 |= set_desc(txcfg->pstat->aid & TX_MACIDMask);
#else
				OR_EQUAL(&pdesc->Dword1, set_desc(txcfg->pstat->aid & TX_MACIDMask));
#endif
#endif
			}
		}
//		else {
//			pdesc->opt |= set_desc((7 & 0x0007) << _RATIDSHIFT_);
//			((struct FWtemplate *)pfwinfo)->rsvd0 = (7 & 0x0038) >> 3;
//		}


/*
		if (
#ifdef WDS
				(txcfg->pstat && (txcfg->pstat->state & WIFI_WDS) &&
					priv->pmib->dot11WdsInfo.entry[txcfg->pstat->wds_idx].txRate) ||
				(txcfg->pstat && !(txcfg->pstat->state & WIFI_WDS) &&
						!priv->pmib->dot11StationConfigEntry.autoRate) ||
#else
				(!priv->pmib->dot11StationConfigEntry.autoRate) ||
#endif
				(txcfg->pstat == NULL) ||
				(txcfg->fr_type != _SKB_FRAME_TYPE_))
			pdesc->opt |= set_desc(_USERATE_ | _DISFB_);
*/

//		pdesc->Dword4 |= set_desc(TX_UserRate);
#ifdef RTL8192SU
		if (q_num!=BEACON_QUEUE)
#endif
#ifndef DISABLE_UNALIGN_TRAP
		pdesc->Dword5 |= set_desc((0x1f) << TX_DataRateFBLmtSHIFT);
#else
		OR_EQUAL(&pdesc->Dword5 , set_desc((0x1f) << TX_DataRateFBLmtSHIFT));
#endif
//		pdesc->Dword5 |= set_desc(TX_DISFB);
		if (txcfg->fixed_rate)
		{
#ifndef DISABLE_UNALIGN_TRAP
			pdesc->Dword4 |= set_desc(TX_UserRate);
			pdesc->Dword5 |= set_desc(TX_DISFB);
			pdesc->Dword4 |= set_desc(TX_DisRTSFB);// disable RTS fall back
#else
			OR_EQUAL(&pdesc->Dword4 , set_desc(TX_UserRate));
			OR_EQUAL(&pdesc->Dword5 , set_desc(TX_DISFB));
			OR_EQUAL(&pdesc->Dword4 , set_desc(TX_DisRTSFB));// disable RTS fall back
#endif
		}
#ifdef RTL8192SU
		if (q_num==BEACON_QUEUE)
		{
#ifndef DISABLE_UNALIGN_TRAP
			pdesc->Dword4 = set_desc((0x7 << TX_RaBRSRIDSHIFT) | TX_UserRate);
			pdesc->Dword5 = set_desc(TX_DISFB);
			pdesc->Dword4 |= set_desc(0x08 << TX_RTSRateSHIFT);
#else
			SET_EQUAL(&pdesc->Dword4 , set_desc((0x7 << TX_RaBRSRIDSHIFT) | TX_UserRate));
			SET_EQUAL(&pdesc->Dword5 , set_desc(TX_DISFB));
			OR_EQUAL(&pdesc->Dword4 , set_desc(0x08 << TX_RTSRateSHIFT));
#endif
		}
#endif //RTL8192SU

#ifdef STA_EXT
		if(txcfg->pstat && txcfg->pstat->remapped_aid == FW_NUM_STAT-1/*(priv->pshare->STA_map & BIT(txcfg->pstat->aid)*/){
			pdesc->Dword4 |= set_desc(TX_UserRate);
		}
#endif

		if (!txcfg->need_ack && txcfg->privacy && UseSwCrypto(priv, NULL, TRUE))
#ifndef DISABLE_UNALIGN_TRAP
			pdesc->Dword1 &= set_desc( ~(TX_SecTypeMask<< TX_SecTypeSHIFT));
#else
			AND_EQUAL(&pdesc->Dword1 , set_desc( ~(TX_SecTypeMask<< TX_SecTypeSHIFT)));
#endif
//			pdesc->opt |= set_desc(_NOENC_);

		if (txcfg->privacy)
		{
			if (UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE)))
			{
//				pdesc->cmd = set_desc(get_desc(pdesc->cmd) + txcfg->icv + txcfg->mic + txcfg->iv);
//				pdesc->flen = set_desc(get_desc(pdesc->flen) + txcfg->iv);
#ifndef DISABLE_UNALIGN_TRAP
				pdesc->Dword0 = set_desc(get_desc(pdesc->Dword0)+ txcfg->icv + txcfg->mic + txcfg->iv);
				pdesc->Dword7 = set_desc(get_desc(pdesc->Dword7)+ txcfg->iv);
#else
				SET_EQUAL(&pdesc->Dword0 , set_desc(get_desc(pdesc->Dword0)+ txcfg->icv + txcfg->mic + txcfg->iv));
				SET_EQUAL(&pdesc->Dword7 , set_desc(get_desc(pdesc->Dword7)+ txcfg->iv));
#endif
			}
			else // hw encrypt
			{
				switch(txcfg->privacy)
				{
				case _WEP_104_PRIVACY_:
				case _WEP_40_PRIVACY_:
//					pdesc->cmd = set_desc(get_desc(pdesc->cmd) + txcfg->iv);
//					pdesc->flen = set_desc(get_desc(pdesc->flen) + txcfg->iv);
#ifndef DISABLE_UNALIGN_TRAP
					pdesc->Dword0 = set_desc(get_desc(pdesc->Dword0) + txcfg->iv);
					pdesc->Dword7 = set_desc(get_desc(pdesc->Dword7) + txcfg->iv);
#else
					SET_EQUAL(&pdesc->Dword0 , set_desc(get_desc(pdesc->Dword0) + txcfg->iv));
					SET_EQUAL(&pdesc->Dword7 , set_desc(get_desc(pdesc->Dword7) + txcfg->iv));
#endif
					wep_fill_iv(priv, pwlhdr, txcfg->hdr_len, keyid);
					//pdesc->opt = set_desc(get_desc(pdesc->opt) | BIT(30));
#ifndef DISABLE_UNALIGN_TRAP
					pdesc->Dword1 |= set_desc(0x1 << TX_SecTypeSHIFT);
#else
					OR_EQUAL(&pdesc->Dword1 , set_desc(0x1 << TX_SecTypeSHIFT));
#endif
					break;

				case _TKIP_PRIVACY_:
//					pdesc->cmd = set_desc(get_desc(pdesc->cmd) + txcfg->iv + txcfg->mic);
//					pdesc->flen = set_desc(get_desc(pdesc->flen) + txcfg->iv);

#ifndef DISABLE_UNALIGN_TRAP
					pdesc->Dword0 = set_desc(get_desc(pdesc->Dword0) + txcfg->iv + txcfg->mic);
					pdesc->Dword7 = set_desc(get_desc(pdesc->Dword7) + txcfg->iv);
#else

					SET_EQUAL(&pdesc->Dword0 , set_desc(get_desc(pdesc->Dword0) + txcfg->iv + txcfg->mic));
					SET_EQUAL(&pdesc->Dword7 , set_desc(get_desc(pdesc->Dword7) + txcfg->iv));
					
#endif
					tkip_fill_encheader(priv, pwlhdr, txcfg->hdr_len, keyid);
					//pdesc->opt = set_desc(get_desc(pdesc->opt) | BIT(31));
#ifndef DISABLE_UNALIGN_TRAP
					pdesc->Dword1 |= set_desc(0x2 << TX_SecTypeSHIFT);
#else
					OR_EQUAL(&pdesc->Dword1 , set_desc(0x2 << TX_SecTypeSHIFT));
#endif
					// cam id is no need to sign in at present, victoryman 20081130
//					if ((!IS_MCAST(da)) && (txcfg->pstat))
//						pdesc->Dword1 |= set_desc((txcfg->pstat->cam_id & TX_MACIDMask)<< TX_MACIDSHIFT);  // am i right??
//						pdesc->opt |= set_desc(BIT(29) | ((txcfg->pstat->cam_id) & 0x1f)<<24);
					break;

				case _CCMP_PRIVACY_:
					//michal also hardware...
//					pdesc->cmd = set_desc(get_desc(pdesc->cmd) + txcfg->iv);
//					pdesc->flen = set_desc(get_desc(pdesc->flen) + txcfg->iv);
#ifndef DISABLE_UNALIGN_TRAP
					pdesc->Dword0 = set_desc(get_desc(pdesc->Dword0) + txcfg->iv);
					pdesc->Dword7 = set_desc(get_desc(pdesc->Dword7) + txcfg->iv);
#else
					SET_EQUAL(&pdesc->Dword0 , set_desc(get_desc(pdesc->Dword0) + txcfg->iv));
					SET_EQUAL(&pdesc->Dword7 , set_desc(get_desc(pdesc->Dword7) + txcfg->iv));

#endif
					aes_fill_encheader(priv, pwlhdr, txcfg->hdr_len, keyid);
//					pdesc->opt = set_desc(get_desc(pdesc->opt) | BIT(31) | BIT(30));
#ifndef DISABLE_UNALIGN_TRAP
					pdesc->Dword1 |= set_desc(0x3 << TX_SecTypeSHIFT);
#else
					OR_EQUAL(&pdesc->Dword1 , set_desc(0x3 << TX_SecTypeSHIFT));
#endif
					// cam id is no need to sign in at present, victoryman 20081130
//					if ((!IS_MCAST(da)) && (txcfg->pstat))
//						pdesc->Dword1 |= set_desc((txcfg->pstat->cam_id & TX_MACIDMask)<< TX_MACIDSHIFT);
//						pdesc->opt |= set_desc(BIT(29) | ((txcfg->pstat->cam_id) & 0x1f)<<24);
					break;

				default:
					DEBUG_ERR("Unknow privacy\n");
					break;
				}
			}
#ifdef RTL8192SU
			if (i==0)
				memcpy(pskb_data, pwlhdr, txcfg->hdr_len+txcfg->iv+txcfg->llc);
			else
				memcpy(pskb_data, pwlhdr, txcfg->hdr_len+txcfg->iv);
#endif
		}
#ifdef RTL8192SU
		else
		{
			//memDump(pskb_data, 64, "pskb_data");
			if (i==0)
				memcpy(pskb_data, pwlhdr, txcfg->hdr_len+txcfg->llc);
			else
				memcpy(pskb_data, pwlhdr, txcfg->hdr_len);
		}
#endif
//		pdesc->paddr = set_desc(get_physical_addr(priv, pwlhdr-sizeof(struct FWtemplate),
//			(get_desc(pdesc->flen)&0xffff), PCI_DMA_TODEVICE));
		// we don't have fwinfo for 8192SE tx
		//TxBufferAddr


#if !defined(RTL8192SU)
		pdesc->Dword8 = set_desc(get_physical_addr(priv, pwlhdr,
			(get_desc(pdesc->Dword7)&0xffff), PCI_DMA_TODEVICE));
#endif

		// below is for sw desc info
#if !defined(RTL8192SU)
		pdescinfo->paddr  = get_desc(pdesc->Dword8);
		pdescinfo->pframe = pwlhdr;
#else
		pdescinfo->phdr = pwlhdr;
		pdescinfo->priv = priv;
#endif
#if defined(SEMI_QOS) && defined(WMM_APSD)
		pdescinfo->priv = priv;
		pdescinfo->pstat = txcfg->pstat;
#endif

#ifdef TX_SHORTCUT
		if (!priv->pmib->dot11OperationEntry.disable_txsc && txcfg->pstat &&
				(txcfg->fr_type == _SKB_FRAME_TYPE_)
#ifdef RTL867X_USBPATCH
			&& (ref_txsc == 1)
#endif
			){
#if !defined(RTL8192SU)
			desc_copy(&txcfg->pstat->hwdesc1, pdesc);
			descinfo_copy(&txcfg->pstat->swdesc1, pdescinfo);
#else
			if (txcfg->fr_len>=1400)
			{
				desc_copy(&txcfg->pstat->hwdesc1, pdesc);
			}
			else
			{
				desc_copy(&txcfg->pstat->hwdesc3, pdesc);
			}
			
#endif
			txcfg->pstat->protection = priv->pmib->dot11ErpInfo.protection;
			txcfg->pstat->sc_keyid = keyid;
			txcfg->pstat->pktpri = ((struct sk_buff *)txcfg->pframe)->cb[1];
//			memcpy(&txcfg->pstat->fw_bckp, pfwinfo, sizeof(struct FWtemplate));
			fit_shortcut = 1;
		}
		else {
			if (txcfg->pstat)
					txcfg->pstat->hwdesc1.Dword7 &= ~TX_TxBufferSizeMask;
		}
#endif

#if !defined(RTL8192SU)
		pfrst_dma_desc = dma_txhead[*tx_head];
#endif

		if (i != 0) {
//			pdesc->OWN = 1;
#ifndef DISABLE_UNALIGN_TRAP
			pdesc->Dword0 |= set_desc(TX_OWN);
#else
			OR_EQUAL(&pdesc->Dword0 , set_desc(TX_OWN));
#endif
#if !defined(RTL8192SU)
#ifndef USE_RTL8186_SDK
			rtl_cache_sync_wback(priv, dma_txhead[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
#endif
		}

#if defined(USE_RTL8186_SDK) && !defined(RTL8192SU)
		flush_addr[flush_num]=(unsigned long)bus_to_virt(get_desc(pdesc->Dword8)); //TxBufferAddr
		flush_len[flush_num++]= (get_desc(pdesc->Dword7) & TX_TxBufferSizeMask);
#endif

/*
		//printk desc content
		{
			unsigned int *ppdesc = (unsigned int *)pdesc;
			printk("%08x    %08x    %08x \n", get_desc(*(ppdesc)), get_desc(*(ppdesc+1)), get_desc(*(ppdesc+2)));
			printk("%08x    %08x    %08x \n", get_desc(*(ppdesc+3)), get_desc(*(ppdesc+4)), get_desc(*(ppdesc+5)));
			printk("%08x    %08x    %08x \n", get_desc(*(ppdesc+6)), get_desc(*(ppdesc+7)), get_desc(*(ppdesc+8)));
			printk("%08x\n", *(ppdesc+9));
			printk("===================================================\n");
		}
*/

#if !defined(RTL8192SU)
		txdesc_rollover(pdesc, (unsigned int *)tx_head);
#endif

		if (txcfg->fr_len == 0)
		{
			if (txcfg->aggre_en != FG_AGGRE_MSDU_FIRST)
#ifndef DISABLE_UNALIGN_TRAP
				pdesc->Dword0 |= set_desc(TX_LastSeg);
#else
				OR_EQUAL(&pdesc->Dword0 , set_desc(TX_LastSeg));
#endif
//				pdesc->LastSeg= 1;
//			  pdesc->cmd |= set_desc(_LS_);
			goto init_deschead;
		}

		/*------------------------------------------------------------*/
		/*              fill descriptor of frame body                 */
		/*------------------------------------------------------------*/
#if !defined(RTL8192SU)
		pndesc     = phdesc + *tx_head;
		pndescinfo = pswdescinfo + *tx_head;
		//clear all bits
		memset(pndesc, 0,32);
/*
		pndesc->cmd	= set_desc((get_desc(pdesc->cmd) & (~_FS_)) | _OWN_);
		pndesc->rsvd0 = pdesc->rsvd0;
		pndesc->rsvd1 = pdesc->rsvd1;
		pndesc->rsvd2 = pdesc->rsvd2;
*/

		pndesc->Dword0 = set_desc((get_desc(pdesc->Dword0) & (~TX_FirstSeg)) | (TX_OWN));
//		pndesc->FirstSeg= 0;
//		pndesc->OWN = 1;
#else
		pdescinfo->type=0;
		pndesc     = pdesc;
		pndescinfo = pdescinfo;
		//pdesc->Dword0 |= set_desc(TX_OWN);
#endif

		if (txcfg->privacy)
		{
#if defined(CONFIG_RTL_WAPI_SUPPORT)
			if (txcfg->privacy == _WAPI_SMS4_)
			{
				if (txcfg->aggre_en != FG_AGGRE_MSDU_FIRST)
					pndesc->Dword0 |= set_desc(TX_LastSeg);
//					pndesc->LastSeg = 1;
//				  pndesc->cmd |= set_desc(_LS_);
				pndescinfo->pstat = txcfg->pstat;
				pndescinfo->rate = txcfg->tx_rate;
			}
			else 
#endif
			if (!UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE)))
			{
				if (txcfg->aggre_en != FG_AGGRE_MSDU_FIRST)
				{
#ifndef DISABLE_UNALIGN_TRAP
					pndesc->Dword0 |= set_desc(TX_LastSeg);
#else
					OR_EQUAL(&pndesc->Dword0 , set_desc(TX_LastSeg));
#endif
				}
//					pndesc->LastSeg = 1;
//				  pndesc->cmd |= set_desc(_LS_);
				pndescinfo->pstat = txcfg->pstat;
				pndescinfo->rate = txcfg->tx_rate;
			}
		}
		else
		{
			if (txcfg->aggre_en != FG_AGGRE_MSDU_FIRST)
#ifndef DISABLE_UNALIGN_TRAP
				pndesc->Dword0 |= set_desc(TX_LastSeg);
#else
				OR_EQUAL(&pndesc->Dword0 , set_desc(TX_LastSeg));
#endif
//			  pndesc->cmd |= set_desc(_LS_);
			pndescinfo->pstat = txcfg->pstat;
			pndescinfo->rate = txcfg->tx_rate;
		}

//		pndesc->flen = set_desc(fr_len);
#if !defined(RTL8192SU)
#if defined(CONFIG_RTL_WAPI_SUPPORT)
		if (txcfg->privacy == _WAPI_SMS4_)
			pndesc->Dword7 |= set_desc( (fr_len+SMS4_MIC_LEN) & TX_TxBufferSizeMask);
		else
#endif
		pndesc->Dword7 |= set_desc(fr_len & TX_TxBufferSizeMask);
#else
#ifndef DISABLE_UNALIGN_TRAP
		//pndesc->Dword7 = set_desc(get_desc(pdesc->Dword7) + txcfg->fr_len);
		pndesc->Dword7 = set_desc(get_desc(pdesc->Dword7) + fr_len);
#else
		//SET_EQUAL(&pndesc->Dword7 , set_desc(get_desc(pdesc->Dword7) + txcfg->fr_len));
		SET_EQUAL(&pndesc->Dword7 , set_desc(get_desc(pdesc->Dword7) + fr_len));
#endif
#endif		

#if !defined(RTL8192SU)
		if (i == 0)
			pndescinfo->type = txcfg->fr_type;
		else
			pndescinfo->type = _RESERVED_FRAME_TYPE_;
#else
		pdescinfo->type = txcfg->fr_type;
#endif

#if defined(CONFIG_RTK_MESH) && defined(MESH_USE_METRICOP)
		if( (txcfg->fr_type == _PRE_ALLOCMEM_) && (txcfg->is_11s & 128)) // for 11s link measurement frame
			pndescinfo->type =_RESERVED_FRAME_TYPE_;
#endif

#ifdef _11s_TEST_MODE_
		mesh_debug_tx10(txcfg, pndescinfo);
#endif

//		pndesc->paddr = set_desc(tmpphyaddr);
#if !defined(RTL8192SU)
		pndesc->Dword8 = set_desc(tmpphyaddr); //TxBufferAddr
		pndescinfo->paddr = get_desc(pndesc->Dword8);
		pndescinfo->pframe = txcfg->pframe;
		pndescinfo->len = txcfg->fr_len;	// for pci_unmap_single
		pndescinfo->priv = priv;
#else
		pdescinfo->pframe = txcfg->pframe;		
#endif

#if !defined(RTL8192SU)
		pbuf += fr_len;
		tmpphyaddr += fr_len;
#endif		

#ifdef TX_SHORTCUT
		if (fit_shortcut) {
#if !defined(RTL8192SU)
			desc_copy(&txcfg->pstat->hwdesc2, pndesc);
			descinfo_copy(&txcfg->pstat->swdesc2, pndescinfo);
#else
			if (txcfg->fr_len>=1400)
			{
				descinfo_copy(&txcfg->pstat->swdesc1, pdescinfo);
				txcfg->pstat->hwdesc2.Dword7 = set_desc(txcfg->fr_len & TX_TxBufferSizeMask);
			}
			else
			{
				descinfo_copy(&txcfg->pstat->swdesc3, pdescinfo);
				txcfg->pstat->hwdesc4.Dword7 = set_desc(txcfg->fr_len & TX_TxBufferSizeMask);
			}
#endif
		}
#endif

#if !defined(RTL8192SU)
#ifndef USE_RTL8186_SDK
		rtl_cache_sync_wback(priv, dma_txhead[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif

		flush_addr[flush_num]=(unsigned long)bus_to_virt(get_desc(pndesc->Dword8));
		flush_len[flush_num++]=get_desc(pndesc->Dword7) & TX_TxBufferSizeMask; // TxBufferSize

		txdesc_rollover(pndesc, (unsigned int *)tx_head);
#endif
#ifndef CONFIG_RTL8671 //tylo, disable to use gdma-memcpy
		// retrieve H/W MIC and put in payload
#if defined(CONFIG_RTL_WAPI_SUPPORT)
		if (txcfg->privacy == _WAPI_SMS4_)
		{
			SecSWSMS4Encryption(priv, txcfg);
		} else
#endif

		if ((txcfg->privacy == _TKIP_PRIVACY_) &&
			(priv->pshare->have_hw_mic) &&
			!(priv->pmib->dot11StationConfigEntry.swTkipMic) &&
			(i == (txcfg->frg_num-1)) )	// last segment
		{
			register unsigned long int l,r;
			unsigned char *mic;
			volatile int i;

			while ((*(volatile unsigned int *)GDMAISR & GDMA_COMPIP) == 0)
				for (i=0; i<10; i++)
					;

			l = *(volatile unsigned int *)GDMAICVL;
			r = *(volatile unsigned int *)GDMAICVR;

			mic = ((struct sk_buff *)txcfg->pframe)->data + txcfg->fr_len - 8;
			mic[0] = (unsigned char)(l & 0xff);
			mic[1] = (unsigned char)((l >> 8) & 0xff);
			mic[2] = (unsigned char)((l >> 16) & 0xff);
			mic[3] = (unsigned char)((l >> 24) & 0xff);
			mic[4] = (unsigned char)(r & 0xff);
			mic[5] = (unsigned char)((r >> 8) & 0xff);
			mic[6] = (unsigned char)((r >> 16) & 0xff);
			mic[7] = (unsigned char)((r >> 24) & 0xff);

#ifdef MICERR_TEST
			if (priv->micerr_flag) {
				mic[7] ^= mic[7];
				priv->micerr_flag = 0;
			}
#endif
		}
#endif

#ifdef RTL8192SU
		if ((i == 0) && (txcfg->fr_type == _SKB_FRAME_TYPE_))
		{
#ifdef RTL8192SU_TX_ZEROCOPY
			if(pdescinfo->non_zcpy)
			{
				#ifdef CONFIG_RTL8671
				gdma_memcpy(pskb_data+(txcfg->hdr_len+txcfg->llc+txcfg->iv), ((struct sk_buff *)txcfg->pframe)->data, fr_len);
				#else
				memcpy(pskb_data+(txcfg->hdr_len+txcfg->llc+txcfg->iv), ((struct sk_buff *)txcfg->pframe)->data, fr_len);
				//memcpy(pskb_data+(txcfg->hdr_len+txcfg->llc+txcfg->iv), ((struct sk_buff *)txcfg->pframe)->data, txcfg->fr_len);
				#endif
			}
#else
			memcpy(pskb_data+(txcfg->hdr_len+txcfg->llc+txcfg->iv), ((struct sk_buff *)txcfg->pframe)->data, fr_len);
			//memcpy(pskb_data+(txcfg->hdr_len+txcfg->llc+txcfg->iv), ((struct sk_buff *)txcfg->pframe)->data, txcfg->fr_len);
#endif
			//pbuf=pskb_data+(txcfg->hdr_len+txcfg->llc+txcfg->iv);
		}
		else
		{
			if ((txcfg->fr_type != _SKB_FRAME_TYPE_))
				memcpy(pskb_data+(txcfg->hdr_len), txcfg->pframe, fr_len);
			else
				memcpy(pskb_data+(txcfg->hdr_len+txcfg->iv), pbuf, fr_len);
			//memcpy(pskb_data+(txcfg->hdr_len), txcfg->pframe, txcfg->fr_len);
			//pbuf=pskb_data+(txcfg->hdr_len);
		}
		pbuf += fr_len;
#endif //RTL8192SU

#ifdef RTL8192SU_FWBCN
		if (txcfg->q_num==BEACON_QUEUE)
			rtl8192SU_fill_TxCmd(priv, txcfg, pdesc_txcmd, en_pktoffset);
#endif

		/*------------------------------------------------------------*/
		/*                insert sw encrypt here!                     */
		/*------------------------------------------------------------*/
		if (txcfg->privacy && UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE)))
		{
#ifdef RTL8192SU
			pwlhdr = pskb_data;
#endif
			if (txcfg->privacy == _TKIP_PRIVACY_ ||
				txcfg->privacy == _WEP_40_PRIVACY_ ||
				txcfg->privacy == _WEP_104_PRIVACY_)
			{
#if !defined(RTL8192SU)
				picvdesc     = phdesc + *tx_head;
				picvdescinfo = pswdescinfo + *tx_head;
				//clear all bits
				memset(picvdesc, 0,32);

				if (txcfg->aggre_en == FG_AGGRE_MSDU_FIRST){
//					memcpy(picvdesc, pdesc, sizeof(struct _TX_DESC_8192SE));
//					picvdesc->FirstSeg = 0;
//					picvdesc->OWN = 1;
					picvdesc->Dword0 = set_desc((get_desc(pdesc->Dword0) & (~TX_FirstSeg)) | TX_OWN);
//				  picvdesc->cmd = set_desc((get_desc(pdesc->cmd) & (~_FS_)) | _OWN_);
				}
				else{
//					memcpy(picvdesc, pdesc, sizeof(struct _TX_DESC_8192SE));
//					picvdesc->FirstSeg = 0;
//					picvdesc->LastSeg = 1;
//					picvdesc->OWN = 1;
					picvdesc->Dword0   = set_desc((get_desc(pdesc->Dword0) & (~TX_FirstSeg)) | TX_OWN | TX_LastSeg);
//	  			  picvdesc->cmd   = set_desc((get_desc(pdesc->cmd) & (~_FS_)) | _OWN_ | _LS_);
				}


//				picvdesc->flen  = set_desc(txcfg->icv);
//				picvdesc->rsvd0 = pdesc->rsvd0;
//				picvdesc->rsvd1 = pdesc->rsvd1;
//				picvdesc->rsvd2 = pdesc->rsvd2;
				picvdesc->Dword7 |= (set_desc(txcfg->icv & TX_TxBufferSizeMask)); //TxBufferSize
#else
#ifndef DISABLE_UNALIGN_TRAP
				pdesc->Dword7 = set_desc(get_desc(pdesc->Dword7) + txcfg->icv);
				pdesc->Dword0 = set_desc((get_desc(pdesc->Dword0)) | TX_LastSeg);
#else
				SET_EQUAL(&pdesc->Dword7 , set_desc(get_desc(pdesc->Dword7) + txcfg->icv));
				SET_EQUAL(&pdesc->Dword0 , set_desc((get_desc(pdesc->Dword0)) | TX_LastSeg));
#endif
#endif

				// append ICV first...
				picv = get_icv_from_poll(priv);
				if (picv == NULL)
				{
					DEBUG_ERR("System-Buf! can't alloc picv\n");
					BUG();
				}

#if !defined(RTL8192SU)
				picvdescinfo->type = _PRE_ALLOCICVHDR_;
				picvdescinfo->pframe = picv;
				picvdescinfo->pstat = txcfg->pstat;
				picvdescinfo->rate = txcfg->tx_rate;
				picvdescinfo->priv = priv;
//				picvdesc->paddr = set_desc(get_physical_addr(priv, picv, txcfg->icv, PCI_DMA_TODEVICE));
				//TxBufferAddr
				picvdesc->Dword8 = set_desc(get_physical_addr(priv, picv, txcfg->icv, PCI_DMA_TODEVICE));
#ifdef USE_RTL8186_SDK
				rtl_cache_sync_wback(priv, (unsigned int)picv, txcfg->icv, PCI_DMA_TODEVICE);
#endif
#else
				pdescinfo->enc_type = _PRE_ALLOCICVHDR_;
				pdescinfo->penc = picv;
				pdescinfo->pstat = txcfg->pstat; //??
				pdescinfo->rate = txcfg->tx_rate;//??
#endif //!RTL8192SU
				if (i == 0)
					tkip_icv(picv,
						pwlhdr + txcfg->hdr_len + txcfg->iv, txcfg->llc,
#if !defined(RTL8192SU)
						pbuf - (get_desc(pndesc->Dword7) & TX_TxBufferSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufferSizeMask));
#else
						pwlhdr + (txcfg->hdr_len+txcfg->llc+txcfg->iv), fr_len);//- txcfg->fr_len, txcfg->fr_len);
						
#endif
				else
					tkip_icv(picv,
						NULL, 0,
#if !defined(RTL8192SU)
						pbuf - (get_desc(pndesc->Dword7) & TX_TxBufferSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufferSizeMask));
#else
						pwlhdr+(txcfg->hdr_len+txcfg->iv), fr_len);//- txcfg->fr_len, txcfg->fr_len);
#endif
				if ((i == 0) && (txcfg->llc != 0)) {
					if (txcfg->privacy == _TKIP_PRIVACY_)
					{
#if !defined(RTL8192SU)
						tkip_encrypt(priv, pwlhdr, txcfg->hdr_len,
							pwlhdr + txcfg->hdr_len + 8, sizeof(struct llc_snap),
							pbuf - (get_desc(pndesc->Dword7) & TX_TxBufferSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufferSizeMask), picv, txcfg->icv);
#else
						tkip_encrypt(priv, pwlhdr, txcfg->hdr_len,
							pwlhdr+txcfg->hdr_len+8, sizeof(struct llc_snap),
							pwlhdr+(txcfg->hdr_len+txcfg->llc+txcfg->iv), fr_len, //txcfg->fr_len,
							picv, txcfg->icv);
#endif
					}
					else
					{
#if !defined(RTL8192SU)
						wep_encrypt(priv, pwlhdr, txcfg->hdr_len,
							pwlhdr + txcfg->hdr_len + 4, sizeof(struct llc_snap),
							pbuf - (get_desc(pndesc->Dword7) & TX_TxBufferSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufferSizeMask), picv, txcfg->icv,
							txcfg->privacy);
#else
						wep_encrypt(priv, pwlhdr, txcfg->hdr_len,
							pwlhdr+txcfg->hdr_len+4, sizeof(struct llc_snap),
							pwlhdr+(txcfg->hdr_len+txcfg->llc+txcfg->iv), fr_len, //txcfg->fr_len,
							picv, txcfg->icv, txcfg->privacy);
#endif
					}
				}
				else { // not first segment or no snap header
					if (txcfg->privacy == _TKIP_PRIVACY_)
#if !defined(RTL8192SU)
						tkip_encrypt(priv, pwlhdr, txcfg->hdr_len, NULL, 0,
							pbuf - (get_desc(pndesc->Dword7) & TX_TxBufferSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufferSizeMask), picv, txcfg->icv);
#else
						tkip_encrypt(priv, pwlhdr, txcfg->hdr_len, NULL, 0,
							pwlhdr+(txcfg->hdr_len+txcfg->iv), fr_len, //txcfg->fr_len,
							picv, txcfg->icv);
#endif
					else
#if !defined(RTL8192SU)
						wep_encrypt(priv, pwlhdr, txcfg->hdr_len, NULL, 0,
							pbuf - (get_desc(pndesc->Dword7) & TX_TxBufferSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufferSizeMask), picv, txcfg->icv,
							txcfg->privacy);
#else
						wep_encrypt(priv, pwlhdr, txcfg->hdr_len, NULL, 0,
							pwlhdr+(txcfg->hdr_len+txcfg->iv), fr_len, //txcfg->fr_len,
							picv, txcfg->icv,
							txcfg->privacy);
#endif
				}
#if !defined(RTL8192SU)
#ifndef USE_RTL8186_SDK
				rtl_cache_sync_wback(priv, dma_txhead[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif

				flush_addr[flush_num]=(unsigned long)bus_to_virt(get_desc(picvdesc->Dword8));//TxBufferAddr
				flush_len[flush_num++]=(get_desc(picvdesc->Dword7) & TX_TxBufferSizeMask);

				txdesc_rollover(picvdesc, (unsigned int *)tx_head);
#else
				//memcpy(pwlhdr+(txcfg->hdr_len+txcfg->iv+txcfg->llc+txcfg->fr_len), picv, txcfg->icv);
				if (i==0)
					memcpy(pwlhdr+(txcfg->hdr_len+txcfg->iv+txcfg->llc+fr_len), picv, txcfg->icv);
				else
					memcpy(pwlhdr+(txcfg->hdr_len+txcfg->iv+fr_len), picv, txcfg->icv);
#endif //!RTL8192SU
			}

			else if (txcfg->privacy == _CCMP_PRIVACY_)
			{
#if !defined(RTL8192SU)
				pmicdesc = phdesc + *tx_head;
				pmicdescinfo = pswdescinfo + *tx_head;
				if (txcfg->aggre_en == FG_AGGRE_MSDU_FIRST)
					pmicdesc->Dword0 = set_desc((get_desc(pdesc->Dword0) & (~TX_FirstSeg)) | TX_OWN);
//				  pmicdesc->cmd = set_desc((get_desc(pdesc->cmd) & (~_FS_)) | _OWN_);
				else
				  pmicdesc->Dword0   = set_desc((get_desc(pdesc->Dword0) & (~TX_FirstSeg)) | TX_OWN | TX_LastSeg);
//				  pmicdesc->cmd   = set_desc((get_desc(pdesc->cmd) & (~_FS_)) | _OWN_ | _LS_);
//				pmicdesc->rsvd0 = pdesc->rsvd0;
//				pmicdesc->rsvd1 = pdesc->rsvd1;
//				pmicdesc->rsvd2 = pdesc->rsvd2;

				// set TxBufferSize
				pmicdesc->Dword7 = set_desc(txcfg->mic & TX_TxBufferSizeMask);
#else
#ifndef DISABLE_UNALIGN_TRAP
				pdesc->Dword7 = set_desc(get_desc(pdesc->Dword7) + txcfg->mic);
				pdesc->Dword0 = set_desc((get_desc(pdesc->Dword0)) | TX_LastSeg);
#else
				SET_EQUAL(&pdesc->Dword7 , set_desc(get_desc(pdesc->Dword7) + txcfg->mic));
				SET_EQUAL(&pdesc->Dword0 , set_desc((get_desc(pdesc->Dword0)) | TX_LastSeg));
#endif
#endif	//!RTL8192SU			

				// append MIC first...
				pmic = get_mic_from_poll(priv);
				if (pmic == NULL)
				{
					DEBUG_ERR("System-Buf! can't alloc pmic\n");
					BUG();
				}

#if !defined(RTL8192SU)
				pmicdescinfo->type = _PRE_ALLOCMICHDR_;
				pmicdescinfo->pframe = pmic;
				pmicdescinfo->pstat = txcfg->pstat;
				pmicdescinfo->rate = txcfg->tx_rate;
				pmicdescinfo->priv = priv;
				// set TxBufferAddr
				pmicdesc->Dword8= set_desc(get_physical_addr(priv, pmic, txcfg->mic, PCI_DMA_TODEVICE));
#ifdef USE_RTL8186_SDK
				rtl_cache_sync_wback(priv, (unsigned int)pmic, txcfg->mic, PCI_DMA_TODEVICE);
#endif
#else
				pdescinfo->enc_type = _PRE_ALLOCMICHDR_;
				pdescinfo->penc = pmic;

				pdescinfo->pstat = txcfg->pstat; //??
				pdescinfo->rate = txcfg->tx_rate;//??
#endif //!RTL8192SU

				// then encrypt all (including ICV) by AES
				if (i == 0) // encrypt 3 segments ==> llc, mpdu, and mic
#if !defined(RTL8192SU)
					aesccmp_encrypt(priv, pwlhdr, pwlhdr + txcfg->hdr_len + 8,
						pbuf - (get_desc(pndesc->Dword7) & TX_TxBufferSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufferSizeMask), pmic);
#else
					aesccmp_encrypt(priv, pwlhdr, ((char*)(tx+8)) + txcfg->hdr_len + 8,
						pwlhdr+(txcfg->hdr_len+txcfg->llc+txcfg->iv), fr_len,//txcfg->fr_len,
						pmic);
#endif	//!RTL8192SU
				else // encrypt 2 segments ==> mpdu and mic
#if !defined(RTL8192SU)
					aesccmp_encrypt(priv, pwlhdr, NULL,
						pbuf - (get_desc(pndesc->Dword7) & TX_TxBufferSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufferSizeMask), pmic);
#ifndef USE_RTL8186_SDK
				rtl_cache_sync_wback(priv, dma_txhead[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
				flush_addr[flush_num]=(unsigned long)bus_to_virt(get_desc(pmicdesc->Dword8));
				flush_len[flush_num++]= (get_desc(pmicdesc->Dword7) & TX_TxBufferSizeMask);

				txdesc_rollover(pmicdesc, (unsigned int *)tx_head);
#else
					aesccmp_encrypt(priv, pwlhdr, NULL,
						pwlhdr+(txcfg->hdr_len+txcfg->iv), fr_len,//txcfg->fr_len,
						pmic);

				//memcpy(pwlhdr+txcfg->hdr_len+txcfg->iv+txcfg->llc+txcfg->fr_len, pmic, txcfg->mic);
				if (i==0)
					memcpy(pwlhdr+txcfg->hdr_len+txcfg->iv+txcfg->llc+fr_len, pmic, txcfg->mic);
				else
					memcpy(pwlhdr+txcfg->hdr_len+txcfg->iv+fr_len, pmic, txcfg->mic);
#endif	//!RTL8192SU
			}
		}
#ifdef RTL8192SU
init_deschead:
	pdesc->Dword7 = 0;
#ifdef RTL8192SU_TX_ZEROCOPY
	if ((skb==NULL)|| (pdescinfo->non_zcpy))
#endif
		pdescinfo->purb= tx;
	priority=get_endpoint(priv, q_num);
	pdescinfo->q_num = q_num;
	{
		usb_fill_bulk_urb(tx_urb,priv->udev,
				usb_sndbulkpipe(priv->udev,priority), tx,
				urb_len, (usb_complete_t)rtl8192su_tx_isr, (void *)pdescinfo);

			//printk("urb_len=%d\n", urb_len);
		//if (txcfg->fr_type == _SKB_FRAME_TYPE_ )
#ifdef CONFIG_USB_OTG_Driver
// rock: usb otg is 4 bytes alignment
		//if ((((unsigned int)(tx))&3) != 0)
			//memDump(tx,urb_len,"tx&3 != 0");
#endif
	}
	if (q_num==BEACON_QUEUE || pdescinfo->non_zcpy==1)
		(((struct urb *)(tx_urb))->transfer_flags) &= (~URB_NO_INTERRUPT);
	//msg_queue("before submit urb1 skb=%x data=%x\n",(u32)pdescinfo->pframe,(u32)tx);
	dma_cache_wback_inv((unsigned long)tx,urb_len);
	//rtl_cache_sync_wback(priv, (unsigned int)tx, urb_len, 0);

#if 1
	//if (atomic_read(&priv->tx_pending[BEACON_QUEUE]) > PRE_ALLOCATED_MMPDU>>2)
		//(((struct urb *)(tx_urb))->transfer_flags) &= (~URB_NO_INTERRUPT);
#if defined (TX_SHORTCUT)&&defined(RTL8192SU_TXSC_DBG)
	if ((txcfg->fr_type!=_SKB_FRAME_TYPE_))
	{
		priv->ext_stats.mgt_pkt_cnt++;
	}
	else
	{
		struct sk_buff * skb2 = (struct sk_buff *)(pdescinfo->pframe);
		if (skb2->cb[8]!=1)
			priv->ext_stats.normal_pkt_cnt++;
	}
#endif
#endif

#ifdef RTL8192SU_CHECKTXHANGUP
	if (priv->pshare->ptx_urb==NULL&&q_num!=BEACON_QUEUE)
	{
		priv->pshare->ptx_urb=tx_urb;
		priv->pshare->txpkt_count=RTL_R16(0x366);
	}
#endif

	status = RTL8192SU_submit_urb(priv, tx_urb);
	if (!status){
		atomic_inc(&priv->tx_pending[q_num]);
	}else{
		printk("Error TX URB ,error %d, q_num=%x, priority=%x, q_select=%x\n", status, q_num, priority, q_select);
		if (pdescinfo->last_seg==1)
		{
			priv->ext_stats.tx_drops++;
			rtl8192su_siginTx_fail(priv, txcfg, wlanhdr);
		}			
		priv->pshare->tx_pool.own[pdescinfo->tx_pool_idx]=0;
		memDump(tx,64,"tx data");
	}
	if (txcfg->fr_len == 0)
		break;
#endif //RTL8192SU
	}


#if !defined(RTL8192SU)
init_deschead:
	for (i=0; i<flush_num; i++)
		rtl_cache_sync_wback(priv, flush_addr[i], flush_len[i], PCI_DMA_TODEVICE);
#endif

	if (txcfg->aggre_en == FG_AGGRE_MSDU_FIRST) {
		priv->amsdu_first_desc = pfrstdesc;
#ifndef USE_RTL8186_SDK
		priv->amsdu_first_dma_desc = pfrst_dma_desc;
#endif
		priv->amsdu_len = get_desc(pfrstdesc->Dword0) & 0xffff; // get pktSize
#if !defined(RTL8192SU)
		return;
#else
		goto signin_tx_fail;
#endif
	}

#ifdef BUFFER_TX_AMPDU
	if ((txcfg->aggre_en >= FG_AGGRE_MPDU_BUFFER_FIRST) &&
					(txcfg->aggre_en <= FG_AGGRE_MPDU_BUFFER_LAST)) {
		if (txcfg->aggre_en == FG_AGGRE_MPDU_BUFFER_FIRST) {
			priv->ampdu_first_desc = pfrstdesc;
#ifndef USE_RTL8186_SDK
			priv->ampdu_first_dma_desc = pfrst_dma_desc;
#endif
		}
		else {
			//pfrstdesc->cmd |= set_desc(_OWN_);
			pfrstdesc->Dword0 |= set_desc(TX_OWN);
#ifndef USE_RTL8186_SDK
			rtl_cache_sync_wback(priv, pfrst_dma_desc, sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
			if (txcfg->aggre_en == FG_AGGRE_MPDU_BUFFER_LAST) {
				pfrstdesc = priv->ampdu_first_desc;
				pfrstdesc->Dword0 |= set_desc(TX_OWN);

#ifndef USE_RTL8186_SDK
				rtl_cache_sync_wback(priv, priv->ampdu_first_dma_desc, sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
				tx_poll(priv, q_num);
			}
		}
#if !defined(RTL8192SU)
		return;
#else
		goto signin_tx_fail;
#endif		
	}
#endif // BUFFER_TX_AMPDU
#if !defined(RTL8192SU)
//	pfrstdesc->cmd |= set_desc(_OWN_);
	pfrstdesc->Dword0 |= set_desc(TX_OWN);
#ifndef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, pfrst_dma_desc, sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif

#ifndef SW_MCAST
	if (q_num == HIGH_QUEUE) {
		priv->pkt_in_dtimQ = 1;
		return;
	}
	else
#endif
		tx_poll(priv, q_num);

	return;
#else //RTL8192SU
	return;
signin_tx_fail:
	priv->ext_stats.tx_drops++;
	rtl8192su_siginTx_fail(priv, txcfg, wlanhdr);
#endif //!RTL8192SU
}


#ifdef SUPPORT_TX_AMSDU
//__IRAM_FASTEXTDEV // marked by joshua, 92SE not support AMSDU now ...
void rtl8192SE_signin_txdesc_amsdu(struct rtl8190_priv *priv, struct tx_insn* txcfg)
{
	struct tx_desc *phdesc, *pdesc, *pfrstdesc;
	struct tx_desc_info *pswdescinfo, *pdescinfo;
	unsigned int  tx_len;
	int *tx_head, q_num;
	unsigned long	tmpphyaddr;
	unsigned char *pbuf;
	struct rtl8190_hw *phw;
	unsigned int *dma_txhead;

	q_num = txcfg->q_num;
	phw	= GET_HW(priv);

#if !defined(RTL8192SU)
	dma_txhead	= get_txdma_addr(phw, q_num);
	tx_head		= get_txhead_addr(phw, q_num);
	phdesc   	= get_txdesc(phw, q_num);
	pswdescinfo = get_txdesc_info(priv->pshare->pdesc_info, q_num);
#endif
	pbuf = ((struct sk_buff *)txcfg->pframe)->data;
	tx_len = ((struct sk_buff *)txcfg->pframe)->len;
	tmpphyaddr = get_physical_addr(priv, pbuf, tx_len, PCI_DMA_TODEVICE);
#ifdef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, (unsigned int)pbuf, tx_len, PCI_DMA_TODEVICE);
#endif

	pdesc     = phdesc + (*tx_head);
	pdescinfo = pswdescinfo + *tx_head;

	//clear all bits
	memset(pdesc, 0, 32);

#if !defined(RTL8192SU)
	pdesc->Dword8 = set_desc(tmpphyaddr); // TXBufferAddr
#endif	
	pdesc->Dword7 |= (set_desc(tx_len & TX_TxBufferSizeMask));
	pdesc->Dword0 |= set_desc(32 << TX_OFFSETSHIFT); // tx_desc size
	if (txcfg->aggre_en == FG_AGGRE_MSDU_LAST){
//		pdesc->cmd = set_desc(_OWN_ | _LS_);
		pdesc->Dword0 = set_desc(TX_OWN | TX_LastSeg);
//		pdesc->OWN =1 ;
//		pdesc->LastSeg = 1 ;
	}
	else
		pdesc->Dword0 = set_desc(TX_OWN);
//		pdesc->OWN = 1;
//		pdesc->cmd = set_desc(_OWN_);

#ifndef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, dma_txhead[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif

	pdescinfo->type = _SKB_FRAME_TYPE_;
#if !defined(RTL8192SU)	
	pdescinfo->paddr = get_desc(pdesc->Dword8); // TXBufferAddr
#endif	
	pdescinfo->pframe = txcfg->pframe;
	pdescinfo->len = txcfg->fr_len;
	pdescinfo->priv = priv;

	txdesc_rollover(pdesc, (unsigned int *)tx_head);

	priv->amsdu_len += tx_len;

	if (txcfg->aggre_en == FG_AGGRE_MSDU_LAST) {
		pfrstdesc = priv->amsdu_first_desc;
//		pfrstdesc->cmd = set_desc((get_desc(pfrstdesc->cmd) &0xff0000) | priv->amsdu_len | _CMDINIT_ | _FS_ | _OWN_);
		pfrstdesc->Dword0 = set_desc((get_desc(pfrstdesc->Dword0) &0xff0000) | priv->amsdu_len | TX_FirstSeg | TX_OWN);

//		pfrstdesc->PktSize = priv->amsdu_len;
//		pfrstdesc->FirstSeg  = 1;
//		pfrstdesc->OWN = 1;
		// how about other field?  I dont know ... Joshua
		//pfrstdesc->AMSDU = 0;//?
#ifndef USE_RTL8186_SDK
		rtl_cache_sync_wback(priv, priv->amsdu_first_dma_desc, sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
		tx_poll(priv, q_num);
	}
}
#endif // SUPPORT_TX_AMSDU


#ifdef FW_SW_BEACON
int rtl8192SE_SendBeaconByCmdQ(struct rtl8190_priv *priv, unsigned char *dat_content, unsigned short txLength)
{
	struct tx_desc *pdesc;
	struct tx_desc *phdesc;
	int	*tx_head;
	struct rtl8190_hw *phw;

	phw = GET_HW(priv);
	tx_head	= &phw->txcmdhead;
	phdesc = phw->txcmd_desc;
	pdesc = phdesc + (*tx_head);

	if (get_desc(pdesc->Dword0) & TX_OWN) {
		return FALSE;
	}

	// Clear all status
	memset(pdesc, 0, 32);

	pdesc->Dword0 |= set_desc(TX_FirstSeg | TX_LastSeg | ((32)<<TX_OFFSETSHIFT));
	pdesc->Dword0 |= set_desc((unsigned short)(txLength) << TX_PktSizeSHIFT);
	pdesc->Dword2 |= set_desc((GetSequence(dat_content) & TX_SeqMask) << TX_SeqSHIFT);
	pdesc->Dword4 |= set_desc((0x08 << TX_RTSRateSHIFT) | (0x7 << TX_RaBRSRIDSHIFT) | TX_UserRate);
	pdesc->Dword5 |= set_desc(TX_DISFB);
	pdesc->Dword7 |= set_desc((unsigned short)(txLength) << TX_TxBufferSizeSHIFT);
	pdesc->Dword8 = set_desc(get_physical_addr(priv, dat_content, txLength, PCI_DMA_TODEVICE));
	rtl_cache_sync_wback(priv, (unsigned int)dat_content, txLength, PCI_DMA_TODEVICE);
	pdesc->Dword1 |= set_desc(0x13 << TX_QueueSelSHIFT);	// sw beacon
	pdesc->Dword0 |= set_desc(TX_OWN);

#ifndef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, phw->txcmd_desc_dma_addr[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif

	*tx_head = (*tx_head + 1) & (NUM_CMD_DESC - 1);

	return TRUE;
}
#endif


//-----------------------------------------------------------------------------
// Procedure:    Fill Tx Command Packet Descriptor
//
// Description:   This routine fill command packet descriptor. We assum that command packet
//				require only one descriptor.
//
// Arguments:   This function is only for Firmware downloading in current stage
//
// Returns:
//-----------------------------------------------------------------------------
int rtl8192SE_SetupOneCmdPacket(struct rtl8190_priv *priv, unsigned char *dat_content, unsigned short txLength, unsigned char LastPkt)
/*
	IN	PADAPTER		Adapter,
	IN	PRT_TCB			pTcb,
	IN	u1Byte			QueueIndex,
	IN	u2Byte			index,
	IN	BOOLEAN			bFirstSeg,
	IN	BOOLEAN			bLastSeg,
	IN	pu1Byte			VirtualAddress,
	IN	u4Byte			PhyAddressLow,
	IN	u4Byte			BufferLen,
	IN	BOOLEAN     		bSetOwnBit,
	IN	BOOLEAN			bLastInitPacket,
	IN    u4Byte			DescPacketType,
	IN	u4Byte			PktLen
	)
*/
{

	unsigned char	ih=0;
	unsigned char	DescNum;
	unsigned short	DebugTimerCount;

	struct tx_desc	*pdesc;
	struct tx_desc	*phdesc;
	volatile unsigned int *ppdesc  ; //= (unsigned int *)pdesc;
	struct rtl8190_hw	*phw = GET_HW(priv);	
#if !defined(RTL8192SU)
	int	*tx_head, *tx_tail;
	tx_head	= &phw->txcmdhead;
	tx_tail = &phw->txcmdtail;
#else
	unsigned int *tx;
	struct urb *tx_urb=NULL;
	int urb_len=0, frame_len=0;
	int status=0;
	struct tx_desc_info	*pdescinfo=NULL;

	if(0 == (txLength & 0x1ff))
	{
		frame_len = txLength+1;
	}
#endif

	phdesc = phw->txcmd_desc;

	DebugTimerCount = 0; // initialize debug counter to exit loop
	DescNum = 1;

//TODO: Fill the dma check here

//	printk("data lens: %d\n", txLength );

#ifdef RTL8192SU
		urb_len = txLength+(8*4);//;+USB_HWDESC_HEADER_LEN;
		{
			tx = kmalloc(urb_len, GFP_ATOMIC);
			if(!tx) return -ENOMEM;
		}

		printk("urb_len=%d, txLength=%d (USB_HWDESC_HEADER_LEN=%d)\n", urb_len, txLength, USB_HWDESC_HEADER_LEN);
		memset(tx, 0, sizeof(unsigned int) * 8);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	 	tx_urb = usb_alloc_urb(0,GFP_ATOMIC);
#else
 		tx_urb = usb_alloc_urb(0);
#endif

		if(!tx_urb)
		{
			kfree(tx);
			return -ENOMEM;
		}
#endif
	for (ih=0; ih<DescNum; ih++) {
#if !defined(RTL8192SU)
		pdesc      = (phdesc + (*tx_head));
#else
		pdesc   	= (struct tx_desc*)tx;
		pdescinfo = kmalloc(sizeof(struct tx_desc_info), GFP_ATOMIC);
		if(!pdescinfo) return -ENOMEM;
		pdescinfo->pframe = tx;
#endif
		ppdesc = (unsigned int *)pdesc;
		// Clear all status
#if !defined(RTL8192SU)
		memset(pdesc, 0, 36);
#endif
//		rtl_cache_sync_wback(priv, phw->txcmd_desc_dma_addr[*tx_head], 32, PCI_DMA_TODEVICE);
		// For firmware downlaod we only need to set LINIP
		if (LastPkt)
#ifndef DISABLE_UNALIGN_TRAP
			pdesc->Dword0 |= set_desc(TX_LINIP);
#else
			OR_EQUAL(&pdesc->Dword0 , set_desc(TX_LINIP));
#endif

		// From Scott --> 92SE must set as 1 for firmware download HW DMA error
#ifndef DISABLE_UNALIGN_TRAP
		pdesc->Dword0 |= set_desc(TX_FirstSeg);;//bFirstSeg;
		pdesc->Dword0 |= set_desc(TX_LastSeg);;//bLastSeg;
#else
		OR_EQUAL(&pdesc->Dword0 , set_desc(TX_FirstSeg));//bFirstSeg;
		OR_EQUAL(&pdesc->Dword0 , set_desc(TX_LastSeg));//bLastSeg;
#endif

#if !defined(RTL8192SU)
		// 92SE need not to set TX packet size when firmware download
		pdesc->Dword7 |=  (set_desc((unsigned short)(txLength) << TX_TxBufferSizeSHIFT));
#endif
		memcpy(priv->pshare->txcmd_buf, dat_content, txLength);

#if !defined(RTL8192SU)
		rtl_cache_sync_wback(priv, (unsigned int)priv->pshare->txcmd_buf, txLength, PCI_DMA_TODEVICE);


		pdesc->Dword8 =  set_desc(priv->pshare->cmdbuf_phyaddr);
#else
		memcpy(tx+8, dat_content, txLength);
#endif		

#ifndef DISABLE_UNALIGN_TRAP
		pdesc->Dword0	|= set_desc((unsigned short)(txLength) << TX_PktIDSHIFT);
#else
		OR_EQUAL(&pdesc->Dword0, set_desc((unsigned short)(txLength) << TX_PktIDSHIFT));
#endif
		//if (bSetOwnBit)
		{
#ifndef DISABLE_UNALIGN_TRAP
			pdesc->Dword0 |= set_desc(TX_OWN);
#else
			OR_EQUAL(&pdesc->Dword0 , set_desc(TX_OWN));
#endif

//			*(ppdesc) |= set_desc(BIT(31));
		}

#if !defined(RTL8192SU)
#ifndef USE_RTL8186_SDK
		rtl_cache_sync_wback(priv, phw->txcmd_desc_dma_addr[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
		*tx_head = (*tx_head + 1) & (NUM_CMD_DESC - 1);
#else
	{
		/* FIXME check what EP is for low/norm PRI */
		usb_fill_bulk_urb(tx_urb,priv->udev,
				usb_sndbulkpipe(priv->udev,0xd), tx,
				urb_len, (usb_complete_t)rtl8192su_tx_cmdPkt, (void *)pdescinfo);

	}

	dma_cache_wback_inv((unsigned long)tx,urb_len);
	//rtl_cache_sync_wback(priv, (unsigned int)tx, urb_len, 0);
#ifdef RTL867X_CP3
rtl8651_romeperfEnterPoint(ROMEPERF_INDEX_USB_SUBMIT_URB_TX );
#endif
	//status = RTL8192SU_submit_urb(priv, tx_urb);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	status = usb_submit_urb(tx_urb, GFP_ATOMIC);
#else
	status = usb_submit_urb(tx_urb);
#endif
#ifdef RTL867X_CP3
rtl8651_romeperfExitPoint(ROMEPERF_INDEX_USB_SUBMIT_URB_TX);
#endif
	if (status){
		printk("Error TX URB ,error %d\n",
				//atomic_read((priority == NORM_PRIORITY)? &priv->tx_np_pending : &priv->tx_lp_pending),
				//atomic_read(&priv->tx_pending[q_num]),
				status);
		//return -1;
	}
#endif	//RTL8192SU
	}

	return TRUE;
}


#elif defined(RTL8190) || defined(RTL8192E)


__IRAM_FASTEXTDEV
static void rtl8190_fill_fwinfo(struct rtl8190_priv *priv, struct tx_insn* txcfg, unsigned char *ptxfw, unsigned int frag_idx)
{
	struct FWtemplate *txfw = (struct FWtemplate *)ptxfw;
	int erp_protection = 0, n_protection = 0;
	unsigned char rate, ampdu_des;

	memset(txfw, 0, sizeof(struct FWtemplate)); // initialize to zero

#ifdef MP_TEST
	if (OPMODE & WIFI_MP_STATE) {
		if (is_MCS_rate(txcfg->tx_rate)) {	// HT rates
			txfw->txRate = txcfg->tx_rate & 0x7f;
			txfw->txHt = 1;
		}
		else
			txfw->txRate = get_rate_index_from_ieee_value((UINT8)txcfg->tx_rate);

		if (priv->pshare->CurrentChannelBW) {
			txfw->txbw = 1;
			txfw->txSC = 3;
			if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor40M)
				txfw->txshort = 1;
		}
		else {
			if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor20M)
				txfw->txshort = 1;
		}

		if (txcfg->retry) {
			txfw->ccx = 1;
			txfw->retryLimit1 = txcfg->retry & 0x3;
			txfw->retryLimit2 = (txcfg->retry >> 2) & 0x3;
		}

		return;
	}
#endif

	if (is_MCS_rate(txcfg->tx_rate))	// HT rates
	{
		txfw->txRate = txcfg->tx_rate & 0x7f;
		txfw->txHt = 1;

		if (priv->pshare->is_40m_bw) {
//			if (txcfg->pstat && (txcfg->pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_))) {
			if (txcfg->pstat && (txcfg->pstat->tx_bw == HT_CHANNEL_WIDTH_20_40)) {
				txfw->txbw = 1;
#ifdef RTL8190
				txfw->txSC = 3;
#elif defined(RTL8192E)
				txfw->txSC = 0; //By SD3's Jerry suggestion, use duplicated mode, cosa 04012008
#endif

				if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor40M &&
					txcfg->pstat && (txcfg->pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SHORTGI_40M_)))
					txfw->txshort = 1;

				if (priv->pshare->bw_pwrdiff_ofst != 0) {
					txfw->txAGCOffset = priv->pshare->bw_pwrdiff_ofst;
					txfw->txAGCSign = priv->pshare->bw_pwrdiff_sign;
				}
			}
			else {
				if (priv->pshare->offset_2nd_chan == HT_2NDCH_OFFSET_BELOW)
					txfw->txSC = 2;
				else
					txfw->txSC = 1;

				if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor20M &&
					txcfg->pstat && (txcfg->pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SHORTGI_20M_)))
					txfw->txshort = 1;
			}
		}
		else {
			if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor20M &&
				txcfg->pstat && (txcfg->pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SHORTGI_20M_)))
				txfw->txshort = 1;
		}

		if ((txcfg->aggre_en >= FG_AGGRE_MPDU) && (txcfg->aggre_en <= FG_AGGRE_MPDU_BUFFER_LAST)) {

			txfw->aggren = 1;
			switch (priv->pmib->dot11nConfigEntry.dot11nAMPDUSendSz) {
			case 8:
				txfw->rxMF = 0;
				break;
			case 16:
				txfw->rxMF = 1;
				break;
			case 32:
				txfw->rxMF = 2;
				break;
			case 64:
				txfw->rxMF = 3;
				break;
			default:
				if (txcfg->pstat->is_rtl8190_sta) {
					txfw->rxMF = 3;
				}
				else {
					//txfw->rxMF = txcfg->pstat->ht_cap_buf.ampdu_para & 0x03;
					if ((txcfg->pstat->ht_cap_buf.ampdu_para & 0x03) > 0)
						txfw->rxMF = 1;		// default 16K of AMPDU size to other clients support more than 8K
					else
						txfw->rxMF = 0;		// default 8K of AMPDU size to other clients support 8K only
				}
				break;
			}
			ampdu_des = (txcfg->pstat->ht_cap_buf.ampdu_para & _HTCAP_AMPDU_SPC_MASK_) >> _HTCAP_AMPDU_SPC_SHIFT_;
			if ((ampdu_des > 0) && (ampdu_des < 7))
				ampdu_des++; // 8190 Spec doesn't fit to 802.11n
			if (priv->pshare->is_40m_bw && txcfg->pstat
				&& (txcfg->pstat->tx_bw == HT_CHANNEL_WIDTH_20_40)
				&& (get_sta_encrypt_algthm(priv, txcfg->pstat) == _CCMP_PRIVACY_)
				&& txcfg->pstat->dot11KeyMapping.keyInCam == TRUE)
				txfw->rxAMD = 7;
			else
				txfw->rxAMD = ampdu_des;
		}
	}
	else	// legacy rate
	{
		txfw->txRate = get_rate_index_from_ieee_value((UINT8)txcfg->tx_rate);
		if (is_CCK_rate(txcfg->tx_rate) && (txcfg->tx_rate != 2)) {
			if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) &&
					(priv->pmib->dot11ErpInfo.longPreambleStaNum > 0))
				; // txfw->txshort = 0
			else {
				if (txcfg->pstat)
					txfw->txshort = (priv->pmib->dot11RFEntry.shortpreamble) &&
									(txcfg->pstat->useShortPreamble);
				else
					txfw->txshort = priv->pmib->dot11RFEntry.shortpreamble;
			}
		}

		if (priv->pshare->is_40m_bw) {
			if (priv->pshare->offset_2nd_chan == HT_2NDCH_OFFSET_BELOW)
				txfw->txSC = 2;
			else
				txfw->txSC = 1;
		}

		// 20080311 Bryant modified Tx power differences and omitted L-OFDM setting
		//if (!is_CCK_rate(txcfg->tx_rate) && !priv->pshare->use_default_para) {
		//	txfw->txAGCOffset = priv->pshare->legacyOFDM_pwrdiff & 0x0f;
		//	txfw->txAGCSign = (priv->pshare->legacyOFDM_pwrdiff & 0x80)? 1:0;
		//}
	}

	if (txcfg->need_ack) { // unicast
		if (frag_idx == 0) {
			if (txcfg->rts_thrshld <= get_mpdu_len(txcfg, txcfg->fr_len))
				txfw->rtsEn = 1;
			else {
				if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
					is_MCS_rate(txcfg->tx_rate) &&
					priv->ht_protection)
				{
					n_protection = 1;
					txfw->rtsEn = 1;
					if (priv->pmib->dot11ErpInfo.ctsToSelf)
						txfw->ctsEn = 1;
				}
				else if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) &&
					(!is_CCK_rate(txcfg->tx_rate)) && // OFDM mode
					priv->pmib->dot11ErpInfo.protection)
				{
					erp_protection = 1;
					txfw->rtsEn = 1;
					if (priv->pmib->dot11ErpInfo.ctsToSelf)
						txfw->ctsEn = 1;
				}
				else if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
					(txcfg->pstat) && (txcfg->pstat->MIMO_ps & _HT_MIMO_PS_DYNAMIC_) &&
					is_MCS_rate(txcfg->tx_rate) && ((txcfg->tx_rate & 0x7f)>7))
				{	// when HT MIMO Dynamic power save is set and rate > MCS7, RTS is needed
					txfw->rtsEn = 1;
				}
			}
		}
	}

	if (txfw->rtsEn == 1) {
		if (erp_protection)
			rate = (unsigned char)find_rate(priv, NULL, 1, 3);
		else
			rate = (unsigned char)find_rate(priv, NULL, 1, 1);

		if (is_MCS_rate(rate)) {	// HT rates
			// can we use HT rates for RTS?
		}
		else {
			txfw->rtsTxRate = get_rate_index_from_ieee_value(rate);
			if (erp_protection) {
				if (is_CCK_rate(rate) && (rate != 2)) {
					if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) &&
							(priv->pmib->dot11ErpInfo.longPreambleStaNum > 0))
						txfw->rtsShort = 0;
					else {
						if (txcfg->pstat)
							txfw->rtsShort = (priv->pmib->dot11RFEntry.shortpreamble) &&
											(txcfg->pstat->useShortPreamble);
						else
							txfw->rtsShort = priv->pmib->dot11RFEntry.shortpreamble;
					}
				}
			}
			else if (n_protection) {
				txfw->rtsTxRate = get_rate_index_from_ieee_value(48);	// User 24M to send RTS for TXOP protection
				if (priv->pshare->is_40m_bw) {
					txfw->rtsbw = 1;
					txfw->rtsSC = 0;	// duplicate RTS
				}
			}
			else {	// > RTS threshold
			}
		}
	}

	if (txcfg->need_ack) {
		txfw->enCPUDur = 1;	// no need to count duration for broadcast & multicast pkts

		// give retry limit to management frame
		if (txcfg->q_num == MANAGE_QUE_NUM) {
			txfw->ccx = 1;
			if (GetFrameSubType(txcfg->phdr) == WIFI_PROBERSP) {
				txfw->retryLimit1 = 0;
				txfw->retryLimit2 = 0;
			}
#ifdef WDS
			else if ((GetFrameSubType(txcfg->phdr) == WIFI_PROBEREQ) && (txcfg->pstat->state & WIFI_WDS)) {
				txfw->retryLimit1 = 2;
				txfw->retryLimit2 = 0;
			}
#endif
			else {
				txfw->retryLimit1 = 6 & 0x3;
				txfw->retryLimit2 = (6 >> 2) & 0x3;
			}
		}
#ifdef WDS
		else if (txcfg->wdsIdx >= 0) {
			if (txcfg->pstat->rx_avarage == 0) {
				txfw->ccx = 1;
				txfw->retryLimit1 = 3;
				txfw->retryLimit2 = 0;
			}
		}
#endif
	}
}


//__IRAM_IN_865X
void signin_txdesc(struct rtl8190_priv *priv, struct tx_insn* txcfg)
{
	struct tx_desc		*phdesc, *pdesc, *pndesc, *picvdesc=NULL, *pmicdesc, *pfrstdesc;
	struct tx_desc_info	*pswdescinfo, *pdescinfo, *pndescinfo, *picvdescinfo, *pmicdescinfo;
	unsigned int 		fr_len, tx_len, i, keyid=0;
	int					*tx_head, q_num;
	unsigned long		tmpphyaddr;
	unsigned char		*da, *pbuf, *pwlhdr, *pmic=NULL, *picv=NULL;
	struct rtl8190_hw	*phw;
	unsigned char		*pfwinfo=NULL, q_select=0;
#ifdef TX_SHORTCUT
	int	fit_shortcut = 0;
#endif
	unsigned int		pfrst_dma_desc=0;
	unsigned int		*dma_txhead;

	unsigned long flush_addr[20];
	int flush_len[20];
	int flush_num=0;

	if (txcfg->tx_rate == 0) {
		DEBUG_ERR("tx_rate=0!\n");
		txcfg->tx_rate = find_rate(priv, NULL, 0, 1);
	}

	q_num = txcfg->q_num;
	phw	= GET_HW(priv);

	dma_txhead	= get_txdma_addr(phw, q_num);
	tx_head		= get_txhead_addr(phw, q_num);
	phdesc   	= get_txdesc(phw, q_num);
	pswdescinfo = get_txdesc_info(priv->pshare->pdesc_info, q_num);

	tx_len = txcfg->fr_len;

	if (txcfg->fr_type == _SKB_FRAME_TYPE_)
		pbuf = ((struct sk_buff *)txcfg->pframe)->data;
	else
		pbuf = (unsigned char*)txcfg->pframe;

	da = get_da((unsigned char *)txcfg->phdr);

	tmpphyaddr = get_physical_addr(priv, pbuf, tx_len, PCI_DMA_TODEVICE);
#ifdef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, (unsigned int)pbuf, tx_len, PCI_DMA_TODEVICE);
#endif

	// in case of default key, then find the key id
	if (GetPrivacy((txcfg->phdr)))
	{
#ifdef WDS
		if (txcfg->wdsIdx >= 0) {
			if (txcfg->pstat)
				keyid = txcfg->pstat->keyid;
			else
				keyid = 0;
		}
		else
#endif

#ifdef __DRAYTEK_OS__
		if (!IEEE8021X_FUN)
			keyid = priv->pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex;
		else {
			if (IS_MCAST(GetAddr1Ptr ((unsigned char *)txcfg->phdr)) || !txcfg->pstat)
				keyid = priv->pmib->dot11GroupKeysTable.keyid;
			else
				keyid = txcfg->pstat->keyid;
		}
#else

		if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm==_WEP_40_PRIVACY_ ||
			priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm==_WEP_104_PRIVACY_) {
			if(IEEE8021X_FUN && txcfg->pstat) {
				if(IS_MCAST(da))
					keyid = 0;
				else
					keyid = txcfg->pstat->keyid;
			}
			else
				keyid = priv->pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex;
		}
#endif

//modify by Joule for SECURITY	0115
#ifdef CONFIG_RTK_MESH
		if( txcfg->is_11s )
			keyid = priv->pmib->dot11sKeysTable.keyid;
#endif
	}

	for(i=0, pfrstdesc= phdesc + (*tx_head); i < txcfg->frg_num; i++)
	{
		/*------------------------------------------------------------*/
		/*           fill descriptor of header + iv + llc             */
		/*------------------------------------------------------------*/
		pdesc     = phdesc + (*tx_head);
		pdescinfo = pswdescinfo + *tx_head;

		if (i != 0)
		{
			// we have to allocate wlan_hdr
			pwlhdr = (UINT8 *)get_wlanhdr_from_poll(priv);
			if (pwlhdr == (UINT8 *)NULL)
			{
				DEBUG_ERR("System-bug... should have enough wlan_hdr\n");
				return;
			}
			// other MPDU will share the same seq with the first MPDU
			memcpy((void *)pwlhdr, (void *)(txcfg->phdr), txcfg->hdr_len); // data pkt has 24 bytes wlan_hdr
		}
		else
		{
#ifdef SEMI_QOS
			if (txcfg->pstat && (is_qos_data(txcfg->phdr))) {
				if ((GetFrameSubType(txcfg->phdr) & (WIFI_DATA_TYPE | BIT(6) | BIT(7))) == (WIFI_DATA_TYPE | BIT(7))) {
					unsigned char tempQosControl[2];
					memset(tempQosControl, 0, 2);
					tempQosControl[0] = ((struct sk_buff *)txcfg->pframe)->cb[1];
#ifdef WMM_APSD
					if ((APSD_ENABLE) && (txcfg->pstat) && (txcfg->pstat->state & WIFI_SLEEP_STATE) &&
						(!GetMData(txcfg->phdr)) &&
						((((tempQosControl[0] == 7) || (tempQosControl[0] == 6)) && (txcfg->pstat->apsd_bitmap & 0x01)) ||
						 (((tempQosControl[0] == 5) || (tempQosControl[0] == 4)) && (txcfg->pstat->apsd_bitmap & 0x02)) ||
						 (((tempQosControl[0] == 3) || (tempQosControl[0] == 0)) && (txcfg->pstat->apsd_bitmap & 0x08)) ||
						 (((tempQosControl[0] == 2) || (tempQosControl[0] == 1)) && (txcfg->pstat->apsd_bitmap & 0x04))))
						tempQosControl[0] |= BIT(4);
#endif
					if (txcfg->aggre_en == FG_AGGRE_MSDU_FIRST)
						tempQosControl[0] |= BIT(7);

					memcpy((void *)GetQosControl((txcfg->phdr)), tempQosControl, 2);
				}
			}
#endif
			assign_wlanseq(GET_HW(priv), txcfg->phdr, txcfg->pstat, GET_MIB(priv)
#ifdef CONFIG_RTK_MESH
				, txcfg->is_11s
#endif
				);
			pwlhdr = txcfg->phdr;
		}
		SetDuration(pwlhdr, 0);

		pfwinfo = pwlhdr - sizeof(struct FWtemplate);
		rtl8190_fill_fwinfo(priv, txcfg, pfwinfo, i);

		if (i != (txcfg->frg_num - 1))
		{
			SetMFrag(pwlhdr);
			if (i == 0) {
				fr_len = (txcfg->frag_thrshld - txcfg->llc);
				tx_len -= (txcfg->frag_thrshld - txcfg->llc);
			}
			else {
				fr_len = txcfg->frag_thrshld;
				tx_len -= txcfg->frag_thrshld;
			}
		}
		else	// last seg, or the only seg (frg_num == 1)
		{
			fr_len = tx_len;
			ClearMFrag(pwlhdr);
		}
		SetFragNum((pwlhdr), i);

		if ((i == 0) && (txcfg->fr_type == _SKB_FRAME_TYPE_))
		{
			pdesc->flen = set_desc(txcfg->hdr_len + txcfg->llc + sizeof(struct FWtemplate));
			pdesc->cmd = set_desc((fr_len + get_desc(pdesc->flen) - sizeof(struct FWtemplate)) | _CMDINIT_ | _FS_);
			pdescinfo->type = _PRE_ALLOCLLCHDR_;
		}
		else
		{
			pdesc->flen = set_desc(txcfg->hdr_len + sizeof(struct FWtemplate));
			pdesc->cmd = set_desc((fr_len + get_desc(pdesc->flen) - sizeof(struct FWtemplate))	| _CMDINIT_ | _FS_);
			pdescinfo->type = _PRE_ALLOCHDR_;
		}

#ifdef _11s_TEST_MODE_
		mesh_debug_tx9(txcfg, pdescinfo);
#endif

		pdesc->opt = set_desc(sizeof(struct FWtemplate));
		pdesc->cmd |= set_desc((sizeof(struct FWtemplate) + 8) << _OFFSETSHIFT_);
		switch (q_num) {
#ifdef SW_MCAST
		case VO_QUEUE:
			q_select = 6;
			break;

		case VI_QUEUE:
			q_select = 5;
			break;
#else
		case HIGH_QUEUE:
			q_select = 0x11;
			break;
#endif
		case MGNT_QUEUE:
			q_select = 0x12;
			break;
		default:
			// data packet
			if (txcfg->tpt_pkt)
				q_select = 0x15;
			else
				q_select = ((struct sk_buff *)txcfg->pframe)->cb[1];
			break;
		}
		pdesc->opt |= set_desc(q_select << _QSELECTSHIFT_);
		if (i != (txcfg->frg_num - 1))
			pdesc->opt |= set_desc(_MFRAG_);

		// Give RATid
		if (txcfg->pstat) {
			if (txcfg->pstat->aid != MANAGEMENT_AID)	{ // AID 7 use RATid 0
				pdesc->opt |= set_desc((txcfg->pstat->aid & 0x0007) << _RATIDSHIFT_);
				((struct FWtemplate *)pfwinfo)->rsvd0 = (txcfg->pstat->aid & 0x0018) >> 3;
			}
		}
		else {
			pdesc->opt |= set_desc((7 & 0x0007) << _RATIDSHIFT_);
			((struct FWtemplate *)pfwinfo)->rsvd0 = (7 & 0x0018) >> 3;
		}

/*
		if (
#ifdef WDS
				(txcfg->pstat && (txcfg->pstat->state & WIFI_WDS) &&
					priv->pmib->dot11WdsInfo.entry[txcfg->pstat->wds_idx].txRate) ||
				(txcfg->pstat && !(txcfg->pstat->state & WIFI_WDS) &&
						!priv->pmib->dot11StationConfigEntry.autoRate) ||
#else
				(!priv->pmib->dot11StationConfigEntry.autoRate) ||
#endif
				(txcfg->pstat == NULL) ||
				(txcfg->fr_type != _SKB_FRAME_TYPE_))
			pdesc->opt |= set_desc(_USERATE_ | _DISFB_);
*/
		if (txcfg->fixed_rate
#ifdef RTL8192E
			|| txcfg->pstat->remapped_aid == FW_NUM_STAT-1/*(priv->pshare->STA_map & BIT(txcfg->pstat->aid)*/)
#endif
			)
			pdesc->opt |= set_desc(_USERATE_ | _DISFB_);

		if (!txcfg->need_ack && txcfg->privacy && UseSwCrypto(priv, NULL, TRUE))
			pdesc->opt |= set_desc(_NOENC_);

		if (txcfg->privacy)
		{
			if (UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE)))
			{
				pdesc->cmd = set_desc(get_desc(pdesc->cmd) + txcfg->icv + txcfg->mic + txcfg->iv);
				pdesc->flen = set_desc(get_desc(pdesc->flen) + txcfg->iv);
			}
			else // hw encrypt
			{
				switch(txcfg->privacy)
				{
				case _WEP_104_PRIVACY_:
				case _WEP_40_PRIVACY_:
					pdesc->cmd = set_desc(get_desc(pdesc->cmd) + txcfg->iv);
					pdesc->flen = set_desc(get_desc(pdesc->flen) + txcfg->iv);
					wep_fill_iv(priv, pwlhdr, txcfg->hdr_len, keyid);
					pdesc->opt = set_desc(get_desc(pdesc->opt) | BIT(30));
					break;

				case _TKIP_PRIVACY_:
					pdesc->cmd = set_desc(get_desc(pdesc->cmd) + txcfg->iv + txcfg->mic);
					pdesc->flen = set_desc(get_desc(pdesc->flen) + txcfg->iv);
					tkip_fill_encheader(priv, pwlhdr, txcfg->hdr_len, keyid);
					pdesc->opt = set_desc(get_desc(pdesc->opt) | BIT(31));
					if ((!IS_MCAST(da)) && (txcfg->pstat))
						pdesc->opt |= set_desc(BIT(29) | ((txcfg->pstat->cam_id) & 0x1f)<<24);
					break;

				case _CCMP_PRIVACY_:
					//michal also hardware...
					pdesc->cmd = set_desc(get_desc(pdesc->cmd) + txcfg->iv);
					pdesc->flen = set_desc(get_desc(pdesc->flen) + txcfg->iv);
					aes_fill_encheader(priv, pwlhdr, txcfg->hdr_len, keyid);
					pdesc->opt = set_desc(get_desc(pdesc->opt) | BIT(31) | BIT(30));
					if ((!IS_MCAST(da)) && (txcfg->pstat))
						pdesc->opt |= set_desc(BIT(29) | ((txcfg->pstat->cam_id) & 0x1f)<<24);
					break;

				default:
					DEBUG_ERR("Unknow privacy\n");
					break;
				}
			}
		}
		pdesc->paddr = set_desc(get_physical_addr(priv, pwlhdr-sizeof(struct FWtemplate),
			(get_desc(pdesc->flen)&0xffff), PCI_DMA_TODEVICE));

		// below is for sw desc info
		pdescinfo->paddr  = get_desc(pdesc->paddr);
		pdescinfo->pframe = pwlhdr;
#if defined(SEMI_QOS) && defined(WMM_APSD)
		pdescinfo->priv = priv;
		pdescinfo->pstat = txcfg->pstat;
#endif

#ifdef TX_SHORTCUT
		if (!priv->pmib->dot11OperationEntry.disable_txsc && txcfg->pstat &&
				(txcfg->fr_type == _SKB_FRAME_TYPE_)) {
			desc_copy(&txcfg->pstat->hwdesc1, pdesc);
			descinfo_copy(&txcfg->pstat->swdesc1, pdescinfo);
			txcfg->pstat->protection = priv->pmib->dot11ErpInfo.protection;
			txcfg->pstat->sc_keyid = keyid;
			txcfg->pstat->pktpri = ((struct sk_buff *)txcfg->pframe)->cb[1];
			memcpy(&txcfg->pstat->fw_bckp, pfwinfo, sizeof(struct FWtemplate));
			fit_shortcut = 1;
		}
		else {
			if (txcfg->pstat)
					txcfg->pstat->hwdesc1.flen = 0;
		}
#endif

		pfrst_dma_desc = dma_txhead[*tx_head];

		if (i != 0) {
			pdesc->cmd |= set_desc(_OWN_);
#ifndef USE_RTL8186_SDK
			rtl_cache_sync_wback(priv, dma_txhead[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
		}

#ifdef USE_RTL8186_SDK
		flush_addr[flush_num]=(unsigned long)bus_to_virt(get_desc(pdesc->paddr));
		flush_len[flush_num++]=get_desc(pdesc->flen);
#endif

		txdesc_rollover(pdesc, (unsigned int *)tx_head);

		if (txcfg->fr_len == 0)
		{
			if (txcfg->aggre_en != FG_AGGRE_MSDU_FIRST)
			  pdesc->cmd |= set_desc(_LS_);
			goto init_deschead;
		}

		/*------------------------------------------------------------*/
		/*              fill descriptor of frame body                 */
		/*------------------------------------------------------------*/
		pndesc     = phdesc + *tx_head;
		pndescinfo = pswdescinfo + *tx_head;

		pndesc->cmd	= set_desc((get_desc(pdesc->cmd) & (~_FS_)) | _OWN_);
		pndesc->rsvd0 = pdesc->rsvd0;
		pndesc->rsvd1 = pdesc->rsvd1;
		pndesc->rsvd2 = pdesc->rsvd2;

		if (txcfg->privacy)
		{
			if (!UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE)))
			{
				if (txcfg->aggre_en != FG_AGGRE_MSDU_FIRST)
				  pndesc->cmd |= set_desc(_LS_);
				pndescinfo->pstat = txcfg->pstat;
				pndescinfo->rate = txcfg->tx_rate;
			}
		}
		else
		{
			if (txcfg->aggre_en != FG_AGGRE_MSDU_FIRST)
			  pndesc->cmd |= set_desc(_LS_);
			pndescinfo->pstat = txcfg->pstat;
			pndescinfo->rate = txcfg->tx_rate;
		}

		pndesc->flen = set_desc(fr_len);

		if (i == 0)
			pndescinfo->type = txcfg->fr_type;
		else
			pndescinfo->type = _RESERVED_FRAME_TYPE_;

#if defined(CONFIG_RTK_MESH) && defined(MESH_USE_METRICOP)
		if( (txcfg->fr_type == _PRE_ALLOCMEM_) && (txcfg->is_11s & 128)) // for 11s link measurement frame
			pndescinfo->type =_RESERVED_FRAME_TYPE_;
#endif

#ifdef _11s_TEST_MODE_
		mesh_debug_tx10(txcfg, pndescinfo);
#endif

		pndesc->paddr = set_desc(tmpphyaddr);
		pndescinfo->paddr = get_desc(pndesc->paddr);
		pndescinfo->pframe = txcfg->pframe;
		pndescinfo->len = txcfg->fr_len;	// for pci_unmap_single
		pndescinfo->priv = priv;

		pbuf += fr_len;
		tmpphyaddr += fr_len;

#ifdef TX_SHORTCUT
		if (fit_shortcut) {
			desc_copy(&txcfg->pstat->hwdesc2, pndesc);
			descinfo_copy(&txcfg->pstat->swdesc2, pndescinfo);
		}
#endif

#ifndef USE_RTL8186_SDK
		rtl_cache_sync_wback(priv, dma_txhead[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif

		flush_addr[flush_num]=(unsigned long)bus_to_virt(get_desc(pndesc->paddr));
		flush_len[flush_num++]=get_desc(pndesc->flen);

		txdesc_rollover(pndesc, (unsigned int *)tx_head);

		// retrieve H/W MIC and put in payload
		if ((txcfg->privacy == _TKIP_PRIVACY_) &&
			(priv->pshare->have_hw_mic) &&
			!(priv->pmib->dot11StationConfigEntry.swTkipMic) &&
			(i == (txcfg->frg_num-1)) )	// last segment
		{
			register unsigned long int l,r;
			unsigned char *mic;
			volatile int i;

			while ((*(volatile unsigned int *)GDMAISR & GDMA_COMPIP) == 0)
				for (i=0; i<10; i++)
					;

			l = *(volatile unsigned int *)GDMAICVL;
			r = *(volatile unsigned int *)GDMAICVR;

			mic = ((struct sk_buff *)txcfg->pframe)->data + txcfg->fr_len - 8;
			mic[0] = (unsigned char)(l & 0xff);
			mic[1] = (unsigned char)((l >> 8) & 0xff);
			mic[2] = (unsigned char)((l >> 16) & 0xff);
			mic[3] = (unsigned char)((l >> 24) & 0xff);
			mic[4] = (unsigned char)(r & 0xff);
			mic[5] = (unsigned char)((r >> 8) & 0xff);
			mic[6] = (unsigned char)((r >> 16) & 0xff);
			mic[7] = (unsigned char)((r >> 24) & 0xff);

#ifdef MICERR_TEST
			if (priv->micerr_flag) {
				mic[7] ^= mic[7];
				priv->micerr_flag = 0;
			}
#endif
		}


		/*------------------------------------------------------------*/
		/*                insert sw encrypt here!                     */
		/*------------------------------------------------------------*/
		if (txcfg->privacy && UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE)))
		{
			if (txcfg->privacy == _TKIP_PRIVACY_ ||
				txcfg->privacy == _WEP_40_PRIVACY_ ||
				txcfg->privacy == _WEP_104_PRIVACY_)
			{
				picvdesc     = phdesc + *tx_head;
				picvdescinfo = pswdescinfo + *tx_head;
				if (txcfg->aggre_en == FG_AGGRE_MSDU_FIRST)
				  picvdesc->cmd = set_desc((get_desc(pdesc->cmd) & (~_FS_)) | _OWN_);
				else
				  picvdesc->cmd   = set_desc((get_desc(pdesc->cmd) & (~_FS_)) | _OWN_ | _LS_);
				picvdesc->flen  = set_desc(txcfg->icv);
				picvdesc->rsvd0 = pdesc->rsvd0;
				picvdesc->rsvd1 = pdesc->rsvd1;
				picvdesc->rsvd2 = pdesc->rsvd2;

				// append ICV first...
				picv = get_icv_from_poll(priv);
				if (picv == NULL)
				{
					DEBUG_ERR("System-Buf! can't alloc picv\n");
					BUG();
				}

				picvdescinfo->type = _PRE_ALLOCICVHDR_;
				picvdescinfo->pframe = picv;
				picvdescinfo->pstat = txcfg->pstat;
				picvdescinfo->rate = txcfg->tx_rate;
				picvdescinfo->priv = priv;
				picvdesc->paddr = set_desc(get_physical_addr(priv, picv, txcfg->icv, PCI_DMA_TODEVICE));
#ifdef USE_RTL8186_SDK
				rtl_cache_sync_wback(priv, (unsigned int)picv, txcfg->icv, PCI_DMA_TODEVICE);
#endif

				if (i == 0)
					tkip_icv(picv,
						pwlhdr + txcfg->hdr_len + txcfg->iv, txcfg->llc,
						pbuf - get_desc(pndesc->flen), get_desc(pndesc->flen));
				else
					tkip_icv(picv,
						NULL, 0,
						pbuf - get_desc(pndesc->flen), get_desc(pndesc->flen));

				if ((i == 0) && (txcfg->llc != 0)) {
					if (txcfg->privacy == _TKIP_PRIVACY_)
						tkip_encrypt(priv, pwlhdr, txcfg->hdr_len,
							pwlhdr + txcfg->hdr_len + 8, sizeof(struct llc_snap),
							pbuf - get_desc(pndesc->flen), get_desc(pndesc->flen), picv, txcfg->icv);
					else
						wep_encrypt(priv, pwlhdr, txcfg->hdr_len,
							pwlhdr + txcfg->hdr_len + 4, sizeof(struct llc_snap),
							pbuf - get_desc(pndesc->flen), get_desc(pndesc->flen), picv, txcfg->icv,
							txcfg->privacy);
				}
				else { // not first segment or no snap header
					if (txcfg->privacy == _TKIP_PRIVACY_)
						tkip_encrypt(priv, pwlhdr, txcfg->hdr_len, NULL, 0,
							pbuf - get_desc(pndesc->flen), get_desc(pndesc->flen), picv, txcfg->icv);
					else
						wep_encrypt(priv, pwlhdr, txcfg->hdr_len, NULL, 0,
							pbuf - get_desc(pndesc->flen), get_desc(pndesc->flen), picv, txcfg->icv,
							txcfg->privacy);
				}
#ifndef USE_RTL8186_SDK
				rtl_cache_sync_wback(priv, dma_txhead[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif

				flush_addr[flush_num]=(unsigned long)bus_to_virt(get_desc(picvdesc->paddr));
				flush_len[flush_num++]=get_desc(picvdesc->flen);

				txdesc_rollover(picvdesc, (unsigned int *)tx_head);
			}

			if (txcfg->privacy == _CCMP_PRIVACY_)
			{
				pmicdesc = phdesc + *tx_head;
				pmicdescinfo = pswdescinfo + *tx_head;
				if (txcfg->aggre_en == FG_AGGRE_MSDU_FIRST)
				  pmicdesc->cmd = set_desc((get_desc(pdesc->cmd) & (~_FS_)) | _OWN_);
				else
				  pmicdesc->cmd   = set_desc((get_desc(pdesc->cmd) & (~_FS_)) | _OWN_ | _LS_);
				pmicdesc->flen 	= set_desc(txcfg->mic);
				pmicdesc->rsvd0 = pdesc->rsvd0;
				pmicdesc->rsvd1 = pdesc->rsvd1;
				pmicdesc->rsvd2 = pdesc->rsvd2;

				// append MIC first...
				pmic = get_mic_from_poll(priv);
				if (pmic == NULL)
				{
					DEBUG_ERR("System-Buf! can't alloc pmic\n");
					BUG();
				}

				pmicdescinfo->type = _PRE_ALLOCMICHDR_;
				pmicdescinfo->pframe = pmic;
				pmicdescinfo->pstat = txcfg->pstat;
				pmicdescinfo->rate = txcfg->tx_rate;
				pmicdescinfo->priv = priv;
				pmicdesc->paddr = set_desc(get_physical_addr(priv, pmic, txcfg->mic, PCI_DMA_TODEVICE));
#ifdef USE_RTL8186_SDK
				rtl_cache_sync_wback(priv, (unsigned int)pmic, txcfg->mic, PCI_DMA_TODEVICE);
#endif

				// then encrypt all (including ICV) by AES
				if (i == 0) // encrypt 3 segments ==> llc, mpdu, and mic
					aesccmp_encrypt(priv, pwlhdr, pwlhdr + txcfg->hdr_len + 8,
						pbuf - get_desc(pndesc->flen), get_desc(pndesc->flen), pmic);
				else // encrypt 2 segments ==> mpdu and mic
					aesccmp_encrypt(priv, pwlhdr, NULL,
						pbuf - get_desc(pndesc->flen), get_desc(pndesc->flen), pmic);
#ifndef USE_RTL8186_SDK
				rtl_cache_sync_wback(priv, dma_txhead[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
				flush_addr[flush_num]=(unsigned long)bus_to_virt(get_desc(pmicdesc->paddr));
				flush_len[flush_num++]=get_desc(pmicdesc->flen);

				txdesc_rollover(pmicdesc, (unsigned int *)tx_head);
			}
		}
	}


init_deschead:
#if 0
	switch (q_select) {
	case 0:
	case 3:
	   if (q_num != BE_QUEUE)
    		printk("%s %d error : q_select[%d], q_num[%d]\n", __FUNCTION__, __LINE__, q_select, q_num);
	   break;
	case 1:
	case 2:
		if (q_num != BK_QUEUE)
		    printk("%s %d error : q_select[%d], q_num[%d]\n", __FUNCTION__, __LINE__, q_select, q_num);
	   break;
	case 4:
	case 5:
		if (q_num != VI_QUEUE)
		    printk("%s %d error : q_select[%d], q_num[%d]\n", __FUNCTION__, __LINE__, q_select, q_num);
		break;
	case 6:
	case 7:
		if (q_num != VO_QUEUE)
			printk("%s %d error : q_select[%d], q_num[%d]\n", __FUNCTION__, __LINE__, q_select, q_num);
		break;
	case 0x11 :
		 if (q_num != HIGH_QUEUE)
			printk("%s %d error : q_select[%d], q_num[%d]\n", __FUNCTION__, __LINE__, q_select, q_num);
		break;
	case 0x12 :
		if (q_num != MGNT_QUEUE)
			printk("%s %d error : q_select[%d], q_num[%d]\n", __FUNCTION__, __LINE__, q_select, q_num);
		break;
	default :
		printk("%s %d warning : q_select[%d], q_num[%d]\n", __FUNCTION__, __LINE__, q_select, q_num);
	break;
	}
#endif

	for (i=0; i<flush_num; i++)
		rtl_cache_sync_wback(priv, flush_addr[i], flush_len[i], PCI_DMA_TODEVICE);

	if (txcfg->aggre_en == FG_AGGRE_MSDU_FIRST) {
		priv->amsdu_first_desc = pfrstdesc;
#ifndef USE_RTL8186_SDK
		priv->amsdu_first_dma_desc = pfrst_dma_desc;
#endif
		priv->amsdu_len = get_desc(pfrstdesc->cmd) & 0xffff;
		return;
	}

#ifdef BUFFER_TX_AMPDU
	if ((txcfg->aggre_en >= FG_AGGRE_MPDU_BUFFER_FIRST) &&
					(txcfg->aggre_en <= FG_AGGRE_MPDU_BUFFER_LAST)) {
		if (txcfg->aggre_en == FG_AGGRE_MPDU_BUFFER_FIRST) {
			priv->ampdu_first_desc = pfrstdesc;
#ifndef USE_RTL8186_SDK
			priv->ampdu_first_dma_desc = pfrst_dma_desc;
#endif
		}
		else {
			pfrstdesc->cmd |= set_desc(_OWN_);

#ifndef USE_RTL8186_SDK
			rtl_cache_sync_wback(priv, pfrst_dma_desc, sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
			if (txcfg->aggre_en == FG_AGGRE_MPDU_BUFFER_LAST) {
				pfrstdesc = priv->ampdu_first_desc;
				pfrstdesc->cmd |= set_desc(_OWN_);

#ifndef USE_RTL8186_SDK
				rtl_cache_sync_wback(priv, priv->ampdu_first_dma_desc, sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
				tx_poll(priv, q_num);
			}
		}
		return;
	}
#endif // BUFFER_TX_AMPDU

	pfrstdesc->cmd |= set_desc(_OWN_);
#ifndef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, pfrst_dma_desc, sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif

#ifndef SW_MCAST
	if (q_num == HIGH_QUEUE) {
		priv->pkt_in_dtimQ = 1;
		return;
	}
	else
#endif
		tx_poll(priv, q_num);

	return;
}

#ifdef SUPPORT_TX_AMSDU
__IRAM_FASTEXTDEV
void signin_txdesc_amsdu(struct rtl8190_priv *priv, struct tx_insn* txcfg)
{
	struct tx_desc *phdesc, *pdesc, *pfrstdesc;
	struct tx_desc_info *pswdescinfo, *pdescinfo;
	unsigned int  tx_len;
	int *tx_head, q_num;
	unsigned long	tmpphyaddr;
	unsigned char *pbuf;
	struct rtl8190_hw *phw;
	unsigned int *dma_txhead;

	q_num = txcfg->q_num;
	phw	= GET_HW(priv);

	dma_txhead	= get_txdma_addr(phw, q_num);
	tx_head		= get_txhead_addr(phw, q_num);
	phdesc   	= get_txdesc(phw, q_num);
	pswdescinfo = get_txdesc_info(priv->pshare->pdesc_info, q_num);

	pbuf = ((struct sk_buff *)txcfg->pframe)->data;
	tx_len = ((struct sk_buff *)txcfg->pframe)->len;

	tmpphyaddr = get_physical_addr(priv, pbuf, tx_len, PCI_DMA_TODEVICE);
#ifdef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, (unsigned int)pbuf, tx_len, PCI_DMA_TODEVICE);
#endif

	pdesc     = phdesc + (*tx_head);
	pdescinfo = pswdescinfo + *tx_head;

	pdesc->paddr = set_desc(tmpphyaddr);
	pdesc->flen = set_desc(tx_len);
	if (txcfg->aggre_en == FG_AGGRE_MSDU_LAST)
		pdesc->cmd = set_desc(_OWN_ | _LS_);
	else
		pdesc->cmd = set_desc(_OWN_);

#ifndef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, dma_txhead[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif

	pdescinfo->type = _SKB_FRAME_TYPE_;
	pdescinfo->paddr = get_desc(pdesc->paddr);
	pdescinfo->pframe = txcfg->pframe;
	pdescinfo->len = txcfg->fr_len;
	pdescinfo->priv = priv;

	txdesc_rollover(pdesc, (unsigned int *)tx_head);

	priv->amsdu_len += tx_len;

	if (txcfg->aggre_en == FG_AGGRE_MSDU_LAST) {
		pfrstdesc = priv->amsdu_first_desc;
		pfrstdesc->cmd = set_desc((get_desc(pfrstdesc->cmd) &0xff0000) | priv->amsdu_len | _CMDINIT_ | _FS_ | _OWN_);
#ifndef USE_RTL8186_SDK
		rtl_cache_sync_wback(priv, priv->amsdu_first_dma_desc, sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
		tx_poll(priv, q_num);
	}
}
#endif // SUPPORT_TX_AMSDU

int SetupOneCmdPacket(struct rtl8190_priv *priv, unsigned char *dat_content,
				unsigned short txLength, unsigned char LastPkt)
{
	unsigned char	ih=0;
	unsigned char	DescNum;
	unsigned short	DebugTimerCount;

	struct tx_desc	*phdesc, *pdesc;
	int	*tx_head, *tx_tail;
	struct rtl8190_hw	*phw = GET_HW(priv);

	tx_head	= &phw->txcmdhead;
	tx_tail = &phw->txcmdtail;
	phdesc = phw->txcmd_desc;

	DebugTimerCount = 0; // initialize debug counter to exit loop
	DescNum = 1;

	//check the preceding desc is finished? to update tail (not in ISR)?  I am not sure.
	if (!(((*tx_head) == 0) && ((*tx_tail) == 0))) // not first desc during initialization
	{
		if((*tx_head) != 0)
			pdesc = phdesc + (*tx_head) - 1; // locate preceding descriptor pointer
		else
			pdesc = phdesc + NUM_CMD_DESC - 1; // last descriptor in chain

		while (((get_desc(pdesc->cmd) & _OWN_) != 0) && (DebugTimerCount < 100)) {
			DebugTimerCount++;
		}

		if (DebugTimerCount == 100) {
			printk("Preceding tx cmd buffer can't dma to NIC\n");
			return FALSE;
		}

		rtl_cache_sync_wback(priv, priv->pshare->cmdbuf_phyaddr, LoadPktSize, PCI_DMA_TODEVICE);

		*tx_tail = ((*tx_tail) + 1) & (NUM_CMD_DESC - 1);
	}


	for (ih=0; ih<DescNum; ih++) {
		pdesc     = phdesc + (*tx_head);
		memset(pdesc, 0, 12);

		// FS and TX MAC Header, descriptor set

		pdesc->cmd |= set_desc(_FS_);

		pdesc->flen = set_desc(txLength);

		memcpy(priv->pshare->txcmd_buf, dat_content, txLength);
		rtl_cache_sync_wback(priv, (unsigned int)priv->pshare->txcmd_buf, txLength, PCI_DMA_TODEVICE);

		pdesc->cmd |= set_desc(_LS_);   // Set Last Segment

		pdesc->paddr = set_desc(priv->pshare->cmdbuf_phyaddr);

		// if LastPkt = 1, the Init_LP bit must be set
		 if (LastPkt)
			pdesc->cmd |= set_desc(_LINIP_);

		pdesc->cmd |= set_desc(_OWN_);

#ifndef USE_RTL8186_SDK
		rtl_cache_sync_wback(priv, phw->txcmd_desc_dma_addr[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
		*tx_head = (*tx_head + 1) & (NUM_CMD_DESC - 1);
	}

	return TRUE;
}
#endif

#if !defined(RTL8192SU)
#ifndef __LINUX_2_6__
__MIPS16
#endif
#ifdef IRAM_FOR_WIRELESS_AND_WAPI_PERFORMANCE
#else
__IRAM_IN_865X_HI
#endif
#else
__IRAM_WIFI_PRI5
#endif
int __rtl8190_start_xmit(struct sk_buff *skb, struct net_device *dev, int tx_flag)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	struct stat_info	*pstat=NULL;
	unsigned char		*da;
	struct sk_buff *newskb = NULL;
	struct net_device *wdsDev = NULL;
#ifdef LOOPBACK_MODE
	local_irq_disable();
#endif

#if defined(RTL8192SU)&&defined(TX_SHORTCUT)
	unsigned int stat_fr_len=0;
#endif

#ifdef RTL867X_CP3
	rtl8651_romeperfEnterPoint(ROMEPERF_INDEX_8187_TX_XMIT );
	//rtl8651_romeperfEnterPoint(ROMEPERF_INDEX_STARTXMIT1 );
#endif

#ifdef CONFIG_RTL8671
	int k;
#endif

#if !defined(RTL8192SU)
	extern int no_ddr_patch;
#endif

	struct tx_insn tx_insn;
	DECLARE_TXCFG(txcfg, tx_insn);

//#ifdef CONFIG_RTL8671
	//IGMP snooping
	//if(enable_IGMP_SNP){
		//if (check_wlan_mcast_tx(skb)==1) {
			//printk("mcast drop!!!\n");
		//	goto free_and_stop;
		//}
	//}
//#endif


#ifdef CONFIG_RTK_MESH

	skb->dev = dev;

#ifdef	_11s_TEST_MODE_
	if(mesh_debug_tx1(dev, priv, skb)<0)
		return 0;
#endif

	if( dev == priv->mesh_dev)
	{
		if(1 == GET_MIB(priv)->dot1180211sInfo.mesh_enable)
		{
#ifdef	_11s_TEST_MODE_
			if(mesh_debug_tx2( priv, skb)<0)
				return 0;
#endif
			// 11s action, send by pathsel daemon
			static unsigned char zero14[14] = {0}; // the first 14 bytes is zero: 802.3: 6 bytes (src) + 6 bytes (dst) + 2 bytes (protocol)

			if(memcmp(skb->data, zero14, sizeof(zero14))==0)
			{
				if(issue_11s_mesh_action(skb, dev)==0)
					dev_kfree_skb_any(skb);
				return 0;
			}

			// drop any packet which has dest to STA but go throu MSH0
			{
				pstat = get_stainfo(priv, skb->data);
				if(pstat!=0 && isSTA(pstat))
				{
					dev_kfree_skb_any(skb);
					return 0;
				}
			}
	 		if(!dot11s_datapath_decision(skb, txcfg, 1)) //the dest form bridge need be update to proxy table
	 			return 0;

			tx_flag = TX_NO_MUL2UNI;
		}
		else
		{
			dev_kfree_skb_any(skb);
			return 0;
		}
	}
#endif // CONFIG_RTK_MESH

#ifdef WDS
	if (!dev->base_addr && IS_MCAST(skb->data) && skb_cloned(skb) &&
		(priv->pmib->dot11WdsInfo.wdsPrivacy == _TKIP_PRIVACY_)) {
		struct sk_buff *mcast_skb = NULL;
		mcast_skb = skb_copy(skb, GFP_ATOMIC);
#ifdef DEBUG_MEMORY_LEAK
		printk("%s %d freeskb=%x\n",__FUNCTION__,__LINE__,(unsigned int)skb);	
#endif
		rtl_kfree_skb(priv, skb, _SKB_TX_);
		if (mcast_skb == NULL) {
			priv->ext_stats.tx_drops++;
			DEBUG_ERR("TX DROP: Can't copy the skb!\n");
			return 0;
		}
		skb = mcast_skb;
	}
#endif

#ifdef	IGMP_FILTER_CMO
	if (priv->pshare->rf_ft_var.igmp_deny) 
	{
		if ((OPMODE & WIFI_AP_STATE) &&	IS_MCAST(skb->data))
		{
			if ((IP_MCAST_MAC(skb->data)
				#ifdef	TX_SUPPORT_IPV6_MCAST2UNI
				|| ICMPV6_MCAST_MAC(skb->data)
				#endif
				))
			{
				if(!IS_IGMP_PROTO(skb->data)){
					dev_kfree_skb_any(skb);
					return 0;
				}				
			}
		}
	
	}
#endif

#ifdef SUPPORT_TX_MCAST2UNI
	if (!priv->pshare->rf_ft_var.mc2u_disable) {
		if ((OPMODE & WIFI_AP_STATE) &&	IS_MCAST(skb->data))
		{
			if ((IP_MCAST_MAC(skb->data)
				#ifdef	TX_SUPPORT_IPV6_MCAST2UNI
				|| ICMPV6_MCAST_MAC(skb->data)
				#endif
				) && (tx_flag != TX_NO_MUL2UNI))
			{
				if (mlcst2unicst(priv, skb)){
					return 0;
				}
#ifdef	IGMP_FILTER_CMO
				else if(!IS_IGMP_PROTO(skb->data)){	
					//this multicast addr is not igmp type packet and it don't join by any one
					// (data type packet)
					dev_kfree_skb_any(skb);
					return 0;
				}
#endif
			}
		}

		skb->cb[2] = 0;	// allow aggregation
	}
#endif

/* WMMFIXME : srip VLAN tag and modify cb */
#ifdef CONFIG_RTL865X
	{
		const int VLAN_protocol_ID_idx=12;

#ifdef RTL8190_FASTEXTDEV_FUNCALL
		memset(skb->cb, 0, RTL865X_EXTDEV_MBUFINFO_CBOFFSET);
#else
		memset(skb->cb, 0, sizeof(skb->cb));
#endif
		if (skb->data[VLAN_protocol_ID_idx] == __constant_htons(0x81)
			&& skb->data[VLAN_protocol_ID_idx+1] == __constant_htons(0x00))//VLAN Protocol ID == 0x8100
		{
			char	skb_tmp[12];
			int vtag_priority;

			memcpy(skb_tmp, skb->data, sizeof(skb_tmp));
			vtag_priority = ((skb->data[VLAN_protocol_ID_idx+2])&0xe0) >> 5;
			skb->cb[0] = vtag_priority;
			skb_pull(skb, 4);
			memcpy(skb->data, skb_tmp, sizeof(skb_tmp));
		}
	}
#endif

#ifdef CONFIG_RTL8671
	// Mason Yu
	if ( g_port_mapping == TRUE ) {
		for (k=0; k<5; k++) {
			if ( skb->dev == wlanDev[k].dev_pointer) {
				if (wlanDev[k].dev_ifgrp_member!=0 && skb->vlan_member!=0 && skb->vlan_member!=wlanDev[k].dev_ifgrp_member) {
					goto free_and_stop;
				}
				break;
			}
		}
	}
#endif // CONFIG_RTL8671

#ifdef DFS
	if (!priv->pmib->dot11DFSEntry.disable_DFS &&
		priv->pmib->dot11DFSEntry.disable_tx) {
		priv->ext_stats.tx_drops++;
		DEBUG_ERR("TX DROP: DFS probation period\n");
		goto free_and_stop;
	}
#endif

	if (!IS_DRV_OPEN(priv))
		goto free_and_stop;

#ifdef MP_TEST
	if (OPMODE & WIFI_MP_STATE) {
		goto free_and_stop;
	}
#endif

#ifdef CONFIG_RTK_VLAN_SUPPORT
	if (priv->pmib->vlan.global_vlan) {
		int get_pri = 0;		
		if (QOS_ENABLE) {
#ifdef CLIENT_MODE
			if ((OPMODE & (WIFI_STATION_STATE | WIFI_ASOC_STATE)) == (WIFI_STATION_STATE | WIFI_ASOC_STATE))
				pstat = get_stainfo(priv, BSSID);
			else
#endif
			{
				if (pstat == NULL && !IS_MCAST(skb->data)) 
					pstat = get_stainfo(priv, skb->data);
			}

			if (pstat && pstat->QosEnabled)
				get_pri = 1;		
		}			

		if (!get_pri)
			skb->cb[0] = '\0';			
		
		if (tx_vlan_process(dev, &priv->pmib->vlan, skb, get_pri)) {
			priv->ext_stats.tx_drops++;
			DEBUG_ERR("TX DROP: by vlan!\n");
			goto free_and_stop;
		}		
	}
	else
		skb->cb[0] = '\0';
#endif

#ifdef WDS
	if (dev->base_addr) {
		// normal packets
		if (priv->pmib->dot11WdsInfo.wdsPure) {
			priv->ext_stats.tx_drops++;
			DEBUG_ERR("TX DROP: Sent normal pkt in Pure WDS mode!\n");
			goto free_and_stop;
		}
	}
	else {
		// WDS process
		if (rtl8190_tx_wdsDevProc(priv, skb, &dev, &wdsDev, txcfg) == TX_PROCEDURE_CTRL_STOP) {
			goto stop_proc;
		}
	}
#endif // WDS

#ifdef LOOPBACK_NORMAL_TX_MODE
#else
	if (priv->pmib->miscEntry.func_off)
		goto free_and_stop;
	// drop packets if link status is null
#ifdef WDS
	if (!wdsDev)
#endif
	{
		if (list_empty(&priv->asoc_list)) {
			priv->ext_stats.tx_drops++;
#ifdef CONFIG_RTK_VOIP
			// rock: error -> info
			DEBUG_INFO("TX DROP: non asoc tx request!\n");
#else
			//DEBUG_ERR("TX DROP: non asoc tx request!\n");
#endif
			goto free_and_stop;
		}
	}
#endif //LOOPBACK_NORMAL_TX_MODE

#ifdef CLIENT_MODE
	// nat2.5 translation, mac clone, dhcp broadcast flag add.
	if (((OPMODE & (WIFI_STATION_STATE | WIFI_ASOC_STATE)) == (WIFI_STATION_STATE | WIFI_ASOC_STATE))
			|| (OPMODE & WIFI_ADHOC_STATE)) {
#ifdef RTK_BR_EXT
		if (!priv->pmib->ethBrExtInfo.nat25_disable &&
				!(skb->data[0] & 1) &&
				priv->dev->br_port &&  memcmp(skb->data+MACADDRLEN, priv->br_mac, MACADDRLEN) &&
				*((unsigned short *)(skb->data+MACADDRLEN*2)) != __constant_htons(ETH_P_8021Q) &&
				*((unsigned short *)(skb->data+MACADDRLEN*2)) == __constant_htons(ETH_P_IP) &&
				!memcmp(priv->scdb_mac, skb->data+MACADDRLEN, MACADDRLEN) && priv->scdb_entry) {
			memcpy(skb->data+MACADDRLEN, GET_MY_HWADDR, MACADDRLEN);
			priv->scdb_entry->ageing_timer = jiffies;
		}
		else
#endif

		if (rtl8190_tx_clientMode(priv, &skb) == TX_PROCEDURE_CTRL_STOP) {
			goto stop_proc;
		}
	}
#endif // CLIENT_MODE

#ifdef MBSSID
#if defined(RTL8192SE) || defined(RTL8192SU)
	if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
#endif
	{
		if ((OPMODE & WIFI_AP_STATE) && !wdsDev) {
			if ((*(unsigned int *)&(skb->cb[8]) == 0x86518190) &&						// come from wlan interface
				(*(unsigned int *)&(skb->cb[12]) != priv->pmib->miscEntry.groupID))		// check group ID
			{
				priv->ext_stats.tx_drops++;
				DEBUG_ERR("TX DROP: not the same group!\n");
				goto free_and_stop;
			}
		}
	}
#endif

#ifdef ENABLE_RTL_SKB_STATS
	rtl_atomic_inc(&priv->rtl_tx_skb_cnt);
#endif

#ifdef WDS
	if (wdsDev)
		da = priv->pmib->dot11WdsInfo.entry[txcfg->wdsIdx].macAddr;
	else
#endif
#ifdef CONFIG_RTK_MESH
	if(dev == priv->mesh_dev)
		da = txcfg->nhop_11s;
	else
#endif
	da = skb->data;

#ifdef CLIENT_MODE
	if ((OPMODE & (WIFI_STATION_STATE | WIFI_ASOC_STATE)) == (WIFI_STATION_STATE | WIFI_ASOC_STATE))
		pstat = get_stainfo(priv, BSSID);
	else
#endif
	{
	pstat = get_stainfo(priv, da);
#ifdef LOOPBACK_NORMAL_TX_MODE
	if(pstat==NULL)
	{
		pstat=alloc_stainfo(priv,da,-1);
		pstat->state = WIFI_ASOC_STATE;
		pstat->protection = priv->pmib->dot11ErpInfo.protection;
	}
#endif
#ifdef A4_TUNNEL
	if ((OPMODE & WIFI_AP_STATE) && (priv->pmib->miscEntry.a4tnl_enable))
		pstat = A4_tunnel_lookup(priv, da);
#endif
	}

	if (UseSwCrypto(priv, pstat, (pstat ? FALSE : TRUE)))
	{
		newskb = NULL;
		if (skb_cloned(skb))
		{
			newskb = skb_copy(skb, GFP_ATOMIC);
#ifdef DEBUG_MEMORY_LEAK
		printk("%s %d freeskb=%x\n",__FUNCTION__,__LINE__,(unsigned int)skb);
#endif
			rtl_kfree_skb(priv, skb, _SKB_TX_);
			if (newskb == NULL) {
				priv->ext_stats.tx_drops++;
				DEBUG_ERR("TX DROP: Can't copy the skb!\n");
				return 0;
			}
#ifdef RTL8190_FASTEXTDEV_FUNCALL
			rtl865x_extDev_pktFromProtocolStack(skb);
#endif
			skb = newskb;

#ifdef ENABLE_RTL_SKB_STATS
			rtl_atomic_inc(&priv->rtl_tx_skb_cnt);
#endif
		}
	}

#ifdef BUFFER_TX_AMPDU
	if ((tx_flag >= TX_AMPDU_BUFFER_SIG) && (tx_flag <= TX_AMPDU_BUFFER_LAST)) {
		if (tx_flag == TX_AMPDU_BUFFER_FIRST)
			txcfg->aggre_en = FG_AGGRE_MPDU_BUFFER_FIRST;
		else if (tx_flag == TX_AMPDU_BUFFER_MID)
			txcfg->aggre_en = FG_AGGRE_MPDU_BUFFER_MID;
		else  if (tx_flag == TX_AMPDU_BUFFER_LAST)
			txcfg->aggre_en = FG_AGGRE_MPDU_BUFFER_LAST;
		goto skip_aggre_que;
	}
#endif

#ifdef SUPPORT_SNMP_MIB
	if (IS_MCAST(skb->data))
		SNMP_MIB_INC(dot11MulticastTransmittedFrameCount, 1);
#endif

	if ((OPMODE & WIFI_AP_STATE)
#ifdef WDS
			&& (!wdsDev) 
#endif	
#ifdef CONFIG_RTK_MESH
			&& (txcfg->is_11s == 0) 
#endif
		) {
		if ((pstat && (pstat->state & (WIFI_SLEEP_STATE | WIFI_ASOC_STATE)) ==
				(WIFI_SLEEP_STATE | WIFI_ASOC_STATE))
#ifdef SW_MCAST
				|| (IS_MCAST(skb->data) && (priv->sleep_list.next != &priv->sleep_list) && (!priv->release_mcast)
#if (defined(RTL8192SE)||defined(RTL8192SU)) && defined(MBSSID)
				&& !GET_ROOT(priv)->pmib->miscEntry.vap_enable
#endif
				)
#endif
			) {

#ifdef _11s_TEST_MODE_
			mesh_debug_tx3(dev, priv, skb);
#endif

			if (dz_queue(priv, pstat, skb) == TRUE) {
				DEBUG_INFO("queue up skb due to sleep mode\n");
				goto stop_proc;
			}
			else {
				if (pstat) {
					DEBUG_WARN("ucst sleep queue full!!\n");
				}
				else {
					DEBUG_WARN("mcst sleep queue full!!\n");
				}
				goto free_and_stop;
			}
		}
	}

#ifdef GBWC
	if (priv->pmib->gbwcEntry.GBWCMode && pstat) {
		if (rtl8190_tx_gbwc(priv, pstat, skb) == TX_PROCEDURE_CTRL_STOP) {
			goto stop_proc;
		}
	}
#endif

#if defined(CONFIG_RTK_MESH) && !defined(MESH_AMSDU)
	if(dev == priv->mesh_dev)
		goto just_skip;
#endif
#ifdef SUPPORT_TX_AMSDU
	if (pstat && (pstat->aggre_mthd == AGGRE_MTHD_MSDU) && (pstat->amsdu_level > 0)
#ifdef SUPPORT_TX_MCAST2UNI
		&& (priv->pshare->rf_ft_var.mc2u_disable || (skb->cb[2] != (char)0xff))
#endif
		) {
		int ret = amsdu_check(priv, skb, pstat, txcfg);

		if (ret == RET_AGGRE_ENQUE)
			goto stop_proc;

		if (ret == RET_AGGRE_DESC_FULL)
			goto free_and_stop;
	}
#endif
#ifdef MESH_AMSDU
	if(dev == priv->mesh_dev)
		goto just_skip;
#endif

#ifdef BUFFER_TX_AMPDU
	if (pstat && (pstat->aggre_mthd == AGGRE_MTHD_MPDU)
#ifdef SUPPORT_TX_MCAST2UNI
		&& (priv->pshare->rf_ft_var.mc2u_disable || (skb->cb[2] != (char)0xff))
#endif
		&& (AMPDU_ENABLE != 3)  // not buffered AMPDU
		) {
		int ret = ampdu_check(priv, skb, pstat);

		if (ret == RET_AGGRE_ENQUE)
			goto stop_proc;

		if (ret == RET_AGGRE_DESC_FULL)
			goto free_and_stop;
	}

skip_aggre_que:
#endif

#ifdef TX_SHORTCUT
#ifdef RTL8192SU
	if (!priv->pmib->dot11OperationEntry.disable_txsc && pstat)
	{
		if ((skb->len-sizeof(struct wlan_ethhdr_t))>=1400)
		{
			if (pstat->txcfg.fr_len > 0)
				stat_fr_len=pstat->txcfg.fr_len;
		}
		else
		{
			if (pstat->txcfg_short.fr_len > 0)
				stat_fr_len=pstat->txcfg_short.fr_len;
		}
	}
#ifdef RTL8192SU_TXSC_DBG
	skb->cb[8]=0;
#endif
#endif //RTL8192SU
	if (!priv->pmib->dot11OperationEntry.disable_txsc && pstat 
#if !defined(RTL8192SU)
		&& (pstat->txcfg.fr_len > 0)
#else
		&& (stat_fr_len >0 )
#endif
		&& !memcmp(skb->data, &pstat->ethhdr, sizeof(struct wlan_ethhdr_t)) &&
		(get_skb_priority(priv, skb) == pstat->pktpri) &&
		(FRAGTHRSLD > 1500))
	{
//			unsigned long			flags=0;
#if !defined(RTL8192SU)
			int						*tx_head, *tx_tail, q_num;
			struct rtl8190_hw		*phw = GET_HW(priv);
#else
			unsigned short			headerlen, fr_len;
			//unsigned char			tx_rate;
#endif

#ifdef BUFFER_TX_AMPDU
			int aggre_back;
#endif

#if !defined(RTL8192SU)
			q_num = pstat->txcfg.q_num;
			tx_head = get_txhead_addr(phw, q_num);
			tx_tail = get_txtail_addr(phw, q_num);

			// Check if we need to reclaim TX-ring before processing TX
			if (CIRC_SPACE_RTK(*tx_head, *tx_tail, NUM_TX_DESC) < 10) {
				rtl8190_tx_queueDsr(priv, q_num);
			}

//			if ((2 + 2) > CIRC_SPACE_RTK(*tx_head, *tx_tail, NUM_TX_DESC)) //per mpdu, we need 2 desc...
			if ((4) > CIRC_SPACE_RTK(*tx_head, *tx_tail, NUM_TX_DESC)) //per mpdu, we need 2 desc...
#else
			//if (get_TxUrb_Pending_num(priv) > (NUM_TX_DESC-(NUM_TX_DESC>>5)))
			if (get_TxUrb_Pending_num(priv) > (NUM_TX_DESC-10))
#endif	//!RTL8192SU			
			{
				// 2 is for spare...
#if !defined(RTL8192SU)				
				DEBUG_ERR("%d hw Queue desc not available! head=%d, tail=%d request %d\n",q_num,*tx_head,*tx_tail,2);
#else
				DEBUG_INFO("Tx shortcut CONGESTED!\n");
#endif				
				rtl8190_tx_xmitSkbFail(priv, skb, dev, wdsDev, txcfg);
				goto stop_proc;
			}

#ifdef BUFFER_TX_AMPDU
			if ((txcfg->aggre_en >= FG_AGGRE_MPDU_BUFFER_FIRST) &&
					(txcfg->aggre_en <= FG_AGGRE_MPDU_BUFFER_LAST))
				aggre_back = txcfg->aggre_en;
			else
				aggre_back = 0;
#endif

#ifdef RTL8192SU
			if ((skb->len-sizeof(struct wlan_ethhdr_t))<1400)
				memcpy(txcfg, &pstat->txcfg_short, sizeof(struct tx_insn));
			else
#endif
			memcpy(txcfg, &pstat->txcfg, sizeof(struct tx_insn));
#ifdef RTL8192SU
			if ((pstat->aggre_mthd == AGGRE_MTHD_MPDU) && (txcfg->aggre_en == 0)  && is_MCS_rate(get_tx_rate(priv, pstat)))
			{
				goto just_skip;
			}
#endif

#ifdef BUFFER_TX_AMPDU
			if (aggre_back)
				txcfg->aggre_en = aggre_back;
#endif

			txcfg->phdr = (UINT8 *)get_wlanllchdr_from_poll(priv);
			if (txcfg->phdr == NULL) {
				DEBUG_ERR("Can't alloc wlan header!\n");
				rtl8190_tx_xmitSkbFail(priv, skb, dev, wdsDev, txcfg);
				goto stop_proc;
			}
			memcpy(txcfg->phdr, (const void *)&pstat->wlanhdr, sizeof(struct wlanllc_hdr));
			txcfg->pframe = skb;
			txcfg->fr_len = skb->len - WLAN_ETHHDR_LEN;
			skb_pull(skb, WLAN_ETHHDR_LEN);

			if (txcfg->privacy == _TKIP_PRIVACY_) {
				if (rtl8190_tx_tkip(priv, skb, pstat, txcfg) == TX_PROCEDURE_CTRL_STOP) {
					goto stop_proc;
				}
			}

			txcfg->tx_rate = get_tx_rate(priv, pstat);
			txcfg->lowest_tx_rate = get_lowest_tx_rate(priv, pstat, txcfg->tx_rate);

			// log tx statistics...
			tx_sum_up(priv, pstat, txcfg->fr_len+txcfg->hdr_len+txcfg->iv+txcfg->llc+txcfg->mic+txcfg->icv);
			SNMP_MIB_INC(dot11TransmittedFragmentCount, 1);

			// for SW LED
			priv->pshare->LED_tx_cnt++;
			if ((txcfg->aggre_en >= FG_AGGRE_MPDU) && (txcfg->aggre_en <= FG_AGGRE_MPDU_BUFFER_LAST)) {
				if (!pstat->ADDBA_ready)
					issue_ADDBAreq(priv, pstat, 0);
			}

#ifdef LOOPBACK_MODE
	txcfg->tx_rate=pstat->txcfg.tx_rate;
#endif

			// check if we could re-use tx descriptor
#if	defined(RTL8190) || defined(RTL8192E)
			if ((get_desc(pstat->hwdesc1.flen) > 0) && (skb->len == get_desc(pstat->hwdesc2.flen)) &&
#elif defined(RTL8192SE)
			if ((get_desc(pstat->hwdesc1.Dword7)&TX_TxBufferSizeMask) > 0 &&
				((
#if defined(CONFIG_RTL_WAPI_SUPPORT)
				 (txcfg->privacy==_WAPI_SMS4_) ? ((skb->len+SMS4_MIC_LEN)==(get_desc(pstat->hwdesc2.Dword7)&TX_TxBufferSizeMask)) :
#endif				
				(skb->len == (get_desc(pstat->hwdesc2.Dword7)&TX_TxBufferSizeMask)))
				) &&
#elif defined(RTL8192SU)
			if (txcfg->fr_len>=1400)
			{
				headerlen=(get_desc(pstat->hwdesc1.Dword7)&TX_TxBufferSizeMask);
				fr_len=(get_desc(pstat->hwdesc2.Dword7)&TX_TxBufferSizeMask);
				//tx_rate=pstat->txcfg.tx_rate;
			}
			else
			{
				headerlen=(get_desc(pstat->hwdesc3.Dword7)&TX_TxBufferSizeMask);
				fr_len=(get_desc(pstat->hwdesc4.Dword7)&TX_TxBufferSizeMask);
				//tx_rate=pstat->txcfg_short.tx_rate;
			}
#ifdef RTL8192SU_TXSC_DBG_MSG
			printk("hwdesc1.Dword7=%x, hwdesc2.Dword7=%x,", (get_desc(pstat->hwdesc1.Dword7)&TX_TxBufferSizeMask), (get_desc(pstat->hwdesc2.Dword7)&TX_TxBufferSizeMask));
			printk("hwdesc3.Dword7=%x, hwdesc4.Dword7=%x,\n", (get_desc(pstat->hwdesc3.Dword7)&TX_TxBufferSizeMask), (get_desc(pstat->hwdesc4.Dword7)&TX_TxBufferSizeMask));
#endif
			if (headerlen > 0 && (skb->len == fr_len) &&
#endif
#if !defined(LOOPBACK_NORMAL_TX_MODE)
#if 1//!defined(RTL8192SU)
				(txcfg->tx_rate == pstat->txcfg.tx_rate) &&
#else
				(txcfg->tx_rate == tx_rate) &&
#endif
#endif //!defined(LOOPBACK_NORMAL_TX_MODE)
				(pstat->protection == priv->pmib->dot11ErpInfo.protection)
#if defined(SEMI_QOS) && defined(WMM_APSD)
				&& (!((APSD_ENABLE) && (pstat->state & WIFI_SLEEP_STATE)))
#endif
				) {

				if (txcfg->privacy) {
					switch (txcfg->privacy) {
					case _WEP_104_PRIVACY_:
					case _WEP_40_PRIVACY_:
						wep_fill_iv(priv, txcfg->phdr, txcfg->hdr_len, pstat->sc_keyid);
						break;

					case _TKIP_PRIVACY_:
						tkip_fill_encheader(priv, txcfg->phdr, txcfg->hdr_len, pstat->sc_keyid);
						break;

					case _CCMP_PRIVACY_:
						aes_fill_encheader(priv, txcfg->phdr, txcfg->hdr_len, pstat->sc_keyid);
						break;
					}
				}

//				if (txcfg->privacy != _TKIP_PRIVACY_)
//					SAVE_INT_AND_CLI(flags);
#if	defined(RTL8190) || defined(RTL8192E)
				signin_txdesc_shortcut(priv, txcfg);
#elif defined(RTL8192SE)

				if (no_ddr_patch)
				{
					if ((skb_headroom(skb) >= (txcfg->hdr_len + txcfg->llc + txcfg->iv)) &&
						!skb_cloned(skb) &&
						(txcfg->privacy != _TKIP_PRIVACY_)
#if defined(CONFIG_RTL_WAPI_SUPPORT)
						&& (txcfg->privacy != _WAPI_SMS4_)
#endif
						)
					{
						memcpy((skb->data - (txcfg->hdr_len + txcfg->llc + txcfg->iv)), txcfg->phdr, (txcfg->hdr_len + txcfg->llc + txcfg->iv));
						release_wlanllchdr_to_poll(priv, txcfg->phdr);
						txcfg->phdr = skb->data - (txcfg->hdr_len + txcfg->llc + txcfg->iv);
						txcfg->one_txdesc = 1;
					}

				}

				rtl8192SE_signin_txdesc_shortcut(priv, txcfg);
#elif defined(RTL8192SU)
				rtl8192SE_signin_txdesc_shortcut(priv, txcfg);
#endif

//				RESTORE_INT(flags);
				goto stop_proc;
			}

//			if (txcfg->privacy != _TKIP_PRIVACY_)
//				SAVE_INT_AND_CLI(flags);

#if	defined(RTL8190) || defined(RTL8192E)
			signin_txdesc(priv, txcfg);
#elif defined(RTL8192SE) || defined(RTL8192SU)
#ifdef RTL8192SU_TXSC_DBG
			{
#ifdef RTL8192SU_TXSC_DBG_MSG
				if (headerlen <= 0)
					printk("(pstat->hwdesc1.Dword7)<=0\n");
				if ((skb->len != fr_len))
				{
					printk("skb->len != (pstat->hwdesc2.Dword7), skb->len=%x, fr_len=%x\n" ,skb->len, fr_len);
					if (txcfg->fr_len>=1400)
						printk("(hwdesc2.Dword7=%x)\n", pstat->hwdesc2.Dword7);
					else
						printk("(hwdesc4.Dword7=%x)\n", pstat->hwdesc4.Dword7);
					//memDump(skb->data, skb->len, "skb->len error pkt");
				}
				if (txcfg->tx_rate != pstat->txcfg.tx_rate)
					printk("(txcfg->tx_rate != pstat->txcfg.tx_rate)\n");
				if ((pstat->protection != priv->pmib->dot11ErpInfo.protection))
					printk("(pstat->protection != priv->pmib->dot11ErpInfo.protection)\n");
#endif
				skb->cb[8]=1;
				priv->ext_stats.nonTxSC_pkt_cnt++;
			}
#endif
			rtl8192SE_signin_txdesc(priv, txcfg);
#endif

			pstat->txcfg.tx_rate = txcfg->tx_rate;
//			RESTORE_INT(flags);
			goto stop_proc;
	}
#ifdef RTL8192SU_TXSC_DBG
	else
	{
		if (!priv->pmib->dot11OperationEntry.disable_txsc && pstat && (pstat->txcfg.fr_len > 0) &&
		!memcmp(skb->data, &pstat->ethhdr, sizeof(struct wlan_ethhdr_t)) &&
		(get_skb_priority(priv, skb) == pstat->pktpri) &&
		(FRAGTHRSLD > 1500))

#ifdef RTL8192SU_TXSC_DBG_MSG
		if (pstat!=NULL)
		{
			if (pstat->txcfg.fr_len <= 0)
				printk("(pstat->txcfg.fr_len <= 0)\n");
			if (memcmp(skb->data, &pstat->ethhdr, sizeof(struct wlan_ethhdr_t)))
				printk("skb->data != pstat->ether\n");
			if ((get_skb_priority(priv, skb) != pstat->pktpri))
				printk("priority error\n");
		}
#endif
		skb->cb[8]=1;
	}
#endif
#endif // TX_SHORTCUT
#if defined(CONFIG_RTK_MESH) || defined(RTL8192SU)
just_skip:
#endif

	/* ==================== Slow path of packet TX process ==================== */
#ifdef RTL867X_CP3
	//rtl8651_romeperfExitPoint(ROMEPERF_INDEX_STARTXMIT1 );
#endif
	if (rtl8190_tx_slowPath(priv, skb, pstat, dev, wdsDev, txcfg) == TX_PROCEDURE_CTRL_STOP) {
		goto stop_proc;
	}

#ifdef __KERNEL__
	dev->trans_start = jiffies;
#endif

	goto stop_proc;

free_and_stop:		/* Free current packet and stop TX process */

#ifdef RTL8190_FASTEXTDEV_FUNCALL
		rtl865x_extDev_kfree_skb(skb, FALSE);
#else
#ifdef DEBUG_MEMORY_LEAK
		printk("%s %d freeskb=%x\n",__FUNCTION__,__LINE__,(unsigned int)skb);
#endif
#if defined(PKT_PROCESSOR) || defined(PRE_ALLOCATE_SKB)
		rtl_kfree_skb(priv,skb,_SKB_TX_);
#else
		dev_kfree_skb_any(skb);
#endif

#endif

stop_proc:			/* Stop process and assume the TX-ed packet is already "processed" (freed or TXed) in previous code. */
#ifdef RTL867X_CP3
	rtl8651_romeperfExitPoint(ROMEPERF_INDEX_8187_TX_XMIT );
#endif

#ifdef LOOPBACK_MODE
	local_irq_enable();
#endif

	return 0;
}

#ifndef CONFIG_RTK_MESH
int	rtl8190_wlantx(struct rtl8190_priv *priv, struct tx_insn* txcfg)
{
	return (rtl8190_firetx(priv, txcfg));
}
#endif


int issue_tpt_tstpkt(struct rtl8190_priv *priv)
{
	DECLARE_TXINSN(txinsn);

	txinsn.q_num = BE_QUEUE;
	txinsn.tx_rate = 0x87;
	txinsn.lowest_tx_rate = txinsn.tx_rate;
	txinsn.fixed_rate = 1;
	txinsn.tpt_pkt = 1;
	txinsn.phdr = get_wlanhdr_from_poll(priv);

	if (txinsn.phdr == NULL)
		goto send_test_pkt_fail;

	memset((void *)(txinsn.phdr), 0, sizeof (struct	wlan_hdr));

	SetFrameSubType(txinsn.phdr, WIFI_DATA_NULL);

	if (OPMODE & WIFI_AP_STATE) {
		memcpy((void *)GetAddr1Ptr((txinsn.phdr)), "\xff\xff\xff\xff\xff\xff", MACADDRLEN);
		memcpy((void *)GetAddr2Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
		memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
	}
	else {
		memcpy((void *)GetAddr1Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
		memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
		memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
	}

	txinsn.hdr_len = WLAN_HDR_A3_LEN;

	if ((rtl8190_firetx(priv, &txinsn)) == SUCCESS)
		return TRUE;

send_test_pkt_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	return FALSE;
}


#ifdef TX_SHORTCUT
#if	defined(RTL8192SE) || defined(RTL8192SU)
#if !defined(RTL8192SU)
__MIPS16
__IRAM_FASTEXTDEV
#else
__IRAM_WIFI_PRI5
#endif
void rtl8192SE_signin_txdesc_shortcut(struct rtl8190_priv *priv, struct tx_insn *txcfg)
{
	
#if !defined(RTL8192SU)
//#ifdef __LINUX_2_6__
#if 0
	struct tx_desc		*phdesc, *pdesc, *pfrstdesc;
	struct tx_desc_info	*pswdescinfo, *pdescinfo;
	struct rtl8190_hw	*phw;
	int					*tx_head, q_num;
	struct stat_info 	*pstat = txcfg->pstat;
	struct sk_buff 		*pskb = (struct sk_buff *)txcfg->pframe;
//	unsigned char		*pfwinfo;
	unsigned int		pfrst_dma_desc=0;
	unsigned int		*dma_txhead;
#else
	__DRAM_IN_865X static struct tx_desc *phdesc, *pdesc, *pfrstdesc;
	__DRAM_IN_865X static struct tx_desc_info *pswdescinfo, *pdescinfo;
	__DRAM_IN_865X static struct rtl8190_hw	*phw;
	__DRAM_IN_865X static int *tx_head, q_num;
	__DRAM_IN_865X static struct stat_info *pstat;
	__DRAM_IN_865X static struct sk_buff *pskb;
	__DRAM_IN_865X static unsigned int pfrst_dma_desc;
	__DRAM_IN_865X static unsigned int *dma_txhead;
#endif

	pstat = txcfg->pstat;
	pskb = (struct sk_buff *)txcfg->pframe;
	pfrst_dma_desc=0;

	phw	= GET_HW(priv);
	q_num = txcfg->q_num;

	dma_txhead	= get_txdma_addr(phw, q_num);
	tx_head		= get_txhead_addr(phw, q_num);
	phdesc		= get_txdesc(phw, q_num);
	pswdescinfo	= get_txdesc_info(priv->pshare->pdesc_info, q_num);
#else //RTL8192SU
#if 0 //if enable this flag, must lock this function.
	__DRAM_IN_865X static struct tx_desc		*pdesc;
	__DRAM_IN_865X static struct tx_desc_info	*pdescinfo;
	__DRAM_IN_865X static struct rtl8190_hw		*phw;
	__DRAM_IN_865X static int			q_num;
	__DRAM_IN_865X static struct stat_info 		*pstat;
	__DRAM_IN_865X static struct sk_buff		*pskb;
	__DRAM_IN_865X static struct urb 		*tx_urb;
	__DRAM_IN_865X static unsigned int 				*tx;
	__DRAM_IN_865X static int 				status;	
	__DRAM_IN_865X static int					 urb_len;		
	__DRAM_IN_865X static unsigned char		en_pktoffset;
	__DRAM_IN_865X static priority_t			priority;
#else //tysu: sometimes, this function is interrupted by RX.... can't use global or static vars, this function maybe reentery.
	struct tx_desc		*pdesc;
	struct tx_desc_info	*pdescinfo;
	struct rtl8190_hw		*phw;
	int					q_num;
	struct stat_info 		*pstat;
	struct sk_buff		*pskb;
	struct urb 			*tx_urb;
	unsigned int 		*tx;
	int 					status;	
	unsigned int		urb_len;		
	unsigned char			en_pktoffset;
	priority_t				priority;
	unsigned char		 q_select=0;
#endif

	en_pktoffset=0;
	priority=BULK_PRIORITY;

	if (!IS_DRV_OPEN(priv))
	{
		printk("%s %d: free packet when intf down!\n",__FUNCTION__,__LINE__);
		if (txcfg->privacy==_TKIP_PRIVACY_)
		{
			((struct sk_buff *)txcfg->pframe)->tail-=8;
			((struct sk_buff *)txcfg->pframe)->len-=8;
		}
		goto signin_tx_shortcut_fail;
	}
	
	pdesc=NULL;
	pdescinfo=NULL;
	pstat = txcfg->pstat;
	pskb = (struct sk_buff *)txcfg->pframe;

	phw	= GET_HW(priv);
	q_num = txcfg->q_num;

	urb_len = txcfg->hdr_len+txcfg->llc+txcfg->iv+txcfg->fr_len+32;
	if (txcfg->privacy)
	{
		if (UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE)))
			urb_len += txcfg->mic+txcfg->icv;
	}
	//if (txcfg->fr_type == _SKB_FRAME_TYPE_)
	//	printk("hdr_len=%d,llc=%d,iv=%d,fr_len=%d,icv=%d,mic=%d\n", txcfg->hdr_len,txcfg->llc,txcfg->iv,txcfg->fr_len,txcfg->icv,txcfg->mic);
#if 0// RTL8672_USB_PATCH
	{
		int rest;
		rest = urb_len&0x3f;
		if((0 == (urb_len & 0x1ff))||((rest>=3) && (rest<=5)))
		{
			en_pktoffset=1;
			urb_len += 8;
		}
	}
#endif

#ifdef RTL8192SU_TX_ZEROCOPY
	if (!skb_cloned(pskb))
	{
		tx = (unsigned int *)(pskb->data - sizeof(struct tx_desc) - txcfg->llc - txcfg->iv - txcfg->hdr_len);
		// printk("skb->head=%x skb->data=%x skb->tail=%x skb->end=%x skb->len=%d\n",(unsigned int)pskb->head,(unsigned int)pskb->data,(unsigned int)pskb->tail,(unsigned int)pskb->end,(unsigned int)pskb->len);
#ifdef RTL867X_USBPATCH
		if (Check1kboundary(tx, urb_len) || check_align(tx))
		{
			//printk("over 1k boundary!\n");
			//pskb=NULL;
			goto ALLOCTXBUFF_SC;
		}
		else
#endif
		{
			en_pktoffset=Check_USBPatch(tx, urb_len, en_pktoffset);
			//if (en_pktoffset!=0)
			{
				if (skb_headroom(pskb) >= (txcfg->hdr_len + txcfg->llc + txcfg->iv + sizeof(struct tx_desc)+(en_pktoffset<<3)))
				{
					urb_len += (en_pktoffset<<3);
					tx = (unsigned int *)(((unsigned int)(tx))-(en_pktoffset<<3));
				}
				else
				{
					goto ALLOCTXBUFF_SC;
				}
			}
		}
	}
	else
#endif
	{
ALLOCTXBUFF_SC:
		en_pktoffset=0;
		tx=NULL;
		if (0==(urb_len & 0x1ff))
		{
			en_pktoffset++;
			urb_len+=(en_pktoffset<<3);
		}
		pskb=NULL;
		tx = kmalloc(urb_len, GFP_ATOMIC);
		if(!tx)
			goto signin_tx_shortcut_fail;
#ifdef RTL867X_USBPATCH
		if (USB_PATCH(&tx, &urb_len, &en_pktoffset)==FAIL)
			goto signin_tx_shortcut_fail;
#endif
	}
	memset(tx, 0, 32);

#ifdef RTL8192SU_TX_FEW_INT
#else

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
 	tx_urb = usb_alloc_urb(0,GFP_ATOMIC);
#else
 	tx_urb = usb_alloc_urb(0);
#endif

	if(!tx_urb){
#ifdef RTL8192SU_TX_ZEROCOPY
	if(txcfg->fr_type != _SKB_FRAME_TYPE_ || pskb==NULL)
	{
		kfree(tx);
	}
#else
	kfree(tx);
#endif //end of RTL8192SU_TX_ZEROCOPY
		goto signin_tx_shortcut_fail;
	}

#endif //end of RTL8192SU_TX_FEW_INT
#endif //!RTL8192SU

#ifdef RTL867X_CP3
//rtl8651_romeperfExitPoint(ROMEPERF_INDEX_TX_SHORTCUT1);
//rtl8651_romeperfEnterPoint(ROMEPERF_INDEX_TX_SHORTCUT2);
#endif

	/*------------------------------------------------------------*/
	/*           fill descriptor of header + iv + llc             */
	/*------------------------------------------------------------*/
#if !defined(RTL8192SU)
	pfrstdesc = pdesc = phdesc + *tx_head;
	pdescinfo = pswdescinfo + *tx_head;
	desc_copy(pdesc, &pstat->hwdesc1);
#else //RTL8192SU
	pdesc   	= (struct tx_desc*)tx;
	//memcpy(pdesc, &pstat->hwdesc1, sizeof(struct tx_desc));
	if (txcfg->fr_len>=1400)
		desc_copy(pdesc, &pstat->hwdesc1);
	else
		desc_copy(pdesc, &pstat->hwdesc3);
#ifdef RTL8192SU_TX_FEW_INT


#if 0 //debug only: show the different URB pointer to the same skb_buffer.
{
	int i;
	uint32 flags;
	struct sk_buff *skb;
	skb=txcfg->pframe;

	SAVE_INT_AND_CLI(flags);
	for(i=0;i<MAX_TX_URB;i++)
	{
		if(priv->pshare->tx_pool.own[i]!=0)
		{
			struct urb *purb= priv->pshare->tx_pool.urb[i];
			if((unsigned int)purb->transfer_buffer==(unsigned int)tx)
			{
				struct sk_buff *skb=priv->pshare->tx_pool.pdescinfo[i]->pframe;
				int j;
				printk("Error... oldurb=%x new->data=%x i=%d newskb=%x newhead=%x newdata=%x old_desc_skb=%x olddata=%x\n",(unsigned int)purb,(unsigned int)tx,i,(unsigned int)pskb,(unsigned int)pskb->head,(unsigned int)pskb->data,(unsigned int)skb,(unsigned int)skb->data);
				for(j=0;j<MAX_TX_URB;j++)
				{
					struct sk_buff *skb2=priv->pshare->tx_pool.pdescinfo[j]->pframe;
					printk("[%02d] own=%d skb=%x\n",j,priv->pshare->tx_pool.own[j],(unsigned int)skb2);
				}
				memDump(tx,128,"tx");
//				memDump(purb->transfer_buffer,128,"old");
				while(1);
				break;
			}

		}
	}
	RESTORE_INT(flags);
}
#endif

	if(get_tx_urb_from_pool(priv,&tx_urb,&pdescinfo) != SUCCESS)
	{
#ifdef RTL8192SU_TX_ZEROCOPY
		if((txcfg->fr_type != _SKB_FRAME_TYPE_) || (pskb==NULL))
		{
			kfree(tx);
		}
#else
		kfree(tx);
#endif
		goto signin_tx_shortcut_fail;
	}
	//msg_queue("get_urb urb=%x skb=%x\n",(u32)tx_urb,(u32)pskb);


#else
	pdescinfo = kmalloc(sizeof(struct tx_desc_info), GFP_ATOMIC);
	if(!pdescinfo) goto signin_tx_shortcut_fail;//return -ENOMEM;
	memset(pdescinfo, 0, sizeof(struct tx_desc_info));
#endif
#ifdef RTL867X_CP3
//rtl8651_romeperfExitPoint(ROMEPERF_INDEX_TX_SHORTCUT2);
#endif
#ifdef RTL8192SU_TX_ZEROCOPY
	if(pskb==NULL)
		pdescinfo->non_zcpy=1;
	else
		pdescinfo->non_zcpy=0;
#endif	
#endif //!RTL8192SU

	assign_wlanseq(GET_HW(priv), txcfg->phdr, pstat, GET_MIB(priv)
#ifdef CONFIG_RTK_MESH	// For broadcast data frame via mesh (ex:ARP requst)
	, txcfg->is_11s
#endif
		);

#ifdef RTL8192SU
#ifdef SEMI_QOS
	if ((txcfg->hdr_len==WLAN_HDR_A3_QOS_LEN || txcfg->hdr_len==WLAN_HDR_A4_QOS_LEN))
#ifndef DISABLE_UNALIGN_TRAP
		pdesc->Dword1 &= set_desc(~TX_NonQos);
#else
		AND_EQUAL(&pdesc->Dword1, set_desc(~TX_NonQos));
#endif
	else
#endif
#ifndef DISABLE_UNALIGN_TRAP
	pdesc->Dword1 |= set_desc(TX_NonQos);
#else
	OR_EQUAL(&pdesc->Dword1, set_desc(TX_NonQos));
#endif

#endif

#ifndef DISABLE_UNALIGN_TRAP
	pdesc->Dword3 = 0;
	pdesc->Dword3 = set_desc((GetSequence(txcfg->phdr) & TX_SeqMask) << TX_SeqSHIFT);
#else
//	SET_EQUAL(&pdesc->Dword3,0);
	SET_EQUAL(&pdesc->Dword3 , set_desc((GetSequence(txcfg->phdr) & TX_SeqMask) << TX_SeqSHIFT));
#endif

	//set Break
	if (txcfg->pstat != priv->pshare->CurPstat) {
#ifndef DISABLE_UNALIGN_TRAP
		pdesc->Dword2 |= set_desc(TX_BK);
#else
		OR_EQUAL(&pdesc->Dword2 , set_desc(TX_BK));
#endif
		priv->pshare->CurPstat = txcfg->pstat;
	}
	else
#ifndef DISABLE_UNALIGN_TRAP
		pdesc->Dword2 &= set_desc(~TX_BK); // clear it
#else
		AND_EQUAL(&pdesc->Dword2 , set_desc(~TX_BK)); // clear it
#endif

#if !defined(RTL8192SU)
	if (txcfg->one_txdesc) {
		pdesc->Dword0 = set_desc((get_desc(pdesc->Dword0) & 0xffff0000) |
			TX_LastSeg | (txcfg->hdr_len + txcfg->llc + txcfg->iv + txcfg->fr_len));
		pdesc->Dword7 = set_desc((get_desc(pdesc->Dword7) & 0xffff0000) |
			(txcfg->hdr_len + txcfg->llc + txcfg->iv + txcfg->fr_len));
	}

	pdesc->Dword8 = set_desc(get_physical_addr(priv, txcfg->phdr,
		(get_desc(pdesc->Dword7)& TX_TxBufferSizeMask), PCI_DMA_TODEVICE));
#else //RTL8192SU
#ifndef DISABLE_UNALIGN_TRAP
	pdesc->Dword0 = set_desc((get_desc(pdesc->Dword0) & 0xffff0000) |
		TX_LastSeg | (txcfg->hdr_len + txcfg->llc + txcfg->iv + txcfg->fr_len));
#else		
	SET_EQUAL(&pdesc->Dword0, set_desc((get_desc(pdesc->Dword0) & 0xffff0000) |
		TX_LastSeg | (txcfg->hdr_len + txcfg->llc + txcfg->iv + txcfg->fr_len)));
#endif
#endif //!RTL8192SU

#ifdef RTL8192SU
	if (txcfg->fr_len<1400)
		descinfo_copy(pdescinfo, &pstat->swdesc3);
	else
#endif
	descinfo_copy(pdescinfo, &pstat->swdesc1);

#if !defined(RTL8192SU)
	pdescinfo->paddr  = get_desc(pdesc->Dword8); // buffer addr
	if (txcfg->one_txdesc) {
		pdescinfo->type = _SKB_FRAME_TYPE_;
		pdescinfo->pframe = pskb;
		pdescinfo->priv = priv;
#if defined(SEMI_QOS) && defined(WMM_APSD)
		pdescinfo->pstat = txcfg->pstat;
#endif
	}
	else {
		pdescinfo->pframe = txcfg->phdr;
#if defined(SEMI_QOS) && defined(WMM_APSD)
		pdescinfo->priv = priv;
		pdescinfo->pstat = txcfg->pstat;
#endif
	}
#else //RTL8192SU
	pdescinfo->phdr = txcfg->phdr;
	pdescinfo->priv = priv;
#if defined(SEMI_QOS) && defined(WMM_APSD)
	pdescinfo->pstat = txcfg->pstat;
#endif
#endif //!RTL8192SU

#if !defined(RTL8192SU)
	pfrst_dma_desc = dma_txhead[*tx_head];
#endif	

#if !defined(RTL8192SU)
/*
#ifdef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, get_desc(pdesc->Dword8), (get_desc(pdesc->Dword7)&TX_TxBufferSizeMask), PCI_DMA_TODEVICE);
#endif
*/
	txdesc_rollover(pdesc, (unsigned int *)tx_head);

	if (txcfg->one_txdesc)
		goto one_txdesc;
#else //RTL8192SU
	if (en_pktoffset)
	{
#ifndef DISABLE_UNALIGN_TRAP
		pdesc->Dword1 |= set_desc((en_pktoffset&TX_PktOffsetMask)<<TX_PktOffsetSHIFT);
#else
		OR_EQUAL(&pdesc->Dword1 , set_desc((en_pktoffset&TX_PktOffsetMask)<<TX_PktOffsetSHIFT));
#endif
	}
	memcpy(tx+8+(en_pktoffset<<1), txcfg->phdr, txcfg->hdr_len+txcfg->iv+txcfg->llc);
#endif //!RTL8192SU

	/*------------------------------------------------------------*/
	/*              fill descriptor of frame body                 */
	/*------------------------------------------------------------*/
#if !defined(RTL8192SU)
	pdesc = phdesc + *tx_head;
	pdescinfo = pswdescinfo + *tx_head;
	desc_copy(pdesc, &pstat->hwdesc2);

	pdesc->Dword8 = set_desc(get_physical_addr(priv, pskb->data,
		(get_desc(pdesc->Dword7)&0x0fff), PCI_DMA_TODEVICE));

	descinfo_copy(pdescinfo, &pstat->swdesc2);
	pdescinfo->paddr  = get_desc(pdesc->Dword8);
	pdescinfo->pframe = pskb;
	pdescinfo->priv = priv;
/*
#ifndef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, dma_txhead[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#else
	rtl_cache_sync_wback(priv, get_desc(pdesc->Dword8), (get_desc(pdesc->Dword7)&TX_TxBufferSizeMask), PCI_DMA_TODEVICE);
#endif
*/
	txdesc_rollover(pdesc, (unsigned int *)tx_head);
#else //RTL8192SU
	pdescinfo->pframe = txcfg->pframe;
	pdescinfo->priv = priv;

#ifdef RTL8192SU_TX_ZEROCOPY
	if(pdescinfo->non_zcpy)
		#ifdef CONFIG_RTL8671
		gdma_memcpy((char*)(tx+8+(en_pktoffset<<1))+(txcfg->hdr_len+txcfg->llc+txcfg->iv), ((struct sk_buff *)txcfg->pframe)->data, txcfg->fr_len);
		#else
		memcpy((char*)(tx+8+(en_pktoffset<<1))+(txcfg->hdr_len+txcfg->llc+txcfg->iv), ((struct sk_buff *)txcfg->pframe)->data, txcfg->fr_len);
		#endif
#else
	memcpy((char*)(tx+8+(en_pktoffset<<1))+(txcfg->hdr_len+txcfg->llc+txcfg->iv), ((struct sk_buff *)txcfg->pframe)->data, txcfg->fr_len);
#endif
#endif //!RTL8192SU

#ifndef CONFIG_RTL8671
#if defined(CONFIG_RTL_WAPI_SUPPORT)
	if (txcfg->privacy == _WAPI_SMS4_)
	{
		SecSWSMS4Encryption(priv, txcfg);
	} else
#endif
	if ((txcfg->privacy == _TKIP_PRIVACY_) &&
		(priv->pshare->have_hw_mic) &&
		!(priv->pmib->dot11StationConfigEntry.swTkipMic))
	{
		register unsigned long int l,r;
		unsigned char *mic;
		int delay = 18;

		while ((*(volatile unsigned int *)GDMAISR & GDMA_COMPIP) == 0) {
			delay_us(delay);
			delay = delay / 2;
		}

		l = *(volatile unsigned int *)GDMAICVL;
		r = *(volatile unsigned int *)GDMAICVR;

		mic = ((struct sk_buff *)txcfg->pframe)->data + txcfg->fr_len - 8;
		mic[0] = (unsigned char)(l & 0xff);
		mic[1] = (unsigned char)((l >> 8) & 0xff);
		mic[2] = (unsigned char)((l >> 16) & 0xff);
		mic[3] = (unsigned char)((l >> 24) & 0xff);
		mic[4] = (unsigned char)(r & 0xff);
		mic[5] = (unsigned char)((r >> 8) & 0xff);
		mic[6] = (unsigned char)((r >> 16) & 0xff);
		mic[7] = (unsigned char)((r >> 24) & 0xff);

#ifdef MICERR_TEST
		if (priv->micerr_flag) {
			mic[7] ^= mic[7];
			priv->micerr_flag = 0;
		}
#endif
	}

#ifndef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, dma_txhead[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#else
	rtl_cache_sync_wback(priv, get_desc(pdesc->Dword8), (get_desc(pdesc->Dword7)&TX_TxBufferSizeMask), PCI_DMA_TODEVICE);
#endif
#endif

#if !defined(RTL8192SU)
one_txdesc:

#ifdef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, get_desc(pfrstdesc->Dword8), (get_desc(pfrstdesc->Dword7)&TX_TxBufferSizeMask), PCI_DMA_TODEVICE);
#endif
#endif

#ifdef SUPPORT_SNMP_MIB
	if (txcfg->rts_thrshld <= get_mpdu_len(txcfg, txcfg->fr_len))
		SNMP_MIB_INC(dot11RTSSuccessCount, 1);
#endif

#ifdef BUFFER_TX_AMPDU
	if ((txcfg->aggre_en >= FG_AGGRE_MPDU_BUFFER_FIRST) &&
					(txcfg->aggre_en <= FG_AGGRE_MPDU_BUFFER_LAST)) {
		if (txcfg->aggre_en == FG_AGGRE_MPDU_BUFFER_FIRST) {
			priv->ampdu_first_desc = pfrstdesc;
#ifndef USE_RTL8186_SDK
			priv->ampdu_first_dma_desc = pfrst_dma_desc;
#endif
		}
		else {
			pfrstdesc->Dword0 |= set_desc(TX_OWN);

#ifndef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, pfrst_dma_desc, sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
			if (txcfg->aggre_en == FG_AGGRE_MPDU_BUFFER_LAST) {
				pfrstdesc = priv->ampdu_first_desc;
				pfrstdesc->Dword0 |= set_desc(TX_OWN);
#ifndef USE_RTL8186_SDK
				rtl_cache_sync_wback(priv, priv->ampdu_first_dma_desc, sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
				tx_poll(priv, q_num);
			}
		}
		return;
	}
#endif // BUFFER_TX_AMPDU

#if !defined(RTL8192SU)
	pfrstdesc->Dword0 |= set_desc(TX_OWN);

#ifndef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, pfrst_dma_desc, sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif

	if (q_num == HIGH_QUEUE) {
		DEBUG_WARN("signin shortcut for DTIM pkt?\n");
		return;
	}
	else
		tx_poll(priv, q_num);
#else //RTL8192SU
	pdescinfo->last_seg=1;
#ifdef RTL8192SU_TX_ZEROCOPY
	if ((pskb==NULL )||(pdescinfo->non_zcpy))
#endif
		pdescinfo->purb= tx;

	q_select=((get_desc(pdesc->Dword1)>>TX_QueueSelSHIFT)&TX_QueueSelMask);
	if (q_select==0||q_select==3) //BE
		q_num=BE_QUEUE;
	else if (q_select==1||q_select==2)//BK
		q_num=BK_QUEUE;
	else if (q_select==5||q_select==4)//VI
		q_num=VI_QUEUE;
	else if (q_select==6||q_select==7)//VO
		q_num=VO_QUEUE;
#if 0
#ifndef DISABLE_UNALIGN_TRAP
	pdesc->Dword1 |= set_desc((q_select & TX_QueueSelMask)<< TX_QueueSelSHIFT);
#else
	OR_EQUAL(&pdesc->Dword1 , set_desc((q_select & TX_QueueSelMask)<< TX_QueueSelSHIFT));
#endif
#endif
	priority=get_endpoint(priv, q_num);
	pdescinfo->q_num = q_num;
	{
		//printk(KERN_WARNING "Tx packet use by submit urb!\n");
		/* FIXME check what EP is for low/norm PRI */
		usb_fill_bulk_urb(tx_urb,priv->udev,
				usb_sndbulkpipe(priv->udev,priority), tx,
				urb_len, (usb_complete_t)rtl8192su_tx_isr, (void *)pdescinfo);

		//if (txcfg->fr_type == _SKB_FRAME_TYPE_)
		{
			//memDump(((struct sk_buff *)(pdescinfo->pframe))->data, 32, "pdescinfo->pframe");
			//printk("seq_num=%d\n", GetSequence(tx+8));
		}
	}

	//memDump(tx,32,"data");
	dma_cache_wback_inv((unsigned long)tx,urb_len);
	//rtl_cache_sync_wback(priv, (unsigned int)tx, urb_len, 0);
#ifdef RTL867X_CP3
rtl8651_romeperfEnterPoint(ROMEPERF_INDEX_USB_SUBMIT_URB_TX_SC );
#endif
	status = RTL8192SU_submit_urb(priv, tx_urb);
	//msg_queue("submit skb=%x data=%x urb=%x\n",(u32)pdescinfo->pframe,(u32)tx,(u32)tx_urb);

#ifdef RTL867X_CP3
rtl8651_romeperfExitPoint(ROMEPERF_INDEX_USB_SUBMIT_URB_TX_SC );
#endif
#ifdef RTL8192SU_TXSC_DBG
	priv->ext_stats.tx_shortcut_cnt++;
#endif

	if (!status){
		atomic_inc(&priv->tx_pending[q_num]);
	}else{
		printk("Error TX URB(shortcut) ,error %d, q_num=%x, priority=%x, q_select=%x\n", status, q_num, priority, q_select);
		priv->pshare->tx_pool.own[pdescinfo->tx_pool_idx]=0;
		goto signin_tx_shortcut_fail;
	}
	return;
signin_tx_shortcut_fail:

	priv->ext_stats.tx_drops++;
	rtl8192su_siginTx_fail(priv, txcfg, 0);
#endif //!RTL8192SU
}


#elif defined(RTL8190) || defined(RTL8192E)
#ifdef __LINUX_2_6__
__MIPS16
#endif
__IRAM_FASTEXTDEV
void signin_txdesc_shortcut(struct rtl8190_priv *priv, struct tx_insn *txcfg)
{
	struct tx_desc			*phdesc, *pdesc, *pfrstdesc;
	struct tx_desc_info		*pswdescinfo, *pdescinfo;
	struct rtl8190_hw		*phw;
	int						*tx_head, q_num;
	struct stat_info 		*pstat = txcfg->pstat;
	struct sk_buff 			*pskb = (struct sk_buff *)txcfg->pframe;
	unsigned char			*pfwinfo;
	unsigned int			pfrst_dma_desc=0;
	unsigned int			*dma_txhead;

	phw	= GET_HW(priv);
	q_num = txcfg->q_num;

	dma_txhead	= get_txdma_addr(phw, q_num);
	tx_head		= get_txhead_addr(phw, q_num);
	phdesc		= get_txdesc(phw, q_num);
	pswdescinfo	= get_txdesc_info(priv->pshare->pdesc_info, q_num);

	/*------------------------------------------------------------*/
	/*           fill descriptor of header + iv + llc             */
	/*------------------------------------------------------------*/
	pfrstdesc = pdesc = phdesc + *tx_head;
	pdescinfo = pswdescinfo + *tx_head;
	desc_copy(pdesc, &pstat->hwdesc1);

	pfwinfo = txcfg->phdr - sizeof(struct FWtemplate);
	memcpy(pfwinfo, &pstat->fw_bckp, sizeof(struct FWtemplate));
	assign_wlanseq(GET_HW(priv), txcfg->phdr, pstat, GET_MIB(priv)
#ifdef CONFIG_RTK_MESH
		, txcfg->is_11s
#endif
		);

	pdesc->paddr = set_desc(get_physical_addr(priv, txcfg->phdr-sizeof(struct FWtemplate),
		(get_desc(pdesc->flen)&0xffff), PCI_DMA_TODEVICE));

	descinfo_copy(pdescinfo, &pstat->swdesc1);
	pdescinfo->paddr  = get_desc(pdesc->paddr);
	pdescinfo->pframe = txcfg->phdr;
#if defined(SEMI_QOS) && defined(WMM_APSD)
	pdescinfo->priv = priv;
	pdescinfo->pstat = txcfg->pstat;
#endif

	pfrst_dma_desc = dma_txhead[*tx_head];

#ifdef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, get_desc(pdesc->paddr), get_desc(pdesc->flen), PCI_DMA_TODEVICE);
#endif
	txdesc_rollover(pdesc, (unsigned int *)tx_head);

	/*------------------------------------------------------------*/
	/*              fill descriptor of frame body                 */
	/*------------------------------------------------------------*/
	pdesc = phdesc + *tx_head;
	pdescinfo = pswdescinfo + *tx_head;
	desc_copy(pdesc, &pstat->hwdesc2);

	pdesc->paddr = set_desc(get_physical_addr(priv, pskb->data,
		(get_desc(pdesc->flen)&0x0fff), PCI_DMA_TODEVICE));

	descinfo_copy(pdescinfo, &pstat->swdesc2);
	pdescinfo->paddr  = get_desc(pdesc->paddr);
	pdescinfo->pframe = pskb;
	pdescinfo->priv = priv;

#ifndef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, dma_txhead[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#else
	rtl_cache_sync_wback(priv, get_desc(pdesc->paddr), get_desc(pdesc->flen), PCI_DMA_TODEVICE);
#endif

	txdesc_rollover(pdesc, (unsigned int *)tx_head);


	if ((txcfg->privacy == _TKIP_PRIVACY_) &&
		(priv->pshare->have_hw_mic) &&
		!(priv->pmib->dot11StationConfigEntry.swTkipMic))
	{
		register unsigned long int l,r;
		unsigned char *mic;
		int delay = 18;

		while ((*(volatile unsigned int *)GDMAISR & GDMA_COMPIP) == 0) {
			delay_us(delay);
			delay = delay / 2;
		}

		l = *(volatile unsigned int *)GDMAICVL;
		r = *(volatile unsigned int *)GDMAICVR;

		mic = ((struct sk_buff *)txcfg->pframe)->data + txcfg->fr_len - 8;
		mic[0] = (unsigned char)(l & 0xff);
		mic[1] = (unsigned char)((l >> 8) & 0xff);
		mic[2] = (unsigned char)((l >> 16) & 0xff);
		mic[3] = (unsigned char)((l >> 24) & 0xff);
		mic[4] = (unsigned char)(r & 0xff);
		mic[5] = (unsigned char)((r >> 8) & 0xff);
		mic[6] = (unsigned char)((r >> 16) & 0xff);
		mic[7] = (unsigned char)((r >> 24) & 0xff);

#ifdef MICERR_TEST
		if (priv->micerr_flag) {
			mic[7] ^= mic[7];
			priv->micerr_flag = 0;
		}
#endif
	}

#ifdef SUPPORT_SNMP_MIB
	if (txcfg->rts_thrshld <= get_mpdu_len(txcfg, txcfg->fr_len))
		SNMP_MIB_INC(dot11RTSSuccessCount, 1);
#endif

#ifdef BUFFER_TX_AMPDU
	if ((txcfg->aggre_en >= FG_AGGRE_MPDU_BUFFER_FIRST) &&
					(txcfg->aggre_en <= FG_AGGRE_MPDU_BUFFER_LAST)) {
		if (txcfg->aggre_en == FG_AGGRE_MPDU_BUFFER_FIRST) {
			priv->ampdu_first_desc = pfrstdesc;
#ifndef USE_RTL8186_SDK
			priv->ampdu_first_dma_desc = pfrst_dma_desc;
#endif
		}
		else {
			pfrstdesc->cmd |= set_desc(_OWN_);

#ifndef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, pfrst_dma_desc, sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
			if (txcfg->aggre_en == FG_AGGRE_MPDU_BUFFER_LAST) {
				pfrstdesc = priv->ampdu_first_desc;
				pfrstdesc->cmd |= set_desc(_OWN_);
#ifndef USE_RTL8186_SDK
				rtl_cache_sync_wback(priv, priv->ampdu_first_dma_desc, sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
				tx_poll(priv, q_num);
			}
		}
		return;
	}
#endif // BUFFER_TX_AMPDU

	pfrstdesc->cmd |= set_desc(_OWN_);

#ifndef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, pfrst_dma_desc, sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif

	if (q_num == HIGH_QUEUE) {
		DEBUG_WARN("signin shortcut for DTIM pkt?\n");
		return;
	}
	else
		tx_poll(priv, q_num);
}
#endif // 8190, 92, 92se
#endif // TX_SHORTCUT


/* This sub-routine is gonna to check how many tx desc we need */
static int check_txdesc(struct rtl8190_priv *priv, struct tx_insn* txcfg)
{
	struct sk_buff 	*pskb=NULL;
	unsigned short  protocol;
	unsigned char   *da=NULL;
	struct stat_info	*pstat=NULL;
	int priority=0;

	if (txcfg->aggre_en == FG_AGGRE_MSDU_MIDDLE || txcfg->aggre_en == FG_AGGRE_MSDU_LAST)
		return TRUE;

	txcfg->privacy = txcfg->iv = txcfg->icv = txcfg->mic = 0;
	txcfg->frg_num = 0;
	txcfg->need_ack = 1;

	if (txcfg->fr_type == _SKB_FRAME_TYPE_)
	{
		pskb = ((struct sk_buff *)txcfg->pframe);
		txcfg->fr_len = pskb->len - WLAN_ETHHDR_LEN;

#ifdef MP_TEST
		if (OPMODE & WIFI_MP_STATE) {
			txcfg->hdr_len = WLAN_HDR_A3_LEN;
			txcfg->frg_num = 1;
			if (IS_MCAST(pskb->data))
				txcfg->need_ack = 0;
			return TRUE;
		}
#endif

#ifdef WDS
		if (txcfg->wdsIdx >= 0) {
			txcfg->hdr_len = WLAN_HDR_A4_LEN;
			pstat = get_stainfo(priv, priv->pmib->dot11WdsInfo.entry[txcfg->wdsIdx].macAddr);
			if (pstat == NULL) {
				DEBUG_ERR("TX DROP: %s: get_stainfo() for wds failed [%d]!\n", (char *)__FUNCTION__, txcfg->wdsIdx);
				return FALSE;
			}

			txcfg->privacy = priv->pmib->dot11WdsInfo.wdsPrivacy;
			switch (txcfg->privacy) {
				case _WEP_40_PRIVACY_:
				case _WEP_104_PRIVACY_:
					txcfg->iv = 4;
					txcfg->icv = 4;
					break;
				case _TKIP_PRIVACY_:
					txcfg->iv = 8;
					txcfg->icv = 4;
					txcfg->mic = 0;
					txcfg->fr_len += 8; // for Michael padding
					break;
				case _CCMP_PRIVACY_:
					txcfg->iv = 8;
					txcfg->icv = 0;
					txcfg->mic = 8;
					break;
			}
			txcfg->frg_num = 1;
			if (txcfg->aggre_en < FG_AGGRE_MSDU_FIRST) {
				priority = get_skb_priority(priv, (struct sk_buff *)txcfg->pframe);
				PRI_TO_QNUM(priority, txcfg->q_num, priv->pmib->dot11OperationEntry.wifi_specific);
			}

			txcfg->tx_rate = get_tx_rate(priv, pstat);
			txcfg->lowest_tx_rate = get_lowest_tx_rate(priv, pstat, txcfg->tx_rate);
			if (priv->pmib->dot11WdsInfo.entry[pstat->wds_idx].txRate)
				txcfg->fixed_rate = 1;
			txcfg->need_ack = 1;
			txcfg->pstat = pstat;
#ifdef SEMI_QOS
			if ((pstat) && (QOS_ENABLE) && (pstat->QosEnabled) && (is_qos_data(txcfg->phdr)))
				txcfg->hdr_len = WLAN_HDR_A4_QOS_LEN;
#endif

			if (txcfg->aggre_en == 0) {
				if ((pstat->aggre_mthd == AGGRE_MTHD_MPDU) && is_MCS_rate(txcfg->tx_rate))
					txcfg->aggre_en = FG_AGGRE_MPDU;
			}

			return TRUE;
		}
#endif

#ifdef SEMI_QOS
		if (OPMODE & WIFI_AP_STATE)
			pstat = get_stainfo(priv, pskb->data);
		else if (OPMODE & WIFI_STATION_STATE)
			pstat = get_stainfo(priv, BSSID);
		else if (OPMODE & WIFI_ADHOC_STATE)
				pstat = get_stainfo(priv, pskb->data);

		if ((pstat) && (QOS_ENABLE) && (pstat->QosEnabled) && (is_qos_data(txcfg->phdr))) {
#ifdef A4_TUNNEL
			if (priv->pmib->miscEntry.a4tnl_enable)
				txcfg->hdr_len = WLAN_HDR_A4_QOS_LEN;
			else
#endif
				txcfg->hdr_len = WLAN_HDR_A3_QOS_LEN;
		}
		else
#endif
		{
#ifdef A4_TUNNEL
			if (priv->pmib->miscEntry.a4tnl_enable)
				txcfg->hdr_len = WLAN_HDR_A4_LEN;
			else
#endif
				txcfg->hdr_len = WLAN_HDR_A3_LEN;
		}

#ifdef CONFIG_RTK_MESH
		if(txcfg->is_11s)
		{
			txcfg->hdr_len = WLAN_HDR_A4_QOS_LEN ;
			da = txcfg->nhop_11s;
		}
		else
#endif
			da = pskb->data;

		//check if da is associated, if not, just drop and return false
#ifdef CLIENT_MODE
		if ((!IS_MCAST(da)) || (OPMODE & WIFI_STATION_STATE))
#else
		if (!IS_MCAST(da))
#endif
		{
#ifdef CLIENT_MODE
			if (OPMODE & WIFI_STATION_STATE)
				pstat = get_stainfo(priv, BSSID);
			else
#endif
			{
			pstat = get_stainfo(priv, da);
#ifdef A4_TUNNEL
			if ((OPMODE & WIFI_AP_STATE) && (priv->pmib->miscEntry.a4tnl_enable))
				pstat = A4_tunnel_lookup(priv, da);
#endif
			}

			if ((pstat == NULL) || (!(pstat->state & WIFI_ASOC_STATE)))
			{
#ifdef CONFIG_RTK_VOIP
				// rock: error -> info
				DEBUG_INFO("TX DROP: non asoc tx request!\n");
#else
				//DEBUG_ERR("TX DROP: non asoc tx request!\n");
#endif
				return FALSE;
			}

			protocol = ntohs(*((UINT16 *)((UINT8 *)pskb->data + ETH_ALEN*2)));

			if ((((protocol == 0x888E) && ((GET_UNICAST_ENCRYP_KEYLEN == 0)
#ifdef WIFI_SIMPLE_CONFIG
				|| (pstat->state & WIFI_WPS_JOIN)
#endif
				))
#if defined(CONFIG_RTL_WAPI_SUPPORT)
				 || (protocol == ETH_P_WAPI)
#endif

				))
				txcfg->privacy = 0;
			else
				txcfg->privacy = get_privacy(priv, pstat, &txcfg->iv, &txcfg->icv, &txcfg->mic);


			if ((protocol == 0x888E) 
#if defined(CONFIG_RTL_WAPI_SUPPORT)
				||(protocol == ETH_P_WAPI)
#endif
			)
			{
				// use basic rate to send EAP packet for sure
				txcfg->tx_rate = find_rate(priv, NULL, 0, 1);
				txcfg->lowest_tx_rate = txcfg->tx_rate;
				txcfg->fixed_rate = 1;
			}
			else {
				txcfg->tx_rate = get_tx_rate(priv, pstat);
				txcfg->lowest_tx_rate = get_lowest_tx_rate(priv, pstat, txcfg->tx_rate);
				if (!priv->pmib->dot11StationConfigEntry.autoRate)
					txcfg->fixed_rate = 1;
			}

			if (txcfg->aggre_en < FG_AGGRE_MSDU_FIRST) {
#ifdef CONFIG_RTK_MESH
				priority = get_skb_priority3(priv, (struct sk_buff *)txcfg->pframe, txcfg->is_11s);
#else
				priority = get_skb_priority(priv, (struct sk_buff *)txcfg->pframe);
#endif
				PRI_TO_QNUM(priority, txcfg->q_num, priv->pmib->dot11OperationEntry.wifi_specific);
			}

#if defined(CONFIG_RTL_WAPI_SUPPORT)
			if (protocol==ETH_P_WAPI)
			{
				txcfg->q_num = MGNT_QUEUE;
			}
#endif

			if (txcfg->aggre_en == 0
#ifdef SUPPORT_TX_MCAST2UNI
					&& (priv->pshare->rf_ft_var.mc2u_disable || (pskb->cb[2] != (char)0xff))
#endif
				) {
				if ((pstat->aggre_mthd == AGGRE_MTHD_MPDU) &&
					is_MCS_rate(txcfg->tx_rate) && (protocol != 0x888E)
#if defined(CONFIG_RTL_WAPI_SUPPORT)
					&& (protocol != ETH_P_WAPI)
#endif
					)
					txcfg->aggre_en = FG_AGGRE_MPDU;
			}

			if (txcfg->aggre_en >= FG_AGGRE_MPDU && txcfg->aggre_en <= FG_AGGRE_MPDU_BUFFER_LAST) {
				if (!pstat->ADDBA_ready)
					issue_ADDBAreq(priv, pstat, 0);
			}
		}
#ifdef CONFIG_RTK_MESH
		else if(!txcfg->is_11s) // txcfg is not an 11s data frame, but it is multicast
#else
		else
#endif
		{
			// if group key not set yet, don't let unencrypted multicast go to air
			if (priv->pmib->dot11GroupKeysTable.dot11Privacy) {
				if (GET_GROUP_ENCRYP_KEYLEN == 0) {
					DEBUG_ERR("TX DROP: group key not set yet!\n");
					return FALSE;
				}
			}

			txcfg->privacy = get_mcast_privacy(priv, &txcfg->iv, &txcfg->icv, &txcfg->mic);
#ifdef SW_MCAST
			if ((OPMODE & WIFI_AP_STATE) && (priv->release_mcast)) {
				if (priv->pmib->dot11OperationEntry.wifi_specific)
					txcfg->q_num = VO_QUEUE;
				else
					txcfg->q_num = VI_QUEUE;
				if (--priv->queued_mcast)
					SetMData(txcfg->phdr);
			}
#else
			if ((OPMODE & WIFI_AP_STATE) && (priv->sleep_list.next != &priv->sleep_list)) {
				txcfg->q_num = MCAST_QNUM;
				SetMData(txcfg->phdr);
			}
#endif
			else {
				txcfg->q_num = BE_QUEUE;
				pskb->cb[1] = 0;
			}

			if ((*da) == 0xff)	// broadcast
				txcfg->tx_rate = find_rate(priv, NULL, 0, 1);
			else {				// multicast
				if (priv->pmib->dot11StationConfigEntry.lowestMlcstRate)
					txcfg->tx_rate = find_rate(priv, NULL, 0, 1);
				else
					txcfg->tx_rate = find_rate(priv, NULL, 1, 1);
			}

#ifdef _11s_TEST_MODE_
			mesh_debug_tx4(priv, txcfg);
#endif

			txcfg->lowest_tx_rate = txcfg->tx_rate;
			txcfg->fixed_rate = 1;
		}
#ifdef CONFIG_RTK_MESH
		else // txcfg is 11s multicast
		{
			if ((*da) == 0xff)	// broadcast
				txcfg->tx_rate = find_rate(priv, NULL, 0, 1);
			else if (priv->pmib->dot11StationConfigEntry.lowestMlcstRate)
				txcfg->tx_rate = find_rate(priv, NULL, 0, 1);
			else
				txcfg->tx_rate = find_rate(priv, NULL, 1, 1);
			txcfg->fixed_rate = 1;

#ifdef _11s_TEST_MODE_
			mesh_debug_tx5(priv, txcfg);
#endif

			txcfg->lowest_tx_rate = txcfg->tx_rate;
 			txcfg->privacy = _NO_PRIVACY_;

#ifdef SW_MCAST
			if ((OPMODE & WIFI_AP_STATE) && (priv->release_mcast)) {
				if (priv->pmib->dot11OperationEntry.wifi_specific)
					txcfg->q_num = VO_QUEUE;
				else
					txcfg->q_num = VI_QUEUE;
				if (--priv->queued_mcast)
					SetMData(txcfg->phdr);
			}
#else
			else if ((OPMODE & WIFI_AP_STATE) && (priv->sleep_list.next != &priv->sleep_list)) {
				txcfg->q_num = MCAST_QNUM;
				SetMData(txcfg->phdr);
			}
#endif
			else {
				txcfg->q_num = BE_QUEUE;
				pskb->cb[1] = 0;
				//txcfg->q_num = VI_QUEUE;
				//pskb->cb[1] = 5;
			}
		}
#endif // CONFIG_RTK_MESH
	}

#ifdef _11s_TEST_MODE_	/*---11s mgt frame---*/
	else if(txcfg->is_11s)
		mesh_debug_tx6(priv, txcfg);
#endif
	if (!da)
	{
		// This is non data frame, no need to frag.
#ifdef CONFIG_RTK_MESH
		if(txcfg->is_11s)
			da = GetAddr1Ptr(txcfg->phdr);
		else
#endif
			da = get_da((unsigned char *) (txcfg->phdr));

		txcfg->frg_num = 1;

		if (IS_MCAST(da))
			txcfg->need_ack = 0;
		else
			txcfg->need_ack = 1;

		if (GetPrivacy(txcfg->phdr))
		{
			// only auth with legacy wep...
			txcfg->iv = 4;
			txcfg->icv = 4;
			txcfg->privacy = priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm;
		}

		if (txcfg->fr_len != 0)	//for mgt frame
			txcfg->hdr_len += WLAN_HDR_A3_LEN;
	}

#ifdef CLIENT_MODE
	if ((OPMODE & WIFI_AP_STATE) || (OPMODE & WIFI_ADHOC_STATE))
#else
	if (OPMODE & WIFI_AP_STATE)
#endif
	{
		if (IS_MCAST(da))
		{
			txcfg->frg_num = 1;
			txcfg->need_ack = 0;
			txcfg->rts_thrshld = 10000;
		}
		else
		{
			pstat = get_stainfo(priv, da);
#ifdef A4_TUNNEL
			if ((OPMODE & WIFI_AP_STATE) && (priv->pmib->miscEntry.a4tnl_enable))
				pstat = A4_tunnel_lookup(priv, da);
#endif
			txcfg->pstat = pstat;
		}
	}
#ifdef CLIENT_MODE
	else if (OPMODE & WIFI_STATION_STATE)
	{
		if ((txcfg->fr_type == _SKB_FRAME_TYPE_) ||							// skb frame
				!memcmp(GetAddr1Ptr(txcfg->phdr), BSSID, MACADDRLEN)) {		// mgt frame to AP
			pstat = get_stainfo(priv, BSSID);
			txcfg->pstat = pstat;
		}
	}
#endif

	if (txcfg->privacy == _TKIP_PRIVACY_)
		txcfg->fr_len += 8;	// for Michael padding.

	txcfg->frag_thrshld -= (txcfg->mic + txcfg->iv + txcfg->icv + txcfg->hdr_len);

	if (txcfg->frg_num == 0)
	{
		if (txcfg->aggre_en > 0)
			txcfg->frg_num = 1;
		else {
		  // how many mpdu we need...
		  int llc;

		  if ((ntohs(*((UINT16 *)((UINT8 *)pskb->data + ETH_ALEN*2))) + WLAN_ETHHDR_LEN) > WLAN_MAX_ETHFRM_LEN)
			llc = sizeof(struct llc_snap);
		  else
			llc = 0;

		  txcfg->frg_num = (txcfg->fr_len + llc) / txcfg->frag_thrshld;
		  if (((txcfg->fr_len + llc) % txcfg->frag_thrshld) != 0)
			txcfg->frg_num += 1;
	    }
	}

	return TRUE;
}


/*-------------------------------------------------------------------------------
tx flow:
	Please refer to design spec for detail flow

rtl8190_firetx: check if hw desc is available for tx
				hdr_len, iv,icv, tx_rate info are available

signin_txdesc: fillin the desc and txpoll is necessary
--------------------------------------------------------------------------------*/
int rtl8190_firetx(struct rtl8190_priv *priv, struct tx_insn* txcfg)
{
#if !defined(RTL8192SU)
	static int			*tx_head, *tx_tail, q_num;
	static unsigned int	val32;
#else
	int					q_num;
#endif
//	unsigned long		flags;
	struct sk_buff		*pskb;
	struct llc_snap		*pllc_snap;
	struct wlan_ethhdr_t	*pethhdr=NULL, *pmsdu_hdr;
#if !defined(RTL8192SU)
	struct rtl8190_hw	*phw = GET_HW(priv);
#endif
#if defined(CONFIG_RTK_MESH) || defined(CONFIG_RTL_WAPI_SUPPORT)
	struct wlan_ethhdr_t	ethhdr;
	pethhdr = &ethhdr;
#endif
#if defined(TX_SHORTCUT)&&defined(RTL8192SU)
	unsigned char	shortcut_pkt=0;
#endif

	/*---frag_thrshld setting---plus tune---0115*/
#ifdef	WDS
	if (txcfg->wdsIdx >= 0){
		txcfg->frag_thrshld = 2346; // if wds, disable fragment
	}else
#endif
#ifdef CONFIG_RTK_MESH
	if(txcfg->is_11s){
		txcfg->frag_thrshld = 2346; // if Mesh case, disable fragment
	}else
#endif
	{
		txcfg->frag_thrshld = FRAGTHRSLD - _CRCLNG_;
	}
	/*---frag_thrshld setting---end*/

	txcfg->rts_thrshld  = RTSTHRSLD;
	txcfg->frg_num = 0;

#ifdef DFS
	if (!priv->pmib->dot11DFSEntry.disable_DFS &&
		priv->pmib->dot11DFSEntry.disable_tx) {
		priv->ext_stats.tx_drops++;
		DEBUG_ERR("TX DROP: DFS probation period\n");
		if (txcfg->fr_type == _SKB_FRAME_TYPE_) {
			rtl_kfree_skb(priv, (struct sk_buff *)txcfg->pframe, _SKB_TX_);
			release_wlanllchdr_to_poll(priv, txcfg->phdr);
		}
		else if (txcfg->fr_type == _PRE_ALLOCMEM_) {
			release_mgtbuf_to_poll(priv, txcfg->pframe);
			release_wlanhdr_to_poll(priv, txcfg->phdr);
		}
		return SUCCESS;
	}
#endif

	if ((check_txdesc(priv, txcfg)) == FALSE) // this will only happen in errorous forwarding
	{
		priv->ext_stats.tx_drops++;
		// Don't need to print "TX DROP", already print in check_txdesc()
		if (txcfg->fr_type == _SKB_FRAME_TYPE_) {
#if defined(PKT_PROCESSOR) || defined(PRE_ALLOCATE_SKB)
#ifdef DEBUG_MEMORY_LEAK
			struct sk_buff *skb=(struct sk_buff*)txcfg->pframe;
			printk("%s %d freeskb=%x\n",__FUNCTION__,__LINE__,(unsigned int)skb);
#endif
#endif
			rtl_kfree_skb(priv, (struct sk_buff *)txcfg->pframe, _SKB_TX_);
			release_wlanllchdr_to_poll(priv, txcfg->phdr);
		}
		else if (txcfg->fr_type == _PRE_ALLOCMEM_) {
			release_mgtbuf_to_poll(priv, txcfg->pframe);
			release_wlanhdr_to_poll(priv, txcfg->phdr);
		}
		return SUCCESS;
	}

	if (txcfg->aggre_en > 0)
		txcfg->frag_thrshld = 2346;

	if (txcfg->aggre_en < FG_AGGRE_MSDU_FIRST) {
		// now we are going to calculate how many hw desc we should have before tx...
		// wlan_hdr(including iv and llc) will occupy one desc, payload will occupy one, and
		// icv/mic will occupy the third desc if swcrypto is utilized.
		q_num = txcfg->q_num;
#if !defined(RTL8192SU)
		tx_head = get_txhead_addr(phw, q_num);
		tx_tail = get_txtail_addr(phw, q_num);

		if (txcfg->privacy)
		{
			if (UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE)))
				val32 = txcfg->frg_num * 3;  //extra one is for ICV padding.
			else
				val32 = txcfg->frg_num * 2;
		}
		else
			val32 = txcfg->frg_num * 2;

		if ((val32 + 2) > CIRC_SPACE_RTK(*tx_head, *tx_tail, NUM_TX_DESC)) //per mpdu, we need 2 desc...
		{
			 // 2 is for spare...
			DEBUG_ERR("%d hw Queue desc not available! head=%d, tail=%d request %d\n",q_num,*tx_head,*tx_tail,val32);
			 return CONGESTED;
		}
#else
		//if (get_TxUrb_Pending_num(priv)> (NUM_TX_DESC-(NUM_TX_DESC>>5)))
		if (get_TxUrb_Pending_num(priv)> (NUM_TX_DESC-8))
		{
			DEBUG_INFO("Tx CONGESTED[Normal path]!\n");
			return CONGESTED;
		}
#endif		
	}
	else
		  q_num = txcfg->q_num;

#if !defined(RTL8192SU)
	// then we have to check if wlan-hdr is available for usage...
	// actually, the checking can be void

	/* ----------
				Actually, I believe the check below is redundant.
				Since at this moment, desc is available, hdr/icv/
				should be enough.
													--------------*/
	val32 = txcfg->frg_num;

	if (val32 >= priv->pshare->pwlan_hdr_poll->count)
	{
		DEBUG_ERR("%d hw Queue tx without enough wlan_hdr\n", q_num);
		return CONGESTED;
	}
#endif

	// then we have to check if wlan_snapllc_hdrQ is enough for use
	// for each msdu, we need wlan_snapllc_hdrQ for maximum

//	SAVE_INT_AND_CLI(flags);

	if (txcfg->fr_type == _SKB_FRAME_TYPE_)
	{
		pskb = (struct sk_buff *)txcfg->pframe;

		// for AMSDU
		if (txcfg->aggre_en >= FG_AGGRE_MSDU_FIRST) {
			unsigned short usLen;
			int msdu_len;
			struct wlan_ethhdr_t  pethhdr_data;

			memcpy(&pethhdr_data, pskb->data, sizeof(struct wlan_ethhdr_t));
			pethhdr = &pethhdr_data;
			msdu_len = pskb->len - WLAN_ETHHDR_LEN;
			if ((ntohs(pethhdr->type) + WLAN_ETHHDR_LEN) > WLAN_MAX_ETHFRM_LEN) {
				if (skb_headroom(pskb) < sizeof(struct llc_snap)) {
					struct sk_buff *skb2 = dev_alloc_skb(pskb->len);
#ifdef DEBUG_MEMORY_LEAK
					printk("%s %d allocskb=%x\n",__FUNCTION__,__LINE__,(unsigned int)skb2);
#endif
					if (skb2 == NULL) {
						printk("%s: %s, dev_alloc_skb() failed!\n", priv->dev->name, __FUNCTION__);
#ifdef RTL8190_FASTEXTDEV_FUNCALL
						rtl865x_extDev_kfree_skb(pskb, FALSE);
#else

#ifdef DEBUG_MEMORY_LEAK
						printk("%s %d freeskb=%x\n",__FUNCTION__,__LINE__,(unsigned int)pskb);	
#endif
#if defined(PKT_PROCESSOR) || defined(PRE_ALLOCATE_SKB)
						rtl_kfree_skb(priv,pskb,_SKB_TX_);
#else
						dev_kfree_skb_any(pskb);
#endif
#endif //RTL8190_FASTEXTDEV_FUNCALL
						return 0;
					}
					memcpy(skb_put(skb2, pskb->len), pskb->data, pskb->len);
#ifdef RTL8190_FASTEXTDEV_FUNCALL
					rtl865x_extDev_kfree_skb(pskb, FALSE);
#else
#ifdef DEBUG_MEMORY_LEAK
					printk("%s %d freeskb=%x\n",__FUNCTION__,__LINE__,(unsigned int)pskb);	
#endif
#if defined(PKT_PROCESSOR) || defined(PRE_ALLOCATE_SKB)
					rtl_kfree_skb(priv,pskb,_SKB_TX_);
#else
					dev_kfree_skb_any(pskb);
#endif
#endif //RTL8190_FASTEXTDEV_FUNCALL
					pskb = skb2;
					txcfg->pframe = (void *)pskb;
				}
				skb_push(pskb, sizeof(struct llc_snap));
			}
			pmsdu_hdr = (struct wlan_ethhdr_t *)pskb->data;

			memcpy(pmsdu_hdr, pethhdr, 12);
			if ((ntohs(pethhdr->type) + WLAN_ETHHDR_LEN) > WLAN_MAX_ETHFRM_LEN) {
				usLen = (unsigned short)(msdu_len + sizeof(struct llc_snap));
				pmsdu_hdr->type = ntohs(usLen);
				eth_2_llc(pethhdr, (struct llc_snap *)(((unsigned long)pmsdu_hdr)+sizeof(struct wlan_ethhdr_t)));
			}
			else {
				usLen = (unsigned short)msdu_len;
				pmsdu_hdr->type = ntohs(usLen);
			}

			if (txcfg->aggre_en == FG_AGGRE_MSDU_FIRST)
				eth2_2_wlanhdr(priv, pethhdr, txcfg);

			txcfg->llc = 0;
			pllc_snap = NULL;

			if (txcfg->aggre_en == FG_AGGRE_MSDU_FIRST || txcfg->aggre_en == FG_AGGRE_MSDU_MIDDLE) {
				if ((usLen+14) % 4)
					skb_put(pskb, 4-((usLen+14)%4));
			}
			txcfg->fr_len = pskb->len;
		}
		else {	// not A-MSDU
			// now, we should adjust the skb ...
			skb_pull(pskb, WLAN_ETHHDR_LEN);
#ifdef CONFIG_RTK_MESH
			memcpy(pethhdr, pskb->data - sizeof(struct wlan_ethhdr_t), sizeof(struct wlan_ethhdr_t));
#else
			pethhdr = (struct wlan_ethhdr_t *)(pskb->data - sizeof(struct wlan_ethhdr_t));
#endif
			pllc_snap = (struct llc_snap *)((UINT8 *)(txcfg->phdr) + txcfg->hdr_len + txcfg->iv);

			if ((ntohs(pethhdr->type) + WLAN_ETHHDR_LEN) > WLAN_MAX_ETHFRM_LEN) {
				eth_2_llc(pethhdr, pllc_snap);
				txcfg->llc = sizeof(struct llc_snap);
			}
			else
			{
				pllc_snap = NULL;
			}

			eth2_2_wlanhdr(priv, pethhdr, txcfg);

#ifdef CONFIG_RTK_MESH
			if(txcfg->is_11s&1 && GetFrameSubType(txcfg->phdr) == WIFI_11S_MESH)
			{
				const short meshhdrlen= (txcfg->mesh_header.mesh_flag & 0x01) ? 16 : 4;
				if (skb_cloned(pskb))
				{
					struct sk_buff	*newskb = skb_copy(pskb, GFP_ATOMIC);
					rtl_kfree_skb(priv, pskb, _SKB_TX_);
					if (newskb == NULL) {
						priv->ext_stats.tx_drops++;
						release_wlanllchdr_to_poll(priv, txcfg->phdr);
						DEBUG_ERR("TX DROP: Can't copy the skb!\n");
						return SUCCESS;
					}
#ifdef RTL8190_FASTEXTDEV_FUNCALL
					rtl865x_extDev_pktFromProtocolStack(skb);
#endif
					txcfg->pframe = pskb = newskb;
#ifdef ENABLE_RTL_SKB_STATS
					rtl_atomic_inc(&priv->rtl_tx_skb_cnt);
#endif
				}
				memcpy(skb_push(pskb,meshhdrlen), &(txcfg->mesh_header), meshhdrlen);
				txcfg->fr_len += meshhdrlen;
			}
#endif // CONFIG_RTK_MESH
		}

		// TODO: modify as when skb is not bigger enough, take ICV from local pool
#if defined(CONFIG_RTL_WAPI_SUPPORT)
		if (txcfg->privacy == _WAPI_SMS4_)
		{
			if ((pskb->tail + SMS4_MIC_LEN) > pskb->end)
			{
				priv->ext_stats.tx_drops++;
				DEBUG_ERR("TX DROP: an over size skb!\n");
				rtl_kfree_skb(priv, (struct sk_buff *)txcfg->pframe, _SKB_TX_);
				release_wlanllchdr_to_poll(priv, txcfg->phdr);
//					RESTORE_INT(flags);
				wapiAssert(0);
				return SUCCESS;
			}
			memcpy(&ethhdr, pethhdr, sizeof(struct wlan_ethhdr_t));
			pethhdr = &ethhdr;
		} else
#endif

		if (txcfg->privacy == _TKIP_PRIVACY_)
		{
			if ((pskb->tail + 8) > pskb->end)
			{
				priv->ext_stats.tx_drops++;
				DEBUG_ERR("TX DROP: an over size skb!\n");
//				RESTORE_INT(flags);

#if defined(DEBUG_MEMORY_LEAK) && (defined(PKT_PROCESSOR) || defined(PRE_ALLOCATE_SKB))
				skb=(struct sk_buff *)txcfg->pframe;
				printk("%s %d freeskb=%x\n",__FUNCTION__,__LINE__,(unsigned int)skb);
#endif
				rtl_kfree_skb(priv, (struct sk_buff *)txcfg->pframe, _SKB_TX_);
				release_wlanllchdr_to_poll(priv, txcfg->phdr);
				return SUCCESS;
			}

#ifdef SEMI_QOS
			if ((tkip_mic_padding(priv, pethhdr->daddr, pethhdr->saddr, ((QOS_ENABLE) && (txcfg->pstat) && (txcfg->pstat->QosEnabled))?pskb->cb[1]:0, (UINT8 *)pllc_snap,
					pskb, txcfg)) == FALSE)
#else
			if ((tkip_mic_padding(priv, pethhdr->daddr, pethhdr->saddr, 0, (UINT8 *)pllc_snap,
					pskb, txcfg)) == FALSE)
#endif
			{
				priv->ext_stats.tx_drops++;
				DEBUG_ERR("TX DROP: Tkip mic padding fail!\n");
//				RESTORE_INT(flags);
#ifdef DEBUG_MEMORY_LEAK
				printk("%s %d freeskb=%x\n",__FUNCTION__,__LINE__,(unsigned int)pskb);	
#endif
				rtl_kfree_skb(priv, pskb, _SKB_TX_);
				release_wlanllchdr_to_poll(priv, txcfg->phdr);
				return SUCCESS;
			}
			if ((txcfg->aggre_en < FG_AGGRE_MSDU_FIRST) || (txcfg->aggre_en == FG_AGGRE_MSDU_LAST))
				skb_put((struct sk_buff *)txcfg->pframe, 8);
		}
	}

	if (txcfg->privacy && txcfg->aggre_en <= FG_AGGRE_MSDU_FIRST)
		SetPrivacy(txcfg->phdr);

	// log tx statistics...
	tx_sum_up(priv, txcfg->pstat, txcfg->fr_len+txcfg->hdr_len+txcfg->iv+txcfg->llc+txcfg->mic+txcfg->icv);
	SNMP_MIB_INC(dot11TransmittedFragmentCount, 1);

	// for SW LED
	if (txcfg->aggre_en > FG_AGGRE_MSDU_FIRST || GetFrameType(txcfg->phdr) == WIFI_DATA_TYPE)
		priv->pshare->LED_tx_cnt++;
	else
	{
		if ((LED_TYPE >= LEDTYPE_SW_LINK_TXRX) && (LED_TYPE <= LEDTYPE_SW_LINKTXRX))
			priv->pshare->LED_tx_cnt++;
	}

#if	defined(RTL8190) || defined(RTL8192E)
#ifdef SUPPORT_TX_AMSDU
	if (txcfg->aggre_en == FG_AGGRE_MSDU_MIDDLE || txcfg->aggre_en == FG_AGGRE_MSDU_LAST)
		signin_txdesc_amsdu(priv, txcfg);
	else
#endif
#ifdef _11s_TEST_MODE_
		// Galileo
		//  broadcast won's use AMSDU
		signin_txdesc_galileo(priv, txcfg);
#else
		signin_txdesc(priv, txcfg);
#endif
#elif defined(RTL8192SE)
#ifdef SUPPORT_TX_AMSDU
	if (txcfg->aggre_en == FG_AGGRE_MSDU_MIDDLE || txcfg->aggre_en == FG_AGGRE_MSDU_LAST)
		rtl8192SE_signin_txdesc_amsdu(priv, txcfg);
	else
#endif

#ifdef _11s_TEST_MODE_
		signin_txdesc_galileo(priv, txcfg);
#else
		rtl8192SE_signin_txdesc(priv, txcfg);
#endif
#elif defined(RTL8192SU)
#ifdef TX_SHORTCUT
	if ((!priv->pmib->dot11OperationEntry.disable_txsc) &&
		(txcfg->fr_type == _SKB_FRAME_TYPE_) &&
		(txcfg->pstat) &&
		(txcfg->frg_num == 1) &&
		((txcfg->privacy == 0)
#if defined(CONFIG_RTL_WAPI_SUPPORT)
		|| (txcfg->privacy == _WAPI_SMS4_)
#endif
		|| (txcfg->privacy && !UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE)))) &&
		/*(!IEEE8021X_FUN ||
			(IEEE8021X_FUN && (txcfg->pstat->ieee8021x_ctrlport == 1) &&
			(cpu_to_be16(pethhdr->type)!=0x888e))) && */
		(cpu_to_be16(pethhdr->type) != 0x888e) &&
#if defined(CONFIG_RTL_WAPI_SUPPORT)
		 (cpu_to_be16(pethhdr->type)  != ETH_P_WAPI)&&
#endif
		!GetMData(txcfg->phdr) &&
		!IS_MCAST((unsigned char *)pethhdr) &&
		(txcfg->aggre_en < FG_AGGRE_MSDU_FIRST)
		)
	{
		shortcut_pkt=1;
		memcpy((void *)&txcfg->pstat->ethhdr, (const void *)pethhdr, sizeof(struct wlan_ethhdr_t));		
	}
#endif // TX_SHORTCUT
#ifdef SUPPORT_TX_AMSDU
	if (txcfg->aggre_en == FG_AGGRE_MSDU_MIDDLE || txcfg->aggre_en == FG_AGGRE_MSDU_LAST)
		rtl8192SE_signin_txdesc_amsdu(priv, txcfg);
	else
#endif

#ifdef _11s_TEST_MODE_
		signin_txdesc_galileo(priv, txcfg);
#else
		rtl8192SE_signin_txdesc(priv, txcfg);
#endif

#endif

#ifdef TX_SHORTCUT
#if !defined(RTL8192SU)
	if ((!priv->pmib->dot11OperationEntry.disable_txsc) &&
		(txcfg->fr_type == _SKB_FRAME_TYPE_) &&
		(txcfg->pstat) &&
		(txcfg->frg_num == 1) &&
		((txcfg->privacy == 0) 
#if defined(CONFIG_RTL_WAPI_SUPPORT)
		|| (txcfg->privacy == _WAPI_SMS4_)
#endif
		|| (txcfg->privacy && !UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE)))) &&
		/*(!IEEE8021X_FUN ||
			(IEEE8021X_FUN && (txcfg->pstat->ieee8021x_ctrlport == 1) &&
			(cpu_to_be16(pethhdr->type)!=0x888e))) && */
		(cpu_to_be16(pethhdr->type) != 0x888e) &&
#if defined(CONFIG_RTL_WAPI_SUPPORT)
		 (cpu_to_be16(pethhdr->type)  != ETH_P_WAPI)&&
#endif
		!GetMData(txcfg->phdr) &&
		!IS_MCAST((unsigned char *)pethhdr) &&
		(txcfg->aggre_en < FG_AGGRE_MSDU_FIRST)
		)
#else
	if (shortcut_pkt)
#endif	//!RTL8192SU
	{
#if !defined(RTL8192SU)
		memcpy(&txcfg->pstat->txcfg, txcfg, sizeof(struct tx_insn));
		memcpy((void *)&txcfg->pstat->wlanhdr, txcfg->phdr, sizeof(struct wlanllc_hdr));
		memcpy((void *)&txcfg->pstat->ethhdr, (const void *)pethhdr, sizeof(struct wlan_ethhdr_t));
#else //RTL8192SU
		if (txcfg->fr_len>=1400)
			memcpy(&txcfg->pstat->txcfg, txcfg, sizeof(struct tx_insn));
		else
			memcpy(&txcfg->pstat->txcfg_short, txcfg, sizeof(struct tx_insn));			
		memcpy((void *)&txcfg->pstat->wlanhdr, txcfg->phdr, sizeof(struct wlanllc_hdr));
#endif		

#ifdef BUFFER_TX_AMPDU
		if ((txcfg->aggre_en >= FG_AGGRE_MPDU_BUFFER_FIRST) &&
					(txcfg->aggre_en <= FG_AGGRE_MPDU_BUFFER_LAST))
			txcfg->pstat->txcfg.aggre_en = FG_AGGRE_MPDU;
#endif

	}
	else {
		if (txcfg->pstat && txcfg->pstat->txcfg.fr_len > 0) {
			txcfg->pstat->txcfg.fr_len = 0;
		}
	}
#endif

//	RESTORE_INT(flags);
	return SUCCESS;
}

#if !defined(RTL8192SU)
/*
	Procedure to re-cycle TXed packet in Queue index "txRingIdx"

	=> Return value means if system need restart-TX-queue or not.

		1: Need Re-start Queue
		0: Don't Need Re-start Queue
*/

static int rtl8190_tx_recycle(struct rtl8190_priv *priv, unsigned int txRingIdx, int *recycleCnt_p)
{
//	unsigned long	flags;
	struct tx_desc	*pdescH, *pdesc;
	struct tx_desc_info *pdescinfoH, *pdescinfo;
	volatile int	head, tail;
	struct rtl8190_hw	*phw=GET_HW(priv);
	int				needRestartQueue=0;
	int				recycleCnt=0;

//	SAVE_INT_AND_CLI(flags);

	head		= get_txhead(phw, txRingIdx);
	tail		= get_txtail(phw, txRingIdx);
	pdescH		= get_txdesc(phw, txRingIdx);
	pdescinfoH	= get_txdesc_info(priv->pshare->pdesc_info, txRingIdx);

	while (CIRC_CNT_RTK(head, tail, NUM_TX_DESC))
	{
		pdesc = pdescH + (tail);
		pdescinfo = pdescinfoH + (tail);

#ifdef __MIPSEB__
		pdesc = (struct tx_desc *)KSEG1ADDR(pdesc);
#endif

#if	defined(RTL8190) || defined(RTL8192E)
		if (!pdesc || ((get_desc(pdesc->cmd)) & _OWN_))
#elif defined(RTL8192SE)
		if (!pdesc || (get_desc(pdesc->Dword0) & TX_OWN))
#endif
			break;

#ifdef CONFIG_NET_PCI
		if (IS_PCIBIOS_TYPE)
#if	defined(RTL8190) || defined(RTL8192E)
			//use the paddr and flen of pdesc field for icv, mic case which doesn't fill the pdescinfo
			pci_unmap_single(priv->pshare->pdev,
							 get_desc(pdesc->paddr),
							 (get_desc(pdesc->flen)&0xfff),
							 PCI_DMA_TODEVICE);
#elif defined(RTL8192SE)
			//use the paddr and flen of pdesc field for icv, mic case which doesn't fill the pdescinfo
			pci_unmap_single(priv->pshare->pdev,
							 get_desc(pdesc->Dword8),
							 (get_desc(pdesc->Dword7)&0xffff),
							 PCI_DMA_TODEVICE);
#endif
#endif

		if (pdescinfo->type == _SKB_FRAME_TYPE_)
		{
#ifdef MP_TEST
			if (OPMODE & WIFI_MP_CTX_BACKGROUND) {
				struct sk_buff *skb = (struct sk_buff *)(pdescinfo->pframe);
				skb->data = skb->head;
				skb->tail = skb->data;
				skb->len = 0;
				priv->pshare->skb_tail = (priv->pshare->skb_tail + 1) & (NUM_MP_SKB - 1);
			}
			else
#endif
			{
#ifdef __LINUX_2_6__
				rtl_kfree_skb(pdescinfo->priv, (struct sk_buff *)(pdescinfo->pframe), _SKB_TX_IRQ_);
#else
// for debug ------------
//				rtl_kfree_skb(pdescinfo->priv, (struct sk_buff *)(pdescinfo->pframe), _SKB_TX_IRQ_);
				if (((struct sk_buff *)pdescinfo->pframe)->list) {
					DEBUG_ERR("Free tx skb error, skip it!\n");
					priv->ext_stats.freeskb_err++;
				}
				else
					rtl_kfree_skb(pdescinfo->priv, (struct sk_buff *)(pdescinfo->pframe), _SKB_TX_IRQ_);
//----------------------
#endif
				needRestartQueue = 1;
			}
		}
		else if (pdescinfo->type == _PRE_ALLOCMEM_)
		{
			release_mgtbuf_to_poll(priv, (UINT8 *)(pdescinfo->pframe));
		}
		else if (pdescinfo->type == _PRE_ALLOCHDR_)
		{
			release_wlanhdr_to_poll(priv, (UINT8 *)(pdescinfo->pframe));
		}
		else if (pdescinfo->type == _PRE_ALLOCLLCHDR_)
		{
			release_wlanllchdr_to_poll(priv, (UINT8 *)(pdescinfo->pframe));
		}
		else if (pdescinfo->type == _PRE_ALLOCICVHDR_)
		{
			release_icv_to_poll(priv, (UINT8 *)(pdescinfo->pframe));
		}
		else if (pdescinfo->type == _PRE_ALLOCMICHDR_)
		{
			release_mic_to_poll(priv, (UINT8 *)(pdescinfo->pframe));
		}
		else if (pdescinfo->type == _RESERVED_FRAME_TYPE_)
		{
			// the chained skb, no need to release memory
		}
		else
		{
			DEBUG_ERR("Unknown tx frame type %d\n", pdescinfo->type);
		}

		// for skb buffer free
		pdescinfo->pframe = NULL;

		recycleCnt ++;

		tail = (tail + 1) % NUM_TX_DESC;
	}

	*get_txtail_addr(phw, txRingIdx) = tail;

//	RESTORE_INT(flags);

	if (recycleCnt_p)
		*recycleCnt_p = recycleCnt;

	return needRestartQueue;
}


/*-----------------------------------------------------------------------------
Purpose of tx_dsr:

	For ALL TX queues
		1. Free Allocated Buf
		2. Update tx_tail
		3. Update tx related counters
		4. Restart tx queue if needed
------------------------------------------------------------------------------*/
void rtl8190_tx_dsr(unsigned long task_priv)
{
	struct rtl8190_priv	*priv = (struct rtl8190_priv *)task_priv;
	unsigned int	j=0;
	unsigned int	restart_queue=0;
	struct rtl8190_hw	*phw=GET_HW(priv);
	int needRestartQueue;
	unsigned long flags;

	if (!phw)
		return;

	for(j=0; j<=HIGH_QUEUE; j++)
	{
		SAVE_INT_AND_CLI(flags);
		needRestartQueue = rtl8190_tx_recycle(priv, j, NULL);
		RESTORE_INT(flags);
		/* If anyone of queue report the TX queue need to be restart : we would set "restart_queue" to process ALL queues */
		if (needRestartQueue == 1)
			restart_queue = 1;
	}

	if (restart_queue)
		rtl8190_tx_restartQueue(priv);

#ifdef MP_TEST
	if ((OPMODE & (WIFI_MP_STATE|WIFI_MP_CTX_BACKGROUND))==(WIFI_MP_STATE|WIFI_MP_CTX_BACKGROUND)) {
		int *tx_head, *tx_tail;
		tx_head = get_txhead_addr(phw, BE_QUEUE);
		tx_tail = get_txtail_addr(phw, BE_QUEUE);
		if (CIRC_SPACE_RTK(*tx_head, *tx_tail, NUM_TX_DESC) > (NUM_TX_DESC/2))
			mp_ctx(priv, (unsigned char *)"tx-isr");
	}
#endif

	refill_skb_queue(priv);

#ifdef RTL8192SE
	{
		unsigned long x;
		SAVE_INT_AND_CLI(x);
		priv->pshare->has_triggered_tx_tasklet = 0;
		RESTORE_INT(x);
	}
#endif

}


/*
	Try to do TX-DSR for only ONE TX-queue ( rtl8190_tx_dsr would check for ALL TX queue )
*/
//__IRAM_FASTEXTDEV
int rtl8190_tx_queueDsr(struct rtl8190_priv *priv, unsigned int txRingIdx)
{
	int recycleCnt;
	unsigned long flags;
	SAVE_INT_AND_CLI(flags);

	if (rtl8190_tx_recycle(priv, txRingIdx, &recycleCnt) == 1)
		rtl8190_tx_restartQueue(priv);

	RESTORE_INT(flags);
	return recycleCnt;
}
#endif //!RTL8192SU

/*
	Procedure to restart TX Queue
*/
#ifdef RTL8192SU
__IRAM_WIFI_PRI5
#endif
static void rtl8190_tx_restartQueue(struct rtl8190_priv *priv)
{
#ifdef __KERNEL__
	if (netif_queue_stopped(priv->dev)) {
		DEBUG_INFO("wake-up queue\n");
		netif_wake_queue(priv->dev);
	}

#ifdef UNIVERSAL_REPEATER
	if (IS_DRV_OPEN(GET_VXD_PRIV(priv)) && netif_queue_stopped(GET_VXD_PRIV(priv)->dev)) {
		DEBUG_INFO("wake-up VXD queue\n");
		netif_wake_queue(GET_VXD_PRIV(priv)->dev);
	}
#endif

#ifdef MBSSID
#if defined(RTL8192SE) || defined(RTL8192SU)
	if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
#endif
	{
		int bssidIdx;
		for (bssidIdx=0; bssidIdx<RTL8190_NUM_VWLAN; bssidIdx++)
		{
			if (IS_DRV_OPEN(priv->pvap_priv[bssidIdx]) && netif_queue_stopped(priv->pvap_priv[bssidIdx]->dev)) {
				DEBUG_INFO("wake-up Vap%d queue\n", bssidIdx);
				netif_wake_queue(priv->pvap_priv[bssidIdx]->dev);
			}
		}
	}
#endif

#ifdef CONFIG_RTK_MESH
	if (1 == GET_MIB(priv)->dot1180211sInfo.mesh_enable)
	{
		if (netif_running(priv->mesh_dev) &&
				netif_queue_stopped(priv->mesh_dev) )
		{
			netif_wake_queue(priv->mesh_dev);
		}
	}
#endif
#ifdef WDS
	if (priv->pmib->dot11WdsInfo.wdsEnabled) {
		int i;
		for (i=0; i<priv->pmib->dot11WdsInfo.wdsNum; i++) {
			if (netif_running(priv->wds_dev[i]) &&
				netif_queue_stopped(priv->wds_dev[i])) {
				DEBUG_INFO("wake-up wds[%d] queue\n", i);
				netif_wake_queue(priv->wds_dev[i]);
			}
		}
	}
#endif
#endif
}


//_USE_IRAM_ __IRAM_IN_865X
static int tkip_mic_padding(struct rtl8190_priv *priv,
				unsigned char *da, unsigned char *sa, unsigned char priority,
				unsigned char *llc, struct sk_buff *pskb, struct tx_insn* txcfg)
{
	// now check what's the mic key we should apply...

	unsigned char	*mickey = NULL;
	unsigned int	keylen = 0;
	struct stat_info	*pstat = NULL;
	unsigned char	*hdr, hdr_buf[16];
	unsigned int	num_blocks;
	unsigned char	tkipmic[8];
	unsigned char	*pbuf=pskb->data;
	unsigned int	len=pskb->len;
	unsigned int	llc_len = 0;

	// check if the mic/tkip key is valid at this moment.

	if ((pskb->tail + 8) > (pskb->end))
	{
		DEBUG_ERR("pskb have no extra room for TKIP Michael padding\n");
		return FALSE;
	}

#ifdef WDS
	if (txcfg->wdsIdx >= 0) {
		pstat = get_stainfo(priv, priv->pmib->dot11WdsInfo.entry[txcfg->wdsIdx].macAddr);
		if (pstat) {
			keylen = GET_UNICAST_MIC_KEYLEN;
			mickey = GET_UNICAST_TKIP_MIC1_KEY;
		}
	}
	else
#endif
	if (OPMODE & WIFI_AP_STATE)
	{

#ifdef CONFIG_RTK_MESH

		if(txcfg->is_11s)
		{
			keylen = GET_GROUP_MIC_KEYLEN;
			mickey = GET_GROUP_TKIP_MIC1_KEY;
		} else
#endif
		if (IS_MCAST(da))
		{
			keylen = GET_GROUP_MIC_KEYLEN;
			mickey = GET_GROUP_TKIP_MIC1_KEY;
		}
		else
		{
			pstat = get_stainfo (priv, da);
#ifdef A4_TUNNEL
			if (priv->pmib->miscEntry.a4tnl_enable)
				pstat = A4_tunnel_lookup(priv, da);
#endif
			if (pstat == NULL) {
				DEBUG_ERR("tx mic pstat == NULL\n");
				return FALSE;
			}

			keylen = GET_UNICAST_MIC_KEYLEN;
			mickey = GET_UNICAST_TKIP_MIC1_KEY;
		}
	}
#ifdef CLIENT_MODE
	else if (OPMODE & WIFI_STATION_STATE)
	{
		pstat = get_stainfo (priv, BSSID);
		if (pstat == NULL) {
			DEBUG_ERR("tx mic pstat == NULL\n");
			return FALSE;
		}

		keylen = GET_UNICAST_MIC_KEYLEN;
		mickey = GET_UNICAST_TKIP_MIC2_KEY;
	}
	else if (OPMODE & WIFI_ADHOC_STATE)
	{
		keylen = GET_GROUP_MIC_KEYLEN;
		mickey = GET_GROUP_TKIP_MIC1_KEY;
	}
#endif

	if ((txcfg->aggre_en == FG_AGGRE_MSDU_MIDDLE) || (txcfg->aggre_en == FG_AGGRE_MSDU_LAST))
		mickey = pstat->tmp_mic_key;

	if (keylen == 0)
	{
		DEBUG_ERR("no mic padding for TKIP due to keylen=0\n");
		return FALSE;
	}

	if (txcfg->aggre_en <= FG_AGGRE_MSDU_FIRST) {
		hdr = hdr_buf;
		memcpy((void *)hdr, (void *)da, WLAN_ADDR_LEN);
		memcpy((void *)(hdr + WLAN_ADDR_LEN), (void *)sa, WLAN_ADDR_LEN);
		hdr[12] = priority;
		hdr[13] = hdr[14] = hdr[15] = 0;
	}
	else
		hdr = NULL;

	pbuf[len] = 0x5a;   /* Insert padding */
	pbuf[len+1] = 0x00;
	pbuf[len+2] = 0x00;
	pbuf[len+3] = 0x00;
	pbuf[len+4] = 0x00;
	pbuf[len+5] = 0x00;
	pbuf[len+6] = 0x00;
	pbuf[len+7] = 0x00;

	if (llc)
		llc_len = 8;
	num_blocks = (16 + llc_len + len + 5) / 4;
	if ((16 + llc_len + len + 5) & (4-1))
		num_blocks++;

	if (txcfg->aggre_en >= FG_AGGRE_MSDU_FIRST) {
		if (txcfg->aggre_en == FG_AGGRE_MSDU_FIRST) {
			num_blocks = (16 + len) / 4;
			if ((16 + len) & (4-1))
				num_blocks++;
		}
		else if (txcfg->aggre_en == FG_AGGRE_MSDU_MIDDLE) {
			num_blocks = len / 4;
			if (len & (4-1))
				num_blocks++;
		}
		else if (txcfg->aggre_en == FG_AGGRE_MSDU_LAST) {
			num_blocks = (len + 5) / 4;
			if ((len + 5) & (4-1))
				num_blocks++;
		}
	}

	michael(priv, mickey, hdr, llc, pbuf, (num_blocks << 2), tkipmic);

	//tkip mic is MSDU-based, before filled-in descriptor, already finished.
#ifndef CONFIG_RTL8671	//tylo, always get tkip-mic for gdma-memcpy
	if (!(priv->pshare->have_hw_mic) ||
		(priv->pmib->dot11StationConfigEntry.swTkipMic))
#endif
	{
#ifdef MICERR_TEST
		if (priv->micerr_flag) {
			tkipmic[7] ^= tkipmic[7];
			priv->micerr_flag = 0;
		}
#endif
		if ((txcfg->aggre_en == FG_AGGRE_MSDU_FIRST) || (txcfg->aggre_en == FG_AGGRE_MSDU_MIDDLE)) {
			memcpy((void *)pstat->tmp_mic_key, (void *)tkipmic, 8);
		}
		else
			memcpy((void *)(pbuf + len), (void *)tkipmic, 8);
	}

	return TRUE;
}


// _USE_IRAM_ __IRAM_IN_865X
static void wep_fill_iv(struct rtl8190_priv *priv,
				unsigned char *pwlhdr, unsigned int hdrlen, unsigned long keyid)
{
	unsigned char *iv = pwlhdr + hdrlen;
	union PN48 *ptsc48 = NULL;
	union PN48 auth_pn48;
	unsigned char *da;
	struct stat_info *pstat = NULL;

	memset(&auth_pn48, 0, sizeof(union PN48));
	da = get_da(pwlhdr);

#ifdef WDS
	if (get_tofr_ds(pwlhdr) == 3)
		da = GetAddr1Ptr(pwlhdr);
#endif

	if (OPMODE & WIFI_AP_STATE)
	{
		if (IS_MCAST(da))
		{
			ptsc48 = GET_GROUP_ENCRYP_PN;
		}
		else
		{
			pstat = get_stainfo(priv, da);
#ifdef A4_TUNNEL
			if (priv->pmib->miscEntry.a4tnl_enable)
				pstat = A4_tunnel_lookup(priv, GetAddr4Ptr(pwlhdr));
#endif
			ptsc48 = GET_UNICAST_ENCRYP_PN;
		}
	}
#ifdef CLIENT_MODE
	else if (OPMODE & WIFI_STATION_STATE)
	{
		pstat = get_stainfo(priv, BSSID);
		if (pstat != NULL)
			ptsc48 = GET_UNICAST_ENCRYP_PN;
		else
			ptsc48 = &auth_pn48;
	}
	else if (OPMODE & WIFI_ADHOC_STATE)
		ptsc48 = GET_GROUP_ENCRYP_PN;
#endif

	if (ptsc48 == NULL)
	{
		DEBUG_ERR("no TSC for WEP due to ptsc48=NULL\n");
		return;
	}

	iv[0] = ptsc48->_byte_.TSC0;
	iv[1] = ptsc48->_byte_.TSC1;
	iv[2] = ptsc48->_byte_.TSC2;
	iv[3] = 0x0 | (keyid << 6);

	if (ptsc48->val48 == 0xffffffffffffULL)
		ptsc48->val48 = 0;
	else
		ptsc48->val48++;
}


// _USE_IRAM_ __IRAM_IN_865X
static void tkip_fill_encheader(struct rtl8190_priv *priv,
				unsigned char *pwlhdr, unsigned int hdrlen, unsigned long keyid_out)
{
	unsigned char *iv = pwlhdr + hdrlen;
	union PN48 *ptsc48 = NULL;
	unsigned int keyid = 0;
	unsigned char *da;
	struct stat_info *pstat = NULL;

	da = get_da(pwlhdr);

	if (OPMODE & WIFI_AP_STATE)
	{
#ifdef WDS
		unsigned int to_fr_ds = (GetToDs(pwlhdr) << 1) | GetFrDs(pwlhdr);
		if (to_fr_ds == 3)
			da = GetAddr1Ptr(pwlhdr);
#endif

		if (IS_MCAST(da))
		{
			ptsc48 = GET_GROUP_ENCRYP_PN;
			keyid = 1;
		}
		else
		{
			pstat = get_stainfo(priv, da);
#ifdef A4_TUNNEL
			if (priv->pmib->miscEntry.a4tnl_enable)
				pstat = A4_tunnel_lookup(priv, GetAddr4Ptr(pwlhdr));
#endif
			ptsc48 = GET_UNICAST_ENCRYP_PN;
			keyid = 0;
		}
	}
#ifdef CLIENT_MODE
	else if (OPMODE & WIFI_STATION_STATE)
	{
		pstat = get_stainfo(priv, BSSID);
		ptsc48 = GET_UNICAST_ENCRYP_PN;
		keyid = 0;
	}
	else if (OPMODE & WIFI_ADHOC_STATE)
	{
		ptsc48 = GET_GROUP_ENCRYP_PN;
		keyid = 0;
	}
#endif

#ifdef __DRAYTEK_OS__
	keyid = keyid_out;
#endif

	if (ptsc48 == NULL)
	{
		DEBUG_ERR("no TSC for TKIP due to ptsc48=NULL\n");
		return;
	}

	iv[0] = ptsc48->_byte_.TSC1;
	iv[1] = (iv[0] | 0x20) & 0x7f;
	iv[2] = ptsc48->_byte_.TSC0;
	iv[3] = 0x20 | (keyid << 6);
	iv[4] = ptsc48->_byte_.TSC2;
	iv[5] = ptsc48->_byte_.TSC3;
	iv[6] = ptsc48->_byte_.TSC4;
	iv[7] = ptsc48->_byte_.TSC5;

	if (ptsc48->val48 == 0xffffffffffffULL)
		ptsc48->val48 = 0;
	else
		ptsc48->val48++;
}


// _USE_IRAM_ __IRAM_IN_865X
static void aes_fill_encheader(struct rtl8190_priv *priv,
				unsigned char *pwlhdr, unsigned int hdrlen, unsigned long keyid)
{
	unsigned char *da;
	struct stat_info *pstat = NULL;
	union PN48 *pn48 = NULL;
	UINT8 pn_vector[6];

#ifdef __DRAYTEK_OS__
	int	keyid_input = keyid;
#endif

	da = get_da(pwlhdr);

	if (OPMODE & WIFI_AP_STATE)
	{
#ifdef WDS
		unsigned int to_fr_ds = (GetToDs(pwlhdr) << 1) | GetFrDs(pwlhdr);
		if (to_fr_ds == 3)
			da = GetAddr1Ptr(pwlhdr);
#endif

#ifdef CONFIG_RTK_MESH
		if( isMeshPoint(pstat) )
		{
			pn48 = (&((GET_MIB(priv))->dot11sKeysTable.dot11EncryptKey.dot11TXPN48));
			keyid = 0;
		}else
#endif
		if (IS_MCAST(da))
		{
			pn48 = GET_GROUP_ENCRYP_PN;
			keyid = 1;
		}
		else
		{
			pstat = get_stainfo(priv, da);
#ifdef A4_TUNNEL
			if (priv->pmib->miscEntry.a4tnl_enable)
				pstat = A4_tunnel_lookup(priv, GetAddr4Ptr(pwlhdr));
#endif
			pn48 = GET_UNICAST_ENCRYP_PN;
			keyid = 0;
		}
	}
#ifdef CLIENT_MODE
	else if (OPMODE & WIFI_STATION_STATE)
	{
		pstat = get_stainfo(priv, BSSID);
		pn48 = GET_UNICAST_ENCRYP_PN;
		keyid = 0;
	}
	else if (OPMODE & WIFI_ADHOC_STATE)
	{
		pn48 = GET_GROUP_ENCRYP_PN;
		keyid = 0;
	}
#endif

	if (pn48 == NULL)
	{
		DEBUG_ERR("no TSC for AES due to pn48=NULL\n");
		return;
	}

#ifdef __DRAYTEK_OS__
	keyid = keyid_input;
#endif

	pn_vector[0] = pwlhdr[hdrlen]   = pn48->_byte_.TSC0;
	pn_vector[1] = pwlhdr[hdrlen+1] = pn48->_byte_.TSC1;
	pwlhdr[hdrlen+2] =  0x00;
	pwlhdr[hdrlen+3] = (0x20 | (keyid << 6));
	pn_vector[2] = pwlhdr[hdrlen+4] = pn48->_byte_.TSC2;
	pn_vector[3] = pwlhdr[hdrlen+5] = pn48->_byte_.TSC3;
	pn_vector[4] = pwlhdr[hdrlen+6] = pn48->_byte_.TSC4;
	pn_vector[5] = pwlhdr[hdrlen+7] = pn48->_byte_.TSC5;

   	if (pn48->val48 == 0xffffffffffffULL)
		pn48->val48 = 0;
	else
		pn48->val48++;
}

#ifdef RTL8192SU_SW_BEACON
void rtk8192su_beacon_time(unsigned long task_priv)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)task_priv;
	struct wifi_mib	*pmib = GET_MIB(priv);
	struct rtl8190_hw	*phw;
	struct tx_desc	*pdesc;
	unsigned char *pbuf, *pBeacon =(unsigned char *)priv->beaconbuf;
#ifdef MBSSID
		int i;
#endif	
#ifdef LOOPBACK_MODE
	return;
#endif
	unsigned char bcn_err=0;
	DECLARE_TXINSN(txinsn);

#ifdef RTL8192SU_FWBCN
	if (priv->pshare->intf_map==0)
	{
#if defined(MBSSID)
		if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
		{
			unsigned short max_vapid=priv->pshare->max_vapid;
			max_vapid+=2;
			set_fw_reg(priv, (0xf1000001|(max_vapid<<8)), 0, 0);
		}
		else
#endif
			set_fw_reg(priv, 0xf1000001, 0, 0);

		priv->pshare->intf_map=0x01<<7;
		delay_ms(100);
	}
#endif

#ifdef MP_TEST
	if (!(OPMODE & WIFI_MP_STATE))
	{
#endif
#ifdef UNIVERSAL_REPEATER
		if ((OPMODE & WIFI_STATION_STATE) && GET_VXD_PRIV(priv) &&
						(GET_VXD_PRIV(priv)->drv_state & DRV_STATE_VXD_AP_STARTED)) {
			if (GET_VXD_PRIV(priv)->timoffset) {
				update_beacon(GET_VXD_PRIV(priv));
				assign_wlanseq(GET_HW(priv), (unsigned char *)GET_VXD_PRIV(priv)->beaconbuf);
			}
		}
		else
#endif
		{
			if (priv->timoffset) {
				update_beacon(priv);
				//assign_wlanseq(GET_HW(priv), (unsigned char *)priv->beaconbuf);
				//assign_wlanseq(GET_HW(priv), (unsigned char *)priv->beaconbuf, NULL ,pmib);
			}
		}
#if 0//#ifdef RTL8192SU_SW_BEACON
		if ((OPMODE & WIFI_AP_STATE) ||
			 ((OPMODE & WIFI_ADHOC_STATE) &&
				((priv->join_res == STATE_Sta_Ibss_Active) || (priv->join_res == STATE_Sta_Ibss_Idle)))){
			if (OPMODE & WIFI_AP_STATE)
				send_beacon_by_sw(priv);
			else {
				if (!(priv->ext_stats.beacon_ok%(priv->assoc_num+1))) // let other client has chance to send
					send_beacon_by_sw(priv);
			}
			priv->ext_stats.beacon_ok++;
		}
#endif
		//priv->ext_stats.beacon_ok++;

#ifdef MBSSID
		if (IS_VAP_INTERFACE(priv))
		{
			for (i=0; i<RTL8190_NUM_VWLAN; i++) {
				if (IS_DRV_OPEN(priv->pvap_priv[i])) {
					if (priv->pvap_priv[i]->timoffset) {
						update_beacon(priv->pvap_priv[i]);
						//assign_wlanseq(GET_HW(priv), (unsigned char *)priv->pvap_priv[i]->beaconbuf);
					}
				}
			}
		}
#endif
#ifdef RTL8192SU_FWBCN
#if defined(MBSSID) && defined(RTL8192SU)
		if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
			if (IS_VAP_INTERFACE(priv))
				if (priv->pshare->root_bcn==0)
					goto issue_send_beacon_fail;
#endif
	if (priv->update_bcn==0 && priv->pshare->intf_map==0x01<<7)
	{
		goto issue_send_beacon_fail;
	}
#endif

	//printk("pBeacon=%x\n", pBeacon);
	phw = GET_HW(priv);
	//pdesc = (struct tx_desc *)kmalloc((sizeof(struct tx_desc)), GFP_KERNEL);
#if 0//def MBSSID
	if (IS_VAP_INTERFACE(priv))
		pdesc = phw->tx_descB + priv->vap_init_seq;
	else
#endif
		pdesc = phw->tx_descB;

	txinsn.retry = 0;
	txinsn.q_num = BEACON_QUEUE;
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);//find_rate(priv, AP_BSSRATE, AP_BSSRATE_LEN, 0, 1);
	//printk("tx_rate=%d\n", txinsn.tx_rate);
	//txinsn.tx_rate = _MCS0_RATE_;
	txinsn.lowest_tx_rate = txinsn.tx_rate;
	txinsn.fixed_rate = 1;
	txinsn.fr_type = _PRE_ALLOCMEM_;

	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);

	if (pbuf == NULL) {
		//printk("get_mgtbuf_from_poll() failed!\n");
		priv->ext_stats.beacon_er++;
		bcn_err=1;
		goto issue_send_beacon_fail;
	}

	txinsn.phdr = get_wlanhdr_from_poll(priv);
	if (txinsn.phdr == NULL) {
		printk("get_wlanhdr_from_poll() failed!\n");
		priv->ext_stats.beacon_er++;
		bcn_err=1;
		goto issue_send_beacon_fail;
		if (IS_ROOT_INTERFACE(priv))
			printk("[wlan0]send beacon fail!\n");
#ifdef MBSSID
		else
			printk("[wlan0-vap%d]send beacon fail!\n", priv->vap_id);
#endif
	}

	memcpy(txinsn.phdr, pBeacon, WLAN_HDR_A3_LEN);
	//*((unsigned long *)pbuf) = le32_to_cpu(RTL_R32(_TSFTR_));
	//*((unsigned long *)&pbuf[4]) = le32_to_cpu(RTL_R32(_TSFTR_+4));
	memcpy(txinsn.pframe+8, pBeacon+WLAN_HDR_A3_LEN+8, (get_desc(pdesc->Dword0)&0xffff)-WLAN_HDR_A3_LEN-8); // 8:timestamp
	txinsn.fr_len = (get_desc(pdesc->Dword0)&0xffff)-WLAN_HDR_A3_LEN;

	//priv->pshare->mgt_poll_idx[0] += 1;
	//memDump(pdesc, 32, "BCN_PDESC");
#ifdef RTL8192SU_FWBCN
#if defined(MBSSID)
		if (IS_ROOT_INTERFACE(priv))
#endif			
			priv->pshare->root_bcn=1;
		if (priv->update_bcn>0)
			priv->update_bcn--;
#else
	priv->pshare->root_bcn=1;
#endif
	if ((rtl8190_firetx(priv, &txinsn)) == SUCCESS)
	{
		mod_timer(&priv->beacon_timer, jiffies+(pmib->dot11StationConfigEntry.dot11BeaconPeriod/10));
		return;
	}

issue_send_beacon_fail:
	mod_timer(&priv->beacon_timer, jiffies+(pmib->dot11StationConfigEntry.dot11BeaconPeriod/10));
	if (bcn_err)
	{
		if (txinsn.phdr)
			release_wlanhdr_to_poll(priv, txinsn.phdr);
		if (txinsn.pframe)
			release_mgtbuf_to_poll(priv, txinsn.pframe);
	}

#ifdef MP_TEST
}
#endif //MP_TEST

}
#endif


