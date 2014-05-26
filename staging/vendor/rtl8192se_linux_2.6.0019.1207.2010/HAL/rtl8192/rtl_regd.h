/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
 *
 * Based on the ath driver, which is:
 * Copyright (c) 2008-2009 Atheros Communications Inc.
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
 ******************************************************************************/
#ifndef RTL_REGD_H
#define RTL_REGD_H

#if defined CONFIG_CFG_80211 && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)  

#include <linux/nl80211.h>
#include <net/cfg80211.h>
#include "rtl_core.h"

struct r8192_priv;

struct country_code_to_enum_rd {
	u16 countryCode;
	const char *isoName;
};


int rtl_regd_init(struct net_device *dev,
		  int (*reg_notifier)(struct wiphy *wiphy,
		  struct regulatory_request *request));
int rtl_reg_notifier(struct wiphy *wiphy,
		     struct regulatory_request *request);
void rtl_dump_channel_map(struct wiphy *wiphy);
int rtllib_set_geo(struct r8192_priv *priv);
bool rtl8192_register_wiphy_dev(struct net_device *dev);

#endif
#endif
