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
#ifndef __REALTEK_92SU_HW_H__
#define __REALTEK_92SU_HW_H__

int rtl92su_hw_init(struct ieee80211_hw *hw);
void rtl92su_card_disable(struct ieee80211_hw *hw);
int rtl92su_set_network_type(struct ieee80211_hw *hw,
			     enum nl80211_iftype type);
void rtl92su_set_check_bssid(struct ieee80211_hw *hw, bool check_bssid);
void rtl92su_allow_all_destaddr(struct ieee80211_hw *hw,
				bool allow_all_da, bool write_into_reg);

/* stubs */
void rtl92su_enable_interrupt(struct ieee80211_hw *hw);
void rtl92su_disable_interrupt(struct ieee80211_hw *hw);
void rtl92su_update_interrupt_mask(struct ieee80211_hw *hw,
                u32 add_msr, u32 rm_msr);
bool rtl92su_gpio_radio_on_off_checking(struct ieee80211_hw *hw, u8 *valid);
#endif
