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
#ifdef ASL  

#include <linux/in.h>
#include <linux/ip.h>
#include "rtl_softap.h"
#include "rtl_core.h"
#include "rtl_wx.h"
#include "../../rtllib/rtllib.h"
#define TKIP_KEY_LEN 32
int needsendBeacon=0;

void ap_send_to_hostapd(struct rtllib_device *ieee, struct sk_buff *skb)
{

	printk("\nRomit : Inside send_to_hostapd.");
	ieee->apdev->last_rx = jiffies;
	skb->dev = ieee->apdev;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
        skb_reset_mac_header(skb);
#else
        skb->mac.raw = skb->data;
#endif


	skb->pkt_type = PACKET_OTHERHOST;
	skb->protocol = __constant_htons(ETH_P_802_2);
	memset(skb->cb, 0, sizeof(skb->cb));
	if (netif_rx(skb) != 0)
	{
		printk("\nnetif_rx failed");
		dev_kfree_skb_any(skb);
	}
	printk("\nExiting send_to_hostapd");
	return;
}
/*For softap's WPA.added by Lawrence.*/
int ap_stations_cryptlist_is_empty(struct rtllib_device *ieee)
{
	int i;
	
	for(i=1;i<APDEV_MAX_ASSOC;i++)
		if(ieee->stations_crypt[i]->used)
			return 0;
	return 1;	
}

extern int ap_cryptlist_find_station(struct rtllib_device *ieee,const u8 *addr,u8 justfind)
{
        int i;
        int ret=-1;
        for (i = 0; i < APDEV_MAX_ASSOC; i++) {
		if(memcmp(ieee->stations_crypt[i]->mac_addr,addr, ETH_ALEN) == 0)
			return i;       /*Find!!*/ 
	}
	if (!justfind) {
		for (i = 0; i < APDEV_MAX_ASSOC; i++) {
			   if (ieee->stations_crypt[i]->used ==0)
			   	return i; 
		}
	}
        if(ret == APDEV_MAX_ASSOC)/*No empty space.*/
                ret = -1;
        return ret;/*Not find,then return a empty space.*/
}
int apdev_rx_mgnt_get_crypt(
		struct rtllib_device *ieee, 
		struct sk_buff *skb, 
		struct rtllib_crypt_data **crypt, 
		size_t hdrlen)
{
	struct rtllib_hdr_4addr *hdr = (struct rtllib_hdr_4addr *)skb->data;
	u16 fc = le16_to_cpu(hdr->frame_ctl);
	int idx = 0;
	u8 stype = WLAN_FC_GET_STYPE(fc);
	u8 type = WLAN_FC_GET_TYPE(fc);
#if defined (RTL8192S_WAPI_SUPPORT)
	if (ieee->host_decrypt && (!ieee->wapiInfo.bWapiEnable))
#else
	if (ieee->host_decrypt)
#endif
	{
		if (skb->len >= hdrlen + 3)
			idx = skb->data[hdrlen + 3] >> 6;
		
		if ((stype == RTLLIB_STYPE_AUTH) && (type == RTLLIB_FTYPE_MGMT) && ieee->iswep) {
			printk("====>haha1\n");
			dump_buf(skb->data,skb->len);
			if ((fc & RTLLIB_FCTL_WEP)) {
				printk("====>haha2\n");
				*crypt = ieee->stations_crypt[0]->crypt[0];
			} else { 
				printk("====>haha3\n");
				*crypt = NULL;
			}
		}
	}
	/* allow NULL decrypt to indicate an station specific override
	 * for default encryption */
	if (*crypt && ((*crypt)->ops == NULL ||
		      (*crypt)->ops->decrypt_mpdu == NULL)){
		printk("haha4\n");
		*crypt = NULL;
	}
	if (!*crypt && (fc & RTLLIB_FCTL_WEP) && (type == RTLLIB_FTYPE_MGMT)) {
		/* This seems to be triggered by some (multicast?)
		 * frames from other than current BSS, so just drop the
		 * frames silently instead of filling system log with
		 * these reports. */
		RTLLIB_DEBUG_DROP("Decryption failed (not set)"
				     " (SA=" MAC_FMT ")\n",
				     MAC_ARG(hdr->addr2));
		printk("===>haha5\n");
		ieee->ieee_stats.rx_discards_undecryptable++;
		return -1;
	}
	return 0;
}
static u8 ap_is_stainfo_exist(struct rtllib_device *rtllib, u8 *addr)
{
	int k=0;
	struct sta_info * psta = NULL;
	u8 sta_idx = APDEV_MAX_ASSOC;
	
	for (k=0; k<APDEV_MAX_ASSOC; k++) {
		psta = rtllib->apdev_assoc_list[k];
		if (NULL != psta) {
			if (memcmp(addr, psta->macaddr, ETH_ALEN) == 0) {
				sta_idx = k;
				break;
			}
		}
	}
	return sta_idx;
}
struct sta_info *ap_get_stainfo(struct rtllib_device *ieee, u8 *addr)
{
	int k=0;
	struct sta_info * psta = NULL;
	struct sta_info * psta_find = NULL;
	
	for (k=0; k<APDEV_MAX_ASSOC; k++) {
		psta = ieee->apdev_assoc_list[k];
		if (NULL != psta) {
			if (memcmp(addr, psta->macaddr, ETH_ALEN) == 0) {
				psta_find = psta;
				break;
			}
		}
	}
	return psta_find;
}
static u8 ap_get_free_stainfo_idx(struct rtllib_device *rtllib, u8 *addr)
{
	int k = 0;
	while((rtllib->apdev_assoc_list[k] != NULL) && (k < APDEV_MAX_ASSOC))
		k++;
	return k;
}

static void ap_del_stainfo(struct rtllib_device *rtllib, u8 *addr)
{
	int idx = 0;
	struct sta_info * psta = NULL;
	
	for(idx = 0; idx < APDEV_MAX_ASSOC; idx++)
	{
		psta = rtllib->apdev_assoc_list[idx];
		if(psta != NULL) 
		{
 			if(memcmp(psta->macaddr, addr, ETH_ALEN) == 0)
 			{
#ifdef ASL_DEBUG_2
				printk("\nRemoved STA %x:%x:%x:%x:%x:%x!!!", psta->macaddr[0],psta->macaddr[1],psta->macaddr[2],psta->macaddr[3],psta->macaddr[4],psta->macaddr[5]);
#endif
				skb_queue_purge(&(psta->PsQueue));
				skb_queue_purge(&(psta->WmmPsQueue));
				kfree(psta);
				rtllib->apdev_assoc_list[idx] = NULL;
			}
		}
	}
}
static void ap_set_del_sta_list(struct rtllib_device *rtllib, u8 *del_addr)
{
	int idx = 0;
	u8* addr;
	for (idx = 0; idx < APDEV_MAX_ASSOC; idx++) {
		if (rtllib->to_be_del_cam_entry[idx].isused) {
			addr = rtllib->to_be_del_cam_entry[idx].addr;
			if (memcmp(addr, del_addr, ETH_ALEN) == 0) {
				printk("%s(): addr "MAC_FMT" is exist in to_be_del_entry\n",__func__,MAC_ARG(del_addr));
				return;
			}	
		}
	}
	if (idx == APDEV_MAX_ASSOC) {
		for (idx = 0; idx < APDEV_MAX_ASSOC; idx++) {
			if (rtllib->to_be_del_cam_entry[idx].isused == false) {
				memcpy(rtllib->to_be_del_cam_entry[idx].addr, del_addr,ETH_ALEN);
				rtllib->to_be_del_cam_entry[idx].isused = true;
			}
		}
	}
}
void ap_del_stainfo_list(struct rtllib_device *rtllib)
{
	int idx = 0;
	struct sta_info * psta = NULL;
	
	for(idx = 0; idx < APDEV_MAX_ASSOC; idx++)
	{
		psta = rtllib->apdev_assoc_list[idx];
		if(psta != NULL) 
		{
			kfree(psta);
			rtllib->apdev_assoc_list[idx] = NULL;
		}
	}
}
#ifdef RATE_ADAPTIVE_FOR_AP
void ap_init_stainfo(struct rtllib_device *ieeedev,int index)
{
	int idx = index;
	ieeedev->apdev_assoc_list[idx]->StaDataRate = 0;
	ieeedev->apdev_assoc_list[idx]->StaSS = 0;
	ieeedev->apdev_assoc_list[idx]->RetryFrameCnt = 0;
	ieeedev->apdev_assoc_list[idx]->NoRetryFrameCnt = 0;
	ieeedev->apdev_assoc_list[idx]->LastRetryCnt = 0;
	ieeedev->apdev_assoc_list[idx]->LastNoRetryCnt = 0;
	ieeedev->apdev_assoc_list[idx]->AvgRetryRate = 0;
	ieeedev->apdev_assoc_list[idx]->LastRetryRate = 0;
	ieeedev->apdev_assoc_list[idx]->txRateIndex = 11;
	ieeedev->apdev_assoc_list[idx]->APDataRate = 0x2; 
	ieeedev->apdev_assoc_list[idx]->ForcedDataRate = 0x2; 
	
}
void ap_init_rateadaptivestate(struct net_device *dev,struct sta_info  *pEntry)
{
	prate_adaptive	pRA = (prate_adaptive)&pEntry->rate_adaptive;

	pRA->ratr_state = DM_RATR_STA_MAX;
	pRA->PreRATRState = DM_RATR_STA_MAX;
	pEntry->rssi_stat.UndecoratedSmoothedPWDB = -1;

}
void ap_reset_stainfo(struct rtllib_device *rtllib,int index)
{
}

u8 ap_frame_retry(u16 fc)
{
	return (fc&0x0800)?1:0;
}


void ap_update_stainfo(struct rtllib_device * ieee,struct rtllib_hdr_4addr *hdr,struct rtllib_rx_stats *rx_stats)
{
	u8		*MacAddr = hdr->addr2 ;
	struct sta_info *pEntry;
	u16 fc;
	u16 supportRate[] = {10,20,55,110,60,90,120,180,240,360,480,540};
	
	u8 * ptmp = (u8*)hdr;
	int i;
	fc = le16_to_cpu(hdr->frame_ctl);

	pEntry = ap_get_stainfo(ieee, MacAddr);
	if(pEntry == NULL)
		return;
	else {
		if((pEntry->LastRetryCnt==pEntry->RetryFrameCnt) &&(pEntry->LastNoRetryCnt==pEntry->NoRetryFrameCnt)) {
			pEntry->RetryFrameCnt=0;
			pEntry->NoRetryFrameCnt=0;
		}
	
		pEntry->StaDataRate = supportRate[rx_stats->rate];
		pEntry->StaSS = rx_stats->SignalStrength;
		if(rx_stats->rate <= 11)
                        pEntry->txRateIndex = 11-rx_stats->rate;
		
		

		if(ap_frame_retry(fc))
			pEntry->RetryFrameCnt++;
		else
			pEntry->NoRetryFrameCnt++;
		
	}
}
u16 ap_sta_rate_adaptation(
	struct rtllib_device *ieee, 
	struct sta_info 	*pEntry, 
	bool			isForce
	)
{
	int retryRate=0, avgRetryRate=0, retryThreshold, avgRetryThreshold;
	u8 i, meanRateIdx=0, txRateIdx=0;
	u16 CurrentRetryCnt, CurrentNoRetryCnt;
	u16 supportRate[12] = {540,480,360,240,180,120,90,60,110,55,20,10};


	CurrentRetryCnt=pEntry->RetryFrameCnt;
	CurrentNoRetryCnt=pEntry->NoRetryFrameCnt;
	if((CurrentRetryCnt + CurrentNoRetryCnt) !=0)
		retryRate=(CurrentRetryCnt*100/(CurrentRetryCnt+CurrentNoRetryCnt));


	if(pEntry->AvgRetryRate!=0)
 		avgRetryRate=(retryRate+pEntry->AvgRetryRate)/2;
	else
		avgRetryRate=retryRate;

	pEntry->LastRetryCnt=CurrentRetryCnt;
	pEntry->LastNoRetryCnt=CurrentNoRetryCnt;
	
	if(isForce)
	{
		pEntry->AvgRetryRate=avgRetryRate;
		pEntry->LastRetryRate=retryRate;
		return pEntry->ForcedDataRate & 0x7F;
	}



	retryThreshold=40;
	avgRetryThreshold=40;


	/* The test of forcedDataRate indicates that 
	*/
	if(pEntry->StaSS >40)
	{
		meanRateIdx=0; 
		retryThreshold=50;
		avgRetryThreshold=50;
	}
	else if(pEntry->StaSS >27)
	{	
		meanRateIdx=1; 
	}else if(pEntry->StaSS >18)
	{	
		meanRateIdx=2; 
	}
	else
		meanRateIdx=3; 


	/*for(i=0, top=DATALEN_BKT0; i<DATALEN_BKTS; i++, top <<=DATALEN_BKTPOWER)
	{
		txRateIdx=i;
		if(PacketLen <= top) break;
	}*/

	if(pEntry->StaSS >18)
	{
		txRateIdx=pEntry->txRateIndex;

		if(retryRate>=retryThreshold)
		{
			if((retryRate-pEntry->LastRetryRate)>=retryThreshold)
			{
				if(txRateIdx==5) 
					txRateIdx=8;
				else if(txRateIdx < 11)
					txRateIdx++;
			}
		}else
		{
			if(txRateIdx ==8) 
				txRateIdx=5;
			else if(txRateIdx > 0)
				txRateIdx--;
		}
	
		if(txRateIdx > 11) txRateIdx=11;
		
		if(txRateIdx < meanRateIdx)
			txRateIdx=meanRateIdx;
	}
	else
	{
		for(i=0; i < 12; i++)
		{
			txRateIdx=i;
			if(supportRate[i]==pEntry->StaDataRate)
				break;
		}	
	}

	pEntry->APDataRate=supportRate[txRateIdx];
	pEntry->txRateIndex=txRateIdx;
	pEntry->AvgRetryRate=avgRetryRate;
	pEntry->LastRetryRate=retryRate;


	return pEntry->APDataRate;
}

void ap_set_dataframe_txrate( struct rtllib_device *ieee, struct sta_info *pEntry )
{
		u16 rate;
		if (ieee->ForcedDataRate == 0) {
		
			if (!(is_multicast_ether_addr(pEntry->macaddr) ||
				is_broadcast_ether_addr(pEntry->macaddr) )) { 
				
				if (pEntry != NULL) {
					bool bStaForceRate =0;
					rate = ap_sta_rate_adaptation(
							ieee, 
							pEntry, 
							bStaForceRate); 
					


				}
			}
		   	else {
                    	rate = ieee->rate;
			}
		} 
		else {
			u16 CurrentRetryCnt, CurrentNoRetryCnt;
			int retryRate=0, avgRetryRate=0;

			rate = (u8)(ieee->ForcedDataRate);

			if(pEntry != NULL){
				CurrentRetryCnt=pEntry->RetryFrameCnt;
				CurrentNoRetryCnt=pEntry->NoRetryFrameCnt;
				retryRate=(CurrentRetryCnt*100/(CurrentRetryCnt+CurrentNoRetryCnt));
				avgRetryRate=(retryRate+pEntry->AvgRetryRate)/2;

				pEntry->LastRetryCnt=CurrentRetryCnt;
				pEntry->LastNoRetryCnt=CurrentNoRetryCnt;
				pEntry->AvgRetryRate=avgRetryRate;
			}
		}

		ieee->rate = rate;
}

#endif
void ap_reset_admit_TRstream_rx(struct rtllib_device *rtllib, u8 *Addr)
{
	u8 	dir;
	bool				search_dir[4] = {0, 0, 0, 0};
	struct list_head*		psearch_list; 
	PTS_COMMON_INFO	pRet = NULL;
	PRX_TS_RECORD pRxTS = NULL;

	if((rtllib->iw_mode != IW_MODE_MASTER) && (rtllib->iw_mode != IW_MODE_APSTA)) 
		return;

	search_dir[DIR_UP] 	= true;
	search_dir[DIR_BI_DIR]	= true;
	psearch_list = &rtllib->Rx_TS_Admit_List;

	for(dir = 0; dir <= DIR_BI_DIR; dir++)
	{
		if(search_dir[dir] ==false )
			continue;
		list_for_each_entry(pRet, psearch_list, List){
			if ((memcmp(pRet->Addr, Addr, 6) == 0) && (pRet->TSpec.f.TSInfo.field.ucDirection == dir))
			{
				pRxTS = (PRX_TS_RECORD)pRet;
				pRxTS->RxIndicateSeq = 0xffff;
				pRxTS->RxTimeoutIndicateSeq = 0xffff;
			}
					
		}	
	}

	return;
}
void ap_ps_return_allqueuedpackets(struct rtllib_device *rtllib, u8 bMulticastOnly)
{
	int	i = 0;
	struct sta_info *pEntry = NULL;

	skb_queue_purge(&rtllib->GroupPsQueue);

	if (!bMulticastOnly) {
		for(i = 0; i < APDEV_MAX_ASSOC; i++)
		{
			pEntry = rtllib->apdev_assoc_list[i];

			if(pEntry != NULL)
			{
				skb_queue_purge(&rtllib->apdev_assoc_list[i]->PsQueue);
				skb_queue_purge(&rtllib->apdev_assoc_list[i]->WmmPsQueue);
			}
		}
	}
}
inline struct sk_buff *apdev_rtllib_disauth_skb( u8* sta_addr,
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
	if (sta_addr == NULL)
		return NULL;
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
	
	memcpy(disauth->header.addr1, sta_addr, ETH_ALEN);
	memcpy(disauth->header.addr2, ieee->dev->dev_addr, ETH_ALEN);
	memcpy(disauth->header.addr3, ieee->current_ap_network.bssid, ETH_ALEN);
	
	disauth->reason = cpu_to_le16(asRsn);
	return skb;
}
inline struct sk_buff *apdev_rtllib_disassociate_skb( u8* sta_addr,
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
	if (sta_addr == NULL)
		return NULL;
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
	
	memcpy(disass->header.addr1, sta_addr, ETH_ALEN);
	memcpy(disass->header.addr2, ieee->dev->dev_addr, ETH_ALEN);
	memcpy(disass->header.addr3, ieee->current_ap_network.bssid, ETH_ALEN);
	
	disass->reason = cpu_to_le16(asRsn);
	return skb;
}

void apdev_send_disassociation_to_all_sta(struct rtllib_device *ieee, bool deauth, u16 asRsn)
{
	struct sk_buff *skb;
	u8 sta_addr[ETH_ALEN] = {0};
	int idx = 0;
	struct sta_info * psta = NULL;
	
	for (idx = 0; idx < APDEV_MAX_ASSOC; idx++) {
		psta = ieee->apdev_assoc_list[idx];
		if (psta != NULL) {
 			memcpy(sta_addr, psta->macaddr, ETH_ALEN);
			if(deauth)
				skb = apdev_rtllib_disauth_skb(sta_addr, ieee, asRsn);
			else
				skb = apdev_rtllib_disassociate_skb(sta_addr, ieee, asRsn);
			if (skb)
				softmac_mgmt_xmit(skb, ieee, false);
		}
	}
	
}
void apdev_notify_wx_assoc_event(struct rtllib_device *ieee)
{
	union iwreq_data wrqu;

	if(ieee->cannot_notify)
		return;

	wrqu.ap_addr.sa_family = ARPHRD_ETHER;
	if (ieee->ap_state == RTLLIB_LINKED) {
		printk("%s(): Tell user space ap connected\n",__func__);
		memcpy(wrqu.ap_addr.sa_data, ieee->current_ap_network.bssid, ETH_ALEN);
	} else{

		printk("%s(): Tell user space ap disconnected\n",__func__);
		memset(wrqu.ap_addr.sa_data, 0, ETH_ALEN);
	}

	wireless_send_event(ieee->apdev, SIOCGIWAP, &wrqu, NULL);
}
void apdev_rtllib_disassociate(struct rtllib_device *ieee)
{
	netif_carrier_off(ieee->apdev);
	printk("===>%s()\n",__FUNCTION__);
	if (!ieee->proto_started) {
		if (ieee->softmac_features & IEEE_SOFTMAC_TX_QUEUE)
			rtllib_reset_queue(ieee);
	}
#ifdef TO_DO_LIST	
	if (ieee->data_hard_stop)
			ieee->data_hard_stop(ieee->apdev);
#ifdef ENABLE_DOT11D
	if(IS_DOT11D_ENABLE(ieee))
		Dot11d_Reset(ieee);
#endif
#endif
	ieee->ap_state = RTLLIB_NOLINK;
	ieee->ap_link_change(ieee->dev);

#ifndef FOR_ANDROID_X86
	apdev_notify_wx_assoc_event(ieee);
#endif
}
void apdev_rtllib_stop_protocol(struct rtllib_device *ieee, u8 shutdown)
{
	int idx = 0;
	struct sta_info * psta = NULL;
	printk("======>%s()\n",__func__);	
	if (!ieee->ap_proto_started)
		return;
	
	if(shutdown){
		ieee->ap_proto_started = 0;
		ieee->ap_proto_stoppping = 1;
	}
	
	rtllib_stop_send_beacons(ieee);
	rtllib_stop_scan(ieee);

	
	if (ieee->ap_state == RTLLIB_LINKED){
		apdev_send_disassociation_to_all_sta(ieee,1,deauth_lv_ss);
		apdev_rtllib_disassociate(ieee);
		ap_del_stainfo_list(ieee);
	}
	if(shutdown){
		if (!ieee->proto_started)
			RemoveAllTS(ieee); 
		else {		
			for(idx = 0; idx < APDEV_MAX_ASSOC; idx++)
			{
				psta = ieee->apdev_assoc_list[idx];
				if(psta != NULL) {
					RemovePeerTS(ieee,psta->macaddr);
				}
			}
		}
		ieee->ap_proto_stoppping = 0;
	}
}
void apdev_rtllib_start_protocol(struct rtllib_device *ieee)
{
	short ch = 0;

	if (!ieee->proto_started)
		rtllib_update_active_chan_map(ieee);

	if (ieee->ap_proto_started)
		return;
		
	ieee->ap_proto_started = 1;
	if (!ieee->proto_started) {
		if (ieee->current_ap_network.channel == 0) {
			do {
				ch++;
				if (ch > MAX_CHANNEL_NUMBER) 
					return; /* no channel found */
			} while(!ieee->active_channel_map[ch]);
			ieee->current_ap_network.channel = ch;
		}
	} else { 
		if (!ieee->current_network.channel)
			ieee->current_ap_network.channel = ieee->current_network.channel;
	}
	
	if (ieee->current_ap_network.beacon_interval == 0)
		ieee->current_ap_network.beacon_interval = 100;
	

	
	if(ieee->UpdateBeaconInterruptHandler) {
		ieee->UpdateBeaconInterruptHandler(ieee->dev, false);
	}		
	ieee->wmm_acm = 0;
	/* if the user set the MAC of the ad-hoc cell and then
	 * switch to managed mode, shall we  make sure that association
	 * attempts does not fail just because the user provide the essid
	 * and the nic is still checking for the AP MAC ??
	 */

	rtllib_start_master_bss(ieee);
}
int apdev_rtllib_wx_get_wap(struct rtllib_device *ieee, 
			    struct iw_request_info *info, 
			    union iwreq_data *wrqu, char *extra)
{
	unsigned long flags;	
	
	wrqu->ap_addr.sa_family = ARPHRD_ETHER;
	
	/* We want avoid to give to the user inconsistent infos*/
	spin_lock_irqsave(&ieee->lock, flags);
	
	if (ieee->ap_state != RTLLIB_LINKED)
		memset(wrqu->ap_addr.sa_data, 0, ETH_ALEN);
	else
		memcpy(wrqu->ap_addr.sa_data, 
		       ieee->current_ap_network.bssid, ETH_ALEN);
	
	spin_unlock_irqrestore(&ieee->lock, flags);
	
	return 0;
}
int apdev_rtllib_wx_get_essid(struct rtllib_device *ieee, struct iw_request_info *a,union iwreq_data *wrqu,char *b)
{
	int len,ret = 0;
	unsigned long flags;
	
	/* We want avoid to give to the user inconsistent infos*/	
	spin_lock_irqsave(&ieee->lock, flags);
	
	if (ieee->current_ap_network.ssid[0] == '\0' ||
		ieee->current_ap_network.ssid_len == 0){ 
		ret = -1;
		goto out;
	}
	
	if (ieee->ap_state != RTLLIB_LINKED && 
		ieee->ap_ssid_set == 0){
		ret = -1;
		goto out;
	}
	len = ieee->current_ap_network.ssid_len;
	wrqu->essid.length = len;
	strncpy(b,ieee->current_ap_network.ssid,len);
	wrqu->essid.flags = 1;

out:
	spin_unlock_irqrestore(&ieee->lock, flags);
	
	return ret;
	
}
int apdev_rtllib_wx_set_essid(struct rtllib_device *ieee, 
			      struct iw_request_info *a,
			      union iwreq_data *wrqu, char *extra)
{
	
	int ret=0,len,i;
	short ap_proto_started;
	unsigned long flags;
	
	rtllib_stop_scan_syncro(ieee);
	down(&ieee->wx_sem);
	printk("====>%s()\n",__func__);	
	ap_proto_started = ieee->ap_proto_started;
	
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
	len = ((wrqu->essid.length-1) < IW_ESSID_MAX_SIZE) ? (wrqu->essid.length-1) : IW_ESSID_MAX_SIZE;
#else
	len = (wrqu->essid.length < IW_ESSID_MAX_SIZE) ? wrqu->essid.length : IW_ESSID_MAX_SIZE;
#endif

	if (len > IW_ESSID_MAX_SIZE){
		ret= -E2BIG;
		goto out;
	}
	
	for (i=0; i<len; i++){
		if(extra[i] < 0){
			ret= -1;
			goto out;
		}
	}
	
	if(ap_proto_started)
		apdev_rtllib_stop_protocol(ieee,true);
	
	
	/* this is just to be sure that the GET wx callback
	 * has consisten infos. not needed otherwise
	 */
	spin_lock_irqsave(&ieee->lock, flags);
	
	if (wrqu->essid.flags && wrqu->essid.length) {
		strncpy(ieee->current_ap_network.ssid, extra, len);
		ieee->current_ap_network.ssid_len = len;
#if 0
		{
			int i;
			for (i=0; i<len; i++)
				printk("%c:%d ", extra[i], extra[i]);
			printk("\n");
		}
#endif
		ieee->cannot_notify = false;
		ieee->ap_ssid_set = 1;
	}
	else{ 
		ieee->ap_ssid_set = 0;
		ieee->current_ap_network.ssid[0] = '\0';
		ieee->current_ap_network.ssid_len = 0;
	}
	spin_unlock_irqrestore(&ieee->lock, flags);
	
	if (ap_proto_started)
		apdev_rtllib_start_protocol(ieee);
out:
	up(&ieee->wx_sem);
	return ret;
}
int apdev_rtllib_wx_set_freq(struct rtllib_device *ieee, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	int ret;
	struct iw_freq *fwrq = & wrqu->freq;

	down(&ieee->wx_sem);
	printk("====>%s()\n",__func__);	
	/* if setting by freq convert to channel */
	if (fwrq->e == 1) {
		if ((fwrq->m >= (int) 2.412e8 &&
		     fwrq->m <= (int) 2.487e8)) {
			int f = fwrq->m / 100000;
			int c = 0;
			
			while ((c < 14) && (f != rtllib_wlan_frequencies[c]))
				c++;
			
			/* hack to fall through */
			fwrq->e = 0;
			fwrq->m = c + 1;
		}
	}
	
	if (fwrq->e > 0 || fwrq->m > 14 || fwrq->m < 1 ){ 
		ret = -EOPNOTSUPP;
		goto out;
	
	}else { /* Set the channel */
		
#ifdef ENABLE_DOT11D
		if (ieee->active_channel_map[fwrq->m] != 1) {
			ret = -EINVAL;
			goto out;
		}
#endif
		if ((ieee->iw_mode == IW_MODE_APSTA && ieee->state != RTLLIB_LINKED) || ieee->iw_mode == IW_MODE_MASTER) {
			ieee->current_ap_network.channel = fwrq->m;
			ieee->current_network.channel = fwrq->m;
			ieee->set_chan(ieee->dev, ieee->current_ap_network.channel);
		}
		
		if (ieee->iw_mode == IW_MODE_MASTER || ieee->iw_mode == IW_MODE_APSTA) {
			if(ieee->ap_state == RTLLIB_LINKED){
				if (ieee->bbeacon_start) {
					rtllib_stop_send_beacons(ieee);
					rtllib_start_send_beacons(ieee);
				}
			}
		}
	}
	printk("==============>ieee->current_ap_network.channel is %d, ieee->current_network.channelis %d\n",ieee->current_ap_network.channel,ieee->current_network.channel);
	ret = 0;
out:
	up(&ieee->wx_sem);
	return ret;
}

int apdev_up(struct net_device *apdev,bool is_silent_reset)
{
	struct apdev_priv * appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	PRT_POWER_SAVE_CONTROL pPSC = (PRT_POWER_SAVE_CONTROL)(&(ieee->PowerSaveControl));
	bool init_status;
	struct net_device *dev = ieee->dev; 
	RT_RF_POWER_STATE	rtState = 0;;

	RT_TRACE(COMP_INIT, "==========>%s()\n", __FUNCTION__);
	printk("==========>%s()\n", __FUNCTION__);

	appriv->priv->up_first_time = 0;
	if (appriv->priv->apdev_up) {
		RT_TRACE(COMP_ERR,"%s():apdev is up,return\n",__FUNCTION__);
		return -1;
	}
#ifdef RTL8192SE        
        appriv->priv->ReceiveConfig =
		RCR_APPFCS | RCR_APWRMGT | /*RCR_ADD3 |*/
		RCR_AMF | RCR_ADF | RCR_APP_MIC | RCR_APP_ICV |
		RCR_AICV | RCR_ACRC32    |                               
		RCR_AB          | RCR_AM                |                               
		RCR_APM         | RCR_ACF |                                                              
		/*RCR_AAP               |*/                                                     
		RCR_APP_PHYST_STAFF | RCR_APP_PHYST_RXFF |      
		(appriv->priv->EarlyRxThreshold<<RCR_FIFO_OFFSET)       ;
#elif defined RTL8192CE
	appriv->priv->ReceiveConfig = (\
		RCR_APPFCS	
		| RCR_AMF | RCR_ADF| RCR_APP_MIC| RCR_APP_ICV
		| RCR_AICV | RCR_ACRC32 
		| RCR_AB | RCR_AM			
		| RCR_APM					
		| RCR_APP_PHYST_RXFF		
		| RCR_HTC_LOC_CTRL
		);
#else
        appriv->priv->ReceiveConfig = RCR_ADD3  |
		RCR_AMF | RCR_ADF |             
		RCR_AICV |                      
		RCR_AB | RCR_AM | RCR_APM |     
		RCR_AAP | ((u32)7<<RCR_MXDMA_OFFSET) |
		((u32)7 << RCR_FIFO_OFFSET) | RCR_ONLYERLPKT;

#endif
	if(!appriv->priv->up) {
		RT_TRACE(COMP_INIT, "Bringing up iface");
		printk("Bringing up iface");
		ieee->iw_mode = IW_MODE_MASTER;
		appriv->priv->bfirst_init = true;
		init_status = appriv->priv->ops->initialize_adapter(dev);
		if(init_status != true) {
			RT_TRACE(COMP_ERR,"ERR!!! %s(): initialization is failed!\n",__FUNCTION__);
			return -1;
		}
		RT_TRACE(COMP_INIT, "start adapter finished\n");
		RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);
		printk("==>%s():priv->up is 0\n",__FUNCTION__);
		appriv->rtllib->ieee_up=1;		
		appriv->priv->bfirst_init = false;
#if defined RTL8192SE || defined RTL8192CE
		if(ieee->eRFPowerState!=eRfOn)
			MgntActSet_RF_State(dev, eRfOn, ieee->RfOffReason, true);	
#endif
#ifdef ENABLE_GPIO_RADIO_CTL
		if(appriv->priv->polling_timer_on == 0){
			check_rfctrl_gpio_timer((unsigned long)dev);
		}
#endif
		if(!is_silent_reset){
			appriv->rtllib->current_network.channel = INIT_DEFAULT_CHAN;
			appriv->rtllib->current_ap_network.channel = INIT_DEFAULT_CHAN;
			printk("%s():set chan %d\n",__FUNCTION__,INIT_DEFAULT_CHAN);
			appriv->rtllib->set_chan(dev, INIT_DEFAULT_CHAN);
		}
		dm_InitRateAdaptiveMask(dev);
		watch_dog_timer_callback((unsigned long) dev);
	
	} else {
		printk("=========>wlan0 is already up\n");
#ifdef ENABLE_IPS	
		rtState = ieee->eRFPowerState;
		if(ieee->PowerSaveControl.bInactivePs){ 
			if(rtState == eRfOff){
				if(ieee->RfOffReason > RF_CHANGE_BY_IPS)
				{
					RT_TRACE(COMP_ERR, "%s(): RF is OFF.\n",__FUNCTION__);
					return -1;
				} else {
					printk("=========>%s(): IPSLeave\n",__FUNCTION__);
					down(&ieee->ips_sem);
					IPSLeave(dev);
					up(&ieee->ips_sem);				
				}
			}
		}
#endif
#ifdef ENABLE_LPS
		if (ieee->state == RTLLIB_LINKED) {
			if (ieee->LeisurePSLeave) {
				ieee->LeisurePSLeave(ieee->dev);
			}
			/* notify AP to be in PS mode */
			rtllib_sta_ps_send_null_frame(ieee, 1);
			rtllib_sta_ps_send_null_frame(ieee, 1);
		}
#endif
		rtllib_stop_scan(ieee);
		ieee->iw_mode = IW_MODE_APSTA;
		msleep(1000);
		
	}
	appriv->priv->apdev_up = 1;
	
	
	if(!ieee->ap_proto_started) {
#ifdef RTL8192E
            if(ieee->eRFPowerState!=eRfOn) {
             	printk("====>rf on\n");
		MgntActSet_RF_State(dev, eRfOn, ieee->RfOffReason,true);	
	}
#endif
#ifndef CONFIG_MP
		rtllib_softmac_start_protocol(ieee, 0, 1);
#endif

	if (!ieee->proto_started)
		rtllib_reset_queue(ieee);
	}
	if(!netif_queue_stopped(apdev))
	    netif_start_queue(apdev);
	else
	    netif_wake_queue(apdev);

	RT_TRACE(COMP_INIT, "<==========%s()\n", __FUNCTION__);
	return 0;
}
static int apdev_open(struct net_device *apdev)
{
	struct apdev_priv * appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct r8192_priv *priv = (void*)ieee->priv;
	int ret;
	SEM_DOWN_PRIV_WX(&priv->wx_sem);	
	ret = apdev_up(apdev, false);
	SEM_UP_PRIV_WX(&priv->wx_sem);	
	
	return ret;
}
int apdev_down(struct net_device *apdev)
{
	struct apdev_priv * appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct r8192_priv *priv = (void*)ieee->priv;
	struct net_device *dev = ieee->dev;
	unsigned long flags = 0;
	u8 RFInProgressTimeOut = 0;
	if(priv->apdev_up == 0) {
		RT_TRACE(COMP_ERR,"%s():ERR!!! apdev is already down\n",__FUNCTION__)
		return -1;
	}

	RT_TRACE(COMP_DOWN, "==========>%s()\n", __FUNCTION__);
	
	if (netif_running(apdev)) {
		netif_stop_queue(apdev);
	}
	if(!priv->up)
	{
		if(priv->rtllib->LedControlHandler)
			priv->rtllib->LedControlHandler(dev, LED_CTL_POWER_OFF);
		printk("===>%s():priv->up is 0\n",__FUNCTION__);
        	priv->bDriverIsGoingToUnload = true;	
		ieee->ieee_up = 0;  
		priv->bfirst_after_down = 1;
		priv->rtllib->wpa_ie_len = 0;
		if(priv->rtllib->wpa_ie)
			kfree(priv->rtllib->wpa_ie);
		priv->rtllib->wpa_ie = NULL;
		CamResetAllEntry(dev);	
		memset(priv->rtllib->swapcamtable,0,sizeof(SW_CAM_TABLE)*32);
		if(priv->rtllib->iw_mode == IW_MODE_MASTER || priv->rtllib->iw_mode == IW_MODE_APSTA)
			ap_ps_return_allqueuedpackets(priv->rtllib, false);
		rtl8192_irq_disable(dev);
		rtl8192_cancel_deferred_work(priv);
#ifndef RTL8190P
		cancel_delayed_work(&priv->rtllib->hw_wakeup_wq);
#endif
		deinit_hal_dm(dev);
		del_timer_sync(&priv->watch_dog_timer);	
		rtllib_softmac_stop_protocol(ieee, 0, 1, true);

		SPIN_LOCK_PRIV_RFPS(&priv->rf_ps_lock);
		while(priv->RFChangeInProgress)
		{
			SPIN_UNLOCK_PRIV_RFPS(&priv->rf_ps_lock);
			if(RFInProgressTimeOut > 100)
			{
				SPIN_LOCK_PRIV_RFPS(&priv->rf_ps_lock);
				break;
			}
			printk("===>%s():RF is in progress, need to wait until rf chang is done.\n",__FUNCTION__);
			mdelay(1);
			RFInProgressTimeOut ++;
			SPIN_LOCK_PRIV_RFPS(&priv->rf_ps_lock);
		}
		priv->RFChangeInProgress = true;
		SPIN_UNLOCK_PRIV_RFPS(&priv->rf_ps_lock);
		priv->ops->stop_adapter(dev, false);
		SPIN_LOCK_PRIV_RFPS(&priv->rf_ps_lock);
		priv->RFChangeInProgress = false;
		SPIN_UNLOCK_PRIV_RFPS(&priv->rf_ps_lock);
		udelay(100);
		memset(&priv->rtllib->current_network, 0 , offsetof(struct rtllib_network, list));
		priv->rtllib->current_network.channel = INIT_DEFAULT_CHAN;
		memset(&priv->rtllib->current_ap_network, 0 , offsetof(struct rtllib_network, list));
		priv->rtllib->current_ap_network.channel = INIT_DEFAULT_CHAN;
#ifdef CONFIG_ASPM_OR_D3
		RT_ENABLE_ASPM(dev);
#endif
	} else {
		printk("===>%s():priv->up is 1\n",__FUNCTION__);
		rtllib_softmac_stop_protocol(ieee, 0, 1, true);
		memset(&ieee->current_ap_network, 0 , offsetof(struct rtllib_network, list));
		if((ieee->current_network.channel > 0) && (ieee->current_network.channel < 15))
			priv->rtllib->current_ap_network.channel = ieee->current_network.channel;
		else
			priv->rtllib->current_ap_network.channel = INIT_DEFAULT_CHAN;
		if(priv->rtllib->iw_mode == IW_MODE_MASTER || priv->rtllib->iw_mode == IW_MODE_APSTA)
			ap_ps_return_allqueuedpackets(priv->rtllib, false);
		CamResetAllEntry(dev);
		CamRestoreEachIFEntry(dev,0, 0);	
		memset(priv->rtllib->swapcamtable,0,sizeof(SW_CAM_TABLE)*32);
		ieee->iw_mode = IW_MODE_INFRA;
	}
	ieee->ap_state = RTLLIB_NOLINK;
	priv->apdev_up = 0;  
	RT_TRACE(COMP_DOWN, "<==========%s()\n", __FUNCTION__);

	return 0;
}

static int apdev_close(struct net_device *apdev)
{
	struct apdev_priv * appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct r8192_priv *priv = (void *)ieee->priv;
	int ret;
	
	SEM_DOWN_PRIV_WX(&priv->wx_sem);	
	ret = apdev_down(apdev);
	SEM_UP_PRIV_WX(&priv->wx_sem);	
	
	return ret;
}
void apdev_txrate_selectmode(struct rtllib_device* ieee, cb_desc* tcb_desc,struct sta_info *psta)
{
        u8 max_aid;
        max_aid = 0;
	if(ieee->bTxDisableRateFallBack)
		tcb_desc->bTxDisableRateFallBack = true;

	if(ieee->bTxUseDriverAssingedRate)
		tcb_desc->bTxUseDriverAssingedRate = true;

	if(!tcb_desc->bTxDisableRateFallBack || !tcb_desc->bTxUseDriverAssingedRate)
	{
		if(psta!=NULL)
			tcb_desc->RATRIndex = psta->ratr_index;
		else
			tcb_desc->RATRIndex = 7;
	}
	if(ieee->bUseRAMask) {
		if (NULL != psta) {
			short peer_AID = psta->aid;
		       
			tcb_desc->macId =0;
                    max_aid = APDEV_MAX_ASSOC;
			if((peer_AID > 0) && (peer_AID < max_aid))
			{
				tcb_desc->macId = peer_AID + 1;
			}else{
				tcb_desc->macId = 1;
			}
		} else {
			if((ieee->apmode & WIRELESS_MODE_N_24G) || (ieee->apmode & WIRELESS_MODE_N_5G))
				tcb_desc->RATRIndex = RATR_INX_WIRELESS_NGB;
			else if(ieee->apmode & WIRELESS_MODE_G)
				tcb_desc->RATRIndex = RATR_INX_WIRELESS_GB;
			else if(ieee->apmode & WIRELESS_MODE_B)
				tcb_desc->RATRIndex = RATR_INX_WIRELESS_B;
				
			tcb_desc->macId = 0;
		}
	}
}
void apdev_qurey_ShortPreambleMode(struct rtllib_device* ieee, cb_desc* tcb_desc)
{
	tcb_desc->bUseShortPreamble = false;
	if (tcb_desc->data_rate == 2)
	{
		return;
	}
	else if (ieee->current_ap_network.capability & WLAN_CAPABILITY_SHORT_PREAMBLE)
	{
		tcb_desc->bUseShortPreamble = true;
	}
	return;	
}
void apdev_tx_query_agg_cap(struct rtllib_device* ieee, struct sk_buff* skb, cb_desc* tcb_desc)
{
	PRT_HIGH_THROUGHPUT	pHTInfo = ieee->pAPHTInfo;
	PTX_TS_RECORD			pTxTs = NULL;
	struct rtllib_hdr_1addr* hdr = (struct rtllib_hdr_1addr*)skb->data;

	if(rtllib_act_scanning(ieee,false))
		return;

	if (!pHTInfo->bCurrentHTSupport||!pHTInfo->bEnableHT){
		return;
	}
	if (!IsQoSDataFrame(skb->data)){
		return;
	}
	if (is_multicast_ether_addr(hdr->addr1) || is_broadcast_ether_addr(hdr->addr1)){
		return;
	}
#ifdef TO_DO_LIST
	if(pTcb->PacketLength >= 4096)
		return;
	if(!Adapter->HalFunc.GetNmodeSupportBySecCfgHandler(Adapter))
		return; 
#endif

	if(tcb_desc->bdhcp || ieee->CntAfterLink<2){
		return;
	}

	if(!ieee->GetNmodeSupportBySecCfg(ieee->dev)){
		return; 
	}
	if(pHTInfo->bCurrentAMPDUEnable){
		if (!GetTs(ieee, (PTS_COMMON_INFO*)(&pTxTs), hdr->addr1, skb->priority, TX_DIR, true)){
			printk("%s: can't get TS\n", __func__);
			return;
		}
		if (pTxTs->TxAdmittedBARecord.bValid == false){
			if (ieee->wpa_ie_len && (ieee->pairwise_key_type == KEY_TYPE_NA)) {
				;
			} else if (tcb_desc->bdhcp == 1){
				;
			} else if (!pTxTs->bDisable_AddBa){
				TsStartAddBaProcess(ieee, pTxTs);
			}
			goto FORCED_AGG_SETTING;
		}
		else if (pTxTs->bUsingBa == false)
		{
			if (SN_LESS(pTxTs->TxAdmittedBARecord.BaStartSeqCtrl.field.SeqNum, (pTxTs->TxCurSeq+1)%4096))
				pTxTs->bUsingBa = true;
			else
				goto FORCED_AGG_SETTING;
		}
		tcb_desc->bAMPDUEnable = true;
		tcb_desc->ampdu_factor = pHTInfo->CurrentAMPDUFactor;
		tcb_desc->ampdu_density = pHTInfo->CurrentMPDUDensity;
	}
FORCED_AGG_SETTING:
	switch(pHTInfo->ForcedAMPDUMode )
	{
		case HT_AGG_AUTO:
			break;

		case HT_AGG_FORCE_ENABLE:
			tcb_desc->bAMPDUEnable = true;
			tcb_desc->ampdu_density = pHTInfo->ForcedMPDUDensity;
			tcb_desc->ampdu_factor = pHTInfo->ForcedAMPDUFactor;
			break;

		case HT_AGG_FORCE_DISABLE:
			tcb_desc->bAMPDUEnable = false;
			tcb_desc->ampdu_density = 0;
			tcb_desc->ampdu_factor = 0;
			break;

	}	
		return;
}
#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE) || defined(RTL8192CE)
extern void apdev_query_HTCapShortGI(struct rtllib_device *ieee, cb_desc *tcb_desc,u8 is_peer_shortGI_40M,u8 is_peer_shortGI_20M)
{
	PRT_HIGH_THROUGHPUT		pHTInfo = ieee->pAPHTInfo;

	tcb_desc->bUseShortGI 		= false;

	if(!pHTInfo->bCurrentHTSupport||!pHTInfo->bEnableHT || ((ieee->iw_mode != IW_MODE_APSTA) && (ieee->iw_mode != IW_MODE_MASTER)))
	{
		return;
	}

	if(pHTInfo->bForcedShortGI)
	{
		tcb_desc->bUseShortGI = true;
		return;
	}
	if((pHTInfo->bCurBW40MHz==true) && is_peer_shortGI_40M)
		tcb_desc->bUseShortGI = true;
	else if((pHTInfo->bCurBW40MHz==false) && is_peer_shortGI_20M)
		tcb_desc->bUseShortGI = true;
}
void apdev_query_BandwidthMode(struct rtllib_device* ieee, cb_desc *tcb_desc, u8 is_peer_40M)
{
	PRT_HIGH_THROUGHPUT	pHTInfo = ieee->pAPHTInfo;

	tcb_desc->bPacketBW = false;

	if(!pHTInfo->bCurrentHTSupport||!pHTInfo->bEnableHT || ((ieee->iw_mode != IW_MODE_APSTA) && (ieee->iw_mode != IW_MODE_MASTER)))
	{
		return;
	}

	if(tcb_desc->bMulticast || tcb_desc->bBroadcast)
	{
		return;
	}

	if((tcb_desc->data_rate & 0x80)==0) 
	{
		return;
	}
	if(pHTInfo->bCurBW40MHz && is_peer_40M && !ieee->bandwidth_auto_switch.bforced_tx20Mhz)
		tcb_desc->bPacketBW = true;
	return;
}
#endif
void apdev_query_protectionmode(struct rtllib_device* ieee, cb_desc* tcb_desc, struct sk_buff* skb)
{
	tcb_desc->bRTSSTBC			= false;
	tcb_desc->bRTSUseShortGI		= false; 
	tcb_desc->bCTSEnable			= false; 
	tcb_desc->RTSSC				= 0;		
	tcb_desc->bRTSBW			= false; 
	
	if(tcb_desc->bBroadcast || tcb_desc->bMulticast)
		return;
	
	if (is_broadcast_ether_addr(skb->data+16))  
		return;

	if (ieee->apmode < IEEE_N_24G) 
	{
		if (skb->len > ieee->rts)
		{
			tcb_desc->bRTSEnable = true;
			tcb_desc->rts_rate = MGN_24M;
		}
		else if (ieee->current_ap_network.buseprotection)
		{
			tcb_desc->bRTSEnable = true;
			tcb_desc->bCTSEnable = true;
			tcb_desc->rts_rate = MGN_24M;
		}
		return;
	}
	else
	{
		PRT_HIGH_THROUGHPUT pHTInfo = ieee->pAPHTInfo;
		while (true)
		{
			if(pHTInfo->bCurrentHTSupport  && pHTInfo->bEnableHT)
			{
				u8 HTOpMode = pHTInfo->CurrentOpMode;
				if((pHTInfo->bCurBW40MHz && (HTOpMode == 2 || HTOpMode == 3)) ||
							(!pHTInfo->bCurBW40MHz && HTOpMode == 3) )
				{
					tcb_desc->rts_rate = MGN_24M; 
					tcb_desc->bRTSEnable = true;
					break;
				}
			}
			if (skb->len > ieee->rts)
			{
				tcb_desc->rts_rate = MGN_24M; 
				tcb_desc->bRTSEnable = true;
				break;
			}
			if(tcb_desc->bAMPDUEnable)
			{
				tcb_desc->rts_rate = MGN_24M; 
				tcb_desc->bRTSEnable = false;
				break;
			}
			goto NO_PROTECTION;
		}
		}
	if( 0 )
	{
		tcb_desc->bCTSEnable	= true;
		tcb_desc->rts_rate = MGN_24M;
		tcb_desc->bRTSEnable 	= true;
	}
	if (ieee->current_ap_network.capability & WLAN_CAPABILITY_SHORT_PREAMBLE)
		tcb_desc->bUseShortPreamble = true;
	goto NO_PROTECTION;
	return;
NO_PROTECTION:
	tcb_desc->bRTSEnable	= false;
	tcb_desc->bCTSEnable	= false;
	tcb_desc->rts_rate		= 0;
	tcb_desc->RTSSC		= 0;
	tcb_desc->bRTSBW		= false;
}
int apdev_xmit_inter(struct sk_buff *skb, struct net_device *dev)
{
	struct rtllib_device *ieee = (struct rtllib_device *)netdev_priv_rsl(dev);
	struct rtllib_txb *txb = NULL;
	struct rtllib_hdr_3addrqos *frag_hdr;
	int i, bytes_per_frag, nr_frags, bytes_last_frag, frag_size;
	unsigned long flags;
	struct net_device_stats *stats = &ieee->stats;
	int ether_type = 0, encrypt;
	int bytes, fc, qos_ctl = 0, hdr_len;
	struct sk_buff *skb_frag;
	struct rtllib_hdr_3addrqos header = { /* Ensure zero initialized */
		.duration_id = 0,
		.seq_ctl = 0,
		.qos_ctl = 0
	};
	u8 dest[ETH_ALEN], src[ETH_ALEN];
	int qos_actived = ieee->current_ap_network.qos_data.active;
	struct rtllib_crypt_data* crypt = NULL;
	cb_desc *tcb_desc;
	u8 bIsMulticast = false;
#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE) || defined RTL8192CE
	struct sta_info *p_sta = NULL;
#endif	
	u8 IsAmsdu = false;
	cb_desc *tcb_desc_skb;
#ifdef ENABLE_AMSDU	
	u8 queue_index = WME_AC_BE;
	u8 bIsSptAmsdu = false;
	int sta_qos_actived = false;
#endif	

	bool	bdhcp =false;
#ifndef _RTL8192_EXT_PATCH_
#endif
	u8 bEosp = false;
	u8 bMoreData = false;
	int idx_sta = -1;
	unsigned long flags1,flags2;

	spin_lock_irqsave(&ieee->lock, flags);

	/* If there is no driver handler to take the TXB, dont' bother
	 * creating it... */
	if ((!ieee->hard_start_xmit && !(ieee->softmac_features & IEEE_SOFTMAC_TX_QUEUE))||
	   ((!ieee->softmac_data_hard_start_xmit && (ieee->softmac_features & IEEE_SOFTMAC_TX_QUEUE)))) {
		printk(KERN_WARNING "%s: No xmit handler.\n",
		       ieee->dev->name);
		goto success;
	}
	

	if(likely(ieee->raw_tx == 0)){
		if (unlikely(skb->len < SNAP_SIZE + sizeof(u16))) {
			printk(KERN_WARNING "%s: skb too small (%d).\n",
			ieee->dev->name, skb->len);
			goto success;
		}
		/* Save source and destination addresses */
		memcpy(dest, skb->data, ETH_ALEN);
		memcpy(src, skb->data+ETH_ALEN, ETH_ALEN);
		tcb_desc_skb = (pcb_desc)(skb->cb + MAX_DEV_ADDR_SIZE);  
#ifdef ENABLE_AMSDU	
			bIsMulticast = is_broadcast_ether_addr(dest) ||is_multicast_ether_addr(dest);
			if (!bIsMulticast) { 
				p_sta = ap_get_stainfo(ieee, dest);
				if(p_sta)	{
					if(p_sta->htinfo.bEnableHT)
						bIsSptAmsdu = true;
					sta_qos_actived = p_sta->wme_enable;
					qos_actived = (sta_qos_actived && qos_actived);
				}
			} else {
				if (tcb_desc_skb->ps_flag & TCB_PS_FLAG_MCAST_DTIM) { 
					tcb_desc_skb->ps_flag &= (u8)(~(TCB_PS_FLAG_MCAST_DTIM));
				}
				else if(ieee->PowerSaveStationNum > 0)
				{
					skb_queue_tail(&ieee->GroupPsQueue, skb);
					spin_unlock_irqrestore(&ieee->lock, flags);
					return 0;
				}
			}
			if(tcb_desc_skb->bMoreData)
				bMoreData = true;

		bIsSptAmsdu = (bIsSptAmsdu && ieee->pAPHTInfo->bCurrent_AMSDU_Support && qos_actived);
		if(bIsSptAmsdu) {
			if(!tcb_desc_skb->bFromAggrQ) {  
				if (qos_actived) {
					queue_index = UP2AC(skb->priority);
				} else {
					queue_index = WME_AC_BE;
				}

				if ((skb_queue_len(&ieee->skb_apaggQ[queue_index]) != 0)||
#if defined RTL8192SE || defined RTL8192CE
				   (ieee->get_nic_desc_num(ieee->dev,queue_index)) > 1||
#else
				   (!ieee->check_nic_enough_desc(ieee->dev,queue_index))||
#endif
				   (ieee->queue_stop) ||
				   (ieee->ap_amsdu_in_process)) 
				{
					/* insert the skb packet to the Aggregation queue */
					skb_queue_tail(&ieee->skb_apaggQ[queue_index], skb);
					spin_unlock_irqrestore(&ieee->lock, flags);
					return 0;
				}
			} else {  
				if(tcb_desc_skb->bAMSDU)
					IsAmsdu = true;
				
				ieee->ap_amsdu_in_process = false;
			}
		}
#else
		if(tcb_desc_skb->bMoreData)
			bMoreData = true;
#endif	
		ether_type = ntohs(((struct ethhdr *)skb->data)->h_proto);

		if (skb->len > 282){
			if (ETH_P_IP == ether_type) {
				const struct iphdr *ip = (struct iphdr *)((u8 *)skb->data+14);
				if (IPPROTO_UDP == ip->protocol) {
					struct udphdr *udp = (struct udphdr *)((u8 *)ip + (ip->ihl << 2));
					if(((((u8 *)udp)[1] == 68) && (((u8 *)udp)[3] == 67)) ||
					    ((((u8 *)udp)[1] == 67) && (((u8 *)udp)[3] == 68))) {
						printk("DHCP pkt src port:%d, dest port:%d!!\n", ((u8 *)udp)[1],((u8 *)udp)[3]);

						bdhcp = true;
#ifdef _RTL8192_EXT_PATCH_
						ieee->LPSDelayCnt = 200;
#else
						ieee->LPSDelayCnt = 200;
#endif	
					}
				}
			}else if(ETH_P_ARP == ether_type){
				printk("=================>DHCP Protocol start tx ARP pkt!!\n");
				bdhcp = true;
				ieee->LPSDelayCnt = ieee->current_ap_network.tim.tim_count;


			}
		}
		
		skb->priority = rtllib_classify(skb, IsAmsdu);
		if (qos_actived) {
			 /* in case we are a client verify acm is not set for this ac */
                    while (unlikely(ieee->wmm_acm & (0x01 << skb->priority))) {
                        printk("skb->priority = %x\n", skb->priority);
                        if (wme_downgrade_ac(skb)) {
                            break;
                        }
                        printk("converted skb->priority = %x\n", skb->priority);
                    }
		}
		if (!bIsMulticast) { 
			if(p_sta)	{
				if (p_sta->bPowerSave) {
					bool		buapsd = false;
					if (qos_actived) {
						switch(qos_ctl & RTLLIB_QOS_TID) {
							case 0:
							case 3:
								buapsd = GET_BE_UAPSD(p_sta->wmm_sta_qosinfo);
							break;
							case 1:
							case 2:
								buapsd = GET_BK_UAPSD(p_sta->wmm_sta_qosinfo);
							break;
							case 4:
							case 5:
								buapsd = GET_VI_UAPSD(p_sta->wmm_sta_qosinfo);
							break;
							case 6:
							case 7:
								buapsd = GET_VO_UAPSD(p_sta->wmm_sta_qosinfo);
							break;
						}
					}

					if (buapsd) {
					    	int len = 0;
						if (p_sta->bwmm_eosp) 	{ 
							p_sta->FillTIMCount = 2;
							p_sta->FillTIM = true;
							spin_lock_irqsave(&(ieee->wmm_psQ_lock), flags2);
							skb_queue_tail(&(p_sta->WmmPsQueue), skb);
							spin_unlock_irqrestore(&(ieee->wmm_psQ_lock), flags2);
							spin_unlock_irqrestore(&ieee->lock, flags);
							return 0;

						} else { 
							spin_lock_irqsave(&(ieee->wmm_psQ_lock), flags2);
							len = skb_queue_len(&(p_sta->WmmPsQueue));
							spin_unlock_irqrestore(&(ieee->wmm_psQ_lock), flags2);
							if (len > 0) {
								tcb_desc_skb->bMoreData = 1;
								bMoreData = true;
								bMoreData = true;
							} else { 
								bEosp = true;
								p_sta->bwmm_eosp = true; 
							}
						}
					} else {
						if (tcb_desc_skb->ps_flag & TCB_PS_FALG_REPLY_PS_POLL) {
							tcb_desc_skb->ps_flag &= (u8)(~(TCB_PS_FALG_REPLY_PS_POLL));
							if(tcb_desc_skb->bMoreData)
								bMoreData = true;
						} else {
							spin_lock_irqsave(&(ieee->psQ_lock), flags1);
							skb_queue_tail(&(p_sta->PsQueue), skb);
							spin_unlock_irqrestore(&(ieee->psQ_lock), flags1);
							spin_unlock_irqrestore(&ieee->lock, flags);
							return 0;
						}
					}
					
				}
				
			}
		}
		if(!ieee->iswep){
			if(bIsMulticast){
				ieee->ap_tx_keyidx = 1;
				idx_sta = 0;
			}else{
				ieee->ap_tx_keyidx = 0;
				idx_sta = ap_cryptlist_find_station(ieee,dest,1);/*the sender.Just find*/
			}
		}else {
			idx_sta = 0;
		}

		memset(skb->cb, 0, sizeof(skb->cb));
		if (idx_sta < 0){
			crypt = NULL;
		}
		else{
			crypt = ieee->stations_crypt[idx_sta]->crypt[ieee->ap_tx_keyidx];
		}
	#ifdef ASL_DEBUG_2
		printk("====>%s(): crypt1 is %p, crypt2 is %p\n",__FUNCTION__,&(ieee->stations_crypt[0]->crypt[0]),&(ieee->stations_crypt[idx_sta]->crypt[ieee->tx_keyidx]));
		printk("\nDEST: %x:%x:%x:%x:%x:%x",dest[0],dest[1],dest[2],dest[3],dest[4],dest[5]);
		printk(" idx_sta = %d  crypt_idx = %d", idx_sta, ieee->tx_keyidx);
		printk("\nether_type == ETH_P_PAE = %d", ether_type == ETH_P_PAE);
		printk("\nieee->ieee802_1x = %d", ieee->ieee802_1x);
		printk("\nieee->host_encrypt = %d", ieee->host_encrypt);
		printk("\nieee->encrypt_activated = %d", ieee->encrypt_activated);
	#endif
		encrypt = (ieee->host_encrypt && crypt && crypt->ops &&  (strcmp(crypt->ops->name, "WEP") == 0)) 
			 	  || (ieee->encrypt_activated && ieee->host_encrypt && crypt && crypt->ops);
		if (!encrypt && ieee->ieee802_1x &&
		ieee->drop_unencrypted && ether_type != ETH_P_PAE) {
			stats->tx_dropped++;
			goto success;
		}
	#ifdef CONFIG_RTLLIB_DEBUG
		if (crypt && !encrypt && ether_type == ETH_P_PAE) {
			struct eapol *eap = (struct eapol *)(skb->data +
				sizeof(struct ethhdr) - SNAP_SIZE - sizeof(u16));
			RTLLIB_DEBUG_EAP("TX: IEEE 802.11 EAPOL frame: %s\n",
				eap_get_type(eap->type));
		}
	#endif
	
		/* Advance the SKB to the start of the payload */
		skb_pull(skb, sizeof(struct ethhdr));

                /* Determine total amount of storage required for TXB packets */
#ifdef ENABLE_AMSDU	
		if(!IsAmsdu)
			bytes = skb->len + SNAP_SIZE + sizeof(u16);
		else
			bytes = skb->len;
#else
		bytes = skb->len + SNAP_SIZE + sizeof(u16);
#endif	

		if (encrypt)
			fc = RTLLIB_FTYPE_DATA | RTLLIB_FCTL_WEP;
		else 
			fc = RTLLIB_FTYPE_DATA; 
		
		if(qos_actived)
			fc |= RTLLIB_STYPE_QOS_DATA; 
		else
			fc |= RTLLIB_STYPE_DATA;
		if(bMoreData)
			fc |= RTLLIB_FCTL_MOREDATA;

		fc |= RTLLIB_FCTL_FROMDS;
		/* From DS: Addr1 = DA, Addr2 = BSSID,
		Addr3 = SA */
		memcpy(&header.addr1, dest, ETH_ALEN);
		memcpy(&header.addr2, ieee->current_ap_network.bssid, ETH_ALEN);
		memcpy(&header.addr3, src, ETH_ALEN);
#ifdef ASL
#ifdef ASL_DEBUG
		printk("\nfc=%x",fc);
		printk("\nIn ap,xmit src = %x:%x:%x:%x:%x:%x", src[0], src[1], src[2], src[3], src[4], src[5]);
		printk("\nIn ap,xmit dest = %x:%x:%x:%x:%x:%x", dest[0], dest[1], dest[2], dest[3], dest[4], dest[5]);
#endif
#endif
	       header.frame_ctl = cpu_to_le16(fc);

		/* Determine fragmentation size based on destination (multicast
		* and broadcast are not fragmented) */
		if (bIsMulticast) {
			frag_size = MAX_FRAG_THRESHOLD;
			qos_ctl |= QOS_CTL_NOTCONTAIN_ACK;
		}  else {
#ifdef ENABLE_AMSDU	
			if(bIsSptAmsdu) {
				frag_size = ieee->pAPHTInfo->nAMSDU_MaxSize;
				qos_ctl = 0;
			} else
#endif	
			{
				frag_size = ieee->fts;
				qos_ctl = 0;
			}
		}
	
		if(qos_actived)
		{
			hdr_len = RTLLIB_3ADDR_LEN + 2;
                        qos_ctl |= skb->priority; 
	    		qos_ctl |= (bEosp?1:0)<<4;	    
#ifdef ENABLE_AMSDU	
			if(IsAmsdu)
			{
				qos_ctl |= QOS_CTL_AMSDU_PRESENT;
			}
                        header.qos_ctl = cpu_to_le16(qos_ctl);
#else	
                        header.qos_ctl = cpu_to_le16(qos_ctl & RTLLIB_QOS_TID);
#endif

		} else {
			hdr_len = RTLLIB_3ADDR_LEN;		
		}
		/* Determine amount of payload per fragment.  Regardless of if
		* this stack is providing the full 802.11 header, one will
		* eventually be affixed to this fragment -- so we must account for
		* it when determining the amount of payload space. */
		bytes_per_frag = frag_size - hdr_len;
		if (ieee->config &
		(CFG_RTLLIB_COMPUTE_FCS | CFG_RTLLIB_RESERVE_FCS))
			bytes_per_frag -= RTLLIB_FCS_LEN;
	
		/* Each fragment may need to have room for encryptiong pre/postfix */
		if (encrypt) {
			bytes_per_frag -= crypt->ops->extra_prefix_len +
				crypt->ops->extra_postfix_len;
		}
		/* Number of fragments is the total bytes_per_frag /
		* payload_per_fragment */
		nr_frags = bytes / bytes_per_frag;
		bytes_last_frag = bytes % bytes_per_frag;
		if (bytes_last_frag)
			nr_frags++;
		else
			bytes_last_frag = bytes_per_frag;
	
		/* When we allocate the TXB we allocate enough space for the reserve
		* and full fragment bytes (bytes_per_frag doesn't include prefix,
		* postfix, header, FCS, etc.) */
		txb = rtllib_alloc_txb(nr_frags, (frag_size + ieee->tx_headroom), GFP_ATOMIC);
		if (unlikely(!txb)) {
			printk(KERN_WARNING "%s: Could not allocate TXB\n",
			ieee->dev->name);
			goto failed;
		}
		txb->encrypted = encrypt;
		txb->payload_size = bytes;

		if(qos_actived)
		{
			txb->queue_index = UP2AC(skb->priority);
		} else {
			txb->queue_index = WME_AC_BE;;
		}

		for (i = 0; i < nr_frags; i++) {
			skb_frag = txb->fragments[i];
			tcb_desc = (cb_desc *)(skb_frag->cb + MAX_DEV_ADDR_SIZE);
			if(qos_actived){
				skb_frag->priority = skb->priority;
				tcb_desc->queue_index =  UP2AC(skb->priority);
			} else {
				skb_frag->priority = WME_AC_BE;
				tcb_desc->queue_index = WME_AC_BE;
			}
			skb_reserve(skb_frag, ieee->tx_headroom);

			if (encrypt) {
				if (ieee->hwsec_active)
					tcb_desc->bHwSec = 1;
				else
					tcb_desc->bHwSec = 0;
				skb_reserve(skb_frag, crypt->ops->extra_prefix_len);
			} else {
				tcb_desc->bHwSec = 0;
			}
			frag_hdr = (struct rtllib_hdr_3addrqos *)skb_put(skb_frag, hdr_len);
			memcpy(frag_hdr, &header, hdr_len);
	
			/* If this is not the last fragment, then add the MOREFRAGS
			* bit to the frame control */
			if (i != nr_frags - 1) {
				frag_hdr->frame_ctl = cpu_to_le16(
					fc | RTLLIB_FCTL_MOREFRAGS);
				bytes = bytes_per_frag;
		
			} else {
				/* The last fragment takes the remaining length */
				bytes = bytes_last_frag;
			}
			if ((qos_actived) && (!bIsMulticast)) {	
				frag_hdr->seq_ctl = rtllib_query_seqnum(ieee, skb_frag, header.addr1); 
				frag_hdr->seq_ctl = cpu_to_le16(frag_hdr->seq_ctl<<4 | i);
			} else {
				frag_hdr->seq_ctl = cpu_to_le16(ieee->seq_ctrl[0]<<4 | i);
			}
			/* Put a SNAP header on the first fragment */
#ifdef ENABLE_AMSDU	
			if ((i == 0) && (!IsAmsdu)) 
#else
			if (i == 0) 
#endif	
			{
				rtllib_put_snap(
					skb_put(skb_frag, SNAP_SIZE + sizeof(u16)),
					ether_type);
				bytes -= SNAP_SIZE + sizeof(u16);
			}
	
			memcpy(skb_put(skb_frag, bytes), skb->data, bytes);
	
			/* Advance the SKB... */
			skb_pull(skb, bytes);
	
			/* Encryption routine will move the header forward in order
			* to insert the IV between the header and the payload */
			if (encrypt) {
				rtllib_encrypt_fragment(ieee, skb_frag, hdr_len, 2, idx_sta);
			}
			if (ieee->config &
			(CFG_RTLLIB_COMPUTE_FCS | CFG_RTLLIB_RESERVE_FCS))
				skb_put(skb_frag, 4);
		}

		if ((!qos_actived) || (bIsMulticast)) {
  		  if (ieee->seq_ctrl[0] == 0xFFF)
			ieee->seq_ctrl[0] = 0;
		  else
			ieee->seq_ctrl[0]++;
		}
	}else{
		if (unlikely(skb->len < sizeof(struct rtllib_hdr_3addr))) {
			printk(KERN_WARNING "%s: skb too small (%d).\n",
			ieee->dev->name, skb->len);
			goto success;
		}
	
		txb = rtllib_alloc_txb(1, skb->len, GFP_ATOMIC);
		if(!txb){
			printk(KERN_WARNING "%s: Could not allocate TXB\n",
			ieee->dev->name);
			goto failed;
		}
		
		txb->encrypted = 0;
		txb->payload_size = skb->len;
		memcpy(skb_put(txb->fragments[0],skb->len), skb->data, skb->len);
	}	

 success:
	if (txb)
	{
		cb_desc *tcb_desc = (cb_desc *)(txb->fragments[0]->cb + MAX_DEV_ADDR_SIZE);
		tcb_desc->bTxEnableFwCalcDur = 1;
		tcb_desc->priority = skb->priority;
		tcb_desc->ap_pkt = 1;
		if(ether_type == ETH_P_PAE) {
			tcb_desc->data_rate = ieee->basic_rate;
			tcb_desc->bTxDisableRateFallBack = 1;
			
			
			tcb_desc->RATRIndex = 7;                        
			tcb_desc->bTxUseDriverAssingedRate = 1;
		} else {
			if (is_multicast_ether_addr(header.addr1))
				tcb_desc->bMulticast = 1;
			if (is_broadcast_ether_addr(header.addr1))
				tcb_desc->bBroadcast = 1;
			if ( tcb_desc->bMulticast ||  tcb_desc->bBroadcast){
				apdev_txrate_selectmode(ieee, tcb_desc, p_sta);  
				tcb_desc->data_rate = ieee->basic_rate;
			}
			else
			{
					u8 is_sta_shortGI_40M = 0;
					u8 is_sta_shortGI_20M = 0;
					u8 is_sta_BW_40M = 0;
					struct sta_info *p_sta = ap_get_stainfo(ieee, header.addr1);
					if(NULL == p_sta)
					{
						apdev_txrate_selectmode(ieee, tcb_desc, p_sta);
						tcb_desc->data_rate = ieee->ap_rate;
				        }
					else
					{
						apdev_txrate_selectmode(ieee, tcb_desc, p_sta);
						tcb_desc->data_rate = CURRENT_RATE(p_sta->wireless_mode, p_sta->CurDataRate, p_sta->htinfo.HTHighestOperaRate);
						is_sta_shortGI_40M = p_sta->htinfo.bCurShortGI40MHz;
						is_sta_shortGI_20M = p_sta->htinfo.bCurShortGI20MHz;
						is_sta_BW_40M = p_sta->htinfo.bCurTxBW40MHz;
					}
					apdev_qurey_ShortPreambleMode(ieee, tcb_desc);
					apdev_tx_query_agg_cap(ieee, txb->fragments[0], tcb_desc);
					apdev_query_HTCapShortGI(ieee, tcb_desc,is_sta_shortGI_40M,is_sta_shortGI_20M); 
					apdev_query_BandwidthMode(ieee, tcb_desc,is_sta_BW_40M);
					apdev_query_protectionmode(ieee, tcb_desc, txb->fragments[0]);
			}
		} 		
	}
	spin_unlock_irqrestore(&ieee->lock, flags);
	dev_kfree_skb_any(skb);
	if (txb) {
		if (ieee->softmac_features & IEEE_SOFTMAC_TX_QUEUE){
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,18))
			dev->stats.tx_packets++;
			dev->stats.tx_bytes += txb->payload_size;
#endif	
#ifdef ASL
#ifdef RATE_ADAPTIVE_FOR_AP
			if(ieee->enableRateAdaptive){
				struct sta_info *pEntry = ap_get_stainfo(ieee, &dest);
				if(pEntry )
					ap_set_dataframe_txrate(ieee, pEntry);
			}
#endif
#endif
			rtllib_softmac_xmit(txb, ieee);
		}else{
			if ((*ieee->hard_start_xmit)(txb, dev) == 0) {
				stats->tx_packets++;
				stats->tx_bytes += txb->payload_size;
				return 0;
			}
			rtllib_txb_free(txb);
		}
	}

	return 0;

 failed:
	spin_unlock_irqrestore(&ieee->lock, flags);
	netif_stop_queue(dev);
	stats->tx_errors++;
	return 1;

}
static int apdev_tx(struct sk_buff *skb, struct net_device *apdev)
{
	struct apdev_priv *appriv = netdev_priv_rsl(apdev);
	struct rtllib_device *rtllib = (struct rtllib_device *)appriv->rtllib;
	struct net_device *dev = rtllib->dev;
	struct rtl_packet *rtl;
	struct rtl_packet *ret_rtl;
	struct rtllib_hdr_4addr * hdr = NULL;
	u8 zero_ether_header[14] = {0};
	bool is_hostapd_pkt = false;
	bool is_ioctl = false;
	u16 fc = 0;
	u8 type = 0, stype = 0;
	u8 *seq;
	void *key;
	int idx=0;
#ifdef ASL_WME
	struct ap_wmm_ac_parameter *ac;
	int aci=0;
	int i;
#endif
	rtl = (struct rtl_packet *)skb->data;
	ret_rtl = NULL;
	seq = NULL;
	key = NULL;	
	if (memcmp(rtl->ether_head, zero_ether_header, 14) == 0)
		is_hostapd_pkt = true;
	if (rtl->is_ioctl == 1)
		is_ioctl = true;
	if (is_hostapd_pkt) {
		if (is_ioctl) {
#ifdef ASL_DEBUG_2
		printk("\nGot an RTL packet!");
		printk("\n-------------------------rtl->id = %x----------------------------------",rtl->id);
		printk("\n-------------------------rtl->type = %x----------------------------------",rtl->type);
		printk("\n-------------------------rtl->subtype = %x----------------------------------",rtl->subtype);
#endif
			switch (rtl->type) {
				case RTL_CUSTOM_TYPE_WRITE:
					switch (rtl->subtype) {
						case RTL_CUSTOM_SUBTYPE_SET_RSNIE:
#ifdef ASL_DEBUG
							printk("\nActivating RSN!!!!!!!!!!!!");
#endif
							if (MAX_WPA_IE_LEN >= rtl->length) 	{
								memcpy(rtllib->rsn_ie, rtl->data, rtl->length);
								rtllib->rsn_ie_len = rtl->length;
								rtllib->rsn_activated = 1;
								printk("\nRSN IE Len=%d rsn_activted=1 now..",rtl->length);
							} else {
								printk("\nRSN IE Len=%d MAX WPA IE Len=%d",rtl->length, MAX_WPA_IE_LEN);
							}
							break;
						case RTL_CUSTOM_SUBTYPE_RESET_RSNIE:
#ifdef ASL_DEBUG
							printk("\nDeactivating RSN!!!!!!!!!!!!!!");
#endif
							rtllib->rsn_activated = 0;
							rtllib->rsn_ie_len = 0;
							break;
#ifdef ASL_WME
						case RTL_CUSTOM_SUBTYPE_SET_WME:
							printk("=======>%s(): RTL_CUSTOM_SUBTYPE_SET_WME\n",__FUNCTION__);
							rtllib->wme_enabled = 1;
							rtllib->current_ap_network.qos_data.active = 1;
							ac = (struct ap_wmm_ac_parameter *)(rtl->data);
							for (i=0;i<4;i++) {	
	                           				aci = (ac->aci_aifsn >> 5) & 0x03;
								rtllib->current_ap_network.qos_data.parameters.flag[aci] = (ac->aci_aifsn >> 4) & 0x01;
								rtllib->current_ap_network.qos_data.parameters.aifs[aci] = ac->aci_aifsn & 0x0f;
								rtllib->current_ap_network.qos_data.parameters.cw_min[aci] = ac->cw & 0x0f;
								rtllib->current_ap_network.qos_data.parameters.cw_max[aci] = (ac->cw >> 4) & 0x0f;
								rtllib->current_ap_network.qos_data.parameters.tx_op_limit[aci] = le16_to_cpu(ac->txop_limit);
								ac++;
							}
							queue_work_rsl(appriv->priv->priv_wq, &appriv->priv->qos_activate);
							RT_TRACE (COMP_QOS, "QoS parameters change call "
									"qos_activate\n");
							printk("\nUpdate wme params OK.");
							break;
#endif
						case RTL_CUSTOM_SUBTYPE_SET_ASSOC:
						{
#ifdef ASL_DEBUG
							printk("\nAppending associated client to the list");
#endif
#ifdef ASL_DEBUG_2
							printk("\nCurrent assoc list : \n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p", rtllib->apdev_assoc_list[0], rtllib->apdev_assoc_list[1], rtllib->apdev_assoc_list[2],rtllib->apdev_assoc_list[3],rtllib->apdev_assoc_list[4],rtllib->apdev_assoc_list[5],rtllib->apdev_assoc_list[6],rtllib->apdev_assoc_list[7],rtllib->apdev_assoc_list[8],rtllib->apdev_assoc_list[9]);
#endif
							struct sta_info *new_sta = (struct sta_info *)rtl->data;
							int idx_exist = ap_is_stainfo_exist(rtllib, new_sta->macaddr);

							if (idx_exist >= APDEV_MAX_ASSOC) {
								idx = ap_get_free_stainfo_idx(rtllib, new_sta->macaddr);
							} else {
								printk("there has one entry in associate list, break!!\n");
								break;
							}
							if (idx >= APDEV_MAX_ASSOC - 1) {
								printk("\nBuffer overflow - could not append!!!");
								return -1;
							} else {
#ifdef ASL_DEBUG_2
								printk("\nSizeof struct sta_info = %d",sizeof(struct sta_info));
								printk("\nrtl->length = %d",rtl->length);
								printk("\nBefore alloction, apdev_assoc_list[idx] = %p",rtllib->apdev_assoc_list[idx]);
#endif
								rtllib->apdev_assoc_list[idx] = (struct sta_info *)kzalloc(sizeof(struct sta_info), GFP_ATOMIC);
								
#ifdef ASL_DEBUG_2
								printk("\nAfter allocation, apdev_assoc_list[idx] = %p", rtllib->apdev_assoc_list[idx]);
#endif
								memcpy(rtllib->apdev_assoc_list[idx], rtl->data, rtl->length);
								rtllib->apdev_assoc_list[idx]->bPowerSave = 0;
								skb_queue_head_init(&(rtllib->apdev_assoc_list[idx]->PsQueue));
								skb_queue_head_init(&(rtllib->apdev_assoc_list[idx]->WmmPsQueue));
								rtllib->apdev_assoc_list[idx]->LastActiveTime = jiffies;
#if 1
								printk("\nAppended!!!");
								printk("\nindex is %d\n",idx);
								printk("\nAid = %d",rtllib->apdev_assoc_list[idx]->aid);
								printk("\nAuthentication = %d",rtllib->apdev_assoc_list[idx]->authentication);
								printk("\nEncryption = %d",rtllib->apdev_assoc_list[idx]->encryption);
								printk("\nCapability = %d",rtllib->apdev_assoc_list[idx]->capability);
								printk("\nwme_enable = %d",rtllib->apdev_assoc_list[idx]->wme_enable);
								printk("\nsta->htinfo.AMSDU_MaxSize = %d\n",rtllib->apdev_assoc_list[idx]->htinfo.AMSDU_MaxSize);
#endif
#ifdef RATE_ADAPTIVE_FOR_AP
								ap_init_stainfo(rtllib, idx);
								ap_init_rateadaptivestate(rtllib,rtllib->apdev_assoc_list[idx]);
#endif
								printk("\nCurrent assoc list : \n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p", rtllib->apdev_assoc_list[0], rtllib->apdev_assoc_list[1], rtllib->apdev_assoc_list[2],rtllib->apdev_assoc_list[3],rtllib->apdev_assoc_list[4],rtllib->apdev_assoc_list[5],rtllib->apdev_assoc_list[6],rtllib->apdev_assoc_list[7],rtllib->apdev_assoc_list[8],rtllib->apdev_assoc_list[9]);
								printk("\n idx is %d\n",idx);
								queue_delayed_work_rsl(rtllib->wq, &rtllib->update_assoc_sta_info_wq, 0);

								ap_reset_admit_TRstream_rx(rtllib, rtllib->apdev_assoc_list[idx]->macaddr);
								if ((rtllib->ap_group_key_type == KEY_TYPE_WEP40) || (rtllib->ap_group_key_type == KEY_TYPE_WEP40)) {
									rtllib->ap_assoc_sta_bitmap |= (BIT0 << idx);
									rtllib->apdev_assoc_list[idx]->sec_state = USED;
									queue_delayed_work_rsl(rtllib->wq, &rtllib->apdev_set_wep_crypt_wq, 0);
								}
							}
							break;
						}
						case RTL_CUSTOM_SUBTYPE_CLEAR_ASSOC:
							printk("\nRemoving associated client "MAC_FMT" from the list!!!", MAC_ARG(rtl->data));
#ifdef ASL_DEBUG_2
							printk("\nCurrent assoc list : \n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p", rtllib->apdev_assoc_list[0], rtllib->apdev_assoc_list[1], rtllib->apdev_assoc_list[2],rtllib->apdev_assoc_list[3],rtllib->apdev_assoc_list[4],rtllib->apdev_assoc_list[5],rtllib->apdev_assoc_list[6],rtllib->apdev_assoc_list[7],rtllib->apdev_assoc_list[8],rtllib->apdev_assoc_list[9]);
#endif
							ap_del_stainfo(rtllib, rtl->data);

							ap_set_del_sta_list(rtllib, rtl->data);
							printk("\nAssociating "MAC_FMT"", MAC_ARG(rtl->data));
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)  
							queue_delayed_work_rsl(rtllib->wq, &rtllib->clear_sta_hw_sec_wq, 0);
#else
							schedule_task(&rtllib->clear_sta_hw_sec_wq);
#endif
#ifdef ASL_DEBUG_2
							printk("\nCurrent assoc list : \n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p", rtllib->apdev_assoc_list[0], rtllib->apdev_assoc_list[1], rtllib->apdev_assoc_list[2],rtllib->apdev_assoc_list[3],rtllib->apdev_assoc_list[4],rtllib->apdev_assoc_list[5],rtllib->apdev_assoc_list[6],rtllib->apdev_assoc_list[7],rtllib->apdev_assoc_list[8],rtllib->apdev_assoc_list[9]);
#endif
							break;
						case RTL_CUSTOM_SUBTYPE_SET_WPS_BEACON:
#ifdef CONFIG_WPS
							printk("RTL_CUSTOM_SUBTYPE_SET_WPS_BEACON:%d\n",rtl->length);
							rtllib->beacon_wps_ie = (u8*)kzalloc(rtl->length, GFP_ATOMIC);
							if (rtllib->beacon_wps_ie == NULL) {
								printk("RTL_CUSTOM_SUBTYPE_SET_WPS_BEACON: ERR!! can't malloc buf\n");
								return -ENOMEM;
							}
							rtllib->beacon_wps_ie_len = rtl->length;
							memcpy(rtllib->beacon_wps_ie, rtl->data,rtl->length);
#endif
							break;
						case RTL_CUSTOM_SUBTYPE_SET_WPS_PROBE_RSP:
#ifdef CONFIG_WPS
							printk("RTL_CUSTOM_SUBTYPE_SET_WPS_PROBE_RSP:%d\n",rtl->length);
							rtllib->probe_rsp_wps_ie = (u8*)kzalloc(rtl->length, GFP_ATOMIC);
							if (rtllib->probe_rsp_wps_ie == NULL) {
								printk("RTL_CUSTOM_SUBTYPE_SET_WPS_PROBE_RSP: ERR!! can't malloc buf\n");
								return -ENOMEM;
							}
							rtllib->probe_rsp_wps_ie_len = rtl->length;
							memcpy(rtllib->probe_rsp_wps_ie, rtl->data,rtl->length);
#endif
							break;
						default:
							break;
					}
					break;
				case RTL_CUSTOM_TYPE_READ:
#if 0
					switch (rtl->subtype) {
						case RTL_CUSTOM_SUBTYPE_STASC:
							printk("\nGetting STA SC!!!!!!!!");
							memcpy(&idx, rtl->data, sizeof(int));
							if (rtllib->crypt[idx]) {
								ret_rtl = (struct rtl_packet *)kmalloc(sizeof(struct rtl_packet)+TKIP_KEY_LEN+6, GFP_ATOMIC);
								ret_rtl->id = 0xffff;
								ret_rtl->type = RTL_CUSTOM_TYPE_READ;
#define TKIP_KEY_LEN 32
								ret_rtl->subtype = RTL_CUSTOM_SUBTYPE_STASC;
								ret_rtl->length = TKIP_KEY_LEN + 6;
								key = (void *)kmalloc(TKIP_KEY_LEN, GFP_ATOMIC);
								seq = (u8 *)kmalloc(6, GFP_ATOMIC);
								rtllib->crypt[idx]->ops->get_key(key, TKIP_KEY_LEN, seq, priv);
								printk("\nkey = %s",key);
								printk("\nSeq = %d %d %d %d %d %d", seq[0],seq[1],seq[2],seq[3],seq[4],seq[5]);
							} else {
								printk("\nCrypt not loaded.");
							}
							break;
						default:
							break;
					}
#endif
					break;
				default:
					break;
			}
		} else {
			struct sk_buff* new_skb = NULL;
			u8* tag = NULL;
			hdr = (struct rtllib_hdr_4addr *)skb->data;
			fc = le16_to_cpu(hdr->frame_ctl);
			type = WLAN_FC_GET_TYPE(fc);
			stype = WLAN_FC_GET_STYPE(fc);
			if (RTLLIB_FTYPE_MGMT == type) {
#ifdef ASL_DEBUG_2
				printk("\nGot a packet to transmit.");
				int i=0;
				printk("\n----------------PKG DUMP------------\n");	
				for (i=0; i<skb->len;i++) {
					printk("%x ",*(u8*)((u32)skb->data+i));
				}
				printk("\n---------------------------------------\n");	
#endif
				new_skb = dev_alloc_skb(skb->len-15);
				if (!new_skb) {
					printk("can't malloc new skb\n");
					return -ENOMEM;
				}
				tag = (u8*)skb_put(new_skb, skb->len-15);
				memcpy(tag, skb->data+15, skb->len-15);
				softmac_mgmt_xmit(new_skb,rtllib,true);
			} 
		}
	dev_kfree_skb_any(skb);
	}else {
#ifdef ASL_DEBUG_2
		bool bIsMulticast = false;
		u8 dest[6] = {0};
		memcpy(dest,skb->data,6);
		bIsMulticast = is_broadcast_ether_addr(dest) ||is_multicast_ether_addr(dest);
		if(!bIsMulticast) {
	    		printk("=============>%s(): send normal data pkt\n",__FUNCTION__);
			dump_buf(skb->data,skb->len);
		}
#endif
		apdev_xmit_inter(skb, dev);
	}
			
	return 0;
}
static void apdev_tx_timeout(struct net_device *apdev)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device * ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;

	printk("%s():Timeout transmitting !\n",__func__);
	rtl8192_tx_timeout(dev);	
	return;
}
struct net_device_stats *apdev_stats(struct net_device *apdev)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0) 
        return &((struct apdev_priv *)netdev_priv(apdev))->stats;
#else
	return &((struct apdev_priv *)apdev->priv)->stats;
#endif	
}
int rtl8192_ap_wpa_set_encryption(struct net_device *dev,struct ieee_param *ipw, int param_len)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;
	u32 key[4];
	u8 *addr = ipw->sta_addr;
	u8 entry_idx = 0;
	
	if (param_len != (int) ((char *) ipw->u.crypt.key - (char *) ipw) + ipw->u.crypt.key_len) 
	{
		printk("%s:Len mismatch %d, %d\n", __FUNCTION__, param_len, ipw->u.crypt.key_len);
		return -EINVAL;
	}
	if (addr[0] == 0xff && addr[1] == 0xff &&
			addr[2] == 0xff && addr[3] == 0xff &&
			addr[4] == 0xff && addr[5] == 0xff) {
		if (ipw->u.crypt.idx >= WEP_KEYS)
			return -EINVAL;
		memcpy((u8*)key, ipw->u.crypt.key, 16);
		if (strcmp(ipw->u.crypt.alg, "CCMP") == 0)
			ieee->ap_group_key_type= KEY_TYPE_CCMP;
		else if (strcmp(ipw->u.crypt.alg, "TKIP") == 0)
			ieee->ap_group_key_type = KEY_TYPE_TKIP;
		else if (strcmp(ipw->u.crypt.alg, "WEP") == 0)
		{
			if (ipw->u.crypt.key_len == 13)
				ieee->ap_group_key_type = KEY_TYPE_WEP104;
			else if (ipw->u.crypt.key_len == 5)
				ieee->ap_group_key_type = KEY_TYPE_WEP40;
			ieee->ap_pairwise_key_type = ieee->ap_group_key_type;
		}
		else
			ieee->ap_group_key_type = KEY_TYPE_NA;
		if (ieee->ap_group_key_type)
		{
#if 0
			set_swcam(	dev, 
					ipw->u.crypt.idx,
					ipw->u.crypt.idx, 
					ieee->ap_group_key_type,	
					broadcast_addr,	
					0,		
					key,
					0,		
					1);		
			setKey(	dev, 
					ipw->u.crypt.idx,
					ipw->u.crypt.idx, 
					ieee->ap_group_key_type,	
					broadcast_addr,	
					0,		
					key);		
#endif
		}
	} else {
	if (ipw->u.crypt.set_tx) 
	{
#ifdef ASL_DEBUG_2
	    	printk("=============>%s():set_tx is 1\n",__FUNCTION__);
#endif
			printk("=============>%s():set_tx is 1\n",__FUNCTION__);
		if (strcmp(ipw->u.crypt.alg, "CCMP") == 0)
				ieee->ap_pairwise_key_type = KEY_TYPE_CCMP;
		else if (strcmp(ipw->u.crypt.alg, "TKIP") == 0)
				ieee->ap_pairwise_key_type = KEY_TYPE_TKIP;
		else if (strcmp(ipw->u.crypt.alg, "WEP") == 0)
		{
			if (ipw->u.crypt.key_len == 13)
					ieee->ap_pairwise_key_type = KEY_TYPE_WEP104;
			else if (ipw->u.crypt.key_len == 5)
					ieee->ap_pairwise_key_type = KEY_TYPE_WEP40;
		}
		else 
				ieee->ap_pairwise_key_type = KEY_TYPE_NA;

			if (ieee->ap_pairwise_key_type)
		{
			entry_idx = rtl8192_get_free_hwsec_cam_entry(ieee, addr);
			if(entry_idx >=  TOTAL_CAM_ENTRY)
			{
				printk("%s: Can not find free hw security cam entry\n", __FUNCTION__);
				return -EINVAL;
			}
			memcpy((u8*)key, ipw->u.crypt.key, 16);
			EnableHWSecurityConfig8192(dev);
				set_swcam(dev,entry_idx,ipw->u.crypt.idx,ieee->ap_pairwise_key_type,addr, 0, key, 0, 1);
				set_swcam(dev,ipw->u.crypt.idx,ipw->u.crypt.idx,ieee->ap_pairwise_key_type, addr, 0, key, 0, 1);
				setKey(dev, entry_idx, ipw->u.crypt.idx, ieee->ap_pairwise_key_type, addr, 0, key);
				setKey(dev, ipw->u.crypt.idx, ipw->u.crypt.idx, ieee->ap_pairwise_key_type, addr, 0, key);
			
		}
	}
	}
	return 0;
}
#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,20)
void apdev_set_wep_crypt(struct work_struct * work)
{
	struct delayed_work *dwork = container_of(work,struct delayed_work,work);
	struct rtllib_device *ieee = container_of(dwork, struct rtllib_device, apdev_set_wep_crypt_wq);
        struct net_device *dev = ieee->dev;
#else
void apdev_set_wep_crypt(struct net_device *dev)
	{
        struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;
#endif
	struct rtllib_crypt_data **crypt;
	u8 i = 0;
	u64 bitmap = 0;
	int key_len=0;
	u32 key[32]; 
	struct sta_info * psta = NULL;
	u8 entry_idx = 0;
	crypt = &ieee->stations_crypt[0]->crypt[ieee->ap_tx_keyidx];
	if (*crypt == NULL || (*crypt)->ops == NULL)
		{
		printk("%s():no encrypt now\n",__FUNCTION__);
		return;
		}
	if (!((*crypt)->ops->set_key && (*crypt)->ops->get_key)) 
		return;
	
	key_len = (*crypt)->ops->get_key(key, 32, NULL, (*crypt)->priv);
	printk("======>%s():key_len:%d\n",__FUNCTION__, key_len);
	for (i=0; i<APDEV_MAX_ASSOC; i++) {
		psta = ieee->apdev_assoc_list[i];
		if (NULL != psta) {
			bitmap = (ieee->ap_assoc_sta_bitmap)>>i;
			if (((bitmap & BIT0) == BIT0) && psta->sec_state == USED) {
				entry_idx = rtl8192_get_free_hwsec_cam_entry(ieee, psta->macaddr);
				if(entry_idx >=  TOTAL_CAM_ENTRY)
		{
					printk("%s: Can not find free hw security cam entry\n", __FUNCTION__);
					return;
				}
				printk("===>entry_idx is %d,addr is "MAC_FMT"\n",entry_idx,MAC_ARG(psta->macaddr));	
				EnableHWSecurityConfig8192(dev);
				set_swcam(dev,entry_idx,ieee->ap_tx_keyidx,ieee->ap_pairwise_key_type, psta->macaddr, 0, key, 0, 1);
				setKey(dev, entry_idx, ieee->ap_tx_keyidx, ieee->ap_pairwise_key_type, psta->macaddr, 0, key);
				psta->sec_state = HW_SEC;
			}
		}
	}
	return;
}
int pid_hostapd = 0;
int apdev_dequeue(spinlock_t *plock, APDEV_QUEUE *q, u8 *item, int *itemsize)
{
	unsigned long flags;

	RT_ASSERT_RET_VALUE(plock, (-E_WAPI_QNULL));
	RT_ASSERT_RET_VALUE(q, (-E_WAPI_QNULL));
	RT_ASSERT_RET_VALUE(item, (-E_WAPI_QNULL));
	printk("===========%s()\n",__FUNCTION__);
	if(APDEV_IsEmptyQueue(q))
		return -E_APDEV_QEMPTY;

 	spin_lock_irqsave(plock, flags);
	
	memcpy(item, q->ItemArray[q->Head].Item, q->ItemArray[q->Head].ItemSize);
	*itemsize = q->ItemArray[q->Head].ItemSize;
	q->NumItem--;
	if((q->Head+1) == q->MaxItem)
		q->Head = 0;
	else
		q->Head++;

	spin_unlock_irqrestore(plock, flags);
	printk("itemsize is %d\n",*itemsize);	
	return E_APDEV_OK;
}
void ap_ht_use_default_setting(struct rtllib_device* ieee)
{
	PRT_HIGH_THROUGHPUT pHTInfo = ieee->pAPHTInfo;
		
	if (pHTInfo->bEnableHT) {
		pHTInfo->bCurrentHTSupport = true;
		pHTInfo->bCurSuppCCK = pHTInfo->bRegSuppCCK;

		pHTInfo->bCurBW40MHz = pHTInfo->bRegBW40MHz;
		pHTInfo->bCurShortGI20MHz= pHTInfo->bRegShortGI20MHz;

		pHTInfo->bCurShortGI40MHz= pHTInfo->bRegShortGI40MHz;

		
#ifdef ENABLE_AMSDU
		pHTInfo->bCurrent_AMSDU_Support = pHTInfo->bAMSDU_Support;
#else
		pHTInfo->bCurrent_AMSDU_Support = pHTInfo->bAMSDU_Support;
#endif
		pHTInfo->nCurrent_AMSDU_MaxSize = pHTInfo->nAMSDU_MaxSize;

#ifdef ENABLE_AMSDU
		pHTInfo->bCurrentAMPDUEnable = pHTInfo->bAMPDUEnable;
#else
		pHTInfo->bCurrentAMPDUEnable = pHTInfo->bAMPDUEnable;
#endif
		pHTInfo->CurrentAMPDUFactor = pHTInfo->AMPDU_Factor;

		pHTInfo->CurrentMPDUDensity = pHTInfo->MPDU_Density;

		pHTInfo->bCurRxReorderEnable = pHTInfo->bRegRxReorderEnable;
#if (defined RTL8192SE || defined RTL8192SU || defined RTL8192CE)
        	if((ieee->SetHwRegHandler != NULL) && (!ieee->pHTInfo->bCurrentAMPDUEnable)) {
           	 	ieee->SetHwRegHandler( ieee->dev, HW_VAR_SHORTGI_DENSITY,  (u8*)(&ieee->MaxMssDensity));
            		ieee->SetHwRegHandler(ieee->dev, HW_VAR_AMPDU_FACTOR, &pHTInfo->CurrentAMPDUFactor);
           	 	ieee->SetHwRegHandler(ieee->dev, HW_VAR_AMPDU_MIN_SPACE, &pHTInfo->CurrentMPDUDensity);
        	}
#endif  

	} else {
		pHTInfo->bCurrentHTSupport = false;
	}
	return;
}
static int apdev_ioctl(struct net_device *apdev, struct ifreq *ifr, int cmd)
{
	int ret;
	struct apdev_priv *priv;
	struct iwreq *wrq = (struct iwreq *)ifr;
	struct rtllib_device *rtllib;
	u8 *seq;
	u8* key;
	int idx=0;
	struct rtl_seqnum *seqn;
	struct r8192_priv *priv_8192;
	priv = netdev_priv_rsl(apdev);
	rtllib = (struct rtllib_device *)priv->rtllib;
	priv_8192 = rtllib_priv(rtllib->dev);
	ret = -1;
#define DATAQUEUE_EMPTY	"Queue is empty"
	switch (cmd) {
		case SIOCIWFIRSTPRIV:
		{
			struct rtl_ioctl_param param;
			u32	length = 0;
			memset((u8*)&param, 0, sizeof(param));
			length = wrq->u.data.length;
			if (!wrq->u.data.pointer) {
				printk("\nwrq->u.data.pointer does not exits!!!");
				return -EINVAL;
			}
			if (length < 10) {
				printk("\nwrq->u.data.pointer does not exits!!!");
				return -EINVAL;
			}
			if(copy_from_user(&param, (void *)wrq->u.data.pointer, length)){
				ret = -1;
				break;
			}

			switch (param.cmd) {
		                case RTL_IOCTL_SHOSTAP:
#ifdef ASL_DEBUG_2
			printk("\nSetting hostap to 1");
#endif
			printk("\nStart hostap!!!");
			rtllib->hostap = 1;
					printk("\nCur_chan=%d ssid=%s ",rtllib->current_ap_network.channel,rtllib->current_ap_network.ssid);
					printk("bssid:%x:%x:%x:%x:%x:%x ", rtllib->current_ap_network.bssid[0], rtllib->current_ap_network.bssid[1], rtllib->current_ap_network.bssid[2], rtllib->current_ap_network.bssid[3],rtllib->current_ap_network.bssid[4],rtllib->current_ap_network.bssid[5]);
					printk("rates-len=%d ext_rates_len=%d\n", rtllib->current_ap_network.rates_len, rtllib->current_ap_network.rates_ex_len);
					if (rtllib->iw_mode == IW_MODE_MASTER)
						HTSetConnectBwMode(rtllib, HT_CHANNEL_WIDTH_20, HT_EXTCHNL_OFFSET_NO_EXT, true);  
		/*resume Send Beacon.added by lawrence.*/
			if(needsendBeacon){
				printk("====>%s()\n",__FUNCTION__);
				rtllib_start_send_beacons(rtllib);	
			}
			memset(rtllib->to_be_del_cam_entry, 0, sizeof(struct del_item));
			rtllib->PowerSaveStationNum = 0;
			ap_ps_return_allqueuedpackets(rtllib,false);
		
			ret = 0;
		break;
	   	case RTL_IOCTL_CHOSTAP:
#ifdef ASL_DEBUG_2
			printk("\nClear hostap to 0");
#endif
			printk("\nClear hostap to 0");
		/*Close ap.Remove list.*/
			ap_del_stainfo_list(rtllib);

		/* delete wps ie.*/
#ifdef CONFIG_WPS
			if (rtllib->beacon_wps_ie) {
				kfree(rtllib->beacon_wps_ie);
				rtllib->beacon_wps_ie = NULL;
				rtllib->beacon_wps_ie_len = 0;
			}
			if (rtllib->probe_rsp_wps_ie) {
				kfree(rtllib->probe_rsp_wps_ie);
				rtllib->probe_rsp_wps_ie = NULL;
				rtllib->probe_rsp_wps_ie_len = 0;
			}
#endif
		/*stop send Beacon.added by lawrence.*/
			printk("\nstop send Beacon.");
					rtllib_softmac_stop_protocol(rtllib, 0, 1, 0);
			rtllib_stop_send_beacons(rtllib);
			rtllib->bbeacon_start = false;
			if (priv_8192 ->up) {
				CamResetAllEntry(rtllib->dev);
				CamRestoreEachIFEntry(rtllib->dev, 0, 0);	
			} else {
				CamResetAllEntry(rtllib->dev);
			}
			memset(rtllib->swapcamtable,0,sizeof(SW_CAM_TABLE)*TOTAL_CAM_ENTRY);
			rtllib->hostap = 0;
			needsendBeacon = 1;/*resume send beacon*/
			ret = 0;
		break;
	    	case RTL_IOCTL_GHOSTAP:
		printk("\nGetting hostap");
		param.u.get_hostap = rtllib->hostap;
		ret = copy_to_user(wrq->u.data.pointer, (void* )&param, wrq->u.data.length);
		break;
		case RTL_IOCTL_SAVE_PID:
		{
			u8 strPID[10];
			int i = 0;

			printk("%s(): RTL_IOCTL_SAVE_PID!\n",__FUNCTION__);
			if (rtllib->hostap != 1){
				ret = -1;
				break;    
			}
			memset(strPID, 0, sizeof(strPID));
			memcpy(strPID, param.u.set_pidbuf, 6);
			pid_hostapd = 0;
			printk("============>strlen(strPID) is %d\n",strlen(strPID));
			for(i = 0; i < strlen(strPID); i++) {
				pid_hostapd = pid_hostapd * 10 + (strPID[i] - 48);
			}		
			printk("%s(): strPID=%x %x %x %x %x %x  pid_wapi=%x!\n",__FUNCTION__, strPID[0],strPID[1],strPID[2],strPID[3],strPID[4],strPID[5], pid_hostapd);
			ret = 0;
			break;	
		}

		case RTL_IOCTL_GET_RFTYPE:
		{
			printk("===============>RTL_IOCTL_GET_RFTYPE: priv->rf_type is %d\n",priv_8192->rf_type);
			param.u.get_chip_rf_type = priv_8192->rf_type;
			ret = copy_to_user((void *)(wrq->u.data.pointer), (void* )&param, wrq->u.data.length);
			break;
		}
/*	    case RTL_IOCTL_SRSN:
		printk("\nActivating RSN!!!!!!!!!!!!");
		rtl =(struct rtl_packet *)kmalloc(wrq->u.data.length);
		copy_from_user(rtl, wrq->u.data.pointer, wr->u.data.length);
		rtllib->rsn_ie = (u8 *)kmalloc(rtl->length,GFP_ATOMIC);
		memcpy(rtllib->rsn_ie, rtl->data, rtl->length);
		rtllib->rsn_ie_len = rtl->length;
		rtllib->rsn_activated = 1;
		break;
	    case RTL_IOCTL_CRSN:
		printk("\nDeactivating RSN!!!!!!!!!!!!!!");
		rtllib->rsn_activated = 0;
		rtllib->rsn_ie_len = 0;
		kfree(rtllib->rsn_ie);
		break;*/
	    	case RTL_IOCTL_GSEQNUM:
#ifdef ASL_DEBUG_2
		printk("\nGetting Seq Num");
#endif
		printk("\nGetting Seq Num");
				
		#if 0
		if (wrq->u.data.length > sizeof(struct rtl_seqnum)) {
			printk("wrq->u.data.length > sizeof(struct rtl_seqnum), ERR\n");
			return ENOMEM;
		}
		#endif
		if (length < (sizeof(struct rtl_seqnum) + 10)) {
			printk("wrq->u.data.length < 10+sizeof(struct rtl_seqnum), ERR!\n");
			ret =  -EINVAL;
			break;
		}
		seqn = (struct rtl_seqnum* )kzalloc(sizeof(struct rtl_seqnum), GFP_KERNEL);
		seq = (u8 *)kzalloc(6, GFP_KERNEL);
		key = (u8 *)kzalloc(TKIP_KEY_LEN, GFP_KERNEL);
				memcpy((u8*)seqn, (u8*)&(param.u.get_seqn), sizeof(struct rtl_seqnum));
		idx = seqn->idx;
				printk("============>idx is %d\n",idx);
		/*if (idx)
			idx--;
		else
		idx = rtllib->tx_keyidx;*/
#ifdef ASL_DEBUG_2
		printk("\nIndex : %d",idx);
#endif
		if(rtllib->stations_crypt[0]->crypt[idx]){
			rtllib->stations_crypt[0]->crypt[idx]->ops->get_key(key, TKIP_KEY_LEN, seq, rtllib->stations_crypt[0]->crypt[idx]->priv);
			printk("\nGot seqnum : %x%x%x%x%x%x", seq[0],seq[1],seq[2],seq[3],seq[4],seq[5]);
			printk("\nGot key:");
					{
						int i=0;
			for(i=0;i<32;i++)
			  printk("%x",(u32)key[i]);
					}
			memcpy(seqn->seqnum, seq, 6);
					memcpy((u8*)&(param.u.get_seqn), (u8*)seqn, sizeof(struct rtl_seqnum));
					ret = copy_to_user(wrq->u.data.pointer, (void*)&param, wrq->u.data.length);
			kfree(seq);
			kfree(seqn);
			kfree(key);
#ifdef ASL_DEBUG_2
			printk("\nGet Seq num DONE!!!");
#endif
			return 0;
		} else {
			printk("\nieee->crypt for the index does not exists!!!");
			return -EINVAL;
		}

#if 0

		if (rtllib->crypt[idx]) {
			rtllib->crypt[idx]->ops->get_key(key, TKIP_KEY_LEN, seq, rtllib->crypt[idx]->priv);
#ifdef ASL_DEBUG_2
			printk("\nGot seqnum : %x%x%x%x%x%x", seq[0],seq[1],seq[2],seq[3],seq[4],seq[5]);
			printk("\nGot key:");
			for(i=0;i<32;i++)
			  printk("%x",(u8*)key[i]);
#endif
			memcpy(seqn->seqnum, seq, 6);
			copy_to_user(wrq->u.data.pointer, seqn, wrq->u.data.length);
			kfree(seq);
			kfree(seqn);
			kfree(key);
#ifdef ASL_DEBUG_2
			printk("\nGet Seq num DONE!!!");
#endif
			return 0;
		} else {
			printk("\nieee->crypt for the index does not exists!!!");
			return -EINVAL;
		}
#endif
		
		break;
	    	case RTL_IOCTL_SHIDDENSSID:
					rtllib->bhidden_ssid = param.u.is_hidden_ssid;
					printk("\nSetting hidden SSID %d\n",rtllib->bhidden_ssid);
			ret = 0;
		break;
	    	case RTL_IOCTL_SMODE:
	    	{
					u8 mode;
					mode = param.u.set_hw_mode;
					rtl8192_SetWirelessMode(rtllib->dev, (u8)mode, true);
					printk("\n=====>%s=====set mode %x\n",__FUNCTION__,rtllib->mode);
			
				priv_8192->ShortRetryLimit=7;
				priv_8192->LongRetryLimit=7;
			priv_8192->ops->rtl819x_hwconfig(rtllib->dev);
					rtllib->current_ap_network.mode = rtllib->apmode;

			ret = 0;
		break;
	    	}
				case RTL_IOCTL_GIWMODE:
				{
					param.u.get_iw_mode = rtllib->iw_mode;
					ret = copy_to_user(wrq->u.data.pointer, (void* )&param, wrq->u.data.length);
				break;
				}
				case RTL_IOCTL_GCHANNEL:
				{
					param.u.get_channel = rtllib->current_ap_network.channel;
					ret = copy_to_user(wrq->u.data.pointer, (void* )&param, wrq->u.data.length);
				break;
				}
				case RTL_IOCTL_GEXTOFFSET:
				{
					param.u.get_ext_offset = rtllib->APExtChlOffset;
					ret = copy_to_user(wrq->u.data.pointer, (void* )&param, wrq->u.data.length);
				break;
				}
		case RTL_IOCTL_START_BN:
		{
			int bstart = false;
					bstart = param.u.set_start_beacon;
					printk("\n=======%s======start beacon======>: start_beacon : %d\n",__func__,bstart);
					if (rtllib->apmode == WIRELESS_MODE_B) {
						rtllib->current_ap_network.rates_ex_len = 0;
			}
					if (rtllib->apmode == WIRELESS_MODE_A) {
						rtllib->current_ap_network.rates_len = 0;
			}
			if (bstart == true) {
				rtllib_stop_send_beacons(rtllib);
				rtllib_start_send_beacons(rtllib);
				rtllib->bbeacon_start = true;
			}
					ret = 0;
			break;
		}
		case RTL_IOCTL_SHTCONF:
	    	{
		       rtl_ht_conf_t ht_conf;
					memcpy((u8*)&ht_conf, (u8*)&(param.u.ht_conf), sizeof(rtl_ht_conf_t));
			printk("\n=====>%s=====set HTCONF\n", __FUNCTION__);
					rtllib->pAPHTInfo->bEnableHT = ht_conf.bEnableHT;
					rtllib->pAPHTInfo->bRegBW40MHz = ht_conf.bRegBW40MHz;
					rtllib->pAPHTInfo->bRegShortGI20MHz= ht_conf.bRegShortGI20MHz;
					rtllib->pAPHTInfo->bRegShortGI40MHz = ht_conf.bRegShortGI40MHz;
					rtllib->pAPHTInfo->bRegSuppCCK = ht_conf.bRegSuppCCK; 
			
					rtllib->pAPHTInfo->bAMSDU_Support = ht_conf.bAMSDU_Support; 
					rtllib->pAPHTInfo->nAMSDU_MaxSize = ht_conf.nAMSDU_MaxSize; 
					rtllib->pAPHTInfo->bAMPDUEnable = ht_conf.bAMPDUEnable; 
					rtllib->pAPHTInfo->AMPDU_Factor = ht_conf.AMPDU_Factor; 
					rtllib->pAPHTInfo->MPDU_Density = ht_conf.MPDU_Density; 
					rtllib->pAPHTInfo->ForcedAMSDUMode = ht_conf.ForcedAMSDUMode; 
					rtllib->pAPHTInfo->ForcedAMSDUMaxSize = ht_conf.ForcedAMSDUMaxSize; 
			
					rtllib->pAPHTInfo->SelfMimoPs = ht_conf.SelfMimoPs; 
					rtllib->pAPHTInfo->bRegRxReorderEnable = ht_conf.bRegRxReorderEnable; 
					rtllib->pAPHTInfo->RxReorderWinSize = ht_conf.RxReorderWinSize; 
					rtllib->pAPHTInfo->RxReorderPendingTime = ht_conf.RxReorderPendingTime; 
			
					rtllib->pAPHTInfo->bRegRT2RTAggregation = ht_conf.bRegRT2RTAggregation;

					memcpy(rtllib->ap_Regdot11HTOperationalRateSet, ht_conf.Regdot11HTOperationalRateSet, 16);
					memcpy(rtllib->ap_Regdot11TxHTOperationalRateSet, ht_conf.Regdot11TxHTOperationalRateSet, 16);
#ifdef ASL_DEBUG_2
					{
						int i=0;
						printk("======>ap_Regdot11HTOperationalRateSet\n");
						for(i=0;i<16;i++)
							printk(" %x",rtllib->ap_Regdot11HTOperationalRateSet[i]);
						printk("======>ap_Regdot11TxHTOperationalRateSet\n");
						for(i=0;i<16;i++)
							printk(" %x",rtllib->ap_Regdot11TxHTOperationalRateSet[i]);
					}
#endif
					rtllib->pAPHTInfo->CurrentOpMode = ht_conf.OptMode;
					ap_ht_use_default_setting(rtllib);		
			dm_init_edca_turbo(rtllib->dev);
					printk("rtllib->current_ap_network.channel is %d\n",rtllib->current_ap_network.channel);
					if (rtllib->iw_mode == IW_MODE_MASTER || (rtllib->iw_mode == IW_MODE_APSTA && rtllib->state == RTLLIB_NOLINK)) {
						if(rtllib->pAPHTInfo->bRegBW40MHz)
							HTSetConnectBwMode(rtllib, HT_CHANNEL_WIDTH_20_40, (rtllib->current_ap_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER, true);  
			else
							HTSetConnectBwMode(rtllib, HT_CHANNEL_WIDTH_20, (rtllib->current_ap_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER, true);  
						rtllib->APExtChlOffset = (rtllib->current_ap_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER;
						ret = 0;
					} else {
						if ((rtllib->mode & (WIRELESS_MODE_N_24G |WIRELESS_MODE_N_5G)) == 0) {
							printk("===>sta is not N mode , set BW which is AP supported\n");
							rtllib->APExtChlOffset = (rtllib->current_ap_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER;
							if(rtllib->pAPHTInfo->bRegBW40MHz)
								HTSetConnectBwMode(rtllib, HT_CHANNEL_WIDTH_20_40, (rtllib->current_ap_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER, true);  
							else
								HTSetConnectBwMode(rtllib, HT_CHANNEL_WIDTH_20, (rtllib->current_ap_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER, true);  
						} else {
							printk("===>sta is N mode\n");
							if (rtllib->pHTInfo->bCurBW40MHz == false) {
								printk("==========>sta is 20M\n");
								rtllib->APExtChlOffset = (rtllib->current_ap_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER;
								if(rtllib->pAPHTInfo->bRegBW40MHz)
									HTSetConnectBwMode(rtllib, HT_CHANNEL_WIDTH_20_40, (rtllib->current_ap_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER, true);  
								else
									HTSetConnectBwMode(rtllib, HT_CHANNEL_WIDTH_20, (rtllib->current_ap_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER, true);  
							}
						}
			ret = 0;
					}
				break;
			    	}
			    	default:
					ret = -EOPNOTSUPP;
		break;
	    	}
		break;
		}
		case RTL_IOCTL_DEQUEUE:
		{
			static u8 QueueData[APDEV_MAX_BUFF_LEN];
			int QueueDataLen = 0;
           
			printk("%s(): RTL_IOCTL_DEQUEUE!\n",__FUNCTION__);
			if((ret = apdev_dequeue(&rtllib->apdev_queue_lock, rtllib->apdev_queue, QueueData, &QueueDataLen)) != 0){
			    	printk("======>empty queue\n");
				if(copy_to_user((void *)(wrq->u.data.pointer), DATAQUEUE_EMPTY, sizeof(DATAQUEUE_EMPTY))==0)
					wrq->u.data.length = sizeof(DATAQUEUE_EMPTY);
			}else{
			    	printk("======>have item in queue\n");
				printk("===>len is %d\n",QueueDataLen);
				if(copy_to_user((void *)wrq->u.data.pointer, (void *)QueueData, QueueDataLen)==0)
					wrq->u.data.length = QueueDataLen;
			}
			ret = 0;
		break;
	    	}
	    	case RTL_IOCTL_WPA:
		{
			struct ieee_param *ipw = NULL;
			struct iw_point *p = &wrq->u.data;
			printk("%s(): set inctl WPA\n",__FUNCTION__);
		   	if(rtllib->iw_mode == IW_MODE_MASTER || rtllib->iw_mode == IW_MODE_APSTA) {
				if (p->length < sizeof(struct ieee_param) || !p->pointer){
				    	printk("===========>length is small or point is null\n");
					ret = -EINVAL;
					goto out;
				}

				ipw = (struct ieee_param *)kmalloc(p->length, GFP_KERNEL);
				if (ipw == NULL){
				    	printk("======>ipw malloc failed\n");
					ret = -ENOMEM;
					goto out;
				}
				if (copy_from_user(ipw, p->pointer, p->length)) {
				    	printk("===========>copy from user failed\n");
					kfree(ipw);
					ipw = NULL;
					return  -EFAULT;
			    	}		
#ifdef ASL_DEBUG_2
				printk("\nGoing to call ieee80211_wpa_suplicant_ioctl!");
#endif
				if(ipw->cmd == IEEE_CMD_SET_ENCRYPTION)
					ret = rtl8192_ap_wpa_set_encryption(rtllib->dev, ipw, p->length);

#ifdef ASL_DEBUG_2
				int i;
				printk("@@ wrq->u pointer = ");
				for(i=0;i<wrq->u.data.length;i++){
					if(i%10==0) printk("\n");
						printk( "%2x-", *(u8*)(wrq->u.data.pointer+i));
				}
				printk("\n");
#endif

				ret = rtllib_wpa_supplicant_ioctl(rtllib, &wrq->u.data, 0, 1);
			}
			kfree(ipw);
        		ipw = NULL;	
		}
		break;

	}
out:
	return ret;
}
void ap_ps_on_pspoll(struct rtllib_device *ieee, u16 fc, u8 *pSaddr, struct sta_info *pEntry)
{
	unsigned long flags;
	struct sk_buff * skb;
	u16 len = 0;		
	if(pEntry == NULL)
		return;
	
	printk("In %s(): fc=0x%x addr="MAC_FMT"\n", __FUNCTION__, fc, MAC_ARG(pSaddr));
	if(WLAN_FC_GET_PWRMGT(fc) == false)
		return;

	spin_lock_irqsave(&(ieee->psQ_lock), flags);
	len = skb_queue_len(&(pEntry->PsQueue));
	spin_unlock_irqrestore(&(ieee->psQ_lock), flags);
	if (len > 0) {
		pcb_desc tcb_desc = NULL;
		spin_lock_irqsave(&(ieee->psQ_lock), flags);
		skb = skb_dequeue(&(pEntry->PsQueue));
		spin_unlock_irqrestore(&(ieee->psQ_lock), flags);
		if (skb) {
			tcb_desc = (pcb_desc)(skb->cb + MAX_DEV_ADDR_SIZE);
			tcb_desc->ps_flag |= TCB_PS_FALG_REPLY_PS_POLL;
			pEntry->FillTIMCount =2;
			pEntry->FillTIM = true;
			if((len-1) > 0) {
				tcb_desc->bMoreData = true;
			}
			apdev_xmit_inter(skb, ieee->dev);
		}
	}
}
void ap_ps_send_null_frame(struct rtllib_device *ieee, u8* daddr, bool bqos, u8 TID, bool beosp)
{
	
	struct sk_buff *buf = rtllib_null_func(ieee, daddr, bqos,TID, beosp,false,true);
	
	if (buf)
		softmac_ps_mgmt_xmit(buf, ieee, VO_QUEUE, true);

}
struct sta_info *ap_ps_update_station_psstate(
	struct rtllib_device *ieee,
	u8 *MacAddr,
	u16 fc,
	u8 TID
	)
{
	struct sta_info *pEntry;
	unsigned long flags,flags1,flags2;
	bool bPowerSave = ((WLAN_FC_GET_PWRMGT(fc)==RTLLIB_FCTL_PM) ? true : false);
	if(ieee->iw_mode != IW_MODE_MASTER && ieee->iw_mode != IW_MODE_APSTA)
		return NULL;
	if (NULL == MacAddr) {
		printk("=============>MacAddr is NULL\n");
		return NULL;
	}
	pEntry = ap_get_stainfo(ieee, MacAddr);
	if(pEntry == NULL)
	{
		return NULL;
	}
	if(bPowerSave)
	{ 
		printk("In %s(): addr="MAC_FMT" bPS=%d,TID is %d\n", __FUNCTION__,MAC_ARG(MacAddr),bPowerSave,TID);
		if (!pEntry->bPowerSave) {
		        pEntry->bPowerSave = true;
		        ieee->PowerSaveStationNum++;
		}
#if 1
		if(IsQoSDataFrame(&fc)) {
			bool wmm_sp = false;
			switch(TID) {
				case 0:
				case 3:
					wmm_sp = GET_BE_UAPSD(pEntry->wmm_sta_qosinfo);
					break;
				case 1:
				case 2:
					wmm_sp = GET_BK_UAPSD(pEntry->wmm_sta_qosinfo);
					break;
				case 4:
				case 5:
					wmm_sp = GET_VI_UAPSD(pEntry->wmm_sta_qosinfo);
					break;
				case 6:
				case 7:
					wmm_sp = GET_VO_UAPSD(pEntry->wmm_sta_qosinfo);
					break;
			}
#if 1
			if (wmm_sp) {
				struct sk_buff *skb = NULL;
				int i=0,len=0;
				pEntry->bwmm_eosp = false;
				spin_lock_irqsave(&(ieee->wmm_psQ_lock), flags1);
				len = skb_queue_len(&pEntry->WmmPsQueue);
				spin_unlock_irqrestore(&(ieee->wmm_psQ_lock), flags1);
				if (len > 0) {
					for (i=0;i<len;i++) {
						spin_lock_irqsave(&(ieee->wmm_psQ_lock), flags1);
						skb = skb_dequeue(&pEntry->WmmPsQueue);
						spin_unlock_irqrestore(&(ieee->wmm_psQ_lock), flags1);
						if(NULL != skb) {		
					    	   	apdev_xmit_inter(skb, ieee->dev);
						}
					}
				} else {
				    	spin_lock_irqsave(&ieee->mgmt_tx_lock, flags2);
					ap_ps_send_null_frame(ieee, MacAddr, true, TID, true);
					spin_unlock_irqrestore(&ieee->mgmt_tx_lock, flags2);
				}
			}
#endif
		} else {
		    	spin_lock_irqsave(&ieee->mgmt_tx_lock, flags2);
			ap_ps_send_null_frame(ieee, MacAddr, true, 0, true);
			spin_unlock_irqrestore(&ieee->mgmt_tx_lock, flags2);
		}
#endif
	}
	else if(!bPowerSave && pEntry->bPowerSave)
	{ 
		struct sk_buff *skb = NULL;
		int i, len;

		printk("In %s(): addr="MAC_FMT" bPS=%d\n", __FUNCTION__,MAC_ARG(MacAddr),bPowerSave);
		pEntry->bPowerSave = false;
		ieee->PowerSaveStationNum--;
#if 1
		spin_lock_irqsave(&(ieee->psQ_lock), flags);
		len = skb_queue_len(&pEntry->PsQueue);
		spin_unlock_irqrestore(&(ieee->psQ_lock), flags);
		if(len > 0)
		{
		    	printk("================>PsQueue has buffer, send it\n");
			pEntry->FillTIMCount = 2;
			pEntry->FillTIM = true;
			for(i=0;i<len;i++)
			{
				spin_lock_irqsave(&(ieee->psQ_lock), flags);
				skb = skb_dequeue(&pEntry->PsQueue);
				spin_unlock_irqrestore(&(ieee->psQ_lock), flags);
				if(NULL != skb)		
					apdev_xmit_inter(skb, ieee->dev);
			}
		}
		spin_lock_irqsave(&(ieee->wmm_psQ_lock), flags1);
		len = skb_queue_len(&pEntry->WmmPsQueue);
		spin_unlock_irqrestore(&(ieee->wmm_psQ_lock), flags1);
		if (len > 0) {
		    	printk("================>WmmPsQueue has buffer, send it\n");
			for (i=0;i<len;i++) {
				spin_lock_irqsave(&(ieee->wmm_psQ_lock), flags1);
				skb = skb_dequeue(&pEntry->WmmPsQueue);
				spin_unlock_irqrestore(&(ieee->wmm_psQ_lock), flags1);
				if(NULL != skb)		
					apdev_xmit_inter(skb, ieee->dev);
				}
	}
#endif	
	}
	return pEntry;
}
/* Return type : 1 - src and dst found
		-1 - src not found, dst found
		-2 - src found, dst not found
		-3 - src and dst both not found */
int ap_is_data_frame_authorized(
	struct rtllib_device *ieee, 
	struct net_device *dev, 
	u8* src, 	u8* dst) 
{
	int i,src_found=0,dst_found=0;
	struct sta_info *sta=NULL;
#ifdef ASL_DEBUG
	printk("%s - Input source address : %02x:%02x:%02x:%02x:%02x:%02x", __FUNCTION__, 
			src[0], src[1], src[2], src[3], src[4], src[5]);
	printk("%s - Input destination address : %02x:%02x:%02x:%02x:%02x:%02x", __FUNCTION__, 
			dst[0], dst[1], dst[2], dst[3], dst[4], dst[5]);
#endif
	for (i = 0; i < APDEV_MAX_ASSOC; i++) {
		sta = (struct sta_info *)ieee->apdev_assoc_list[i];
		if(sta == NULL)
			continue;
#ifdef ASL_DEBUG
		printk("\n%s - Found address : %02x:%02x:%02x:%02x:%02x:%02x", __FUNCTION__, 
				sta->macaddr[0], sta->macaddr[1], sta->macaddr[2], sta->macaddr[3], 
				sta->macaddr[4], sta->macaddr[5]);
#endif
		if(memcmp(src, sta->macaddr, ETH_ALEN) == 0)
			src_found = 1;
		if((memcmp(dst, sta->macaddr, ETH_ALEN) == 0) || (memcmp(dst, dev->dev_addr, ETH_ALEN) == 0))
			dst_found = 1;
		if((dst[0] == 0xff) && (dst[1] == 0xff) && (dst[2] == 0xff) && (dst[3] == 0xff) && \
				(dst[4] == 0xff) && (dst[5] == 0xff)) {
			dst_found = 1;
		}
	
	}
	if(src_found) {
		if(dst_found)
			return 1;
		else
			return -2;
	} else {
		if(dst_found)
			return -1;
		else
			return -3;
	}
}
u16 ap_ps_fill_tim(struct rtllib_device *ieee)
{
	if (ieee->PowerSaveStationNum == 0) {
		ieee->Tim.Octet = (u8 *)ieee->TimBuf;
		ieee->Tim.Octet[0] = (u8)ieee->mDtimCount;
		ieee->Tim.Octet[1] = (u8)ieee->mDtimPeriod;
		ieee->Tim.Octet[2] = 0;
		ieee->Tim.Octet[3] = 0;

		ieee->Tim.Length = 4;
	} else {
		int i;
		int MaxAID=-1,MinAID=-1;
		u8 ByteOffset,BitOffset,BeginByte,EndByte;

		memset(ieee->TimBuf, 0, 254);

		if (skb_queue_len(&(ieee->GroupPsQueue))) {
			ieee->TimBuf[3]=0x01;
			MinAID = 0;
		}
		
		for (i = 0;i < APDEV_MAX_ASSOC; i++) {
			if (NULL != ieee->apdev_assoc_list[i]		&&
				ieee->apdev_assoc_list[i]->bPowerSave	&&
				(skb_queue_len(&(ieee->apdev_assoc_list[i]->PsQueue)) || 
				(skb_queue_len(&(ieee->apdev_assoc_list[i]->WmmPsQueue)) && 
				((ieee->apdev_assoc_list[i]->wmm_sta_qosinfo & 0xF) == 0xF)) ||
				ieee->apdev_assoc_list[i]->FillTIM)) {
				ByteOffset = ieee->apdev_assoc_list[i]->aid >> 3;
				BitOffset = ieee->apdev_assoc_list[i]->aid % 8;

				ieee->TimBuf[3 + ByteOffset] |= (1<<BitOffset);

				if(MaxAID == -1 || ieee->apdev_assoc_list[i]->aid > MaxAID)
					MaxAID = ieee->apdev_assoc_list[i]->aid;

				if(MinAID == -1 || ieee->apdev_assoc_list[i]->aid < MinAID)
					MinAID = ieee->apdev_assoc_list[i]->aid;
				ieee->apdev_assoc_list[i]->FillTIMCount --;
				if (ieee->apdev_assoc_list[i]->FillTIMCount == 0)
					ieee->apdev_assoc_list[i]->FillTIM = false;
			}
		}

		if (MinAID == -1 || MaxAID == -1) {
			BeginByte = 0;
			EndByte = 0;
		} else {
			BeginByte = ((MinAID>>4)<<1);
			EndByte = (MaxAID>>3);
		}
		ieee->Tim.Octet = (u8 *)(&ieee->TimBuf[BeginByte]);

		ieee->Tim.Octet[0] = (u8)ieee->mDtimCount;
		ieee->Tim.Octet[1] = (u8)ieee->mDtimPeriod;
 		ieee->Tim.Octet[2] = BeginByte | (skb_queue_len(&(ieee->GroupPsQueue)) ? 1 : 0);

		ieee->Tim.Length = 4 + (EndByte-BeginByte);
		if(MaxAID>0)
		{
			printk("Beg=%d End=%d MaxAID=%d MinAID=%d\n",BeginByte,EndByte,MaxAID,MinAID);
			printk("********Tim Length %d*************\n", ieee->Tim.Length);
			for(i=0;i<ieee->Tim.Length;i++)
				printk("%x-", ieee->Tim.Octet[i]);
			printk("\n");
		}
	}
	return ieee->Tim.Length;
}
#ifdef ASL_WME

/* Add WME Parameter Element to Beacon and Probe Response frames. */
u8 * ap_eid_wme(struct rtllib_device *ieee,u8 *eid)
{
		
	u8 *pos = eid;
	struct wme_parameter_element *wme =
		(struct wme_parameter_element *) (pos + 2);
	int e;

	eid[0] = MFIE_TYPE_GENERIC;
	wme->oui[0] = 0x00;
	wme->oui[1] = 0x50;
	wme->oui[2] = 0xf2;
	wme->oui_type = WME_OUI_TYPE;
	wme->oui_subtype = WME_OUI_SUBTYPE_PARAMETER_ELEMENT;
	wme->version = WME_VERSION;
	wme->acInfo = ((ieee->bsupport_wmm_uapsd) ? (0x01<<7) : 0) | (ieee->parameter_set_count & 0xf);

	/* fill in a parameter set record for each AC */
	for (e = 0; e < 4; e++) {
		struct wme_ac_parameter *ac = &wme->ac[e];
		struct rtllib_qos_parameters *acp =&ieee->current_ap_network.qos_data.parameters;

		ac->aifsn = acp->aifs[e];
		ac->acm = acp->flag[e];
		ac->aci = e;
		ac->reserved = 0;
		ac->eCWmin = acp->cw_min[e];
		ac->eCWmax = acp->cw_max[e];
		ac->txopLimit = le16_to_cpu(acp->tx_op_limit[e]);
	}

	pos = (u8 *) (wme + 1);
	eid[1] = pos - eid - 2; /* element length */

	return pos;
}

#endif
void ap_start_aprequest(struct net_device* dev)
{
#if 1
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;

	if (IS_NIC_DOWN(priv)) {
		RT_TRACE(COMP_ERR,"%s(): Driver is not up!!!\n",__FUNCTION__);
		return;
	}
	

	RT_TRACE(COMP_RESET, "===> ++++++ AP_StartApRequest() ++++++\n");
	printk("===> ++++++ AP_StartApRequest() ++++++\n");

	
	

	ieee->ap_state = RTLLIB_LINKED;
	ieee->ap_link_change(ieee->dev);
	priv->ops->rtl819x_hwconfig(ieee->dev);
	apdev_notify_wx_assoc_event(ieee);
	
	/*When hostap setup,begin send beacon.By lawrence.*/
	rtllib_start_send_beacons(ieee);
	if (ieee->data_hard_resume)
		ieee->data_hard_resume(ieee->dev);
#ifdef ASL
	netif_carrier_on(ieee->apdev);
#else
	netif_carrier_on(ieee->dev);
#endif
	queue_work_rsl(priv->priv_wq, &priv->qos_activate);

	if (ieee->iw_mode == IW_MODE_MASTER) {
	if(ieee->pHTInfo->bRegBW40MHz)
			HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20_40, (ieee->current_ap_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER, true);  
	else
			HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20, (ieee->current_ap_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER, true);  
	}
		
	RT_TRACE(COMP_RESET,"<=== ++++++ AP_StartApRequest() ++++++\n");
#endif
}
void ap_cam_restore_allentry(struct net_device* dev)
{
	u32 i;
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;
	for( i = 0 ; i<	TOTAL_CAM_ENTRY; i++) {
		if (ieee->swapcamtable[i].bused ) {
			setKey(dev,
					i,
					ieee->swapcamtable[i].key_index,
					ieee->swapcamtable[i].key_type,
					ieee->swapcamtable[i].macaddr,
					ieee->swapcamtable[i].useDK,
					(u32*)(&ieee->swapcamtable[i].key_buf[0]));
		}
	}
	
}
#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,20)
void clear_sta_hw_sec(struct work_struct * work)
{
	struct delayed_work *dwork = container_of(work,struct delayed_work,work);
	struct rtllib_device *ieee = container_of(dwork, struct rtllib_device, clear_sta_hw_sec_wq);
#else
void clear_sta_hw_sec(struct net_device *dev)
{
        struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;
#endif
	int index = 0;
	for (index = 0; index < APDEV_MAX_ASSOC; index++) {
		if (ieee->to_be_del_cam_entry[index].isused) {
			rtl8192_del_hwsec_cam_entry(ieee, ieee->to_be_del_cam_entry[index].addr, false, true);
			memset(ieee->to_be_del_cam_entry[index].addr, 0, ETH_ALEN);
			ieee->to_be_del_cam_entry[index].isused = false;
		}
	}
}
#ifdef HAVE_NET_DEVICE_OPS
static const struct net_device_ops apdev_netdev_ops = {
	.ndo_open = apdev_open,
	.ndo_stop = apdev_close,
	.ndo_get_stats = apdev_stats,
	.ndo_tx_timeout = apdev_tx_timeout,
	.ndo_do_ioctl = apdev_ioctl,
	.ndo_start_xmit = apdev_tx,
};
#endif
void apdev_init_queue(APDEV_QUEUE * q, int szMaxItem, int szMaxData)
{
	RT_ASSERT_RET(q);
	
	q->Head = 0;
	q->Tail = 0;
	q->NumItem = 0;
	q->MaxItem = szMaxItem;
	q->MaxData = szMaxData;
}
int apdev_en_queue(spinlock_t *plock, APDEV_QUEUE *q, u8 *item, int itemsize)
{
	unsigned long flags;

	RT_ASSERT_RET_VALUE(plock, (-E_WAPI_QNULL));
	RT_ASSERT_RET_VALUE(q, (-E_WAPI_QNULL));
	RT_ASSERT_RET_VALUE(item, (-E_WAPI_QNULL));
	
	if(APDEV_IsFullQueue(q))
		return -E_APDEV_QFULL;
	if(itemsize > q->MaxData)
		return -E_APDEV_ITEM_TOO_LARGE;

 	spin_lock_irqsave(plock, flags);

	q->ItemArray[q->Tail].ItemSize = itemsize;
	memset(q->ItemArray[q->Tail].Item, 0, sizeof(q->ItemArray[q->Tail].Item));
	memcpy(q->ItemArray[q->Tail].Item, item, itemsize);
	q->NumItem++;
	if((q->Tail+1) == q->MaxItem)
		q->Tail = 0;
	else
		q->Tail++;

	spin_unlock_irqrestore(plock, flags);
	
	return E_APDEV_OK;
}
void notify_hostapd(void)
{ 
	struct task_struct *p;
        
	if(pid_hostapd != 0){

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26))
		p = find_task_by_pid(pid_hostapd);
#else
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
                p = find_task_by_vpid(pid_hostapd);
#else
		p = pid_task(find_vpid(pid_hostapd), PIDTYPE_PID);
#endif
#endif
		if(p){
			send_sig(SIGUSR1,p,0); 
		}else {
			pid_hostapd = 0;
		}
	}
}
int apdev_create_event_send(struct rtllib_device *ieee, u8 *Buff, u16 BufLen)
{
	APDEV_EVENT_T *pEvent;
	u8 *pbuf = NULL;
	int ret = 0;
	
	printk("==========> %s\n", __FUNCTION__);
	
	RT_ASSERT_RET_VALUE(ieee, -1);
	RT_ASSERT_RET_VALUE(MacAddr, -1);

	pbuf= (u8 *)kmalloc((sizeof(APDEV_EVENT_T) + BufLen), GFP_ATOMIC);
	if(NULL == pbuf)
	    return -1;

	pEvent = (APDEV_EVENT_T *)pbuf;	
	pEvent->BuffLength = BufLen;
	if(BufLen > 0){
		memcpy(pEvent->Buff, Buff, BufLen);
	}

	ret = apdev_en_queue(&ieee->apdev_queue_lock, ieee->apdev_queue, pbuf, (sizeof(APDEV_EVENT_T) + BufLen));
	notify_hostapd();

	if(pbuf)
		kfree(pbuf);

	printk("<========== %s\n", __FUNCTION__);
	return ret;
}

void apdev_init(struct net_device* apdev)
{
	struct apdev_priv *appriv;
	ether_setup(apdev);
#ifdef HAVE_NET_DEVICE_OPS
	apdev->netdev_ops = &apdev_netdev_ops;
#else
	apdev->open = apdev_open;
	apdev->stop = apdev_close;
	apdev->hard_start_xmit = apdev_tx;
	apdev->do_ioctl = apdev_ioctl;
	apdev->get_stats = apdev_stats;
	apdev->tx_timeout = apdev_tx_timeout;
#endif
	apdev->wireless_handlers = &apdev_wx_handlers_def;
	apdev->watchdog_timeo = HZ*3;
	apdev->type = ARPHRD_ETHER;
	apdev->destructor = free_netdev;

	memset(apdev->broadcast, 0xFF, ETH_ALEN);


	appriv = netdev_priv_rsl(apdev);
	memset(appriv, 0, sizeof(struct apdev_priv));
	return;
}
#endif
