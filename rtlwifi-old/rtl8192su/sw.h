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
 *****************************************************************************/
#ifndef __REALTEK_PCI92SE_SW_H__
#define __REALTEK_PCI92SE_SW_H__

#define EFUSE_MAX_SECTION	16

#define	CAM_NONE				0x0
#define	CAM_WEP40				0x01
#define	CAM_TKIP				0x02
#define	CAM_AES					0x04
#define	CAM_WEP104				0x05

#define	TOTAL_CAM_ENTRY				32
#define	HALF_CAM_ENTRY				16


int rtl92su_init_sw(struct ieee80211_hw *hw);
void rtl92su_deinit_sw(struct ieee80211_hw *hw);
void rtl92su_init_var_map(struct ieee80211_hw *hw);

#endif
