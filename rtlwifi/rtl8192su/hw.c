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
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	struct rtl_ps_ctl *ppsc = rtl_psc(rtl_priv(hw));

	printk(KERN_ERR "UPDATE %d\n", variable);

	switch (variable) {
	case HW_VAR_ETHER_ADDR:
		rtl92su_set_mac_addr(hw, val);
		break;
	case HW_VAR_BASIC_RATE: {
			// use H2C_SETBASICRATE_CMD
			break;
		}
	case HW_VAR_BSSID: {
			// use H2C_CREATEBSS_CMD or H2C_JOINBSS_CMD
			break;
		}
	case HW_VAR_SIFS: {
			// Not supported
			break;
		}
	case HW_VAR_SLOT_TIME: {
			// Not supported
			break;
		}
	case HW_VAR_ACK_PREAMBLE: {
			// Not supported
			break;
		}
	case HW_VAR_AMPDU_MIN_SPACE: {
			// Not supported
			break;
		}
	case HW_VAR_SHORTGI_DENSITY: {
			// Not supported
			break;
		}
	case HW_VAR_AMPDU_FACTOR: {
			// Not supported
			break;
		}
	case HW_VAR_AC_PARAM: {
			// Not supported
			break;
		}
	case HW_VAR_ACM_CTRL: {
			// Not supported
			break;
		}
	case HW_VAR_RCR:
		/* Note: The FW might have a word in the filtering as well */
		rtl_write_dword(rtlpriv, REG_RCR, ((u32 *) (val))[0]);
		mac->rx_conf = ((u32 *) (val))[0];
		break;

	case HW_VAR_RETRY_LIMIT: {
			// Not supported
			break;
		}
	case HW_VAR_DUAL_TSF_RST: {
			// Not supported
			break;
		}
	case HW_VAR_EFUSE_BYTES:
		rtlefuse->efuse_usedbytes = *((u16 *) val);
		break;

	case HW_VAR_EFUSE_USAGE:
		rtlefuse->efuse_usedpercentage = *((u8 *) val);
		break;

	case HW_VAR_IO_CMD: {
			break;
		}
	case HW_VAR_WPA_CONFIG: {
			break;
		}
	case HW_VAR_SET_RPWM:
		rtl_write_byte(rtlpriv, REG_USB_HRPWM, *((u8 *) val));
		break;
	case HW_VAR_H2C_FW_PWRMODE:
		break;
	case HW_VAR_FW_PSMODE_STATUS:
		ppsc->fw_current_inpsmode = *((bool *) val);
		break;
	case HW_VAR_H2C_FW_JOINBSSRPT:
		rtl92s_set_fw_joinbss_report_cmd(hw, *((u8 *) val), 0);
		break;
	case HW_VAR_AID:{
			break;
		}
	case HW_VAR_CORRECT_TSF:{
		break;
		}
	case HW_VAR_MRC: {
		bool bmrc_toset = *((bool *)val);

		if (bmrc_toset) {
			/* Update current settings. */
			rtlpriv->dm.current_mrc_switch = bmrc_toset;
		} else {
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

	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "PairwiseEncAlgorithm = %d GroupEncAlgorithm = %d\n",
		 rtlpriv->sec.pairwise_enc_algorithm,
		 rtlpriv->sec.group_enc_algorithm);

	if (rtlpriv->cfg->mod_params->sw_crypto || rtlpriv->sec.use_sw_sec) {
		RT_TRACE(rtlpriv, COMP_SEC, DBG_DMESG,
			 "not open hw encryption\n");
		return;
	}

	// use set (sta) key

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

static int rtl8192su_gpiobit4_cfg_inputmode(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 u1tmp;

	/* The following config GPIO function */
	rtl_write_byte(rtlpriv, REG_MAC_PINMUX_CTRL, (GPIOMUX_EN | GPIOSEL_GPIO));
	u1tmp = rtl_read_byte(rtlpriv, REG_GPIO_IO_SEL);

	/* config GPIO4 to input */
	u1tmp &= ~HAL_8192S_HW_GPIO_WPS_BIT;
	rtl_write_byte(rtlpriv, REG_GPIO_IO_SEL, u1tmp);
	return 0;
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

static int _rtl92su_init_2nd_3rd_cut(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	unsigned int pollingcnt = 20;
	u8 tmpu1b;
	u16 tmpu2b;

	/* Prevent EFUSE leakage */
	rtl_write_byte(rtlpriv, REG_EFUSE_TEST + 3, 0xb0);
	msleep(20);
	rtl_write_byte(rtlpriv, REG_EFUSE_TEST + 3, 0x30);

	/*
	 * Set control path switch to HW control and reset digital core,
	 * CPU core and MAC I/O core.
	 */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_CLKR);
	if (tmpu2b & SYS_FWHW_SEL) {
		tmpu2b &= ~(SYS_SWHW_SEL | SYS_FWHW_SEL);

		/* Set failed, return to prevent hang. */
		if (!_rtl92su_halset_sysclk(hw, tmpu2b))
			return -EIO;
	}

	/* Reset MAC-IO and CPU and Core Digital BIT(10)/11/15 */
	tmpu1b = rtl_read_byte(rtlpriv, REG_SYS_FUNC_EN + 1);
	tmpu1b &= 0x73;
	rtl_write_byte(rtlpriv, REG_SYS_FUNC_EN + 1, tmpu1b);
	/* wait for BIT 10/11/15 to pull high automatically!! */
	mdelay(1);

	rtl_write_byte(rtlpriv, REG_SPS0_CTRL + 1, 0x53);
	rtl_write_byte(rtlpriv, REG_SPS0_CTRL, 0x57);

	/* Enable AFE Macro Block's Bandgap */
	tmpu1b = rtl_read_byte(rtlpriv, REG_AFE_MISC);
	rtl_write_byte(rtlpriv, REG_AFE_MISC, (tmpu1b | AFE_BGEN));
	mdelay(1);

	/* Enable AFE Mbias */
	tmpu1b = rtl_read_byte(rtlpriv, REG_AFE_MISC);
	rtl_write_byte(rtlpriv, REG_AFE_MISC, (tmpu1b | AFE_BGEN |
		       AFE_MBEN | AFE_MISC_I32_EN));
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
	/* Also known as: Engineer Packet CP test Enable */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_FUNC_EN);
	rtl_write_word(rtlpriv, REG_SYS_FUNC_EN, (tmpu2b | BIT(13)));

	/* Support 64k IMEM */
	tmpu1b = rtl_read_byte(rtlpriv, REG_SYS_ISO_CTRL + 1);
	rtl_write_byte(rtlpriv, REG_SYS_ISO_CTRL + 1, (tmpu1b & 0x68));

	/* Enable AFE clock source */
	tmpu1b = rtl_read_byte(rtlpriv, REG_AFE_XTAL_CTRL);
	rtl_write_byte(rtlpriv, REG_AFE_XTAL_CTRL, (tmpu1b | 0x01));
	/* Delay 1.5ms */
	mdelay(2);
	tmpu1b = rtl_read_byte(rtlpriv, REG_AFE_XTAL_CTRL + 1);
	rtl_write_byte(rtlpriv, REG_AFE_XTAL_CTRL + 1, (tmpu1b & 0xfb));

	/* Enable AFE PLL Macro Block */
	/* We need to delay 100u before enabling PLL. */
	udelay(200);
	tmpu1b = rtl_read_byte(rtlpriv, REG_AFE_PLL_CTRL);
	rtl_write_byte(rtlpriv, REG_AFE_PLL_CTRL, (tmpu1b | BIT(0) | BIT(4)));

	/* for divider reset
	 * The clock will be stable with 500us delay after the PLL reset
	 */
	udelay(500);
	rtl_write_byte(rtlpriv, REG_AFE_PLL_CTRL, (tmpu1b | BIT(0) |
		       BIT(4) | BIT(6)));
	udelay(500);
	rtl_write_byte(rtlpriv, REG_AFE_PLL_CTRL, (tmpu1b | BIT(0) | BIT(4)));
	udelay(500);

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
	tmpu2b |= BIT(12) | BIT(11);
	if (!_rtl92su_halset_sysclk(hw, tmpu2b))
		return -EIO;

	rtl_write_byte(rtlpriv, REG_PMC_FSM, 0x02);

	/* Enable Core digital and enable IOREG R/W */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_FUNC_EN);
	rtl_write_word(rtlpriv, REG_SYS_FUNC_EN, (tmpu2b | BIT(11)));

	/* enable REG_EN */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_FUNC_EN);
	rtl_write_word(rtlpriv, REG_SYS_FUNC_EN, (tmpu2b | BIT(15)));

	/* Switch the control path to FW */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_CLKR);
	tmpu2b |= SYS_FWHW_SEL;
	tmpu2b &= ~SYS_SWHW_SEL;
	if (!_rtl92su_halset_sysclk(hw, tmpu2b))
		return -EIO;

	rtl_write_word(rtlpriv, REG_CR, HCI_TXDMA_EN |
		HCI_RXDMA_EN | TXDMA_EN | RXDMA_EN | FW2HW_EN |
		DDMA_EN | MACTXEN | MACRXEN | SCHEDULE_EN |
		BB_GLB_RSTN | BBRSTN);

	/* Fix USB RX FIFO error */
	tmpu1b = rtl_read_byte(rtlpriv, REG_USB_AGG_TO);
	rtl_write_byte(rtlpriv, REG_USB_AGG_TO, tmpu1b | BIT(7));

	/* Enable MAC clock */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_CLKR);
	tmpu2b &= ~SYS_CPU_CLKSEL;
	if (!_rtl92su_halset_sysclk(hw, tmpu2b))
		return -EIO;

	/* Fix 8051 ROM incorrect code operation */
	rtl_write_byte(rtlpriv, REG_USB_MAGIC, USB_MAGIC_BIT7);

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

	return 0;
}

static int _rtl92su_macconfig_before_fwdownload(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_ps_ctl *ppsc = rtl_psc(rtl_priv(hw));
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	int err = 0;

	/* Clear RPWM to ensure driver and fw are back in the initial state */
	rtl_write_byte(rtlpriv, REG_USB_HRPWM, 0x00);

	switch (rtlhal->version) {
	case 2:
	case 3:
		err = _rtl92su_init_2nd_3rd_cut(hw);
		break;
	default:
		err = -EOPNOTSUPP;
		break;
	}

	if (err) {
		return err;
	}

	/* After MACIO reset,we must refresh LED state. */
	if ((ppsc->rfoff_reason == RF_CHANGE_BY_IPS) ||
	   (ppsc->rfoff_reason == 0)) {
		struct rtl_usb_priv *usbpriv = rtl_usbpriv(hw);
		struct rtl_led *pLed0 = &(usbpriv->ledctl.sw_led0);
		rtl92su_sw_led_on(hw, pLed0);
	}

	return 0;
}

static int _rtl92su_macconfig_after_fwdownload(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	u32 tmpu4b;
	u8 tmpu1b;

	rtl_write_dword(rtlpriv, REG_RCR, mac->rx_conf);

	tmpu4b = rtl_read_word(rtlpriv, REG_CR);
	tmpu4b &= ~(BIT(31) | BIT(30) | BIT(29) | BIT(28) | BIT(27) |
		    BIT(26) | BIT(25) | BIT(24));
	rtl_write_dword(rtlpriv, REG_CR, tmpu4b);

	/* Setting TX/RX page size 0/1/2/3/4=128/256/512/1024/2048 */
	tmpu1b = rtl_read_byte(rtlpriv, REG_PBP);
	tmpu1b |= PBP_PAGE_128B;
	rtl_write_byte(rtlpriv, REG_PBP, tmpu1b);

	tmpu1b = rtl_read_byte(rtlpriv, REG_RXDMA_RXCTRL);
	tmpu1b &= ~RXDMA_AGG_EN;
	rtl_write_byte(rtlpriv, REG_RXDMA_RXCTRL, tmpu1b);

	/* Just one page => no aggregation */
	rtl_write_byte(rtlpriv, REG_RXDMA_AGG_PG_TH, 1);

	/* 1.7 ms / "0x04" */
	rtl_write_byte(rtlpriv, REG_USB_DMA_AGG_TO, 0x04);

	/* Fix the RX FIFO issue (USB Error) */
	tmpu1b = rtl_read_byte(rtlpriv, REG_USB_AGG_TO);
	tmpu1b |= BIT(7);
	rtl_write_byte(rtlpriv, REG_USB_AGG_TO, tmpu1b);

	rtl92su_set_network_type(hw, NL80211_IFTYPE_STATION);
	return 0;
}

static int _rtl92su_hw_configure(struct ieee80211_hw *hw)
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
	return 0;
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

	rtlhal->hw_type = HARDWARE_TYPE_RTL8192SU;
}

int rtl92su_hw_init(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	struct rtl_phy *rtlphy = &(rtlpriv->phy);
	int err;

	rtlhal->h2c_txcmd_seq = 0x01;

	/* 1. MAC Initialize */
	/* Before FW download, we have to set some MAC register */
	err = _rtl92su_macconfig_before_fwdownload(hw);
	if (err)
		return err;

	/* 2. download firmware */
	err = rtl92s_download_fw(hw);
	if (err) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
			 "Failed to download FW. Init HW without FW now... "
			 "Please copy FW into /lib/firmware/rtlwifi\n");
		return err;
	}

	/* After FW download, we have to reset MAC register */
	err = _rtl92su_macconfig_after_fwdownload(hw);
	if (err)
		return err;

	err = rtl8192su_gpiobit4_cfg_inputmode(hw);
	if (err)
		return err;

	/*Retrieve default FW Cmd IO map. */
	//rtlhal->fwcmd_iomap =	rtl_read_word(rtlpriv, LBUS_MON_ADDR);
	//rtlhal->fwcmd_ioparam = rtl_read_dword(rtlpriv, LBUS_ADDR_MASK);

	/* 3. Initialize MAC/PHY Config by MACPHY_reg.txt */
	err = rtl92s_phy_mac_config(hw);
	if (err) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "MAC Config failed (%d)\n", err);
		return err;
	}

	/* Make sure BB/RF write OK. We should prevent enter IPS. radio off. */
	/* We must set flag avoid BB/RF config period later!! */

	/* 4. Initialize BB After MAC Config PHY_reg.txt, AGC_Tab.txt */
	err = rtl92s_phy_bb_config(hw);
	if (err) {
		RT_TRACE(rtlpriv, COMP_INIT, DBG_EMERG,
			 "BB Config failed (%d)\n", err);
		return err;
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

	err = rtl92s_phy_rf_config(hw);
	if (err) {
		RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG,
			 "RF Config failed (%d)\n", err);
		return err;
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
	err = _rtl92su_hw_configure(hw);

	/* Read EEPROM TX power index and PHY_REG_PG.txt to capture correct */
	/* TX power index for different rate set. */
	/* Get original hw reg values */
	//rtl92s_phy_get_hw_reg_originalvalue(hw);
	/* Write correct tx power index */
	err = rtl92s_phy_set_txpower(hw, rtlphy->current_channel);

	rtlpriv->cfg->ops->led_control(hw, LED_CTL_POWER_ON);

	/* give fw time to boot */
	msleep(500);

	return err;
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
	int err = -EINVAL;

	switch (type) {
	case NL80211_IFTYPE_UNSPECIFIED:
		err = rtl92s_set_fw_opmode_cmd(hw, OP_ADHOC);
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			 "Set Network type to NO LINK!\n");
		break;

	case NL80211_IFTYPE_ADHOC:
		err = rtl92s_set_fw_opmode_cmd(hw, OP_ADHOC);
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			 "Set Network type to Ad Hoc!\n");
		break;

	case NL80211_IFTYPE_STATION:
		err = rtl92s_set_fw_opmode_cmd(hw, OP_INFRA);
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			 "Set Network type to STA!\n");
		break;

	case NL80211_IFTYPE_AP:
	case NL80211_IFTYPE_MONITOR:
	default:
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "Network type %d not supported!\n", type);
		err = -ENOTSUPP;
		break;

	}

	/* disable pesky frame filters */
	rtl_write_word(rtlpriv, REG_RXFLTMAP0, 0x0);
	rtl_write_word(rtlpriv, REG_RXFLTMAP1, 0x0);
	rtl_write_word(rtlpriv, REG_RXFLTMAP2, 0x0);

	return err;
}

/* HW_VAR_MEDIA_STATUS & HW_VAR_CECHK_BSSID */
int rtl92su_set_network_type(struct ieee80211_hw *hw, enum nl80211_iftype type)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	if (_rtl92su_set_media_status(hw, type))
		return -EOPNOTSUPP;

	if (rtlpriv->mac80211.link_state == MAC80211_LINKED) {
		if (type != NL80211_IFTYPE_AP)
			rtl92su_set_check_bssid(hw, true);
	} else {
		rtl92su_set_check_bssid(hw, false);
	}

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
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	struct rtl_ps_ctl *ppsc = rtl_psc(rtl_priv(hw));
	u8 u1btmp;

	/* Power save for BB/RF */
	u1btmp = rtl_read_byte(rtlpriv, REG_LDOV12D_CTRL);
	u1btmp &= ~LDV12_EN;

	/* Turn off BB */
	rtl_write_byte(rtlpriv, REG_RF_CTRL, 0x00);
	msleep(20);

	WARN_ON_ONCE(!_rtl92s_set_sysclk(hw, 0x38));

	rtl_write_byte(rtlpriv, REG_SYS_FUNC_EN + 1, 0x70);
	 /* Enable Loader Data Keep */
	rtl_write_byte(rtlpriv, REG_PMC_FSM, 0x06);
	/* Isolation signals from CORE, PLL */
	rtl_write_byte(rtlpriv, REG_SYS_ISO_CTRL, 0xf9);
	/* Enable EFUSE 1.2V */
	rtl_write_byte(rtlpriv, REG_SYS_ISO_CTRL + 1, 0xe8);
	/* Disable AFE PLL. */
	rtl_write_byte(rtlpriv, REG_AFE_PLL_CTRL, 0x00);
	/* Disable A15V */
	rtl_write_byte(rtlpriv, REG_LDOA15_CTRL, 0x54);
	/* Disable E-Fuse 1.2V */
	rtl_write_byte(rtlpriv, REG_SYS_FUNC_EN + 1, 0x50);
	/* Disable LDO12(for CE) */
	rtl_write_byte(rtlpriv, REG_LDOV12D_CTRL, 0x24);
	/* Disable AFE BG&MB */
	rtl_write_byte(rtlpriv, REG_AFE_MISC, 0x30);
	/* Disable 1.6V LDO */
	rtl_write_byte(rtlpriv, REG_SPS0_CTRL, 0x56);
	/* Set SW PFM */
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
