/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
 *
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
#include "rtl_core.h"
#include "rtl_dm.h"
#include "rtl_rfkill.h"

#ifdef CONFIG_RTL_RFKILL
static void rtl8192_before_radio_check(struct net_device *dev, 
				       bool *rf_state, 
				       bool *turnonbypowerdomain)
{  
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	PRT_POWER_SAVE_CONTROL   pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));

	*rf_state = (priv->rtllib->eRFPowerState != eRfOn);	
#ifdef CONFIG_ASPM_OR_D3
	if((pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_ASPM) && RT_IN_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_ASPM))
	{
		RT_DISABLE_ASPM(dev);
		RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_ASPM);
	}
	else if((pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_PCI_D3) && RT_IN_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_PCI_D3))
	{
#ifdef TODO             
		RT_LEAVE_D3(dev, false);
		RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_PCI_D3);
#endif
	}
#endif	
	if (RT_IN_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC)) {
		Power_DomainInit92SE(dev);
		*turnonbypowerdomain = true;
	}

}

static bool rtl8192_radio_on_off_checking(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	u8      u1Tmp = 0;
	u8 	gpio;
	
	if (priv->pwrdown) {
		u1Tmp = read_nic_byte(dev, 0x06);
		gpio = u1Tmp & BIT6;
	} else	
#ifdef CONFIG_BT_COEXIST
	if (pHalData->bt_coexist.BluetoothCoexist) {
		if (pHalData->bt_coexist.BT_CoexistType == BT_2Wire) {
			PlatformEFIOWrite1Byte(pAdapter, MAC_PINMUX_CFG, 0xa);
			u1Tmp = PlatformEFIORead1Byte(pAdapter, GPIO_IO_SEL);
			delay_us(100);
			u1Tmp = PlatformEFIORead1Byte(pAdapter, GPIO_IN);
			RTPRINT(FPWR, PWRHW, ("GPIO_IN=%02x\n", u1Tmp));
			retval = (u1Tmp & HAL_8192S_HW_GPIO_OFF_BIT) ? eRfOn : eRfOff;
		} else if ((pHalData->bt_coexist.BT_CoexistType == BT_ISSC_3Wire) ||
				(pHalData->bt_coexist.BT_CoexistType == BT_Accel) ||
				(pHalData->bt_coexist.BT_CoexistType == BT_CSR_BC4)) {
			u4tmp = PHY_QueryBBReg(pAdapter, 0x87c, bMaskDWord);
			if ((u4tmp & BIT17) != 0) {
				PHY_SetBBReg(pAdapter, 0x87c, bMaskDWord, u4tmp & ~BIT17);
				delay_us(50);
				RTPRINT(FBT, BT_RFPoll, ("BT write 0x87c (~BIT17) = 0x%x\n", u4tmp &~BIT17));
			}
			u4tmp = PHY_QueryBBReg(pAdapter, 0x8e0, bMaskDWord);
			RTPRINT(FBT, BT_RFPoll, ("BT read 0x8e0 (BIT24)= 0x%x\n", u4tmp));
			retval = (u4tmp & BIT24) ? eRfOn : eRfOff;
			RTPRINT(FBT, BT_RFPoll, ("BT check RF state to %s\n", (retval==eRfOn)? "ON":"OFF"));
		}
	} else 
#endif
	{
		write_nic_byte(dev, MAC_PINMUX_CFG, (GPIOMUX_EN | GPIOSEL_GPIO));
		u1Tmp = read_nic_byte(dev, GPIO_IO_SEL);

		u1Tmp &= HAL_8192S_HW_GPIO_OFF_MASK;
		write_nic_byte(dev, GPIO_IO_SEL, u1Tmp);

		mdelay(10);

		u1Tmp = read_nic_byte(dev, GPIO_IN);
		gpio = u1Tmp & HAL_8192S_HW_GPIO_OFF_BIT;
	}
#ifdef DEBUG_RFKILL 
	{
		static u8 	gpio_test;
		printk("%s: gpio = %x\n", __FUNCTION__, gpio);
		if(gpio_test % 5 == 0) {
			gpio = 0;
		} else {
			gpio = 1;
		}
		printk("%s: gpio_test = %d, gpio = %x\n", __FUNCTION__, gpio_test++ % 20, gpio);
	}
#endif 

	return gpio;
}

static void  rtl8192_after_radio_check(struct net_device *dev, bool rf_state, bool turnonbypowerdomain)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
        PRT_POWER_SAVE_CONTROL  pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));

	if (turnonbypowerdomain) {
		PHY_SetRtl8192seRfHalt(dev);
		RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);
	}
#ifdef CONFIG_ASPM_OR_D3
	if (!rf_state) {
		if (pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_ASPM) {
			RT_ENABLE_ASPM(dev);
			RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_ASPM);
		}
#ifdef TODO             
		else if(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_PCI_D3) {
			RT_ENTER_D3(dev, false);
			RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_PCI_D3);
		}
#endif
	}
#endif
	return;
}

static bool rtl8192_is_radio_enabled(struct net_device *dev, bool *radio_enabled)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	bool rf_state = false;
	bool turnonbypowerdomain = false;
	bool valid;

	rtl8192_before_radio_check(dev, &rf_state, &turnonbypowerdomain);
	*radio_enabled = rtl8192_radio_on_off_checking(dev);
	rtl8192_after_radio_check(dev, rf_state, turnonbypowerdomain);
	if (priv->bResetInProgress) {
		priv->RFChangeInProgress = false;
		valid = false;
	} else {
		valid = true;
	}
	
	return valid;
}

bool rtl8192_rfkill_init(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	struct wireless_dev *wdev = &priv->rtllib->wdev;
	bool radio_enabled;
	bool valid = rtl8192_is_radio_enabled(dev, &radio_enabled);

	if (valid) {
		priv->rfkill_off = radio_enabled;
		printk(KERN_INFO "rtl8192: wireless switch is %s\n",
				priv->rfkill_off ? "on" : "off");
		wiphy_rfkill_set_hw_state(wdev->wiphy, !priv->rfkill_off);
		wiphy_rfkill_start_polling(wdev->wiphy);
		return true;
	} else {
		return false;
	}
}

void rtl8192_rfkill_poll(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	struct wireless_dev *wdev = &priv->rtllib->wdev;
	bool radio_enabled;
	bool valid;
	
	if (priv->being_init_adapter) {
		return;
	}

	if (priv->ResetProgress == RESET_TYPE_SILENT) {
		RT_TRACE((COMP_INIT | COMP_POWER | COMP_RF), 
				"%s(): silent Reseting, ignore rf polling!\n", __FUNCTION__);
		return;
	}

	valid = rtl8192_is_radio_enabled(dev, &radio_enabled);
	if (valid) {
		if (unlikely(radio_enabled != priv->rfkill_off)) {
			priv->rfkill_off = radio_enabled;
			printk(KERN_INFO "rtl8192: wireless radio switch turned %s\n",
					radio_enabled ? "on" : "off");
			wiphy_rfkill_set_hw_state(wdev->wiphy, !radio_enabled);
		}
	}
}

void rtl8192_rfkill_exit(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	struct wireless_dev *wdev = &priv->rtllib->wdev;

	wiphy_rfkill_stop_polling(wdev->wiphy);
}

#endif 
