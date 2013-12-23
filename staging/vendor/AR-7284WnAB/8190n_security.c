/*
 *      Handle routines to communicate with AUTH daemon (802.1x authenticator)
 *
 *      $Id: 8190n_security.c,v 1.8 2009/08/11 11:19:44 yachang Exp $
 */

#define _8190N_SECURITY_C_
#ifdef __KERNEL__
#include <asm/uaccess.h>
#include <linux/module.h>
#endif

#include "./8190n_cfg.h"

#ifndef __KERNEL__
#include "./sys-support.h"
#endif

#include "./8190n.h"
#include "./8190n_security.h"
#include "./8190n_headers.h"
#include "./8190n_debug.h"

#define E_MSG_DOT11_2LARGE      "ItemSize Too Large"
#define E_MSG_DOT11_QFULL       "Event Queue Full"
#define E_MSG_DOT11_QEMPTY      "Event Queue Empty"


void DOT11_InitQueue(DOT11_QUEUE * q)
{
	q->Head = 0;
	q->Tail = 0;
	q->NumItem = 0;
	q->MaxItem = MAXQUEUESIZE;
}

#ifndef WITHOUT_ENQUEUE
/*--------------------------------------------------------------------------------
 Return Value
	 Success : return 0;
	 Fail	 : return E_DOT11_QFULL if queue is full
	           return E_DOT1ZX_2LARGE if the item size is large than allocated
--------------------------------------------------------------------------------*/
int DOT11_EnQueue(unsigned long task_priv, DOT11_QUEUE *q, unsigned char *item, int itemsize)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)task_priv;
	unsigned long flags;

	if(DOT11_IsFullQueue(q))
		return E_DOT11_QFULL;
	if(itemsize > MAXDATALEN)
		return E_DOT11_2LARGE;

	SAVE_INT_AND_CLI(flags);
	q->ItemArray[q->Tail].ItemSize = itemsize;
	memset(q->ItemArray[q->Tail].Item, 0, sizeof(q->ItemArray[q->Tail].Item));
	memcpy(q->ItemArray[q->Tail].Item, item, itemsize);
	q->NumItem++;
	if((q->Tail+1) == MAXQUEUESIZE)
		q->Tail = 0;
	else
		q->Tail++;

	RESTORE_INT(flags);
	return 0;
}


int DOT11_DeQueue(unsigned long task_priv, DOT11_QUEUE *q, unsigned char *item, int *itemsize)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)task_priv;
	unsigned long flags;

	if(DOT11_IsEmptyQueue(q))
		return E_DOT11_QEMPTY;

	SAVE_INT_AND_CLI(flags);
	memcpy(item, q->ItemArray[q->Head].Item, q->ItemArray[q->Head].ItemSize);
	*itemsize = q->ItemArray[q->Head].ItemSize;
	q->NumItem--;
	if((q->Head+1) == MAXQUEUESIZE)
		q->Head = 0;
	else
		q->Head++;

	RESTORE_INT(flags);
	return 0;
}
#endif // !WITHOUT_ENQUEUE


#if 0
void DOT11_PrintQueue(DOT11_QUEUE *q)
{
	int i, j, index;

	printk("\n/-------------------------------------------------\n[DOT11_PrintQueue]: MaxItem = %d, NumItem = %d, Head = %d, Tail = %d\n", q->MaxItem, q->NumItem, q->Head, q->Tail);
	for(i=0; i<q->NumItem; i++) {
		index = (i + q->Head) % q->MaxItem;
		printk("Queue[%d].ItemSize = %d  ", index, q->ItemArray[index].ItemSize);
		for(j=0; j<q->ItemArray[index].ItemSize; j++)
			printk(" %x", q->ItemArray[index].Item[j]);
		printk("\n");
	}
	printk("------------------------------------------------/\n");
}


char *DOT11_ErrMsgQueue(int err)
{
	switch(err)
	{
	case E_DOT11_2LARGE:
		return E_MSG_DOT11_2LARGE;
	case E_DOT11_QFULL:
		return E_MSG_DOT11_QFULL;
	case E_DOT11_QEMPTY:
		return E_MSG_DOT11_QEMPTY;
	default:
		return "E_MSG_DOT11_QNOERR";
	}
}


void DOT11_Dump(char *fun, UINT8 *buf, int size, char *comment)
{
	int i;

	printk("$$ %s $$: %s", fun, comment);
	if (buf != NULL) {
		printk("\nMessage is %d bytes %x hex", size, size);
		for (i = 0; i < size; i++) {
			if (i % 16 == 0) printk("\n\t");
			printk("%2x ", *(buf+i));
		}
	}
	printk("\n");
}
#endif


/*------------------------------------------------------------
 Set RSNIE is to set Information Element of AP
 Return Value
        Success : 0
------------------------------------------------------------*/
static int DOT11_Process_Set_RSNIE(struct net_device *dev, struct iw_point *data)
{
	struct Dot11RsnIE	*pdot11RsnIE;
	struct wifi_mib		*pmib;
    struct rtl8190_priv	*priv = (struct rtl8190_priv *)(dev->priv);
	DOT11_SET_RSNIE		*Set_RSNIE = (DOT11_SET_RSNIE *)data->pointer;
	//unsigned long		ioaddr = priv->pshare->ioaddr;

	DEBUG_INFO("going to set rsnie\n");
	pmib = priv->pmib;
	pdot11RsnIE = &pmib->dot11RsnIE;
	if(Set_RSNIE->Flag == DOT11_Ioctl_Set)
	{
		pdot11RsnIE->rsnielen = Set_RSNIE->RSNIELen;
		memcpy((void *)pdot11RsnIE->rsnie, Set_RSNIE->RSNIE, pdot11RsnIE->rsnielen);
		priv->pmib->dot118021xAuthEntry.dot118021xAlgrthm = 1;
		DEBUG_INFO("DOT11_Process_Set_RSNIE rsnielen=%d\n", pdot11RsnIE->rsnielen);

		// see whether if driver is open. If not, do not enable tx/rx, david
		if (!netif_running(priv->dev))
			return 0;

		// Alway enable tx/rx in rtl8190_init_hw_PCI()
		//RTL_W8(_CR_, BIT(2) | BIT(3));
#ifdef INCLUDE_WPA_PSK
		if (!priv->pmib->dot1180211AuthEntry.dot11EnablePSK)
#endif
		{
			if (OPMODE & WIFI_AP_STATE) {
#ifdef MBSSID
				if (IS_ROOT_INTERFACE(priv))
#endif
				{
					if (priv->auto_channel) {
						priv->ss_ssidlen = 0;
						DEBUG_INFO("start_clnt_ss, trigger by %s, ss_ssidlen=0\n", (char *)__FUNCTION__);
						start_clnt_ss(priv);
					}
				}
			}
#ifdef CLIENT_MODE
			if (OPMODE & (WIFI_STATION_STATE | WIFI_ADHOC_STATE))
				start_clnt_lookup(priv, 1);
#endif
		}
	}
	else if(Set_RSNIE->Flag == DOT11_Ioctl_Query)
	{
	}
	return 0;
}


#if 0
char *DOT11_Parse_RSNIE_Err(int err)
{
	switch(err)
	{
	case ERROR_BUFFER_TOO_SMALL:
		return RSN_STRERROR_BUFFER_TOO_SMALL;
	case ERROR_INVALID_PARA:
		return RSN_STRERROR_INVALID_PARAMETER;
	case ERROR_INVALID_RSNIE:
		return RSN_STRERROR_INVALID_RSNIE;
	case ERROR_INVALID_MULTICASTCIPHER:
		return RSN_STRERROR_INVALID_MULTICASTCIPHER;
	case ERROR_INVALID_UNICASTCIPHER:
		return RSN_STRERROR_INVALID_UNICASTCIPHER;
	case ERROR_INVALID_AUTHKEYMANAGE:
		return RSN_STRERROR_INVALID_AUTHKEYMANAGE;
	case ERROR_UNSUPPORTED_RSNEVERSION:
		return RSN_STRERROR_UNSUPPORTED_RSNEVERSION;
	case ERROR_INVALID_CAPABILITIES:
		return RSN_STRERROR_INVALID_CAPABILITIES;
	default:
		return "Uknown Failure";
	}
}
#endif


/*-------------------------------------------------
 Send association response to STA
 Return Value
        Success : 0
---------------------------------------------------*/
static int DOT11_Process_Association_Rsp(struct net_device *dev, struct iw_point *data, int frame_type)
{
	unsigned long		flags;
	struct stat_info	*pstat;
	unsigned char		*hwaddr;
	struct rtl8190_priv	*priv = (struct rtl8190_priv *)dev->priv;
	DOT11_ASSOCIATIIN_RSP	*Assoc_Rsp = (DOT11_ASSOCIATIIN_RSP *)data->pointer;

	pstat = get_stainfo(priv, (UINT8 *)Assoc_Rsp->MACAddr);

	if (pstat == NULL)
		return (-1);

	hwaddr = (unsigned char *)Assoc_Rsp->MACAddr;
	DEBUG_INFO("802.1x issue assoc_rsp sta:%02X%02X%02X%02X%02X%02X status:%d\n",
		hwaddr[0],hwaddr[1],hwaddr[2],hwaddr[3],hwaddr[4],hwaddr[5], Assoc_Rsp->Status);

	issue_asocrsp(priv, Assoc_Rsp->Status, pstat, frame_type);

	if (Assoc_Rsp->Status != 0)
	{
		if (!list_empty(&pstat->asoc_list))
		{
			list_del_init(&pstat->asoc_list);
			if (pstat->expire_to > 0)
			{
				SAVE_INT_AND_CLI(flags);
				cnt_assoc_num(priv, pstat, DECREASE, (char *)__FUNCTION__);
				check_sta_characteristic(priv, pstat, DECREASE);
				RESTORE_INT(flags);

				LOG_MSG("A STA is rejected by 802.1x daemon - %02X:%02X:%02X:%02X:%02X:%02X\n",
					pstat->hwaddr[0],pstat->hwaddr[1],pstat->hwaddr[2],pstat->hwaddr[3],pstat->hwaddr[4],pstat->hwaddr[5]);
			}
		}

		free_stainfo(priv, pstat);
	}
	else
		update_fwtbl_asoclst(priv, pstat);

	return 0;
}


/*-------------------------------------------------
 Send Disassociation request to STA
 Return Value
        Success : 0
---------------------------------------------------*/
static int DOT11_Process_Disconnect_Req(struct net_device *dev, struct iw_point *data)
{
	unsigned long	flags;
	struct stat_info	*pstat;
	unsigned char	*hwaddr;
	struct rtl8190_priv	*priv = (struct rtl8190_priv *)dev->priv;
	DOT11_DISCONNECT_REQ	*Disconnect_Req = (DOT11_DISCONNECT_REQ *)data->pointer;

	pstat = get_stainfo(priv, (UINT8 *)Disconnect_Req->MACAddr);

	if (pstat == NULL)
		return (-1);

	hwaddr = (unsigned char *)Disconnect_Req->MACAddr;
	DEBUG_INFO("802.1x issue disassoc sta:%02X%02X%02X%02X%02X%02X reason:%d\n",
		hwaddr[0],hwaddr[1],hwaddr[2],hwaddr[3],hwaddr[4],hwaddr[5], Disconnect_Req->Reason);

	issue_disassoc(priv, pstat->hwaddr, Disconnect_Req->Reason);

	if (!list_empty(&pstat->asoc_list))
	{
		list_del_init(&pstat->asoc_list);
		if (pstat->expire_to > 0)
		{
			SAVE_INT_AND_CLI(flags);
			cnt_assoc_num(priv, pstat, DECREASE, (char *)__FUNCTION__);
			check_sta_characteristic(priv, pstat, DECREASE);
			RESTORE_INT(flags);

			LOG_MSG("A STA is rejected by 802.1x daemon - %02X:%02X:%02X:%02X:%02X:%02X\n",
				pstat->hwaddr[0],pstat->hwaddr[1],pstat->hwaddr[2],pstat->hwaddr[3],pstat->hwaddr[4],pstat->hwaddr[5]);
		}
	}

	free_stainfo(priv, pstat);

	return 0;
}


/*---------------------------------------------------------------------
 Delete Group Key for AP and Pairwise Key for specific STA
 Return Value
        Success : 0
-----------------------------------------------------------------------*/
static int DOT11_Process_Delete_Key(struct net_device *dev, struct iw_point *data)
{
	struct stat_info	*pstat = NULL;
	struct rtl8190_priv	*priv = (struct rtl8190_priv *)dev->priv;
	DOT11_DELETE_KEY	*Delete_Key = (DOT11_DELETE_KEY *)data->pointer;
	struct wifi_mib 	*pmib = priv->pmib;

	unsigned char MULTICAST_ADD[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

	if (Delete_Key->KeyType == DOT11_KeyType_Group)
	{
		pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKeyLen = 0;
		pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TMicKeyLen = 0;

		DEBUG_INFO("Delete Group Key\n");
		if (CamDeleteOneEntry(priv, MULTICAST_ADD, 1, 0))
			priv->pshare->CamEntryOccupied--;
	}
	else if (Delete_Key->KeyType == DOT11_KeyType_Pairwise)
	{
		pstat = get_stainfo(priv, (UINT8 *)Delete_Key->MACAddr);
		if (pstat == NULL)
			return (-1);

		pstat->dot11KeyMapping.dot11EncryptKey.dot11TTKeyLen = 0;
		pstat->dot11KeyMapping.dot11EncryptKey.dot11TMicKeyLen = 0;

		DEBUG_INFO("Delete Unicast Key\n");
		if (pstat->dot11KeyMapping.keyInCam == TRUE) {
			if (CamDeleteOneEntry(priv, (unsigned char *)Delete_Key->MACAddr, 0, 0)) {
				priv->pshare->CamEntryOccupied--;
				if (pstat)	pstat->dot11KeyMapping.keyInCam = FALSE;
			}
		}
	}

	return 0;
}


static __inline__ void set_ttkeylen(struct Dot11EncryptKey *pEncryptKey, UINT8 len)
{
	pEncryptKey->dot11TTKeyLen = len;
}


static __inline__ void set_tmickeylen(struct Dot11EncryptKey *pEncryptKey, UINT8 len)
{
	pEncryptKey->dot11TMicKeyLen = len;
}


static __inline__ void set_tkip_key(struct Dot11EncryptKey *pEncryptKey, UINT8 *src)
{
	memcpy(pEncryptKey->dot11TTKey.skey, src, pEncryptKey->dot11TTKeyLen);

	memcpy(pEncryptKey->dot11TMicKey1.skey, src + 16, pEncryptKey->dot11TMicKeyLen);

	memcpy(pEncryptKey->dot11TMicKey2.skey, src + 24, pEncryptKey->dot11TMicKeyLen);

	pEncryptKey->dot11TXPN48.val48 = 0;
}


static __inline__ void set_aes_key(struct Dot11EncryptKey *pEncryptKey, UINT8 *src)
{
	memcpy(pEncryptKey->dot11TTKey.skey, src, pEncryptKey->dot11TTKeyLen);

	memcpy(pEncryptKey->dot11TMicKey1.skey, src, pEncryptKey->dot11TMicKeyLen);
}


static __inline__ void set_wep40_key(struct Dot11EncryptKey *pEncryptKey, UINT8 *src)
{
	memcpy(pEncryptKey->dot11TTKey.skey, src, pEncryptKey->dot11TTKeyLen);
}


static __inline__ void set_wep104_key(struct Dot11EncryptKey *pEncryptKey, UINT8 *src)
{
	memcpy(pEncryptKey->dot11TTKey.skey, src, pEncryptKey->dot11TTKeyLen);
}


static __inline__ void clean_pn(unsigned char *pn)
{
	memset(pn, 0, 6);
}


/*---------------------------------------------------------------------
 Set Group Key for AP and Pairwise Key for specific STA
 Return Value
        Success : 0
-----------------------------------------------------------------------*/
int DOT11_Process_Set_Key(struct net_device *dev, struct iw_point *data,
	DOT11_SET_KEY *pSetKey, unsigned char *pKey)
{
	struct rtl8190_priv	*priv = (struct rtl8190_priv *)dev->priv;
	struct wifi_mib		*pmib = priv->pmib;
	struct Dot11EncryptKey	*pEncryptKey = NULL;
	struct stat_info	*pstat = NULL;
	DOT11_SET_KEY	Set_Key;
	unsigned char	key_combo[32];
	unsigned char MULTICAST_ADD[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	int	retVal;

	if (data) {
#ifdef INCLUDE_WPA_PSK
		memcpy((void *)&Set_Key, (void *)data->pointer, sizeof(DOT11_SET_KEY));
		memcpy((void *)key_combo,
				   ((DOT11_SET_KEY *)data->pointer)->KeyMaterial, 32);
#else
		if (copy_from_user((void *)&Set_Key, (void *)data->pointer, sizeof(DOT11_SET_KEY))) {
			DEBUG_ERR("copy_from_user error!\n");
			return -1;
		}
		if (copy_from_user((void *)key_combo,
				   ((DOT11_SET_KEY *)data->pointer)->KeyMaterial, 32)) {
			DEBUG_ERR("copy_from_user error!\n");
			return -1;
		}
#endif
	}
	else {
		Set_Key = *pSetKey;
		memcpy(key_combo, pKey, 32);
	}

#ifdef WDS
	if (priv->pmib->dot11WdsInfo.wdsEnabled && priv->pmib->dot11WdsInfo.wdsNum &&
		((priv->pmib->dot11WdsInfo.wdsPrivacy == _TKIP_PRIVACY_) ||
			(priv->pmib->dot11WdsInfo.wdsPrivacy == _CCMP_PRIVACY_)) &&
				(Set_Key.KeyType == DOT11_KeyType_Group) &&
					!memcmp(Set_Key.MACAddr, "\x0\x0\x0\x0\x0\x0", 6) ) {
			int i;
			for (i=0; i<priv->pmib->dot11WdsInfo.wdsNum; i++) {
				memcpy(Set_Key.MACAddr, priv->pmib->dot11WdsInfo.entry[i].macAddr, 6);
				Set_Key.KeyType = DOT11_KeyType_Pairwise;
				Set_Key.EncType = priv->pmib->dot11WdsInfo.wdsPrivacy;
				Set_Key.KeyIndex = 0;
				DOT11_Process_Set_Key(priv->dev, NULL, &Set_Key, key_combo);
			}
			return 0;
	}
#endif

	if(Set_Key.KeyType == DOT11_KeyType_Group)
	{
		int set_gkey_to_cam;

		if (priv->pshare->VersionID == VERSION_8190_C)
			set_gkey_to_cam = 1;
		else {
			// 8190 B-cut bug, can't use h/w cam to encrypt multicast packet
			set_gkey_to_cam = 0;
		}

#ifdef UNIVERSAL_REPEATER
		if (IS_VXD_INTERFACE(priv))
			set_gkey_to_cam = 0;
		else {
			if (IS_ROOT_INTERFACE(priv)) {
				if (IS_DRV_OPEN(GET_VXD_PRIV(priv)))
					set_gkey_to_cam = 0;
			}
		}
#endif
#ifdef MBSSID
#if defined(RTL8192SE) || defined(RTL8192SU)
		if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
#endif
		{
			if (IS_VAP_INTERFACE(priv))
				set_gkey_to_cam = 0;
			else {
				if (IS_ROOT_INTERFACE(priv)) {
					int i;
					for (i=0; i<RTL8190_NUM_VWLAN; i++) {
						if (IS_DRV_OPEN(priv->pvap_priv[i])) {
							set_gkey_to_cam = 0;
							break;
						}
					}
				}
			}
		}
#endif

#ifdef CONFIG_RTK_MESH
		//modify by Joule for SECURITY
		if(  dev == priv->mesh_dev )
		{
			pmib->dot11sKeysTable.dot11Privacy = Set_Key.EncType;
			pEncryptKey = &pmib->dot11sKeysTable.dot11EncryptKey;
			pmib->dot11sKeysTable.keyid = (UINT)Set_Key.KeyIndex;
		}
		else
#endif
		{
			pmib->dot11GroupKeysTable.dot11Privacy = Set_Key.EncType;
			pEncryptKey = &pmib->dot11GroupKeysTable.dot11EncryptKey;
			pmib->dot11GroupKeysTable.keyid = (UINT)Set_Key.KeyIndex;
		}


		switch(Set_Key.EncType)
		{
		case DOT11_ENC_TKIP:
			set_ttkeylen(pEncryptKey, 16);
			set_tmickeylen(pEncryptKey, 8);
			set_tkip_key(pEncryptKey, key_combo);

			DEBUG_INFO("going to set TKIP group key! id %X\n", (UINT)Set_Key.KeyIndex);
			if (!SWCRYPTO) {
//				if (OPMODE & WIFI_ADHOC_STATE) {
//					init_DefaultKey_Enc(priv, key_combo, DOT11_ENC_TKIP);
//					pmib->dot11GroupKeysTable.keyInCam = TRUE;
//				}
//				else {
					if (set_gkey_to_cam)
					{
					    retVal = CamDeleteOneEntry(priv, MULTICAST_ADD, Set_Key.KeyIndex, 0);
					    if (retVal) {
						    priv->pshare->CamEntryOccupied--;
						    pmib->dot11GroupKeysTable.keyInCam = FALSE;
					    }
					    retVal = CamAddOneEntry(priv, MULTICAST_ADD, Set_Key.KeyIndex, DOT11_ENC_TKIP<<2, 0, key_combo);
					    if (retVal) {
						    priv->pshare->CamEntryOccupied++;
						    pmib->dot11GroupKeysTable.keyInCam = TRUE;
					    }
//				}
					}
//				}
			}
			break;

		case DOT11_ENC_WEP40:
			set_ttkeylen(pEncryptKey, 5);
			set_tmickeylen(pEncryptKey, 0);
			set_wep40_key(pEncryptKey, key_combo);

			DEBUG_INFO("going to set WEP40 group key!\n");
			if (!SWCRYPTO) {
				if (set_gkey_to_cam)
				{
					retVal = CamDeleteOneEntry(priv, MULTICAST_ADD, Set_Key.KeyIndex, 0);
					if (retVal)	{
						priv->pshare->CamEntryOccupied--;
						pmib->dot11GroupKeysTable.keyInCam = FALSE;
					}
					retVal = CamAddOneEntry(priv, MULTICAST_ADD, Set_Key.KeyIndex, DOT11_ENC_WEP40<<2, 0, key_combo);
					if (retVal)	{
						priv->pshare->CamEntryOccupied++;
						pmib->dot11GroupKeysTable.keyInCam = TRUE;
					}
				}
			}
			break;

		case DOT11_ENC_WEP104:
			set_ttkeylen(pEncryptKey, 13);
			set_tmickeylen(pEncryptKey, 0);
			set_wep104_key(pEncryptKey, key_combo);

			DEBUG_INFO("going to set WEP104 group key!\n");
			if (!SWCRYPTO) {
				if (set_gkey_to_cam)
				{
					retVal = CamDeleteOneEntry(priv, MULTICAST_ADD, Set_Key.KeyIndex, 0);
					if (retVal)	{
						priv->pshare->CamEntryOccupied--;
						pmib->dot11GroupKeysTable.keyInCam = FALSE;
					}
					retVal = CamAddOneEntry(priv, MULTICAST_ADD, Set_Key.KeyIndex, DOT11_ENC_WEP104<<2, 0, key_combo);
					if (retVal)	{
						priv->pshare->CamEntryOccupied++;
						pmib->dot11GroupKeysTable.keyInCam = TRUE;
					}
				}
			}
			break;

		case DOT11_ENC_CCMP:
			set_ttkeylen(pEncryptKey, 16);
			set_tmickeylen(pEncryptKey, 16);
			set_aes_key(pEncryptKey, key_combo);

			DEBUG_INFO("going to set CCMP-AES group key!\n");
			if (!SWCRYPTO) {
#ifdef CONFIG_RTK_MESH
//modify by Joule for SECURITY
				if(  dev == priv->mesh_dev )
			 	{
// KEY_MAP_KEY_PATCH_0223
//					init_DefaultKey_Enc(priv, key_combo, DOT11_ENC_CCMP);
// KEY_MAP_KEY_PATCH_0223
					pmib->dot11sKeysTable.keyInCam = TRUE;		 	
			 	}
				else 
#endif						
			 	{
//				if (OPMODE & WIFI_ADHOC_STATE) {
//					init_DefaultKey_Enc(priv, key_combo, DOT11_ENC_CCMP);
//					pmib->dot11GroupKeysTable.keyInCam = TRUE;
//				}
//				else {
					if (set_gkey_to_cam)
					{
						retVal = CamDeleteOneEntry(priv, MULTICAST_ADD, Set_Key.KeyIndex, 0);
						if (retVal) {
							priv->pshare->CamEntryOccupied--;
							pmib->dot11GroupKeysTable.keyInCam = FALSE;
						}
						retVal = CamAddOneEntry(priv, MULTICAST_ADD, Set_Key.KeyIndex, DOT11_ENC_CCMP<<2, 0, key_combo);
						if (retVal) {
							priv->pshare->CamEntryOccupied++;
							pmib->dot11GroupKeysTable.keyInCam = TRUE;
						}
					}	
				}//end of if(  dev == priv->mesh_dev )
			}
			break;

		case DOT11_ENC_NONE:
		default:
			DEBUG_ERR("No group encryption key is set!\n");
			set_ttkeylen(pEncryptKey, 0);
			set_tmickeylen(pEncryptKey, 0);
			break;
		}
	}

	else if(Set_Key.KeyType == DOT11_KeyType_Pairwise)
	{
#ifdef WDS
// always to store WDS key for tx-hangup re-init
#if 0
		// if driver is not opened, save the WDS key into mib. The key value will
		// be updated into CAM when driver opened. david
		if ( !IS_DRV_OPEN(priv)) { // not open or not in open
			// if interface is not open and STA is a WDS entry, save the key
			// to mib and restore it to STA table when driver is open, david
			struct net_device *pNet = getWdsDevByAddr(priv, Set_Key.MACAddr);
			if (pNet) {
				int widx = getWdsIdxByDev(priv, pNet);
				if (widx >= 0) {
					if (Set_Key.EncType == _WEP_40_PRIVACY_ ||
						Set_Key.EncType == _WEP_104_PRIVACY_)
						memcpy(pmib->dot11WdsInfo.wdsWepKey, key_combo, 32);
					else {
						memcpy(pmib->dot11WdsInfo.wdsMapingKey[widx], key_combo, 32);
						pmib->dot11WdsInfo.wdsMappingKeyLen[widx] = 32;
					}

					pmib->dot11WdsInfo.wdsKeyId = Set_Key.KeyIndex;
					pmib->dot11WdsInfo.wdsPrivacy = Set_Key.EncType;

					return 0;
				}
			}
			return -1;
		}
#endif
		int widx = -1;
		struct net_device *pNet = getWdsDevByAddr(priv, Set_Key.MACAddr);
		if (pNet) {
			widx = getWdsIdxByDev(priv, pNet);
			if (widx >= 0) {
				if (Set_Key.EncType == _WEP_40_PRIVACY_ ||
					Set_Key.EncType == _WEP_104_PRIVACY_)
					memcpy(pmib->dot11WdsInfo.wdsWepKey, key_combo, 32);
				else {
					memcpy(pmib->dot11WdsInfo.wdsMapingKey[widx], key_combo, 32);
					pmib->dot11WdsInfo.wdsMappingKeyLen[widx] = 32;
				}
				pmib->dot11WdsInfo.wdsKeyId = Set_Key.KeyIndex;
				pmib->dot11WdsInfo.wdsPrivacy = Set_Key.EncType;
			}
		}
		if ( !IS_DRV_OPEN(priv)) {
			if (widx > 0 && pmib->dot11WdsInfo.wdsMappingKeyLen[widx] > 0)
				pmib->dot11WdsInfo.wdsMappingKeyLen[widx] |= 0x80000000;
			return 0;
		}
//----------------------------------- david+2006-06-30
#endif // WDS
		pstat = get_stainfo(priv, Set_Key.MACAddr);
		if (pstat == NULL) {
			DEBUG_ERR("Set key failed, invalid mac address: %02x%02x%02x%02x%02x%02x\n",
				Set_Key.MACAddr[0], Set_Key.MACAddr[1], Set_Key.MACAddr[2], Set_Key.MACAddr[3],
				Set_Key.MACAddr[4], Set_Key.MACAddr[5]);
			return (-1);
		}

		pstat->dot11KeyMapping.dot11Privacy = Set_Key.EncType;
		pEncryptKey = &pstat->dot11KeyMapping.dot11EncryptKey;
		pstat->keyid = Set_Key.KeyIndex;

#if defined(__DRAYTEK_OS__) && defined(WDS)
		if (pstat->state & WIFI_WDS)
			priv->pmib->dot11WdsInfo.wdsPrivacy = Set_Key.EncType;
#endif

		switch(Set_Key.EncType)
		{
		case DOT11_ENC_TKIP:
			set_ttkeylen(pEncryptKey, 16);
			set_tmickeylen(pEncryptKey, 8);
			set_tkip_key(pEncryptKey, key_combo);

			DEBUG_INFO("going to set TKIP Unicast key for sta %02X%02X%02X%02X%02X%02X, id=%d\n",
				Set_Key.MACAddr[0], Set_Key.MACAddr[1], Set_Key.MACAddr[2],
				Set_Key.MACAddr[3], Set_Key.MACAddr[4], Set_Key.MACAddr[5], pstat->keyid);
			if (!SWCRYPTO) {
				retVal = CamDeleteOneEntry(priv, Set_Key.MACAddr, Set_Key.KeyIndex, 0);
				if (retVal) {
					priv->pshare->CamEntryOccupied--;
					if (pstat)	pstat->dot11KeyMapping.keyInCam = FALSE;
				}
				retVal = CamAddOneEntry(priv, Set_Key.MACAddr, Set_Key.KeyIndex, DOT11_ENC_TKIP<<2, 0, key_combo);
				if (retVal) {
					priv->pshare->CamEntryOccupied++;
					if (pstat)	pstat->dot11KeyMapping.keyInCam = TRUE;
				}
				else {
					if (pstat->aggre_mthd != AGGRE_MTHD_NONE)
						pstat->aggre_mthd = AGGRE_MTHD_NONE;
				}
			}

			// Use legacy rate for TKIP
			if ((priv->pmib->dot11nConfigEntry.dot11nLgyEncRstrct & BIT(1)) &&
				(!pstat->is_rtl8192s_sta || (priv->pmib->dot11nConfigEntry.dot11nLgyEncRstrct & BIT(2)))) {
				if (pstat->ht_cap_len) {
					pstat->ht_cap_len = 0;
					pstat->MIMO_ps = 0;
					pstat->is_8k_amsdu = 0;
					pstat->tx_bw = HT_CHANNEL_WIDTH_20;
					if (priv->pmib->dot11StationConfigEntry.autoRate && (pstat->current_tx_rate & 0x80))
						pstat->current_tx_rate = find_rate(priv, pstat, 1, 0);
					add_update_RATid(priv, pstat);
				}
			}
			break;

		case DOT11_ENC_WEP40:
			set_ttkeylen(pEncryptKey, 5);
			set_tmickeylen(pEncryptKey, 0);
			set_wep40_key(pEncryptKey, key_combo);

			DEBUG_INFO("going to set WEP40 unicast key for sta %02X%02X%02X%02X%02X%02X, id=%d\n",
				Set_Key.MACAddr[0], Set_Key.MACAddr[1], Set_Key.MACAddr[2],
				Set_Key.MACAddr[3], Set_Key.MACAddr[4], Set_Key.MACAddr[5], pstat->keyid);
			if (!SWCRYPTO) {
				retVal = CamDeleteOneEntry(priv, Set_Key.MACAddr, Set_Key.KeyIndex, 0);
				if (retVal) {
					priv->pshare->CamEntryOccupied--;
					if (pstat)	pstat->dot11KeyMapping.keyInCam = FALSE;
				}
				retVal = CamAddOneEntry(priv, Set_Key.MACAddr, Set_Key.KeyIndex, DOT11_ENC_WEP40<<2, 0, key_combo);
				if (retVal) {
					priv->pshare->CamEntryOccupied++;
					if (pstat)	pstat->dot11KeyMapping.keyInCam = TRUE;
				}
				else {
					if (pstat->aggre_mthd != AGGRE_MTHD_NONE)
						pstat->aggre_mthd = AGGRE_MTHD_NONE;
				}
			}

			// Use legacy rate for Wep
			if ((priv->pmib->dot11nConfigEntry.dot11nLgyEncRstrct & BIT(0)) &&
				(!pstat->is_rtl8192s_sta || (priv->pmib->dot11nConfigEntry.dot11nLgyEncRstrct & BIT(2)))) {
				if (pstat->ht_cap_len) {
					pstat->ht_cap_len = 0;
					pstat->MIMO_ps = 0;
					pstat->is_8k_amsdu = 0;
					pstat->tx_bw = HT_CHANNEL_WIDTH_20;
					if (priv->pmib->dot11StationConfigEntry.autoRate && (pstat->current_tx_rate & 0x80))
						pstat->current_tx_rate = find_rate(priv, pstat, 1, 0);
					add_update_RATid(priv, pstat);
				}
			}
			break;

		case DOT11_ENC_WEP104:
			set_ttkeylen(pEncryptKey, 13);
			set_tmickeylen(pEncryptKey, 0);
			set_wep104_key(pEncryptKey, key_combo);

			DEBUG_INFO("going to set WEP104 unicast key for sta %02X%02X%02X%02X%02X%02X, id=%d\n",
				Set_Key.MACAddr[0], Set_Key.MACAddr[1], Set_Key.MACAddr[2],
				Set_Key.MACAddr[3], Set_Key.MACAddr[4], Set_Key.MACAddr[5], pstat->keyid);
			if (!SWCRYPTO) {
				retVal = CamDeleteOneEntry(priv, Set_Key.MACAddr, Set_Key.KeyIndex, 0);
				if (retVal) {
					priv->pshare->CamEntryOccupied--;
					if (pstat)	pstat->dot11KeyMapping.keyInCam = FALSE;
				}
				retVal = CamAddOneEntry(priv, Set_Key.MACAddr, Set_Key.KeyIndex, DOT11_ENC_WEP104<<2, 0, key_combo);
				if (retVal) {
					priv->pshare->CamEntryOccupied++;
					if (pstat)	pstat->dot11KeyMapping.keyInCam = TRUE;
				}
				else {
					if (pstat->aggre_mthd != AGGRE_MTHD_NONE)
						pstat->aggre_mthd = AGGRE_MTHD_NONE;
				}
			}

			// Use legacy rate for Wep
			if ((priv->pmib->dot11nConfigEntry.dot11nLgyEncRstrct & BIT(0)) &&
				(!pstat->is_rtl8192s_sta || (priv->pmib->dot11nConfigEntry.dot11nLgyEncRstrct & BIT(2)))) {
				if (pstat->ht_cap_len) {
					pstat->ht_cap_len = 0;
					pstat->MIMO_ps = 0;
					pstat->is_8k_amsdu = 0;
					pstat->tx_bw = HT_CHANNEL_WIDTH_20;
					if (priv->pmib->dot11StationConfigEntry.autoRate && (pstat->current_tx_rate & 0x80))
						pstat->current_tx_rate = find_rate(priv, pstat, 1, 0);
					add_update_RATid(priv, pstat);
				}
			}
			break;

		case DOT11_ENC_CCMP:
			set_ttkeylen(pEncryptKey, 16);
			set_tmickeylen(pEncryptKey, 16);
			set_aes_key(pEncryptKey, key_combo);

			DEBUG_INFO("going to set CCMP-AES unicast key for sta %02X%02X%02X%02X%02X%02X, id=%d\n",
				Set_Key.MACAddr[0], Set_Key.MACAddr[1], Set_Key.MACAddr[2],
				Set_Key.MACAddr[3], Set_Key.MACAddr[4], Set_Key.MACAddr[5], pstat->keyid);
			if (!SWCRYPTO) {
				retVal = CamDeleteOneEntry(priv, Set_Key.MACAddr, Set_Key.KeyIndex, 0);
				if (retVal) {
					priv->pshare->CamEntryOccupied--;
					if (pstat)	pstat->dot11KeyMapping.keyInCam = FALSE;
				}
				retVal = CamAddOneEntry(priv, Set_Key.MACAddr, Set_Key.KeyIndex, DOT11_ENC_CCMP<<2, 0, key_combo);
				if (retVal) {
					priv->pshare->CamEntryOccupied++;
					if (pstat)	pstat->dot11KeyMapping.keyInCam = TRUE;
					assign_aggre_mthod(priv, pstat);
				}
				else {
					if (pstat->aggre_mthd != AGGRE_MTHD_NONE)
						pstat->aggre_mthd = AGGRE_MTHD_NONE;
				}
			}
			break;

		case DOT11_ENC_NONE:
		default:
			DEBUG_ERR("No pairewise encryption key is set!\n");
			set_ttkeylen(pEncryptKey, 0);
			set_tmickeylen(pEncryptKey, 0);
			break;
		}
	}

	return 0;
}


/*---------------------------------------------------------------------
 Set Port Enable of Disable for STA
 Return Value
        Success : 0
-----------------------------------------------------------------------*/
static int DOT11_Process_Set_Port(struct net_device *dev, struct iw_point *data)
{
	struct stat_info	*pstat;
	struct rtl8190_priv	*priv = (struct rtl8190_priv *)dev->priv;
	struct wifi_mib		*pmib = priv->pmib;
	DOT11_SET_PORT		*Set_Port = (DOT11_SET_PORT *)data->pointer;

	DEBUG_INFO("Set_Port sta %02X%02X%02X%02X%02X%02X Status %X\n",
		Set_Port->MACAddr[0],Set_Port->MACAddr[1],Set_Port->MACAddr[2],
		Set_Port->MACAddr[3],Set_Port->MACAddr[4],Set_Port->MACAddr[5],
		Set_Port->PortStatus);

	// if driver is not opened, return immediately, david
	if (!netif_running(priv->dev))
		return (-1);

	pstat = get_stainfo(priv, Set_Port->MACAddr);

	if ((pstat == NULL) || (!(pstat->state & WIFI_ASOC_STATE)))
		return (-1);

	if (Set_Port->PortStatus)
		pstat->ieee8021x_ctrlport = Set_Port->PortStatus;
	else
		pstat->ieee8021x_ctrlport = pmib->dot118021xAuthEntry.dot118021xDefaultPort;

	return 0;
}


static int DOT11_Process_QueryRSC(struct net_device *dev, struct iw_point *data)
{
	DOT11_GKEY_TSC		*pGkey_TSC = (DOT11_GKEY_TSC *)data->pointer;
	struct rtl8190_priv	*priv = (struct rtl8190_priv *)dev->priv;
	struct wifi_mib		*pmib = priv->pmib;

	pGkey_TSC->EventId = DOT11_EVENT_GKEY_TSC;
	pGkey_TSC->IsMoreEvent = FALSE;

	// Fix bug of getting RSC for auth (should consider endian issue)
	//memcpy((void *)pGkey_TSC->KeyTSC,
	//	(void *)&(pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TXPN48.val48), 6);
	pGkey_TSC->KeyTSC[0] = pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TXPN48._byte_.TSC0;
	pGkey_TSC->KeyTSC[1] = pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TXPN48._byte_.TSC1;
	pGkey_TSC->KeyTSC[2] = pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TXPN48._byte_.TSC2;
	pGkey_TSC->KeyTSC[3] = pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TXPN48._byte_.TSC3;
	pGkey_TSC->KeyTSC[4] = pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TXPN48._byte_.TSC4;
	pGkey_TSC->KeyTSC[5] = pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TXPN48._byte_.TSC5;
	pGkey_TSC->KeyTSC[6] = 0x00;
	pGkey_TSC->KeyTSC[7] = 0x00;

	return 0;
}


static int DOT11_Porcess_EAPOL_MICReport(struct net_device *dev, struct iw_point *data)
{
#ifdef _DEBUG_RTL8190_
	struct rtl8190_priv	*priv = (struct rtl8190_priv *)dev->priv;
#endif
	struct stat_info *pstat = NULL;
	//DOT11_MIC_FAILURE *MIC_Failure = (DOT11_MIC_FAILURE *)data->pointer;

	DEBUG_INFO("MIC fail report from 802.1x daemon\n");
	//DEBUG_INFO("mac: %02x%02x%02x%02x%02x%02x\n", MIC_Failure->MACAddr[0], MIC_Failure->MACAddr[1],
	//MIC_Failure->MACAddr[2], MIC_Failure->MACAddr[3], MIC_Failure->MACAddr[4], MIC_Failure->MACAddr[5]);
#ifdef RTL_WPA2
	PRINT_INFO("%s: DOT11_Indicate_MIC_Failure \n", (char *)__FUNCTION__);
#endif

	DOT11_Indicate_MIC_Failure(dev, pstat);

	return 0;
}


int DOT11_Indicate_MIC_Failure(struct net_device *dev, struct stat_info *pstat)
{
	struct rtl8190_priv	*priv = (struct rtl8190_priv *)dev->priv;
	struct stat_info *pstat_del;
	DOT11_MIC_FAILURE	Mic_Failure;
	DOT11_DISASSOCIATION_IND Disassociation_Ind;
	int	i;

	// Indicate to upper layer
	if (pstat != NULL) // if pstat==NULL, it is called by upper layer
	{
		DEBUG_INFO("MIC error indicate to 1x in driver\n");
		//DEBUG_INFO("mac: %02x%02x%02x%02x%02x%02x\n", pstat->hwaddr[0], pstat->hwaddr[1],
		//pstat->hwaddr[2], pstat->hwaddr[3], pstat->hwaddr[4], pstat->hwaddr[5]);

#ifdef __DRAYTEK_OS__
		cb_tkip_micFailure(dev, pstat->hwaddr);
		return 0;
#endif

#ifndef WITHOUT_ENQUEUE
		Mic_Failure.EventId = DOT11_EVENT_MIC_FAILURE;
		Mic_Failure.IsMoreEvent = 0;
		memcpy(&Mic_Failure.MACAddr, pstat->hwaddr, MACADDRLEN);
		DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&Mic_Failure, sizeof(DOT11_MIC_FAILURE));
#endif
#ifdef INCLUDE_WPA_PSK
		psk_indicate_evt(priv, DOT11_EVENT_MIC_FAILURE, pstat->hwaddr, NULL, 0);
#endif
		event_indicate(priv, pstat->hwaddr, 5);
	}

	if (priv->MIC_timer_on)
	{
		// Second time to detect MIC error in a time period

		// Stop Timer (fill a timer before than now)
		#ifdef CONFIG_RTL8671
		if (pstat != NULL) {
			if( (pstat->dot11KeyMapping.keyInCam == TRUE) && ((jiffies - pstat->setCamTime) < 500 ) ) //cathy, to avoid disconnect sta if rx tkip data when hw is not ready for decryption
				return 0;
			else
				printk("pstat->setCamTime=%lu, jiffies=%lu\n", pstat->setCamTime, jiffies);
		}
		#endif
		if (timer_pending(&priv->MIC_check_timer))
			del_timer_sync(&priv->MIC_check_timer);
		priv->MIC_timer_on = FALSE;

		// Start Timer to reject assocaiton request from STA, and reject all the packet
		mod_timer(&priv->assoc_reject_timer, jiffies + REJECT_ASSOC_PERIOD);
		priv->assoc_reject_on = TRUE;
		for(i=0; i<NUM_STAT; i++)
		{
			if (priv->pshare->aidarray[i] && (priv->pshare->aidarray[i]->used == TRUE)
#ifdef WDS
				&& !(priv->pshare->aidarray[i]->station.state & WIFI_WDS)
#endif
			) {
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
				if (priv != priv->pshare->aidarray[i]->priv)
					continue;
#endif

				pstat_del = &(priv->pshare->aidarray[i]->station);
				if (!list_empty(&pstat_del->asoc_list))
				{
#ifndef WITHOUT_ENQUEUE
					memcpy((void *)Disassociation_Ind.MACAddr, (void *)pstat_del->hwaddr, MACADDRLEN);
					Disassociation_Ind.EventId = DOT11_EVENT_DISASSOCIATION_IND;
					Disassociation_Ind.IsMoreEvent = 0;
					Disassociation_Ind.Reason = _STATS_OTHER_;
					Disassociation_Ind.tx_packets = pstat_del->tx_pkts;
					Disassociation_Ind.rx_packets = pstat_del->rx_pkts;
					Disassociation_Ind.tx_bytes   = pstat_del->tx_bytes;
					Disassociation_Ind.rx_bytes   = pstat_del->rx_bytes;
					DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&Disassociation_Ind,
								sizeof(DOT11_DISASSOCIATION_IND));
#endif
#ifdef INCLUDE_WPA_PSK
					psk_indicate_evt(priv, DOT11_EVENT_DISASSOCIATION_IND, pstat_del->hwaddr, NULL, 0);
#endif
					event_indicate(priv, pstat_del->hwaddr, 2);

					issue_disassoc(priv, pstat_del->hwaddr, _RSON_MIC_FAILURE_);
				}
				free_stainfo(priv, pstat_del);
			}
		}
		priv->assoc_num = 0;
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) {
			priv->pmib->dot11ErpInfo.nonErpStaNum = 0;
			check_protection_shortslot(priv);
			priv->pmib->dot11ErpInfo.longPreambleStaNum = 0;
		}
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)
			priv->ht_legacy_sta_num = 0;

		// ToDo: Should we delete group key?

		DEBUG_INFO("-------------------------------------------------------\n");
		DEBUG_INFO("Second time of MIC failure in a time period            \n");
		DEBUG_INFO("-------------------------------------------------------\n");
	}
	else
	{
		// First time to detect MIC error, start the timer
		mod_timer(&priv->MIC_check_timer, jiffies + MIC_TIMER_PERIOD);
		priv->MIC_timer_on = TRUE;

		DEBUG_INFO("-------------------------------------------------------\n");
		DEBUG_INFO("First time of MIC failure in a time period, start timer\n");
		DEBUG_INFO("-------------------------------------------------------\n");
	}

	return 0;
}


void DOT11_Process_MIC_Timerup(unsigned long data)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)data;
	DEBUG_INFO("MIC Timer is up. Cancel Timer\n");
	priv->MIC_timer_on = FALSE;
}


void DOT11_Process_Reject_Assoc_Timerup(unsigned long data)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)data;
	DEBUG_INFO("Reject Association Request Timer is up. Cancel Timer\n");
	priv->assoc_reject_on = FALSE;
}


void DOT11_Indicate_MIC_Failure_Clnt(struct rtl8190_priv *priv, unsigned char *sa)
{
	DOT11_MIC_FAILURE	Mic_Failure;
#ifndef WITHOUT_ENQUEUE
	Mic_Failure.EventId = DOT11_EVENT_MIC_FAILURE;
	Mic_Failure.IsMoreEvent = 0;
	memcpy(&Mic_Failure.MACAddr, sa, MACADDRLEN);
	DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&Mic_Failure, sizeof(DOT11_MIC_FAILURE));
#endif
	event_indicate(priv, sa, 5);
}


#if 0
void DOT11_Process_Acc_SetExpiredTime(struct net_device *dev, struct iw_point *data)
{
	struct rtl8180_priv	*priv = (struct rtl8180_priv *) dev->priv;
	WLAN_CTX        	*wCtx = (WLAN_CTX *) ( priv->pwlanCtx );
	DOT11_SET_EXPIREDTIME	*Set_ExpireTime = (DOT11_SET_EXPIREDTIME *)data->pointer;
	int	sta_tbl_idx;

	Set_ExpireTime->EventId = DOT11_EVENT_ACC_SET_EXPIREDTIME;
	Set_ExpireTime->IsMoreEvent = 0;

	if( Set_ExpireTime != NULL ){
		if( wlan_sta_tbl_lookup(wCtx, Set_ExpireTime->MACAddr, &sta_tbl_idx) == TRUE ){
			wCtx->wlan_sta_tbl[sta_tbl_idx].def_expired_time = Set_ExpireTime->ExpireTime;
			DEBUG_INFO("%s: Set wlan_sta_tbl[%d].def_expired_time = %ld!\n", (char *)__FUNCTION__, sta_tbl_idx, Set_ExpireTime->ExpireTime);
		}
		else{
			DEBUG_ERR("%s: ERROR wlan_sta_tbl_lookup!\n", (char *)__FUNCTION__);
		}
	}
	else{
		DEBUG_ERR("%s: NULL POINTER!\n", (char *)__FUNCTION__);
	}
}


void DOT11_Process_Acc_QueryStats(struct net_device *dev, struct iw_point *data)
{
	struct rtl8180_priv	*priv = (struct rtl8180_priv *) dev->priv;
	WLAN_CTX        	*wCtx = (WLAN_CTX *) ( priv->pwlanCtx );
	DOT11_QUERY_STATS	*pStats = (DOT11_QUERY_STATS *)data->pointer;
	int	sta_tbl_idx;

	pStats->EventId = DOT11_EVENT_ACC_QUERY_STATS;
	pStats->IsMoreEvent = 0;

	if( pStats != NULL ){
		if( wlan_sta_tbl_lookup(wCtx, pStats->MACAddr, &sta_tbl_idx) == TRUE ){
			pStats->tx_packets = wCtx->wlan_sta_tbl[sta_tbl_idx].tx_packets ;
			pStats->rx_packets = wCtx->wlan_sta_tbl[sta_tbl_idx].rx_packets ;
			pStats->tx_bytes = wCtx->wlan_sta_tbl[sta_tbl_idx].tx_bytes ;
			pStats->rx_bytes = wCtx->wlan_sta_tbl[sta_tbl_idx].rx_bytes ;

			pStats->IsSuccess = TRUE;
			DEBUG_INFO("%s: Get wlan_sta_tbl[%d].stats!\n", (char *)__FUNCTION__, sta_tbl_idx);
		}
		else{
			pStats->IsSuccess = FALSE;
			DEBUG_ERR("%s: ERROR wlan_sta_tbl_lookup!\n", (char *)__FUNCTION__);
		}
	}
	else{
		DEBUG_ERR("%s: NULL POINTER!\n", (char *)__FUNCTION__);
	}
}


//-------------------------
//DOT11_QUERY_STATS		stats[RTL_AP_MAX_STA_NUM+1];
//data->pointer = (unsigned char *)stats;
void DOT11_Process_Acc_QueryStats_All(struct net_device *dev, struct iw_point *data)
{
	struct rtl8180_priv	*priv = (struct rtl8180_priv *) dev->priv;
	WLAN_CTX        	*wCtx = (WLAN_CTX *) ( priv->pwlanCtx );
	DOT11_QUERY_STATS	*pStats = (DOT11_QUERY_STATS *)data->pointer;
	int i;
	int cnt = 0;

	if( pStats != NULL ){

		for( i=1; i<=RTL_AP_MAX_STA_NUM; i++ ){
			if( ( wCtx->wlan_sta_tbl[i].aid != 0 ) && GstaInfo_assoc( wCtx, i ) ){

				pStats[cnt].EventId = DOT11_EVENT_ACC_QUERY_STATS_ALL;
				pStats[cnt].IsMoreEvent = 0;

				memcpy( pStats[cnt].MACAddr, wCtx->wlan_sta_tbl[i].addr, 6 );
				pStats[cnt].tx_packets = wCtx->wlan_sta_tbl[i].tx_packets ;
				pStats[cnt].rx_packets = wCtx->wlan_sta_tbl[i].rx_packets ;
				pStats[cnt].tx_bytes = wCtx->wlan_sta_tbl[i].tx_bytes ;
				pStats[cnt].rx_bytes = wCtx->wlan_sta_tbl[i].rx_bytes ;

				cnt++;
			}
		}
	}
	else{
		printk("%s: NULL POINTER!\n", (char *)__FUNCTION__);
	}
}
#endif


static int DOT11_Process_STA_Query_Bssid(struct net_device *dev, struct iw_point *data)
{
	DOT11_STA_QUERY_BSSID	*pQuery = (DOT11_STA_QUERY_BSSID *)data->pointer;
	struct rtl8190_priv	*priv = (struct rtl8190_priv *)dev->priv;

	pQuery->EventId = DOT11_EVENT_STA_QUERY_BSSID;
	pQuery->IsMoreEvent = FALSE;
	if ((OPMODE & (WIFI_STATION_STATE | WIFI_ASOC_STATE)) ==
		(WIFI_STATION_STATE | WIFI_ASOC_STATE))
	{
		pQuery->IsValid = TRUE;
		memcpy(pQuery->Bssid, BSSID, 6);
	}
	else
		pQuery->IsValid = FALSE;

	return 0;
}


static int DOT11_Process_STA_Query_Ssid(struct net_device *dev, struct iw_point *data)
{
	DOT11_STA_QUERY_SSID	*pQuery = (DOT11_STA_QUERY_SSID *)data->pointer;
	struct rtl8190_priv	*priv = (struct rtl8190_priv *)dev->priv;

	pQuery->EventId = DOT11_EVENT_STA_QUERY_SSID;
	pQuery->IsMoreEvent = FALSE;
	if ((OPMODE & (WIFI_STATION_STATE | WIFI_ASOC_STATE)) ==
		(WIFI_STATION_STATE | WIFI_ASOC_STATE))
	{
		pQuery->IsValid = TRUE;
		memcpy(pQuery->ssid, SSID, 32);
		pQuery->ssid_len = SSID_LEN;
	}
	else
		pQuery->IsValid = FALSE;

	return 0;
}


#ifdef WIFI_SIMPLE_CONFIG
static int DOT11_WSC_set_ie(struct net_device *dev, struct iw_point *data)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)(dev->priv);
	DOT11_SET_RSNIE *Set_RSNIE = (DOT11_SET_RSNIE *)data->pointer;

	if (Set_RSNIE->Flag == SET_IE_FLAG_BEACON) {
		DEBUG_INFO("WSC: set beacon IE\n");
		priv->pmib->wscEntry.beacon_ielen = Set_RSNIE->RSNIELen;
		memcpy((void *)priv->pmib->wscEntry.beacon_ie, Set_RSNIE->RSNIE, Set_RSNIE->RSNIELen);
#ifdef RTL8192SU_FWBCN
		priv->update_bcn++;
#endif
	}
	else if (Set_RSNIE->Flag == SET_IE_FLAG_PROBE_RSP) {
		DEBUG_INFO("WSC: set probe response IE\n");
		priv->pmib->wscEntry.probe_rsp_ielen = Set_RSNIE->RSNIELen;
		memcpy((void *)priv->pmib->wscEntry.probe_rsp_ie, Set_RSNIE->RSNIE, Set_RSNIE->RSNIELen);
	}
	else if (Set_RSNIE->Flag == SET_IE_FLAG_PROBE_REQ) {
		DEBUG_INFO("WSC: set probe request IE\n");
		priv->pmib->wscEntry.probe_req_ielen = Set_RSNIE->RSNIELen;
		memcpy((void *)priv->pmib->wscEntry.probe_req_ie, Set_RSNIE->RSNIE, Set_RSNIE->RSNIELen);
	}
	else if (Set_RSNIE->Flag == SET_IE_FLAG_ASSOC_REQ ||
			Set_RSNIE->Flag == SET_IE_FLAG_ASSOC_RSP) {
		DEBUG_INFO("WSC: set assoc IE\n");
		priv->pmib->wscEntry.assoc_ielen = Set_RSNIE->RSNIELen;
		memcpy((void *)priv->pmib->wscEntry.assoc_ie, Set_RSNIE->RSNIE, Set_RSNIE->RSNIELen);
	}
#if 0
	else if (Set_RSNIE->Flag == 3) {
		printk("WSC: set RSN IE\n");
		priv->pmib->dot11RsnIE.rsnielen = Set_RSNIE->RSNIELen;
		memcpy((void *)priv->pmib->dot11RsnIE.rsnie, Set_RSNIE->RSNIE, Set_RSNIE->RSNIELen);
	}
#endif
	else {
		DEBUG_ERR("Invalid flag of set IE [%d]!\n", Set_RSNIE->Flag);
	}
	return 0;
}
#endif // WIFI_SIMPLE_CONFIG


/*-----------------------------------------------------------------------------
Most of the time, we don't have to worry about the racing condition of
"event_queue" in wlan drivers, since all the queue/dequeue are handled
in non-isr context.
However, my guess is, someone always want to port these driver to fit different
OS platform. At that time, please always keep in mind all the possilbe racing
condition... That could be big disasters...
------------------------------------------------------------------------------*/
#if defined(RTL8192SU) && defined(CONFIG_ENABLE_MIPS16)
__attribute__((nomips16)) 
#endif
int rtl8190_ioctl_priv_daemonreq(struct net_device *dev, struct iw_point *data)
{
	int		ret;
#ifndef WITHOUT_ENQUEUE
	static UINT8 QueueData[MAXDATALEN];
	int		QueueDataLen;
#elif defined(CONFIG_RTL_WAPI_SUPPORT)
	static UINT8 QueueData[MAXDATALEN];
	int		QueueDataLen;
#endif
	UINT8	val8;
	DOT11_REQUEST	req;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;

#ifdef INCLUDE_WPA_PSK
	memcpy((void *)&req, (void *)(data->pointer), sizeof(DOT11_REQUEST));
#else
	if (copy_from_user((void *)&req, (void *)(data->pointer), sizeof(DOT11_REQUEST))) {
		DEBUG_ERR("copy_from_user error!\n");
		return -1;
	}
#endif

	switch(req.EventId)
	{
		case DOT11_EVENT_NO_EVENT:
			break;

#ifndef WITHOUT_ENQUEUE
		case DOT11_EVENT_REQUEST:
			if((ret = DOT11_DeQueue((unsigned long)priv, priv->pevent_queue, QueueData, &QueueDataLen)) != 0)
			{
				val8 = DOT11_EVENT_NO_EVENT;
				if (copy_to_user((void *)((UINT32)(data->pointer)), &val8, 1)) {
					DEBUG_ERR("copy_to_user fail!\n");
					return -1;
				}
				val8 = 0;
				if (copy_to_user((void *)((UINT32)(data->pointer) + 1), &val8, 1)) {
					DEBUG_ERR("copy_to_user fail!\n");
					return -1;
				}
				data->length = sizeof(DOT11_NO_EVENT);
			}
			else
			{
				QueueData[1] = (priv->pevent_queue->NumItem != 0)? 1 : 0;
				if (copy_to_user((void *)data->pointer, (void *)QueueData, QueueDataLen)) {
					DEBUG_ERR("copy_to_user fail!\n");
					return -1;
				}
				data->length = QueueDataLen;
			}
			break;
#endif

		case DOT11_EVENT_ASSOCIATION_RSP:
			if(!DOT11_Process_Association_Rsp(dev, data, WIFI_ASSOCRSP))
			{}
			break;

		case DOT11_EVENT_DISCONNECT_REQ:
			if(!DOT11_Process_Disconnect_Req(dev, data))
			{}
			break;

		case DOT11_EVENT_SET_802DOT11:
			break;

		case DOT11_EVENT_SET_KEY:
			if(!DOT11_Process_Set_Key(dev, data, NULL, NULL))
			{}
			break;

		case DOT11_EVENT_SET_PORT:
			if(!DOT11_Process_Set_Port(dev, data))
			{}
			break;

		case DOT11_EVENT_DELETE_KEY:
			if(!DOT11_Process_Delete_Key(dev, data))
			{}
			break;

		case DOT11_EVENT_SET_RSNIE:
			if(priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _NO_PRIVACY_ &&
				priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _WEP_40_PRIVACY_ &&
				priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _WEP_104_PRIVACY_)
				if(!DOT11_Process_Set_RSNIE(dev, data))
				{}
			break;

		case DOT11_EVENT_GKEY_TSC:
			if(!DOT11_Process_QueryRSC(dev, data))
			{}
			break;

		case DOT11_EVENT_MIC_FAILURE:
			if(!DOT11_Porcess_EAPOL_MICReport(dev, data))
			{}
			break;

		case DOT11_EVENT_ASSOCIATION_INFO:
			#if 0
			if(!DOT11_Process_Association_Info(dev, data))
			{}
			#endif
			DEBUG_INFO("trying to process assoc info\n");
			break;

		case DOT11_EVENT_INIT_QUEUE:
			DOT11_InitQueue(priv->pevent_queue);
			break;

		case DOT11_EVENT_ACC_SET_EXPIREDTIME:
			//DOT11_Process_Acc_SetExpiredTime(dev, data);
			DEBUG_INFO("trying to Set ACC Expiredtime\n");
			break;

		case DOT11_EVENT_ACC_QUERY_STATS:
			//DOT11_Process_Acc_QueryStats(dev, data);
			DEBUG_INFO("trying to Set ACC Expiredtime\n");
			break;

		case DOT11_EVENT_REASSOCIATION_RSP:
			if(!DOT11_Process_Association_Rsp(dev, data, WIFI_REASSOCRSP))
			{}
			break;

		case DOT11_EVENT_STA_QUERY_BSSID:
			if(!DOT11_Process_STA_Query_Bssid(dev, data))
			{};
			break;

		case DOT11_EVENT_STA_QUERY_SSID:
			if(!DOT11_Process_STA_Query_Ssid(dev, data))
			{};
			break;

#ifdef WIFI_SIMPLE_CONFIG
		case DOT11_EVENT_WSC_SET_IE:
			if(!DOT11_WSC_set_ie(dev, data))
			{};
			break;
#endif

#ifdef CONFIG_RTL_WAPI_SUPPORT
		case DOT11_EVENT_WAPI_INIT_QUEUE:
			DOT11_InitQueue(priv->wapiEvent_queue);
			break;
		case DOT11_EVENT_WAPI_READ_QUEUE:
			if((ret = DOT11_DeQueue((unsigned long)priv, priv->wapiEvent_queue, QueueData, &QueueDataLen)) != 0)
			{
				val8 = DOT11_EVENT_NO_EVENT;
				if (copy_to_user((void *)((UINT32)(data->pointer)), &val8, 1)) {
					DEBUG_ERR("copy_to_user fail!\n");
					return -1;
				}
				val8 = 0;
				if (copy_to_user((void *)((UINT32)(data->pointer) + 1), &val8, 1)) {
					DEBUG_ERR("copy_to_user fail!\n");
					return -1;
				}
				data->length = sizeof(DOT11_NO_EVENT);
			}
			else
			{
				QueueData[1] = (priv->wapiEvent_queue->NumItem != 0)? 1 : 0;
				if (copy_to_user((void *)data->pointer, (void *)QueueData, QueueDataLen)) {
					DEBUG_ERR("copy_to_user fail!\n");
					return -1;
				}
				data->length = QueueDataLen;
			}
			break;
		case DOT11_EVENT_WAPI_WRITE_QUEUE:
			{
				unsigned char *kernelbuf;
				kernelbuf = (unsigned char *)kmalloc(data->length,GFP_ATOMIC);
				if(NULL == kernelbuf)
				{
					DEBUG_ERR("Memory alloc fail!\n");
					return -1;
				}
				if (copy_from_user((void *)kernelbuf, (void *)(data->pointer), data->length)) {
					DEBUG_ERR("copy_from_user error!\n");
					kfree(kernelbuf);
					return -1;
				}
				DOT11_Process_WAPI_Info(priv,kernelbuf,data->length);
				kfree(kernelbuf);
			}
			break;
#endif
		default:
			DEBUG_ERR("unknown user daemon command\n");
			break;
	} // switch(req->EventId)

	return 0;
}

