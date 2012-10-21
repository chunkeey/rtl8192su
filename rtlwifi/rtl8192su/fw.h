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
#ifndef __REALTEK_FIRMWARE92S_H__
#define __REALTEK_FIRMWARE92S_H__

#define RTL8192_MAX_FIRMWARE_CODE_SIZE		(64 * 1024)
#define RTL8192_MAX_RAW_FIRMWARE_CODE_SIZE	200000
#define RTL8192_CPU_START_OFFSET		0x80
/* Firmware Local buffer size. 64k */
#define	MAX_FIRMWARE_CODE_SIZE			0xFF00

#define	RT_8192S_FIRMWARE_HDR_SIZE		80
#define RT_8192S_FIRMWARE_HDR_EXCLUDE_PRI_SIZE	32

/* support till 64 bit bus width OS */
#define MAX_DEV_ADDR_SIZE			8
#define MAX_FIRMWARE_INFORMATION_SIZE		32
#define MAX_802_11_HEADER_LENGTH		(40 + \
						MAX_FIRMWARE_INFORMATION_SIZE)
#define ENCRYPTION_MAX_OVERHEAD			128
#define MAX_FRAGMENT_COUNT			8
#define MAX_TRANSMIT_BUFFER_SIZE		(1600 + \
						(MAX_802_11_HEADER_LENGTH + \
						ENCRYPTION_MAX_OVERHEAD) *\
						MAX_FRAGMENT_COUNT)

#define H2C_TX_CMD_HDR_LEN			8

/* The following DM control code are for Reg0x364, */
#define	FW_DIG_ENABLE_CTL			BIT(0)
#define	FW_HIGH_PWR_ENABLE_CTL			BIT(1)
#define	FW_SS_CTL				BIT(2)
#define	FW_RA_INIT_CTL				BIT(3)
#define	FW_RA_BG_CTL				BIT(4)
#define	FW_RA_N_CTL				BIT(5)
#define	FW_PWR_TRK_CTL				BIT(6)
#define	FW_IQK_CTL				BIT(7)
#define	FW_FA_CTL				BIT(8)
#define	FW_DRIVER_CTRL_DM_CTL			BIT(9)
#define	FW_PAPE_CTL_BY_SW_HW			BIT(10)
#define	FW_DISABLE_ALL_DM			0
#define	FW_PWR_TRK_PARAM_CLR			0x0000ffff
#define	FW_RA_PARAM_CLR				0xffff0000

enum desc_packet_type {
	DESC_PACKET_TYPE_INIT = 0,
	DESC_PACKET_TYPE_NORMAL = 1,
};

/* 8-bytes alignment required */
struct fw_priv {
	/* --- long word 0 ---- */
	/* 0x12: CE product, 0x92: IT product */
	u8 signature_0;
	/* 0x87: CE product, 0x81: IT product */
	u8 signature_1;
	/* 0x81: PCI-AP, 01:PCIe, 02: 92S-U,
	 * 0x82: USB-AP, 0x12: 72S-U, 03:SDIO */
	u8 hci_sel;
	/* the same value as reigster value  */
	u8 chip_version;
	/* customer  ID low byte */
	u8 customer_id_0;
	/* customer  ID high byte */
	u8 customer_id_1;
	/* 0x11:  1T1R, 0x12: 1T2R,
	 * 0x92: 1T2R turbo, 0x22: 2T2R */
	u8 rf_config;
	/* 4: 4EP, 6: 6EP, 11: 11EP */
	u8 usb_ep_num;

	/* --- long word 1 ---- */
	/* regulatory class bit map 0 */
	u8 regulatory_class_0;
	/* regulatory class bit map 1 */
	u8 regulatory_class_1;
	/* regulatory class bit map 2 */
	u8 regulatory_class_2;
	/* regulatory class bit map 3 */
	u8 regulatory_class_3;
	/* 0:SWSI, 1:HWSI, 2:HWPI */
	u8 rfintfs;
	u8 def_nettype;
	u8 turbo_mode;
	u8 lowpower_mode;

	/* --- long word 2 ---- */
	/* 0x00: normal, 0x03: MACLBK, 0x01: PHYLBK */
	u8 lbk_mode;
	/* 1: for MP use, 0: for normal
	 * driver (to be discussed) */
	u8 mp_mode;
	/* 0: off, 1: on, 2: auto */
	u8 vcs_type;
	/* 0: none, 1: RTS/CTS, 2: CTS to self */
	u8 vcs_mode;
	u8 rsvd022;
	u8 rsvd023;
	u8 rsvd024;
	u8 rsvd025;

	/* --- long word 3 ---- */
	/* QoS enable */
	u8 qos_en;
	/* 40MHz BW enable */
	/* 4181 convert AMSDU to AMPDU, 0: disable */
	u8 bw_40mhz_en;
	u8 amsdu2ampdu_en;
	/* 11n AMPDU enable */
	u8 ampdu_en;
	/* FW offloads, 0: driver handles */
	u8 rate_control_offload;
	/* FW offloads, 0: driver handles */
	u8 aggregation_offload;
	u8 rsvd030;
	u8 rsvd031;

	/* --- long word 4 ---- */
	/* 1. FW offloads, 0: driver handles */
	u8 beacon_offload;
	/* 2. FW offloads, 0: driver handles */
	u8 mlme_offload;
	/* 3. FW offloads, 0: driver handles */
	u8 hwpc_offload;
	/* 4. FW offloads, 0: driver handles */
	u8 tcp_checksum_offload;
	/* 5. FW offloads, 0: driver handles */
	u8 tcp_offload;
	/* 6. FW offloads, 0: driver handles */
	u8 ps_control_offload;
	/* 7. FW offloads, 0: driver handles */
	u8 wwlan_offload;
	u8 rsvd040;

	/* --- long word 5 ---- */
	/* tcp tx packet length low byte */
	u8 tcp_tx_frame_len_L;
	/* tcp tx packet length high byte */
	u8 tcp_tx_frame_len_H;
	/* tcp rx packet length low byte */
	u8 tcp_rx_frame_len_L;
	/* tcp rx packet length high byte */
	u8 tcp_rx_frame_len_H;
	u8 rsvd050;
	u8 rsvd051;
	u8 rsvd052;
	u8 rsvd053;
} __packed;

/* 8-byte alinment required */
struct fw_hdr {

	/* --- LONG WORD 0 ---- */
	__le16 signature;
	/* 0x8000 ~ 0x8FFF for FPGA version,
	 * 0x0000 ~ 0x7FFF for ASIC version, */
	__le16 version;
	/* define the size of boot loader */
	__le32 dmem_size;


	/* --- LONG WORD 1 ---- */
	/* define the size of FW in IMEM */
	__le32 img_imem_size;
	/* define the size of FW in SRAM */
	__le32 img_sram_size;

	/* --- LONG WORD 2 ---- */
	/* define the size of DMEM variable */
	__le32 fw_priv_size;
	__le16 efuse_addr;
        __le16 h2ccnd_resp_addr;

	/* --- LONG WORD 3 ---- */
	__le32 svn_evision;
	__le32 release_time;

	struct fw_priv fwpriv;

} __packed __aligned(8);

enum fw_status {
	FW_STATUS_INIT = 0,
	FW_STATUS_LOAD_IMEM = 1,
	FW_STATUS_LOAD_EMEM = 2,
	FW_STATUS_LOAD_DMEM = 3,
	FW_STATUS_READY = 4,
};

struct rt_firmware {
	struct fw_hdr *pfwheader;
	enum fw_status fwstatus;
	u16 firmwareversion;
	u8 fw_imem[RTL8192_MAX_FIRMWARE_CODE_SIZE];
	u8 fw_emem[RTL8192_MAX_FIRMWARE_CODE_SIZE];
	u32 fw_imem_len;
	u32 fw_emem_len;
	u32 fw_dmem_len;
	u8 sz_fw_tmpbuffer[RTL8192_MAX_RAW_FIRMWARE_CODE_SIZE];
	u32 sz_fw_tmpbufferlen;
	u16 cmdpacket_fragthresold;
};

struct h2c_rates {
	u8 rates[8];
} __packed;

struct h2c_ext_rates {
	u8 rates[16];
} __packed;

struct h2c_iocmd {
	union {
		struct {
			u8 cmdclass;
			u16 value;
			u8 index;
		} __packed;

		__le32 cmd;
	} __packed;
} __packed;

struct h2c_set_pwrmode_parm {
	u8 mode;
	u8 flag_low_traffic_en;
	u8 flag_lpnav_en;
	u8 flag_rf_low_snr_en;
	/* 1: dps, 0: 32k */
	u8 flag_dps_en;
	u8 bcn_rx_en;
	u8 bcn_pass_cnt;
	/* beacon TO (ms). ¡§=0¡¨ no limit. */
	u8 bcn_to;
	__le16	bcn_itv;
	/* only for VOIP mode. */
	u8 app_itv;
	u8 awake_bcn_itvl;
	u8 smart_ps;
	/* unit: 100 ms */
	u8 bcn_pass_period;
} __packed;

struct h2c_sta_psm {
	u8 aid;
	u8 status;
	u8 addr[ETH_ALEN];
} __packed;

struct h2c_basic_rates {
	struct h2c_rates basic_rates;
} __packed;

enum h2c_channel_plan_types {
	CHANNEL_PLAN_FCC = 0,
	CHANNEL_PLAN_IC = 1,
	CHANNEL_PLAN_ETSI = 2,
	CHANNEL_PLAN_SPAIN = 3,
	CHANNEL_PLAN_FRANCE = 4,
	CHANNEL_PLAN_MKK = 5,
	CHANNEL_PLAN_MKK1 = 6,
	CHANNEL_PLAN_ISRAEL = 7,
	CHANNEL_PLAN_TELEC = 8,

	/* Deprecated */
	CHANNEL_PLAN_MIC = 9,
	CHANNEL_PLAN_GLOBAL_DOMAIN = 10,
	CHANNEL_PLAN_WORLD_WIDE_13 = 11,
	CHANNEL_PLAN_TELEC_NETGEAR = 12,

	CHANNEL_PLAN_NCC = 13,
	CHANNEL_PLAN_5G = 14,
	CHANNEL_PLAN_5G_40M = 15,

	/* keep it last */
	__MAX_CHANNEL_PLAN
};

struct h2c_channel_plan {
	__le32 channel_plan;	/* see h2c_channel_plan_types */
} __packed;

struct h2c_antenna {
	u8 tx_antenna_set;
	u8 rx_antenna_set;
	u8 tx_antenna;
	u8 rx_antenna;
} __packed;

struct h2c_set_mac {
	u8 mac[ETH_ALEN];
} __packed;

struct h2c_joinbss_rpt_parm {
	u8 opmode;
	u8 ps_qos_info;
	u8 bssid[6];
	__le16 bcnitv;
	__le16 aid;
} __packed;

struct h2c_sitesurvey_parm {
	__le32 active;
	__le32 bsslimit;
	__le32 ssidlen;
	u8 ssid[IEEE80211_MAX_SSID_LEN + 1];
} __packed;

struct h2c_disconnect_parm {
	__le32 rsvd0;
} __packed;

struct h2c_setchannel_parm {
	__le32 channel;
} __packed;

struct h2c_setauth_parm {
	u8 mode;
	u8 _1x;		/* 0 = PSK, 1 = EAP */
	u8 rsvd2[2];
} __packed;

struct h2c_wpa_ptk {
	/* EAPOL-Key Key Confirmation Key (KCK) */
	u8 kck[16];
	/* EAPOL-Key Key Encryption Key (KEK) */
	u8 kek[16];
	/* Temporal Key 1 (TK1) */
	u8 tk1[16];
	union {
		/* Temporal Key 2 (TK2) */
		u8 tk2[16];
		struct {
			u8 tx_mic_key[8];
			u8 rx_mic_key[8];
		} athu;
	} u;
};

struct h2c_11fh_network_configuration {	/* ndis_802_11_configuration_fh */
	__le32 length;
	__le32 hop_pattern;		/* as defined by 802.11, MSB set */
	__le32 hop_set;			/* = 1, if non-802.11 */
	__le32 dwell_time;		/* units are in Kibi usec */
} __packed;

struct h2c_network_configuration {	/* ndis_802_11_configuration */
	__le32 length;
	__le32 beacon_period;		/* units are in Kibi usec */
	__le32 atim_window;		/* units are in Kibi usec */
	__le32 frequency;		/* units are kHz - needed but not used?! */
	struct h2c_11fh_network_configuration fh_config;
} __packed;

enum h2c_network_infrastruct_mode {
	MODE_IBSS,
	MODE_BSS,
	MODE_AUTO,
	MODE_INFRA_MAX,		/*
				 * Apparently that's not a real value,
				 * just the upper bound
				 */
	MODE_AP,		/* maybe this should be = INFRA_MAX? */

	/* keep this last */
	__MAX_NETWORK_MODE
};

enum h2c_op_modes {
	OP_AUTO = 0,		/* Let the driver decides which AP to join */
	OP_ADHOC,		/* Single cell network (Ad-Hoc Clients) */
	OP_INFRA,		/* Multi cell network, roaming, ... */
	OP_MASTER,		/* Synchronisation master or AP - useless */
	OP_REPEAT,		/* Wireless Repeater (forwarder) - useless */
	OP_SECOND,		/* Secondary master/repeater (backup) - useless */
	OP_MONITOR,		/* Passive monitor (listen only) - useless */

	/* keep this last */
	__MAC_OP_MODES
};

enum h2c_network_type {
	TYPE_11FH = 0,
	TYPE_11DS,
	TYPE_11OFDM5GHZ,
	TYPE_11OFDM2GHZ,

	/* keep this last */
	__MAX_NETWORK_TYPE
};

struct h2c_opmode {
	u8 mode;		/* see h2c_op_modes */
	u8 padding[3];
} __packed;

struct h2c_ssid {		/* ndis_802_11_ssid */
	u8 length;
	u8 ssid[IEEE80211_MAX_SSID_LEN];
} __packed;

struct h2c_fixed_ies {
	__le32 timestamp;
	__le16 beaconint;
	__le16 caps;
	u8 ie[0];		/*
				 * (fixed?) and variable IE content -
				 * can be up to 768 bytes
				 */
} __packed;

struct h2c_bss {		/* ndis_wlan_bssid_ex */
	__le32 length;		/* length of the full struct */
	u8 bssid[6];
	u8 padding[2];
	struct h2c_ssid	ssid;
	__le32 privacy;
	__le32 rssi;		/* ??? - maybe for request/survey */

	__le32 type;		/* see h2c_network_type */
	struct h2c_network_configuration config;
	__le32 mode;		/* see h2c_network_infrastruct_mode */
	struct h2c_ext_rates rates;
	__le32 ie_length;	/* not sure if this include fixed too */
	struct h2c_fixed_ies ies;
} __packed;

struct h2c_wpa_two_way_parm {
	/* algorithm TKIP or AES */
	u8 pairwise_en_alg;
	u8 group_en_alg;
	struct h2c_wpa_ptk wpa_ptk_value;
} __packed;

struct c2h_addsta_event {
	u8 cam_id;
	u8 padding[3];
} __packed;

enum fw_c2h_event {
	C2H_READ_MACREG_EVENT,				/* 0 */
	C2H_READBB_EVENT,
	C2H_READRF_EVENT,
	C2H_READ_EEPROM_EVENT,
	C2H_READ_EFUSE_EVENT,
	C2H_READ_CAM_EVENT,				/* 5 */
	C2H_GET_BASIC_RATE_EVENT,
	C2H_GET_DATA_RATE_EVENT,
	C2H_SURVEY_EVENT,
	C2H_SURVEY_DONE_EVENT,
	C2H_JOIN_BSS_EVENT,				/* 10 */
	C2H_ADD_STA_EVENT,
	C2H_DEL_STA_EVENT,
	C2H_ATIM_DONE_EVENT,
	C2H_TX_REPORT_EVENT,
	C2H_CCX_REPORT_EVENT,				/* 15 */
	C2H_DTM_REPORT_EVENT,
	C2H_TX_RATE_STATS_EVENT,
	C2H_C2H_LBK_EVENT,
	C2H_FWDBG_EVENT,
	C2H_C2HFEEDBACK_EVENT,				/* 20 */
	C2H_ADDBA_EVENT,
	C2H_HBCN_EVENT,
	C2H_REPORT_PWR_STATE_EVENT,
	C2H_WPS_PBC_EVENT,
	C2H_ADDBA_REPORT_EVENT,				/* 25 */
};

enum fw_h2c_cmd {
	H2C_READ_MACREG_CMD,				/* 0 */
	H2C_WRITE_MACREG_CMD,
	H2C_READBB_CMD,
	H2C_WRITEBB_CMD,
	H2C_READRF_CMD,
	H2C_WRITERF_CMD,				/* 5 */
	H2C_READ_EEPROM_CMD,
	H2C_WRITE_EEPROM_CMD,
	H2C_READ_EFUSE_CMD,
	H2C_WRITE_EFUSE_CMD,
	H2C_READ_CAM_CMD,				/* 10 */
	H2C_WRITE_CAM_CMD,
	H2C_SETBCNITV_CMD,
	H2C_SETMBIDCFG_CMD,
	H2C_JOINBSS_CMD,
	H2C_DISCONNECT_CMD,				/* 15 */
	H2C_CREATEBSS_CMD,
	H2C_SETOPMODE_CMD,
	H2C_SITESURVEY_CMD,
	H2C_SETAUTH_CMD,
	H2C_SETKEY_CMD,					/* 20 */
	H2C_SETSTAKEY_CMD,
	H2C_SETASSOCSTA_CMD,
	H2C_DELASSOCSTA_CMD,
	H2C_SETSTAPWRSTATE_CMD,
	H2C_SETBASICRATE_CMD,				/* 25 */
	H2C_GETBASICRATE_CMD,
	H2C_SETDATARATE_CMD,
	H2C_GETDATARATE_CMD,
	H2C_SETPHYINFO_CMD,
	H2C_GETPHYINFO_CMD,				/* 30 */
	H2C_SETPHY_CMD,
	H2C_GETPHY_CMD,
	H2C_READRSSI_CMD,
	H2C_READGAIN_CMD,
	H2C_SETATIM_CMD,				/* 35 */
	H2C_SETPWRMODE_CMD,
	H2C_JOINBSSRPT_CMD,
	H2C_SETRATABLE_CMD,
	H2C_GETRATABLE_CMD,
	H2C_GETCCXREPORT_CMD,				/* 40 */
	H2C_GETDTMREPORT_CMD,
	H2C_GETTXRATESTATICS_CMD,
	H2C_SETUSBSUSPEND_CMD,
	H2C_SETH2CLBK_CMD,
	H2C_ADDBA_REQ_CMD,				/* 45 */
	H2C_SETCHANNEL_CMD,
	H2C_SET_TXPOWER_CMD,
	H2C_SWITCH_ANTENNA_CMD,
	H2C_SET_XTAL_CAP_CMD,
	H2C_SET_SINGLE_CARRIER_TX_CMD,			/* 50 */
	H2C_SET_SINGLE_TONE_CMD,
	H2C_SET_CARRIER_SUPPRESION_TX_CMD,
	H2C_SET_CONTINOUS_TX_CMD,
	H2C_SWITCH_BW_CMD,
	H2C_TX_BEACON_CMD,				/* 55 */
	H2C_SET_POWER_TRACKING_CMD,
	H2C_AMSDU_TO_AMPDU_CMD,
	H2C_SET_MAC_ADDRESS_CMD,
	H2C_DISCONNECT_CTRL_CMD,
	H2C_SET_CHANNELPLAN_CMD,			/* 60 */
	H2C_DISCONNECT_CTRL_EX_CMD,
	H2C_GET_H2C_LBK_CMD,
	H2C_SET_PROBE_REQ_EXTRA_IE_CMD,
	H2C_SET_ASSOC_REQ_EXTRA_IE_CMD,
	H2C_SET_PROBE_RSP_EXTRA_IE_CMD,			/* 65 */
	H2C_SET_ASSOC_RSP_EXTRA_IE_CMD,
	H2C_GET_CURRENT_DATA_RATE_CMD,
	H2C_GET_TX_RETRY_CNT_CMD,
	H2C_GET_RX_RETRY_CNT_CMD,
	H2C_GET_BCN_OK_CNT_CMD,				/* 70 */
	H2C_GET_BCN_ERR_CNT_CMD,
	H2C_GET_CURRENT_TXPOWER_CMD,
	H2C_SET_DIG_CMD,
	H2C_SET_RA_CMD,
	H2C_SET_PT_CMD,					/* 75 */
	H2C_READ_RSSI_CMD,
	MAX_H2CCMD,					/* 77 */
};

/* The following macros are used for FW
 * CMD map and parameter updated. */
#define FW_CMD_IO_CLR(rtlpriv, _Bit)				\
	do {							\
		udelay(1000);					\
		rtlpriv->rtlhal.fwcmd_iomap &= (~_Bit);		\
	} while (0)

#define FW_CMD_IO_UPDATE(rtlpriv, _val)				\
	rtlpriv->rtlhal.fwcmd_iomap = _val;

#define FW_CMD_IO_SET(rtlpriv, _val)				\
	do {							\
		rtl_write_word(rtlpriv, LBUS_MON_ADDR, (u16)_val);	\
		FW_CMD_IO_UPDATE(rtlpriv, _val);		\
	} while (0)

#define FW_CMD_PARA_SET(rtlpriv, _val)				\
	do {							\
		rtl_write_dword(rtlpriv, LBUS_ADDR_MASK, _val);	\
		rtlpriv->rtlhal.fwcmd_ioparam = _val;		\
	} while (0)

#define FW_CMD_IO_QUERY(rtlpriv)				\
	(u16)(rtlpriv->rtlhal.fwcmd_iomap)
#define FW_CMD_IO_PARA_QUERY(rtlpriv)				\
	((u32)(rtlpriv->rtlhal.fwcmd_ioparam))

int rtl92s_download_fw(struct ieee80211_hw *hw);
void rtl92s_set_fw_pwrmode_cmd(struct ieee80211_hw *hw, u8 mode);
void rtl92s_set_fw_joinbss_report_cmd(struct ieee80211_hw *hw,
				      u8 mstatus, u8 ps_qosinfo);
int rtl92s_set_fw_sitesurvey_cmd(struct ieee80211_hw *hw,
				  struct ieee80211_vif *vif,
				  struct cfg80211_scan_request *req);
int rtl92s_set_fw_disconnect_cmd(struct ieee80211_hw *hw);
u8 rtl92s_set_fw_setchannel_cmd(struct ieee80211_hw *hw);
int rtl92s_set_fw_setauth_cmd(struct ieee80211_hw *hw);
int rtl92s_set_fw_opmode_cmd(struct ieee80211_hw *hw, enum h2c_op_modes mode);

u8 rtl92s_fw_iocmd(struct ieee80211_hw *hw, const u32 cmd);
void rtl92s_fw_iocmd_data(struct ieee80211_hw *hw, u32 *cmd, const u8 flag);

u32 rtl92s_fw_iocmd_read(struct ieee80211_hw *hw, u8 ioclass, u16 iovalue, u8 ioindex);
u8 rtl92s_fw_iocmd_write(struct ieee80211_hw *hw, u8 ioclass, u16 iovalue, u8 ioindex, u32 val);

void rtl92su_set_mac_addr(struct ieee80211_hw *hw, const u8 * addr);
void rtl92su_set_basic_rate(struct ieee80211_hw *hw);

u8 rtl92s_set_fw_assoc(struct ieee80211_hw *hw, u8 *mac, bool assoc);

#endif
