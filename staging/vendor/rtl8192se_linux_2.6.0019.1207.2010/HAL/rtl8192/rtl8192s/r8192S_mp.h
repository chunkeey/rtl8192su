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

#ifndef __INC_HAL8192S_MP_H
#define __INC_HAL8192S_MP_H

/*--------------------------Define Parameters-------------------------------*/

#define MPT_NOOP				0
#define MPT_READ_MAC_1BYTE			1
#define MPT_READ_MAC_2BYTE			2
#define MPT_READ_MAC_4BYTE			3
#define MPT_WRITE_MAC_1BYTE			4
#define MPT_WRITE_MAC_2BYTE			5
#define MPT_WRITE_MAC_4BYTE			6
#define MPT_READ_BB_CCK				7
#define MPT_WRITE_BB_CCK				8
#define MPT_READ_BB_OFDM				9
#define MPT_WRITE_BB_OFDM				10
#define MPT_READ_RF					11
#define MPT_WRITE_RF					12
#define MPT_READ_EEPROM_1BYTE			13
#define MPT_WRITE_EEPROM_1BYTE		14
#define MPT_READ_EEPROM_2BYTE			15
#define MPT_WRITE_EEPROM_2BYTE		16
#define MPT_SET_CSTHRESHOLD			21
#define MPT_SET_INITGAIN				22
#define MPT_SWITCH_BAND				23
#define MPT_SWITCH_CHANNEL			24
#define MPT_SET_DATARATE				25
#define MPT_SWITCH_ANTENNA			26
#define MPT_SET_TX_POWER				27
#define MPT_SET_CONT_TX				28
#define MPT_SET_SINGLE_CARRIER		29
#define MPT_SET_CARRIER_SUPPRESSION	30

#define MPT_SET_ANTENNA_GAIN_OFFSET	40
#define MPT_SET_CRYSTAL_CAP			41
#define MPT_TRIGGER_RF_THERMAL_METER	42		
#define MPT_SET_SINGLE_TONE			43		
#define MPT_READ_RF_THERMAL_METER	44		
#define MPT_SWITCH_BAND_WIDTH		45		
#define MPT_QUERY_TSSI_VALUE     		46          
#define MPT_SET_TX_POWER_ADJUST          47
#define MPT_DO_TX_POWER_TRACK		48
#define MPT_QUERY_NIC_TYPE				49
#define MPT_QUERY_WPS_PUSHED			50
#define MPT_SET_LED_CONTROL			51
#define MPT_TX_POWER_TRACK_CONTROL	52		

#define MPT_WRITE_EFUSE_1BYTE  		53
#define MPT_READ_EFUSE_1BYTE			54
#define MPT_READ_EFUSE_2BYTE			55
#define MPT_READ_EFUSE_4BYTE			56
#define MPT_UPDATE_EFUSE				57
#define MPT_UPDATE_EFUSE_UTILIZE		58
#define MPT_UPDATE_AUTOLOAD_STS		59

#define MAX_TX_PWR_INDEX_N_MODE 		64
/*--------------------------Define Parameters-------------------------------*/

/*------------------------------Define structure----------------------------*/ 
/* MP set force data rate base on the definition. */
typedef enum _MPT_RATE_INDEX{
	/* CCK rate. */
	MPT_RATE_1M = 1, 
	MPT_RATE_2M,
	MPT_RATE_55M,
	MPT_RATE_11M,

	/* OFDM rate. */
	MPT_RATE_6M,
	MPT_RATE_9M,
	MPT_RATE_12M,
	MPT_RATE_18M,
	MPT_RATE_24M,
	MPT_RATE_36M,
	MPT_RATE_48M,
	MPT_RATE_54M,
	
	/* HT rate. */
	MPT_RATE_MCS0,
	MPT_RATE_MCS1,
	MPT_RATE_MCS2,
	MPT_RATE_MCS3,
	MPT_RATE_MCS4,
	MPT_RATE_MCS5,
	MPT_RATE_MCS6,
	MPT_RATE_MCS7,
	MPT_RATE_MCS8,
	MPT_RATE_MCS9,
	MPT_RATE_MCS10,
	MPT_RATE_MCS11,
	MPT_RATE_MCS12,
	MPT_RATE_MCS13,
	MPT_RATE_MCS14,
	MPT_RATE_MCS15,
	MPT_RATE_LAST
	 
}MPT_RATE_E, *PMPT_RATE_E;

typedef enum _MPT_Bandwidth_Switch_Mode{
	BAND_20MHZ_MODE = 0,
	BAND_40MHZ_DUPLICATE_MODE = 1,
	BAND_40MHZ_LOWER_MODE = 2,
	BAND_40MHZ_UPPER_MODE = 3,
	BAND_40MHZ_DONTCARE_MODE = 4
}MPT_BANDWIDTH_MODE_E, *PMPT_BANDWIDTH_MODE_E;

typedef enum _TxAGC_Offset{
        TxAGC_Offset_0 	= 0x00,
        TxAGC_Offset_1,
        TxAGC_Offset_2,
        TxAGC_Offset_3,
        TxAGC_Offset_4,
        TxAGC_Offset_5,
        TxAGC_Offset_6,
        TxAGC_Offset_7,
        TxAGC_Offset_neg8,
        TxAGC_Offset_neg7,
        TxAGC_Offset_neg6,
        TxAGC_Offset_neg5,
        TxAGC_Offset_neg4,
        TxAGC_Offset_neg3,
        TxAGC_Offset_neg2,
        TxAGC_Offset_neg1
} TxAGC_Offset;
/*------------------------------Define structure----------------------------*/ 
void rtl8192_init_mp(struct net_device* dev);
int r8192_wx_mp_set_chan(struct net_device *dev, struct iw_request_info *info, 
		union iwreq_data *wrqu, char *extra);
int r8192_wx_mp_set_txrate(struct net_device *dev, struct iw_request_info *info, 
		union iwreq_data *wrqu, char *extra);
int r8192_wx_mp_set_txpower(struct net_device *dev, struct iw_request_info *info, 
		union iwreq_data *wrqu, char *extra);
int r8192_wx_mp_set_bw(struct net_device *dev,struct iw_request_info *info, 
		union iwreq_data *wrqu, char *extra);
int r8192_wx_mp_set_txstart(struct net_device *dev, struct iw_request_info *info, 
		union iwreq_data *wrqu, char *extra);
int r8192_wx_mp_set_singlecarrier(struct net_device *dev, struct iw_request_info *info, 
		union iwreq_data *wrqu, char *extra);
int r8192_wx_mp_write_rf(struct net_device *dev, struct iw_request_info *info, 
		union iwreq_data *wrqu, char *extra);
int r8192_wx_mp_write_mac(struct net_device *dev, struct iw_request_info *info, 
		union iwreq_data *wrqu, char *extra);

void mpt_StartCckContTx(struct net_device *dev,bool bScrambleOn);
void mpt_StartOfdmContTx(struct net_device *dev);
void mpt_StopCckCoNtTx(struct net_device *dev);
void mpt_StopOfdmContTx(struct net_device *dev);
void mpt_SwitchRfSetting92S(struct net_device *dev);
bool mpt_ProSetTxPower(	struct net_device *dev,	u32 ulTxPower);
bool mpt_ProSetTxAGCOffset(struct net_device *dev,	u32	ulTxAGCOffset);
bool mpt_ProSetRxFilter(struct net_device *dev, u32	RCRMode);
bool mpt_ProSetRxFilter(struct net_device *dev, u32	RCRMode);
bool mpt_ProSetModulation(struct net_device *dev, u32 ulWirelessMode);
void mpt_Pro819xIoCallback(struct net_device *dev);
void MPT_ProSetSingleCarrier(struct net_device *dev, bool ulMode);
void MPT_ProSetBandWidth819x(struct net_device *dev);
void MPT_ProSwChannel(struct net_device *dev);
void MPT_ProSetDataRate819x(struct net_device *dev);
void MPT_ProSetTxAGCOffset(struct net_device *dev);
#endif
