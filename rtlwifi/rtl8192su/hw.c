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
#include "../efuse.h"
#include "../base.h"
#include "../regd.h"
#include "../cam.h"
#include "../ps.h"
#include "../usb.h"
#include "reg.h"
#include "def.h"
#include "phy.h"
#include "dm.h"
#include "fw.h"
#include "led.h"
#include "hw.h"
#include "eeprom.h"

void rtl92su_get_hw_reg(struct ieee80211_hw *hw, u8 variable, u8 *val)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_ps_ctl *ppsc = rtl_psc(rtl_priv(hw));
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));

	switch (variable) {
	case HW_VAR_RCR: {
			*((u32 *)(val)) = mac->rx_conf;
			break;
		}
	case HW_VAR_RF_STATE: {
			*((enum rf_pwrstate *)(val)) = ppsc->rfpwr_state;
			break;
		}
	case HW_VAR_FW_PSMODE_STATUS: {
			*((bool *) (val)) = ppsc->fw_current_inpsmode;
			break;
		}
	case HW_VAR_CORRECT_TSF: {
			u64 tsf;
			u32 *ptsf_low = (u32 *)&tsf;
			u32 *ptsf_high = ((u32 *)&tsf) + 1;

			*ptsf_high = rtl_read_dword(rtlpriv, (REG_TSFTR + 4));
			*ptsf_low = rtl_read_dword(rtlpriv, REG_TSFTR);

			*((u64 *) (val)) = tsf;

			break;
		}
	case HW_VAR_MRC: {
			*((bool *)(val)) = rtlpriv->dm.current_mrc_switch;
			break;
		}
	default: {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "switch case not processed\n");
			break;
		}
	}
}

void rtl92su_set_hw_reg(struct ieee80211_hw *hw, u8 variable, u8 *val)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	struct rtl_ps_ctl *ppsc = rtl_psc(rtl_priv(hw));
	struct rtl_usb *rtlusb = rtl_usbdev(rtl_usbpriv(hw));

	switch (variable) {
	case HW_VAR_ETHER_ADDR:{
			// use SET_MAC_CMD!
			rtl_write_dword(rtlpriv, REG_MACID, ((u32 *)(val))[0]);
			rtl_write_word(rtlpriv, REG_MACID + 4, ((u16 *)(val + 4))[0]);
			break;
		}
	case HW_VAR_BASIC_RATE:{
			u16 rate_cfg = ((u16 *) val)[0];
			u8 rate_index = 0;

			if (rtlhal->version == VERSION_8192S_ACUT)
				rate_cfg = rate_cfg & 0x150;
			else
				rate_cfg = rate_cfg & 0x15f;

			rate_cfg |= 0x01;

			rtl_write_byte(rtlpriv, REG_RRSR, rate_cfg & 0xff);
			rtl_write_byte(rtlpriv, REG_RRSR + 1,
				       (rate_cfg >> 8) & 0xff);

			while (rate_cfg > 0x1) {
				rate_cfg = (rate_cfg >> 1);
				rate_index++;
			}
			rtl_write_byte(rtlpriv, REG_INIRTSMCS_SEL, rate_index);

			break;
		}
	case HW_VAR_BSSID:{
			rtl_write_dword(rtlpriv, REG_BSSIDR, ((u32 *)(val))[0]);
			rtl_write_word(rtlpriv, REG_BSSIDR + 4,
				       ((u16 *)(val + 4))[0]);
			break;
		}
	case HW_VAR_SIFS:{
			rtl_write_byte(rtlpriv, REG_SIFS_OFDM, val[0]);
			rtl_write_byte(rtlpriv, REG_SIFS_OFDM + 1, val[1]);
			break;
		}
	case HW_VAR_SLOT_TIME:{
			u8 e_aci;

			RT_TRACE(rtlpriv, COMP_MLME, DBG_LOUD,
				 "HW_VAR_SLOT_TIME %x\n", val[0]);

			rtl_write_byte(rtlpriv, REG_SLOT_TIME, val[0]);

			for (e_aci = 0; e_aci < AC_MAX; e_aci++) {
				rtlpriv->cfg->ops->set_hw_reg(hw,
						HW_VAR_AC_PARAM,
						(u8 *)(&e_aci));
			}
			break;
		}
	case HW_VAR_ACK_PREAMBLE:{
			u8 reg_tmp;
			u8 short_preamble = (bool) (*(u8 *) val);
			reg_tmp = (mac->cur_40_prime_sc) << 5;
			if (short_preamble)
				reg_tmp |= 0x80;

			rtl_write_byte(rtlpriv, REG_RRSR + 2, reg_tmp);
			break;
		}
	case HW_VAR_AMPDU_MIN_SPACE:{
			u8 min_spacing_to_set;
			u8 sec_min_space;

			min_spacing_to_set = *((u8 *)val);
			if (min_spacing_to_set <= 7) {
				if (rtlpriv->sec.pairwise_enc_algorithm ==
				    NO_ENCRYPTION)
					sec_min_space = 0;
				else
					sec_min_space = 1;

				if (min_spacing_to_set < sec_min_space)
					min_spacing_to_set = sec_min_space;
				if (min_spacing_to_set > 5)
					min_spacing_to_set = 5;

				mac->min_space_cfg =
						((mac->min_space_cfg & 0xf8) |
						min_spacing_to_set);

				*val = min_spacing_to_set;

				RT_TRACE(rtlpriv, COMP_MLME, DBG_LOUD,
					 "Set HW_VAR_AMPDU_MIN_SPACE: %#x\n",
					 mac->min_space_cfg);

				rtl_write_byte(rtlpriv, REG_AMPDU_MIN_SPACE,
					       mac->min_space_cfg);
			}
			break;
		}
	case HW_VAR_SHORTGI_DENSITY:{
			u8 density_to_set;

			density_to_set = *((u8 *) val);
			mac->min_space_cfg = rtlpriv->rtlhal.minspace_cfg;
			mac->min_space_cfg |= (density_to_set << 3);

			RT_TRACE(rtlpriv, COMP_MLME, DBG_LOUD,
				 "Set HW_VAR_SHORTGI_DENSITY: %#x\n",
				 mac->min_space_cfg);

			rtl_write_byte(rtlpriv, REG_AMPDU_MIN_SPACE,
				       mac->min_space_cfg);

			break;
		}
	case HW_VAR_AMPDU_FACTOR:{
			u8 factor_toset;
			u8 regtoset;
			u8 factorlevel[18] = {
				2, 4, 4, 7, 7, 13, 13,
				13, 2, 7, 7, 13, 13,
				15, 15, 15, 15, 0};
			u8 index = 0;

			factor_toset = *((u8 *) val);
			if (factor_toset <= 3) {
				factor_toset = (1 << (factor_toset + 2));
				if (factor_toset > 0xf)
					factor_toset = 0xf;

				for (index = 0; index < 17; index++) {
					if (factorlevel[index] > factor_toset)
						factorlevel[index] =
								 factor_toset;
				}

				for (index = 0; index < 8; index++) {
					regtoset = ((factorlevel[index * 2]) |
						    (factorlevel[index *
						    2 + 1] << 4));
					rtl_write_byte(rtlpriv,
						       REG_AGGLEN_LMT_L + index,
						       regtoset);
				}

				regtoset = ((factorlevel[16]) |
					    (factorlevel[17] << 4));
				rtl_write_byte(rtlpriv, REG_AGGLEN_LMT_H, regtoset);

				RT_TRACE(rtlpriv, COMP_MLME, DBG_LOUD,
					 "Set HW_VAR_AMPDU_FACTOR: %#x\n",
					 factor_toset);
			}
			break;
		}
	case HW_VAR_AC_PARAM:{
			u8 e_aci = *((u8 *) val);
			rtl92s_dm_init_edca_turbo(hw);

			if (rtlusb->acm_method != eAcmWay2_SW)
				rtlpriv->cfg->ops->set_hw_reg(hw,
						 HW_VAR_ACM_CTRL,
						 (u8 *)(&e_aci));
			break;
		}
	case HW_VAR_ACM_CTRL:{
			u8 e_aci = *((u8 *) val);
			union aci_aifsn *p_aci_aifsn = (union aci_aifsn *)(&(
							mac->ac[0].aifs));
			u8 acm = p_aci_aifsn->f.acm;
			u8 acm_ctrl = rtl_read_byte(rtlpriv, REG_ACMHWCTRL);

			acm_ctrl = acm_ctrl | ((rtlusb->acm_method == 2) ?
				   0x0 : 0x1);

			if (acm) {
				switch (e_aci) {
				case AC0_BE:
					acm_ctrl |= AcmHw_BeqEn;
					break;
				case AC2_VI:
					acm_ctrl |= AcmHw_ViqEn;
					break;
				case AC3_VO:
					acm_ctrl |= AcmHw_VoqEn;
					break;
				default:
					RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
						 "HW_VAR_ACM_CTRL acm set failed: eACI is %d\n",
						 acm);
					break;
				}
			} else {
				switch (e_aci) {
				case AC0_BE:
					acm_ctrl &= (~AcmHw_BeqEn);
					break;
				case AC2_VI:
					acm_ctrl &= (~AcmHw_ViqEn);
					break;
				case AC3_VO:
					acm_ctrl &= (~AcmHw_BeqEn);
					break;
				default:
					RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
						 "switch case not processed\n");
					break;
				}
			}

			RT_TRACE(rtlpriv, COMP_QOS, DBG_TRACE,
				 "HW_VAR_ACM_CTRL Write 0x%X\n", acm_ctrl);
			rtl_write_byte(rtlpriv, REG_ACMHWCTRL, acm_ctrl);
			break;
		}
	case HW_VAR_RCR:{
			rtl_write_dword(rtlpriv, REG_RCR, ((u32 *) (val))[0]);
			mac->rx_conf = ((u32 *) (val))[0];
			break;
		}
	case HW_VAR_RETRY_LIMIT:{
			u8 retry_limit = ((u8 *) (val))[0];

			rtl_write_word(rtlpriv, REG_RETRY_LIMIT,
				       retry_limit << RETRY_LIMIT_SHORT_SHIFT |
				       retry_limit << RETRY_LIMIT_LONG_SHIFT);
			break;
		}
	case HW_VAR_DUAL_TSF_RST: {
			break;
		}
	case HW_VAR_EFUSE_BYTES: {
			rtlefuse->efuse_usedbytes = *((u16 *) val);
			break;
		}
	case HW_VAR_EFUSE_USAGE: {
			rtlefuse->efuse_usedpercentage = *((u8 *) val);
			break;
		}
	case HW_VAR_IO_CMD: {
			break;
		}
	case HW_VAR_WPA_CONFIG: {
			rtl_write_byte(rtlpriv, REG_SECR, *((u8 *) val));
			break;
		}
	case HW_VAR_SET_RPWM:{
			break;
		}
	case HW_VAR_H2C_FW_PWRMODE:{
			break;
		}
	case HW_VAR_FW_PSMODE_STATUS: {
			ppsc->fw_current_inpsmode = *((bool *) val);
			break;
		}
	case HW_VAR_H2C_FW_JOINBSSRPT:{
			break;
		}
	case HW_VAR_AID:{
			break;
		}
	case HW_VAR_CORRECT_TSF:{
			break;
		}
	case HW_VAR_MRC: {
			bool bmrc_toset = *((bool *)val);
//			u8 u1bdata = 0;

			if (bmrc_toset) {
/*
				rtl_set_bbreg(hw, ROFDM0_TRXPATHENABLE,
					      MASKBYTE0, 0x33);
				u1bdata = (u8)rtl_get_bbreg(hw,
						ROFDM1_TRXPATHENABLE,
						MASKBYTE0);
				rtl_set_bbreg(hw, ROFDM1_TRXPATHENABLE,
					      MASKBYTE0,
					      ((u1bdata & 0xf0) | 0x03));
				u1bdata = (u8)rtl_get_bbreg(hw,
						ROFDM0_TRXPATHENABLE,
						MASKBYTE1);
				rtl_set_bbreg(hw, ROFDM0_TRXPATHENABLE,
					      MASKBYTE1,
					      (u1bdata | 0x04));
*/

				/* Update current settings. */
				rtlpriv->dm.current_mrc_switch = bmrc_toset;
			} else {
/*
				rtl_set_bbreg(hw, ROFDM0_TRXPATHENABLE,
					      MASKBYTE0, 0x13);
				u1bdata = (u8)rtl_get_bbreg(hw,
						 ROFDM1_TRXPATHENABLE,
						 MASKBYTE0);
				rtl_set_bbreg(hw, ROFDM1_TRXPATHENABLE,
					      MASKBYTE0,
					      ((u1bdata & 0xf0) | 0x01));
				u1bdata = (u8)rtl_get_bbreg(hw,
						ROFDM0_TRXPATHENABLE,
						MASKBYTE1);
				rtl_set_bbreg(hw, ROFDM0_TRXPATHENABLE,
					      MASKBYTE1, (u1bdata & 0xfb));
*/

				/* Update current settings. */
				rtlpriv->dm.current_mrc_switch = bmrc_toset;
			}

			break;
		}
	default:
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "switch case not processed\n");
		break;
	}

}

void rtl92su_enable_hw_security_config(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 sec_reg_value = 0x0;

	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "PairwiseEncAlgorithm = %d GroupEncAlgorithm = %d\n",
		 rtlpriv->sec.pairwise_enc_algorithm,
		 rtlpriv->sec.group_enc_algorithm);

	if (rtlpriv->cfg->mod_params->sw_crypto || rtlpriv->sec.use_sw_sec) {
		RT_TRACE(rtlpriv, COMP_SEC, DBG_DMESG,
			 "not open hw encryption\n");
		return;
	}

	sec_reg_value = SCR_TXENCENABLE | SCR_RXENCENABLE;

	if (rtlpriv->sec.use_defaultkey) {
		sec_reg_value |= SCR_TXUSEDK;
		sec_reg_value |= SCR_RXUSEDK;
	}

	RT_TRACE(rtlpriv, COMP_SEC, DBG_LOUD, "The SECR-value %x\n",
		 sec_reg_value);

	rtlpriv->cfg->ops->set_hw_reg(hw, HW_VAR_WPA_CONFIG, &sec_reg_value);

}

void rtl92su_update_interrupt_mask(struct ieee80211_hw *hw,
                                  u32 add_msr, u32 rm_msr)
{
}

static u8 _rtl92su_halset_sysclk(struct ieee80211_hw *hw, u16 data)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 waitcount = 100;
	bool bresult = false;
	u16 tmpvalue;

	rtl_write_word(rtlpriv, REG_SYS_CLKR, data);

	/* Wait the MAC synchronized. */
	udelay(400);

	/* Check if it is set ready. */
	tmpvalue = rtl_read_word(rtlpriv, REG_SYS_CLKR);
	bresult = ((tmpvalue & SYS_FWHW_SEL) == (data & SYS_FWHW_SEL));

	if ((data & (SYS_SWHW_SEL | SYS_FWHW_SEL)) == false) {
		waitcount = 100;
		tmpvalue = 0;

		while (1) {
			waitcount--;

			tmpvalue = rtl_read_word(rtlpriv, REG_SYS_CLKR);
			if ((tmpvalue & SYS_SWHW_SEL))
				break;

			pr_err("wait for SYS_SWHW_SEL in %x\n", tmpvalue);
			if (waitcount == 0)
				break;

			udelay(10);
		}

		if (waitcount == 0)
			bresult = false;
		else
			bresult = true;
	}

	return bresult;
}

static void rtl8192su_gpiobit4_cfg_inputmode(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 u1tmp;

	/* The following config GPIO function */
	rtl_write_byte(rtlpriv, REG_MAC_PINMUX_CTRL, (GPIOMUX_EN | GPIOSEL_GPIO));
	u1tmp = rtl_read_byte(rtlpriv, REG_GPIO_IO_SEL);

	/* config GPIO4 to input */
	u1tmp &= ~HAL_8192S_HW_GPIO_WPS_BIT;
	rtl_write_byte(rtlpriv, REG_GPIO_IO_SEL, u1tmp);
}

static u8 _rtl92su_wps_detect(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 u1tmp;
	u8 retval = ERFON;

	/* The following config GPIO function */
	rtl_write_byte(rtlpriv, REG_MAC_PINMUX_CTRL, (GPIOMUX_EN | GPIOSEL_GPIO));
	u1tmp = rtl_read_byte(rtlpriv, REG_GPIO_IO_SEL);

	/* config GPIO4 to input */
	u1tmp &= ~HAL_8192S_HW_GPIO_WPS_BIT;
	rtl_write_byte(rtlpriv, REG_GPIO_IO_SEL, u1tmp);

	/* On some of the platform, driver cannot read correct
	 * value without delay between Write_GPIO_SEL and Read_GPIO_IN */
	mdelay(10);

	/* check GPIO4 */
	u1tmp = rtl_read_byte(rtlpriv, REG_GPIO_CTRL);
	retval = (u1tmp & HAL_8192S_HW_GPIO_WPS_BIT) ? ERFON : ERFOFF;

	return retval;
}

static void _rtl92su_macconfig_before_fwdownload(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_ps_ctl *ppsc = rtl_psc(rtl_priv(hw));

	u8 i;
	u8 tmpu1b;
	u16 tmpu2b;
	u8 pollingcnt = 20;

	/* Prevent EFUSE leakage */
	rtl_write_byte(rtlpriv, REG_EFUSE_TEST + 3, 0xb0);
	msleep(20);
	rtl_write_byte(rtlpriv, REG_EFUSE_TEST + 3, 0x30);

	/* Switch to SW IO control */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_CLKR);
	if (tmpu2b & SYS_FWHW_SEL) {
		tmpu2b &= ~(SYS_SWHW_SEL | SYS_FWHW_SEL);

		/* Set failed, return to prevent hang. */
		if (!_rtl92su_halset_sysclk(hw, tmpu2b))
			return;
	}

	rtl_write_byte(rtlpriv, REG_AFE_PLL_CTRL, 0x0);
	udelay(50);
	rtl_write_byte(rtlpriv, REG_LDOA15_CTRL, 0x54); // 0x34
	udelay(50);

	/* Reset MAC-IO and CPU and Core Digital BIT(10)/11/15 */
	tmpu1b = rtl_read_byte(rtlpriv, REG_SYS_FUNC_EN + 1);
	tmpu1b &= 0x73;
	rtl_write_byte(rtlpriv, REG_SYS_FUNC_EN + 1, tmpu1b);
	/* wait for BIT 10/11/15 to pull high automatically!! */
	mdelay(1);

	rtl_write_byte(rtlpriv, REG_SPS0_CTRL + 1, 0x53);
	rtl_write_byte(rtlpriv, REG_SPS0_CTRL + 0, 0x57);

	rtl_write_byte(rtlpriv, REG_CR, 0);
	rtl_write_byte(rtlpriv, REG_TCR, 0);

	/* Enable AFE clock source */
	tmpu1b = rtl_read_byte(rtlpriv, REG_AFE_XTAL_CTRL);
	rtl_write_byte(rtlpriv, REG_AFE_XTAL_CTRL, (tmpu1b | 0x01));
	/* Delay 1.5ms */
	mdelay(2);
	tmpu1b = rtl_read_byte(rtlpriv, REG_AFE_XTAL_CTRL + 1);
	rtl_write_byte(rtlpriv, REG_AFE_XTAL_CTRL + 1, (tmpu1b & 0xfb));

	/* Enable AFE Macro Block's Bandgap */
	tmpu1b = rtl_read_byte(rtlpriv, REG_AFE_MISC);
	rtl_write_byte(rtlpriv, REG_AFE_MISC, (tmpu1b | AFE_BGEN));
	mdelay(1);

	/* Enable AFE Mbias */
	tmpu1b = rtl_read_byte(rtlpriv, REG_AFE_MISC);
	rtl_write_byte(rtlpriv, REG_AFE_MISC, (tmpu1b | AFE_MBEN |
		       AFE_MISC_I32_EN));
	mdelay(1);

	/* Enable LDOA15 block	*/
	tmpu1b = rtl_read_byte(rtlpriv, REG_LDOA15_CTRL);
	rtl_write_byte(rtlpriv, REG_LDOA15_CTRL, (tmpu1b | LDA15_EN));

	/* Enable LDOV12D block */
	tmpu1b = rtl_read_byte(rtlpriv, REG_LDOV12D_CTRL);
	rtl_write_byte(rtlpriv, REG_LDOV12D_CTRL, (tmpu1b | LDV12_EN));

	/* Set Digital Vdd to Retention isolation Path. */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_ISO_CTRL);
	rtl_write_word(rtlpriv, REG_SYS_ISO_CTRL, (tmpu2b | ISO_PWC_DV2RP));

	/* For warm reboot NIC disappear bug. */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_FUNC_EN);
	rtl_write_word(rtlpriv, REG_SYS_FUNC_EN, (tmpu2b | BIT(13)));

	/* Support 64k IMEM */
	tmpu1b = rtl_read_byte(rtlpriv, REG_SYS_ISO_CTRL + 1);
	rtl_write_byte(rtlpriv, REG_SYS_ISO_CTRL + 1, (tmpu1b & 0x68));

	/* Enable AFE PLL Macro Block */
	/* We need to delay 100u before enabling PLL. */
	udelay(200);
	tmpu1b = rtl_read_byte(rtlpriv, REG_AFE_PLL_CTRL);
	rtl_write_byte(rtlpriv, REG_AFE_PLL_CTRL, (tmpu1b | BIT(0) | BIT(4)));

	/* for divider reset  */
	udelay(500);
	rtl_write_byte(rtlpriv, REG_AFE_PLL_CTRL, (tmpu1b | BIT(0) |
		       BIT(4) | BIT(6)));
	udelay(500);
	rtl_write_byte(rtlpriv, REG_AFE_PLL_CTRL, (tmpu1b | BIT(0) | BIT(4)));
	udelay(500);

	/* Enable MAC 80MHZ clock  */
	tmpu1b = rtl_read_byte(rtlpriv, REG_AFE_PLL_CTRL + 1);
	rtl_write_byte(rtlpriv, REG_AFE_PLL_CTRL + 1, (tmpu1b | BIT(0)));
	mdelay(1);

	/* Release isolation AFE PLL & MD */
	tmpu1b = rtl_read_byte(rtlpriv, REG_SYS_ISO_CTRL);
	rtl_write_byte(rtlpriv, REG_SYS_ISO_CTRL, (tmpu1b & 0xee));

	/* Switch to 40MHz clock */
	rtl_write_byte(rtlpriv, REG_SYS_CLKR, 0x00);
	/* Disable CPU clock and 80MHz SSC to fix FW download timing issue */
	tmpu1b = rtl_read_byte(rtlpriv, REG_SYS_CLKR);
	rtl_write_byte(rtlpriv, REG_SYS_CLKR, (tmpu1b | 0xa0));
	/* Enable MAC clock */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_CLKR);
	rtl_write_word(rtlpriv, REG_SYS_CLKR, (tmpu2b | BIT(12) | BIT(11)));

	rtl_write_byte(rtlpriv, REG_PMC_FSM, 0x02);

	/* Enable Core digital and enable IOREG R/W */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_FUNC_EN);
	rtl_write_word(rtlpriv, REG_SYS_FUNC_EN, (tmpu2b | BIT(11)));

	tmpu1b = rtl_read_byte(rtlpriv, REG_SYS_FUNC_EN + 1);
	rtl_write_byte(rtlpriv, REG_SYS_FUNC_EN + 1, tmpu1b & ~(BIT(7)));

	/* enable REG_EN */
	rtl_write_word(rtlpriv, REG_SYS_FUNC_EN, (tmpu2b | BIT(11) | BIT(15)));

	/* Switch the control path. */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_CLKR);
	rtl_write_word(rtlpriv, REG_SYS_CLKR, (tmpu2b & ~SYS_CPU_CLKSEL));

	tmpu2b = ((tmpu2b | SYS_FWHW_SEL) & (~SYS_SWHW_SEL));
	if (!_rtl92su_halset_sysclk(hw, tmpu2b))
		return; /* Set failed, return to prevent hang. */

	rtl_write_word(rtlpriv, REG_CR, 0x07FC);

	/* MH We must enable the section of code to prevent load IMEM fail. */
	/* Load MAC register from WMAc temporarily We simulate macreg. */
	/* txt HW will provide MAC txt later  */
	rtl_write_byte(rtlpriv, 0x10250006, 0x30);

	rtl_write_byte(rtlpriv, REG_RCR + 1, 0xf0);
	rtl_write_byte(rtlpriv, REG_RCR + 3, 0x81);

	rtl_write_byte(rtlpriv, REG_PBP, 0x21);

	rtl_write_byte(rtlpriv, REG_TXDESC_MSK, 0xff);
	rtl_write_byte(rtlpriv, REG_TXDESC_MSK + 1, 0xff);
	rtl_write_byte(rtlpriv, REG_TXDESC_MSK + 2, 0xff);
	rtl_write_byte(rtlpriv, REG_TXDESC_MSK + 3, 0xff);

	rtl_write_byte(rtlpriv, REG_RXFLTMAP2, 0x00);
	rtl_write_byte(rtlpriv, REG_RXFLTMAP2 + 1, 0x00);

	for (i = 0; i < 32; i++)
		rtl_write_byte(rtlpriv, REG_INIMCS_SEL + i, 0x1b);

/*
	rtl_write_byte(rtlpriv, CFEND_TH, 0xff);

	rtl_write_byte(rtlpriv, DBG_PORT, 0x91);
*/

	rtl_write_word(rtlpriv, REG_CR, 0x37FC);

	/* Fix USB RX FIFO error */
	rtl_write_byte(rtlpriv, 0x1025FE5C, rtl_read_byte(rtlpriv, 0x1025FE5C) |
			BIT(7));

	/* Fix 8051 ROM incorrect code operation */
	rtl_write_byte(rtlpriv, 0x1025FE1C, 0x80);

	/* To make sure that TxDMA can ready to download FW. */
	/* We should reset TxDMA if IMEM RPT was not ready. */
	do {
		tmpu1b = rtl_read_byte(rtlpriv, REG_TCR);
		if ((tmpu1b & TXDMA_INIT_VALUE) == TXDMA_INIT_VALUE)
			break;

		udelay(5);
	} while (pollingcnt--);

	if (pollingcnt <= 0) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "Polling TXDMA_INIT_VALUE timeout!! Current TCR(%#x)\n",
			 tmpu1b);
		tmpu1b = rtl_read_byte(rtlpriv, REG_CR);
		rtl_write_byte(rtlpriv, REG_CR, tmpu1b & (~TXDMA_EN));
		udelay(2);
		/* Reset TxDMA */
		rtl_write_byte(rtlpriv, REG_CR, tmpu1b | TXDMA_EN);
	}

	/* After MACIO reset,we must refresh LED state. */
	if ((ppsc->rfoff_reason == RF_CHANGE_BY_IPS) ||
	   (ppsc->rfoff_reason == 0)) {
		struct rtl_usb_priv *usbpriv = rtl_usbpriv(hw);
		struct rtl_led *pLed0 = &(usbpriv->ledctl.sw_led0);
		rtl92su_sw_led_on(hw, pLed0);
	}
}

static void _rtl92su_macconfig_after_fwdownload(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	u8 i;
	u16 tmpu2b;

	/* 1. System Configure Register (Offset: 0x0000 - 0x003F) */

	/* 2. Command Control Register (Offset: 0x0040 - 0x004F) */
	/* Turn on 0x40 Command register */
	rtl_write_word(rtlpriv, REG_CR, (BBRSTN | BB_GLB_RSTN |
			SCHEDULE_EN | MACRXEN | MACTXEN | DDMA_EN | FW2HW_EN |
			RXDMA_EN | TXDMA_EN | HCI_RXDMA_EN | HCI_TXDMA_EN));

	/* Set TCR TX DMA pre 2 FULL enable bit	*/
/*
	rtl_write_dword(rtlpriv, TCR, rtl_read_dword(rtlpriv, TCR) |
			TXDMAPRE2FULL);
*/

	/* Set RCR	*/
	rtl_write_dword(rtlpriv, REG_RCR, mac->rx_conf);

	/* 3. MACID Setting Register (Offset: 0x0050 - 0x007F) */

	/* 4. Timing Control Register  (Offset: 0x0080 - 0x009F) */
	/* Set CCK/OFDM SIFS */
	/* CCK SIFS shall always be 10us. */
	rtl_write_word(rtlpriv, REG_SIFS_CCK, 0x0a0a);
	rtl_write_word(rtlpriv, REG_SIFS_OFDM, 0x1010);

	/* Set AckTimeout */
	rtl_write_byte(rtlpriv, REG_ACK_TIMEOUT, 0x40);

	/* Beacon related */
	rtl_write_word(rtlpriv, REG_BCN_INTERVAL, 100);
	rtl_write_word(rtlpriv, REG_ATIMWND, 2);

	/* 5. FIFO Control Register (Offset: 0x00A0 - 0x015F) */
	tmpu2b = rtl_read_word(rtlpriv, REG_RXFLTMAP0);
	RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE, "MgtRxFilter: %04x\n", tmpu2b);
	tmpu2b &= ~(BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5) |
		    BIT(10) | BIT(11) | BIT(12) | BIT(13));
	RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE, "MgtRxFilter: %04x\n", tmpu2b);
	rtl_write_word(rtlpriv, REG_RXFLTMAP0, tmpu2b);
	rtl_write_word(rtlpriv, REG_RXFLTMAP1, 0x0000);
	rtl_write_word(rtlpriv, REG_RXFLTMAP2, 0x0000);
	/* 5.1 Initialize Number of Reserved Pages in Firmware Queue */
	/* Firmware allocate now, associate with FW internal setting.!!! */

	/* 5.2 Setting TX/RX page size 0/1/2/3/4=64/128/256/512/1024 */
	rtl_write_byte(rtlpriv, REG_PBP, rtl_read_byte(rtlpriv, REG_PBP) | BIT(0));
	/* 5.3 Set driver info, we only accept PHY status now. */
	/* 5.4 Set RXDMA arbitration to control RXDMA/MAC/FW R/W for RXFIFO  */
	/* Set USB RX aggregation */
	rtl_write_byte(rtlpriv, REG_RXDMA_RXCTRL,
		rtl_read_byte(rtlpriv, REG_RXDMA_RXCTRL) | RXDMA_AGG_EN);
	//rtl_write_byte(rtlpriv, RXDMA, rtl_read_byte(rtlpriv, RXDMA) | BIT(6));
	/* Set TH=1 */
	rtl_write_byte(rtlpriv, REG_RXDMA_AGG_PG_TH, 1);

	/* 6. Adaptive Control Register  (Offset: 0x0160 - 0x01CF) */
	/* Set RRSR to all legacy rate and HT rate
	 * CCK rate is supported by default.
	 * CCK rate will be filtered out only when associated
	 * AP does not support it.
	 * Only enable ACK rate to OFDM 24M
	 * Disable RRSR for CCK rate in A-Cut	*/

	if (rtlhal->version == VERSION_8192S_ACUT)
		rtl_write_byte(rtlpriv, REG_RRSR, 0xf0);
	else if (rtlhal->version == VERSION_8192S_BCUT)
		rtl_write_byte(rtlpriv, REG_RRSR, 0xff);
	rtl_write_byte(rtlpriv, REG_RRSR + 1, 0x01);
	rtl_write_byte(rtlpriv, REG_RRSR + 2, 0x00);

	/* A-Cut IC do not support CCK rate. We forbid ARFR to */
	/* fallback to CCK rate */
	for (i = 0; i < 8; i++) {
		/*Disable RRSR for CCK rate in A-Cut */
		if (rtlhal->version == VERSION_8192S_ACUT)
			rtl_write_dword(rtlpriv, REG_ARFR0 + i * 4, 0x1f0ff0f0);
	}

	/* Different rate use different AMPDU size */
	/* MCS32/ MCS15_SG use max AMPDU size 15*2=30K */
	rtl_write_byte(rtlpriv, REG_AGGLEN_LMT_H, 0x0f);
	/* MCS0/1/2/3 use max AMPDU size 4*2=8K */
	rtl_write_word(rtlpriv, REG_AGGLEN_LMT_L, 0x7442);
	/* MCS4/5 use max AMPDU size 8*2=16K 6/7 use 10*2=20K */
	rtl_write_word(rtlpriv, REG_AGGLEN_LMT_L + 2, 0xddd7);
	/* MCS8/9 use max AMPDU size 8*2=16K 10/11 use 10*2=20K */
	rtl_write_word(rtlpriv, REG_AGGLEN_LMT_L + 4, 0xd772);
	/* MCS12/13/14/15 use max AMPDU size 15*2=30K */
	rtl_write_word(rtlpriv, REG_AGGLEN_LMT_L + 6, 0xfffd);

	/* Set Data / Response auto rate fallack retry count */
	rtl_write_dword(rtlpriv, REG_DARFRC, 0x04010000);
	rtl_write_dword(rtlpriv, REG_DARFRC + 4, 0x09070605);
	rtl_write_dword(rtlpriv, REG_RARFRC, 0x04010000);
	rtl_write_dword(rtlpriv, REG_RARFRC + 4, 0x09070605);

	/* 7. EDCA Setting Register (Offset: 0x01D0 - 0x01FF) */
	/* Set all rate to support SG */
	rtl_write_word(rtlpriv, REG_SG_RATE, 0xFFFF);

	/* 8. WMAC, BA, and CCX related Register (Offset: 0x0200 - 0x023F) */
	/* Set NAV protection length */
	rtl_write_word(rtlpriv, REG_NAV_PROT_LEN, 0x0080);
	/* CF-END Threshold */
	rtl_write_byte(rtlpriv, REG_CFEND_TH, 0xFF);
	/* Set AMPDU minimum space */
	rtl_write_byte(rtlpriv, REG_AMPDU_MIN_SPACE, 0x07);
	/* Set TXOP stall control for several queue/HI/BCN/MGT/ */
	rtl_write_byte(rtlpriv, REG_TXOP_STALL_CTRL, 0x00);

	/* 9. Security Control Register (Offset: 0x0240 - 0x025F) */
	/* 10. Power Save Control Register (Offset: 0x0260 - 0x02DF) */
	/* 11. General Purpose Register (Offset: 0x02E0 - 0x02FF) */
	/* 12. Host Interrupt Status Register (Offset: 0x0300 - 0x030F) */
	/* 13. Test Mode and Debug Control Register (Offset: 0x0310 - 0x034F) */

	/* 14. Set driver info, we only accept PHY status now. */
	rtl_write_byte(rtlpriv, REG_RX_DRVINFO_SZ, 4);

	/* 15. For EEPROM R/W Workaround */
	/* 16. For EFUSE to share REG_SYS_FUNC_EN with EEPROM!!! */
/*
	tmpu2b = rtl_read_byte(rtlpriv, REG_SYS_FUNC_EN);
	rtl_write_byte(rtlpriv, REG_SYS_FUNC_EN, tmpu2b | BIT(13));
	tmpu2b = rtl_read_byte(rtlpriv, REG_SYS_ISO_CTRL);
	rtl_write_byte(rtlpriv, REG_SYS_ISO_CTRL, tmpu2b & (~BIT(8)));
*/

	/* 17. For EFUSE */
#if 0
	/* We may R/W EFUSE in EEPROM mode */
	if (rtlefuse->epromtype == EEPROM_BOOT_EFUSE) {
		u8	tempval;

		tempval = rtl_read_byte(rtlpriv, REG_SYS_ISO_CTRL + 1);
		tempval &= 0xFE;
		rtl_write_byte(rtlpriv, REG_SYS_ISO_CTRL + 1, tempval);

		/* Change Program timing */
		rtl_write_byte(rtlpriv, REG_EFUSE_CTRL + 3, 0x72);
		RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG, "EFUSE CONFIG OK\n");
	}
#endif

	/* Set USB DMA Aggregation Timeout: 1.7/4ms */
	rtl_write_byte(rtlpriv, REG_USB_DMA_AGG_TO, 0x04);
	/* Fix USB RX FIFO error */
	rtl_write_byte(rtlpriv, 0x1025FE5C, rtl_read_byte(rtlpriv, 0xFE5C) |
			BIT(7));

	RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG, "OK\n");

}

static void _rtl92su_hw_configure(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &(rtlpriv->phy);
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));

	u8 reg_bw_opmode = 0;
	u32 reg_rrsr = 0;
	u8 regtmp = 0;

	reg_bw_opmode = BW_OPMODE_20MHZ;
	reg_rrsr = RATE_ALL_CCK | RATE_ALL_OFDM_AG;

	regtmp = rtl_read_byte(rtlpriv, REG_INIRTSMCS_SEL);
	reg_rrsr = ((reg_rrsr & 0x000fffff) << 8) | regtmp;
	rtl_write_dword(rtlpriv, REG_INIRTSMCS_SEL, reg_rrsr);
	rtl_write_byte(rtlpriv, REG_BWOPMODE, reg_bw_opmode);

	rtl_write_byte(rtlpriv, REG_MLT, 0x8f);

	/* For Min Spacing configuration. */
	switch (rtlphy->rf_type) {
	case RF_1T2R:
	case RF_1T1R:
		rtlhal->minspace_cfg = (MAX_MSS_DENSITY_1T << 3);
		break;
	case RF_2T2R:
	case RF_2T2R_GREEN:
		rtlhal->minspace_cfg = (MAX_MSS_DENSITY_2T << 3);
		break;
	}
	rtl_write_byte(rtlpriv, REG_AMPDU_MIN_SPACE, rtlhal->minspace_cfg);
}

void rtl92su_read_chip_version(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	u8 tmp_byte;

	tmp_byte = (rtl_read_dword(rtlpriv, REG_PMC_FSM) >> 15) & 0x1F;
	if (tmp_byte != 0x3) {
		tmp_byte = (tmp_byte >> 1) + 1;
		if (tmp_byte > 0x3)
			tmp_byte = 0x2; /* default to BCUT */
	}
	rtlhal->version = (enum version_8192s)tmp_byte;

	printk(KERN_INFO " --------------------------- Chip version %d\n", tmp_byte);

	rtlhal->hw_type = HARDWARE_TYPE_RTL8192SU;
}

int rtl92su_hw_init(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	struct rtl_phy *rtlphy = &(rtlpriv->phy);
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	//u8 tmp_byte = 0;

	bool rtstatus = true;
	//u8 tmp_u1b;
	int err = false;
	u8 i;
	int wdcapra_add[] = {
		REG_EDCA_BE_PARAM, REG_EDCA_BK_PARAM,
		REG_EDCA_VI_PARAM, REG_EDCA_VO_PARAM};
	u8 secr_value = 0x0;

	rtlhal->h2c_txcmd_seq = 0x01;

	rtl_write_byte(rtlpriv, REG_USB_HRPWM, 0x00);

	/* 1. MAC Initialize */
	/* Before FW download, we have to set some MAC register */
	_rtl92su_macconfig_before_fwdownload(hw);

	rtl8192su_gpiobit4_cfg_inputmode(hw);

	/* 2. download firmware */
	rtstatus = rtl92s_download_fw(hw);
	if (!rtstatus) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
			 "Failed to download FW. Init HW without FW now... "
			 "Please copy FW into /lib/firmware/rtlwifi\n");
		return 1;
	}

	/* After FW download, we have to reset MAC register */
	_rtl92su_macconfig_after_fwdownload(hw);

	/*Retrieve default FW Cmd IO map. */
	//rtlhal->fwcmd_iomap =	rtl_read_word(rtlpriv, LBUS_MON_ADDR);
	//rtlhal->fwcmd_ioparam = rtl_read_dword(rtlpriv, LBUS_ADDR_MASK);

	/* 3. Initialize MAC/PHY Config by MACPHY_reg.txt */
	if (!rtl92s_phy_mac_config(hw)) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG, "MAC Config failed\n");
		return rtstatus;
	}

	/* Make sure BB/RF write OK. We should prevent enter IPS. radio off. */
	/* We must set flag avoid BB/RF config period later!! */
	//rtl_write_dword(rtlpriv, CMDR, 0x37FC);

	/* 4. Initialize BB After MAC Config PHY_reg.txt, AGC_Tab.txt */
	if (!rtl92s_phy_bb_config(hw)) {
		RT_TRACE(rtlpriv, COMP_INIT, DBG_EMERG, "BB Config failed\n");
		return rtstatus;
	}

	/* 5. Initiailze RF RAIO_A.txt RF RAIO_B.txt */
	/* Before initalizing RF. We can not use FW to do RF-R/W. */

	rtlphy->rf_mode = RF_OP_BY_SW_3WIRE;

	/* RF Power Save */
#if 0
	/* H/W or S/W RF OFF before sleep. */
	if (rtlpriv->psc.rfoff_reason > RF_CHANGE_BY_PS) {
		u32 rfoffreason = rtlpriv->psc.rfoff_reason;

		rtlpriv->psc.rfoff_reason = RF_CHANGE_BY_INIT;
		rtlpriv->psc.rfpwr_state = ERFON;
		/* FIXME: check spinlocks if this block is uncommented */
		rtl_ps_set_rf_state(hw, ERFOFF, rfoffreason);
	} else {
		/* gpio radio on/off is out of adapter start */
		if (rtlpriv->psc.hwradiooff == false) {
			rtlpriv->psc.rfpwr_state = ERFON;
			rtlpriv->psc.rfoff_reason = 0;
		}
	}
#endif

	/* Before RF-R/W we must execute the IO from Scott's suggestion. */
/*
	rtl_write_byte(rtlpriv, AFE_XTAL_CTRL + 1, 0xDB);
	if (rtlhal->version == VERSION_8192S_ACUT)
		rtl_write_byte(rtlpriv, SPS1_CTRL + 3, 0x07);
	else
		rtl_write_byte(rtlpriv, RF_CTRL, 0x07);
*/

	if (!rtl92s_phy_rf_config(hw)) {
		RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG, "RF Config failed\n");
		return rtstatus;
	}

	/* After read predefined TXT, we must set BB/MAC/RF
	 * register as our requirement */

/*
	rtlphy->rfreg_chnlval[0] = rtl92s_phy_query_rf_reg(hw,
							   (enum radio_path)0,
							   RF_CHNLBW,
							   RFREG_OFFSET_MASK);
	rtlphy->rfreg_chnlval[1] = rtl92s_phy_query_rf_reg(hw,
							   (enum radio_path)1,
							   RF_CHNLBW,
							   RFREG_OFFSET_MASK);
*/

	/*---- Set CCK and OFDM Block "ON"----*/
	//rtl_set_bbreg(hw, RFPGA0_RFMOD, BCCKEN, 0x1);
	//rtl_set_bbreg(hw, RFPGA0_RFMOD, BOFDMEN, 0x1);

	/*3 Set Hardware(Do nothing now) */
	_rtl92su_hw_configure(hw);

	/* Read EEPROM TX power index and PHY_REG_PG.txt to capture correct */
	/* TX power index for different rate set. */
	/* Get original hw reg values */
	//rtl92s_phy_get_hw_reg_originalvalue(hw);
	/* Write correct tx power index */
	rtl92s_phy_set_txpower(hw, rtlphy->current_channel);

	/* We must set MAC address after firmware download. */
	for (i = 0; i < 6; i++)
		rtl_write_byte(rtlpriv, REG_MACID + i, rtlefuse->dev_addr[i]);

	/* EEPROM R/W workaround */
/*
	tmp_u1b = rtl_read_byte(rtlpriv, MAC_PINMUX_CTRL);
	rtl_write_byte(rtlpriv, MAC_PINMUX_CTRL, tmp_u1b & (~BIT(3)));

	rtl_write_byte(rtlpriv, 0x4d, 0x0);

	if (hal_get_firmwareversion(rtlpriv) >= 0x49) {
		tmp_byte = rtl_read_byte(rtlpriv, FW_RSVD_PG_CRTL) & (~BIT(4));
		tmp_byte = tmp_byte | BIT(5);
		rtl_write_byte(rtlpriv, FW_RSVD_PG_CRTL, tmp_byte);
		rtl_write_dword(rtlpriv, TXDESC_MSK, 0xFFFFCFFF);
	}
*/

	/* We enable high power and RA related mechanism after NIC
	 * initialized. */
	//rtl92s_phy_set_fw_cmd(hw, FW_CMD_RA_INIT);

	/* Add to prevent ASPM bug. */
	/* Always enable hst and NIC clock request. */
	//rtl92s_phy_switch_ephy_parameter(hw);

	/* Security related
	 * 1. Clear all H/W keys.
	 * 2. Enable H/W encryption/decryption. */
	rtl_cam_reset_all_entry(hw);
	secr_value |= SCR_TXENCENABLE;
	secr_value |= SCR_RXENCENABLE;
	secr_value |= SCR_NOSKMC;
	rtl_write_byte(rtlpriv, REG_SECR, secr_value);

	for (i = 0; i < 4; i++)
		rtl_write_dword(rtlpriv, wdcapra_add[i], 0x5e4322);

	if (rtlphy->rf_type == RF_1T2R) {
		bool mrc2set = true;
		/* Turn on B-Path */
		rtlpriv->cfg->ops->set_hw_reg(hw, HW_VAR_MRC, (u8 *)&mrc2set);
	}

	rtlpriv->cfg->ops->led_control(hw, LED_CTL_POWER_ON);
	rtl92s_dm_init(hw);

	/* give fw time to boot */
	msleep(500);

	return err;
}

void rtl92su_set_mac_addr(struct rtl_io *io, const u8 * addr)
{
}

void rtl92su_set_check_bssid(struct ieee80211_hw *hw, bool check_bssid)
{
}

static void _rtl92s_cmd_complete(struct urb *urb)
{
}

static int _rtl92su_set_media_status(struct ieee80211_hw *hw,
				     enum nl80211_iftype type)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	//u8 bt_msr = rtl_read_byte(rtlpriv, MSR);
	//u32 temp;
	struct sk_buff *skb;
	struct rtl_tcb_desc *tcb_desc;

	//bt_msr &= ~MSR_LINK_MASK;

	skb = dev_alloc_skb(RTL_TX_HEADER_SIZE + 8 + IEEE80211_MAX_FRAME_LEN);
	tcb_desc = (struct rtl_tcb_desc *)(skb->cb);
	tcb_desc->queue_index = RTL_TXQ_MGT;
	tcb_desc->cmd_or_init = DESC_PACKET_TYPE_NORMAL;
	tcb_desc->last_inipkt = 1;
	// _rtl92s_cmd_send_packet
	{
		struct rtl_usb *rtlusb = rtl_usbdev(rtl_usbpriv(hw));
		struct urb *urb;
		u8 *pdesc, *fwcmd_hdr, *fwcmd;
		u32 ep_num;

		pdesc = (u8 *)skb_push(skb, RTL_TX_HEADER_SIZE);

		// _rtl92s_fill_h2c_cmd
		fwcmd_hdr = (u8 *)skb_put(skb, 8);
		memset(fwcmd_hdr, 0, 8);
		fwcmd_hdr[0] = 0x08; // size
		fwcmd_hdr[1] = 0x00; // size
		fwcmd_hdr[2] = 0x11; // cmd code
		fwcmd_hdr[3] = rtlhal->h2c_txcmd_seq++; // seq#

		fwcmd = (u8 *)skb_put(skb, 8); // 4 rounded up to 8
		memset(fwcmd, 0, 8);
		fwcmd[0] = 0x02; // AutoUnknown

		rtlpriv->cfg->ops->fill_tx_cmddesc(hw, pdesc, 1, 1, skb);

		ep_num = rtlusb->ep_map.ep_mapping[tcb_desc->queue_index];

		urb = usb_alloc_urb(0, GFP_ATOMIC);
		if (!urb) {
			kfree_skb(skb);
			return 1;
		}
		usb_fill_bulk_urb(urb, rtlusb->udev, usb_sndbulkpipe(rtlusb->udev,
				  ep_num), skb->data, skb->len, _rtl92s_cmd_complete,
				  skb);
		urb->transfer_flags |= URB_ZERO_PACKET;

		usb_anchor_urb(urb, &rtlusb->tx_submitted);
		if (usb_submit_urb(urb, GFP_ATOMIC) < 0) {
			usb_unanchor_urb(urb);
			kfree_skb(skb);
			return 1;
		}
		usb_free_urb(urb);
	}

	switch (type) {
	case NL80211_IFTYPE_UNSPECIFIED:
		//bt_msr |= (MSR_LINK_NONE << MSR_LINK_SHIFT);
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			 "Set Network type to NO LINK!\n");
		break;
	case NL80211_IFTYPE_ADHOC:
		//bt_msr |= (MSR_LINK_ADHOC << MSR_LINK_SHIFT);
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			 "Set Network type to Ad Hoc!\n");
		break;
	case NL80211_IFTYPE_STATION:
		//bt_msr |= (MSR_LINK_MANAGED << MSR_LINK_SHIFT);
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			 "Set Network type to STA!\n");
		break;
	case NL80211_IFTYPE_AP:
		//bt_msr |= (MSR_LINK_MASTER << MSR_LINK_SHIFT);
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			 "Set Network type to AP!\n");
		break;
	default:
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "Network type %d not supported!\n", type);
		return 1;
		break;

	}

	//rtl_write_byte(rtlpriv, (MSR), bt_msr);

	//temp = rtl_read_dword(rtlpriv, TCR);
	//rtl_write_dword(rtlpriv, TCR, temp & (~BIT(8)));
	//rtl_write_dword(rtlpriv, TCR, temp | BIT(8));

	return 0;
}

/* HW_VAR_MEDIA_STATUS & HW_VAR_CECHK_BSSID */
int rtl92su_set_network_type(struct ieee80211_hw *hw, enum nl80211_iftype type)
{
	return 0;
}

/* don't set REG_EDCA_BE_PARAM here because mac80211 will send pkt when scan */
void rtl92su_set_qos(struct ieee80211_hw *hw, int aci)
{
}

void rtl92su_enable_interrupt(struct ieee80211_hw *hw)
{
}

void rtl92su_disable_interrupt(struct ieee80211_hw *hw)
{
}

static u8 _rtl92s_set_sysclk(struct ieee80211_hw *hw, u8 data)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 waitcnt = 100;
	bool result = false;
	u8 tmp;

	rtl_write_byte(rtlpriv, REG_SYS_CLKR + 1, data);

	/* Wait the MAC synchronized. */
	udelay(400);

	/* Check if it is set ready. */
	tmp = rtl_read_byte(rtlpriv, REG_SYS_CLKR + 1);
	result = ((tmp & BIT(7)) == (data & BIT(7)));

	if ((data & (BIT(6) | BIT(7))) == false) {
		waitcnt = 100;
		tmp = 0;

		while (1) {
			waitcnt--;
			tmp = rtl_read_byte(rtlpriv, REG_SYS_CLKR + 1);

			if ((tmp & BIT(6)))
				break;

			pr_err("wait for BIT(6) return value %x\n", tmp);

			if (waitcnt == 0)
				break;
			udelay(10);
		}

		if (waitcnt == 0)
			result = false;
		else
			result = true;
	}

	return result;
}

static void _rtl92s_phy_set_rfhalt(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	//struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	struct rtl_ps_ctl *ppsc = rtl_psc(rtl_priv(hw));
	u8 u1btmp;

	/* Power save for BB/RF */
	u1btmp = rtl_read_byte(rtlpriv, REG_LDOV12D_CTRL);
	u1btmp &= ~LDV12_EN;
	rtl_write_byte(rtlpriv, REG_LDOV12D_CTRL, u1btmp);
	rtl_write_byte(rtlpriv, REG_SPS1_CTRL, 0x0);
	rtl_write_byte(rtlpriv, REG_TXPAUSE, 0xFF);
	rtl_write_byte(rtlpriv, REG_RF_CTRL, 0x00);
/*
	rtl_write_word(rtlpriv, CMDR, 0x57FC);
	udelay(100);
	rtl_write_word(rtlpriv, CMDR, 0x77FC);
	udelay(10);
	rtl_write_word(rtlpriv, CMDR, 0x37FC);
	udelay(10);
	rtl_write_word(rtlpriv, CMDR, 0x77FC);
	udelay(10);
	rtl_write_word(rtlpriv, CMDR, 0x57FC);
	rtl_write_word(rtlpriv, CMDR, 0x0000);

	if (rtlhal->driver_going2unload) {
		u1btmp = rtl_read_byte(rtlpriv, (REG_SYS_FUNC_EN + 1));
		u1btmp &= ~(BIT(0));
		rtl_write_byte(rtlpriv, REG_SYS_FUNC_EN + 1, u1btmp);
	}
*/

	u1btmp = rtl_read_byte(rtlpriv, (REG_SYS_CLKR + 1));

	/* Add description. After switch control path. register
	 * after page1 will be invisible. We can not do any IO
	 * for register>0x40. After resume&MACIO reset, we need
	 * to remember previous reg content. */
	if (u1btmp & BIT(7)) {
		u1btmp &= ~(BIT(6) | BIT(7));
		if (!_rtl92s_set_sysclk(hw, u1btmp)) {
			pr_err("Switch ctrl path fail\n");
			return;
		}
	}

#if 0
	/* Power save for MAC */
	if (ppsc->rfoff_reason == RF_CHANGE_BY_IPS  &&
		!rtlhal->driver_going2unload) {
		/* enable LED function */
		rtl_write_byte(rtlpriv, 0x03, 0xF9);
	/* SW/HW radio off or halt adapter!! For example S3/S4 */
	} else {
		/* LED function disable. Power range is about 8mA now. */
		/* if write 0xF1 disconnet_pci power
		 *	 ifconfig wlan0 down power are both high 35:70 */
		/* if write oxF9 disconnet_pci power
		 * ifconfig wlan0 down power are both low  12:45*/
		rtl_write_byte(rtlpriv, 0x03, 0xF9);
	}
#endif
	rtl_write_byte(rtlpriv, REG_SYS_FUNC_EN + 1, 0x70);
	rtl_write_byte(rtlpriv, REG_PMC_FSM, 0x06);
	rtl_write_byte(rtlpriv, REG_SYS_ISO_CTRL, 0xf9);
	rtl_write_byte(rtlpriv, REG_SYS_ISO_CTRL + 1, 0xe8);

	//rtl_write_byte(rtlpriv, SYS_CLKR + 1, 0x70);
	//rtl_write_byte(rtlpriv, AFE_PLL_CTRL + 1, 0x68);
	rtl_write_byte(rtlpriv, REG_AFE_PLL_CTRL, 0x00);
	rtl_write_byte(rtlpriv, REG_LDOA15_CTRL, 0x54); // 0x34
	rtl_write_byte(rtlpriv, REG_SYS_FUNC_EN + 1, 0x50);
	//rtl_write_byte(rtlpriv, AFE_XTAL_CTRL, 0x0E);
	rtl_write_byte(rtlpriv, REG_AFE_MISC, 0x30);
	rtl_write_byte(rtlpriv, REG_SPS0_CTRL, 0x56);
	rtl_write_byte(rtlpriv, REG_SPS0_CTRL + 1, 0x43);
	RT_SET_PS_LEVEL(ppsc, RT_RF_OFF_LEVL_HALT_NIC);

}

static void _rtl92su_gen_refreshledstate(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_usb_priv *usbpriv = rtl_usbpriv(hw);
	struct rtl_led *pLed0 = &(usbpriv->ledctl.sw_led0);
		return;

	if (rtlpriv->psc.rfoff_reason == RF_CHANGE_BY_IPS)
		rtl92su_sw_led_on(hw, pLed0);
	else
		rtl92su_sw_led_off(hw, pLed0);
}

static void _rtl92su_power_domain_init(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u16 tmpu2b;
	u8 tmpu1b;

	rtlpriv->psc.pwrdomain_protect = true;

	tmpu1b = rtl_read_byte(rtlpriv, (REG_SYS_CLKR + 1));
	if (tmpu1b & BIT(7)) {
		tmpu1b &= ~(BIT(6) | BIT(7));
		if (!_rtl92s_set_sysclk(hw, tmpu1b)) {
			rtlpriv->psc.pwrdomain_protect = false;
			return;
		}
	}

	rtl_write_byte(rtlpriv, REG_AFE_PLL_CTRL, 0x0);
	rtl_write_byte(rtlpriv, REG_LDOA15_CTRL, 0x34);

	/* Reset MAC-IO and CPU and Core Digital BIT10/11/15 */
	tmpu1b = rtl_read_byte(rtlpriv, REG_SYS_FUNC_EN + 1);

	/* If IPS we need to turn LED on. So we not
	 * not disable BIT 3/7 of reg3. */
	if (rtlpriv->psc.rfoff_reason & (RF_CHANGE_BY_IPS | RF_CHANGE_BY_HW))
		tmpu1b &= 0xFB;
	else
		tmpu1b &= 0x73;

	rtl_write_byte(rtlpriv, REG_SYS_FUNC_EN + 1, tmpu1b);
	/* wait for BIT 10/11/15 to pull high automatically!! */
	mdelay(1);

	rtl_write_byte(rtlpriv, REG_SPS0_CTRL + 1, 0x53);
	rtl_write_byte(rtlpriv, REG_SPS0_CTRL + 0, 0x57);

	rtl_write_byte(rtlpriv, REG_CR, 0);
	rtl_write_byte(rtlpriv, REG_TCR, 0);

	/* Enable AFE clock source */
	tmpu1b = rtl_read_byte(rtlpriv, REG_AFE_XTAL_CTRL);
	rtl_write_byte(rtlpriv, REG_AFE_XTAL_CTRL, (tmpu1b | 0x01));
	/* Delay 1.5ms */
	mdelay(2);
	tmpu1b = rtl_read_byte(rtlpriv, REG_AFE_XTAL_CTRL + 1);
	rtl_write_byte(rtlpriv, REG_AFE_XTAL_CTRL + 1, (tmpu1b & 0xfb));

	/* Enable AFE Macro Block's Bandgap */
	tmpu1b = rtl_read_byte(rtlpriv, REG_AFE_MISC);
	rtl_write_byte(rtlpriv, REG_AFE_MISC, (tmpu1b | AFE_BGEN));
	mdelay(1);

	/* Enable AFE Mbias */
	tmpu1b = rtl_read_byte(rtlpriv, REG_AFE_MISC);
	rtl_write_byte(rtlpriv, REG_AFE_MISC, (tmpu1b | AFE_MBEN |
		       AFE_MISC_I32_EN));
	mdelay(1);

	/* Enable LDOA15 block */
	tmpu1b = rtl_read_byte(rtlpriv, REG_LDOA15_CTRL);
	rtl_write_byte(rtlpriv, REG_LDOA15_CTRL, (tmpu1b | LDA15_EN));

	/* Enable LDOV12D block */
	tmpu1b = rtl_read_byte(rtlpriv, REG_LDOV12D_CTRL);
	rtl_write_byte(rtlpriv, REG_LDOV12D_CTRL, (tmpu1b | LDV12_EN));

	/* Set Digital Vdd to Retention isolation Path. */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_ISO_CTRL);
	rtl_write_word(rtlpriv, REG_SYS_ISO_CTRL, (tmpu2b | ISO_PWC_DV2RP));

	/* For warm reboot NIC disappera bug. */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_FUNC_EN);
	rtl_write_word(rtlpriv, REG_SYS_FUNC_EN, (tmpu2b | BIT(13)));

	/* Support 64k IMEM */
	tmpu1b = rtl_read_byte(rtlpriv, REG_SYS_ISO_CTRL + 1);
	rtl_write_byte(rtlpriv, REG_SYS_ISO_CTRL + 1, (tmpu1b & 0x68));

	/* Enable AFE PLL Macro Block */
	tmpu1b = rtl_read_byte(rtlpriv, REG_AFE_PLL_CTRL);
	rtl_write_byte(rtlpriv, REG_AFE_PLL_CTRL, (tmpu1b | BIT(0) | BIT(4)));
	/* Enable MAC 80MHZ clock */
	tmpu1b = rtl_read_byte(rtlpriv, REG_AFE_PLL_CTRL + 1);
	rtl_write_byte(rtlpriv, REG_AFE_PLL_CTRL + 1, (tmpu1b | BIT(0)));
	mdelay(1);

	/* Release isolation AFE PLL & MD */
	rtl_write_byte(rtlpriv, REG_SYS_ISO_CTRL, 0xA6);

	/* Enable MAC clock */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_CLKR);
	rtl_write_word(rtlpriv, REG_SYS_CLKR, (tmpu2b | BIT(12) | BIT(11)));

	/* Enable Core digital and enable IOREG R/W */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_FUNC_EN);
	rtl_write_word(rtlpriv, REG_SYS_FUNC_EN, (tmpu2b | BIT(11)));
	/* enable REG_EN */
	rtl_write_word(rtlpriv, REG_SYS_FUNC_EN, (tmpu2b | BIT(11) | BIT(15)));

	/* Switch the control path. */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_CLKR);
	rtl_write_word(rtlpriv, REG_SYS_CLKR, (tmpu2b & (~BIT(2))));

	tmpu1b = rtl_read_byte(rtlpriv, (REG_SYS_CLKR + 1));
	tmpu1b = ((tmpu1b | BIT(7)) & (~BIT(6)));
	if (!_rtl92s_set_sysclk(hw, tmpu1b)) {
		rtlpriv->psc.pwrdomain_protect = false;
		return;
	}

	rtl_write_word(rtlpriv, REG_CR, 0x37FC);

	/* After MACIO reset,we must refresh LED state. */
	_rtl92su_gen_refreshledstate(hw);

	rtlpriv->psc.pwrdomain_protect = false;
}

void rtl92su_card_disable(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	enum nl80211_iftype opmode;
	u8 wait = 30;

	/* we should change GPIO to input mode
	 * this will drop away current about 25mA*/
	rtl8192su_gpiobit4_cfg_inputmode(hw);

	/* this is very important for ips power save */
	while (wait-- >= 10 && rtlpriv->psc.pwrdomain_protect) {
		if (rtlpriv->psc.pwrdomain_protect)
			mdelay(20);
		else
			break;
	}

	/* notify mac80211 to stop any work related to scanning */
	//ieee80211_scan_completed(hw, true);

	mac->link_state = MAC80211_NOLINK;
	opmode = NL80211_IFTYPE_UNSPECIFIED;
	_rtl92su_set_media_status(hw, opmode);

	_rtl92s_phy_set_rfhalt(hw);
	udelay(100);
}

void rtl92su_interrupt_recognized(struct ieee80211_hw *hw, u32 *p_inta,
			     u32 *p_intb)
{
}

void rtl92su_set_beacon_related_registers(struct ieee80211_hw *hw)
{
}

void rtl92su_set_beacon_interval(struct ieee80211_hw *hw)
{
}

static void _rtl8192se_get_IC_Inferiority(struct ieee80211_hw *hw)
{
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	u8 efuse_id;

	rtlhal->ic_class = IC_INFERIORITY_A;

	/* Only retrieving while using EFUSE. */
	if ((rtlefuse->epromtype == EEPROM_BOOT_EFUSE) &&
		!rtlefuse->autoload_failflag) {
		efuse_id = efuse_read_1byte(hw, EFUSE_IC_ID_OFFSET);

		if (efuse_id == 0xfe)
			rtlhal->ic_class = IC_INFERIORITY_B;
	}
}

static void _rtl92su_read_adapter_info(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	struct rtl_phy *rtlphy = &(rtlpriv->phy);
	u16 i, usvalue;
	u16	eeprom_id;
	u8 tempval;
	u8 hwinfo[HWSET_MAX_SIZE_92S];
	u8 rf_path, index;

	if (rtlefuse->epromtype == EEPROM_93C46) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "RTL819X Not boot from eeprom, check it !!\n");
	} else if (rtlefuse->epromtype == EEPROM_BOOT_EFUSE) {
		rtl_efuse_shadow_map_update(hw);

		memcpy((void *)hwinfo, (void *)
			&rtlefuse->efuse_map[EFUSE_INIT_MAP][0],
			HWSET_MAX_SIZE_92S);
	}

	RT_PRINT_DATA(rtlpriv, COMP_INIT, DBG_DMESG, "MAP",
		      hwinfo, HWSET_MAX_SIZE_92S);

	eeprom_id = *((u16 *)&hwinfo[0]);
	if (eeprom_id != RTL8190_EEPROM_ID) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
			 "EEPROM ID(%#x) is invalid!!\n", eeprom_id);
		rtlefuse->autoload_failflag = true;
	} else {
		RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD, "Autoload OK\n");
		rtlefuse->autoload_failflag = false;
	}

	if (rtlefuse->autoload_failflag)
		return;

	_rtl8192se_get_IC_Inferiority(hw);

	/* Read IC Version && Channel Plan */
	/* VID, DID	 SE	0xA-D */
	rtlefuse->eeprom_vid = *(u16 *)&hwinfo[EEPROM_VID];
	rtlefuse->eeprom_did = *(u16 *)&hwinfo[EEPROM_DID];
//	rtlefuse->eeprom_svid = *(u16 *)&hwinfo[EEPROM_SVID];
//	rtlefuse->eeprom_smid = *(u16 *)&hwinfo[EEPROM_SMID];
	rtlefuse->eeprom_version = *(u16 *)&hwinfo[EEPROM_VERSION];

	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "EEPROMId = 0x%4x\n", eeprom_id);
	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "EEPROM VID = 0x%4x\n", rtlefuse->eeprom_vid);
	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "EEPROM DID = 0x%4x\n", rtlefuse->eeprom_did);
//	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
//		 "EEPROM SVID = 0x%4x\n", rtlefuse->eeprom_svid);
//	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
//		 "EEPROM SMID = 0x%4x\n", rtlefuse->eeprom_smid);

	for (i = 0; i < 6; i += 2) {
		usvalue = *(u16 *)&hwinfo[EEPROM_MAC_ADDR + i];
		*((u16 *) (&rtlefuse->dev_addr[i])) = usvalue;
	}

	for (i = 0; i < 6; i++)
		rtl_write_byte(rtlpriv, REG_MACID + i, rtlefuse->dev_addr[i]);

	RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG, "%pM\n", rtlefuse->dev_addr);

	/* Get Tx Power Level by Channel */
	/* Read Tx power of Channel 1 ~ 14 from EEPROM. */
	/* 92S suupport RF A & B */
	for (rf_path = 0; rf_path < 2; rf_path++) {
		for (i = 0; i < 3; i++) {
			/* Read CCK RF A & B Tx power  */
			rtlefuse->eeprom_chnlarea_txpwr_cck[rf_path][i] =
			hwinfo[EEPROM_TXPOWERBASE + rf_path * 3 + i];

			/* Read OFDM RF A & B Tx power for 1T */
			rtlefuse->eeprom_chnlarea_txpwr_ht40_1s[rf_path][i] =
			hwinfo[EEPROM_TXPOWERBASE + 6 + rf_path * 3 + i];

			/* Read OFDM RF A & B Tx power for 2T */
			rtlefuse->eeprom_chnlarea_txpwr_ht40_2sdiif[rf_path][i]
				 = hwinfo[EEPROM_TXPOWERBASE + 12 +
				   rf_path * 3 + i];
		}
	}

	for (rf_path = 0; rf_path < 2; rf_path++)
		for (i = 0; i < 3; i++)
			RTPRINT(rtlpriv, FINIT, INIT_EEPROM,
				"RF(%d) EEPROM CCK Area(%d) = 0x%x\n",
				rf_path, i,
				rtlefuse->eeprom_chnlarea_txpwr_cck
				[rf_path][i]);
	for (rf_path = 0; rf_path < 2; rf_path++)
		for (i = 0; i < 3; i++)
			RTPRINT(rtlpriv, FINIT, INIT_EEPROM,
				"RF(%d) EEPROM HT40 1S Area(%d) = 0x%x\n",
				rf_path, i,
				rtlefuse->eeprom_chnlarea_txpwr_ht40_1s
				[rf_path][i]);
	for (rf_path = 0; rf_path < 2; rf_path++)
		for (i = 0; i < 3; i++)
			RTPRINT(rtlpriv, FINIT, INIT_EEPROM,
				"RF(%d) EEPROM HT40 2S Diff Area(%d) = 0x%x\n",
				rf_path, i,
				rtlefuse->eeprom_chnlarea_txpwr_ht40_2sdiif
				[rf_path][i]);

	for (rf_path = 0; rf_path < 2; rf_path++) {

		/* Assign dedicated channel tx power */
		for (i = 0; i < 14; i++)	{
			/* channel 1~3 use the same Tx Power Level. */
			if (i < 3)
				index = 0;
			/* Channel 4-8 */
			else if (i < 8)
				index = 1;
			/* Channel 9-14 */
			else
				index = 2;

			/* Record A & B CCK /OFDM - 1T/2T Channel area
			 * tx power */
			rtlefuse->txpwrlevel_cck[rf_path][i]  =
				rtlefuse->eeprom_chnlarea_txpwr_cck
							[rf_path][index];
			rtlefuse->txpwrlevel_ht40_1s[rf_path][i]  =
				rtlefuse->eeprom_chnlarea_txpwr_ht40_1s
							[rf_path][index];
			rtlefuse->txpwrlevel_ht40_2s[rf_path][i]  =
				rtlefuse->eeprom_chnlarea_txpwr_ht40_2sdiif
							[rf_path][index];
		}

		for (i = 0; i < 14; i++) {
			RTPRINT(rtlpriv, FINIT, INIT_TxPower,
				"RF(%d)-Ch(%d) [CCK / HT40_1S / HT40_2S] = [0x%x / 0x%x / 0x%x]\n",
				rf_path, i,
				rtlefuse->txpwrlevel_cck[rf_path][i],
				rtlefuse->txpwrlevel_ht40_1s[rf_path][i],
				rtlefuse->txpwrlevel_ht40_2s[rf_path][i]);
		}
	}

	for (rf_path = 0; rf_path < 2; rf_path++) {
		for (i = 0; i < 3; i++) {
			/* Read Power diff limit. */
			rtlefuse->eeprom_pwrgroup[rf_path][i] =
				hwinfo[EEPROM_TXPWRGROUP + rf_path * 3 + i];
		}
	}

	for (rf_path = 0; rf_path < 2; rf_path++) {
		/* Fill Pwr group */
		for (i = 0; i < 14; i++) {
			/* Chanel 1-3 */
			if (i < 3)
				index = 0;
			/* Channel 4-8 */
			else if (i < 8)
				index = 1;
			/* Channel 9-13 */
			else
				index = 2;

			rtlefuse->pwrgroup_ht20[rf_path][i] =
				(rtlefuse->eeprom_pwrgroup[rf_path][index] &
				0xf);
			rtlefuse->pwrgroup_ht40[rf_path][i] =
				((rtlefuse->eeprom_pwrgroup[rf_path][index] &
				0xf0) >> 4);

			RTPRINT(rtlpriv, FINIT, INIT_TxPower,
				"RF-%d pwrgroup_ht20[%d] = 0x%x\n",
				rf_path, i,
				rtlefuse->pwrgroup_ht20[rf_path][i]);
			RTPRINT(rtlpriv, FINIT, INIT_TxPower,
				"RF-%d pwrgroup_ht40[%d] = 0x%x\n",
				rf_path, i,
				rtlefuse->pwrgroup_ht40[rf_path][i]);
			}
	}

	for (i = 0; i < 14; i++) {
		/* Read tx power difference between HT OFDM 20/40 MHZ */
		/* channel 1-3 */
		if (i < 3)
			index = 0;
		/* Channel 4-8 */
		else if (i < 8)
			index = 1;
		/* Channel 9-14 */
		else
			index = 2;

		tempval = (*(u8 *)&hwinfo[EEPROM_TX_PWR_HT20_DIFF +
			   index]) & 0xff;
		rtlefuse->txpwr_ht20diff[RF90_PATH_A][i] = (tempval & 0xF);
		rtlefuse->txpwr_ht20diff[RF90_PATH_B][i] =
						 ((tempval >> 4) & 0xF);

		/* Read OFDM<->HT tx power diff */
		/* Channel 1-3 */
		if (i < 3)
			index = 0;
		/* Channel 4-8 */
		else if (i < 8)
			index = 0x11;
		/* Channel 9-14 */
		else
			index = 1;

		tempval = (*(u8 *)&hwinfo[EEPROM_TX_PWR_OFDM_DIFF + index])
				  & 0xff;
		rtlefuse->txpwr_legacyhtdiff[RF90_PATH_A][i] =
				 (tempval & 0xF);
		rtlefuse->txpwr_legacyhtdiff[RF90_PATH_B][i] =
				 ((tempval >> 4) & 0xF);

//		tempval = (*(u8 *)&hwinfo[TX_PWR_SAFETY_CHK]);
//		rtlefuse->txpwr_safetyflag = (tempval & 0x01);
	}

	rtlefuse->eeprom_regulatory = 0;
	if (rtlefuse->eeprom_version >= 2) {
		/* BIT(0)~2 */
		if (rtlefuse->eeprom_version >= 4)
			rtlefuse->eeprom_regulatory =
				 (hwinfo[EEPROM_REGULATORY] & 0x7);
		else /* BIT(0) */
			rtlefuse->eeprom_regulatory =
				 (hwinfo[EEPROM_REGULATORY] & 0x1);
	}
	RTPRINT(rtlpriv, FINIT, INIT_TxPower,
		"eeprom_regulatory = 0x%x\n", rtlefuse->eeprom_regulatory);

	for (i = 0; i < 14; i++)
		RTPRINT(rtlpriv, FINIT, INIT_TxPower,
			"RF-A Ht20 to HT40 Diff[%d] = 0x%x\n",
			i, rtlefuse->txpwr_ht20diff[RF90_PATH_A][i]);
	for (i = 0; i < 14; i++)
		RTPRINT(rtlpriv, FINIT, INIT_TxPower,
			"RF-A Legacy to Ht40 Diff[%d] = 0x%x\n",
			i, rtlefuse->txpwr_legacyhtdiff[RF90_PATH_A][i]);
	for (i = 0; i < 14; i++)
		RTPRINT(rtlpriv, FINIT, INIT_TxPower,
			"RF-B Ht20 to HT40 Diff[%d] = 0x%x\n",
			i, rtlefuse->txpwr_ht20diff[RF90_PATH_B][i]);
	for (i = 0; i < 14; i++)
		RTPRINT(rtlpriv, FINIT, INIT_TxPower,
			"RF-B Legacy to HT40 Diff[%d] = 0x%x\n",
			i, rtlefuse->txpwr_legacyhtdiff[RF90_PATH_B][i]);

//	RTPRINT(rtlpriv, FINIT, INIT_TxPower,
//		"TxPwrSafetyFlag = %d\n", rtlefuse->txpwr_safetyflag);

	/* Read RF-indication and Tx Power gain
	 * index diff of legacy to HT OFDM rate. */
	tempval = (*(u8 *)&hwinfo[EEPROM_RFIND_POWERDIFF]) & 0xff;
	rtlefuse->eeprom_txpowerdiff = tempval;
	rtlefuse->legacy_httxpowerdiff =
		rtlefuse->txpwr_legacyhtdiff[RF90_PATH_A][0];

	RTPRINT(rtlpriv, FINIT, INIT_TxPower,
		"TxPowerDiff = %#x\n", rtlefuse->eeprom_txpowerdiff);

	/* Get TSSI value for each path. */
	usvalue = *(u16 *)&hwinfo[EEPROM_TSSI_A];
	rtlefuse->eeprom_tssi[RF90_PATH_A] = (u8)((usvalue & 0xff00) >> 8);
	usvalue = *(u8 *)&hwinfo[EEPROM_TSSI_B];
	rtlefuse->eeprom_tssi[RF90_PATH_B] = (u8)(usvalue & 0xff);

	RTPRINT(rtlpriv, FINIT, INIT_TxPower, "TSSI_A = 0x%x, TSSI_B = 0x%x\n",
		rtlefuse->eeprom_tssi[RF90_PATH_A],
		rtlefuse->eeprom_tssi[RF90_PATH_B]);

	/* Read antenna tx power offset of B/C/D to A  from EEPROM */
	/* and read ThermalMeter from EEPROM */
	tempval = *(u8 *)&hwinfo[EEPROM_THERMALMETER];
	rtlefuse->eeprom_thermalmeter = tempval;
	RTPRINT(rtlpriv, FINIT, INIT_TxPower,
		"thermalmeter = 0x%x\n", rtlefuse->eeprom_thermalmeter);

	/* ThermalMeter, BIT(0)~3 for RFIC1, BIT(4)~7 for RFIC2 */
	rtlefuse->thermalmeter[0] = (rtlefuse->eeprom_thermalmeter & 0x1f);
	rtlefuse->tssi_13dbm = rtlefuse->eeprom_thermalmeter * 100;

	/* Read CrystalCap from EEPROM */
	tempval = (*(u8 *)&hwinfo[EEPROM_CRYSTALCAP]) >> 4;
	rtlefuse->eeprom_crystalcap = tempval;
	/* CrystalCap, BIT(12)~15 */
	rtlefuse->crystalcap = rtlefuse->eeprom_crystalcap;

	/* Read IC Version && Channel Plan */
	/* Version ID, Channel plan */
	rtlefuse->eeprom_channelplan = *(u8 *)&hwinfo[EEPROM_CHANNELPLAN];
	rtlefuse->txpwr_fromeprom = true;
	RTPRINT(rtlpriv, FINIT, INIT_TxPower,
		"EEPROM ChannelPlan = 0x%4x\n", rtlefuse->eeprom_channelplan);

	/* Read Customer ID or Board Type!!! */
	//tempval = *(u8 *)&hwinfo[EEPROM_BOARDTYPE];
	/* Change RF type definition */
/*
	if (tempval == 0)
		rtlphy->rf_type = RF_2T2R;
	else if (tempval == 1)
		rtlphy->rf_type = RF_1T2R;
	else if (tempval == 2)
		rtlphy->rf_type = RF_1T2R;
	else if (tempval == 3)
		rtlphy->rf_type = RF_1T1R;
*/
	// XXX
	rtlphy->rf_type = RF_1T2R;

	/* 1T2R but 1SS (1x1 receive combining) */
	rtlefuse->b1x1_recvcombine = false;
	if (rtlphy->rf_type == RF_1T2R) {
		tempval = rtl_read_byte(rtlpriv, 0x07);
		if (!(tempval & BIT(0))) {
			rtlefuse->b1x1_recvcombine = true;
			RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
				 "RF_TYPE=1T2R but only 1SS\n");
		}
	}
	rtlefuse->b1ss_support = rtlefuse->b1x1_recvcombine;
	rtlefuse->eeprom_oemid = *(u8 *)&hwinfo[EEPROM_CUSTOMID];

	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD, "EEPROM Customer ID: 0x%2x",
		 rtlefuse->eeprom_oemid);

	/* set channel paln to world wide 13 */
	rtlefuse->channel_plan = COUNTRY_CODE_WORLD_WIDE_13;
}

void rtl92su_read_eeprom_info(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	u8 tmp_u1b = 0;

	tmp_u1b = rtl_read_byte(rtlpriv, REG_EPROM_CMD);

	if (tmp_u1b & BIT(4)) {
		RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG, "Boot from EEPROM\n");
		rtlefuse->epromtype = EEPROM_93C46;
	} else {
		RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG, "Boot from EFUSE\n");
		rtlefuse->epromtype = EEPROM_BOOT_EFUSE;
	}

	if (tmp_u1b & BIT(5)) {
		RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD, "Autoload OK\n");
		rtlefuse->autoload_failflag = false;
		_rtl92su_read_adapter_info(hw);
	} else {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG, "Autoload ERR!!\n");
		rtlefuse->autoload_failflag = true;
	}
}

void rtl92su_update_hal_rate_tbl(struct ieee80211_hw *hw,
		struct ieee80211_sta *sta, u8 rssi_level)
{
}

void rtl92su_update_channel_access_setting(struct ieee80211_hw *hw)
{
}


bool rtl92su_gpio_radio_on_off_checking(struct ieee80211_hw *hw, u8 *valid)
{
	*valid = 1;
	return true;
}

/* Is_wepkey just used for WEP used as group & pairwise key
 * if pairwise is AES ang group is WEP Is_wepkey == false.*/
void rtl92su_set_key(struct ieee80211_hw *hw, u32 key_index, u8 *p_macaddr,
	bool is_group, u8 enc_algo, bool is_wepkey, bool clear_all)
{
}
