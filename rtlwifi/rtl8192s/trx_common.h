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
#ifndef __REALTEK_92S_HW_COMMON_H__
#define __REALTEK_92S_HW_COMMON_H__

void rtl92s_translate_rx_signal_stuff(struct ieee80211_hw *hw,
				      struct sk_buff *skb,
				      struct rtl_stats *pstats,
				      u8 *pdesc,
				      struct rx_fwinfo *p_drvinfo);
u8 rtl92s_map_hwqueue_to_fwqueue(struct sk_buff *skb, u8 skb_queue);

#endif
