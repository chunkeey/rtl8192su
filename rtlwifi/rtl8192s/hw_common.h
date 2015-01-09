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

enum WIRELESS_NETWORK_TYPE {
	WIRELESS_11B = 1,
	WIRELESS_11G = 2,
	WIRELESS_11A = 4,
	WIRELESS_11N = 8
};

void rtl92s_get_hw_reg(struct ieee80211_hw *hw,
		       u8 variable, u8 *val);
void rtl92s_set_hw_reg(struct ieee80211_hw *hw, u8 variable,
		       u8 *val);
void rtl92s_set_check_bssid(struct ieee80211_hw *hw, bool check_bssid);
void rtl92s_get_IC_Inferiority(struct ieee80211_hw *hw);
void rtl92s_read_eeprom_info(struct ieee80211_hw *hw);
void rtl92s_get_IC_Inferiority(struct ieee80211_hw *hw);
void rtl92s_card_disable(struct ieee80211_hw *hw);
int rtl92s_set_network_type(struct ieee80211_hw *hw,
			    enum nl80211_iftype type);
void rtl92s_set_check_bssid(struct ieee80211_hw *hw, bool check_bssid);
void rtl92s_set_qos(struct ieee80211_hw *hw, int aci);
void rtl92s_set_beacon_related_registers(struct ieee80211_hw *hw);
void rtl92s_set_beacon_interval(struct ieee80211_hw *hw);
void rtl92s_update_hal_rate_tbl(struct ieee80211_hw *hw,
				struct ieee80211_sta *sta, u8 rssi_level);
void rtl92s_update_channel_access_setting(struct ieee80211_hw *hw);
void rtl92s_gpiobit3_cfg_inputmode(struct ieee80211_hw *hw);
void rtl92s_enable_hw_security_config(struct ieee80211_hw *hw);
void rtl92s_set_key(struct ieee80211_hw *hw, u32 key_index, u8 *macaddr,
		    bool is_group, u8 enc_algo, bool is_wepkey,
		    bool clear_all);
int rtl92s_set_media_status(struct ieee80211_hw *hw, enum nl80211_iftype type);
void rtl92s_phy_set_rfhalt(struct ieee80211_hw *hw);
void rtl92s_read_chip_version(struct ieee80211_hw *hw);
bool rtl92s_halset_sysclk(struct ieee80211_hw *hw, u16 data);
bool rtl92s_rf_onoff_detect(struct ieee80211_hw *hw);
#endif
