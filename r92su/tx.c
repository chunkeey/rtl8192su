/******************************************************************************
 *
 * Copyright(c) 2009-2013  Realtek Corporation.
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
 * Joshua Roys <Joshua.Roys@gtri.gatech.edu>
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/
#include <linux/kernel.h>
#include <net/cfg80211.h>
#include <linux/etherdevice.h>
#include <linux/ieee80211.h>
#include <linux/ip.h>
#include <linux/udp.h>

#include "r92su.h"
#include "tx.h"
#include "def.h"
#include "usb.h"
#include "cmd.h"
#include "michael.h"

static const enum rtl8712_queues_t r92su_802_1d_to_ac[] = {
	[IEEE80211_AC_BK] = RTL8712_BKQ,
	[IEEE80211_AC_BE] = RTL8712_BEQ,
	[IEEE80211_AC_VI] = RTL8712_VIQ,
	[IEEE80211_AC_VO] = RTL8712_VOQ,
};

enum r92su_tx_control_t {
	TX_CONTINUE,
	TX_DROP,
};

struct r92su_tx_info {
	struct r92su_sta *sta;
	struct r92su_key *key;
	u8 tid;
	bool low_rate;
	bool ampdu;
	bool ht_possible;
};

static inline struct r92su_tx_info *r92su_get_tx_info(struct sk_buff *skb)
{
	BUILD_BUG_ON(sizeof(skb->cb) < sizeof(struct r92su_tx_info));
	return (struct r92su_tx_info *) skb->cb;
}

static enum r92su_tx_control_t
r92su_tx_fill_desc(struct r92su *r92su, struct sk_buff *skb,
		   struct r92su_bss_priv *bss_priv)
{
	struct r92su_tx_info *tx_info = r92su_get_tx_info(skb);
	struct ieee80211_hdr *i3e = (struct ieee80211_hdr *) skb->data;
	struct tx_hdr *hdr = NULL;
	u8 prio = skb->priority % ARRAY_SIZE(ieee802_1d_to_ac);

	hdr = (struct tx_hdr *) skb_push(skb, sizeof(*hdr));
	memset(hdr, 0 , sizeof(*hdr));

	hdr->pkt_len = cpu_to_le16(skb->len - sizeof(*hdr));
	hdr->offset = sizeof(*hdr);
	hdr->linip = 0;

	if (ieee80211_is_data(i3e->frame_control))
		hdr->mac_id = tx_info->sta->mac_id;
	else if (ieee80211_is_mgmt(i3e->frame_control))
		hdr->mac_id = 5;

	hdr->queue_sel = r92su_802_1d_to_ac[ieee802_1d_to_ac[prio]];

	/* firmware will increase the seqnum by itself, when
	 * the driver passes the correct "priority" to it */
	hdr->seq = prio;
	hdr->more_frag = !!(i3e->frame_control &
		cpu_to_le16(IEEE80211_FCTL_MOREFRAGS));
	hdr->more_data = !!(i3e->frame_control &
		cpu_to_le16(IEEE80211_FCTL_MOREDATA));
	hdr->frag = le16_to_cpu(i3e->seq_ctrl & IEEE80211_SCTL_FRAG);

	hdr->non_qos = !ieee80211_is_data_qos(i3e->frame_control);
	hdr->bmc = is_multicast_ether_addr(ieee80211_get_DA(i3e));

	if (tx_info->key) {
		switch (tx_info->key->type) {
		case WEP40_ENCRYPTION:
		case WEP104_ENCRYPTION: {
			hdr->sec_type = 1;
			hdr->key_id = tx_info->key->index;
			break;
		}

		case TKIP_ENCRYPTION:
			hdr->sec_type = 2;
			break;

		case AESCCMP_ENCRYPTION:
			hdr->sec_type = 3;
			break;

		default:
			WARN(1, "invalid encryption type %d\n",
			     tx_info->key->type);
			return TX_DROP;
		}
	}

	/* send EAPOL with failsafe rate */
	if (tx_info->low_rate) {
		hdr->user_rate = 1;
		hdr->dis_fb = 1;
		hdr->data_rate_fb_limit = 0x1f;
		hdr->agg_break = 1;
	} else if (tx_info->ampdu) {
		/* The firmware will automatically enable aggregation
		 * there's no need to set hdr->agg_en = 1 and hdr->tx_ht = 1;
		 */
	}

	hdr->own = 1;
	hdr->first_seg = 1;
	hdr->last_seg = 1;
	return TX_CONTINUE;
}

#define WEP_IV_LEN		4
#define WEP_ICV_LEN		4
#define CCMP_HDR_LEN		8
#define CCMP_MIC_LEN		8
#define CCMP_TK_LEN		16
#define TKIP_IV_LEN		8
#define TKIP_ICV_LEN		4

static enum r92su_tx_control_t
r92su_tx_add_iv(struct r92su *r92su, struct sk_buff *skb,
		struct r92su_bss_priv *bss_priv)
{
	struct ieee80211_hdr *hdr = (void *) skb->data;
	struct r92su_tx_info *tx_info = r92su_get_tx_info(skb);
	struct r92su_key *key = tx_info->key;
	unsigned int iv_len, hdr_len;
	u8 *iv;

	if (!key)
		return TX_CONTINUE;

	iv_len = r92su_get_iv_len(key);
	hdr_len = ieee80211_hdrlen(hdr->frame_control);

	/* make room for iv and move the ieee80211 header
	 * to the new location */
	hdr = (void *) skb_push(skb, iv_len);
	memmove(hdr, (u8 *) hdr + iv_len, hdr_len);

	/* the space for the IV is now right behind the
	 * IEEE 802.11 MAC header (QoS + HTC are optional)
	 */
	iv = ((u8 *) hdr) + hdr_len;

	switch (key->type) {
	case WEP40_ENCRYPTION:
	case WEP104_ENCRYPTION:
		/* filter weak wep iv */
		if ((key->wep.seq & 0xff00) == 0xff00) {
			u8 B = (key->wep.seq >> 16) & 0xff;
			if (B >= 3 && B < (3 + key->key_len))
				key->wep.seq += 0x0100;
		}

		iv[0] = (key->wep.seq >> 16) & 0xff;
		iv[1] = (key->wep.seq >> 8) & 0xff;
		iv[2] = (key->wep.seq) & 0xff;
		iv[3] = key->index;
		key->wep.seq++;
		break;

	case TKIP_ENCRYPTION:
		iv[0] = (key->tkip.tx_seq >> 8) & 0xff;
		iv[1] = ((key->tkip.tx_seq >> 8) | 0x20) & 0x7f;
		iv[2] = (key->tkip.tx_seq) & 0xff;
		iv[3] = (key->index << 6) | BIT(5 /* Ext IV */);
		iv[4] = (key->tkip.tx_seq >> 16) & 0xff;
		iv[5] = (key->tkip.tx_seq >> 24) & 0xff;
		iv[6] = (key->tkip.tx_seq >> 32) & 0xff;
		iv[7] = (key->tkip.tx_seq >> 40) & 0xff;
		key->tkip.tx_seq++;
		break;

	case AESCCMP_ENCRYPTION:
		iv[0] = (key->ccmp.tx_seq) & 0xff;
		iv[1] = (key->ccmp.tx_seq >> 8) & 0xff;
		iv[2] = 0;
		iv[3] = (key->index << 6) | BIT(5 /* Ext IV */);
		iv[4] = (key->ccmp.tx_seq >> 16) & 0xff;
		iv[5] = (key->ccmp.tx_seq >> 24) & 0xff;
		iv[6] = (key->ccmp.tx_seq >> 32) & 0xff;
		iv[7] = (key->ccmp.tx_seq >> 40) & 0xff;
		key->ccmp.tx_seq++;
		break;

	default:
		WARN(1, "invalid encryption type %d\n", key->type);
		return TX_DROP;
	}

	return TX_CONTINUE;
}

static enum r92su_tx_control_t
r92su_tx_add_icv_mic(struct r92su *r92su, struct sk_buff *skb,
			struct r92su_bss_priv *bss_priv)
{
	struct ieee80211_hdr *hdr = (void *) skb->data;
	struct r92su_tx_info *tx_info = r92su_get_tx_info(skb);
	struct r92su_key *key = tx_info->key;
	unsigned int data_len, hdr_len;
	u8 *data;

	if (!key)
		return TX_CONTINUE;

	hdr_len = ieee80211_hdrlen(hdr->frame_control);
	data_len = skb->len - hdr_len;
	data = ((u8 *) hdr) + hdr_len;

	switch (key->type) {
	case WEP40_ENCRYPTION:
	case WEP104_ENCRYPTION:
		/* done by the firmware/hardware, just alloc the space */
		memset(skb_put(skb, WEP_ICV_LEN), 0, WEP_ICV_LEN);
		tx_info->ht_possible = false;
		return TX_CONTINUE;

	case TKIP_ENCRYPTION:
		michael_mic(&key->tkip.key.key[NL80211_TKIP_DATA_OFFSET_TX_MIC_KEY],
			    hdr, data, data_len, skb_put(skb, MICHAEL_MIC_LEN));
		tx_info->ht_possible = false;
		return TX_CONTINUE;

	case AESCCMP_ENCRYPTION:
		/* done by the firmware/hardware, just alloc the space */
		memset(skb_put(skb, CCMP_MIC_LEN), 0, CCMP_MIC_LEN);
		return TX_CONTINUE;

	default:
		WARN(1, "invalid encryption type %d\n", key->type);
		return TX_DROP;
	}
}

static enum r92su_tx_control_t
r92su_tx_prepare_tx_info_and_find_sta(struct r92su *r92su, struct sk_buff *skb,
				      struct r92su_bss_priv *bss_priv)
{
	struct r92su_tx_info *tx_info = r92su_get_tx_info(skb);
	struct r92su_sta *sta;
	struct ethhdr *hdr = (void *) skb->data;

	/* clean up tx info */
	memset(tx_info, 0, sizeof(tx_info));

	sta = r92su_sta_get(r92su, hdr->h_dest);
	if (!sta) {
		sta = bss_priv->sta;
		if (!sta)
			return TX_DROP;

		/* We only support aggregation when we are talking to the AP */
		tx_info->ht_possible = sta->ht_sta;
	}

	tx_info->sta = sta;
	return TX_CONTINUE;
}

static enum r92su_tx_control_t
r92su_tx_rate_control_hint(struct r92su *r92su, struct sk_buff *skb,
			   struct r92su_bss_priv *bss_priv)
{
	/* The rate control starts out with the faster rates
	 * and if it doesn't work, it will move down.
	 * This means that the few, but very important PAE,
	 * ARP, DHCP frames which are in serious danger of
	 * being "dropped" on a weak link... And the
	 * connection could never be established... unless
	 * we force the firmware/hardware to use lower and
	 * more robust rates.
	 */
	bool low_rate = false;

	struct r92su_tx_info *tx_info = r92su_get_tx_info(skb);
	switch (skb->protocol) {
	case cpu_to_be16(ETH_P_PAE):
	case cpu_to_be16(ETH_P_ARP):
		low_rate = true;
		break;

	/* detect DHCP */
	case cpu_to_be16(ETH_P_IP): {
		struct iphdr *ip;

		ip = ((void *) skb->data) + ETH_HLEN;
		if (ip->protocol == IPPROTO_UDP) {
			struct udphdr *udp = (struct udphdr *)((u8 *) ip +
							       (ip->ihl << 2));
			if ((udp->source == cpu_to_be16(68) &&
			     udp->dest == cpu_to_be16(67)) ||
			    (udp->source == cpu_to_be16(67) &&
			     udp->dest == cpu_to_be16(68))) {
				/* 68 : UDP BOOTP client
				 * 67 : UDP BOOTP server
				 */
				low_rate = true;
			}
		}
		break;
	}
	}

	if (low_rate) {
		tx_info->ht_possible = false;
		tx_info->low_rate = true;
	}

	return TX_CONTINUE;
}

static enum r92su_tx_control_t
r92su_tx_add_80211(struct r92su *r92su, struct sk_buff *skb,
		   struct r92su_bss_priv *bss_priv)
{
	struct r92su_tx_info *tx_info = r92su_get_tx_info(skb);
	struct wireless_dev *wdev = &r92su->wdev;
	int err, tid;
	bool qos;

	qos = tx_info->sta->qos_sta;
	if (bss_priv->control_port_ethertype == skb->protocol)
		qos = false;

	err = ieee80211_data_from_8023(skb, wdev_address(wdev), wdev->iftype,
				       bss_priv->fw_bss.bssid, qos);
	if (err)
		return TX_DROP;

	if (qos) {
		struct ieee80211_hdr *hdr = (struct ieee80211_hdr *) skb->data;
		u8 *qos_ctl = ieee80211_get_qos_ctl(hdr);
		memset(qos_ctl, 0, 2);

		tid = skb->priority % ARRAY_SIZE(ieee802_1d_to_ac);
		qos_ctl[0] = tid;

		if (tx_info->ht_possible &&
		    skb_get_queue_mapping(skb) != IEEE80211_AC_VO) {
			if (!bss_priv->tx_tid[tid].addba_issued) {
				r92su_h2c_start_ba(r92su, tid);
				bss_priv->tx_tid[tid].addba_issued = true;
			} else {
				tx_info->ampdu = true;
				tx_info->tid = tid;
			}
		}
	}

	return TX_CONTINUE;
}

static enum r92su_tx_control_t
r92su_tx_fragment(struct r92su *r92su, struct sk_buff *skb,
		  struct r92su_bss_priv *bss_priv, struct sk_buff_head *queue)
{
	struct ieee80211_hdr *hdr;
	unsigned int limit = r92su->wdev.wiphy->frag_threshold;
	unsigned int hdr_len;
	unsigned int frag_num = 0;
	unsigned int per_frag;
	unsigned int pos;
	int rem;

	if ((skb->len + FCS_LEN) < limit) {
		__skb_queue_tail(queue, skb);
		return TX_CONTINUE;
	}

	hdr = (struct ieee80211_hdr *) skb->data;
	hdr_len = ieee80211_hdrlen(hdr->frame_control);
	per_frag = limit - hdr_len - FCS_LEN;
	pos = hdr_len;
	rem = skb->len - hdr_len - per_frag;

	if (WARN(rem <= 0, "can't make zero length fragment."))
		return TX_DROP;

	while (rem) {
		struct sk_buff *tmp;
		int frag_len = per_frag;

		if (frag_len > rem)
			frag_len = rem;

		rem -= frag_len;
		tmp = dev_alloc_skb(R92SU_TX_HEAD_ROOM +
				    limit +
				    R92SU_TX_TAIL_ROOM);
		if (!tmp)
			return TX_DROP;

		skb_reserve(tmp, TX_DESC_SIZE + 4 + 8); /* + align + iv */
		memcpy(tmp->cb, skb->cb, sizeof(tmp->cb));

		skb_copy_queue_mapping(tmp, skb);
		tmp->protocol = skb->protocol;
		tmp->priority = skb->priority;
		tmp->dev = skb->dev;

		memcpy(skb_put(tmp, hdr_len), skb->data, hdr_len);
		memcpy(skb_put(tmp, frag_len), skb->data + pos, frag_len);

		hdr = (struct ieee80211_hdr *) tmp->data;

		if (rem) {
			hdr->frame_control |=
				cpu_to_le16(IEEE80211_FCTL_MOREFRAGS);
		}

		hdr->seq_ctrl |= cpu_to_le16(frag_num++ & IEEE80211_SCTL_FRAG);

		pos += frag_len;

		__skb_queue_tail(queue, tmp);
	}

	/* no longer needed */
	dev_kfree_skb_any(skb);
	return TX_CONTINUE;
}

static enum r92su_tx_control_t
r92su_tx_select_key(struct r92su *r92su, struct sk_buff *skb,
		    struct r92su_bss_priv *bss_priv)
{
	struct r92su_tx_info *tx_info = r92su_get_tx_info(skb);
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *) skb->data;
	struct r92su_sta *sta = tx_info->sta;
	struct r92su_key *key = NULL;

	if (!bss_priv->sta->enc_sta)
		return TX_CONTINUE;

	if (is_multicast_ether_addr(ieee80211_get_DA(hdr))) {
		key = rcu_dereference(bss_priv->
			group_key[bss_priv->def_multi_key_idx]);
	} else {
		sta = r92su_sta_get(r92su, ieee80211_get_DA(hdr));
		if (sta)
			key = rcu_dereference(sta->sta_key);
	}

	if (!key)
		key = rcu_dereference(bss_priv->sta->sta_key);

	if (!key) {
		key = rcu_dereference(bss_priv->group_key
				[bss_priv->def_uni_key_idx]);
	}

	if (!key) {
		/* check control_port and control_port_no_encrypt ? */
		if (bss_priv->control_port_ethertype != skb->protocol)
			return TX_DROP;
		else
			return TX_CONTINUE;
	}

	hdr->frame_control |= cpu_to_le16(IEEE80211_FCTL_PROTECTED);
	tx_info->key = key;
	return TX_CONTINUE;
}

static enum r92su_tx_control_t
r92su_tx_invalidate_rcu_data(struct r92su *r92su, struct sk_buff *skb,
			     struct r92su_bss_priv *bss_priv)
{
	struct r92su_tx_info *tx_info = r92su_get_tx_info(skb);
	tx_info->sta = NULL;
	tx_info->key = NULL;
	return TX_CONTINUE;
}

void r92su_tx(struct r92su *r92su, struct sk_buff *skb)
{
	struct cfg80211_bss *bss;
	struct r92su_bss_priv *bss_priv;

	struct sk_buff_head in_queue;
	struct sk_buff_head out_queue;

#define __TX_HANDLER(func, drop_goto, args...) do {	\
	enum r92su_tx_control_t __ret;			\
							\
	__ret = func(r92su, skb, bss_priv, ## args);	\
	switch (__ret) {				\
	case TX_CONTINUE:				\
		break;					\
	case TX_DROP:					\
		goto drop_goto;				\
	}						\
} while (0)

#define TX_HANDLER_PREP(func, args...)			\
	__TX_HANDLER(func, err_unlock, ## args)

#define TX_HANDLER(func, args...)			\
	__TX_HANDLER(func, tx_drop, ## args)

	__skb_queue_head_init(&in_queue);
	__skb_queue_head_init(&out_queue);

	/* isn't this check sort of done by the caller already?! */
	if (skb->len < ETH_ALEN + ETH_ALEN + 2)
		goto err_out;

	if (!r92su_is_connected(r92su))
		goto err_out;

	rcu_read_lock();
	bss = rcu_dereference(r92su->connect_bss);
	if (!bss)
		goto err_unlock;

	bss_priv = r92su_get_bss_priv(bss);

	TX_HANDLER_PREP(r92su_tx_prepare_tx_info_and_find_sta);
	TX_HANDLER_PREP(r92su_tx_rate_control_hint);
	TX_HANDLER_PREP(r92su_tx_add_80211);
	TX_HANDLER_PREP(r92su_tx_select_key);
	TX_HANDLER_PREP(r92su_tx_fragment, &in_queue);

	while ((skb = __skb_dequeue(&in_queue))) {
		TX_HANDLER(r92su_tx_add_icv_mic);
		TX_HANDLER(r92su_tx_add_iv);
		TX_HANDLER(r92su_tx_fill_desc);
		TX_HANDLER(r92su_tx_invalidate_rcu_data);
		__skb_queue_tail(&out_queue, skb);
		continue;
tx_drop:
		r92su->wdev.netdev->stats.tx_dropped++;
		dev_kfree_skb_any(skb);
	}

	while ((skb = __skb_dequeue(&out_queue))) {
		struct tx_hdr *tx_hdr = (struct tx_hdr *)skb->data;

		r92su->wdev.netdev->stats.tx_packets++;
		r92su->wdev.netdev->stats.tx_bytes += skb->len;

		r92su_usb_tx(r92su, skb, tx_hdr->queue_sel);
	}
	rcu_read_unlock();
	return;

err_unlock:
	rcu_read_unlock();

err_out:
	__skb_queue_purge(&in_queue);
	__skb_queue_purge(&out_queue);
	r92su->wdev.netdev->stats.tx_dropped++;
	dev_kfree_skb_any(skb);

#undef TX_HANDLER
}

void r92su_tx_cb(struct r92su *r92su, struct sk_buff *skb)
{
}
