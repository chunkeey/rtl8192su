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
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/

#include "../wifi.h"
#include "../pci.h"
#include "../usb.h"
#include "../base.h"
#include "reg_common.h"
#include "def_common.h"
#include "fw_common.h"

void rtl92s_fw_cb(const struct firmware *firmware, void *context)
{
	struct ieee80211_hw *hw = context;
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rt_firmware *pfirmware = NULL;

	RT_TRACE(rtlpriv, COMP_ERR, DBG_LOUD,
			 "Firmware callback routine entered!\n");
	complete(&rtlpriv->firmware_loading_complete);
	if (!firmware) {
		pr_err("Firmware not available\n");
		goto bad_fw;
	}
	if (firmware->size > rtlpriv->max_fw_size) {
		pr_err("Firmware is too big!\n");
		goto bad_fw;
	}

	pfirmware = (struct rt_firmware *)rtlpriv->rtlhal.pfirmware;
	memcpy(pfirmware->sz_fw_tmpbuffer, firmware->data, firmware->size);
	pfirmware->sz_fw_tmpbufferlen = firmware->size;
	release_firmware(firmware);
	return;

bad_fw:
	rtlpriv->max_fw_size = 0;
	release_firmware(firmware);
	rtlpriv->intf_ops->adapter_unbind(hw);
}
EXPORT_SYMBOL_GPL(rtl92s_fw_cb);

static void _rtl92s_fw_set_rqpn(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));

	/* For 92SE only */
	if (IS_HARDWARE_TYPE_8192SE(rtlhal)) {
		rtl_write_dword(rtlpriv, RQPN, 0xffffffff);
		rtl_write_dword(rtlpriv, RQPN + 4, 0xffffffff);
		rtl_write_byte(rtlpriv, RQPN + 8, 0xff);
		rtl_write_byte(rtlpriv, RQPN + 0xB, 0x80);
	}
}

static bool _rtl92s_firmware_enable_cpu(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 ichecktime = 10;
	u16 tmpu2b;
	u8 tmpu1b, cpustatus = 0;

	_rtl92s_fw_set_rqpn(hw);

	/* Enable CPU. */
	tmpu1b = rtl_read_byte(rtlpriv, SYS_CLKR);
	/* AFE source */
	rtl_write_byte(rtlpriv, SYS_CLKR, (tmpu1b | SYS_CPU_CLKSEL));

	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_FUNC_EN);
	rtl_write_word(rtlpriv, REG_SYS_FUNC_EN, (tmpu2b | FEN_CPUEN));

	/* Polling IMEM Ready after CPU has refilled. */
	do {
		cpustatus = rtl_read_byte(rtlpriv, TCR);
		if (cpustatus & IMEM_RDY) {
			RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
				 "IMEM Ready after CPU has refilled\n");
			break;
		}

		msleep(20);
	} while (ichecktime--);

	if (!(cpustatus & IMEM_RDY))
		return false;

	return true;
}

static enum fw_status _rtl92s_firmware_get_nextstatus(
		enum fw_status fw_currentstatus)
{
	enum fw_status	next_fwstatus = 0;

	switch (fw_currentstatus) {
	case FW_STATUS_INIT:
		next_fwstatus = FW_STATUS_LOAD_IMEM;
		break;
	case FW_STATUS_LOAD_IMEM:
		next_fwstatus = FW_STATUS_LOAD_EMEM;
		break;
	case FW_STATUS_LOAD_EMEM:
		next_fwstatus = FW_STATUS_LOAD_DMEM;
		break;
	case FW_STATUS_LOAD_DMEM:
		next_fwstatus = FW_STATUS_READY;
		break;
	default:
		break;
	}

	return next_fwstatus;
}

static u8 _rtl92s_firmware_header_map_rftype(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &(rtlpriv->phy);

	switch (rtlphy->rf_type) {
	case RF_1T1R:
		return 0x11;
	case RF_1T2R:
		return 0x12;
	case RF_2T2R:
		return 0x22;
	default:
		pr_err("Unknown RF type(%x)\n", rtlphy->rf_type);
		break;
	}
	return 0x22;
}

static void _rtl92s_firmwareheader_priveupdate(struct ieee80211_hw *hw,
		struct fw_priv *pfw_priv)
{
	/* Update RF types for RATR settings. */
	pfw_priv->rf_config = _rtl92s_firmware_header_map_rftype(hw);
}

static uint8_t _rtl92s_get_fw_queue(struct rtl_priv *rtlpriv)
{
	struct rtl_hal *rtlhal = rtl_hal(rtlpriv);

	if (IS_HARDWARE_TYPE_8192SE(rtlhal))
		return TXCMD_QUEUE;
	else if (IS_HARDWARE_TYPE_8192SU(rtlhal))
		return RTL_TXQ_VO;
	return TXCMD_QUEUE;
}

static bool _rtl92s_firmware_downloadcode(struct ieee80211_hw *hw,
		u8 *code_virtual_address, u32 buffer_len)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct sk_buff *skb;
	struct rtl_tcb_desc *tcb_desc;
	unsigned char *seg_ptr;
	u16 frag_threshold = MAX_FIRMWARE_CODE_SIZE;
	u16 frag_length, frag_offset = 0;
	u16 extra_descoffset = 0;
	u8 last_inipkt = 0;

	_rtl92s_fw_set_rqpn(hw);

	if (buffer_len >= MAX_FIRMWARE_CODE_SIZE) {
		pr_err("Size over FIRMWARE_CODE_SIZE!\n");
		return false;
	}

	extra_descoffset = 0;

	do {
		if ((buffer_len - frag_offset) > frag_threshold) {
			frag_length = frag_threshold + extra_descoffset;
		} else {
			frag_length = (u16)(buffer_len - frag_offset +
					    extra_descoffset);
			last_inipkt = 1;
		}

		/* Allocate skb buffer to contain firmware */
		/* info and tx descriptor info. */
		skb = dev_alloc_skb(frag_length);
		if (!skb)
			return false;
		skb_reserve(skb, extra_descoffset);
		seg_ptr = (u8 *)skb_put(skb, (u32)(frag_length -
					extra_descoffset));
		memcpy(seg_ptr, code_virtual_address + frag_offset,
		       (u32)(frag_length - extra_descoffset));

		tcb_desc = (struct rtl_tcb_desc *)(skb->cb);
		tcb_desc->cmd_or_init = DESC_PACKET_TYPE_INIT;
		tcb_desc->queue_index = _rtl92s_get_fw_queue(rtlpriv);
		tcb_desc->last_inipkt = last_inipkt;

		rtlpriv->cfg->ops->cmd_send_packet(hw, skb);

		frag_offset += (frag_length - extra_descoffset);

	} while (frag_offset < buffer_len);

	rtl_write_byte(rtlpriv, TP_POLL, TPPOLL_CQ);

	return true;
}

static bool _rtl92s_firmware_checkready(struct ieee80211_hw *hw,
		u8 loadfw_status)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	struct rt_firmware *firmware = (struct rt_firmware *)rtlhal->pfirmware;
	u32 tmpu4b;
	u8 cpustatus = 0;
	short pollingcnt = 10;
	bool rtstatus = true;

	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "LoadStaus(%d)\n", loadfw_status);

	firmware->fwstatus = (enum fw_status)loadfw_status;

	switch (loadfw_status) {
	case FW_STATUS_LOAD_IMEM:
		/* Polling IMEM code done. */
		do {
			cpustatus = rtl_read_byte(rtlpriv, TCR);
			if (cpustatus & IMEM_CODE_DONE)
				break;
			msleep(20);
		} while (--pollingcnt);

		if (!(cpustatus & IMEM_CHK_RPT) || (pollingcnt <= 0)) {
			pr_err("FW_STATUS_LOAD_IMEM FAIL CPU, Status=%x\n",
			       cpustatus);
			goto status_check_fail;
		}
		break;

	case FW_STATUS_LOAD_EMEM:
		/* Check Put Code OK and Turn On CPU */
		/* Polling EMEM code done. */
		do {
			cpustatus = rtl_read_byte(rtlpriv, TCR);
			if (cpustatus & EMEM_CODE_DONE)
				break;
			msleep(20);
		} while (pollingcnt--);

		if (!(cpustatus & EMEM_CHK_RPT) || (pollingcnt <= 0)) {
			pr_err("FW_STATUS_LOAD_EMEM FAIL CPU, Status=%x\n",
			       cpustatus);
			goto status_check_fail;
		}

		/* Turn On CPU */
		rtstatus = _rtl92s_firmware_enable_cpu(hw);
		if (!rtstatus) {
			pr_err("Enable CPU fail!\n");
			goto status_check_fail;
		}
		break;

	case FW_STATUS_LOAD_DMEM:
		/* Polling DMEM code done */
		do {
			cpustatus = rtl_read_byte(rtlpriv, TCR);
			if (cpustatus & DMEM_CODE_DONE)
				break;
			msleep(20);
		} while (pollingcnt--);

		if (!(cpustatus & DMEM_CODE_DONE) || (pollingcnt <= 0)) {
			pr_err("Polling DMEM code done fail ! cpustatus(%#x)\n",
			       cpustatus);
			goto status_check_fail;
		}

		RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
			 "DMEM code download success, cpustatus(%#x)\n",
			 cpustatus);

		if (rtl_read_byte(rtlpriv, REG_EEPROM_CMD) &
		    EEPROM_CMD_93C46) {
			/* When booting from the eeprom, the firmware
			 * needs more time to complete the boot procedure. */
			pollingcnt = 60;
		} else {
			/* Boot from eefuse is faster */
			pollingcnt = 30;
		}

		do {
			cpustatus = rtl_read_byte(rtlpriv, TCR);
			if (cpustatus & FWRDY)
				break;
			msleep(100);
		} while (--pollingcnt);

		RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
			 "Polling Load Firmware ready, cpustatus(%x)\n",
			 cpustatus);

		if (((cpustatus & LOAD_FW_READY) != LOAD_FW_READY) ||
		    (pollingcnt <= 0)) {
			pr_err("Polling Load Firmware ready fail ! cpustatus(%x)\n",
			       cpustatus);
			goto status_check_fail;
		}

		/* If right here, we can set TCR/RCR to desired value  */
		/* and config MAC lookback mode to normal mode */
		tmpu4b = rtl_read_dword(rtlpriv, TCR);
		rtl_write_dword(rtlpriv, TCR, (tmpu4b & (~TCR_ICV)));

		tmpu4b = rtl_read_dword(rtlpriv, RCR);
		rtl_write_dword(rtlpriv, RCR, (tmpu4b | RCR_APPFCS |
				RCR_APP_ICV | RCR_APP_MIC));

		RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
			 "Current RCR settings(%#x)\n", tmpu4b);

		/* Set to normal mode. */
		rtl_write_byte(rtlpriv, LBKMD_SEL, LBK_NORMAL);
		break;

	default:
		pr_err("Unknown status check!\n");
		rtstatus = false;
		break;
	}

status_check_fail:
	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "loadfw_status(%d), rtstatus(%x)\n",
		 loadfw_status, rtstatus);
	return rtstatus;
}

int rtl92s_download_fw(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	struct rt_firmware *firmware = NULL;
	struct fw_hdr *pfwheader;
	struct fw_priv *pfw_priv = NULL;
	u8 *puc_mappedfile = NULL;
	u32 ul_filelength = 0;
	u8 fwhdr_size = RT_8192S_FIRMWARE_HDR_SIZE;
	u8 fwstatus = FW_STATUS_INIT;
	bool rtstatus = true;

	if (rtlpriv->max_fw_size == 0 || !rtlhal->pfirmware)
		return 1;

	firmware = (struct rt_firmware *)rtlhal->pfirmware;
	firmware->fwstatus = FW_STATUS_INIT;

	puc_mappedfile = firmware->sz_fw_tmpbuffer;

	/* 1. Retrieve FW header. */
	firmware->pfwheader = (struct fw_hdr *) puc_mappedfile;
	pfwheader = firmware->pfwheader;
	firmware->firmwareversion = le16_to_cpu(pfwheader->version) & 0xff;

	if (IS_HARDWARE_TYPE_8192SE(rtlhal)) {
		pfwheader->fwpriv.hci_sel = RTL8712_HCI_TYPE_PCIE;
	} else if (IS_HARDWARE_TYPE_8192SU(rtlhal)) {
		pfwheader->fwpriv.hci_sel = RTL8712_HCI_TYPE_USB;
		pfwheader->fwpriv.usb_ep_num =
			rtl_usbdev(rtl_usbpriv(rtlpriv))->epnums;
		pfwheader->fwpriv.beacon_offload = 2; /* BCNOFFLOAD_FW */
	} else {
		pr_err("unsupported device\n");
		return -ENODEV;
	}

	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "signature:%x, version:%x, size:%x, imemsize:%x, sram size:%x\n",
		 le16_to_cpu(pfwheader->signature),
		 le16_to_cpu(pfwheader->version),
		 le32_to_cpu(pfwheader->dmem_size),
		 le32_to_cpu(pfwheader->img_imem_size),
		 le32_to_cpu(pfwheader->img_sram_size));

	/* 2. Retrieve IMEM image. */
	if ((pfwheader->img_imem_size == 0) ||
	    (le32_to_cpu(pfwheader->img_imem_size) >
	    sizeof(firmware->fw_imem))) {
		pr_err("memory for data image is less than IMEM required\n");
		goto fail;
	} else {
		puc_mappedfile += fwhdr_size;

		memcpy(firmware->fw_imem, puc_mappedfile,
		       le32_to_cpu(pfwheader->img_imem_size));
		firmware->fw_imem_len = le32_to_cpu(pfwheader->img_imem_size);
	}

	/* 3. Retriecve EMEM image. */
	if (le32_to_cpu(pfwheader->img_sram_size) >
	    sizeof(firmware->fw_emem)) {
		pr_err("memory for data image is less than EMEM required\n");
		goto fail;
	} else {
		puc_mappedfile += firmware->fw_imem_len;

		memcpy(firmware->fw_emem, puc_mappedfile,
		       le32_to_cpu(pfwheader->img_sram_size));

		firmware->fw_emem_len = le32_to_cpu(pfwheader->img_sram_size);
	}

	/* 4. download fw now */
	fwstatus = _rtl92s_firmware_get_nextstatus(firmware->fwstatus);
	while (fwstatus != FW_STATUS_READY) {
		/* Image buffer redirection. */
		switch (fwstatus) {
		case FW_STATUS_LOAD_IMEM:
			puc_mappedfile = firmware->fw_imem;
			ul_filelength = firmware->fw_imem_len;
			break;
		case FW_STATUS_LOAD_EMEM:
			puc_mappedfile = firmware->fw_emem;
			ul_filelength = firmware->fw_emem_len;
			break;
		case FW_STATUS_LOAD_DMEM:
			/* Partial update the content of header private. */
			pfwheader = firmware->pfwheader;
			pfw_priv = &pfwheader->fwpriv;
			_rtl92s_firmwareheader_priveupdate(hw, pfw_priv);
			puc_mappedfile = (u8 *)(firmware->pfwheader) +
					RT_8192S_FIRMWARE_HDR_EXCLUDE_PRI_SIZE;
			ul_filelength = fwhdr_size -
					RT_8192S_FIRMWARE_HDR_EXCLUDE_PRI_SIZE;
			break;
		default:
			pr_err("Unexpected Download step!!\n");
			goto fail;
		}

		/* <2> Download image file */
		rtstatus = _rtl92s_firmware_downloadcode(hw, puc_mappedfile,
				ul_filelength);

		if (!rtstatus) {
			pr_err("fail!\n");
			goto fail;
		}

		/* <3> Check whether load FW process is ready */
		rtstatus = _rtl92s_firmware_checkready(hw, fwstatus);
		if (!rtstatus) {
			pr_err("rtl8192se: firmware fail!\n");
			goto fail;
		}

		fwstatus = _rtl92s_firmware_get_nextstatus(firmware->fwstatus);
	}

	return rtstatus;
fail:
	return 0;
}
EXPORT_SYMBOL_GPL(rtl92s_download_fw);

static u32 _rtl92s_fill_h2c_cmd(struct sk_buff *skb, u32 h2cbufferlen,
				u32 cmd_num, u32 *pelement_id, u32 *cmd_rsvd,
				u32 *pcmd_len, u8 **pcmb_buffer,
				u8 *cmd_start_seq)
{
	u32 totallen = 0, len = 0, tx_desclen = 0;
	u32 pre_continueoffset = 0;
	u8 *ph2c_buffer;
	u8 i = 0;

	do {
		/* 8 - Byte alignment */
		len = H2C_TX_CMD_HDR_LEN + N_BYTE_ALIGMENT(pcmd_len[i], 8);

		/* Buffer length is not enough */
		if (h2cbufferlen < totallen + len + tx_desclen)
			break;

		/* Clear content */
		ph2c_buffer = (u8 *)skb_put(skb, (u32)len);
		memset((ph2c_buffer + totallen + tx_desclen), 0, len);

		/* CMD len */
		SET_BITS_TO_LE_4BYTE((ph2c_buffer + totallen + tx_desclen),
				      0, 16, pcmd_len[i]);

		/* CMD ID */
		SET_BITS_TO_LE_4BYTE((ph2c_buffer + totallen + tx_desclen),
				      16, 8, pelement_id[i]);

		/* CMD Sequence */
		*cmd_start_seq = *cmd_start_seq % 0x80;
		SET_BITS_TO_LE_4BYTE((ph2c_buffer + totallen + tx_desclen),
				      24, 7, *cmd_start_seq);
		++*cmd_start_seq;

		/* Reserved */
		SET_BITS_TO_LE_4BYTE((ph2c_buffer + totallen + tx_desclen + 4),
				      0, 32, cmd_rsvd[i]);

		/* Copy memory */
		memcpy((ph2c_buffer + totallen + tx_desclen +
			H2C_TX_CMD_HDR_LEN), pcmb_buffer[i], pcmd_len[i]);

		/* CMD continue */
		/* set the continue in prevoius cmd. */
		if (i < cmd_num - 1)
			SET_BITS_TO_LE_4BYTE((ph2c_buffer + pre_continueoffset),
					      31, 1, 1);

		pre_continueoffset = totallen;

		totallen += len;
	} while (++i < cmd_num);

	return totallen;
}

static u32 _rtl92s_get_h2c_cmdlen(u32 h2cbufferlen, u32 cmd_num, u32 *pcmd_len)
{
	u32 totallen = 0, len = 0, tx_desclen = 0;
	u8 i = 0;

	do {
		/* 8 - Byte alignment */
		len = H2C_TX_CMD_HDR_LEN + N_BYTE_ALIGMENT(pcmd_len[i], 8);

		/* Buffer length is not enough */
		if (h2cbufferlen < totallen + len + tx_desclen)
			break;

		totallen += len;
	} while (++i < cmd_num);

	return totallen + tx_desclen;
}

int rtl92s_firmware_set_h2c_cmd(struct ieee80211_hw *hw, u32 element_id,
				u32 rsvd, u8 *pcmd_buffer, u32 cmd_len)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	struct rtl_tcb_desc *cb_desc;
	struct sk_buff *skb;
	u32	len;

	len = _rtl92s_get_h2c_cmdlen(MAX_TRANSMIT_BUFFER_SIZE, 1, &cmd_len);
	skb = dev_alloc_skb(len);
	if (!skb)
		return -ENOMEM;
	cb_desc = (struct rtl_tcb_desc *)(skb->cb);
	cb_desc->queue_index = TXCMD_QUEUE;
	cb_desc->cmd_or_init = DESC_PACKET_TYPE_CMD;
	cb_desc->last_inipkt = false;

	_rtl92s_fill_h2c_cmd(skb, MAX_TRANSMIT_BUFFER_SIZE, 1, &element_id,
			&rsvd, &cmd_len, &pcmd_buffer, &rtlhal->h2c_txcmd_seq);
	rtlpriv->cfg->ops->cmd_send_packet(hw, skb);
	rtlpriv->cfg->ops->tx_polling(hw, _rtl92s_get_fw_queue(rtlpriv));
	return 0;
}
EXPORT_SYMBOL_GPL(rtl92s_firmware_set_h2c_cmd);

void rtl92s_set_fw_pwrmode_cmd(struct ieee80211_hw *hw, u8 Mode)
{
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	struct rtl_ps_ctl *ppsc = rtl_psc(rtl_priv(hw));
	struct h2c_set_pwrmode_parm pwrmode;
	u16 max_wakeup_period = 0;

	pwrmode.mode = Mode;
	pwrmode.flag_low_traffic_en = 0;
	pwrmode.flag_lpnav_en = 0;
	pwrmode.flag_rf_low_snr_en = 0;
	pwrmode.flag_dps_en = 0;
	pwrmode.bcn_rx_en = 0;
	pwrmode.bcn_to = 0;
	SET_BITS_TO_LE_2BYTE((u8 *)(&pwrmode) + 8, 0, 16,
			mac->vif->bss_conf.beacon_int);
	pwrmode.app_itv = 0;
	pwrmode.awake_bcn_itvl = ppsc->reg_max_lps_awakeintvl;
	pwrmode.smart_ps = 1;
	pwrmode.bcn_pass_period = 10;

	/* Set beacon pass count */
	if (pwrmode.mode == FW_PS_MIN_MODE)
		max_wakeup_period = mac->vif->bss_conf.beacon_int;
	else if (pwrmode.mode == FW_PS_MAX_MODE)
		max_wakeup_period = mac->vif->bss_conf.beacon_int *
			mac->vif->bss_conf.dtim_period;

	if (max_wakeup_period >= 500)
		pwrmode.bcn_pass_cnt = 1;
	else if ((max_wakeup_period >= 300) && (max_wakeup_period < 500))
		pwrmode.bcn_pass_cnt = 2;
	else if ((max_wakeup_period >= 200) && (max_wakeup_period < 300))
		pwrmode.bcn_pass_cnt = 3;
	else if ((max_wakeup_period >= 20) && (max_wakeup_period < 200))
		pwrmode.bcn_pass_cnt = 5;
	else
		pwrmode.bcn_pass_cnt = 1;

	rtl92s_firmware_set_h2c_cmd(hw, H2C_SETPWRMODE_CMD, 0, (u8 *)&pwrmode,
				    sizeof(pwrmode));
}
EXPORT_SYMBOL_GPL(rtl92s_set_fw_pwrmode_cmd);

void rtl92s_set_fw_joinbss_report_cmd(struct ieee80211_hw *hw,
		u8 mstatus, u8 ps_qosinfo)
{
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	struct h2c_joinbss_rpt_parm joinbss_rpt;

	joinbss_rpt.opmode = mstatus;
	joinbss_rpt.ps_qos_info = ps_qosinfo;
	joinbss_rpt.bssid[0] = mac->bssid[0];
	joinbss_rpt.bssid[1] = mac->bssid[1];
	joinbss_rpt.bssid[2] = mac->bssid[2];
	joinbss_rpt.bssid[3] = mac->bssid[3];
	joinbss_rpt.bssid[4] = mac->bssid[4];
	joinbss_rpt.bssid[5] = mac->bssid[5];
	SET_BITS_TO_LE_2BYTE((u8 *)(&joinbss_rpt) + 8, 0, 16,
			mac->vif->bss_conf.beacon_int);
	SET_BITS_TO_LE_2BYTE((u8 *)(&joinbss_rpt) + 10, 0, 16, mac->assoc_id);

	rtl92s_firmware_set_h2c_cmd(hw, H2C_JOINBSSRPT_CMD, 0,
				    (u8 *)&joinbss_rpt, sizeof(joinbss_rpt));
}
EXPORT_SYMBOL_GPL(rtl92s_set_fw_joinbss_report_cmd);
