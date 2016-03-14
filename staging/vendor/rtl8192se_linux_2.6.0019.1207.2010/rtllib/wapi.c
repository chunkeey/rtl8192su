#if defined (RTL8192S_WAPI_SUPPORT)

#include <linux/unistd.h>
#include <linux/etherdevice.h>
#include "wapi.h"
#include "wapi_interface.h"
#include "rtllib.h"

#define LITTLE_ENDIAN   
#define ENCRYPT  0     
#define DECRYPT  1     

u32 wapi_debug_component = 	WAPI_INIT    	|
				WAPI_ERR ; 

void WapiInit(struct rtllib_device *ieee)
{
	PRT_WAPI_T		pWapiInfo;
	int				i;

	WAPI_TRACE(WAPI_INIT, "===========> %s\n", __FUNCTION__);
	RT_ASSERT_RET(ieee);

	pWapiInfo =  &ieee->wapiInfo;
	pWapiInfo->bWapiEnable = false;

	INIT_LIST_HEAD(&ieee->cache_frag_list);

	INIT_LIST_HEAD(&pWapiInfo->wapiBKIDIdleList);
	INIT_LIST_HEAD(&pWapiInfo->wapiBKIDStoreList);
	for(i=0;i<WAPI_MAX_BKID_NUM;i++)
	{
		list_add_tail(&pWapiInfo->wapiBKID[i].list, &pWapiInfo->wapiBKIDIdleList);
	}

	INIT_LIST_HEAD(&pWapiInfo->wapiSTAIdleList);
	INIT_LIST_HEAD(&pWapiInfo->wapiSTAUsedList);
	for(i=0;i<WAPI_MAX_STAINFO_NUM;i++)
	{
		list_add_tail(&pWapiInfo->wapiSta[i].list, &pWapiInfo->wapiSTAIdleList);
	}

	spin_lock_init(&ieee->wapi_queue_lock);

	ieee->wapi_queue = (WAPI_QUEUE *)kmalloc((sizeof(WAPI_QUEUE)), GFP_KERNEL);
	if (!ieee->wapi_queue) {
		return;
	}
	memset((void *)ieee->wapi_queue, 0, sizeof (WAPI_QUEUE));
	WAPI_InitQueue(ieee->wapi_queue, WAPI_MAX_QUEUE_LEN, WAPI_MAX_BUFF_LEN);	
	
	WAPI_TRACE(WAPI_INIT, "<========== %s\n", __FUNCTION__);
}

void WapiExit(struct rtllib_device *ieee)
{
	WAPI_TRACE(WAPI_INIT, "===========> %s\n", __FUNCTION__);
	RT_ASSERT_RET(ieee);
	
	if(ieee->wapi_queue)
		kfree(ieee->wapi_queue);
	ieee->wapi_queue = 0;
	
	WAPI_TRACE(WAPI_INIT, "<========== %s\n", __FUNCTION__);
}

void WapiCreateAppEventAndSend(
	struct rtllib_device *ieee,
	u8 		*pbuffer,
	u16		buf_len,
	u8 		*DestAddr,
	u8		bUpdateBK,
	u8		bUpdateUSK,
	u8		bUpdateMSK,
	u8		RcvPktType,
	u8		bDisconnect
)
{
	PRT_WAPI_T pWapiInfo = &(ieee->wapiInfo);
	PRT_WAPI_STA_INFO pWapiSta = NULL;
	u8 WapiASUEPNInitialValueSrc[16] = {0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C} ;	
	u8 WapiAEMultiCastPNInitialValueSrc[16] = {0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C} ;	
	u8 bFind = false, bRecvAEPacket = false, bRecvASUEPacket = false, EventId = 0;

	WAPI_TRACE(WAPI_API, "==========> %s\n", __FUNCTION__);
	WAPI_TRACE(WAPI_API, "DestAddr="MAC_FMT" bUpdateBK=%d bUpdateUSK=%d bUpdateMSK=%d RcvPktType=%d bDisconnect=%d\n", 
			MAC_ARG(DestAddr), bUpdateBK, bUpdateUSK, bUpdateMSK, RcvPktType,bDisconnect);

	/*if(!pWapiInfo->bWapiEnable){
		WAPI_TRACE(WAPI_ERR,"%s: ieee->WapiSupport = 0!!\n",__FUNCTION__);
		return;
	}*/
	
	if(list_empty(&pWapiInfo->wapiSTAUsedList)){
		bFind = false;
	}else{
		list_for_each_entry(pWapiSta, &pWapiInfo->wapiSTAUsedList, list) {
			if(!memcmp(DestAddr,pWapiSta->PeerMacAddr,ETH_ALEN)){
				bFind = true;
				break;
			}
		}
	}

	WAPI_TRACE(WAPI_API, "%s: DestAddr="MAC_FMT" bFind=%d\n", __FUNCTION__, MAC_ARG(DestAddr), bFind);
	switch(RcvPktType){
		case WAPI_PREAUTHENTICATE:
			 EventId = WAPI_EVENT_RCV_PREAUTHENTICATE;
			 bRecvAEPacket = true;
			 bRecvASUEPacket = false;
			 break;
		case WAPI_ACCESS_AUTHENTICATE_REQUEST:
			 EventId = WAPI_EVENT_RCV_ACCESS_AUTHENTICATE_REQUEST;
			 bRecvAEPacket = true;
			 bRecvASUEPacket = false;
			 break;
		case WAPI_USK_RESPONSE:
			 EventId = WAPI_EVENT_RCV_USK_RESPONSE;
			 bRecvAEPacket = true;
			 bRecvASUEPacket = false;
			 break;
		case WAPI_MSK_RESPONSE:
			 EventId = WAPI_EVENT_RCV_MSK_RESPONSE;
			 bRecvAEPacket = true;
			 bRecvASUEPacket = false;
			 break;
		case WAPI_STAKEY_REQUEST:
			 EventId = WAPI_EVENT_RCV_STAKEY_REQUEST;
			 bRecvAEPacket = false;
			 bRecvASUEPacket = true;
			 break;
		case WAPI_AUTHENTICATE_ACTIVE:
			 EventId = WAPI_EVENT_RCV_AUTHENTICATE_ACTIVE;
			 bRecvAEPacket = false;
			 bRecvASUEPacket = true;
			 break;
		case WAPI_ACCESS_AUTHENTICATE_RESPONSE:
			 EventId = WAPI_EVENT_RCV_ACCESS_AUTHENTICATE_RESPONSE;
			 bRecvAEPacket = false;
			 bRecvASUEPacket = true;
			 break;
		case WAPI_USK_REQUEST:
			 EventId = WAPI_EVENT_RCV_USK_REQUEST;
			 bRecvAEPacket = false;
			 bRecvASUEPacket = true;
			 break;
		case WAPI_USK_CONFIRM:
			 EventId = WAPI_EVENT_RCV_USK_CONFIRM;
			 bRecvAEPacket = false;
			 bRecvASUEPacket = true;
			 break;
		case WAPI_MSK_NOTIFICATION:
			 EventId = WAPI_EVENT_RCV_MSK_NOTIFICATION;
			 bRecvAEPacket = false;
			 bRecvASUEPacket = true;
			 break;
		default:
			 break;
	}

	if(ieee->iw_mode == IW_MODE_INFRA
#ifdef ASL
		|| ieee->iw_mode == IW_MODE_APSTA
#endif
		)
	{
		if(bRecvAEPacket || bUpdateMSK){
			goto out;
		}
		if(bRecvASUEPacket){
			WAPI_CreateEvent_Send(ieee, EventId, DestAddr, pbuffer, buf_len);
			goto out;
		}
		if(bUpdateBK && bFind){
			EventId = WAPI_EVENT_ASUE_UPDATE_BK;
			WAPI_CreateEvent_Send(ieee, EventId, DestAddr, NULL, 0);
			goto out;
		}
		if(bUpdateUSK&& bFind){
			EventId = WAPI_EVENT_ASUE_UPDATE_USK;
			WAPI_CreateEvent_Send(ieee, EventId, DestAddr, NULL, 0);
			goto out;
		}
		if(bDisconnect && bFind){
			EventId = WAPI_EVENT_DISCONNECT;
			WAPI_CreateEvent_Send(ieee, EventId, DestAddr, NULL, 0);
			goto out;
		}
	}
	else if(ieee->iw_mode == IW_MODE_ADHOC)
	{
		if((bFind )&& (!pWapiSta->bSetkeyOk) && (bUpdateBK ||bUpdateUSK||bUpdateMSK))
			goto out;
		if(bRecvASUEPacket){
			WAPI_CreateEvent_Send(ieee, EventId, DestAddr, pbuffer, buf_len);
			if(EventId != WAPI_EVENT_RCV_USK_REQUEST)
				goto out;
		}
		if((!bFind) && (!bDisconnect) && (!bUpdateMSK))
		{
			if(!list_empty(&pWapiInfo->wapiSTAIdleList))
			{
				pWapiSta =(PRT_WAPI_STA_INFO)list_entry(pWapiInfo->wapiSTAIdleList.next, RT_WAPI_STA_INFO, list);
				list_del_init(&pWapiSta->list);
				list_add_tail(&pWapiSta->list, &pWapiInfo->wapiSTAUsedList);
				WAPI_TRACE(WAPI_API, "%s: Add wapi station "MAC_FMT"\n", __FUNCTION__, MAC_ARG(DestAddr));
				memcpy(pWapiSta->PeerMacAddr,DestAddr,6);
				memcpy(pWapiSta->lastRxMulticastPN, WapiAEMultiCastPNInitialValueSrc, 16);
				memcpy(pWapiSta->lastRxUnicastPN, WapiASUEPNInitialValueSrc, 16);
			}
			
			pWapiInfo->bFirstAuthentiateInProgress= true;

			EventId = WAPI_EVENT_FIRST_AUTHENTICATOR;
			WAPI_CreateEvent_Send(ieee, EventId, DestAddr, NULL, 0);
			goto out;
		}else{
			if(bRecvAEPacket){
				WAPI_CreateEvent_Send(ieee, EventId, DestAddr, pbuffer, buf_len);
				goto out;
			}
			if(bDisconnect){
				EventId = WAPI_EVENT_DISCONNECT;
				WAPI_CreateEvent_Send(ieee, EventId, DestAddr, NULL, 0);
				goto out;
			}
			if(bUpdateBK){
				EventId = WAPI_EVENT_AE_UPDATE_BK;
				WAPI_CreateEvent_Send(ieee, EventId, DestAddr, NULL, 0);
				goto out;
			}
			if(bUpdateUSK){
				EventId = WAPI_EVENT_AE_UPDATE_USK;
				WAPI_CreateEvent_Send(ieee, EventId, DestAddr, NULL, 0);
				goto out;
			}
			if(bUpdateMSK){
				list_for_each_entry(pWapiSta, &pWapiInfo->wapiSTAUsedList, list) {
					if(pWapiSta->bSetkeyOk){
						EventId = WAPI_EVENT_AE_UPDATE_MSK;
						WAPI_CreateEvent_Send(ieee, EventId, pWapiSta->PeerMacAddr, NULL, 0);
					}
				}
				goto out;
			}
		}
	}
	
out:	
	WAPI_TRACE(WAPI_API, "<========== %s\n", __FUNCTION__);
	return;
}


void WapiReturnAllStaInfo(struct rtllib_device *ieee)
{
	PRT_WAPI_T				pWapiInfo;
	PRT_WAPI_STA_INFO		pWapiStaInfo;
	PRT_WAPI_BKID			pWapiBkid;
	WAPI_TRACE(WAPI_INIT, "===========> %s\n", __FUNCTION__);
	
	pWapiInfo = &ieee->wapiInfo;

	while(!list_empty(&(pWapiInfo->wapiSTAUsedList)))
	{
		pWapiStaInfo = (PRT_WAPI_STA_INFO)list_entry(pWapiInfo->wapiSTAUsedList.next, RT_WAPI_STA_INFO, list);
		list_del_init(&pWapiStaInfo->list);
		memset(pWapiStaInfo->PeerMacAddr,0,ETH_ALEN);
		pWapiStaInfo->bSetkeyOk = 0;
		list_add_tail(&pWapiStaInfo->list, &pWapiInfo->wapiSTAIdleList);
	}

	while(!list_empty(&(pWapiInfo->wapiBKIDStoreList)))
	{
		pWapiBkid = (PRT_WAPI_BKID)list_entry(pWapiInfo->wapiBKIDStoreList.next, RT_WAPI_BKID, list);
		list_del_init(&pWapiBkid->list);
		memset(pWapiBkid->bkid,0,16);
		list_add_tail(&pWapiBkid->list, &pWapiInfo->wapiBKIDIdleList);
	}
	WAPI_TRACE(WAPI_INIT, "<========== %s\n", __FUNCTION__);
}

void WapiReturnOneStaInfo(struct rtllib_device *ieee, u8 *MacAddr, u8 from_app)
{
	PRT_WAPI_T				pWapiInfo;
	PRT_WAPI_STA_INFO		pWapiStaInfo = NULL;
	PRT_WAPI_BKID			pWapiBkid = NULL;

	pWapiInfo = &ieee->wapiInfo;

	WAPI_TRACE(WAPI_API, "==========> %s\n", __FUNCTION__);

	if(!from_app)
		WapiCreateAppEventAndSend(ieee,NULL,0,MacAddr, false,false,false,0,true);
	if(list_empty(&(pWapiInfo->wapiSTAUsedList))){
		return;
	}else{
		list_for_each_entry(pWapiStaInfo, &pWapiInfo->wapiSTAUsedList, list) {
			if(!memcmp(pWapiStaInfo->PeerMacAddr,MacAddr,ETH_ALEN)){
				pWapiStaInfo->bAuthenticateInProgress = false;
				pWapiStaInfo->bSetkeyOk = false;
				memset(pWapiStaInfo->PeerMacAddr,0,ETH_ALEN);
				list_del_init(&pWapiStaInfo->list);
				list_add_tail(&pWapiStaInfo->list, &pWapiInfo->wapiSTAIdleList);
				break;
			}

		}
	}

	if(ieee->iw_mode == IW_MODE_INFRA
#ifdef ASL
		|| ieee->iw_mode == IW_MODE_APSTA
#endif
		)
	{
		while(!list_empty(&(pWapiInfo->wapiBKIDStoreList)))
		{
			pWapiBkid = (PRT_WAPI_BKID)list_entry(pWapiInfo->wapiBKIDStoreList.next, RT_WAPI_BKID, list);
			list_del_init(&pWapiBkid->list);
			memset(pWapiBkid->bkid,0,16);
			list_add_tail(&pWapiBkid->list, &pWapiInfo->wapiBKIDIdleList);
		}
	}
	WAPI_TRACE(WAPI_API, "<========== %s\n", __FUNCTION__);
	return;
}

void WapiFreeAllStaInfo(struct rtllib_device *ieee)
{
	PRT_WAPI_T				pWapiInfo;
	PRT_WAPI_STA_INFO		pWapiStaInfo;
	PRT_WAPI_BKID			pWapiBkid;
	WAPI_TRACE(WAPI_INIT, "===========> %s\n", __FUNCTION__);
	pWapiInfo = &ieee->wapiInfo;

	WapiReturnAllStaInfo(ieee);
	while(!list_empty(&(pWapiInfo->wapiSTAIdleList)))
	{
		pWapiStaInfo = (PRT_WAPI_STA_INFO)list_entry(pWapiInfo->wapiSTAIdleList.next, RT_WAPI_STA_INFO, list);
		list_del_init(&pWapiStaInfo->list);
	}

	while(!list_empty(&(pWapiInfo->wapiBKIDIdleList)))
	{
		pWapiBkid = (PRT_WAPI_BKID)list_entry(pWapiInfo->wapiBKIDIdleList.next, RT_WAPI_BKID, list);
		list_del_init(&pWapiBkid->list);
	}
	WAPI_TRACE(WAPI_INIT, "<=========== %s\n", __FUNCTION__);
	return;
}

u8 SecIsWAIPacket(struct rtllib_device* ieee,struct sk_buff *skb)
{
	PRT_WAPI_T pWapiInfo = &(ieee->wapiInfo);
	PRT_WAPI_STA_INFO pWapiSta = NULL;
	u8 WaiPkt = 0, *pTaddr, bFind = false;
	u8 Offset_TypeWAI = 24 + 6;	
	struct rtllib_hdr_3addrqos *header;
	u16 mask = 1, fc = 0;

	if((!pWapiInfo->bWapiEnable) || (!ieee->wapiInfo.bWapiEnable))
		return 0;

	header = (struct rtllib_hdr_3addrqos *)skb->data;

	fc = le16_to_cpu(header->frame_ctl);
	if(fc & RTLLIB_FCTL_WEP)
	    return 0;

	pTaddr = header->addr2;
	if(list_empty(&pWapiInfo->wapiSTAUsedList)){
		bFind = false;
	}else{
		list_for_each_entry(pWapiSta, &pWapiInfo->wapiSTAUsedList, list){
			if(!memcmp(pTaddr,pWapiSta->PeerMacAddr,6)){
				bFind = true;
				break;
			}
		}
	}

	WAPI_TRACE(WAPI_API, "%s: bFind=%d pTaddr="MAC_FMT"\n", __FUNCTION__, bFind, MAC_ARG(pTaddr));

	if( IsQoSDataFrame(skb->data) ){
		Offset_TypeWAI += sQoSCtlLng;	
	}

	if((header->frame_ctl & (mask<<14)) != 0){
		Offset_TypeWAI += WAPI_EXT_LEN;	
	}

	if( skb->len < (Offset_TypeWAI+1) ){
		WAPI_TRACE(WAPI_ERR, "%s(): invalid length(%d)\n",__FUNCTION__,skb->len);
		return 0;
	}

	if( (skb->data[Offset_TypeWAI]==0x88) && (skb->data[Offset_TypeWAI+1]==0xb4) ){
		WaiPkt = skb->data[Offset_TypeWAI+5];
		if(ieee->iw_mode == IW_MODE_ADHOC){
			if(bFind){
				if((WaiPkt == 8) && (pWapiInfo->wapiTxMsk.bSet) && (pWapiSta->wapiMsk.bSet) && ((skb->data[Offset_TypeWAI+14] & 0x10)==0)){
					printk("==============> %s(): Receive USK Request After MSK set!\n", __FUNCTION__);
					DelStaInfo(ieee, pTaddr);
					WapiReturnOneStaInfo (ieee, pTaddr, 0);
					WAPI_CreateEvent_Send(ieee, WAPI_EVENT_CONNECT, ieee->current_network.bssid, NULL, 0);
				}

			}
		}
	}else{
		WAPI_TRACE(WAPI_API, "%s(): non wai packet\n",__FUNCTION__);
	}

	WAPI_TRACE(WAPI_API, "%s(): Recvd WAI frame. IsWAIPkt(%d)\n",__FUNCTION__, WaiPkt);

	return	WaiPkt;
}

/******************
*********************/
u8 Wapi_defragment(struct rtllib_device* ieee,u8* data, u16 len,
	          u8* pTaddr,bool bAuthenticator,u8* rxbuffer,u16* rxbuffer_len)
{
	PRT_CACHE_INFO pcache_info = NULL;
	bool bfind = false;
	
	if(!list_empty(&ieee->cache_frag_list)){
		list_for_each_entry(pcache_info, &ieee->cache_frag_list, list) {
			if((memcmp(pcache_info->saddr,pTaddr,ETH_ALEN)==0) 
				&& (pcache_info->bAuthenticator == bAuthenticator))
			{
				bfind = true;
				break;
			}
		}		
	}
	WAPI_TRACE(WAPI_RX, "%s: bFind=%d pTaddr="MAC_FMT"\n", __FUNCTION__, bfind, MAC_ARG(pTaddr));
	if(bfind == false) {
		pcache_info = kmalloc(sizeof(RT_CACHE_INFO),GFP_ATOMIC);
		if(pcache_info == NULL){
			WAPI_TRACE(WAPI_ERR,"%s(): can't malloc mem\n", __FUNCTION__);
			goto drop2;
		}
		memset(pcache_info,0,sizeof(RT_CACHE_INFO));
		if(data[11] == 0x1){
			WAPI_TRACE(WAPI_RX, "%s(): First fragment, and have More fragments\n",__FUNCTION__);
			memcpy(&(pcache_info->recvSeq),data+8, 2);
			if(data[10] != 0x00)
			{
				WAPI_TRACE(WAPI_ERR, "%s(): First fragment,but fragnum is not 0.\n",__FUNCTION__);
				goto drop1;
			}else{	
				data[10] = 0x0;
				data[11] = 0x0;
				if (len > 2000) {
					WAPI_TRACE(WAPI_ERR,"111****************************%s():cache buf len is not enough: %d\n",__FUNCTION__,len);
					goto drop1;
				}
				pcache_info->lastFragNum= 0x00;
				memcpy(pcache_info->cache_buffer,data,len);
				pcache_info->cache_buffer_len = len;
				pcache_info->bAuthenticator = bAuthenticator;
				memcpy(pcache_info->saddr,pTaddr,ETH_ALEN);
				list_add_tail(&pcache_info->list, &ieee->cache_frag_list);
				WAPI_TRACE(WAPI_RX, "%s(): First fragment, allocate cache to store.\n",__FUNCTION__);
				goto drop2;
			}
		}else{
			if(data[10] == 0){
				WAPI_TRACE(WAPI_RX, "%s(): First fragment, no More fragment, ready to send to App.\n",__FUNCTION__);
				if (len > 2000) {
					WAPI_TRACE(WAPI_ERR,"222****************************%s():cache buf len is not enough: %d\n",__FUNCTION__,len);
					goto drop1;
				}
				memcpy(rxbuffer,data,len);
				*rxbuffer_len = len;
				goto success;
			}else{
				WAPI_TRACE(WAPI_ERR, "%s(): First fragment,no More fragment, but fragnum is not 0.\n",__FUNCTION__);
				goto drop1;
			}
		}
	}
	else{
		if(data[11] == 0x1){
			if(memcmp(data+8,&(pcache_info->recvSeq),2)) {
				WAPI_TRACE(WAPI_ERR, "%s(): Not First fragment, More fragment, seq num error.\n",__FUNCTION__);
				list_del(&pcache_info->list);
				goto drop1;
			}else{
				if(data[10] == (pcache_info->lastFragNum+1)){
					WAPI_TRACE(WAPI_RX, "%s(): Not First fragment, More fragment, same seq num, copy to cache.\n",__FUNCTION__);
					if ((pcache_info->cache_buffer_len + len - 12) > 2000) {
						WAPI_TRACE(WAPI_ERR,"333****************************%s():cache buf len is not enough: %d\n",__FUNCTION__,pcache_info->cache_buffer_len + len - 12);
						list_del(&pcache_info->list);
						goto drop1;
					}
					memcpy(pcache_info->cache_buffer+(pcache_info->cache_buffer_len),data+12,len-12);
					pcache_info->cache_buffer_len += len-12;
					pcache_info->lastFragNum  = data[10];
					goto drop2;
				}else{
					WAPI_TRACE(WAPI_ERR, "%s(): Not First fragment, More fragment, same seq num, fragnum error.\n",__FUNCTION__);
					list_del(&pcache_info->list);
					goto drop1;
				}
			}
		}else{
			if(memcmp(data+8,&(pcache_info->recvSeq),2)) {
				WAPI_TRACE(WAPI_ERR, "%s(): Not First fragment, no More fragment, seq num error.\n",__FUNCTION__);
				list_del(&pcache_info->list);
				goto drop1;
			}else{
				if(data[10] == (pcache_info->lastFragNum+1)){					
					WAPI_TRACE(WAPI_RX, "%s(): Not First fragment, no More fragment, same seq num, ready to send to App.\n",__FUNCTION__);
					if ((pcache_info->cache_buffer_len + len - 12) > 2000) {
						WAPI_TRACE(WAPI_ERR,"444****************************%s():cache buf len is not enough: %d\n",__FUNCTION__,pcache_info->cache_buffer_len + len - 12);
						list_del(&pcache_info->list);
						goto drop1;
					}
					memcpy(pcache_info->cache_buffer+(pcache_info->cache_buffer_len),data+12,len-12);
					pcache_info->cache_buffer_len += len-12;
					memcpy(rxbuffer,pcache_info->cache_buffer,pcache_info->cache_buffer_len);
					*rxbuffer_len = pcache_info->cache_buffer_len;
					list_del(&pcache_info->list);
					goto success;
				}else{
					WAPI_TRACE(WAPI_ERR, "%s(): Not First fragment, no More fragment, same seq num, fragnum error.\n",__FUNCTION__);
					list_del(&pcache_info->list);
					goto drop1;
				}
			}
		}
	}

drop1:
	if(pcache_info)
		kfree(pcache_info);
	pcache_info = NULL;
	return false;
drop2:
	return false;
success:
	if(pcache_info)
		kfree(pcache_info);
	pcache_info = NULL;
	return true;
}

/****************************************************************************
 * data[8-9]: Sequence Number
 * data[10]: Fragment No
 * data[11]: Flag = 1 indicates more data.
 *****************************************************************************/
void WapiHandleRecvPacket(struct rtllib_device* ieee,struct sk_buff *skb,u8 WaiPkt)
{
	PRT_WAPI_T			pWapiInfo;
	struct rtllib_hdr_3addrqos *hdr;
	u8 *pTaddr, *recvPtr, *rxbuffer;
 	u8 bAuthenticator = false, receive_result = false;
	int hdrlen = 0;
	u16 recvLength = 0, fc = 0, rxbuffer_len = 0;

	WAPI_TRACE(WAPI_RX, "===========> %s: WaiPkt is %d\n", __FUNCTION__,WaiPkt);
	
	hdr = (struct rtllib_hdr_3addrqos *)skb->data;
	pTaddr = hdr->addr2;
	fc = hdr->frame_ctl;
	hdrlen = rtllib_get_hdrlen(fc);
	
	pWapiInfo = &(ieee->wapiInfo);
	if((WaiPkt == WAPI_CERTIFICATE_AUTHENTICATE_REQUEST) 
		||(WaiPkt == WAPI_CERTIFICATE_AUTHENTICATE_RESPONSE)) 
	{
		WAPI_TRACE(WAPI_RX, "%s: Valid Wai Packet \n", __FUNCTION__);
		return;
	}else{
		switch(WaiPkt)
		{
			case WAPI_PREAUTHENTICATE:
			case WAPI_ACCESS_AUTHENTICATE_REQUEST:
			case WAPI_USK_RESPONSE:
			case WAPI_MSK_RESPONSE:
				 bAuthenticator = true;
				 break;
			default:
				  break;
		}
		recvLength = skb->len - hdrlen - (SNAP_SIZE + sizeof(u16));
		recvPtr = skb->data + hdrlen + (SNAP_SIZE + sizeof(u16));

		rxbuffer = kmalloc(2000, GFP_ATOMIC);
		if(NULL == rxbuffer)
			return;
		
		receive_result = Wapi_defragment(ieee,recvPtr, recvLength,pTaddr,bAuthenticator,rxbuffer,&rxbuffer_len);
		if(receive_result)
			WapiCreateAppEventAndSend(ieee, rxbuffer, rxbuffer_len, pTaddr, false,false, false, WaiPkt, false);
		
		kfree(rxbuffer);
	}
	WAPI_TRACE(WAPI_RX, "<=========== %s\n", __FUNCTION__);
}

void WapiSetIE(struct rtllib_device *ieee)
{
	PRT_WAPI_T		pWapiInfo = &(ieee->wapiInfo);
	u16		protocolVer = 1;
	u16		akmCnt = 1;
	u16		suiteCnt = 1;
	u16		capability = 0;
	u8		OUI[3];

	OUI[0] = 0x00;
	OUI[1] = 0x14;
	OUI[2] = 0x72;
	
	pWapiInfo->wapiIELength = 0;
	memcpy(pWapiInfo->wapiIE+pWapiInfo->wapiIELength, &protocolVer, 2);
	pWapiInfo->wapiIELength +=2;
	memcpy(pWapiInfo->wapiIE+pWapiInfo->wapiIELength, &akmCnt, 2);
	pWapiInfo->wapiIELength +=2;

	if(pWapiInfo->bWapiPSK){
		memcpy(pWapiInfo->wapiIE+pWapiInfo->wapiIELength,OUI, 3);
		pWapiInfo->wapiIELength +=3;
		pWapiInfo->wapiIE[pWapiInfo->wapiIELength] = 0x2;
		pWapiInfo->wapiIELength +=1;
	}else{
		memcpy(pWapiInfo->wapiIE+pWapiInfo->wapiIELength,OUI, 3);
		pWapiInfo->wapiIELength +=3;
		pWapiInfo->wapiIE[pWapiInfo->wapiIELength] = 0x1;
		pWapiInfo->wapiIELength +=1;
	}

	memcpy(pWapiInfo->wapiIE+pWapiInfo->wapiIELength, &suiteCnt, 2);
	pWapiInfo->wapiIELength +=2;
	memcpy(pWapiInfo->wapiIE+pWapiInfo->wapiIELength,OUI, 3);
	pWapiInfo->wapiIELength +=3;
	pWapiInfo->wapiIE[pWapiInfo->wapiIELength] = 0x1;
	pWapiInfo->wapiIELength +=1;

	memcpy(pWapiInfo->wapiIE+pWapiInfo->wapiIELength,OUI, 3);
	pWapiInfo->wapiIELength +=3;
	pWapiInfo->wapiIE[pWapiInfo->wapiIELength] = 0x1;
	pWapiInfo->wapiIELength +=1;

	memcpy(pWapiInfo->wapiIE+pWapiInfo->wapiIELength, &capability, 2);
	pWapiInfo->wapiIELength +=2;
}


/*  PN1 > PN2, return 1,
 *  else return 0.
 */
u32 WapiComparePN(u8 *PN1, u8 *PN2)
{
	char i;

	if ((NULL == PN1) || (NULL == PN2))
		return 1;

	if ((PN2[15] - PN1[15]) & 0x80)
		return 1;

	for (i=16; i>0; i--)
	{
		if(PN1[i-1] == PN2[i-1])
		    	continue;
		else if(PN1[i-1] > PN2[i-1])
			return 1;
		else
			return 0;			
	}

	return 0;
}

/* AddCount: 1 or 2. 
 *  If overflow, return 1,
 *  else return 0.
 */
u8 WapiIncreasePN(u8 *PN, u8 AddCount)
{
	u8  i;

	if (NULL == PN)
		return 1;
	/*
	if(AddCount == 2){
		printk("############################%s(): PN[0]=0x%x\n", __FUNCTION__, PN[0]);
		if(PN[0] == 0x48){
			PN[0] += AddCount;
			return 1;
		}else{
			PN[0] += AddCount;
			return 0;
		}
	}
	*/

	for (i=0; i<16; i++)
	{
		if (PN[i] + AddCount <= 0xff)
		{
			PN[i] += AddCount;
			return 0;
		}
		else
		{
			PN[i] += AddCount;
			AddCount = 1;
		}
	}

	return 1;
}


void WapiGetLastRxUnicastPNForQoSData(
	u8 			UserPriority,
	PRT_WAPI_STA_INFO    pWapiStaInfo,
	u8 *PNOut
)
{
	WAPI_TRACE(WAPI_RX, "===========> %s\n", __FUNCTION__);
	switch(UserPriority)
	{
		case 0:
		case 3:
			      memcpy(PNOut,pWapiStaInfo->lastRxUnicastPNBEQueue,16);
			      break;
		case 1:
		case 2:
			      memcpy(PNOut,pWapiStaInfo->lastRxUnicastPNBKQueue,16);
			      break;
		case 4:
		case 5:
			      memcpy(PNOut,pWapiStaInfo->lastRxUnicastPNVIQueue,16);
			      break;
		case 6:
		case 7:
			      memcpy(PNOut,pWapiStaInfo->lastRxUnicastPNVOQueue,16);
			      break;
		default:
				WAPI_TRACE(WAPI_ERR, "%s: Unknown TID \n", __FUNCTION__);
				break;
	}
	WAPI_TRACE(WAPI_RX, "<=========== %s\n", __FUNCTION__);
}


void WapiSetLastRxUnicastPNForQoSData(
	u8 		UserPriority,
	u8           *PNIn,
	PRT_WAPI_STA_INFO    pWapiStaInfo
)
{
	WAPI_TRACE(WAPI_RX, "===========> %s\n", __FUNCTION__);
	switch(UserPriority)
	{
		case 0:
		case 3:
			      memcpy(pWapiStaInfo->lastRxUnicastPNBEQueue,PNIn,16);
			      break;
		case 1:
		case 2:
			      memcpy(pWapiStaInfo->lastRxUnicastPNBKQueue,PNIn,16);
			      break;
		case 4:
		case 5:
			      memcpy(pWapiStaInfo->lastRxUnicastPNVIQueue,PNIn,16);
			      break;
		case 6:
		case 7:
			      memcpy(pWapiStaInfo->lastRxUnicastPNVOQueue,PNIn,16);
			      break;
		default:
				WAPI_TRACE(WAPI_ERR, "%s: Unknown TID \n", __FUNCTION__);
				break;
	}
	WAPI_TRACE(WAPI_RX, "<=========== %s\n", __FUNCTION__);
}


/****************************************************************************
TRUE-----------------bRxReorder == FALSE not RX-Reorder
FALSE----------------bRxReorder == TRUE do RX Reorder
add to support WAPI to N-mode
*****************************************************************************/
u8 WapiCheckPnInSwDecrypt(
	struct rtllib_device *ieee,
	struct sk_buff *pskb
)
{
	struct rtllib_hdr_3addrqos *header;
	u16 				fc;
	u8				*pDaddr, *pTaddr, *pRaddr;
	u8 				ret = false;

	header = (struct rtllib_hdr_3addrqos *)pskb->data;
	pTaddr = header->addr2;
	pRaddr = header->addr1;
	fc = le16_to_cpu(header->frame_ctl);
	
	if((fc & RTLLIB_FCTL_TODS) == RTLLIB_FCTL_TODS)
		pDaddr = header->addr3;
	else
		pDaddr = header->addr1;
	
	if(eqMacAddr(pRaddr, ieee->dev->dev_addr) &&
		!is_multicast_ether_addr(pDaddr) &&
		ieee->current_network.qos_data.active &&
		IsQoSDataFrame(pskb->data) && ieee->pHTInfo->bCurrentHTSupport &&
		ieee->pHTInfo->bCurRxReorderEnable)
		ret = false;
	else
		ret = true;

	WAPI_TRACE(WAPI_RX, "%s: return %d\n", __FUNCTION__, ret);
	return ret;
}


/****************************************************************************
TRUE-----------------Drop
FALSE---------------- handle 
add to support WAPI to N-mode
*****************************************************************************/
u8 WapiCheckDropForRxReorderCase(
	struct rtllib_device *ieee,
	struct rtllib_rxb* prxb
)
{
	PRT_WAPI_T     pWapiInfo = &(ieee->wapiInfo);
	u8			*pLastRecvPN = NULL;
	u8			bFind = false;
	PRT_WAPI_STA_INFO	pWapiSta = NULL;

	if(!pWapiInfo->bWapiEnable)
		return false;

	if(list_empty(&pWapiInfo->wapiSTAUsedList)){
		bFind = false;
	}else{
		list_for_each_entry(pWapiSta, &pWapiInfo->wapiSTAUsedList, list) {
			if(!memcmp(prxb->WapiSrcAddr,pWapiSta->PeerMacAddr,ETH_ALEN)){
				bFind = true;
				break;
			}
		}
	}
	WAPI_TRACE(WAPI_RX, "%s: bFind=%d prxb->WapiSrcAddr="MAC_FMT"\n", __FUNCTION__, bFind, MAC_ARG(prxb->WapiSrcAddr));

	if(bFind){
		switch(prxb->UserPriority)
		{
			case 0:
			case 3:
				pLastRecvPN = pWapiSta->lastRxUnicastPNBEQueue;
				break;
			case 1:
			case 2:
				pLastRecvPN = pWapiSta->lastRxUnicastPNBKQueue;
				break;
			case 4:
			case 5:
				pLastRecvPN = pWapiSta->lastRxUnicastPNVIQueue;
				break;
			case 6:
			case 7:
				pLastRecvPN = pWapiSta->lastRxUnicastPNVOQueue;
				break;
			default:
				WAPI_TRACE(WAPI_ERR,"%s: Unknown TID \n",__FUNCTION__);
				break;
		}

		if(!WapiComparePN(prxb->WapiTempPN,pLastRecvPN))
		{
			WAPI_TRACE(WAPI_RX,"%s: Equal PN!!\n",__FUNCTION__);
			return true;
		}
		else
		{
			memcpy(pLastRecvPN,prxb->WapiTempPN,16);
			return false;
		}
	}
	else
		return false;
}

int WapiSendWaiPacket(struct rtllib_device *ieee, struct sk_buff *pskb)
{
	struct sk_buff * newskb = NULL;
	struct rtllib_hdr_3addr *mac_hdr=NULL;
	cb_desc *tcb_desc = NULL;

	RT_ASSERT_RET_VALUE(ieee,-1);
	RT_ASSERT_RET_VALUE(pskb,-1);

	if(pskb->len < (14 + sizeof(struct rtllib_hdr_3addr)))
	{
		WAPI_TRACE(WAPI_ERR, "%s: WAI frame is too small!!\n", __FUNCTION__);
		goto failed;
	}

	skb_pull(pskb, 14);
	
	newskb = dev_alloc_skb(pskb->len+ieee->tx_headroom); 
	if(!newskb){
	    	WAPI_TRACE(WAPI_ERR,"%s: can't alloc skb\n",__FUNCTION__);
		goto failed;
	}
	skb_reserve(newskb, ieee->tx_headroom);	
	memcpy(skb_put(newskb, pskb->len), pskb->data, pskb->len);
	dev_kfree_skb_any(pskb);
	/* called with 2nd parm 0, no tx mgmt lock required */
	rtllib_sta_wakeup(ieee,0);

	tcb_desc = (cb_desc *)(newskb->cb + MAX_DEV_ADDR_SIZE);
#ifdef RTL8192SU_FPGA_UNSPECIFIED_NETWORK
	tcb_desc->queue_index = NORMAL_QUEUE;
#else
	tcb_desc->queue_index = BE_QUEUE;
#endif
	tcb_desc->data_rate = 0x02;	
	tcb_desc->bTxUseDriverAssingedRate = true;
	tcb_desc->RATRIndex = 7;
	tcb_desc->bTxDisableRateFallBack = 1;
#ifdef _RTL8192_EXT_PATCH_
	tcb_desc->macId = 0;
#endif	

	mac_hdr = (struct rtllib_hdr_3addr *)(newskb->data);
	mac_hdr->seq_ctl = cpu_to_le16(ieee->seq_ctrl[0] << 4);
	if (ieee->seq_ctrl[0] == 0xFFF)
		ieee->seq_ctrl[0] = 0;
	else
		ieee->seq_ctrl[0]++;

	if(!ieee->check_nic_enough_desc(ieee->dev,tcb_desc->queue_index)||\
			(skb_queue_len(&ieee->skb_waitQ[tcb_desc->queue_index]) != 0)||\
			(ieee->queue_stop) ) {
		WAPI_TRACE(WAPI_TX, "%s: Insert to waitqueue (idx=%d)!!\n", __FUNCTION__, tcb_desc->queue_index);
		skb_queue_tail(&ieee->skb_waitQ[tcb_desc->queue_index], newskb);
	} else {
		ieee->softmac_hard_start_xmit(newskb,ieee->dev);
	}

	return 0;
	
failed:	
	dev_kfree_skb_any(pskb);
	return -1;
}
/**********************************************************
 **********************************************************/
const u8 Sbox[256] = {
0xd6,0x90,0xe9,0xfe,0xcc,0xe1,0x3d,0xb7,0x16,0xb6,0x14,0xc2,0x28,0xfb,0x2c,0x05,
0x2b,0x67,0x9a,0x76,0x2a,0xbe,0x04,0xc3,0xaa,0x44,0x13,0x26,0x49,0x86,0x06,0x99,
0x9c,0x42,0x50,0xf4,0x91,0xef,0x98,0x7a,0x33,0x54,0x0b,0x43,0xed,0xcf,0xac,0x62,
0xe4,0xb3,0x1c,0xa9,0xc9,0x08,0xe8,0x95,0x80,0xdf,0x94,0xfa,0x75,0x8f,0x3f,0xa6,
0x47,0x07,0xa7,0xfc,0xf3,0x73,0x17,0xba,0x83,0x59,0x3c,0x19,0xe6,0x85,0x4f,0xa8,
0x68,0x6b,0x81,0xb2,0x71,0x64,0xda,0x8b,0xf8,0xeb,0x0f,0x4b,0x70,0x56,0x9d,0x35,
0x1e,0x24,0x0e,0x5e,0x63,0x58,0xd1,0xa2,0x25,0x22,0x7c,0x3b,0x01,0x21,0x78,0x87,
0xd4,0x00,0x46,0x57,0x9f,0xd3,0x27,0x52,0x4c,0x36,0x02,0xe7,0xa0,0xc4,0xc8,0x9e,
0xea,0xbf,0x8a,0xd2,0x40,0xc7,0x38,0xb5,0xa3,0xf7,0xf2,0xce,0xf9,0x61,0x15,0xa1,
0xe0,0xae,0x5d,0xa4,0x9b,0x34,0x1a,0x55,0xad,0x93,0x32,0x30,0xf5,0x8c,0xb1,0xe3,
0x1d,0xf6,0xe2,0x2e,0x82,0x66,0xca,0x60,0xc0,0x29,0x23,0xab,0x0d,0x53,0x4e,0x6f,
0xd5,0xdb,0x37,0x45,0xde,0xfd,0x8e,0x2f,0x03,0xff,0x6a,0x72,0x6d,0x6c,0x5b,0x51,
0x8d,0x1b,0xaf,0x92,0xbb,0xdd,0xbc,0x7f,0x11,0xd9,0x5c,0x41,0x1f,0x10,0x5a,0xd8,
0x0a,0xc1,0x31,0x88,0xa5,0xcd,0x7b,0xbd,0x2d,0x74,0xd0,0x12,0xb8,0xe5,0xb4,0xb0,
0x89,0x69,0x97,0x4a,0x0c,0x96,0x77,0x7e,0x65,0xb9,0xf1,0x09,0xc5,0x6e,0xc6,0x84,
0x18,0xf0,0x7d,0xec,0x3a,0xdc,0x4d,0x20,0x79,0xee,0x5f,0x3e,0xd7,0xcb,0x39,0x48
};

const u32 CK[32] = {
	0x00070e15, 0x1c232a31, 0x383f464d, 0x545b6269,
	0x70777e85, 0x8c939aa1, 0xa8afb6bd, 0xc4cbd2d9,
	0xe0e7eef5, 0xfc030a11, 0x181f262d, 0x343b4249,
	0x50575e65, 0x6c737a81, 0x888f969d, 0xa4abb2b9,
	0xc0c7ced5, 0xdce3eaf1, 0xf8ff060d, 0x141b2229,
	0x30373e45, 0x4c535a61, 0x686f767d, 0x848b9299,
	0xa0a7aeb5, 0xbcc3cad1, 0xd8dfe6ed, 0xf4fb0209,
	0x10171e25, 0x2c333a41, 0x484f565d, 0x646b7279 };

#define Rotl(_x, _y) (((_x) << (_y)) | ((_x) >> (32 - (_y))))

#define ByteSub(_A) (Sbox[(_A) >> 24 & 0xFF] << 24 | \
                     Sbox[(_A) >> 16 & 0xFF] << 16 | \
                     Sbox[(_A) >>  8 & 0xFF] <<  8 | \
                     Sbox[(_A) & 0xFF])

#define L1(_B) ((_B) ^ Rotl(_B, 2) ^ Rotl(_B, 10) ^ Rotl(_B, 18) ^ Rotl(_B, 24))
#define L2(_B) ((_B) ^ Rotl(_B, 13) ^ Rotl(_B, 23))

static void
xor_block(void *dst, void *src1, void *src2)
/* 128-bit xor: *dst = *src1 xor *src2. Pointers must be 32-bit aligned  */
{
    ((u32 *)dst)[0] = ((u32 *)src1)[0] ^ ((u32 *)src2)[0];
    ((u32 *)dst)[1] = ((u32 *)src1)[1] ^ ((u32 *)src2)[1];
    ((u32 *)dst)[2] = ((u32 *)src1)[2] ^ ((u32 *)src2)[2];
    ((u32 *)dst)[3] = ((u32 *)src1)[3] ^ ((u32 *)src2)[3];
}


void SMS4Crypt(u8 *Input, u8 *Output, u32 *rk)
{
	 u32 r, mid, x0, x1, x2, x3, *p;
	 p = (u32 *)Input;
	 x0 = p[0];
	 x1 = p[1];
	 x2 = p[2];
	 x3 = p[3];
#ifdef LITTLE_ENDIAN
	 x0 = Rotl(x0, 16); x0 = ((x0 & 0x00FF00FF) << 8) | ((x0 & 0xFF00FF00) >> 8);
	 x1 = Rotl(x1, 16); x1 = ((x1 & 0x00FF00FF) << 8) | ((x1 & 0xFF00FF00) >> 8);
	 x2 = Rotl(x2, 16); x2 = ((x2 & 0x00FF00FF) << 8) | ((x2 & 0xFF00FF00) >> 8);
	 x3 = Rotl(x3, 16); x3 = ((x3 & 0x00FF00FF) << 8) | ((x3 & 0xFF00FF00) >> 8);
#endif
	 for (r = 0; r < 32; r += 4)
	 {
		  mid = x1 ^ x2 ^ x3 ^ rk[r + 0];
		  mid = ByteSub(mid);
		  x0 ^= L1(mid);
		  mid = x2 ^ x3 ^ x0 ^ rk[r + 1];
		  mid = ByteSub(mid);
		  x1 ^= L1(mid);
		  mid = x3 ^ x0 ^ x1 ^ rk[r + 2];
		  mid = ByteSub(mid);
		  x2 ^= L1(mid);
		  mid = x0 ^ x1 ^ x2 ^ rk[r + 3];
		  mid = ByteSub(mid);
		  x3 ^= L1(mid);
	 }
#ifdef LITTLE_ENDIAN
	 x0 = Rotl(x0, 16); x0 = ((x0 & 0x00FF00FF) << 8) | ((x0 & 0xFF00FF00) >> 8);
	 x1 = Rotl(x1, 16); x1 = ((x1 & 0x00FF00FF) << 8) | ((x1 & 0xFF00FF00) >> 8);
	 x2 = Rotl(x2, 16); x2 = ((x2 & 0x00FF00FF) << 8) | ((x2 & 0xFF00FF00) >> 8);
	 x3 = Rotl(x3, 16); x3 = ((x3 & 0x00FF00FF) << 8) | ((x3 & 0xFF00FF00) >> 8);
#endif
	 p = (u32 *)Output;
	 p[0] = x3;
	 p[1] = x2;
	 p[2] = x1;
	 p[3] = x0;
}



void SMS4KeyExt(u8 *Key, u32 *rk, u32 CryptFlag)
{
	 u32 r, mid, x0, x1, x2, x3, *p;
	 
	 p = (u32 *)Key;
	 x0 = p[0];
	 x1 = p[1];
	 x2 = p[2];
	 x3 = p[3];
#ifdef LITTLE_ENDIAN
	 x0 = Rotl(x0, 16); x0 = ((x0 & 0xFF00FF) << 8) | ((x0 & 0xFF00FF00) >> 8);
	 x1 = Rotl(x1, 16); x1 = ((x1 & 0xFF00FF) << 8) | ((x1 & 0xFF00FF00) >> 8);
	 x2 = Rotl(x2, 16); x2 = ((x2 & 0xFF00FF) << 8) | ((x2 & 0xFF00FF00) >> 8);
	 x3 = Rotl(x3, 16); x3 = ((x3 & 0xFF00FF) << 8) | ((x3 & 0xFF00FF00) >> 8);
#endif

	 x0 ^= 0xa3b1bac6;
	 x1 ^= 0x56aa3350;
	 x2 ^= 0x677d9197;
	 x3 ^= 0xb27022dc;
	 for (r = 0; r < 32; r += 4)
	 {
		  mid = x1 ^ x2 ^ x3 ^ CK[r + 0];
		  mid = ByteSub(mid);
		  rk[r + 0] = x0 ^= L2(mid);
		  mid = x2 ^ x3 ^ x0 ^ CK[r + 1];
		  mid = ByteSub(mid);
		  rk[r + 1] = x1 ^= L2(mid);
		  mid = x3 ^ x0 ^ x1 ^ CK[r + 2];
		  mid = ByteSub(mid);
		  rk[r + 2] = x2 ^= L2(mid);
		  mid = x0 ^ x1 ^ x2 ^ CK[r + 3];
		  mid = ByteSub(mid);
		  rk[r + 3] = x3 ^= L2(mid);
	 }
	 if (CryptFlag == DECRYPT)
	 {
	 	  for (r = 0; r < 16; r++)
	 	  	 mid = rk[r], rk[r] = rk[31 - r], rk[31 - r] = mid;
	 }
}


void WapiSMS4Cryption(u8 *Key, u8 *IV, u8 *Input, u16 InputLength, 
                                                u8 *Output, u16 *OutputLength, u32 CryptFlag)
{
	u32 blockNum,i,j, rk[32];
	u16 remainder;
	u8 blockIn[16],blockOut[16], tempIV[16], k;

	*OutputLength = 0;
	remainder = InputLength & 0x0F;
	blockNum = InputLength >> 4;
	if(remainder !=0)
		blockNum++;
	else
		remainder = 16;

	for(k=0;k<16;k++)
		tempIV[k] = IV[15-k];

	memcpy(blockIn, tempIV, 16);

      SMS4KeyExt((u8 *)Key, rk,CryptFlag);
 
	for(i=0; i<blockNum-1; i++)
	{
		SMS4Crypt((u8 *)blockIn, blockOut, rk);
             xor_block(&Output[i*16], &Input[i*16], blockOut);
		memcpy(blockIn,blockOut,16);
	}
	
	*OutputLength = i*16;

	SMS4Crypt((u8 *)blockIn, blockOut, rk);

	for(j=0; j<remainder; j++)
	{
		Output[i*16+j] = Input[i*16+j] ^ blockOut[j];
	}
      *OutputLength += remainder;  

}

void WapiSMS4Encryption(u8 *Key, u8 *IV, u8 *Input, u16 InputLength, 
                                                    u8 *Output, u16 *OutputLength)
{
    
	WapiSMS4Cryption(Key, IV, Input, InputLength, Output, OutputLength, ENCRYPT);
}

void WapiSMS4Decryption(u8 *Key, u8 *IV, u8 *Input, u16 InputLength, 
                                                    u8 *Output, u16 *OutputLength)
{
	WapiSMS4Cryption(Key, IV, Input, InputLength, Output, OutputLength, ENCRYPT);
}

void WapiSMS4CalculateMic(u8 *Key, u8 *IV, u8 *Input1, u8 Input1Length,
                                                 u8 *Input2, u16 Input2Length, u8 *Output, u8 *OutputLength)
{
	u32 blockNum, i, remainder, rk[32];
	u8 BlockIn[16], BlockOut[16], TempBlock[16], tempIV[16], k;

	*OutputLength = 0;
	remainder = Input1Length & 0x0F;
	blockNum = Input1Length >> 4;

	for(k=0;k<16;k++)
		tempIV[k] = IV[15-k];

	memcpy(BlockIn, tempIV, 16);

	SMS4KeyExt((u8 *)Key, rk, ENCRYPT);

	SMS4Crypt((u8 *)BlockIn, BlockOut, rk);
      
	for(i=0; i<blockNum; i++){
		xor_block(BlockIn, (Input1+i*16), BlockOut);
		SMS4Crypt((u8 *)BlockIn, BlockOut, rk);
	}

	if(remainder !=0){
		memset(TempBlock, 0, 16);
		memcpy(TempBlock, (Input1+blockNum*16), remainder);

		xor_block(BlockIn, TempBlock, BlockOut);
		SMS4Crypt((u8 *)BlockIn, BlockOut, rk);	
      }

	remainder = Input2Length & 0x0F;
	blockNum = Input2Length >> 4;

  	for(i=0; i<blockNum; i++){
		xor_block(BlockIn, (Input2+i*16), BlockOut);
		SMS4Crypt((u8 *)BlockIn, BlockOut, rk);
	}

	if(remainder !=0){
		memset(TempBlock, 0, 16);
		memcpy(TempBlock, (Input2+blockNum*16), remainder);

		xor_block(BlockIn, TempBlock, BlockOut);
		SMS4Crypt((u8 *)BlockIn, BlockOut, rk);
	}
	
	memcpy(Output, BlockOut, 16);
	*OutputLength = 16;
}

void SecCalculateMicSMS4(
	u8		KeyIdx,
	u8        *MicKey, 
	u8        *pHeader,
	u8        *pData,
	u16       DataLen,
	u8        *MicBuffer
	)
{
	struct rtllib_hdr_3addrqos *header;
	u8 TempBuf[34], TempLen = 32, MicLen, QosOffset, *IV;
	u16 *pTemp, fc;

	header = (struct rtllib_hdr_3addrqos *)pHeader;
	memset(TempBuf, 0, 34);
	memcpy(TempBuf, pHeader, 2); 
	pTemp = (u16*)TempBuf;
	*pTemp &= 0xc78f;       

	memcpy((TempBuf+2), (pHeader+4), 12); 
	memcpy((TempBuf+14), (pHeader+22), 2); 
	pTemp = (u16*)(TempBuf + 14);
	*pTemp &= 0x000f;
    
	memcpy((TempBuf+16), (pHeader+16), 6); 

	fc = le16_to_cpu(header->frame_ctl);
	if((fc & (RTLLIB_FCTL_FROMDS | RTLLIB_FCTL_TODS)) == (RTLLIB_FCTL_FROMDS | RTLLIB_FCTL_TODS))
	{
		memcpy((TempBuf+22), (pHeader+24), 6);
		QosOffset = 30;
	}else{
		memset((TempBuf+22), 0, 6);
		QosOffset = 24;
	}

	if( IsQoSDataFrame(pHeader)){ 
		memcpy((TempBuf+28), (pHeader+QosOffset), 2);
		TempLen += 2;
		IV = pHeader + QosOffset + 2 + 2;
	}else{
		IV = pHeader + QosOffset + 2;
	}

	TempBuf[TempLen-1] = (u8)(DataLen & 0xff);
	TempBuf[TempLen-2] = (u8)((DataLen & 0xff00)>>8);
	TempBuf[TempLen-4] = KeyIdx;

	WAPI_DATA(WAPI_TX, "CalculateMic - KEY", MicKey, 16);
	WAPI_DATA(WAPI_TX, "CalculateMic - IV", IV, 16);
	WAPI_DATA(WAPI_TX, "CalculateMic - TempBuf", TempBuf, TempLen);
	WAPI_DATA(WAPI_TX, "CalculateMic - pData", pData, DataLen);

	WapiSMS4CalculateMic(MicKey, IV, TempBuf, TempLen,
	                                             pData, DataLen, MicBuffer, &MicLen);

	if (MicLen != 16)
		WAPI_TRACE(WAPI_ERR,"%s: MIC Length Error!!\n",__FUNCTION__);
}
	
int SecSMS4HeaderFillIV(struct rtllib_device *ieee, struct sk_buff *pskb)
{
	u8 *pSecHeader = NULL, *pos = NULL, *pRA = NULL;
	u8 bPNOverflow = false, bFindMatchPeer = false, hdr_len = 0;
	PWLAN_HEADER_WAPI_EXTENSION pWapiExt = NULL;
	PRT_WAPI_T         pWapiInfo = &ieee->wapiInfo; 
	PRT_WAPI_STA_INFO  pWapiSta = NULL; 
	int ret = 0;

	if ((!ieee->WapiSupport) || (!ieee->wapiInfo.bWapiEnable)){
		WAPI_TRACE(WAPI_ERR,"%s: ieee->WapiSupport = 0!!\n",__FUNCTION__);
		return -1;
	}

	hdr_len = sMacHdrLng;
	if(IsQoSDataFrame(pskb->data) ){
		hdr_len += sQoSCtlLng;
	}

	pos = skb_push(pskb, ieee->wapiInfo.extra_prefix_len);
	memmove(pos, pos+ieee->wapiInfo.extra_prefix_len, hdr_len);

	pSecHeader = pskb->data + hdr_len;
	pWapiExt = (PWLAN_HEADER_WAPI_EXTENSION)pSecHeader;
	pRA = pskb->data + 4;

	WAPI_DATA(WAPI_TX, "FillIV - Before Fill IV", pskb->data, pskb->len);

	if( is_multicast_ether_addr(pRA) ){ 
		if(!pWapiInfo->wapiTxMsk.bTxEnable){
			WAPI_TRACE(WAPI_ERR,"%s: bTxEnable = 0!!\n",__FUNCTION__);
			return -2;
		}
		if(pWapiInfo->wapiTxMsk.keyId <= 1){
			pWapiExt->KeyIdx = pWapiInfo->wapiTxMsk.keyId;
			pWapiExt->Reserved = 0;
			bPNOverflow = WapiIncreasePN(pWapiInfo->lastTxMulticastPN, 1);
			memcpy(pWapiExt->PN, pWapiInfo->lastTxMulticastPN, 16);
			if (bPNOverflow){
				WAPI_TRACE(WAPI_ERR,"===============>%s():multicast PN overflow\n",__FUNCTION__);
				WapiCreateAppEventAndSend(ieee,NULL,0,pRA, false, false, true, 0, false);
			}		          
		}else{
			WAPI_TRACE(WAPI_ERR,"%s: Invalid Wapi Multicast KeyIdx!!\n",__FUNCTION__);
			ret = -3;
		}
	}
    	else{ 
		list_for_each_entry(pWapiSta, &pWapiInfo->wapiSTAUsedList, list) {
			if(!memcmp(pWapiSta->PeerMacAddr,pRA,6)){
    				bFindMatchPeer = true;
				break;
			}
		}
		if (bFindMatchPeer){
			if((!pWapiSta->wapiUskUpdate.bTxEnable) && (!pWapiSta->wapiUsk.bTxEnable)){
				WAPI_TRACE(WAPI_ERR,"%s: bTxEnable = 0!!\n",__FUNCTION__);
				return -4;
			}
			if (pWapiSta->wapiUsk.keyId <= 1){
				if(pWapiSta->wapiUskUpdate.bTxEnable)
					pWapiExt->KeyIdx = pWapiSta->wapiUskUpdate.keyId;
				else
					pWapiExt->KeyIdx = pWapiSta->wapiUsk.keyId;

				pWapiExt->Reserved = 0;
				bPNOverflow = WapiIncreasePN(pWapiSta->lastTxUnicastPN, 2);
				memcpy(pWapiExt->PN, pWapiSta->lastTxUnicastPN, 16);
				if (bPNOverflow){
					WAPI_TRACE(WAPI_ERR,"===============>%s():unicast PN overflow\n",__FUNCTION__);
					WapiCreateAppEventAndSend(ieee,NULL,0,pWapiSta->PeerMacAddr, false, true, false, 0, false);
				}
			}else{
				WAPI_TRACE(WAPI_ERR,"%s: Invalid Wapi Unicast KeyIdx!!\n",__FUNCTION__);
				ret = -5;
			}
		}
		else{		
			WAPI_TRACE(WAPI_ERR,"%s: Can not find Peer Sta "MAC_FMT"!!\n",__FUNCTION__, MAC_ARG(pRA));
			ret = -6;
		}
    	}
		
	WAPI_DATA(WAPI_TX, "FillIV - After Fill IV", pskb->data, pskb->len);
	return ret;
}

void SecSWSMS4Encryption(
	struct rtllib_device *ieee,
	struct sk_buff *pskb
	)
{
	PRT_WAPI_T		pWapiInfo = &ieee->wapiInfo; 
	PRT_WAPI_STA_INFO   pWapiSta = NULL; 	
	u8 *SecPtr = NULL, *pRA, *pMicKey = NULL, *pDataKey = NULL, *pIV = NULL, *pHeader = pskb->data;	
	u8 IVOffset, DataOffset, bFindMatchPeer = false, KeyIdx = 0, MicBuffer[16];	
	u16 OutputLength;

#if defined(RTL8192U) || defined(RTL8192SU)
	u32 SpecificHeadOverhead = 0;
#ifdef USB_TX_DRIVER_AGGREGATION_ENABLE
	cb_desc *tcb_desc = (cb_desc *)(pskb->cb + MAX_DEV_ADDR_SIZE);
	if (tcb_desc->drv_agg_enable) 
		SpecificHeadOverhead = TX_PACKET_DRVAGGR_SUBFRAME_SHIFT_BYTES;
	else
#endif
	SpecificHeadOverhead = TX_PACKET_SHIFT_BYTES;

	pHeader += SpecificHeadOverhead;
#endif
	WAPI_TRACE(WAPI_TX, "=========>%s\n", __FUNCTION__);

	if( IsQoSDataFrame(pHeader) ){
		IVOffset = sMacHdrLng + sQoSCtlLng;
	}else{
		IVOffset = sMacHdrLng;
	}
	
	DataOffset = IVOffset + ieee->wapiInfo.extra_prefix_len;
	
	pRA = pHeader + 4;
	if( is_multicast_ether_addr(pRA) ){ 
		KeyIdx = pWapiInfo->wapiTxMsk.keyId;
		pIV = pWapiInfo->lastTxMulticastPN;
		pMicKey = pWapiInfo->wapiTxMsk.micKey;
		pDataKey = pWapiInfo->wapiTxMsk.dataKey;
	}else{ 
		if (!list_empty(&(pWapiInfo->wapiSTAUsedList))){
			list_for_each_entry(pWapiSta, &pWapiInfo->wapiSTAUsedList, list) {
				if (0 == memcmp(pWapiSta->PeerMacAddr, pRA, 6)){
					bFindMatchPeer = true;	    
					break;
				}
			}    
			
			if (bFindMatchPeer){
				if (pWapiSta->wapiUskUpdate.bTxEnable){
					KeyIdx = pWapiSta->wapiUskUpdate.keyId;
					WAPI_TRACE(WAPI_TX, "%s(): Use update USK!! KeyIdx=%d\n", __FUNCTION__, KeyIdx);
					pIV = pWapiSta->lastTxUnicastPN;
					pMicKey = pWapiSta->wapiUskUpdate.micKey;
					pDataKey = pWapiSta->wapiUskUpdate.dataKey;
				}else{
					KeyIdx = pWapiSta->wapiUsk.keyId;
					WAPI_TRACE(WAPI_TX, "%s(): Use USK!! KeyIdx=%d\n", __FUNCTION__, KeyIdx);
					pIV = pWapiSta->lastTxUnicastPN;
					pMicKey = pWapiSta->wapiUsk.micKey;
					pDataKey = pWapiSta->wapiUsk.dataKey;
				}
  			}else{
				WAPI_TRACE(WAPI_ERR,"%s: Can not find Peer Sta!!\n",__FUNCTION__);
				return;
			}
		}else{
			WAPI_TRACE(WAPI_ERR,"%s: wapiSTAUsedList is empty!!\n",__FUNCTION__);
			return;
		}
	}	

	SecPtr = pHeader;
	SecCalculateMicSMS4(KeyIdx, pMicKey, SecPtr, (SecPtr+DataOffset), pskb->len-DataOffset, MicBuffer);

	WAPI_DATA(WAPI_TX, "Encryption - MIC", MicBuffer, ieee->wapiInfo.extra_postfix_len);

	memcpy(skb_put(pskb,ieee->wapiInfo.extra_postfix_len),
			MicBuffer,
			ieee->wapiInfo.extra_postfix_len
			);
		
	WapiSMS4Encryption(pDataKey, pIV, (SecPtr+DataOffset), pskb->len-DataOffset, (SecPtr+DataOffset), &OutputLength);
	
	WAPI_DATA(WAPI_TX, "Encryption - After SMS4 encryption", pskb->data,pskb->len);

	if (OutputLength != pskb->len-DataOffset)
		WAPI_TRACE(WAPI_ERR,"%s: Output Length Error!!\n",__FUNCTION__);

	WAPI_TRACE(WAPI_TX, "<=========%s\n", __FUNCTION__);
}

u8 SecSWSMS4Decryption(
	struct rtllib_device *ieee,
	struct sk_buff *pskb,
	struct rtllib_rx_stats *rx_stats
	)
{
	PRT_WAPI_T pWapiInfo = &ieee->wapiInfo; 
	PRT_WAPI_STA_INFO   pWapiSta = NULL; 	
	u8 IVOffset, DataOffset, bFindMatchPeer = false, bUseUpdatedKey = false;
	u8 KeyIdx, MicBuffer[16], lastRxPNforQoS[16];
	u8 *pRA, *pTA, *pMicKey, *pDataKey, *pLastRxPN, *pRecvPN, *pSecData, *pRecvMic, *pos;
	u8 TID = 0;
	u16 OutputLength, DataLen;

	rx_stats->bWapiCheckPNInDecrypt = WapiCheckPnInSwDecrypt(ieee, pskb);
	WAPI_TRACE(WAPI_RX, "=========>%s: check PN  %d\n", __FUNCTION__,rx_stats->bWapiCheckPNInDecrypt);
	WAPI_DATA(WAPI_RX, "Decryption - Before decryption", pskb->data, pskb->len);

	IVOffset = sMacHdrLng;
	if( rx_stats->bIsQosData ){
		IVOffset += sQoSCtlLng;
	}
	if( rx_stats->bContainHTC )
		IVOffset += sHTCLng;
	
		
	DataOffset = IVOffset + ieee->wapiInfo.extra_prefix_len;

	pRA = pskb->data + 4;
	pTA = pskb->data + 10;
	KeyIdx = *(pskb->data + IVOffset);
	pRecvPN = pskb->data + IVOffset + 2;
	pSecData = pskb->data + DataOffset;
	DataLen = pskb->len - DataOffset;
	pRecvMic = pskb->data + pskb->len - ieee->wapiInfo.extra_postfix_len;
	TID = Frame_QoSTID(pskb->data);

	if (!list_empty(&(pWapiInfo->wapiSTAUsedList))){
		list_for_each_entry(pWapiSta, &pWapiInfo->wapiSTAUsedList, list) {
			if (0 == memcmp(pWapiSta->PeerMacAddr, pTA, 6)){
				bFindMatchPeer = true;	    
				break;
			}
		}    
	}	

	if (!bFindMatchPeer){
		WAPI_TRACE(WAPI_ERR, "%s: Can not find Peer Sta "MAC_FMT" for Key Info!!!\n", __FUNCTION__, MAC_ARG(pTA));
		return false;
	}

	if( is_multicast_ether_addr(pRA) ){
		WAPI_TRACE(WAPI_RX, "%s: Multicast decryption !!!\n", __FUNCTION__);
		if (pWapiSta->wapiMsk.keyId == KeyIdx && pWapiSta->wapiMsk.bSet){
			pLastRxPN = pWapiSta->lastRxMulticastPN;  
			if (!WapiComparePN(pRecvPN, pLastRxPN)){
				WAPI_TRACE(WAPI_ERR, "%s: MSK PN is not larger than last, Dropped!!!\n", __FUNCTION__);
				WAPI_DATA(WAPI_ERR, "pRecvPN:", pRecvPN, 16);
				WAPI_DATA(WAPI_ERR, "pLastRxPN:", pLastRxPN, 16);
				return false;
			}

			memcpy(pLastRxPN, pRecvPN, 16);
			pMicKey = pWapiSta->wapiMsk.micKey;
			pDataKey = pWapiSta->wapiMsk.dataKey;
		}else if (pWapiSta->wapiMskUpdate.keyId == KeyIdx && pWapiSta->wapiMskUpdate.bSet){
			WAPI_TRACE(WAPI_RX, "%s: Use Updated MSK for Decryption !!!\n", __FUNCTION__);
			bUseUpdatedKey = true;
			memcpy(pWapiSta->lastRxMulticastPN, pRecvPN, 16);
			pMicKey = pWapiSta->wapiMskUpdate.micKey;
			pDataKey = pWapiSta->wapiMskUpdate.dataKey;
		}else{
			WAPI_TRACE(WAPI_ERR, "%s: Can not find MSK with matched KeyIdx(%d), Dropped !!!\n", __FUNCTION__,KeyIdx);
			return false;
		}     
	}
	else{ 
		WAPI_TRACE(WAPI_RX, "%s: Unicast decryption !!!\n", __FUNCTION__);
		if (pWapiSta->wapiUsk.keyId == KeyIdx && pWapiSta->wapiUsk.bSet){
			WAPI_TRACE(WAPI_RX, "%s: Use USK for Decryption!!!\n", __FUNCTION__);
			if(rx_stats->bWapiCheckPNInDecrypt){
				if(rx_stats->bIsQosData){
					WapiGetLastRxUnicastPNForQoSData(TID, pWapiSta, lastRxPNforQoS);
					pLastRxPN = lastRxPNforQoS;
				}else{
					pLastRxPN = pWapiSta->lastRxUnicastPN;  
				}
				if (!WapiComparePN(pRecvPN, pLastRxPN)){
					return false;
				}
				if(rx_stats->bIsQosData){
					WapiSetLastRxUnicastPNForQoSData(TID, pRecvPN, pWapiSta);
				}else{
					memcpy(pWapiSta->lastRxUnicastPN, pRecvPN, 16);
				}
			}else{
				memcpy(rx_stats->WapiTempPN,pRecvPN,16);
			}

			if (ieee->iw_mode == IW_MODE_INFRA
#ifdef ASL
				|| ieee->iw_mode == IW_MODE_APSTA
#endif
				)
			{
				if ((pRecvPN[0] & 0x1) == 0){
					WAPI_TRACE(WAPI_ERR, "%s: Rx USK PN is not odd when Infra STA mode, Dropped !!!\n", __FUNCTION__);
					return false;
				}
			}
                   
			pMicKey = pWapiSta->wapiUsk.micKey;
			pDataKey = pWapiSta->wapiUsk.dataKey;
		}
		else if (pWapiSta->wapiUskUpdate.keyId == KeyIdx && pWapiSta->wapiUskUpdate.bSet ){
			WAPI_TRACE(WAPI_RX, "%s: Use Updated USK for Decryption!!!\n", __FUNCTION__);
			if(pWapiSta->bAuthenticatorInUpdata)
				bUseUpdatedKey = true;
			else
				bUseUpdatedKey = false;

			if(rx_stats->bIsQosData){
				WapiSetLastRxUnicastPNForQoSData(TID, pRecvPN, pWapiSta);
			}else{
				memcpy(pWapiSta->lastRxUnicastPN, pRecvPN, 16);
			}
			pMicKey = pWapiSta->wapiUskUpdate.micKey;
			pDataKey = pWapiSta->wapiUskUpdate.dataKey;
		}else{
			WAPI_TRACE(WAPI_ERR, "%s: No valid USK!!!KeyIdx=%d pWapiSta->wapiUsk.keyId=%d pWapiSta->wapiUskUpdate.keyId=%d\n", __FUNCTION__, KeyIdx, pWapiSta->wapiUsk.keyId, pWapiSta->wapiUskUpdate.keyId);
			dump_buf(pskb->data,pskb->len);
			return false;
		}     	
	}	

	WAPI_DATA(WAPI_RX, "Decryption - DataKey", pDataKey, 16);
	WAPI_DATA(WAPI_RX, "Decryption - IV", pRecvPN, 16);
	WapiSMS4Decryption(pDataKey, pRecvPN, pSecData, DataLen, pSecData, &OutputLength);

	if (OutputLength != DataLen)
		WAPI_TRACE(WAPI_ERR, "%s:  Output Length Error!!!!\n", __FUNCTION__);      

	WAPI_DATA(WAPI_RX, "Decryption - After decryption", pskb->data, pskb->len);

	DataLen -= ieee->wapiInfo.extra_postfix_len;     

	SecCalculateMicSMS4(KeyIdx, pMicKey, pskb->data, pSecData, DataLen, MicBuffer);

	WAPI_DATA(WAPI_RX, "Decryption - MIC received", pRecvMic, SMS4_MIC_LEN);
	WAPI_DATA(WAPI_RX, "Decryption - MIC calculated", MicBuffer, SMS4_MIC_LEN);

	if (0 == memcmp(MicBuffer, pRecvMic, ieee->wapiInfo.extra_postfix_len)){
		WAPI_TRACE(WAPI_RX, "%s: Check MIC OK!!\n", __FUNCTION__);
		if (bUseUpdatedKey){
			if ( is_multicast_ether_addr(pRA) ){
				WAPI_TRACE(WAPI_API, "%s(): AE use new update MSK!!\n", __FUNCTION__);
				pWapiSta->wapiMsk.keyId = pWapiSta->wapiMskUpdate.keyId;
				memcpy(pWapiSta->wapiMsk.dataKey, pWapiSta->wapiMskUpdate.dataKey, 16);  
				memcpy(pWapiSta->wapiMsk.micKey, pWapiSta->wapiMskUpdate.micKey, 16);  
				pWapiSta->wapiMskUpdate.bTxEnable = pWapiSta->wapiMskUpdate.bSet = false;
			}else{
				WAPI_TRACE(WAPI_API, "%s(): AE use new update USK!!\n", __FUNCTION__);
				pWapiSta->wapiUsk.keyId = pWapiSta->wapiUskUpdate.keyId;
				memcpy(pWapiSta->wapiUsk.dataKey, pWapiSta->wapiUskUpdate.dataKey, 16);  
				memcpy(pWapiSta->wapiUsk.micKey, pWapiSta->wapiUskUpdate.micKey, 16);  
				pWapiSta->wapiUskUpdate.bTxEnable = pWapiSta->wapiUskUpdate.bSet = false; 
			}       
		}
	}else{
		WAPI_TRACE(WAPI_ERR, "%s:  Check MIC Error, Dropped !!!!\n", __FUNCTION__);      
		return false;
	}

	pos = pskb->data;
	memmove(pos+ieee->wapiInfo.extra_prefix_len, pos, IVOffset);
	skb_pull(pskb, ieee->wapiInfo.extra_prefix_len);

	WAPI_TRACE(WAPI_RX, "<=========%s\n", __FUNCTION__);

	return true;
}

void wapi_test_set_key(struct rtllib_device *ieee, u8* buf)
{ /*Data: keyType(1) + bTxEnable(1) + bAuthenticator(1) + bUpdate(1) + PeerAddr(6) + DataKey(16) + MicKey(16) + KeyId(1)*/
	PRT_WAPI_T			pWapiInfo = &ieee->wapiInfo;
	PRT_WAPI_BKID		pWapiBkid;
	PRT_WAPI_STA_INFO	pWapiSta;
	u8					data[43];
	bool					bTxEnable;
	bool					bUpdate;
	bool					bAuthenticator;
	u8					PeerAddr[6];
	u8					WapiAEPNInitialValueSrc[16] = {0x37,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C} ;
	u8					WapiASUEPNInitialValueSrc[16] = {0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C} ;
	u8					WapiAEMultiCastPNInitialValueSrc[16] = {0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C} ;		

	WAPI_TRACE(WAPI_INIT, "===========>%s\n", __FUNCTION__);

	if (!ieee->WapiSupport){
	    return;
	}

	copy_from_user(data, buf, 43);
	bTxEnable = data[1];
	bAuthenticator = data[2];
	bUpdate = data[3];
 	memcpy(PeerAddr,data+4,6);
		
	if(data[0] == 0x3){
		if(!list_empty(&(pWapiInfo->wapiBKIDIdleList))){
			pWapiBkid = (PRT_WAPI_BKID)list_entry(pWapiInfo->wapiBKIDIdleList.next, RT_WAPI_BKID, list);
			list_del_init(&pWapiBkid->list);
			memcpy(pWapiBkid->bkid, data+10, 16);
			WAPI_DATA(WAPI_INIT, "SetKey - BKID", pWapiBkid->bkid, 16);
			list_add_tail(&pWapiBkid->list, &pWapiInfo->wapiBKIDStoreList);
		}
	}else{
		list_for_each_entry(pWapiSta, &pWapiInfo->wapiSTAUsedList, list) {
			if(!memcmp(pWapiSta->PeerMacAddr,PeerAddr,6)){
				pWapiSta->bAuthenticatorInUpdata = false;
				switch(data[0]){
				case 1:              
					if(bAuthenticator){         
						memcpy(pWapiSta->lastTxUnicastPN,WapiAEPNInitialValueSrc,16);
						if(!bUpdate) {     
							WAPI_TRACE(WAPI_INIT,"AE fisrt set usk \n");
							pWapiSta->wapiUsk.bSet = true;
							memcpy(pWapiSta->wapiUsk.dataKey,data+10,16);
							memcpy(pWapiSta->wapiUsk.micKey,data+26,16);
							pWapiSta->wapiUsk.keyId = *(data+42);
							pWapiSta->wapiUsk.bTxEnable = true;
							WAPI_DATA(WAPI_INIT, "SetKey - AE USK Data Key", pWapiSta->wapiUsk.dataKey, 16);
							WAPI_DATA(WAPI_INIT, "SetKey - AE USK Mic Key", pWapiSta->wapiUsk.micKey, 16);
						}
						else               
						{
							WAPI_TRACE(WAPI_INIT, "AE update usk \n");
							pWapiSta->wapiUskUpdate.bSet = true;
							pWapiSta->bAuthenticatorInUpdata = true;
							memcpy(pWapiSta->wapiUskUpdate.dataKey,data+10,16);
							memcpy(pWapiSta->wapiUskUpdate.micKey,data+26,16);
							memcpy(pWapiSta->lastRxUnicastPNBEQueue,WapiASUEPNInitialValueSrc,16);
							memcpy(pWapiSta->lastRxUnicastPNBKQueue,WapiASUEPNInitialValueSrc,16);
							memcpy(pWapiSta->lastRxUnicastPNVIQueue,WapiASUEPNInitialValueSrc,16);
							memcpy(pWapiSta->lastRxUnicastPNVOQueue,WapiASUEPNInitialValueSrc,16);
							memcpy(pWapiSta->lastRxUnicastPN,WapiASUEPNInitialValueSrc,16);
							pWapiSta->wapiUskUpdate.keyId = *(data+42);
							pWapiSta->wapiUskUpdate.bTxEnable = true;
						}
					}
					else{
						if(!bUpdate){
							WAPI_TRACE(WAPI_INIT,"ASUE fisrt set usk \n");
							if(bTxEnable){
								pWapiSta->wapiUsk.bTxEnable = true;
								memcpy(pWapiSta->lastTxUnicastPN,WapiASUEPNInitialValueSrc,16);
							}else{
								pWapiSta->wapiUsk.bSet = true;
								memcpy(pWapiSta->wapiUsk.dataKey,data+10,16);
								memcpy(pWapiSta->wapiUsk.micKey,data+26,16);
								pWapiSta->wapiUsk.keyId = *(data+42);
								pWapiSta->wapiUsk.bTxEnable = false;
							}
						}else{
							WAPI_TRACE(WAPI_INIT,"ASUE update usk \n");
							if(bTxEnable){
								pWapiSta->wapiUskUpdate.bTxEnable = true;
								if(pWapiSta->wapiUskUpdate.bSet){
									memcpy(pWapiSta->wapiUsk.dataKey,pWapiSta->wapiUskUpdate.dataKey,16);
									memcpy(pWapiSta->wapiUsk.micKey,pWapiSta->wapiUskUpdate.micKey,16);
									pWapiSta->wapiUsk.keyId=pWapiSta->wapiUskUpdate.keyId;
									memcpy(pWapiSta->lastRxUnicastPNBEQueue,WapiASUEPNInitialValueSrc,16);
									memcpy(pWapiSta->lastRxUnicastPNBKQueue,WapiASUEPNInitialValueSrc,16);
									memcpy(pWapiSta->lastRxUnicastPNVIQueue,WapiASUEPNInitialValueSrc,16);
									memcpy(pWapiSta->lastRxUnicastPNVOQueue,WapiASUEPNInitialValueSrc,16);
									memcpy(pWapiSta->lastRxUnicastPN,WapiASUEPNInitialValueSrc,16);
									pWapiSta->wapiUskUpdate.bTxEnable = false;
									pWapiSta->wapiUskUpdate.bSet = false;
								}
								memcpy(pWapiSta->lastTxUnicastPN,WapiASUEPNInitialValueSrc,16);
							}else{
								pWapiSta->wapiUskUpdate.bSet = true;
								memcpy(pWapiSta->wapiUskUpdate.dataKey,data+10,16);
								memcpy(pWapiSta->wapiUskUpdate.micKey,data+26,16);
								pWapiSta->wapiUskUpdate.keyId = *(data+42);
								pWapiSta->wapiUskUpdate.bTxEnable = false;
							}
						}
					}
					break;
				case 2:		
					if(bAuthenticator){          
						pWapiInfo->wapiTxMsk.bSet = true;
						memcpy(pWapiInfo->wapiTxMsk.dataKey,data+10,16);													
						memcpy(pWapiInfo->wapiTxMsk.micKey,data+26,16);
						pWapiInfo->wapiTxMsk.keyId = *(data+42);
						pWapiInfo->wapiTxMsk.bTxEnable = true;
						memcpy(pWapiInfo->lastTxMulticastPN,WapiAEMultiCastPNInitialValueSrc,16);

						if(!bUpdate){      
							WAPI_TRACE(WAPI_INIT, "AE fisrt set msk \n");
							if(!pWapiSta->bSetkeyOk)
								pWapiSta->bSetkeyOk = true;
							pWapiInfo->bFirstAuthentiateInProgress= false;
						}else{               
							WAPI_TRACE(WAPI_INIT,"AE update msk \n");
						}

						WAPI_DATA(WAPI_INIT, "SetKey - AE MSK Data Key", pWapiInfo->wapiTxMsk.dataKey, 16);
						WAPI_DATA(WAPI_INIT, "SetKey - AE MSK Mic Key", pWapiInfo->wapiTxMsk.micKey, 16);
					}
					else{
						if(!bUpdate){
							WAPI_TRACE(WAPI_INIT,"ASUE fisrt set msk \n");
							pWapiSta->wapiMsk.bSet = true;
							memcpy(pWapiSta->wapiMsk.dataKey,data+10,16);													
							memcpy(pWapiSta->wapiMsk.micKey,data+26,16);
							pWapiSta->wapiMsk.keyId = *(data+42);
							pWapiSta->wapiMsk.bTxEnable = false;
							if(!pWapiSta->bSetkeyOk)
								pWapiSta->bSetkeyOk = true;
							pWapiInfo->bFirstAuthentiateInProgress= false;
							WAPI_DATA(WAPI_INIT, "SetKey - ASUE MSK Data Key", pWapiSta->wapiMsk.dataKey, 16);
							WAPI_DATA(WAPI_INIT, "SetKey - ASUE MSK Mic Key", pWapiSta->wapiMsk.micKey, 16);
						}else{
							WAPI_TRACE(WAPI_INIT,"ASUE update msk \n");
							pWapiSta->wapiMskUpdate.bSet = true;
							memcpy(pWapiSta->wapiMskUpdate.dataKey,data+10,16);													
							memcpy(pWapiSta->wapiMskUpdate.micKey,data+26,16);
							pWapiSta->wapiMskUpdate.keyId = *(data+42);
							pWapiSta->wapiMskUpdate.bTxEnable = false;
						}
					}
					break;
				default:
					WAPI_TRACE(WAPI_ERR,"Unknown Flag \n");
					break;
				}
			}
		}
	}
	WAPI_TRACE(WAPI_INIT, "<===========%s\n", __FUNCTION__);
}

void wapi_test_init(struct rtllib_device *ieee)
{
	u8 keybuf[100];
	u8 mac_addr[6]={0x00,0xe0,0x4c,0x72,0x04,0x70};
	u8 UskDataKey[16]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
	u8 UskMicKey[16]={0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f};
	u8 UskId = 0;
	u8 MskDataKey[16]={0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f};
	u8 MskMicKey[16]={0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f};
	u8 MskId = 0;

	WAPI_TRACE(WAPI_INIT, "===========>%s\n", __FUNCTION__);

	WAPI_TRACE(WAPI_INIT, "%s: Enable wapi!!!!\n", __FUNCTION__);
	ieee->wapiInfo.bWapiEnable = true;
	ieee->pairwise_key_type = KEY_TYPE_SMS4;
	ieee->group_key_type = KEY_TYPE_SMS4;
	ieee->wapiInfo.extra_prefix_len = WAPI_EXT_LEN;
	ieee->wapiInfo.extra_postfix_len = SMS4_MIC_LEN;

	WAPI_TRACE(WAPI_INIT, "%s: Set USK!!!!\n", __FUNCTION__);
	memset(keybuf,0,100);
	keybuf[0] = 1;                           
	keybuf[1] = 1; 				
	keybuf[2] = 1; 				
	keybuf[3] = 0; 				
	
	memcpy(keybuf+4,mac_addr,6);
	memcpy(keybuf+10,UskDataKey,16);
	memcpy(keybuf+26,UskMicKey,16);
	keybuf[42]=UskId;
	wapi_test_set_key(ieee, keybuf);
	
	memset(keybuf,0,100);
	keybuf[0] = 1;                           
	keybuf[1] = 1; 				
	keybuf[2] = 0; 				
	keybuf[3] = 0; 				
	
	memcpy(keybuf+4,mac_addr,6);
	memcpy(keybuf+10,UskDataKey,16);
	memcpy(keybuf+26,UskMicKey,16);
	keybuf[42]=UskId;
	wapi_test_set_key(ieee, keybuf);
	
	WAPI_TRACE(WAPI_INIT, "%s: Set MSK!!!!\n", __FUNCTION__);
	memset(keybuf,0,100);
	keybuf[0] = 2;                                
	keybuf[1] = 1;                               
	keybuf[2] = 1; 				
	keybuf[3] = 0;                              
	memcpy(keybuf+4,mac_addr,6);
	memcpy(keybuf+10,MskDataKey,16);
	memcpy(keybuf+26,MskMicKey,16);
	keybuf[42] = MskId;
	wapi_test_set_key(ieee, keybuf);

	memset(keybuf,0,100);
	keybuf[0] = 2;                                
	keybuf[1] = 1;                               
	keybuf[2] = 0; 				
	keybuf[3] = 0;                              
	memcpy(keybuf+4,mac_addr,6);
	memcpy(keybuf+10,MskDataKey,16);
	memcpy(keybuf+26,MskMicKey,16);
	keybuf[42] = MskId;
	wapi_test_set_key(ieee, keybuf);
	WAPI_TRACE(WAPI_INIT, "<===========%s\n", __FUNCTION__);
}

#endif
