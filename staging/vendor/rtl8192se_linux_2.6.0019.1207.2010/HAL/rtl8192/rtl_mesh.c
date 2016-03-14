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
#ifdef _RTL8192_EXT_PATCH_

#include <asm/uaccess.h>
#include <linux/pci.h>

#include "rtl_wx.h"
#include "rtl_core.h"
#include "../../mshclass/msh_class.h"

int meshdev_up(struct net_device *meshdev,bool is_silent_reset)
{
	struct meshdev_priv * mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	PRT_POWER_SAVE_CONTROL pPSC = (PRT_POWER_SAVE_CONTROL)(&(mpriv->priv->rtllib->PowerSaveControl));
        bool init_status;
        struct net_device *dev = ieee->dev; 
	RT_TRACE(COMP_DOWN, "==========>%s()\n", __FUNCTION__);

	mpriv->priv->up_first_time = 0;
	mpriv->rtllib->iw_mode = IW_MODE_MESH;   
	if(mpriv->priv->mesh_up){
		RT_TRACE(COMP_INIT,"%s():mesh is up,return\n",__FUNCTION__);
		return -1;
	}
#ifdef RTL8192SE        
        mpriv->priv->ReceiveConfig =
        RCR_APPFCS | RCR_APWRMGT | /*RCR_ADD3 |*/
        RCR_AMF | RCR_ADF | RCR_APP_MIC | RCR_APP_ICV |
       RCR_AICV | RCR_ACRC32    |                               
        RCR_AB          | RCR_AM                |                               
        RCR_APM         |                                                               
        /*RCR_AAP               |*/                                                     
        RCR_APP_PHYST_STAFF | RCR_APP_PHYST_RXFF |      
        (mpriv->priv->EarlyRxThreshold<<RCR_FIFO_OFFSET)       ;
#elif defined RTL8192CE
		mpriv->priv->ReceiveConfig = (\
				RCR_APPFCS	
				| RCR_AMF | RCR_ADF| RCR_APP_MIC| RCR_APP_ICV
				| RCR_AICV | RCR_ACRC32 
				| RCR_AB | RCR_AM			
					| RCR_APM					
					| RCR_APP_PHYST_RXFF		
					| RCR_HTC_LOC_CTRL
				);
#else
        mpriv->priv->ReceiveConfig = RCR_ADD3  |
                RCR_AMF | RCR_ADF |             
                RCR_AICV |                      
                RCR_AB | RCR_AM | RCR_APM |     
                RCR_AAP | ((u32)7<<RCR_MXDMA_OFFSET) |
                ((u32)7 << RCR_FIFO_OFFSET) | RCR_ONLYERLPKT;

#endif

	if(!mpriv->priv->up) {
		RT_TRACE(COMP_INIT, "Bringing up iface");
		mpriv->priv->bfirst_init = true;
		init_status = mpriv->priv->ops->initialize_adapter(dev);
		if(init_status != true) {
			RT_TRACE(COMP_ERR,"ERR!!! %s(): initialization is failed!\n",__FUNCTION__);
			return -1;
		}
		RT_TRACE(COMP_INIT, "start adapter finished\n");
		RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);
		printk("==>%s():priv->up is 0\n",__FUNCTION__);
		mpriv->rtllib->ieee_up=1;		
		mpriv->priv->bfirst_init = false;
#ifdef ENABLE_GPIO_RADIO_CTL
		if(mpriv->priv->polling_timer_on == 0){
			check_rfctrl_gpio_timer((unsigned long)dev);
		}
#endif
		if(!is_silent_reset){
			mpriv->rtllib->current_network.channel = INIT_DEFAULT_CHAN;
			mpriv->rtllib->current_mesh_network.channel = INIT_DEFAULT_CHAN;
			if(mpriv->priv->mshobj->ext_patch_r819x_wx_set_mesh_chan) 
				mpriv->priv->mshobj->ext_patch_r819x_wx_set_mesh_chan(dev,INIT_DEFAULT_CHAN);          
			if(mpriv->priv->mshobj->ext_patch_r819x_wx_set_channel)
			{
				mpriv->priv->mshobj->ext_patch_r819x_wx_set_channel(mpriv->rtllib, INIT_DEFAULT_CHAN);
			}
			printk("%s():set chan %d\n",__FUNCTION__,INIT_DEFAULT_CHAN);
			mpriv->rtllib->set_chan(dev, INIT_DEFAULT_CHAN);
		}
		dm_InitRateAdaptiveMask(dev);
		watch_dog_timer_callback((unsigned long) dev);
	
	} else {
		rtllib_stop_scan(ieee);
		msleep(1000);
	}
        if(!ieee->mesh_started) {
#ifdef RTL8192E
            if(ieee->eRFPowerState!=eRfOn)
                MgntActSet_RF_State(dev, eRfOn, ieee->RfOffReason,true);	
#endif
            if(mpriv->priv->mshobj && mpriv->priv->mshobj->ext_patch_rtl819x_up )
                mpriv->priv->mshobj->ext_patch_rtl819x_up(mpriv->priv->mshobj);


            if(!netif_queue_stopped(meshdev))
                netif_start_queue(meshdev);
            else
                netif_wake_queue(meshdev);

            rtllib_reset_queue(ieee);
	}
		
	mpriv->priv->mesh_up = 1;

	RT_TRACE(COMP_DOWN, "<==========%s()\n", __FUNCTION__);
	return 0;
}

int meshdev_down(struct net_device *meshdev)
{
	struct meshdev_priv * mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct r8192_priv *priv = (void*)ieee->priv;
	struct net_device *dev = ieee->dev;
	unsigned long flags = 0;
	u8 RFInProgressTimeOut = 0;
	if(priv->mesh_up == 0) {
		RT_TRACE(COMP_ERR,"%s():ERR!!! mesh is already down\n",__FUNCTION__)
		return -1;
	}

	RT_TRACE(COMP_DOWN, "==========>%s()\n", __FUNCTION__);
	
	if (netif_running(meshdev)) {
		netif_stop_queue(meshdev);
	}

	if(!priv->up)
	{
        	priv->bDriverIsGoingToUnload = true;	
		ieee->ieee_up = 0;  
		rtl8192_irq_disable(dev);
		rtl8192_cancel_deferred_work(priv);
#ifndef RTL8190P
		cancel_delayed_work(&priv->rtllib->hw_wakeup_wq);
#endif
		deinit_hal_dm(dev);
		del_timer_sync(&priv->watch_dog_timer);	
		rtllib_softmac_stop_protocol(ieee, 1, 0,true);

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
#ifdef CONFIG_ASPM_OR_D3
		RT_ENABLE_ASPM(dev);
#endif
		memset(&ieee->current_mesh_network, 0 , offsetof(struct rtllib_network, list));
		priv->rtllib->current_mesh_network.channel = INIT_DEFAULT_CHAN;
		ieee->mesh_state = RTLLIB_NOLINK;
	} else {
		rtllib_softmac_stop_protocol(ieee, 1, 0, true);
		memset(&ieee->current_mesh_network, 0 , offsetof(struct rtllib_network, list));
		if((ieee->current_network.channel > 0) && (ieee->current_network.channel < 15))
			priv->rtllib->current_mesh_network.channel = ieee->current_network.channel;
		else
			priv->rtllib->current_mesh_network.channel = INIT_DEFAULT_CHAN;
				
		ieee->mesh_state = RTLLIB_NOLINK;
		ieee->iw_mode = IW_MODE_INFRA;
	}
	priv->mesh_up = 0;  
	RT_TRACE(COMP_DOWN, "<==========%s()\n", __FUNCTION__);

	return 0;
}

static int meshdev_open(struct net_device *meshdev)
{
	struct meshdev_priv * mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct r8192_priv *priv = (void*)ieee->priv;
	int ret;
	
	SEM_DOWN_PRIV_WX(&priv->wx_sem);	
	ret = meshdev_up(meshdev,false);
	SEM_UP_PRIV_WX(&priv->wx_sem);	

	return ret;
}

static int meshdev_close(struct net_device *meshdev)
{
	struct meshdev_priv * mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct r8192_priv *priv = (void *)ieee->priv;
	int ret;
	
	SEM_DOWN_PRIV_WX(&priv->wx_sem);	
	ret = meshdev_down(meshdev);
	SEM_UP_PRIV_WX(&priv->wx_sem);	
	
	return ret;
}

extern int meshdev_wx_mesh(struct net_device *meshdev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra);
static int meshdev_ioctl(struct net_device *meshdev, struct ifreq *rq, int cmd)
{
	struct iwreq *wrq = (struct iwreq *)rq;
	int ret = -1;
	if(cmd == 0x8BEE)
	{
		ret = meshdev_wx_mesh(meshdev, NULL, &(wrq->u), wrq->u.data.pointer);
	}
	return ret;
}

struct net_device_stats *meshdev_stats(struct net_device *meshdev)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0) 
	return &((struct meshdev_priv*)netdev_priv(meshdev))->stats;
#else
	return &((struct meshdev_priv*)meshdev->priv)->stats;
#endif
}

static int meshdev_tx(struct sk_buff *skb, struct net_device *meshdev)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device * ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

	memset(skb->cb,0,sizeof(skb->cb));
	
	return rtllib_mesh_xmit(skb, dev);	
}

static void meshdev_tx_timeout(struct net_device *meshdev)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device * ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

	rtl8192_tx_timeout(dev);	
}

#ifdef HAVE_NET_DEVICE_OPS
static const struct net_device_ops mesh_netdev_ops = {
	.ndo_open = meshdev_open,
	.ndo_stop = meshdev_close,
	.ndo_get_stats = meshdev_stats,
	.ndo_tx_timeout = meshdev_tx_timeout,
	.ndo_do_ioctl = meshdev_ioctl,
	.ndo_start_xmit = meshdev_tx,
};
#endif

void meshdev_init(struct net_device* meshdev)
{
	struct meshdev_priv *mpriv;
	ether_setup(meshdev);

#ifdef HAVE_NET_DEVICE_OPS
	meshdev->netdev_ops = &mesh_netdev_ops;
#else	
	meshdev->open = meshdev_open;
	meshdev->stop = meshdev_close;
	meshdev->tx_timeout = meshdev_tx_timeout;
	meshdev->do_ioctl = meshdev_ioctl;
	meshdev->get_stats = meshdev_stats;
	meshdev->hard_start_xmit = meshdev_tx;
#endif	
	meshdev->wireless_handlers = &meshdev_wx_handlers_def;
	meshdev->destructor = free_netdev;
	meshdev->watchdog_timeo = HZ*3;	
	meshdev->type = ARPHRD_ETHER; 
	memset(meshdev->broadcast,0xFF, ETH_ALEN);
	
	meshdev->watchdog_timeo = 3 * HZ;

	mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	memset(mpriv, 0, sizeof(struct meshdev_priv));

	return;
}

int meshdev_update_ext_chnl_offset_as_client(void *data)
{
	struct rtllib_device *ieee = container_of_work_rsl(data, struct rtllib_device,
			ext_update_extchnloffset_wq);
	struct r8192_priv *priv = (void *)ieee->priv;
	struct net_device *dev = ieee->dev;
	struct mshclass *mshobj= priv->mshobj;
	u8 updateBW = 0;
	u8 bserverHT = 0;

	updateBW=mshobj->ext_patch_r819x_wx_update_beacon(dev,&bserverHT);
	printk("%s(): Cur_network.chan=%d, cur_mesh_net.chan=%d, updateBW=%d, bserverHT=%d\n",
			__FUNCTION__,ieee->current_network.channel,ieee->current_mesh_network.channel,updateBW, bserverHT);
	if (updateBW == 1) {
		if (bserverHT == 0) {
			printk("===>server is not HT supported,set 20M\n");
			HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20, HT_EXTCHNL_OFFSET_NO_EXT);  
		} else {
			printk("===>updateBW is 1,bCurBW40MHz is %d,ieee->serverExtChlOffset is %d\n",
					ieee->pHTInfo->bCurBW40MHz,ieee->serverExtChlOffset);
			if (ieee->pHTInfo->bCurBW40MHz)
				HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20_40, ieee->serverExtChlOffset);  
			else
				HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20, ieee->serverExtChlOffset);  
		}
	} else {
		printk("===>there is no same hostname server, ERR!!!\n");
		return -1;
	}
	return 0;
}

int meshdev_start_mesh_protocol_wq(void *data)
{
	struct rtllib_device *ieee = container_of_work_rsl(data, struct rtllib_device,
			ext_start_mesh_protocol_wq);

	ieee->set_chan(ieee->dev, ieee->current_mesh_network.channel);
	if(ieee->pHTInfo->bCurBW40MHz)
		HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20_40, (ieee->current_mesh_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER);  
	else
		HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20, (ieee->current_mesh_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER);  

	rtllib_start_mesh_protocol(ieee);

	return 0;
}
#endif 
