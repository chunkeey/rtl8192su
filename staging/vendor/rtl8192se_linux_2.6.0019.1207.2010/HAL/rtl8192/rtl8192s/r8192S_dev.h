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
#ifndef _RTL8192SE_H
#define _RTL8192SE_H

#include "r8192S_def.h"

u8 rtl8192se_QueryIsShort(u8 TxHT, u8 TxRate, cb_desc *tcb_desc);
bool rtl8192se_GetHalfNmodeSupportByAPs(struct net_device* dev);
bool rtl8192se_GetNmodeSupportBySecCfg(struct net_device *dev);
bool rtl8192se_HalTxCheckStuck(struct net_device *dev);
bool rtl8192se_HalRxCheckStuck(struct net_device *dev);
void rtl8192se_interrupt_recognized(struct net_device *dev, u32 *p_inta, u32 *p_intb);
void rtl8192se_enable_rx(struct net_device *dev);
void rtl8192se_enable_tx(struct net_device *dev);
void rtl8192se_EnableInterrupt(struct net_device *dev);
void rtl8192se_DisableInterrupt(struct net_device *dev);
void rtl8192se_ClearInterrupt(struct net_device *dev);
void rtl8192se_InitializeVariables(struct net_device  *dev);
void rtl8192se_start_beacon(struct net_device *dev);
u8 MRateToHwRate8192SE(struct net_device*dev, u8 rate);
void rtl8192se_get_eeprom_size(struct net_device* dev);
void MacConfigBeforeFwDownload(struct net_device *dev);
bool rtl8192se_adapter_start(struct net_device* dev);
void rtl8192se_link_change(struct net_device *dev);
void rtl8192se_AllowAllDestAddr(struct net_device* dev, bool bAllowAllDA, bool WriteIntoReg);
void rtl8192se_tx_fill_desc(struct net_device *dev, tx_desc *pDesc, cb_desc *cb_desc, struct sk_buff *skb);
void rtl8192se_tx_fill_cmd_desc(struct net_device *dev, tx_desc_cmd *entry, cb_desc *cb_desc, 
		struct sk_buff *skb);
bool rtl8192se_rx_query_status_desc(struct net_device* dev, struct rtllib_rx_stats*  stats, 
		rx_desc *pdesc, struct sk_buff* skb);
void rtl8192se_halt_adapter(struct net_device *dev, bool bReset);
void rtl8192se_update_ratr_table(struct net_device* dev,u8* pMcsRate,struct sta_info* pEntry);
int r8192se_resume_firm(struct net_device *dev);
void PHY_SetRtl8192seRfHalt(struct net_device* dev);
void UpdateHalRAMask8192SE(struct net_device* dev, bool bMulticast, u8 macId, u8 MimoPs, u8 WirelessMode, u8 bCurTxBW40MHz, u8 rssi_level,
		u8* entry_McsRateSet, u8* entry_ratrindex, u32* entry_ratr_bitmap, u32* entry_mask, bool is_ap);
u8 HalSetSysClk8192SE(struct net_device *dev, u8 Data);
bool	rtl8192se_RxCommandPacketHandle(struct net_device *dev, struct sk_buff* skb,rx_desc *pdesc);
bool rtl8192se_check_ht_cap(struct net_device* dev, struct sta_info *sta, 
		struct rtllib_network* net);
u8 rtl8192se_MapHwQueueToFirmwareQueue(u8 QueueID, u8 priority);

void GetHwReg8192SE(struct net_device *dev,u8 variable,u8* val);
void SetHwReg8192SE(struct net_device *dev,u8 variable,u8* val);
void Adhoc_InitRateAdaptive(struct net_device *dev,struct sta_info  *pEntry);
void SetBeaconRelatedRegisters8192SE(struct net_device *dev);

#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,20)
void rtl8192se_check_tsf_wq(struct work_struct * work);
void rtl8192se_update_assoc_sta_info_wq(struct work_struct * work);
#else
void rtl8192se_check_tsf_wq(struct net_device *dev);
void rtl8192se_update_assoc_sta_info_wq(struct net_device *dev);
#endif
void gen_RefreshLedState(struct net_device *dev);
#ifdef ASL
extern void HwConfigureRTL8192SE(struct net_device *dev);
extern void rtl8192se_ap_link_change(struct net_device *dev);
#endif

#endif
