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
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Christian Lamparter <chunkeey@googlemail.com>
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/

#include "../wifi.h"
#include "../usb.h"
#include "../efuse.h"
#include "../base.h"
#include "../regd.h"
#include "../cam.h"
#include "../ps.h"
#include "../rtl8192s/reg_common.h"
#include "../rtl8192s/def_common.h"
#include "../rtl8192s/phy_common.h"
#include "../rtl8192s/dm_common.h"
#include "../rtl8192s/fw_common.h"
#include "../rtl8192s/hw_common.h"
#include "../rtl8192s/led_common.h"
#include "hw.h"
#include "trx.h"

/* mix between 8190n and r92su */
static int _rtl92su_macconfig_before_fwdownload(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_ps_ctl *ppsc = rtl_psc(rtlpriv);
	unsigned int tries = 20;
	u8 tmpu1b;
	u16 tmpu2b;

	rtl_write_byte(rtlpriv, REG_USB_HRPWM, 0x00);

	/* Prevent EFUSE leakage */
	rtl_write_byte(rtlpriv, REG_EFUSE_TEST + 3, 0xb0);
	msleep(20);
	rtl_write_byte(rtlpriv, REG_EFUSE_TEST + 3, 0x30);

	/* Set control path switch to HW control and reset digital core,
	 * CPU core and MAC I/O core. */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_CLKR);
	if (tmpu2b & SYS_FWHW_SEL) {
		tmpu2b &= ~(SYS_SWHW_SEL | SYS_FWHW_SEL);

		/* Set failed, return to prevent hang. */
		if (!rtl92s_halset_sysclk(hw, tmpu2b))
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
	tmpu1b |= AFE_BGEN /* | AFE_MBEN */;
	rtl_write_byte(rtlpriv, REG_AFE_MISC, tmpu1b);
	mdelay(1);

	/* Enable AFE Mbias */
	tmpu1b = rtl_read_byte(rtlpriv, REG_AFE_MISC);
	tmpu1b |= AFE_BGEN | AFE_MBEN;
	rtl_write_byte(rtlpriv, REG_AFE_MISC, tmpu1b);
	mdelay(1);

	/* Enable LDOA15 block	*/
	tmpu1b = rtl_read_byte(rtlpriv, REG_LDOA15_CTRL);
	tmpu1b |= LDA15_EN;
	rtl_write_byte(rtlpriv, REG_LDOA15_CTRL, tmpu1b);

	/* Enable LDOV12D block */
	tmpu1b = rtl_read_byte(rtlpriv, REG_LDOV12D_CTRL);
	tmpu1b |= LDV12_EN;
	rtl_write_byte(rtlpriv, REG_LDOV12D_CTRL, tmpu1b);

	/* Set Digital Vdd to Retention isolation Path. */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_ISO_CTRL);
	tmpu2b |= ISO_PWC_DV2RP;
	rtl_write_word(rtlpriv, REG_SYS_ISO_CTRL, tmpu2b);

	/* For warm reboot NIC disappear bug.
	 * Also known as: Engineer Packet CP test Enable */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_FUNC_EN);
	tmpu2b |= BIT(13);
	rtl_write_word(rtlpriv, REG_SYS_FUNC_EN, tmpu2b);

	/* Support 64k IMEM */
	tmpu1b = rtl_read_byte(rtlpriv, REG_SYS_ISO_CTRL + 1);
	tmpu1b &= 0x68;
	rtl_write_byte(rtlpriv, REG_SYS_ISO_CTRL + 1, tmpu1b);

	/* Enable AFE clock source */
	tmpu1b = rtl_read_byte(rtlpriv, REG_AFE_XTAL_CTRL);
	tmpu1b |= BIT(0);
	rtl_write_byte(rtlpriv, REG_AFE_XTAL_CTRL, tmpu1b);
	/* Delay 1.5ms */
	mdelay(2);

	tmpu1b = rtl_read_byte(rtlpriv, REG_AFE_XTAL_CTRL + 1);
	tmpu1b &= ~BIT(2);
	rtl_write_byte(rtlpriv, REG_AFE_XTAL_CTRL + 1, tmpu1b);

	/* Enable AFE PLL Macro Block *
	 * We need to delay 100u before enabling PLL. */
	udelay(200);
	tmpu1b = rtl_read_byte(rtlpriv, REG_AFE_PLL_CTRL);
	tmpu1b |= BIT(0) | BIT(4);
	rtl_write_byte(rtlpriv, REG_AFE_PLL_CTRL, tmpu1b);

	/* for divider reset
	 * The clock will be stable with 500us delay after the PLL reset */
	udelay(500);
	rtl_write_byte(rtlpriv, REG_AFE_PLL_CTRL, tmpu1b | BIT(6));
	udelay(500);
	rtl_write_byte(rtlpriv, REG_AFE_PLL_CTRL, tmpu1b);
	udelay(500);

	/* Release isolation AFE PLL & MD */
	tmpu1b = rtl_read_byte(rtlpriv, REG_SYS_ISO_CTRL);
	tmpu1b &= 0xee;
	rtl_write_byte(rtlpriv, REG_SYS_ISO_CTRL, tmpu1b);

	/* Switch to 40MHz clock */
	rtl_write_byte(rtlpriv, REG_SYS_CLKR, 0x00);

	/* Disable CPU clock and 80MHz SSC to fix FW download timing issue */
	tmpu1b = rtl_read_byte(rtlpriv, REG_SYS_CLKR);
	tmpu1b |= 0xa0;
	rtl_write_byte(rtlpriv, REG_SYS_CLKR, tmpu1b);

	/* Enable MAC clock */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_CLKR);
	tmpu2b |= BIT(12) | SYS_MAC_CLK_EN;
	rtl_write_word(rtlpriv, REG_SYS_CLKR, tmpu2b);

	rtl_write_byte(rtlpriv, REG_PMC_FSM, 0x02);

	/* Enable Core digital and enable IOREG R/W */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_FUNC_EN);
	tmpu2b |= FEN_DCORE;
	rtl_write_word(rtlpriv, REG_SYS_FUNC_EN, tmpu2b);

	/* enable REG_EN */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_FUNC_EN);
	tmpu2b |= FEN_MREGEN;
	rtl_write_word(rtlpriv, REG_SYS_FUNC_EN, tmpu2b);

	/* Switch the control path to FW */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_CLKR);
	tmpu2b |= SYS_FWHW_SEL;
	tmpu2b &= ~SYS_SWHW_SEL;
	rtl_write_word(rtlpriv, REG_SYS_CLKR, tmpu2b);

	rtl_write_word(rtlpriv, REG_CR, HCI_TXDMA_EN |
		HCI_RXDMA_EN | TXDMA_EN | RXDMA_EN | FW2HW_EN |
		DDMA_EN | MACTXEN | MACRXEN | SCHEDULE_EN |
		BB_GLB_RSTN | BBRSTN);

	/* Fix USB RX FIFO error */
	tmpu1b = rtl_read_byte(rtlpriv, REG_USB_AGG_TO);
	tmpu1b |= BIT(7);
	rtl_write_byte(rtlpriv, REG_USB_AGG_TO, tmpu1b);

	/* Fix 8051 ROM incorrect code operation */
	rtl_write_byte(rtlpriv, REG_USB_MAGIC, USB_MAGIC_BIT7);

	/* To make sure that TxDMA can ready to download FW. */
	/* We should reset TxDMA if IMEM RPT was not ready. */
	do {
		tmpu1b = rtl_read_byte(rtlpriv, REG_TCR);
		if ((tmpu1b & TXDMA_INIT_VALUE) == TXDMA_INIT_VALUE)
			break;
		msleep(20);
	} while (--tries);

	if (tries == 0) {
		pr_err("Polling TXDMA_INIT_VALUE timeout!! Current TCR(%#x)\n",
			 tmpu1b);
		tmpu1b = rtl_read_byte(rtlpriv, REG_CR);
		rtl_write_byte(rtlpriv, REG_CR, tmpu1b & (~TXDMA_EN));
		msleep(20);
		/* Reset TxDMA */
		rtl_write_byte(rtlpriv, REG_CR, tmpu1b | TXDMA_EN);
	}

	/* After MACIO reset,we must refresh LED state. */
	if (ppsc->rfoff_reason == RF_CHANGE_BY_IPS ||
	    ppsc->rfoff_reason == 0) {
		enum rf_pwrstate rfpwr_state_toset;
		struct rtl_led *pled0 = &rtlpriv->ledctl.sw_led0;

		rfpwr_state_toset = rtl92s_rf_onoff_detect(hw);
		if (rfpwr_state_toset == ERFON)
			rtl92s_sw_led_on(hw, pled0);
	}
	return 0;
}

static void _r92su_fw_reg_ready(struct rtl_priv *rtlpriv)
{
	unsigned int delay_count = 10;

	do {
		if (rtl_read_dword(rtlpriv, RF_BB_CMD_ADDR) == 0)
			break;
		udelay(5);
	} while (delay_count--);
}

static void __maybe_unused _r92su_set_fw_reg(struct rtl_priv *rtlpriv, u32 cmd,
			      u32 val, bool with_val)
{
	_r92su_fw_reg_ready(rtlpriv);

	if (with_val)
		rtl_write_dword(rtlpriv, RF_BB_CMD_DATA, val);

	FW_CMD_IO_SET(rtlpriv, cmd);
	_r92su_fw_reg_ready(rtlpriv);
}

static void _rtl92su_macconfig_after_fwdownload(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtlpriv);
	u8 i, tmpu1b;

	/* 1. System Configure Register (Offset: 0x0000 - 0x003F) */

	/* 2. Command Control Register (Offset: 0x0040 - 0x004F) */
	/* Turn on 0x40 Command register */
	rtl_write_word(rtlpriv, REG_CR, (BBRSTN | BB_GLB_RSTN |
			SCHEDULE_EN | MACRXEN | MACTXEN | DDMA_EN | FW2HW_EN |
			RXDMA_EN | TXDMA_EN | HCI_RXDMA_EN | HCI_TXDMA_EN));

	/* Set TCR TX DMA pre 2 FULL enable bit	*/
	rtl_write_dword(rtlpriv, REG_TCR, rtl_read_dword(rtlpriv, TCR) |
			TXDMAPRE2FULL);

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
	/* 5.1 Initialize Number of Reserved Pages in Firmware Queue */
	/* Firmware allocate now, associate with FW internal setting.!!! */
	/* Setting TX/RX page size*/
	rtl_write_byte(rtlpriv, REG_PBP, 0x22);

	/* 5.2 Setting TX/RX page size 0/1/2/3/4=64/128/256/512/1024 */
	/* 5.3 Set driver info, we only accept PHY status now. */

	/* 5.4 Set RXDMA arbitration to control RXDMA/MAC/FW R/W for RXFIFO
	 * tmpu1b = rtl_read_byte(rtlpriv, REG_RXDMA_RXCTRL);
	 * tmpu1b |= RXDMA_AGG_EN;
	 * rtl_write_byte(rtlpriv, REG_RXDMA_RXCTRL, tmpu1b);
	 *
	 * NB: rx-streaming is not implemented.
	 */

	rtl_write_byte(rtlpriv, REG_RXDMA_AGG_PG_TH, 0x1);

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
	rtl_write_word(rtlpriv, REG_AGGLEN_LMT_L, 0x5221);
	/* MCS4/5 use max AMPDU size 8*2=16K 6/7 use 10*2=20K */
	rtl_write_word(rtlpriv, REG_AGGLEN_LMT_L + 2, 0xbbb5);
	/* MCS8/9 use max AMPDU size 8*2=16K 10/11 use 10*2=20K */
	rtl_write_word(rtlpriv, REG_AGGLEN_LMT_L + 4, 0xb551);
	/* MCS12/13/14/15 use max AMPDU size 15*2=30K */
	rtl_write_word(rtlpriv, REG_AGGLEN_LMT_L + 6, 0xfffb);

	/* Set Data / Response auto rate fallack retry count */
	rtl_write_dword(rtlpriv, REG_DARFRC, 0x01000000);
	rtl_write_dword(rtlpriv, REG_DARFRC + 4, 0x07060504);
	rtl_write_dword(rtlpriv, REG_RARFRC, 0x01000000);
	rtl_write_dword(rtlpriv, REG_RARFRC + 4, 0x07060605);

	/* 7. EDCA Setting Register (Offset: 0x01D0 - 0x01FF) */
	/* Set all rate to support SG */
	rtl_write_dword(rtlpriv, REG_EDCA_BE_PARAM, 0xa44f);
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

	tmpu1b = rtl_read_byte(rtlpriv, REG_LD_RQPN);
	/* tmpu1b |= BIT(6)|BIT(7);
	 * tmpu1b |= BIT(5); // Only for USBEP_FOUR */
	tmpu1b |= 0xa0;
	rtl_write_byte(rtlpriv, REG_LD_RQPN, tmpu1b);

	rtl_write_byte(rtlpriv, REG_USB_DMA_AGG_TO, 0x0a);

	/* Fix the RX FIFO issue(USB error) */
	tmpu1b = rtl_read_byte(rtlpriv, REG_USB_AGG_TO);
	tmpu1b |= BIT(7);
	rtl_write_byte(rtlpriv, REG_USB_AGG_TO, tmpu1b);

	tmpu1b = rtl_read_byte(rtlpriv, REG_SYS_ISO_CTRL + 1);
	tmpu1b &= 0xFE;
	rtl_write_byte(rtlpriv, REG_SYS_ISO_CTRL + 1, tmpu1b);

	/* Disable DIG as default */
	tmpu1b = rtl_read_byte(rtlpriv, REG_LBUS_MON_ADDR);
	tmpu1b &= ~BIT(0);
	rtl_write_byte(rtlpriv, REG_LBUS_MON_ADDR, tmpu1b);

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

	/* Set MSDU lifetime. */
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
	rtl_write_byte(rtlpriv, AMPDU_MIN_SPACE, rtlhal->minspace_cfg);
}

int rtl92su_hw_init(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	struct rtl_phy *rtlphy = &(rtlpriv->phy);
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	struct rtl_usb *rtlusb = rtl_usbdev(rtl_usbpriv(rtlpriv));

	int err = 0;
	bool rtstatus = true;
	u8 i;

	int wdcapra_add[] = {
		REG_EDCA_BE_PARAM, REG_EDCA_BK_PARAM,
		REG_EDCA_VI_PARAM, REG_EDCA_VO_PARAM};
	u8 secr_value = 0x0;

	/* 1. MAC Initialize */
	/* Before FW download, we have to set some MAC register */
	err = _rtl92su_macconfig_before_fwdownload(hw);
	if (err) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
		 "Failed to get the device ready for the firmware (%d)\n",
		err);
		return err;
	}

	rtlhal->version = (enum version_8192s)((rtl_read_dword(rtlpriv,
			REG_PMC_FSM) >> 16) & 0xF);

	/* 2. download firmware */
	rtstatus = rtl92s_download_fw(hw);
	if (!rtstatus) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
			 "Failed to download FW. Init HW without FW now... Please copy FW into /lib/firmware/rtlwifi\n");
		return -ENOENT;
	}

	/* After FW download, we have to reset MAC register */
	_rtl92su_macconfig_after_fwdownload(hw);

	/* Retrieve default FW Cmd IO map. */
	rtlhal->fwcmd_iomap =	rtl_read_word(rtlpriv, REG_LBUS_MON_ADDR);
	rtlhal->fwcmd_ioparam = rtl_read_dword(rtlpriv, REG_LBUS_ADDR_MASK);

	/* 3. Initialize MAC/PHY Config by MACPHY_reg.txt */
	rtstatus = rtl92s_phy_mac_config(hw);
	if (!rtstatus) {
		pr_err("MAC Config failed\n");
		return -EINVAL;
	}

	/* because last function modify RCR, so we update
	 * rcr var here, or TP will unstable for receive_config
	 * is wrong, RX RCR_ACRC32 will cause TP unstable & Rx
	 * RCR_APP_ICV will cause mac80211 unassoc for cisco 1252
	 */

	/* Make sure BB/RF write OK. We should prevent enter IPS. radio off. */
	/* We must set flag avoid BB/RF config period later!! */
	rtl_write_word(rtlpriv, CMDR, 0x37FC);

	/* 4. Initialize BB After MAC Config PHY_reg.txt, AGC_Tab.txt */
	rtstatus = rtl92s_phy_bb_config(hw);
	if (!rtstatus) {
		pr_err("BB Config failed\n");
		return -ENODEV;
	}

	/* 5. Initiailze RF RAIO_A.txt RF RAIO_B.txt */
	/* Before initalizing RF. We can not use FW to do RF-R/W. */
	rtlphy->rf_mode = RF_OP_BY_SW_3WIRE;

	/* Before RF-R/W we must execute the IO from Scott's suggestion. */
	rtl_write_byte(rtlpriv, REG_AFE_XTAL_CTRL + 1, 0xDB);
	if (rtlhal->version == VERSION_8192S_ACUT)
		rtl_write_byte(rtlpriv, REG_SPS1_CTRL + 3, 0x07);
	else
		rtl_write_byte(rtlpriv, REG_RF_CTRL, 0x07);

	rtstatus = rtl92s_phy_rf_config(hw);
	if (!rtstatus) {
		RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG, "RF Config failed\n");
		return -EOPNOTSUPP;
	}

	/* After read predefined TXT, we must set BB/MAC/RF
	 * register as our requirement */
	rtlphy->rfreg_chnlval[0] = rtl92s_phy_query_rf_reg(hw,
							   (enum radio_path)0,
							   RF_CHNLBW,
							   RFREG_OFFSET_MASK);
	rtlphy->rfreg_chnlval[1] = rtl92s_phy_query_rf_reg(hw,
							   (enum radio_path)1,
							   RF_CHNLBW,
							   RFREG_OFFSET_MASK);

	/*---- Set CCK and OFDM Block "ON"----*/
	rtl_set_bbreg(hw, REG_RFPGA0_RFMOD, BCCKEN, 0x1);
	rtl_set_bbreg(hw, REG_RFPGA0_RFMOD, BOFDMEN, 0x1);

	/*3 Set Hardware(Do nothing now) */
	_rtl92su_hw_configure(hw);

	/* Read EEPROM TX power index and PHY_REG_PG.txt to capture correct */
	/* TX power index for different rate set. */
	/* Get original hw reg values */
	rtl92s_phy_get_hw_reg_originalvalue(hw);
	/* Write correct tx power index */
	rtl92s_phy_set_txpower(hw, 1);

	/* We must set MAC address after firmware download. */
	for (i = 0; i < 6; i++)
		rtl_write_byte(rtlpriv, MACIDR0 + i, rtlefuse->dev_addr[i]);

	/* We enable high power and RA related mechanism after NIC
	 * initialized. */
	if (hal_get_firmwareversion(rtlpriv) >= 0x35) {
		/* Fw v.53 and later. */
		rtl92s_phy_set_fw_cmd(hw, FW_CMD_RA_INIT);
	} else if (hal_get_firmwareversion(rtlpriv) == 0x34) {
		/* Fw v.52. */
		rtl_write_dword(rtlpriv, REG_WFM5, FW_RA_INIT);
		rtl92s_phy_chk_fwcmd_iodone(hw);
	} else {
		/* Compatible earlier FW version. */
		rtl_write_dword(rtlpriv, REG_WFM5, FW_RA_RESET);
		rtl92s_phy_chk_fwcmd_iodone(hw);
		rtl_write_dword(rtlpriv, REG_WFM5, FW_RA_ACTIVE);
		rtl92s_phy_chk_fwcmd_iodone(hw);
		rtl_write_dword(rtlpriv, REG_WFM5, FW_RA_REFRESH);
		rtl92s_phy_chk_fwcmd_iodone(hw);
	}

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

	if (rtlusb->acm_method == eAcmWay1_HW)
		rtl_write_byte(rtlpriv, REG_ACMHWCTRL, ACMHW_HWEN);

	if (rtlphy->rf_type == RF_1T2R) {
		bool mrc2set = true;
		/* Turn on B-Path */
		rtlpriv->cfg->ops->set_hw_reg(hw, HW_VAR_MRC, (u8 *)&mrc2set);
	}

	rtlpriv->cfg->ops->led_control(hw, LED_CTL_POWER_ON);
	rtl92s_dm_init(hw);
	return err;
}

/* HW_VAR_MEDIA_STATUS & HW_VAR_CECHK_BSSID */
int rtl92su_set_network_type(struct ieee80211_hw *hw, enum nl80211_iftype type)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	if (rtl92s_set_media_status(hw, type))
		return -EOPNOTSUPP;

	if (rtlpriv->mac80211.link_state == MAC80211_LINKED) {
		if (type != NL80211_IFTYPE_AP)
			rtl92s_set_check_bssid(hw, true);
	} else {
		rtl92s_set_check_bssid(hw, false);
	}

	return 0;
}

void rtl92su_enable_interrupt(struct ieee80211_hw *hw)
{
}

void rtl92su_disable_interrupt(struct ieee80211_hw *hw)
{
}

void rtl92su_card_disable(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	struct rtl_ps_ctl *ppsc = rtl_psc(rtl_priv(hw));
	enum nl80211_iftype opmode;
	u8 wait = 30;

	rtlpriv->cfg->ops->led_control(hw, LED_CTL_POWER_OFF);
	RT_SET_PS_LEVEL(ppsc, RT_RF_OFF_LEVL_HALT_NIC);

	/* this is very important for ips power save */
	while (wait-- >= 10 && rtlpriv->psc.pwrdomain_protect) {
		if (rtlpriv->psc.pwrdomain_protect)
			mdelay(20);
		else
			break;
	}

	mac->link_state = MAC80211_NOLINK;
	opmode = NL80211_IFTYPE_UNSPECIFIED;
	rtl92s_set_media_status(hw, opmode);

	rtl92s_phy_set_rfhalt(hw);
	udelay(100);
}

void rtl92su_update_interrupt_mask(struct ieee80211_hw *hw,
		u32 add_msr, u32 rm_msr)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_mac *mac = rtl_mac(rtlpriv);

	/* Free beacon resources */
	if (((mac->vif->type == NL80211_IFTYPE_AP) ||
	     (mac->vif->type == NL80211_IFTYPE_ADHOC) ||
	     (mac->vif->type == NL80211_IFTYPE_MESH_POINT)) &&
	     (mac->beacon_enabled)) {
		rtl92su_update_beacon(hw);
	}
}

bool rtl92su_gpio_radio_on_off_checking(struct ieee80211_hw *hw, u8 *valid)
{
	*valid = false;
	/* always on */
	return true;
}

/* Turn on AAP (RCR:bit 0) for promicuous mode. */
void rtl92su_allow_all_destaddr(struct ieee80211_hw *hw,
				bool allow_all_da, bool write_into_reg)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 reg_rcr = rtl_read_dword(rtlpriv, RCR);

	if (allow_all_da) /* Set BIT0 */
		reg_rcr |= RCR_AAP;
	else /* Clear BIT0 */
		reg_rcr &= ~RCR_AAP;

	if (write_into_reg)
		rtl_write_dword(rtlpriv, RCR, reg_rcr);

	RT_TRACE(rtlpriv, COMP_TURBO | COMP_INIT, DBG_LOUD,
		 "receive_config=0x%08X, write_into_reg=%d\n",
		 reg_rcr, write_into_reg);
}

void rtl92su_read_eeprom_info(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_efuse *rtlefuse = rtl_efuse(rtlpriv);
	struct rtl_phy *rtlphy = &(rtlpriv->phy);
	struct r92su_eeprom eeprom;
	u16 i, eeprom_id;
	u8 tempval;
	u8 rf_path, index;

	rtl92s_read_eeprom_info(hw);

	switch (rtlefuse->epromtype) {
	case EEPROM_BOOT_EFUSE:
		rtl_efuse_shadow_map_update(hw);
		break;

	case EEPROM_93C46:
		pr_err("RTL819X Not boot from eeprom, check it !!\n");
		return;

	default:
		pr_warn("rtl92su: no efuse data\n\n");
		return;
	}

	memcpy(&eeprom, &rtlefuse->efuse_map[EFUSE_INIT_MAP][0],
	       HWSET_MAX_SIZE_92S);

	RT_PRINT_DATA(rtlpriv, COMP_INIT, DBG_DMESG, "MAP",
		      &eeprom, sizeof(eeprom));
	eeprom_id = le16_to_cpu(eeprom.id);

	if (eeprom_id != RTL8190_EEPROM_ID) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
			 "EEPROM ID(%#x) is invalid!!\n", eeprom_id);
		rtlefuse->autoload_failflag = true;
		return;
	}

	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD, "Autoload OK\n");
	rtlefuse->autoload_failflag = false;

	rtl92s_get_IC_Inferiority(hw);

	/* Read IC Version && Channel Plan */
	/* VID, DID	 SE	0xA-D */
	rtlefuse->eeprom_vid = le16_to_cpu(eeprom.vid);
	rtlefuse->eeprom_did = le16_to_cpu(eeprom.did);
	rtlefuse->eeprom_svid = 0;
	rtlefuse->eeprom_smid = 0;
	rtlefuse->eeprom_version = eeprom.version;

	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "EEPROMId = 0x%4x\n", eeprom_id);
	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "EEPROM VID = 0x%4x\n", rtlefuse->eeprom_vid);
	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "EEPROM DID = 0x%4x\n", rtlefuse->eeprom_did);
	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "EEPROM SVID = 0x%4x\n", rtlefuse->eeprom_svid);
	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "EEPROM SMID = 0x%4x\n", rtlefuse->eeprom_smid);

	ether_addr_copy(rtlefuse->dev_addr, eeprom.mac_addr);

	for (i = 0; i < 6; i++)
		rtl_write_byte(rtlpriv, MACIDR0 + i, rtlefuse->dev_addr[i]);

	RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG, "%pM\n", rtlefuse->dev_addr);

	/* Get Tx Power Level by Channel */
	/* Read Tx power of Channel 1 ~ 14 from EEPROM. */
	/* 92S suupport RF A & B */
	for (rf_path = 0; rf_path < RF_PATH; rf_path++) {
		for (i = 0; i < CHAN_SET; i++) {
			/* Read CCK RF A & B Tx power  */
			rtlefuse->eeprom_chnlarea_txpwr_cck[rf_path][i] =
				eeprom.tx_pwr_cck[rf_path][i];

			/* Read OFDM RF A & B Tx power for 1T */
			rtlefuse->eeprom_chnlarea_txpwr_ht40_1s[rf_path][i] =
				eeprom.tx_pwr_ht40_1t[rf_path][i];

			/* Read OFDM RF A & B Tx power for 2T */
			rtlefuse->eprom_chnl_txpwr_ht40_2sdf[rf_path][i] =
				eeprom.tx_pwr_ht40_2t[rf_path][i];
		}
	}

	for (rf_path = 0; rf_path < RF_PATH; rf_path++) {
		for (i = 0; i < CHAN_SET; i++) {
			/* Read Power diff limit. */
			rtlefuse->eeprom_pwrgroup[rf_path][i] =
				eeprom.tx_pwr_edge[rf_path][i];
		}
	}

	for (rf_path = 0; rf_path < RF_PATH; rf_path++)
		for (i = 0; i < CHAN_SET; i++)
			RTPRINT(rtlpriv, FINIT, INIT_EEPROM,
				"RF(%d) EEPROM CCK Area(%d) = 0x%x\n",
				rf_path, i,
				rtlefuse->eeprom_chnlarea_txpwr_cck
				[rf_path][i]);
	for (rf_path = 0; rf_path < RF_PATH; rf_path++)
		for (i = 0; i < CHAN_SET; i++)
			RTPRINT(rtlpriv, FINIT, INIT_EEPROM,
				"RF(%d) EEPROM HT40 1S Area(%d) = 0x%x\n",
				rf_path, i,
				rtlefuse->eeprom_chnlarea_txpwr_ht40_1s
				[rf_path][i]);
	for (rf_path = 0; rf_path < RF_PATH; rf_path++)
		for (i = 0; i < CHAN_SET; i++)
			RTPRINT(rtlpriv, FINIT, INIT_EEPROM,
				"RF(%d) EEPROM HT40 2S Diff Area(%d) = 0x%x\n",
				rf_path, i,
				rtlefuse->eprom_chnl_txpwr_ht40_2sdf
				[rf_path][i]);

	for (rf_path = 0; rf_path < RF_PATH; rf_path++) {

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
				rtlefuse->eprom_chnl_txpwr_ht40_2sdf
							[rf_path][index];
		}

		for (i = 0; i < 14; i++) {
			RTPRINT(rtlpriv, FINIT, INIT_TXPOWER,
				"RF(%d)-Ch(%d) [CCK / HT40_1S / HT40_2S] = [0x%x / 0x%x / 0x%x]\n",
				rf_path, i,
				rtlefuse->txpwrlevel_cck[rf_path][i],
				rtlefuse->txpwrlevel_ht40_1s[rf_path][i],
				rtlefuse->txpwrlevel_ht40_2s[rf_path][i]);
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

			RTPRINT(rtlpriv, FINIT, INIT_TXPOWER,
				"RF-%d pwrgroup_ht20[%d] = 0x%x\n",
				rf_path, i,
				rtlefuse->pwrgroup_ht20[rf_path][i]);
			RTPRINT(rtlpriv, FINIT, INIT_TXPOWER,
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

		tempval = eeprom.tx_pwr_ht20_diff[index] & 0xff;
		rtlefuse->txpwr_ht20diff[RF90_PATH_A][i] = (tempval & 0xF);
		rtlefuse->txpwr_ht20diff[RF90_PATH_B][i] =
						 ((tempval >> 4) & 0xF);

		/* Read OFDM<->HT tx power diff */
		/* Channel 1-3 */
		if (i < 3)
			tempval = eeprom.tx_pwr_ofdm_diff[0];
		/* Channel 4-8 */
		else if (i < 8)
			tempval = eeprom.tx_pwr_ofdm_diff_cont;
		/* Channel 9-14 */
		else
			tempval = eeprom.tx_pwr_ofdm_diff[1];

		rtlefuse->txpwr_legacyhtdiff[RF90_PATH_A][i] =
				 (tempval & 0xF);
		rtlefuse->txpwr_legacyhtdiff[RF90_PATH_B][i] =
				 ((tempval >> 4) & 0xF);

		tempval = eeprom.tx_pwr_edge_chk;
		rtlefuse->txpwr_safetyflag = (tempval & 0x01);
	}

	rtlefuse->eeprom_regulatory = 0;
	if (rtlefuse->eeprom_version >= 2) {
		/* BIT(0)~2 */
		if (rtlefuse->eeprom_version >= 4)
			rtlefuse->eeprom_regulatory =
				 (eeprom.regulatory & 0x7);
		else /* BIT(0) */
			rtlefuse->eeprom_regulatory =
				 (eeprom.regulatory & 0x1);
	}
	RTPRINT(rtlpriv, FINIT, INIT_TXPOWER,
		"eeprom_regulatory = 0x%x\n", rtlefuse->eeprom_regulatory);

	for (i = 0; i < 14; i++)
		RTPRINT(rtlpriv, FINIT, INIT_TXPOWER,
			"RF-A Ht20 to HT40 Diff[%d] = 0x%x\n",
			i, rtlefuse->txpwr_ht20diff[RF90_PATH_A][i]);
	for (i = 0; i < 14; i++)
		RTPRINT(rtlpriv, FINIT, INIT_TXPOWER,
			"RF-A Legacy to Ht40 Diff[%d] = 0x%x\n",
			i, rtlefuse->txpwr_legacyhtdiff[RF90_PATH_A][i]);
	for (i = 0; i < 14; i++)
		RTPRINT(rtlpriv, FINIT, INIT_TXPOWER,
			"RF-B Ht20 to HT40 Diff[%d] = 0x%x\n",
			i, rtlefuse->txpwr_ht20diff[RF90_PATH_B][i]);
	for (i = 0; i < 14; i++)
		RTPRINT(rtlpriv, FINIT, INIT_TXPOWER,
			"RF-B Legacy to HT40 Diff[%d] = 0x%x\n",
			i, rtlefuse->txpwr_legacyhtdiff[RF90_PATH_B][i]);

	RTPRINT(rtlpriv, FINIT, INIT_TXPOWER,
		"TxPwrSafetyFlag = %d\n", rtlefuse->txpwr_safetyflag);

	/* Read RF-indication and Tx Power gain
	 * index diff of legacy to HT OFDM rate. */
	tempval = eeprom.rf_ind_power_diff & 0xff;
	rtlefuse->eeprom_txpowerdiff = tempval;
	rtlefuse->legacy_httxpowerdiff =
		rtlefuse->txpwr_legacyhtdiff[RF90_PATH_A][0];

	RTPRINT(rtlpriv, FINIT, INIT_TXPOWER,
		"TxPowerDiff = %#x\n", rtlefuse->eeprom_txpowerdiff);

	/* Get TSSI value for each path. */
	rtlefuse->eeprom_tssi[RF90_PATH_A] = eeprom.tssi[RF90_PATH_A];
	rtlefuse->eeprom_tssi[RF90_PATH_B] = eeprom.tssi[RF90_PATH_B];

	RTPRINT(rtlpriv, FINIT, INIT_TXPOWER, "TSSI_A = 0x%x, TSSI_B = 0x%x\n",
		rtlefuse->eeprom_tssi[RF90_PATH_A],
		rtlefuse->eeprom_tssi[RF90_PATH_B]);

	/* Read antenna tx power offset of B/C/D to A  from EEPROM */
	/* and read ThermalMeter from EEPROM */
	tempval = eeprom.thermal_meter;
	rtlefuse->eeprom_thermalmeter = tempval;
	RTPRINT(rtlpriv, FINIT, INIT_TXPOWER,
		"thermalmeter = 0x%x\n", rtlefuse->eeprom_thermalmeter);

	/* ThermalMeter, BIT(0)~3 for RFIC1, BIT(4)~7 for RFIC2 */
	rtlefuse->thermalmeter[0] = (rtlefuse->eeprom_thermalmeter & 0x1f);
	rtlefuse->tssi_13dbm = rtlefuse->eeprom_thermalmeter * 100;

	/* Read CrystalCap from EEPROM */
	rtlefuse->eeprom_crystalcap = eeprom.crystal_cap >> 4;
	/* CrystalCap, BIT(12)~15 */
	rtlefuse->crystalcap = rtlefuse->eeprom_crystalcap;

	/* Read IC Version && Channel Plan */
	/* Version ID, Channel plan */
	rtlefuse->eeprom_channelplan = eeprom.channel_plan;
	rtlefuse->txpwr_fromeprom = true;
	RTPRINT(rtlpriv, FINIT, INIT_TXPOWER,
		"EEPROM ChannelPlan = 0x%4x\n", rtlefuse->eeprom_channelplan);

	/* Read Customer ID or Board Type!!! */
	tempval = eeprom.board_type;
	/* Change RF type definition */
	if (tempval == 0)
		rtlphy->rf_type = RF_1T1R;
	else if (tempval == 1)
		rtlphy->rf_type = RF_1T2R;
	else if (tempval == 2)
		rtlphy->rf_type = RF_2T2R;
	else if (tempval == 3)
		rtlphy->rf_type = RF_1T1R;

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
	rtlefuse->eeprom_oemid = eeprom.custom_id;

	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD, "EEPROM Customer ID: 0x%2x\n",
		 rtlefuse->eeprom_oemid);

	/* set channel plan to world wide 13 */
	rtlefuse->channel_plan = COUNTRY_CODE_WORLD_WIDE_13;
}

bool rtl92su_phy_set_rf_power_state(struct ieee80211_hw *hw,
				    enum rf_pwrstate rfpwr_state)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_ps_ctl *ppsc = rtl_psc(rtl_priv(hw));
	bool bresult = true;

	if (rfpwr_state == ppsc->rfpwr_state)
		return false;

	switch (rfpwr_state) {
	case ERFON:{
			if ((ppsc->rfpwr_state == ERFOFF) &&
			    RT_IN_PS_LEVEL(ppsc, RT_RF_OFF_LEVL_HALT_NIC)) {

				bool rtstatus;
				u32 InitializeCount = 0;

				do {
					InitializeCount++;
					RT_TRACE(rtlpriv, COMP_RF, DBG_DMESG,
						 "IPS Set eRf nic enable\n");
					rtstatus = rtl_ps_enable_nic(hw);
				} while (!rtstatus && (InitializeCount < 10));

				RT_CLEAR_PS_LEVEL(ppsc,
						  RT_RF_OFF_LEVL_HALT_NIC);
			} else {
				RT_TRACE(rtlpriv, COMP_POWER, DBG_DMESG,
					 "awake, slept:%d ms state_inap:%x\n",
					 jiffies_to_msecs(jiffies -
							  ppsc->
							  last_sleep_jiffies),
					 rtlpriv->psc.state_inap);
				ppsc->last_awake_jiffies = jiffies;

				rtl_write_word(rtlpriv, CMDR, 0x37FC);
				rtl_write_byte(rtlpriv, REG_TXPAUSE, 0x00);
				rtl_write_byte(rtlpriv, REG_RFPGA0_CCA, 0x3);
				rtl_write_byte(rtlpriv, REG_SPS1_CTRL, 0x64);
			}
			break;
		}
	case ERFOFF:{
			if (ppsc->reg_rfps_level & RT_RF_OFF_LEVL_HALT_NIC) {
				RT_TRACE(rtlpriv, COMP_RF, DBG_DMESG,
					 "IPS Set eRf nic disable\n");
				rtl_ps_disable_nic(hw);
				RT_SET_PS_LEVEL(ppsc, RT_RF_OFF_LEVL_HALT_NIC);
			}
			break;
		}
	case ERFSLEEP:
			if (ppsc->rfpwr_state == ERFOFF)
				return false;

			RT_TRACE(rtlpriv, COMP_POWER, DBG_DMESG,
				 "Set ERFSLEEP awaked:%d ms\n",
				 jiffies_to_msecs(jiffies -
						  ppsc->last_awake_jiffies));

			RT_TRACE(rtlpriv, COMP_POWER, DBG_DMESG,
				 "sleep awaked:%d ms state_inap:%x\n",
				 jiffies_to_msecs(jiffies -
						  ppsc->last_awake_jiffies),
				 rtlpriv->psc.state_inap);
			ppsc->last_sleep_jiffies = jiffies;
			rtl92s_phy_set_rf_sleep(hw);
			break;
	default:
		pr_err("switch case %#x not processed\n", rfpwr_state);
		bresult = false;
		break;
	}

	if (bresult)
		ppsc->rfpwr_state = rfpwr_state;

	return bresult;
}
