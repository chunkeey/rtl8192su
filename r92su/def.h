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
#ifndef __R92SU_DEF_H__
#define __R92SU_DEF_H__

#include "h2cc2h.h"

#define RX_MPDU_QUEUE				0
#define RX_CMD_QUEUE				1
#define RX_MAX_QUEUE				2
#define NUM_ACS					4

#define SHORT_SLOT_TIME				9
#define NON_SHORT_SLOT_TIME			20

/* Rx smooth factor */
#define	RX_SMOOTH_FACTOR			20

/* Queue Select Value in TxDesc */
#define QSLT_BK					0x1
#define QSLT_BE					0x3
#define QSLT_VI					0x5
#define QSLT_VO					0x7
#define QSLT_BEACON				0x10
#define QSLT_HIGH				0x11
#define QSLT_MGNT				0x12
#define QSLT_CMD				0x13

#define	PHY_RSSI_SLID_WIN_MAX			100
#define	PHY_LINKQUALITY_SLID_WIN_MAX		20
#define	PHY_BEACON_RSSI_SLID_WIN_MAX		10

/* Tx Desc */
#define TX_DESC_SIZE				32

struct tx_hdr {
	/* DWORD 0 */
	__le16 pkt_len;		/*  0 - 15 */
	__u8 offset;		/* 16 - 23 */
	u8 type:2;		/* 24 - 25 */
	u8 last_seg:1;		/* 26 */
	u8 first_seg:1;		/* 27 */
	u8 linip:1;		/* 28 */
	u8 amsdu:1;		/* 29 */
	u8 gf:1;		/* 30 */
	u8 own:1;		/* 31 */

	/* DWORD 1 */
	u8 mac_id:5;		/*  0 -  4 */
	u8 more_data:1;		/*  5 */
	u8 more_frag:1;		/*  6 */
	u8 pifs:1;		/*  7 */
	u8 queue_sel:5;		/*  8 - 12 */
	u8 ack_policy:2;	/* 13 - 14 */
	u8 no_acm:1;		/* 15 */
	u8 non_qos:1;		/* 16 */
	u8 key_id:2;		/* 17 - 18 */
	u8 oui:1;		/* 19 */
	u8 pkt_type:1;		/* 20 */
	u8 en_desc_id:1;	/* 21 */
	u8 sec_type:2;		/* 22 - 23 */
	u8 wds:1;		/* 24 */
	u8 htc:1;		/* 25 */
	u8 pkt_offset:5;	/* 26 - 30 */
	u8 hwpc:1;		/* 31 */

	/* DWORD 2 */
	u8 data_retry_limit:6;	/*  0 -  5 */
	u8 retry_limit_en:1;	/*  6 */
	u8 bmc:1;		/*  7 */
	u8 tsfl:4;		/*  8 - 11 */
	u8 rts_retry_count:6;	/* 12 - 17 */
	u8 data_retry_count:6;	/* 18 - 23 */
	u8 rsvd_macid:5;	/* 24 - 28 */
	u8 agg_en:1;		/* 29 */
	u8 agg_break:1;		/* 30 */
	u8 own_mac:1;		/* 31 */

	/* DWORD 3 */
	u8 heap_page;		/* 0  -  7 */
	u8 tail_page;		/* 8  - 15 */
	__le16 seq:12;		/* 16 - 27 */
	u8 frag:4;		/* 28 - 31 */

	/* DWORD 4 */
	u8 rts_rate:6;		/*  0 -  5 */
	u8 dis_rts_fb:1;	/*  6 */
	u8 rts_rate_fb_limit:4;	/*  7 - 10 */
	u8 cts_en:1;		/* 11 */
	u8 rts_en:1;		/* 12 */
	u8 ra_brsr_id:3;	/* 13 - 15 */
	u8 tx_ht:1;		/* 16 */
	u8 tx_short:1;		/* 17 */
	u8 tx_bw:1;		/* 18 */
	u8 tx_sub_carrier:2;	/* 19 - 20 */
	u8 tx_stbc:2;		/* 21 - 22 */
	u8 tx_rd:1;		/* 23 */
	u8 rts_ht:1;		/* 24 */
	u8 rts_short:1;		/* 25 */
	u8 rts_bw:1;		/* 26 */
	u8 rts_sub_carrier:2;	/* 27 - 28 */
	u8 rts_stbc:2;		/* 29 - 30 */
	u8 user_rate:1;		/* 31 */

	/* DWORD 5 */
	__le16 packet_id:9;	/*  0 -  8 */
	u8 tx_rate:6;		/*  9 - 14 */
	u8 dis_fb:1;		/* 15 */
	u8 data_rate_fb_limit:5;/* 16 - 20 */
	__le16 tx_agc:11;	/* 21 - 31 */

	/* DWORD 6 */
	__le16 ip_check_sum;	/* 0  - 15 */
	__le16 tcp_check_sum;	/* 16 - 31 */

	/* DWORD 7 */
	__le16 tx_buffer_size;	/* 0  - 15 */
	u8 ip_hdr_offset:8;	/* 16 - 23 */
	u8 cmd_seq:7;		/* 24 - 30 */
	u8 tcp_en:1;		/* 31 */
} __packed;

/* Rx Desc */
#define RX_DESC_SIZE				24
#define RX_DRV_INFO_SIZE_UNIT			8

struct rx_hdr {
	/* DWORD 0 */
	__le16 pkt_len:14;	/*  0 - 13 */
	u8 crc32:1;		/* 14 */
	u8 icv:1;		/* 15 */
	u8 drvinfo_size:4;	/* 16 - 19 */
	u8 security:3;		/* 20 - 22 */
	u8 qos:1;		/* 23 */
	u8 shift:2;		/* 24 - 25 */
	u8 phy_status:1;	/* 26 */
	u8 swdec:1;		/* 27 */
	u8 last_seg:1;		/* 28 */
	u8 first_seg:1;		/* 29 */
	u8 eor:1;		/* 30 */
	u8 own:1;		/* 31 */

	/* DWORD 1 */
	u8 mac_id:5;		/*  0 -  4 */
	u8 tid:4;		/*  5 -  8 */
	u8 unkn0100:5;		/*  9 - 13 */
	u8 paggr:1;		/* 14 */
	u8 faggr:1;		/* 15 */
	u8 a1_fit:4;		/* 16 - 19 */
	u8 a2_fit:4;		/* 20 - 23 */
	u8 pam:1;		/* 24 */
	u8 pwr:1;		/* 25 */
	u8 more_data:1;		/* 26 */
	u8 more_frag:1;		/* 27 */
	u8 type:2;		/* 28 - 29 */
	u8 mc:1;		/* 30 */
	u8 bc:1;		/* 31 */

	/* DWORD 2 */
	__le16 seq:12;		/*  0 - 11 */
	u8 frag:4;		/* 12 - 15 */
	u8 pkt_cnt;		/* 16 - 23 */
	u8 unkn0200:6;		/* 24 - 29 */
	u8 next_ind:1;		/* 30 */
	u8 unkn0201:1;		/* 31 */

	/* DWORD 3 */
	u8 rx_mcs:6;		/*  0 -  5 */
	u8 rx_ht:1;		/*  6 */
	u8 amsdu:1;		/*  7 */
	u8 splcp:1;		/*  8 */
	u8 bw:1;		/*  9 */
	u8 htc:1;		/* 10 */
	u8 tcp_chk_rpt:1;	/* 11 */
	u8 ip_chk_rpt:1;	/* 12 */
	u8 tcp_chk_valid:1;	/* 13 */
	u8 htc2:1;		/* 14 */
	u8 hwpc_ind:1;		/* 15 */
	__le16 iv0;		/* 16 - 31 */

	/* DWORD 4 */
	__le32 iv1;		/*  0 - 31 */

	/* DWORD 5 */
	__le32 tsf32;		/*  0 - 31 */
} __packed;

struct rx_hdr_phy_cck {
	/* For CCK rate descriptor. This is an unsigned 8:1 variable.
	 * LSB bit present 0.5. And MSB 7 bts present a signed value.
	 * Range from -64 to + 63.5 */
	u8 adc_pwdb_X[4];
	u8 sq_rpt;
	u8 cck_agc_rpt:6;
	u8 report:2;
} __packed;

struct rx_hdr_phy_ofdm {
	u8 gain_trws[4];
	u8 pwdb_all;
	u8 cfosho[4];
	u8 cfotail[4];
	u8 rxevm[2];
	u8 rxsnr[4];
	u8 pdsnr[2];
	u8 csi_current[2];
	u8 csi_target[2];
	u8 sigevm[2];
	u8 max_ex_power;
} __packed;

union rx_hdr_phy {
	struct rx_hdr_phy_cck cck;
	struct rx_hdr_phy_ofdm ofdm;
};

#define H2CC2H_HDR_LEN		(8)

struct h2cc2h {
	__le16 len;
	u8 event;
	u8 cmd_seq:7;
	u8 last:1;

	u8 agg_num;
	u8 unkn;
	__le16 agg_total_len;

	u8 data[0];
} __packed;

struct tx_packet {
	struct tx_hdr hdr;

	union {
		struct ieee80211_hdr i3e;
		struct h2cc2h h2c;
		u8 raw_data[0];
	} __packed;
} __packed;

struct rx_packet {
	struct rx_hdr hdr;
	union {
		/* No direct access to the rx data possible. The rx_hdr
		 * contains shift value (used to tell the offset of the
		 * ieee80211_hdr in 64-bit words).
		 */

		struct h2cc2h c2h;
		union rx_hdr_phy phy;
		u8 raw_data[0];
	} __packed;
} __packed;

enum rtl_desc92_rate {
	DESC92_RATE1M = 0x00,
	DESC92_RATE2M = 0x01,
	DESC92_RATE5_5M = 0x02,
	DESC92_RATE11M = 0x03,

	DESC92_RATE6M = 0x04,
	DESC92_RATE9M = 0x05,
	DESC92_RATE12M = 0x06,
	DESC92_RATE18M = 0x07,
	DESC92_RATE24M = 0x08,
	DESC92_RATE36M = 0x09,
	DESC92_RATE48M = 0x0a,
	DESC92_RATE54M = 0x0b,

	DESC92_RATEMCS0 = 0x0c,
	DESC92_RATEMCS1 = 0x0d,
	DESC92_RATEMCS2 = 0x0e,
	DESC92_RATEMCS3 = 0x0f,
	DESC92_RATEMCS4 = 0x10,
	DESC92_RATEMCS5 = 0x11,
	DESC92_RATEMCS6 = 0x12,
	DESC92_RATEMCS7 = 0x13,
	DESC92_RATEMCS8 = 0x14,
	DESC92_RATEMCS9 = 0x15,
	DESC92_RATEMCS10 = 0x16,
	DESC92_RATEMCS11 = 0x17,
	DESC92_RATEMCS12 = 0x18,
	DESC92_RATEMCS13 = 0x19,
	DESC92_RATEMCS14 = 0x1a,
	DESC92_RATEMCS15 = 0x1b,
	DESC92_RATEMCS15_SG = 0x1c,
	DESC92_RATEMCS32 = 0x20,
};

enum rf_optype {
	RF_OP_BY_SW_3WIRE = 0,
	RF_OP_BY_FW,
	RF_OP_MAX
};

enum ic_inferiority {
	IC_INFERIORITY_A = 0,
	IC_INFERIORITY_B = 1,
};

enum rf_type {
	RF_1T1R = 0,
	RF_1T2R = 1,
	RF_2T2R = 2,
	RF_2T2R_GREEN = 3,
};

enum ht_channel_width {
	HT_CHANNEL_WIDTH_20 = 0,
	HT_CHANNEL_WIDTH_20_40 = 1,
};

enum r92su_enc_alg {
	NO_ENCRYPTION = 0,
	WEP40_ENCRYPTION = 1,
	TKIP_ENCRYPTION = 2,
	TKIP_WTMIC = 3,
	AESCCMP_ENCRYPTION = 4,
	WEP104_ENCRYPTION = 5,

	__MAX_ENCRYPTION
};

static inline void __check_def__(void)
{
	BUILD_BUG_ON(sizeof(struct tx_hdr) != TX_DESC_SIZE);
	BUILD_BUG_ON(sizeof(struct rx_hdr) != RX_DESC_SIZE);
	BUILD_BUG_ON(sizeof(struct h2cc2h) != H2CC2H_HDR_LEN);

	BUILD_BUG_ON(offsetof(struct tx_hdr, ip_check_sum) != 24);
	BUILD_BUG_ON(offsetof(struct tx_hdr, heap_page) != 12);
	BUILD_BUG_ON(offsetof(struct rx_hdr, tsf32) != 20);
}

#endif /* __R92SU_DEF_H__ */
