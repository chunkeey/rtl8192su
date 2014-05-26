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
#include "../rtl_core.h"

#define IWL_SCAN_CHECK_WATCHDOG (7 * HZ)

void rtl8192se_hw_scan_simu(void *data)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
	struct r8192_priv *priv = container_of_dwork_rsl(data,struct r8192_priv,hw_scan_simu_wq);
	struct net_device *dev = priv->rtllib->dev;
#else
	struct net_device *dev = (struct net_device *)data;
	struct r8192_priv *priv = rtllib_priv(dev);
#endif

	rtllib_update_active_chan_map(priv->rtllib);

	priv->rtllib->current_network.channel =	(priv->rtllib->current_network.channel + 1) % 12;
	if (priv->rtllib->scan_watch_dog++ > 12)
	{
		goto out; /* no good chans */
	}

	if ((!test_bit(STATUS_SCANNING, &priv->rtllib->status)) || 
			(test_bit(STATUS_SCAN_ABORTING, &priv->rtllib->status))){
		goto out;
	}

	if(priv->rtllib->current_network.channel != 0){
		priv->rtllib->set_chan(dev,priv->rtllib->current_network.channel);
#ifdef _RTL8192_EXT_PATCH_
		rtllib_send_probe_requests(priv->rtllib, 1);
#else
		rtllib_send_probe_requests(priv->rtllib, 0);
#endif
	}

	queue_delayed_work_rsl(priv->priv_wq, &priv->hw_scan_simu_wq, MSECS(RTLLIB_SOFTMAC_SCAN_TIME));

	return;
out:
	priv->rtllib->scan_watch_dog =0;
	rtl8192se_rx_surveydone_cmd(dev);

	if(priv->rtllib->state == RTLLIB_LINKED_SCANNING){
		priv->rtllib->current_network.channel = priv->rtllib->hwscan_ch_bk;
		printk("%s():back to linked chan:%d\n", __func__,priv->rtllib->current_network.channel);
		priv->rtllib->set_chan(dev,priv->rtllib->current_network.channel);
	}
}

void rtl8192se_before_hw_scan(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	HT_EXTCHNL_OFFSET chan_offset=0;
	HT_CHANNEL_WIDTH bandwidth=0;
	struct rtllib_device *ieee = priv->rtllib;

	if(priv->rtllib->state != RTLLIB_LINKED)
		return;


#ifdef ENABLE_LPS
	if (ieee->LeisurePSLeave) {
		ieee->LeisurePSLeave(ieee->dev);
	}
	/* notify AP to be in PS mode */
	rtllib_sta_ps_send_null_frame(ieee, 1);
	rtllib_sta_ps_send_null_frame(ieee, 1);
#endif

	rtllib_stop_all_queues(ieee);

	if (ieee->data_hard_stop)
		ieee->data_hard_stop(ieee->dev);
	rtllib_stop_send_beacons(ieee);
	ieee->state = RTLLIB_LINKED_SCANNING;
	ieee->link_change(ieee->dev);
	/* wait for ps packet to be kicked out successfully */
	mdelay(50);

#if 1
	if (ieee->SetFwCmdHandler) {
		ieee->SetFwCmdHandler(ieee->dev, FW_CMD_PAUSE_DM_BY_SCAN);
	}
#endif

	if (ieee->pHTInfo->bCurrentHTSupport && ieee->pHTInfo->bEnableHT && ieee->pHTInfo->bCurBW40MHz) {
		priv->hwscan_bw_40 = 1;
		priv->rtllib->chan_offset_bk = chan_offset = ieee->pHTInfo->CurSTAExtChnlOffset;
		priv->rtllib->bandwidth_bk = bandwidth = (HT_CHANNEL_WIDTH)ieee->pHTInfo->bCurBW40MHz;
		printk("before scan force BW to 20M:%d, %d\n", chan_offset, bandwidth);
		ieee->SetBWModeHandler(ieee->dev, HT_CHANNEL_WIDTH_20, HT_EXTCHNL_OFFSET_NO_EXT);
	}
}
void rtl8192se_after_hw_scan(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	HT_EXTCHNL_OFFSET chan_offset = priv->rtllib->chan_offset_bk;
	HT_CHANNEL_WIDTH bandwidth = priv->rtllib->bandwidth_bk;
	struct rtllib_device *ieee = priv->rtllib;

	if(priv->rtllib->state != RTLLIB_LINKED_SCANNING){
		goto out;
	}

	if (ieee->LinkDetectInfo.NumRecvBcnInPeriod == 0 || 
			ieee->LinkDetectInfo.NumRecvDataInPeriod == 0 ) {
		ieee->LinkDetectInfo.NumRecvBcnInPeriod = 1;
		ieee->LinkDetectInfo.NumRecvDataInPeriod= 1;	
	}

	if (priv->hwscan_bw_40) {
		priv->hwscan_bw_40 = 0;
		printk("after scan back BW to 40M:%d, %d\n", chan_offset, bandwidth);
#if 1
		if (chan_offset == HT_EXTCHNL_OFFSET_UPPER){
			ieee->set_chan(ieee->dev, priv->rtllib->hwscan_ch_bk + 2);
		}else if (chan_offset == HT_EXTCHNL_OFFSET_LOWER){
			ieee->set_chan(ieee->dev, priv->rtllib->hwscan_ch_bk - 2);
		}else{
			priv->rtllib->current_network.channel = priv->rtllib->hwscan_ch_bk;
			ieee->set_chan(ieee->dev, priv->rtllib->hwscan_ch_bk);
		}
#endif
		ieee->SetBWModeHandler(ieee->dev, bandwidth, chan_offset);
	} else {
		ieee->set_chan(ieee->dev, priv->rtllib->hwscan_ch_bk);
	}
	
#if 1
	if (ieee->SetFwCmdHandler) {
		ieee->SetFwCmdHandler(ieee->dev, FW_CMD_RESUME_DM_BY_SCAN);
	}
#endif
	ieee->state = RTLLIB_LINKED;
	ieee->link_change(ieee->dev);

#ifdef ENABLE_LPS
	/* Notify AP that I wake up again */
	rtllib_sta_ps_send_null_frame(ieee, 0);
#endif

	if (ieee->data_hard_resume)
		ieee->data_hard_resume(ieee->dev);
	
	if(ieee->iw_mode == IW_MODE_ADHOC 
#ifdef ASL
		|| ((ieee->iw_mode == IW_MODE_MASTER || ieee->iw_mode == IW_MODE_APSTA) && ieee->ap_state == RTLLIB_LINKED)
#endif
	)
		rtllib_start_send_beacons(ieee);
	
	rtllib_wake_all_queues(ieee);
	
out:
	return;
}
/* Service HAL_FW_C2H_CMD_SurveyDone (0x9) */
void rtl8192se_rx_surveydone_cmd(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	

	cancel_delayed_work(&priv->check_hw_scan_wq);

	/* If a request to abort was given, or the scan did not succeed
	 * then we reset the scan state machine and terminate,
	 * re-queuing another scan if one has been requested */
	if (test_bit(STATUS_SCAN_ABORTING, &priv->rtllib->status)) {
		RT_TRACE(COMP_ERR, "Aborted scan completed.\n");
		clear_bit(STATUS_SCAN_ABORTING, &priv->rtllib->status);
	} 
	
	rtl8192se_after_hw_scan(dev);

#ifdef CONFIG_FW_PARSEBEACON
	if(priv->rtllib->state < RTLLIB_LINKED)
	write_nic_dword(dev, RXFILTERMAP, 0x0100);
#endif

	mdelay(1);

	clear_bit(STATUS_SCANNING, &priv->rtllib->status);

	{
		union iwreq_data wrqu;
		memset(&wrqu, 0, sizeof(wrqu));
		wireless_send_event(priv->rtllib->dev,SIOCGIWSCAN,&wrqu,NULL);
	}

	if(priv->rtllib->hwscan_sem_up == 0){
		up(&priv->wx_sem);
		priv->rtllib->hwscan_sem_up = 1;
	}


	return;
}


void	rtl8192se_check_hw_scan(void *data)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
	struct r8192_priv *priv = container_of_dwork_rsl(data,struct r8192_priv,check_hw_scan_wq);
	struct net_device *dev = priv->rtllib->dev;
#else
	struct net_device *dev = (struct net_device *)data;
	struct r8192_priv *priv = rtllib_priv(dev);
#endif
	unsigned long flags;


	spin_lock_irqsave(&priv->fw_scan_lock,flags);
	printk("----------->%s()\n", __func__);

	if (test_bit(STATUS_SCANNING, &priv->rtllib->status) ||
	    test_bit(STATUS_SCAN_ABORTING, &priv->rtllib->status)) {
		printk("FW Scan long time without stop, stop hw scan\n");
		rtl8192se_send_scan_abort(dev);
	}
	

	if(priv->rtllib->hwscan_sem_up == 0){
		up(&priv->wx_sem);
		priv->rtllib->hwscan_sem_up = 1;
	}

	printk("<-----------%s()\n", __func__);
	spin_unlock_irqrestore(&priv->fw_scan_lock,flags);
}

void rtl8192se_start_hw_scan(void *data)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
	struct r8192_priv *priv = container_of_dwork_rsl(data,struct r8192_priv,start_hw_scan_wq);
	struct net_device *dev = priv->rtllib->dev;
#else
	struct net_device *dev = (struct net_device *)data;
	struct r8192_priv *priv = rtllib_priv(dev);
#endif
	bool start_flag =true;
	unsigned long flags;

	down(&priv->wx_sem);
	priv->rtllib->hwscan_sem_up = 0;

	spin_lock_irqsave(&priv->fw_scan_lock,flags);


	cancel_delayed_work(&priv->check_hw_scan_wq);

	/* Make sure the scan wasn't canceled before this queued work
	 * was given the chance to run... */
	if (!test_bit(STATUS_SCANNING, &priv->rtllib->status)){
		RT_TRACE(COMP_ERR,"scan was canceled.");
		goto done;
	}

	if (test_bit(STATUS_SCAN_ABORTING, &priv->rtllib->status)) {
		RT_TRACE(COMP_ERR, "Scan request while abort pending.  Queuing.");
		goto done;
	}
	
	
#ifdef CONFIG_FW_PARSEBEACON
	if(priv->rtllib->state < RTLLIB_LINKED)
	write_nic_dword(dev, RXFILTERMAP, 0x0000);
#endif

	rtl8192se_before_hw_scan(dev);
	rtl8192se_send_scan_cmd(dev,start_flag);

	queue_delayed_work_rsl(priv->priv_wq,&priv->check_hw_scan_wq,MSECS(7000));

	
	spin_unlock_irqrestore(&priv->fw_scan_lock,flags);
	
	return;

 done:
	up(&priv->wx_sem);
	priv->rtllib->hwscan_sem_up = 1;
	/* Cannot perform scan. Make sure we clear scanning
	* bits from status so next scan request can be performed.
	* If we don't clear scanning status bit here all next scan
	* will fail
	*/
	clear_bit(STATUS_SCANNING, &priv->rtllib->status);
	
	/* inform mac80211 scan aborted */
	
	spin_unlock_irqrestore(&priv->fw_scan_lock,flags);

	return;
}

void rtl8192se_hw_scan_initiate(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	unsigned long flags;

	spin_lock_irqsave(&priv->fw_scan_lock,flags);


	if (test_bit(STATUS_SCANNING, &priv->rtllib->status)) {
		printk("Scan already in progress.\n");
		goto done;
	}

	if (test_bit(STATUS_SCAN_ABORTING, &priv->rtllib->status)) {
		printk("Scan request while abort pending\n");
		goto done;
	}

	
	set_bit(STATUS_SCANNING, &priv->rtllib->status);

	queue_delayed_work_rsl(priv->priv_wq,&priv->start_hw_scan_wq,0);

done:
	spin_unlock_irqrestore(&priv->fw_scan_lock,flags);

	return;
}

void rtl8192se_send_scan_abort(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	int ret = true;
	bool start_flag =false;


	/* If there isn't a scan actively going on in the hardware
	 * then we are in between scan bands and not actually
	 * actively scanning, so don't send the abort command */
	if (!test_bit(STATUS_SCANNING, &priv->rtllib->status)) {
		goto done;
	}

	ret = rtl8192se_send_scan_cmd(dev, start_flag);
	mdelay(1); 

#ifdef CONFIG_FW_PARSEBEACON
	if(priv->rtllib->state < RTLLIB_LINKED)
	write_nic_dword(dev, RXFILTERMAP, 0x0100);
#endif

	if (ret) {
		goto done;
	}
	

done:	
	clear_bit(STATUS_SCAN_ABORTING, &priv->rtllib->status);
	clear_bit(STATUS_SCANNING, &priv->rtllib->status);
	
	return;
}

/**
 * rtl8192se_cancel_hw_scan - Cancel any currently executing HW scan
 *
 * NOTE: priv->mutex is not required before calling this function
 */
void rtl8192se_cancel_hw_scan(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	unsigned long flags;
	
	spin_lock_irqsave(&priv->fw_scan_lock,flags);

	if (test_bit(STATUS_SCANNING, &priv->rtllib->status)) {
		if (!test_bit(STATUS_SCAN_ABORTING, &priv->rtllib->status)) {
			printk("====>%s stop HW scan\n", __func__);
			cancel_delayed_work(&priv->check_hw_scan_wq);

			set_bit(STATUS_SCAN_ABORTING, &priv->rtllib->status);
			rtl8192se_send_scan_abort(dev);
		} else {
			printk("-------------->%s()Scan abort already in progress.\n", __func__);
		}
	}

	if(priv->rtllib->hwscan_sem_up == 0){
		up(&priv->wx_sem);
		priv->rtllib->hwscan_sem_up = 1;
	}

	spin_unlock_irqrestore(&priv->fw_scan_lock,flags);
	return;
}

