/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
 *
 * Based on the ath driver, which is:
 * Copyright (c) 2008-2009 Atheros Communications Inc.
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
 ******************************************************************************/
#include "rtl_core.h"
#include <linux/kernel.h>
#include <linux/slab.h>

#if defined CONFIG_CFG_80211 && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)  
#include <net/cfg80211.h>

#ifdef CONFIG_CRDA
static struct country_code_to_enum_rd allCountries[] = {
	{COUNTRY_CODE_FCC, "US"},
	{COUNTRY_CODE_IC, "US"},
	{COUNTRY_CODE_ETSI, "EC"},
	{COUNTRY_CODE_SPAIN, "EC"},
	{COUNTRY_CODE_FRANCE, "EC"},
	{COUNTRY_CODE_MKK, "JP"},
	{COUNTRY_CODE_MKK1, "JP"},
	{COUNTRY_CODE_ISRAEL, "EC"},
	{COUNTRY_CODE_TELEC, "JP"},
	{COUNTRY_CODE_MIC, "JP"},
	{COUNTRY_CODE_GLOBAL_DOMAIN, "JP"},
	{COUNTRY_CODE_WORLD_WIDE_13, "EC"},
	{COUNTRY_CODE_TELEC_NETGEAR, "EC"},
};

/* Only these channels all allow active scan on all world regulatory domains */
#define RTL819x_2GHZ_CH01_11	REG_RULE(2412-10, 2462+10, 40, 0, 20, 0)

/* We enable active scan on these a case by case basis by regulatory domain */
#define RTL819x_2GHZ_CH12_13	REG_RULE(2467-10, 2472+10, 40, 0, 20, NL80211_RRF_PASSIVE_SCAN)
#define RTL819x_2GHZ_CH14	REG_RULE(2484-10, 2484+10, 40, 0, 20, NL80211_RRF_PASSIVE_SCAN | \
								      NL80211_RRF_NO_OFDM)

static const struct ieee80211_regdomain rtl_regdom_11 = {
	.n_reg_rules = 1,
	.alpha2 =  "99",
	.reg_rules = {
		RTL819x_2GHZ_CH01_11,
	}
};

static const struct ieee80211_regdomain rtl_regdom_global = {
	.n_reg_rules = 3,
	.alpha2 =  "99",
	.reg_rules = {
		RTL819x_2GHZ_CH01_11,
		RTL819x_2GHZ_CH12_13, 
		RTL819x_2GHZ_CH14,
	}
};

static const struct ieee80211_regdomain rtl_regdom_world = {
	.n_reg_rules = 2,
	.alpha2 =  "99",
	.reg_rules = {
		RTL819x_2GHZ_CH01_11,
		RTL819x_2GHZ_CH12_13,
	}
};

static void rtl_reg_apply_chan_plan(struct wiphy *wiphy)
{
	struct net_device *dev = wiphy_to_net_device(wiphy);
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	struct rtllib_device *rtllib = priv->rtllib;
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *ch;
	int i;

	sband = wiphy->bands[IEEE80211_BAND_2GHZ];

	for (i = 0; i < sband->n_channels; i++) {
		ch = &sband->channels[i];
		if (ch->flags & IEEE80211_CHAN_DISABLED) {
			GET_DOT11D_INFO(rtllib)->channel_map[ch->hw_value] = 0;	
		} else {
			if (ch->flags & IEEE80211_CHAN_PASSIVE_SCAN) {
				GET_DOT11D_INFO(rtllib)->channel_map[ch->hw_value] = 2;	
			} else {
				GET_DOT11D_INFO(rtllib)->channel_map[ch->hw_value] = 1;	
			}
		}

	}

	for (i = sband->n_channels - 1; i >= 0; i--) {
		ch = &sband->channels[i];
		if (!(ch->flags & IEEE80211_CHAN_NO_IBSS)) {
			rtllib->ibss_maxjoin_chal = ch->hw_value;
			break;
		}
	}

	rtllib->IbssStartChnl = 10;

	return;
}

void rtl_dump_channel_map(struct wiphy *wiphy)
{
	enum ieee80211_band band;
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *ch;
	unsigned int i;

	for (band = 0; band < IEEE80211_NUM_BANDS; band++) {

		if (!wiphy->bands[band])
			continue;

		sband = wiphy->bands[band];

		for (i = 0; i < sband->n_channels; i++) {
			ch = &sband->channels[i];
			printk("chan:%d, NO_IBSS:%d," 
					" PASSIVE_SCAN:%d, RADAR:%d, DISABLED:%d\n", i+1,
					(ch->flags&IEEE80211_CHAN_NO_IBSS) ? 1:0, 
					(ch->flags&IEEE80211_CHAN_PASSIVE_SCAN) ? 1:0,
					(ch->flags&IEEE80211_CHAN_RADAR) ? 1:0,
					(ch->flags&IEEE80211_CHAN_DISABLED) ? 1:0
			      );
		}

	}
}

static void rtl_reg_apply_world_flags(struct wiphy *wiphy,
				      enum nl80211_reg_initiator initiator,
				      struct rtl_regulatory *reg)
{
	rtl_reg_apply_chan_plan(wiphy);
	return;
}

int rtl_reg_notifier_apply(struct wiphy *wiphy,
			   struct regulatory_request *request,
			   struct rtl_regulatory *reg)
{
	switch (request->initiator) {
	case NL80211_REGDOM_SET_BY_CORE:
		break;
	case NL80211_REGDOM_SET_BY_DRIVER:
	case NL80211_REGDOM_SET_BY_USER:
		rtl_reg_apply_world_flags(wiphy, request->initiator, reg);
		break;
	case NL80211_REGDOM_SET_BY_COUNTRY_IE:
		rtl_reg_apply_world_flags(wiphy, request->initiator, reg);
		break;
	}

	return 0;
}

static const struct
ieee80211_regdomain *rtl_regdomain_select(struct rtl_regulatory *reg)
{
	switch (reg->country_code) {
	case COUNTRY_CODE_FCC:
	case COUNTRY_CODE_IC:
		return &rtl_regdom_11;
	case COUNTRY_CODE_ETSI:
	case COUNTRY_CODE_SPAIN:
	case COUNTRY_CODE_FRANCE:
	case COUNTRY_CODE_ISRAEL:
	case COUNTRY_CODE_TELEC_NETGEAR:
		return &rtl_regdom_world;
	case COUNTRY_CODE_MKK:
	case COUNTRY_CODE_MKK1:
	case COUNTRY_CODE_TELEC:
	case COUNTRY_CODE_MIC:
		return &rtl_regdom_global;
	case COUNTRY_CODE_GLOBAL_DOMAIN:
		return &rtl_regdom_global;
	case COUNTRY_CODE_WORLD_WIDE_13:
		return &rtl_regdom_world;
	default:
		WARN_ON(1);
		return &rtl_regdom_world;
	}
}

static int
rtl_regd_init_wiphy(struct rtl_regulatory *reg,
		    struct wiphy *wiphy,
		    int (*reg_notifier)(struct wiphy *wiphy,
					struct regulatory_request *request))
{
	const struct ieee80211_regdomain *regd;

	wiphy->reg_notifier = reg_notifier;

#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,33)
	wiphy->flags |= WIPHY_FLAG_CUSTOM_REGULATORY;
	wiphy->flags &= ~WIPHY_FLAG_STRICT_REGULATORY;
	wiphy->flags &= WIPHY_FLAG_DISABLE_BEACON_HINTS;
#else
	wiphy->custom_regulatory = true;
	wiphy->strict_regulatory = false;
	wiphy->disable_beacon_hints = true;
#endif

	regd = rtl_regdomain_select(reg);

	wiphy_apply_custom_regulatory(wiphy, regd);

	rtl_reg_apply_world_flags(wiphy, NL80211_REGDOM_SET_BY_DRIVER, reg);
	return 0;
}

static struct 
country_code_to_enum_rd *rtl_regd_find_country(u16 countryCode)
{       
	int i;

	for (i = 0; i < ARRAY_SIZE(allCountries); i++) {
		if (allCountries[i].countryCode == countryCode)
			return &allCountries[i];
	}
	return NULL;
} 

int rtl_regd_init(struct net_device *dev,
	      int (*reg_notifier)(struct wiphy *wiphy,
				  struct regulatory_request *request))
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);	
	struct rtl_regulatory *reg = &priv->rtllib->regulatory;
	struct wiphy *wiphy = priv->rtllib->wdev.wiphy;
	struct country_code_to_enum_rd *country = NULL;

	if (wiphy == NULL || reg == NULL){
		return -EINVAL;
	}

	printk(KERN_DEBUG "rtl: EEPROM regdomain: 0x%0x\n", priv->ChannelPlan);

	reg->country_code = priv->ChannelPlan;

	if (reg->country_code >= COUNTRY_CODE_MAX) {
		printk(KERN_DEBUG "rtl: EEPROM indicates invalid contry code"
		       "world wide 13 should be used\n");
		reg->country_code = COUNTRY_CODE_WORLD_WIDE_13;
	}

	country = rtl_regd_find_country(reg->country_code);

	if (country) {
		reg->alpha2[0] = country->isoName[0];
		reg->alpha2[1] = country->isoName[1];
	} else {
		reg->alpha2[0] = '0';
		reg->alpha2[1] = '0';
	}

	printk(KERN_DEBUG "rtl: Country alpha2 being used: %c%c\n",
		reg->alpha2[0], reg->alpha2[1]);
	rtl_regd_init_wiphy(reg, wiphy, reg_notifier);
	return 0;
}

int rtl_reg_notifier(struct wiphy *wiphy,
			      struct regulatory_request *request)
{
	struct net_device *dev = wiphy_to_net_device(wiphy);
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);	
	struct rtl_regulatory *reg = &priv->rtllib->regulatory;

	printk("rtl_regd: %s\n", __func__);
	return rtl_reg_notifier_apply(wiphy, request, reg);
}
#endif

struct net_device *wiphy_to_net_device(struct wiphy *wiphy)
{
	struct rtllib_device *rtllib;

	rtllib = wiphy_priv(wiphy);
	return rtllib->dev;
}

static const struct ieee80211_rate rtl819x_rates[] = {
	{ .bitrate = 10, .hw_value = 0, },
	{ .bitrate = 20, .hw_value = 1, },
	{ .bitrate = 55, .hw_value = 2, },
	{ .bitrate = 110, .hw_value = 3, },
	{ .bitrate = 60, .hw_value = 4, },
	{ .bitrate = 90, .hw_value = 5, },
	{ .bitrate = 120, .hw_value = 6, },
	{ .bitrate = 180, .hw_value = 7, },
	{ .bitrate = 240, .hw_value = 8, },
	{ .bitrate = 360, .hw_value = 9, },
	{ .bitrate = 480, .hw_value = 10, },
	{ .bitrate = 540, .hw_value = 11, },
};

#define CHAN2G(_freq, _flags, _idx)  { \
	        .band = IEEE80211_BAND_2GHZ, \
	        .center_freq = (_freq), \
		.flags = (_flags), \
	        .hw_value = (_idx), \
	        .max_power = 20, \
}

static struct ieee80211_channel rtl819x_2ghz_chantable[] = {
	CHAN2G(2412, 0, 1), /* Channel 1 */
	CHAN2G(2417, 0, 2), /* Channel 2 */
	CHAN2G(2422, 0, 3), /* Channel 3 */
	CHAN2G(2427, 0, 4), /* Channel 4 */
	CHAN2G(2432, 0, 5), /* Channel 5 */
	CHAN2G(2437, 0, 6), /* Channel 6 */
	CHAN2G(2442, 0, 7), /* Channel 7 */
	CHAN2G(2447, 0, 8), /* Channel 8 */
	CHAN2G(2452, 0, 9), /* Channel 9 */
	CHAN2G(2457, 0, 10), /* Channel 10 */
	CHAN2G(2462, 0, 11), /* Channel 11 */
	CHAN2G(2467, IEEE80211_CHAN_NO_IBSS|IEEE80211_CHAN_PASSIVE_SCAN, 12), /* Channel 12 */
	CHAN2G(2472, IEEE80211_CHAN_NO_IBSS|IEEE80211_CHAN_PASSIVE_SCAN, 13), /* Channel 13 */
	CHAN2G(2484, IEEE80211_CHAN_NO_IBSS|IEEE80211_CHAN_PASSIVE_SCAN, 14), /* Channel 14 */
};

int rtllib_set_geo(struct r8192_priv *priv)
{	
	priv->bands[IEEE80211_BAND_2GHZ].band = IEEE80211_BAND_2GHZ;
	priv->bands[IEEE80211_BAND_2GHZ].channels = rtl819x_2ghz_chantable;
	priv->bands[IEEE80211_BAND_2GHZ].n_channels = ARRAY_SIZE(rtl819x_2ghz_chantable);
	
	memcpy(&priv->rates[IEEE80211_BAND_2GHZ], rtl819x_rates, sizeof(rtl819x_rates));

	priv->bands[IEEE80211_BAND_2GHZ].n_bitrates = ARRAY_SIZE(rtl819x_rates);
	priv->bands[IEEE80211_BAND_2GHZ].bitrates = priv->rates[IEEE80211_BAND_2GHZ];

	return 0;
}

bool rtl8192_register_wiphy_dev(struct net_device *dev)
{	
	struct r8192_priv *priv = rtllib_priv(dev);
	struct wireless_dev *wdev = &priv->rtllib->wdev;
#ifdef CONFIG_CRDA
	struct rtl_regulatory *reg;
#endif
	memcpy(wdev->wiphy->perm_addr, dev->dev_addr, ETH_ALEN);
	wdev->wiphy->bands[IEEE80211_BAND_2GHZ] = &(priv->bands[IEEE80211_BAND_2GHZ]);
	set_wiphy_dev(wdev->wiphy, &priv->pdev->dev);

#ifdef CONFIG_CRDA
	if (rtl_regd_init(dev, rtl_reg_notifier)) {
		return false;
	}
#endif

	if (wiphy_register(wdev->wiphy)) {
		return false;
	}

#ifdef CONFIG_CRDA
	reg = &priv->rtllib->regulatory;
	if (reg != NULL) {
		if (regulatory_hint(wdev->wiphy, reg->alpha2)) {
			printk("########>%s() regulatory_hint fail\n", __func__);
			;
		} else {
			printk("########>#%s() regulatory_hint success\n", __func__);
		}
	} else {
		printk("#########%s() regulator null\n", __func__);
	}
#endif
	return true;
}

#endif
