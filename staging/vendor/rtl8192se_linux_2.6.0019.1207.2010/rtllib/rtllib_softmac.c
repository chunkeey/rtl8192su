/* IEEE 802.11 SoftMAC layer
 * Copyright (c) 2005 Andrea Merello <andreamrl@tiscali.it>
 *
 * Mostly extracted from the rtl8180-sa2400 driver for the 
 * in-kernel generic ieee802.11 stack.
 *
 * Few lines might be stolen from other part of the rtllib
 * stack. Copyright who own it's copyright
 *
 * WPA code stolen from the ipw2200 driver.
 * Copyright who own it's copyright. 
 *
 * released under the GPL
 */


#include "rtllib.h"

#include <linux/random.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <asm/uaccess.h>
#ifdef ENABLE_DOT11D
#include "dot11d.h"
#endif

#ifdef ASL
#include "../HAL/rtl8192/rtl_softap.h"
#endif

#ifdef RTK_DMP_PLATFORM
#include <linux/usb_setting.h> 
#endif
extern void _setup_timer( struct timer_list*, void*, unsigned long );
u8 rsn_authen_cipher_suite[16][4] = {
	{0x00,0x0F,0xAC,0x00}, 
	{0x00,0x0F,0xAC,0x01}, 
	{0x00,0x0F,0xAC,0x02}, 
	{0x00,0x0F,0xAC,0x03}, 
	{0x00,0x0F,0xAC,0x04}, 
	{0x00,0x0F,0xAC,0x05}, 
};

short rtllib_is_54g(struct rtllib_network *net)
{
	return ((net->rates_ex_len > 0) || (net->rates_len > 4));
}

short rtllib_is_shortslot(struct rtllib_network net)
{
	return (net.capability & WLAN_CAPABILITY_SHORT_SLOT_TIME);
}

/* returns the total length needed for pleacing the RATE MFIE
 * tag and the EXTENDED RATE MFIE tag if needed.
 * It encludes two bytes per tag for the tag itself and its len
 */
unsigned int rtllib_MFIE_rate_len(struct rtllib_device *ieee)
{
	unsigned int rate_len = 0;
	
	if (ieee->modulation & RTLLIB_CCK_MODULATION)
		rate_len = RTLLIB_CCK_RATE_LEN + 2;
		
	if (ieee->modulation & RTLLIB_OFDM_MODULATION)
		
		rate_len += RTLLIB_OFDM_RATE_LEN + 2;
	
	return rate_len;
}

/* pleace the MFIE rate, tag to the memory (double) poined. 
 * Then it updates the pointer so that
 * it points after the new MFIE tag added.
 */  
void rtllib_MFIE_Brate(struct rtllib_device *ieee, u8 **tag_p)
{
	u8 *tag = *tag_p; 
	
	if (ieee->modulation & RTLLIB_CCK_MODULATION){
		*tag++ = MFIE_TYPE_RATES;
		*tag++ = 4;
		*tag++ = RTLLIB_BASIC_RATE_MASK | RTLLIB_CCK_RATE_1MB;
		*tag++ = RTLLIB_BASIC_RATE_MASK | RTLLIB_CCK_RATE_2MB;
		*tag++ = RTLLIB_BASIC_RATE_MASK | RTLLIB_CCK_RATE_5MB;
		*tag++ = RTLLIB_BASIC_RATE_MASK | RTLLIB_CCK_RATE_11MB;
	}
	
	/* We may add an option for custom rates that specific HW might support */
	*tag_p = tag;
}

void rtllib_MFIE_Grate(struct rtllib_device *ieee, u8 **tag_p)
{	
	u8 *tag = *tag_p; 
	
		if (ieee->modulation & RTLLIB_OFDM_MODULATION){
		
		*tag++ = MFIE_TYPE_RATES_EX;
		*tag++ = 8;
		*tag++ = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_6MB;
		*tag++ = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_9MB;
		*tag++ = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_12MB;
		*tag++ = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_18MB;
		*tag++ = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_24MB;
		*tag++ = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_36MB;
		*tag++ = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_48MB;
		*tag++ = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_54MB;
		
	}
	
	/* We may add an option for custom rates that specific HW might support */
	*tag_p = tag;
}

void rtllib_WMM_Info(struct rtllib_device *ieee, u8 **tag_p) {
	u8 *tag = *tag_p;

	*tag++ = MFIE_TYPE_GENERIC; 
	*tag++ = 7;
	*tag++ = 0x00;
	*tag++ = 0x50;
	*tag++ = 0xf2;
	*tag++ = 0x02;
	*tag++ = 0x00;
	*tag++ = 0x01;
	if(ieee->current_qos_mode & QOS_WMM_UAPSD)
		if(ieee->b4ac_Uapsd & 0x0f) {
			*tag++ = (ieee->b4ac_Uapsd & 0x0f) |MAX_SP_Len;
	} else {
		*tag++ = MAX_SP_Len;
	}
	else
		*tag++ = 0;
	*tag_p = tag;
}

#ifdef THOMAS_TURBO
void rtllib_TURBO_Info(struct rtllib_device *ieee, u8 **tag_p) {
	u8 *tag = *tag_p;

        *tag++ = MFIE_TYPE_GENERIC; 
        *tag++ = 7;
        *tag++ = 0x00;
        *tag++ = 0xe0;
        *tag++ = 0x4c;
        *tag++ = 0x01;
        *tag++ = 0x02;
        *tag++ = 0x11;
	*tag++ = 0x00;

	*tag_p = tag;
	printk(KERN_ALERT "This is enable turbo mode IE process\n");
}
#endif

void enqueue_mgmt(struct rtllib_device *ieee, struct sk_buff *skb)
{
	int nh;
	nh = (ieee->mgmt_queue_head +1) % MGMT_QUEUE_NUM;
			
/*
 * if the queue is full but we have newer frames then
 * just overwrites the oldest.
 *	
 * if (nh == ieee->mgmt_queue_tail)
 *		return -1;
 */		
	ieee->mgmt_queue_head = nh;
	ieee->mgmt_queue_ring[nh] = skb;
	
}

struct sk_buff *dequeue_mgmt(struct rtllib_device *ieee)
{
	struct sk_buff *ret;
	
	if(ieee->mgmt_queue_tail == ieee->mgmt_queue_head)
		return NULL;
		
	ret = ieee->mgmt_queue_ring[ieee->mgmt_queue_tail];
	
	ieee->mgmt_queue_tail = 
		(ieee->mgmt_queue_tail+1) % MGMT_QUEUE_NUM;
	
	return ret;
}

void init_mgmt_queue(struct rtllib_device *ieee)
{
	ieee->mgmt_queue_tail = ieee->mgmt_queue_head = 0;
}

#ifdef ASL
u8 MgntQuery_TxRateExcludeCCKRates(struct rtllib_device *ieee, bool is_ap)
#else
u8 MgntQuery_TxRateExcludeCCKRates(struct rtllib_device *ieee)
#endif
{
	u16	i;
	u8	QueryRate = 0;
	u8	BasicRate;
	struct rtllib_network * net = NULL;
#ifdef ASL
	if (is_ap)
		net = &ieee->current_ap_network;
	else
#endif
		net = &ieee->current_network;
	
	for( i = 0; i < net->rates_len; i++)
	{
		BasicRate = net->rates[i]&0x7F;
		if(!rtllib_is_cck_rate(BasicRate))
		{
			if(QueryRate == 0)
			{
				QueryRate = BasicRate;
			}
			else
			{
				if(BasicRate < QueryRate)
				{
					QueryRate = BasicRate;
				}
			}
		}		
	}

	if(QueryRate == 0)
	{
		QueryRate = 12;	
		printk("No BasicRate found!!\n");
	}
	return QueryRate;
}
#ifdef ASL
u8 MgntQuery_MgntFrameTxRate(struct rtllib_device *ieee, bool is_ap)
#else
u8 MgntQuery_MgntFrameTxRate(struct rtllib_device *ieee)
#endif
{
	PRT_HIGH_THROUGHPUT      pHTInfo = NULL;
	u8 rate = 0;
	int mode = 0;
#ifdef ASL
	if (is_ap) {
		pHTInfo = ieee->pAPHTInfo;
		mode = ieee->apmode;
	}
	else 
#endif
	{
		pHTInfo = ieee->pHTInfo;
		mode = ieee->mode;
	}
#if defined RTL8190P || defined RTL8192E || defined RTL8192U
	if(pHTInfo->IOTAction & HT_IOT_ACT_MGNT_USE_CCK_6M)
		rate = 0x0c;    
	else
		rate = ieee->basic_rate & 0x7f;
#elif defined RTL8192SE || defined RTL8192SU || defined RTL8192CE
	if(pHTInfo->IOTAction & HT_IOT_ACT_WA_IOT_Broadcom)
	{
#ifdef ASL
		rate = MgntQuery_TxRateExcludeCCKRates(ieee, is_ap);
#else
		rate = MgntQuery_TxRateExcludeCCKRates(ieee);
#endif
	}
	else
		rate = ieee->basic_rate & 0x7f;
#endif

	if(rate == 0){
		if(mode == IEEE_A||
		   mode== IEEE_N_5G||
		   (mode== IEEE_N_24G&&!pHTInfo->bCurSuppCCK))
			rate = 0x0c;
		else
			rate = 0x02;
	}

	/*
	if( pMgntInfo->bScanInProgress || (pMgntInfo->bDualModeScanStep!=0) )
	{
	if(pMgntInfo->dot11CurrentWirelessMode==WIRELESS_MODE_A)
	rate = 0x0c;
	else
	rate = 0x02;
	}
	 */
	return rate;
}


void rtllib_sta_wakeup(struct rtllib_device *ieee, short nl);
#ifdef ASL
inline void softmac_mgmt_xmit(struct sk_buff *skb, struct rtllib_device *ieee, bool is_ap)
#else
inline void softmac_mgmt_xmit(struct sk_buff *skb, struct rtllib_device *ieee)
#endif
{
	unsigned long flags;
	short single = ieee->softmac_features & IEEE_SOFTMAC_SINGLE_QUEUE;
	struct rtllib_hdr_3addr  *header=
		(struct rtllib_hdr_3addr  *) skb->data;

	cb_desc *tcb_desc = (cb_desc *)(skb->cb + 8);
	spin_lock_irqsave(&ieee->lock, flags);
	
	/* called with 2nd param 0, no mgmt lock required */
	rtllib_sta_wakeup(ieee,0);

	if(header->frame_ctl == RTLLIB_STYPE_BEACON) {
#if defined(RTL8192CE) && defined(_RTL8192_EXT_PATCH_)
		if((ieee->iw_mode == IW_MODE_MESH) && !ieee->only_mesh)
			tcb_desc->queue_index = MGNT_QUEUE;
	else
#endif
			tcb_desc->queue_index = BEACON_QUEUE;	
	} else {
		tcb_desc->queue_index = MGNT_QUEUE;
	}

	if(ieee->disable_mgnt_queue)
		tcb_desc->queue_index = HIGH_QUEUE;
#ifdef ASL
	tcb_desc->data_rate = MgntQuery_MgntFrameTxRate(ieee, is_ap);
#else
	tcb_desc->data_rate = MgntQuery_MgntFrameTxRate(ieee);	
#endif
	tcb_desc->RATRIndex = 7;
	tcb_desc->bTxDisableRateFallBack = 1;
	tcb_desc->bTxUseDriverAssingedRate = 1;
#ifdef _RTL8192_EXT_PATCH_
	tcb_desc->macId = 0;
#endif	
	if(single){
		if(ieee->queue_stop){
			enqueue_mgmt(ieee,skb);
		}else{
			header->seq_ctl = cpu_to_le16(ieee->seq_ctrl[0]<<4);

			if (ieee->seq_ctrl[0] == 0xFFF)
				ieee->seq_ctrl[0] = 0;
			else
				ieee->seq_ctrl[0]++;
			
			/* avoid watchdog triggers */
			ieee->softmac_data_hard_start_xmit(skb,ieee->dev,ieee->basic_rate);
		}
		
		spin_unlock_irqrestore(&ieee->lock, flags);
	}else{
		spin_unlock_irqrestore(&ieee->lock, flags);
		spin_lock_irqsave(&ieee->mgmt_tx_lock, flags);

		header->seq_ctl = cpu_to_le16(ieee->seq_ctrl[0] << 4);

		if (ieee->seq_ctrl[0] == 0xFFF)
			ieee->seq_ctrl[0] = 0;
		else
			ieee->seq_ctrl[0]++;

		/* check wether the managed packet queued greater than 5 */
		if(!ieee->check_nic_enough_desc(ieee->dev,tcb_desc->queue_index)||\
				(skb_queue_len(&ieee->skb_waitQ[tcb_desc->queue_index]) != 0)||\
				(ieee->queue_stop) ) {
			/* insert the skb packet to the management queue */
			/* as for the completion function, it does not need 
			 * to check it any more.
			 * */
			printk("%s():insert to waitqueue, queue_index:%d!\n",__FUNCTION__,tcb_desc->queue_index);
			skb_queue_tail(&ieee->skb_waitQ[tcb_desc->queue_index], skb);
		} else {
			ieee->softmac_hard_start_xmit(skb,ieee->dev);
		}
		spin_unlock_irqrestore(&ieee->mgmt_tx_lock, flags);
	}
}
#ifdef ASL
inline void softmac_ps_mgmt_xmit(struct sk_buff *skb, struct rtllib_device *ieee, u8 queue_index, bool is_ap)
#else
inline void softmac_ps_mgmt_xmit(struct sk_buff *skb, struct rtllib_device *ieee, u8 queue_index)
#endif
{
	short single = ieee->softmac_features & IEEE_SOFTMAC_SINGLE_QUEUE;
	struct rtllib_hdr_3addr  *header =
		(struct rtllib_hdr_3addr  *) skb->data;
	u16 fc,type,stype;
	cb_desc *tcb_desc = (cb_desc *)(skb->cb + 8);

	fc = header->frame_ctl;
	type = WLAN_FC_GET_TYPE(fc);
	stype = WLAN_FC_GET_STYPE(fc);

	tcb_desc->queue_index = queue_index;
	if(ieee->disable_mgnt_queue && (queue_index == MGNT_QUEUE))
		tcb_desc->queue_index = HIGH_QUEUE;
		
#ifdef ASL
		tcb_desc->data_rate = MgntQuery_MgntFrameTxRate(ieee, is_ap);
#else
	tcb_desc->data_rate = MgntQuery_MgntFrameTxRate(ieee);
#endif
	tcb_desc->RATRIndex = 7;
	tcb_desc->bTxDisableRateFallBack = 1;
	tcb_desc->bTxUseDriverAssingedRate = 1;
	if (single) {
		if (type != RTLLIB_FTYPE_CTL) {
			header->seq_ctl = cpu_to_le16(ieee->seq_ctrl[0] << 4);

			if (ieee->seq_ctrl[0] == 0xFFF)
				ieee->seq_ctrl[0] = 0;
			else
				ieee->seq_ctrl[0]++;

		}
		/* avoid watchdog triggers */
		ieee->softmac_data_hard_start_xmit(skb,ieee->dev,ieee->basic_rate);

	} else {
		if (type != RTLLIB_FTYPE_CTL) {
			header->seq_ctl = cpu_to_le16(ieee->seq_ctrl[0] << 4);

			if (ieee->seq_ctrl[0] == 0xFFF)
				ieee->seq_ctrl[0] = 0;
			else
				ieee->seq_ctrl[0]++;
		}
		ieee->softmac_hard_start_xmit(skb,ieee->dev);

	}
}

inline struct sk_buff *rtllib_probe_req(struct rtllib_device *ieee)
{
	unsigned int len,rate_len;
	u8 *tag;
	struct sk_buff *skb;
	struct rtllib_probe_request *req;
	
	u8* ht_cap_buf = NULL;
	u8 ht_cap_len=0;
	int encrypt;
	struct rtllib_crypt_data* crypt;
	int wpa_ie_len = ieee->wpa_ie_len;
	
#ifdef _RTL8192_EXT_PATCH_
	short extMore = 0;
	if(ieee->ext_patch_rtllib_probe_req_1)
		extMore = ieee->ext_patch_rtllib_probe_req_1(ieee);
#endif

	len = ieee->current_network.ssid_len;
	
	rate_len = rtllib_MFIE_rate_len(ieee);
	
#ifdef _RTL8192_EXT_PATCH_
	if(!extMore)
		skb = dev_alloc_skb(sizeof(struct rtllib_probe_request) +
			    2 + len + rate_len + ieee->tx_headroom);
	else
		skb = dev_alloc_skb(sizeof(struct rtllib_probe_request) +
			    2 + len + rate_len+128+ieee->tx_headroom); 
#else
#ifdef USB_USE_ALIGNMENT
        u32 Tmpaddr;
        int alignment;
        skb = dev_alloc_skb(sizeof(struct rtllib_probe_request) +
                            2 + len + rate_len + ieee->tx_headroom + USB_512B_ALIGNMENT_SIZE);
#else	
	skb = dev_alloc_skb(sizeof(struct rtllib_probe_request) +
			    2 + len + rate_len + ieee->tx_headroom);
#endif
#endif

	if (!skb) 
		return NULL;

#ifdef USB_USE_ALIGNMENT
        Tmpaddr = (u32)skb->data;
        alignment = Tmpaddr & 0x1ff;
        skb_reserve(skb,(USB_512B_ALIGNMENT_SIZE - alignment));
#endif
	
	skb_reserve(skb, ieee->tx_headroom);
	
	req = (struct rtllib_probe_request *) skb_put(skb,sizeof(struct rtllib_probe_request));
	req->header.frame_ctl = cpu_to_le16(RTLLIB_STYPE_PROBE_REQ);
	req->header.duration_id = 0; 
	
	memset(req->header.addr1, 0xff, ETH_ALEN);
	memcpy(req->header.addr2, ieee->dev->dev_addr, ETH_ALEN);
	memset(req->header.addr3, 0xff, ETH_ALEN);
	
	tag = (u8 *) skb_put(skb,len+2+rate_len);
	
	*tag++ = MFIE_TYPE_SSID;
	*tag++ = len;
	memcpy(tag, ieee->current_network.ssid, len);
	tag += len;
	
	rtllib_MFIE_Brate(ieee,&tag);
	rtllib_MFIE_Grate(ieee,&tag);

#ifdef _RTL8192_EXT_PATCH_
	if(extMore)
		ieee->ext_patch_rtllib_probe_req_2(ieee, skb, tag);
#endif

        if(0)
	{
#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
		crypt = ieee->sta_crypt[ieee->tx_keyidx];
#else
		crypt = ieee->crypt[ieee->tx_keyidx];
#endif
		if(crypt != NULL) {
			encrypt = ieee->host_encrypt && crypt && crypt->ops && ((0 == strcmp(crypt->ops->name,"WEP") || wpa_ie_len));
		} else {
			encrypt = 0;
		}
	
		ht_cap_buf = (u8*)&(ieee->pHTInfo->SelfHTCap);
		ht_cap_len = sizeof(ieee->pHTInfo->SelfHTCap);
#ifdef _RTL8192_EXT_PATCH_
		HTConstructCapabilityElement(ieee, ht_cap_buf, &ht_cap_len, encrypt, 0, false);
#else
#ifdef ASL
		HTConstructCapabilityElement(ieee, ht_cap_buf, &ht_cap_len, encrypt, 0, false, false);
#else
		HTConstructCapabilityElement(ieee, ht_cap_buf, &ht_cap_len, encrypt, false);
#endif
#endif
		if(ht_cap_len){
			*(tag++) = MFIE_TYPE_HT_CAP;
			*(tag++) = ht_cap_len - 2;
			memcpy(tag, ht_cap_buf, ht_cap_len - 2);	
			tag += ht_cap_len - 2;
		}
	}
	
	return skb;
}

struct sk_buff *rtllib_get_beacon_(struct rtllib_device *ieee);

#ifdef _RTL8192_EXT_PATCH_
void ext_rtllib_send_beacon_wq(void *data)
{
	struct rtllib_device *ieee = (struct rtllib_device *)container_of_work_rsl((struct work_struct *)data, struct rtllib_device, ext_send_beacon_wq);
	struct sk_buff *skb;
	
	skb = rtllib_get_beacon_(ieee);

	if (skb){
		softmac_mgmt_xmit(skb, ieee);
		ieee->softmac_stats.tx_beacons++;
	}
	

	
}
#endif
void rtllib_send_beacon(struct rtllib_device *ieee)
{
	struct sk_buff *skb;
	if(!ieee->ieee_up)
		return;		
	skb = rtllib_get_beacon_(ieee);
	
	if (skb){
#ifdef ASL
		if (ieee->iw_mode == IW_MODE_MASTER || ieee->iw_mode == IW_MODE_APSTA)
			softmac_mgmt_xmit(skb, ieee, true);
		else 
			softmac_mgmt_xmit(skb, ieee, false);
#else
		softmac_mgmt_xmit(skb, ieee);
#endif
		ieee->softmac_stats.tx_beacons++;
	}
	
	if(ieee->beacon_txing && ieee->ieee_up){
#ifdef ASL
		if (ieee->iw_mode == IW_MODE_MASTER || ieee->iw_mode == IW_MODE_APSTA)
			mod_timer(&ieee->beacon_timer,jiffies+(MSECS(ieee->current_ap_network.beacon_interval-5)));
		else
#endif
		mod_timer(&ieee->beacon_timer,jiffies+(MSECS(ieee->current_network.beacon_interval-5)));

	}
}


void rtllib_send_beacon_cb(unsigned long _ieee)
{
	struct rtllib_device *ieee =
		(struct rtllib_device *) _ieee;
	unsigned long flags;

	spin_lock_irqsave(&ieee->beacon_lock, flags);
	rtllib_send_beacon(ieee);
	spin_unlock_irqrestore(&ieee->beacon_lock, flags);
}

#ifdef _RTL8192_EXT_PATCH_

inline struct sk_buff *rtllib_probe_req_with_SSID(struct rtllib_device *ieee, char *ssid, int len_ssid)
{
	unsigned int len,rate_len;
	u8 *tag;
	struct sk_buff *skb;
	struct rtllib_probe_request *req;
	
#ifdef _RTL8192_EXT_PATCH_
	short extMore = 0;
	if(ieee->ext_patch_rtllib_probe_req_1)
		extMore = ieee->ext_patch_rtllib_probe_req_1(ieee);
#endif

	len = len_ssid;
	
	rate_len = rtllib_MFIE_rate_len(ieee);
	
#ifdef _RTL8192_EXT_PATCH_
	if(!extMore)
#endif
		skb = dev_alloc_skb(sizeof(struct rtllib_probe_request) +
			    2 + len + rate_len + ieee->tx_headroom);
#ifdef _RTL8192_EXT_PATCH_
	else
		skb = dev_alloc_skb(sizeof(struct rtllib_probe_request) +
			    2 + len + rate_len+128+ieee->tx_headroom); 
#endif
	
	if (!skb) 
		return NULL;
	
	req = (struct rtllib_probe_request *) skb_put(skb,sizeof(struct rtllib_probe_request));
	req->header.frame_ctl = cpu_to_le16(RTLLIB_STYPE_PROBE_REQ);
	req->header.duration_id = 0; 
	
	memset(req->header.addr1, 0xff, ETH_ALEN);
	memcpy(req->header.addr2, ieee->dev->dev_addr, ETH_ALEN);
	memset(req->header.addr3, 0xff, ETH_ALEN);
	
	tag = (u8 *) skb_put(skb,len+2+rate_len);
	
	*tag++ = MFIE_TYPE_SSID;
	*tag++ = len;
	if(len)
	{
		memcpy(tag, ssid, len);
		tag += len;
	}
	
	rtllib_MFIE_Brate(ieee,&tag);
	rtllib_MFIE_Grate(ieee,&tag);
	
#ifdef _RTL8192_EXT_PATCH_
	if(extMore)
		ieee->ext_patch_rtllib_probe_req_2(ieee, skb, tag);
#endif
	return skb;
}

#endif 

#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE)
void rtllib_ibss_wait_timeout(unsigned long _ieee)
{
	struct rtllib_device *ieee =
		(struct rtllib_device *) _ieee;
	printk("======>%s():oh oh ibss wait beacon time out, search a new ibss now\n",__FUNCTION__);	
	rtllib_stop_send_beacons(ieee);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)	
	cancel_delayed_work(&ieee->start_ibss_wq);
	cancel_delayed_work(&ieee->link_change_wq);
#endif	
	rtllib_stop_scan(ieee);
	queue_delayed_work_rsl(ieee->wq, &ieee->start_ibss_wq, MSECS(150)); 
}
#endif


/*
 * Description:
 *              Enable network monitor mode, all rx packets will be received.
 */
void rtllib_EnableNetMonitorMode(struct net_device* dev,
                bool bInitState)
{
        struct rtllib_device* ieee = netdev_priv_rsl(dev);

        printk("========>Enter Monitor Mode\n");

        ieee->AllowAllDestAddrHandler(dev, true, !bInitState);
}


/*
 *      Description:
 *              Disable network network monitor mode, only packets destinated to
 *              us will be received.
 */
void rtllib_DisableNetMonitorMode(struct net_device* dev,
                bool bInitState)
{
	struct rtllib_device* ieee = netdev_priv_rsl(dev);

        printk("========>Exit Monitor Mode\n");

        ieee->AllowAllDestAddrHandler(dev, false, !bInitState);
}


/*
 * Description:
 *              This enables the specialized promiscuous mode required by Intel.
 *              In this mode, Intel intends to hear traffics from/to other STAs in the same BSS.
 *              Therefore we don't have to disable checking BSSID and we only need to allow all dest.
 *              BUT: if we enable checking BSSID then we can't recv packets from other STA.
 */
void rtllib_EnableIntelPromiscuousMode(struct net_device* dev,
                bool bInitState)
{
        bool bFilterOutNonAssociatedBSSID = false;

	struct rtllib_device* ieee = netdev_priv_rsl(dev);

        printk("========>Enter Intel Promiscuous Mode\n");

        ieee->AllowAllDestAddrHandler(dev, true, !bInitState);
        ieee->SetHwRegHandler(dev, HW_VAR_CECHK_BSSID, (u8*)&bFilterOutNonAssociatedBSSID);

        ieee->bNetPromiscuousMode = true;
}


/*
 * Description:
 *              This disables the specialized promiscuous mode required by Intel.
 *              See MgntEnableIntelPromiscuousMode for detail.
 */
void rtllib_DisableIntelPromiscuousMode(struct net_device* dev,
                bool bInitState)
{
        bool bFilterOutNonAssociatedBSSID = true;

	struct rtllib_device* ieee = netdev_priv_rsl(dev);

        printk("========>Exit Intel Promiscuous Mode\n");

        ieee->AllowAllDestAddrHandler(dev, false, !bInitState);
        ieee->SetHwRegHandler(dev, HW_VAR_CECHK_BSSID, (u8*)&bFilterOutNonAssociatedBSSID);

        ieee->bNetPromiscuousMode = false;
}

void rtllib_send_probe(struct rtllib_device *ieee, u8 is_mesh)
{
	struct sk_buff *skb;
#ifdef _RTL8192_EXT_PATCH_
	if(is_mesh) 
		skb = rtllib_probe_req_with_SSID(ieee, NULL, 0);
	else
#endif
	skb = rtllib_probe_req(ieee);
	if (skb){
#ifdef ASL
		softmac_mgmt_xmit(skb, ieee, false);
#else
		softmac_mgmt_xmit(skb, ieee);
#endif
		ieee->softmac_stats.tx_probe_rq++;
	}
}


void rtllib_send_probe_requests(struct rtllib_device *ieee, u8 is_mesh)
{
#ifdef _RTL8192_EXT_PATCH_
	if (ieee->active_scan && (ieee->softmac_features & IEEE_SOFTMAC_PROBERQ)){
		rtllib_send_probe(ieee,is_mesh);
		rtllib_send_probe(ieee,is_mesh);
	}
#else
	if (ieee->active_scan && (ieee->softmac_features & IEEE_SOFTMAC_PROBERQ)){
		rtllib_send_probe(ieee, 0);
		rtllib_send_probe(ieee, 0);
}
#endif
}

void rtllib_softmac_hint11d_wq(void *data)
{
#if defined CONFIG_CRDA && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30) 
#ifdef MERGE_TODO
	struct rtllib_device *ieee = container_of_dwork_rsl(data, struct rtllib_device, softmac_hint11d_wq);
	PRT_DOT11D_INFO pDot11dInfo = GET_DOT11D_INFO(ieee);
	struct wireless_dev *wdev = &ieee->wdev;

   	regulatory_hint_11d(wdev->wiphy, pDot11dInfo->CountryIeBuf, pDot11dInfo->CountryIeLen);
#endif
#endif
}

void rtllib_update_active_chan_map(struct rtllib_device *ieee)
{
#ifdef ENABLE_DOT11D
	memcpy(ieee->active_channel_map, GET_DOT11D_INFO(ieee)->channel_map, MAX_CHANNEL_NUMBER+1);
#else
	memcpy(ieee->active_channel_map, ieee->channel_map, MAX_CHANNEL_NUMBER+1);
#endif
#if 0
	{
		int i;
		for(i=1;i<=14;i++)
			printk("%d ", ieee->active_channel_map[i]);
		printk("\n");
	}
#endif
}

/* this performs syncro scan blocking the caller until all channels
 * in the allowed channel map has been checked. 
 */
void rtllib_softmac_scan_syncro(struct rtllib_device *ieee, u8 is_mesh)
{
	short ch = 0;

	rtllib_update_active_chan_map(ieee);

	ieee->be_scan_inprogress = true;

	down(&ieee->scan_sem);
	
	while(1)
	{
		
		do {
			ch++;
			if (ch > MAX_CHANNEL_NUMBER) 
				goto out; /* scan completed */
		} while(!ieee->active_channel_map[ch]);
	
		/* this fuction can be called in two situations
		 * 1- We have switched to ad-hoc mode and we are
		 *    performing a complete syncro scan before conclude
		 *    there are no interesting cell and to create a 
		 *    new one. In this case the link state is 
		 *    RTLLIB_NOLINK until we found an interesting cell.
		 *    If so the ieee8021_new_net, called by the RX path
		 *    will set the state to RTLLIB_LINKED, so we stop
		 *    scanning
		 * 2- We are linked and the root uses run iwlist scan.
		 *    So we switch to RTLLIB_LINKED_SCANNING to remember
		 *    that we are still logically linked (not interested in
		 *    new network events, despite for updating the net list,
		 *    but we are temporarly 'unlinked' as the driver shall
		 *    not filter RX frames and the channel is changing.
		 * So the only situation in witch are interested is to check
		 * if the state become LINKED because of the #1 situation
		 */    
		    
		if (ieee->state == RTLLIB_LINKED)
			goto out;
		if (ieee->sync_scan_hurryup){
			printk("============>sync_scan_hurryup out\n");
			goto out;
		}

		ieee->set_chan(ieee->dev, ch);
		if(ieee->active_channel_map[ch] == 1)
#ifdef _RTL8192_EXT_PATCH_
		rtllib_send_probe_requests(ieee, is_mesh);
#else
		rtllib_send_probe_requests(ieee, 0);
#endif
		
		/* this prevent excessive time wait when we
		 * need to wait for a syncro scan to end..
		 */  		
		msleep_interruptible_rsl(RTLLIB_SOFTMAC_SCAN_TIME);
	}
out:
	ieee->actscanning = false;
	ieee->sync_scan_hurryup = 0;

	if(ieee->state >= RTLLIB_LINKED){
#ifdef ENABLE_DOT11D
		if(IS_DOT11D_ENABLE(ieee))
			DOT11D_ScanComplete(ieee);
#endif	
	}
	up(&ieee->scan_sem);
	
	ieee->be_scan_inprogress = false;

#ifndef FOR_MOBLIN
	{
	union iwreq_data wrqu;
	memset(&wrqu, 0, sizeof(wrqu));
	wireless_send_event(ieee->dev,SIOCGIWSCAN,&wrqu,NULL);
	}
#endif
}

void rtllib_softmac_scan_wq(void *data)
{
	struct rtllib_device *ieee = container_of_dwork_rsl(data, struct rtllib_device, softmac_scan_wq);

#ifndef _RTL8192_EXT_PATCH_
	u8 last_channel = ieee->current_network.channel; 
#endif
	rtllib_update_active_chan_map(ieee);

	if(!ieee->ieee_up)
		return;	
	if(rtllib_act_scanning(ieee,true) == true)
		return;

	down(&ieee->scan_sem);

	if(ieee->eRFPowerState == eRfOff)
	{
		printk("======>%s():rf state is eRfOff, return\n",__FUNCTION__);
		goto out1;
	}

	do{
		ieee->current_network.channel = 
			(ieee->current_network.channel + 1) % MAX_CHANNEL_NUMBER;
		if (ieee->scan_watch_dog++ > MAX_CHANNEL_NUMBER) 
		{
			if (!ieee->active_channel_map[ieee->current_network.channel])
				ieee->current_network.channel = 6;
			goto out; /* no good chans */
		}
	} while(!ieee->active_channel_map[ieee->current_network.channel]);

	if (ieee->scanning_continue == 0 )
		goto out;

	ieee->set_chan(ieee->dev, ieee->current_network.channel);

	if(ieee->active_channel_map[ieee->current_network.channel] == 1)
#ifdef _RTL8192_EXT_PATCH_
	rtllib_send_probe_requests(ieee, 0);
#else
	rtllib_send_probe_requests(ieee, 0);
#endif				

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,40)
	queue_delayed_work_rsl(ieee->wq, &ieee->softmac_scan_wq, MSECS(RTLLIB_SOFTMAC_SCAN_TIME));
#else
	if (ieee->scanning_continue == 1) 
		mod_timer(&ieee->scan_timer,(jiffies + MSECS(RTLLIB_SOFTMAC_SCAN_TIME)));
#endif

	up(&ieee->scan_sem);
	return;
	
out:
#ifdef ENABLE_DOT11D
	if(IS_DOT11D_ENABLE(ieee))
		DOT11D_ScanComplete(ieee);
#endif	
#ifdef _RTL8192_EXT_PATCH_
	ieee->current_network.channel = ieee->backup_channel;
	ieee->set_chan(ieee->dev, ieee->current_network.channel);
#else
	ieee->current_network.channel = last_channel;
#endif

out1:
	ieee->actscanning = false;
	ieee->scan_watch_dog = 0;
	ieee->scanning_continue = 0;
	up(&ieee->scan_sem);
}



void rtllib_beacons_start(struct rtllib_device *ieee)
{
	unsigned long flags;	
	spin_lock_irqsave(&ieee->beacon_lock,flags);

	ieee->beacon_txing = 1;
	rtllib_send_beacon(ieee);
	
	spin_unlock_irqrestore(&ieee->beacon_lock,flags);
}

void rtllib_beacons_stop(struct rtllib_device *ieee)
{
	unsigned long flags;

	spin_lock_irqsave(&ieee->beacon_lock,flags);

	ieee->beacon_txing = 0;
 	del_timer_sync(&ieee->beacon_timer);

	spin_unlock_irqrestore(&ieee->beacon_lock,flags);

}


void rtllib_stop_send_beacons(struct rtllib_device *ieee)
{
	if(ieee->stop_send_beacons)
		ieee->stop_send_beacons(ieee->dev);
	if (ieee->softmac_features & IEEE_SOFTMAC_BEACONS)
		rtllib_beacons_stop(ieee);
}


void rtllib_start_send_beacons(struct rtllib_device *ieee)
{
	if(ieee->start_send_beacons)
		ieee->start_send_beacons(ieee->dev);
	if(ieee->softmac_features & IEEE_SOFTMAC_BEACONS)
		rtllib_beacons_start(ieee); 
}


void rtllib_softmac_stop_scan(struct rtllib_device *ieee)
{
	
	
	down(&ieee->scan_sem);
	ieee->scan_watch_dog = 0;
	if (ieee->scanning_continue == 1){
		ieee->scanning_continue = 0;
		ieee->actscanning = 0;  

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,40)
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,67)
		cancel_delayed_work(&ieee->softmac_scan_wq);
#endif
#else
		del_timer_sync(&ieee->scan_timer);
#endif
	}
	
	up(&ieee->scan_sem);
}

void rtllib_stop_scan(struct rtllib_device *ieee)
{
	if (ieee->softmac_features & IEEE_SOFTMAC_SCAN){
		rtllib_softmac_stop_scan(ieee);
	}else{
		if(ieee->rtllib_stop_hw_scan)
			ieee->rtllib_stop_hw_scan(ieee->dev);
	}
}

void rtllib_stop_scan_syncro(struct rtllib_device *ieee)
{
	if (ieee->softmac_features & IEEE_SOFTMAC_SCAN){
			ieee->sync_scan_hurryup = 1;
	}else{
		if(ieee->rtllib_stop_hw_scan)
			ieee->rtllib_stop_hw_scan(ieee->dev);
	}
}

bool rtllib_act_scanning(struct rtllib_device *ieee, bool sync_scan)
{
	if (ieee->softmac_features & IEEE_SOFTMAC_SCAN){
		if(sync_scan){ 
			return ieee->be_scan_inprogress;
		}else{
			return (ieee->actscanning ||ieee->be_scan_inprogress);
		}
	}else{
		return test_bit(STATUS_SCANNING, &ieee->status);
	}
}

/* called with ieee->lock held */
void rtllib_start_scan(struct rtllib_device *ieee)
{
	if(ieee->RfOffReason > RF_CHANGE_BY_PS || ieee->RfOffReason == RF_CHANGE_BY_INIT)
		return;
	
	printk("===>%s()\n",__FUNCTION__);
#ifdef ENABLE_IPS
	if(ieee->rtllib_ips_leave_wq != NULL)
	ieee->rtllib_ips_leave_wq(ieee->dev);
#endif


#ifdef ENABLE_DOT11D
	if(IS_DOT11D_ENABLE(ieee) )
	{			
		if(IS_COUNTRY_IE_VALID(ieee))
		{
			RESET_CIE_WATCHDOG(ieee); 
		}
	}
#endif
	if (ieee->softmac_features & IEEE_SOFTMAC_SCAN){	
		if (ieee->scanning_continue == 0){
			ieee->actscanning = true;
			ieee->scanning_continue = 1;
#ifdef _RTL8192_EXT_PATCH_
			ieee->backup_channel = ieee->current_network.channel;
			printk("===>backup_channel is %d\n",ieee->backup_channel);
#endif
			queue_delayed_work_rsl(ieee->wq, &ieee->softmac_scan_wq, 0);
		}
	}else{
		if(ieee->rtllib_start_hw_scan)
			ieee->rtllib_start_hw_scan(ieee->dev);
	}
	
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,40)	
void rtllib_softmac_scan_cb(unsigned long _dev)
{
	unsigned long flags;
	struct rtllib_device *ieee = (struct rtllib_device *)_dev;
	
	spin_lock_irqsave(&ieee->lock, flags);
	rtllib_start_scan(ieee);
	spin_unlock_irqrestore(&ieee->lock, flags);
}
#endif

/* called with wx_sem held */
void rtllib_start_scan_syncro(struct rtllib_device *ieee, u8 is_mesh)
{
#ifdef ENABLE_DOT11D
	if(IS_DOT11D_ENABLE(ieee) )
	{			
		if(IS_COUNTRY_IE_VALID(ieee))
		{
			RESET_CIE_WATCHDOG(ieee); 
		}
	}
#endif
	ieee->sync_scan_hurryup = 0;
	if (ieee->softmac_features & IEEE_SOFTMAC_SCAN){
		rtllib_softmac_scan_syncro(ieee, is_mesh);
	}else{
		if(ieee->rtllib_start_hw_scan)
			ieee->rtllib_start_hw_scan(ieee->dev);
	}
		
}

inline struct sk_buff *rtllib_authentication_req(struct rtllib_network *beacon, 
	struct rtllib_device *ieee, int challengelen,u8 * daddr)
{
	struct sk_buff *skb;	
	struct rtllib_authentication *auth;
	int  len = 0;
#ifdef _RTL8192_EXT_PATCH_
	bool is_mesh = false;
	is_mesh = ieee->ext_patch_rtllib_is_mesh(ieee,daddr);
#endif
#ifdef _RTL8192_EXT_PATCH_
	if((ieee->mesh_sec_type == 1) && (ieee->mesh_security_setting == 3) && is_mesh){
		skb = ieee->ext_patch_rtllib_send_ath_commit(ieee,daddr);
		return skb;
	}
#endif
	len = sizeof(struct rtllib_authentication) + challengelen + ieee->tx_headroom + 4;
#ifdef USB_USE_ALIGNMENT
        u32 Tmpaddr;
        int alignment;
        skb = dev_alloc_skb(len + USB_512B_ALIGNMENT_SIZE);
#else
	skb = dev_alloc_skb(len); 
#endif

	if (!skb) return NULL;
	
#ifdef USB_USE_ALIGNMENT
        Tmpaddr = (u32)skb->data;
        alignment = Tmpaddr & 0x1ff;
        skb_reserve(skb,(USB_512B_ALIGNMENT_SIZE - alignment));
#endif
	
	skb_reserve(skb, ieee->tx_headroom);
	
	auth = (struct rtllib_authentication *)
		skb_put(skb, sizeof(struct rtllib_authentication));
	
	auth->header.frame_ctl = RTLLIB_STYPE_AUTH;
	if (challengelen) auth->header.frame_ctl |= RTLLIB_FCTL_WEP;
	
	auth->header.duration_id = 0x013a; 
#ifdef _RTL8192_EXT_PATCH_
	if(is_mesh)
		memcpy(auth->header.addr1,daddr,ETH_ALEN);
	else
#endif
		memcpy(auth->header.addr1, beacon->bssid, ETH_ALEN);
	memcpy(auth->header.addr2, ieee->dev->dev_addr, ETH_ALEN);
#ifdef _RTL8192_EXT_PATCH_
	if(is_mesh)
		memcpy(auth->header.addr3, ieee->current_mesh_network.bssid, ETH_ALEN); 
	else
		memcpy(auth->header.addr3, beacon->bssid, ETH_ALEN);
#else
	memcpy(auth->header.addr3, beacon->bssid, ETH_ALEN);
#endif	
	if(ieee->auth_mode == 0)
		auth->algorithm = WLAN_AUTH_OPEN;
	else if(ieee->auth_mode == 1)
		auth->algorithm = WLAN_AUTH_SHARED_KEY;
	else if(ieee->auth_mode == 2)
		auth->algorithm = WLAN_AUTH_OPEN;
	auth->transaction = cpu_to_le16(ieee->associate_seq);
	ieee->associate_seq++;
	
	auth->status = cpu_to_le16(RTLLIB_WLAN_STATUS_SUCCESS);
	
	return skb;
	
}

void constructWMMIE(u8* wmmie, u8* wmm_len,u8 oui_subtype)
{
	u8	szQoSOUI[] ={221, 0, 0x00, 0x50, 0xf2, 0x02, 0, 1};
	
	if (oui_subtype == OUI_SUBTYPE_QOS_CAPABI)
	{
		szQoSOUI[0] = 46;
		szQoSOUI[1] = *wmm_len;
		memcpy(wmmie,szQoSOUI,3);
		*wmm_len = 3;
	}	
	else
	{
		szQoSOUI[1] = *wmm_len + 6;
		szQoSOUI[6] = oui_subtype;
		memcpy(wmmie, szQoSOUI, 8);
		*(wmmie+8) = 0;
		*wmm_len = 9;
	}	
}

static struct sk_buff* rtllib_probe_resp(struct rtllib_device *ieee, u8 *dest, bool is_beacon)
{
	u8 *tag;
	int beacon_size;
	struct rtllib_probe_response *beacon_buf;
	struct sk_buff *skb = NULL;
	int encrypt;
	int atim_len=0,erp_len=0;
	struct rtllib_crypt_data* crypt;
	struct rtllib_network* net = NULL;
	int mode = 0;
	char *ssid = NULL;
	int ssid_len = 0;
	int rate_len = 0;
	int rate_ex_len = 0;
	int wpa_ie_len = 0;
	u8 erpinfo_content = 0;

	u8* tmp_ht_cap_buf=NULL;
	u8 tmp_ht_cap_len=0;
	u8* tmp_ht_info_buf=NULL;
	u8 tmp_ht_info_len=0;
	PRT_HIGH_THROUGHPUT	pHTInfo = NULL;
	u8* tmp_generic_ie_buf=NULL;
	u8 tmp_generic_ie_len=0;
#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE)
	u8 wmmie[9] = {0};
	u8 wmm_len = 0;
#endif
#ifdef RTL8192S_WAPI_SUPPORT
	PRT_WAPI_T pWapiInfo = &(ieee->wapiInfo);
	u8 pWapiIELen = 0;
#endif
#ifdef ASL
	int rsn_len = 0;
	int tim_len = 0;
#ifdef ASL_WME
	int wme_len=0 ; 
#endif
#ifdef CONFIG_WPS
	int wps_ie_len=0;
#endif
#endif	
#ifdef ASL
	if (ieee->iw_mode == IW_MODE_MASTER || ieee->iw_mode == IW_MODE_APSTA) {
		pHTInfo = ieee->pAPHTInfo;
		net = &ieee->current_ap_network;
		mode = ieee->apmode;
		wpa_ie_len = 0;
	} else
#endif
	{
		pHTInfo = ieee->pHTInfo;
		net = &ieee->current_network;
		mode = ieee->mode;
		wpa_ie_len = ieee->wpa_ie_len;
	}
	ssid =net->ssid;
	ssid_len = net->ssid_len;
	rate_len = net->rates_len+2;
	rate_ex_len = net->rates_ex_len;
	
	if (mode == WIRELESS_MODE_B)
		rate_ex_len = 0;
	if(rate_ex_len > 0) rate_ex_len+=2;
	
	if(net->capability & WLAN_CAPABILITY_IBSS)
		atim_len = 4;
	else
		atim_len = 0;
	
#ifdef ASL
	if ((ieee->iw_mode == IW_MODE_MASTER) || (ieee->iw_mode == IW_MODE_APSTA)) {
		if(ieee->mDtimCount == 0)
			ieee->mDtimCount = ieee->mDtimPeriod - 1;
		else 
			ieee->mDtimCount--;

		tim_len = ap_ps_fill_tim(ieee);
		tim_len = tim_len+2;
	} else
		tim_len = 0;

	if (((ieee->iw_mode == IW_MODE_MASTER) || (ieee->iw_mode == IW_MODE_APSTA)) 
		&& (ieee->bhidden_ssid == 1) && is_beacon) {
		ssid = NULL;
		ssid_len = 0;
	} else {
	}
	if (ieee->rsn_activated) {
#ifdef ASL_DEBUG_2
		printk("\nRSN Activated. RSN len : %d",ieee->rsn_ie_len);
#endif
		rsn_len = ieee->rsn_ie_len;
	} else {
#ifdef ASL_DEBUG_2
		printk("\nRSN NOT Activated.");
#endif
		rsn_len = 0;
	}

#ifdef ASL_WME
	if(ieee->wme_enabled)
		wme_len = sizeof(struct wme_parameter_element)+2;
	else
		wme_len = 0;
#endif
#endif
#if 0
	if(rtllib_is_54g(&ieee->current_network)) 
		erp_len = 3;
	else
		erp_len = 0;
#else
#ifdef ASL
	if ((ieee->iw_mode == IW_MODE_ADHOC)) {
      if((ieee->current_network.mode == IEEE_G) 
		  	||( ieee->current_network.mode == IEEE_N_24G && pHTInfo->bCurSuppCCK)) {
	  	erp_len = 3;
		erpinfo_content = 0;
		if(ieee->current_network.buseprotection)
			erpinfo_content |= ERP_UseProtection;
      	}
	else
		erp_len = 0;
	} else if (ieee->iw_mode == IW_MODE_APSTA || (ieee->iw_mode == IW_MODE_MASTER)) {
		if((ieee->apmode == IEEE_G) 
		  	||( ieee->apmode == IEEE_N_24G && pHTInfo->bCurSuppCCK)) {
		  	erp_len = 3;
			erpinfo_content = 0;
			if(ieee->current_ap_network.buseprotection)
				erpinfo_content |= ERP_UseProtection;
	      	}
		else
			erp_len = 0;
	}
#else
      if((ieee->current_network.mode == IEEE_G) 
	  	||( ieee->current_network.mode == IEEE_N_24G && pHTInfo->bCurSuppCCK)) {
	  	erp_len = 3;
		erpinfo_content = 0;
		if(ieee->current_network.buseprotection)
			erpinfo_content |= ERP_UseProtection;
      	}
	else
		erp_len = 0;
#endif
#endif

#ifdef ASL
	if (ieee->iw_mode == IW_MODE_MASTER || ieee->iw_mode == IW_MODE_APSTA) {
		crypt = ieee->stations_crypt[0]->crypt[ieee->tx_keyidx];
	}else
#endif
#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
	crypt = ieee->sta_crypt[ieee->tx_keyidx];
#else
	crypt = ieee->crypt[ieee->tx_keyidx];
#endif	
#ifdef ASL
#ifdef CONFIG_WPS
	if (is_beacon)
		wps_ie_len = ieee->beacon_wps_ie_len;
	else
		wps_ie_len = ieee->probe_rsp_wps_ie_len;
#endif
#endif
#ifdef ASL
	if (ieee->iw_mode == IW_MODE_MASTER || ieee->iw_mode == IW_MODE_APSTA) {
		encrypt = ieee->host_encrypt && crypt && crypt->ops && 
			((0 == strcmp(crypt->ops->name, "WEP")) || (0 == strcmp(crypt->ops->name, "TKIP")) || (0 == strcmp(crypt->ops->name, "CCMP")));
		encrypt = encrypt || ieee->rsn_activated || wpa_ie_len;
	} else 
#endif
	encrypt = ieee->host_encrypt && crypt && crypt->ops && 
		((0 == strcmp(crypt->ops->name, "WEP") || wpa_ie_len));

#if 1	
	if(pHTInfo->bCurrentHTSupport){
		tmp_ht_cap_buf =(u8*) &(pHTInfo->SelfHTCap);
		tmp_ht_cap_len = sizeof(pHTInfo->SelfHTCap);
		tmp_ht_info_buf =(u8*) &(pHTInfo->SelfHTInfo);
		tmp_ht_info_len = sizeof(pHTInfo->SelfHTInfo);
#ifdef _RTL8192_EXT_PATCH_
		HTConstructCapabilityElement(ieee, tmp_ht_cap_buf, &tmp_ht_cap_len,encrypt, 1, false);
#else
#ifdef ASL
		if (ieee->iw_mode == IW_MODE_MASTER || ieee->iw_mode == IW_MODE_APSTA)
			HTConstructCapabilityElement(ieee, tmp_ht_cap_buf, &tmp_ht_cap_len,encrypt, 1, false, true);
		else
			HTConstructCapabilityElement(ieee, tmp_ht_cap_buf, &tmp_ht_cap_len,encrypt, 1, false, false);	
#else
		HTConstructCapabilityElement(ieee, tmp_ht_cap_buf, &tmp_ht_cap_len,encrypt, false);
#endif	
#endif	
		HTConstructInfoElement(ieee,tmp_ht_info_buf,&tmp_ht_info_len, encrypt);
		

		if(pHTInfo->bRegRT2RTAggregation)
		{
			tmp_generic_ie_buf = pHTInfo->szRT2RTAggBuffer;
			tmp_generic_ie_len = sizeof(pHTInfo->szRT2RTAggBuffer);
			HTConstructRT2RTAggElement(ieee, tmp_generic_ie_buf, &tmp_generic_ie_len);
		}
	}
#endif

#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE)
	if(ieee->iw_mode == IW_MODE_ADHOC){
		if(ieee->current_network.qos_data.supported){
			wmm_len = 1;
			constructWMMIE(wmmie,&wmm_len,OUI_SUBTYPE_WMM_INFO);
		}else{
			wmm_len = 0;
		}
	}
#endif
#ifdef RTL8192S_WAPI_SUPPORT
	if(ieee->WapiSupport && pWapiInfo->bWapiEnable)
	{
		WapiSetIE(ieee);
		pWapiIELen = pWapiInfo->wapiIELength;
	}
#endif
	beacon_size = sizeof(struct rtllib_probe_response)+2+
		ssid_len
		+3 
		+rate_len
		+rate_ex_len
		+atim_len
		+erp_len
		+wpa_ie_len
#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE)
		+tmp_ht_cap_len
		+tmp_ht_info_len
		+tmp_generic_ie_len
		+wmm_len
#endif
#ifdef ASL
		+tim_len
		+rsn_len
#ifdef ASL_WME
		+wme_len
#endif
#ifdef CONFIG_WPS
		+wps_ie_len
#endif
#endif
#ifdef RTL8192S_WAPI_SUPPORT
		+pWapiIELen+2
#endif
		+ieee->tx_headroom;
#ifdef USB_USE_ALIGNMENT        
        u32 Tmpaddr=0;
        int alignment=0;
        skb = dev_alloc_skb(beacon_size + USB_512B_ALIGNMENT_SIZE);
#else
	skb = dev_alloc_skb(beacon_size);
#endif
	if (!skb) 
		return NULL;

#ifdef USB_USE_ALIGNMENT
        Tmpaddr = (u32)skb->data;
        alignment = Tmpaddr & 0x1ff;
        skb_reserve(skb,(USB_512B_ALIGNMENT_SIZE - alignment));
#endif
	
	skb_reserve(skb, ieee->tx_headroom);
	
	beacon_buf = (struct rtllib_probe_response*) skb_put(skb, (beacon_size - ieee->tx_headroom));
	memcpy (beacon_buf->header.addr1, dest,ETH_ALEN);
	memcpy (beacon_buf->header.addr2, ieee->dev->dev_addr, ETH_ALEN);
	memcpy (beacon_buf->header.addr3, net->bssid, ETH_ALEN);

	beacon_buf->header.duration_id = 0; 
	beacon_buf->beacon_interval = 
		cpu_to_le16(net->beacon_interval);
#ifdef ASL
	if (ieee->iw_mode == IW_MODE_MASTER || ieee->iw_mode == IW_MODE_APSTA)
#else
	if (ieee->iw_mode == IW_MODE_MASTER)
#endif
		beacon_buf->capability = 
			cpu_to_le16(net->capability & WLAN_CAPABILITY_ESS);
	else
	beacon_buf->capability = 
			cpu_to_le16(net->capability & WLAN_CAPABILITY_IBSS);
	beacon_buf->capability |= 
		cpu_to_le16(net->capability & WLAN_CAPABILITY_SHORT_PREAMBLE); 

	if(ieee->short_slot && (net->capability & WLAN_CAPABILITY_SHORT_SLOT_TIME))
		cpu_to_le16((beacon_buf->capability |= WLAN_CAPABILITY_SHORT_SLOT_TIME));		
	
	if (encrypt)	
		beacon_buf->capability |= cpu_to_le16(WLAN_CAPABILITY_PRIVACY);
	
		
	beacon_buf->header.frame_ctl = cpu_to_le16(RTLLIB_STYPE_PROBE_RESP);
	beacon_buf->info_element[0].id = MFIE_TYPE_SSID;	
	beacon_buf->info_element[0].len = ssid_len;
	
	tag = (u8*) beacon_buf->info_element[0].data;
	
	memcpy(tag, ssid, ssid_len);
	
	tag += ssid_len;
	
	*(tag++) = MFIE_TYPE_RATES;
	*(tag++) = rate_len-2; 
	memcpy(tag, net->rates, rate_len-2);
	tag+=rate_len-2;
	
	*(tag++) = MFIE_TYPE_DS_SET;
	*(tag++) = 1;
	*(tag++) = net->channel;
	
	if(atim_len){
	u16 val16;
		*(tag++) = MFIE_TYPE_IBSS_SET;
		*(tag++) = 2;
		 val16 = cpu_to_le16(net->atim_window);
		memcpy((u8 *)tag, (u8 *)&val16, 2);
		tag+=2;
	}
#ifdef ASL
	if (tim_len) {
		*(tag++) = MFIE_TYPE_TIM;
		*(tag++) = tim_len-2;
		memcpy(tag, ieee->Tim.Octet, tim_len-2);
		tag+=tim_len-2;
	}
#endif
	
	if(erp_len){
		*(tag++) = MFIE_TYPE_ERP;
		*(tag++) = 1;
		*(tag++) = erpinfo_content; 
	}
#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE)
	if(tmp_ht_cap_len){
		*(tag++) = MFIE_TYPE_HT_CAP;
		*(tag++) = tmp_ht_cap_len - 2;
		memcpy(tag, tmp_ht_cap_buf, tmp_ht_cap_len - 2);	
		tag += tmp_ht_cap_len - 2;
	}
#endif
	if(rate_ex_len){
		*(tag++) = MFIE_TYPE_RATES_EX;
		*(tag++) = rate_ex_len-2; 
		memcpy(tag, net->rates_ex, rate_ex_len-2);
		tag+=rate_ex_len-2;
	}

#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE)
	if(tmp_ht_info_len){
		*(tag++) = MFIE_TYPE_HT_INFO;
		*(tag++) = tmp_ht_info_len - 2;
		memcpy(tag, tmp_ht_info_buf, tmp_ht_info_len -2);
		tag += tmp_ht_info_len - 2;
	}
#endif
	
	if (wpa_ie_len)
	{
		if (ieee->iw_mode == IW_MODE_ADHOC)
		{
			memcpy(&ieee->wpa_ie[14], &ieee->wpa_ie[8], 4);
		}
		memcpy(tag, ieee->wpa_ie, ieee->wpa_ie_len);
		tag += ieee->wpa_ie_len;
	}

#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE)
	if(tmp_generic_ie_len)
	{
		(*tag++) = 0xdd;
		(*tag++) = tmp_generic_ie_len - 2;
		memcpy(tag,tmp_generic_ie_buf,tmp_generic_ie_len -2);
		tag += tmp_generic_ie_len -2;
		
	}
#endif

#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE)
	if(wmm_len) {
		memcpy(tag,wmmie,wmm_len);
		tag += wmm_len;
	}
#endif
#ifdef ASL
	if(rsn_len) {
#ifdef ASL_DEBUG_2
		printk("\nRSN IE set in beacon/probe response!");
#endif
		memcpy(tag,ieee->rsn_ie,rsn_len);
		tag += rsn_len;
	}
#ifdef ASL_WME
	if (ieee->wme_enabled)
		tag = ap_eid_wme(ieee,tag);
#endif
#ifdef CONFIG_WPS
	if (wps_ie_len) {
		if (is_beacon)
			memcpy(tag, ieee->beacon_wps_ie, wps_ie_len); 
		else
			memcpy(tag, ieee->probe_rsp_wps_ie, wps_ie_len);
		tag += wps_ie_len;
	}
#endif
#endif

#ifdef RTL8192S_WAPI_SUPPORT
	if(pWapiIELen)
	{
		*(tag++) = MFIE_TYPE_WAPI;
		*(tag++) = pWapiIELen;
		memcpy(tag, pWapiInfo->wapiIE, pWapiIELen);
		tag += pWapiIELen;

		pWapiInfo->sendbeaconWapiIE[0] = MFIE_TYPE_WAPI;
		pWapiInfo->sendbeaconWapiIE[1] = pWapiIELen;
		pWapiInfo->sendbeaconWapiIELength = pWapiIELen+2;
		memcpy(pWapiInfo->sendbeaconWapiIE+2,  pWapiInfo->wapiIE, pWapiIELen);
	}
#endif
	return skb;
}

#ifdef RTL8192S_WAPI_SUPPORT
void ConstructWapiIEForInit(struct rtllib_device* ieee)
{
	PRT_WAPI_T pWapiInfo = &(ieee->wapiInfo);
	u8 pWapiIELen = 0;

	if(ieee->WapiSupport && pWapiInfo->bWapiEnable){
		WapiSetIE(ieee);
		pWapiIELen = pWapiInfo->wapiIELength;
		if(pWapiIELen){
			pWapiInfo->sendbeaconWapiIE[0] = MFIE_TYPE_WAPI;
			pWapiInfo->sendbeaconWapiIE[1] = pWapiIELen;
			pWapiInfo->sendbeaconWapiIELength = pWapiIELen+2;
			memcpy(pWapiInfo->sendbeaconWapiIE+2,  pWapiInfo->wapiIE, pWapiIELen);
		}
	}
}
#endif

#ifdef _RTL8192_EXT_PATCH_
#ifdef COMPATIBLE_WITH_RALINK_MESH
extern void ConstructRalinkHostNameIE(struct rtllib_device* ieee, 
		u8* p_hostname_ie, u8* hostname_ie_len);
#else
extern void ConstructHostNameIE(struct rtllib_device* ieee, 
		u8* hostname_ie, u8* hostname_ie_len);
#endif
struct sk_buff* rtllib_ext_probe_resp_by_net(struct rtllib_device *ieee, u8 *dest, struct rtllib_network *net)
{
	u8 *tag;
	int beacon_size;
	struct rtllib_probe_response *beacon_buf;
	struct sk_buff *skb;
	int encrypt;
	int atim_len = 0,erp_len = 0;
	struct rtllib_crypt_data* crypt;
	u8 broadcast_addr[] = {0xff,0xff,0xff,0xff,0xff,0xff};
	
	char *ssid = net->ssid;
	int ssid_len = net->ssid_len;

	int rate_len = ieee->current_mesh_network.rates_len+2;
	int rate_ex_len = ieee->current_mesh_network.rates_ex_len;
	int wpa_ie_len = 0;
	u8 erpinfo_content = 0;
#if 0
	u8* tmp_ht_cap_buf=NULL;
	u8 tmp_ht_cap_len=0;
	u8* tmp_ht_info_buf=NULL;
	u8 tmp_ht_info_len=0;
	PRT_HIGH_THROUGHPUT	pHTInfo = ieee->pHTInfo;
	u8* tmp_generic_ie_buf=NULL;
	u8 tmp_generic_ie_len=0;
#endif
	u8 wmmie[9] = {0};
	u8 wmm_len = 0;
	u8 hostname_ie_buf[MAX_HOST_NAME_LENGTH+4];
	u8 hostname_ie_len=0;
	if(rate_ex_len > 0) rate_ex_len+=2;
	if( ieee->meshScanMode&4){
		ieee->current_mesh_network.channel = ieee->ext_patch_rtllib_ext_stop_scan_wq_set_channel(ieee);
		if(ieee->current_mesh_network.channel == 0)
			ieee->current_mesh_network.channel = 1;
	}
	if( ieee->meshScanMode&6)
	{
		queue_work_rsl(ieee->wq, &ieee->ext_stop_scan_wq);
	}	
	if(ieee->current_mesh_network.capability & WLAN_CAPABILITY_IBSS) 
		atim_len = 4;
	else
		atim_len = 0;
	
	
	if((ieee->current_mesh_network.mode == IEEE_G) 
	  	||( ieee->mode == IEEE_N_24G && ieee->pHTInfo->bCurSuppCCK)){ 
		erp_len = 3;
		erpinfo_content = 0;
		if(ieee->bUseProtection)
			erpinfo_content |= ERP_UseProtection;
	}	
	else
		erp_len = 0;
	
	if ((IW_MODE_MESH==ieee->iw_mode))
	{
		wpa_ie_len = ieee->wpa_ie_len; 
	}
		
	crypt = ieee->cryptlist[0]->crypt[ieee->mesh_txkeyidx];
	
	encrypt = ieee->host_encrypt && crypt && crypt->ops && 
		((0 == strcmp(crypt->ops->name, "WEP")||wpa_ie_len));
#if 0
	if(ieee->pHTInfo->bCurrentHTSupport){
		tmp_ht_cap_buf =(u8*) &(ieee->pHTInfo->SelfHTCap);
		tmp_ht_cap_len = sizeof(ieee->pHTInfo->SelfHTCap);
		tmp_ht_info_buf =(u8*) &(ieee->pHTInfo->SelfHTInfo);
		tmp_ht_info_len = sizeof(ieee->pHTInfo->SelfHTInfo);
		HTConstructCapabilityElement(ieee, tmp_ht_cap_buf, &tmp_ht_cap_len,encrypt, 1);
		HTConstructInfoElement(ieee,tmp_ht_info_buf,&tmp_ht_info_len, encrypt);


		if(pHTInfo->bRegRT2RTAggregation)
		{
			tmp_generic_ie_buf = ieee->pHTInfo->szRT2RTAggBuffer;
			tmp_generic_ie_len = sizeof(ieee->pHTInfo->szRT2RTAggBuffer);
			HTConstructRT2RTAggElement(ieee, tmp_generic_ie_buf, &tmp_generic_ie_len);
		}
	}
#endif
	{
#ifdef COMPATIBLE_WITH_RALINK_MESH		
		ConstructRalinkHostNameIE(ieee, hostname_ie_buf, &hostname_ie_len);
#else
		ConstructHostNameIE(ieee, hostname_ie_buf, &hostname_ie_len);
#endif
	}
	if((ieee->iw_mode == IW_MODE_MESH) && (ieee->current_mesh_network.qos_data.supported == 1)){
		wmm_len = 1;
		constructWMMIE(wmmie,&wmm_len,OUI_SUBTYPE_WMM_INFO);
	}
	beacon_size = sizeof(struct rtllib_probe_response)+2+
		ssid_len
		+3 
		+rate_len
		+rate_ex_len
		+atim_len
		+erp_len
		+wpa_ie_len
#if 0
		+tmp_ht_cap_len
		+tmp_ht_info_len
		+tmp_generic_ie_len
#endif
		+hostname_ie_len
		+wmm_len
		+ieee->tx_headroom;
	skb = dev_alloc_skb(beacon_size+300); 
	
	if (!skb) 
 		return NULL;
	skb_reserve(skb, ieee->tx_headroom);
	beacon_buf = (struct rtllib_probe_response*) skb_put(skb, (beacon_size - ieee->tx_headroom));
	
	memcpy (beacon_buf->header.addr1, dest,ETH_ALEN);
	memcpy (beacon_buf->header.addr2, ieee->meshdev->dev_addr, ETH_ALEN);
	memcpy (beacon_buf->header.addr3, ieee->current_mesh_network.bssid, ETH_ALEN);

	beacon_buf->header.duration_id = 0; 
	
	beacon_buf->beacon_interval = 
		cpu_to_le16(ieee->current_mesh_network.beacon_interval);  
	beacon_buf->capability = 
		cpu_to_le16(ieee->current_mesh_network.capability & WLAN_CAPABILITY_IBSS);
	
	if(ieee->short_slot && (ieee->current_mesh_network.capability & WLAN_CAPABILITY_SHORT_SLOT_TIME))
		cpu_to_le16((beacon_buf->capability |= WLAN_CAPABILITY_SHORT_SLOT_TIME));		

	if (encrypt)	
		beacon_buf->capability |= cpu_to_le16(WLAN_CAPABILITY_PRIVACY);
	
		
	beacon_buf->header.frame_ctl = cpu_to_le16(RTLLIB_STYPE_PROBE_RESP);
	
	beacon_buf->info_element[0].id = MFIE_TYPE_SSID;	
	beacon_buf->info_element[0].len = ssid_len;
	
	tag = (u8*) beacon_buf->info_element[0].data;
	
	if(memcmp(dest, broadcast_addr, ETH_ALEN ))
		memcpy(tag, ssid, ssid_len);	
	else	
		ssid_len=0;
	
	tag += ssid_len;
	
		
	*(tag++) = MFIE_TYPE_RATES;
	*(tag++) = rate_len-2; 
	memcpy(tag,ieee->current_mesh_network.rates,rate_len-2);
	tag+=rate_len-2;
	
	*(tag++) = MFIE_TYPE_DS_SET;
	*(tag++) = 1;
	*(tag++) = ieee->current_mesh_network.channel;  
	
	
	if(atim_len){
		u16 val16;
		*(tag++) = MFIE_TYPE_IBSS_SET;
		*(tag++) = 2;
		 val16 = cpu_to_le16(ieee->current_mesh_network.atim_window);
		memcpy((u8 *)tag, (u8 *)&val16, 2);
		tag+=2;
	}
	
	if(erp_len){
		*(tag++) = MFIE_TYPE_ERP;
		*(tag++) = 1;
		*(tag++) = erpinfo_content; 
	}
#if 0	
	if(tmp_ht_cap_len){
	*(tag++) = MFIE_TYPE_HT_CAP;
	*(tag++) = tmp_ht_cap_len - 2;
	memcpy(tag, tmp_ht_cap_buf, tmp_ht_cap_len - 2);	
	tag += tmp_ht_cap_len - 2;
	}
#endif

	if(rate_ex_len){
		*(tag++) = MFIE_TYPE_RATES_EX;
		*(tag++) = rate_ex_len-2; 
		memcpy(tag,ieee->current_mesh_network.rates_ex,rate_ex_len-2);
		tag+=rate_ex_len-2;
	}

#if 0 
	if(tmp_ht_info_len){
	*(tag++) = MFIE_TYPE_HT_INFO;
	*(tag++) = tmp_ht_info_len - 2;
	memcpy(tag, tmp_ht_info_buf, tmp_ht_info_len -2);
	tag += tmp_ht_info_len - 2;
	}
#endif
	if (wpa_ie_len) {
		memcpy(tag, ieee->wpa_ie, ieee->wpa_ie_len);
		tag+=ieee->wpa_ie_len;
	}

#if 0	
	if(tmp_generic_ie_len)
	{
		(*tag++) = 0xdd;
		(*tag++) = tmp_generic_ie_len - 2;
		memcpy(tag,tmp_generic_ie_buf,tmp_generic_ie_len -2);
		tag += tmp_generic_ie_len -2;
		
	}
#endif	
	if(hostname_ie_len)
	{
		(*tag++) = 0xdd;
		(*tag++) = hostname_ie_len - 2;
		memcpy(tag, hostname_ie_buf, hostname_ie_len - 2);
		tag += hostname_ie_len -2;
	}

	if(wmm_len)
	{
		memcpy(tag,wmmie,wmm_len);
	}

	skb->dev = ieee->dev;
	return skb;
}
#endif

struct sk_buff* rtllib_assoc_resp(struct rtllib_device *ieee, u8 *dest)
{
	struct sk_buff *skb;
	u8* tag;
	
	struct rtllib_crypt_data* crypt;
	struct rtllib_assoc_response_frame *assoc;
	short encrypt;
	
	unsigned int rate_len = rtllib_MFIE_rate_len(ieee);
	int len = sizeof(struct rtllib_assoc_response_frame) + rate_len + ieee->tx_headroom;
	
#ifdef USB_USE_ALIGNMENT
        u32 Tmpaddr=0;
        int alignment=0;
        skb = dev_alloc_skb(len + USB_512B_ALIGNMENT_SIZE);
#else	
	skb = dev_alloc_skb(len); 	
#endif
	
	if (!skb) 
		return NULL;
	
#ifdef USB_USE_ALIGNMENT
        Tmpaddr = (u32)skb->data;
        alignment = Tmpaddr & 0x1ff;
        skb_reserve(skb,(USB_512B_ALIGNMENT_SIZE - alignment));
#endif
	
	skb_reserve(skb, ieee->tx_headroom);
	
	assoc = (struct rtllib_assoc_response_frame *)
		skb_put(skb,sizeof(struct rtllib_assoc_response_frame));
	
	assoc->header.frame_ctl = cpu_to_le16(RTLLIB_STYPE_ASSOC_RESP);
	memcpy(assoc->header.addr1, dest,ETH_ALEN);
	memcpy(assoc->header.addr3, ieee->dev->dev_addr, ETH_ALEN);
	memcpy(assoc->header.addr2, ieee->dev->dev_addr, ETH_ALEN);
#ifdef ASL
	assoc->capability = cpu_to_le16((ieee->iw_mode == IW_MODE_MASTER || ieee->iw_mode == IW_MODE_APSTA) ? 
		WLAN_CAPABILITY_ESS : WLAN_CAPABILITY_IBSS);
#else	
	assoc->capability = cpu_to_le16(ieee->iw_mode == IW_MODE_MASTER ? 
		WLAN_CAPABILITY_ESS : WLAN_CAPABILITY_IBSS);
#endif		
	if(ieee->short_slot)
		assoc->capability |= cpu_to_le16(WLAN_CAPABILITY_SHORT_SLOT_TIME);
		
	if (ieee->host_encrypt){
#ifdef ASL
		if (ieee->iw_mode == IW_MODE_MASTER || ieee->iw_mode == IW_MODE_APSTA){ 
			crypt = ieee->stations_crypt[0]->crypt[ieee->tx_keyidx];
		}
		else
#endif
#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
		crypt = ieee->sta_crypt[ieee->tx_keyidx];
#else
		crypt = ieee->crypt[ieee->tx_keyidx];
#endif

	}		
	else crypt = NULL;
	
	encrypt = ( crypt && crypt->ops);
	   
	if (encrypt)
		assoc->capability |= cpu_to_le16(WLAN_CAPABILITY_PRIVACY);
	
	assoc->status = 0;
	assoc->aid = cpu_to_le16(ieee->assoc_id);
	if (ieee->assoc_id == 0x2007) 
		ieee->assoc_id=0;
	else 
		ieee->assoc_id++;
	
	tag = (u8*) skb_put(skb, rate_len);
	rtllib_MFIE_Brate(ieee, &tag);
	rtllib_MFIE_Grate(ieee, &tag);
	
	return skb;
}

struct sk_buff* rtllib_auth_resp(struct rtllib_device *ieee,int status, u8 *dest)
{
	struct sk_buff *skb = NULL;
	struct rtllib_authentication *auth;
	int len = ieee->tx_headroom + sizeof(struct rtllib_authentication)+1;
#ifdef _RTL8192_EXT_PATCH_
	bool is_mesh = ieee->ext_patch_rtllib_is_mesh(ieee,dest);
	if(is_mesh && (ieee->mesh_sec_type == 1) && (ieee->mesh_security_setting == 3))
		skb = ieee->ext_patch_rtllib_send_ath_confirm(ieee,dest);
	return skb;
#endif
#ifdef USB_USE_ALIGNMENT
        u32 Tmpaddr=0;
        int alignment=0;
        skb = dev_alloc_skb(len + USB_512B_ALIGNMENT_SIZE);
#else	
	skb = dev_alloc_skb(len); 	
#endif
	if (!skb) 
		return NULL;
	
	skb->len = sizeof(struct rtllib_authentication);
	
#ifdef USB_USE_ALIGNMENT
        Tmpaddr = (u32)skb->data;
        alignment = Tmpaddr & 0x1ff;
        skb_reserve(skb,(USB_512B_ALIGNMENT_SIZE - alignment));
#endif
	
	skb_reserve(skb, ieee->tx_headroom);

	auth = (struct rtllib_authentication *)
		skb_put(skb, sizeof(struct rtllib_authentication));
	
	auth->status = cpu_to_le16(status);
	auth->transaction = cpu_to_le16(2);
	auth->algorithm = cpu_to_le16(WLAN_AUTH_OPEN);
	
#ifdef _RTL8192_EXT_PATCH_
	if(is_mesh)
		memset(auth->header.addr3, 0, ETH_ALEN);
	else
		memcpy(auth->header.addr3, ieee->dev->dev_addr, ETH_ALEN);
#else
	memcpy(auth->header.addr3, ieee->dev->dev_addr, ETH_ALEN);
#endif
	memcpy(auth->header.addr2, ieee->dev->dev_addr, ETH_ALEN);
	memcpy(auth->header.addr1, dest, ETH_ALEN);
	auth->header.frame_ctl = cpu_to_le16(RTLLIB_STYPE_AUTH); 
	return skb;
	
	
}

struct sk_buff* rtllib_null_func(struct rtllib_device *ieee,u8* daddr, bool bqos, u8 TID, bool beosp, short pwr, bool is_ap)
{
	struct sk_buff *skb;
	struct rtllib_hdr_3addr* hdr;
	frameqos* pqos_ctl= NULL;
#ifdef USB_USE_ALIGNMENT
        u32 Tmpaddr=0;
        int alignment=0;
	
	skb = dev_alloc_skb(sizeof(struct rtllib_hdr_3addr) + ieee->tx_headroom  + (bqos?2:0) + USB_512B_ALIGNMENT_SIZE);
#else	
	skb = dev_alloc_skb(sizeof(struct rtllib_hdr_3addr) + (bqos?2:0) + ieee->tx_headroom); 
#endif	
	if (!skb) 
		return NULL;
	if (NULL == daddr) {
		printk("===>ERR!!!%s(), dest addr is NULL\n",__func__);
		dev_kfree_skb(skb);
		return NULL;
	}
#ifdef USB_USE_ALIGNMENT
        Tmpaddr = (u32)skb->data;
        alignment = Tmpaddr & 0x1ff;
        skb_reserve(skb,(USB_512B_ALIGNMENT_SIZE - alignment));
#endif	
	skb_reserve(skb, ieee->tx_headroom);

	hdr = (struct rtllib_hdr_3addr*)skb_put(skb,sizeof(struct rtllib_hdr_3addr));
	switch (ieee->iw_mode) {
		case IW_MODE_INFRA:
			hdr->frame_ctl = RTLLIB_FCTL_TODS;
	memcpy(hdr->addr1, ieee->current_network.bssid, ETH_ALEN);
	memcpy(hdr->addr2, ieee->dev->dev_addr, ETH_ALEN);
			memcpy(hdr->addr3, daddr, ETH_ALEN);
		break;
#ifdef ASL
		case IW_MODE_MASTER:
			hdr->frame_ctl = RTLLIB_FCTL_FROMDS;
			memcpy(hdr->addr1, daddr, ETH_ALEN);
			memcpy(hdr->addr2, ieee->current_ap_network.bssid, ETH_ALEN);
			memcpy(hdr->addr3, ieee->dev->dev_addr, ETH_ALEN);
		break;
		case IW_MODE_APSTA:
			if (is_ap) {
				hdr->frame_ctl = RTLLIB_FCTL_FROMDS;
				memcpy(hdr->addr1, daddr, ETH_ALEN);
				memcpy(hdr->addr2, ieee->current_ap_network.bssid, ETH_ALEN);
				memcpy(hdr->addr3, ieee->dev->dev_addr, ETH_ALEN);
			} else {
				hdr->frame_ctl = RTLLIB_FCTL_TODS;
				memcpy(hdr->addr1, ieee->current_network.bssid, ETH_ALEN);
				memcpy(hdr->addr2, ieee->dev->dev_addr, ETH_ALEN);
				memcpy(hdr->addr3, daddr, ETH_ALEN);
			}
		break;
#endif
		case IW_MODE_ADHOC:
		default:
			memcpy(hdr->addr1, daddr, ETH_ALEN);
			memcpy(hdr->addr2, ieee->dev->dev_addr, ETH_ALEN);
	memcpy(hdr->addr3, ieee->current_network.bssid, ETH_ALEN);
		break;
	}
	if (bqos) {
		pqos_ctl = (frameqos*)skb_put(skb, 2);
		memset(pqos_ctl, 0, 2);
		pqos_ctl->field.tid = TID;
		pqos_ctl->field.eosp = beosp;
		hdr->frame_ctl |= RTLLIB_STYPE_QOS_NULL | RTLLIB_FTYPE_DATA;
	} else 
		hdr->frame_ctl |= RTLLIB_FTYPE_DATA | RTLLIB_STYPE_NULLFUNC;
	
	hdr->frame_ctl |= (pwr ? RTLLIB_FCTL_PM:0);
	hdr->frame_ctl = cpu_to_le16(hdr->frame_ctl); 
	return skb;
	
	
}

struct sk_buff* rtllib_pspoll_func(struct rtllib_device *ieee)
{
	struct sk_buff *skb;
	struct rtllib_pspoll_hdr* hdr;
	
#ifdef USB_USE_ALIGNMENT
        u32 Tmpaddr=0;
        int alignment=0;
        skb = dev_alloc_skb(sizeof(struct rtllib_pspoll_hdr) + ieee->tx_headroom + USB_512B_ALIGNMENT_SIZE);
#else	
	skb = dev_alloc_skb(sizeof(struct rtllib_pspoll_hdr)+ieee->tx_headroom); 
#endif	
	if (!skb) 
		return NULL;
	
#ifdef USB_USE_ALIGNMENT
        Tmpaddr = (u32)skb->data;
        alignment = Tmpaddr & 0x1ff;
        skb_reserve(skb,(USB_512B_ALIGNMENT_SIZE - alignment));
#endif	
	skb_reserve(skb, ieee->tx_headroom);

	hdr = (struct rtllib_pspoll_hdr*)skb_put(skb,sizeof(struct rtllib_pspoll_hdr));

	memcpy(hdr->bssid, ieee->current_network.bssid, ETH_ALEN);
	memcpy(hdr->ta, ieee->dev->dev_addr, ETH_ALEN);

	hdr->aid = cpu_to_le16(ieee->assoc_id | 0xc000); 
	hdr->frame_ctl = cpu_to_le16(RTLLIB_FTYPE_CTL |RTLLIB_STYPE_PSPOLL | RTLLIB_FCTL_PM); 
	
	return skb;	
	
}

void rtllib_resp_to_assoc_rq(struct rtllib_device *ieee, u8* dest)
{
	struct sk_buff *buf = rtllib_assoc_resp(ieee, dest);
#ifdef ASL
#ifdef ASL_DEBUG_2	
	int i;
	int len = buf->len;
#endif	
#endif
	
	if (buf) {
#ifdef ASL
#ifdef ASL_DEBUG_2
		printk("\nASSOCIATION RESPONSE+++++++++++++++++++++++++++");
        	printk("\n----------------------------------------\n");
	        for(i = 0; i < len; i++)
        	        printk("%02X-", buf->data[i] & 0xff);
	        printk("\n----------------------------------------\n");
#endif
#endif
#ifdef ASL
		softmac_mgmt_xmit(buf, ieee, true);
#else
		softmac_mgmt_xmit(buf, ieee);
#endif
        }
}


void rtllib_resp_to_auth(struct rtllib_device *ieee, int s, u8* dest)
{
	struct sk_buff *buf = rtllib_auth_resp(ieee, s, dest);
#ifdef ASL_DEBUG_2
	int i;
	int len = buf->len;
#endif
	
	if (buf) {
#ifdef ASL
#ifdef ASL_DEBUG_2
		printk("\nAUTH RESPONSE+++++++++++++++++++++++++++");
        	printk("\n----------------------------------------\n");
	        for(i = 0; i < len; i++)
        	        printk("%02X-", buf->data[i] & 0xff);
	        printk("\n----------------------------------------\n");
#endif
#endif		
#ifdef ASL
		softmac_mgmt_xmit(buf, ieee, true);
#else
		softmac_mgmt_xmit(buf, ieee);
#endif
        }
}


void rtllib_resp_to_probe(struct rtllib_device *ieee, u8 *dest)
{
	
	struct sk_buff *buf = rtllib_probe_resp(ieee, dest, false);
#ifdef ASL_DUBUG_2
	int i;
	int len = buf->len;
#endif

	if (buf) {
#ifdef ASL
#ifdef ASL_DEBUG_2
		printk("\nPROBE RESPONSE+++++++++++++++++++++++++++");
        	printk("\n----------------------------------------\n");
	        for(i = 0; i < len; i++)
        	        printk("%02X-", buf->data[i] & 0xff);
	        printk("\n----------------------------------------\n");
#endif
#endif		
#ifdef ASL
		if (ieee->iw_mode == IW_MODE_ADHOC)
			softmac_mgmt_xmit(buf, ieee, false);
		else
			softmac_mgmt_xmit(buf, ieee, true);
#else
		softmac_mgmt_xmit(buf, ieee);
#endif
        }
}


inline int SecIsInPMKIDList(struct rtllib_device *ieee, u8 *bssid)
{
	int i = 0;

	do 
	{
		if ((ieee->PMKIDList[i].bUsed) && (memcmp(ieee->PMKIDList[i].Bssid, bssid, ETH_ALEN) == 0))
		{
			break;
		}
		else
		{	
			i++;
		}
	} while (i < NUM_PMKID_CACHE);

	if (i == NUM_PMKID_CACHE)
	{ 
		i = -1;
	}
	else
	{ 
	}

	return (i);
	
}


inline struct sk_buff *rtllib_association_req(struct rtllib_network *beacon,struct rtllib_device *ieee)
{
	struct sk_buff *skb;
	
	struct rtllib_assoc_request_frame *hdr;
	u8 *tag, *ies;
	int i;
	u8* ht_cap_buf = NULL;
	u8 ht_cap_len=0;
	u8* realtek_ie_buf=NULL;
	u8 realtek_ie_len=0;
	int wpa_ie_len= ieee->wpa_ie_len;
	int wps_ie_len = ieee->wps_ie_len;
	unsigned int ckip_ie_len=0;
	unsigned int ccxrm_ie_len=0;
	unsigned int cxvernum_ie_len=0;
	struct rtllib_crypt_data* crypt;
	int encrypt;
	int	PMKCacheIdx;
	
	unsigned int rate_len = (beacon->rates_len?(beacon->rates_len+2):0) + (beacon->rates_ex_len?(beacon->rates_ex_len)+2:0);
	
	unsigned int wmm_info_len = beacon->qos_data.supported?9:0;
#ifdef THOMAS_TURBO	
	unsigned int turbo_info_len = beacon->Turbo_Enable?9:0;
#endif

	int len = 0; 
#ifdef RTL8192S_WAPI_SUPPORT
	PRT_WAPI_BKID		pWapiBKID;
	u16					bkidNum;
	PRT_WAPI_T			pWapiInfo = &(ieee->wapiInfo);
	u8					WapiIE[256];
	u8					WapiIELength = 0;
#endif                             
#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
	crypt = ieee->sta_crypt[ieee->tx_keyidx];
#else
	crypt = ieee->crypt[ieee->tx_keyidx];
#endif
	if(crypt != NULL) {
		encrypt = ieee->host_encrypt && crypt && crypt->ops && ((0 == strcmp(crypt->ops->name,"WEP") || wpa_ie_len));
	} else {
		encrypt = 0;
	}

#ifdef ENABLE_TKIP11N
	if (ieee->bForcedBgMode == true)
#else
	if ((ieee->rtllib_ap_sec_type && 
		((ieee->rtllib_ap_sec_type(ieee) & SEC_ALG_TKIP) ||
		ieee->rtllib_ap_sec_type(ieee) & SEC_ALG_WEP)) ||
		(ieee->bForcedBgMode == true)) 
#endif
	{
		ieee->pHTInfo->bEnableHT = 0;
		ieee->mode = WIRELESS_MODE_G;
	}
	
	if(ieee->pHTInfo->bCurrentHTSupport&&ieee->pHTInfo->bEnableHT)
	{
		ht_cap_buf = (u8*)&(ieee->pHTInfo->SelfHTCap);
		ht_cap_len = sizeof(ieee->pHTInfo->SelfHTCap);
#ifdef _RTL8192_EXT_PATCH_
		HTConstructCapabilityElement(ieee, ht_cap_buf, &ht_cap_len, encrypt, 0, true);
#else
#ifdef ASL
		HTConstructCapabilityElement(ieee, ht_cap_buf, &ht_cap_len, encrypt, 0, true, false);
#else
		HTConstructCapabilityElement(ieee, ht_cap_buf, &ht_cap_len, encrypt, true);
#endif
#endif
		if(ieee->pHTInfo->bCurrentRT2RTAggregation)
		{
			realtek_ie_buf = ieee->pHTInfo->szRT2RTAggBuffer;
			realtek_ie_len = sizeof( ieee->pHTInfo->szRT2RTAggBuffer);
			HTConstructRT2RTAggElement(ieee, realtek_ie_buf, &realtek_ie_len);
       	 			
		}
	}

	if(beacon->bCkipSupported)
	{
		ckip_ie_len = 30+2;
	}
	if(beacon->bCcxRmEnable)
	{	
		ccxrm_ie_len = 6+2;
	}
	if( beacon->BssCcxVerNumber >= 2 )
	{	
		cxvernum_ie_len = 5+2;
	}
	
	PMKCacheIdx = SecIsInPMKIDList(ieee, ieee->current_network.bssid);
	if (PMKCacheIdx >= 0)
	{
		wpa_ie_len += 18;
		printk("[PMK cache]: WPA2 IE length: %x\n", wpa_ie_len);
	}	
#ifdef RTL8192S_WAPI_SUPPORT
	if(ieee->WapiSupport && pWapiInfo->bWapiEnable)
	{
		wpa_ie_len = 0;
		
		WAPI_TRACE(WAPI_API, "%s(): Set WAPI IE in assoc req!!\n",__FUNCTION__);
		WapiSetIE(ieee);
		WapiIELength = pWapiInfo->wapiIELength;
		memcpy(WapiIE, pWapiInfo->wapiIE ,pWapiInfo->wapiIELength);
		
		bkidNum = 0;
		WapiIELength +=2;
		if(!list_empty(&(pWapiInfo->wapiBKIDStoreList))){
			list_for_each_entry(pWapiBKID, &pWapiInfo->wapiBKIDStoreList, list) {
				bkidNum ++;
				memcpy(WapiIE+WapiIELength, pWapiBKID->bkid,16);
				WapiIELength += 16;
			}
		
		}
		memcpy(WapiIE+pWapiInfo->wapiIELength,&bkidNum,2);
	}
#endif
	
	len = sizeof(struct rtllib_assoc_request_frame)+ 2
		+ beacon->ssid_len
		+ rate_len
		+ wpa_ie_len 
		+ wps_ie_len
#ifdef RTL8192S_WAPI_SUPPORT
		+ WapiIELength + 2
#endif
		+ wmm_info_len
#ifdef THOMAS_TURBO
		+ turbo_info_len
#endif
                + ht_cap_len
		+ realtek_ie_len
		+ ckip_ie_len
		+ ccxrm_ie_len
		+ cxvernum_ie_len
		+ ieee->tx_headroom;

#ifdef USB_USE_ALIGNMENT
	u32 Tmpaddr=0;
	int alignment=0;
	skb = dev_alloc_skb(len + USB_512B_ALIGNMENT_SIZE);
#else	
	skb = dev_alloc_skb(len);
#endif
	
	if (!skb) 
		return NULL;

#ifdef USB_USE_ALIGNMENT
        Tmpaddr = (u32)skb->data;
        alignment = Tmpaddr & 0x1ff;
        skb_reserve(skb,(USB_512B_ALIGNMENT_SIZE - alignment));
#endif

	skb_reserve(skb, ieee->tx_headroom);
	
	hdr = (struct rtllib_assoc_request_frame *)
		skb_put(skb, sizeof(struct rtllib_assoc_request_frame)+2);
	
	
	hdr->header.frame_ctl = RTLLIB_STYPE_ASSOC_REQ;
	hdr->header.duration_id= 37; 
	memcpy(hdr->header.addr1, beacon->bssid, ETH_ALEN);
	memcpy(hdr->header.addr2, ieee->dev->dev_addr, ETH_ALEN);
	memcpy(hdr->header.addr3, beacon->bssid, ETH_ALEN);

	memcpy(ieee->ap_mac_addr, beacon->bssid, ETH_ALEN);
	
	hdr->capability = cpu_to_le16(WLAN_CAPABILITY_ESS);
	if (beacon->capability & WLAN_CAPABILITY_PRIVACY ) 
		hdr->capability |= cpu_to_le16(WLAN_CAPABILITY_PRIVACY);

	if (beacon->capability & WLAN_CAPABILITY_SHORT_PREAMBLE)
		hdr->capability |= cpu_to_le16(WLAN_CAPABILITY_SHORT_PREAMBLE); 

	if(ieee->short_slot && (beacon->capability&WLAN_CAPABILITY_SHORT_SLOT_TIME))
		hdr->capability |= cpu_to_le16(WLAN_CAPABILITY_SHORT_SLOT_TIME);

 	if (wmm_info_len) 
		hdr->capability |= cpu_to_le16(WLAN_CAPABILITY_QOS);

	hdr->listen_interval = beacon->listen_interval;
	
	hdr->info_element[0].id = MFIE_TYPE_SSID;

	hdr->info_element[0].len = beacon->ssid_len;
	tag = skb_put(skb, beacon->ssid_len);
	memcpy(tag, beacon->ssid, beacon->ssid_len);
	
	tag = skb_put(skb, rate_len); 
	
	if(beacon->rates_len){
		*tag++ = MFIE_TYPE_RATES;
		*tag++ = beacon->rates_len;
		for(i=0;i<beacon->rates_len;i++){
			*tag++ = beacon->rates[i];
		}
	}

	if(beacon->rates_ex_len){
		*tag++ = MFIE_TYPE_RATES_EX;
		*tag++ = beacon->rates_ex_len;
		for(i=0;i<beacon->rates_ex_len;i++){
			*tag++ = beacon->rates_ex[i];
		}
	}

	if( beacon->bCkipSupported )
	{
		static u8	AironetIeOui[] = {0x00, 0x01, 0x66}; 
		u8	CcxAironetBuf[30];
		OCTET_STRING	osCcxAironetIE;

		memset(CcxAironetBuf, 0,30);
		osCcxAironetIE.Octet = CcxAironetBuf;
		osCcxAironetIE.Length = sizeof(CcxAironetBuf);
		memcpy(osCcxAironetIE.Octet, AironetIeOui, sizeof(AironetIeOui));

		osCcxAironetIE.Octet[IE_CISCO_FLAG_POSITION] |=  (SUPPORT_CKIP_PK|SUPPORT_CKIP_MIC) ;
		tag = skb_put(skb, ckip_ie_len);
		*tag++ = MFIE_TYPE_AIRONET;
		*tag++ = osCcxAironetIE.Length;
		memcpy(tag,osCcxAironetIE.Octet,osCcxAironetIE.Length);
		tag += osCcxAironetIE.Length;
	}

	if(beacon->bCcxRmEnable)
	{
		static u8 CcxRmCapBuf[] = {0x00, 0x40, 0x96, 0x01, 0x01, 0x00};
		OCTET_STRING osCcxRmCap;

		osCcxRmCap.Octet = CcxRmCapBuf;
		osCcxRmCap.Length = sizeof(CcxRmCapBuf);
		tag = skb_put(skb,ccxrm_ie_len);
		*tag++ = MFIE_TYPE_GENERIC;
		*tag++ = osCcxRmCap.Length;
		memcpy(tag,osCcxRmCap.Octet,osCcxRmCap.Length);
		tag += osCcxRmCap.Length;
	}

	if( beacon->BssCcxVerNumber >= 2 )
	{
		u8			CcxVerNumBuf[] = {0x00, 0x40, 0x96, 0x03, 0x00};
		OCTET_STRING	osCcxVerNum;
		CcxVerNumBuf[4] = beacon->BssCcxVerNumber;
		osCcxVerNum.Octet = CcxVerNumBuf;
		osCcxVerNum.Length = sizeof(CcxVerNumBuf);
		tag = skb_put(skb,cxvernum_ie_len);
		*tag++ = MFIE_TYPE_GENERIC;
		*tag++ = osCcxVerNum.Length;
		memcpy(tag,osCcxVerNum.Octet,osCcxVerNum.Length);
		tag += osCcxVerNum.Length;
	}
	if(ieee->pHTInfo->bCurrentHTSupport&&ieee->pHTInfo->bEnableHT){
		if(ieee->pHTInfo->ePeerHTSpecVer != HT_SPEC_VER_EWC)
		{
			tag = skb_put(skb, ht_cap_len);
			*tag++ = MFIE_TYPE_HT_CAP;
			*tag++ = ht_cap_len - 2;
			memcpy(tag, ht_cap_buf,ht_cap_len -2);
			tag += ht_cap_len -2;
		}
	}	         

	
	if (wpa_ie_len){
		tag = skb_put(skb, ieee->wpa_ie_len);	
		memcpy(tag, ieee->wpa_ie, ieee->wpa_ie_len);
		
		if (PMKCacheIdx >= 0)
		{
			tag = skb_put(skb, 18);		
			*tag = 1;
			*(tag + 1) = 0;	
			memcpy((tag + 2), &ieee->PMKIDList[PMKCacheIdx].PMKID, 16);
		}
	}
#ifdef RTL8192S_WAPI_SUPPORT
	if(WapiIELength){
		tag = skb_put(skb, WapiIELength+2);
		*tag = MFIE_TYPE_WAPI;
		*(tag+1) = WapiIELength;
		memcpy((tag+2), WapiIE, WapiIELength);

		pWapiInfo->assoReqWapiIELength = WapiIELength+2;
		pWapiInfo->assoReqWapiIE[0] = MFIE_TYPE_WAPI;
		pWapiInfo->assoReqWapiIE[1] = WapiIELength;
		memcpy(pWapiInfo->assoReqWapiIE+2,WapiIE,WapiIELength);
	}
#endif

	if(wmm_info_len) {
		tag = skb_put(skb,wmm_info_len);
		rtllib_WMM_Info(ieee, &tag);
	}

	if(wps_ie_len && ieee->wps_ie)
	{
		tag = skb_put(skb, wps_ie_len);		
		memcpy(tag, ieee->wps_ie, wps_ie_len);		
	}	
	
#ifdef THOMAS_TURBO
	tag = skb_put(skb,turbo_info_len);
        if(turbo_info_len) {
                rtllib_TURBO_Info(ieee, &tag);
        }
#endif

	if(ieee->pHTInfo->bCurrentHTSupport&&ieee->pHTInfo->bEnableHT){
		if(ieee->pHTInfo->ePeerHTSpecVer == HT_SPEC_VER_EWC)
		{
			tag = skb_put(skb, ht_cap_len);
			*tag++ = MFIE_TYPE_GENERIC;
			*tag++ = ht_cap_len - 2;
			memcpy(tag, ht_cap_buf,ht_cap_len - 2);
			tag += ht_cap_len -2;
		}

		if(ieee->pHTInfo->bCurrentRT2RTAggregation){
			tag = skb_put(skb, realtek_ie_len);
			*tag++ = MFIE_TYPE_GENERIC;
			*tag++ = realtek_ie_len - 2;
			memcpy(tag, realtek_ie_buf,realtek_ie_len -2 );
		}
	}

	if(ieee->assocreq_ies){
		kfree(ieee->assocreq_ies);
		ieee->assocreq_ies = NULL;
	}
	ies = &(hdr->info_element[0].id);
	ieee->assocreq_ies_len = (skb->data + skb->len) - ies;
	ieee->assocreq_ies = kmalloc(ieee->assocreq_ies_len, GFP_ATOMIC);
	if (ieee->assocreq_ies)
		memcpy(ieee->assocreq_ies, ies, ieee->assocreq_ies_len);
	else{
		printk("%s()Warning: can't alloc memory for assocreq_ies\n", __func__);
		ieee->assocreq_ies_len = 0;
	}	
	
	return skb;
}

void rtllib_associate_abort(struct rtllib_device *ieee)
{
	
	unsigned long flags;
	spin_lock_irqsave(&ieee->lock, flags);
	
	ieee->associate_seq++;
	
	/* don't scan, and avoid to have the RX path possibily
	 * try again to associate. Even do not react to AUTH or
	 * ASSOC response. Just wait for the retry wq to be scheduled.
	 * Here we will check if there are good nets to associate
	 * with, so we retry or just get back to NO_LINK and scanning
	 */
	if (ieee->state == RTLLIB_ASSOCIATING_AUTHENTICATING){
		RTLLIB_DEBUG_MGMT("Authentication failed\n"); 
		ieee->softmac_stats.no_auth_rs++;
	}else{
		RTLLIB_DEBUG_MGMT("Association failed\n"); 
		ieee->softmac_stats.no_ass_rs++;
	}
		
	ieee->state = RTLLIB_ASSOCIATING_RETRY;
		
	queue_delayed_work_rsl(ieee->wq, &ieee->associate_retry_wq, \
                           RTLLIB_SOFTMAC_ASSOC_RETRY_TIME);
	
	spin_unlock_irqrestore(&ieee->lock, flags);
}

void rtllib_associate_abort_cb(unsigned long dev)
{
	rtllib_associate_abort((struct rtllib_device *) dev);
}

void rtllib_associate_step1(struct rtllib_device *ieee,u8 * daddr)
{
	struct rtllib_network *beacon = &ieee->current_network;
	struct sk_buff *skb;
#ifdef _RTL8192_EXT_PATCH_
	bool is_mesh = false;
#endif
	RTLLIB_DEBUG_MGMT("Stopping scan\n");

	ieee->softmac_stats.tx_auth_rq++;

	skb=rtllib_authentication_req(beacon, ieee, 0,daddr);

#ifdef _RTL8192_EXT_PATCH_
	is_mesh = ieee->ext_patch_rtllib_is_mesh(ieee,daddr);
	if(is_mesh)	{
		if(skb){	
			softmac_mgmt_xmit(skb, ieee);
		}
		return;
	}else
#endif		
	if (!skb) 
		rtllib_associate_abort(ieee);
	else{ 
		ieee->state = RTLLIB_ASSOCIATING_AUTHENTICATING ;
		RTLLIB_DEBUG_MGMT("Sending authentication request\n");
#ifdef ASL
		softmac_mgmt_xmit(skb, ieee,false);
#else
		softmac_mgmt_xmit(skb, ieee);
#endif
		if(!timer_pending(&ieee->associate_timer)){
			ieee->associate_timer.expires = jiffies + (HZ / 2);
			add_timer(&ieee->associate_timer);
		}
	}
}

void rtllib_auth_challenge(struct rtllib_device *ieee, u8 *challenge, int chlen)
{
	u8 *c;	
	struct sk_buff *skb;
	struct rtllib_network *beacon = &ieee->current_network;
	
	ieee->associate_seq++;
	ieee->softmac_stats.tx_auth_rq++;
	
	skb = rtllib_authentication_req(beacon, ieee, chlen+2,beacon->bssid);
	
	if (!skb) 
		rtllib_associate_abort(ieee);
	else{
		c = skb_put(skb, chlen+2);
		*(c++) = MFIE_TYPE_CHALLENGE;
		*(c++) = chlen;
		memcpy(c, challenge, chlen);
		
		RTLLIB_DEBUG_MGMT("Sending authentication challenge response\n");
		
#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
		rtllib_encrypt_fragment(ieee, skb, sizeof(struct rtllib_hdr_3addr  ), 0, 0);
#else
		rtllib_encrypt_fragment(ieee, skb, sizeof(struct rtllib_hdr_3addr  ));
#endif		
#ifdef ASL
		softmac_mgmt_xmit(skb, ieee, false);
#else
		softmac_mgmt_xmit(skb, ieee);
#endif
		mod_timer(&ieee->associate_timer, jiffies + (HZ/2));
#if 0
		ieee->associate_timer.expires = jiffies + (HZ / 2);
		add_timer(&ieee->associate_timer);
#endif
	}	
	kfree(challenge);
}

void rtllib_associate_step2(struct rtllib_device *ieee)
{
	struct sk_buff* skb;
	struct rtllib_network *beacon = &ieee->current_network;
	
	del_timer_sync(&ieee->associate_timer);
	
	RTLLIB_DEBUG_MGMT("Sending association request\n");
	
	ieee->softmac_stats.tx_ass_rq++;
	skb=rtllib_association_req(beacon, ieee);
	if (!skb) 
		rtllib_associate_abort(ieee);
	else{
#ifdef ASL
		softmac_mgmt_xmit(skb, ieee, false);
#else
		softmac_mgmt_xmit(skb, ieee);
#endif
		mod_timer(&ieee->associate_timer, jiffies + (HZ/2));
#if 0
		ieee->associate_timer.expires = jiffies + (HZ / 2);
		add_timer(&ieee->associate_timer);
#endif
	}	
}

#define CANCELLED  2
void rtllib_associate_complete_wq(void *data)
{
	struct rtllib_device *ieee = (struct rtllib_device *)container_of_work_rsl(data, struct rtllib_device, associate_complete_wq);
	PRT_POWER_SAVE_CONTROL pPSC = (PRT_POWER_SAVE_CONTROL)(&(ieee->PowerSaveControl));
	printk(KERN_INFO "Associated successfully\n");
	if(ieee->is_silent_reset == 0){
            printk("normal associate\n");
            notify_wx_assoc_event(ieee); 
        }

	netif_carrier_on(ieee->dev);
	ieee->is_roaming = false;
	if(rtllib_is_54g(&ieee->current_network) && 
		(ieee->modulation & RTLLIB_OFDM_MODULATION)){
		
		ieee->rate = 108;
		printk(KERN_INFO"Using G rates:%d\n", ieee->rate);
	}else{
		ieee->rate = 22;
#ifdef ASL
		ieee->SetWirelessMode(ieee->dev, IEEE_B, false);
#else
		ieee->SetWirelessMode(ieee->dev, IEEE_B);
#endif
		printk(KERN_INFO"Using B rates:%d\n", ieee->rate);
	}
	if (ieee->pHTInfo->bCurrentHTSupport&&ieee->pHTInfo->bEnableHT)
	{
		printk("Successfully associated, ht enabled\n");
		HTOnAssocRsp(ieee);
	} else {
		printk("Successfully associated, ht not enabled(%d, %d)\n", 
				ieee->pHTInfo->bCurrentHTSupport, ieee->pHTInfo->bEnableHT);
		memset(ieee->dot11HTOperationalRateSet, 0, 16);
	}
	ieee->LinkDetectInfo.SlotNum = 2 * (1 + ieee->current_network.beacon_interval/500);
	if(ieee->LinkDetectInfo.NumRecvBcnInPeriod==0||ieee->LinkDetectInfo.NumRecvDataInPeriod==0 )
	{
		ieee->LinkDetectInfo.NumRecvBcnInPeriod = 1;
		ieee->LinkDetectInfo.NumRecvDataInPeriod= 1;	
	}
	pPSC->LpsIdleCount = 0;
	ieee->link_change(ieee->dev);

#ifdef _RTL8192_EXT_PATCH_	
	if(ieee->set_key_for_AP)
		ieee->set_key_for_AP(ieee);
	/* Synchronize mesh channel to wlan channel in MSTA mode.*/
	{
		if(ieee->current_mesh_network.channel != ieee->current_network.channel)
		{
			printk("^^^^^^^^^^^^^^^^Change mesh channel %d with wlan channel %d\n", 
					ieee->current_mesh_network.channel, ieee->current_network.channel);
			ieee->ext_patch_rtllib_close_all_peerlink(ieee, CANCELLED);
			ieee->current_mesh_network.channel = ieee->current_network.channel;
			if(ieee->ext_patch_r819x_wx_set_channel)
			{
				ieee->ext_patch_r819x_wx_set_channel(ieee, ieee->current_network.channel);
				ieee->ext_patch_r819x_wx_set_mesh_chan(ieee->dev,ieee->current_network.channel);
			}
		}
	}
#endif	
#ifdef ASL
	if(ieee->set_key_for_AP)
		ieee->set_key_for_AP(ieee);
	if (ieee->current_ap_network.channel != ieee->current_network.channel) {
		ieee->current_ap_network.channel = ieee->current_network.channel;
		if (ieee->ap_state == RTLLIB_LINKED) {
			int idx = 0;
			struct sta_info * psta = NULL;
			apdev_send_disassociation_to_all_sta(ieee,1,deauth_lv_ss);
			ap_del_stainfo_list(ieee);
			for(idx = 0; idx < APDEV_MAX_ASSOC; idx++)
			{
				psta = ieee->apdev_assoc_list[idx];
				if(psta != NULL) {
					RemovePeerTS(ieee,psta->macaddr);
				}
			}
		}
	}
#endif
        if(ieee->is_silent_reset == 0){
        } else if(ieee->is_silent_reset == 1) {
            printk("silent reset associate\n");
            ieee->is_silent_reset = 0;
        }

	if (ieee->data_hard_resume)
		ieee->data_hard_resume(ieee->dev);

#ifdef RTK_DMP_PLATFORM
	kobject_hotplug(&ieee->dev->class_dev.kobj, KOBJ_LINKUP);
#endif
}

static void rtllib_sta_send_associnfo(struct rtllib_device *ieee)
{
	char *buf;
	size_t len;
	int i;
	union iwreq_data wrqu;

		return;

	
	buf = kmalloc(50 + 2 * (ieee->assocreq_ies_len + ieee->assocresp_ies_len), GFP_ATOMIC);
	if (!buf)
		return;

	len = sprintf(buf, "ASSOCINFO(");
	if (ieee->assocreq_ies) {
		len += sprintf(buf + len, "ReqIEs=");
		for (i = 0; i < ieee->assocreq_ies_len; i++) {
			len += sprintf(buf + len, "%02x", ieee->assocreq_ies[i]);
		}
	}
	if (ieee->assocresp_ies) {
		if (ieee->assocreq_ies)
			len += sprintf(buf + len, " ");
		len += sprintf(buf + len, "RespIEs=");
		for (i = 0; i < ieee->assocresp_ies_len; i++) {
			len += sprintf(buf + len, "%02x", ieee->assocresp_ies[i]);
		}
	}
	len += sprintf(buf + len, ")");

	if (len > IW_CUSTOM_MAX) {
		len = sprintf(buf, "ASSOCRESPIE=");
		for (i = 0; i < ieee->assocresp_ies_len; i++) {
			len += sprintf(buf + len, "%02x", ieee->assocresp_ies[i]);
		}
	}

	if (len <= IW_CUSTOM_MAX) {
		memset(&wrqu, 0, sizeof(wrqu));
		wrqu.data.length = len;
		wireless_send_event(ieee->dev, IWEVCUSTOM, &wrqu, buf);
	}

	kfree(buf);
}

void rtllib_associate_complete(struct rtllib_device *ieee)
{
	del_timer_sync(&ieee->associate_timer);

#if 0
	for(i = 0; i < 6; i++) {
	  ieee->seq_ctrl[i] = 0;
	}
#endif	
	ieee->state = RTLLIB_LINKED;
#if defined (RTL8192S_WAPI_SUPPORT)
	if ((ieee->WapiSupport) && (ieee->wapiInfo.bWapiEnable)){
		WAPI_CreateEvent_Send(ieee, WAPI_EVENT_CONNECT, ieee->current_network.bssid, NULL, 0);
	}
#endif
#if 0
	if (ieee->pHTInfo->bCurrentHTSupport)
	{
		printk("Successfully associated, ht enabled\n");
		queue_work_rsl(ieee->wq, &ieee->ht_onAssRsp);
	}
	else
	{
		printk("Successfully associated, ht not enabled\n");
		memset(ieee->dot11HTOperationalRateSet, 0, 16);
		HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20, HT_EXTCHNL_OFFSET_NO_EXT);
	}
#endif

	rtllib_sta_send_associnfo(ieee);

	queue_work_rsl(ieee->wq, &ieee->associate_complete_wq);
}

void rtllib_associate_procedure_wq(void *data)
{
	struct rtllib_device *ieee = container_of_dwork_rsl(data, struct rtllib_device, associate_procedure_wq);
	rtllib_stop_scan_syncro(ieee);
#ifdef ENABLE_IPS
	if(ieee->rtllib_ips_leave != NULL)
        	ieee->rtllib_ips_leave(ieee->dev);
#endif
	down(&ieee->wx_sem);
	
	if (ieee->data_hard_stop)
		ieee->data_hard_stop(ieee->dev);
	
	rtllib_stop_scan(ieee);
	printk("%s(), chan:%d\n", __FUNCTION__, ieee->current_network.channel);	
#ifdef ASL
	HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20, HT_EXTCHNL_OFFSET_NO_EXT, false);
#else
	HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20, HT_EXTCHNL_OFFSET_NO_EXT);
#endif
	if(ieee->eRFPowerState == eRfOff)
	{
            printk("=============>%s():Rf state is eRfOff, schedule ipsleave wq again,return\n",__FUNCTION__);
#ifdef ENABLE_IPS
		if(ieee->rtllib_ips_leave_wq != NULL)
			ieee->rtllib_ips_leave_wq(ieee->dev);
#endif
		up(&ieee->wx_sem);
		return;
	}
	ieee->associate_seq = 1;
	ieee->current_qos_mode = (ieee->current_network.qos_data.qos_mode) & (ieee->qos_capability);
	printk("==============>%s():ieee->current_network.qos_data.qos_mode is %x,ieee->qos_capability is %x\n",__FUNCTION__,ieee->current_network.qos_data.qos_mode,ieee->qos_capability);
	rtllib_associate_step1(ieee, ieee->current_network.bssid);
	
	up(&ieee->wx_sem);
}

#ifdef _RTL8192_EXT_PATCH_

void rtllib_ext_stop_scan_wq(void *data)
{
	struct rtllib_device *ieee = (struct rtllib_device *)container_of_work_rsl(data, struct rtllib_device, ext_stop_scan_wq);
	rtllib_stop_scan_syncro(ieee);
	
	down(&ieee->wx_sem);
	
	if (ieee->data_hard_stop)
		ieee->data_hard_stop(ieee->dev);
	
	rtllib_stop_scan(ieee);

	up(&ieee->wx_sem);
}


void rtllib_ext_send_11s_beacon(struct rtllib_device *ieee)
{	
	queue_work_rsl(ieee->wq, &ieee->ext_send_beacon_wq);
}

#endif 


inline void rtllib_softmac_new_net(struct rtllib_device *ieee, struct rtllib_network *net)
{
	u8 tmp_ssid[IW_ESSID_MAX_SIZE+1];
	int tmp_ssid_len = 0;
	
	short apset,ssidset,ssidbroad,apmatch,ssidmatch;
	
	/* we are interested in new new only if we are not associated 
	 * and we are not associating / authenticating
	 */
	if (ieee->state != RTLLIB_NOLINK)
		return; 

	if ((ieee->iw_mode == IW_MODE_INFRA) && !(net->capability & WLAN_CAPABILITY_ESS))
		return;
	
	if ((ieee->iw_mode == IW_MODE_ADHOC) && !(net->capability & WLAN_CAPABILITY_IBSS))
		return;

	if ((ieee->iw_mode == IW_MODE_ADHOC) && (net->channel > ieee->ibss_maxjoin_chal)) {
		return;
	}
#ifdef _RTL8192_EXT_PATCH_
	if (ieee->iw_mode == IW_MODE_INFRA || ieee->iw_mode == IW_MODE_ADHOC 
	    || ((ieee->iw_mode == IW_MODE_MESH) && (ieee->only_mesh == 0)))
#else
#ifdef ASL
	if (ieee->iw_mode == IW_MODE_INFRA || ieee->iw_mode == IW_MODE_ADHOC || ieee->iw_mode == IW_MODE_APSTA) 
#else
	if (ieee->iw_mode == IW_MODE_INFRA || ieee->iw_mode == IW_MODE_ADHOC) 
#endif
#endif
		{
		/* if the user specified the AP MAC, we need also the essid
		 * This could be obtained by beacons or, if the network does not
		 * broadcast it, it can be put manually.
		 */
		apset = ieee->wap_set;
		ssidset = ieee->ssid_set;
		ssidbroad =  !(net->ssid_len == 0 || net->ssid[0]== '\0');
		apmatch = (memcmp(ieee->current_network.bssid, net->bssid, ETH_ALEN)==0);
		if(!ssidbroad){
		    	ssidmatch = (ieee->current_network.ssid_len == net->hidden_ssid_len)&&\
					(!strncmp(ieee->current_network.ssid, net->hidden_ssid, net->hidden_ssid_len));
			if(net->hidden_ssid_len > 0)
                        {
			        strncpy(net->ssid, net->hidden_ssid, net->hidden_ssid_len);
			        net->ssid_len = net->hidden_ssid_len;
                                ssidbroad = 1;
                        }
		}
		else
			ssidmatch = (ieee->current_network.ssid_len == net->ssid_len)&&\
					(!strncmp(ieee->current_network.ssid, net->ssid, net->ssid_len));

		if (	/* if the user set the AP check if match.
		         * if the network does not broadcast essid we check the user supplyed ANY essid
			 * if the network does broadcast and the user does not set essid it is OK
			 * if the network does broadcast and the user did set essid chech if essid match
			 */
			( apset && apmatch && 
				((ssidset && ssidbroad && ssidmatch) || (ssidbroad && !ssidset) || (!ssidbroad && ssidset)) )
			/* if the ap is not set, check that the user set the bssid
			 * and the network does bradcast and that those two bssid matches
			 */ 
			 ||  (!apset && ssidset && ssidbroad && ssidmatch) || (ieee->is_roaming && ssidset && ssidbroad && ssidmatch) 
			){
				/* if the essid is hidden replace it with the
				* essid provided by the user.
				*/
				if (!ssidbroad){
					strncpy(tmp_ssid, ieee->current_network.ssid, IW_ESSID_MAX_SIZE);
					tmp_ssid_len = ieee->current_network.ssid_len;
				}
				memcpy(&ieee->current_network, net, sizeof(struct rtllib_network));
				if (!ssidbroad){
					strncpy(ieee->current_network.ssid, tmp_ssid, IW_ESSID_MAX_SIZE);
					ieee->current_network.ssid_len = tmp_ssid_len;
				}
				printk(KERN_INFO"Linking with %s,channel:%d, qos:%d, myHT:%d, networkHT:%d, mode:%x cur_net.flags:0x%x\n",ieee->current_network.ssid,ieee->current_network.channel, ieee->current_network.qos_data.supported, ieee->pHTInfo->bEnableHT, ieee->current_network.bssht.bdSupportHT, ieee->current_network.mode, ieee->current_network.flags);

				if ((rtllib_act_scanning(ieee, false)) && !(ieee->softmac_features & IEEE_SOFTMAC_SCAN)){
					rtllib_stop_scan_syncro(ieee);
				}

				ieee->hwscan_ch_bk = ieee->current_network.channel;
				HTResetIOTSetting(ieee->pHTInfo);
				ieee->wmm_acm = 0;
#ifdef _RTL8192_EXT_PATCH_
				if ((ieee->iw_mode == IW_MODE_INFRA) || 
					((ieee->iw_mode == IW_MODE_MESH) && (ieee->only_mesh == 0)))  
#else
#ifdef ASL
				if (ieee->iw_mode == IW_MODE_INFRA || ieee->iw_mode == IW_MODE_APSTA)
#else
				if (ieee->iw_mode == IW_MODE_INFRA)
#endif
#endif
				{
					/* Join the network for the first time */
					ieee->AsocRetryCount = 0;	
					if(/*(ieee->current_network.qos_data.supported == 1) &&*/
					   ieee->current_network.bssht.bdSupportHT)
/*WB, 2008.09.09:bCurrentHTSupport and bEnableHT two flags are going to put together to check whether we are in HT now, so needn't to check bEnableHT flags here. That's is to say we will set to HT support whenever joined AP has the ability to support HT. And whether we are in HT or not, please check bCurrentHTSupport&&bEnableHT now please.*/
					{
#ifdef ENABLE_AMSDU   
						if((ieee->mode == IEEE_N_24G) && (ieee->mode == IEEE_N_5G))
							HTUseDefaultSetting(ieee);
#endif
						HTResetSelfAndSavePeerSetting(ieee, &(ieee->current_network));
					}
					else
					{
						ieee->pHTInfo->bCurrentHTSupport = false;
					}

					ieee->state = RTLLIB_ASSOCIATING;
					if(ieee->LedControlHandler != NULL)
					        ieee->LedControlHandler(ieee->dev, LED_CTL_START_TO_LINK); 
					queue_delayed_work_rsl(ieee->wq, &ieee->associate_procedure_wq, 0);
				}else{
#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE)
#if defined (RTL8192S_WAPI_SUPPORT)
					if ((ieee->WapiSupport) && (ieee->wapiInfo.bWapiEnable)){
						WAPI_CreateEvent_Send(ieee, WAPI_EVENT_CONNECT, ieee->current_network.bssid, NULL, 0);
					}
#endif		
					ieee->state = RTLLIB_LINKED;
					if(ieee->mode > ieee->current_network.mode)
#ifdef ASL
						ieee->SetWirelessMode(ieee->dev, ieee->current_network.mode, false);
#else
						ieee->SetWirelessMode(ieee->dev, ieee->current_network.mode);
#endif
					else
#ifdef ASL
						ieee->SetWirelessMode(ieee->dev, ieee->mode, false);
#else
						ieee->SetWirelessMode(ieee->dev, ieee->mode);
#endif
					mod_timer(&ieee->ibss_wait_timer,jiffies+(MSECS(20000)));
#else
					if(rtllib_is_54g(&ieee->current_network) && 
						(ieee->modulation & RTLLIB_OFDM_MODULATION)){
						ieee->rate = 108;
#ifdef ASL
						ieee->SetWirelessMode(ieee->dev, IEEE_G, false);
#else
						ieee->SetWirelessMode(ieee->dev, IEEE_G);
#endif
						printk(KERN_INFO"Using G rates\n");
					}else{
						ieee->rate = 22;
#ifdef ASL
						ieee->SetWirelessMode(ieee->dev, IEEE_B, false);
#else
						ieee->SetWirelessMode(ieee->dev, IEEE_B);
#endif
						printk(KERN_INFO"Using B rates\n");
					}
					memset(ieee->dot11HTOperationalRateSet, 0, 16);
					ieee->state = RTLLIB_LINKED;
#endif
				}
			
		}
	}

}

void rtllib_softmac_check_all_nets(struct rtllib_device *ieee)
{
	unsigned long flags;
	struct rtllib_network *target;

	spin_lock_irqsave(&ieee->lock, flags);
			
	list_for_each_entry(target, &ieee->network_list, list) {
		
		/* if the state become different that NOLINK means
		 * we had found what we are searching for
		 */

		if (ieee->state != RTLLIB_NOLINK) 
			break;
	
		if (ieee->scan_age == 0 || time_after(target->last_scanned + ieee->scan_age, jiffies))
		rtllib_softmac_new_net(ieee, target);
	}
	
	spin_unlock_irqrestore(&ieee->lock, flags);
	
}


static inline u16 auth_parse(struct sk_buff *skb, u8** challenge, int *chlen)
{
	struct rtllib_authentication *a;
	u8 *t;
	if (skb->len <  (sizeof(struct rtllib_authentication)-sizeof(struct rtllib_info_element))){ 
		RTLLIB_DEBUG_MGMT("invalid len in auth resp: %d\n",skb->len);
		return 0xcafe;
	}
	*challenge = NULL;
	a = (struct rtllib_authentication*) skb->data;
	if(skb->len > (sizeof(struct rtllib_authentication) +3)){
		t = skb->data + sizeof(struct rtllib_authentication);
		
		if(*(t++) == MFIE_TYPE_CHALLENGE){
			*chlen = *(t++);
			*challenge = (u8*)kmalloc(*chlen, GFP_ATOMIC);
			memcpy(*challenge, t, *chlen);
		}
	}
	
	return cpu_to_le16(a->status);
	
}


int auth_rq_parse(struct sk_buff *skb,u8* dest)
{
	struct rtllib_authentication *a;
	
	if (skb->len <  (sizeof(struct rtllib_authentication)-sizeof(struct rtllib_info_element))){ 
		RTLLIB_DEBUG_MGMT("invalid len in auth request: %d\n",skb->len);	
		return -1;
	}
	a = (struct rtllib_authentication*) skb->data;
	
	memcpy(dest,a->header.addr2, ETH_ALEN);
	
	if (le16_to_cpu(a->algorithm) != WLAN_AUTH_OPEN) 
		return  RTLLIB_WLAN_STATUS_NOT_SUPPORTED_AUTH_ALG;
	
	return RTLLIB_WLAN_STATUS_SUCCESS;
}

static short probe_rq_parse(struct rtllib_device *ieee, struct sk_buff *skb, u8 *src)
{
	u8 *tag;
	u8 *skbend;
	u8 *ssid=NULL;
	u8 ssidlen = 0;
	struct rtllib_network* net = NULL;
	struct rtllib_hdr_3addr   *header =
		(struct rtllib_hdr_3addr   *) skb->data;
	
	if (skb->len < sizeof (struct rtllib_hdr_3addr  )) 
		return -1; /* corrupted */
#ifdef ASL
	if (ieee->iw_mode == IW_MODE_MASTER || ieee->iw_mode == IW_MODE_APSTA) 
		net = &ieee->current_ap_network;
	else
#endif
		net = &ieee->current_network;

        if((memcmp(header->addr3,net->bssid,ETH_ALEN) != 0)&&
                (memcmp(header->addr3,"\xff\xff\xff\xff\xff\xff",ETH_ALEN) != 0)) {
            return -1;
        }

        if(memcmp(header->addr3,net->bssid,ETH_ALEN) == 0) {
        }
	
        if(memcmp(header->addr3,"\xff\xff\xff\xff\xff\xff",ETH_ALEN) == 0) {
        }
	memcpy(src,header->addr2, ETH_ALEN);
	
	skbend = (u8*)skb->data + skb->len;
	
	tag = skb->data + sizeof (struct rtllib_hdr_3addr  );
	
	while (tag+1 < skbend){
		if (*tag == 0){ 
			ssid = tag+2;
			ssidlen = *(tag+1);
			break;
		}
		if (*tag == MFIE_TYPE_GENERIC && tag[1] > 4) {
			if(tag[2] == 0x00 &&
				tag[3] == 0x50 &&
				tag[4] == 0xf2 &&
				tag[5] == 0x04){
				printk("============>%s():has WPS IE\n",__FUNCTION__);
				return 2;
			}
		}
		tag++; /* point to the len field */
		tag = tag + *(tag); /* point to the last data byte of the tag */
		tag++; /* point to the next tag */
	}
	
#ifdef ASL
	if (ieee->bhidden_ssid == 0) {
#endif
		if (ssidlen == 0) {
			return 1;
		}
		if (!ssid) {
			return 1; /* ssid not found in tagged param */
		}
#ifdef ASL
	} else {
		if ((ssidlen == 0) || !ssid)
			return -1;
	}
#endif
	return (!strncmp(ssid, net->ssid, ssidlen));
		
}

int assoc_rq_parse(struct sk_buff *skb,u8* dest)
{
	struct rtllib_assoc_request_frame *a;
	
	if (skb->len < (sizeof(struct rtllib_assoc_request_frame) - 
		sizeof(struct rtllib_info_element))) { 
		
		RTLLIB_DEBUG_MGMT("invalid len in auth request:%d \n", skb->len);
		return -1;
	}
	
	a = (struct rtllib_assoc_request_frame*) skb->data;
		
	memcpy(dest,a->header.addr2,ETH_ALEN);
	
	return 0;
}

static inline u16 assoc_parse(struct rtllib_device *ieee, struct sk_buff *skb, int *aid)
{
	struct rtllib_assoc_response_frame *response_head;
	u16 status_code;

	if (skb->len <  sizeof(struct rtllib_assoc_response_frame)){ 
		RTLLIB_DEBUG_MGMT("invalid len in auth resp: %d\n", skb->len);
		return 0xcafe;
	}
	
	response_head = (struct rtllib_assoc_response_frame*) skb->data;
	*aid = le16_to_cpu(response_head->aid) & 0x3fff;

	status_code = le16_to_cpu(response_head->status);
	if((status_code==RTLLIB_WLAN_STATUS_ASSOC_DENIED_RATES || \
	   status_code==RTLLIB_WLAN_STATUS_CAPS_UNSUPPORTED)&&
	   ((ieee->mode == IEEE_G) && 
	    (ieee->current_network.mode == IEEE_N_24G) &&
            (ieee->AsocRetryCount++ < (RT_ASOC_RETRY_LIMIT-1)))) {
                 ieee->pHTInfo->IOTAction |= HT_IOT_ACT_PURE_N_MODE;
	}else {
		 ieee->AsocRetryCount = 0;
	}

	return le16_to_cpu(response_head->status);
}

void rtllib_rx_probe_rq(struct rtllib_device *ieee, struct sk_buff *skb)
{
	u8 dest[ETH_ALEN];
	int ret = 0;
#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE)
	struct sta_info *psta = NULL;
#endif
	ieee->softmac_stats.rx_probe_rq++;
	ret = probe_rq_parse(ieee, skb, dest);
	if ( ret > 0){
		ieee->softmac_stats.tx_probe_rs++;
		rtllib_resp_to_probe(ieee, dest);
#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE)
		if(ieee->iw_mode == IW_MODE_ADHOC){
			psta = GetStaInfo(ieee, dest);
			if(NULL != psta)
				psta->LastActiveTime = jiffies;
		}
#endif
#ifdef ASL
		if (ieee->iw_mode == IW_MODE_MASTER || ieee->iw_mode == IW_MODE_APSTA) {
			psta = ap_get_stainfo(ieee, dest);
			if(NULL != psta)
				psta->LastActiveTime = jiffies;
		}
		if (ret ==2 ) {
			apdev_create_event_send(ieee,skb->data,skb->len);
		}
#endif


	}
}

static inline void rtllib_rx_auth_rq(struct rtllib_device *ieee, struct sk_buff *skb)
{
	u8 dest[ETH_ALEN];
	int status;
	ieee->softmac_stats.rx_auth_rq++;

	if ((status = auth_rq_parse(skb, dest))!= -1){
		rtllib_resp_to_auth(ieee, status, dest);
	}

}

static inline void rtllib_rx_assoc_rq(struct rtllib_device *ieee, struct sk_buff *skb)
{
	
	u8 dest[ETH_ALEN];
	
	ieee->softmac_stats.rx_ass_rq++;
	if (assoc_rq_parse(skb,dest) != -1){
		rtllib_resp_to_assoc_rq(ieee, dest);
	}
	
	printk(KERN_INFO"New client associated: "MAC_FMT"\n", MAC_ARG(dest));
	#if 0
	spin_lock_irqsave(&ieee->lock,flags);
	add_associate(ieee,dest);
	spin_unlock_irqrestore(&ieee->lock,flags);
	#endif
}


void rtllib_sta_ps_send_null_frame(struct rtllib_device *ieee, short pwr)
{
	
	struct sk_buff *buf = rtllib_null_func(ieee, ieee->current_network.bssid, 0, 0, 0, pwr,false);
	if (buf)
#ifdef ASL
		softmac_ps_mgmt_xmit(buf, ieee, MGNT_QUEUE, false);
#else
		softmac_ps_mgmt_xmit(buf, ieee, MGNT_QUEUE);
#endif

} 
void rtllib_sta_ps_send_qos_null_frame(struct rtllib_device *ieee, short pwr)
{
	
	struct sk_buff *buf = rtllib_null_func(ieee, ieee->current_network.bssid, true, 0, 0, pwr, false);
	cb_desc *tcb_desc = NULL;
	if (buf) {
		tcb_desc = (cb_desc *)(buf->cb + 8);
		if(GET_BE_UAPSD(ieee->b4ac_Uapsd))
			tcb_desc->priority = 3;
		if(GET_BK_UAPSD(ieee->b4ac_Uapsd))
			tcb_desc->priority = 1;
		if(GET_VI_UAPSD(ieee->b4ac_Uapsd))
			tcb_desc->priority = 5;
		if(GET_VO_UAPSD(ieee->b4ac_Uapsd))
			tcb_desc->priority = 7;
#ifdef ASL
		softmac_ps_mgmt_xmit(buf, ieee, BE_QUEUE, false);
#else
		softmac_ps_mgmt_xmit(buf, ieee, BE_QUEUE);
#endif
	}

} 
void rtllib_sta_ps_send_pspoll_frame(struct rtllib_device *ieee)
{
	
	struct sk_buff *buf = rtllib_pspoll_func(ieee);
	
	if (buf)
#ifdef RTL8192CE
#ifdef ASL
		softmac_ps_mgmt_xmit(buf, ieee, MGNT_QUEUE, false);
#else
		softmac_ps_mgmt_xmit(buf, ieee, MGNT_QUEUE);
#endif
#else
#ifdef ASL
		softmac_ps_mgmt_xmit(buf, ieee, HIGH_QUEUE, false);
#else
		softmac_ps_mgmt_xmit(buf, ieee, HIGH_QUEUE);
#endif
#endif

} 

short rtllib_sta_ps_sleep(struct rtllib_device *ieee, u32 *time_h, u32 *time_l)
{	
	int timeout = ieee->ps_timeout;
	u8 dtim;
	PRT_POWER_SAVE_CONTROL	pPSC = (PRT_POWER_SAVE_CONTROL)(&(ieee->PowerSaveControl));
	/*if(ieee->ps == RTLLIB_PS_DISABLED ||
		ieee->iw_mode != IW_MODE_INFRA || 
		ieee->state != RTLLIB_LINKED)
		
		return 0;
	*/

	if(ieee->LPSDelayCnt)
	{
		ieee->LPSDelayCnt --;
		return 0;
	}
		
	dtim = ieee->current_network.dtim_data;
	if(!(dtim & RTLLIB_DTIM_VALID))
		return 0;
	timeout = ieee->current_network.beacon_interval; 
	ieee->current_network.dtim_data = RTLLIB_DTIM_INVALID;
	/* there's no need to nofity AP that I find you buffered with broadcast packet */
	if(dtim & (RTLLIB_DTIM_UCAST & ieee->ps))
		return 2;
	
	if(!time_after(jiffies, ieee->dev->trans_start + MSECS(timeout))){
		return 0;
	}
	if(!time_after(jiffies, ieee->last_rx_ps_time + MSECS(timeout))){
		return 0;
	}
	if((ieee->softmac_features & IEEE_SOFTMAC_SINGLE_QUEUE ) &&
		(ieee->mgmt_queue_tail != ieee->mgmt_queue_head))
		return 0;
	
	if(time_l){
		if(ieee->bAwakePktSent == true) {
			pPSC->LPSAwakeIntvl = 1;
		} else {
			u8		MaxPeriod = 1;

			if(pPSC->LPSAwakeIntvl == 0)
				pPSC->LPSAwakeIntvl = 1;
			if(pPSC->RegMaxLPSAwakeIntvl == 0) 
				MaxPeriod = 1; 
			else if(pPSC->RegMaxLPSAwakeIntvl == 0xFF) 
				MaxPeriod = ieee->current_network.dtim_period;
			else
				MaxPeriod = pPSC->RegMaxLPSAwakeIntvl;
			pPSC->LPSAwakeIntvl = (pPSC->LPSAwakeIntvl >= MaxPeriod) ? MaxPeriod : (pPSC->LPSAwakeIntvl + 1);
		}
		{
			u8 LPSAwakeIntvl_tmp = 0;
			u8 period = ieee->current_network.dtim_period;
			u8 count = ieee->current_network.tim.tim_count;
			if(count == 0 ) {
				if(pPSC->LPSAwakeIntvl > period)
					LPSAwakeIntvl_tmp = period + (pPSC->LPSAwakeIntvl - period) -((pPSC->LPSAwakeIntvl-period)%period);
				else
					LPSAwakeIntvl_tmp = pPSC->LPSAwakeIntvl;

			} else {
				if(pPSC->LPSAwakeIntvl > ieee->current_network.tim.tim_count)
					LPSAwakeIntvl_tmp = count + (pPSC->LPSAwakeIntvl - count) -((pPSC->LPSAwakeIntvl-count)%period);
				else
					LPSAwakeIntvl_tmp = pPSC->LPSAwakeIntvl;
			}
		
		*time_l = ieee->current_network.last_dtim_sta_time[0] 
			+ MSECS(ieee->current_network.beacon_interval * LPSAwakeIntvl_tmp); 
	}
	}
	
	if(time_h){
#ifdef _RTL8192_EXT_PATCH_
		if(time_l && *time_l < ieee->current_network.last_dtim_sta_time[0])
			*time_h =1;
		else
			*time_h =0;
#else
		*time_h = ieee->current_network.last_dtim_sta_time[1];
		if(time_l && *time_l < ieee->current_network.last_dtim_sta_time[0])
			*time_h += 1;
#endif
	}
	
	return 1;
	
	
}
void wmmps_dozecomplete(struct rtllib_device *ieee)
{
	u32	doze_period;
	u32	time_l=0,time_h=0;



	ieee->sta_sleep = LPS_IS_SLEEP;
	doze_period = (ieee->current_network.beacon_interval) * (ieee->current_network.dtim_period);
	time_l = ieee->current_network.last_dtim_sta_time[0]
		                        + MSECS(doze_period);






	ieee->enter_sleep_state(ieee->dev,time_h,time_l);	
}

inline void rtllib_sta_ps(struct rtllib_device *ieee)
{

	u32 th,tl;
	short sleep;
	unsigned long queue_len = 0;
	unsigned long flags,flags2;
	u8 queue_index = 0;
	spin_lock_irqsave(&ieee->lock, flags);
	
	if(ieee->ps == RTLLIB_PS_DISABLED ||
		ieee->iw_mode != IW_MODE_INFRA || 
		ieee->state != RTLLIB_LINKED || (ieee->bin_service_period && ((ieee->b4ac_Uapsd & 0x0f) != 0))){
		
		printk("=====>%s(): no need to ps,wake up!! ieee->ps is %d,ieee->iw_mode is %d,ieee->state is %d,ieee->bin_service_period is %d\n",
			__FUNCTION__,ieee->ps,ieee->iw_mode,ieee->state,ieee->bin_service_period);
		spin_lock_irqsave(&ieee->mgmt_tx_lock, flags2);
		
		rtllib_sta_wakeup(ieee, 1);	
		
		spin_unlock_irqrestore(&ieee->mgmt_tx_lock, flags2);
	}
	for (queue_index=0;queue_index<=VO_QUEUE;queue_index++){
		queue_len = skb_queue_len(&ieee->skb_waitQ[queue_index]);
		if (queue_len) {
			printk("=====>%s(): no need to ps,wake up!! %d queue is not empty\n",
				__FUNCTION__,queue_index);
			spin_lock_irqsave(&ieee->mgmt_tx_lock, flags2);
		
			rtllib_sta_wakeup(ieee, 1);	
	
			spin_unlock_irqrestore(&ieee->mgmt_tx_lock, flags2);
		}
	}
	sleep = rtllib_sta_ps_sleep(ieee,&th, &tl);
	/* 2 wake, 1 sleep, 0 do nothing */
	if(sleep == 0)
	{
		goto out;
	}
	if(sleep == 1){
		if (ieee->b4ac_Uapsd != 0x00 && ieee->bin_service_period)
			goto out;
		if (ieee->b4ac_Uapsd == 0x0f)
			wmmps_dozecomplete(ieee);
		else {
		        if(ieee->sta_sleep == LPS_IS_SLEEP){
			        ieee->enter_sleep_state(ieee->dev,th,tl);
		        }
		
		        else if(ieee->sta_sleep == LPS_IS_WAKE){
			        spin_lock_irqsave(&ieee->mgmt_tx_lock, flags2);
			
			        if(ieee->ps_is_queue_empty(ieee->dev)){
				        ieee->sta_sleep = LPS_WAIT_NULL_DATA_SEND;
				        ieee->ack_tx_to_ieee = 1;
				        rtllib_sta_ps_send_null_frame(ieee,1);
				        ieee->ps_th = th;
				        ieee->ps_tl = tl;
			        }		
			        spin_unlock_irqrestore(&ieee->mgmt_tx_lock, flags2);
		        }
		}
		ieee->bAwakePktSent = false;
	}else if(sleep == 2){
		spin_lock_irqsave(&ieee->mgmt_tx_lock, flags2);
			
		rtllib_sta_wakeup(ieee,1);
		
		spin_unlock_irqrestore(&ieee->mgmt_tx_lock, flags2);
	}

out:	
	spin_unlock_irqrestore(&ieee->lock, flags);
	
}

void rtllib_sta_wakeup(struct rtllib_device *ieee, short nl)
{
	if(ieee->sta_sleep == LPS_IS_WAKE){
		if(nl){
			if (ieee->b4ac_Uapsd == 0x0f) {
				rtllib_sta_ps_send_qos_null_frame(ieee, true);
			} else {
			    if(ieee->pHTInfo->IOTAction & HT_IOT_ACT_NULL_DATA_POWER_SAVING)
			    {
				ieee->ack_tx_to_ieee = 1;
				rtllib_sta_ps_send_null_frame(ieee, 0);
			    }
			    else 
			    {
				ieee->ack_tx_to_ieee = 1;
				rtllib_sta_ps_send_pspoll_frame(ieee);
			    }
			}
		}
		return;
		
	}
	
	if(ieee->sta_sleep == LPS_IS_SLEEP) 
		ieee->sta_wake_up(ieee->dev);
	if(nl){
		/*		
			ieee->ack_tx_to_ieee = 1;
			printk("%s(3): notify AP we are awaked ++++++++++ SendNullFunctionData\n", __func__);
			rtllib_sta_ps_send_null_frame(ieee, 0);
		*/
		if (ieee->b4ac_Uapsd == 0x0f) {
			rtllib_sta_ps_send_qos_null_frame(ieee, true);
			printk("%s(3): notify AP we are awaked ++++++++++ SendNullFunctionData\n", __func__);
		} else {
		    if(ieee->pHTInfo->IOTAction & HT_IOT_ACT_NULL_DATA_POWER_SAVING)
		    {
			ieee->ack_tx_to_ieee = 1;
			rtllib_sta_ps_send_null_frame(ieee, 0);
		    }
		    else 
		    {
			ieee->ack_tx_to_ieee = 1;
			ieee->polling = true;
			rtllib_sta_ps_send_pspoll_frame(ieee);
		    }
		}
	} else {
		ieee->sta_sleep = LPS_IS_WAKE;
		ieee->polling = false;
	}
}

void rtllib_ps_tx_ack(struct rtllib_device *ieee, short success)
{
	unsigned long flags,flags2;
	
	spin_lock_irqsave(&ieee->lock, flags);
	
	if(ieee->sta_sleep == LPS_WAIT_NULL_DATA_SEND){
		/* Null frame with PS bit set */
		if(success){
			ieee->sta_sleep = LPS_IS_SLEEP;
			ieee->enter_sleep_state(ieee->dev,ieee->ps_th,ieee->ps_tl);
		}
		/* if the card report not success we can't be sure the AP
		 * has not RXed so we can't assume the AP believe us awake
		 */
	} else {/* 21112005 - tx again null without PS bit if lost */
	
		if((ieee->sta_sleep == LPS_IS_WAKE) && !success){
			spin_lock_irqsave(&ieee->mgmt_tx_lock, flags2);
			if(ieee->pHTInfo->IOTAction & HT_IOT_ACT_NULL_DATA_POWER_SAVING)
			{
				rtllib_sta_ps_send_null_frame(ieee, 0);
			}
			else 
			{
				rtllib_sta_ps_send_pspoll_frame(ieee);
			}
			spin_unlock_irqrestore(&ieee->mgmt_tx_lock, flags2);
		}
	}
	spin_unlock_irqrestore(&ieee->lock, flags);
}

void rtllib_process_action(struct rtllib_device* ieee, struct sk_buff* skb)
{
	struct rtllib_hdr_3addr *header = (struct rtllib_hdr_3addr *) skb->data;
	u8* act = rtllib_get_payload((struct rtllib_hdr *)header);
	u8 category = 0;

	if (act == NULL) {
		RTLLIB_DEBUG(RTLLIB_DL_ERR, "error to get payload of action frame\n");
		return;
	}

	category = *act;
	act ++;
	switch (category) {
		case ACT_CAT_BA:
			switch (*act) {
				case ACT_ADDBAREQ:
					rtllib_rx_ADDBAReq(ieee, skb);
					break;
				case ACT_ADDBARSP:
					rtllib_rx_ADDBARsp(ieee, skb);
					break;
				case ACT_DELBA:
					rtllib_rx_DELBA(ieee, skb);
					break;
			}
			break;
		default:
			break;
	}
	return;
}

#ifdef _RTL8192_EXT_PATCH_      
#define GetAddr2Ptr(pbuf)       ((u8 *)((unsigned int)(pbuf) + 10))
extern u8 mesh_acl_query(struct rtllib_device *ieee, u8 *sa) ;
void rtllib_process_action_mesh(struct rtllib_device* ieee, struct sk_buff* skb,
		struct rtllib_rx_stats *rx_stats)
{
	struct rtllib_hdr_3addr *header = NULL;
	u8* act = NULL;
	u8 category = 0;

	/* filter the deny mac address */
	u8 * sa = GetAddr2Ptr(skb->data); 
	if(1 != mesh_acl_query(ieee, sa)){
		printk("%s: sa("MAC_FMT") is denied!!!\n", __FUNCTION__, MAC_ARG(sa));
		return;
	}

#ifdef COMPATIBLE_WITH_RALINK_MESH	
	memmove(skb->data + WLAN_MESH_HDR_LEN, skb->data, sizeof(struct rtllib_hdr_3addr));
	skb_pull(skb, WLAN_MESH_HDR_LEN);
#endif
	
	header = (struct rtllib_hdr_3addr *) skb->data;
	act = rtllib_get_payload((struct rtllib_hdr *)header);

	if (act == NULL) {
		RTLLIB_DEBUG(RTLLIB_DL_ERR, "error to get payload of action frame\n");
		return;
	}
	if (ieee->iw_mode != IW_MODE_MESH) {
		RTLLIB_DEBUG(RTLLIB_DL_ERR, "not in mesh mode\n");
		return;
	}

	category = *act;
	act ++;
	switch (category) {
		case ACT_CAT_MESH_PEERLINK_MGNT:
			if(ieee->iw_mode != IW_MODE_MESH) {
				RTLLIB_DEBUG(RTLLIB_DL_ERR, "peerlink received not in mesh mode\n");
				return;
			}
			switch (*act) {
				case ACT_PEERLINK_OPEN:
					if (ieee->ext_patch_rtllib_rx_frame_softmac_on_peerlink_open) {
						ieee->ext_patch_rtllib_rx_frame_softmac_on_peerlink_open(ieee, skb, rx_stats);
					}
					break;
				case ACT_PEERLINK_CONFIRM:
					if (ieee->ext_patch_rtllib_rx_frame_softmac_on_peerlink_confirm) {
						ieee->ext_patch_rtllib_rx_frame_softmac_on_peerlink_confirm(ieee, skb);
					}
					break;
				case ACT_PEERLINK_CLOSE:
					if ( ieee->ext_patch_rtllib_rx_frame_softmac_on_peerlink_close) {
						ieee->ext_patch_rtllib_rx_frame_softmac_on_peerlink_close(ieee, skb);
					}
					break;
			}
			break;
		case ACT_CAT_MESH_LINK_METRIC:
			if (ieee->iw_mode != IW_MODE_MESH) {
				RTLLIB_DEBUG(RTLLIB_DL_ERR, "link metric received not in mesh mode\n");
				return;
			}
			switch (*act) {
				case ACT_LINKMETRIC_REQ:
					if (ieee->ext_patch_rtllib_rx_frame_softmac_on_linkmetric_req) {
						ieee->ext_patch_rtllib_rx_frame_softmac_on_linkmetric_req(
								ieee, skb);
					}
					break;
				case ACT_LINKMETRIC_RSP:
					if (ieee->ext_patch_rtllib_rx_frame_softmac_on_linkmetric_report) {
						ieee->ext_patch_rtllib_rx_frame_softmac_on_linkmetric_report(
								ieee, skb);
					}
					break;
			}

			break;
		case ACT_CAT_MESH_PATH_SELECT:
			if (ieee->iw_mode != IW_MODE_MESH) {
				RTLLIB_DEBUG(RTLLIB_DL_ERR, "not in mesh mode\n");
				return;
			}

			switch (*act) {
				case ACT_PATH_REQ:
					if( ieee->ext_patch_rtllib_rx_frame_softmac_on_pathselect_preq) {
						ieee->ext_patch_rtllib_rx_frame_softmac_on_pathselect_preq(
								ieee, skb, rx_stats);
					}
					break;
				case ACT_PATH_REPLY:
					if(ieee->ext_patch_rtllib_rx_frame_softmac_on_pathselect_prep) {
						ieee->ext_patch_rtllib_rx_frame_softmac_on_pathselect_prep(
								ieee, skb, rx_stats);
					}
					break;
				case ACT_PATH_ERR:
					if(ieee->ext_patch_rtllib_rx_frame_softmac_on_pathselect_perr) {
						ieee->ext_patch_rtllib_rx_frame_softmac_on_pathselect_perr(
								ieee, skb, rx_stats);
					}
					break;
				case ACT_RANN:
					if(ieee->ext_patch_rtllib_rx_frame_softmac_on_pathselect_rann) {
						ieee->ext_patch_rtllib_rx_frame_softmac_on_pathselect_rann(
								ieee, skb, rx_stats);
					}
					break;
			}
			break;
		case ACT_CAT_MESH_INTERWORKING:
			if (ieee->iw_mode != IW_MODE_MESH) {
				RTLLIB_DEBUG(RTLLIB_DL_ERR, "not in mesh mode\n");
				return;
			}
			if (*act == 0) {
				if (ieee->ext_patch_rtllib_rx_frame_softmac_on_pathselect_pann) {
					ieee->ext_patch_rtllib_rx_frame_softmac_on_pathselect_pann(
							ieee, skb, rx_stats);
				}
			}
			break;
		default:
			break;
	}
	return;
}
#endif


inline int rtllib_rx_assoc_resp(struct rtllib_device *ieee, struct sk_buff *skb, struct rtllib_rx_stats *rx_stats)
{
	u16 errcode;
	int aid;
	u8* ies;
	struct rtllib_assoc_response_frame *assoc_resp;
	struct rtllib_hdr_3addr *header = (struct rtllib_hdr_3addr *) skb->data;

	RTLLIB_DEBUG_MGMT("received [RE]ASSOCIATION RESPONSE (%d)\n",
					WLAN_FC_GET_STYPE(header->frame_ctl));
			
	if ((ieee->softmac_features & IEEE_SOFTMAC_ASSOCIATE) &&
		ieee->state == RTLLIB_ASSOCIATING_AUTHENTICATED && 
#ifdef _RTL8192_EXT_PATCH_
		((ieee->iw_mode == IW_MODE_INFRA) || 
		(ieee->iw_mode == IW_MODE_MESH && ieee->only_mesh == 0)))
#else
#ifdef ASL
		(ieee->iw_mode == IW_MODE_INFRA || ieee->iw_mode == IW_MODE_APSTA))
#else
		(ieee->iw_mode == IW_MODE_INFRA)) 
#endif
#endif
	{
		if (0 == (errcode=assoc_parse(ieee,skb, &aid))){
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,13))
			struct rtllib_network *network = kzalloc(sizeof(struct rtllib_network), GFP_ATOMIC);
#else
			struct rtllib_network *network = kmalloc(sizeof(*network), GFP_ATOMIC);
#endif
			if (!network) {
				return 1;
			}
			memset(network,0,sizeof(*network));
			ieee->state=RTLLIB_LINKED;
			ieee->assoc_id = aid;
			ieee->softmac_stats.rx_ass_ok++;
			/* station support qos */
			/* Let the register setting defaultly with Legacy station */
			assoc_resp = (struct rtllib_assoc_response_frame*)skb->data;
			if(ieee->current_network.qos_data.supported == 1) {
				if (rtllib_parse_info_param(ieee,assoc_resp->info_element,\
							rx_stats->len - sizeof(*assoc_resp),\
							network,rx_stats)){
					kfree(network);
					return 1;
				}
				else
				{	
					memcpy(ieee->pHTInfo->PeerHTCapBuf, network->bssht.bdHTCapBuf, network->bssht.bdHTCapLen);
					memcpy(ieee->pHTInfo->PeerHTInfoBuf, network->bssht.bdHTInfoBuf, network->bssht.bdHTInfoLen);
				}
				if (ieee->handle_assoc_response != NULL)
					ieee->handle_assoc_response(ieee->dev, (struct rtllib_assoc_response_frame*)header, network);
				kfree(network);
			}

			if(ieee->assocresp_ies){
				kfree(ieee->assocresp_ies);
				ieee->assocresp_ies = NULL;
			}
			ies = &(assoc_resp->info_element[0].id);
			ieee->assocresp_ies_len = (skb->data + skb->len) - ies;
			ieee->assocresp_ies = kmalloc(ieee->assocresp_ies_len, GFP_ATOMIC);
			if (ieee->assocresp_ies)
				memcpy(ieee->assocresp_ies, ies, ieee->assocresp_ies_len);
			else{
				printk("%s()Warning: can't alloc memory for assocresp_ies\n", __func__);
				ieee->assocresp_ies_len = 0;
			}	
#ifdef RTL8192S_WAPI_SUPPORT
			{
				PRT_WAPI_T pWapiInfo = &(ieee->wapiInfo);
				PRT_WAPI_STA_INFO pWapiSta;
				u8 WapiAEPNInitialValueSrc[16] = {0x37,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C} ;
				u8 WapiAEMultiCastPNInitialValueSrc[16] = {0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C} ;	
				if ((ieee->WapiSupport) && (ieee->wapiInfo.bWapiEnable))
				{
					pWapiSta =(PRT_WAPI_STA_INFO)list_entry(pWapiInfo->wapiSTAIdleList.next, RT_WAPI_STA_INFO, list);
					list_del_init(&pWapiSta->list);
					list_add_tail(&pWapiSta->list, &pWapiInfo->wapiSTAUsedList);
					WAPI_TRACE(WAPI_API, "%s(): New WAPI STA ("MAC_FMT")!!\n",__FUNCTION__,MAC_ARG(ieee->current_network.bssid));
					memcpy(pWapiSta->PeerMacAddr,ieee->current_network.bssid,6);
					memcpy(pWapiSta->lastRxMulticastPN, WapiAEMultiCastPNInitialValueSrc, 16);
					memcpy(pWapiSta->lastRxUnicastPN, WapiAEPNInitialValueSrc, 16);
					pWapiInfo->bFirstAuthentiateInProgress= true;
					pWapiInfo->assoRspWapiIE[0] = MFIE_TYPE_WAPI;
					pWapiInfo->assoRspWapiIE[1] = network->wapi_ie_len;
					pWapiInfo->assoRspWapiIELength = network->wapi_ie_len+2;
					memcpy(pWapiInfo->assoRspWapiIE+2,network->wapi_ie,network->wapi_ie_len);
				}
			}
#endif	
			rtllib_associate_complete(ieee);
		} else {
			/* aid could not been allocated */
			ieee->softmac_stats.rx_ass_err++;
			printk(
				"Association response status code 0x%x\n",
				errcode);
			RTLLIB_DEBUG_MGMT(
				"Association response status code 0x%x\n",
				errcode);
			if(ieee->AsocRetryCount < RT_ASOC_RETRY_LIMIT) {
				queue_delayed_work_rsl(ieee->wq, &ieee->associate_procedure_wq, 0);
			} else {
				rtllib_associate_abort(ieee); 
			}
		}
	}

	return 0;
}

inline int rtllib_rx_auth(struct rtllib_device *ieee, struct sk_buff *skb, struct rtllib_rx_stats *rx_stats)
{
#ifdef _RTL8192_EXT_PATCH_
	struct rtllib_hdr_3addr *header = (struct rtllib_hdr_3addr *) skb->data;
	u8 mesh_bssid[6] = {0,0,0,0,0,0};
#endif
	u16 errcode;
	u8* challenge;
	int chlen=0;
	bool bSupportNmode = true, bHalfSupportNmode = false; 

#ifdef _RTL8192_EXT_PATCH_
	if((ieee->iw_mode == IW_MODE_MESH) && ieee->ext_patch_rtllib_rx_frame_softmac_on_auth && ieee->mesh_started && ((memcmp(header->addr3, ieee->current_network.bssid, 6)) || (memcmp(ieee->current_network.bssid, mesh_bssid, 6) == 0))) 
		ieee->ext_patch_rtllib_rx_frame_softmac_on_auth(ieee, skb, rx_stats);
	else
#endif
	if (ieee->softmac_features & IEEE_SOFTMAC_ASSOCIATE){
		if (ieee->state == RTLLIB_ASSOCIATING_AUTHENTICATING && 
#ifdef _RTL8192_EXT_PATCH_
			((ieee->iw_mode == IW_MODE_INFRA)||
			((ieee->iw_mode == IW_MODE_MESH) && (ieee->only_mesh == 0))))
#else
#ifdef ASL
			(ieee->iw_mode == IW_MODE_INFRA || ieee->iw_mode == IW_MODE_APSTA))
#else
			(ieee->iw_mode == IW_MODE_INFRA))
#endif
#endif
		{
			RTLLIB_DEBUG_MGMT("Received authentication response");
			
			if (0 == (errcode=auth_parse(skb, &challenge, &chlen))){
				if(ieee->open_wep || !challenge){
					ieee->state = RTLLIB_ASSOCIATING_AUTHENTICATED;
					ieee->softmac_stats.rx_auth_rs_ok++;
					if(!(ieee->pHTInfo->IOTAction&HT_IOT_ACT_PURE_N_MODE))
					{
						if (!ieee->GetNmodeSupportBySecCfg(ieee->dev))
						{
							if(IsHTHalfNmodeAPs(ieee))
							{	
								bSupportNmode = true;
								bHalfSupportNmode = true;
							}
							else
							{
								bSupportNmode = false;
								bHalfSupportNmode = false;
							}
						}
					}								
					/* Dummy wirless mode setting to avoid encryption issue */
					if(bSupportNmode) {
#ifdef ASL
						ieee->SetWirelessMode(ieee->dev, \
							ieee->current_network.mode, false);
#else
						ieee->SetWirelessMode(ieee->dev, \
							ieee->current_network.mode);
#endif
					}else{
						/*TODO*/
#ifdef ASL
						ieee->SetWirelessMode(ieee->dev, IEEE_G, false);
#else
						ieee->SetWirelessMode(ieee->dev, IEEE_G);
#endif
					}

					if (ieee->current_network.mode == IEEE_N_24G && bHalfSupportNmode == true)
					{
						printk("===============>entern half N mode\n");
						ieee->bHalfWirelessN24GMode = true;
					}
					else
						ieee->bHalfWirelessN24GMode = false;
					
					rtllib_associate_step2(ieee);
				}else{
					rtllib_auth_challenge(ieee, challenge, chlen);
				}
			}else{
				ieee->softmac_stats.rx_auth_rs_err++;
				RTLLIB_DEBUG_MGMT("Authentication respose status code 0x%x",errcode);

				printk("Authentication respose status code 0x%x",errcode);
				rtllib_associate_abort(ieee);
			}
				
		}
		else if (ieee->iw_mode == IW_MODE_MASTER
#ifdef ASL
			|| ieee->iw_mode == IW_MODE_APSTA
#endif
			)
		{
			printk("========>rx auth rq??");
			dump_buf(skb->data,skb->len);
			rtllib_rx_auth_rq(ieee, skb);
		}
	}

	return 0;
}

inline int rtllib_rx_deauth(struct rtllib_device *ieee, struct sk_buff *skb)
{
	struct rtllib_hdr_3addr *header = (struct rtllib_hdr_3addr *) skb->data;
	
	if(memcmp(header->addr3, ieee->current_network.bssid, ETH_ALEN) != 0)
		return 0;
	
	/* FIXME for now repeat all the association procedure 
	* both for disassociation and deauthentication
	*/
	if ((ieee->softmac_features & IEEE_SOFTMAC_ASSOCIATE) &&
		ieee->state == RTLLIB_LINKED && 
#ifdef _RTL8192_EXT_PATCH_
		((ieee->iw_mode == IW_MODE_INFRA) ||
		((ieee->iw_mode == IW_MODE_MESH) && (ieee->only_mesh == 0))))
#else
#ifdef ASL
		(ieee->iw_mode == IW_MODE_INFRA || ieee->iw_mode == IW_MODE_APSTA))
#else
		(ieee->iw_mode == IW_MODE_INFRA))
#endif
#endif
	{
		printk("==========>received disassoc/deauth(%x) frame, reason code:%x\n",WLAN_FC_GET_STYPE(header->frame_ctl), ((struct rtllib_disassoc*)skb->data)->reason);	
		ieee->state = RTLLIB_ASSOCIATING;
		ieee->softmac_stats.reassoc++;
		ieee->is_roaming = true;
		ieee->LinkDetectInfo.bBusyTraffic = false;
		rtllib_disassociate(ieee);
		RemovePeerTS(ieee, header->addr2); 	
		if(ieee->LedControlHandler != NULL)
		        ieee->LedControlHandler(ieee->dev, LED_CTL_START_TO_LINK); 

		if(!(ieee->rtllib_ap_sec_type(ieee)&(SEC_ALG_CCMP|SEC_ALG_TKIP)))
		queue_delayed_work_rsl(ieee->wq, &ieee->associate_procedure_wq, 5);
	}

	return 0;
}

inline int rtllib_rx_frame_softmac(struct rtllib_device *ieee, struct sk_buff *skb,
			struct rtllib_rx_stats *rx_stats, u16 type,
			u16 stype)
{
	struct rtllib_hdr_3addr *header = (struct rtllib_hdr_3addr *) skb->data;

#ifdef _RTL8192_EXT_PATCH_
	if((!ieee->proto_started)&&(!ieee->mesh_started))
#else
#ifdef ASL
	if((!ieee->proto_started) && (!ieee->ap_proto_started))
#else
	if(!ieee->proto_started)
#endif
#endif
		return 0;
		
	switch (WLAN_FC_GET_STYPE(header->frame_ctl)) {
	
		case RTLLIB_STYPE_ASSOC_RESP:
		case RTLLIB_STYPE_REASSOC_RESP:

			if(rtllib_rx_assoc_resp(ieee, skb, rx_stats) == 1)
				return 1;

			break;
		
		case RTLLIB_STYPE_ASSOC_REQ:
		case RTLLIB_STYPE_REASSOC_REQ:
		
			if ((ieee->softmac_features & IEEE_SOFTMAC_ASSOCIATE) &&
				(ieee->iw_mode == IW_MODE_MASTER
#ifdef ASL
				|| ieee->iw_mode == IW_MODE_APSTA
#endif
				))
					
				rtllib_rx_assoc_rq(ieee, skb);
			break;
			
		case RTLLIB_STYPE_AUTH:

			rtllib_rx_auth(ieee, skb, rx_stats);

			break;
#if 0				
		case RTLLIB_STYPE_PROBE_REQ:

			if ((ieee->softmac_features & IEEE_SOFTMAC_PROBERS) && 
				((ieee->iw_mode == IW_MODE_ADHOC || 
				ieee->iw_mode == IW_MODE_MASTER) &&
				ieee->state == RTLLIB_LINKED)){
				rtllib_rx_probe_rq(ieee, skb);
			}
			break;
#endif			
		case RTLLIB_STYPE_DISASSOC:
		case RTLLIB_STYPE_DEAUTH:

			rtllib_rx_deauth(ieee, skb);
			
			break;
			
		case RTLLIB_STYPE_MANAGE_ACT:
			rtllib_process_action(ieee,skb);
#ifdef _RTL8192_EXT_PATCH_
			rtllib_process_action_mesh(ieee,skb,rx_stats);
#endif
			break;
#ifdef COMPATIBLE_WITH_RALINK_MESH
		case RTLLIB_STYPE_MESH_ACT:
			rtllib_process_action_mesh(ieee,skb,rx_stats);
			break;
#endif			
		default: 
			return -1;
			break;
	}
	
	return 0;
}

/* following are for a simplier TX queue management.
 * Instead of using netif_[stop/wake]_queue the driver
 * will uses these two function (plus a reset one), that
 * will internally uses the kernel netif_* and takes
 * care of the ieee802.11 fragmentation.
 * So the driver receives a fragment per time and might
 * call the stop function when it want without take care
 * to have enought room to TX an entire packet.
 * This might be useful if each fragment need it's own
 * descriptor, thus just keep a total free memory > than
 * the max fragmentation treshold is not enought.. If the
 * ieee802.11 stack passed a TXB struct then you needed  
 * to keep N free descriptors where 
 * N = MAX_PACKET_SIZE / MIN_FRAG_TRESHOLD
 * In this way you need just one and the 802.11 stack
 * will take care of buffering fragments and pass them to 
 * to the driver later, when it wakes the queue.
 */ 
void rtllib_softmac_xmit(struct rtllib_txb *txb, struct rtllib_device *ieee)
{

	unsigned int queue_index = txb->queue_index;
	unsigned long flags;
	int  i;
	cb_desc *tcb_desc = NULL;
	unsigned long queue_len = 0;

	spin_lock_irqsave(&ieee->lock,flags);

	/* called with 2nd parm 0, no tx mgmt lock required */
	rtllib_sta_wakeup(ieee,0);

	/* update the tx status */
	tcb_desc = (cb_desc *)(txb->fragments[0]->cb + MAX_DEV_ADDR_SIZE);
	if(tcb_desc->bMulticast) {
		ieee->stats.multicast++;
	}
#if 1
	/* if xmit available, just xmit it immediately, else just insert it to the wait queue */
	for(i = 0; i < txb->nr_frags; i++) {
#ifdef USB_TX_DRIVER_AGGREGATION_ENABLE
		queue_len = skb_queue_len(&ieee->skb_drv_aggQ[queue_index]);
#else
		queue_len = skb_queue_len(&ieee->skb_waitQ[queue_index]);
#endif
		if((queue_len  != 0) ||\
			(!ieee->check_nic_enough_desc(ieee->dev,queue_index))||\
		       (ieee->queue_stop)) {
			/* insert the skb packet to the wait queue */
			/* as for the completion function, it does not need 
			 * to check it any more.
			 * */
#ifdef WIFI_TEST
			if (1) 
#else
			if(queue_len < 200)
#endif
			{
#ifdef USB_TX_DRIVER_AGGREGATION_ENABLE
				skb_queue_tail(&ieee->skb_drv_aggQ[queue_index], txb->fragments[i]);
#else
				skb_queue_tail(&ieee->skb_waitQ[queue_index], txb->fragments[i]);
#endif
			}else{
				kfree_skb(txb->fragments[i]);
			}
		}else{
			ieee->softmac_data_hard_start_xmit(
					txb->fragments[i],
					ieee->dev,ieee->rate);
		}
	}	
#endif
	rtllib_txb_free(txb);

	spin_unlock_irqrestore(&ieee->lock,flags);

}

/* called with ieee->lock acquired */
void rtllib_resume_tx(struct rtllib_device *ieee)
{
	int i;
	for(i = ieee->tx_pending.frag; i < ieee->tx_pending.txb->nr_frags; i++) {
		
		if (ieee->queue_stop){
			ieee->tx_pending.frag = i;
			return;
		}else{
		
			ieee->softmac_data_hard_start_xmit( 
				ieee->tx_pending.txb->fragments[i],
				ieee->dev,ieee->rate);
			ieee->stats.tx_packets++;
		}
	}
	
	rtllib_txb_free(ieee->tx_pending.txb);
	ieee->tx_pending.txb = NULL;
}


void rtllib_reset_queue(struct rtllib_device *ieee)
{
	unsigned long flags;
	
	spin_lock_irqsave(&ieee->lock,flags);
	init_mgmt_queue(ieee);
	if (ieee->tx_pending.txb){
		rtllib_txb_free(ieee->tx_pending.txb);
		ieee->tx_pending.txb = NULL;
	}
	ieee->queue_stop = 0;
	spin_unlock_irqrestore(&ieee->lock,flags);

}

void rtllib_wake_queue(struct rtllib_device *ieee)
{

	unsigned long flags;
	struct sk_buff *skb;
	struct rtllib_hdr_3addr  *header;
	
	spin_lock_irqsave(&ieee->lock,flags);
	if (! ieee->queue_stop) goto exit;
	
	ieee->queue_stop = 0;
	
	if(ieee->softmac_features & IEEE_SOFTMAC_SINGLE_QUEUE){
		while (!ieee->queue_stop && (skb = dequeue_mgmt(ieee))){
			
			header = (struct rtllib_hdr_3addr  *) skb->data;
			
			header->seq_ctl = cpu_to_le16(ieee->seq_ctrl[0] << 4);

			if (ieee->seq_ctrl[0] == 0xFFF)
				ieee->seq_ctrl[0] = 0;
			else
				ieee->seq_ctrl[0]++;

			ieee->softmac_data_hard_start_xmit(skb,ieee->dev,ieee->basic_rate);
		}
	}
	if (!ieee->queue_stop && ieee->tx_pending.txb)
		rtllib_resume_tx(ieee);
	
	if (!ieee->queue_stop && netif_queue_stopped(ieee->dev)){
		ieee->softmac_stats.swtxawake++;
		netif_wake_queue(ieee->dev);
	}
	
exit :
	spin_unlock_irqrestore(&ieee->lock,flags);
}


void rtllib_stop_queue(struct rtllib_device *ieee)
{

	if (! netif_queue_stopped(ieee->dev)){
		netif_stop_queue(ieee->dev);
		ieee->softmac_stats.swtxstop++;
	}
	ieee->queue_stop = 1;
	
}

void rtllib_stop_all_queues(struct rtllib_device *ieee)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
	unsigned int i;
	for(i=0; i < ieee->dev->num_tx_queues; i++)
                netdev_get_tx_queue(ieee->dev,i)->trans_start = jiffies;
#else
	ieee->dev->trans_start = jiffies;
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27) 
	netif_carrier_off(ieee->dev);
#else
	netif_tx_stop_all_queues(ieee->dev);
#endif
}

void rtllib_wake_all_queues(struct rtllib_device *ieee)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27) 
	netif_carrier_on(ieee->dev);
#else
	netif_tx_wake_all_queues(ieee->dev);
#endif
}

inline void rtllib_randomize_cell(struct rtllib_device *ieee)
{
	
	get_random_bytes(ieee->current_network.bssid, ETH_ALEN);
	
	/* an IBSS cell address must have the two less significant
	 * bits of the first byte = 2 
	 */
	ieee->current_network.bssid[0] &= ~0x01;
	ieee->current_network.bssid[0] |= 0x02;
}

/* called in user context only */
void rtllib_start_master_bss(struct rtllib_device *ieee)
{
	struct rtllib_network* net = NULL;
#ifdef ASL
	net = &ieee->current_ap_network;
#else
	net = &ieee->current_network;
#endif
	if (net->ssid_len == 0){
		strncpy(net->ssid, 
			RTLLIB_DEFAULT_TX_ESSID,
			IW_ESSID_MAX_SIZE);
			
		net->ssid_len = strlen(RTLLIB_DEFAULT_TX_ESSID);
#ifdef ASL
		ieee->ap_ssid_set = 1;
#else
		ieee->ssid_set = 1;
#endif
	}
	memcpy(net->bssid, ieee->dev->dev_addr, ETH_ALEN);
#ifdef ASL
	printk("ieee->current_ap_network.bssid is "MAC_FMT"\n",MAC_ARG(ieee->current_ap_network.bssid));
	printk("ieee->current_network.bssid is "MAC_FMT"\n",MAC_ARG(ieee->current_network.bssid));
#endif
	if(ieee->modulation & RTLLIB_CCK_MODULATION){

		net->rates_len = 4;
		
		net->rates[0] = RTLLIB_BASIC_RATE_MASK | RTLLIB_CCK_RATE_1MB;
		net->rates[1] = RTLLIB_BASIC_RATE_MASK | RTLLIB_CCK_RATE_2MB;
		net->rates[2] = RTLLIB_BASIC_RATE_MASK | RTLLIB_CCK_RATE_5MB;
		net->rates[3] = RTLLIB_BASIC_RATE_MASK | RTLLIB_CCK_RATE_11MB;
			
	}else
		net->rates_len = 0;
	
	if(ieee->modulation & RTLLIB_OFDM_MODULATION){
		net->rates_ex_len = 8;

		net->rates_ex[0] = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_6MB;
		net->rates_ex[1] = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_9MB;
		net->rates_ex[2] = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_12MB;
		net->rates_ex[3] = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_18MB;
		net->rates_ex[4] = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_24MB;
		net->rates_ex[5] = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_36MB;
		net->rates_ex[6] = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_48MB;
		net->rates_ex[7] = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_54MB;
#ifdef ASL
		ieee->ap_rate = 540;
#else
		ieee->rate = 540;
#endif
	}else{
		net->rates_ex_len = 0;
#ifdef ASL
		ieee->ap_rate = 110;
#else
		ieee->rate = 110;
#endif
	}

	net->capability = WLAN_CAPABILITY_ESS;
#ifndef ASL
	ieee->set_chan(ieee->dev, net->channel);
#endif
#ifdef ASL
	ieee->ap_state = RTLLIB_LINKED;
	ieee->ap_link_change(ieee->dev);
	apdev_notify_wx_assoc_event(ieee);
#else
	ieee->state = RTLLIB_LINKED;
	ieee->link_change(ieee->dev);
	notify_wx_assoc_event(ieee);
#endif
	
#ifdef TO_DO_LIST
	if (ieee->data_hard_resume)
		ieee->data_hard_resume(ieee->dev);
#endif
#ifdef ASL	
	netif_carrier_on(ieee->apdev);
#else
	netif_carrier_on(ieee->dev);
#endif
}

void rtllib_start_monitor_mode(struct rtllib_device *ieee)
{
	/* reset hardware status */
	if(ieee->raw_tx){
		if (ieee->data_hard_resume)
			ieee->data_hard_resume(ieee->dev);
	
		netif_carrier_on(ieee->dev);
	}
}

void rtllib_start_ibss_wq(void *data)
{
	struct rtllib_device *ieee = container_of_dwork_rsl(data, struct rtllib_device, start_ibss_wq);
	/* iwconfig mode ad-hoc will schedule this and return
	 * on the other hand this will block further iwconfig SET
	 * operations because of the wx_sem hold.
	 * Anyway some most set operations set a flag to speed-up
	 * (abort) this wq (when syncro scanning) before sleeping 
	 * on the semaphore
	 */
	if(!ieee->proto_started){
		printk("==========oh driver down return\n");
		return;
	}
	down(&ieee->wx_sem);
	
	if (ieee->current_network.ssid_len == 0){
		strcpy(ieee->current_network.ssid,RTLLIB_DEFAULT_TX_ESSID);
		ieee->current_network.ssid_len = strlen(RTLLIB_DEFAULT_TX_ESSID);
		ieee->ssid_set = 1;
	} 
	
	ieee->state = RTLLIB_NOLINK;
#ifdef ADHOC_11N
	ieee->mode = IEEE_N_24G;
#else
	ieee->mode = IEEE_G;
#endif
	/* check if we have this cell in our network list */
	rtllib_softmac_check_all_nets(ieee);
	

	/* if not then the state is not linked. Maybe the user swithced to
	 * ad-hoc mode just after being in monitor mode, or just after
	 * being very few time in managed mode (so the card have had no
	 * time to scan all the chans..) or we have just run up the iface
	 * after setting ad-hoc mode. So we have to give another try..
	 * Here, in ibss mode, should be safe to do this without extra care
	 * (in bss mode we had to make sure no-one tryed to associate when
	 * we had just checked the ieee->state and we was going to start the
	 * scan) beacause in ibss mode the rtllib_new_net function, when
	 * finds a good net, just set the ieee->state to RTLLIB_LINKED,
	 * so, at worst, we waste a bit of time to initiate an unneeded syncro
	 * scan, that will stop at the first round because it sees the state
	 * associated.
	 */
	if (ieee->state == RTLLIB_NOLINK)
		rtllib_start_scan_syncro(ieee, 0);

	/* the network definitively is not here.. create a new cell */
	if (ieee->state == RTLLIB_NOLINK){
		printk("creating new IBSS cell\n"); 
		ieee->current_network.channel = ieee->IbssStartChnl;
		if(!ieee->wap_set)
			rtllib_randomize_cell(ieee);
		
		if(ieee->modulation & RTLLIB_CCK_MODULATION){
		
			ieee->current_network.rates_len = 4;
			
			ieee->current_network.rates[0] = RTLLIB_BASIC_RATE_MASK | RTLLIB_CCK_RATE_1MB;
			ieee->current_network.rates[1] = RTLLIB_BASIC_RATE_MASK | RTLLIB_CCK_RATE_2MB;
			ieee->current_network.rates[2] = RTLLIB_BASIC_RATE_MASK | RTLLIB_CCK_RATE_5MB;
			ieee->current_network.rates[3] = RTLLIB_BASIC_RATE_MASK | RTLLIB_CCK_RATE_11MB;
				
		}else
			ieee->current_network.rates_len = 0;
		
		if(ieee->modulation & RTLLIB_OFDM_MODULATION){
			ieee->current_network.rates_ex_len = 8;
			
			/*ieee->current_network.rates_ex[0] = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_6MB;
			ieee->current_network.rates_ex[1] = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_9MB;
			ieee->current_network.rates_ex[2] = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_12MB;
			ieee->current_network.rates_ex[3] = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_18MB;
			ieee->current_network.rates_ex[4] = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_24MB;
			ieee->current_network.rates_ex[5] = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_36MB;
			ieee->current_network.rates_ex[6] = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_48MB;
			ieee->current_network.rates_ex[7] = RTLLIB_BASIC_RATE_MASK | RTLLIB_OFDM_RATE_54MB;*/

			ieee->current_network.rates_ex[0] = RTLLIB_OFDM_RATE_6MB;
			ieee->current_network.rates_ex[1] = RTLLIB_OFDM_RATE_9MB;
			ieee->current_network.rates_ex[2] = RTLLIB_OFDM_RATE_12MB;
			ieee->current_network.rates_ex[3] = RTLLIB_OFDM_RATE_18MB;
			ieee->current_network.rates_ex[4] = RTLLIB_OFDM_RATE_24MB;
			ieee->current_network.rates_ex[5] = RTLLIB_OFDM_RATE_36MB;
			ieee->current_network.rates_ex[6] = RTLLIB_OFDM_RATE_48MB;
			ieee->current_network.rates_ex[7] = RTLLIB_OFDM_RATE_54MB;
			
			ieee->rate = 108;
		}else{
			ieee->current_network.rates_ex_len = 0;
			ieee->rate = 22;
		}

#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE)
#ifdef ADHOC_11N
		ieee->current_network.qos_data.supported = 1;
#else
		ieee->current_network.qos_data.supported = 0;
#endif
#ifdef ASL
		ieee->SetWirelessMode(ieee->dev, ieee->mode, false);
#else
		ieee->SetWirelessMode(ieee->dev, ieee->mode);
#endif
#else
		ieee->current_network.qos_data.supported = 0;
#ifdef ASL
		ieee->SetWirelessMode(ieee->dev, IEEE_G, false);
#endif
		ieee->SetWirelessMode(ieee->dev, IEEE_G);	
#endif
		ieee->current_network.mode = ieee->mode;
		ieee->current_network.atim_window = 0;
		ieee->current_network.capability = WLAN_CAPABILITY_IBSS;
	}

	printk("%s(): ieee->mode = %d\n", __FUNCTION__, ieee->mode);
	if((ieee->mode == IEEE_N_24G) || (ieee->mode == IEEE_N_5G))
		HTUseDefaultSetting(ieee);
	else
		ieee->pHTInfo->bCurrentHTSupport = false;	

	ieee->SetHwRegHandler(ieee->dev, HW_VAR_MEDIA_STATUS, (u8 *)(&ieee->state));

#if defined (RTL8192S_WAPI_SUPPORT)
	if ((ieee->WapiSupport) && (ieee->wapiInfo.bWapiEnable)){
		WAPI_CreateEvent_Send(ieee, WAPI_EVENT_CONNECT, ieee->current_network.bssid, NULL, 0);
	}
#endif		
	ieee->state = RTLLIB_LINKED;
#ifdef _RTL8192_EXT_PATCH_
	ieee->set_chan(ieee->dev, ieee->current_network.channel);  
#endif
	ieee->link_change(ieee->dev);

#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE)

	if(ieee->pHTInfo->bCurBW40MHz)
#ifdef ASL
		HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20_40, (ieee->current_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER, false);  
#else
		HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20_40, (ieee->current_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER);  
#endif
	else
#ifdef ASL
		HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20, (ieee->current_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER, false);  
#else
		HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20, (ieee->current_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER);  
#endif
#else
	HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20, HT_EXTCHNL_OFFSET_NO_EXT);
#endif
	if(ieee->LedControlHandler != NULL)
	        ieee->LedControlHandler(ieee->dev,LED_CTL_LINK);
	
	rtllib_start_send_beacons(ieee);
	
	notify_wx_assoc_event(ieee);
	
	if (ieee->data_hard_resume)
		ieee->data_hard_resume(ieee->dev);

	netif_carrier_on(ieee->dev);
	
	up(&ieee->wx_sem);
}

inline void rtllib_start_ibss(struct rtllib_device *ieee)
{
	queue_delayed_work_rsl(ieee->wq, &ieee->start_ibss_wq, MSECS(150));
}

/* this is called only in user context, with wx_sem held */
void rtllib_start_bss(struct rtllib_device *ieee)
{
	unsigned long flags;
#ifdef ENABLE_DOT11D
	if(IS_DOT11D_ENABLE(ieee) && !IS_COUNTRY_IE_VALID(ieee))
	{
		if(! ieee->bGlobalDomain)
		{
			return;
		}
	}
#endif	
	/* check if we have already found the net we
	 * are interested in (if any).
	 * if not (we are disassociated and we are not
	 * in associating / authenticating phase) start the background scanning.
	 */
	rtllib_softmac_check_all_nets(ieee);
	
	/* ensure no-one start an associating process (thus setting
	 * the ieee->state to rtllib_ASSOCIATING) while we
	 * have just cheked it and we are going to enable scan.
	 * The rtllib_new_net function is always called with
	 * lock held (from both rtllib_softmac_check_all_nets and
	 * the rx path), so we cannot be in the middle of such function
	 */
	spin_lock_irqsave(&ieee->lock, flags);
	
	if (ieee->state == RTLLIB_NOLINK) {
		rtllib_start_scan(ieee);
	}
	spin_unlock_irqrestore(&ieee->lock, flags);
}

void rtllib_link_change_wq(void *data)
{
	struct rtllib_device *ieee = container_of_dwork_rsl(data, struct rtllib_device, link_change_wq);
	ieee->link_change(ieee->dev);
}
/* called only in userspace context */
void rtllib_disassociate(struct rtllib_device *ieee)
{
	netif_carrier_off(ieee->dev);
#ifdef ASL 
	if (!ieee->ap_proto_started) {
#endif
	if (ieee->softmac_features & IEEE_SOFTMAC_TX_QUEUE)
			rtllib_reset_queue(ieee);
#ifdef ASL
	}
#endif
	if (ieee->data_hard_stop)
			ieee->data_hard_stop(ieee->dev);
#ifdef ENABLE_DOT11D
	if(IS_DOT11D_ENABLE(ieee))
		Dot11d_Reset(ieee);
#endif
	ieee->state = RTLLIB_NOLINK;
	ieee->is_set_key = false;
	ieee->wap_set = 0;

	queue_delayed_work_rsl(ieee->wq, &ieee->link_change_wq, 0);


#ifndef FOR_ANDROID_X86
	notify_wx_assoc_event(ieee);
#endif
#ifdef RTL8192S_WAPI_SUPPORT
	printk("==============> %s()\n", __FUNCTION__);
	WapiReturnOneStaInfo(ieee, ieee->current_network.bssid, 0);
#endif
}
#ifdef RTL8192S_WAPI_SUPPORT
void ieee80211_disassociate_from_app(struct rtllib_device *ieee)
{
	netif_carrier_off(ieee->dev);
	if (ieee->softmac_features & IEEE_SOFTMAC_TX_QUEUE)
			rtllib_reset_queue(ieee);
	
	if (ieee->data_hard_stop)
			ieee->data_hard_stop(ieee->dev);
#ifdef ENABLE_DOT11D
	if(IS_DOT11D_ENABLE(ieee))
		Dot11d_Reset(ieee);
#endif
	ieee->state = RTLLIB_NOLINK;
	ieee->is_set_key = false;
	ieee->wap_set = 0;

	queue_delayed_work_rsl(ieee->wq, &ieee->link_change_wq, 0);


#ifndef FOR_ANDROID_X86
	notify_wx_assoc_event(ieee);
#endif
	
	printk("==============> %s()\n", __FUNCTION__);
	WapiReturnOneStaInfo(ieee, ieee->current_network.bssid, 1);
}

#endif
void rtllib_associate_retry_wq(void *data)
{
	struct rtllib_device *ieee = container_of_dwork_rsl(data, struct rtllib_device, associate_retry_wq);
	unsigned long flags;
	
	down(&ieee->wx_sem);
	if(!ieee->proto_started)
		goto exit;
		
	if(ieee->state != RTLLIB_ASSOCIATING_RETRY)
		goto exit;
		
	/* until we do not set the state to RTLLIB_NOLINK 
	* there are no possibility to have someone else trying
	* to start an association procdure (we get here with
	* ieee->state = RTLLIB_ASSOCIATING).
	* When we set the state to RTLLIB_NOLINK it is possible
	* that the RX path run an attempt to associate, but
	* both rtllib_softmac_check_all_nets and the
	* RX path works with ieee->lock held so there are no
	* problems. If we are still disassociated then start a scan.
	* the lock here is necessary to ensure no one try to start
	* an association procedure when we have just checked the 
	* state and we are going to start the scan.
	*/
	ieee->beinretry = true;
	ieee->state = RTLLIB_NOLINK;

	rtllib_softmac_check_all_nets(ieee);
	
	spin_lock_irqsave(&ieee->lock, flags);
	
	if(ieee->state == RTLLIB_NOLINK)
	{
		rtllib_start_scan(ieee);
	}
	spin_unlock_irqrestore(&ieee->lock, flags);

	ieee->beinretry = false;
exit:
	up(&ieee->wx_sem);
}

struct sk_buff *rtllib_get_beacon_(struct rtllib_device *ieee)
{
	u8 broadcast_addr[] = {0xff,0xff,0xff,0xff,0xff,0xff};
	
	struct sk_buff *skb;
	struct rtllib_probe_response *b;
#ifdef _RTL8192_EXT_PATCH_
	if((ieee->iw_mode == IW_MODE_MESH)&&(ieee->ext_patch_get_beacon_get_probersp ))
		skb = ieee->ext_patch_get_beacon_get_probersp(ieee, broadcast_addr, &(ieee->current_mesh_network));
	else
		skb = rtllib_probe_resp(ieee, broadcast_addr, true);
#else
	skb = rtllib_probe_resp(ieee, broadcast_addr, true);
#endif
	
	if (!skb) 
		return NULL;
	
	b = (struct rtllib_probe_response *) skb->data;
	b->header.frame_ctl = cpu_to_le16(RTLLIB_STYPE_BEACON);
		
	return skb;
	
}

struct sk_buff *rtllib_get_beacon(struct rtllib_device *ieee)
{
	struct sk_buff *skb;
	struct rtllib_probe_response *b;
	
	skb = rtllib_get_beacon_(ieee);
	if(!skb) 
		return NULL;
		
	b = (struct rtllib_probe_response *) skb->data;	
	b->header.seq_ctl = cpu_to_le16(ieee->seq_ctrl[0] << 4);
	
	if (ieee->seq_ctrl[0] == 0xFFF)
		ieee->seq_ctrl[0] = 0;
	else
		ieee->seq_ctrl[0]++;
	
	return skb;
}

void rtllib_softmac_stop_protocol(struct rtllib_device *ieee, u8 mesh_flag, u8 ap_flag, u8 shutdown)
{
	rtllib_stop_scan_syncro(ieee);
	down(&ieee->wx_sem);
#ifdef _RTL8192_EXT_PATCH_	
	if(mesh_flag) {
		rtllib_stop_mesh_protocol(ieee);
	}
	else
#endif	
#ifdef ASL
	if (ap_flag)
		apdev_rtllib_stop_protocol(ieee, shutdown);
	else
#endif
		rtllib_stop_protocol(ieee,shutdown);
	up(&ieee->wx_sem);
}


void rtllib_stop_protocol(struct rtllib_device *ieee, u8 shutdown)
{
	if (!ieee->proto_started)
		return;
	if(shutdown){
	ieee->proto_started = 0;
		ieee->proto_stoppping = 1;
#ifdef ENABLE_IPS
		if(ieee->rtllib_ips_leave != NULL)
			ieee->rtllib_ips_leave(ieee->dev);
#endif
	}
	
	rtllib_stop_send_beacons(ieee);
	del_timer_sync(&ieee->associate_timer);
#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE)
	del_timer_sync(&ieee->ibss_wait_timer);
#endif
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)	
	cancel_delayed_work(&ieee->associate_retry_wq);	
	cancel_delayed_work(&ieee->start_ibss_wq);
	cancel_delayed_work(&ieee->link_change_wq);
#endif	
	rtllib_stop_scan(ieee);

	if(ieee->state <= RTLLIB_ASSOCIATING_AUTHENTICATED)
		ieee->state = RTLLIB_NOLINK;

	if (ieee->state == RTLLIB_LINKED){
		if (ieee->iw_mode == IW_MODE_INFRA
#ifdef ASL
			|| ieee->iw_mode == IW_MODE_APSTA
#endif
			)
			SendDisassociation(ieee,1,deauth_lv_ss);
		rtllib_disassociate(ieee);
	}
	if(shutdown){
		RemoveAllTS(ieee); 
		ieee->proto_stoppping = 0;
#ifdef RTL8192S_WAPI_SUPPORT
		if ((ieee->WapiSupport) && (ieee->wapiInfo.bWapiEnable))
		{
			WapiReturnAllStaInfo(ieee);
		}
#endif
	}
#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE)
	if(ieee->iw_mode == IW_MODE_ADHOC) 
		DelStaInfoList(ieee);
#endif

	if(ieee->assocreq_ies){
		kfree(ieee->assocreq_ies);
		ieee->assocreq_ies = NULL;
		ieee->assocreq_ies_len = 0;
	}
	if(ieee->assocresp_ies){
		kfree(ieee->assocresp_ies);
		ieee->assocresp_ies = NULL;
		ieee->assocresp_ies_len = 0;
	}
}

#ifdef _RTL8192_EXT_PATCH_	
void rtllib_stop_mesh_protocol(struct rtllib_device *ieee)
{
	if (!ieee->mesh_started)
		return;
	ieee->mesh_started = 0;

	if(ieee->ext_patch_rtllib_stop_protocol)
		ieee->ext_patch_rtllib_stop_protocol(ieee,0);
	
	rtllib_stop_send_beacons(ieee);
}
#endif	

void rtllib_softmac_start_protocol(struct rtllib_device *ieee, u8 mesh_flag, u8 ap_flag)
{
	down(&ieee->wx_sem);
#ifdef _RTL8192_EXT_PATCH_	
	if (mesh_flag) {
		rtllib_start_mesh_protocol(ieee);
	}
	else
#endif	
#ifdef ASL
	if (ap_flag)
		apdev_rtllib_start_protocol(ieee);
	else
#endif
		rtllib_start_protocol(ieee);

	up(&ieee->wx_sem);
}

void rtllib_start_protocol(struct rtllib_device *ieee)
{
	short ch = 0;
 	int i = 0;	

        HTInitializeHTInfo(ieee);
	
	rtllib_update_active_chan_map(ieee);

	if (ieee->proto_started)
		return;
		
	ieee->proto_started = 1;
	
	if (ieee->current_network.channel == 0) {
		do {
			ch++;
			if (ch > MAX_CHANNEL_NUMBER) 
				return; /* no channel found */
		} while(!ieee->active_channel_map[ch]);
		ieee->current_network.channel = ch;
	}
	
	if (ieee->current_network.beacon_interval == 0)
		ieee->current_network.beacon_interval = 100;
	
	for(i = 0; i < 17; i++) {
		ieee->last_rxseq_num[i] = -1;
		ieee->last_rxfrag_num[i] = -1;
		ieee->last_packet_time[i] = 0;
	}

	if(ieee->UpdateBeaconInterruptHandler) {
#ifdef _RTL8192_EXT_PATCH_
		if (ieee->iw_mode == IW_MODE_MESH)
			ieee->UpdateBeaconInterruptHandler(ieee->dev, true);
		else
#endif
		ieee->UpdateBeaconInterruptHandler(ieee->dev, false);
	}		
	ieee->wmm_acm = 0;
	/* if the user set the MAC of the ad-hoc cell and then
	 * switch to managed mode, shall we  make sure that association
	 * attempts does not fail just because the user provide the essid
	 * and the nic is still checking for the AP MAC ??
	 */
#ifdef _RTL8192_EXT_PATCH_
	if ((ieee->iw_mode == IW_MODE_INFRA) || ((ieee->iw_mode == IW_MODE_MESH) && (ieee->only_mesh == 0)))  
#else
	if (ieee->iw_mode == IW_MODE_INFRA
#ifdef ASL
		|| ieee->iw_mode == IW_MODE_APSTA
#endif
		)  
#endif		
	{
		rtllib_start_bss(ieee);
	}else if (ieee->iw_mode == IW_MODE_ADHOC){
		if(ieee->UpdateBeaconInterruptHandler)
			ieee->UpdateBeaconInterruptHandler(ieee->dev, true);
		
		rtllib_start_ibss(ieee);
		
	}
#ifndef ASL
	else if (ieee->iw_mode == IW_MODE_MASTER)
		rtllib_start_master_bss(ieee);
#endif		
	else if(ieee->iw_mode == IW_MODE_MONITOR)
		rtllib_start_monitor_mode(ieee);	
}

#ifdef _RTL8192_EXT_PATCH_
void rtllib_start_mesh_protocol(struct rtllib_device *ieee)
{
	short ch = 0;

	rtllib_update_active_chan_map(ieee);

	if (ieee->mesh_started)
		return;

	ieee->mesh_started = 1;
	
	if (ieee->current_mesh_network.channel == 0){
		do {
			ch++;
			if (ch > MAX_CHANNEL_NUMBER) 
				return; /* no channel found */
		} while(ieee->active_channel_map[ch]);

		ieee->current_mesh_network.channel = ch;
	}
	
	if (ieee->current_mesh_network.beacon_interval == 0)
		ieee->current_mesh_network.beacon_interval = 100;
	ieee->wmm_acm = 0;

	if(ieee->UpdateBeaconInterruptHandler)
		ieee->UpdateBeaconInterruptHandler(ieee->dev, true);

	if(ieee->ext_patch_rtllib_start_mesh)
		ieee->ext_patch_rtllib_start_mesh(ieee);
}
#endif


#define DRV_NAME  "Ieee80211"
void rtllib_softmac_init(struct rtllib_device *ieee)
{
	int i;
	memset(&ieee->current_network, 0, sizeof(struct rtllib_network));
	
	ieee->state = RTLLIB_NOLINK;
#ifdef _RTL8192_EXT_PATCH_
	ieee->mesh_state = RTLLIB_NOLINK;
#endif
	for(i = 0; i < 5; i++) {
	  ieee->seq_ctrl[i] = 0;
	}
#ifdef ENABLE_DOT11D
	ieee->pDot11dInfo = kmalloc(sizeof(RT_DOT11D_INFO), GFP_ATOMIC);
	if (!ieee->pDot11dInfo)
		RTLLIB_DEBUG(RTLLIB_DL_ERR, "can't alloc memory for DOT11D\n");
	memset(ieee->pDot11dInfo, 0, sizeof(RT_DOT11D_INFO));
#endif
	ieee->LinkDetectInfo.SlotIndex = 0;
	ieee->LinkDetectInfo.SlotNum = 2;
	ieee->LinkDetectInfo.NumRecvBcnInPeriod=0;
        ieee->LinkDetectInfo.NumRecvDataInPeriod=0;
	ieee->LinkDetectInfo.NumTxOkInPeriod =0;
	ieee->LinkDetectInfo.NumRxOkInPeriod =0;
	ieee->LinkDetectInfo.NumRxUnicastOkInPeriod=0;
#ifdef _RTL8192_EXT_PATCH_
	ieee->LinkDetectInfo.NumTxUnicastOkInPeriod = 0;
	ieee->LinkDetectInfo.LastNumRxUnicast = 0;
	ieee->LinkDetectInfo.LastNumTxUnicast = 0;
	ieee->LinkDetectInfo.IdleCount = 0;
#endif
	ieee->bIsAggregateFrame = false;
	ieee->assoc_id = 0;
	ieee->queue_stop = 0;
	ieee->scanning_continue = 0;
	ieee->softmac_features = 0; 
	ieee->wap_set = 0;
	ieee->ssid_set = 0;
	ieee->proto_started = 0;
	ieee->proto_stoppping = 0;
	ieee->basic_rate = RTLLIB_DEFAULT_BASIC_RATE;
	ieee->rate = 22;
	ieee->ps = RTLLIB_PS_DISABLED;
	ieee->sta_sleep = LPS_IS_WAKE;

#ifdef _RTL8192_EXT_PATCH_
	ieee->mesh_started = 0;
#endif
#ifdef ASL
	ieee->ap_proto_started = 0;
	ieee->ap_proto_stoppping = 0;
	memset(&ieee->current_ap_network, 0, sizeof(struct rtllib_network));
	ieee->ap_Regdot11HTOperationalRateSet[0]= 0xff;
	ieee->ap_Regdot11HTOperationalRateSet[1]= 0xff;
	ieee->ap_Regdot11HTOperationalRateSet[4]= 0x01;
	ieee->ap_Regdot11TxHTOperationalRateSet[0]= 0xff;
	ieee->ap_Regdot11TxHTOperationalRateSet[1]= 0xff;
	ieee->ap_Regdot11TxHTOperationalRateSet[4]= 0x01;
#endif
	ieee->Regdot11HTOperationalRateSet[0]= 0xff;
	ieee->Regdot11HTOperationalRateSet[1]= 0xff;
	ieee->Regdot11HTOperationalRateSet[4]= 0x01;

	ieee->Regdot11TxHTOperationalRateSet[0]= 0xff;
	ieee->Regdot11TxHTOperationalRateSet[1]= 0xff;
	ieee->Regdot11TxHTOperationalRateSet[4]= 0x01;
	
	ieee->FirstIe_InScan = false;
	ieee->actscanning = false;
	ieee->beinretry = false;
	ieee->is_set_key = false;
	init_mgmt_queue(ieee);

	ieee->sta_edca_param[0] = 0x0000A403;
	ieee->sta_edca_param[1] = 0x0000A427;
	ieee->sta_edca_param[2] = 0x005E4342;
	ieee->sta_edca_param[3] = 0x002F3262;
	ieee->aggregation = true;
	ieee->enable_rx_imm_BA = 1;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,40)	
	_setup_timer(&ieee->scan_timer,
		    rtllib_softmac_scan_cb,
		    (unsigned long) ieee);
#endif
	ieee->tx_pending.txb = NULL;
	
	_setup_timer(&ieee->associate_timer,
		    rtllib_associate_abort_cb,
		    (unsigned long) ieee);

	_setup_timer(&ieee->beacon_timer,
		    rtllib_send_beacon_cb,
		    (unsigned long) ieee);

#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE)
	_setup_timer(&ieee->ibss_wait_timer,
		    rtllib_ibss_wait_timeout,
		    (unsigned long) ieee);
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)	
#ifdef PF_SYNCTHREAD
	ieee->wq = create_workqueue(DRV_NAME,0);
#else	
	ieee->wq = create_workqueue(DRV_NAME);
#endif
#endif

	INIT_DELAYED_WORK_RSL(&ieee->link_change_wq,(void*)rtllib_link_change_wq,ieee);
	INIT_DELAYED_WORK_RSL(&ieee->start_ibss_wq,(void*)rtllib_start_ibss_wq,ieee);
	INIT_WORK_RSL(&ieee->associate_complete_wq, (void*)rtllib_associate_complete_wq,ieee);
	INIT_DELAYED_WORK_RSL(&ieee->associate_procedure_wq, (void*)rtllib_associate_procedure_wq,ieee);
	INIT_DELAYED_WORK_RSL(&ieee->softmac_scan_wq,(void*)rtllib_softmac_scan_wq,ieee);
	INIT_DELAYED_WORK_RSL(&ieee->softmac_hint11d_wq,(void*)rtllib_softmac_hint11d_wq,ieee);
	INIT_DELAYED_WORK_RSL(&ieee->associate_retry_wq, (void*)rtllib_associate_retry_wq,ieee);
	INIT_WORK_RSL(&ieee->wx_sync_scan_wq,(void*)rtllib_wx_sync_scan_wq,ieee);

#ifdef _RTL8192_EXT_PATCH_
	INIT_WORK_RSL(&ieee->ext_stop_scan_wq,(void*) rtllib_ext_stop_scan_wq,ieee);
	INIT_WORK_RSL(&ieee->ext_send_beacon_wq,(void*) ext_rtllib_send_beacon_wq,ieee);
#endif 

	sema_init(&ieee->wx_sem, 1);
	sema_init(&ieee->scan_sem, 1);
	sema_init(&ieee->ips_sem,1);
	
	spin_lock_init(&ieee->mgmt_tx_lock);
	spin_lock_init(&ieee->beacon_lock);
	
	tasklet_init(&ieee->ps_task,
	     (void(*)(unsigned long)) rtllib_sta_ps,
	     (unsigned long)ieee);

}

void rtllib_softmac_free(struct rtllib_device *ieee)
{
	down(&ieee->wx_sem);
#ifdef ENABLE_DOT11D
	if(NULL != ieee->pDot11dInfo)
	{
		kfree(ieee->pDot11dInfo);
		ieee->pDot11dInfo = NULL;
	}
#endif
	del_timer_sync(&ieee->associate_timer);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)	
	cancel_delayed_work(&ieee->associate_retry_wq);
	destroy_workqueue(ieee->wq);
#endif
	
	up(&ieee->wx_sem);
}

/******************************************************** 
 * Start of WPA code.                                   *
 * this is stolen from the ipw2200 driver               *
 ********************************************************/

 
static int rtllib_wpa_enable(struct rtllib_device *ieee, int value)
{
	/* This is called when wpa_supplicant loads and closes the driver
	 * interface. */
	printk("%s WPA\n",value ? "enabling" : "disabling");
	ieee->wpa_enabled = value;
	memset(ieee->ap_mac_addr, 0, 6); 
	return 0;
}

 
void rtllib_wpa_assoc_frame(struct rtllib_device *ieee, char *wpa_ie, int wpa_ie_len)
{
	/* make sure WPA is enabled */
	rtllib_wpa_enable(ieee, 1);

	rtllib_disassociate(ieee);
}


static int rtllib_wpa_mlme(struct rtllib_device *ieee, int command, int reason)
{
	
	int ret = 0;

	switch (command) {
	case IEEE_MLME_STA_DEAUTH:
		break;

	case IEEE_MLME_STA_DISASSOC:
		rtllib_disassociate(ieee);
		break;

	default:
		printk("Unknown MLME request: %d\n", command);
		ret = -EOPNOTSUPP;
	}

	return ret;
}


static int rtllib_wpa_set_wpa_ie(struct rtllib_device *ieee,
			      struct ieee_param *param, int plen)
{
	u8 *buf;

	if (param->u.wpa_ie.len > MAX_WPA_IE_LEN ||
	    (param->u.wpa_ie.len && param->u.wpa_ie.data == NULL))
		return -EINVAL;

	if (param->u.wpa_ie.len) {
		buf = kmalloc(param->u.wpa_ie.len, GFP_KERNEL);
		if (buf == NULL)
			return -ENOMEM;

		memcpy(buf, param->u.wpa_ie.data, param->u.wpa_ie.len);
		kfree(ieee->wpa_ie);
		ieee->wpa_ie = buf;
		ieee->wpa_ie_len = param->u.wpa_ie.len;
	} else {
		kfree(ieee->wpa_ie);
		ieee->wpa_ie = NULL;
		ieee->wpa_ie_len = 0;
	}

	rtllib_wpa_assoc_frame(ieee, ieee->wpa_ie, ieee->wpa_ie_len);
	return 0;
}

#define AUTH_ALG_OPEN_SYSTEM			0x1
#define AUTH_ALG_SHARED_KEY			0x2
#define AUTH_ALG_LEAP				0x4
static int rtllib_wpa_set_auth_algs(struct rtllib_device *ieee, int value)
{
	
	struct rtllib_security sec = {
		.flags = SEC_AUTH_MODE,
	};
	int ret = 0;

	if (value & AUTH_ALG_SHARED_KEY) {
		sec.auth_mode = WLAN_AUTH_SHARED_KEY;
		ieee->open_wep = 0;
		ieee->auth_mode = 1;
	} else if (value & AUTH_ALG_OPEN_SYSTEM){
		sec.auth_mode = WLAN_AUTH_OPEN;
		ieee->open_wep = 1;
		ieee->auth_mode = 0;
	}
	else if (value & AUTH_ALG_LEAP){
		sec.auth_mode = WLAN_AUTH_LEAP  >> 6;
		ieee->open_wep = 1;
		ieee->auth_mode = 2;
	}


	if (ieee->set_security)
		ieee->set_security(ieee->dev, &sec);

	return ret;
}

static int rtllib_wpa_set_param(struct rtllib_device *ieee, u8 name, u32 value)
{
	int ret=0;
	unsigned long flags;

	switch (name) {
	case IEEE_PARAM_WPA_ENABLED:
		ret = rtllib_wpa_enable(ieee, value);
		break;

	case IEEE_PARAM_TKIP_COUNTERMEASURES:
		ieee->tkip_countermeasures=value;
		break;

		case IEEE_PARAM_DROP_UNENCRYPTED: 
		{
		/* HACK:
		 *
		 * wpa_supplicant calls set_wpa_enabled when the driver
		 * is loaded and unloaded, regardless of if WPA is being
		 * used.  No other calls are made which can be used to
		 * determine if encryption will be used or not prior to
		 * association being expected.  If encryption is not being
		 * used, drop_unencrypted is set to false, else true -- we
		 * can use this to determine if the CAP_PRIVACY_ON bit should
		 * be set.
		 */
		struct rtllib_security sec = {
			.flags = SEC_ENABLED,
			.enabled = value,
		};
 		ieee->drop_unencrypted = value;
		/* We only change SEC_LEVEL for open mode. Others
		 * are set by ipw_wpa_set_encryption.
		 */
		if (!value) {
			sec.flags |= SEC_LEVEL;
			sec.level = SEC_LEVEL_0;
		}
		else {
			sec.flags |= SEC_LEVEL;
			sec.level = SEC_LEVEL_1;
		}
		if (ieee->set_security)
			ieee->set_security(ieee->dev, &sec);
		break;
	}

	case IEEE_PARAM_PRIVACY_INVOKED:
		ieee->privacy_invoked=value;
		break;

	case IEEE_PARAM_AUTH_ALGS:
		ret = rtllib_wpa_set_auth_algs(ieee, value);
		break;

	case IEEE_PARAM_IEEE_802_1X:
		ieee->ieee802_1x=value;
		break;
	case IEEE_PARAM_WPAX_SELECT:
		spin_lock_irqsave(&ieee->wpax_suitlist_lock,flags);
		spin_unlock_irqrestore(&ieee->wpax_suitlist_lock,flags);
		break;

	default:
		printk("Unknown WPA param: %d\n",name);
		ret = -EOPNOTSUPP;
	}

	return ret;
}

/* implementation borrowed from hostap driver */
static int rtllib_wpa_set_encryption(struct rtllib_device *ieee,
				  struct ieee_param *param, int param_len, u8 is_mesh, u8 is_ap)
{
	int ret = 0;
#ifdef ASL
	int idx_sta=0;
#endif
#ifdef _RTL8192_EXT_PATCH_
	u8 i = 0;
#endif
	struct rtllib_crypto_ops *ops;
	struct rtllib_crypt_data **crypt;

	struct rtllib_security sec = {
		.flags = 0,
	};

	param->u.crypt.err = 0;
	param->u.crypt.alg[IEEE_CRYPT_ALG_NAME_LEN - 1] = '\0';

	if (param_len !=
	    (int) ((char *) param->u.crypt.key - (char *) param) +
	    param->u.crypt.key_len) {
		printk("Len mismatch %d, %d\n", param_len,
			       param->u.crypt.key_len);
		return -EINVAL;
	}
	if (param->sta_addr[0] == 0xff && param->sta_addr[1] == 0xff &&
	    param->sta_addr[2] == 0xff && param->sta_addr[3] == 0xff &&
	    param->sta_addr[4] == 0xff && param->sta_addr[5] == 0xff) {
		if (param->u.crypt.idx >= WEP_KEYS)
			return -EINVAL;
#ifdef _RTL8192_EXT_PATCH_
		if(is_mesh)
			crypt = &ieee->cryptlist[0]->crypt[param->u.crypt.idx];
		else {
#endif
#ifdef ASL
			if (is_ap) {
			    	printk("========>MASTER: multicast idx is %d\n",param->u.crypt.idx);
			    	if ((ieee->iw_mode == IW_MODE_MASTER || ieee->iw_mode == IW_MODE_APSTA)) {
				if( strcmp(param->u.crypt.alg, "WEP")==0)
					ieee->iswep = 1;
				crypt = &ieee->stations_crypt[0]->crypt[param->u.crypt.idx];
				ieee->stations_crypt[0]->used = 1;
			    	} else {
					printk("=====>%s(): multicast :isap but the iw_mode is wrong\n",__FUNCTION__);
					return -EINVAL;
				}
			}else
#endif
#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
			crypt = &ieee->sta_crypt[param->u.crypt.idx];
#else
		crypt = &ieee->crypt[param->u.crypt.idx];
#endif
#ifdef _RTL8192_EXT_PATCH_
		}
#endif
	} else {
#ifdef ASL
		printk("unicast addr: idx is %d\n",param->u.crypt.idx);
		if (is_ap) {
			if (ieee->iw_mode == IW_MODE_MASTER || ieee->iw_mode == IW_MODE_APSTA) {
			idx_sta = ap_cryptlist_find_station(ieee,param->sta_addr,0);/*Not just find.*/
			if (idx_sta < 0) {
				printk("\nNo more empty space to save new WPA connection.");
				ret = -EINVAL; 
			}
#ifdef ASL_DEBUG_2
			printk("\nSTA Addr: %x:%x:%x:%x:%x:%x",param->sta_addr[0],param->sta_addr[1],param->sta_addr[2],param->sta_addr[3],param->sta_addr[4],param->sta_addr[5]);
			printk("  idx_sta=%d\n", idx_sta);	
#endif		
			memcpy(ieee->stations_crypt[idx_sta]->mac_addr,param->sta_addr,ETH_ALEN);	
			crypt = &ieee->stations_crypt[idx_sta]->crypt[param->u.crypt.idx];
			ieee->stations_crypt[idx_sta]->used = 1;
			} else {
					printk("=====>%s(): unicast addr :isap but the iw_mode is wrong\n",__FUNCTION__);
					return -EINVAL;
			}
		}
		else
#endif
		return -EINVAL;
	}
	if (strcmp(param->u.crypt.alg, "none") == 0) {
		if (crypt) {
			sec.enabled = 0;
			sec.level = SEC_LEVEL_0;
			sec.flags |= SEC_ENABLED | SEC_LEVEL;
			rtllib_crypt_delayed_deinit(ieee, crypt);
#ifdef ASL		
			if (is_ap) { 
				if (ieee->iw_mode == IW_MODE_MASTER || ieee->iw_mode == IW_MODE_APSTA) {
				memset(ieee->stations_crypt[idx_sta]->mac_addr, 0, ETH_ALEN);
			
				ieee->stations_crypt[idx_sta]->used = 0;
			
				ieee->iswep = 0;/*OK???,need verify.*/
			}
			}
#endif
		}
		goto done;
	}
	sec.enabled = 1;
	sec.flags |= SEC_ENABLED;

	/* IPW HW cannot build TKIP MIC, host decryption still needed. */
	if (!(ieee->host_encrypt || ieee->host_decrypt) &&
	    strcmp(param->u.crypt.alg, "TKIP"))
		goto skip_host_crypt;

	ops = rtllib_get_crypto_ops(param->u.crypt.alg);
	if (ops == NULL && strcmp(param->u.crypt.alg, "WEP") == 0) {
		request_module("rtllib_crypt_wep");
		ops = rtllib_get_crypto_ops(param->u.crypt.alg);
	} else if (ops == NULL && strcmp(param->u.crypt.alg, "TKIP") == 0) {
		request_module("rtllib_crypt_tkip");
		ops = rtllib_get_crypto_ops(param->u.crypt.alg);
	} else if (ops == NULL && strcmp(param->u.crypt.alg, "CCMP") == 0) {
		request_module("rtllib_crypt_ccmp");
		ops = rtllib_get_crypto_ops(param->u.crypt.alg);
	}
	if (ops == NULL) {
		printk("unknown crypto alg '%s'\n", param->u.crypt.alg);
		param->u.crypt.err = IEEE_CRYPT_ERR_UNKNOWN_ALG;
		ret = -EINVAL;
		goto done;
	}
#ifdef _RTL8192_EXT_PATCH_
	if(is_mesh)  
	{
	#if 0  
		for (i=0; i<MAX_MP; i++){
			crypt = &ieee->cryptlist[i]->crypt[param->u.crypt.idx];

			*crypt = ieee->cryptlist[i]->crypt[param->u.crypt.idx];
		}
#endif
#endif

		if (*crypt == NULL || (*crypt)->ops != ops) {
			struct rtllib_crypt_data *new_crypt;

			rtllib_crypt_delayed_deinit(ieee, crypt);

			new_crypt = (struct rtllib_crypt_data *)
				kmalloc(sizeof(*new_crypt), GFP_KERNEL);
			if (new_crypt == NULL) {
				ret = -ENOMEM;
				goto done;
			}
			memset(new_crypt, 0, sizeof(struct rtllib_crypt_data));
			new_crypt->ops = ops;
#ifdef BUILT_IN_RTLLIB
			if (new_crypt->ops)
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
			if (new_crypt->ops && try_module_get(new_crypt->ops->owner))
#else
			if (new_crypt->ops && try_inc_mod_count(new_crypt->ops->owner))
#endif			
#endif			
					new_crypt->priv =
						new_crypt->ops->init(param->u.crypt.idx);

			if (new_crypt->priv == NULL) {
				kfree(new_crypt);
				param->u.crypt.err = IEEE_CRYPT_ERR_CRYPT_INIT_FAILED;
				ret = -EINVAL;
				goto done;
			}

			*crypt = new_crypt;
		}

		if (param->u.crypt.key_len > 0 && (*crypt)->ops->set_key &&
				(*crypt)->ops->set_key(param->u.crypt.key,
					param->u.crypt.key_len, param->u.crypt.seq,
					(*crypt)->priv) < 0) {
			printk("key setting failed\n");
			param->u.crypt.err = IEEE_CRYPT_ERR_KEY_SET_FAILED;
			ret = -EINVAL;
			goto done;
		}
#ifdef _RTL8192_EXT_PATCH_
	}
#endif

 skip_host_crypt:
	printk("====>set tx is %d\n",param->u.crypt.set_tx);
	if (param->u.crypt.set_tx) {
#ifdef ASL
		if (is_ap)
			ieee->ap_tx_keyidx = param->u.crypt.idx;
		else
#endif
		ieee->tx_keyidx = param->u.crypt.idx;
#ifdef ASL
		printk("===========>ieee->ap_tx_keyidx is %d\n",ieee->ap_tx_keyidx);
#endif
		sec.active_key = param->u.crypt.idx;
		sec.flags |= SEC_ACTIVE_KEY;
	} else
		sec.flags &= ~SEC_ACTIVE_KEY;

	if (param->u.crypt.alg != NULL) {
		memcpy(sec.keys[param->u.crypt.idx],
		       param->u.crypt.key,
		       param->u.crypt.key_len);
		sec.key_sizes[param->u.crypt.idx] = param->u.crypt.key_len;
		sec.flags |= (1 << param->u.crypt.idx);

		if (strcmp(param->u.crypt.alg, "WEP") == 0) {
			sec.flags |= SEC_LEVEL;
			sec.level = SEC_LEVEL_1;
		} else if (strcmp(param->u.crypt.alg, "TKIP") == 0) {
			sec.flags |= SEC_LEVEL;
			sec.level = SEC_LEVEL_2;
		} else if (strcmp(param->u.crypt.alg, "CCMP") == 0) {
			sec.flags |= SEC_LEVEL;
			sec.level = SEC_LEVEL_3;
		}
	}
 done:
 #ifdef ASL
	if (is_ap) {
		if (ieee->iw_mode == IW_MODE_MASTER || ieee->iw_mode == IW_MODE_APSTA) {
	if (strcmp(param->u.crypt.alg, "none") == 0){
		if(ap_stations_cryptlist_is_empty(ieee))
			ieee->encrypt_activated = 0;
	} else
		ieee->encrypt_activated = 1;
        }
	}
#endif	

	if (ieee->set_security)
		ieee->set_security(ieee->dev, &sec);

#ifdef _RTL8192_EXT_PATCH_
	if (ret != 0)
	{
		if(is_mesh)   
		{
			for (i=0; i<MAX_MP; i++)
			{
				if (ieee->cryptlist[i]->crypt[param->u.crypt.idx]==NULL){
					break;
				}
				else{
					kfree(ieee->cryptlist[i]->crypt[param->u.crypt.idx]);
					ieee->cryptlist[i]->crypt[param->u.crypt.idx] = NULL;
				}
			}
		}
		else
		{
			kfree(ieee->sta_crypt[param->u.crypt.idx]);
			ieee->sta_crypt[param->u.crypt.idx] = NULL;
		}
	}
#endif
	

	/* Do not reset port if card is in Managed mode since resetting will
	 * generate new IEEE 802.11 authentication which may end up in looping
	 * with IEEE 802.1X.  If your hardware requires a reset after WEP
	 * configuration (for example... Prism2), implement the reset_port in
	 * the callbacks structures used to initialize the 802.11 stack. */
	if (ieee->reset_on_keychange &&
	    ieee->iw_mode != IW_MODE_INFRA &&
	    ieee->reset_port &&
	    ieee->reset_port(ieee->dev)) {
		printk("reset_port failed\n");
		param->u.crypt.err = IEEE_CRYPT_ERR_CARD_CONF_FAILED;
		return -EINVAL;
	}

	return ret;
}

inline struct sk_buff *rtllib_disauth_skb( struct rtllib_network *beacon,
		struct rtllib_device *ieee, u16 asRsn)
{
	struct sk_buff *skb;
	struct rtllib_disauth *disauth;
#ifdef USB_USE_ALIGNMENT
        u32 Tmpaddr=0;
        int alignment=0;
	int len = sizeof(struct rtllib_disauth) + ieee->tx_headroom + USB_512B_ALIGNMENT_SIZE;
#else
	int len = sizeof(struct rtllib_disauth) + ieee->tx_headroom;

#endif
	skb = dev_alloc_skb(len);
	if (!skb) {
		return NULL;
	}

#ifdef USB_USE_ALIGNMENT
        Tmpaddr = (u32)skb->data;
        alignment = Tmpaddr & 0x1ff;
        skb_reserve(skb,(USB_512B_ALIGNMENT_SIZE - alignment));
#endif
	skb_reserve(skb, ieee->tx_headroom);
	
	disauth = (struct rtllib_disauth *) skb_put(skb,sizeof(struct rtllib_disauth));
	disauth->header.frame_ctl = cpu_to_le16(RTLLIB_STYPE_DEAUTH);
	disauth->header.duration_id = 0;
	
	memcpy(disauth->header.addr1, beacon->bssid, ETH_ALEN);
	memcpy(disauth->header.addr2, ieee->dev->dev_addr, ETH_ALEN);
	memcpy(disauth->header.addr3, beacon->bssid, ETH_ALEN);
	
	disauth->reason = cpu_to_le16(asRsn);
	return skb;
}

inline struct sk_buff *rtllib_disassociate_skb( struct rtllib_network *beacon,
		struct rtllib_device *ieee, u16 asRsn)
{
	struct sk_buff *skb;
	struct rtllib_disassoc *disass;
#ifdef USB_USE_ALIGNMENT
        u32 Tmpaddr=0;
        int alignment=0;
	int len = sizeof(struct rtllib_disassoc) + ieee->tx_headroom + USB_512B_ALIGNMENT_SIZE;
#else
	int len = sizeof(struct rtllib_disassoc) + ieee->tx_headroom;
#endif
	skb = dev_alloc_skb(len);

	if (!skb) {
		return NULL;
	}
	
#ifdef USB_USE_ALIGNMENT
        Tmpaddr = (u32)skb->data;
        alignment = Tmpaddr & 0x1ff;
        skb_reserve(skb,(USB_512B_ALIGNMENT_SIZE - alignment));
#endif
	skb_reserve(skb, ieee->tx_headroom);

	disass = (struct rtllib_disassoc *) skb_put(skb,sizeof(struct rtllib_disassoc));
	disass->header.frame_ctl = cpu_to_le16(RTLLIB_STYPE_DISASSOC);
	disass->header.duration_id = 0;
	
	memcpy(disass->header.addr1, beacon->bssid, ETH_ALEN);
	memcpy(disass->header.addr2, ieee->dev->dev_addr, ETH_ALEN);
	memcpy(disass->header.addr3, beacon->bssid, ETH_ALEN);
	
	disass->reason = cpu_to_le16(asRsn);
	return skb;
}

void SendDisassociation(struct rtllib_device *ieee, bool deauth, u16 asRsn)
{
	struct rtllib_network *beacon = &ieee->current_network;
	struct sk_buff *skb;

	if(deauth) {
		skb = rtllib_disauth_skb(beacon,ieee,asRsn);
	} else {
		skb = rtllib_disassociate_skb(beacon,ieee,asRsn);
	}

	if (skb){
#ifdef ASL
		softmac_mgmt_xmit(skb, ieee, false);
#else
		softmac_mgmt_xmit(skb, ieee);
#endif
	}
}

u8 rtllib_ap_sec_type(struct rtllib_device *ieee)
{
	static u8 ccmp_ie[4] = {0x00,0x50,0xf2,0x04};
	static u8 ccmp_rsn_ie[4] = {0x00, 0x0f, 0xac, 0x04};
	int wpa_ie_len= ieee->wpa_ie_len;
	struct rtllib_crypt_data* crypt;
	int encrypt;

#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
	crypt = ieee->sta_crypt[ieee->tx_keyidx];
#else
	crypt = ieee->crypt[ieee->tx_keyidx];
#endif
	encrypt = (ieee->current_network.capability & WLAN_CAPABILITY_PRIVACY) ||\
		  (ieee->host_encrypt && crypt && crypt->ops && \
		   (0 == strcmp(crypt->ops->name,"WEP")));

	/* simply judge  */
	if(encrypt && (wpa_ie_len == 0)) {
		return SEC_ALG_WEP;
	} else if((wpa_ie_len != 0)) {
		if (((ieee->wpa_ie[0] == 0xdd) && (!memcmp(&(ieee->wpa_ie[14]),ccmp_ie,4))) || 
				((ieee->wpa_ie[0] == 0x30) && (!memcmp(&ieee->wpa_ie[10],ccmp_rsn_ie, 4))))
			return SEC_ALG_CCMP;
		else
			return SEC_ALG_TKIP;
	} else {
		return SEC_ALG_NONE;
	}
}

int rtllib_wpa_supplicant_ioctl(struct rtllib_device *ieee, struct iw_point *p, u8 is_mesh, u8 is_ap)
{
	struct ieee_param *param;
	int ret=0;

	down(&ieee->wx_sem);

	if (p->length < sizeof(struct ieee_param) || !p->pointer){
		ret = -EINVAL;
		goto out;
	}
	
	param = (struct ieee_param *)kmalloc(p->length, GFP_KERNEL);
	if (param == NULL){
		ret = -ENOMEM;
		goto out;
	}
	if (copy_from_user(param, p->pointer, p->length)) {
		kfree(param);
		ret = -EFAULT;
		goto out;
	}

	switch (param->cmd) {

	case IEEE_CMD_SET_WPA_PARAM:
		ret = rtllib_wpa_set_param(ieee, param->u.wpa_param.name,
					param->u.wpa_param.value);
		break;

	case IEEE_CMD_SET_WPA_IE:
		ret = rtllib_wpa_set_wpa_ie(ieee, param, p->length);
		break;

	case IEEE_CMD_SET_ENCRYPTION:
#ifdef _RTL8192_EXT_PATCH_
		ret = rtllib_wpa_set_encryption(ieee, param, p->length, is_mesh, 0);
#else
#ifdef ASL
		ret = rtllib_wpa_set_encryption(ieee, param, p->length, 0, is_ap);
#else
		ret = rtllib_wpa_set_encryption(ieee, param, p->length, 0, 0);
#endif
#endif
		break;

	case IEEE_CMD_MLME:
		ret = rtllib_wpa_mlme(ieee, param->u.mlme.command,
				   param->u.mlme.reason_code);
		break;

	default:
		printk("Unknown WPA supplicant request: %d\n",param->cmd);
		ret = -EOPNOTSUPP;
		break;
	}

	if (ret == 0 && copy_to_user(p->pointer, param, p->length))
		ret = -EFAULT;

	kfree(param);
out:
	up(&ieee->wx_sem);
	
	return ret;
}

void
rtllib_MgntDisconnectIBSS(struct rtllib_device* rtllib) 
{
	u8	OpMode;
	u8	i;
	bool	bFilterOutNonAssociatedBSSID = false;

	rtllib->state = RTLLIB_NOLINK;
	for(i=0;i<6;i++)  rtllib->current_network.bssid[i]= 0x55;

	rtllib->OpMode = RT_OP_MODE_NO_LINK;
	rtllib->SetHwRegHandler(rtllib->dev, HW_VAR_BSSID, rtllib->current_network.bssid);
	OpMode = RT_OP_MODE_NO_LINK;
	rtllib->SetHwRegHandler(rtllib->dev, HW_VAR_MEDIA_STATUS, &OpMode);
	rtllib_stop_send_beacons(rtllib);
	
	bFilterOutNonAssociatedBSSID = false;
	rtllib->SetHwRegHandler(rtllib->dev, HW_VAR_CECHK_BSSID, (u8*)(&bFilterOutNonAssociatedBSSID));
	notify_wx_assoc_event(rtllib);

}

void
rtllib_MlmeDisassociateRequest(
	struct rtllib_device* rtllib,
	u8* 		asSta,
	u8		asRsn
	)
{
	u8 i;
	u8	OpMode;

	RemovePeerTS(rtllib, asSta);
	
	
	if(memcpy(rtllib->current_network.bssid,asSta,6) == 0)
	{
		rtllib->state = RTLLIB_NOLINK;
		
		for(i=0;i<6;i++)  rtllib->current_network.bssid[i] = 0x22;
		OpMode = RT_OP_MODE_NO_LINK;
		rtllib->OpMode = RT_OP_MODE_NO_LINK;
		rtllib->SetHwRegHandler(rtllib->dev, HW_VAR_MEDIA_STATUS, (u8 *)(&OpMode) );
		rtllib_disassociate(rtllib);

		rtllib->SetHwRegHandler(rtllib->dev, HW_VAR_BSSID, rtllib->current_network.bssid);
		
	}

}

void
rtllib_MgntDisconnectAP(
	struct rtllib_device* rtllib,
	u8 asRsn
) 
{
	bool bFilterOutNonAssociatedBSSID = false;


#ifdef TO_DO
	if(   pMgntInfo->SecurityInfo.AuthMode > RT_802_11AuthModeAutoSwitch ||
		(pMgntInfo->bAPSuportCCKM && pMgntInfo->bCCX8021xenable) )	
	{
		SecClearAllKeys(rtllib->dev);
		RT_TRACE(COMP_SEC, DBG_LOUD,("======>CCKM clear key..."))
	}
#endif
	bFilterOutNonAssociatedBSSID = false;
	rtllib->SetHwRegHandler(rtllib->dev, HW_VAR_CECHK_BSSID, (u8*)(&bFilterOutNonAssociatedBSSID));
	rtllib_MlmeDisassociateRequest( rtllib, rtllib->current_network.bssid, asRsn );

	rtllib->state = RTLLIB_NOLINK;
}

bool
rtllib_MgntDisconnect(
	struct rtllib_device* rtllib,
	u8 asRsn
) 
{
	if(rtllib->ps != RTLLIB_PS_DISABLED)
	{
#ifndef RTL8190P
                rtllib->sta_wake_up(rtllib->dev);
#endif
	}

#ifdef TO_DO
	if(pMgntInfo->mActingAsAp)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntDisconnect() ===> AP_DisassociateAllStation\n"));
		AP_DisassociateAllStation(rtllib->dev, unspec_reason);
		return true;
	}	
#endif

	if( rtllib->state == RTLLIB_LINKED )
	{
		if( rtllib->iw_mode == IW_MODE_ADHOC )
		{
			rtllib_MgntDisconnectIBSS(rtllib);
		}
#ifdef _RTL8192_EXT_PATCH_
		if((rtllib->iw_mode == IW_MODE_INFRA ) || ((rtllib->iw_mode == IW_MODE_MESH) && (rtllib->only_mesh == 0)))
#else
		if( rtllib->iw_mode == IW_MODE_INFRA 
#ifdef ASL
			|| rtllib->iw_mode == IW_MODE_APSTA
#endif
			)
#endif
		{
#ifdef TO_DO_LIST
			SecClearAllKeys(Adapter); 
#endif
			rtllib_MgntDisconnectAP(rtllib, asRsn);
		}

	}

	return true;
}

void notify_wx_assoc_event(struct rtllib_device *ieee)
{
	union iwreq_data wrqu;

	if(ieee->cannot_notify)
		return;

	wrqu.ap_addr.sa_family = ARPHRD_ETHER;
	if (ieee->state == RTLLIB_LINKED)
		memcpy(wrqu.ap_addr.sa_data, ieee->current_network.bssid, ETH_ALEN);
	else{

		printk("%s(): Tell user space disconnected\n",__func__);
		memset(wrqu.ap_addr.sa_data, 0, ETH_ALEN);
	}
	wireless_send_event(ieee->dev, SIOCGIWAP, &wrqu, NULL);
}

#ifndef BUILT_IN_RTLLIB
EXPORT_SYMBOL_RSL(rtllib_get_beacon);
EXPORT_SYMBOL_RSL(rtllib_wake_queue);
EXPORT_SYMBOL_RSL(rtllib_stop_queue);
EXPORT_SYMBOL_RSL(rtllib_reset_queue);
EXPORT_SYMBOL_RSL(rtllib_softmac_stop_protocol);
EXPORT_SYMBOL_RSL(rtllib_softmac_start_protocol);
EXPORT_SYMBOL_RSL(rtllib_is_shortslot);
EXPORT_SYMBOL_RSL(rtllib_is_54g);
EXPORT_SYMBOL_RSL(rtllib_wpa_supplicant_ioctl);
EXPORT_SYMBOL_RSL(rtllib_ps_tx_ack);
EXPORT_SYMBOL_RSL(rtllib_softmac_xmit);
EXPORT_SYMBOL_RSL(rtllib_stop_send_beacons);
EXPORT_SYMBOL_RSL(notify_wx_assoc_event);
EXPORT_SYMBOL_RSL(SendDisassociation);
EXPORT_SYMBOL_RSL(rtllib_disassociate);
EXPORT_SYMBOL_RSL(rtllib_start_send_beacons);
EXPORT_SYMBOL_RSL(rtllib_stop_scan);
EXPORT_SYMBOL_RSL(rtllib_send_probe_requests);
EXPORT_SYMBOL_RSL(rtllib_softmac_scan_syncro);
EXPORT_SYMBOL_RSL(rtllib_start_scan_syncro);
EXPORT_SYMBOL_RSL(rtllib_sta_ps_send_null_frame);
EXPORT_SYMBOL_RSL(rtllib_sta_ps_send_pspoll_frame);
EXPORT_SYMBOL_RSL(rtllib_sta_wakeup);
EXPORT_SYMBOL_RSL(rtllib_ap_sec_type);
#ifdef _RTL8192_EXT_PATCH_
EXPORT_SYMBOL_RSL(rtllib_MFIE_rate_len);
EXPORT_SYMBOL_RSL(rtllib_MFIE_Brate);
EXPORT_SYMBOL_RSL(rtllib_MFIE_Grate);
EXPORT_SYMBOL_RSL(rtllib_WMM_Info);
EXPORT_SYMBOL_RSL(rtllib_TURBO_Info);
EXPORT_SYMBOL_RSL(rtllib_ext_probe_resp_by_net);
EXPORT_SYMBOL_RSL(softmac_mgmt_xmit);
EXPORT_SYMBOL_RSL(rtllib_start_scan);
EXPORT_SYMBOL_RSL(rtllib_ext_send_11s_beacon);
EXPORT_SYMBOL_RSL(rtllib_rx_auth_rq);
EXPORT_SYMBOL_RSL(rtllib_associate_step1);
EXPORT_SYMBOL_RSL(rtllib_stop_protocol);
EXPORT_SYMBOL_RSL(rtllib_start_protocol);
EXPORT_SYMBOL_RSL(rtllib_resp_to_auth);

#endif 
#endif
