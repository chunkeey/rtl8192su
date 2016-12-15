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
#include "../usb.h"
#include "../base.h"
#include "../stats.h"
#include "../rtl8192s/reg_common.h"
#include "../rtl8192s/def_common.h"
#include "../rtl8192s/phy_common.h"
#include "../rtl8192s/trx_common.h"
#include "../rtl8192s/fw_common.h"
#include "../rtl8192s/hw_common.h"
#include "trx.h"
#include "led.h"

/* endpoint mapping */
int rtl92su_endpoint_mapping(struct ieee80211_hw *hw)
{
	struct rtl_usb_priv *usb_priv = rtl_usbpriv(hw);
	struct rtl_usb *rtlusb = rtl_usbdev(usb_priv);
	struct rtl_ep_map *ep_map = &(rtlusb->ep_map);

	ep_map->ep_mapping[RTL_TXQ_BE]  = 0x06;
	ep_map->ep_mapping[RTL_TXQ_VO]	= 0x04;

	switch (rtlusb->out_ep_nums) {
	case 3:
		ep_map->ep_mapping[RTL_TXQ_BK]	= 0x06;
		ep_map->ep_mapping[RTL_TXQ_MGT] = 0x0d;
		ep_map->ep_mapping[RTL_TXQ_VI]	= 0x04;
		ep_map->ep_mapping[RTL_TXQ_BCN] = 0x0d;
		ep_map->ep_mapping[RTL_TXQ_HI]	= 0x0d;
		break;
	case 5:
		ep_map->ep_mapping[RTL_TXQ_BK]  = 0x07;
		ep_map->ep_mapping[RTL_TXQ_VI]  = 0x05;
		ep_map->ep_mapping[RTL_TXQ_MGT] = 0x0d;
		ep_map->ep_mapping[RTL_TXQ_BCN] = 0x0d;
		ep_map->ep_mapping[RTL_TXQ_HI]  = 0x0d;
		break;
	case 8:
		ep_map->ep_mapping[RTL_TXQ_BK]  = 0x07;
		ep_map->ep_mapping[RTL_TXQ_VI]  = 0x05;
		ep_map->ep_mapping[RTL_TXQ_MGT] = 0x0c;
		ep_map->ep_mapping[RTL_TXQ_BCN] = 0x0a;
		ep_map->ep_mapping[RTL_TXQ_HI]  = 0x0b;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

u16 rtl92su_mq_to_hwq(__le16 fc, u16 mac80211_queue_index)
{
	u16 hw_queue_index;

	if (unlikely(ieee80211_is_beacon(fc))) {
		hw_queue_index = RTL_TXQ_BCN;
		goto out;
	}
	if (ieee80211_is_mgmt(fc)) {
		hw_queue_index = RTL_TXQ_MGT;
		goto out;
	}
	switch (mac80211_queue_index) {
	case 0:
		hw_queue_index = RTL_TXQ_VO;
		break;
	case 1:
		hw_queue_index = RTL_TXQ_VI;
		break;
	case 2:
		hw_queue_index = RTL_TXQ_BE;
		break;
	case 3:
		hw_queue_index = RTL_TXQ_BK;
		break;
	default:
		hw_queue_index = RTL_TXQ_BE;
		WARN_ONCE(true, "rtl8192su: QSLT_BE queue, skb_queue:%d\n",
			  mac80211_queue_index);
		break;
	}
out:
	return hw_queue_index;
}

bool rtl92su_rx_query_desc(struct ieee80211_hw *hw, struct rtl_stats *stats,
			   struct ieee80211_rx_status *rx_status, u8 *pdesc,
			   struct sk_buff *skb)
{
	struct rx_fwinfo *p_drvinfo;
	u32 phystatus = (u32)GET_RX_STATUS_DESC_PHY_STATUS(pdesc);
	struct ieee80211_hdr *hdr;

	stats->length = (u16)GET_RX_STATUS_DESC_PKT_LEN(pdesc);
	stats->rx_drvinfo_size = (u8)GET_RX_STATUS_DESC_DRVINFO_SIZE(pdesc) * 8;
	stats->rx_bufshift = (u8)(GET_RX_STATUS_DESC_SHIFT(pdesc) & 0x03);
	stats->icv = (u16)GET_RX_STATUS_DESC_ICV(pdesc);
	stats->crc = (u16)GET_RX_STATUS_DESC_CRC32(pdesc);
	stats->hwerror = (u16)(stats->crc | stats->icv);
	stats->decrypted = !GET_RX_STATUS_DESC_SWDEC(pdesc);

	stats->rate = (u8)GET_RX_STATUS_DESC_RX_MCS(pdesc);
	stats->shortpreamble = (u16)GET_RX_STATUS_DESC_SPLCP(pdesc);
	stats->isampdu = (bool)(GET_RX_STATUS_DESC_PAGGR(pdesc) == 1);
	stats->isfirst_ampdu = (bool) ((GET_RX_STATUS_DESC_PAGGR(pdesc) == 1)
			       && (GET_RX_STATUS_DESC_FAGGR(pdesc) == 1));
	stats->timestamp_low = GET_RX_STATUS_DESC_TSFL(pdesc);
	stats->rx_is40Mhzpacket = (bool)GET_RX_STATUS_DESC_BW(pdesc);
	stats->is_ht = (bool)GET_RX_STATUS_DESC_RX_HT(pdesc);
	stats->is_cck = SE_RX_HAL_IS_CCK_RATE(pdesc);

	rx_status->freq = hw->conf.chandef.chan->center_freq;
	rx_status->band = hw->conf.chandef.chan->band;

	if (stats->crc)
		rx_status->flag |= RX_FLAG_FAILED_FCS_CRC;

	if (stats->rx_is40Mhzpacket)
		rx_status->flag |= RX_FLAG_40MHZ;

	if (stats->is_ht)
		rx_status->flag |= RX_FLAG_HT;

	rx_status->flag |= RX_FLAG_MACTIME_START;

	/* hw will set stats->decrypted true, if it finds the
	 * frame is open data frame or mgmt frame,
	 * hw will not decrypt robust management frame
	 * for IEEE80211w but still set stats->decrypted
	 * true, so here we should set it back to undecrypted
	 * for IEEE80211w frame, and mac80211 sw will help
	 * to decrypt it */
	if (stats->decrypted) {
		hdr = (struct ieee80211_hdr *)(skb->data +
		       stats->rx_drvinfo_size + stats->rx_bufshift);

		if (!hdr) {
			/* during testing, hdr was NULL here */
			return false;
		}
		if ((_ieee80211_is_robust_mgmt_frame(hdr)) &&
			(ieee80211_has_protected(hdr->frame_control)))
			rx_status->flag &= ~RX_FLAG_DECRYPTED;
		else
			rx_status->flag |= RX_FLAG_DECRYPTED;
	}

	rx_status->rate_idx = rtlwifi_rate_mapping(hw,
			      stats->is_ht, false, stats->rate);

	rx_status->mactime = stats->timestamp_low;
	if (phystatus) {
		p_drvinfo = (struct rx_fwinfo *)(skb->data +
						 stats->rx_bufshift);
		rtl92s_translate_rx_signal_stuff(hw, skb, stats, pdesc,
						 p_drvinfo);
	}

	/*rx_status->qual = stats->signal; */
	rx_status->signal = stats->recvsignalpower + 10;

	return true;
}

void rtl92su_tx_fill_desc(struct ieee80211_hw *hw,
		struct ieee80211_hdr *hdr, u8 *pdesc_tx, u8 *pbd_desc_tx,
		struct ieee80211_tx_info *info,
		struct ieee80211_sta *sta,
		struct sk_buff *skb,
		u8 hw_queue, struct rtl_tcb_desc *ptcb_desc)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	u8 *pdesc;
	u16 seq_number;
	__le16 fc = hdr->frame_control;
	u8 reserved_macid = 0;
	u8 fw_qsel = rtl92s_map_hwqueue_to_fwqueue(skb, hw_queue);
	bool firstseg = (!(hdr->seq_ctrl & cpu_to_le16(IEEE80211_SCTL_FRAG)));
	bool lastseg = (!(hdr->frame_control &
			cpu_to_le16(IEEE80211_FCTL_MOREFRAGS)));
	u8 bw_40 = 0;

	if (mac->opmode == NL80211_IFTYPE_STATION) {
		bw_40 = mac->bw_40;
	} else if (mac->opmode == NL80211_IFTYPE_AP ||
		mac->opmode == NL80211_IFTYPE_ADHOC) {
		if (sta)
			bw_40 = sta->bandwidth >= IEEE80211_STA_RX_BW_40;
	}

	seq_number = (le16_to_cpu(hdr->seq_ctrl) & IEEE80211_SCTL_SEQ) >> 4;

	rtl_get_tcb_desc(hw, info, sta, skb, ptcb_desc);
	pdesc = (u8 *)skb_push(skb, RTL_TX_HEADER_SIZE);
	memset(pdesc, 0, RTL_TX_HEADER_SIZE);

	if (ieee80211_is_nullfunc(fc) || ieee80211_is_ctl(fc)) {
		firstseg = true;
		lastseg = true;
	}

	if (firstseg) {
		if (rtlpriv->dm.useramask) {
			/* set txdesc macId */
			if (ptcb_desc->mac_id < 32) {
				SET_TX_DESC_MACID(pdesc, ptcb_desc->mac_id);
				reserved_macid |= ptcb_desc->mac_id;
			}
		}
		SET_TX_DESC_RSVD_MACID(pdesc, reserved_macid);

		SET_TX_DESC_TXHT(pdesc, ((ptcb_desc->hw_rate >=
				 DESC_RATEMCS0) ? 1 : 0));

		if (rtlhal->version == VERSION_8192S_ACUT) {
			if (ptcb_desc->hw_rate == DESC_RATE1M ||
				ptcb_desc->hw_rate  == DESC_RATE2M ||
				ptcb_desc->hw_rate == DESC_RATE5_5M ||
				ptcb_desc->hw_rate == DESC_RATE11M) {
				ptcb_desc->hw_rate = DESC_RATE12M;
			}
		}

		SET_TX_DESC_TX_RATE(pdesc, ptcb_desc->hw_rate);

		if (ptcb_desc->use_shortgi || ptcb_desc->use_shortpreamble)
			SET_TX_DESC_TX_SHORT(pdesc, 0);

		/* Aggregation related */
		if (info->flags & IEEE80211_TX_CTL_AMPDU)
			SET_TX_DESC_AGG_ENABLE(pdesc, 1);

		/* For AMPDU, we must insert SSN into TX_DESC */
		SET_TX_DESC_SEQ(pdesc, seq_number);

		/* Protection mode related */
		/* For 92S, if RTS/CTS are set, HW will execute RTS. */
		/* We choose only one protection mode to execute */
		SET_TX_DESC_RTS_ENABLE(pdesc, ((ptcb_desc->rts_enable &&
				!ptcb_desc->cts_enable) ? 1 : 0));
		SET_TX_DESC_CTS_ENABLE(pdesc, ((ptcb_desc->cts_enable) ?
				       1 : 0));
		SET_TX_DESC_RTS_STBC(pdesc, ((ptcb_desc->rts_stbc) ? 1 : 0));

		SET_TX_DESC_RTS_RATE(pdesc, ptcb_desc->rts_rate);
		SET_TX_DESC_RTS_BANDWIDTH(pdesc, 0);
		SET_TX_DESC_RTS_SUB_CARRIER(pdesc, ptcb_desc->rts_sc);
		SET_TX_DESC_RTS_SHORT(pdesc, ((ptcb_desc->rts_rate <=
		       DESC_RATE54M) ?
		       (ptcb_desc->rts_use_shortpreamble ? 1 : 0)
		       : (ptcb_desc->rts_use_shortgi ? 1 : 0)));


		/* Set Bandwidth and sub-channel settings. */
		if (bw_40) {
			if (ptcb_desc->packet_bw) {
				SET_TX_DESC_TX_BANDWIDTH(pdesc, 1);
				/* use duplicated mode */
				SET_TX_DESC_TX_SUB_CARRIER(pdesc, 0);
			} else {
				SET_TX_DESC_TX_BANDWIDTH(pdesc, 0);
				SET_TX_DESC_TX_SUB_CARRIER(pdesc,
						   mac->cur_40_prime_sc);
			}
		} else {
			SET_TX_DESC_TX_BANDWIDTH(pdesc, 0);
			SET_TX_DESC_TX_SUB_CARRIER(pdesc, 0);
		}

		/* 3 Fill necessary field in First Descriptor */
		/*DWORD 0*/
		SET_TX_DESC_LINIP(pdesc, 0);
		SET_TX_DESC_OFFSET(pdesc, 32);
		SET_TX_DESC_PKT_SIZE(pdesc,
				    (u16)skb->len - RTL_TX_HEADER_SIZE);

		/*DWORD 1*/
		SET_TX_DESC_RA_BRSR_ID(pdesc, ptcb_desc->ratr_index);

		/* Fill security related */
		if (info->control.hw_key) {
			struct ieee80211_key_conf *keyconf;

			keyconf = info->control.hw_key;
			switch (keyconf->cipher) {
			case WLAN_CIPHER_SUITE_WEP40:
			case WLAN_CIPHER_SUITE_WEP104:
				SET_TX_DESC_SEC_TYPE(pdesc, 0x1);
				break;
			case WLAN_CIPHER_SUITE_TKIP:
				SET_TX_DESC_SEC_TYPE(pdesc, 0x2);
				break;
			case WLAN_CIPHER_SUITE_CCMP:
				SET_TX_DESC_SEC_TYPE(pdesc, 0x3);
				break;
			default:
				SET_TX_DESC_SEC_TYPE(pdesc, 0x0);
				break;

			}
		}

		/* Set Packet ID */
		SET_TX_DESC_PACKET_ID(pdesc, 0);

		/* We will assign magement queue to BK. */
		SET_TX_DESC_QUEUE_SEL(pdesc, fw_qsel);

		/* Always enable all rate fallback range */
		SET_TX_DESC_DATA_RATE_FB_LIMIT(pdesc, 0x1F);

		/* Fix: I don't know why hw use 6.5M to tx when set it */
		SET_TX_DESC_USER_RATE(pdesc,
				      ptcb_desc->use_driver_rate ? 1 : 0);

		/* Set NON_QOS bit. */
		if (!ieee80211_is_data_qos(fc))
			SET_TX_DESC_NON_QOS(pdesc, 1);

	}

	/* Fill fields that are required to be initialized
	 * in all of the descriptors */
	/*DWORD 0 */
	SET_TX_DESC_FIRST_SEG(pdesc, (firstseg ? 1 : 0));
	SET_TX_DESC_LAST_SEG(pdesc, (lastseg ? 1 : 0));
	SET_TX_DESC_OWN(pdesc, 1);

	/* DWORD 7 */
	SET_TX_DESC_TX_BUFFER_SIZE(pdesc,
				   (u16)(skb->len - RTL_TX_HEADER_SIZE));

	RT_TRACE(rtlpriv, COMP_SEND, DBG_TRACE, "\n");
}

void rtl92su_tx_fill_cmddesc(struct ieee80211_hw *hw, u8 *pdesc,
	bool firstseg, bool lastseg, struct sk_buff *skb)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtlpriv);
	struct rtl_tcb_desc *tcb_desc = (struct rtl_tcb_desc *)(skb->cb);

	/* Clear all status	*/
	memset((void *)pdesc, 0, RTL_TX_HEADER_SIZE);

	/* This bit indicate this packet is used for FW download. */
	if (tcb_desc->cmd_or_init == DESC_PACKET_TYPE_INIT) {
		/* For firmware downlaod we only need to set LINIP */
		SET_TX_DESC_LINIP(pdesc, tcb_desc->last_inipkt);

		/* 92SU need not to set TX packet size when firmware download */
		SET_TX_DESC_PKT_SIZE(pdesc,
				     (u16)(skb->len - RTL_TX_HEADER_SIZE));
	} else { /* H2C Command Desc format (Host TXCMD) */
		/* 92SE must set as 1 for firmware download HW DMA error */
		SET_TX_DESC_FIRST_SEG(pdesc, 1);
		SET_TX_DESC_LAST_SEG(pdesc, 1);

		SET_TX_DESC_OFFSET(pdesc, 0x20);

		/* Buffer size + command header */
		SET_TX_DESC_PKT_SIZE(pdesc,
				     (u16)(skb->len - RTL_TX_HEADER_SIZE));
		/* Fixed queue of H2C command */
		SET_TX_DESC_QUEUE_SEL(pdesc, 0x13);

		SET_BITS_TO_LE_4BYTE(skb->data, 24, 7, rtlhal->h2c_txcmd_seq);

		SET_TX_DESC_TX_BUFFER_SIZE(pdesc, (u16)(skb->len));
	}
}

bool rtl92su_cmd_send_packet(struct ieee80211_hw *hw, struct sk_buff *skb)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_tcb_desc *tcb_desc;
	u8 *pdesc;
	int err;

	pdesc = (u8 *)skb_push(skb, RTL_TX_HEADER_SIZE);
	rtlpriv->cfg->ops->fill_tx_cmddesc(hw, pdesc, 1, 1, skb);

	tcb_desc = (struct rtl_tcb_desc *)(skb->cb);
	err = rtl_usb_transmit(hw, skb, tcb_desc->queue_index);
	return err ? false : true;
}

int rtl92su_update_beacon(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_mac *mac = rtl_mac(rtlpriv);
	struct sk_buff *skb;
	struct ieee80211_mutable_offsets offs;
	int err = -ENOMEM;
	u32 extra = 0;

	skb = ieee80211_beacon_get_template(hw, mac->vif, &offs);
	if (skb) {
		struct ieee80211_hdr *hdr = rtl_get_hdr(skb);
		struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
		struct rtl_tcb_desc tcb_desc;

		rtlpriv->cfg->ops->fill_tx_desc(hw, hdr, NULL, NULL,
						info, NULL, skb,
						QSLT_CMD, &tcb_desc);

		SET_BITS_TO_LE_4BYTE(&extra, 16, 16, offs.tim_offset);
		err = rtl92s_firmware_set_h2c_cmd(hw, H2C_UPDATE_BCN_CMD,
						  extra, skb->data, skb->len);
		dev_kfree_skb_any(skb);
	}
	return err;
}

#define RTL_RX_DRV_INFO_UNIT		8

int rtl92su_rx_hdl(struct ieee80211_hw *hw, struct sk_buff *skb)
{
	u32 skb_len, pkt_len, drvinfo_len;
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 *rxdesc;

	rxdesc	= skb->data;
	skb_len	= skb->len;
	drvinfo_len = (GET_RX_STATUS_DESC_DRVINFO_SIZE(rxdesc) *
		       RTL_RX_DRV_INFO_UNIT);
	pkt_len	= GET_RX_STATUS_DESC_PKT_LEN(rxdesc);

	if (GET_RX_STATUS_DEC_MACID(rxdesc) == 0x1ff) {
		u8 event = LE_BITS_CLEARED_TO_4BYTE(rxdesc + RTL_RX_DESC_SIZE,
						    16, 8);

		switch (event) {
		case 0x15:
			rtl_send_buffered_bc(hw);
			break;
		default:
			RT_TRACE(rtlpriv, COMP_RECV, DBG_DMESG,
				 "Unknown event 0x%x\n", event);
			break;
		}
		dev_kfree_skb_any(skb);
		return 1;
	} else {
		return 0;
	}
}

void rtl92su_tx_cleanup(struct ieee80211_hw *hw, struct sk_buff *skb)
{
}

int rtl92su_tx_post_hdl(struct ieee80211_hw *hw, struct urb *urb,
			struct sk_buff *skb)
{
	return 0;
}

struct sk_buff *rtl92su_tx_aggregate_hdl(struct ieee80211_hw *hw,
					 struct sk_buff_head *list)
{
	return skb_dequeue(list);
}

void rtl92su_tx_polling(struct ieee80211_hw *hw, u8 hw_queue)
{
}
