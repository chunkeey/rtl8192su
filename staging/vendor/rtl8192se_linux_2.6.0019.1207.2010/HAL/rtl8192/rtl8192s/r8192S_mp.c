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

#ifdef CONFIG_MP
#include "../rtl_core.h"

void rtl8192_init_mp(struct net_device* dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	priv->chan_forced = false;

	priv->bSingleCarrier = false;
	priv->RegBoard = 0;
	priv->bCckContTx = false; 
	priv->bOfdmContTx = false; 
	priv->bStartContTx = false; 
	priv->RegPaModel = 0; 
	priv->btMpCckTxPower = 0; 
	priv->btMpOfdmTxPower = 0; 
}

static bool r8192_MgntIsRateValidForWirelessMode(u8 rate, u8 wirelessmode)
{
	bool bReturn = false;

	switch(wirelessmode)
	{
	case WIRELESS_MODE_A:
	case WIRELESS_MODE_N_5G:
		if((rate >= 12) && (rate <= 108) && (rate != 22) && (rate != 44))
		{
			bReturn = true;
		}
		break;

	case WIRELESS_MODE_B:
		if( ((rate <= 22) && (rate != 12) && (rate != 18)) || 
			(rate == 44) )
		{
			bReturn = true;
		}
		break;

	case WIRELESS_MODE_G:
	case WIRELESS_MODE_G | WIRELESS_MODE_B:
	case WIRELESS_MODE_N_24G:
		if((rate >= 2) && (rate <= 108))
		{
			bReturn = true;
		}
		break;

	case WIRELESS_MODE_AUTO:
		printk("MgntIsRateValidForWirelessMode(): wirelessmode should not be WIRELESS_MODE_AUTO\n");
		break;

	default:
		printk("MgntIsRateValidForWirelessMode(): Unknown wirelessmode: %d\n", wirelessmode);
		break;
	}

	if(!bReturn)
	{
		if(wirelessmode&(WIRELESS_MODE_N_24G|WIRELESS_MODE_N_5G))
		{
			if((rate>=0x80) && (rate<=MGN_MCS15_SG)) 
				bReturn = true;
		}
	}
	return bReturn;
}

inline u8 r8192_is_wireless_b_mode(u16 rate)
{
	if( ((rate <= 110) && (rate != 60) && (rate != 90)) || (rate == 220) )
		return 1;
	else return 0;
}

static void r8192_XmitOnePacket(struct net_device* dev)
{
	struct r8192_priv* priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;
	struct sk_buff* skb = rtllib_get_beacon(ieee);
	
	if (unlikely(!skb)){
		printk("========>error alloc skb\n");
		return;
	}

	priv->rtllib->softmac_data_hard_start_xmit(skb, dev, ieee->rate);
}

int r8192_wx_mp_set_chan(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;
        int ret = -1;
	u8 channel;

	down(&priv->wx_sem);

	channel = *extra;
	
	rtllib_stop_scan(ieee);

        printk("####RTL819X MP####set channel[1-11] %d\n",channel);

	if((channel > 11) || (channel < 1)) {
	up(&priv->wx_sem);
       	 	return ret;
	}

	priv->rtllib->current_network.channel = channel;
	priv->MptChannelToSw = channel;	

	priv->chan_forced = false;
	MPT_ProSwChannel(dev);
	priv->chan_forced = true;

	ret = 0;
        up(&priv->wx_sem);
		
        return ret;
	 
}

int r8192_wx_mp_set_txrate(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
        struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;
	int ret = -1;
	u32 data_rate_index = 0;

        down(&priv->wx_sem);
             
	data_rate_index = *extra;

	printk("####RTL819X MP####set tx rate index %d\n",data_rate_index);
	
	priv->MptRateIndex = data_rate_index;

	if((data_rate_index > 27) || (data_rate_index < 0)) {
        	up(&priv->wx_sem);		
       	 	return ret;
	} else if(data_rate_index <= 3) {
		ieee->mode = WIRELESS_MODE_B;
	} else if (data_rate_index <= 11) {
		ieee->mode = WIRELESS_MODE_G;
	} else {
		ieee->mode = WIRELESS_MODE_N_24G;
	}	

	MPT_ProSetDataRate819x(dev);
	
	printk("####RTL819X MP####set tx rate %d\n",ieee->rate);
	
	ret = 0;
        up(&priv->wx_sem);

	return ret;
}

int r8192_wx_mp_set_txpower(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	int ret = -1;
	u8 power_index = 0;
	
	down(&priv->wx_sem);

	power_index = *extra;

        printk("####RTL819X MP####set tx power index %d\n",power_index);

	if((power_index > 0x3F) || (power_index < 0x00)) {
        	up(&priv->wx_sem);		
       	 	return ret;
	}

	mpt_ProSetTxPower(dev, power_index);

	ret = 0;
	up(&priv->wx_sem);
	
	return ret;
	
}

int r8192_wx_mp_set_bw(struct net_device *dev,
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
        struct r8192_priv *priv = rtllib_priv(dev);
	int ret = -1;
	u8 bw_index = 0;

        down(&priv->wx_sem);

	bw_index = *extra;

        printk("####RTL819X MP####set bandwith  index %d [0: 20MHz 1: 40MHz]\n",bw_index);

	priv->MptBandWidth = bw_index;

#if 0
	if((bw_index > 1) || (bw_index < 0)) {
	up(&priv->wx_sem);
       	 	return ret;
	} else if(bw_index == 1) {
		HTSetConnectBwMode(priv->rtllib, HT_CHANNEL_WIDTH_20_40, 
			(priv->rtllib->current_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER);  
	} else {
		HTSetConnectBwMode(priv->rtllib, HT_CHANNEL_WIDTH_20, 
			(priv->rtllib->current_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER);  
	}
#else
	MPT_ProSetBandWidth819x(dev);
#endif

	ret = 0;
	up(&priv->wx_sem);
	
	return ret;
	
}



int r8192_wx_mp_set_txstart(struct net_device *dev,
                               struct iw_request_info *info,
                               union iwreq_data *wrqu, char *extra)
{
        struct r8192_priv *priv = rtllib_priv(dev);
    struct rtllib_device *ieee = priv->rtllib;
    int ret = -1;
    u8 start_flag = 0;

        down(&priv->wx_sem);

    start_flag = *extra;

    if(start_flag == 1) { 
	if (priv->bCckContTx || priv->bOfdmContTx) {
	    printk("####RTL819X MP####continious Tx is undergoing, please close it first\n");
	    ret = -EBUSY;
        up(&priv->wx_sem);
	    return ret;
	}

	if(r8192_is_wireless_b_mode(ieee->rate)) {
	    printk("####RTL819X MP####start cck continious TX, rate:%d\n", ieee->rate);
	    mpt_StartCckContTx(dev, true);
	    r8192_XmitOnePacket(dev);
	} else {
	    printk("####RTL819X MP####start  ofdm continious TX, rate:%d\n", ieee->rate);
	    mpt_StartOfdmContTx(dev);
	    r8192_XmitOnePacket(dev);
	} 
    } else if(start_flag == 2) {
	bool	bCckContTx = priv->bCckContTx;
	bool	bOfdmContTx = priv->bOfdmContTx;

	if(bCckContTx && !bOfdmContTx) {
	    printk("####RTL819X MP####stop cck continious TX\n");
	    mpt_StopCckCoNtTx(dev);
	} else if (!bCckContTx && bOfdmContTx) {
	    printk("####RTL819X MP####stop ofdm continious TX\n");
	    mpt_StopOfdmContTx(dev);
	} else if(!bCckContTx && !bOfdmContTx) { 
	    ;
	} else { 
	    printk("####RTL819X MP#### Unexpected case! bCckContTx: %d , bOfdmContTx: %d\n",
		    bCckContTx, bOfdmContTx);
}
    } else {
	ret = -1;
	up(&priv->wx_sem);
	return ret;
    }


    ret = 0;
    up(&priv->wx_sem);

    return ret;

}

int r8192_wx_mp_set_singlecarrier(struct net_device *dev,
                               struct iw_request_info *info,
                               union iwreq_data *wrqu, char *extra)
{
        struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;
        int ret = -1;
	u8 start_flag = 0;

	if((ieee->rate > 108) || (ieee->rate < 12))
		printk("####RTL819X MP#### we did not do singlecarrier when rate not in [6M-54M] tmp, see StartTesting_SingleCarrierTx to get more\n");

        down(&priv->wx_sem);
	
	start_flag = *extra;

	if(start_flag == 1){ 
		if (priv->bCckContTx || priv->bOfdmContTx || priv->bSingleCarrier){
			printk("####RTL819X MP#### single carrier continious Tx is undergoing, please close it first\n");
			ret = -EBUSY;
        up(&priv->wx_sem);
			return ret;
		}	

		printk("####RTL819X MP####start single carrier cck continious TX\n");
		mpt_StartOfdmContTx(dev);
		r8192_XmitOnePacket(dev);
		
	} else if(start_flag == 2) {
		 if (priv->bCckContTx) {
			printk("####RTL819X MP####stop single cck continious TX\n");
			mpt_StopCckCoNtTx(dev);
		} 
		 if (priv->bOfdmContTx) {
			printk("####RTL819X MP####stop single ofdm continious TX\n");
			mpt_StopOfdmContTx(dev);
		} 
		 if (priv->bSingleCarrier) {
			printk("####RTL819X MP####stop single carrier mode\n");
			MPT_ProSetSingleCarrier(dev, false);
		}
	} else {
		ret = -1;
		up(&priv->wx_sem);

		return ret;
	}

	ret = 0;
        up(&priv->wx_sem);
		
        return ret;

}
int r8192_wx_mp_write_rf(struct net_device *dev,
                               struct iw_request_info *info,
                               union iwreq_data *wrqu, char *extra)
{
        struct r8192_priv *priv = rtllib_priv(dev);
	u32	ulIoType, INulRegOffset, INulRegValue;
	u32 *info_buf = (u32*)(&wrqu->freq.m);
        u32 ulRegOffset = info_buf[0];
	u32 ulRegValue = info_buf[1];
	u32 RF_PATH = info_buf[2];

        down(&priv->wx_sem);
	printk("####RTL819X MP####%s :ulRegOffset %x, ulRegValue %x, RF_PATH:%x\n", 
		__func__, ulRegOffset, ulRegValue, RF_PATH);

	ulIoType = MPT_WRITE_RF;
	INulRegOffset = ulRegOffset & bRFRegOffsetMask;
	INulRegValue = ulRegValue & bRFRegOffsetMask;


	priv->MptIoOffset = INulRegOffset;
	priv->MptIoValue = INulRegValue;
	priv->MptRfPath = RF_PATH;
	priv->MptActType = ulIoType;

	mpt_Pro819xIoCallback(dev);

        up(&priv->wx_sem);
        return 0;

}

int r8192_wx_mp_write_mac(struct net_device *dev,
                               struct iw_request_info *info,
                               union iwreq_data *wrqu, char *extra)
{
        struct r8192_priv *priv = rtllib_priv(dev);
	u32 *info_buf = (u32*)(&wrqu->freq.m);
        u32 ulRegOffset = info_buf[0];
	u32 ulRegValue = info_buf[1];
	u32 ulRegDataWidth = info_buf[2];
	u32 ulIoType = 0;

        down(&priv->wx_sem);

	printk("####RTL819X MP####%s :ulRegOffset %x, ulRegValue %x, ulRegDataWidth:%x\n", 
		__func__, ulRegOffset, ulRegValue, ulRegDataWidth);
		
	switch(ulRegDataWidth)
	{
	case 1:
		ulIoType = MPT_WRITE_MAC_1BYTE;
		break;

	case 2:
		ulIoType = MPT_WRITE_MAC_2BYTE;
		break;
	case 4:
		ulIoType = MPT_WRITE_MAC_4BYTE;
		break;
	default:
		printk("####RTL819X MP####%s :error ulRegDataWidth:%x\n", __func__, ulRegDataWidth);
		break;
                }

	if(ulIoType != 0){
		priv->MptIoOffset = ulRegOffset;
		priv->MptIoValue = ulRegValue;
		priv->MptActType = ulIoType;
		mpt_Pro819xIoCallback(dev);
        }

        up(&priv->wx_sem);

	return 0;

}

/*-----------------------------------------------------------------------------
 * Function:	mpt_StartCckContTx()
 *
 * Overview:	Start CCK Continuous Tx.
 *
 * Input:		PADAPTER	pAdapter
 *				BOOLEAN		bScrambleOn
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	05/16/2007	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
void mpt_StartCckContTx(struct net_device *dev,bool bScrambleOn)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	u32			cckrate;

	if(!rtl8192_QueryBBReg(dev, rFPGA0_RFMOD, bCCKEn))
		rtl8192_setBBreg(dev, rFPGA0_RFMOD, bCCKEn, bEnable);

	rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMContinueTx, bDisable);
	rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMSingleCarrier, bDisable);
	rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMSingleTone, bDisable);
	switch(priv->rtllib->rate)
	{
		case 2:
			cckrate = 0;
			break;
		case 4:
			cckrate = 1;
			break;
		case 11:
			cckrate = 2;
			break;
		case 22:
			cckrate = 3;
			break;
		default:
			cckrate = 0;
			break;
	}
	rtl8192_setBBreg(dev, rCCK0_System, bCCKTxRate, cckrate);

	rtl8192_setBBreg(dev, rCCK0_System, bCCKBBMode, 0x2);    
	rtl8192_setBBreg(dev, rCCK0_System, bCCKScramble, 0x1);  

	priv->bCckContTx = true;
	priv->bOfdmContTx = false;
	
}	/* mpt_StartCckContTx */

/*-----------------------------------------------------------------------------
 * Function:	mpt_StartOfdmContTx()
 *
 * Overview:	Start OFDM Continuous Tx.
 *
 * Input:		PADAPTER	pAdapter
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	05/16/2007	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
void mpt_StartOfdmContTx(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);

	if(!rtl8192_QueryBBReg(dev, rFPGA0_RFMOD, bOFDMEn))
		rtl8192_setBBreg(dev, rFPGA0_RFMOD, bOFDMEn, bEnable);

	rtl8192_setBBreg(dev, rCCK0_System, bCCKBBMode, bDisable);

	rtl8192_setBBreg(dev, rCCK0_System, bCCKScramble, bEnable);

	rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMContinueTx, bEnable);
	rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMSingleCarrier, bDisable);
	rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMSingleTone, bDisable);

	priv->bCckContTx = false;
	priv->bOfdmContTx = true;

}	/* mpt_StartOfdmContTx */

/*-----------------------------------------------------------------------------
 * Function:	mpt_StopCckCoNtTx()
 *
 * Overview:	Stop CCK Continuous Tx.
 *
 * Input:		PADAPTER	pAdapter
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	05/16/2007	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
void mpt_StopCckCoNtTx(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);

	priv->bCckContTx = false;
	priv->bOfdmContTx = false;

	rtl8192_setBBreg(dev, rCCK0_System, bCCKBBMode, 0x0);    
	rtl8192_setBBreg(dev, rCCK0_System, bCCKScramble, 0x1);  

	rtl8192_setBBreg(dev, rPMAC_Reset, bBBResetB, 0x0);
	rtl8192_setBBreg(dev, rPMAC_Reset, bBBResetB, 0x1);
	
}	/* mpt_StopCckCoNtTx */

/*-----------------------------------------------------------------------------
 * Function:	mpt_StopOfdmContTx()
 *
 * Overview:	Stop 818xB OFDM Continuous Tx.
 *
 * Input:		PADAPTER	pAdapter
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	05/16/2007	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
void mpt_StopOfdmContTx(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);

	priv->bCckContTx = false;
	priv->bOfdmContTx = false;

	rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMContinueTx, bDisable);
	rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMSingleCarrier, bDisable);
	rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMSingleTone, bDisable);
	mdelay(10);
	rtl8192_setBBreg(dev, rPMAC_Reset, bBBResetB, 0x0);
	rtl8192_setBBreg(dev, rPMAC_Reset, bBBResetB, 0x1);

}	/* mpt_StopOfdmContTx */

/*-----------------------------------------------------------------------------
 * Function:	mpt_SwitchRfSetting92S
 *
 * Overview:	Change RF Setting when we siwthc channel/rate/BW for 92S series MP.
 *
 * Input:       IN	PADAPTER				pAdapter
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 01/08/2009	MHC		Suggestion from SD3 Willis for 92S series.
 * 01/09/2009	MHC		Add CCK modification for 40MHZ. Suggestion from SD3.
 *
 *---------------------------------------------------------------------------*/
 void mpt_SwitchRfSetting92S(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	u8	ChannelToSw = priv->MptChannelToSw;
	u32	ulRateIdx = priv->MptRateIndex;
	u32	ulbandwidth = priv->MptBandWidth;

	if (ulbandwidth == BAND_20MHZ_MODE)
	{
		if (ChannelToSw == 1)
		{
			rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)0, RF_IPA, 0xFFFFF, 0x0A8F4);
		}
		else if (ChannelToSw == 11)
		{
			rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)0, RF_IPA, 0xFFFFF, 0x0F8F5);
		}
		else	
		{
			rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)0, RF_IPA, 0xFFFFF, 0x0F8F4);
		}
	}
	else 	
	{
		if (ChannelToSw == 3)
		{
			rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)0, RF_IPA, 0xFFFFF, 0x0A8F4);
		}
		else if (ChannelToSw == 9 || ChannelToSw == 10 || ChannelToSw == 11)
		{
			rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)0, RF_IPA, 0xFFFFF, 0x0F8F5);
		}
		else	
		{
			rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)0, RF_IPA, 0xFFFFF, 0x0F8F4);
		}
	}


	if (ulRateIdx < MPT_RATE_6M)	
		rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)0, RF_SYN_G2, 0xFFFFF, 0x04440);
	else		
		rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)0, RF_SYN_G2, 0xFFFFF, 0x0F200);
}	

/*-----------------------------------------------------------------------------
 * Function:	mpt_ProSetTxPower()
 *
 * Overview:	Change Tx Power of current channel for 
 *				OID_RT_PRO_SET_TX_POWER_CONTROL.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	05/16/2007	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
bool mpt_ProSetTxPower(	struct net_device *dev,	u32 ulTxPower)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	u8		CurrChannel = priv->rtllib->current_network.channel;
	bool		bResult = true;
	
	CurrChannel = priv->MptChannelToSw;
	
	if(CurrChannel > 11 || CurrChannel < 1)
	{
		printk("mpt_ProSetTxPower(): CurrentChannel:%d is not valid\n", CurrChannel);		
		return false;
	}

	if(ulTxPower > MAX_TX_PWR_INDEX_N_MODE)
	{
		printk("mpt_ProSetTxPower(): TxPWR:%d is invalid\n", ulTxPower);		
		return false;
	}

	if( priv->MptRateIndex >= MPT_RATE_1M &&
		priv->MptRateIndex <= MPT_RATE_11M )
	{
		priv->TxPowerLevelCCK[CurrChannel-1] = (u8)ulTxPower;
		
		priv->RfTxPwrLevelCck[0][CurrChannel-1] = (u8)ulTxPower;
	}
	else if(priv->MptRateIndex >= MPT_RATE_6M &&
			priv->MptRateIndex <= MPT_RATE_MCS15 )
	{
		priv->TxPowerLevelOFDM24G[CurrChannel-1] = (u8)ulTxPower;

		priv->RfTxPwrLevelOfdm1T[0][CurrChannel-1] = (u8)ulTxPower;
		priv->RfTxPwrLevelOfdm2T[0][CurrChannel-1] = (u8)ulTxPower;
	}

	 rtl8192_phy_setTxPower(dev,CurrChannel);

	return bResult;

}	/* mpt_ProSetTxPower */

/*-----------------------------------------------------------------------------
 * Function:	mpt_ProSetTxAGCOffset()
 *
 * Overview:	Change Tx AGC Offset 
 *				OID_RT_PRO_SET_TX_AGC_OFFSET.
 *
 * Input:			NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	08/09/2007	Cosa	Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
bool mpt_ProSetTxAGCOffset(struct net_device *dev,	u32	ulTxAGCOffset)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	bool	bResult = true;
	u32	TxAGCOffset_B, TxAGCOffset_C, TxAGCOffset_D;

	TxAGCOffset_B = (ulTxAGCOffset&0x000000ff);
	TxAGCOffset_C = ((ulTxAGCOffset&0x0000ff00)>>8);
	TxAGCOffset_D = ((ulTxAGCOffset&0x00ff0000)>>16);

	if( TxAGCOffset_B > TxAGC_Offset_neg1 ||
		TxAGCOffset_C > TxAGC_Offset_neg1 ||
		TxAGCOffset_D > TxAGC_Offset_neg1 )
	{
		printk("mpt_ProSetTxAGCOffset(): TxAGCOffset:%d is invalid\n", ulTxAGCOffset);
		return false;
	}

	priv->AntennaTxPwDiff[0] = TxAGCOffset_B;
	priv->AntennaTxPwDiff[1] = TxAGCOffset_C;
	priv->AntennaTxPwDiff[2] = TxAGCOffset_D;
	
	MPT_ProSetTxAGCOffset(dev);

	return bResult;

}	/* mpt_ProSetTxPower */

/*-----------------------------------------------------------------------------
 * Function:	mpt_ProSetTxAGCOffset()
 *
 * Overview:	Change Tx AGC Offset 
 *				OID_RT_PRO_SET_TX_AGC_OFFSET.
 *
 * Input:			NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	08/09/2007	Cosa	Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
bool mpt_ProSetRxFilter(struct net_device *dev, u32	RCRMode)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);

	if(RCRMode == 1)
	{ 
		priv->MptRCR &= ~(RCR_AAP|RCR_AM|RCR_AB);
		priv->MptRCR |= RCR_APM;
		write_nic_dword(dev, RCR, priv->MptRCR);
	}
	else
	{ 
		priv->MptRCR |= (RCR_AAP|RCR_APM|RCR_AM|RCR_AB);
		write_nic_dword(dev, RCR, priv->MptRCR);
	}

	return 1;
}	/* mpt_ProSetTxPower */

/*-----------------------------------------------------------------------------
 * Function:	mpt_ProSetModulation()
 *
 * Overview:	Switch wireless mode for OID_RT_PRO_SET_MODULATION.
 *
 * Input:		PADAPTER	pAdapter
 *				ULONG		ulWirelessMode
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	05/16/2007	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
bool mpt_ProSetModulation(struct net_device *dev, u32 ulWirelessMode)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	WIRELESS_MODE	WirelessMode;
	bool				bResult = false;

	switch(ulWirelessMode)
	{
	case WIRELESS_MODE_A: 
		WirelessMode = WIRELESS_MODE_A;
		break;

	case WIRELESS_MODE_B: 
		WirelessMode = WIRELESS_MODE_B;
		break;

	case WIRELESS_MODE_G: 
	case WIRELESS_MODE_G |WIRELESS_MODE_B: 
		WirelessMode = WIRELESS_MODE_G;
		break;

	case WIRELESS_MODE_N_24G:
		WirelessMode = WIRELESS_MODE_N_24G;
		break;

	case WIRELESS_MODE_N_5G:
		WirelessMode = WIRELESS_MODE_N_5G;
		break;

	case WIRELESS_MODE_AUTO: 
	default:
		bResult = false;
		return bResult;
		break;
	}

	priv->rtllib->mode = WirelessMode;
	priv->RegWirelessMode = WirelessMode;
	rtl8192_SetWirelessMode(dev, priv->rtllib->mode);
	HTUseDefaultSetting(priv->rtllib);
	

	if (IS_HARDWARE_TYPE_8192SE(dev))
	{		
		mpt_SwitchRfSetting92S(dev);
	}
	
	bResult = true;
	
	return bResult;

}

/*-----------------------------------------------------------------------------
 * Function:	mpt_Pro819xIoCallback()
 *
 * Overview:	Callback function of a workitem for IO.	
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	05/16/2007	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
void mpt_Pro819xIoCallback(struct net_device *dev)
{

	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	u32 MptActType = priv->MptActType;

	printk("####RTL819X MP####%s :ulRegOffset %x, ulRegValue %x, MptActType:%x, MptRfPath:%x\n", 
		__func__, priv->MptIoOffset, priv->MptIoValue, MptActType, priv->MptRfPath);

	switch(MptActType)
	{
	case MPT_WRITE_MAC_1BYTE:
		write_nic_byte(dev, priv->MptIoOffset, (u8)(priv->MptIoValue));
		break;

	case MPT_WRITE_MAC_2BYTE:
		write_nic_word(dev, priv->MptIoOffset, (u16)(priv->MptIoValue));
		break;

	case MPT_WRITE_MAC_4BYTE:
		write_nic_dword(dev, priv->MptIoOffset, (u32)(priv->MptIoValue));
		break;

	case MPT_WRITE_RF:
		rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)priv->MptRfPath, 
			priv->MptIoOffset, bRFRegOffsetMask, priv->MptIoValue);
		break;

	default:
		break;
	}
}

void MPT_ProSetSingleCarrier(struct net_device *dev, bool ulMode)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);

	if(ulMode == 1) { 
		priv->bSingleCarrier = true;
	} else { 
		priv->bSingleCarrier = false;
	}
	
	if(priv->bSingleCarrier)
	{
		if(!rtl8192_QueryBBReg(dev, rFPGA0_RFMOD, bOFDMEn))
			rtl8192_setBBreg(dev, rFPGA0_RFMOD, bOFDMEn, bEnable);

		rtl8192_setBBreg(dev, rCCK0_System, bCCKBBMode, bDisable);

		rtl8192_setBBreg(dev, rCCK0_System, bCCKScramble, bEnable);

		rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMContinueTx, bDisable);
		rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMSingleCarrier, bEnable);
		rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMSingleTone, bDisable);
	}
	else
	{ 
	    rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMContinueTx, bDisable);
	    rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMSingleCarrier, bDisable);
	    rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMSingleTone, bDisable);
	    mdelay(10);
	    rtl8192_setBBreg(dev, rPMAC_Reset, bBBResetB, 0x0);
	    rtl8192_setBBreg(dev, rPMAC_Reset, bBBResetB, 0x1);
	}
}

/*-----------------------------------------------------------------------------
 * Function:	MPT_ProSetBandWidth819x()
 *
 * Overview:	None
 *
 * Input:		PADAPTER			pAdapter
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	01/03/2007	Cosa	Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
void MPT_ProSetBandWidth819x(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	PRT_HIGH_THROUGHPUT	pHTInfo = priv->rtllib->pHTInfo;
	u32	ulbandwidth = priv->MptBandWidth;

	printk("##################MPT_ProSetBandWidth819x() is start. \n");
	/* 2007/06/07 MH Call normal driver API and set 40MHZ mode. */			
	if (ulbandwidth == BAND_20MHZ_MODE) {
		/* 20 MHZ sub-carrier mode --> dont care. */
		pHTInfo->bCurBW40MHz = false;
		pHTInfo->CurSTAExtChnlOffset = HT_EXTCHNL_OFFSET_NO_EXT;
		priv->rtllib->SetBWModeHandler(dev, HT_CHANNEL_WIDTH_20, pHTInfo->CurSTAExtChnlOffset);
	} else if (ulbandwidth == BAND_40MHZ_DUPLICATE_MODE) {
		/* Sub-Carrier mode is defined in MAC data sheet chapter 12.3. */	
		/* 40MHX sub-carrier mode --> duplicate. */
		pHTInfo->bCurBW40MHz = true;				
		pHTInfo->bCurTxBW40MHz = true;								
		pHTInfo->CurSTAExtChnlOffset = HT_EXTCHNL_OFFSET_NO_DEF;			
		
		priv->rtllib->SetBWModeHandler(dev, HT_CHANNEL_WIDTH_20_40, pHTInfo->CurSTAExtChnlOffset);
	} else if (ulbandwidth == BAND_40MHZ_LOWER_MODE) {
		/* 40MHX sub-carrier mode --> lower mode  */
		pHTInfo->bCurBW40MHz = true;				
		pHTInfo->bCurTxBW40MHz = true;								
		pHTInfo->CurSTAExtChnlOffset = HT_EXTCHNL_OFFSET_LOWER;
		
		/* Extention channel is lower. Current channel must > 3. */
		/*if (pMgntInfo->dot11CurrentChannelNumber < 3)				
			DbgPrint("Illegal Current_Chl=%d\r\n", pMgntInfo->dot11CurrentChannelNumber);
		else
			pAdapter->HalFunc.SwChnlByTimerHandler(pAdapter, pMgntInfo->dot11CurrentChannelNumber-2);*/
		
		priv->rtllib->SetBWModeHandler(dev, HT_CHANNEL_WIDTH_20_40, pHTInfo->CurSTAExtChnlOffset);
	} else if (ulbandwidth == BAND_40MHZ_UPPER_MODE)	{
		/* 40MHX sub-carrier mode --> upper mode  */
		pHTInfo->bCurBW40MHz = true;				
		pHTInfo->bCurTxBW40MHz = true;
		pHTInfo->CurSTAExtChnlOffset = HT_EXTCHNL_OFFSET_UPPER;
		
		/* Extention channel is upper. Current channel must < 12. */
		/*if (pMgntInfo->dot11CurrentChannelNumber > 12)
			DbgPrint("Illegal Current_Chl=%d", pMgntInfo->dot11CurrentChannelNumber);
		else
			pAdapter->HalFunc.SwChnlByTimerHandler(pAdapter, pMgntInfo->dot11CurrentChannelNumber+2);*/
		
		priv->rtllib->SetBWModeHandler(dev, HT_CHANNEL_WIDTH_20_40, pHTInfo->CurSTAExtChnlOffset);
	} else if (ulbandwidth == BAND_40MHZ_DONTCARE_MODE) {
		/* 40MHX sub-carrier mode --> dont care mode  */
		pHTInfo->bCurBW40MHz = true;
		pHTInfo->bCurTxBW40MHz = true;
		pHTInfo->CurSTAExtChnlOffset = HT_EXTCHNL_OFFSET_LOWER;
		
		priv->rtllib->SetBWModeHandler(dev, HT_CHANNEL_WIDTH_20_40, pHTInfo->CurSTAExtChnlOffset);
	} else {
		printk("##################MPT_ProSetBandWidth819x() error BW. \n");
		return;
	}

{
		mpt_SwitchRfSetting92S(dev);
		return;
	}
	
	printk("##################MPT_ProSetBandWidth819x() is finished. \n");
}

/*-----------------------------------------------------------------------------
 * Function:	MPT_ProSwChannel()
 *
 * Overview:	Callback function of a work item to switch channel for 
 *				OID_RT_PRO_SET_CHANNEL_DIRECT_CALL
 *
 * Input:		PVOID	Context
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	05/16/2007	MHC		Create Version 0.  
 *	06/07/2007	MHC		Normal driver change switch channel handler. 
 *	09/03/2008	MHC		RF channel register for 92S.
 *	01/08/2008	MHC		For MP verification for 92S,weneed to change setting according
 *						to SD3 Willis's document.
 *
 *---------------------------------------------------------------------------*/
void MPT_ProSwChannel(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	u8	ChannelToSw = priv->MptChannelToSw;
	u8	eRFPath;

	priv->rtllib->current_network.channel = ChannelToSw;
	priv->MptChannelToSw = ChannelToSw;	

	if (IS_HARDWARE_TYPE_8192SE(dev))
	{
		for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
		{
			rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, rRfChannel, 0x3FF, ChannelToSw);
		}

		if (IS_HARDWARE_TYPE_8192SE(dev))
		{
			mpt_SwitchRfSetting92S(dev);
		}
	}

#ifdef MP_DEVELOP_READY 
	for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
	{
		if (IS_HARDWARE_TYPE_8192SE(dev))
			rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, rRfChannel, 0x3FF, ChannelToSw);		

		udelay(100);
	}

	
	for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
	{
		if (IS_HARDWARE_TYPE_8192SE(dev))
			rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, rRfChannel, 0x3FF, ChannelToSw);		

		udelay(100);
	}
#endif
	
	/* 2007/06/07 MH Normal driver change sw channel handler. It does not 
	   support SwChnlByDelayHandler ans replace with SwChnlByTimerHandler. */
	priv->rtllib->set_chan(dev, ChannelToSw);

#if 0 
	if(pHalData->CurrentChannel == 14 && !pHalData->bCCKinCH14){
		pHalData->bCCKinCH14 = true;
		MPT_CCKTxPowerAdjust(pAdapter,pHalData->bCCKinCH14);
	}
	else if(pHalData->CurrentChannel != 14 && pHalData->bCCKinCH14){
		pHalData->bCCKinCH14 = false;
		MPT_CCKTxPowerAdjust(pAdapter,pHalData->bCCKinCH14);
	}
#endif
}	/* MPT_ProSwChannel */

/*-----------------------------------------------------------------------------
 * Function:	MPT_ProSetDataRate819x()
 *
 * Overview:	None
 *
 * Input:		PADAPTER			pAdapter
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	01/03/2007	Cosa	Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
void MPT_ProSetDataRate819x(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	u8 	DataRate = 0xFF;
	u32	ulRateIdx = priv->MptRateIndex;


	printk("################MPT_ProSetDataRate819x():Rate=%d\n", ulRateIdx);
	switch(ulRateIdx)
	{
		/* CCK rate. */
		case	MPT_RATE_1M:		DataRate = 2;		break;
		case	MPT_RATE_2M:		DataRate = 4;		break;
		case	MPT_RATE_55M:	DataRate = 11;		break;
		case	MPT_RATE_11M:	DataRate = 22;		break;
	
		/* OFDM rate. */
		case	MPT_RATE_6M:		DataRate = 12;		break;
		case	MPT_RATE_9M:		DataRate = 18;		break;
		case	MPT_RATE_12M:	DataRate = 24;		break;
		case	MPT_RATE_18M:	DataRate = 36;		break;
		case	MPT_RATE_24M:	DataRate = 48;		break;			
		case	MPT_RATE_36M:	DataRate = 72;		break;
		case	MPT_RATE_48M:	DataRate = 96;		break;
		case	MPT_RATE_54M:	DataRate = 108;	break;
	
		/* HT rate. */
		case	MPT_RATE_MCS0:	DataRate = 0x80;	break;
		case	MPT_RATE_MCS1:	DataRate = 0x81;	break;
		case	MPT_RATE_MCS2:	DataRate = 0x82;	break;
		case	MPT_RATE_MCS3:	DataRate = 0x83;	break;
		case	MPT_RATE_MCS4:	DataRate = 0x84;	break;
		case	MPT_RATE_MCS5:	DataRate = 0x85;	break;
		case	MPT_RATE_MCS6:	DataRate = 0x86;	break;
		case	MPT_RATE_MCS7:	DataRate = 0x87;	break;
		case	MPT_RATE_MCS8:	DataRate = 0x88;	break;
		case	MPT_RATE_MCS9:	DataRate = 0x89;	break;
		case	MPT_RATE_MCS10:	DataRate = 0x8A;	break;
		case	MPT_RATE_MCS11:	DataRate = 0x8B;	break;
		case	MPT_RATE_MCS12:	DataRate = 0x8C;	break;
		case	MPT_RATE_MCS13:	DataRate = 0x8D;	break;
		case	MPT_RATE_MCS14:	DataRate = 0x8E;	break;
		case	MPT_RATE_MCS15:	DataRate = 0x8F;	break;
		case MPT_RATE_LAST:
		break;

	default:
		break;
	}
	
	{
		mpt_SwitchRfSetting92S(dev);
	}
	
#ifdef MP_DEVELOP_READY 	
	if (IS_HARDWARE_TYPE_8192SE(dev))
	{
		for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
		{
			if (ulbandwidth == BAND_20MHZ_MODE)
				rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, rRfChannel, (BIT11|BIT10), 0x01);		
			else
				rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, rRfChannel, (BIT11|BIT10), 0x00);		

			delay_us(100);
		}
	} 	
#endif


	if(!r8192_MgntIsRateValidForWirelessMode(DataRate, 	priv->rtllib->mode) && 	DataRate != 0 ) 
	{
		printk("[MPT]: unknow wmode=%d", priv->rtllib->mode);
	}	
	if (DataRate != 0xFF)
	{
		printk("[MPT]: Force rate=0x%02x", DataRate);
		priv->rtllib->rate = (int)DataRate;
	}

}

/*-----------------------------------------------------------------------------
 * Function:	MPT_ProSetTxAGCOffset()
 *
 * Overview:	Set Tx power level for
 *				OID_RT_PRO_SET_TX_AGC_OFFSET
 *
 * Input:			PVOID	Context
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	08/09/2007	Cosa	Create Version 0.
 *
 *---------------------------------------------------------------------------*/
void MPT_ProSetTxAGCOffset(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	u32	u4RegValue, TxAGCOffset_B, TxAGCOffset_C, TxAGCOffset_D;

	TxAGCOffset_B = priv->AntennaTxPwDiff[0];
	TxAGCOffset_C = priv->AntennaTxPwDiff[1];
	TxAGCOffset_D = priv->AntennaTxPwDiff[2];


	u4RegValue = (TxAGCOffset_D<<8 | TxAGCOffset_C<<4 | TxAGCOffset_B);
	rtl8192_setBBreg(dev, rFPGA0_TxGainStage, (bXBTxAGC|bXCTxAGC|bXDTxAGC), u4RegValue);
	printk("##################MPT_ProSetTxAGCOffset() is finished \n");
}

#endif

