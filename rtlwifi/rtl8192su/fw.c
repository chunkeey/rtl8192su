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
#include "../usb.h"
#include "../base.h"
#include "reg.h"
#include "def.h"
#include "fw.h"

static int _rtl92s_firmware_enable_cpu(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 ichecktime = 200;
	u16 tmpu2b;
	u8 tmpu1b, cpustatus = 0;

	/* select clock */
	tmpu1b = rtl_read_byte(rtlpriv, REG_SYS_CLKR);
	rtl_write_byte(rtlpriv, REG_SYS_CLKR, (tmpu1b | SYS_CPU_CLKSEL));

	/* Enable CPU. */
	tmpu2b = rtl_read_word(rtlpriv, REG_SYS_FUNC_EN);
	rtl_write_word(rtlpriv, REG_SYS_FUNC_EN, (tmpu2b | FEN_CPUEN));

	/* Polling IMEM Ready after CPU has refilled. */
	do {
		cpustatus = rtl_read_byte(rtlpriv, REG_TCR);
		if (cpustatus & IMEM_RDY) {
			RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
				 "IMEM Ready after CPU has refilled\n");
			break;
		}

		udelay(100);
	} while (ichecktime--);

	return !!(cpustatus & IMEM_RDY) ? 0 : -ETIMEDOUT;
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
		RT_TRACE(rtlpriv, COMP_INIT, DBG_EMERG, "Unknown RF type(%x)\n",
			 rtlphy->rf_type);
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


static void _rtl92s_cmd_complete(struct urb *urb)
{
	struct sk_buff *skb = urb->context;
	dev_kfree_skb_irq(skb);
}

static int _rtl92s_cmd_send_packet(struct ieee80211_hw *hw,
		struct sk_buff *skb, u8 last)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_tcb_desc *tcb_desc;
	u8 *pdesc;
	int err;

	pdesc = (u8 *)skb_push(skb, RTL_TX_HEADER_SIZE);
	rtlpriv->cfg->ops->fill_tx_cmddesc(hw, pdesc, 1, 1, skb);

	tcb_desc = (struct rtl_tcb_desc *)(skb->cb);
	err = rtl_usb_transmit(hw, skb, tcb_desc->queue_index,
				_rtl92s_cmd_complete);
	/*
	 * If an error occured in rtl_usb_transmit, the skb is
	 * being freed automatically.
	 */
	return err;
}

static int _rtl92s_firmware_downloadcode(struct ieee80211_hw *hw,
		u8 *code_virtual_address, u32 buffer_len)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct sk_buff *skb;
	struct rtl_tcb_desc *tcb_desc;
	unsigned char *seg_ptr;
	int err;
	u16 frag_threshold = 0xC000;
	u16 frag_length, frag_offset = 0;
	u16 extra_descoffset = 0;
	u8 last_inipkt = 0;

	if (buffer_len >= MAX_FIRMWARE_CODE_SIZE) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "Size over FIRMWARE_CODE_SIZE!\n");
		return -EINVAL;
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
			return -ENOMEM;
		skb_reserve(skb, extra_descoffset);
		seg_ptr = (u8 *)skb_put(skb, (u32)(frag_length -
					extra_descoffset));
		memcpy(seg_ptr, code_virtual_address + frag_offset,
		       (u32)(frag_length - extra_descoffset));

		tcb_desc = (struct rtl_tcb_desc *)(skb->cb);
		tcb_desc->queue_index = RTL_TXQ_VO;
		tcb_desc->cmd_or_init = DESC_PACKET_TYPE_INIT;
		tcb_desc->last_inipkt = last_inipkt;

		err = _rtl92s_cmd_send_packet(hw, skb, last_inipkt);
		if (err)
			return err;

		frag_offset += (frag_length - extra_descoffset);

	} while (frag_offset < buffer_len);

	return 0;
}

static int _rtl92s_firmware_checkready(struct ieee80211_hw *hw,
		u8 loadfw_status)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	struct rt_firmware *firmware = (struct rt_firmware *)rtlhal->pfirmware;
	u32 tmpu4b;
	u8 cpustatus = 0;
	int err = 0;
	int pollingcnt = 1000;

	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "LoadStaus(%d)\n", loadfw_status);

	firmware->fwstatus = (enum fw_status)loadfw_status;

	switch (loadfw_status) {
	case FW_STATUS_LOAD_IMEM:
		/* Polling IMEM code done. */
		do {
			cpustatus = rtl_read_byte(rtlpriv, REG_TCR);
			if (cpustatus & IMEM_CODE_DONE)
				break;
			udelay(5);
		} while (pollingcnt--);

		if (!(cpustatus & IMEM_CHK_RPT) || (pollingcnt <= 0)) {
			RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
				 "FW_STATUS_LOAD_IMEM FAIL CPU, Status=%x\n",
				 cpustatus);
			err = -EAGAIN;
			goto status_check_fail;
		}
		break;

	case FW_STATUS_LOAD_EMEM:
		/* Check Put Code OK and Turn On CPU */
		/* Polling EMEM code done. */
		do {
			cpustatus = rtl_read_byte(rtlpriv, REG_TCR);
			if (cpustatus & EMEM_CODE_DONE)
				break;
			udelay(5);
		} while (pollingcnt--);

		if (!(cpustatus & EMEM_CHK_RPT) || (pollingcnt <= 0)) {
			RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
				 "FW_STATUS_LOAD_EMEM FAIL CPU, Status=%x\n",
				 cpustatus);
			err = -EAGAIN;
			goto status_check_fail;
		}

		/* Turn On CPU */
		err = _rtl92s_firmware_enable_cpu(hw);
		if (err) {
			RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
				 "Enable CPU fail!\n");
			err = -EAGAIN;
			goto status_check_fail;
		}
		break;

	case FW_STATUS_LOAD_DMEM:
		/* Polling DMEM code done */
		do {
			cpustatus = rtl_read_byte(rtlpriv, REG_TCR);
			if (cpustatus & DMEM_CODE_DONE)
				break;
			udelay(5);
		} while (pollingcnt--);

		if (!(cpustatus & DMEM_CODE_DONE) || (pollingcnt <= 0)) {
			RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
				 "Polling DMEM code done fail ! cpustatus(%#x)\n",
				 cpustatus);
			err = -EAGAIN;
			goto status_check_fail;
		}

		RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
			 "DMEM code download success, cpustatus(%#x)\n",
			 cpustatus);

		/* Prevent Delay too much and being scheduled out */
		/* Polling Load Firmware ready */
		pollingcnt = 30;
		do {
			cpustatus = rtl_read_byte(rtlpriv, REG_TCR);
			if (cpustatus & FWRDY)
				break;
			msleep(100);
		} while (pollingcnt--);

		RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
			 "Polling Load Firmware ready, cpustatus(%x)\n",
			 cpustatus);

		if (((cpustatus & LOAD_FW_READY) != LOAD_FW_READY) ||
		    (pollingcnt <= 0)) {
			RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
				 "Polling Load Firmware ready fail ! cpustatus(%x)\n",
				 cpustatus);
			err = -EAGAIN;
			goto status_check_fail;
		}

		/* If right here, we can set TCR/RCR to desired value  */
		/* and config MAC lookback mode to normal mode */
		tmpu4b = rtl_read_dword(rtlpriv, REG_TCR);
		rtl_write_dword(rtlpriv, REG_TCR, (tmpu4b & (~TCR_ICV)));

		tmpu4b = rtl_read_dword(rtlpriv, REG_RCR);
		rtl_write_dword(rtlpriv, REG_RCR, (tmpu4b |
			RCR_APP_PHYST_RXFF | RCR_APP_ICV | RCR_APP_MIC));

		RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
			 "Current RCR settings(%#x)\n", tmpu4b);

		/* Set to normal mode. */
		rtl_write_byte(rtlpriv, REG_LBKMD_SEL, LBK_NORMAL);
		break;

	default:
		RT_TRACE(rtlpriv, COMP_INIT, DBG_EMERG,
			 "Unknown status check!\n");
		err = -EINVAL;
		break;
	}

status_check_fail:
	if (err) {
		RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "loadfw_status(%d), err(%d)\n",
			 loadfw_status, err);
	}

	return err;
}

int rtl92s_download_fw(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	struct rtl_usb *rtlusb = rtl_usbdev(rtl_usbpriv(hw));
	struct rt_firmware *firmware = NULL;
	struct fw_hdr *pfwheader;
	struct fw_priv *pfw_priv = NULL;
	u8 *puc_mappedfile = NULL;
	u32 ul_filelength = 0;
	u8 fwhdr_size = RT_8192S_FIRMWARE_HDR_SIZE;
	u8 fwstatus = FW_STATUS_INIT;
	int err = 0;

	if (rtlpriv->max_fw_size == 0 || !rtlhal->pfirmware)
		return -EINVAL;

	firmware = (struct rt_firmware *)rtlhal->pfirmware;
	firmware->fwstatus = FW_STATUS_INIT;
	puc_mappedfile = firmware->sz_fw_tmpbuffer;

	/* 1. Retrieve FW header. */
	firmware->pfwheader = (struct fw_hdr *) puc_mappedfile;
	pfwheader = firmware->pfwheader;
	firmware->firmwareversion = le16_to_cpu(pfwheader->version);
	pfwheader->fwpriv.usb_ep_num = rtlusb->in_ep_nums +
			rtlusb->out_ep_nums;

	pfwheader->fwpriv.mp_mode = 0;
	pfwheader->fwpriv.turbo_mode = 0;
	pfwheader->fwpriv.beacon_offload = 0;
	pfwheader->fwpriv.mlme_offload = 0;
	pfwheader->fwpriv.hwpc_offload = 0;

	firmware->fw_imem_len = le32_to_cpu(pfwheader->img_imem_size);
	firmware->fw_emem_len = le32_to_cpu(pfwheader->img_sram_size);
	firmware->fw_dmem_len = le32_to_cpu(pfwheader->dmem_size);

	/* 2. Retrieve IMEM image. */
	if ((firmware->fw_imem_len == 0) ||
	    (firmware->fw_imem_len > sizeof(firmware->fw_imem))) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "memory for data image is less than IMEM required\n");
		err = -EINVAL;
		goto fail;
	} else {
		puc_mappedfile += fwhdr_size;

		memcpy(firmware->fw_imem, puc_mappedfile,
		       firmware->fw_imem_len);
	}

	/* 3. Retrieve EMEM image. */
	if (firmware->fw_emem_len > sizeof(firmware->fw_emem)) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "memory for data image is less than EMEM required\n");
		err = -EINVAL;
		goto fail;
	} else {
		puc_mappedfile += firmware->fw_imem_len;

		memcpy(firmware->fw_emem, puc_mappedfile,
		       firmware->fw_emem_len);
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
			err = -EINVAL;
			RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
				 "Unexpected Download step!!\n");
			goto fail;
			break;
		}

		/* <2> Download image file */
		err = _rtl92s_firmware_downloadcode(hw, puc_mappedfile,
				ul_filelength);

		if (err) {
			RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
				"fw download failed (%d)!\n", err);
			goto fail;
		}

		/* <3> Check whether load FW process is ready */
		err = _rtl92s_firmware_checkready(hw, fwstatus);
		if (err) {
			RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
				 "fw not ready (%d)!\n", err);
			goto fail;
		}

		fwstatus = _rtl92s_firmware_get_nextstatus(firmware->fwstatus);
	}

	return 0;
fail:
	return err;
}

static u32 _rtl92s_fill_h2c_cmd(struct sk_buff *skb, u32 h2cbufferlen,
				u32 cmd_num, u32 *pelement_id, u32 *pcmd_len,
				u8 **pcmb_buffer, u8 *cmd_start_seq)
{
	u32 totallen = 0, len = 0;
	u8 *ph2c_buffer, *prev = NULL;
	u8 i = 0;

	do {
		/* 8 - Byte aligment */
		len = H2C_TX_CMD_HDR_LEN + N_BYTE_ALIGMENT(pcmd_len[i], 8);

		/* Buffer length is not enough  */
		if (h2cbufferlen < totallen + len)
			break;

		/* Clear content */
		ph2c_buffer = (u8 *)skb_put(skb, (u32)len);
		memset(ph2c_buffer, 0, len);

		/* CMD len */
		SET_BITS_TO_LE_4BYTE(ph2c_buffer, 0, 16,
				     N_BYTE_ALIGMENT(pcmd_len[i], 8));

		/* CMD ID */
		SET_BITS_TO_LE_4BYTE(ph2c_buffer, 16, 8, pelement_id[i]);

		/* CMD Sequence */
		*cmd_start_seq = *cmd_start_seq % 0x80;
		SET_BITS_TO_LE_4BYTE(ph2c_buffer, 24, 7, *cmd_start_seq);
		++*cmd_start_seq;

		/* Copy memory */
		memcpy((ph2c_buffer + H2C_TX_CMD_HDR_LEN),
		       pcmb_buffer[i], pcmd_len[i]);

		/* CMD continue */
		/* set the continue in prevoius cmd. */
		if (prev)
			SET_BITS_TO_LE_4BYTE(prev, 31, 1, 1);

		prev = ph2c_buffer;

		totallen += len;
	} while (++i < cmd_num);
	return totallen;
}

static u32 _rtl92s_get_h2c_cmdlen(u32 h2cbufferlen, u32 cmd_num, u32 *pcmd_len)
{
	u32 totallen = 0, len = 0, tx_desclen = 0;
	u8 i = 0;

	do {
		/* 8 - Byte aligment */
		len = H2C_TX_CMD_HDR_LEN + N_BYTE_ALIGMENT(pcmd_len[i], 8);

		/* Buffer length is not enough */
		if (h2cbufferlen < totallen + len + tx_desclen)
			break;

		totallen += len;
	} while (++i < cmd_num);

	return totallen + tx_desclen;
}

static int _rtl92s_firmware_set_h2c_cmd(struct ieee80211_hw *hw, u32 h2c_cmd,
					 u32 cmd_len, u8 *pcmd_buffer)
{
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	struct rtl_tcb_desc *cb_desc;
	struct sk_buff *skb;
	u32 len;

	len = _rtl92s_get_h2c_cmdlen(MAX_TRANSMIT_BUFFER_SIZE, 1, &cmd_len);
	skb = dev_alloc_skb(len);
	if (!skb)
		return -ENOMEM;

	cb_desc = (struct rtl_tcb_desc *)(skb->cb);
	cb_desc->queue_index = TXCMD_QUEUE;
	cb_desc->cmd_or_init = DESC_PACKET_TYPE_NORMAL;
	cb_desc->last_inipkt = false;

	_rtl92s_fill_h2c_cmd(skb, MAX_TRANSMIT_BUFFER_SIZE, 1, &h2c_cmd,
			&cmd_len, &pcmd_buffer,	&rtlhal->h2c_txcmd_seq);
	return _rtl92s_cmd_send_packet(hw, skb, false);
}

void rtl92s_set_fw_pwrmode_cmd(struct ieee80211_hw *hw, u8 Mode)
{
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	struct rtl_ps_ctl *ppsc = rtl_psc(rtl_priv(hw));
	struct h2c_set_pwrmode_parm	pwrmode;
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

	_rtl92s_firmware_set_h2c_cmd(hw, H2C_SETPWRMODE_CMD,
		 sizeof(pwrmode), (u8 *)&pwrmode);
}

void rtl92s_set_fw_joinbss_report_cmd(struct ieee80211_hw *hw,
		u8 mstatus, u8 ps_qosinfo)
{
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	struct h2c_joinbss_rpt_parm joinbss_rpt;

	joinbss_rpt.opmode = mstatus;
	joinbss_rpt.ps_qos_info = ps_qosinfo;
	memcpy(joinbss_rpt.bssid, mac->bssid, ETH_ALEN);
	joinbss_rpt.bcnitv = cpu_to_le16(mac->vif->bss_conf.beacon_int);
	joinbss_rpt.aid = cpu_to_le16(mac->assoc_id);

	_rtl92s_firmware_set_h2c_cmd(hw, H2C_JOINBSSRPT_CMD, sizeof(joinbss_rpt),
		(u8 *)&joinbss_rpt);
}

u8 rtl92s_set_fw_assoc(struct ieee80211_hw *hw, u8 *mac, bool assoc) {
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_cmd_rsp *rtl_cmdrsp = rtl_cmd_rsp(rtlpriv);
	unsigned long flags;
	struct c2h_addsta_event macid;
	u32 cmd;
	int ret;

	if (assoc)
		cmd = H2C_SETASSOCSTA_CMD;
	else
		cmd = H2C_DELASSOCSTA_CMD;

	spin_lock_irqsave(&rtlpriv->locks.cmd_rsp_lock, flags);
	rtl_cmdrsp->buf = (u8 *) &macid;
	rtl_cmdrsp->size = sizeof(macid);
	spin_unlock_irqrestore(&rtlpriv->locks.cmd_rsp_lock, flags);

	/*
	 * Doesn't work? no callback
	 */

	_rtl92s_firmware_set_h2c_cmd(hw, cmd, ETH_ALEN, mac);

	if (assoc) {
		ret = wait_for_completion_timeout(&rtl_cmdrsp->complete, HZ);

		if (ret == 0)
			return -ETIMEDOUT;

		return macid.cam_id;
	} else {
		return 0;
	}
}

int rtl92s_set_fw_sitesurvey_cmd(struct ieee80211_hw *hw,
		struct ieee80211_vif *vif, struct cfg80211_scan_request *req)
{
	struct h2c_sitesurvey_parm survey_args;

	memset(&survey_args, 0, sizeof(struct h2c_sitesurvey_parm));
	survey_args.bsslimit = cpu_to_le32(48);

	if (req->n_ssids) {
		survey_args.active = cpu_to_le32(1);
		survey_args.ssidlen = cpu_to_le32(req->ssids[0].ssid_len);
		memcpy(survey_args.ssid, req->ssids[0].ssid, req->ssids[0].ssid_len);
	}

	return (int)!_rtl92s_firmware_set_h2c_cmd(hw, H2C_SITESURVEY_CMD,
		sizeof(survey_args), (u8 *)&survey_args);
}

int rtl92s_set_fw_disconnect_cmd(struct ieee80211_hw *hw)
{
	struct h2c_disconnect_parm disconnect_args = { };
	return _rtl92s_firmware_set_h2c_cmd(hw, H2C_DISCONNECT_CMD,
		sizeof(disconnect_args), (u8 *)&disconnect_args);
}

u8 rtl92s_set_fw_setchannel_cmd(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &(rtlpriv->phy);
	struct h2c_setchannel_parm chan_args;

	RT_TRACE(rtlpriv, COMP_CMD, DBG_TRACE, "SetChannel: %i\n",
		 rtlphy->current_channel);

	chan_args.channel = cpu_to_le32(rtlphy->current_channel);

	return (u8)_rtl92s_firmware_set_h2c_cmd(hw, H2C_SETCHANNEL_CMD,
		sizeof(chan_args), (u8 *)&chan_args);
}

int rtl92s_set_fw_setauth_cmd(struct ieee80211_hw *hw)
{
	struct h2c_setauth_parm auth_args = { };

	//auth_args.mode = cpu_to_le32(0);
	return _rtl92s_firmware_set_h2c_cmd(hw, H2C_SETAUTH_CMD, sizeof(auth_args),
		(u8 *)&auth_args);
}

int rtl92s_set_fw_opmode_cmd(struct ieee80211_hw *hw, enum h2c_op_modes mode)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct h2c_opmode mode_args = { };

	mode_args.mode = mode;

	RT_TRACE(rtlpriv, COMP_CMD, DBG_TRACE, "SetOpMode: %d\n",
			 mode_args.mode);

	return _rtl92s_firmware_set_h2c_cmd(hw, H2C_SETOPMODE_CMD, sizeof(mode_args),
		(u8 *)&mode_args);
}

int rtl92s_set_fw_datarate_cmd(struct ieee80211_hw *hw, u8 mac_id, mac_rates_t rates)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct h2c_data_rates rates_args = { };

	rates_args.mac_id = mac_id;
	memcpy(rates_args.rates, rates, sizeof(rates_args.rates));

	RT_TRACE(rtlpriv, COMP_CMD, DBG_TRACE, "Set Data Rates for macId %d: [%*ph]\n",
		mac_id, (int)sizeof(*rates), rates);

	return _rtl92s_firmware_set_h2c_cmd(hw, H2C_SETDATARATE_CMD,
		sizeof(rates_args), (u8 *)&rates_args);
}

void rtl92su_set_mac_addr(struct ieee80211_hw *hw, const u8 *addr)
{
	struct h2c_set_mac mac_args = { };

	memcpy(&mac_args.mac, addr, ETH_ALEN);
	_rtl92s_firmware_set_h2c_cmd(hw, H2C_SET_MAC_ADDRESS_CMD,
		 sizeof(mac_args), (u8 *)&mac_args);
}

void rtl92su_set_basic_rate(struct ieee80211_hw *hw)
{
	struct h2c_basic_rates rates_args = { };
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_mac *mac = rtl_mac(rtlpriv);
	struct ieee80211_channel *chan = hw->conf.channel;
	int i, j = 0;

	if (!chan)
		return;

	for (i = 0; i < mac->bands[chan->band].n_bitrates; i++) {
		if (!!(mac->basic_rates & (1 << i))) {
			/*
			 * Mac80211 supported_bands provides bitrates
			 * in 100 kbit units, however for the firmware
			 * we need to convert them into 500 kbit units.
			 */
			rates_args.basic_rates.rates[j++] =
				mac->bands[chan->band].bitrates[i].bitrate / 5;

			if (j >= ARRAY_SIZE(rates_args.basic_rates.rates))
				break;
		}
	}

	_rtl92s_firmware_set_h2c_cmd(hw, H2C_SETBASICRATE_CMD,
		sizeof(rates_args), (u8 *)&rates_args);
}

u8 rtl92s_fw_iocmd(struct ieee80211_hw *hw, const u32 cmd)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	int pollcount = 50;

	rtl_write_dword(rtlpriv, REG_IOCMD_CTRL, cmd);
	msleep(100);
	while (rtl_read_dword(rtlpriv, REG_IOCMD_CTRL) && pollcount > 0) {
		pollcount--;
		msleep(20);
	}

	return !!pollcount;
}

void rtl92s_fw_iocmd_data(struct ieee80211_hw *hw, u32* cmd, u8 flag)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	if (flag == 0) {
		/* set */
		rtl_write_dword(rtlpriv, REG_IOCMD_DATA, *cmd);
	} else {
		/* query */
		*cmd = rtl_read_dword(rtlpriv, REG_IOCMD_DATA);
	}
}

u32 rtl92s_fw_iocmd_read(struct ieee80211_hw *hw, u8 ioclass, u16 iovalue, u8 ioindex)
{
	struct h2c_iocmd cmd = { };
	u32 retval;

	cmd.cmdclass = ioclass;
	cmd.value = cpu_to_le16(iovalue);
	cmd.index = ioindex;

	if (rtl92s_fw_iocmd(hw, cmd.cmd))
		rtl92s_fw_iocmd_data(hw, &retval, 1);
	else
		retval = 0;

	return retval;
}

u8 rtl92s_fw_iocmd_write(struct ieee80211_hw *hw, u8 ioclass, u16 iovalue, u8 ioindex, u32 val)
{
	struct h2c_iocmd cmd = { };

	cmd.cmdclass = ioclass;
	cmd.value = cpu_to_le16(iovalue);
	cmd.index = ioindex;

	rtl92s_fw_iocmd_data(hw, &val, 0);
	msleep(100);
	return rtl92s_fw_iocmd(hw, cmd.cmd);
}
