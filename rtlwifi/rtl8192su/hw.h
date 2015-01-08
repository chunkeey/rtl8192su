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
#ifndef __REALTEK_92SU_HW_H__
#define __REALTEK_92SU_HW_H__

#define CHAN_NUM_2G     14
#define RF_PATH         2
#define CHAN_SET        3
/* 0 = Chan 1-3, 1 = Chan 4-8, 2 = Chan 9-14 */

struct r92su_eeprom {
	__le16 id;				/*  0 -  1 */
	__le16 hpon;				/*  2 -  3 */
	__le16 clk;				/*  4 -  5 */
	__le16 testr;				/*  6 -  7 */
	__le16 vid;				/*  8 -  9 */
	__le16 did;				/* 10 - 11 */
	u8 usb_optional;			/* 12 */
	u8 usb_phy_para1[5];			/* 13 - 17 */
	u8 mac_addr[6];				/* 18 - 23 */

	/* seems to contain vendor and device identification strings */
	u8 unkn2[56];				/* 24 - 79 */

	/* WARNING
	 * These definitions are mostly guesswork
	 */
	u8 version;				/* 80 */
	u8 channel_plan;			/* 81 */
	u8 custom_id;				/* 82 */
	u8 sub_custom_id;			/* 83 */
	u8 board_type;				/* 84 */

	/* tx power base */
	u8 tx_pwr_cck[RF_PATH][CHAN_SET];	/* 85 - 90 */
	u8 tx_pwr_ht40_1t[RF_PATH][CHAN_SET];	/* 91 - 96 */
	u8 tx_pwr_ht40_2t[RF_PATH][CHAN_SET];	/* 97 - 102 */

	u8 pw_diff;				/* 103 */
	u8 thermal_meter;			/* 104 */
	u8 crystal_cap;				/* 105 */
	u8 unkn3;				/* 106 */
	u8 tssi[RF_PATH];			/* 107 - 108 */
	u8 unkn4;				/* 109 */
	u8 tx_pwr_ht20_diff[3];			/* 110 - 112 */
	u8 tx_pwr_ofdm_diff[2];			/* 113 - 114, 124 */
	u8 tx_pwr_edge[RF_PATH][CHAN_SET];	/* 115 - 120 */
	u8 tx_pwr_edge_chk;			/* 121 */
	u8 regulatory;				/* 122 ??? */
	u8 rf_ind_power_diff;			/* 123 */
	u8 tx_pwr_ofdm_diff_cont;		/* 124 */
	u8 unkn6[3];				/* 125 - 127 */
} __packed;


int rtl92su_hw_init(struct ieee80211_hw *hw);
void rtl92su_card_disable(struct ieee80211_hw *hw);
int rtl92su_set_network_type(struct ieee80211_hw *hw,
			     enum nl80211_iftype type);
void rtl92su_set_check_bssid(struct ieee80211_hw *hw, bool check_bssid);
void rtl92su_allow_all_destaddr(struct ieee80211_hw *hw,
				bool allow_all_da, bool write_into_reg);
void rtl92su_read_eeprom_info(struct ieee80211_hw *hw);

/* stubs */
void rtl92su_enable_interrupt(struct ieee80211_hw *hw);
void rtl92su_disable_interrupt(struct ieee80211_hw *hw);
void rtl92su_update_interrupt_mask(struct ieee80211_hw *hw,
				   u32 add_msr, u32 rm_msr);
bool rtl92su_phy_set_rf_power_state(struct ieee80211_hw *hw,
				    enum rf_pwrstate rfpwr_state);
bool rtl92su_gpio_radio_on_off_checking(struct ieee80211_hw *hw, u8 *valid);
#endif
