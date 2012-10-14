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
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/
#ifndef __RTL92S_PHY_H__
#define __RTL92S_PHY_H__

#define MAX_TXPWR_IDX_NMODE_92S		63
#define MAX_DOZE_WAITING_TIMES_9x	64

/* Channel switch:The size of
 * command tables for switch channel */
#define MAX_PRECMD_CNT			16
#define MAX_RFDEPENDCMD_CNT		16
#define MAX_POSTCMD_CNT			16

#define RF90_PATH_MAX			4


#define IOCMD_CLASS_BB_RF			0xF0
#define IOCMD_BB_READ_IDX			0x00
#define IOCMD_BB_WRITE_IDX			0x01
#define IOCMD_RF_READ_IDX			0x02
#define IOCMD_RF_WRITE_IDX			0x03

#define	MASKBYTE0				0xff
#define	MASKBYTE1				0xff00
#define	MASKBYTE2				0xff0000
#define	MASKBYTE3				0xff000000
#define	MASKHWORD				0xffff0000
#define	MASKLWORD				0x0000ffff
#define	MASKDWORD				0xffffffff

#define	MAKS12BITS				0xfffff
#define	MASK20BITS				0xfffff
#define RFREG_OFFSET_MASK			0xfffff

enum version_8192s {
	VERSION_8192S_ACUT,
	VERSION_8192S_BCUT,
	VERSION_8192S_CCUT
};

enum swchnlcmd_id {
	CMDID_END,
	CMDID_SET_TXPOWEROWER_LEVEL,
	CMDID_BBREGWRITE10,
	CMDID_WRITEPORT_ULONG,
	CMDID_WRITEPORT_USHORT,
	CMDID_WRITEPORT_UCHAR,
	CMDID_RF_WRITEREG,
};

struct swchnlcmd {
	enum swchnlcmd_id cmdid;
	u32 para1;
	u32 para2;
	u32 msdelay;
};

enum baseband_config_type {
	/* Radio Path A */
	BASEBAND_CONFIG_PHY_REG = 0,
	/* Radio Path B */
	BASEBAND_CONFIG_AGC_TAB = 1,
};

#define hal_get_firmwareversion(rtlpriv) \
	(((struct rt_firmware *)(rtlpriv->rtlhal.pfirmware))->firmwareversion)

u32 rtl92s_phy_query_bb_reg(struct ieee80211_hw *hw, u32 regaddr, u32 bitmask);
void rtl92s_phy_set_bb_reg(struct ieee80211_hw *hw, u32 regaddr, u32 bitmask,
			   u32 data);
void rtl92s_phy_scan_operation_backup(struct ieee80211_hw *hw, u8 operation);
u32 rtl92s_phy_query_rf_reg(struct ieee80211_hw *hw, enum radio_path rfpath,
			    u32 regaddr, u32 bitmask);
void rtl92s_phy_set_rf_reg(struct ieee80211_hw *hw,	enum radio_path rfpath,
			   u32 regaddr, u32 bitmask, u32 data);
void rtl92s_phy_set_bw_mode(struct ieee80211_hw *hw,
			    enum nl80211_channel_type ch_type);
u8 rtl92s_phy_sw_chnl(struct ieee80211_hw *hw);
bool rtl92s_phy_set_rf_power_state(struct ieee80211_hw *hw,
				   enum rf_pwrstate rfpower_state);
bool rtl92s_phy_mac_config(struct ieee80211_hw *hw);
void rtl92s_phy_switch_ephy_parameter(struct ieee80211_hw *hw);
bool rtl92s_phy_bb_config(struct ieee80211_hw *hw);
bool rtl92s_phy_rf_config(struct ieee80211_hw *hw);
void rtl92s_phy_get_hw_reg_originalvalue(struct ieee80211_hw *hw);
void rtl92s_phy_set_txpower(struct ieee80211_hw *hw, u8	channel);
bool rtl92s_phy_set_fw_cmd(struct ieee80211_hw *hw, enum fwcmd_iotype fwcmd_io);
void rtl92s_phy_chk_fwcmd_iodone(struct ieee80211_hw *hw);
void rtl92s_phy_set_beacon_hwreg(struct ieee80211_hw *hw, u16 beaconinterval);
u8 rtl92s_phy_config_rf(struct ieee80211_hw *hw, enum radio_path rfpath) ;

#endif
