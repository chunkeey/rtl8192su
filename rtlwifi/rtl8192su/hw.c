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
 * Christian Lamparter <chunkeey@googlemail.com>
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/

#include "../wifi.h"
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
#include "led.h"
#include "hw.h"

static int _rtl92su_macconfig_before_fwdownload(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	unsigned int tries = 20;
	u8 tmpu1b;
	u16 tmpu2b;

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
	tmpu1b |= AFE_BGEN;
	rtl_write_byte(rtlpriv, REG_AFE_MISC, tmpu1b);
	mdelay(1);

	/* Enable AFE Mbias */
	tmpu1b = rtl_read_byte(rtlpriv, REG_AFE_MISC);
	tmpu1b |= AFE_BGEN | AFE_MBEN | AFE_MISC_I32_EN;
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
	rtl_write_byte(rtlpriv, REG_AFE_PLL_CTRL, (tmpu1b | BIT(0) |
		       BIT(4) | BIT(6)));
	udelay(500);
	rtl_write_byte(rtlpriv, REG_AFE_PLL_CTRL, (tmpu1b | BIT(0) | BIT(4)));
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
	tmpu2b |= BIT(12) | BIT(11);
	if (!rtl92s_halset_sysclk(hw, tmpu2b))
		return -EIO;

	rtl_write_byte(rtlpriv, REG_PMC_FSM, 0x02);

	/* Enable Core digital and enable IOREG R/W */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_FUNC_EN);
	tmpu2b |= BIT(11);
	rtl_write_word(rtlpriv, REG_SYS_FUNC_EN, tmpu2b);

	/* enable REG_EN */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_FUNC_EN);
	tmpu2b |= BIT(15);
	rtl_write_word(rtlpriv, REG_SYS_FUNC_EN, tmpu2b);

	/* Switch the control path to FW */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_CLKR);
	tmpu2b |= SYS_FWHW_SEL;
	tmpu2b &= ~SYS_SWHW_SEL;
	if (!rtl92s_halset_sysclk(hw, tmpu2b))
		return -EIO;

	 /* For power save, used this in the bit file after 970621 */
	tmpu1b = rtl_read_byte(rtlpriv, SYS_CLKR);
	tmpu1b &= ~SYS_CPU_CLKSEL;
	rtl_write_byte(rtlpriv, SYS_CLKR, tmpu1b);

	tmpu1b = rtl_read_byte(rtlpriv, RTL8712_FIFOCTRL_ + 0xb);
	tmpu1b |= BIT(6)|BIT(7);
	tmpu1b |= BIT(5); /* Only for USBEP_FOUR */
	rtl_write_byte(rtlpriv, RTL8712_FIFOCTRL_ + 0xb, tmpu1b);

	rtl_write_word(rtlpriv, REG_CR, HCI_TXDMA_EN |
		HCI_RXDMA_EN | TXDMA_EN | RXDMA_EN | FW2HW_EN |
		DDMA_EN | MACTXEN | MACRXEN | SCHEDULE_EN |
		BB_GLB_RSTN | BBRSTN);

	/* Fix USB RX FIFO error */
	tmpu1b = rtl_read_byte(rtlpriv, REG_USB_AGG_TO);
	tmpu1b |= BIT(7);
	rtl_write_byte(rtlpriv, REG_USB_AGG_TO, tmpu1b);

	/* Enable MAC clock */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_CLKR);
	tmpu2b &= ~SYS_CPU_CLKSEL;
	if (!rtl92s_halset_sysclk(hw, tmpu2b))
		return -EIO;

	/* Fix 8051 ROM incorrect code operation */
	rtl_write_byte(rtlpriv, REG_USB_MAGIC, USB_MAGIC_BIT7);

	/* To make sure that TxDMA can ready to download FW. */
	/* We should reset TxDMA if IMEM RPT was not ready. */
	do {
		tmpu1b = rtl_read_byte(rtlpriv, REG_TCR);
		if ((tmpu1b & TXDMA_INIT_VALUE) == TXDMA_INIT_VALUE)
			break;
		msleep(5);
	} while (--tries);

	if (tries == 0) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "Polling TXDMA_INIT_VALUE timeout!! Current TCR(%#x)\n",
			 tmpu1b);
		tmpu1b = rtl_read_byte(rtlpriv, REG_CR);
		rtl_write_byte(rtlpriv, REG_CR, tmpu1b & (~TXDMA_EN));
		msleep(2);
		/* Reset TxDMA */
		rtl_write_byte(rtlpriv, REG_CR, tmpu1b | TXDMA_EN);
	}
	rtl_write_word(rtlpriv, BCN_DRV_EARLY_INT, 0x0000);

	/* After MACIO reset,we must refresh LED state. */
	if ((ppsc->rfoff_reason == RF_CHANGE_BY_IPS) ||
	    (ppsc->rfoff_reason == 0) {
		enum rf_pwrstate rfpwr_state_toset;
		rfpwr_state_toset = rtl92s_rf_onoff_detect(hw);
		if (rfpwr_state_toset == ERFON)
			rtl92s_sw_led_on(hw, pLed0);
	}
	return 0;
}

static void _r92su_fw_reg_ready(struct rtl_priv *rtlpriv)
{
	unsigned int delay_count = 10;
	do {
		if (rtl_read_dword(rtlpriv, RF_BB_CMD_ADDR) != 0)
			break;
		udelay(5);
	} while (delay_count--);
}

static void _r92su_set_fw_reg(struct rtl_priv *rtlpriv, u32 cmd, u32 val, bool with_val)
{
	_r92su_fw_reg_ready(rtlpriv);

	if (with_val)
		rtl_write_dword(rtlpriv, RF_BB_CMD_DATA, val);

        rtl_write_dword(rtlpriv, RF_BB_CMD_ADDR, cmd);

	_r92su_fw_reg_ready(rtlpriv);
}

static void _rtl92su_macconfig_after_fwdownload(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	u8 i, tmpu1b;
	u16 tmpu2b;

	/* 1. System Configure Register (Offset: 0x0000 - 0x003F) */

	/* 2. Command Control Register (Offset: 0x0040 - 0x004F) */
	/* Turn on 0x40 Command register */
	rtl_write_word(rtlpriv, CMDR, (BBRSTN | BB_GLB_RSTN |
			SCHEDULE_EN | MACRXEN | MACTXEN | DDMA_EN | FW2HW_EN |
			RXDMA_EN | TXDMA_EN | HCI_RXDMA_EN | HCI_TXDMA_EN));

	/* Set TCR TX DMA pre 2 FULL enable bit	*/
	rtl_write_dword(rtlpriv, TCR, rtl_read_dword(rtlpriv, TCR) |
			TXDMAPRE2FULL);

	/* 3. MACID Setting Register (Offset: 0x0050 - 0x007F) */

	/* 4. Timing Control Register  (Offset: 0x0080 - 0x009F) */
	/* Set CCK/OFDM SIFS */
	/* CCK SIFS shall always be 10us. */
	rtl_write_word(rtlpriv, SIFS_CCK, 0x0a0a);
	rtl_write_word(rtlpriv, SIFS_OFDM, 0x1010);

	/* Set AckTimeout */
	rtl_write_byte(rtlpriv, ACK_TIMEOUT, 0x40);

	/* Beacon related */
	_r92su_set_fw_reg(rtlpriv, 0xf1006400, 0, false);
	rtl_write_word(rtlpriv, BCN_INTERVAL, 100);
	rtl_write_word(rtlpriv, ATIMWND, 2);

	/* 5. FIFO Control Register (Offset: 0x00A0 - 0x015F) */
	/* 5.1 Initialize Number of Reserved Pages in Firmware Queue */
	/* Firmware allocate now, associate with FW internal setting.!!! */
	/* Setting TX/RX page size*/
	rtl_write_byte(rtlpriv, REG_PBP, 0x22);
	
	/* 5.2 Setting TX/RX page size 0/1/2/3/4=64/128/256/512/1024 */
	/* 5.3 Set driver info, we only accept PHY status now. */
	/* 5.4 Set RXDMA arbitration to control RXDMA/MAC/FW R/W for RXFIFO  */
	tmpu1b = rtl_read_byte(rtlpriv, REG_RXDMA);
	tmpu1b |= BIT(6);
	rtl_write_byte(rtlpriv, REG_RXDMA, tmpu1b);

	/* 6. Adaptive Control Register  (Offset: 0x0160 - 0x01CF) */
	/* Set RRSR to all legacy rate and HT rate
	 * CCK rate is supported by default.
	 * CCK rate will be filtered out only when associated
	 * AP does not support it.
	 * Only enable ACK rate to OFDM 24M
	 * Disable RRSR for CCK rate in A-Cut	*/

	if (rtlhal->version == VERSION_8192S_ACUT)
		rtl_write_byte(rtlpriv, RRSR, 0xf0);
	else if (rtlhal->version == VERSION_8192S_BCUT)
		rtl_write_byte(rtlpriv, RRSR, 0xff);
	rtl_write_byte(rtlpriv, RRSR + 1, 0x01);
	rtl_write_byte(rtlpriv, RRSR + 2, 0x00);

	/* A-Cut IC do not support CCK rate. We forbid ARFR to */
	/* fallback to CCK rate */
	for (i = 0; i < 8; i++) {
		/*Disable RRSR for CCK rate in A-Cut */
		if (rtlhal->version == VERSION_8192S_ACUT)
			rtl_write_dword(rtlpriv, ARFR0 + i * 4, 0x1f0ff0f0);
	}

	/* Different rate use different AMPDU size */
	/* MCS32/ MCS15_SG use max AMPDU size 15*2=30K */
	rtl_write_byte(rtlpriv, AGGLEN_LMT_H, 0x0f);
	/* MCS0/1/2/3 use max AMPDU size 4*2=8K */
	rtl_write_word(rtlpriv, AGGLEN_LMT_L, 0x5221);
	/* MCS4/5 use max AMPDU size 8*2=16K 6/7 use 10*2=20K */
	rtl_write_word(rtlpriv, AGGLEN_LMT_L + 2, 0xbbb5);
	/* MCS8/9 use max AMPDU size 8*2=16K 10/11 use 10*2=20K */
	rtl_write_word(rtlpriv, AGGLEN_LMT_L + 4, 0xb551);
	/* MCS12/13/14/15 use max AMPDU size 15*2=30K */
	rtl_write_word(rtlpriv, AGGLEN_LMT_L + 6, 0xfffb);

	/* Set Data / Response auto rate fallack retry count */
	rtl_write_dword(rtlpriv, DARFRC, 0x01000000);
	rtl_write_dword(rtlpriv, DARFRC + 4, 0x07060504);
	rtl_write_dword(rtlpriv, RARFRC, 0x01000000);
	rtl_write_dword(rtlpriv, RARFRC + 4, 0x07060605);

	/* 7. EDCA Setting Register (Offset: 0x01D0 - 0x01FF) */
	/* Set all rate to support SG */
	rtl_write_dword(rtlpriv, EDCAPARA_BE, 0xa44f);
	rtl_write_word(rtlpriv, SG_RATE, 0xFFFF);

	/* 8. WMAC, BA, and CCX related Register (Offset: 0x0200 - 0x023F) */
	/* Set NAV protection length */
	rtl_write_word(rtlpriv, NAV_PROT_LEN, 0x0080);
	/* CF-END Threshold */
	rtl_write_byte(rtlpriv, CFEND_TH, 0xFF);
	/* Set AMPDU minimum space */
	rtl_write_byte(rtlpriv, AMPDU_MIN_SPACE, 0x07);
	/* Set TXOP stall control for several queue/HI/BCN/MGT/ */
	rtl_write_byte(rtlpriv, TXOP_STALL_CTRL, 0x00);

	/* 9. Security Control Register (Offset: 0x0240 - 0x025F) */
	/* 10. Power Save Control Register (Offset: 0x0260 - 0x02DF) */
	/* 11. General Purpose Register (Offset: 0x02E0 - 0x02FF) */
	/* 12. Host Interrupt Status Register (Offset: 0x0300 - 0x030F) */
	/* 13. Test Mode and Debug Control Register (Offset: 0x0310 - 0x034F) */

	/* 14. Set driver info, we only accept PHY status now. */
	rtl_write_byte(rtlpriv, RXDRVINFO_SZ, 4);

	tmpu1b = rtl_read_byte(rtlpriv, 0x00ab);
	tmpu1b |= BIT(6)|BIT(7);
	tmpu1b |= BIT(5); /* Only for USBEP_FOUR */
	rtl_write_byte(rtlpriv, 0x00ab, tmpu1b);

	/* Fix the RX FIFO issue(USB error) */
	tmpu1b = rtl_read_byte(rtlpriv, 0xfe5C);
	tmpu1b |= BIT(7);
	rtl_write_byte(rtlpriv, 0xfe5C, tmpu1b);

	/* Revise USB PHY to solve the issue of Rx payload error */
	rtl_write_byte(rtlpriv, 0xfe41, 0xf4);
	rtl_write_byte(rtlpriv, 0xfe40, 0x00);
	rtl_write_byte(rtlpriv, 0xfe42, 0x00);
	rtl_write_byte(rtlpriv, 0xfe42, 0x01);
	rtl_write_byte(rtlpriv, 0xfe40, 0x0f);
	rtl_write_byte(rtlpriv, 0xfe42, 0x00);
	rtl_write_byte(rtlpriv, 0xfe42, 0x01);

	/* 15. For EEPROM R/W Workaround */
	/* 16. For EFUSE to share REG_SYS_FUNC_EN with EEPROM!!! */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_FUNC_EN);
	tmpu2b |= BIT(13);
	rtl_write_word(rtlpriv, REG_SYS_FUNC_EN, tmpu2b);
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_ISO_CTRL);
	tmpu2b &= (~BIT(8));
	rtl_write_word(rtlpriv, REG_SYS_ISO_CTRL, tmpu2b);

	/* 17. For EFUSE */
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

	/* Make sure DIG is done by false alarm */
	_r92su_set_fw_reg(rtlpriv, 0xfd00ff00, 0x0, false);

	/* Disable DIG as default */
	tmpu1b = rtl_read_byte(rtlpriv, 0x364);
	tmpu1b &= ~BIT(0);
	rtl_write_byte(rtlpriv, 0x364, tmpu1b);

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

	regtmp = rtl_read_byte(rtlpriv, INIRTSMCS_SEL);
	reg_rrsr = ((reg_rrsr & 0x000fffff) << 8) | regtmp;
	rtl_write_dword(rtlpriv, INIRTSMCS_SEL, reg_rrsr);
	rtl_write_byte(rtlpriv, BW_OPMODE, reg_bw_opmode);

	rtl_write_byte(rtlpriv, MLT, 0x8f);

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
	u8 tmp_byte = 0;

	bool rtstatus = true;
	u8 tmp_u1b;
	int err = 0;
	u8 i;
	int wdcapra_add[] = {
		EDCAPARA_BE, EDCAPARA_BK,
		EDCAPARA_VI, EDCAPARA_VO};
	u8 secr_value = 0x0;

	rtlhal->hw_type = HARDWARE_TYPE_RTL8192SU;

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
			PMC_FSM) >> 16) & 0xF);

	rtl92s_gpiobit3_cfg_inputmode(hw);

	/* 2. download firmware */
	rtstatus = rtl92s_download_fw(hw);
	if (!rtstatus) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
			 "Failed to download FW. Init HW without FW now... "
			 "Please copy FW into /lib/firmware/rtlwifi\n");
		return -ENOENT;
	}

	/* After FW download, we have to reset MAC register */
	_rtl92su_macconfig_after_fwdownload(hw);

	/*Retrieve default FW Cmd IO map. */
	rtlhal->fwcmd_iomap =	rtl_read_word(rtlpriv, LBUS_MON_ADDR);
	rtlhal->fwcmd_ioparam = rtl_read_dword(rtlpriv, LBUS_ADDR_MASK);

	/* 3. Initialize MAC/PHY Config by MACPHY_reg.txt */
	rtstatus = rtl92s_phy_mac_config(hw);
	if (!rtstatus) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG, "MAC Config failed\n");
		return -EINVAL;
	}

	/* because last function modify RCR, so we update
	 * rcr var here, or TP will unstable for receive_config
	 * is wrong, RX RCR_ACRC32 will cause TP unstabel & Rx
	 * RCR_APP_ICV will cause mac80211 unassoc for cisco 1252
	 */
	//

	/* Make sure BB/RF write OK. We should prevent enter IPS. radio off. */
	/* We must set flag avoid BB/RF config period later!! */
	rtl_write_dword(rtlpriv, CMDR, 0x37FC);

	/* 4. Initialize BB After MAC Config PHY_reg.txt, AGC_Tab.txt */
	rtstatus = rtl92s_phy_bb_config(hw);
	if (!rtstatus) {
		RT_TRACE(rtlpriv, COMP_INIT, DBG_EMERG, "BB Config failed\n");
		return -ENODEV;
	}

	/* 5. Initiailze RF RAIO_A.txt RF RAIO_B.txt */
	/* Before initalizing RF. We can not use FW to do RF-R/W. */

	rtlphy->rf_mode = RF_OP_BY_SW_3WIRE;

	/* Before RF-R/W we must execute the IO from Scott's suggestion. */
	rtl_write_byte(rtlpriv, AFE_XTAL_CTRL + 1, 0xDB);
	if (rtlhal->version == VERSION_8192S_ACUT)
		rtl_write_byte(rtlpriv, SPS1_CTRL + 3, 0x07);
	else
		rtl_write_byte(rtlpriv, RF_CTRL, 0x07);

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
	rtl_set_bbreg(hw, RFPGA0_RFMOD, BCCKEN, 0x1);
	rtl_set_bbreg(hw, RFPGA0_RFMOD, BOFDMEN, 0x1);

	/*3 Set Hardware(Do nothing now) */
	_rtl92su_hw_configure(hw);

	/* Read EEPROM TX power index and PHY_REG_PG.txt to capture correct */
	/* TX power index for different rate set. */
	/* Get original hw reg values */
	rtl92s_phy_get_hw_reg_originalvalue(hw);
	/* Write correct tx power index */
	rtl92s_phy_set_txpower(hw, rtlphy->current_channel);

	/* We must set MAC address after firmware download. */
	for (i = 0; i < 6; i++)
		rtl_write_byte(rtlpriv, MACIDR0 + i, rtlefuse->dev_addr[i]);

	/* EEPROM R/W workaround */
	tmp_u1b = rtl_read_byte(rtlpriv, MAC_PINMUX_CFG);
	rtl_write_byte(rtlpriv, MAC_PINMUX_CFG, tmp_u1b & (~BIT(3)));

	rtl_write_byte(rtlpriv, 0x4d, 0x0);

	if (hal_get_firmwareversion(rtlpriv) >= 0x49) {
		tmp_byte = rtl_read_byte(rtlpriv, FW_RSVD_PG_CRTL) & (~BIT(4));
		tmp_byte = tmp_byte | BIT(5);
		rtl_write_byte(rtlpriv, FW_RSVD_PG_CRTL, tmp_byte);
		rtl_write_dword(rtlpriv, TXDESC_MSK, 0xFFFFCFFF);
	}

	/* We enable high power and RA related mechanism after NIC
	 * initialized. */
	if (hal_get_firmwareversion(rtlpriv) >= 0x35) {
		/* Fw v.53 and later. */
		rtl92s_phy_set_fw_cmd(hw, FW_CMD_RA_INIT);
	} else if (hal_get_firmwareversion(rtlpriv) == 0x34) {
		/* Fw v.52. */
		rtl_write_dword(rtlpriv, WFM5, FW_RA_INIT);
		rtl92s_phy_chk_fwcmd_iodone(hw);
	} else {
		/* Compatible earlier FW version. */
		rtl_write_dword(rtlpriv, WFM5, FW_RA_RESET);
		rtl92s_phy_chk_fwcmd_iodone(hw);
		rtl_write_dword(rtlpriv, WFM5, FW_RA_ACTIVE);
		rtl92s_phy_chk_fwcmd_iodone(hw);
		rtl_write_dword(rtlpriv, WFM5, FW_RA_REFRESH);
		rtl92s_phy_chk_fwcmd_iodone(hw);
	}

	/* Add to prevent ASPM bug. */
	/* Always enable hst and NIC clock request. */
	rtl92s_phy_switch_ephy_parameter(hw);

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
	return err;
}

void rtl92su_set_check_bssid(struct ieee80211_hw *hw, bool check_bssid)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 reg_rcr = rtl_read_dword(rtlpriv, RCR);

	if (rtlpriv->psc.rfpwr_state != ERFON)
		return;

        if (check_bssid) {
		reg_rcr |= RCR_CBSSID;
	} else {
		reg_rcr &= ~RCR_CBSSID;
	}
	rtlpriv->cfg->ops->set_hw_reg(hw, HW_VAR_RCR,
				      (u8 *) (&reg_rcr));
}

/* HW_VAR_MEDIA_STATUS & HW_VAR_CECHK_BSSID */
int rtl92su_set_network_type(struct ieee80211_hw *hw, enum nl80211_iftype type)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	if (rtl92s_set_media_status(hw, type))
		return -EOPNOTSUPP;

	if (rtlpriv->mac80211.link_state == MAC80211_LINKED) {
		if (type != NL80211_IFTYPE_AP)
			rtl92su_set_check_bssid(hw, true);
	} else {
		rtl92su_set_check_bssid(hw, false);
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

	/* we should chnge GPIO to input mode
	 * this will drop away current about 25mA*/
	rtl92s_gpiobit3_cfg_inputmode(hw);

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

void rtl92su_interrupt_recognized(struct ieee80211_hw *hw, u32 *p_inta,
			     u32 *p_intb)
{
}

void rtl92su_update_interrupt_mask(struct ieee80211_hw *hw,
		u32 add_msr, u32 rm_msr)
{
}

bool rtl92su_gpio_radio_on_off_checking(struct ieee80211_hw *hw, u8 *valid)
{
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
