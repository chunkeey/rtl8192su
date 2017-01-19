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
 *
 *****************************************************************************/

#include "../wifi.h"
#include "../core.h"
#include "../base.h"
#include "../usb.h"
#include "../rtl8192s/reg_common.h"
#include "../rtl8192s/def_common.h"
#include "../rtl8192s/phy_common.h"
#include "../rtl8192s/dm_common.h"
#include "../rtl8192s/fw_common.h"
#include "../rtl8192s/hw_common.h"
#include "hw.h"
#include "sw.h"
#include "trx.h"
#include "led.h"

#include <linux/module.h>

#define RTL8192SU_FW_NAME "RTL8192SU/rtl8192sfw.bin"

static int rtl92su_set_tim(struct ieee80211_hw *hw, struct ieee80211_sta *sta,
			   bool set)
{
	return rtl92su_update_beacon(hw);
}

static int rtl92su_init_sw_vars(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	int err = 0;

	rtlpriv->dm.dm_initialgain_enable = true;
	rtlpriv->dm.dm_flag = 0;
	rtlpriv->dm.disable_framebursting = false;
	rtlpriv->dm.thermalvalue = 0;
	rtlpriv->dm.useramask = true;

	rtlpriv->rtlhal.current_bandtype = BAND_ON_2_4G;
	rtlpriv->rtlhal.bandset = BAND_ON_2_4G;
	rtlpriv->rtlhal.macphymode = SINGLEMAC_SINGLEPHY;

	/* for LPS & IPS */
	rtlpriv->psc.inactiveps = rtlpriv->cfg->mod_params->inactiveps;
	rtlpriv->psc.swctrl_lps = rtlpriv->cfg->mod_params->swctrl_lps;
	rtlpriv->psc.fwctrl_lps = rtlpriv->cfg->mod_params->fwctrl_lps;
	rtlpriv->cfg->mod_params->sw_crypto =
		rtlpriv->cfg->mod_params->sw_crypto;
	if (!rtlpriv->psc.inactiveps)
		pr_info("Power Save off (module option)\n");
	if (!rtlpriv->psc.fwctrl_lps)
		pr_info("FW Power Save off (module option)\n");
	rtlpriv->psc.reg_fwctrl_lps = 3;
	rtlpriv->psc.reg_max_lps_awakeintvl = 5;

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

	rtlpriv->max_fw_size = RTL8190_MAX_FIRMWARE_CODE_SIZE * 2 +
			       sizeof(struct fw_hdr);

	pr_info("Driver for Realtek RTL8192SU/RTL8191SU\n"
		"Loading firmware " RTL8192SU_FW_NAME "\n");
	/* request fw */
	err = request_firmware_nowait(THIS_MODULE, 1, RTL8192SU_FW_NAME,
				      rtlpriv->io.dev, GFP_KERNEL, hw,
				      rtl92s_fw_cb);
	if (err) {
		pr_err("Failed to request firmware!\n");
		return err;
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
	.read_eeprom_info = rtl92su_read_eeprom_info,
	.read_chip_version = rtl92s_read_chip_version,
	.hw_init = rtl92su_hw_init,
	.hw_disable = rtl92su_card_disable,
	.set_network_type = rtl92su_set_network_type,
	.set_chk_bssid = rtl92s_set_check_bssid,
	.set_qos = rtl92s_set_qos,
	.set_bcn_reg = rtl92s_set_beacon_related_registers,
	.set_bcn_intv = rtl92s_set_beacon_interval,
	.get_hw_reg = rtl92s_get_hw_reg,
	.set_hw_reg = rtl92s_set_hw_reg,
	.update_rate_tbl = rtl92s_update_hal_rate_tbl,
	.set_channel_access = rtl92s_update_channel_access_setting,
	.radio_onoff_checking = rtl92su_gpio_radio_on_off_checking,
	.set_bw_mode = rtl92s_phy_set_bw_mode,
	.switch_channel = rtl92s_phy_sw_chnl,
	.dm_watchdog = rtl92s_dm_watchdog,
	.scan_operation_backup = rtl92s_phy_scan_operation_backup,
	.set_rf_power_state = rtl92su_phy_set_rf_power_state,
	.led_control = rtl92su_led_control,
	.set_tim = rtl92su_set_tim,

	.enable_hw_sec = rtl92s_enable_hw_security_config,
	.set_key = rtl92s_set_key,
	.init_sw_leds = rtl92su_init_sw_leds,
	.deinit_sw_leds = rtl92su_deinit_sw_leds,
	.get_bbreg = rtl92s_phy_query_bb_reg,
	.set_bbreg = rtl92s_phy_set_bb_reg,
	.get_rfreg = rtl92s_phy_query_rf_reg,
	.set_rfreg = rtl92s_phy_set_rf_reg,

	.query_rx_desc = rtl92su_rx_query_desc,

	.fill_tx_desc = rtl92su_tx_fill_desc,
	.cmd_send_packet = rtl92su_cmd_send_packet,
	.get_btc_status = rtl_btc_status_false,
	.fill_tx_cmddesc = rtl92su_tx_fill_cmddesc,
	.tx_polling = rtl92su_tx_polling,

	.enable_interrupt = rtl92su_enable_interrupt,
	.disable_interrupt = rtl92su_disable_interrupt,
	.update_interrupt_mask = rtl92su_update_interrupt_mask,
};

static struct rtl_mod_params rtl92su_mod_params = {
	.sw_crypto = true,
	.inactiveps = false,
	.fwctrl_lps = false,
	.swctrl_lps = false,
	.debug_level = 0,
	.debug_mask = 0,
};

static struct rtl_hal_usbint_cfg rtl92su_interface_cfg = {
	/* rx */
	.in_ep_num = RTL92S_USB_BULK_IN_NUM,
	.in_ep = RTL92S_USB_BULK_IN_EP,
	.rx_urb_num = RTL92S_NUM_RX_URBS,
	.rx_max_size = RTL92SU_SIZE_MAX_RX_BUFFER,
	.usb_rx_hdl = rtl92su_rx_hdl,
	.usb_rx_segregate_hdl = NULL, /* rtl8192s_rx_segregate_hdl; */
	/* tx */
	.usb_tx_cleanup = rtl92su_tx_cleanup,
	.usb_tx_post_hdl = rtl92su_tx_post_hdl,
	.usb_tx_aggregate_hdl = rtl92su_tx_aggregate_hdl,
	/* endpoint mapping */
	.usb_endpoint_mapping = rtl92su_endpoint_mapping,
	.usb_mq_to_hwq = rtl92su_mq_to_hwq,
};

static struct rtl_hal_cfg rtl92su_hal_cfg = {
	.name = "rtl8192su",
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
	.maps[EFUSE_CLK] = REG_EFUSE_CLK_CTRL,
	.maps[EFUSE_CLK_CTRL] = REG_EFUSE_CLK_CTRL,
	.maps[EFUSE_PWC_EV12V] = 0, /* not used for 8192su */
	.maps[EFUSE_FEN_ELDR] = 0, /* not used for 8192su */
	.maps[EFUSE_LOADER_CLK_EN] = 0,/* not used for 8192su */
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

	.maps[RTL_RC_CCK_RATE1M] = DESC_RATE1M,
	.maps[RTL_RC_CCK_RATE2M] = DESC_RATE2M,
	.maps[RTL_RC_CCK_RATE5_5M] = DESC_RATE5_5M,
	.maps[RTL_RC_CCK_RATE11M] = DESC_RATE11M,
	.maps[RTL_RC_OFDM_RATE6M] = DESC_RATE6M,
	.maps[RTL_RC_OFDM_RATE9M] = DESC_RATE9M,
	.maps[RTL_RC_OFDM_RATE12M] = DESC_RATE12M,
	.maps[RTL_RC_OFDM_RATE18M] = DESC_RATE18M,
	.maps[RTL_RC_OFDM_RATE24M] = DESC_RATE24M,
	.maps[RTL_RC_OFDM_RATE36M] = DESC_RATE36M,
	.maps[RTL_RC_OFDM_RATE48M] = DESC_RATE48M,
	.maps[RTL_RC_OFDM_RATE54M] = DESC_RATE54M,

	.maps[RTL_RC_HT_RATEMCS7] = DESC_RATEMCS7,
	.maps[RTL_RC_HT_RATEMCS15] = DESC_RATEMCS15,
};

#define USB_VENDER_ID_REALTEK		0x0bda

static struct usb_device_id rtl92s_usb_ids[] = {

/* RTL8188SU */
	/* Realtek */
	{RTL_USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8171, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8173, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8712, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8713, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(USB_VENDER_ID_REALTEK, 0xC047, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(USB_VENDER_ID_REALTEK, 0xC512, rtl92su_hal_cfg)},
	/* Abocom */
	{RTL_USB_DEVICE(0x07B8, 0x8188, rtl92su_hal_cfg)},
	/* Accton Technology */
	{RTL_USB_DEVICE(0x083A, 0xC512, rtl92su_hal_cfg)},
	/* Airlive */
	{RTL_USB_DEVICE(0x1B75, 0x8171, rtl92su_hal_cfg)},
	/* ASUS */
	{RTL_USB_DEVICE(0x0B05, 0x1786, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x0B05, 0x1791, rtl92su_hal_cfg)}, /* disable 11n */
	/* Belkin */
	{RTL_USB_DEVICE(0x050D, 0x945A, rtl92su_hal_cfg)},
	/* ISY IWL - Belkin clone */
	{RTL_USB_DEVICE(0x050D, 0x11F1, rtl92su_hal_cfg)},
	/* Corega */
	{RTL_USB_DEVICE(0x07AA, 0x0047, rtl92su_hal_cfg)},
	/* D-Link */
	{RTL_USB_DEVICE(0x2001, 0x3306, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x07D1, 0x3306, rtl92su_hal_cfg)}, /* disable 11n */
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
	{RTL_USB_DEVICE(0x0DF6, 0x0059, rtl92su_hal_cfg)}, /* disable 11n */
	{RTL_USB_DEVICE(0x0DF6, 0x004B, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x0DF6, 0x005B, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x0DF6, 0x005D, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x0DF6, 0x0063, rtl92su_hal_cfg)},
	/* Sweex */
	{RTL_USB_DEVICE(0x177F, 0x0154, rtl92su_hal_cfg)},
	/* Thinkware */
	{RTL_USB_DEVICE(USB_VENDER_ID_REALTEK, 0x5077, rtl92su_hal_cfg)},
	/* Toshiba */
	{RTL_USB_DEVICE(0x1690, 0x0752, rtl92su_hal_cfg)},
	/* ??? */
	{RTL_USB_DEVICE(0x20F4, 0x646B, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x25D4, 0x4CA1, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x25D4, 0x4CAB, rtl92su_hal_cfg)},

/* RTL8191SU */
	/* Realtek */
	{RTL_USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8175, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8172, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8192, rtl92su_hal_cfg)},
	/* Airlive */
	{RTL_USB_DEVICE(0x1b75, 0x8172, rtl92su_hal_cfg)},
	/* Amigo */
	{RTL_USB_DEVICE(0x0EB0, 0x9061, rtl92su_hal_cfg)},
	/* ASUS/EKB */
	{RTL_USB_DEVICE(0x13D3, 0x3323, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x13D3, 0x3311, rtl92su_hal_cfg)}, /* disable 11n */
	{RTL_USB_DEVICE(0x13D3, 0x3342, rtl92su_hal_cfg)},
	/* ASUS/EKBLenovo */
	{RTL_USB_DEVICE(0x13D3, 0x3333, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x13D3, 0x3334, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x13D3, 0x3335, rtl92su_hal_cfg)}, /* disable 11n */
	{RTL_USB_DEVICE(0x13D3, 0x3336, rtl92su_hal_cfg)}, /* disable 11n */
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
	{RTL_USB_DEVICE(0x14B2, 0x3300, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x14B2, 0x3301, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x14B2, 0x3302, rtl92su_hal_cfg)},
	/* Z-Com */
	{RTL_USB_DEVICE(0x0CDE, 0x0030, rtl92su_hal_cfg)},
	/* ??? */
	{RTL_USB_DEVICE(0x04F2, 0xAFF2, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x04F2, 0xAFF5, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x04F2, 0xAFF6, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x13D3, 0x3339, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x13D3, 0x3340, rtl92su_hal_cfg)}, /* disable 11n */
	{RTL_USB_DEVICE(0x13D3, 0x3341, rtl92su_hal_cfg)}, /* disable 11n */
	{RTL_USB_DEVICE(0x13D3, 0x3310, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x13D3, 0x3325, rtl92su_hal_cfg)},

/* RTL8192SU */
	/* Realtek */
	{RTL_USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8174, rtl92su_hal_cfg)},
	/* Belkin */
	{RTL_USB_DEVICE(0x050D, 0x845A, rtl92su_hal_cfg)},
	/* Corega */
	{RTL_USB_DEVICE(0x07AA, 0x0051, rtl92su_hal_cfg)},
	/* Edimax */
	{RTL_USB_DEVICE(0x7392, 0x7622, rtl92su_hal_cfg)},
	/* NEC */
	{RTL_USB_DEVICE(0x0409, 0x02B6, rtl92su_hal_cfg)},
	/* Sitecom */
	{RTL_USB_DEVICE(0x0DF6, 0x0061, rtl92su_hal_cfg)},
	{RTL_USB_DEVICE(0x0DF6, 0x006C, rtl92su_hal_cfg)},

/* Unknown Chips */
	/* Sagemcom */
	{RTL_USB_DEVICE(0x0009, 0x21E7, rtl92su_hal_cfg)},
	/* Amigo */
	{RTL_USB_DEVICE(0x0E0B, 0x9063, rtl92su_hal_cfg)},
	/* Zinwell */
	{RTL_USB_DEVICE(0x5A57, 0x0291, rtl92su_hal_cfg)},
	{}
};

MODULE_DEVICE_TABLE(usb, rtl92s_usb_ids);

static int rtl8192su_probe(struct usb_interface *intf,
			   const struct usb_device_id *id)
{
	return rtl_usb_probe(intf, id, &rtl92su_hal_cfg);
}

static struct usb_driver rtl92su_driver = {
	.name = "rtl8192su",
	.probe = rtl8192su_probe,
	.disconnect = rtl_usb_disconnect,
	.id_table = rtl92s_usb_ids,

#ifdef CONFIG_PM
	/* .suspend = rtl_usb_suspend, */
	/* .resume = rtl_usb_resume, */
	/* .reset_resume = rtl8192c_resume, */
#endif /* CONFIG_PM */
#ifdef CONFIG_AUTOSUSPEND
	.supports_autosuspend = 1,
#endif
};

module_usb_driver(rtl92su_driver);

MODULE_AUTHOR("lizhaoming		<chaoming_li@realsil.com.cn>");
MODULE_AUTHOR("Realtek WlanFAE		<wlanfae@realtek.com>");
MODULE_AUTHOR("Larry Finger		<Larry.Finger@lwfinger.net>");
MODULE_AUTHOR("Joshua Roys		<Joshua.Roys@gtri.gatech.edu>");
MODULE_AUTHOR("Christian Lamparter	<chunkeey@googlemail.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Realtek 8188S/8191S/8192S 802.11n USB wireless");
MODULE_FIRMWARE(RTL8192SU_FW_NAME);

module_param_named(swenc, rtl92su_mod_params.sw_crypto, bool, 0444);
module_param_named(debug_level, rtl92su_mod_params.debug_level, int, 0644);
module_param_named(debug_mask, rtl92su_mod_params.debug_mask, ullong, 0644);

MODULE_PARM_DESC(swenc, "Set to 0 for hardware crypto (default 1)\n");
MODULE_PARM_DESC(debug, "Set debug level (0-5) (default 0)");
MODULE_PARM_DESC(swlps, "Set to 1 to use SW control power save (default 0)\n");
MODULE_PARM_DESC(fwlps, "Set to 1 to use FW control power save (default 0)\n");
