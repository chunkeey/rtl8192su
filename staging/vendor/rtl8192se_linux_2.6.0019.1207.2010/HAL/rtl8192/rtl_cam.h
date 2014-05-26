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
#ifndef _RTL_CAM_H
#define _RTL_CAM_H

#include <linux/types.h>
struct net_device;

void CamResetAllEntry(struct net_device* dev);
void EnableHWSecurityConfig8192(struct net_device *dev);
void setKey(struct net_device *dev, u8 EntryNo, u8 KeyIndex, u16 KeyType, u8 *MacAddr, u8 DefaultKey, u32 *KeyContent );
void set_swcam(struct net_device *dev, u8 EntryNo, u8 KeyIndex, u16 KeyType, u8 *MacAddr, u8 DefaultKey, u32 *KeyContent, u8 is_mesh, u8 is_ap);
void CamPrintDbgReg(struct net_device* dev);

#ifdef _RTL8192_EXT_PATCH_
void CamDeleteOneEntry(struct net_device *dev, u8 EntryNo);
#endif

u32 read_cam(struct net_device *dev, u8 addr);
void write_cam(struct net_device *dev, u8 addr, u32 data);

void CamRestoreAllEntry(struct net_device *dev);
void reset_IFswcam(struct net_device *dev, u8 is_mesh, u8 is_ap);
#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
void CamRestoreEachIFEntry(struct net_device* dev,u8 is_mesh,u8 is_ap);
int rtl8192_set_key_for_AP(struct rtllib_device *ieee);
#endif

void CAM_read_entry(struct net_device *dev, u32 iIndex);

#if defined(_RTL8192_EXT_PATCH_) || defined(ASL) 
u8 rtl8192_get_free_hwsec_cam_entry(struct rtllib_device *ieee, u8 *sta_addr);
void rtl8192_del_hwsec_cam_entry(struct rtllib_device *ieee, u8 *sta_addr, bool is_mesh, bool is_ap);
#endif
#ifdef _RTL8192_EXT_PATCH_
int meshdev_set_key_for_peer(struct net_device *dev, u8 *Addr, u8 KeyIndex, u16 KeyType, u32 *KeyContent);
#endif

#endif
