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
#ifndef __R92SU_RX_H__
#define __R92SU_RX_H__

#include <linux/mm.h>
#include <linux/skbuff.h>

#include "r92su.h"

void r92su_rx_init(struct r92su *r92su);
void r92su_rx_deinit(struct r92su *r92su);

u8 *r92su_find_ie(u8 *ies, const u32 len, const u8 ie);


//void r92su_reorder_tid_timer(unsigned long arg);
void r92su_reorder_tid_timer(struct timer_list *ptr);
void r92su_rx(struct r92su *r92su, void *skb, const unsigned int len);

#endif /* __R92SU_RX_H__ */


