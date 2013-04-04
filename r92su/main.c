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
#include <linux/ieee80211.h>
#include <linux/etherdevice.h>
#include <linux/if_arp.h>
#include <net/cfg80211.h>

#include "r92su.h"
#include "reg.h"
#include "def.h"
#include "usb.h"
#include "cmd.h"
#include "sta.h"
#include "tx.h"
#include "rx.h"
#include "fw.h"
#include "hw.h"
#include "debugfs.h"

#define CHAN2G(_hw_value, _freq, _flags) {	\
	.band		= IEEE80211_BAND_2GHZ,	\
	.center_freq	= (_freq),		\
	.hw_value	= (_hw_value),		\
	.flags		= (_flags),		\
	.max_power	= 20,			\
}

#define RATE(_rate, _hw_value, _flags) {	\
	.bitrate	= (_rate),		\
	.hw_value	= (_hw_value),		\
	.flags		= (_flags),		\
}

static struct ieee80211_channel r92su_channeltable[] = {
	CHAN2G(1,  2412, 0),
	CHAN2G(2,  2417, 0),
	CHAN2G(3,  2422, 0),
	CHAN2G(4,  2427, 0),
	CHAN2G(5,  2432, 0),
	CHAN2G(6,  2437, 0),
	CHAN2G(7,  2442, 0),
	CHAN2G(8,  2447, 0),
	CHAN2G(9,  2452, 0),
	CHAN2G(10, 2457, 0),
	CHAN2G(11, 2462, 0),
	CHAN2G(12, 2467, 0),
	CHAN2G(13, 2472, 0),
	CHAN2G(14, 2484, 0),
};

static struct ieee80211_rate r92su_ratetable[] = {
	RATE(10,  0, 0),
	RATE(20,  1, IEEE80211_RATE_SHORT_PREAMBLE),
	RATE(55,  2, IEEE80211_RATE_SHORT_PREAMBLE),
	RATE(110, 3, IEEE80211_RATE_SHORT_PREAMBLE),
	RATE(60,  4, 0),
	RATE(90,  5, 0),
	RATE(120, 6, 0),
	RATE(180, 7, 0),
	RATE(240, 8, 0),
	RATE(360, 9, 0),
	RATE(480, 10, 0),
	RATE(540, 11, 0),
};

#undef CHAN2G
#undef RATE

static const u32 r92su_chiper_suites[] = {
	WLAN_CIPHER_SUITE_WEP40,
	WLAN_CIPHER_SUITE_WEP104,
	WLAN_CIPHER_SUITE_TKIP,
	WLAN_CIPHER_SUITE_CCMP,
};

static const struct ieee80211_sta_ht_cap r92su_ht_info = {
	.ht_supported = true,
	.cap = IEEE80211_HT_CAP_SUP_WIDTH_20_40 |
	       IEEE80211_HT_CAP_DSSSCCK40 |
	       IEEE80211_HT_CAP_SM_PS |
	       IEEE80211_HT_CAP_MAX_AMSDU |
	       (1 << IEEE80211_HT_CAP_RX_STBC_SHIFT) |
	       IEEE80211_HT_CAP_TX_STBC |
	       IEEE80211_HT_CAP_SGI_40 |
	       IEEE80211_HT_CAP_SGI_20,
	.ampdu_factor = IEEE80211_HT_MAX_AMPDU_32K,
	.ampdu_density = IEEE80211_HT_MPDU_DENSITY_16,
	.mcs = {
		.tx_params = IEEE80211_HT_MCS_TX_DEFINED,
		.rx_mask = { 0xff, 00, 00, 00, 0x01, 0x00, 0x00, 0x00 },
		.rx_highest = cpu_to_le16(150),
	},
};

static int r92su_get_station(struct wiphy *wiphy, struct net_device *ndev,
			     u8 *mac, struct station_info *sinfo)
{
	struct r92su *r92su = wiphy_priv(wiphy);
	int err = -EAGAIN;

	mutex_lock(&r92su->lock);
	if (!r92su_is_connected(r92su))
		goto out;

	err = -EOPNOTSUPP;

out:
	mutex_unlock(&r92su->lock);
	return err;

}

static bool r92su_parse_ht_cap_ie(struct r92su *r92su, u8 *ies, const u32 len)
{
	u8 *ht_cap_ie;

	ht_cap_ie = r92su_find_ie(ies, len, WLAN_EID_HT_CAPABILITY);
	if (!ht_cap_ie || ht_cap_ie[1] != sizeof(struct ieee80211_ht_cap))
		return false;

	return true;
}

static u8 *r92su_find_wmm_ie(u8 *ies, const u32 len)
{
	u8 *wmm_ie = ies;
	u8 *end = ies + len;

	while (wmm_ie && wmm_ie < end) {
		wmm_ie = r92su_find_ie(wmm_ie, len, WLAN_EID_VENDOR_SPECIFIC);
		if (wmm_ie) {
			u8 elen = wmm_ie[1];
			u8 *pos = &wmm_ie[2];

			if (elen >= 7 &&
			    pos[0] == 0x00 && pos[1] == 0x50 &&
			    pos[2] == 0xf2 /* Microsoft OUI */ &&
			    pos[3] == 0x02 &&
			    pos[5] == 1 /* Version Check */)
				return wmm_ie;

			wmm_ie = wmm_ie + 2 + wmm_ie[1];
		}

		return NULL;
	}

	return NULL;
}

static bool r92su_add_ies(struct r92su *r92su, u8 **ie,
			 u32 *ie_len_left, const u8 *add_ies,
			 const u32 add_ies_len)
{
	if (*ie_len_left < add_ies_len)
		return false;

	*ie_len_left -= add_ies_len;
	memcpy(*ie, add_ies, add_ies_len);
	*ie += add_ies_len;
	return true;

}

static bool r92su_add_ie(struct r92su *r92su, u8 ied, u8 **ie,
			 u32 *ie_len_left, const u8 *add_ie,
			 const u32 add_ie_len)
{
	unsigned int ie_total_len = add_ie_len + 2;

	if (*ie_len_left < ie_total_len)
		return false;

	*ie_len_left -= ie_total_len;

	*(*ie)++ = ied;
	*(*ie)++ = add_ie_len;
	memcpy(*ie, add_ie, add_ie_len);
	*ie += add_ie_len;
	return true;
}

static bool r92su_ht_update(struct r92su *r92su, u8 **ie, u32 *ie_len_left)
{
	struct ieee80211_ht_cap ht_cap = { };
	struct ieee80211_sta_ht_cap *me_ht;

	me_ht = &r92su->band_2GHZ.ht_cap;

	ht_cap.cap_info = cpu_to_le16(me_ht->cap);
	ht_cap.ampdu_params_info = ((me_ht->ampdu_density <<
				    IEEE80211_HT_AMPDU_PARM_DENSITY_SHIFT) &
				    IEEE80211_HT_AMPDU_PARM_DENSITY) |
				    (me_ht->ampdu_factor &
				    IEEE80211_HT_AMPDU_PARM_FACTOR);
	memcpy(&ht_cap.mcs, &r92su->band_2GHZ.ht_cap.mcs,
	       sizeof(ht_cap.mcs));

	return r92su_add_ie(r92su, WLAN_EID_HT_CAPABILITY, ie, ie_len_left,
			    (u8 *) &ht_cap, sizeof(ht_cap));
}

static bool r92su_wmm_update(struct r92su *r92su, u8 **ie, u32 *ie_len_left)
{
	static const u8 wmm_ie_add[] = {
		0x00, 0x50, 0xf2,	/* Microsoft OUI */
		0x02,			/* Information Element */
		0x00,
		0x01,			/* WME Version */
		0x00			/* WME QoS Info */
	};

	/* The firmware does something strange with the IE and
	 * it shouldn't be changed */
	return r92su_add_ie(r92su, WLAN_EID_VENDOR_SPECIFIC, ie, ie_len_left,
			    wmm_ie_add, ARRAY_SIZE(wmm_ie_add));
}

static int r92su_connect_set_auth(struct r92su *r92su,
				  struct cfg80211_connect_params *sme)
{
	enum r92su_auth_mode auth_mode;
	enum r92su_auth_1x _1x;

	switch (sme->auth_type) {
	case NL80211_AUTHTYPE_AUTOMATIC:
	case NL80211_AUTHTYPE_OPEN_SYSTEM:
		if (sme->crypto.wpa_versions > 0) {
			auth_mode = R92SU_AUTH_8021X;
			/* TODO: get the right 802.1x mode */
			_1x = R92SU_WPA_PSK;
		} else {
			auth_mode = R92SU_AUTH_OPEN;
			_1x = R92SU_WPA_PSK; /* dummy */
		}
		break;

	case NL80211_AUTHTYPE_SHARED_KEY:
		auth_mode = R92SU_AUTH_SHARED;
		_1x = R92SU_WPA_PSK; /* dummy */
		break;

	default:
		wiphy_err(r92su->wdev.wiphy, "Invalid auth type %d\n",
			  sme->auth_type);
		return -EINVAL;
	}

	return r92su_h2c_set_auth(r92su, auth_mode, _1x);
}

static int r92su_connect_set_shared_key(struct r92su *r92su,
					struct cfg80211_connect_params *sme)
{
	enum r92su_enc_alg algo;

	if (!sme->key_len || !sme->key)
		return 0;

	switch (sme->key_len) {
	case WLAN_KEY_LEN_WEP40:
		algo = WEP40_ENCRYPTION;
		break;
	case WLAN_KEY_LEN_WEP104:
		algo = WEP104_ENCRYPTION;
		break;

	default:
		wiphy_err(r92su->wdev.wiphy, "Invalid shared key\n");
		return -EINVAL;
	}

	return r92su_h2c_set_key(r92su, algo, sme->key_idx, true,
				sme->key);
}

static int r92su_connect(struct wiphy *wiphy, struct net_device *ndev,
			 struct cfg80211_connect_params *sme)
{
	struct r92su *r92su = wiphy_priv(wiphy);
	struct cfg80211_bss *bss = NULL;
	struct r92su_bss_priv *bss_priv = NULL;
	int i, err = -EAGAIN;
	u8 ie_buf[256];
	u8 *ie = ie_buf;
	u32 ie_len_left = sizeof(ie_buf);

	mutex_lock(&r92su->lock);
	if (!r92su_is_open(r92su))
		goto out;

	bss = cfg80211_get_bss(wiphy, sme->channel, sme->bssid,
			       sme->ssid, sme->ssid_len,
			       WLAN_CAPABILITY_ESS, WLAN_CAPABILITY_ESS);
	if (!bss) {
		err = -ENOENT;
		goto out;
	}

	bss_priv = r92su_get_bss_priv(bss);
	err = r92su_connect_set_auth(r92su, sme);
	if (err)
		goto out;

	err = r92su_connect_set_shared_key(r92su, sme);
	if (err)
		goto out;

	for (i = 0; i < ARRAY_SIZE(bss_priv->tx_tid); i++)
		skb_queue_head_init(&bss_priv->tx_tid[i].agg_queue);

	bss_priv->control_port = sme->crypto.control_port;
	bss_priv->control_port_ethertype = sme->crypto.control_port_ethertype;
	bss_priv->control_port_no_encrypt = sme->crypto.control_port_no_encrypt;

	WARN(!r92su_add_ies(r92su, &ie, &ie_len_left, sme->ie, sme->ie_len),
	     "no space left for cfg80211's ies");

	if (!(sme->flags & ASSOC_REQ_DISABLE_HT)) {
		WARN(!r92su_ht_update(r92su, &ie, &ie_len_left),
		     "no space left for ht caps ie");
	}

	WARN(!r92su_wmm_update(r92su, &ie, &ie_len_left),
	     "no space left for wmm ie");

	bss_priv->assoc_ie_len = ie - ie_buf;
	bss_priv->assoc_ie = kmemdup(ie_buf, bss_priv->assoc_ie_len,
				     GFP_KERNEL);
	if (!bss_priv->assoc_ie)
		goto out;

	r92su->want_connect_bss = bss;
	err = r92su_h2c_connect(r92su, &bss_priv->fw_bss, ie_buf, ie - ie_buf);
	if (err)
		goto out;

out:
	if (err) {
		if (bss_priv)
			kfree(bss_priv->assoc_ie);

		r92su->want_connect_bss = NULL;
	}

	mutex_unlock(&r92su->lock);

	if (bss)
		cfg80211_put_bss(wiphy, bss);
	return err;
}

static void r92su_bss_free(struct r92su *r92su, struct cfg80211_bss *bss)
{
	struct r92su_bss_priv *bss_priv;
	int i;

	if (!bss)
		return;

	bss_priv = r92su_get_bss_priv(bss);

	for (i = 0; i < ARRAY_SIZE(bss_priv->tx_tid); i++)
		skb_queue_purge(&bss_priv->tx_tid[i].agg_queue);

	kfree(bss_priv->assoc_ie);

	rcu_read_lock();
	for (i = 0; i < ARRAY_SIZE(bss_priv->group_key); i++) {
		struct r92su_key *key;
		key = rcu_dereference(bss_priv->group_key[i]);
		rcu_assign_pointer(bss_priv->group_key[i], NULL);
		r92su_key_free(key);
	}
	rcu_read_unlock();
}

static int r92su_disconnect(struct wiphy *wiphy, struct net_device *ndev,
			    u16 reason_code)
{
	struct r92su *r92su = wiphy_priv(wiphy);
	struct cfg80211_bss *old_bss;
	int err = -EAGAIN;

	mutex_lock(&r92su->lock);
	if (!r92su_is_connected(r92su))
		goto out;

	err = r92su_h2c_disconnect(r92su);
	if (err)
		goto out;

	old_bss = rcu_dereference_protected(r92su->connect_bss,
					    lockdep_is_held(&r92su->lock));
	rcu_assign_pointer(r92su->connect_bss, NULL);
	r92su_bss_free(r92su, old_bss);

	synchronize_rcu();
out:
	mutex_unlock(&r92su->lock);
	return err;
}

/* called from irq-context */
void r92su_disconnect_bss_event(struct r92su *r92su)
{
	struct cfg80211_bss *bss;

	cfg80211_disconnected(r92su->wdev.netdev,
			      WLAN_STATUS_UNSPECIFIED_FAILURE, NULL, 0,
			      GFP_ATOMIC);

	if (r92su_is_connected(r92su))
		r92su_set_state(r92su, R92SU_OPEN);

	rcu_read_lock();
	bss = rcu_dereference(r92su->connect_bss);
	rcu_assign_pointer(r92su->connect_bss, NULL);
	r92su_bss_free(r92su, bss);
	rcu_read_unlock();

	netif_tx_stop_all_queues(r92su->wdev.netdev);
	netif_carrier_off(r92su->wdev.netdev);
}

static void r92su_add_bss_work(struct work_struct *work)
{
	struct r92su *r92su;
	struct llist_node *node;

	r92su = container_of(work, struct r92su, add_bss_work);
	node = llist_del_all(&r92su->add_bss_list);
	while (node) {
		const struct h2cc2h_bss *c2h_bss;
		struct r92su_add_bss *bss_priv;
		struct r92su_bss_priv *cfg_priv;
		struct cfg80211_bss *bss;
		int chan_idx;
		int ie_len;

		bss_priv = llist_entry(node, struct r92su_add_bss, head);
		c2h_bss = &bss_priv->fw_bss;

		chan_idx = le32_to_cpu(c2h_bss->config.frequency) - 1;
		if (chan_idx < 0 || chan_idx >= r92su->band_2GHZ.n_channels) {
			wiphy_err(r92su->wdev.wiphy, "received survey event on bad channel.");
			goto next;
		}

		ie_len = le32_to_cpu(c2h_bss->ie_length) - 12;
		if (ie_len < 0)
			goto next;

		bss = cfg80211_inform_bss(r92su->wdev.wiphy,
			&r92su->band_2GHZ.channels[chan_idx], c2h_bss->bssid,
			le64_to_cpu(c2h_bss->ies.timestamp),
			le16_to_cpu(c2h_bss->ies.caps),
			le32_to_cpu(c2h_bss->config.beacon_period),
			c2h_bss->ies.ie, ie_len,
			le32_to_cpu(c2h_bss->rssi), GFP_KERNEL);

		cfg_priv = (void *) bss->priv;
		memcpy(&cfg_priv->fw_bss, c2h_bss, sizeof(*c2h_bss));

		if (bss)
			cfg80211_put_bss(r92su->wdev.wiphy, bss);

next:
		node = ACCESS_ONCE(node->next);

		/* these bss_priv have been generated by "c2h_survey_event"
		 * they are not part of the cfg80211 framework and this is
		 * why we have to managed & destroy them.
		 */
		kfree(bss_priv);
	}
}

static bool r92su_parse_wmm_cap_ie(struct r92su *r92su, u8 *ies, const u32 len)
{
	return r92su_find_wmm_ie(ies, len) != NULL;
}

static void r92su_connect_bss_work(struct work_struct *work)
{
	struct r92su *r92su;
	struct c2h_join_bss_event *join_bss = NULL;
	struct cfg80211_bss *cfg_bss;
	struct r92su_bss_priv *bss_priv;
	u8 *resp_ie = NULL;
	unsigned int resp_ie_len = 0;
	u16 status = WLAN_STATUS_UNSPECIFIED_FAILURE;

	r92su = container_of(work, struct r92su, connect_bss_work);

	mutex_lock(&r92su->lock);
	if (!r92su_is_open(r92su))
		goto out;

	cfg_bss = r92su->want_connect_bss;
	bss_priv = r92su_get_bss_priv(cfg_bss);
	join_bss = r92su->connect_result;

	if (!cfg_bss || !join_bss)
		goto out;

	r92su->connect_result = NULL;

	if (le32_to_cpu(join_bss->bss.ie_length) < 12)
		goto report_cfg80211;

	if (join_bss->join_result) {
		struct r92su_bss_priv *bss_priv = r92su_get_bss_priv(cfg_bss);
		struct r92su_sta *sta;

		sta = r92su_sta_alloc(r92su, join_bss->bss.bssid,
			5 /* seems like the FW has this hardcoded */,
			GFP_KERNEL);
		if (!sta)
			goto report_cfg80211;

		resp_ie = join_bss->bss.ies.ie;
		resp_ie_len = le32_to_cpu(join_bss->bss.ie_length) - 12;

		sta->aid = le32_to_cpu(join_bss->aid);
		sta->enc_sta = le32_to_cpu(join_bss->bss.privacy) ?
			       true : false;
		sta->qos_sta = r92su_parse_wmm_cap_ie(r92su, resp_ie,
						      resp_ie_len);

		/* The 802.11-2012 spec says that a HT STA has to be QoS STA
		 * as well. So in theory we should do instead:
		 *	sta->qos_sta |= sta->ht_sta;
		 * However, the QoS parameters are needed for legacy STAs as
		 * well. Therefore, there's no excuse for a HT STA to forget
		 * the WMM IE!
		 */
		if (sta->qos_sta)
			sta->ht_sta = r92su_parse_ht_cap_ie(r92su, resp_ie,
							    resp_ie_len);

		r92su_sta_add(r92su, sta);
		status = WLAN_STATUS_SUCCESS;

		bss_priv->sta = sta;
		rcu_assign_pointer(r92su->connect_bss, r92su->want_connect_bss);
		r92su->want_connect_bss = NULL;
		r92su_set_state(r92su, R92SU_CONNECTED);
	}

report_cfg80211:
	cfg80211_connect_result(r92su->wdev.netdev, join_bss->bss.bssid,
		bss_priv->assoc_ie, bss_priv->assoc_ie_len,
		resp_ie, resp_ie_len, status, GFP_KERNEL);

	kfree(bss_priv->assoc_ie);
	bss_priv->assoc_ie = NULL;

out:
	mutex_unlock(&r92su->lock);
	kfree(join_bss);

	if (status == WLAN_STATUS_SUCCESS) {
		netif_tx_start_all_queues(r92su->wdev.netdev);
		netif_carrier_on(r92su->wdev.netdev);
	}
}

#define R92SU_SCAN_TIMEOUT	5000

static int r92su_scan(struct wiphy *wiphy, struct cfg80211_scan_request *req)
{
	struct r92su *r92su = wiphy_priv(wiphy);
	int err = -EINVAL;

	mutex_lock(&r92su->lock);
	if (!r92su_is_open(r92su)) {
		err = -EINVAL;
		goto out;
	}

	if (r92su->scan_request) {
		err = -EAGAIN;
		goto out;
	}

	r92su->scan_request = req;
	err = r92su_h2c_survey(r92su, req->n_ssids == 1 ? req->ssids : NULL);
	if (err) {
		r92su->scan_request = NULL;
		goto out;
	}

	queue_delayed_work(r92su->wq, &r92su->survey_done_work,
			   msecs_to_jiffies(R92SU_SCAN_TIMEOUT));

	err = 0;
out:
	mutex_unlock(&r92su->lock);
	return err;
}

static int r92su_change_virtual_intf(struct wiphy *wiphy,
				     struct net_device *ndev,
				     enum nl80211_iftype type, u32 *flags,
				     struct vif_params *params)
{
	struct r92su *r92su = wiphy_priv(wiphy);
	int err = -EAGAIN;

	mutex_lock(&r92su->lock);
	if (!r92su_is_stopped(r92su))
		goto out;

	switch (type) {
	case NL80211_IFTYPE_MONITOR:
		ndev->type = ARPHRD_IEEE80211_RADIOTAP;
		break;

	case NL80211_IFTYPE_STATION:
	case NL80211_IFTYPE_ADHOC:
		ndev->type = ARPHRD_ETHER;
		break;

	default:
		err = -EOPNOTSUPP;
		goto out;
	}

	if (r92su_is_open(r92su)) {
		err = -EBUSY;
		goto out;
	}

	r92su->wdev.iftype = type;
	err = 0;

out:
	mutex_unlock(&r92su->lock);
	return err;
}

static int _r92su_set_channel(struct r92su *r92su)
{
	struct ieee80211_channel *chan = r92su->current_channel;
	if (!chan)
		return 0;

	return r92su_h2c_set_channel(r92su, chan->hw_value);
}

static int r92su_set_monitor_channel(struct wiphy *wiphy,
				     struct cfg80211_chan_def *chandef)
{
	struct r92su *r92su = wiphy_priv(wiphy);
	int err = -EAGAIN;

	mutex_lock(&r92su->lock);

	r92su->current_channel = chandef->chan;

	if (!r92su_is_open(r92su)) {
		/* the channel can be set while the device is not up
		 * therefore cache the new channel, until the device
		 * goes up.
		 */
		err = 0;
		goto out;
	}

	err = _r92su_set_channel(r92su);
out:
	mutex_unlock(&r92su->lock);

	return err;
}

static int r92su_add_key(struct wiphy *wiphy, struct net_device *ndev,
			 u8 idx, bool pairwise, const u8 *mac_addr,
			 struct key_params *params)
{
	struct r92su *r92su = wiphy_priv(wiphy);
	struct r92su_sta *sta;
	int err = -EAGAIN;
	struct r92su_key *new_key, *old_key = NULL;

	new_key = r92su_key_alloc(params->cipher, idx, mac_addr, pairwise,
				  params->key);
	if (IS_ERR(new_key))
		return PTR_ERR(new_key);

	mutex_lock(&r92su->lock);
	if (!r92su_is_connected(r92su)) {
		err = -EAGAIN;
		goto out_unlock;
	}

	if (pairwise) {
		err = r92su_h2c_set_sta_key(r92su, new_key->type,
					    new_key->mac_addr,
					    params->key);
		if (err)
			goto out_unlock;

		rcu_read_lock();
		sta = r92su_sta_get(r92su, mac_addr);
		if (!sta) {
			rcu_read_unlock();
			err = -EINVAL;
			goto out_unlock;
		}
		old_key = rcu_dereference(sta->sta_key);
		rcu_assign_pointer(sta->sta_key, new_key);
		rcu_read_unlock();
	} else {
		struct cfg80211_bss *bss;
		struct r92su_bss_priv *bss_priv;
		err = r92su_h2c_set_key(r92su, new_key->type,
					new_key->index,
					!new_key->pairwise,
					params->key);
		if (err)
			goto out_unlock;

		bss = rcu_dereference(r92su->connect_bss);
		bss_priv = r92su_get_bss_priv(bss);
		old_key = rcu_dereference(bss_priv->group_key[idx]);
		rcu_assign_pointer(bss_priv->group_key[idx],
				   new_key);
	}

	r92su_key_free(old_key);

out_unlock:
	mutex_unlock(&r92su->lock);
	if (err)
		kfree(new_key);
	return err;
}

static int r92su_del_key(struct wiphy *wiphy, struct net_device *ndev,
			 u8 idx, bool pairwise, const u8 *mac_addr)
{
	static const enum r92su_enc_alg no_key = NO_ENCRYPTION;
	struct r92su *r92su = wiphy_priv(wiphy);
	struct r92su_key *old_key = NULL;
	int err = -EAGAIN;

	mutex_lock(&r92su->lock);
	if (!r92su_is_connected(r92su))
		goto out;

	if (pairwise) {
		struct r92su_sta *sta;
		err = r92su_h2c_set_sta_key(r92su, no_key, mac_addr,
					    NULL);
		if (err)
			goto out;

		rcu_read_lock();
		sta = r92su_sta_get(r92su, mac_addr);
		if (!sta)
			goto out_free;

		old_key = rcu_dereference(sta->sta_key);
		rcu_assign_pointer(sta->sta_key, NULL);
	} else {
		struct cfg80211_bss *bss;
		struct r92su_bss_priv *bss_priv;
		err = r92su_h2c_set_key(r92su, no_key, idx, !pairwise,
					NULL);
		if (err)
			goto out;

		rcu_read_lock();
		bss = rcu_dereference(r92su->connect_bss);
		if (bss) {
			bss_priv = r92su_get_bss_priv(bss);
			old_key = rcu_dereference(bss_priv->group_key[idx]);
			rcu_assign_pointer(bss_priv->group_key[idx], NULL);
		} else {
			/* BSS which held the key is already gone! */
		}
	}

out_free:
	r92su_key_free(old_key);
	rcu_read_unlock();

out:
	mutex_unlock(&r92su->lock);
	return err;
}

static int r92su_set_default_key(struct wiphy *wiphy, struct net_device *ndev,
				 u8 idx, bool unicast, bool multicast)
{
	struct r92su *r92su = wiphy_priv(wiphy);
	struct cfg80211_bss *bss;
	struct r92su_bss_priv *bss_priv;
	int err = -EAGAIN;

	mutex_lock(&r92su->lock);
	if (!r92su_is_connected(r92su))
		goto out;

	rcu_read_lock();
	bss = rcu_dereference(r92su->connect_bss);
	if (!bss)
		goto out_rcu;

	bss_priv = r92su_get_bss_priv(bss);

	if (unicast)
		bss_priv->def_uni_key_idx = idx;

	if (multicast)
		bss_priv->def_multi_key_idx = idx;

	err = 0;
out_rcu:
	rcu_read_unlock();

out:
	mutex_unlock(&r92su->lock);
	return err;
}

static int r92su_set_wiphy_params(struct wiphy *wiphy, u32 changed)
{
	struct r92su *r92su = wiphy_priv(wiphy);

	if (changed & WIPHY_PARAM_FRAG_THRESHOLD) {
		/* For some reason, the firmware increases the sequence
		 * counter for fragments. This breaks the defragmentation
		 * on the receiver because all fragments have to have the
		 * same sequence couter.
		 */
		WARN_ONCE(1, "fragmentation is not supported");

		/* for now, reset fragmentation threshold. However, once the
		 * fragmentation bug get fixed, this can be removed altogether
		 + and "everything should just work (tm)".
		 */
		r92su->wdev.wiphy->frag_threshold =
			IEEE80211_MAX_FRAG_THRESHOLD;
		return 0;
	} else
		return -EOPNOTSUPP;
}

static const struct cfg80211_ops r92su_cfg80211_ops = {
	.change_virtual_intf = r92su_change_virtual_intf,
	.set_monitor_channel = r92su_set_monitor_channel,

	.get_station = r92su_get_station,

	.add_key = r92su_add_key,
	.del_key = r92su_del_key,
	.set_default_key = r92su_set_default_key,

	.scan = r92su_scan,
	.connect = r92su_connect,
	.disconnect = r92su_disconnect,

	.set_wiphy_params = r92su_set_wiphy_params,
};

static const void *r92su_priv_id = &r92su_priv_id;

static void r92su_set_rx_mode(struct net_device *ndev)
{
	struct r92su *r92su = ndev->ml_priv;

	if (!r92su_is_open(r92su))
		return;
}

static int r92su_init_mac(struct r92su *r92su)
{
	enum h2c_op_modes fw_mode;
	int err;
	bool data = true, all_mgmt = false, ctrl = false, mntr = false;

	switch (r92su->wdev.iftype) {
	case NL80211_IFTYPE_MONITOR:
		all_mgmt = true;
		ctrl = true;
		mntr = true;
	case NL80211_IFTYPE_UNSPECIFIED:
		fw_mode = OP_AUTO;
		break;
	case NL80211_IFTYPE_ADHOC:
		fw_mode = OP_ADHOC;
		break;
	case NL80211_IFTYPE_STATION:
		fw_mode = OP_INFRA;
		break;
	default:
		return -EOPNOTSUPP;
	}

	err = r92su_hw_mac_set_rx_filter(r92su, data, all_mgmt, ctrl, mntr);
	if (err)
		return err;

	/* enable video mode, 40MHz mode and STBC
	 * bit 8:
	 *  1 -> enable video mode to 96B AP
	 *  0 -> disable video mode to 96B AP
	 *  bit 9:
	 *  1 -> enable 40MHz mode
	 *  0 -> disable 40MHz mode
	 *  bit 10:
	 *  1 -> enable STBC
	 *  0 -> disable STBC
	 */
	err = r92su_fw_iocmd(r92su, 0xf4000700);
	if (err)
		return err;

	err = r92su_h2c_set_opmode(r92su, fw_mode);
	if (err)
		return err;

	err = r92su_h2c_set_mac_addr(r92su, r92su->wdev.netdev->dev_addr);
	if (err)
		return err;

	err = _r92su_set_channel(r92su);
	if (err)
		return err;

	return err;
}

static int r92su_open(struct net_device *ndev)
{
	struct r92su *r92su = ndev->ml_priv;
	int err = -EAGAIN;

	if (!is_valid_ether_addr(ndev->dev_addr))
		return -EADDRNOTAVAIL;

	/* Since the firmware starts sending frames as soon as its initialized,
	 * we have to have a valid current_channel set, otherwise the rx-path
	 * would panic
	 */
	r92su->current_channel =
		&r92su->wdev.wiphy->bands[IEEE80211_BAND_2GHZ]->channels[0];

	err = r92su_load_firmware(r92su);
	if (err)
		return err;

	mutex_lock(&r92su->lock);
	if (!r92su_is_stopped(r92su)) {
		err = -EAGAIN;
		goto out;
	}

	err = r92su_hw_early_mac_setup(r92su);
	if (err)
		goto out;

	err = r92su_upload_firmware(r92su);
	if (err)
		goto out;

	err = r92su_hw_late_mac_setup(r92su);
	if (err)
		goto out;

	err = r92su_init_mac(r92su);
	if (err)
		goto out;

	if (!r92su_is_stopped(r92su)) {
		err = -EAGAIN;
		goto out;
	}

	switch (r92su->wdev.iftype) {
	case NL80211_IFTYPE_MONITOR:
		r92su_set_state(r92su, R92SU_CONNECTED);
		break;

	default:
		r92su_set_state(r92su, R92SU_OPEN);
		break;
	}

	r92su_hw_queue_service_work(r92su);
out:
	mutex_unlock(&r92su->lock);
	return err;
}

static void r92su_survey_done_work(struct work_struct *work)
{
	struct cfg80211_scan_request *req;
	struct r92su *r92su = container_of(work, struct r92su,
					   survey_done_work.work);

	mutex_lock(&r92su->lock);
	if (!r92su_is_open(r92su))
		goto out;

	req = r92su->scan_request;
	r92su->scan_request = NULL;

	if (req)
		cfg80211_scan_done(req, req->aborted);

out:
	mutex_unlock(&r92su->lock);
}

static int r92su_stop(struct net_device *ndev)
{
	struct r92su *r92su = ndev->ml_priv;
	int err = -EINVAL, i;

	mutex_lock(&r92su->lock);
	if (r92su_is_initializing(r92su)) {
		err = r92su_hw_mac_deinit(r92su);
		if (err)
			goto out;
	}

	if (r92su_is_initializing(r92su))
		r92su_set_state(r92su, R92SU_STOP);

	if (r92su->scan_request)
		cfg80211_scan_done(r92su->scan_request, true);

	r92su_bss_free(r92su, r92su->want_connect_bss);
	r92su->scan_request = NULL;
	r92su->want_connect_bss = NULL;

	for (i = 0; i < ARRAY_SIZE(r92su->sta_table); i++)
		r92su_sta_del(r92su, i);
out:
	mutex_unlock(&r92su->lock);

	cancel_delayed_work_sync(&r92su->survey_done_work);
	cancel_delayed_work_sync(&r92su->service_work);
	cancel_work_sync(&r92su->add_bss_work);
	cancel_work_sync(&r92su->connect_bss_work);

	/* wait for keys and stas to be freed */
	synchronize_rcu();

	return err;
}

static netdev_tx_t r92su_start_xmit(struct sk_buff *skb,
				    struct net_device *ndev)
{
	struct r92su *r92su = ndev->ml_priv;
	r92su_tx(r92su, skb);
	return NETDEV_TX_OK;
}

static const struct net_device_ops r92su_netdevice_ops = {
	.ndo_open = r92su_open,
	.ndo_stop = r92su_stop,
	.ndo_start_xmit = r92su_start_xmit,
	.ndo_set_mac_address = eth_mac_addr,
	.ndo_set_rx_mode = r92su_set_rx_mode,
	.ndo_change_mtu = eth_change_mtu,
	.ndo_validate_addr = eth_validate_addr,
};

static void *devm_dup(struct device *dev, void *src, size_t len)
{
	void *tmp;

	tmp = devm_kzalloc(dev, len, GFP_KERNEL);
	if (tmp)
		memcpy(tmp, src, len);
	return tmp;
}

static int r92su_init_band(struct r92su *r92su)
{
	struct ieee80211_supported_band *band;

	band = &r92su->band_2GHZ;
	band->channels = devm_dup(&r92su->wdev.wiphy->dev,
		r92su_channeltable, sizeof(r92su_channeltable));
	if (!band->channels)
		return -ENOMEM;

	band->bitrates = devm_dup(&r92su->wdev.wiphy->dev,
		r92su_ratetable, sizeof(r92su_ratetable));
	if (!band->bitrates)
		return -ENOMEM;

	band->n_channels = ARRAY_SIZE(r92su_channeltable);
	band->n_bitrates = ARRAY_SIZE(r92su_ratetable);

	memcpy(&band->ht_cap, &r92su_ht_info, sizeof(r92su_ht_info));
	switch (r92su->rf_type) {
	case R92SU_1T1R:
		/* nothing needs to be done. The default ht_cap
		 * contains all the necessary bits for just 1T1R
		 * devices */
		break;

	case R92SU_1T2R:
	case R92SU_2T2R:
		band->ht_cap.mcs.rx_mask[1] = 0xff;
		band->ht_cap.mcs.rx_highest = cpu_to_le16(300);
		break;
	}

	r92su->wdev.wiphy->bands[IEEE80211_BAND_2GHZ] = &r92su->band_2GHZ;

	return 0;
}

struct r92su *r92su_alloc(struct device *main_dev)
{
	struct r92su *r92su = NULL;
	struct wiphy *wiphy;
	int err;

	wiphy = wiphy_new(&r92su_cfg80211_ops, sizeof(struct r92su));
	if (!wiphy) {
		err = -ENOMEM;
		goto err_out;
	}

	r92su = wiphy_priv(wiphy);
	r92su->wdev.wiphy = wiphy;
	mutex_init(&r92su->lock);
	spin_lock_init(&r92su->rx_path);

	set_wiphy_dev(r92su->wdev.wiphy, main_dev);
	r92su->wdev.iftype = NL80211_IFTYPE_STATION;

	wiphy->privid = r92su_priv_id;
	wiphy->interface_modes = BIT(NL80211_IFTYPE_STATION) |
				 BIT(NL80211_IFTYPE_MONITOR);
	wiphy->max_scan_ssids = 1;
	wiphy->max_scan_ie_len = 256;
	wiphy->signal_type = CFG80211_SIGNAL_TYPE_UNSPEC;
	wiphy->cipher_suites = r92su_chiper_suites;
	wiphy->n_cipher_suites = ARRAY_SIZE(r92su_chiper_suites);
	wiphy->bss_priv_size = sizeof(struct r92su_bss_priv);

	init_llist_head(&r92su->add_bss_list);
	INIT_WORK(&r92su->add_bss_work, r92su_add_bss_work);
	INIT_WORK(&r92su->connect_bss_work, r92su_connect_bss_work);
	INIT_DELAYED_WORK(&r92su->survey_done_work, r92su_survey_done_work);
	r92su_hw_init(r92su);

	r92su->wq = create_singlethread_workqueue(R92SU_DRVNAME);
	if (!r92su->wq) {
		err = -ENOMEM;
		goto err_out;
	}

	return r92su;

err_out:
	r92su_unalloc(r92su);
	return ERR_PTR(err);
}

static void r92su_if_setup(struct net_device *ndev)
{
	ether_setup(ndev);
	ndev->priv_flags &= ~IFF_TX_SKB_SHARING;
	ndev->netdev_ops = &r92su_netdevice_ops;
	ndev->destructor = free_netdev;
	ndev->needed_headroom = R92SU_TX_HEAD_ROOM;
	ndev->needed_tailroom = R92SU_TX_TAIL_ROOM;
	ndev->flags |= IFF_BROADCAST | IFF_MULTICAST;
	ndev->watchdog_timeo = 5 * HZ; /* USB_CTRL_TIMEOUT */
}

static int r92su_alloc_netdev(struct r92su *r92su)
{
	struct net_device *ndev;
	/* The firmware/hardware does not support multiple interfaces.
	 * So, we are fine with just a single netdevice.
	 */
	ndev = alloc_netdev_mqs(0, "wlan%d", r92su_if_setup, NUM_ACS, 1);
	if (!ndev)
		return -ENOMEM;

	ndev->ml_priv = r92su;
	r92su->wdev.netdev = ndev;
	ndev->ieee80211_ptr = &r92su->wdev;
	SET_NETDEV_DEV(ndev, wiphy_dev(r92su->wdev.wiphy));
	return 0;
}

static int r92su_read_adapter_info(struct r92su *r92su)
{
	int err = -EINVAL;

	err = r92su_hw_read_chip_version(r92su);
	if (err)
		return err;

	err = r92su_eeprom_read(r92su);
	if (err)
		return err;

	return 0;
}

int r92su_setup(struct r92su *r92su)
{
	struct wiphy *wiphy = r92su->wdev.wiphy;
	int err;

	r92su_rx_init(r92su);
	r92su_cmd_init(r92su);

	err = r92su_read_adapter_info(r92su);
	if (err)
		goto err_out;

	err = r92su_init_band(r92su);
	if (err)
		goto err_out;

	err = r92su_alloc_netdev(r92su);
	if (err)
		goto err_out;

	memcpy(wiphy->perm_addr, r92su->eeprom.mac_addr, ETH_ALEN);
	memcpy(r92su->wdev.netdev->dev_addr, r92su->wdev.wiphy->perm_addr,
	       ETH_ALEN);

err_out:
	return err;
}

#define ADD_REV(type)	[(R92SU_##type)] = (#type)
static const char *rev_to_string[__R92SU_MAX_REV] = {
	ADD_REV(FPGA),
	ADD_REV(A_CUT),
	ADD_REV(B_CUT),
	ADD_REV(C_CUT),
};

static const char *rf_to_string(const enum r92su_rf_type_t type)
{
#define ADD_RF(type)	case R92SU_##type: return #type
	switch (type) {
	ADD_RF(1T1R);
	ADD_RF(1T2R);
	ADD_RF(2T2R);
	default:
		return "UNKN";
	};
}
#undef ADD_REV

#define NAME_LEN 32

static int r92su_register_wps_button(struct r92su *r92su)
{
#ifdef CONFIG_R92SU_WPC
	struct input_dev *input;
	char *name, *phys;
	int err;

	name = devm_kzalloc(&r92su->wdev.wiphy->dev, NAME_LEN, GFP_KERNEL);
	phys = devm_kzalloc(&r92su->wdev.wiphy->dev, NAME_LEN, GFP_KERNEL);

	input = input_allocate_device();
	if (!input || !name || !phys)
		return -ENOMEM;

	snprintf(name, NAME_LEN, "%s WPS Button",
		 wiphy_name(r92su->wdev.wiphy));

	snprintf(phys, NAME_LEN, "ieee80211/%s/input0",
		 wiphy_name(r92su->wdev.wiphy));

	input->name = name;
	input->name = phys;
	input->id.bustype = BUS_USB; /* SDIO */
	input->dev.parent = &r92su->wdev.wiphy->dev;

	input_set_capability(input, EV_KEY, KEY_WPS_BUTTON);

	err = input_register_device(input);
	if (err) {
		input_free_device(input);
		return err;
	}
	r92su->wps_pbc = input;
#endif /* CONFIG_R92SU_WPC */

	return 0;
}

int r92su_register(struct r92su *r92su)
{
	int err;

	err = wiphy_register(r92su->wdev.wiphy);
	if (err)
		return err;

	err = register_netdev(r92su->wdev.netdev);
	if (err)
		return err;

	err = r92su_register_debugfs(r92su);
	if (err)
		return err;

	err = r92su_register_wps_button(r92su);
	if (err)
		return err;

	dev_info(wiphy_dev(r92su->wdev.wiphy),
		 "Realtek RTL81XX rev %s, rf:%s is registered as '%s'.\n",
		 rev_to_string[r92su->chip_rev],
		 rf_to_string(r92su->rf_type),
		 wiphy_name(r92su->wdev.wiphy));

	r92su_set_state(r92su, R92SU_STOP);
	return 0;
}

void r92su_unalloc(struct r92su *r92su)
{
	if (r92su) {
		mutex_lock(&r92su->lock);
		r92su_set_state(r92su, R92SU_UNLOAD);
		mutex_unlock(&r92su->lock);

		if (r92su->wps_pbc) {
			input_unregister_device(r92su->wps_pbc);
			r92su->wps_pbc = NULL;
		}

		r92su_unregister_debugfs(r92su);

		if (r92su->wdev.netdev)
			unregister_netdev(r92su->wdev.netdev);

		if (r92su->wdev.wiphy->registered)
			wiphy_unregister(r92su->wdev.wiphy);

		synchronize_rcu();

		destroy_workqueue(r92su->wq);
		mutex_destroy(&r92su->lock);
		r92su_release_firmware(r92su);
		r92su_rx_deinit(r92su);
		wiphy_free(r92su->wdev.wiphy);
	}
}
