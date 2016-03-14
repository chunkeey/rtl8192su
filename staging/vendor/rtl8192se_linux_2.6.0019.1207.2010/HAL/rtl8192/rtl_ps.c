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
 *****************************************************************************/
#include "rtl_ps.h"
#include "rtl_core.h"

#if defined(RTL8192E) || defined(RTL8192SE) || defined RTL8192CE
void rtl8192_hw_sleep_down(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	unsigned long flags = 0;
#ifdef CONFIG_ASPM_OR_D3
	PRT_POWER_SAVE_CONTROL	pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));
#endif	
	spin_lock_irqsave(&priv->rf_ps_lock,flags);
	if (priv->RFChangeInProgress) {
		spin_unlock_irqrestore(&priv->rf_ps_lock,flags);
		RT_TRACE(COMP_RF, "rtl8192_hw_sleep_down(): RF Change in progress! \n");
		printk("rtl8192_hw_sleep_down(): RF Change in progress!\n");
		return;
	}
	spin_unlock_irqrestore(&priv->rf_ps_lock,flags);
	RT_TRACE(COMP_PS, "%s()============>come to sleep down\n", __FUNCTION__);

#ifdef CONFIG_RTLWIFI_DEBUGFS	
	if(priv->debug->hw_holding) {
		return;
	}
#endif	
	MgntActSet_RF_State(dev, eRfSleep, RF_CHANGE_BY_PS,false);
#ifdef CONFIG_ASPM_OR_D3
	if(pPSC->RegRfPsLevel & RT_RF_LPS_LEVEL_ASPM)
	{
		RT_ENABLE_ASPM(dev);
		RT_SET_PS_LEVEL(pPSC, RT_RF_LPS_LEVEL_ASPM);
	}
#endif
}

void rtl8192_hw_sleep_wq(void *data)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
	struct rtllib_device *ieee = container_of_dwork_rsl(data,struct rtllib_device,hw_sleep_wq);
	struct net_device *dev = ieee->dev;
#else
	struct net_device *dev = (struct net_device *)data;
#endif
        rtl8192_hw_sleep_down(dev);
}

/* debug macros not dependent on CONFIG_RTLLIB_DEBUG */
bool get_qos_queueID_maskAPSD(	u8 QueueID, u8 APSD)
{
	switch(QueueID)
	{
		case BE_QUEUE:
			return	GET_BE_UAPSD(APSD) ? true : false;

		case BK_QUEUE:
			return	GET_BK_UAPSD(APSD) ? true : false;

		case VI_QUEUE:
			return	GET_VI_UAPSD(APSD) ? true : false;

		case VO_QUEUE:
			return	GET_VO_UAPSD(APSD) ? true : false;

		default:
			return false;
	}

	return false;
}
void rtl8192_hw_wakeup(struct net_device* dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	unsigned long flags = 0;
	u8 queue_index = 0;
	unsigned long queue_len = 0;
	struct sk_buff* skb = NULL;
#ifdef CONFIG_ASPM_OR_D3
	PRT_POWER_SAVE_CONTROL	pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));
#endif	
	spin_lock_irqsave(&priv->rf_ps_lock,flags);
	if (priv->RFChangeInProgress) {
		spin_unlock_irqrestore(&priv->rf_ps_lock,flags);
		RT_TRACE(COMP_RF, "rtl8192_hw_wakeup(): RF Change in progress! \n");
		printk("rtl8192_hw_wakeup(): RF Change in progress! schedule wake up task again\n");
		queue_delayed_work_rsl(priv->rtllib->wq,&priv->rtllib->hw_wakeup_wq,MSECS(10));
		return;
	}
	spin_unlock_irqrestore(&priv->rf_ps_lock,flags);
#ifdef CONFIG_ASPM_OR_D3
	if (pPSC->RegRfPsLevel & RT_RF_LPS_LEVEL_ASPM) {
		RT_DISABLE_ASPM(dev);
		RT_CLEAR_PS_LEVEL(pPSC, RT_RF_LPS_LEVEL_ASPM);
	}
#endif
	RT_TRACE(COMP_PS, "%s()============>come to wake up\n", __FUNCTION__);
	MgntActSet_RF_State(dev, eRfOn, RF_CHANGE_BY_PS,false);
	for (queue_index=0; queue_index<=VO_QUEUE; queue_index++){
		
		queue_len = skb_queue_len(&priv->rtllib->skb_waitQ[queue_index]);
		
		if (queue_len) {
			printk("=====>%s(): no need to ps,wake up!! %d queue is not empty\n",
				__FUNCTION__,queue_index);
			skb = skb_dequeue(&priv->rtllib->skb_waitQ[queue_index]);
			if (skb == NULL) {
				RT_TRACE(COMP_ERR,"rtl8192_hw_wakeup():skb is NULL\n");
				return;
			}
			if ((priv->rtllib->b4ac_Uapsd & 0x0f) !=0) {
				if ( IsMgntQosData(skb->data) || IsMgntQosNull(skb->data) ) { 
					if(!priv->rtllib->bin_service_period && get_qos_queueID_maskAPSD(queue_index, priv->rtllib->b4ac_Uapsd))
					{
						RT_TRACE(COMP_PS, "rtl8192_hw_wakeup(): >>>>>>>>>> Enter APSD service period >>>>>>>>>>\n");
						printk("rtl8192_hw_wakeup(): >>>>>>>>>> Enter APSD service period >>>>>>>>>>\n");
						priv->rtllib->bin_service_period = true;
					}
				}
			}
			priv->rtllib->softmac_data_hard_start_xmit(skb,dev,0/* rate useless now*/);
		}
	}
}

void rtl8192_hw_wakeup_wq(void *data)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
	struct rtllib_device *ieee = container_of_dwork_rsl(data,struct rtllib_device,hw_wakeup_wq);  
	struct net_device *dev = ieee->dev;
#else
	struct net_device *dev = (struct net_device *)data;
#endif
	rtl8192_hw_wakeup(dev);

}

#define MIN_SLEEP_TIME 50
#define MAX_SLEEP_TIME 10000
void rtl8192_hw_to_sleep(struct net_device *dev, u32 th, u32 tl)
{
#ifdef _RTL8192_EXT_PATCH_
	struct r8192_priv *priv = rtllib_priv(dev);
	u32 rb = jiffies, sleep_cost = MSECS(8+16+7), delta = 0;
	unsigned long flags;

	if((tl > rb) && (th > 0))
		return;

	spin_lock_irqsave(&priv->ps_lock,flags);

	if (tl >= sleep_cost)
		tl -= sleep_cost;
	else if (th > 0) {
		tl = 0xffffffff - sleep_cost + tl;  
		th--;
	} else {
		spin_unlock_irqrestore(&priv->ps_lock,flags);
		return;
	}

	if (tl > rb) {
		delta = tl - rb;
	} else if (th > 0) {
		delta = 0xffffffff - rb + tl;
		th --;
	} else {
		spin_unlock_irqrestore(&priv->ps_lock,flags);
		return;
	}

	if (delta <= MSECS(MIN_SLEEP_TIME)) {
		spin_unlock_irqrestore(&priv->ps_lock,flags);
		printk("too short to sleep::%x, %x, %lx\n",tl, rb,  MSECS(MIN_SLEEP_TIME));
		return;
	}	

	if(delta > MSECS(MAX_SLEEP_TIME)) {
		spin_unlock_irqrestore(&priv->ps_lock,flags);
		printk("========>too long to sleep:%x, %x, %lx\n", tl, rb,  MSECS(MAX_SLEEP_TIME));
		return;
	}

	RT_TRACE(COMP_LPS, "==============>%s(): wake up time is %d,%d\n",__FUNCTION__,delta,jiffies_to_msecs(delta));
	queue_delayed_work_rsl(priv->rtllib->wq,&priv->rtllib->hw_wakeup_wq,delta); 
	queue_delayed_work_rsl(priv->rtllib->wq, (void *)&priv->rtllib->hw_sleep_wq,0);

	spin_unlock_irqrestore(&priv->ps_lock,flags);
#else
	struct r8192_priv *priv = rtllib_priv(dev);

	u32 rb = jiffies;
	unsigned long flags;

	spin_lock_irqsave(&priv->ps_lock,flags);

	tl -= MSECS(8+16+7);

	if(((tl>=rb)&& (tl-rb) <= MSECS(MIN_SLEEP_TIME))
			||((rb>tl)&& (rb-tl) < MSECS(MIN_SLEEP_TIME))) {
		spin_unlock_irqrestore(&priv->ps_lock,flags);
		printk("too short to sleep::%x, %x, %lx\n",tl, rb,  MSECS(MIN_SLEEP_TIME));
		return;
	}	

	if(((tl > rb) && ((tl-rb) > MSECS(MAX_SLEEP_TIME)))||
			((tl < rb) && (tl>MSECS(69)) && ((rb-tl) > MSECS(MAX_SLEEP_TIME)))||
			((tl<rb)&&(tl<MSECS(69))&&((tl+0xffffffff-rb)>MSECS(MAX_SLEEP_TIME)))) {
		printk("========>too long to sleep:%x, %x, %lx\n", tl, rb,  MSECS(MAX_SLEEP_TIME));
		spin_unlock_irqrestore(&priv->ps_lock,flags);
		return;
	}
	{
		u32 tmp = (tl>rb)?(tl-rb):(rb-tl);
		queue_delayed_work_rsl(priv->rtllib->wq,
				&priv->rtllib->hw_wakeup_wq,tmp); 
	}
	queue_delayed_work_rsl(priv->rtllib->wq, 
			(void *)&priv->rtllib->hw_sleep_wq,0);
	spin_unlock_irqrestore(&priv->ps_lock,flags);
#endif
}
#endif

#ifdef ENABLE_IPS
void InactivePsWorkItemCallback(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PRT_POWER_SAVE_CONTROL	pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));

	RT_TRACE(COMP_PS, "InactivePsWorkItemCallback() ---------> \n");			
	pPSC->bSwRfProcessing = true;

	RT_TRACE(COMP_PS, "InactivePsWorkItemCallback(): Set RF to %s.\n", \
			pPSC->eInactivePowerState == eRfOff?"OFF":"ON");
#ifdef CONFIG_ASPM_OR_D3
	if(pPSC->eInactivePowerState == eRfOn)
	{

		if((pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_ASPM) && RT_IN_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_ASPM))
		{
			RT_DISABLE_ASPM(dev);
			RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_ASPM);
		}
#ifdef TODO		
		else if((pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_PCI_D3) && RT_IN_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_PCI_D3))
		{
			RT_LEAVE_D3(dev, false);
			RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_PCI_D3);
		}
#endif		
	}
#endif
	MgntActSet_RF_State(dev, pPSC->eInactivePowerState, RF_CHANGE_BY_IPS,false);

#ifdef CONFIG_ASPM_OR_D3
	if(pPSC->eInactivePowerState == eRfOff)
	{
		if(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_ASPM)
		{
			RT_ENABLE_ASPM(dev);
			RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_ASPM);
		}
#ifdef TODO		
		else if(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_PCI_D3)
		{
			RT_ENTER_D3(dev, false);
			RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_PCI_D3);
		}
#endif		
	}
#endif
	
#if 0
	if(pPSC->eInactivePowerState == eRfOn)
	{
		while( index < 4 )
		{
			if( ( pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == WEP104_Encryption ) ||
				(pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == WEP40_Encryption) )
			{
				if( pMgntInfo->SecurityInfo.KeyLen[index] != 0)
				pAdapter->HalFunc.SetKeyHandler(pAdapter, index, 0, false, pMgntInfo->SecurityInfo.PairwiseEncAlgorithm, true, false);

			}
			index++;
		}
	}
#endif
	pPSC->bSwRfProcessing = false;	
	RT_TRACE(COMP_PS, "InactivePsWorkItemCallback() <--------- \n");			
}

void
IPSEnter(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PRT_POWER_SAVE_CONTROL		pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));
	RT_RF_POWER_STATE 			rtState;

	if (pPSC->bInactivePs)
	{
		rtState = priv->rtllib->eRFPowerState;
		if (rtState == eRfOn && !pPSC->bSwRfProcessing &&\
			(priv->rtllib->state != RTLLIB_LINKED)
#ifdef ASL
			&&(priv->rtllib->iw_mode != IW_MODE_MASTER)
			&&(priv->rtllib->iw_mode != IW_MODE_APSTA)
#endif
			)
		{
			RT_TRACE(COMP_PS,"IPSEnter(): Turn off RF.\n");
			pPSC->eInactivePowerState = eRfOff;
			priv->isRFOff = true;
			priv->bInPowerSaveMode = true;
			InactivePsWorkItemCallback(dev);
		}
	}	
}

void
IPSLeave(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PRT_POWER_SAVE_CONTROL	pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));
	RT_RF_POWER_STATE 	rtState;

	if (pPSC->bInactivePs)
	{	
		rtState = priv->rtllib->eRFPowerState;	
		if (rtState != eRfOn  && !pPSC->bSwRfProcessing && priv->rtllib->RfOffReason <= RF_CHANGE_BY_IPS)
		{
			RT_TRACE(COMP_PS, "IPSLeave(): Turn on RF.\n");
			pPSC->eInactivePowerState = eRfOn;
			priv->bInPowerSaveMode = false;
			InactivePsWorkItemCallback(dev);
		}
	}
}
void IPSLeave_wq(void *data)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
	struct rtllib_device *ieee = container_of_work_rsl(data,struct rtllib_device,ips_leave_wq);
	struct net_device *dev = ieee->dev;
#else
	struct net_device *dev = (struct net_device *)data;
#endif
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	down(&priv->rtllib->ips_sem);
	IPSLeave(dev);	
	up(&priv->rtllib->ips_sem);	
}
void rtllib_ips_leave_wq(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	RT_RF_POWER_STATE	rtState;
	rtState = priv->rtllib->eRFPowerState;

	if(priv->rtllib->PowerSaveControl.bInactivePs){ 
		if(rtState == eRfOff){
			if(priv->rtllib->RfOffReason > RF_CHANGE_BY_IPS)
			{
				RT_TRACE(COMP_ERR, "%s(): RF is OFF.\n",__FUNCTION__);
				return;
			}
			else{
				printk("=========>%s(): IPSLeave\n",__FUNCTION__);
				queue_work_rsl(priv->rtllib->wq,&priv->rtllib->ips_leave_wq);				
			}
		}
	}
}
void rtllib_ips_leave(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	down(&priv->rtllib->ips_sem);
	IPSLeave(dev);	
	up(&priv->rtllib->ips_sem);	
}
#endif

#ifdef ENABLE_LPS
bool MgntActSet_802_11_PowerSaveMode(struct net_device *dev,	u8 rtPsMode)
{
	struct r8192_priv *priv = rtllib_priv(dev);

#ifdef _RTL8192_EXT_PATCH_
	if((priv->rtllib->iw_mode == IW_MODE_ADHOC) || (priv->rtllib->iw_mode == IW_MODE_MASTER)
		|| (priv->rtllib->iw_mode == IW_MODE_MESH))	
#else
#ifdef ASL
	if((priv->rtllib->iw_mode == IW_MODE_ADHOC) || (priv->rtllib->iw_mode == IW_MODE_MASTER) || (priv->rtllib->iw_mode == IW_MODE_APSTA))
#else
	if(priv->rtllib->iw_mode == IW_MODE_ADHOC)	
#endif
#endif
	{
		return false;
	}

	
	RT_TRACE(COMP_LPS,"%s(): set ieee->ps = %x\n",__FUNCTION__,rtPsMode);
	if(!priv->ps_force) {
		priv->rtllib->ps = rtPsMode;
	}
#if 0
	priv->rtllib->dot11PowerSaveMode = rtPsMode;

	if(priv->rtllib->dot11PowerSaveMode == eMaxPs)
	{
	}
	else
	{
	}
#endif
	if(priv->rtllib->sta_sleep != LPS_IS_WAKE && rtPsMode == RTLLIB_PS_DISABLED)
	{
                unsigned long flags;

		rtl8192_hw_wakeup(dev);
		priv->rtllib->sta_sleep = LPS_IS_WAKE;

                spin_lock_irqsave(&(priv->rtllib->mgmt_tx_lock), flags);
		printk("LPS leave: notify AP we are awaked ++++++++++ SendNullFunctionData\n");
		rtllib_sta_ps_send_null_frame(priv->rtllib, 0);
                spin_unlock_irqrestore(&(priv->rtllib->mgmt_tx_lock), flags);
	} else if (rtPsMode != RTLLIB_PS_DISABLED) {
                unsigned long flags;
		spin_lock_irqsave(&(priv->rtllib->mgmt_tx_lock), flags);
		printk("LPS Enter: notify AP we are dozing ++++++++++ SendNullFunctionData\n");
		rtllib_sta_ps_send_null_frame(priv->rtllib, true);
		spin_unlock_irqrestore(&(priv->rtllib->mgmt_tx_lock), flags);
	}

#if 0
	if((pPSC->bFwCtrlLPS) && (pPSC->bLeisurePs))
	{	
		if(priv->rtllib->dot11PowerSaveMode == eActive)
		{
			RpwmVal = 0x0C; 
			FwPwrMode = FW_PS_ACTIVE_MODE;
			Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_SET_RPWM, (pu1Byte)(&RpwmVal));
			Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_H2C_FW_PWRMODE, (pu1Byte)(&FwPwrMode));
		}
		else
		{
			if(GetFwLPS_Doze(Adapter))
			{
				RpwmVal = 0x02; 
				Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_H2C_FW_PWRMODE, (pu1Byte)(&pPSC->FWCtrlPSMode));
				Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_SET_RPWM, (pu1Byte)(&RpwmVal));
			}
			else
			{
				pMgntInfo->dot11PowerSaveMode = eActive;
				Adapter->bInPowerSaveMode = false;	
			}
		}
	}
#endif
	return true;
}


void LeisurePSEnter(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PRT_POWER_SAVE_CONTROL pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));

	RT_TRACE(COMP_PS, "LeisurePSEnter()...\n");
	RT_TRACE(COMP_PS, "pPSC->bLeisurePs = %d, ieee->ps = %d,pPSC->LpsIdleCount is %d,RT_CHECK_FOR_HANG_PERIOD is %d\n", 
		pPSC->bLeisurePs, priv->rtllib->ps,pPSC->LpsIdleCount,RT_CHECK_FOR_HANG_PERIOD);

#ifdef _RTL8192_EXT_PATCH_
	if(!((priv->rtllib->iw_mode == IW_MODE_INFRA) && (priv->rtllib->state == RTLLIB_LINKED))
		|| (priv->rtllib->iw_mode == IW_MODE_ADHOC) || (priv->rtllib->iw_mode == IW_MODE_MASTER)
		|| (priv->rtllib->iw_mode == IW_MODE_MESH))
#else
#ifdef ASL
	if(!((priv->rtllib->iw_mode == IW_MODE_INFRA) && (priv->rtllib->state == RTLLIB_LINKED))
		|| (priv->rtllib->iw_mode == IW_MODE_ADHOC) || (priv->rtllib->iw_mode == IW_MODE_MASTER) || (priv->rtllib->iw_mode == IW_MODE_APSTA))
#else
	if(!((priv->rtllib->iw_mode == IW_MODE_INFRA) && (priv->rtllib->state == RTLLIB_LINKED))
		|| (priv->rtllib->iw_mode == IW_MODE_ADHOC) || (priv->rtllib->iw_mode == IW_MODE_MASTER))
#endif
#endif
		return;

	if (pPSC->bLeisurePs)
	{
		if(pPSC->LpsIdleCount >= RT_CHECK_FOR_HANG_PERIOD) 
		{
	
			if(priv->rtllib->ps == RTLLIB_PS_DISABLED)
			{

				RT_TRACE(COMP_LPS, "LeisurePSEnter(): Enter 802.11 power save mode...\n");

				if(!pPSC->bFwCtrlLPS)
				{
					if (priv->rtllib->SetFwCmdHandler)
					{
						priv->rtllib->SetFwCmdHandler(dev, FW_CMD_LPS_ENTER);
					} 
				}	
				MgntActSet_802_11_PowerSaveMode(dev, RTLLIB_PS_MBCAST|RTLLIB_PS_UNICAST);

				/*if(pPSC->RegRfPsLevel & RT_RF_LPS_LEVEL_ASPM)
				{
					RT_ENABLE_ASPM(pAdapter);
					RT_SET_PS_LEVEL(pAdapter, RT_RF_LPS_LEVEL_ASPM);
				}*/

			}	
		}
		else
			pPSC->LpsIdleCount++;
	}	
}


void LeisurePSLeave(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PRT_POWER_SAVE_CONTROL pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));


	RT_TRACE(COMP_PS, "LeisurePSLeave()...\n");
	RT_TRACE(COMP_PS, "pPSC->bLeisurePs = %d, ieee->ps = %d\n", 
		pPSC->bLeisurePs, priv->rtllib->ps);

	if (pPSC->bLeisurePs)
	{	
		if(priv->rtllib->ps != RTLLIB_PS_DISABLED)
		{
#ifdef CONFIG_ASPM_OR_D3
			if(pPSC->RegRfPsLevel & RT_RF_LPS_LEVEL_ASPM && RT_IN_PS_LEVEL(pPSC, RT_RF_LPS_LEVEL_ASPM))
			{
				RT_DISABLE_ASPM(dev);
				RT_CLEAR_PS_LEVEL(pPSC, RT_RF_LPS_LEVEL_ASPM);
			} 
#endif
			RT_TRACE(COMP_LPS, "LeisurePSLeave(): Busy Traffic , Leave 802.11 power save..\n");
			MgntActSet_802_11_PowerSaveMode(dev, RTLLIB_PS_DISABLED);

			if(!pPSC->bFwCtrlLPS) 
			{
				if (priv->rtllib->SetFwCmdHandler)
				{
					priv->rtllib->SetFwCmdHandler(dev, FW_CMD_LPS_LEAVE);
				} 
                    }
		}
	}
}
#endif

#ifdef CONFIG_ASPM_OR_D3

void
PlatformDisableHostL0s(struct net_device *dev)
{
	struct r8192_priv 	*priv = (struct r8192_priv *)rtllib_priv(dev);
	u32				PciCfgAddrPort=0;
	u8				Num4Bytes;
	u8				uPciBridgeASPMSetting = 0;

	
	if( (priv->NdisAdapter.BusNumber == 0xff && 
		priv->NdisAdapter.DevNumber == 0xff && 
		priv->NdisAdapter.FuncNumber == 0xff) ||
		(priv->NdisAdapter.PciBridgeBusNum == 0xff && 
		priv->NdisAdapter.PciBridgeDevNum == 0xff && 
		priv->NdisAdapter.PciBridgeFuncNum == 0xff) )
	{
		printk("PlatformDisableHostL0s(): Fail to enable ASPM. "
		"Cannot find the Bus of PCI(Bridge).\n");
		return;
	}
	
	PciCfgAddrPort= (priv->NdisAdapter.PciBridgeBusNum << 16)|
			(priv->NdisAdapter.PciBridgeDevNum<< 11)|
			(priv->NdisAdapter.PciBridgeFuncNum <<  8)|(1 << 31);
	Num4Bytes = (priv->NdisAdapter.PciBridgePCIeHdrOffset+0x10)/4;


	NdisRawWritePortUlong(PCI_CONF_ADDRESS , PciCfgAddrPort+(Num4Bytes << 2));

	NdisRawReadPortUchar(PCI_CONF_DATA, &uPciBridgeASPMSetting);

	if(uPciBridgeASPMSetting & BIT0)
		uPciBridgeASPMSetting &=  ~(BIT0);

	NdisRawWritePortUlong(PCI_CONF_ADDRESS , PciCfgAddrPort+(Num4Bytes << 2));
	NdisRawWritePortUchar(PCI_CONF_DATA, uPciBridgeASPMSetting);

	udelay(50);

	printk("PlatformDisableHostL0s():PciBridge BusNumber[%x], "
		"DevNumbe[%x], FuncNumber[%x], Write reg[%x] = %x\n",
		priv->NdisAdapter.PciBridgeBusNum, 
		priv->NdisAdapter.PciBridgeDevNum, 
		priv->NdisAdapter.PciBridgeFuncNum, 
		(priv->NdisAdapter.PciBridgePCIeHdrOffset+0x10), 
		(priv->NdisAdapter.PciBridgeLinkCtrlReg | 
		(priv->RegDevicePciASPMSetting&~BIT0)));
}

bool PlatformEnable92CEBackDoor(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	bool			bResult = true;
	u8			value;

	if( (priv->NdisAdapter.BusNumber == 0xff && 
		priv->NdisAdapter.DevNumber == 0xff && 
		priv->NdisAdapter.FuncNumber == 0xff) ||
		(priv->NdisAdapter.PciBridgeBusNum == 0xff && 
		priv->NdisAdapter.PciBridgeDevNum == 0xff && 
		priv->NdisAdapter.PciBridgeFuncNum == 0xff) )
	{
		RT_TRACE(COMP_INIT, "PlatformEnableASPM(): Fail to enable ASPM. "
				"Cannot find the Bus of PCI(Bridge).\n");
		return false;
	}
	
	pci_read_config_byte(priv->pdev, 0x70f, &value);
	
	if(priv->NdisAdapter.PciBridgeVendor == PCI_BRIDGE_VENDOR_INTEL) {
		value |= BIT7;
	} else {
		value = 0x23;
	}
	
	pci_write_config_byte(priv->pdev, 0x70f, value);


	pci_read_config_byte(priv->pdev, 0x719, &value);
	value |= (BIT3|BIT4);
	pci_write_config_byte(priv->pdev, 0x719, value);
	

	return bResult;
}

bool PlatformSwitchDevicePciASPM(struct net_device *dev, u8 value)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	bool bResult = false;

#ifdef RTL8192CE
	value |= 0x40;
#endif

	pci_write_config_byte(priv->pdev, 0x80, value);

#ifdef RTL8192SE
	udelay(100);
#endif

	return bResult;
}

bool PlatformSwitchClkReq(struct net_device *dev, u8 value)
{
	bool bResult = false;
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	u8	Buffer;

	Buffer= value;	
	
	pci_write_config_byte(priv->pdev,0x81,value);
	bResult = true;
	
#ifdef TODO
	if(Buffer) {
		priv->ClkReqState = true;
	} else {
		priv->ClkReqState = false;
	}
#endif

#ifdef RTL8192SE
	udelay(100);
#endif

	return bResult;
}

void PlatformDisableASPM(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	PRT_POWER_SAVE_CONTROL	pPSC = 
		(PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));
	u32	PciCfgAddrPort=0;
	u8	Num4Bytes;
	u8	LinkCtrlReg;
	u16	PciBridgeLinkCtrlReg, ASPMLevel=0;

	if( (priv->NdisAdapter.BusNumber == 0xff && 
		priv->NdisAdapter.DevNumber == 0xff && 
		priv->NdisAdapter.FuncNumber == 0xff) ||
		(priv->NdisAdapter.PciBridgeBusNum == 0xff && 
		priv->NdisAdapter.PciBridgeDevNum == 0xff && 
		priv->NdisAdapter.PciBridgeFuncNum == 0xff) ) {
		RT_TRACE(COMP_INIT, "PlatformEnableASPM(): Fail to enable ASPM."
				" Cannot find the Bus of PCI(Bridge).\n");
		return;
	}

#ifdef RTL8192SE
	if(priv->NdisAdapter.PciBridgeVendor != PCI_BRIDGE_VENDOR_INTEL) {
		RT_TRACE(COMP_POWER, "%s(): Dont modify ASPM for non intel "
				"chipset. For Bridge Vendor %d.\n"
				,__func__,priv->NdisAdapter.PciBridgeVendor);
		return;
	}
#endif

#ifdef RTL8192CE
	if(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_CLK_REQ) {
		RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_CLK_REQ);
		PlatformSwitchClkReq(dev, 0x0);
	}	

	{
		u8	tmpU1b;	
		pci_read_config_byte(priv->pdev, 0x80, &tmpU1b);		
	}
#endif

	LinkCtrlReg = priv->NdisAdapter.LinkCtrlReg;
	PciBridgeLinkCtrlReg = priv->NdisAdapter.PciBridgeLinkCtrlReg;

	ASPMLevel |= BIT0|BIT1;
	LinkCtrlReg &=~ASPMLevel;
	PciBridgeLinkCtrlReg &=~(BIT0|BIT1);

#ifdef RTL8192CE
	PlatformSwitchDevicePciASPM(dev, LinkCtrlReg);
	udelay( 50);
#endif

	PciCfgAddrPort= (priv->NdisAdapter.PciBridgeBusNum << 16)|
			(priv->NdisAdapter.PciBridgeDevNum<< 11)|
			(priv->NdisAdapter.PciBridgeFuncNum <<  8)|(1 << 31);
	Num4Bytes = (priv->NdisAdapter.PciBridgePCIeHdrOffset+0x10)/4;
	NdisRawWritePortUlong(PCI_CONF_ADDRESS , PciCfgAddrPort+(Num4Bytes << 2));
	NdisRawWritePortUchar(PCI_CONF_DATA, PciBridgeLinkCtrlReg);	
	RT_TRACE(COMP_POWER, "%s:PciBridge BusNumber[%x], "
			"DevNumbe[%x], FuncNumber[%x], Write reg[%x] = %x\n", __func__,
			priv->NdisAdapter.PciBridgeBusNum, 
			priv->NdisAdapter.PciBridgeDevNum, 
			priv->NdisAdapter.PciBridgeFuncNum, 
			(priv->NdisAdapter.PciBridgePCIeHdrOffset+0x10), 
			PciBridgeLinkCtrlReg);
	udelay(50);

#ifdef RTL8192SE
	PlatformSwitchDevicePciASPM(dev, LinkCtrlReg);
	PlatformSwitchClkReq(dev, 0x0);
	if (pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_CLK_REQ)
		RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_CLK_REQ);
	udelay(100);
#endif

}

void PlatformEnableASPM(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	PRT_POWER_SAVE_CONTROL pPSC = 
		(PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));
	u16	ASPMLevel = 0;
	u32	PciCfgAddrPort=0;
	u8	Num4Bytes;
	u8	uPciBridgeASPMSetting = 0;
	u8	uDeviceASPMSetting = 0;

	if( (priv->NdisAdapter.BusNumber == 0xff && 
		priv->NdisAdapter.DevNumber == 0xff && 
		priv->NdisAdapter.FuncNumber == 0xff) || 
		(priv->NdisAdapter.PciBridgeBusNum == 0xff && 
		priv->NdisAdapter.PciBridgeDevNum == 0xff && 
		priv->NdisAdapter.PciBridgeFuncNum == 0xff) ) {
		RT_TRACE(COMP_INIT, "PlatformEnableASPM(): Fail to enable ASPM."
				" Cannot find the Bus of PCI(Bridge).\n");
		return;
	}

#ifdef RTL8192SE
	if(priv->NdisAdapter.PciBridgeVendor != PCI_BRIDGE_VENDOR_INTEL) {
		RT_TRACE(COMP_POWER, "%s(): Dont modify ASPM for non intel "
				"chipset. For Bridge Vendor %d.\n"
				,__func__,priv->NdisAdapter.PciBridgeVendor);
		return;
	}
#endif

#ifdef RTL8192CE
	PciCfgAddrPort= (priv->NdisAdapter.PciBridgeBusNum << 16)|
			(priv->NdisAdapter.PciBridgeDevNum<< 11) |
			(priv->NdisAdapter.PciBridgeFuncNum <<  8)|(1 << 31);
	Num4Bytes = (priv->NdisAdapter.PciBridgePCIeHdrOffset+0x10)/4;
	NdisRawWritePortUlong(PCI_CONF_ADDRESS , PciCfgAddrPort+(Num4Bytes << 2));
	uPciBridgeASPMSetting = priv->NdisAdapter.PciBridgeLinkCtrlReg |
			priv->RegHostPciASPMSetting;
	
	if(priv->NdisAdapter.PciBridgeVendor == PCI_BRIDGE_VENDOR_INTEL)
		uPciBridgeASPMSetting &=  ~ BIT0;

	NdisRawWritePortUchar(PCI_CONF_DATA, uPciBridgeASPMSetting);
	RT_TRACE(COMP_INIT, "PlatformEnableASPM():PciBridge BusNumber[%x], "
		"DevNumbe[%x], FuncNumber[%x], Write reg[%x] = %x\n",
		priv->NdisAdapter.PciBridgeBusNum, 
		priv->NdisAdapter.PciBridgeDevNum, 
		priv->NdisAdapter.PciBridgeFuncNum, 
		(priv->NdisAdapter.PciBridgePCIeHdrOffset+0x10), 
		uPciBridgeASPMSetting);

	udelay(50);
#endif

	ASPMLevel |= priv->RegDevicePciASPMSetting;
	uDeviceASPMSetting = priv->NdisAdapter.LinkCtrlReg;


#ifdef RTL8192SE
	if(priv->CustomerID == RT_CID_TOSHIBA && 
		priv->NdisAdapter.PciBridgeVendor == PCI_BRIDGE_VENDOR_AMD) {
		if(priv->NdisAdapter.LinkCtrlReg & BIT1)
			uDeviceASPMSetting |= BIT0;
		else 
			uDeviceASPMSetting |= ASPMLevel;			
	}
	else
#endif
		uDeviceASPMSetting |= ASPMLevel;
	
	PlatformSwitchDevicePciASPM(dev, uDeviceASPMSetting);

	if (pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_CLK_REQ) {
		PlatformSwitchClkReq(dev,(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_CLK_REQ) ? 1 : 0);
		RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_CLK_REQ);
	}
	udelay(100);

#ifdef RTL8192SE
	PciCfgAddrPort= (priv->NdisAdapter.PciBridgeBusNum << 16)|
			(priv->NdisAdapter.PciBridgeDevNum<< 11)|
			(priv->NdisAdapter.PciBridgeFuncNum <<  8)|(1 << 31);
	Num4Bytes = (priv->NdisAdapter.PciBridgePCIeHdrOffset+0x10)/4;
	NdisRawWritePortUlong(PCI_CONF_ADDRESS , PciCfgAddrPort+(Num4Bytes << 2));

	uPciBridgeASPMSetting = priv->NdisAdapter.PciBridgeLinkCtrlReg |
			priv->RegHostPciASPMSetting;
	
	if(priv->CustomerID == RT_CID_TOSHIBA && 
		priv->NdisAdapter.PciBridgeVendor == PCI_BRIDGE_VENDOR_AMD 
		&& pPSC->RegAMDPciASPM) {
		if(priv->NdisAdapter.PciBridgeLinkCtrlReg & BIT1)
			uPciBridgeASPMSetting |= BIT0;
	} else if(priv->NdisAdapter.PciBridgeVendor == PCI_BRIDGE_VENDOR_INTEL ) {
		uPciBridgeASPMSetting &=  ~ BIT0;
	}

	NdisRawWritePortUchar(PCI_CONF_DATA, uPciBridgeASPMSetting);
	RT_TRACE(COMP_INIT, "%s:PciBridge BusNumber[%x], DevNumbe[%x], "
		"FuncNumber[%x], Write reg[%x] = %x\n",__func__,
		priv->NdisAdapter.PciBridgeBusNum, 
		priv->NdisAdapter.PciBridgeDevNum, 
		priv->NdisAdapter.PciBridgeFuncNum, 
		(priv->NdisAdapter.PciBridgePCIeHdrOffset+0x10), 
		uPciBridgeASPMSetting);
#endif

	udelay(100);
}

u32 PlatformResetPciSpace(struct net_device *dev,u8 Value)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);

	pci_write_config_byte(priv->pdev,0x04,Value);	

	return 1;
	
}
bool PlatformSetPMCSR(struct net_device *dev,u8 value,bool bTempSetting)
{
	bool bResult = false;
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	u8  Buffer;
	bool bActuallySet=false;
	unsigned long flag;

	Buffer= value;
	spin_lock_irqsave(&priv->D3_lock,flag);

	if (bActuallySet) {
		if (Buffer) {
			PlatformSwitchClkReq(dev, 0x01);
		} else {
			PlatformSwitchClkReq(dev, 0x00);
		}
		
		pci_write_config_byte(priv->pdev,0x44,Buffer);
		RT_TRACE(COMP_POWER, "PlatformSetPMCSR(): D3(value: %d)\n", Buffer);

		bResult = true;
		if (!Buffer) {
			PlatformResetPciSpace(dev, 0x06);
			PlatformResetPciSpace(dev, 0x07);
		}
		
	}
	spin_unlock_irqrestore(&priv->D3_lock,flag);
	return bResult;
}
#endif
