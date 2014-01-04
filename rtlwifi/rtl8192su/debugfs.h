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
#ifndef __R92SU_DEBUGFS_H__
#define __R92SU_DEBUGFS_H__

#if 1
//#ifdef CONFIG_RTL8192SU_DEBUGFS

int rtl8192su_register_debugfs(struct ieee80211_hw *hw);
void rtl8192su_unregister_debugfs(struct ieee80211_hw *hw);

#else /* CONFIG_RTL8192SU_DEBUGFS */
static inline int rtl8192su_register_debugfs(struct ieee80211_hw *hw)
{
	return 0;
}

static inline void rtl8192su_unregister_debugfs(struct ieee80211_hw *hw)
{
}
#endif /* CONFIG_RTL8192SU_DEBUGFS */

#endif /* __R92SU_DEBUGFS_H__ */
