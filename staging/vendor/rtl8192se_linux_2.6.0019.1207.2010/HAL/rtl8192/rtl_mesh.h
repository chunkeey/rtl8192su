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
#ifdef _RTL8192_EXT_PATCH_
#ifndef _RTL_MESH_H
#define _RTL_MESH_H

struct net_device;
struct net_device_stats;

int meshdev_up(struct net_device *meshdev,bool is_silent_reset);
int meshdev_down(struct net_device *meshdev);
struct net_device_stats *meshdev_stats(struct net_device *meshdev);
void meshdev_init(struct net_device* meshdev);

int meshdev_update_ext_chnl_offset_as_client(void *data);
int meshdev_start_mesh_protocol_wq(void *data);
#endif
#endif
