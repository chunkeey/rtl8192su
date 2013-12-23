/******************************************************************************
 *
 * Copyright(c) 2009-2012  Realtek Corporation.
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
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/

#include "../wifi.h"
#include "../pci.h"
#include "../ps.h"
#include "reg.h"
#include "def.h"
#include "phy.h"
#include "rf.h"
#include "dm.h"
#include "fw.h"
#include "hw.h"

static u32 _rtl92s_phy_calculate_bit_shift(u32 bitmask)
{
	u32 i;

	for (i = 0; i <= 31; i++) {
		if (((bitmask >> i) & 0x1) == 1)
			break;
	}

	return i;
}

u32 rtl92s_phy_query_bb_reg(struct ieee80211_hw *hw, u32 regaddr, u32 bitmask)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 returnvalue = 0, originalvalue, bitshift;

	RT_TRACE(rtlpriv, COMP_RF, DBG_TRACE, "regaddr(%#x), bitmask(%#x)\n",
		 regaddr, bitmask);

	originalvalue = rtl92s_fw_iocmd_read(hw,
				IOCMD_CLASS_BB_RF,
				(regaddr & 0x0fff),
				IOCMD_BB_READ_IDX);
	bitshift = _rtl92s_phy_calculate_bit_shift(bitmask);
	returnvalue = (originalvalue & bitmask) >> bitshift;

	RT_TRACE(rtlpriv, COMP_RF, DBG_TRACE, "BBR MASK=0x%x Addr[0x%x]=0x%x\n",
		 bitmask, regaddr, originalvalue);

	return returnvalue;

}

void rtl92s_phy_set_bb_reg(struct ieee80211_hw *hw, u32 regaddr, u32 bitmask,
			   u32 data)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 originalvalue, bitshift;

	RT_TRACE(rtlpriv, COMP_RF, DBG_TRACE,
		 "regaddr(%#x), bitmask(%#x), data(%#x)\n",
		 regaddr, bitmask, data);

	if (bitmask != MASKDWORD) {
		originalvalue = rtl92s_fw_iocmd_read(hw,
					IOCMD_CLASS_BB_RF,
					(regaddr & 0x0fff),
					IOCMD_BB_READ_IDX);
		bitshift = _rtl92s_phy_calculate_bit_shift(bitmask);
		data = ((originalvalue & (~bitmask)) | (data << bitshift));
	}

	rtl92s_fw_iocmd_write(hw,
			IOCMD_CLASS_BB_RF,
			(regaddr & 0x0fff),
			IOCMD_BB_WRITE_IDX,
			data);

	RT_TRACE(rtlpriv, COMP_RF, DBG_TRACE,
		 "regaddr(%#x), bitmask(%#x), data(%#x)\n",
		 regaddr, bitmask, data);

}

static void _rtl92s_phy_rf_serial_write(struct ieee80211_hw *hw,
					enum radio_path rfpath, u32 offset,
					u32 data)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &(rtlpriv->phy);
	struct bb_reg_def *pphyreg = &rtlphy->phyreg_def[rfpath];
	u32 data_and_addr = 0;
	u32 newoffset;

	offset &= 0x3f;
	newoffset = offset;

	data_and_addr = ((newoffset << 20) | (data & 0x000fffff)) & 0x0fffffff;
	rtl_set_bbreg(hw, pphyreg->rf3wire_offset, MASKDWORD, data_and_addr);

	RT_TRACE(rtlpriv, COMP_RF, DBG_TRACE, "RFW-%d Addr[0x%x]=0x%x\n",
		 rfpath, pphyreg->rf3wire_offset, data_and_addr);
}

u32 rtl92s_phy_query_rf_reg(struct ieee80211_hw *hw, enum radio_path rfpath,
			    u32 regaddr, u32 bitmask)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 original_value, readback_value, bitshift;

	RT_TRACE(rtlpriv, COMP_RF, DBG_TRACE,
		 "regaddr(%#x), rfpath(%#x), bitmask(%#x)\n",
		 regaddr, rfpath, bitmask);

	spin_lock(&rtlpriv->locks.rf_lock);

	original_value = rtl92s_fw_iocmd_read(hw,
				IOCMD_CLASS_BB_RF,
				(rfpath << 8) | (regaddr & 0xff),
				IOCMD_RF_READ_IDX);

	bitshift = _rtl92s_phy_calculate_bit_shift(bitmask);
	readback_value = (original_value & bitmask) >> bitshift;

	spin_unlock(&rtlpriv->locks.rf_lock);

	RT_TRACE(rtlpriv, COMP_RF, DBG_TRACE,
		 "regaddr(%#x), rfpath(%#x), bitmask(%#x), original_value(%#x)\n",
		 regaddr, rfpath, bitmask, original_value);

	return readback_value;
}

void rtl92s_phy_set_rf_reg(struct ieee80211_hw *hw, enum radio_path rfpath,
			   u32 regaddr, u32 bitmask, u32 data)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &(rtlpriv->phy);
	u32 original_value, bitshift;

	if (!((rtlphy->rf_pathmap >> rfpath) & 0x1))
		return;

	RT_TRACE(rtlpriv, COMP_RF, DBG_TRACE,
		 "regaddr(%#x), bitmask(%#x), data(%#x), rfpath(%#x)\n",
		 regaddr, bitmask, data, rfpath);

	spin_lock(&rtlpriv->locks.rf_lock);

	if (bitmask != RFREG_OFFSET_MASK) {
		original_value = rtl92s_fw_iocmd_read(hw, IOCMD_CLASS_BB_RF,
			(rfpath << 8) | (regaddr & 0xff), IOCMD_RF_READ_IDX);
		bitshift = _rtl92s_phy_calculate_bit_shift(bitmask);
		data = ((original_value & (~bitmask)) | (data << bitshift));
	}

	rtl92s_fw_iocmd_write(hw, IOCMD_CLASS_BB_RF,
		(rfpath << 8) | (regaddr & 0xff),
		IOCMD_RF_WRITE_IDX, data);

	spin_unlock(&rtlpriv->locks.rf_lock);

	RT_TRACE(rtlpriv, COMP_RF, DBG_TRACE,
		 "regaddr(%#x), bitmask(%#x), data(%#x), rfpath(%#x)\n",
		 regaddr, bitmask, data, rfpath);
}

void rtl92s_phy_scan_operation_backup(struct ieee80211_hw *hw,
				      u8 operation)
{
}

void rtl92s_phy_set_bw_mode(struct ieee80211_hw *hw,
			    enum nl80211_channel_type ch_type)
{
	return;
}

static void _rtl92se_phy_set_rf_sleep(struct ieee80211_hw *hw)
{
	return;
}

bool rtl92s_phy_set_rf_power_state(struct ieee80211_hw *hw,
				   enum rf_pwrstate rfpwr_state)
{
	return true;
}

int rtl92s_phy_config_rf(struct ieee80211_hw *hw, enum radio_path rfpath)
{
	return 0;
}


int rtl92s_phy_mac_config(struct ieee80211_hw *hw)
{
	return 0;
}


int rtl92s_phy_bb_config(struct ieee80211_hw *hw)
{
	return 0;
}

int rtl92s_phy_rf_config(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &(rtlpriv->phy);

	/* Initialize general global value */
	if (rtlphy->rf_type == RF_1T1R)
		rtlphy->num_total_rfpath = 1;
	else
		rtlphy->num_total_rfpath = 2;

	/* Config BB and RF */
	return rtl92s_phy_rf6052_config(hw);
}

void rtl92s_phy_get_hw_reg_originalvalue(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &(rtlpriv->phy);

	/* read rx initial gain */
	rtlphy->default_initialgain[0] = rtl_get_bbreg(hw,
			REG_ROFDM0_XAAGCCORE1, MASKBYTE0);
	rtlphy->default_initialgain[1] = rtl_get_bbreg(hw,
			REG_ROFDM0_XBAGCCORE1, MASKBYTE0);
	rtlphy->default_initialgain[2] = rtl_get_bbreg(hw,
			REG_ROFDM0_XCAGCCORE1, MASKBYTE0);
	rtlphy->default_initialgain[3] = rtl_get_bbreg(hw,
			REG_ROFDM0_XDAGCCORE1, MASKBYTE0);
	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "Default initial gain (c50=0x%x, c58=0x%x, c60=0x%x, c68=0x%x)\n",
		 rtlphy->default_initialgain[0],
		 rtlphy->default_initialgain[1],
		 rtlphy->default_initialgain[2],
		 rtlphy->default_initialgain[3]);

	/* read framesync */
	rtlphy->framesync = rtl_get_bbreg(hw, REG_ROFDM0_RXDETECTOR3, MASKBYTE0);
	rtlphy->framesync_c34 = rtl_get_bbreg(hw, REG_ROFDM0_RXDETECTOR2,
					      MASKDWORD);
	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "Default framesync (0x%x) = 0x%x\n",
		 REG_ROFDM0_RXDETECTOR3, rtlphy->framesync);
}

int rtl92s_phy_set_txpower(struct ieee80211_hw *hw, u8 channel)
{
	return 0;
}

void rtl92s_phy_chk_fwcmd_iodone(struct ieee80211_hw *hw)
{
	return;
}

void rtl92s_phy_switch_ephy_parameter(struct ieee80211_hw *hw)
{
	return;
}

void rtl92s_phy_set_beacon_hwreg(struct ieee80211_hw *hw, u16 BeaconInterval)
{
}
