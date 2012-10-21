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
 * Joshua Roys <Joshua.Roys@gtri.gatech.edu>
 *
 *****************************************************************************/

#include "../wifi.h"
#include "../core.h"
#include "../usb.h"
#include "../base.h"
#include "../efuse.h"
#include "reg.h"
#include "def.h"
#include "phy.h"
#include "dm.h"
#include "fw.h"
#include "hw.h"
#include "sw.h"
#include "trx.h"
#include "led.h"
#include "eeprom.h"

#include <linux/module.h>

MODULE_AUTHOR("lizhaoming	<chaoming_li@realsil.com.cn>");
MODULE_AUTHOR("Realtek WlanFAE	<wlanfae@realtek.com>");
MODULE_AUTHOR("Larry Finger	<Larry.Finger@lwfinger.net>");
MODULE_AUTHOR("Joshua Roys      <Joshua.Roys@gtri.gatech.edu>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Realtek 8192S/8191S 802.11n USB wireless");
MODULE_FIRMWARE("rtlwifi/rtl8712u.bin");

static void rtl92su_fw_cb(const struct firmware *firmware, void *context)
{
	struct ieee80211_hw *hw = context;
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rt_firmware *pfirmware = NULL;
	int err;

	RT_TRACE(rtlpriv, COMP_ERR, DBG_LOUD,
			 "Firmware callback routine entered!\n");
	complete(&rtlpriv->firmware_loading_complete);
	if (!firmware) {
		pr_err("Firmware %s not available\n", rtlpriv->cfg->fw_name);
		rtlpriv->max_fw_size = 0;
		return;
	}
	if (firmware->size > rtlpriv->max_fw_size) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "Firmware is too big!\n");
		rtlpriv->max_fw_size = 0;
		release_firmware(firmware);
		return;
	}
	pfirmware = (struct rt_firmware *)rtlpriv->rtlhal.pfirmware;
	memcpy(pfirmware->sz_fw_tmpbuffer, firmware->data, firmware->size);
	pfirmware->sz_fw_tmpbufferlen = firmware->size;
	release_firmware(firmware);

	err = ieee80211_register_hw(hw);
	if (err) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "Can't register mac80211 hw\n");
		return;
	} else {
		rtlpriv->mac80211.mac80211_registered = 1;
	}
	set_bit(RTL_STATUS_INTERFACE_START, &rtlpriv->status);

	/*init rfkill */
	rtl_init_rfkill(hw);
}

static int rtl92su_init_sw_vars(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	struct rtl_usb *rtlusb = rtl_usbdev(rtl_usbpriv(hw));
	int err = 0;
	u16 earlyrxthreshold = 7;

	hw->wiphy->max_scan_ssids = 1;

	rtlpriv->dm.dm_initialgain_enable = true;
	rtlpriv->dm.dm_flag = 0;
	rtlpriv->dm.disable_framebursting = false;
	rtlpriv->dm.thermalvalue = 0;
	rtlpriv->dm.useramask = true;
	rtlpriv->dbg.global_debuglevel = rtlpriv->cfg->mod_params->debug;

	/* compatible 5G band 91su just 2.4G band & smsp */
	rtlpriv->rtlhal.current_bandtype = BAND_ON_2_4G;
	rtlpriv->rtlhal.bandset = BAND_ON_2_4G;
	rtlpriv->rtlhal.macphymode = SINGLEMAC_SINGLEPHY;

	mac->rx_conf =
			RCR_APPFCS |
			RCR_APWRMGT |
			/*RCR_ADD3 |*/
			RCR_ACF	|
			RCR_AMF	|
			RCR_ADF |
			RCR_APP_MIC |
			RCR_APP_ICV |
			RCR_AICV |
			/* Accept ICV error, CRC32 Error */
			RCR_ACRC32 |
			RCR_AB |
			/* Accept Broadcast, Multicast */
			RCR_AM	|
			/* Accept Physical match */
			RCR_APM |
			/* Accept Destination Address packets */
			/*RCR_AAP |*/
			RCR_APP_PHYST_STAFF |
			/* Accept PHY status */
			RCR_APP_PHYST_RXFF |
			RCR_RX_TCPOFDL_EN |
			(earlyrxthreshold << RCR_FIFO_OFFSET);

	rtlusb->irq_mask[0] = 0;
	rtlusb->irq_mask[1] = (u32) 0;

	/* for LPS & IPS */
	rtlpriv->psc.inactiveps = rtlpriv->cfg->mod_params->inactiveps;
	rtlpriv->psc.swctrl_lps = rtlpriv->cfg->mod_params->swctrl_lps;
	rtlpriv->psc.fwctrl_lps = rtlpriv->cfg->mod_params->fwctrl_lps;
	if (!rtlpriv->psc.inactiveps)
		pr_info("Power Save off (module option)\n");
	if (!rtlpriv->psc.fwctrl_lps)
		pr_info("FW Power Save off (module option)\n");
	rtlpriv->psc.reg_fwctrl_lps = 3;
	rtlpriv->psc.reg_max_lps_awakeintvl = 5;
	rtlpriv->psc.hwradiooff = false;

	if (rtlpriv->psc.reg_fwctrl_lps == 1)
		rtlpriv->psc.fwctrl_psmode = FW_PS_MIN_MODE;
	else if (rtlpriv->psc.reg_fwctrl_lps == 2)
		rtlpriv->psc.fwctrl_psmode = FW_PS_MAX_MODE;
	else if (rtlpriv->psc.reg_fwctrl_lps == 3)
		rtlpriv->psc.fwctrl_psmode = FW_PS_DTIM_MODE;

	/* for firmware buf */
	rtlpriv->rtlhal.pfirmware = vzalloc(sizeof(struct rt_firmware));
	if (!rtlpriv->rtlhal.pfirmware)
		return -ENOMEM;

	rtlpriv->max_fw_size = RTL8192_MAX_RAW_FIRMWARE_CODE_SIZE;

	pr_info("Driver for Realtek RTL8192SU/RTL8191SU/RTL8188SU\n"
		"Loading firmware %s\n", rtlpriv->cfg->fw_name);
	/* request fw */
	err = request_firmware_nowait(THIS_MODULE, 1, rtlpriv->cfg->fw_name,
				      rtlpriv->io.dev, GFP_KERNEL, hw,
				      rtl92su_fw_cb);
	if (err) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "Failed to request firmware!\n");
		return 1;
	}

	return err;
}

static void rtl92su_deinit_sw_vars(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	if (rtlpriv->rtlhal.pfirmware) {
		vfree(rtlpriv->rtlhal.pfirmware);
		rtlpriv->rtlhal.pfirmware = NULL;
	}
}

static struct rtl_hal_ops rtl8192su_hal_ops = {
	.init_sw_vars = rtl92su_init_sw_vars,
	.deinit_sw_vars = rtl92su_deinit_sw_vars,
	.read_chip_version = rtl92su_read_chip_version,
	.read_eeprom_info = rtl92su_read_eeprom_info,
	.interrupt_recognized = rtl92su_interrupt_recognized,
	.hw_init = rtl92su_hw_init,
	.hw_disable = rtl92su_card_disable,
	.enable_interrupt = rtl92su_enable_interrupt,
	.disable_interrupt = rtl92su_disable_interrupt,
	.set_network_type = rtl92su_set_network_type,
	.set_chk_bssid = rtl92su_set_check_bssid,
	.set_qos = rtl92su_set_qos,
	.set_bcn_reg = rtl92su_set_beacon_related_registers,
	.set_bcn_intv = rtl92su_set_beacon_interval,
	.update_interrupt_mask = rtl92su_update_interrupt_mask,
	.get_hw_reg = rtl92su_get_hw_reg,
	.set_hw_reg = rtl92su_set_hw_reg,
	.update_rate_tbl = rtl92su_update_hal_rate_tbl,
	.fill_tx_desc = rtl92su_tx_fill_desc,
	.fill_tx_cmddesc = rtl92su_tx_fill_cmddesc,
	.query_rx_desc = rtl92su_rx_query_desc,
	.set_channel_access = rtl92su_update_channel_access_setting,
	.radio_onoff_checking = rtl92su_gpio_radio_on_off_checking,
	.set_bw_mode = rtl92s_phy_set_bw_mode,
	.switch_channel = rtl92s_set_fw_setchannel_cmd,
	.dm_watchdog = rtl92s_dm_watchdog,
	.scan_operation_backup = rtl92s_phy_scan_operation_backup,
	.set_rf_power_state = rtl92s_phy_set_rf_power_state,
	.led_control = rtl92su_led_control,
	.enable_hw_sec = rtl92su_enable_hw_security_config,
	.set_key = rtl92su_set_key,
	.init_sw_leds = rtl92su_init_sw_leds,
	.deinit_sw_leds = rtl92su_deinit_sw_leds,
	.get_bbreg = rtl92s_phy_query_bb_reg,
	.set_bbreg = rtl92s_phy_set_bb_reg,
	.get_rfreg = rtl92s_phy_query_rf_reg,
	.set_rfreg = rtl92s_phy_set_rf_reg,
};

static struct rtl_mod_params rtl92su_mod_params = {
	.sw_crypto = true,
	.inactiveps = false,
	.swctrl_lps = false,
	.fwctrl_lps = true,
	.debug = DBG_EMERG,
};

static struct rtl_hal_usbint_cfg rtl92su_interface_cfg = {
	/* rx */
	.in_ep_num = RTL92S_USB_BULK_IN_NUM,
	.in_ep = RTL92S_USB_BULK_IN_EP,
	.rx_urb_num = RTL92S_NUM_RX_URBS,
	.rx_max_size = RTL92SU_SIZE_MAX_RX_BUFFER,
	.usb_rx_hdl = rtl8192su_rx_hdl,
	.usb_rx_segregate_hdl = NULL, /* rtl8192s_rx_segregate_hdl; */
	/* tx */
	.usb_tx_cleanup = rtl8192s_tx_cleanup,
	.usb_tx_post_hdl = rtl8192s_tx_post_hdl,
	.usb_tx_aggregate_hdl = rtl8192s_tx_aggregate_hdl,
	/* endpoint mapping */
	.usb_endpoint_mapping = rtl8192su_endpoint_mapping,
	.usb_mq_to_hwq = rtl8192su_mq_to_hwq,
};

static struct rtl_hal_cfg rtl92su_hal_cfg = {
	.name = "rtl92s_usb",
	.fw_name = "rtlwifi/rtl8712u.bin",
	.ops = &rtl8192su_hal_ops,
	.mod_params = &rtl92su_mod_params,
	.usb_interface_cfg = &rtl92su_interface_cfg,

	.maps[SYS_ISO_CTRL] = REG_SYS_ISO_CTRL,
	.maps[SYS_FUNC_EN] = REG_SYS_FUNC_EN,
	.maps[SYS_CLK] = REG_SYS_CLKR,
	.maps[MAC_RCR_AM] = RCR_AM,
	.maps[MAC_RCR_AB] = RCR_AB,
	.maps[MAC_RCR_ACRC32] = RCR_ACRC32,
	.maps[MAC_RCR_ACF] = RCR_ACF,
	.maps[MAC_RCR_AAP] = RCR_AAP,

	.maps[EFUSE_TEST] = REG_EFUSE_TEST,
	.maps[EFUSE_CTRL] = REG_EFUSE_CTRL,
	.maps[EFUSE_CLK] = REG_EFUSE_CLK,
	.maps[EFUSE_CLK_CTRL] = REG_EFUSE_CLK_CTRL,
	.maps[EFUSE_PWC_EV12V] = 0,
	.maps[EFUSE_FEN_ELDR] = 0,
	.maps[EFUSE_LOADER_CLK_EN] = 0,
	.maps[EFUSE_ANA8M] = EFUSE_ANA8M,
	.maps[EFUSE_HWSET_MAX_SIZE] = HWSET_MAX_SIZE_92S,
	.maps[EFUSE_MAX_SECTION_MAP] = EFUSE_MAX_SECTION,
	.maps[EFUSE_REAL_CONTENT_SIZE] = EFUSE_REAL_CONTENT_LEN,
	.maps[EFUSE_OOB_PROTECT_BYTES_LEN] = EFUSE_OOB_PROTECT_BYTES,

	.maps[RWCAM] = REG_RWCAM,
	.maps[WCAMI] = REG_WCAMI,
	.maps[RCAMO] = REG_RCAMO,
	.maps[CAMDBG] = REG_CAMDBG,
	.maps[SECR] = REG_SECR,
	.maps[SEC_CAM_NONE] = CAM_NONE,
	.maps[SEC_CAM_WEP40] = CAM_WEP40,
	.maps[SEC_CAM_TKIP] = CAM_TKIP,
	.maps[SEC_CAM_AES] = CAM_AES,
	.maps[SEC_CAM_WEP104] = CAM_WEP104,

	.maps[RTL_RC_CCK_RATE1M] = DESC92_RATE1M,
	.maps[RTL_RC_CCK_RATE2M] = DESC92_RATE2M,
	.maps[RTL_RC_CCK_RATE5_5M] = DESC92_RATE5_5M,
	.maps[RTL_RC_CCK_RATE11M] = DESC92_RATE11M,
	.maps[RTL_RC_OFDM_RATE6M] = DESC92_RATE6M,
	.maps[RTL_RC_OFDM_RATE9M] = DESC92_RATE9M,
	.maps[RTL_RC_OFDM_RATE12M] = DESC92_RATE12M,
	.maps[RTL_RC_OFDM_RATE18M] = DESC92_RATE18M,
	.maps[RTL_RC_OFDM_RATE24M] = DESC92_RATE24M,
	.maps[RTL_RC_OFDM_RATE36M] = DESC92_RATE36M,
	.maps[RTL_RC_OFDM_RATE48M] = DESC92_RATE48M,
	.maps[RTL_RC_OFDM_RATE54M] = DESC92_RATE54M,

	.maps[RTL_RC_HT_RATEMCS7] = DESC92_RATEMCS7,
	.maps[RTL_RC_HT_RATEMCS15] = DESC92_RATEMCS15,
};

/*
 * Currently we only support sw crypto. Once the HW crypto
 * is implemented this comment can be removed and the setting
 * can be configurable.
 *
 * module_param_named(swenc, rtl92su_mod_params.sw_crypto, bool, 0444);
 * MODULE_PARM_DESC(swenc, "Set to 1 for software crypto (default 1)");
 */
module_param_named(debug, rtl92su_mod_params.debug, int, 0444);
MODULE_PARM_DESC(debug, "Set debug level (0-5) (default 0)");

#define USB_VENDER_ID_REALTEK		0x0bda

static struct usb_device_id rtl8192s_usb_ids[] = {

/* RTL8188SU */
	/* Realtek */
	{RTL_USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8171, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8173, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8712, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8713, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(USB_VENDER_ID_REALTEK, 0xC512, rtl92su_hal_cfg)},
	/* Abocom */
	{RTL_USB_DEVICE(0x07B8, 0x8188, rtl92su_hal_cfg)},
	/* ASUS */
	{RTL_USB_DEVICE(0x0B05, 0x1786, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x0B05, 0x1791, rtl92su_hal_cfg)}, /* 11n mode disable */
	/* Belkin */
	{RTL_USB_DEVICE(0x050D, 0x945A, rtl92su_hal_cfg)},
	/* Corega */
	{RTL_USB_DEVICE(0x07AA, 0x0047, rtl92su_hal_cfg)},
	/* D-Link */
	{RTL_USB_DEVICE(0x2001, 0x3306, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x07D1, 0x3306, rtl92su_hal_cfg)}, /* 11n mode disable */
	/* Edimax */
	{RTL_USB_DEVICE(0x7392, 0x7611, rtl92su_hal_cfg)},
	/* EnGenius */
	{RTL_USB_DEVICE(0x1740, 0x9603, rtl92su_hal_cfg)},
	/* Hawking */
	{RTL_USB_DEVICE(0x0E66, 0x0016, rtl92su_hal_cfg)},
	/* Hercules */
	{RTL_USB_DEVICE(0x06F8, 0xE034, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x06F8, 0xE032, rtl92su_hal_cfg)},
	/* Logitec */
	{RTL_USB_DEVICE(0x0789, 0x0167, rtl92su_hal_cfg)},
	/* PCI */
	{RTL_USB_DEVICE(0x2019, 0xAB28, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x2019, 0xED16, rtl92su_hal_cfg)},
	/* Sitecom */
	{RTL_USB_DEVICE(0x0DF6, 0x0057, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x0DF6, 0x0045, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x0DF6, 0x0059, rtl92su_hal_cfg)}, /* 11n mode disable */
	{RTL_USB_DEVICE(0x0DF6, 0x004B, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x0DF6, 0x005B, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x0DF6, 0x005D, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x0DF6, 0x0063, rtl92su_hal_cfg)},
	/* Sweex */
	{RTL_USB_DEVICE(0x177F, 0x0154, rtl92su_hal_cfg)},
	/* Thinkware */
	{RTL_USB_DEVICE(0x0BDA, 0x5077, rtl92su_hal_cfg)},
	/* Toshiba */
	{RTL_USB_DEVICE(0x1690, 0x0752, rtl92su_hal_cfg)},
	/* - */
	{RTL_USB_DEVICE(0x20F4, 0x646B, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x083A, 0xC512, rtl92su_hal_cfg)},

/* RTL8191SU */
	/* Realtek */
	{RTL_USB_DEVICE(0x0BDA, 0x8172, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x0BDA, 0x8192, rtl92su_hal_cfg)},
	/* Amigo */
	{RTL_USB_DEVICE(0x0EB0, 0x9061, rtl92su_hal_cfg)},
	/* ASUS/EKB */
	{RTL_USB_DEVICE(0x13D3, 0x3323, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x13D3, 0x3311, rtl92su_hal_cfg)}, /* 11n mode disable */
	{RTL_USB_DEVICE(0x13D3, 0x3342, rtl92su_hal_cfg)},
	/* ASUS/EKBLenovo */
	{RTL_USB_DEVICE(0x13D3, 0x3333, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x13D3, 0x3334, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x13D3, 0x3335, rtl92su_hal_cfg)}, /* 11n mode disable */
	{RTL_USB_DEVICE(0x13D3, 0x3336, rtl92su_hal_cfg)}, /* 11n mode disable */
	/* ASUS/Media BOX */
	{RTL_USB_DEVICE(0x13D3, 0x3309, rtl92su_hal_cfg)},
	/* Belkin */
	{RTL_USB_DEVICE(0x050D, 0x815F, rtl92su_hal_cfg)},
	/* D-Link */
	{RTL_USB_DEVICE(0x07D1, 0x3302, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x07D1, 0x3300, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x07D1, 0x3303, rtl92su_hal_cfg)},
	/* Edimax */
	{RTL_USB_DEVICE(0x7392, 0x7612, rtl92su_hal_cfg)},
	/* EnGenius */
	{RTL_USB_DEVICE(0x1740, 0x9605, rtl92su_hal_cfg)},
	/* Guillemot */
	{RTL_USB_DEVICE(0x06F8, 0xE031, rtl92su_hal_cfg)},
	/* Hawking */
	{RTL_USB_DEVICE(0x0E66, 0x0015, rtl92su_hal_cfg)},
	/* Mediao */
	{RTL_USB_DEVICE(0x13D3, 0x3306, rtl92su_hal_cfg)},
	/* PCI */
	{RTL_USB_DEVICE(0x2019, 0xED18, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x2019, 0x4901, rtl92su_hal_cfg)},
	/* Sitecom */
	{RTL_USB_DEVICE(0x0DF6, 0x0058, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x0DF6, 0x0049, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x0DF6, 0x004C, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x0DF6, 0x0064, rtl92su_hal_cfg)},
	/* Skyworth */
	{RTL_USB_DEVICE(0x14b2, 0x3300, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x14b2, 0x3301, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x14B2, 0x3302, rtl92su_hal_cfg)},
	/* - */
	{RTL_USB_DEVICE(0x04F2, 0xAFF2, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x04F2, 0xAFF5, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x04F2, 0xAFF6, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x13D3, 0x3339, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x13D3, 0x3340, rtl92su_hal_cfg)}, /* 11n mode disable */
	{RTL_USB_DEVICE(0x13D3, 0x3341, rtl92su_hal_cfg)}, /* 11n mode disable */
	{RTL_USB_DEVICE(0x13D3, 0x3310, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x13D3, 0x3325, rtl92su_hal_cfg)},

/* RTL8192SU */
	/* Realtek */
	{RTL_USB_DEVICE(0x0BDA, 0x8174, rtl92su_hal_cfg)},
	/* Belkin */
	{RTL_USB_DEVICE(0x050D, 0x845A, rtl92su_hal_cfg)},
	/* Corega */
	{RTL_USB_DEVICE(0x07AA, 0x0051, rtl92su_hal_cfg)},
	/* Edimax */
	{RTL_USB_DEVICE(0x7392, 0x7622, rtl92su_hal_cfg)},
	/* NEC */
	{RTL_USB_DEVICE(0x0409, 0x02B6, rtl92su_hal_cfg)},
	{}
};

MODULE_DEVICE_TABLE(usb, rtl8192s_usb_ids);

static struct usb_driver rtl8192su_driver = {
	.name = "rtl8192su",
	.probe = rtl_usb_probe,
	.disconnect = rtl_usb_disconnect,
	.id_table = rtl8192s_usb_ids,

#ifdef CONFIG_PM
	/* .suspend = rtl_usb_suspend, */
	/* .resume = rtl_usb_resume, */
	/* .reset_resume = rtl8192c_resume, */
#endif /* CONFIG_PM */
#ifdef CONFIG_AUTOSUSPEND
	.supports_autosuspend = 1,
#endif
};

module_usb_driver(rtl8192su_driver);
