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
#include "../rtl_dm.h"
#include "../rtl_wx.h"

#ifdef _RTL8192_EXT_PATCH_
#include "../../../mshclass/msh_class.h"
#include "../rtl_mesh.h"
#endif
#ifdef ASL
#include "../rtl_softap.h"
#endif
extern int WDCAPARA_ADD[];

void rtl8192se_start_beacon(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	struct rtllib_network *net = NULL;
	u16 BcnTimeCfg = 0;
        u16 BcnCW = 6;
        u16 BcnIFS = 0xf;
	printk("=======>%s()\n",__func__);
	if(priv->rtllib->iw_mode == IW_MODE_ADHOC)
		net = &priv->rtllib->current_network;
#ifdef ASL
	else if ((priv->rtllib->iw_mode == IW_MODE_APSTA) || (priv->rtllib->iw_mode == IW_MODE_MASTER))
		net = &priv->rtllib->current_ap_network;
#endif
#ifdef _RTL8192_EXT_PATCH_
	else if(priv->rtllib->iw_mode == IW_MODE_MESH)
		net = &priv->rtllib->current_mesh_network;
#endif
	else 
	    return;

	rtl8192_irq_disable(dev);

	write_nic_word(dev, ATIMWND, 2);

	write_nic_word(dev, BCN_INTERVAL, net->beacon_interval);
	PHY_SetBeaconHwReg(dev, net->beacon_interval);

#ifdef RTL8192SE
	write_nic_word(dev, BCN_DRV_EARLY_INT, 10<<4);
#else
	write_nic_word(dev, BCN_DRV_EARLY_INT, 10);
#endif
	write_nic_word(dev, BCN_DMATIME, 256); 

	write_nic_byte(dev, BCN_ERR_THRESH, 100);	

if(priv->pFirmware->FirmwareVersion > 49){
	switch(priv->rtllib->iw_mode)
	{
		case IW_MODE_ADHOC:
		case IW_MODE_MESH:
		case IW_MODE_MASTER:
#ifdef ASL
		case IW_MODE_APSTA:
#endif
			BcnTimeCfg |= (BcnCW<<BCN_TCFG_CW_SHIFT);
			break;
		default:
			printk("Invalid Operation Mode!!\n");
			break;
	}

	BcnTimeCfg |= BcnIFS<<BCN_TCFG_IFS;


	{
		u8 u1Temp = (u8)(net->beacon_interval);
		write_nic_dword(dev, WFM5, 0xF1000000 |((u16)( u1Temp) << 8));
		ChkFwCmdIoDone(dev);
	}
}else{
	BcnTimeCfg |= BcnCW<<BCN_TCFG_CW_SHIFT;
	BcnTimeCfg |= BcnIFS<<BCN_TCFG_IFS;
	write_nic_word(dev, BCN_TCFG, BcnTimeCfg);
}	
	rtl8192_irq_enable(dev);
}

void rtl8192se_update_msr(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8 msr;
	LED_CTL_MODE	LedAction = LED_CTL_NO_LINK;
	msr  = read_nic_byte(dev, MSR);
	msr &= ~ MSR_LINK_MASK;
	
	switch (priv->rtllib->iw_mode) {
	case IW_MODE_INFRA:
		if (priv->rtllib->state == RTLLIB_LINKED){
			LedAction = LED_CTL_LINK;
			msr |= (MSR_LINK_MANAGED << MSR_LINK_SHIFT);
		}else
			msr |= (MSR_LINK_NONE << MSR_LINK_SHIFT);

		break;
	case IW_MODE_ADHOC:
		if (priv->rtllib->state == RTLLIB_LINKED)
			msr |= (MSR_LINK_ADHOC << MSR_LINK_SHIFT);
		else
			msr |= (MSR_LINK_NONE << MSR_LINK_SHIFT);
		break;
	case IW_MODE_MASTER:
#ifdef ASL
		if (priv->rtllib->ap_state == RTLLIB_LINKED)
#else
		if (priv->rtllib->state == RTLLIB_LINKED)
#endif
			msr |= (MSR_LINK_MASTER << MSR_LINK_SHIFT);
		else
			msr |= (MSR_LINK_NONE << MSR_LINK_SHIFT);
		break;
#ifdef ASL
	case IW_MODE_APSTA:
		if (priv->rtllib->ap_state == RTLLIB_LINKED)
			msr |= (MSR_LINK_MASTER << MSR_LINK_SHIFT);
		else
			msr |= (MSR_LINK_NONE << MSR_LINK_SHIFT);
		if (priv->rtllib->state == RTLLIB_LINKED){
			printk("===========>state is LINKED\n");
			msr |= (MSR_LINK_MANAGED << MSR_LINK_SHIFT);
		}
		break;
#endif
#ifdef _RTL8192_EXT_PATCH_
	case IW_MODE_MESH:
		printk("%s: iw_mode=%d state=%d only_mesh=%d mesh_state=%d\n", __FUNCTION__,priv->rtllib->iw_mode,priv->rtllib->state, priv->rtllib->only_mesh, priv->rtllib->mesh_state); 
		if (priv->rtllib->only_mesh) {
			if (priv->rtllib->mesh_state == RTLLIB_MESH_LINKED)
				msr |= (MSR_LINK_MASTER<<MSR_LINK_SHIFT); 
			else
				msr |= (MSR_LINK_NONE<<MSR_LINK_SHIFT);
		} else {
			if (priv->rtllib->mesh_state == RTLLIB_MESH_LINKED) {
				msr |= (MSR_LINK_ADHOC << MSR_LINK_SHIFT);
			} else {	
				msr |= (MSR_LINK_NONE << MSR_LINK_SHIFT);
			}
			if (priv->rtllib->state == RTLLIB_LINKED)
				msr |= (MSR_LINK_MANAGED << MSR_LINK_SHIFT);
		}
		break;
#endif			
	default:
		break;
	}

	write_nic_byte(dev, MSR, msr);
	if(priv->rtllib->LedControlHandler)
		priv->rtllib->LedControlHandler(dev, LedAction);
}

static void rtl8192se_config_hw_for_load_fail(struct net_device* dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u16			i;
	u8 			sMacAddr[6] = {0x00, 0xE0, 0x4C, 0x81, 0x92, 0x00};
	u8			rf_path, index;
	u32			tmp1111 = 0;
	u32			tmp2222 = 0;

	RT_TRACE(COMP_INIT, "====> rtl8192se_config_hw_for_load_fail\n");

	for(;tmp2222<0xff;tmp2222+=4)
	{
		tmp1111= read_nic_dword(dev, tmp2222);
	}

	write_nic_byte(dev, SYS_ISO_CTRL+1, 0xE8); 
	mdelay(10);
	write_nic_byte(dev, PMC_FSM, 0x02); 
	
	priv->eeprom_vid= 0;
	priv->eeprom_did= 0;		
	priv->eeprom_ChannelPlan= 0;
	priv->eeprom_CustomerID= 0;	

       get_random_bytes(&sMacAddr[5], 1);
	for(i = 0; i < 6; i++)
		dev->dev_addr[i] = sMacAddr[i];			


	priv->rf_type= RTL819X_DEFAULT_RF_TYPE;	
	priv->rf_chip = RF_6052;

	for (rf_path = 0; rf_path < 2; rf_path++)
	{
		for (i = 0; i < 3; i++)
		{
			priv->RfCckChnlAreaTxPwr[rf_path][i] = 
			priv->RfOfdmChnlAreaTxPwr1T[rf_path][i] = 
			priv->RfOfdmChnlAreaTxPwr2T[rf_path][i] = 
			EEPROM_Default_TxPowerLevel;
		}
	}	
				
	for (rf_path = 0; rf_path < 2; rf_path++)
	{
		for(i=0; i<14; i++)	
		{
			if (i < 3)			
				index = 0;
			else if (i < 8)		
				index = 1;
			else				
				index = 2;

			priv->RfTxPwrLevelCck[rf_path][i]  = 
			priv->RfCckChnlAreaTxPwr[rf_path][index] = 
			priv->RfTxPwrLevelOfdm1T[rf_path][i]  = 
			priv->RfOfdmChnlAreaTxPwr1T[rf_path][index] = 
			priv->RfTxPwrLevelOfdm2T[rf_path][i]  = 
			priv->RfOfdmChnlAreaTxPwr2T[rf_path][index] = 
			(u8)(EEPROM_Default_TxPower & 0xff);				

			if (rf_path == 0)
			{
				priv->TxPowerLevelOFDM24G[i] = (u8)(EEPROM_Default_TxPower & 0xff);
				priv->TxPowerLevelCCK[i] = (u8)(EEPROM_Default_TxPower & 0xff);
			}
		}
		
	}
	
	RT_TRACE(COMP_INIT, "All TxPwr = 0x%x\n", EEPROM_Default_TxPower);

	for(i=0; i<14; i++)	
	{			
		priv->TxPwrHt20Diff[RF90_PATH_A][i] = DEFAULT_HT20_TXPWR_DIFF;
		priv->TxPwrHt20Diff[RF90_PATH_B][i] = DEFAULT_HT20_TXPWR_DIFF;

		priv->TxPwrLegacyHtDiff[0][i] = EEPROM_Default_LegacyHTTxPowerDiff;
		priv->TxPwrLegacyHtDiff[1][i] = EEPROM_Default_LegacyHTTxPowerDiff;
	}
	
	priv->TxPwrSafetyFlag = 0;
	
	priv->EEPROMTxPowerDiff = EEPROM_Default_LegacyHTTxPowerDiff;
	priv->LegacyHTTxPowerDiff = priv->EEPROMTxPowerDiff;
	RT_TRACE(COMP_INIT, "TxPowerDiff = %#x\n", priv->EEPROMTxPowerDiff);

	for (rf_path = 0; rf_path < 2; rf_path++)
	{
		for (i = 0; i < 3; i++)
		{
			priv->EEPROMPwrGroup[rf_path][i] = 0;
		}
	}

	for (rf_path = 0; rf_path < 2; rf_path++)
	{
		for(i=0; i<14; i++)
		{
			if (i < 3)			
				index = 0;
			else if (i < 8)		
				index = 1;
			else				
				index = 2;
			priv->PwrGroupHT20[rf_path][i] = (priv->EEPROMPwrGroup[rf_path][index]&0xf);
			priv->PwrGroupHT40[rf_path][i] = ((priv->EEPROMPwrGroup[rf_path][index]&0xf0)>>4);
		}
	}
	priv->EEPROMRegulatory = 0;

	priv->EEPROMTSSI_A = EEPROM_Default_TSSI;
	priv->EEPROMTSSI_B = EEPROM_Default_TSSI;			

	for(i=0; i<6; i++)
	{
		priv->EEPROMHT2T_TxPwr[i] = EEPROM_Default_HT2T_TxPwr;
	}

	
	priv->EEPROMThermalMeter = EEPROM_Default_ThermalMeter;
	priv->ThermalMeter[0] = (priv->EEPROMThermalMeter&0x1f);	
	priv->TSSI_13dBm = priv->EEPROMThermalMeter *100;

	priv->BluetoothCoexist = EEPROM_Default_BlueToothCoexist;
	priv->EEPROMBluetoothType = EEPROM_Default_BlueToothType;
	priv->EEPROMBluetoothAntNum = EEPROM_Default_BlueToothAntNum;
	priv->EEPROMBluetoothAntIsolation = EEPROM_Default_BlueToothAntIso;
	
	priv->EEPROMCrystalCap = EEPROM_Default_CrystalCap;
	priv->CrystalCap = priv->EEPROMCrystalCap;

	priv->eeprom_ChannelPlan = 0;
	priv->eeprom_version = 1;		
	priv->bTXPowerDataReadFromEEPORM = false;

	priv->rf_type = RTL819X_DEFAULT_RF_TYPE;	
	priv->rf_chip = RF_6052;
	priv->eeprom_CustomerID = 0;
	RT_TRACE(COMP_INIT, "EEPROM Customer ID: 0x%2x\n", priv->eeprom_CustomerID);

			
	priv->EEPROMBoardType = EEPROM_Default_BoardType;	
	RT_TRACE(COMP_INIT, "BoardType = %#x\n", priv->EEPROMBoardType);

	priv->LedStrategy = SW_LED_MODE7;

	
	RT_TRACE(COMP_INIT,"<==== rtl8192se_config_hw_for_load_fail\n");
}

static void rtl8192se_get_IC_Inferiority(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8  Efuse_ID;

	priv->IC_Class = IC_INFERIORITY_A; 
	if((priv->epromtype == EEPROM_BOOT_EFUSE) && !priv->AutoloadFailFlag) 
	{
		Efuse_ID = EFUSE_Read1Byte(dev, EFUSE_IC_ID_OFFSET);

		if(Efuse_ID == 0xfe)
		{
			priv->IC_Class = IC_INFERIORITY_B;

		}
	}
}

void
HalCustomizedBehavior8192S(struct net_device* dev)
{
	struct r8192_priv* priv = rtllib_priv(dev);
	priv->rtllib->bForcedShowRateStill = false;
	switch(priv->CustomerID)
	{
		case RT_CID_DEFAULT:
		case RT_CID_819x_SAMSUNG:
			if(priv->CustomerID == RT_CID_819x_SAMSUNG)
				;
			priv->LedStrategy = SW_LED_MODE7;
			if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
				priv->eeprom_svid == 0x1A3B && priv->eeprom_smid == 0x1A07)
			{
				if(priv->RegWirelessMode != WIRELESS_MODE_B){
					priv->RegWirelessMode = WIRELESS_MODE_G;
					priv->rtllib->mode = WIRELESS_MODE_G;
					priv->rtllib->bForcedBgMode = true;
					priv->rtllib->bForcedShowRateStill = true;
				}
			}
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8172 &&
					priv->eeprom_svid == 0x1A3B && priv->eeprom_smid == 0x1A04)
			{
				if(priv->RegWirelessMode != WIRELESS_MODE_B){
					priv->RegWirelessMode = WIRELESS_MODE_G;	
					priv->rtllib->mode = WIRELESS_MODE_G;
					priv->rtllib->bForcedBgMode = true;
					priv->rtllib->bForcedShowRateStill = true;
				}
			}
			else if(priv->eeprom_svid == 0x1A3B && (priv->eeprom_smid == 0x1104 ||
				priv->eeprom_smid == 0x1107))
			{
				priv->rtllib->bForcedShowRateStill = true;
			}
			else if(priv->eeprom_vid == 0x10ec && priv->eeprom_did == 0x8171 &&
				priv->eeprom_svid == 0x10ec && priv->eeprom_smid == 0x7171)
			{
				if(priv->RegWirelessMode != WIRELESS_MODE_B){
				 	priv->RegWirelessMode = WIRELESS_MODE_G;	
					priv->rtllib->mode = WIRELESS_MODE_G;
					priv->rtllib->bForcedBgMode = true;
				}
			}
			else if(priv->eeprom_vid == 0x10ec && priv->eeprom_did == 0x8171 &&
				priv->eeprom_svid == 0x10ec && priv->eeprom_smid == 0x7150)
			{
				if(priv->RegWirelessMode != WIRELESS_MODE_B){
				 	priv->RegWirelessMode = WIRELESS_MODE_G;	
					priv->rtllib->mode = WIRELESS_MODE_G;
				}
			}
			else if(priv->eeprom_vid == 0x10ec && priv->eeprom_did == 0x8172 &&
				priv->eeprom_svid == 0x10ec && priv->eeprom_smid == 0x7172)
			{
				if(priv->RegWirelessMode != WIRELESS_MODE_B){
				 	priv->RegWirelessMode = WIRELESS_MODE_G;	
					priv->rtllib->mode = WIRELESS_MODE_G;
				}
			}
			else if(priv->eeprom_vid == 0x10ec && priv->eeprom_did == 0x8172 &&
				priv->eeprom_svid == 0x10ec && priv->eeprom_smid == 0x7150)
			{
				if(priv->RegWirelessMode != WIRELESS_MODE_B){
				 	priv->RegWirelessMode = WIRELESS_MODE_G;	
					priv->rtllib->mode = WIRELESS_MODE_G;
				}
			}
			else if(priv->eeprom_vid == 0x10ec && priv->eeprom_did == 0x8172 &&
				priv->eeprom_svid == 0x10ec && priv->eeprom_smid == 0x7186)
			{
				if(priv->RegWirelessMode != WIRELESS_MODE_B){
				 	priv->RegWirelessMode = WIRELESS_MODE_G;	
					priv->rtllib->mode = WIRELESS_MODE_G;
				}
			}
			else if(priv->eeprom_vid == 0x10ec && priv->eeprom_did == 0x8174 &&
				priv->eeprom_svid == 0x10ec && priv->eeprom_smid == 0x7174)
			{
				if(priv->RegWirelessMode != WIRELESS_MODE_B){
				 	priv->RegWirelessMode = WIRELESS_MODE_G;	
					priv->rtllib->mode = WIRELESS_MODE_G;
				}
			}
			else if(priv->eeprom_vid == 0x10ec && priv->eeprom_did == 0x8174 &&
				priv->eeprom_svid == 0x10ec && priv->eeprom_smid == 0x7150)
			{
				if(priv->RegWirelessMode != WIRELESS_MODE_B){
				 	priv->RegWirelessMode = WIRELESS_MODE_G;	
					priv->rtllib->mode = WIRELESS_MODE_G;
				}
			}
			
			break;
			
		case RT_CID_819x_Acer:
			priv->LedStrategy = SW_LED_MODE7;	
						
			if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
				priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7172)
			{
				if(priv->RegWirelessMode != WIRELESS_MODE_B){
				 	priv->RegWirelessMode = WIRELESS_MODE_G;	
					priv->rtllib->mode = WIRELESS_MODE_G;
				}
			}
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
						priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7173)
			{
				if(priv->RegWirelessMode != WIRELESS_MODE_B){
				 	priv->RegWirelessMode = WIRELESS_MODE_G;	
					priv->rtllib->mode = WIRELESS_MODE_G;
				}
			}
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8174 &&
					priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7186)
			{
				if(priv->RegWirelessMode != WIRELESS_MODE_B){
				 	priv->RegWirelessMode = WIRELESS_MODE_G;	
					priv->rtllib->mode = WIRELESS_MODE_G;
				}
			}
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8174 &&
					priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7187)
			{
				if(priv->RegWirelessMode != WIRELESS_MODE_B){
				 	priv->RegWirelessMode = WIRELESS_MODE_G;	
					priv->rtllib->mode = WIRELESS_MODE_G;
				}
			}
			break;
			
		case RT_CID_TOSHIBA:
			priv->rtllib->current_network.channel = 10;
			priv->LedStrategy = SW_LED_MODE7;
			priv->EEPROMRegulatory = 1;		
			if(priv->eeprom_smid >=  0x7000 && priv->eeprom_smid < 0x8000){
				if(priv->RegWirelessMode != WIRELESS_MODE_B){
				 	priv->RegWirelessMode = WIRELESS_MODE_G;	
					priv->rtllib->mode = WIRELESS_MODE_G;
					priv->rtllib->bForcedBgMode = true;
				}
			}
			break;
		case RT_CID_CCX:
			priv->DMFlag |= (HAL_DM_DIG_DISABLE | HAL_DM_HIPWR_DISABLE);
			break;

		case RT_CID_819x_Lenovo:
			priv->LedStrategy = SW_LED_MODE7;
			if(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0xE025){
				if(priv->RegWirelessMode != WIRELESS_MODE_B){
				 	priv->RegWirelessMode = WIRELESS_MODE_G;	
					priv->rtllib->mode = WIRELESS_MODE_G;
				}
			}
			else if (priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
				priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8191)
				if(priv->RegWirelessMode != WIRELESS_MODE_B){
				 	priv->RegWirelessMode = WIRELESS_MODE_G;	
					priv->rtllib->mode = WIRELESS_MODE_G;
				}
			break;

		case RT_CID_819x_QMI:
			priv->LedStrategy = SW_LED_MODE8;			
			break;

		case RT_CID_819x_MSI:	
			priv->LedStrategy = SW_LED_MODE9;			
			if(priv->eeprom_svid == 0x1462 && priv->eeprom_smid == 0x897A){
				if(priv->RegWirelessMode != WIRELESS_MODE_B){
				 	priv->RegWirelessMode = WIRELESS_MODE_G;	
					priv->rtllib->mode = WIRELESS_MODE_G;
				}
			}
			break;

		case RT_CID_819x_HP:
			priv->LedStrategy = SW_LED_MODE7;
			priv->bLedOpenDrain = true;
			
		case RT_CID_819x_Foxcoon:
			priv->LedStrategy = SW_LED_MODE10;
			if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
		 		priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8192)
				if(priv->RegWirelessMode != WIRELESS_MODE_B){
				 	priv->RegWirelessMode = WIRELESS_MODE_G;	
					priv->rtllib->mode = WIRELESS_MODE_G;
				}					
			break;	
			
		case RT_CID_WHQL:
			;
			break;
	
		default:
			RT_TRACE(COMP_INIT,"Unkown hardware Type \n");
			break;
	}
}

static	void rtl8192se_read_eeprom_info(struct net_device* dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u16			i,usValue;
	u16			EEPROMId;
	u8			tempval;
	u8			hwinfo[HWSET_MAX_SIZE_92S];
	u8			rf_path, index;	

	RT_TRACE(COMP_INIT, "====> rtl8192se_read_eeprom_info\n");

	if (priv->epromtype== EEPROM_93C46)
	{	
		write_nic_byte(dev, SYS_ISO_CTRL+1, 0xE8); 
		mdelay(10);
		write_nic_byte(dev, PMC_FSM, 0x02); 

		RT_TRACE(COMP_INIT, "EEPROM\n");
		for(i = 0; i < HWSET_MAX_SIZE_92S; i += 2)
		{
			usValue = eprom_read(dev, (u16) (i>>1));
			*((u16*)(&hwinfo[i])) = usValue;					
		}	
	}
	else if (priv->epromtype == EEPROM_BOOT_EFUSE)
	{	
		RT_TRACE(COMP_INIT, "EFUSE\n");

		EFUSE_ShadowMapUpdate(dev);

		memcpy( hwinfo, &priv->EfuseMap[EFUSE_INIT_MAP][0], HWSET_MAX_SIZE_92S);		
	}


	EEPROMId = *((u16 *)&hwinfo[0]);
	if( EEPROMId != RTL8190_EEPROM_ID )
	{
		RT_TRACE(COMP_ERR, "EEPROM ID(%#x) is invalid!!\n", EEPROMId); 
		priv->AutoloadFailFlag=true;
	}
	else
	{
		RT_TRACE(COMP_EPROM, "Autoload OK\n"); 
		priv->AutoloadFailFlag=false;
	}	

	if (priv->AutoloadFailFlag == true)
	{
		rtl8192se_config_hw_for_load_fail(dev);
		return;
	}

	rtl8192se_get_IC_Inferiority(dev);
	
	priv->eeprom_vid		= *(u16 *)&hwinfo[EEPROM_VID];
	priv->eeprom_did         = *(u16 *)&hwinfo[EEPROM_DID];
	priv->eeprom_svid		= *(u16 *)&hwinfo[EEPROM_SVID];
	priv->eeprom_smid		= *(u16 *)&hwinfo[EEPROM_SMID];
	priv->eeprom_version 	= *(u16 *)&hwinfo[EEPROM_Version];

	RT_TRACE(COMP_EPROM, "EEPROMId = 0x%4x\n", EEPROMId);
	RT_TRACE(COMP_EPROM, "EEPROM VID = 0x%4x\n", priv->eeprom_vid);
	RT_TRACE(COMP_EPROM, "EEPROM DID = 0x%4x\n", priv->eeprom_did);
	RT_TRACE(COMP_EPROM, "EEPROM SVID = 0x%4x\n", priv->eeprom_svid);
	RT_TRACE(COMP_EPROM, "EEPROM SMID = 0x%4x\n", priv->eeprom_smid);

	priv->EEPROMOptional = *(u8 *)&hwinfo[EEPROM_Optional]; 
    	priv->ShowRateMode = 2;
	priv->rtllib->bForcedShowRxRate = false;

	if(priv->ShowRateMode == 0) {
	    if((priv->EEPROMOptional & BIT3) == 0x08/*0000_1000*/) {
		priv->rtllib->bForcedShowRxRate = true;
	    }
	} else if(priv->ShowRateMode == 2){
		  priv->rtllib->bForcedShowRxRate = true;
	}
	
	for(i = 0; i < 6; i += 2)
	{
		usValue = *(u16 *)&hwinfo[EEPROM_MAC_ADDR+i];
		*((u16*)(&dev->dev_addr[i])) = usValue;
	}
	for (i=0;i<6;i++)
		write_nic_byte(dev, MACIDR0+i, dev->dev_addr[i]); 	

	RT_TRACE(COMP_EPROM, "ReadAdapterInfo8192S(), Permanent Address = %02x-%02x-%02x-%02x-%02x-%02x\n", 
	dev->dev_addr[0], dev->dev_addr[1], dev->dev_addr[2], dev->dev_addr[3], 
	dev->dev_addr[4], dev->dev_addr[5]); 	


	for (rf_path = 0; rf_path < 2; rf_path++)
	{
		for (i = 0; i < 3; i++)
		{
			priv->RfCckChnlAreaTxPwr[rf_path][i] = 
			hwinfo[EEPROM_TxPowerBase+rf_path*3+i];

			priv->RfOfdmChnlAreaTxPwr1T[rf_path][i] = 
			hwinfo[EEPROM_TxPowerBase+6+rf_path*3+i];

			priv->RfOfdmChnlAreaTxPwr2T[rf_path][i] = 
			hwinfo[EEPROM_TxPowerBase+12+rf_path*3+i];
		}
	}

	for (rf_path = 0; rf_path < 2; rf_path++)
	{
		for (i = 0; i < 3; i++)
		{
			RT_TRACE(COMP_EPROM,"CCK RF-%d CHan_Area-%d = 0x%x\n",  rf_path, i,
			priv->RfCckChnlAreaTxPwr[rf_path][i]);
			RT_TRACE(COMP_EPROM, "OFDM-1T RF-%d CHan_Area-%d = 0x%x\n",  rf_path, i,
			priv->RfOfdmChnlAreaTxPwr1T[rf_path][i]);
			RT_TRACE(COMP_EPROM,"OFDM-2T RF-%d CHan_Area-%d = 0x%x\n",  rf_path, i,
			priv->RfOfdmChnlAreaTxPwr2T[rf_path][i]);			
		}

		for(i=0; i<14; i++)	
		{
			if (i < 3)			
				index = 0;
			else if (i < 8)		
				index = 1;
			else				
				index = 2;

			priv->RfTxPwrLevelCck[rf_path][i]  = 
			priv->RfCckChnlAreaTxPwr[rf_path][index];
			priv->RfTxPwrLevelOfdm1T[rf_path][i]  = 
			priv->RfOfdmChnlAreaTxPwr1T[rf_path][index];
			priv->RfTxPwrLevelOfdm2T[rf_path][i]  = 
			priv->RfOfdmChnlAreaTxPwr2T[rf_path][index];

			if (rf_path == 0)
			{
				priv->TxPowerLevelOFDM24G[i] = priv->RfTxPwrLevelOfdm1T[rf_path][i] ;
				priv->TxPowerLevelCCK[i] = priv->RfTxPwrLevelCck[rf_path][i];					
                    }
		}

		for(i=0; i<14; i++)
		{
			RT_TRACE(COMP_EPROM, "Rf-%d TxPwr CH-%d CCK OFDM_1T OFDM_2T= 0x%x/0x%x/0x%x\n", 
			rf_path, i, priv->RfTxPwrLevelCck[rf_path][i], 
			priv->RfTxPwrLevelOfdm1T[rf_path][i] ,
			priv->RfTxPwrLevelOfdm2T[rf_path][i] );
		}
	}	
	for (rf_path = 0; rf_path < 2; rf_path++)
	{
		for (i = 0; i < 3; i++)
		{
			priv->EEPROMPwrGroup[rf_path][i] = hwinfo[EEPROM_TxPWRGroup+rf_path*3+i];
		}
	}

	for (rf_path = 0; rf_path < 2; rf_path++)
	{
		for(i=0; i<14; i++)
		{
			if (i < 3)			
				index = 0;
			else if (i < 8)		
				index = 1;
			else				
				index = 2;
			priv->PwrGroupHT20[rf_path][i] = (priv->EEPROMPwrGroup[rf_path][index]&0xf);
			priv->PwrGroupHT40[rf_path][i] = ((priv->EEPROMPwrGroup[rf_path][index]&0xf0)>>4);
			RT_TRACE(COMP_INIT, "RF-%d PwrGroupHT20[%d] = 0x%x\n", rf_path, i, priv->PwrGroupHT20[rf_path][i]);
			RT_TRACE(COMP_INIT, "RF-%d PwrGroupHT40[%d] = 0x%x\n", rf_path, i, priv->PwrGroupHT40[rf_path][i]);
		}
	}

	for(i=0; i<14; i++)	
	{
		if (i < 3)			
			index = 0;
			else if (i < 8)		
			index = 1;
		else				
			index = 2;
		
		tempval = (*(u8 *)&hwinfo[EEPROM_TX_PWR_HT20_DIFF+index])&0xff;
		priv->TxPwrHt20Diff[RF90_PATH_A][i] = (tempval&0xF);
		priv->TxPwrHt20Diff[RF90_PATH_B][i] = ((tempval>>4)&0xF);

		if (i < 3)			
			index = 0;
		else if (i < 8)		
			index = 0x11;
		else				
			index = 1;

		tempval = (*(u8 *)&hwinfo[EEPROM_TX_PWR_OFDM_DIFF+index])&0xff;
		priv->TxPwrLegacyHtDiff[RF90_PATH_A][i] = (tempval&0xF);
		priv->TxPwrLegacyHtDiff[RF90_PATH_B][i] = ((tempval>>4)&0xF);

		tempval = (*(u8 *)&hwinfo[TX_PWR_SAFETY_CHK]);
		priv->TxPwrSafetyFlag = (tempval&0x01);		
	}
	
	priv->EEPROMRegulatory = 0;
	if(priv->eeprom_version >= 2)
	{
		if(priv->eeprom_version >= 4)
			priv->EEPROMRegulatory = (hwinfo[EEPROM_Regulatory]&0x7);	
		else
			priv->EEPROMRegulatory = (hwinfo[EEPROM_Regulatory]&0x1);	
	}
	RT_TRACE(COMP_INIT, "EEPROMRegulatory = 0x%x\n", priv->EEPROMRegulatory);

	for(i=0; i<14; i++)
		RT_TRACE(COMP_EPROM, "RF-A Ht20 to HT40 Diff[%d] = 0x%x\n", i, priv->TxPwrHt20Diff[RF90_PATH_A][i]);
	for(i=0; i<14; i++)
		RT_TRACE(COMP_EPROM, "RF-A Legacy to Ht40 Diff[%d] = 0x%x\n", i, priv->TxPwrLegacyHtDiff[RF90_PATH_A][i]);
	for(i=0; i<14; i++)
		RT_TRACE(COMP_EPROM, "RF-B Ht20 to HT40 Diff[%d] = 0x%x\n", i, priv->TxPwrHt20Diff[RF90_PATH_B][i]);
	for(i=0; i<14; i++)
		RT_TRACE(COMP_EPROM, "RF-B Legacy to HT40 Diff[%d] = 0x%x\n", i, priv->TxPwrLegacyHtDiff[RF90_PATH_B][i]);

	RT_TRACE(COMP_EPROM, "TxPwrSafetyFlag = %d\n", priv->TxPwrSafetyFlag);

	tempval = (*(u8 *)&hwinfo[EEPROM_RFInd_PowerDiff])&0xff;
	priv->EEPROMTxPowerDiff = tempval;	
	priv->LegacyHTTxPowerDiff = priv->TxPwrLegacyHtDiff[RF90_PATH_A][0];

	RT_TRACE(COMP_EPROM, "TxPowerDiff = %#x\n", priv->EEPROMTxPowerDiff);

	usValue = *(u16 *)&hwinfo[EEPROM_TSSI_A];
	priv->EEPROMTSSI_A = (u8)((usValue&0xff00)>>8);
	usValue = *(u8 *)&hwinfo[EEPROM_TSSI_B];
	priv->EEPROMTSSI_B = (u8)(usValue&0xff);
	RT_TRACE(COMP_EPROM, "TSSI_A = %#x, TSSI_B = %#x\n", 
			priv->EEPROMTSSI_A, priv->EEPROMTSSI_B);
		
	tempval = *(u8 *)&hwinfo[EEPROM_ThermalMeter];
	priv->EEPROMThermalMeter = tempval;			
	RT_TRACE(COMP_EPROM, "ThermalMeter = %#x\n", priv->EEPROMThermalMeter);
	priv->ThermalMeter[0] =(priv->EEPROMThermalMeter&0x1f);
	priv->TSSI_13dBm = priv->EEPROMThermalMeter *100;
	
	tempval = *(u8 *)&hwinfo[EEPROM_BLUETOOTH_COEXIST];
	priv->EEPROMBluetoothCoexist = ((tempval&0x2)>>1);	
	priv->BluetoothCoexist = priv->EEPROMBluetoothCoexist;
	tempval = hwinfo[EEPROM_BLUETOOTH_TYPE];
	priv->EEPROMBluetoothType = ((tempval&0xe)>>1);
	priv->EEPROMBluetoothAntNum = (tempval&0x1);
	priv->EEPROMBluetoothAntIsolation = ((tempval&0x10)>>4);
	RT_TRACE(COMP_EPROM, "BlueTooth Coexistance = %#x\n", priv->BluetoothCoexist);

#ifdef MERGE_TO_DO
	BT_VAR_INIT(Adapter);

	tempval = hwinfo[EEPROM_WoWLAN];
	priv->EEPROMSupportWoWLAN = ((tempval&0x4)>>2); 
	if (priv->EEPROMSupportWoWLAN)
		priv->bHwSupportRemoteWakeUp = true;
	else
		priv->bHwSupportRemoteWakeUp = false;
#endif
#ifdef RTL8192S_WAPI_SUPPORT
	tempval = *(u8 *)&hwinfo[EEPROM_WAPI_SUPPORT];
	priv->EEPROMWapiSupport = ((tempval&0x10)>>4);	
	priv->EEPROMWapiSupport = 1;

	priv->WapiSupport = priv->EEPROMWapiSupport;
	priv->rtllib->WapiSupport = priv->WapiSupport;		
	RT_TRACE(COMP_EPROM, "WAPI Support = %#x\n", priv->WapiSupport);
#endif

	tempval = (*(u8 *)&hwinfo[EEPROM_CrystalCap])>>4;
	priv->EEPROMCrystalCap =tempval;		
	RT_TRACE(COMP_EPROM, "CrystalCap = %#x\n", priv->EEPROMCrystalCap);
	priv->CrystalCap = priv->EEPROMCrystalCap;	
	
	priv->eeprom_ChannelPlan = *(u8 *)&hwinfo[EEPROM_ChannelPlan];
	priv->bTXPowerDataReadFromEEPORM = true;
	RT_TRACE(COMP_EPROM, "EEPROM ChannelPlan = 0x%4x\n", priv->eeprom_ChannelPlan);
	
	tempval = *(u8*)&hwinfo[EEPROM_BoardType];  
	if (tempval == 0)	
		priv->rf_type= RF_2T2R;
	else if (tempval == 1)	 
		priv->rf_type = RF_1T2R;
	else if (tempval == 2)	 
		priv->rf_type = RF_1T2R;
	else if (tempval == 3)	 
		priv->rf_type = RF_1T1R;

	priv->rtllib->RF_Type = priv->rf_type;
	priv->rtllib->b1x1RecvCombine = false;
	if (priv->rf_type == RF_1T2R)
	{
		tempval = read_nic_byte(dev, 0x07);
		if (!(tempval & BIT0))
		{
			priv->rtllib->b1x1RecvCombine = true;
			RT_TRACE(COMP_INIT, "RF_TYPE=1T2R but only 1SS\n");
		}
	}
	priv->rtllib->b1SSSupport = 	priv->rtllib->b1x1RecvCombine;
	
	priv->rf_chip = RF_6052;

	priv->eeprom_CustomerID = *(u8 *)&hwinfo[EEPROM_CustomID];

	RT_TRACE(COMP_EPROM, "EEPROM Customer ID: 0x%2x, rf_chip:%x\n", priv->eeprom_CustomerID, priv->rf_chip);
	
	priv->rtllib->b_customer_lenovo_id = false;
	
	switch(priv->eeprom_CustomerID)
	{	
		case EEPROM_CID_DEFAULT:

			if(priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
					priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8190)
				priv->CustomerID = RT_CID_819x_Lenovo;	
			else if(priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
					priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8191)
					priv->CustomerID = RT_CID_819x_Lenovo;	
			else	if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8172 &&
					priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0xE020)
					priv->CustomerID = RT_CID_819x_Lenovo;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8172 &&
					priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0xE025)
					priv->CustomerID = RT_CID_819x_Lenovo;
			else if(priv->eeprom_svid == 0x1A32 && priv->eeprom_smid == 0x0308)
					priv->CustomerID = RT_CID_819x_QMI;											
			else if(priv->eeprom_svid == 0x1A32 && priv->eeprom_smid == 0x0311)
					priv->CustomerID = RT_CID_819x_QMI;
			else if(priv->eeprom_svid == 0x1462 && priv->eeprom_smid == 0x6897)
				priv->CustomerID = RT_CID_819x_MSI;
			else if(priv->eeprom_svid == 0x1462 && priv->eeprom_smid == 0x3821)
				priv->CustomerID = RT_CID_819x_MSI;			
			else if(priv->eeprom_svid == 0x1462 && priv->eeprom_smid == 0x897A)
				priv->CustomerID = RT_CID_819x_MSI;
			/****************************************************************************/
			/* 	ACER ID																	*/
			/****************************************************************************/
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
					priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7172)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
					priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7173)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
					priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8186)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
					priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8187)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
					priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8156)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
					priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8157)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
					priv->eeprom_svid == 0x1025 && priv->eeprom_smid == 0x7172)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
					priv->eeprom_svid == 0x1025 && priv->eeprom_smid == 0x7173)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
					priv->eeprom_svid == 0x1025 && priv->eeprom_smid == 0x8186)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
					priv->eeprom_svid == 0x1025 && priv->eeprom_smid == 0x8187)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
					priv->eeprom_svid == 0x1025 && priv->eeprom_smid == 0x8156)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
					priv->eeprom_svid == 0x1025 && priv->eeprom_smid == 0x8157)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8172 &&
					priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0xE021)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8172 &&
					priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0xE022)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8172 &&
					priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8158)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8172 &&
					priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8159)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8172 &&
					priv->eeprom_svid == 0x1025 && priv->eeprom_smid == 0xE021)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8172 &&
					priv->eeprom_svid == 0x1025 && priv->eeprom_smid == 0xE022)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8172 &&
					priv->eeprom_svid == 0x1025 && priv->eeprom_smid == 0x8158)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8172 &&
					priv->eeprom_svid == 0x1025 && priv->eeprom_smid == 0x8159)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8174 &&
					priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7186)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8174 &&
					priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7187)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8174 &&
					priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8186)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8174 &&
					priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8187)
				priv->CustomerID = RT_CID_819x_Acer;
				else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8174 &&
					priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8156)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8174 &&
					priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8157)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8174 &&
					priv->eeprom_svid == 0x1025 && priv->eeprom_smid == 0x7186)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8174 &&
					priv->eeprom_svid == 0x1025 && priv->eeprom_smid == 0x7187)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8174 &&
					priv->eeprom_svid == 0x1025 && priv->eeprom_smid == 0x8186)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8174 &&
					priv->eeprom_svid == 0x1025 && priv->eeprom_smid == 0x8187)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8174 &&
					priv->eeprom_svid == 0x1025 && priv->eeprom_smid == 0x8156)
				priv->CustomerID = RT_CID_819x_Acer;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8174 &&
					priv->eeprom_svid == 0x1025 && priv->eeprom_smid == 0x8157)
				priv->CustomerID = RT_CID_819x_Acer;
			/****************************************************************************/
			/* 	ACER ID  END																*/
			/****************************************************************************/				

			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
			 		priv->eeprom_svid == 0x103C && priv->eeprom_smid == 0x1467)
			 priv->CustomerID = RT_CID_819x_HP;
			
			else if(priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
	 			priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8192)
				priv->CustomerID = RT_CID_819x_Foxcoon;
			else if(priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
	 			priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8193)
				priv->CustomerID = RT_CID_819x_Foxcoon;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
		 			priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8188)
				priv->CustomerID = RT_CID_819x_SAMSUNG;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8171 &&
		 			priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8189)
				priv->CustomerID = RT_CID_819x_SAMSUNG;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8172 &&
		 			priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0xE023)
				priv->CustomerID = RT_CID_819x_SAMSUNG;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8172 &&
		 			priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0xE024)
				priv->CustomerID = RT_CID_819x_SAMSUNG;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8174 &&
		 			priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8190)
				priv->CustomerID = RT_CID_819x_SAMSUNG;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8174 &&
		 			priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8191)
				priv->CustomerID = RT_CID_819x_SAMSUNG;
			else if(	priv->eeprom_vid == 0x10EC && priv->eeprom_did == 0x8174 &&
		 			priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8192)
				priv->CustomerID = RT_CID_819x_SAMSUNG;
			else
			priv->CustomerID = RT_CID_DEFAULT;
			break;
			
		case EEPROM_CID_TOSHIBA:       
			priv->CustomerID = RT_CID_TOSHIBA;
			break;

		case EEPROM_CID_QMI:
			priv->CustomerID = RT_CID_819x_QMI;
			break;
			
		case EEPROM_CID_WHQL:
#ifdef TO_DO_LIST			
			priv->bInHctTest = true;
			priv->bSupportTurboMode = false;
			priv->bAutoTurboBy8186 = false;
			priv->PowerSaveControl.bInactivePs = false;
			priv->PowerSaveControl.bIPSModeBackup = false;
			priv->PowerSaveControl.bLeisurePs = false;
			priv->keepAliveLevel = 0;	
			priv->bUnloadDriverwhenS3S4 = false;
#endif
			break;
				
		default:
			priv->CustomerID = RT_CID_DEFAULT;
			break;
				
	}
	
#if defined (RTL8192S_WAPI_SUPPORT)
	if (priv->rtllib->WapiSupport)
	{
            WapiInit(priv->rtllib);
	}
#endif	
	RT_TRACE(COMP_INIT, "<==== rtl8192se_read_eeprom_info\n");
}

void rtl8192se_get_eeprom_size(struct net_device* dev)
{
	struct r8192_priv* priv = rtllib_priv(dev);
	u8 curCR = 0;
	curCR = read_nic_byte(dev, EPROM_CMD);
	PHY_RFShadowRefresh(dev);
	if (curCR & BIT4){
		RT_TRACE(COMP_EPROM, "Boot from EEPROM\n");
		priv->epromtype = EEPROM_93C46;
	}
	else{
		RT_TRACE(COMP_EPROM, "Boot from EFUSE\n");
		priv->epromtype = EEPROM_BOOT_EFUSE;
	}
	if (curCR & BIT5){
		RT_TRACE(COMP_EPROM,"Autoload OK\n"); 
		priv->AutoloadFailFlag=false;		
		rtl8192se_read_eeprom_info(dev);
	}
	else{
		RT_TRACE(COMP_INIT, "AutoLoad Fail reported from CR9346!!\n"); 
		priv->AutoloadFailFlag=true;		
		rtl8192se_config_hw_for_load_fail(dev);		

		if (priv->epromtype == EEPROM_BOOT_EFUSE)
		{
#if 1
			EFUSE_ShadowMapUpdate(dev);
#endif
		}
	}			
#ifdef TO_DO_LIST
	if(Adapter->bInHctTest)
	{
		pMgntInfo->PowerSaveControl.bInactivePs = false;
		pMgntInfo->PowerSaveControl.bIPSModeBackup = false;
		pMgntInfo->PowerSaveControl.bLeisurePs = false;
		pMgntInfo->keepAliveLevel = 0;
	}
#endif	
#ifdef ENABLE_DOT11D
	priv->ChannelPlan = COUNTRY_CODE_WORLD_WIDE_13;

	if(priv->ChannelPlan == COUNTRY_CODE_GLOBAL_DOMAIN) {
		GET_DOT11D_INFO(priv->rtllib)->bEnabled = 1;
		RT_TRACE(COMP_INIT, "%s: Enable dot11d when RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN!\n", __FUNCTION__);
	}
#endif
	
	RT_TRACE(COMP_INIT, "RegChannelPlan(%d) EEPROMChannelPlan(%d)", \
			priv->RegChannelPlan, priv->eeprom_ChannelPlan);
	RT_TRACE(COMP_INIT, "ChannelPlan = %d\n" , priv->ChannelPlan);
	HalCustomizedBehavior8192S(dev);
}

#if 0
static void MacConfigBeforeFwDownload(struct net_device* dev)
{
	struct r8192_priv* priv = rtllib_priv(dev);
	u8				i;
	u8				tmpU1b;
	u16				tmpU2b;
	u32				addr;
	
	RT_TRACE(COMP_INIT, "Set some register before enable NIC\r\n");

	tmpU1b = read_nic_byte(dev, 0x562);
	tmpU1b |= 0x08;
	write_nic_byte(dev, 0x562, tmpU1b);
	tmpU1b &= ~(BIT3);
	write_nic_byte(dev, 0x562, tmpU1b);
	
	tmpU1b = read_nic_byte(dev, SYS_FUNC_EN+1);
	tmpU1b &= 0x73;
	write_nic_byte(dev, SYS_FUNC_EN+1, tmpU1b);

	tmpU1b = read_nic_byte(dev, SYS_CLKR);	
	tmpU1b &= 0xfa;
	write_nic_byte(dev, SYS_CLKR, tmpU1b);

	RT_TRACE(COMP_INIT, "Delay 1000ms before reset NIC. I dont know how long should we delay!!!!!\r\n");	
	ssleep(1);

	write_nic_byte(dev, SYS_CLKR, SYS_CLKSEL_80M);
	
	tmpU1b = read_nic_byte(dev, SPS1_CTRL);	
	write_nic_byte(dev, SPS1_CTRL, (tmpU1b|SPS1_LDEN));
	
	tmpU1b = read_nic_byte(dev, AFE_MISC);	
	write_nic_byte(dev, AFE_MISC, (tmpU1b|AFE_BGEN));

	tmpU1b = read_nic_byte(dev, LDOA15_CTRL);	
	write_nic_byte(dev, LDOA15_CTRL, (tmpU1b|LDA15_EN));

	tmpU1b = read_nic_byte(dev, SPS1_CTRL);	
	write_nic_byte(dev, SPS1_CTRL, (tmpU1b|SPS1_SWEN));

	tmpU1b = read_nic_byte(dev, AFE_MISC);	
	write_nic_byte(dev, AFE_MISC, (tmpU1b|AFE_MBEN));

	tmpU2b = read_nic_word(dev, SYS_ISO_CTRL);	
	write_nic_word(dev, SYS_ISO_CTRL, (tmpU2b|ISO_PWC_DV2RP));

	tmpU2b = read_nic_word(dev, SYS_ISO_CTRL);	
	write_nic_word(dev, SYS_ISO_CTRL, (tmpU2b &(~ISO_PWC_RV2RP)));

	tmpU2b = read_nic_word(dev, AFE_XTAL_CTRL);	
	write_nic_word(dev, AFE_XTAL_CTRL, (tmpU2b &(~XTAL_GATE_AFE)));

	tmpU1b = read_nic_byte(dev, AFE_PLL_CTRL);	
	write_nic_byte(dev, AFE_PLL_CTRL, (tmpU1b|APLL_EN));

	write_nic_byte(dev, SYS_ISO_CTRL, 0xEE);

	tmpU2b = read_nic_word(dev, SYS_CLKR);	
	write_nic_word(dev, SYS_CLKR, (tmpU2b|SYS_MAC_CLK_EN));

	tmpU2b = read_nic_word(dev, SYS_FUNC_EN);	
	write_nic_word(dev, SYS_FUNC_EN, (tmpU2b|FEN_DCORE|FEN_MREGEN));

	tmpU2b = read_nic_word(dev, SYS_CLKR);	
	write_nic_word(dev, SYS_CLKR, ((tmpU2b|SYS_FWHW_SEL)&(~SYS_SWHW_SEL)));

	write_nic_byte(dev, RF_CTRL, 0);
	write_nic_byte(dev, RF_CTRL, 7);

	write_nic_word(dev, CMDR, 0x37FC);	
	
#if 1
	write_nic_byte(dev, 0x6, 0x30);
	write_nic_byte(dev, 0x49, 0xf0);

	write_nic_byte(dev, 0x4b, 0x81);

	write_nic_byte(dev, 0xb5, 0x21);

	write_nic_byte(dev, 0xdc, 0xff);
	write_nic_byte(dev, 0xdd, 0xff);	
	write_nic_byte(dev, 0xde, 0xff);
	write_nic_byte(dev, 0xdf, 0xff);

	write_nic_byte(dev, 0x11a, 0x00);
	write_nic_byte(dev, 0x11b, 0x00);

	for (i = 0; i < 32; i++)
		write_nic_byte(dev, INIMCS_SEL+i, 0x1b);	

	write_nic_byte(dev, 0x236, 0xff);
	
	write_nic_byte(dev, 0x503, 0x22);

	if(priv->bSupportASPM){
		write_nic_byte(dev, 0x560, 0x40);
	} else {
		write_nic_byte(dev, 0x560, 0x00);
	}

	write_nic_byte(dev, DBG_PORT, 0x91);	
#endif

#ifdef CONFIG_RX_CMD 
	write_nic_dword(dev, RCDA, priv->rx_ring_dma[RX_CMD_QUEUE]);
#endif
	write_nic_dword(dev, RDQDA, priv->rx_ring_dma[RX_MPDU_QUEUE]);
	rtl8192_tx_enable(dev);
	
	RT_TRACE(COMP_INIT, "<---MacConfig8192SE()\n");

}	/* MacConfigBeforeFwDownload */
#else
void gen_RefreshLedState(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PLED_8190 pLed0 = &(priv->SwLed0);	

	if(priv->bfirst_init)
	{
		RT_TRACE(COMP_INIT, "gen_RefreshLedState first init\n");
		return;
	}

	if(priv->rtllib->RfOffReason == RF_CHANGE_BY_IPS )
	{
		SwLedOn(dev, pLed0);
	}
	else		
	{
		SwLedOff(dev, pLed0);		
	}

}	
void MacConfigBeforeFwDownload(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);	
	u8				i;
	u8				tmpU1b;
	u16				tmpU2b;
	u8				PollingCnt = 20;

	RT_TRACE(COMP_INIT, "--->MacConfigBeforeFwDownload()\n");

	if (priv->initialized_at_probe) {
		return;
	}

	if(priv->bfirst_init)
	{
		tmpU1b = read_nic_byte(dev, SYS_FUNC_EN+1);
		tmpU1b &= 0xFE;
		write_nic_byte(dev, SYS_FUNC_EN+1, tmpU1b);
		udelay(1);
		write_nic_byte(dev, SYS_FUNC_EN+1, tmpU1b|BIT0);
	}

	tmpU1b = read_nic_byte(dev, (SYS_CLKR + 1));
	if(tmpU1b & BIT7)
	{
		tmpU1b &= ~(BIT6 | BIT7);
		if(!HalSetSysClk8192SE(dev, tmpU1b))
			return; 
	}

#ifdef MERGE_TO_DO
	if( (pHalData->bt_coexist.BluetoothCoexist) &&
		(pHalData->bt_coexist.BT_CoexistType == BT_CSR) )
	{
		write_nic_byte(dev, AFE_PLL_CTRL, 0x0);
		udelay(50);
		write_nic_byte(dev, LDOA15_CTRL, 0x34);
		udelay(50);
	}
	else
#endif
	{
        write_nic_byte(dev, AFE_PLL_CTRL, 0x0);
		udelay(50);
		write_nic_byte(dev, LDOA15_CTRL, 0x34);
		udelay(50);
	}
	

	write_nic_byte(dev, RPWM, 0x0);

	tmpU1b = read_nic_byte(dev, SYS_FUNC_EN+1);
	tmpU1b &= 0x73;
	write_nic_byte(dev, SYS_FUNC_EN+1, tmpU1b);
	mdelay(1);
	
	write_nic_byte(dev, CMDR, 0);
	write_nic_byte(dev, TCR, 0);

#if 0
	tmpU1b = read_nic_byte(dev, SPS1_CTRL);
	tmpU1b &= 0xfc;
	write_nic_byte(dev, SPS1_CTRL, tmpU1b);
#endif

	tmpU1b = read_nic_byte(dev, 0x562);
	tmpU1b |= 0x08;
	write_nic_byte(dev, 0x562, tmpU1b);
	tmpU1b &= ~(BIT3);
	write_nic_byte(dev, 0x562, tmpU1b);
	


	

	RT_TRACE(COMP_INIT, "Enable AFE clock source\r\n");	
	tmpU1b = read_nic_byte(dev, AFE_XTAL_CTRL);	
	write_nic_byte(dev, AFE_XTAL_CTRL, (tmpU1b|0x01));
	udelay(2000);	
	tmpU1b = read_nic_byte(dev, AFE_XTAL_CTRL+1);	
	write_nic_byte(dev, AFE_XTAL_CTRL+1, (tmpU1b&0xfb));


	RT_TRACE(COMP_INIT, "Enable AFE Macro Block's Bandgap\r\n");	
	tmpU1b = read_nic_byte(dev, AFE_MISC);	
	write_nic_byte(dev, AFE_MISC, (tmpU1b|BIT0));
	mdelay(1);

	RT_TRACE(COMP_INIT, "Enable AFE Mbias\r\n");	
	tmpU1b = read_nic_byte(dev, AFE_MISC);	
	write_nic_byte(dev, AFE_MISC, (tmpU1b|0x02));
	mdelay(1);
	
	tmpU1b = read_nic_byte(dev, LDOA15_CTRL);	
	write_nic_byte(dev, LDOA15_CTRL, (tmpU1b|BIT0));



	tmpU2b = read_nic_word(dev, SYS_ISO_CTRL);	
	write_nic_word(dev, SYS_ISO_CTRL, (tmpU2b|BIT11));


	tmpU2b = read_nic_word(dev, SYS_FUNC_EN);
	write_nic_word(dev, SYS_FUNC_EN, (tmpU2b |BIT13));


	write_nic_byte(dev, SYS_ISO_CTRL+1, 0x68);

	udelay(200);
	tmpU1b = read_nic_byte(dev, AFE_PLL_CTRL);	
	write_nic_byte(dev, AFE_PLL_CTRL, (tmpU1b|BIT0|BIT4));

#if 1 
	udelay(100);
	write_nic_byte(dev, AFE_PLL_CTRL, (tmpU1b|BIT0|BIT4|BIT6));
	udelay(10);
	write_nic_byte(dev, AFE_PLL_CTRL, (tmpU1b|BIT0|BIT4));
	udelay(10);
#endif
	tmpU1b = read_nic_byte(dev, AFE_PLL_CTRL+1);	
	write_nic_byte(dev, AFE_PLL_CTRL+1, (tmpU1b|BIT0));
	mdelay(1);

	write_nic_byte(dev, SYS_ISO_CTRL, 0xA6);

	tmpU2b = read_nic_word(dev, SYS_CLKR);	
	write_nic_word(dev, SYS_CLKR, (tmpU2b|BIT12|BIT11));

	tmpU2b = read_nic_word(dev, SYS_FUNC_EN);	
	write_nic_word(dev, SYS_FUNC_EN, (tmpU2b|BIT11));

	tmpU1b = read_nic_byte(dev, SYS_FUNC_EN+1);
	write_nic_byte(dev, SYS_FUNC_EN+1, tmpU1b&~(BIT7));
	
	write_nic_word(dev, SYS_FUNC_EN, (tmpU2b|BIT11|BIT15));

	 tmpU2b = read_nic_word(dev, SYS_CLKR);	
	write_nic_word(dev, SYS_CLKR, (tmpU2b&(~BIT2)));

	tmpU1b = read_nic_byte(dev, (SYS_CLKR + 1));
	tmpU1b = ((tmpU1b | BIT7) & (~BIT6));
	if(!HalSetSysClk8192SE(dev, tmpU1b))
		return; 

#if 0	
	tmpU2b = read_nic_word(dev, SYS_CLKR);	
	write_nic_word(dev, SYS_CLKR, ((tmpU2b|BIT15)&(~BIT14)));
#endif

	write_nic_word(dev, CMDR, 0x07FC);
	
#if 1
	write_nic_byte(dev, 0x6, 0x30);
	write_nic_byte(dev, 0x49, 0xf0);

	write_nic_byte(dev, 0x4b, 0x81);

	write_nic_byte(dev, 0xb5, 0x21);

	write_nic_byte(dev, 0xdc, 0xff);
	write_nic_byte(dev, 0xdd, 0xff);	
	write_nic_byte(dev, 0xde, 0xff);
	write_nic_byte(dev, 0xdf, 0xff);

	write_nic_byte(dev, 0x11a, 0x00);
	write_nic_byte(dev, 0x11b, 0x00);

	for (i = 0; i < 32; i++)
		write_nic_byte(dev, INIMCS_SEL+i, 0x1b);	

	write_nic_byte(dev, 0x236, 0xff);
	
	write_nic_byte(dev, 0x503, 0x22);
	if(priv->bSupportASPM && !priv->bSupportBackDoor){ 
		write_nic_byte(dev, 0x560, 0x40);
	} else {
		write_nic_byte(dev, 0x560, 0x00);
	}

	write_nic_byte(dev, DBG_PORT, 0x91);	
#endif

	write_nic_dword(dev, RDQDA, priv->rx_ring_dma[RX_MPDU_QUEUE]);
#ifdef CONFIG_RX_CMD
	write_nic_dword(dev, RCDA, priv->rx_ring_dma[RX_CMD_QUEUE]);
#endif	
	rtl8192_tx_enable(dev);

	write_nic_word(dev, CMDR, 0x37FC);
	do {
		tmpU1b = read_nic_byte(dev, TCR);
		if((tmpU1b & TXDMA_INIT_VALUE) == TXDMA_INIT_VALUE)
			break;	

		udelay(5);
	} while(PollingCnt--);	
	
	if(PollingCnt <= 0 )
	{
		RT_TRACE(COMP_ERR, "MacConfigBeforeFwDownloadASIC(): Polling TXDMA_INIT_VALUE timeout!! Current TCR(%#x)\n", tmpU1b);
		tmpU1b = read_nic_byte(dev, CMDR);			
		write_nic_byte(dev, CMDR, tmpU1b&(~TXDMA_EN));
		udelay(2);
		write_nic_byte(dev, CMDR, tmpU1b|TXDMA_EN);
	}

	if((priv->rtllib->RfOffReason == RF_CHANGE_BY_IPS) || 
	   (priv->rtllib->RfOffReason == 0))
	{
		PLED_8190 pLed0 = &(priv->SwLed0);	
		RT_RF_POWER_STATE eRfPowerStateToSet;
		eRfPowerStateToSet = RfOnOffDetect(dev);
		if(eRfPowerStateToSet == eRfOn){
			SwLedOn(dev, pLed0);
		}
	}
	
	RT_TRACE(COMP_INIT, "<---MacConfigBeforeFwDownload()\n");

}	/* MacConfigBeforeFwDownload */
#endif

static void MacConfigAfterFwDownload(struct net_device* dev)
{
	u8				i;
	u16				tmpU2b;
	struct r8192_priv* priv = rtllib_priv(dev);

	
	write_nic_byte(dev, CMDR, 
	(u8)(BBRSTn|BB_GLB_RSTn|SCHEDULE_EN|MACRXEN|MACTXEN|DDMA_EN|FW2HW_EN|
	RXDMA_EN|TXDMA_EN|HCI_RXDMA_EN|HCI_TXDMA_EN));
	write_nic_dword(dev, TCR, 
	read_nic_dword(dev, TCR)|TXDMAPRE2FULL);
	write_nic_dword(dev, RCR, priv->ReceiveConfig);

#if 0
	for (i=0;i<6;i++)
	write_nic_byte(dev, MACIDR0+i, dev->dev_addr[i]); 	
#endif
	write_nic_word(dev, SIFS_CCK, 0x0a0a); 
	write_nic_word(dev, SIFS_OFDM, 0x1010);
	write_nic_byte(dev, ACK_TIMEOUT, 0x40);
	
	write_nic_word(dev, BCN_INTERVAL, 100);
	write_nic_word(dev, ATIMWND, 2);	
#ifdef _ENABLE_SW_BEACON
	write_nic_word(dev, BCN_DRV_EARLY_INT, BIT15);
#endif        

#if 0
	write_nic_dword(dev, RQPN1,  
	NUM_OF_PAGE_IN_FW_QUEUE_BK<<0 | NUM_OF_PAGE_IN_FW_QUEUE_BE<<8 |\
	NUM_OF_PAGE_IN_FW_QUEUE_VI<<16 | NUM_OF_PAGE_IN_FW_QUEUE_VO<<24);												
	write_nic_dword(dev, RQPN2, 
	NUM_OF_PAGE_IN_FW_QUEUE_HCCA << 0 | NUM_OF_PAGE_IN_FW_QUEUE_CMD << 8|\
	NUM_OF_PAGE_IN_FW_QUEUE_MGNT << 16 |NUM_OF_PAGE_IN_FW_QUEUE_HIGH << 24);
	write_nic_dword(dev, RQPN3, 
	NUM_OF_PAGE_IN_FW_QUEUE_BCN<<0 | NUM_OF_PAGE_IN_FW_QUEUE_PUB<<8);
	write_nic_byte(dev, LD_RQPN, BIT7);
#endif




	write_nic_byte(dev, RXDMA, 
							read_nic_byte(dev, RXDMA)|BIT6);

	if (priv->card_8192_version== VERSION_8192S_ACUT)
		write_nic_byte(dev, RRSR, 0xf0);
	else if (priv->card_8192_version == VERSION_8192S_BCUT)
		write_nic_byte(dev, RRSR, 0xff);
	write_nic_byte(dev, RRSR+1, 0x01);
	write_nic_byte(dev, RRSR+2, 0x00);

	for (i = 0; i < 8; i++)
	{

		if (priv->card_8192_version == VERSION_8192S_ACUT)
			write_nic_dword(dev, ARFR0+i*4, 0x1f0ff0f0);
#if 0 
		else if (priv->card_8192_version == VERSION_8192S_BCUT)
			write_nic_dword(dev, ARFR0+i*4, 0x1f0ff0f0);
#endif
	}

	write_nic_byte(dev, AGGLEN_LMT_H, 0x0f);
	write_nic_word(dev, AGGLEN_LMT_L, 0x7442);
	write_nic_word(dev, AGGLEN_LMT_L+2, 0xddd7);
	write_nic_word(dev, AGGLEN_LMT_L+4, 0xd772);
	write_nic_word(dev, AGGLEN_LMT_L+6, 0xfffd);

	write_nic_dword(dev, DARFRC, 0x04010000);
	write_nic_dword(dev, DARFRC+4, 0x09070605);
	write_nic_dword(dev, RARFRC, 0x04010000);
	write_nic_dword(dev, RARFRC+4, 0x09070605);

	
	
	write_nic_word(dev, SG_RATE, 0xFFFF);


	write_nic_word(dev, NAV_PROT_LEN, 0x0080);	
	write_nic_byte(dev, CFEND_TH, 0xFF);	
	write_nic_byte(dev, AMPDU_MIN_SPACE, 0x07);
	write_nic_byte(dev, TXOP_STALL_CTRL, 0x00);
	

	/*write_nic_byte(dev, PCIF, ((MXDMA2_NoLimit<<MXDMA2_RX_SHIFT) | \
											(MXDMA2_NoLimit<<MXDMA2_TX_SHIFT) | \
											(1<<MULRW_SHIFT)));*/

	write_nic_byte(dev, RXDRVINFO_SZ, 4);  

	tmpU2b= read_nic_byte(dev, SYS_FUNC_EN);
	write_nic_byte(dev, SYS_FUNC_EN, tmpU2b | BIT13);  
	tmpU2b= read_nic_byte(dev, SYS_ISO_CTRL);
	write_nic_byte(dev, SYS_ISO_CTRL, tmpU2b & (~BIT8));  

	if (priv->epromtype == EEPROM_BOOT_EFUSE)	
	{	
		u8	tempval;		
		
		tempval = read_nic_byte(dev, SYS_ISO_CTRL+1); 
		tempval &= 0xFE;
		write_nic_byte(dev, SYS_ISO_CTRL+1, tempval); 


		
		write_nic_byte(dev, EFUSE_CTRL+3, 0x72); 
		RT_TRACE(COMP_INIT, "EFUSE CONFIG OK\n");
	}
	RT_TRACE(COMP_INIT, "MacConfigAfterFwDownload OK\n");

}	/* MacConfigAfterFwDownload */

static void rtl8192se_HalDetectPwrDownMode(struct net_device*dev)
{
    u8 tmpvalue;
    struct r8192_priv *priv = rtllib_priv(dev);	
    EFUSE_ShadowRead(dev, 1, 0x78, (u32 *)&tmpvalue);

    if (tmpvalue & BIT0) {
        priv->pwrdown = true;
    } else {
        priv->pwrdown = false;
    }
}

void HwConfigureRTL8192SE(struct net_device *dev)
{

	struct r8192_priv* priv = rtllib_priv(dev);
	
	u8	regBwOpMode = 0;
	u32	regRATR = 0, regRRSR = 0;
	u8	regTmp = 0;


	switch(priv->rtllib->mode)
	{
	case WIRELESS_MODE_B:
		regBwOpMode = BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_CCK;
		regRRSR = RATE_ALL_CCK;
		break;
	case WIRELESS_MODE_A:
		regBwOpMode = BW_OPMODE_5G |BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_OFDM_AG;
		regRRSR = RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_G:
		regBwOpMode = BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_AUTO:
	case WIRELESS_MODE_N_24G:
			regBwOpMode = BW_OPMODE_20MHZ;
			regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
			regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_N_5G:
		regBwOpMode = BW_OPMODE_5G;
		regRATR = RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
		regRRSR = RATE_ALL_OFDM_AG;
		break;
	default:
		regBwOpMode = BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		break;
	}

	regTmp = read_nic_byte(dev, INIRTSMCS_SEL);
	regRRSR = ((regRRSR & 0x000fffff)<<8) | regTmp;
	write_nic_dword(dev, INIRTSMCS_SEL, regRRSR);

	write_nic_byte(dev, BW_OPMODE, regBwOpMode);
	priv->rtllib->SetHwRegHandler(dev, HW_VAR_RETRY_LIMIT, (u8*)(&priv->ShortRetryLimit));

	write_nic_byte(dev, MLT, 0x8f);


		
#if 1
	switch(priv->rf_type)
	{
		case RF_1T2R:
		case RF_1T1R:
			RT_TRACE(COMP_INIT, "Initializeadapter: RF_Type%s\n", priv->rf_type==RF_1T1R? "(1T1R)":"(1T2R)");
                priv->rtllib->MinSpaceCfg = (MAX_MSS_DENSITY_1T<<3);						
			break;
		case RF_2T2R:
		case RF_2T2R_GREEN:
			RT_TRACE(COMP_INIT, "Initializeadapter:RF_Type(2T2R)\n");
                priv->rtllib->MinSpaceCfg = (MAX_MSS_DENSITY_2T<<3);			
			break;
	}
	write_nic_byte(dev, AMPDU_MIN_SPACE, priv->rtllib->MinSpaceCfg);
#else
	priv->rtllib->MinSpaceCfg = 0x90;	
	SetHwReg8192SE(dev, HW_VAR_AMPDU_MIN_SPACE, (u8*)(&priv->rtllib->MinSpaceCfg));
#endif
}

void 
RF_RECOVERY(struct net_device*dev	,u8 Start, u8 End)
{
	u8	offset;
	u8	counter;


	for(offset = Start; offset<End; offset++)
	{
		PHY_RFShadowCompareFlagSet(dev, (RF90_RADIO_PATH_E)0, offset,true);
		
		if(PHY_RFShadowCompare(dev, (RF90_RADIO_PATH_E)0, offset))
		{
			PHY_RFShadowRecorverFlagSet(dev, (RF90_RADIO_PATH_E)0, offset, true);

			for (counter = 0; counter < 10; counter++)
			{
				RT_TRACE(COMP_INIT, "PHY_RFShadowCompare OKCNT=%d offset=%0x\n", counter, offset);
			PHY_RFShadowRecorver( dev, (RF90_RADIO_PATH_E)0, offset);
				if (offset == 0x18)
				{
					if (!PHY_RFShadowCompare(dev, (RF90_RADIO_PATH_E)0, offset))
					{
						RT_TRACE(COMP_INIT, "PHY_RFShadowCompare OKCNT=%d\n", counter);
						PHY_RFShadowRecorverFlagSet(dev, (RF90_RADIO_PATH_E)0, offset, false);
						break;
					}
				}
				else
				{
					PHY_RFShadowRecorverFlagSet(dev, (RF90_RADIO_PATH_E)0, offset, false);
					break;
		}
	}
}
	}

}

void	rtl8192se_GPIOBit3CfgInputMode(struct net_device* dev)
{
	u8				u1Tmp;
	
	write_nic_byte(dev, MAC_PINMUX_CFG, (GPIOMUX_EN | GPIOSEL_GPIO));
	u1Tmp = read_nic_byte(dev, GPIO_IO_SEL);
	u1Tmp &= HAL_8192S_HW_GPIO_OFF_MASK;
	write_nic_byte(dev, GPIO_IO_SEL, u1Tmp);
	
}	

bool rtl8192se_adapter_start(struct net_device* dev)
{ 	
	struct r8192_priv *priv = rtllib_priv(dev);	
	bool rtStatus 	= true;	
	u8				tmpU1b;
	u8				eRFPath;
	u8				fw_download_times = 1;
	u8				i=0;
	RT_TRACE(COMP_INIT, "rtl8192se_adapter_start()\n");
	priv->being_init_adapter = true;	
	
	if(priv->pwrdown)
	{
		tmpU1b = read_nic_byte(dev, 0x06);
		if((tmpU1b & BIT6) == 0 || tmpU1b == 0xff) {
			rtStatus = false;
			goto end;
		} else {
		}
	}	

#ifdef CONFIG_ASPM_OR_D3
	RT_DISABLE_ASPM(dev);
#endif
start:
	rtl8192_pci_resetdescring(dev);
	MacConfigBeforeFwDownload(dev);
	priv->initialized_at_probe = false;

	priv->card_8192_version = priv->rtllib->VersionID
	= (VERSION_8192S)((read_nic_dword(dev, PMC_FSM)>>16)&0xF);

	RT_TRACE(COMP_INIT, "NIC version : %s\n", ((read_nic_dword(dev, PMC_FSM)>>15)&0x1)?"C-cut":"B-cut");

	rtl8192se_GPIOBit3CfgInputMode(dev);
	
	rtStatus = FirmwareDownload92S((struct net_device*)dev); 
	if(rtStatus != true)
	{
		if(fw_download_times <= 10){
			RT_TRACE(COMP_INIT, "rtl8192se_adapter_start(): Download Firmware failed %d times, Download again!!\n",fw_download_times);
			fw_download_times = fw_download_times + 1;
			goto start;
		}else{
			RT_TRACE(COMP_INIT, "rtl8192se_adapter_start(): Download Firmware failed 10, end!!\n");
			goto end;
		}
	}

	MacConfigAfterFwDownload(dev);

	priv->FwCmdIOMap = 	read_nic_word(dev, LBUS_MON_ADDR);
	priv->FwCmdIOParam = read_nic_dword(dev, LBUS_ADDR_MASK);

	
#if 0
	write_nic_dword(dev, WFM5, FW_DM_DISABLE); 
	ChkFwCmdIoDone(dev);
	write_nic_dword(dev, WFM5, FW_TXANT_SWITCH_DISABLE); 
	ChkFwCmdIoDone(dev);
#endif

#if 1
	RT_TRACE(COMP_INIT, "MAC Config Start!\n");
	if (PHY_MACConfig8192S(dev) != true)
	{
		RT_TRACE(COMP_ERR, "MAC Config failed\n");
		return rtStatus;
	}
	RT_TRACE(COMP_INIT, "MAC Config Finished!\n");
#endif	

	write_nic_dword(dev, CMDR, 0x37FC); 

#if 1
	RT_TRACE(COMP_INIT, "BB Config Start!\n");
	if (PHY_BBConfig8192S(dev) != true)
	{
		RT_TRACE(COMP_INIT, "BB Config failed\n");
		return rtStatus;
	}
	RT_TRACE(COMP_INIT, "BB Config Finished!\n");
#endif	




	priv->Rf_Mode = RF_OP_By_SW_3wire;


	if(!priv->pwrdown){
		if(priv->RegRfOff == true)
		{ 
			RT_TRACE((COMP_INIT|COMP_RF), "==++==> InitializeAdapter8190(): Turn off RF for RegRfOff ----------\n");
			MgntActSet_RF_State(dev, eRfOff, RF_CHANGE_BY_SW,true);

			for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
				rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, 0x4, 0xC00, 0x0);
		
		}
		else if(priv->rtllib->RfOffReason > RF_CHANGE_BY_PS)
		{ 
			RT_RF_CHANGE_SOURCE rfoffreason = priv->rtllib->RfOffReason;

			RT_TRACE((COMP_INIT|COMP_RF), "InitializeAdapter8190(): ==++==> Turn off RF for RfOffReason(%d) ----------\n", priv->rtllib->RfOffReason);
			printk("InitializeAdapter8190(): ==++==> Turn off RF for RfOffReason(%d) ----------\n", priv->rtllib->RfOffReason);
			
			priv->rtllib->RfOffReason = RF_CHANGE_BY_INIT;
			priv->rtllib->eRFPowerState = eRfOn;
			MgntActSet_RF_State(dev, eRfOff, rfoffreason,true);

		}
		else
		{
#if 0 
			u8				u1Tmp;
			RT_RF_POWER_STATE	eRfPowerStateToSet;
			
			write_nic_byte(dev, MAC_PINMUX_CFG, (GPIOMUX_EN | GPIOSEL_GPIO));
			u1Tmp = read_nic_byte(dev, MAC_PINMUX_CFG);
			u1Tmp = read_nic_byte(dev, GPIO_IO_SEL);
			u1Tmp &= HAL_8192S_HW_GPIO_OFF_MASK;			
			write_nic_byte(dev, GPIO_IO_SEL, u1Tmp);
			u1Tmp = read_nic_byte(dev, GPIO_IO_SEL);
			mdelay(5);
			u1Tmp = read_nic_byte(dev, GPIO_IN);
			RTPRINT(FPWR, PWRHW, ("GPIO_IN=%02x\n", u1Tmp));
			eRfPowerStateToSet = (u1Tmp & HAL_8192S_HW_GPIO_OFF_BIT) ? eRfOn : eRfOff;

			RT_TRACE(COMP_INIT, "==++==> InitializeAdapter8190(): RF=%d \n", eRfPowerStateToSet);
			if (eRfPowerStateToSet == eRfOff)
			{				
				MgntActSet_RF_State(dev, eRfOff, RF_CHANGE_BY_HW, true);
				
		}
		else
		{
				priv->rtllib->RfOffReason = 0; 
			}	
#else					
	            if(priv->bHwRadioOff == false){
			priv->rtllib->eRFPowerState = eRfOn;
			priv->rtllib->RfOffReason = 0; 
			if(priv->rtllib->LedControlHandler)
				;
			}		
#endif
		}	
		}	

#if 1
#if 1
	RT_TRACE(COMP_INIT, "RF Config started!\n");

#if 1
	write_nic_byte(dev, AFE_XTAL_CTRL+1, 0xDB);
	if(priv->card_8192_version== VERSION_8192S_ACUT)
		write_nic_byte(dev, SPS1_CTRL+3, 0x07);
	else
		write_nic_byte(dev, RF_CTRL, 0x07);
#endif	
	if(PHY_RFConfig8192S(dev) != true)
	{
		RT_TRACE(COMP_INIT, "RF Config failed\n");
		return rtStatus;
	}
	RT_TRACE(COMP_INIT, "RF Config Finished!\n");	
#endif	

	priv->RfRegChnlVal[0] = rtl8192_phy_QueryRFReg(dev, (RF90_RADIO_PATH_E)0, RF_CHNLBW, bRFRegOffsetMask);
	priv->RfRegChnlVal[1] = rtl8192_phy_QueryRFReg(dev, (RF90_RADIO_PATH_E)1, RF_CHNLBW, bRFRegOffsetMask);

	/*---- Set CCK and OFDM Block "ON"----*/
	rtl8192_setBBreg(dev, rFPGA0_RFMOD, bCCKEn, 0x1);
	rtl8192_setBBreg(dev, rFPGA0_RFMOD, bOFDMEn, 0x1);
	
	HwConfigureRTL8192SE(dev);



	{
		/* Get original hw reg values */
		PHY_GetHWRegOriginalValue(dev);
		/* Write correct tx power index */
#ifndef CONFIG_MP
		rtl8192_phy_setTxPower(dev, priv->chan);
#endif
	}
#endif


	
	{
		int i;
		for (i=0;i<6;i++)
			write_nic_byte(dev, MACIDR0+i, dev->dev_addr[i]); 	
	}

	tmpU1b = read_nic_byte(dev, MAC_PINMUX_CFG);
	write_nic_byte(dev, MAC_PINMUX_CFG, tmpU1b&(~BIT3));
	
	if(priv->CustomerID == RT_CID_CCX)
	{
		RT_TRACE(COMP_INIT ,"InitializeAdapter8192SE(): Set FW Cmd FW_TX_FEEDBACK_CCX_ENABLE\n");
		write_nic_dword(dev, WFM5, FW_TX_FEEDBACK_CCX_ENABLE); 
		ChkFwCmdIoDone(dev);
		
		write_nic_dword(dev, WFM5, FW_HIGH_PWR_DISABLE); 
		ChkFwCmdIoDone(dev);
		write_nic_dword(dev, WFM5, FW_DIG_HALT);
		ChkFwCmdIoDone(dev);

		write_nic_byte(dev, 0xC50, 0x1C);
		write_nic_byte(dev, 0xC58, 0x1C);
	}
	
		write_nic_byte(dev, 0x4d, 0x0);
	
	if(priv->pFirmware->FirmwareVersion >= 0x49){
		u8 tmp_byte = 0;
		
		tmp_byte = read_nic_byte(dev, FW_RSVD_PG_CRTL) & (~BIT4);
		tmp_byte = tmp_byte | BIT5;
		write_nic_byte(dev, FW_RSVD_PG_CRTL, tmp_byte); 

		write_nic_dword(dev, TXDESC_MSK, 0xFFFFCFFF); 
	}
	
	if(priv->pFirmware->FirmwareVersion >= 0x35)
	{
		priv->rtllib->SetFwCmdHandler(dev, FW_CMD_RA_INIT);
	}
       else if(priv->pFirmware->FirmwareVersion >= 0x34) 
	{
		write_nic_dword(dev, WFM5, FW_RA_INIT); 
		ChkFwCmdIoDone(dev);
	}
	else
	{
	write_nic_dword(dev, WFM5, FW_RA_RESET); 
	ChkFwCmdIoDone(dev);
	write_nic_dword(dev, WFM5, FW_RA_ACTIVE); 
	ChkFwCmdIoDone(dev);
	write_nic_dword(dev, WFM5, FW_RA_REFRESH); 
		ChkFwCmdIoDone(dev);
        }
	
	

		PHY_SwitchEphyParameter(dev);
	RF_RECOVERY(dev, 0x25, 0x29);

	if(priv->ResetProgress == RESET_TYPE_NORESET)
#ifdef ASL
	{
		if (priv->rtllib->iw_mode == IW_MODE_MASTER)
			rtl8192_SetWirelessMode(dev, priv->rtllib->mode, true);
		else
			rtl8192_SetWirelessMode(dev, priv->rtllib->mode, false);
	}
#else
		rtl8192_SetWirelessMode(dev, priv->rtllib->mode);
#endif
	CamResetAllEntry(dev);
	{
		u8 SECR_value = 0x0;
		SECR_value |= SCR_TxEncEnable;
		SECR_value |= SCR_RxDecEnable;
		SECR_value |= SCR_NoSKMC;
		write_nic_byte(dev, SECR, SECR_value);
	}

	{
		int i;
		for (i=0; i<4; i++)
		 write_nic_dword(dev, WDCAPARA_ADD[i], 0x5e4322); 
	}
	
	priv->SilentResetRxSlotIndex = 0;
	for( i=0; i < MAX_SILENT_RESET_RX_SLOT_NUM; i++ )
	{
		priv->SilentResetRxStuckEvent[i] = 0;
	}

	if(priv->BluetoothCoexist)
	{
		printk("Write reg 0x%x = 1 for Bluetooth Co-existance\n", SYSF_CFG);
		write_nic_byte(dev, SYSF_CFG, 0x1);
	}		
	
	priv->bIgnoreSilentReset = true;
	

	if(priv->rf_type ==RF_1T2R)
	{			
		bool		MrcToSet = true;
		printk("InitializeAdapter8192SE(): Set MRC settings on as default!!\n");			
		priv->rtllib->SetHwRegHandler(dev, HW_VAR_MRC, (u8*)&MrcToSet);
	}
	
#ifdef CONFIG_FW_PARSEBEACON
	if (!(priv->rtllib->softmac_features & IEEE_SOFTMAC_SCAN)){
		write_nic_dword(dev, RXFILTERMAP, 0x0100);
	}	
#endif
	rtl8192_irq_enable(dev);
end:

	priv->rtllib->LPSDelayCnt = 10;	

	priv->being_init_adapter = false;
	return rtStatus;

	
}

#ifdef ASL
void rtl8192se_net_update(struct net_device *dev, bool is_ap)
#else
void rtl8192se_net_update(struct net_device *dev)
#endif
{

	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_network *net = NULL;
	u16 rate_config = 0;
	u32 regTmp = 0;
	u8 rateIndex = 0;
	u8	RetryLimit = 0x30;
	u16 cap = 0;
#ifdef ASL
	if (is_ap) {
		net = &priv->rtllib->current_ap_network;
		cap = net->capability;

	} else
#endif
	{
		net = &priv->rtllib->current_network;
		cap = net->capability;
	}
	priv->short_preamble = cap & WLAN_CAPABILITY_SHORT_PREAMBLE;
	
	{
#ifdef ASL
	rtl8192_config_rate(dev, &rate_config, is_ap);	
#else
	rtl8192_config_rate(dev, &rate_config);	
#endif
	if (priv->card_8192_version== VERSION_8192S_ACUT)
		priv->basic_rate = rate_config  = rate_config & 0x150;
	else if (priv->card_8192_version == VERSION_8192S_BCUT)
		priv->basic_rate= rate_config = rate_config & 0x15f;

	priv->dot11CurrentPreambleMode = PREAMBLE_AUTO;

#if 1	
#ifdef ASL
	if (!is_ap) {
#endif
  	if(priv->rtllib->pHTInfo->IOTPeer == HT_IOT_PEER_CISCO && ((rate_config &0x150)==0))
	{
		rate_config |=0x010;
	}
	if(priv->rtllib->pHTInfo->IOTPeer & HT_IOT_ACT_WA_IOT_Broadcom)
	{	
		rate_config &= 0x1f0;
		printk("HW_VAR_BASIC_RATE, HT_IOT_ACT_WA_IOT_Broadcom, BrateCfg = 0x%x\n", rate_config);
	}
#ifdef ASL
	}
#endif
	write_nic_byte(dev, RRSR, rate_config&0xff);
	write_nic_byte(dev, RRSR+1, (rate_config>>8)&0xff);

	while(rate_config > 0x1)
	{
		rate_config = (rate_config>> 1);
		rateIndex++;
	}
	write_nic_byte(dev, INIRTSMCS_SEL, rateIndex);

	regTmp = (priv->nCur40MhzPrimeSC) << 5;
	if (priv->short_preamble)
		regTmp |= 0x80;
	write_nic_byte(dev, RRSR+2, regTmp);
#endif
	}

	write_nic_dword(dev,BSSIDR,((u32*)net->bssid)[0]);
	write_nic_word(dev,BSSIDR+4,((u16*)net->bssid)[2]);
#ifdef ASL
	if (priv->rtllib->iw_mode == IW_MODE_ADHOC || priv->rtllib->iw_mode == IW_MODE_MASTER || (priv->rtllib->iw_mode == IW_MODE_APSTA && is_ap))
#else
	if (priv->rtllib->iw_mode == IW_MODE_ADHOC)
#endif
	{
		RetryLimit = HAL_RETRY_LIMIT_AP_ADHOC;
	} else {
		RetryLimit = (priv->CustomerID == RT_CID_CCX) ? 7 : HAL_RETRY_LIMIT_INFRA;
	}	
	priv->rtllib->SetHwRegHandler(dev, HW_VAR_RETRY_LIMIT, (u8*)(&RetryLimit));
		
#ifdef ASL
	if (priv->rtllib->iw_mode == IW_MODE_ADHOC || priv->rtllib->iw_mode == IW_MODE_MASTER || (priv->rtllib->iw_mode == IW_MODE_APSTA && is_ap))
#else
	if (priv->rtllib->iw_mode == IW_MODE_ADHOC)
#endif
	{
		priv->rtllib->SetHwRegHandler( dev, HW_VAR_BEACON_INTERVAL, (u8*)(&net->beacon_interval));
	}
#ifdef ASL
	rtl8192_update_cap(dev, cap, is_ap);
#else
	rtl8192_update_cap(dev, cap);
#endif
}

#ifdef _RTL8192_EXT_PATCH_
void rtl8192se_mesh_net_update(struct net_device *dev)
{

	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_network *net = &priv->rtllib->current_mesh_network;
	u16 rate_config = 0;
	u32 regTmp = 0;
	u8 rateIndex = 0;
	u8	retrylimit = 0x7;
	u16 cap = net->capability;
	/* At the peer mesh mode, the peer MP shall recognize the short preamble */
	priv->short_preamble = 1;
	
	if (priv->card_8192_version== VERSION_8192S_ACUT)
		priv->basic_rate = rate_config  = 0x150;
	else if (priv->card_8192_version == VERSION_8192S_BCUT)
		priv->basic_rate= rate_config = 0x15f;

	write_nic_byte(dev, RRSR, rate_config&0xff);
	write_nic_byte(dev, RRSR+1, (rate_config>>8)&0xff);

	while(rate_config > 0x1)
	{
		rate_config = (rate_config>> 1);
		rateIndex++;
	}
	write_nic_byte(dev, INIRTSMCS_SEL, rateIndex);

	regTmp = (priv->nCur40MhzPrimeSC) << 5;
	if (priv->short_preamble)
		regTmp |= 0x80;
	write_nic_byte(dev, RRSR+2, regTmp);


	write_nic_word(dev, BCN_INTERVAL, net->beacon_interval);
	PHY_SetBeaconHwReg( dev, net->beacon_interval);
	rtl8192_update_cap(dev, cap);
	
	priv->ShortRetryLimit = 
		priv->LongRetryLimit = retrylimit;
			
	write_nic_word(dev,RETRY_LIMIT, \
			retrylimit << RETRY_LIMIT_SHORT_SHIFT | \
			retrylimit << RETRY_LIMIT_LONG_SHIFT);	
}
#endif
#ifdef ASL
void rtl8192se_ap_link_change(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;
	u32 reg = 0;

	if(IS_NIC_DOWN(priv))
		return;
	printk("=====>%s()\n",__func__);
	priv->rtllib->GetHwRegHandler(dev, HW_VAR_RCR, (u8*)(&reg));
	
#ifdef CONFIG_FW_PARSEBEACON
	if (!(priv->rtllib->softmac_features & IEEE_SOFTMAC_SCAN)){
		write_nic_dword(dev, RXFILTERMAP, 0x0000);
	}	
#endif
	if(priv->DM_Type == DM_Type_ByFW) 
	{
		if(ieee->pHTInfo->IOTAction & HT_IOT_ACT_DISABLE_HIGH_POWER)
			ieee->SetFwCmdHandler(dev, FW_CMD_HIGH_PWR_DISABLE);
		else
			ieee->SetFwCmdHandler(dev, FW_CMD_HIGH_PWR_ENABLE);	
	}

	rtl8192se_net_update(dev, true);
#ifdef ASL
	if(ieee->IntelPromiscuousModeInfo.bPromiscuousOn)
		;
	else {
		if (priv->rtllib->iw_mode == IW_MODE_MASTER)
			reg |= RCR_CBSSID;
	}
#endif
	priv->rtllib->SetHwRegHandler( dev, HW_VAR_RCR, (u8*)(&reg) );
	if ((ieee->bbeacon_start && (ieee->ap_state == RTLLIB_LINKED)) || (ieee->ap_state == RTLLIB_NOLINK)) {
		rtl8192se_update_msr(dev);
		printk("================>update_msr\n");
	} else 
		printk("================>not update msr\n");
	{		
		u32	temp = read_nic_dword(dev, TCR);
		write_nic_dword(dev, TCR, temp&(~BIT8));
		write_nic_dword(dev, TCR, temp|BIT8);
	}
}

#endif
void rtl8192se_link_change(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;
	u32 reg = 0;

	if(IS_NIC_DOWN(priv))
		return;

	priv->rtllib->GetHwRegHandler(dev, HW_VAR_RCR, (u8*)(&reg));
	
	if (ieee->state == RTLLIB_LINKED) {
#ifdef CONFIG_FW_PARSEBEACON
		if (!(priv->rtllib->softmac_features & IEEE_SOFTMAC_SCAN)){
			write_nic_dword(dev, RXFILTERMAP, 0x0000);
		}	
#endif
		if(priv->DM_Type == DM_Type_ByFW) 
		{
			if(ieee->pHTInfo->IOTAction & HT_IOT_ACT_DISABLE_HIGH_POWER)
				ieee->SetFwCmdHandler(dev, FW_CMD_HIGH_PWR_DISABLE);
			else
				ieee->SetFwCmdHandler(dev, FW_CMD_HIGH_PWR_ENABLE);	
		}
#ifdef ASL		
		rtl8192se_net_update(dev, false);
#else
		rtl8192se_net_update(dev);
#endif

		if(ieee->bUseRAMask)
		{
			ieee->UpdateHalRAMaskHandler(
									dev,
									false,
									0,
									ieee->pHTInfo->PeerMimoPs,
					ieee->current_network.mode,
									ieee->pHTInfo->bCurTxBW40MHz,
				0,
					NULL,
					NULL,
					NULL,
					NULL,
					false);
			priv->rssi_level = 0;
		}
		else
		{
			rtl8192se_update_ratr_table(dev,ieee->dot11HTOperationalRateSet,NULL);
		}

		{
			prate_adaptive	pRA =  (prate_adaptive)&priv->rate_adaptive;
			pRA->PreRATRState = DM_RATR_STA_MAX;
		}
		if(ieee->IntelPromiscuousModeInfo.bPromiscuousOn)
			;
		else {
#ifdef ASL
			if (ieee->iw_mode == IW_MODE_INFRA)
#endif
			reg |= RCR_CBSSID;
	} 
	} 
#ifdef _RTL8192_EXT_PATCH_
	else if ((ieee->mesh_state == RTLLIB_MESH_LINKED) && ieee->only_mesh) {
		rtl8192se_mesh_net_update(dev);
		if(ieee->bUseRAMask){
			ieee->UpdateHalRAMaskHandler(
									dev,
									false,
									0,
									ieee->pHTInfo->PeerMimoPs,
									ieee->mode,
									ieee->pHTInfo->bCurTxBW40MHz,
									0,
					NULL,
					NULL,
					NULL,
					NULL,
					false);
			priv->rssi_level = 0;
		}else{
			rtl8192se_update_ratr_table(dev,ieee->dot11HTOperationalRateSet,NULL);
		}
#ifdef CONFIG_FW_PARSEBEACON
		if (!(priv->rtllib->softmac_features & IEEE_SOFTMAC_SCAN)){
			write_nic_dword(dev, RXFILTERMAP, 0x0100);
		}	
#endif
		reg &= ~RCR_CBSSID;
	} 
#endif
	else{
#ifdef CONFIG_FW_PARSEBEACON
		if (!(priv->rtllib->softmac_features & IEEE_SOFTMAC_SCAN)){
			write_nic_dword(dev, RXFILTERMAP, 0x0100);
		}	
#endif
		reg &= ~RCR_CBSSID;
	}
	priv->rtllib->SetHwRegHandler( dev, HW_VAR_RCR, (u8*)(&reg) );
	rtl8192se_update_msr(dev);
	{		
		u32	temp = read_nic_dword(dev, TCR);
		write_nic_dword(dev, TCR, temp&(~BIT8));
		write_nic_dword(dev, TCR, temp|BIT8);
	}
}

void rtl8192se_AllowAllDestAddr(struct net_device* dev,
                        bool bAllowAllDA, bool WriteIntoReg)
{
        struct r8192_priv* priv = rtllib_priv(dev);

	if( bAllowAllDA )
        {
                priv->ReceiveConfig |= RCR_AAP;         
        }
        else
        {
                priv->ReceiveConfig &= ~RCR_AAP;                
        }

        if( WriteIntoReg )
        {
                write_nic_dword( dev, RCR, priv->ReceiveConfig );
        }
}


u8 MRateToHwRate8192SE(struct net_device*dev, u8 rate)
{
	u8	ret = DESC92S_RATE1M;
	u16	max_sg_rate;
	static	u16	multibss_sg_old = 0x1234;
	u16	multibss_sg;
		
	switch(rate)
	{
	case MGN_1M:	ret = DESC92S_RATE1M;		break;
	case MGN_2M:	ret = DESC92S_RATE2M;		break;
	case MGN_5_5M:	ret = DESC92S_RATE5_5M;	break;
	case MGN_11M:	ret = DESC92S_RATE11M;	break;
	case MGN_6M:	ret = DESC92S_RATE6M;		break;
	case MGN_9M:	ret = DESC92S_RATE9M;		break;
	case MGN_12M:	ret = DESC92S_RATE12M;	break;
	case MGN_18M:	ret = DESC92S_RATE18M;	break;
	case MGN_24M:	ret = DESC92S_RATE24M;	break;
	case MGN_36M:	ret = DESC92S_RATE36M;	break;
	case MGN_48M:	ret = DESC92S_RATE48M;	break;
	case MGN_54M:	ret = DESC92S_RATE54M;	break;

	case MGN_MCS0:		ret = DESC92S_RATEMCS0;	break;
	case MGN_MCS1:		ret = DESC92S_RATEMCS1;	break;
	case MGN_MCS2:		ret = DESC92S_RATEMCS2;	break;
	case MGN_MCS3:		ret = DESC92S_RATEMCS3;	break;
	case MGN_MCS4:		ret = DESC92S_RATEMCS4;	break;
	case MGN_MCS5:		ret = DESC92S_RATEMCS5;	break;
	case MGN_MCS6:		ret = DESC92S_RATEMCS6;	break;
	case MGN_MCS7:		ret = DESC92S_RATEMCS7;	break;
	case MGN_MCS8:		ret = DESC92S_RATEMCS8;	break;
	case MGN_MCS9:		ret = DESC92S_RATEMCS9;	break;
	case MGN_MCS10:	ret = DESC92S_RATEMCS10;	break;
	case MGN_MCS11:	ret = DESC92S_RATEMCS11;	break;
	case MGN_MCS12:	ret = DESC92S_RATEMCS12;	break;
	case MGN_MCS13:	ret = DESC92S_RATEMCS13;	break;
	case MGN_MCS14:	ret = DESC92S_RATEMCS14;	break;
	case MGN_MCS15:	ret = DESC92S_RATEMCS15;	break;	

	case MGN_MCS0_SG:	
	case MGN_MCS1_SG:
	case MGN_MCS2_SG:	
	case MGN_MCS3_SG:	
	case MGN_MCS4_SG:	
	case MGN_MCS5_SG:
	case MGN_MCS6_SG:	
	case MGN_MCS7_SG:	
	case MGN_MCS8_SG:	
	case MGN_MCS9_SG:
	case MGN_MCS10_SG:
	case MGN_MCS11_SG:	
	case MGN_MCS12_SG:	
	case MGN_MCS13_SG:	
	case MGN_MCS14_SG:
	case MGN_MCS15_SG:	
			ret = DESC92S_RATEMCS15_SG;
			max_sg_rate = rate&0xf;
			multibss_sg = max_sg_rate | (max_sg_rate<<4) | (max_sg_rate<<8) | (max_sg_rate<<12);
			if (multibss_sg_old != multibss_sg)
			{
				write_nic_dword(dev, SG_RATE, multibss_sg);
				multibss_sg_old = multibss_sg;
			}
			break;


	case (0x80|0x20): 	ret = DESC92S_RATEMCS32; break;

	default:				ret = DESC92S_RATEMCS15;	break;

	}

	return ret;
}

u8 rtl8192se_MapHwQueueToFirmwareQueue(u8 QueueID, u8 priority)
{
	u8 QueueSelect = 0x0;       

	switch(QueueID) {
#if defined RTL8192E || defined RTL8190P
		case BE_QUEUE:
			QueueSelect = QSLT_BE;  
			break;

		case BK_QUEUE:
			QueueSelect = QSLT_BK;  
			break;

		case VO_QUEUE:
			QueueSelect = QSLT_VO;  
			break;

		case VI_QUEUE:
			QueueSelect = QSLT_VI;  
			break;
		case MGNT_QUEUE:
			QueueSelect = QSLT_MGNT;
			break;
		case BEACON_QUEUE:
			QueueSelect = QSLT_BEACON;
			break;
		case TXCMD_QUEUE:
			QueueSelect = QSLT_CMD;
			break;
		case HIGH_QUEUE:
			QueueSelect = QSLT_HIGH;
			break;
#elif defined RTL8192SE
		case BE_QUEUE:
			QueueSelect = priority; 
			break;
		case BK_QUEUE:
			QueueSelect = priority; 
			break;
		case VO_QUEUE:
			QueueSelect = priority; 
			break;
		case VI_QUEUE:
			QueueSelect = priority; 
			break;
		case MGNT_QUEUE:
			QueueSelect = QSLT_BE;
			break;
		case BEACON_QUEUE:
			QueueSelect = QSLT_BEACON;
			break;
		case TXCMD_QUEUE:
			QueueSelect = QSLT_CMD;
			break;
		case HIGH_QUEUE:
			QueueSelect = QSLT_HIGH;
			break;
#endif
		default:
			RT_TRACE(COMP_ERR, "TransmitTCB(): Impossible Queue Selection: %d \n", QueueID);
			break;
	}
	return QueueSelect;
}


void  rtl8192se_tx_fill_desc(struct net_device* dev, tx_desc * pDesc, cb_desc * cb_desc, struct sk_buff* skb)
{
	u8				*pSeq;
	u16				Temp;
	struct r8192_priv* priv = rtllib_priv(dev);

	struct rtllib_hdr_1addr * header = NULL;

	dma_addr_t mapping = pci_map_single(priv->pdev, skb->data, skb->len, PCI_DMA_TODEVICE);

	u16 fc=0, stype=0;
	header = (struct rtllib_hdr_1addr *)(((u8*)skb->data));
	fc = header->frame_ctl;
	stype = WLAN_FC_GET_STYPE(fc);
	memset((void*)pDesc, 0, 32);		

	{
		
		if(priv->rtllib->bUseRAMask){
			if(cb_desc->macId < 32)
			{
				pDesc->MacID = cb_desc->macId;
				pDesc->Rsvd_MacID = cb_desc->macId;
			}
		}
		pDesc->TXHT		= (cb_desc->data_rate&0x80)?1:0;	

#if 1
		if (priv->card_8192_version== VERSION_8192S_ACUT)
		{
			if (cb_desc->data_rate== MGN_1M || cb_desc->data_rate == MGN_2M || 
				cb_desc->data_rate == MGN_5_5M || cb_desc->data_rate == MGN_11M)
			{
				cb_desc->data_rate = MGN_12M;
			}
		}
#endif			
		pDesc->TxRate	= MRateToHwRate8192SE(dev,cb_desc->data_rate);
		pDesc->TxShort	= rtl8192se_QueryIsShort(((cb_desc->data_rate&0x80)?1:0), MRateToHwRate8192SE(dev,cb_desc->data_rate), cb_desc);

		if(cb_desc->bAMPDUEnable)
		{			
			pDesc->AggEn = 1;
		}
		else
		{			
			pDesc->AggEn = 0;
		}

		{
			pSeq = (u8 *)(skb->data+22);
			Temp = pSeq[0];
			Temp <<= 12;			
			Temp |= (*(u16 *)pSeq)>>4;
			pDesc->Seq = Temp;
		}
		
		pDesc->RTSEn	= (cb_desc->bRTSEnable && cb_desc->bCTSEnable==false)?1:0;				
		pDesc->CTS2Self	= (cb_desc->bCTSEnable)?1:0;		
		pDesc->RTSSTBC	= (cb_desc->bRTSSTBC)?1:0;
		pDesc->RTSHT	= (cb_desc->rts_rate&0x80)?1:0;

#if 1
		if (priv->card_8192_version== VERSION_8192S_ACUT)
		{
			if (cb_desc->rts_rate == MGN_1M || cb_desc->rts_rate == MGN_2M || 
				cb_desc->rts_rate == MGN_5_5M || cb_desc->rts_rate == MGN_11M)
			{
				cb_desc->rts_rate = MGN_12M;
			}
		}
#endif	
		pDesc->RTSRate	= MRateToHwRate8192SE(dev,cb_desc->rts_rate);
		pDesc->RTSRate	= MRateToHwRate8192SE(dev,MGN_24M);
		pDesc->RTSBW	= 0;
		pDesc->RTSSC	= cb_desc->RTSSC;
		pDesc->RTSShort	= (pDesc->RTSHT==0)?(cb_desc->bRTSUseShortPreamble?1:0):(cb_desc->bRTSUseShortGI?1:0);
		if(priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
		{
			if(cb_desc->bPacketBW)
			{
				pDesc->TxBw		= 1;
				pDesc->TXSC		= 0;
			}
			else
			{				
				pDesc->TxBw		= 0;
				pDesc->TXSC		= priv->nCur40MhzPrimeSC;
			}
		}
		else
		{
			pDesc->TxBw		= 0;
			pDesc->TXSC		= 0;
		}

		pDesc->LINIP = 0;
		pDesc->Offset = 32;
		pDesc->PktSize = (u16)skb->len;		
		
		pDesc->RaBRSRID = cb_desc->RATRIndex;
#if 0		
printk("*************TXDESC:\n");
printk("\tTxRate: %d\n", pDesc->TxRate);
printk("\tAMPDUEn: %d\n", pDesc->AggEn);
printk("\tTxBw: %d\n", pDesc->TxBw);
printk("\tTXSC: %d\n", pDesc->TXSC);
printk("\tPktSize: %d\n", pDesc->PktSize);
printk("\tRatrIdx: %d\n", pDesc->RaBRSRID);
#endif
		 if (cb_desc->bHwSec) {
       		 static u8 tmp =0;
       		 if (!tmp) {
            		 tmp = 1;
	        	}
#ifdef _RTL8192_EXT_PATCH_
			if(cb_desc->mesh_pkt == 0)
#else
#ifdef ASL
			if(cb_desc->ap_pkt == 0)
#endif
#endif	
			{
				switch (priv->rtllib->pairwise_key_type) {
					case KEY_TYPE_WEP40:
					case KEY_TYPE_WEP104:
						pDesc->SecType = 0x1;
						break;
					case KEY_TYPE_TKIP:
						pDesc->SecType = 0x2;
						break;
					case KEY_TYPE_CCMP:
						pDesc->SecType = 0x3;
						break;
					case KEY_TYPE_NA:
						pDesc->SecType = 0x0;
						break;
				}
			}
#ifdef _RTL8192_EXT_PATCH_
			else if(cb_desc->mesh_pkt == 1)
			{
				switch (priv->rtllib->mesh_pairwise_key_type) {
					case KEY_TYPE_WEP40:
					case KEY_TYPE_WEP104:
						pDesc->SecType = 0x1;
						break;
					case KEY_TYPE_TKIP:
						pDesc->SecType = 0x2;
						break;
					case KEY_TYPE_CCMP:
						pDesc->SecType = 0x3;
						break;
					case KEY_TYPE_NA:
						pDesc->SecType = 0x0;
						break;
				}
			}
#endif			
#ifdef ASL
			else if(cb_desc->ap_pkt == 1)
			{
				switch (priv->rtllib->ap_pairwise_key_type) {
					case KEY_TYPE_WEP40:
					case KEY_TYPE_WEP104:
						pDesc->SecType = 0x1;
						break;
					case KEY_TYPE_TKIP:
						pDesc->SecType = 0x2;
						break;
					case KEY_TYPE_CCMP:
						pDesc->SecType = 0x3;
						break;
					case KEY_TYPE_NA:
						pDesc->SecType = 0x0;
						break;
				}
			}
#endif	
		}

		pDesc->PktID			= 0x0;		
		pDesc->QueueSel		= rtl8192se_MapHwQueueToFirmwareQueue(cb_desc->queue_index, cb_desc->priority); 
		
		pDesc->DataRateFBLmt	= 0x1F;		
		pDesc->DISFB	= cb_desc->bTxDisableRateFallBack;
		pDesc->UserRate	= cb_desc->bTxUseDriverAssingedRate;

		if (pDesc->UserRate == true && pDesc->TXHT == true)		
			RF_ChangeTxPath(dev, cb_desc->data_rate);

	}
	

	pDesc->FirstSeg	= 1;
	pDesc->LastSeg	= 1;
	
	pDesc->TxBufferSize= (u16)skb->len;	
	
	pDesc->TxBuffAddr = cpu_to_le32(mapping);

}

void  rtl8192se_tx_fill_cmd_desc(struct net_device* dev, tx_desc_cmd * entry, cb_desc * cb_desc, struct sk_buff* skb)
{
    struct r8192_priv *priv = rtllib_priv(dev);

	if(cb_desc->bCmdOrInit == DESC_PACKET_TYPE_INIT) {
		
    dma_addr_t mapping = pci_map_single(priv->pdev, skb->data, skb->len, PCI_DMA_TODEVICE);
		
	memset((void*)entry, 0, 32); 	
	
	entry->LINIP = cb_desc->bLastIniPkt;
		
	entry->FirstSeg = 1;
	entry->LastSeg = 1;
	
	entry->TxBufferSize= (u16)(skb->len);
	entry->TxBufferAddr = cpu_to_le32(mapping);
	entry->PktSize = (u16)(skb->len);
	
	{
		entry->OWN = 1;
	}
	} else { 

		u8*	pDesc = (u8*)entry;
		
    		dma_addr_t mapping = pci_map_single(priv->pdev, skb->data, skb->len, PCI_DMA_TODEVICE);
		
		CLEAR_PCI_TX_DESC_CONTENT(pDesc, sizeof(tx_desc_cmd));

		SET_TX_DESC_FIRST_SEG(pDesc, 1); 
		SET_TX_DESC_LAST_SEG(pDesc, 1); 

		SET_TX_DESC_OFFSET(pDesc, 0x20); 

		SET_TX_DESC_PKT_SIZE(pDesc, (u16)(skb->len)); 
		SET_TX_DESC_QUEUE_SEL(pDesc, 0x13); 

		
		SET_BITS_TO_LE_4BYTE(skb->data, 24, 7, priv->H2CTxCmdSeq);

		SET_TX_DESC_TX_BUFFER_SIZE(pDesc, (u16)(skb->len));
		SET_TX_DESC_TX_BUFFER_ADDRESS(pDesc, cpu_to_le32(mapping));

		SET_TX_DESC_OWN(pDesc, 1);

		RT_TRACE(COMP_CMD, "TxFillCmdDesc8192SE(): H2C Tx Cmd Content ----->\n");
	}
}

u8 HwRateToMRate92S(bool bIsHT,	u8 rate)
{
	u8	ret_rate = 0x02;

	if (!bIsHT) {
		switch (rate) {
		case DESC92S_RATE1M:		
			ret_rate = MGN_1M;		
			break;
		case DESC92S_RATE2M:		
			ret_rate = MGN_2M;		
			break;
		case DESC92S_RATE5_5M:		
			ret_rate = MGN_5_5M;		
			break;
		case DESC92S_RATE11M:		
			ret_rate = MGN_11M;		
			break;
		case DESC92S_RATE6M:		
			ret_rate = MGN_6M;		
			break;
		case DESC92S_RATE9M:		
			ret_rate = MGN_9M;		
			break;
		case DESC92S_RATE12M:		
			ret_rate = MGN_12M;		
			break;
		case DESC92S_RATE18M:		
			ret_rate = MGN_18M;		
			break;
		case DESC92S_RATE24M:		
			ret_rate = MGN_24M;		
			break;
		case DESC92S_RATE36M:		
			ret_rate = MGN_36M;		
			break;
		case DESC92S_RATE48M:		
			ret_rate = MGN_48M;		
			break;
		case DESC92S_RATE54M:		
			ret_rate = MGN_54M;		
			break;
		default:							
			ret_rate = 0xff;
			break;
		}
	} else {
		switch (rate) {
		case DESC92S_RATEMCS0:	
			ret_rate = MGN_MCS0;		
			break;
		case DESC92S_RATEMCS1:	
			ret_rate = MGN_MCS1;		
			break;
		case DESC92S_RATEMCS2:	
			ret_rate = MGN_MCS2;		
			break;
		case DESC92S_RATEMCS3:	
			ret_rate = MGN_MCS3;		
			break;
		case DESC92S_RATEMCS4:	
			ret_rate = MGN_MCS4;		
			break;
		case DESC92S_RATEMCS5:	
			ret_rate = MGN_MCS5;		
			break;
		case DESC92S_RATEMCS6:	
			ret_rate = MGN_MCS6;		
			break;
		case DESC92S_RATEMCS7:	
			ret_rate = MGN_MCS7;		
			break;
		case DESC92S_RATEMCS8:	
			ret_rate = MGN_MCS8;		
			break;
		case DESC92S_RATEMCS9:	
			ret_rate = MGN_MCS9;		
			break;
		case DESC92S_RATEMCS10:	
			ret_rate = MGN_MCS10;	
			break;
		case DESC92S_RATEMCS11:	
			ret_rate = MGN_MCS11;	
			break;
		case DESC92S_RATEMCS12:	
			ret_rate = MGN_MCS12;	
			break;
		case DESC92S_RATEMCS13:	
			ret_rate = MGN_MCS13;	
			break;
		case DESC92S_RATEMCS14:	
			ret_rate = MGN_MCS14;	
			break;
		case DESC92S_RATEMCS15:	
			ret_rate = MGN_MCS15;	
			break;
		case DESC92S_RATEMCS32:	
			ret_rate = (0x80|0x20);	
			break;
		default:							
			ret_rate = 0xff;
			break;
		}

	}	
	return ret_rate;
}

long
rtl8192se_signal_scale_mapping(struct r8192_priv * priv,
	long currsig
	)
{
	long retsig = 0;

	if(currsig> 47)
		retsig = 100; 
	else if(currsig >14 && currsig <=47)
		retsig = 100 - ((47-currsig)*3)/2;
	else if(currsig >2 && currsig <=14)
		retsig = 48-((14-currsig)*15)/7;
	else if(currsig >=0)
		retsig = currsig * 9+1;
	
	return retsig;
}


#define 	rx_hal_is_cck_rate(_pdesc)\
			(_pdesc->RxMCS == DESC92S_RATE1M ||\
			 _pdesc->RxMCS == DESC92S_RATE2M ||\
			 _pdesc->RxMCS == DESC92S_RATE5_5M ||\
			 _pdesc->RxMCS == DESC92S_RATE11M)

void rtl8192se_query_rxphystatus(
	struct r8192_priv * priv,
	struct rtllib_rx_stats * pstats,
	prx_desc  pdesc,	
	prx_fwinfo   pDrvInfo,
	bool bpacket_match_bssid,
	bool bpacket_toself,
	bool bPacketBeacon
	)
{
	bool is_cck_rate;
	phy_sts_cck_8192s_t* cck_buf;
	u8 rx_pwr_all=0, rx_pwr[4], rf_rx_num=0, EVM, PWDB_ALL;
	u8 i, max_spatial_stream;
	u32 rssi, total_rssi = 0;
	u8 cck_highpwr = 0;
	is_cck_rate = rx_hal_is_cck_rate(pdesc);

	pstats->bPacketMatchBSSID = bpacket_match_bssid;
	pstats->bPacketToSelf = bpacket_toself;
	pstats->bIsCCK = is_cck_rate;
	pstats->bPacketBeacon = bPacketBeacon;

	pstats->RxMIMOSignalQuality[0] = -1;
	pstats->RxMIMOSignalQuality[1] = -1;

	if (is_cck_rate){
		u8 report;
			
		cck_buf = (phy_sts_cck_8192s_t*)pDrvInfo;

		if(priv->rtllib->eRFPowerState == eRfOn)
			cck_highpwr = (u8)priv->bCckHighPower;
		else
			cck_highpwr = false;
		if (!cck_highpwr){
			report = cck_buf->cck_agc_rpt & 0xc0;
			report = report >> 6;
			switch(report){
				case 0x3:
					rx_pwr_all = -40 - (cck_buf->cck_agc_rpt&0x3e);
					break;
				case 0x2:
					rx_pwr_all = -20 - (cck_buf->cck_agc_rpt&0x3e);
					break;
				case 0x1:
					rx_pwr_all = -2 - (cck_buf->cck_agc_rpt&0x3e);
					break;
				case 0x0:
					rx_pwr_all = 14 - (cck_buf->cck_agc_rpt&0x3e);
					break;
			}
		}
		else{
			report = pDrvInfo->cfosho[0] & 0x60;
			report = report >> 5;
			switch(report){
				case 0x3:
					rx_pwr_all = -40 - ((cck_buf->cck_agc_rpt & 0x1f)<<1);
					break;
				case 0x2:
					rx_pwr_all = -20 - ((cck_buf->cck_agc_rpt & 0x1f)<<1);
					break;
				case 0x1:
					rx_pwr_all = -2 - ((cck_buf->cck_agc_rpt & 0x1f)<<1);
					break;
				case 0x0:
					rx_pwr_all = 14 - ((cck_buf->cck_agc_rpt & 0x1f)<<1);
					break;
			}
		}

		PWDB_ALL= rtl819x_query_rxpwrpercentage(rx_pwr_all);
		{
			PWDB_ALL+=6;
			if(PWDB_ALL > 100)
				PWDB_ALL = 100;
			if(PWDB_ALL > 34 && PWDB_ALL <= 42)
				PWDB_ALL -= 2;
			else if(PWDB_ALL > 26 && PWDB_ALL <= 34)
				PWDB_ALL -= 6;
			else if(PWDB_ALL > 14 && PWDB_ALL <= 26)
				PWDB_ALL -= 8;
			else if(PWDB_ALL > 4 && PWDB_ALL <= 14)
				PWDB_ALL -= 4;
		}
		pstats->RxPWDBAll = PWDB_ALL;
		pstats->RecvSignalPower = rx_pwr_all;

		if (bpacket_match_bssid){
			u8 sq;
			if(priv->CustomerID == RT_CID_819x_Lenovo)		
			{
				if(PWDB_ALL >= 50)
					sq = 100;
				else if(PWDB_ALL >= 35 && PWDB_ALL < 50)
					sq = 80;
				else if(PWDB_ALL >= 22 && PWDB_ALL < 35)
					sq = 60;
				else if(PWDB_ALL >= 18 && PWDB_ALL < 22)
					sq = 40;
				else
					sq = 20;
			}
			else
			{
			if (pstats->RxPWDBAll > 40)
				sq = 100;
			else{
				sq = cck_buf->sq_rpt;
				if (sq > 64)
					sq = 0;
				else if(sq < 20)
					sq = 100;
				else
					sq = ((64-sq)*100)/44;
			}
			}
			pstats->SignalQuality = sq;
			pstats->RxMIMOSignalQuality[0] = sq;
			pstats->RxMIMOSignalQuality[1] = -1;
		}
	}
	else{
		priv->brfpath_rxenable[0] = priv->brfpath_rxenable[1] = true;

		for (i=RF90_PATH_A; i<RF90_PATH_MAX; i++){
			if (priv->brfpath_rxenable[i])
				rf_rx_num ++;

			rx_pwr[i] = ((pDrvInfo->gain_trsw[i]&0x3f)*2) - 110;
			rssi = rtl819x_query_rxpwrpercentage(rx_pwr[i]);
			total_rssi += rssi;

			priv->stats.rxSNRdB[i] = (long)(pDrvInfo->rxsnr[i]/2);

			if (bpacket_match_bssid){
				pstats->RxMIMOSignalStrength[i] = (u8)rssi;
				if(priv->CustomerID == RT_CID_819x_Lenovo)
				{
					u8	SQ;
					
					if(i == 0)
					{
						if(rssi >= 50)
							SQ = 100;
						else if(rssi >= 35 && rssi < 50)
							SQ = 80;
						else if(rssi >= 22 && rssi < 35)
							SQ = 60;
						else if(rssi >= 18 && rssi < 22)
							SQ = 40;
						else
							SQ = 20;
						pstats->SignalQuality = SQ;
			}
		}
			}
		}
		rx_pwr_all = ((pDrvInfo->pwdb_all >> 1) & 0x7f) - 106;
		PWDB_ALL = rtl819x_query_rxpwrpercentage(rx_pwr_all);

		pstats->RxPWDBAll = PWDB_ALL;
		pstats->RxPower = rx_pwr_all;
		pstats->RecvSignalPower = rx_pwr_all;

		if(priv->CustomerID != RT_CID_819x_Lenovo){	
		if (pdesc->RxHT && pdesc->RxMCS >= DESC92S_RATEMCS8 && pdesc->RxMCS <= DESC92S_RATEMCS15)
			max_spatial_stream = 2;
		else
			max_spatial_stream = 1;

		for(i=0; i<max_spatial_stream; i++){
			EVM = rtl819x_evm_dbtopercentage(pDrvInfo->rxevm[i]);

			if (bpacket_match_bssid)
			{
				if (i==0)
						pstats->SignalQuality = (u8)(EVM & 0xff);
					pstats->RxMIMOSignalQuality[i] = (u8)(EVM&0xff);
				}
			}
		}
#if 1
		if (pdesc->BandWidth)
			priv->stats.received_bwtype[1+pDrvInfo->rxsc]++;
		else
			priv->stats.received_bwtype[0]++;
#endif
	}

	if (is_cck_rate){
		pstats->SignalStrength = (u8)(rtl8192se_signal_scale_mapping(priv,PWDB_ALL));
	}
	else {
		if (rf_rx_num != 0)
			pstats->SignalStrength = (u8)(rtl8192se_signal_scale_mapping(priv,total_rssi/=rf_rx_num));
	}
}

void rtl819x_update_rxsignalstatistics8192S(
	struct r8192_priv * priv,
	struct rtllib_rx_stats * pstats
	)
{
	int	weighting = 0;


	if(priv->stats.recv_signal_power == 0)
		priv->stats.recv_signal_power = pstats->RecvSignalPower;

	if(pstats->RecvSignalPower > priv->stats.recv_signal_power)
		weighting = 5;
	else if(pstats->RecvSignalPower < priv->stats.recv_signal_power)
		weighting = (-5);
	priv->stats.recv_signal_power = (priv->stats.recv_signal_power * 5 + pstats->RecvSignalPower + weighting) / 6;

}	

void Process_UI_RSSI_8192S(struct r8192_priv * priv,struct rtllib_rx_stats * pstats)
{
	u8			rfPath;
	u32			last_rssi, tmpVal;

	if(pstats->bPacketToSelf || pstats->bPacketBeacon)
	{
		priv->stats.RssiCalculateCnt++;	
		if(priv->stats.ui_rssi.TotalNum++ >= PHY_RSSI_SLID_WIN_MAX)
		{
			priv->stats.ui_rssi.TotalNum = PHY_RSSI_SLID_WIN_MAX;
			last_rssi = priv->stats.ui_rssi.elements[priv->stats.ui_rssi.index];
			priv->stats.ui_rssi.TotalVal -= last_rssi;
		}
		priv->stats.ui_rssi.TotalVal += pstats->SignalStrength;
	
		priv->stats.ui_rssi.elements[priv->stats.ui_rssi.index++] = pstats->SignalStrength;
		if(priv->stats.ui_rssi.index >= PHY_RSSI_SLID_WIN_MAX)
			priv->stats.ui_rssi.index = 0;
	
		tmpVal = priv->stats.ui_rssi.TotalVal/priv->stats.ui_rssi.TotalNum;
		priv->stats.signal_strength = rtl819x_translate_todbm(priv, (u8)tmpVal);
		pstats->rssi = priv->stats.signal_strength;

	}

	if(!pstats->bIsCCK && pstats->bPacketToSelf)
	{
		for (rfPath = RF90_PATH_A; rfPath < priv->NumTotalRFPath; rfPath++)
		{
			if (!rtl8192_phy_CheckIsLegalRFPath(priv->rtllib->dev, rfPath))
				continue;
			RT_TRACE(COMP_DBG, "pstats->RxMIMOSignalStrength[%d]  = %d \n", rfPath, pstats->RxMIMOSignalStrength[rfPath] );

			if(priv->stats.rx_rssi_percentage[rfPath] == 0)	
			{
				priv->stats.rx_rssi_percentage[rfPath] = pstats->RxMIMOSignalStrength[rfPath];
			}
			
			if(pstats->RxMIMOSignalStrength[rfPath]  > priv->stats.rx_rssi_percentage[rfPath])
			{
				priv->stats.rx_rssi_percentage[rfPath] = 
					( (priv->stats.rx_rssi_percentage[rfPath]*(Rx_Smooth_Factor-1)) + 
					(pstats->RxMIMOSignalStrength[rfPath])) /(Rx_Smooth_Factor);
				priv->stats.rx_rssi_percentage[rfPath] = priv->stats.rx_rssi_percentage[rfPath] + 1;
			}
			else
			{
				priv->stats.rx_rssi_percentage[rfPath] = 
					( (priv->stats.rx_rssi_percentage[rfPath]*(Rx_Smooth_Factor-1)) + 
					(pstats->RxMIMOSignalStrength[rfPath])) /(Rx_Smooth_Factor);
			}
			RT_TRACE(COMP_DBG, "priv->stats.rx_rssi_percentage[%d]  = %d \n",rfPath, priv->stats.rx_rssi_percentage[rfPath] );
		}
	}
	
}	

void Process_PWDB_8192S(struct r8192_priv * priv,struct rtllib_rx_stats * pstats,struct rtllib_network* pnet, struct sta_info *pEntry)
{
	long	UndecoratedSmoothedPWDB=0;
#ifdef _RTL8192_EXT_PATCH_
	if(priv->rtllib->iw_mode == IW_MODE_MESH){
		if(pnet){
			if(priv->mshobj->ext_patch_get_peermp_rssi_param)
				UndecoratedSmoothedPWDB = priv->mshobj->ext_patch_get_peermp_rssi_param(pnet);
		}else
			UndecoratedSmoothedPWDB = priv->undecorated_smoothed_pwdb;

	} else 
#endif
	if((priv->rtllib->iw_mode == IW_MODE_ADHOC) || (priv->rtllib->iw_mode == IW_MODE_MASTER)){
		if(pEntry){
			UndecoratedSmoothedPWDB = pEntry->rssi_stat.UndecoratedSmoothedPWDB;
		}
	}
#ifdef ASL
	else if (priv->rtllib->iw_mode == IW_MODE_APSTA) {
		if(pEntry){
			UndecoratedSmoothedPWDB = pEntry->rssi_stat.UndecoratedSmoothedPWDB;
		}
		else
			UndecoratedSmoothedPWDB = priv->undecorated_smoothed_pwdb;
	}
#endif
	else
		UndecoratedSmoothedPWDB = priv->undecorated_smoothed_pwdb;

	if(pstats->bPacketToSelf || pstats->bPacketBeacon){
		if(UndecoratedSmoothedPWDB < 0){	
			UndecoratedSmoothedPWDB = pstats->RxPWDBAll;
		}
		
		if(pstats->RxPWDBAll > (u32)UndecoratedSmoothedPWDB){
			UndecoratedSmoothedPWDB = 	
					( ((UndecoratedSmoothedPWDB)*(Rx_Smooth_Factor-1)) + 
					(pstats->RxPWDBAll)) /(Rx_Smooth_Factor);
			UndecoratedSmoothedPWDB = UndecoratedSmoothedPWDB + 1;
		}else{
			UndecoratedSmoothedPWDB = 	
					( ((UndecoratedSmoothedPWDB)*(Rx_Smooth_Factor-1)) + 
					(pstats->RxPWDBAll)) /(Rx_Smooth_Factor);
		}

#ifdef _RTL8192_EXT_PATCH_
		if(priv->rtllib->iw_mode == IW_MODE_MESH){
			if(pnet){
				if(priv->mshobj->ext_patch_set_peermp_rssi_param)
					priv->mshobj->ext_patch_set_peermp_rssi_param(pnet,UndecoratedSmoothedPWDB);
			}else
				priv->undecorated_smoothed_pwdb = UndecoratedSmoothedPWDB;

		} else 
#endif
		if((priv->rtllib->iw_mode == IW_MODE_ADHOC) || (priv->rtllib->iw_mode == IW_MODE_MASTER)){
			if(pEntry){
				pEntry->rssi_stat.UndecoratedSmoothedPWDB = UndecoratedSmoothedPWDB;
			}
		}
#ifdef ASL
		else if (priv->rtllib->iw_mode == IW_MODE_APSTA) {
			if(pEntry){
				pEntry->rssi_stat.UndecoratedSmoothedPWDB = UndecoratedSmoothedPWDB;
			} else
				priv->undecorated_smoothed_pwdb = UndecoratedSmoothedPWDB;
		}
#endif
		else{
			priv->undecorated_smoothed_pwdb = UndecoratedSmoothedPWDB;
		}
		rtl819x_update_rxsignalstatistics8192S(priv, pstats);
	}
}

void Process_UiLinkQuality8192S(struct r8192_priv * priv,struct rtllib_rx_stats * pstats)
{
	u32	last_evm=0, nSpatialStream, tmpVal;

	if(pstats->SignalQuality != 0)	
	{
		if (pstats->bPacketToSelf || pstats->bPacketBeacon)
		{
			if(priv->stats.ui_link_quality.TotalNum++ >= PHY_LINKQUALITY_SLID_WIN_MAX)
			{
				priv->stats.ui_link_quality.TotalNum = PHY_LINKQUALITY_SLID_WIN_MAX;
				last_evm = priv->stats.ui_link_quality.elements[priv->stats.ui_link_quality.index];
				priv->stats.ui_link_quality.TotalVal -= last_evm;
			}
			priv->stats.ui_link_quality.TotalVal += pstats->SignalQuality;

			priv->stats.ui_link_quality.elements[priv->stats.ui_link_quality.index++] = pstats->SignalQuality;
			if(priv->stats.ui_link_quality.index >= PHY_LINKQUALITY_SLID_WIN_MAX)
				priv->stats.ui_link_quality.index = 0;


			tmpVal = priv->stats.ui_link_quality.TotalVal/priv->stats.ui_link_quality.TotalNum;
			priv->stats.signal_quality = tmpVal;
			priv->stats.last_signal_strength_inpercent = tmpVal;
		
			for(nSpatialStream = 0; nSpatialStream<2 ; nSpatialStream++)	
			{
				if(pstats->RxMIMOSignalQuality[nSpatialStream] != -1)
				{
					if(priv->stats.rx_evm_percentage[nSpatialStream] == 0)	
					{
						priv->stats.rx_evm_percentage[nSpatialStream] = pstats->RxMIMOSignalQuality[nSpatialStream];
					}
					priv->stats.rx_evm_percentage[nSpatialStream] = 
					( (priv->stats.rx_evm_percentage[nSpatialStream]*(Rx_Smooth_Factor-1)) + 
					(pstats->RxMIMOSignalQuality[nSpatialStream]* 1)) /(Rx_Smooth_Factor);
				}
			}
		}
	}
	else
		;
	
}	


void rtl8192se_process_phyinfo(struct r8192_priv * priv, u8* buffer,struct rtllib_rx_stats * pcurrent_stats,struct rtllib_network * pnet, struct sta_info *pEntry)
{
	if(!pcurrent_stats->bPacketMatchBSSID)
		return;
	
	Process_UI_RSSI_8192S(priv, pcurrent_stats);
	
	Process_PWDB_8192S(priv, pcurrent_stats, pnet, pEntry);

	Process_UiLinkQuality8192S(priv, pcurrent_stats);	
}



void rtl8192se_TranslateRxSignalStuff(struct net_device *dev, 
        struct sk_buff *skb,
        struct rtllib_rx_stats * pstats,
        prx_desc pdesc,	
        prx_fwinfo pdrvinfo)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	bool bpacket_match_bssid, bpacket_toself;
	bool bPacketBeacon=false;
	struct rtllib_hdr_3addr *hdr;

#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
	u8	*psaddr;
	struct rtllib_network* pnet=NULL;
	struct sta_info *pEntry = NULL;
#endif

	u16 fc,type;
	u8* tmp_buf;
	u8	*praddr;

	tmp_buf = skb->data + pstats->RxDrvInfoSize + pstats->RxBufShift;

	hdr = (struct rtllib_hdr_3addr *)tmp_buf;
	fc = le16_to_cpu(hdr->frame_ctl);
	type = WLAN_FC_GET_TYPE(fc);	
	praddr = hdr->addr1;
#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
	psaddr = hdr->addr2;
#endif

#ifdef _RTL8192_EXT_PATCH_
	if((priv->rtllib->iw_mode == IW_MODE_MESH) && (priv->mshobj->ext_patch_get_mpinfo))
		pnet = priv->mshobj->ext_patch_get_mpinfo(dev,psaddr);	
	if(priv->rtllib->iw_mode == IW_MODE_ADHOC){
		pEntry = GetStaInfo(priv->rtllib, psaddr);
	}
#endif

#ifdef ASL
	if(priv->rtllib->iw_mode == IW_MODE_MASTER || priv->rtllib->iw_mode == IW_MODE_APSTA) {
		pEntry = ap_get_stainfo(priv->rtllib, psaddr);
	}
#endif

#ifdef _RTL8192_EXT_PATCH_
	bpacket_match_bssid = ((RTLLIB_FTYPE_CTL != type) && (!pstats->bHwError) && (!pstats->bCRC)&& (!pstats->bICV));
	if(pnet){
		bpacket_match_bssid = bpacket_match_bssid;
	}
	else{
		bpacket_match_bssid = bpacket_match_bssid &&
			(!compare_ether_addr(priv->rtllib->current_network.bssid, 
					     (fc & RTLLIB_FCTL_TODS)? hdr->addr1 : 
					     (fc & RTLLIB_FCTL_FROMDS )? hdr->addr2 : hdr->addr3));
	}
#else
#ifdef ASL
	bpacket_match_bssid = ((RTLLIB_FTYPE_CTL != type) &&
			(!compare_ether_addr(priv->rtllib->current_network.bssid,	
					     (fc & RTLLIB_FCTL_TODS)? hdr->addr1 : 
					     (fc & RTLLIB_FCTL_FROMDS )? hdr->addr2 : hdr->addr3))
			&& (!pstats->bHwError) && (!pstats->bCRC)&& (!pstats->bICV)) | 
			((RTLLIB_FTYPE_CTL != type) &&
			(!compare_ether_addr(priv->rtllib->current_ap_network.bssid,	
					     (fc & RTLLIB_FCTL_TODS)? hdr->addr1 : 
					     (fc & RTLLIB_FCTL_FROMDS )? hdr->addr2 : hdr->addr3))
			&& (!pstats->bHwError) && (!pstats->bCRC)&& (!pstats->bICV));
#else
	bpacket_match_bssid = ((RTLLIB_FTYPE_CTL != type) &&
			(!compare_ether_addr(priv->rtllib->current_network.bssid,	
					     (fc & RTLLIB_FCTL_TODS)? hdr->addr1 : 
					     (fc & RTLLIB_FCTL_FROMDS )? hdr->addr2 : hdr->addr3))
			&& (!pstats->bHwError) && (!pstats->bCRC)&& (!pstats->bICV));
#endif
#endif

	bpacket_toself =  bpacket_match_bssid & (!compare_ether_addr(praddr, priv->rtllib->dev->dev_addr));
	if(WLAN_FC_GET_FRAMETYPE(fc)== RTLLIB_STYPE_BEACON){
		bPacketBeacon = true;
	}

	if(bpacket_match_bssid){
		priv->stats.numpacket_matchbssid++;
	}
	
	if(bpacket_toself){
		priv->stats.numpacket_toself++;
	}
	

	rtl8192se_query_rxphystatus(priv, pstats, pdesc, pdrvinfo, bpacket_match_bssid,
			bpacket_toself ,bPacketBeacon);
	
#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
	rtl8192se_process_phyinfo(priv, tmp_buf,pstats,pnet, pEntry);
#else
	rtl8192se_process_phyinfo(priv, tmp_buf,pstats,NULL, NULL);
#endif
}

void rtl8192se_UpdateReceivedRateHistogramStatistics(
	struct net_device *dev,
	struct rtllib_rx_stats* pstats
	)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	u32 rcvType=1;   
	u32 rateIndex;
	u32 preamble_guardinterval;  
	    
	#if 0
	if (pRfd->queue_id == CMPK_RX_QUEUE_ID)
		return;
	#endif
	if(pstats->bCRC)
		rcvType = 2;
	else if(pstats->bICV)
		rcvType = 3;
	    
	if(pstats->bShortPreamble)
		preamble_guardinterval = 1;
	else
		preamble_guardinterval = 0;

	switch(pstats->rate)
	{
		case MGN_1M:    rateIndex = 0;  break;
	    	case MGN_2M:    rateIndex = 1;  break;
	    	case MGN_5_5M:  rateIndex = 2;  break;
	    	case MGN_11M:   rateIndex = 3;  break;
	    	case MGN_6M:    rateIndex = 4;  break;
	    	case MGN_9M:    rateIndex = 5;  break;
	    	case MGN_12M:   rateIndex = 6;  break;
	    	case MGN_18M:   rateIndex = 7;  break;
	    	case MGN_24M:   rateIndex = 8;  break;
	    	case MGN_36M:   rateIndex = 9;  break;
	    	case MGN_48M:   rateIndex = 10; break;
	    	case MGN_54M:   rateIndex = 11; break;
	    	case MGN_MCS0:  rateIndex = 12; break;
	    	case MGN_MCS1:  rateIndex = 13; break;
	    	case MGN_MCS2:  rateIndex = 14; break;
	    	case MGN_MCS3:  rateIndex = 15; break;
	    	case MGN_MCS4:  rateIndex = 16; break;
	    	case MGN_MCS5:  rateIndex = 17; break;
	    	case MGN_MCS6:  rateIndex = 18; break;
	    	case MGN_MCS7:  rateIndex = 19; break;
	    	case MGN_MCS8:  rateIndex = 20; break;
	    	case MGN_MCS9:  rateIndex = 21; break;
	    	case MGN_MCS10: rateIndex = 22; break;
	    	case MGN_MCS11: rateIndex = 23; break;
	    	case MGN_MCS12: rateIndex = 24; break;
	    	case MGN_MCS13: rateIndex = 25; break;
	    	case MGN_MCS14: rateIndex = 26; break;
	    	case MGN_MCS15: rateIndex = 27; break;
		default:        rateIndex = 28; break;
	}
	priv->stats.received_preamble_GI[preamble_guardinterval][rateIndex]++;
	priv->stats.received_rate_histogram[0][rateIndex]++; 
	priv->stats.received_rate_histogram[rcvType][rateIndex]++;
}

bool rtl8192se_rx_query_status_desc(struct net_device* dev, struct rtllib_rx_stats*  stats, rx_desc *pdesc, struct sk_buff* skb)
{
	struct r8192_priv* priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;
	struct rtllib_hdr_3addr * hdr = NULL;
	u8* tmp_buf = NULL;
	u16	fc = 0;
	u8	type = 0;
	u32	PHYStatus	= pdesc->PHYStatus;
	rx_fwinfo*		pDrvInfo;
	stats->Length 		= (u16)pdesc->Length;	
	stats->RxDrvInfoSize = (u8)pdesc->DrvInfoSize*8;
	stats->RxBufShift 	= (u8)((pdesc->Shift)&0x03);      
	stats->bICV 			= (u16)pdesc->ICVError;
	stats->bCRC 			= (u16)pdesc->CRC32;
	stats->bHwError 		= (u16)(pdesc->CRC32|pdesc->ICVError);	
	stats->Decrypted 	= !pdesc->SWDec;
	stats->rate = (u8)pdesc->RxMCS;
	stats->bShortPreamble= (u16)pdesc->SPLCP;
	stats->bIsAMPDU 		= (bool)(pdesc->PAGGR==1);
	stats->bFirstMPDU 	= (bool)((pdesc->PAGGR==1) && (pdesc->FAGGR==1));
	stats->TimeStampLow 	= pdesc->TSFL;
	stats->RxIs40MHzPacket= (bool)pdesc->BandWidth;
	if(IS_UNDER_11N_AES_MODE(ieee))
	{
		if(stats->bICV && !stats->bCRC)
		{
			stats->bICV = false;
			stats->bHwError = false;
		}
	}
	tmp_buf = skb->data + stats->RxDrvInfoSize + stats->RxBufShift;
	hdr = (struct rtllib_hdr_3addr *)tmp_buf;
	fc = le16_to_cpu(hdr->frame_ctl);
	type = WLAN_FC_GET_TYPE(fc);
	
	if (type == RTLLIB_FTYPE_DATA) {
		if(stats->Length > 0x2000 || stats->Length < 24)
		{
			stats->bHwError |= 1;
		}
	}
	rtl8192se_UpdateReceivedRateHistogramStatistics(dev, stats);

	if(!stats->bHwError)
		stats->rate = HwRateToMRate92S((bool)(pdesc->RxHT), (u8)(pdesc->RxMCS));
	else
	{
		stats->rate = MGN_1M;
		return false;
	}

	rtl819x_UpdateRxPktTimeStamp(dev, stats);	

	if((stats->RxBufShift + stats->RxDrvInfoSize) > 0)
		stats->bShift = 1;	

	if (PHYStatus == true)
	{
		pDrvInfo = (rx_fwinfo*)(skb->data + stats->RxBufShift);
			
		rtl8192se_TranslateRxSignalStuff(dev, skb, stats, pdesc, pDrvInfo); 

	}
	return true;	
}

void rtl8192se_halt_adapter(struct net_device *dev, bool bReset)
{
	struct r8192_priv *priv = rtllib_priv(dev);
        int i;
	u8	wait = 30;	

	RT_TRACE(COMP_INIT, "==> rtl8192se_halt_adapter()\n");
	
#if 1
	while (wait-- >= 10 && priv->PwrDomainProtect == true)
	{
		if (priv->PwrDomainProtect == true)
		{
			RT_TRACE(COMP_INIT, "Delay 20ms to wait PwrDomainProtect\n");
			mdelay(20);
		}
		else
			break;
	}

	if (wait == 9)
		RT_TRACE(COMP_INIT, "PwrDomainProtect FAIL\n");
#endif

	priv->rtllib->state = RTLLIB_NOLINK;
	rtl8192se_update_msr(dev);
#if 1
	PHY_SetRtl8192seRfHalt(dev);
#endif
	udelay(100);

#if 0
	udelay(20);
	if (!bReset) {
		mdelay(20);
	}	
#endif	
        for(i = 0; i < MAX_QUEUE_SIZE; i++) {
                skb_queue_purge(&priv->rtllib->skb_waitQ [i]);
        }
        for(i = 0; i < MAX_QUEUE_SIZE; i++) {
                skb_queue_purge(&priv->rtllib->skb_aggQ [i]);
        }
#ifdef _RTL8192_EXT_PATCH_	
		for(i = 0; i < MAX_QUEUE_SIZE; i++) {
                skb_queue_purge(&priv->rtllib->skb_meshaggQ [i]);
        }
#endif
#ifdef ASL
	for(i = 0; i < MAX_QUEUE_SIZE; i++) {
		skb_queue_purge(&priv->rtllib->skb_apaggQ [i]);
	}
#endif
	skb_queue_purge(&priv->skb_queue);
	RT_TRACE(COMP_INIT, "<== HaltAdapter8192SE()\n");
	return;
}

u8 GetFreeRATRIndex8192SE (struct net_device* dev)
{
	struct r8192_priv* priv = rtllib_priv(dev);
	u8 bitmap = priv->RATRTableBitmap;
	u8 ratr_index = 0;
	for(	;ratr_index<7; ratr_index++)
	{
		if((bitmap & BIT0) == 0)
			{
				priv->RATRTableBitmap |= BIT0<<ratr_index;
				return ratr_index;
			}
		bitmap = bitmap >>1;
	}
	return ratr_index;
}

void rtl8192se_update_ratr_table(struct net_device* dev,u8* pMcsRate,struct sta_info* pEntry)
{
	struct r8192_priv* priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;
	u32 ratr_value = 0;
	u8 ratr_index = 0;
	u8 bNMode = 0;
	u16 shortGI_rate = 0;
	u32 tmp_ratr_value = 0;
	u8 MimoPs;
	WIRELESS_MODE WirelessMode;
	u8 bCurTxBW40MHz, bCurShortGI40MHz, bCurShortGI20MHz;
	PRT_HIGH_THROUGHPUT	pHTInfo = NULL;
#ifdef ASL
	bool is_ap_role = false;
#endif
	if((ieee->iw_mode == IW_MODE_ADHOC)
#ifdef ASL
		|| (ieee->iw_mode == IW_MODE_MASTER) 
		|| ((ieee->iw_mode == IW_MODE_APSTA) && pEntry)
#endif
	) {	
		if(pEntry == NULL){
			printk("Doesn't have match Entry\n");
			return;
		}	

		if(pEntry->ratr_index != 8)
			ratr_index = pEntry->ratr_index;
		else
		        ratr_index = GetFreeRATRIndex8192SE(dev);
		
		if(ratr_index == 7){
			RT_TRACE(COMP_RATE, "Ratrtable are full");
			return;
		}
		MimoPs = pEntry->htinfo.MimoPs;

#ifdef ASL
		if ((ieee->iw_mode == IW_MODE_MASTER) || (ieee->iw_mode == IW_MODE_APSTA)) {
			is_ap_role = true;
			if((ieee->apmode == WIRELESS_MODE_G) && (pEntry->wireless_mode == WIRELESS_MODE_N_24G))
				WirelessMode = ieee->apmode;
			else	
				WirelessMode = pEntry->wireless_mode;
		} else {
#endif
		if((ieee->mode == WIRELESS_MODE_G) && (pEntry->wireless_mode == WIRELESS_MODE_N_24G))
			WirelessMode = ieee->mode;
		else	
			WirelessMode = pEntry->wireless_mode;
#ifdef ASL
		}
#endif
		bCurTxBW40MHz = pEntry->htinfo.bCurTxBW40MHz;
		bCurShortGI40MHz = pEntry->htinfo.bCurShortGI40MHz;
		bCurShortGI20MHz = pEntry->htinfo.bCurShortGI20MHz;		
		pEntry->ratr_index = ratr_index;
	}
	else  
	{
		ratr_index = 0;
		WirelessMode = ieee->mode;
		MimoPs = ieee->pHTInfo->PeerMimoPs;
		bCurTxBW40MHz = ieee->pHTInfo->bCurTxBW40MHz;
		bCurShortGI40MHz = ieee->pHTInfo->bCurShortGI40MHz;
		bCurShortGI20MHz = ieee->pHTInfo->bCurShortGI20MHz;					
	}
#ifdef ASL
	rtl8192_config_rate(dev, (u16*)(&ratr_value), is_ap_role);
#else
	rtl8192_config_rate(dev, (u16*)(&ratr_value));
#endif
#ifdef ASL
	if (is_ap_role) 
		pHTInfo = ieee->pAPHTInfo;
	else 
#endif
	pHTInfo = ieee->pHTInfo;
	ratr_value |= (*(u16*)(pMcsRate)) << 12;
	switch (WirelessMode)
	{
		case IEEE_A:
			ratr_value &= 0x00000FF0;
			break;
		case IEEE_B:
			ratr_value &= 0x0000000D;
			break;
		case IEEE_G:
		case IEEE_G|IEEE_B:
			ratr_value &= 0x00000FF5;
			break;
		case IEEE_N_24G:
		case IEEE_N_5G:
			bNMode = 1;
			if (MimoPs == 0) 
				ratr_value &= 0x0007F005;
			else{
				if (priv->rf_type == RF_1T2R ||priv->rf_type  == RF_1T1R || (pHTInfo->IOTAction & HT_IOT_ACT_DISABLE_TX_2SS)){
					if ((bCurTxBW40MHz) && !(pHTInfo->IOTAction & HT_IOT_ACT_DISABLE_TX_40_MHZ))
						ratr_value &= 0x000FF015;
					else
						ratr_value &= 0x000ff005;
				}else{
					if ((bCurTxBW40MHz) && !(pHTInfo->IOTAction & HT_IOT_ACT_DISABLE_TX_40_MHZ))
						ratr_value &= 0x0f0ff015;
					else
						ratr_value &= 0x0f0ff005;
					}
			}
			break;
		default:
			printk("====>%s(), mode is not correct:%x\n", __FUNCTION__,WirelessMode);
			break; 
	}
	if (priv->card_8192_version>= VERSION_8192S_BCUT)
		ratr_value &= 0x0FFFFFFF;
	else if (priv->card_8192_version == VERSION_8192S_ACUT)
		ratr_value &= 0x0FFFFFF0;	
	
	if (((pHTInfo->IOTAction & HT_IOT_ACT_DISABLE_SHORT_GI)==0) &&
		bNMode && ((bCurTxBW40MHz && bCurShortGI40MHz) ||
	    (!bCurTxBW40MHz && bCurShortGI20MHz)))
	{
		ratr_value |= 0x10000000;  
		tmp_ratr_value = (ratr_value>>12);
		for(shortGI_rate=15; shortGI_rate>0; shortGI_rate--)
		{
			if((1<<shortGI_rate) & tmp_ratr_value)
				break;
		}	
		shortGI_rate = (shortGI_rate<<12)|(shortGI_rate<<8)|\
			       (shortGI_rate<<4)|(shortGI_rate);
		write_nic_byte(dev, SG_RATE, shortGI_rate);
	}

	if(pHTInfo->IOTAction & HT_IOT_ACT_WA_IOT_Broadcom)
	{
		ratr_value &= 0xfffffff0; 
		printk("UpdateHalRATRTable8192SE(), for HT_IOT_ACT_WA_IOT_Broadcom, ratr_value = 0x%x\n", ratr_value);
	}
#ifdef ASL
	if (is_ap_role) {
		priv->rtllib->swapratrtable[ratr_index].bused = true;
		priv->rtllib->swapratrtable[ratr_index].ratr_value = ratr_value;
	}
#endif
	write_nic_dword(dev, ARFR0+ratr_index*4, ratr_value);
	RT_TRACE(COMP_RATE, "%s: ratr_index=%d ratr_table=0x%8.8x\n", __FUNCTION__,ratr_index, read_nic_dword(dev, ARFR0+ratr_index*4));
	printk("%s: ratr_index=%d ratr_table=0x%8.8x\n", __FUNCTION__,ratr_index, ratr_value);
	if (ratr_value & 0xfffff000){
		rtl8192se_set_fw_cmd(dev, FW_CMD_RA_REFRESH_N);
	}
	else{
		rtl8192se_set_fw_cmd(dev, FW_CMD_RA_REFRESH_BG);
	}
}

int r8192se_resume_firm(struct net_device *dev)
{
	write_nic_byte(dev, 0x42, 0xFF);
	write_nic_word(dev, 0x40, 0x77FC);
	write_nic_word(dev, 0x40, 0x57FC);
	write_nic_word(dev, 0x40, 0x37FC);
	write_nic_word(dev, 0x40, 0x77FC);
	
	udelay(100);

	write_nic_word(dev, 0x40, 0x57FC);
	write_nic_word(dev, 0x40, 0x37FC);
	write_nic_byte(dev, 0x42, 0x00);

	return 0;
}
void PHY_SetRtl8192seRfHalt(struct net_device* dev)
{
	struct r8192_priv* priv = rtllib_priv(dev);
	PRT_POWER_SAVE_CONTROL	pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));
	u8 u1bTmp;

	if(priv->rtllib->RfOffReason == RF_CHANGE_BY_IPS && priv->LedStrategy == SW_LED_MODE8)	
	{
		SET_RTL8192SE_RF_SLEEP(dev);
		return;
	}

#ifdef LOW_DOWN_POWER
	if(0)
#else
	if(priv->bDriverIsGoingToUnload)
#endif
		write_nic_byte(dev,0x560,0x0);

	RT_TRACE(COMP_PS, "PHY_SetRtl8192seRfHalt save BB/RF\n");
	u1bTmp = read_nic_byte(dev, LDOV12D_CTRL);
	u1bTmp |= BIT0;
	write_nic_byte(dev, LDOV12D_CTRL, u1bTmp);
	write_nic_byte(dev, SPS1_CTRL, 0x0);
	write_nic_byte(dev, TXPAUSE, 0xFF);
	write_nic_word(dev, CMDR, 0x57FC);
	udelay(100);
	write_nic_word(dev, CMDR, 0x77FC);
	write_nic_byte(dev, PHY_CCA, 0x0);
	udelay(10);
	write_nic_word(dev, CMDR, 0x37FC);
	udelay(10);
	write_nic_word(dev, CMDR, 0x77FC);			
	udelay(10);
	write_nic_word(dev, CMDR, 0x57FC);
	write_nic_word(dev, CMDR, 0x0000);


#ifdef LOW_DOWN_POWER
	if(0)
#else
	if(priv->bDriverIsGoingToUnload)
#endif
        {
	       u1bTmp = read_nic_byte(dev, (SYS_FUNC_EN+ 1));	
		u1bTmp &= ~(BIT0);
		write_nic_byte(dev, SYS_FUNC_EN+1, u1bTmp);
        }

	
	u1bTmp = read_nic_byte(dev, (SYS_CLKR + 1));
	
	if (u1bTmp & BIT7) {
		u1bTmp &= ~(BIT6 | BIT7);					
		if(!HalSetSysClk8192SE(dev, u1bTmp)) {
			printk("Switch ctrl path fail\n");
			return;
		}
	}

#ifdef LOW_DOWN_POWER
        if((priv->rtllib->RfOffReason & (RF_CHANGE_BY_IPS | RF_CHANGE_BY_HW)) &&
                !priv->bDriverIsGoingToUnload)
#else
	if(priv->rtllib->RfOffReason==RF_CHANGE_BY_IPS  && !priv->bDriverIsGoingToUnload)
#endif
	{
		write_nic_byte(dev, 0x03, 0xF9);
	} else		
	{
		write_nic_byte(dev, 0x03, 0xF9);
	}
	write_nic_byte(dev, SYS_CLKR+1, 0x70);
	write_nic_byte(dev, AFE_PLL_CTRL+1, 0x68);
	write_nic_byte(dev,  AFE_PLL_CTRL, 0x00);	
	write_nic_byte(dev, LDOA15_CTRL, 0x34);
	write_nic_byte(dev, AFE_XTAL_CTRL, 0x0E);
	RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);
}	


/*
*	Description:
*		Set RateAdaptive Mask
*	/param 	Adapter		Pionter to Adapter entity
*	/param	bMulticast	TURE if broadcast or multicast, used for softAP basic rate
*	/param	macId		macID to set
*	/param 	wirelessMode	wireless mode of associated AP/client
*	/return	void
*	
*/
void UpdateHalRAMask8192SE(
			struct net_device* dev,
			bool bMulticast,
			u8   macId,
			u8   MimoPs,
			u8   WirelessMode,
			u8   bCurTxBW40MHz,
			u8   rssi_level,
			u8*  entry_McsRateSet,
			u8* entry_ratrindex,
			u32* entry_ratr_bitmap,
			u32* entry_mask,
			bool is_ap){
	struct r8192_priv* 	priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;
	PRT_HIGH_THROUGHPUT pHTInfo = NULL;
	u8* 	pMcsRate = NULL;
	u32	ratr_bitmap, RateSet, mask=0, band = 0, ratr_value = 0;
	u8   shortGI_rate = 0, bShortGI = false; 
	u8 	ratr_index = 8;
	RT_TRACE(COMP_RATE, "%s: macid:%d MimoPs=%d WirelessMode=0x%x bCurTxBW40MHz=%d rssid_level=%d\n",__FUNCTION__, macId, MimoPs, WirelessMode,bCurTxBW40MHz, rssi_level);
	
	if (is_ap) {
#ifdef ASL
		pHTInfo = priv->rtllib->pAPHTInfo;
		pMcsRate = entry_McsRateSet;
#endif
	} else {
		pHTInfo = priv->rtllib->pHTInfo;
		pMcsRate = ieee->dot11HTOperationalRateSet;
	}
#ifdef ASL
	rtl8192_config_rate(dev, (u16*)&RateSet, is_ap);
#else
	rtl8192_config_rate(dev, (u16*)&RateSet);
#endif
	RateSet |= (*(u16*)(pMcsRate)) << 12;
	ratr_bitmap = RateSet;
	switch (WirelessMode){
		case WIRELESS_MODE_B:
			band |= WIRELESS_11B;
			ratr_index = RATR_INX_WIRELESS_B;
			if (ratr_bitmap & 0x0000000c)
			ratr_bitmap &= 0x0000000d;
			else
				ratr_bitmap &= 0x0000000f;
			break;
		case WIRELESS_MODE_G:
		case (WIRELESS_MODE_G |WIRELESS_MODE_B):
			band |= (WIRELESS_11G | WIRELESS_11B);
			ratr_index = RATR_INX_WIRELESS_GB;
			if(rssi_level == 1)
				ratr_bitmap &= 0x00000f00;
			else if(rssi_level == 2)
				ratr_bitmap &= 0x00000ff0;
			else
				ratr_bitmap &= 0x00000ff5;
			break;
		case WIRELESS_MODE_A:
			band |= WIRELESS_11A;
			ratr_index = RATR_INX_WIRELESS_A;
			ratr_bitmap &= 0x00000ff0;
			break;
		case WIRELESS_MODE_N_24G:
		case WIRELESS_MODE_N_5G:
		{
			band |= (WIRELESS_11N| WIRELESS_11G| WIRELESS_11B);
			ratr_index = RATR_INX_WIRELESS_NGB;
			if(MimoPs == MIMO_PS_STATIC){
				if(rssi_level == 1)
					ratr_bitmap &= 0x00070000;
				else if(rssi_level == 2)
					ratr_bitmap &= 0x0007f000;
				else
					ratr_bitmap &= 0x0007f005;
			}else{
				if (priv->rf_type == RF_1T2R || priv->rf_type == RF_1T1R)
				{

						if(rssi_level == 1)
							ratr_bitmap &= 0x000f0000;
					else if(rssi_level ==3)
						ratr_bitmap &= 0x000fc000;
					else if(rssi_level == 5)
							ratr_bitmap &= 0x000ff000;
					else  
					{
						if (bCurTxBW40MHz)
							ratr_bitmap &= 0x000ff015;
						else
							ratr_bitmap &= 0x000ff005;
					}	

				}
						else
				{
						if(rssi_level == 1)
						ratr_bitmap &= 0x0f8f0000;
					else if(rssi_level == 3)
						ratr_bitmap &= 0x0f8fc000;					
					else if(rssi_level == 5)
						ratr_bitmap &= 0x0f8ff000;		
					else
					{						
						if (bCurTxBW40MHz)						
							ratr_bitmap &= 0x0f8ff015;
						else
							ratr_bitmap &= 0x0f8ff005;
					}
					
				}
			}
			if( (pHTInfo->bCurTxBW40MHz && pHTInfo->bCurShortGI40MHz) ||
				(!pHTInfo->bCurTxBW40MHz && pHTInfo->bCurShortGI20MHz)){
				if(macId == 0)
					bShortGI = true;
				else
					bShortGI = false;
			}
			break;
		}
		default:
			band |= (WIRELESS_11N| WIRELESS_11G| WIRELESS_11B);
			ratr_index = RATR_INX_WIRELESS_NGB;
			if(priv->rf_type == RF_1T2R)
				ratr_bitmap &= 0x000ff0ff;
			else
				ratr_bitmap &= 0x0f8ff0ff;
			break;
	}

	if (priv->card_8192_version  >= VERSION_8192S_BCUT)
		ratr_bitmap &= 0x0FFFFFFF;
	else if (priv->card_8192_version  == VERSION_8192S_ACUT)
		ratr_bitmap &= 0x0FFFFFF0;	

	if(bShortGI){
		ratr_bitmap |= 0x10000000;
		ratr_value = (ratr_bitmap>>12);
		for(shortGI_rate=15; shortGI_rate>0; shortGI_rate--){
			if((1<<shortGI_rate) & ratr_value)
				break;
		}	
		shortGI_rate = (shortGI_rate<<12)|(shortGI_rate<<8)|(shortGI_rate<<4)|(shortGI_rate);
		write_nic_byte(dev, SG_RATE, shortGI_rate);
	}	
	
	if(macId == 0)
	{
		if(priv->rtllib->pHTInfo->IOTAction & HT_IOT_ACT_WA_IOT_Broadcom)
			ratr_bitmap &= 0xfffffff0; 
		if(priv->rtllib->pHTInfo->IOTAction & HT_IOT_ACT_DISABLE_SHORT_GI)
			ratr_bitmap &=0x0fffffff;
	}		
	mask |= (bMulticast ? 1 : 0)<<9 | (macId & 0x1f)<<4 | (band & 0xf);

	if (macId != 0) {
		if(entry_ratrindex != NULL)
			*entry_ratrindex = ratr_index;
		if(entry_ratr_bitmap != NULL)
			*entry_ratr_bitmap = ratr_bitmap;
		if(entry_mask != NULL)
			*entry_mask = mask;
	}
	RT_TRACE(COMP_RATE, "%s(): mask = %x, bitmap = %x\n",__func__, mask, ratr_bitmap);
	write_nic_dword(dev, 0x2c4, ratr_bitmap);
	write_nic_dword(dev, WFM5, (FW_RA_UPDATE_MASK | (mask << 8)));
}

u8 HalSetSysClk8192SE( struct net_device *dev, u8 Data)
{
#if 0
	write_nic_byte(dev, (SYS_CLKR + 1), Data);
	udelay(200);;
	return 1;
#else
	{
		u8				WaitCount = 100;
		bool bResult = false;

#ifdef TO_DO_LIST
		RT_DISABLE_FUNC(Adapter, DF_IO_BIT);

		do
		{
			if(pDevice->IOCount == 0)
				break;
			delay_us(10);
		}while(WaitCount -- > 0);

		if(WaitCount == 0)
		{ 
			RT_ENABLE_FUNC(Adapter, DF_IO_BIT);
			RT_TRACE(COMP_POWER, DBG_WARNING, ("HalSetSysClk8192SE(): Wait too long! Skip ....\n"));
			return false;
		}
		#endif
		write_nic_byte(dev,SYS_CLKR + 1,Data);

		udelay(400);


		{
			u8 TmpValue;
			TmpValue=read_nic_byte(dev,SYS_CLKR + 1);
			bResult = ((TmpValue&BIT7)== (Data & BIT7));
			if((Data &(BIT6|BIT7)) == false)
			{			
				WaitCount = 100;
				TmpValue = 0;
				while(1) 
				{
					WaitCount--;
					TmpValue=read_nic_byte(dev, SYS_CLKR + 1); 
					if((TmpValue &BIT6))
						break;
					printk("wait for BIT6 return value %x\n",TmpValue);	
					if(WaitCount==0)
						break;
					udelay(10);
				}
				if(WaitCount == 0)
					bResult = false;
				else
					bResult = true;
			}
		}
#ifdef TO_DO_LIST
		RT_ENABLE_FUNC(Adapter, DF_IO_BIT);
#endif
		RT_TRACE(COMP_PS,"HalSetSysClk8192SE():Value = %02X, return: %d\n", Data, bResult);
		return bResult;
	}
#endif
}

static u8 LegacyRateSet[12] = {0x02 , 0x04 , 0x0b , 0x16 , 0x0c , 0x12 , 0x18 , 0x24 , 0x30 , 0x48 , 0x60 , 0x6c};
void GetHwReg8192SE(struct net_device *dev,u8 variable,u8* val)
{
	struct r8192_priv* priv = rtllib_priv(dev);
	switch(variable)
	{
		case HW_VAR_INIT_TX_RATE: 
			{
				u8 RateIdx = read_nic_byte(dev, TX_RATE_REG);
				if(RateIdx < 76)
					*((u8*)(val)) = (RateIdx<12)?(LegacyRateSet[RateIdx]):((RateIdx-12)|0x80);
				else
					*((u8*)(val)) = 0;
			}
			break;

		case HW_VAR_RCR:
			*((u32*)(val)) = priv->ReceiveConfig;
		break;
		
		case HW_VAR_MRC:
		{
			*((bool*)(val)) = priv->bCurrentMrcSwitch;
		}
		break;
		
		default:
			break;
	}
}

void SetHwReg8192SE(struct net_device *dev,u8 variable,u8* val)
{
	struct r8192_priv* priv = rtllib_priv(dev);
	switch(variable)
	{
		case HW_VAR_AMPDU_MIN_SPACE:
		{
			u8	MinSpacingToSet;
			u8	SecMinSpace;

			MinSpacingToSet = *((u8*)val);
			if(MinSpacingToSet <= 7)
			{
				if((priv->rtllib->current_network.capability & WLAN_CAPABILITY_PRIVACY) == 0)  
					SecMinSpace = 0;
				else if (priv->rtllib->rtllib_ap_sec_type && 
						(priv->rtllib->rtllib_ap_sec_type(priv->rtllib) 
							 & (SEC_ALG_WEP|SEC_ALG_TKIP))) 
					SecMinSpace = 7;
				else
					SecMinSpace = 1;

				if(MinSpacingToSet < SecMinSpace)
					MinSpacingToSet = SecMinSpace;
				priv->rtllib->MinSpaceCfg = ((priv->rtllib->MinSpaceCfg&0xf8) |MinSpacingToSet);
				RT_TRACE(COMP_MLME, "Set HW_VAR_AMPDU_MIN_SPACE: %#x\n", priv->rtllib->MinSpaceCfg);
				write_nic_byte(dev, AMPDU_MIN_SPACE, priv->rtllib->MinSpaceCfg);	
			}
		}
		break;	
		case HW_VAR_SHORTGI_DENSITY:
		{
			u8	DensityToSet;
		
			DensityToSet = *((u8*)val);		
			priv->rtllib->MinSpaceCfg|= (DensityToSet<<3);		
			RT_TRACE(COMP_MLME, "Set HW_VAR_SHORTGI_DENSITY: %#x\n", priv->rtllib->MinSpaceCfg);
			write_nic_byte(dev, AMPDU_MIN_SPACE, priv->rtllib->MinSpaceCfg);
			break;		
		}
		case HW_VAR_AMPDU_FACTOR:
		{
			u8	FactorToSet;
			u8	RegToSet;
			u8	FactorLevel[18] = {2, 4, 4, 7, 7, 13, 13, 13, 2, 7, 7, 13, 13, 15, 15, 15, 15, 0};
			u8	index = 0;
		
			FactorToSet = *((u8*)val);
			if(FactorToSet <= 3)
			{
				FactorToSet = (1<<(FactorToSet + 2));
				if(FactorToSet>0xf)
					FactorToSet = 0xf;

				for(index=0; index<17; index++)
				{
					if(FactorLevel[index] > FactorToSet)
						FactorLevel[index] = FactorToSet;
				}

				for(index=0; index<8; index++)
				{
					RegToSet = ((FactorLevel[index*2]) | (FactorLevel[index*2+1]<<4));
					write_nic_byte(dev, AGGLEN_LMT_L+index, RegToSet);
				}
				RegToSet = ((FactorLevel[16]) | (FactorLevel[17]<<4));
				write_nic_byte(dev, AGGLEN_LMT_H, RegToSet);

				RT_TRACE(COMP_MLME, "Set HW_VAR_AMPDU_FACTOR: %#x\n", FactorToSet);
			}
		}
		break;
                case HW_VAR_AC_PARAM:
                {
                        u8      pAcParam = *((u8*)val);
#ifdef MERGE_TO_DO
                        u32     eACI = GET_WMM_AC_PARAM_ACI(pAcParam);
#else
                        u32     eACI = pAcParam;
#endif
                        u8              u1bAIFS;
                        u32             u4bAcParam;
                        u8 mode = priv->rtllib->mode;
                        struct rtllib_qos_parameters *qos_parameters = &priv->rtllib->current_network.qos_data.parameters;
#ifdef ASL
			if (priv->rtllib->iw_mode == IW_MODE_MASTER || priv->rtllib->iw_mode == IW_MODE_APSTA)
				qos_parameters = &priv->rtllib->current_ap_network.qos_data.parameters;
#endif
#ifdef ASL
			if (priv->rtllib->iw_mode == IW_MODE_MASTER || priv->rtllib->iw_mode == IW_MODE_APSTA)
				u1bAIFS = qos_parameters->aifs[pAcParam] * ((mode&(IEEE_G|IEEE_N_24G)) ?9:20) + aAPSifsTime;
			else	
#endif
                        u1bAIFS = qos_parameters->aifs[pAcParam] * ((mode&(IEEE_G|IEEE_N_24G)) ?9:20) + aSifsTime;

                        dm_init_edca_turbo(dev);

                        u4bAcParam = (  (((u32)(qos_parameters->tx_op_limit[pAcParam])) << AC_PARAM_TXOP_LIMIT_OFFSET)  |
                                                        (((u32)(qos_parameters->cw_max[pAcParam])) << AC_PARAM_ECW_MAX_OFFSET)  |
                                                        (((u32)(qos_parameters->cw_min[pAcParam])) << AC_PARAM_ECW_MIN_OFFSET)  |
                                                        (((u32)u1bAIFS) << AC_PARAM_AIFS_OFFSET)        );

			printk("%s():HW_VAR_AC_PARAM eACI:%x:%x\n", __func__,eACI, u4bAcParam);
                        switch(eACI)
                        {
                        case AC1_BK:
                                write_nic_dword(dev, EDCAPARA_BK, u4bAcParam);
                                break;

                        case AC0_BE:
                                write_nic_dword(dev, EDCAPARA_BE, u4bAcParam);
                                break;

                        case AC2_VI:
                                write_nic_dword(dev, EDCAPARA_VI, u4bAcParam);
                                break;

                        case AC3_VO:
                                write_nic_dword(dev, EDCAPARA_VO, u4bAcParam);
                                break;

                        default:
                                printk("SetHwReg8185(): invalid ACI: %d !\n", eACI);
                                break;
                        }
                        if(priv->AcmMethod != eAcmWay2_SW)
                                priv->rtllib->SetHwRegHandler(dev, HW_VAR_ACM_CTRL, (u8*)(&pAcParam));
                }
                break;
        	case HW_VAR_ACM_CTRL:
                {
                        struct rtllib_qos_parameters *qos_parameters = &priv->rtllib->current_network.qos_data.parameters;
                        u8      pAcParam = *((u8*)val);
#ifdef MERGE_TO_DO
                        u32     eACI = GET_WMM_AC_PARAM_ACI(pAciAifsn);
#else
                        u32     eACI = pAcParam;
#endif
                        PACI_AIFSN      pAciAifsn = (PACI_AIFSN)&(qos_parameters->aifs[0]);
                        u8              ACM = pAciAifsn->f.ACM;
                        u8              AcmCtrl = read_nic_byte( dev, AcmHwCtrl);
#ifdef ASL
			    if (priv->rtllib->iw_mode == IW_MODE_MASTER || priv->rtllib->iw_mode == IW_MODE_APSTA) {
					qos_parameters = &priv->rtllib->current_ap_network.qos_data.parameters;
				       pAciAifsn = (PACI_AIFSN)&(qos_parameters->aifs[0]);
			    		ACM = pAciAifsn->f.ACM;
			    }
#endif

                        printk("===========>%s():HW_VAR_ACM_CTRL:%x\n", __func__,eACI);
                        AcmCtrl = AcmCtrl | ((priv->AcmMethod == 2)?0x0:0x1);

                        if( ACM )
                        { 
                                switch(eACI)
                                {
                                case AC0_BE:
                                        AcmCtrl |= AcmHw_BeqEn;
                                        break;

                                case AC2_VI:
                                        AcmCtrl |= AcmHw_ViqEn;
                                        break;
                                case AC3_VO:
                                        AcmCtrl |= AcmHw_VoqEn;
                                        break;

                                default:
                                        RT_TRACE( COMP_QOS, "SetHwReg8185(): [HW_VAR_ACM_CTRL] ACM set failed: eACI is %d\n", eACI );
                                        break;
                                }
                        }
                        else
                        { 
                                switch(eACI)
                                {
                                case AC0_BE:
                                        AcmCtrl &= (~AcmHw_BeqEn);
                                        break;

                                case AC2_VI:
                                        AcmCtrl &= (~AcmHw_ViqEn);
                                        break;

                                case AC3_VO:
                                        AcmCtrl &= (~AcmHw_BeqEn);
                                        break;

                                default:
                                        break;
                                }
                        }

                        RT_TRACE( COMP_QOS, "SetHwReg8190pci(): [HW_VAR_ACM_CTRL] Write 0x%X\n", AcmCtrl );
                        write_nic_byte(dev, AcmHwCtrl, AcmCtrl );
                }
                break;

		case HW_VAR_BASIC_RATE:
		{
			u16				BrateCfg = 0;
			u8				RateIndex = 0;

			
#ifdef ASL
			rtl8192_config_rate(dev, &BrateCfg, false);
#else
			rtl8192_config_rate(dev, &BrateCfg);
#endif

			if (priv->card_8192_version == VERSION_8192S_ACUT)
				priv->basic_rate = BrateCfg = BrateCfg & 0x150;
			else if (priv->card_8192_version == VERSION_8192S_BCUT)
				priv->basic_rate = BrateCfg = BrateCfg & 0x15f;
		
  	                if(priv->rtllib->pHTInfo->IOTPeer == HT_IOT_PEER_CISCO && ((BrateCfg &0x150)==0))
			{
				BrateCfg |=0x010;
			}
			if(priv->rtllib->pHTInfo->IOTAction & HT_IOT_ACT_WA_IOT_Broadcom)
			{	
				BrateCfg &= 0x1f0;
				printk("HW_VAR_BASIC_RATE, HT_IOT_ACT_WA_IOT_Broadcom, BrateCfg = 0x%x\n", BrateCfg);
			}

			BrateCfg |= 0x01; 
			
			write_nic_byte(dev, RRSR, BrateCfg&0xff);
			write_nic_byte(dev, RRSR+1, (BrateCfg>>8)&0xff);

			while(BrateCfg > 0x1)
			{
				BrateCfg = (BrateCfg >> 1);
				RateIndex++;
			}
			write_nic_byte(dev, INIRTSMCS_SEL, RateIndex);
		}
		break;
		case HW_VAR_RETRY_LIMIT:
		{
			u8 RetryLimit = ((u8*)(val))[0];
			
			priv->ShortRetryLimit = RetryLimit;
			priv->LongRetryLimit = RetryLimit;
			
			write_nic_word(dev, RETRY_LIMIT, 
							RetryLimit << RETRY_LIMIT_SHORT_SHIFT | \
							RetryLimit << RETRY_LIMIT_LONG_SHIFT);
		}					
		break;
		case HW_VAR_BEACON_INTERVAL:
		{
			write_nic_word(dev, BCN_INTERVAL, *((u16*)val));
			PHY_SetBeaconHwReg(dev, *((u16*)val));
		}
		break;
		
		case HW_VAR_BSSID:
			write_nic_dword(dev, BSSIDR, ((u32*)(val))[0]);
			write_nic_word(dev, BSSIDR+4, ((u16*)(val+4))[0]);
		break;

		case HW_VAR_MEDIA_STATUS:
		{
			RT_OP_MODE	OpMode = *((RT_OP_MODE *)(val));
			LED_CTL_MODE	LedAction = LED_CTL_NO_LINK;
			u8		btMsr = read_nic_byte(dev, MSR);

			btMsr &= 0xfc;
			switch(OpMode)
			{
			case RT_OP_MODE_INFRASTRUCTURE:
				btMsr |= MSR_INFRA;
				LedAction = LED_CTL_LINK;
				break;

			case RT_OP_MODE_IBSS:
				btMsr |= MSR_ADHOC;
				break;

			case RT_OP_MODE_AP:
				btMsr |= MSR_AP;
				LedAction = LED_CTL_LINK;
				break;

			default:
				btMsr |= MSR_NOLINK;
				break;
			}

			write_nic_byte(dev, MSR, btMsr);

			{
				u32	temp = read_nic_dword(dev, TCR);
				write_nic_dword(dev, TCR, temp&(~BIT8));
				write_nic_dword(dev, TCR, temp|BIT8);
			}
			priv->rtllib->LedControlHandler(dev, LedAction);
		}
		break;
		
		case HW_VAR_RCR:
		{
			write_nic_dword(dev, RCR,((u32*)(val))[0]);
			priv->ReceiveConfig = ((u32*)(val))[0];
		}
		break;
		
		case HW_VAR_CECHK_BSSID:
		{
			u32	RegRCR, Type;

			Type = ((u8*)(val))[0];
			priv->rtllib->GetHwRegHandler(dev, HW_VAR_RCR, (u8*)(&RegRCR));
			
#if 0
			RegRCR &= (~RCR_CBSSID);
#else
#if 1
			if (Type == true)
				RegRCR |= (RCR_CBSSID);
			else if (Type == false)
				RegRCR &= (~RCR_CBSSID);
#endif
			priv->rtllib->SetHwRegHandler( dev, HW_VAR_RCR, (u8*)(&RegRCR) );
#endif			
		}
		break;
		
		case HW_VAR_SLOT_TIME:
		{

			priv->slot_time = val[0];		
			write_nic_byte(dev, SLOT_TIME, val[0]);

#ifdef MERGE_TO_DO
			if(priv->rtllib->current_network.qos_data.supported !=0)
			{
				for(eACI = 0; eACI < AC_MAX; eACI++)
				{
					priv->rtllib->SetHwRegHandler(dev, HW_VAR_AC_PARAM, (u8*)(&eACI));
				}
			}
			else
			{
				u8	u1bAIFS = aSifsTime + (2 * priv->slot_time);
				
				write_nic_byte(dev, EDCAPARA_VO, u1bAIFS);
				write_nic_byte(dev, EDCAPARA_VI, u1bAIFS);
				write_nic_byte(dev, EDCAPARA_BE, u1bAIFS);
				write_nic_byte(dev, EDCAPARA_BK, u1bAIFS);
			}
#endif
		}
		break;

		case HW_VAR_ACK_PREAMBLE:	
		{
			u8	regTmp;
			priv->short_preamble = (bool)(*(u8*)val );
			regTmp = (priv->nCur40MhzPrimeSC)<<5;		
			if(priv->short_preamble)
				regTmp |= 0x80;

			write_nic_byte(dev, RRSR+2, regTmp);
		}
		break;	

	case HW_VAR_SIFS:		
		write_nic_byte(dev, SIFS_OFDM, val[0]);
		write_nic_byte(dev, SIFS_OFDM+1, val[1]);
		break;
		
	case HW_VAR_MRC:
		{
			
			bool bMrcToSet = *( (bool*)val );
			u8	U1bData = 0;
		
			if( bMrcToSet )
			{
				printk("HW_VAR_MRC: Turn on 1T1R MRC!\n");	

				rtl8192_setBBreg(dev, rOFDM0_TRxPathEnable, bMaskByte0, 0x33);					
				U1bData = (u8)rtl8192_QueryBBReg(dev, rOFDM1_TRxPathEnable, bMaskByte0);
				rtl8192_setBBreg(dev, rOFDM1_TRxPathEnable, bMaskByte0, ((U1bData&0xf0)|0x03));
				U1bData = (u8)rtl8192_QueryBBReg(dev, rOFDM0_TRxPathEnable, bMaskByte1);			
				rtl8192_setBBreg(dev, rOFDM0_TRxPathEnable, bMaskByte1, (U1bData|0x04));

				priv->bCurrentMrcSwitch = bMrcToSet; 
			}
			else
			{
				printk("HW_VAR_MRC: Turn off 1T1R MRC!\n");	
				
				rtl8192_setBBreg(dev, rOFDM0_TRxPathEnable, bMaskByte0, 0x13);
				U1bData = (u8)rtl8192_QueryBBReg(dev, rOFDM1_TRxPathEnable, bMaskByte0);
				rtl8192_setBBreg(dev, rOFDM1_TRxPathEnable, bMaskByte0, ((U1bData&0xf0)|0x01));
				U1bData = (u8)rtl8192_QueryBBReg(dev, rOFDM0_TRxPathEnable, bMaskByte1);
				rtl8192_setBBreg(dev, rOFDM0_TRxPathEnable, bMaskByte1, (U1bData&0xfb));

				priv->bCurrentMrcSwitch = bMrcToSet; 
			}
		}		
		break;
		
		default:
			break;
	}
}

void SetBeaconRelatedRegisters8192SE(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	struct rtllib_network *net = &priv->rtllib->current_network;
	u16			BcnTimeCfg = 0;
	u16			BcnCW = 6, BcnIFS = 0xf;
	u16			AtimWindow = 2;	
	int			OpMode = priv->rtllib->iw_mode;
	u16			BcnInterval = net->beacon_interval;
	write_nic_word(dev, ATIMWND, AtimWindow);
	
	write_nic_word(dev, BCN_INTERVAL, BcnInterval);

	write_nic_word(dev, BCN_DRV_EARLY_INT, 10<<4);

	write_nic_word(dev, BCN_DMATIME, 256); 

	write_nic_byte(dev, BCN_ERR_THRESH, 100); 

		
	switch(OpMode)
	{
		case IW_MODE_ADHOC:
			BcnTimeCfg |= (BcnCW<<BCN_TCFG_CW_SHIFT);
			break;
		default:
			printk("Invalid Operation Mode!!\n");
			break;
	}

	BcnTimeCfg |= BcnIFS<<BCN_TCFG_IFS;


	PHY_SetBeaconHwReg( dev, BcnInterval );
}

void UpdateHalRATRTableIndex(struct net_device *dev)
{
	struct r8192_priv* priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;
	u8		bitmap = 0;
	int		i;
	if (ieee->iw_mode == IW_MODE_ADHOC) {
	for (i = 0; i < PEER_MAX_ASSOC; i++) {
		if (NULL != ieee->peer_assoc_list[i]) {
			bitmap |= BIT0 << ieee->peer_assoc_list[i]->ratr_index;
		}
	}
	} else {
#ifdef ASL
		for (i = 0; i < APDEV_MAX_ASSOC; i++) {
			if (NULL != ieee->apdev_assoc_list[i]) {
				bitmap |= BIT0 << ieee->apdev_assoc_list[i]->ratr_index;
			}
		}
#endif
	}
	priv->RATRTableBitmap = bitmap;
	return;
}

bool rtl8192se_check_ht_cap(struct net_device* dev, struct sta_info *sta, 
		struct rtllib_network* net)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;
	PHT_CAPABILITY_ELE  pHTCapIE = NULL;
	PHT_INFORMATION_ELE  pPeerHTInfo = NULL;
	u8 ExtChlOffset=0;
	u8	*pMcsFilter = NULL;
	u16	nMaxAMSDUSize = 0;	
	static u8	EWC11NHTCap[] = {0x00, 0x90, 0x4c, 0x33};	
	static u8 	EWC11NHTInfo[] = {0x00, 0x90, 0x4c, 0x34};

	if ((ieee->mode != WIRELESS_MODE_N_24G) && 
			(ieee->mode != WIRELESS_MODE_N_5G)) {
		if (net->mode == IEEE_N_5G)
			sta->wireless_mode = IEEE_A;
		else if (net->mode == IEEE_N_24G) {
			if (net->rates_ex_len > 0)
				sta->wireless_mode = IEEE_G;
			else
				sta->wireless_mode = IEEE_B;
		} else
			sta->wireless_mode = net->mode;
		printk("%s():i am G mode ,do not need to check Cap IE. wireless_mode=0x%x\n",
				__FUNCTION__, sta->wireless_mode);
		return false;
	}
	if ((ieee->mode ==WIRELESS_MODE_N_24G) 
		&& ieee->pHTInfo->bRegSuppCCK== false) {
		if(net->mode == IEEE_B){
			sta->wireless_mode = net->mode;
			printk("%s(): peer is B MODE return\n", __FUNCTION__);
			return false;
		}
	}
	if(net->bssht.bdHTCapLen  != 0)
	{
		sta->htinfo.bEnableHT = true;
		sta->htinfo.bCurRxReorderEnable = ieee->pHTInfo->bRegRxReorderEnable;
		if(net->mode == IEEE_A)
			sta->wireless_mode = IEEE_N_5G;
		else
			sta->wireless_mode = IEEE_N_24G;
	} else {
		printk("%s(): have no HTCap IE, mode is %d\n",__FUNCTION__,net->mode);
		sta->wireless_mode = net->mode;
		sta->htinfo.bEnableHT = false;
		return true;
	}

	if (!memcmp(net->bssht.bdHTCapBuf ,EWC11NHTCap, sizeof(EWC11NHTCap)))
		pHTCapIE = (PHT_CAPABILITY_ELE)(&(net->bssht.bdHTCapBuf[4]));
	else
		pHTCapIE = (PHT_CAPABILITY_ELE)(net->bssht.bdHTCapBuf);

	if (!memcmp(net->bssht.bdHTInfoBuf, EWC11NHTInfo, sizeof(EWC11NHTInfo)))
		pPeerHTInfo = (PHT_INFORMATION_ELE)(&net->bssht.bdHTInfoBuf[4]);
	else		
		pPeerHTInfo = (PHT_INFORMATION_ELE)(net->bssht.bdHTInfoBuf);
	
	ExtChlOffset=((ieee->pHTInfo->bRegBW40MHz == false)?HT_EXTCHNL_OFFSET_NO_EXT:
					(ieee->current_network.channel<=6)?
					HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER);
	printk("******** STA wireless mode %d\n", sta->wireless_mode);
		
	if (ieee->pHTInfo->bRegSuppCCK)
		sta->htinfo.bSupportCck = (pHTCapIE->DssCCk==1)?true:false;
	else {
		if(pHTCapIE->DssCCk==1)
			return false;
	}

	sta->htinfo.MimoPs= pHTCapIE->MimoPwrSave;

	printk("******** PEER MP MimoPs %d\n", sta->htinfo.MimoPs);
	if(ieee->pHTInfo->bRegBW40MHz)
		sta->htinfo.bBw40MHz= (pHTCapIE->ChlWidth==1)?true:false;
	else
		sta->htinfo.bBw40MHz = false;

	if((pPeerHTInfo->ExtChlOffset) != ExtChlOffset)
		sta->htinfo.bBw40MHz = false;
	
	ieee->Peer_bCurBW40M = sta->htinfo.bBw40MHz;
	printk("******** PEER MP bCurBW40M %d\n", sta->htinfo.bBw40MHz);
	if(ieee->pHTInfo->bRegBW40MHz == true)
		sta->htinfo.bCurTxBW40MHz = sta->htinfo.bBw40MHz;

	printk("******** PEER MP bCurTxBW40MHz %d\n", sta->htinfo.bCurTxBW40MHz);
	sta->htinfo.bCurShortGI20MHz= 
		((ieee->pHTInfo->bRegShortGI20MHz)?((pHTCapIE->ShortGI20Mhz==1)?true:false):false);
	sta->htinfo.bCurShortGI40MHz= 
		((ieee->pHTInfo->bRegShortGI40MHz)?((pHTCapIE->ShortGI40Mhz==1)?true:false):false);
	
	printk("******** PEER MP bCurShortGI20MHz %d, bCurShortGI40MHz %d\n",sta->htinfo.bCurShortGI20MHz,sta->htinfo.bCurShortGI40MHz);
	nMaxAMSDUSize = (pHTCapIE->MaxAMSDUSize==0)?3839:7935;
	if(ieee->pHTInfo->nAMSDU_MaxSize >= nMaxAMSDUSize)	
		sta->htinfo.AMSDU_MaxSize = nMaxAMSDUSize;
	else
		sta->htinfo.AMSDU_MaxSize = ieee->pHTInfo->nAMSDU_MaxSize;

	printk("****************AMSDU_MaxSize=%d\n",sta->htinfo.AMSDU_MaxSize);
		
	if(ieee->pHTInfo->AMPDU_Factor >= pHTCapIE->MaxRxAMPDUFactor)
		sta->htinfo.AMPDU_Factor = pHTCapIE->MaxRxAMPDUFactor;
	else
		sta->htinfo.AMPDU_Factor = ieee->pHTInfo->AMPDU_Factor;

	if(ieee->pHTInfo->MPDU_Density >= pHTCapIE->MPDUDensity)
		sta->htinfo.MPDU_Density = pHTCapIE->MPDUDensity;
	else
		sta->htinfo.MPDU_Density = ieee->pHTInfo->MPDU_Density;

	HTFilterMCSRate(ieee, pHTCapIE->MCS, sta->htinfo.McsRateSet);
	if(sta->htinfo.MimoPs == 0)  
		pMcsFilter = MCS_FILTER_1SS;
	else
		pMcsFilter = MCS_FILTER_ALL;

	sta->htinfo.HTHighestOperaRate = HTGetHighestMCSRate(ieee, sta->htinfo.McsRateSet, pMcsFilter);
	printk("******** PEER MP HTHighestOperaRate %x\n",sta->htinfo.HTHighestOperaRate);

	return true;
	
}

#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,20)
void rtl8192se_check_tsf_wq(struct work_struct * work)
{
	struct delayed_work *dwork = container_of(work,struct delayed_work,work);
	struct rtllib_device *ieee = container_of(dwork, struct rtllib_device, check_tsf_wq);
	struct net_device *dev = ieee->dev;
#else
void rtl8192se_check_tsf_wq(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;
#endif
	u32	CurrTsfHigh,CurrTsfLow;
	u32	TargetTsfHigh,TargetTsfLow;
					
	CurrTsfHigh = read_nic_dword(dev, TSFR+4);
	CurrTsfLow = (u32)(ieee->CurrTsf & 0xffff);
	TargetTsfHigh = (u32)(ieee->TargetTsf >> 32);
	TargetTsfLow = (u32)(ieee->TargetTsf & 0xffff);

	printk("Current TSF Low = %x, Hight = %x\n",CurrTsfLow,CurrTsfHigh);
	printk("Target TSF Low = %x, Hight = %x\n",TargetTsfLow,TargetTsfHigh);

	ieee->CurrTsf |= (u64)CurrTsfHigh << 32;

	if(ieee->CurrTsf < ieee->TargetTsf)
	{
		down(&ieee->wx_sem);

		rtllib_stop_protocol(ieee,true);

		ieee->ssid_set = 1;

		rtllib_start_protocol(ieee);

		up(&ieee->wx_sem);
	}
}

#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,20)
void rtl8192se_update_assoc_sta_info_wq(struct work_struct * work)
{
	struct delayed_work *dwork = container_of(work,struct delayed_work,work);
	struct rtllib_device *ieee = container_of(dwork, struct rtllib_device, update_assoc_sta_info_wq);
        struct net_device *dev = ieee->dev;
#else
void rtl8192se_update_assoc_sta_info_wq(struct net_device *dev)
{
        struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;
#endif
	struct sta_info * pEntry = NULL;
	int idx = 0;
	u8 max_index = 0;
	bool is_ap = false;
	if (ieee->iw_mode == IW_MODE_ADHOC) {
		max_index = PEER_MAX_ASSOC;
		is_ap = false;
	}
#ifdef ASL
	else
	{
		max_index = APDEV_MAX_ASSOC;
		is_ap = true;
	}
#endif
	for (idx=0; idx<max_index; idx++) {	

		if (ieee->iw_mode == IW_MODE_ADHOC)
			pEntry = ieee->peer_assoc_list[idx];
#ifdef ASL
		else
			pEntry = ieee->apdev_assoc_list[idx];
#endif

		if(NULL != pEntry)
		{
			u8 * addr = pEntry->macaddr;
				
			if(ieee->bUseRAMask){
				if((pEntry->wireless_mode & WIRELESS_MODE_N_24G) || (pEntry->wireless_mode & WIRELESS_MODE_N_5G))
					pEntry->ratr_index = RATR_INX_WIRELESS_NGB;
				else if(pEntry->wireless_mode & WIRELESS_MODE_G)
					pEntry->ratr_index = RATR_INX_WIRELESS_GB;
				else if(pEntry->wireless_mode & WIRELESS_MODE_B)
					pEntry->ratr_index = RATR_INX_WIRELESS_B;
				ieee->UpdateHalRAMaskHandler(dev,
										false,
										pEntry->aid+1,
										pEntry->htinfo.MimoPs,
										pEntry->wireless_mode,	
										pEntry->htinfo.bCurTxBW40MHz,
										0,
						pEntry->htinfo.McsRateSet,
						&pEntry->ratr_index,
						&(pEntry->swratrmask.ratr_bitmap),
						&(pEntry->swratrmask.mask),
						is_ap);
				pEntry->swratrmask.bused = true;
			}
			else	
				rtl8192se_update_ratr_table(dev,pEntry->htinfo.McsRateSet,pEntry);
			printk("%s: STA:"MAC_FMT", aid:%d, wireless_mode=0x%x ratr_index=%d\n",__FUNCTION__,MAC_ARG(addr), pEntry->aid, pEntry->wireless_mode, pEntry->ratr_index);
		}
	}
	if(!ieee->bUseRAMask)
		UpdateHalRATRTableIndex(dev);

	if (ieee->iw_mode == IW_MODE_ADHOC) {
	if(ieee->Peer_bCurBW40M)
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
}
}

bool	
rtl8192se_RxCommandPacketHandle(
	struct net_device *dev, 
	struct sk_buff* skb,
	rx_desc *pdesc)
{
	u8*			pRxCmdHeader;
	u8*			pCmdContent;
	u16			total_length, offset;
	u16			RxCmdLen;
	u8			RxCmdElementID, RxCmdSeq;
	bool			RxCmdContinue;

#if 0
	if ((pRfd->queue_id != CMPK_RX_QUEUE_ID) || (pRfd == NULL))
	{
		return 0;
	}
#endif
	
	
#if 0
	if(pdesc->MACID == 0x1e)
	{	
		return 0;
	}
#endif	

	total_length = pdesc->Length;
	pRxCmdHeader = skb->data;
	offset = 0;

	do
	{
		RxCmdLen = (u16)GET_C2H_CMD_CMD_LEN(pRxCmdHeader + offset);
		RxCmdElementID = (u8)GET_C2H_CMD_ELEMENT_ID(pRxCmdHeader + offset);
		RxCmdSeq = (u8)GET_C2H_CMD_CMD_SEQ(pRxCmdHeader + offset);
		RxCmdContinue = (bool)GET_C2H_CMD_CONTINUE(pRxCmdHeader + offset);
		pCmdContent = (u8*)GET_C2H_CMD_CONTENT(pRxCmdHeader + offset);

		if((offset + C2H_RX_CMD_HDR_LEN + RxCmdLen) > total_length)
		{
			printk("Wrong C2H Cmd length!\n");
			break;
		}

		RT_TRACE(COMP_CMD, "RxCmdLen = 0x%x, RxCmdElementID = 0x%x, RxCmdSeq = 0x%x, RxCmdContinue = 0x%x\n", 
			RxCmdLen, RxCmdElementID, RxCmdSeq, RxCmdContinue);
		RT_TRACE(COMP_CMD, "Rx CMD Packet Hex Data :%x\n", total_length);
		RT_TRACE(COMP_CMD, "Rx CMD Content Hex Data :%x:%x\n", *pCmdContent, RxCmdLen);

		switch(RxCmdElementID)
		{
		case HAL_FW_C2H_CMD_C2HFEEDBACK:
			{
				switch(GET_C2H_CMD_FEEDBACK_ELEMENT_ID(pCmdContent))
				{
				case HAL_FW_C2H_CMD_C2HFEEDBACK_CCX_PER_PKT_RPT:
					{
						RT_TRACE(COMP_CMD, "HAL_FW_C2H_CMD_C2HFEEDBACK_CCX_PER_PKT_RPT FW_DBG CMD Hex:%x\n", total_length);
					}
					break;

				case HAL_FW_C2H_CMD_C2HFEEDBACK_DTM_TX_STATISTICS_RPT:
					break;

				default:
					break;
				}
			}
			break;

		case HAL_FW_C2H_CMD_C2HDBG:
			RT_TRACE(COMP_CMD,  "rtl8192se_RxCommandPacketHandle(): %x:%x<*** FW_DBG CMD String ***>\n", *pCmdContent, RxCmdLen);
			break;

		case HAL_FW_C2H_CMD_BT_State:
			{
			}
			break;
		case HAL_FW_C2H_CMD_BT_Service:
			{
			}
			break;		
		case HAL_FW_C2H_CMD_SurveyDone:
			{
				rtl8192se_rx_surveydone_cmd(dev);
			}
			break;
		default:
			RT_TRACE(COMP_CMD, "rtl8192se_RxCommandPacketHandle(): Receive unhandled C2H CMD (%d)\n", RxCmdElementID);
			break;
		}

		offset += (C2H_RX_CMD_HDR_LEN + N_BYTE_ALIGMENT(RxCmdLen, 8));

		if(!RxCmdContinue || ((offset + C2H_RX_CMD_HDR_LEN) > total_length))
		{
			break;
		}
	}while(true);
	RT_TRACE(COMP_RECV, "RxCommandPacketHandle8190Pci(): It is a command packet\n");
	
	return 1;
}

void
rtl8192se_InitializeVariables(struct net_device  *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	if (priv->rf_type == RF_1T1R) 
		strcpy(priv->nick, "rtl8191SEVA1");
	else if (priv->rf_type == RF_1T2R)
		strcpy(priv->nick, "rtl8191SEVA2");
	else
		strcpy(priv->nick, "rtl8192SE");

#ifdef _ENABLE_SW_BEACON
	priv->rtllib->softmac_features  = IEEE_SOFTMAC_SCAN | 
		IEEE_SOFTMAC_ASSOCIATE | IEEE_SOFTMAC_PROBERQ | 
		IEEE_SOFTMAC_PROBERS | IEEE_SOFTMAC_TX_QUEUE  |
		IEEE_SOFTMAC_BEACONS;
#else
#ifdef _RTL8192_EXT_PATCH_	
	priv->rtllib->softmac_features  = IEEE_SOFTMAC_SCAN | 
		IEEE_SOFTMAC_ASSOCIATE | IEEE_SOFTMAC_PROBERQ | 
		IEEE_SOFTMAC_PROBERS | IEEE_SOFTMAC_TX_QUEUE /* |
		IEEE_SOFTMAC_BEACONS*/;
#else
	priv->rtllib->softmac_features  = IEEE_SOFTMAC_SCAN | 
		IEEE_SOFTMAC_ASSOCIATE | IEEE_SOFTMAC_PROBERQ | 
		IEEE_SOFTMAC_PROBERS | IEEE_SOFTMAC_TX_QUEUE /* |
		IEEE_SOFTMAC_BEACONS*/;
#endif
#endif

	priv->rtllib->tx_headroom = 0;

	priv->ShortRetryLimit = 0x30;
	priv->LongRetryLimit = 0x30;
	
	priv->EarlyRxThreshold = 7;
	priv->pwrGroupCnt = 0;

	priv->bIgnoreSilentReset = false;  
	priv->enable_gpio0 = 0;

	priv->TransmitConfig = 0;

	priv->ReceiveConfig = 
	RCR_APPFCS | RCR_APWRMGT | /*RCR_ADD3 |*/
	RCR_AMF	| RCR_ADF | RCR_APP_MIC | RCR_APP_ICV |
       RCR_AICV	| RCR_ACRC32	|				
	RCR_AB 		| RCR_AM		|				
     	RCR_APM 	|  								
     	/*RCR_AAP		|*/	 						
     	RCR_APP_PHYST_STAFF | RCR_APP_PHYST_RXFF |	
	(priv->EarlyRxThreshold<<RCR_FIFO_OFFSET)	;

#ifdef _ENABLE_SW_BEACON
	priv->irq_mask[0] = 
	(IMR_ROK | IMR_VODOK | IMR_VIDOK | IMR_BEDOK | IMR_BKDOK |		\
	IMR_HCCADOK | IMR_MGNTDOK | IMR_COMDOK | IMR_HIGHDOK | 					\
	IMR_BDOK | IMR_RXCMDOK | /*IMR_TIMEOUT0 |*/ IMR_RDU | IMR_RXFOVW/*	|			\
	IMR_BcnInt| IMR_TXFOVW | IMR_TBDOK | IMR_TBDER*/);
#else
	priv->irq_mask[0] = 
	(IMR_ROK | IMR_VODOK | IMR_VIDOK | IMR_BEDOK | IMR_BKDOK |		\
	IMR_HCCADOK | IMR_MGNTDOK | IMR_COMDOK | IMR_HIGHDOK | 					\
	IMR_BDOK | IMR_RXCMDOK | /*IMR_TIMEOUT0 |*/ IMR_RDU | IMR_RXFOVW	|		\
	IMR_BcnInt/*| IMR_TXFOVW*/ /*| IMR_TBDOK | IMR_TBDER*/);
#endif
	priv->irq_mask[1] = 0;/* IMR_TBDOK | IMR_TBDER*/


	priv->MidHighPwrTHR_L1 = 0x3B;
	priv->MidHighPwrTHR_L2 = 0x40;
	priv->PwrDomainProtect = false;

	if (!(priv->rtllib->softmac_features & IEEE_SOFTMAC_SCAN)){
		if (!priv->scan_cmd) {
			priv->scan_cmd = kmalloc(sizeof(H2C_SITESURVEY_PARA) +
				RTL_MAX_SCAN_SIZE, GFP_KERNEL);
		}
	}

	rtl8192se_HalDetectPwrDownMode(dev); 
}

void rtl8192se_EnableInterrupt(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);	
	priv->irq_enabled = 1;
	
#ifdef RTL8192CE
	write_nic_dword(dev, REG_HIMR, priv->irq_mask[0]&0xFFFFFFFF);	
#else
	write_nic_dword(dev,INTA_MASK, priv->irq_mask[0]);
#endif

#ifdef RTL8192SE	
	write_nic_dword(dev,INTA_MASK+4, priv->irq_mask[1]&0x3F);
#endif	

}

void rtl8192se_DisableInterrupt(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);	

#ifdef RTL8192CE
	write_nic_dword(dev, REG_HIMR, IMR8190_DISABLED);	
#else
	write_nic_dword(dev,INTA_MASK,0);
#endif

#ifdef RTL8192SE	
	write_nic_dword(dev,INTA_MASK + 4,0);
#endif	
	priv->irq_enabled = 0;
}

void rtl8192se_ClearInterrupt(struct net_device *dev)
{
	u32 tmp = 0;
#ifdef RTL8192CE
	tmp = read_nic_dword(dev, REG_HISR);	
	write_nic_dword(dev, REG_HISR, tmp);
#else
	tmp = read_nic_dword(dev, ISR);	
	write_nic_dword(dev, ISR, tmp);
#endif

#ifdef RTL8192SE	
	tmp = read_nic_dword(dev, ISR+4);	
	write_nic_dword(dev, ISR+4, tmp);
#endif
}

void rtl8192se_enable_rx(struct net_device *dev)
{
    struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);

    write_nic_dword(dev, RDQDA,priv->rx_ring_dma[RX_MPDU_QUEUE]);
#ifdef CONFIG_RX_CMD
    write_nic_dword(dev, RCDA, priv->rx_ring_dma[RX_CMD_QUEUE]);
#endif
}

u32 TX_DESC_BASE[] = {TBKDA, TBEDA, TVIDA, TVODA, TBDA, TCDA, TMDA, THPDA, HDA};
void rtl8192se_enable_tx(struct net_device *dev)
{
    struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
    u32 i;
	
    for (i = 0; i < MAX_TX_QUEUE_COUNT; i++)
        write_nic_dword(dev, TX_DESC_BASE[i], priv->tx_ring[i].dma);
}


void rtl8192se_beacon_disable(struct net_device *dev) 
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	u32 reg;

#ifdef RTL8192CE
	reg = read_nic_dword(priv->rtllib->dev,REG_HIMR);

	reg &= ~(IMR_BcnInt | IMR_BcnInt | IMR_TBDOK | IMR_TBDER);
	write_nic_dword(priv->rtllib->dev, REG_HIMR, reg);	
#else
	reg = read_nic_dword(priv->rtllib->dev,INTA_MASK);

	reg &= ~(IMR_BcnInt | IMR_BcnInt | IMR_TBDOK | IMR_TBDER);
	write_nic_dword(priv->rtllib->dev, INTA_MASK, reg);	
#endif
}

void rtl8192se_interrupt_recognized(struct net_device *dev, u32 *p_inta, u32 *p_intb)
{
#ifdef RTL8192SE
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	*p_inta = read_nic_dword(dev, ISR) & priv->irq_mask[0];
#else
	*p_inta = read_nic_dword(dev, ISR) ;
#endif
	write_nic_dword(dev,ISR,*p_inta); 
#ifdef RTL8192SE
	*p_intb = read_nic_dword(dev, ISR+4);
	write_nic_dword(dev, ISR+4, *p_intb);
#endif
}

bool rtl8192se_HalRxCheckStuck(struct net_device *dev)
{
	
	struct r8192_priv *priv = rtllib_priv(dev);
	u16 				RegRxCounter = (u16)(priv->InterruptLog.nIMR_ROK&0xffff);
	bool				bStuck = false;
	u32 				SlotIndex = 0, TotalRxStuckCount = 0;
	u8				i;
	u8				SilentResetRxSoltNum = 4;


	SlotIndex = (priv->SilentResetRxSlotIndex++)%SilentResetRxSoltNum;

	if(priv->RxCounter==RegRxCounter)
	{		
		priv->SilentResetRxStuckEvent[SlotIndex] = 1;

		for( i = 0; i < SilentResetRxSoltNum ; i++ )
			TotalRxStuckCount += priv->SilentResetRxStuckEvent[i];

		if(TotalRxStuckCount  == SilentResetRxSoltNum)
		{
			bStuck = true;
			for( i = 0; i < SilentResetRxSoltNum ; i++ )
				TotalRxStuckCount += priv->SilentResetRxStuckEvent[i];
		}


	} else {
		priv->SilentResetRxStuckEvent[SlotIndex] = 0;
	}

	priv->RxCounter = RegRxCounter;

	return bStuck;
}

bool rtl8192se_HalTxCheckStuck(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	bool   	bStuck = false;
#if defined(RTL8192E) || defined(RTL8190P)
	u16    RegTxCounter = read_nic_word(dev, 0x128);
#elif defined (RTL8192SE) || defined (RTL8192CE)
	u16 	RegTxCounter = read_nic_word(dev, 0x366);
#else
	u16 	RegTxCounter = priv->TxCounter + 1;
 	WARN_ON(1);	
#endif

	RT_TRACE(COMP_RESET, "%s():RegTxCounter is %d,TxCounter is %d\n", 
			__FUNCTION__,RegTxCounter,priv->TxCounter);

	if(priv->TxCounter == RegTxCounter)
		bStuck = true;

	priv->TxCounter = RegTxCounter;

	return bStuck;
}

bool rtl8192se_GetNmodeSupportBySecCfg(struct net_device *dev)
{
#ifdef RTL8192SE
	return true;
#else
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;
	if (ieee->rtllib_ap_sec_type && 
	   (ieee->rtllib_ap_sec_type(priv->rtllib)&(SEC_ALG_WEP|SEC_ALG_TKIP))) {
		return false;
	} else {
		return true;
	}
#endif
}

bool rtl8192se_GetHalfNmodeSupportByAPs(struct net_device* dev)
{
#ifdef RTL8192SE
	return false;
#else	
	bool			Reval;
	struct r8192_priv* priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;
	
	if(ieee->bHalfWirelessN24GMode == true)
		Reval = true;
	else
		Reval =  false;

	return Reval;
#endif
}

u8 rtl8192se_QueryIsShort(u8 TxHT, u8 TxRate, cb_desc *tcb_desc)
{
	u8   tmp_Short;

	tmp_Short = (TxHT==1)?((tcb_desc->bUseShortGI)?1:0):((tcb_desc->bUseShortPreamble)?1:0);
#if defined RTL8192SE || defined RTL8192CE
	if(TxHT==1 && TxRate != DESC92S_RATEMCS15)
#elif defined RTL8192E || defined RTL8190P
	if(TxHT==1 && TxRate != DESC90_RATEMCS15)
#endif
		tmp_Short = 0;

	return tmp_Short;
}

void
ActUpdateChannelAccessSetting(
	struct net_device* 			dev,
	WIRELESS_MODE			WirelessMode,
	PCHANNEL_ACCESS_SETTING	ChnlAccessSetting
	)
{		
		struct r8192_priv* priv = rtllib_priv(dev);	
		
#if 0
		WIRELESS_MODE Tmp_WirelessMode = WirelessMode;

		switch( Tmp_WirelessMode )
		{
		case WIRELESS_MODE_A:
			ChnlAccessSetting->SlotTimeTimer = 9;
			ChnlAccessSetting->CWminIndex = 4;
			ChnlAccessSetting->CWmaxIndex = 10;
			break;
		case WIRELESS_MODE_B:
			ChnlAccessSetting->SlotTimeTimer = 20;
			ChnlAccessSetting->CWminIndex = 5;
			ChnlAccessSetting->CWmaxIndex = 10;
			break;
		case WIRELESS_MODE_G:
			ChnlAccessSetting->SlotTimeTimer = 20;
			ChnlAccessSetting->CWminIndex = 4;
			ChnlAccessSetting->CWmaxIndex = 10;
			break;
		case WIRELESS_MODE_N_24G:
		case WIRELESS_MODE_N_5G:
			ChnlAccessSetting->SlotTimeTimer = 9;
			ChnlAccessSetting->CWminIndex = 4;
			ChnlAccessSetting->CWmaxIndex = 10;
			break;
		default:
			RT_ASSERT(false, ("ActUpdateChannelAccessSetting(): Wireless mode is not defined!\n"));
			break;
		}

#endif

		{
			u16	SIFS_Timer;

			if(WirelessMode == WIRELESS_MODE_G)
				SIFS_Timer = 0x0e0e;
 			else
				SIFS_Timer = 0x1010;

			SIFS_Timer = 0x0e0e;
			priv->rtllib->SetHwRegHandler( dev, HW_VAR_SIFS,  (u8*)&SIFS_Timer);
		
		}

}
