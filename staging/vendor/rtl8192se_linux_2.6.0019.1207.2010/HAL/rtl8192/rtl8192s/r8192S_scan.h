/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
 *
 * Based on the r8180 driver, which is:
 * Copyright 2004-2005 Andrea Merello <andreamrl@tiscali.it>, et al.
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
#ifndef _RTL8192SE_SCAN
#define _RTL8192SE_SCAN

void rtl8192se_hw_scan_simu(void *data);
void rtl8192se_rx_surveydone_cmd(struct net_device *dev);
void	rtl8192se_check_hw_scan(void *data);
void rtl8192se_start_hw_scan(void *data);
void rtl8192se_abort_hw_scan(struct net_device *dev);
void rtl8192se_hw_scan_initiate(struct net_device *dev);
void rtl8192se_send_scan_abort(struct net_device *dev);
void rtl8192se_cancel_hw_scan(struct net_device *dev);

#endif

