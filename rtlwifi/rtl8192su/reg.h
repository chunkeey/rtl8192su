/******************************************************************************
 *
 * Copyright(c) 2007 - 2010 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * Modifications for inclusion into the Linux staging tree are
 * Copyright(c) 2010 Larry Finger. All rights reserved.
 *
 * Contact information:
 * WLAN FAE <wlanfae@realtek.com>
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 ******************************************************************************/
#ifndef __RTL8712_REGDEF_H__
#define __RTL8712_REGDEF_H__

/* Hardware / Firmware Memory Layout */
#define RTL8712_IOBASE_TXPKT		(0x10200000)	/* IOBASE_TXPKT */
#define RTL8712_IOBASE_RXPKT		(0x10210000)	/* IOBASE_RXPKT */
#define RTL8712_IOBASE_RXCMD		(0x10220000)	/* IOBASE_RXCMD */
#define RTL8712_IOBASE_TXSTATUS		(0x10230000)	/* IOBASE_TXSTATUS */
#define RTL8712_IOBASE_RXSTATUS		(0x10240000)	/* IOBASE_RXSTATUS */
#define RTL8712_IOBASE_IOREG		(0x10250000)	/* IOBASE_IOREG ADDR */
#define RTL8712_IOBASE_SCHEDULER	(0x10260000)	/* IOBASE_SCHEDULE */

#define RTL8712_IOBASE_TRXDMA		(0x10270000)	/* IOBASE_TRXDMA */
#define RTL8712_IOBASE_TXLLT		(0x10280000)	/* IOBASE_TXLLT */
#define RTL8712_IOBASE_WMAC		(0x10290000)	/* IOBASE_WMAC */
#define RTL8712_IOBASE_FW2HW		(0x102A0000)	/* IOBASE_FW2HW */
#define RTL8712_IOBASE_ACCESS_PHYREG	(0x102B0000)	/* IOBASE_ACCESS_PHYREG */

#define RTL8712_IOBASE_FF		(0x10300000)	/* IOBASE_FIFO 0x1031000~0x103AFFFF */

/* IOREG Offsets for 8712 and friends */
#define RTL8712_SYSCFG_			(RTL8712_IOBASE_IOREG)
#define RTL8712_CMDCTRL_		(RTL8712_IOBASE_IOREG + 0x40)
#define RTL8712_MACIDSETTING_		(RTL8712_IOBASE_IOREG + 0x50)
#define RTL8712_TIMECTRL_		(RTL8712_IOBASE_IOREG + 0x80)
#define RTL8712_FIFOCTRL_		(RTL8712_IOBASE_IOREG + 0xA0)
#define RTL8712_RATECTRL_		(RTL8712_IOBASE_IOREG + 0x160)
#define RTL8712_EDCASETTING_		(RTL8712_IOBASE_IOREG + 0x1D0)
#define RTL8712_WMAC_			(RTL8712_IOBASE_IOREG + 0x200)
#define RTL8712_SECURITY_		(RTL8712_IOBASE_IOREG + 0x240)
#define RTL8712_POWERSAVE_		(RTL8712_IOBASE_IOREG + 0x260)
#define RTL8712_BB_			(RTL8712_IOBASE_IOREG + 0x2C0)
#define RTL8712_OFFLOAD_		(RTL8712_IOBASE_IOREG + 0x2D0)
#define RTL8712_GP_			(RTL8712_IOBASE_IOREG + 0x2E0)
#define RTL8712_INTERRUPT_		(RTL8712_IOBASE_IOREG + 0x300)
#define RTL8712_DEBUGCTRL_		(RTL8712_IOBASE_IOREG + 0x310)
#define RTL8712_IOCMD_			(RTL8712_IOBASE_IOREG + 0x370)
#define RTL8712_PHY_			(RTL8712_IOBASE_IOREG + 0x800)
#define RTL8712_PHY_P1_			(RTL8712_IOBASE_IOREG + 0x900)
#define RTL8712_PHY_CCK_		(RTL8712_IOBASE_IOREG + 0xA00)
#define RTL8712_PHY_OFDM_		(RTL8712_IOBASE_IOREG + 0xC00)
#define RTL8712_PHY_RXAGC_		(RTL8712_IOBASE_IOREG + 0xE00)
#define RTL8712_USB_			(RTL8712_IOBASE_IOREG + 0xFE00)

/* ----------------------------------------------------- */
/* 0x10250000h ~ 0x1025003Fh System Configuration */
/* ----------------------------------------------------- */
#define REG_SYS_ISO_CTRL		(RTL8712_SYSCFG_ + 0x0000)
#define		ISO_MD2PP			BIT(0)
#define		ISO_PA2PCIE			BIT(3)
#define		ISO_PLL2MD			BIT(4)
#define		ISO_PWC_DV2RP			BIT(11)
#define		ISO_PWC_RV2RP			BIT(12)

#define REG_SYS_FUNC_EN			(RTL8712_SYSCFG_ + 0x0002)
#define		FEN_CPUEN			BIT(10)
#define		FEN_DCORE			BIT(11)
#define		FEN_MREGEN			BIT(15)

#define REG_PMC_FSM			(RTL8712_SYSCFG_ + 0x0004)
#define REG_SYS_CLKR			(RTL8712_SYSCFG_ + 0x0008)
#define		SYS_CLKSEL_80M			BIT(0)
#define		SYS_PS_CLKSEL			BIT(1)
#define		SYS_CPU_CLKSEL			BIT(2)
#define		SYS_MAC_CLK_EN			BIT(11)
#define		SYS_SWHW_SEL			BIT(14)
#define		SYS_FWHW_SEL			BIT(15)

#define REG_EE_9346CR			(RTL8712_SYSCFG_ + 0x000A)
#define REG_EPROM_CMD			(RTL8712_SYSCFG_ + 0x000A)	// DUP
#define REG_EE_VPD			(RTL8712_SYSCFG_ + 0x000C)
#define REG_AFE_MISC			(RTL8712_SYSCFG_ + 0x0010)
#define		AFE_BGEN			BIT(0)
#define		AFE_MBEN			BIT(1)
#define 	AFE_MISC_I32_EN			BIT(3)

#define REG_SPS0_CTRL			(RTL8712_SYSCFG_ + 0x0011)
#define REG_SPS1_CTRL			(RTL8712_SYSCFG_ + 0x0018)
#define REG_RF_CTRL			(RTL8712_SYSCFG_ + 0x001F)
#define REG_LDOA15_CTRL			(RTL8712_SYSCFG_ + 0x0020)
#define		LDA15_EN			BIT(0)

#define REG_LDOV12D_CTRL		(RTL8712_SYSCFG_ + 0x0021)
#define		LDV12_EN			BIT(0)
#define		LDV12_SDBY			BIT(1)

#define REG_LDOHCI12_CTRL		(RTL8712_SYSCFG_ + 0x0022)
#define REG_LDO_USB_CTRL		(RTL8712_SYSCFG_ + 0x0023)
#define REG_LPLDO_CTRL			(RTL8712_SYSCFG_ + 0x0024)
#define REG_AFE_XTAL_CTRL		(RTL8712_SYSCFG_ + 0x0026)
#define REG_AFE_PLL_CTRL		(RTL8712_SYSCFG_ + 0x0028)
#define REG_EFUSE_CTRL			(RTL8712_SYSCFG_ + 0x0030)
#define REG_EFUSE_TEST			(RTL8712_SYSCFG_ + 0x0034)
#define REG_PWR_DATA			(RTL8712_SYSCFG_ + 0x0038)
#define REG_DPS_TIMER			(RTL8712_SYSCFG_ + 0x003C)
#define REG_RCLK_MON			(RTL8712_SYSCFG_ + 0x003E)
#define REG_EFUSE_CLK_CTRL		(RTL8712_SYSCFG_ + 0x02F8)

/* ----------------------------------------------------- */
/* 0x10250040h ~ 0x1025004Fh Command Control */
/* ----------------------------------------------------- */
#define REG_CR				(RTL8712_CMDCTRL_ + 0x0000)
#define		HCI_TXDMA_EN			BIT(2)
#define		HCI_RXDMA_EN			BIT(3)
#define		TXDMA_EN			BIT(4)
#define		RXDMA_EN			BIT(5)
#define		FW2HW_EN			BIT(6)
#define		DDMA_EN				BIT(7)
#define		MACTXEN				BIT(8)
#define		MACRXEN				BIT(9)
#define		SCHEDULE_EN			BIT(10)
#define		BB_GLB_RSTN			BIT(12)
#define		BBRSTN				BIT(13)
#define		APSDOFF				BIT(14)
#define		APSDOFF_STATUS			BIT(15)

#define REG_TXPAUSE			(RTL8712_CMDCTRL_ + 0x0002)
#define		StopBK				BIT(0)
#define		StopBE				BIT(1)
#define		StopVI				BIT(2)
#define		StopVO				BIT(3)
#define		StopMgt				BIT(4)
#define		StopHigh			BIT(5)
#define		StopHCCA			BIT(6)

#define	REG_LBKMD_SEL			(RTL8712_CMDCTRL_ + 0x0003)
#define		LBK_NORMAL			0x00
#define		LBK_MAC_DLB			(BIT(0) | BIT(1))
#define		LBK_DMA_LB			(BIT(0) | BIT(1) | BIT(2))
#define		LBK_MAC_LB			(BIT(0) | BIT(1) | BIT(3))

#define REG_TCR				(RTL8712_CMDCTRL_ + 0x0004)
#define		IMEM_CODE_DONE			BIT(0)
#define		IMEM_CHK_RPT			BIT(1)
#define		EMEM_CODE_DONE			BIT(2)
#define		EXT_IMEM_CODE_DONE		BIT(2)	// DUP
#define		EMEM_CHK_RPT			BIT(3)
#define		EXT_IMEM_CHK_RPT		BIT(3)	// DUP
#define		DMEM_CODE_DONE			BIT(4)
#define		IMEM_RDY			BIT(5)
#define		IMEM				BIT(5)	// DUP
#define		BASECHG				BIT(6)
#define		FWRDY				BIT(7)
#define		TSFEN				BIT(8)
#define		TCR_TSFEN			BIT(8)  // DUP
#define		TSFRST				BIT(9)
#define		TCR_TSFRST			BIT(9)  // DUP
#define		FAKE_IMEM_EN			BIT(15)
#define		TCR_FAKE_IMEM_EN		BIT(15)  // DUP
#define		TCRCRC				BIT(16)
#define		TCR_CRC				BIT(16)  // DUP
#define		CfendForm			BIT(17)
#define		TCRICV				BIT(19)
#define		TCR_ICV				BIT(19)  // DUP
#define		DISCW				BIT(20)
#define		TCR_DISCW			BIT(20)  // DUP
#define		TXDMAPRE2FULL			BIT(23)
#define		HWPC_TX_EN			BIT(24)
#define		TCP_OFDL_EN			BIT(25)
#define		TCR_HWPC_TX_EN			BIT(24)  // DUP
#define		TCR_TCP_OFDL_EN			BIT(25)
#define		FWALLRDY			(BIT(0) | BIT(1) | BIT(2) | \
						BIT(3) | BIT(4) | BIT(5) | \
						BIT(6) | BIT(7))
#define		LOAD_FW_READY			(IMEM_CODE_DONE | \
						IMEM_CHK_RPT | \
						EMEM_CODE_DONE | \
						EMEM_CHK_RPT | \
						DMEM_CODE_DONE | \
						IMEM_RDY | \
						BASECHG | \
						FWRDY)
#define		TXDMA_INIT_VALUE		(IMEM_CHK_RPT | \
						EXT_IMEM_CHK_RPT)

#define REG_RCR				(RTL8712_CMDCTRL_ + 0x0008)
#define		RCR_AAP				BIT(0)
#define		RCR_APM				BIT(1)
#define		RCR_AM				BIT(2)
#define		RCR_AB				BIT(3)
#define		RCR_RXSHFT_EN			BIT(4)
#define		RCR_ACRC32			BIT(5)
#define		RCR_APP_BA_SSN			BIT(6)
#define		RCR_MXDMA_OFFSET		8
#define		RCR_RXDESC_LK_EN		BIT(11)
#define		RCR_AICV			BIT(12)
#define		RCR_RXFTH			BIT(13)
#define		RCR_FIFO_OFFSET			13	// should that be 14?
#define		RCR_APP_ICV			BIT(16)
#define		RCR_APP_MIC			BIT(17)
#define		RCR_ADF				BIT(18)
#define		RCR_ACF				BIT(19)
#define		RCR_AMF				BIT(20)
#define		RCR_ADD3			BIT(21)
#define		RCR_APWRMGT			BIT(22)
#define		RCR_CBSSID			BIT(23)
#define		RCR_APP_PHYST_STAFF		BIT(24)
#define		RCR_APP_PHYST_RXFF		BIT(25)
#define		RCR_RX_TCPOFDL_EN		BIT(26)
#define		RCR_ENMBID			BIT(27)
#define		RCR_HTC_LOC_CTRL		BIT(28)
#define		RCR_DIS_AES_2BYTE		BIT(29)
#define		RCR_DIS_ENC_2BYTE		BIT(30)
#define		RCR_APPFCS			BIT(31)

#define REG_MSR				(RTL8712_CMDCTRL_ + 0x000C)
#define REG_SYSF_CFG			(RTL8712_CMDCTRL_ + 0x000D)
#define REG_MBIDCTRL			(RTL8712_CMDCTRL_ + 0x000E)

/* ----------------------------------------------------- */
/* 0x10250050h ~ 0x1025007Fh MAC ID Settings */
/* ----------------------------------------------------- */
#define REG_MACID			(RTL8712_MACIDSETTING_ + 0x0000)
#define REG_BSSIDR			(RTL8712_MACIDSETTING_ + 0x0008)
#define REG_HWVID			(RTL8712_MACIDSETTING_ + 0x000E)
#define REG_MAR				(RTL8712_MACIDSETTING_ + 0x0010)
#define REG_MBIDCANCONTENT		(RTL8712_MACIDSETTING_ + 0x0018)
#define REG_MBIDCANCFG			(RTL8712_MACIDSETTING_ + 0x0020)
#define REG_BUILDTIME			(RTL8712_MACIDSETTING_ + 0x0024)
#define REG_BUILDUSER			(RTL8712_MACIDSETTING_ + 0x0028)

/* ----------------------------------------------------- */
/* 0x10250080h ~ 0x1025009Fh Timer Control */
/* ----------------------------------------------------- */
#define REG_TSFTR			(RTL8712_TIMECTRL_ + 0x00)
#define REG_USTIME			(RTL8712_TIMECTRL_ + 0x08)
#define REG_SLOT_TIME			(RTL8712_TIMECTRL_ + 0x09)
#define REG_TUBASE			(RTL8712_TIMECTRL_ + 0x0A)
#define REG_SIFS_CCK			(RTL8712_TIMECTRL_ + 0x0C)
#define REG_SIFS_OFDM			(RTL8712_TIMECTRL_ + 0x0E)
#define REG_PIFS			(RTL8712_TIMECTRL_ + 0x10)
#define REG_ACK_TIMEOUT			(RTL8712_TIMECTRL_ + 0x11)
#define REG_EIFS			(RTL8712_TIMECTRL_ + 0x12)
#define REG_BCN_INTERVAL		(RTL8712_TIMECTRL_ + 0x14)
#define REG_ATIMWND			(RTL8712_TIMECTRL_ + 0x16)
#define REG_DRVERLYINT			(RTL8712_TIMECTRL_ + 0x18)
#define REG_BCNDMATIM			(RTL8712_TIMECTRL_ + 0x1A)
#define REG_BCNERRTH			(RTL8712_TIMECTRL_ + 0x1C)
#define REG_MLT				(RTL8712_TIMECTRL_ + 0x1D)

/* ----------------------------------------------------- */
/* 0x102500A0h ~ 0x1025015Fh FIFO Control */
/* ----------------------------------------------------- */
#define REG_RQPN			(RTL8712_FIFOCTRL_ + 0x00)
#define REG_RXFF_BNDY			(RTL8712_FIFOCTRL_ + 0x0C)
#define REG_RXRPT_BNDY			(RTL8712_FIFOCTRL_ + 0x10)
#define REG_TXPKTBUF_PGBNDY		(RTL8712_FIFOCTRL_ + 0x14)
#define REG_PBP				(RTL8712_FIFOCTRL_ + 0x15)
#define REG_RX_DRVINFO_SZ		(RTL8712_FIFOCTRL_ + 0x16)
#define REG_TXFF_STATUS			(RTL8712_FIFOCTRL_ + 0x17)
#define REG_RXFF_STATUS			(RTL8712_FIFOCTRL_ + 0x18)
#define REG_TXFF_EMPTY_TH		(RTL8712_FIFOCTRL_ + 0x19)
#define REG_SDIO_RX_BLKSZ		(RTL8712_FIFOCTRL_ + 0x1C)
#define REG_RXDMA_RXCTRL		(RTL8712_FIFOCTRL_ + 0x1D)
#define		RXDMA_AGG_EN			BIT(7)

#define REG_RXPKT_NUM			(RTL8712_FIFOCTRL_ + 0x1E)
#define REG_RXPKT_NUM_C2H		(RTL8712_FIFOCTRL_ + 0x1F)
#define REG_C2HCMD_UDT_SIZE		(RTL8712_FIFOCTRL_ + 0x20)
#define REG_C2HCMD_UDT_ADDR		(RTL8712_FIFOCTRL_ + 0x22)
#define REG_FIFOPAGE2			(RTL8712_FIFOCTRL_ + 0x24)
#define REG_FIFOPAGE1			(RTL8712_FIFOCTRL_ + 0x28) // PAGE1 after PAGE2??
#define REG_FW_RSVD_PG_CTRL		(RTL8712_FIFOCTRL_ + 0x30)
#define	REG_FIFOPAGE5			(RTL8712_FIFOCTRL_ + 0x34)
#define	REG_FW_RSVD_PG_CRTL		(RTL8712_FIFOCTRL_ + 0x38)
#define	REG_RXDMA_AGG_PG_TH		(RTL8712_FIFOCTRL_ + 0x39)
#define	REG_TXDESC_MSK			(RTL8712_FIFOCTRL_ + 0x3C)
#define REG_TXRPTFF_RDPTR		(RTL8712_FIFOCTRL_ + 0x40)
#define REG_TXRPTFF_WTPTR		(RTL8712_FIFOCTRL_ + 0x44)
#define REG_C2HFF_RDPTR			(RTL8712_FIFOCTRL_ + 0x48)
#define REG_C2HFF_WTPTR			(RTL8712_FIFOCTRL_ + 0x4C)
#define REG_RXFF0_RDPTR			(RTL8712_FIFOCTRL_ + 0x50)
#define REG_RXFF0_WTPTR			(RTL8712_FIFOCTRL_ + 0x54)
#define REG_RXFF1_RDPTR			(RTL8712_FIFOCTRL_ + 0x58)
#define REG_RXFF1_WTPTR			(RTL8712_FIFOCTRL_ + 0x5C)
#define REG_RXRPT0FF_RDPTR		(RTL8712_FIFOCTRL_ + 0x60)
#define REG_RXRPT0FF_WTPTR		(RTL8712_FIFOCTRL_ + 0x64)
#define REG_RXRPT1FF_RDPTR		(RTL8712_FIFOCTRL_ + 0x68)
#define REG_RXRPT1FF_WTPTR		(RTL8712_FIFOCTRL_ + 0x6C)
#define REG_RX0PKTNUM			(RTL8712_FIFOCTRL_ + 0x72)
#define REG_RX1PKTNUM			(RTL8712_FIFOCTRL_ + 0x74)
#define REG_RXFLTMAP0			(RTL8712_FIFOCTRL_ + 0x76)
#define REG_RXFLTMAP1			(RTL8712_FIFOCTRL_ + 0x78)
#define REG_RXFLTMAP2			(RTL8712_FIFOCTRL_ + 0x7A)
#define REG_RXFLTMAP3			(RTL8712_FIFOCTRL_ + 0x7c)
#define REG_TBDA			(RTL8712_FIFOCTRL_ + 0x84)
#define REG_THPDA			(RTL8712_FIFOCTRL_ + 0x88)
#define REG_TCDA			(RTL8712_FIFOCTRL_ + 0x8C)
#define REG_TMDA			(RTL8712_FIFOCTRL_ + 0x90)
#define REG_HDA				(RTL8712_FIFOCTRL_ + 0x94)
#define REG_TVODA			(RTL8712_FIFOCTRL_ + 0x98)
#define REG_TVIDA			(RTL8712_FIFOCTRL_ + 0x9C)
#define REG_TBEDA			(RTL8712_FIFOCTRL_ + 0xA0)
#define REG_TBKDA			(RTL8712_FIFOCTRL_ + 0xA4)
#define REG_RCDA			(RTL8712_FIFOCTRL_ + 0xA8)
#define REG_RDSA			(RTL8712_FIFOCTRL_ + 0xAC)
#define REG_TXPKT_NUM_CTRL		(RTL8712_FIFOCTRL_ + 0xB0)
#define REG_TXQ_PGADD			(RTL8712_FIFOCTRL_ + 0xB3)
#define REG_TXFF_PG_NUM			(RTL8712_FIFOCTRL_ + 0xB4)

/* ----------------------------------------------------- */
/* 0x10250160h ~ 0x102501CFh Adaptive Control */
/* ----------------------------------------------------- */
#define REG_INIMCS_SEL			(RTL8712_RATECTRL_ + 0x00)
#define REG_INIRTSMCS_SEL		(RTL8712_RATECTRL_ + 0x20)
#define		RRSR_RSC_OFFSET			21
#define		RRSR_SHORT_OFFSET		23
#define		RRSR_1M				BIT(0)
#define		RRSR_2M				BIT(1)
#define		RRSR_5_5M			BIT(2)
#define		RRSR_11M			BIT(3)
#define		RRSR_6M				BIT(4)
#define		RRSR_9M				BIT(5)
#define		RRSR_12M			BIT(6)
#define		RRSR_18M			BIT(7)
#define		RRSR_24M			BIT(8)
#define		RRSR_36M			BIT(9)
#define		RRSR_48M			BIT(10)
#define		RRSR_54M			BIT(11)
#define		RRSR_MCS0			BIT(12)
#define		RRSR_MCS1			BIT(13)
#define		RRSR_MCS2			BIT(14)
#define		RRSR_MCS3			BIT(15)
#define		RRSR_MCS4			BIT(16)
#define		RRSR_MCS5			BIT(17)
#define		RRSR_MCS6			BIT(18)
#define		RRSR_MCS7			BIT(19)
#define		BRSR_AckShortPmb		BIT(23)
#define		RATR_1M				0x00000001
#define		RATR_2M				0x00000002
#define		RATR_55M			0x00000004
#define		RATR_11M			0x00000008
#define		RATR_6M				0x00000010
#define		RATR_9M				0x00000020
#define		RATR_12M			0x00000040
#define		RATR_18M			0x00000080
#define		RATR_24M			0x00000100
#define		RATR_36M			0x00000200
#define		RATR_48M			0x00000400
#define		RATR_54M			0x00000800
#define		RATR_MCS0			0x00001000
#define		RATR_MCS1			0x00002000
#define		RATR_MCS2			0x00004000
#define		RATR_MCS3			0x00008000
#define		RATR_MCS4			0x00010000
#define		RATR_MCS5			0x00020000
#define		RATR_MCS6			0x00040000
#define		RATR_MCS7			0x00080000
#define		RATR_MCS8			0x00100000
#define		RATR_MCS9			0x00200000
#define		RATR_MCS10			0x00400000
#define		RATR_MCS11			0x00800000
#define		RATR_MCS12			0x01000000
#define		RATR_MCS13			0x02000000
#define		RATR_MCS14			0x04000000
#define		RATR_MCS15			0x08000000
#define		RATE_ALL_CCK			(RATR_1M | RATR_2M | \
						RATR_55M | RATR_11M)
#define		RATE_ALL_OFDM_AG		(RATR_6M | RATR_9M | \
						RATR_12M | RATR_18M | \
						RATR_24M | RATR_36M | \
						RATR_48M | RATR_54M)
#define		RATE_ALL_OFDM_1SS		(RATR_MCS0 | RATR_MCS1 | \
						RATR_MCS2 | RATR_MCS3 | \
						RATR_MCS4 | RATR_MCS5 | \
						RATR_MCS6 | RATR_MCS7)
#define		RATE_ALL_OFDM_2SS		(RATR_MCS8 | RATR_MCS9 | \
						RATR_MCS10 | RATR_MCS11 | \
						RATR_MCS12 | RATR_MCS13 | \
						RATR_MCS14 | RATR_MCS15)
#define REG_RRSR			(RTL8712_RATECTRL_ + 0x21)
#define		RRSR_RSC_LOWSUBCHNL		0x00200000	// that's a bit weird, why not use BIT(28)?
#define		RRSR_RSC_UPSUBCHNL		0x00400000
#define		RRSR_RSC_BW_40M			0x00600000
#define		RRSR_SHORT			0x00800000

#define REG_ARFR0			(RTL8712_RATECTRL_ + 0x24)
#define REG_ARFR1			(RTL8712_RATECTRL_ + 0x28)
#define REG_ARFR2			(RTL8712_RATECTRL_ + 0x2C)
#define REG_ARFR3			(RTL8712_RATECTRL_ + 0x30)
#define REG_ARFR4			(RTL8712_RATECTRL_ + 0x34)
#define REG_ARFR5			(RTL8712_RATECTRL_ + 0x38)
#define REG_ARFR6			(RTL8712_RATECTRL_ + 0x3C)
#define REG_ARFR7			(RTL8712_RATECTRL_ + 0x40)
#define REG_AGGLEN_LMT_H		(RTL8712_RATECTRL_ + 0x47)
#define REG_AGGLEN_LMT_L		(RTL8712_RATECTRL_ + 0x48)
#define REG_DARFRC			(RTL8712_RATECTRL_ + 0x50)
#define REG_RARFRC			(RTL8712_RATECTRL_ + 0x58)
#define REG_MCS_TXAGC0			(RTL8712_RATECTRL_ + 0x60)
#define REG_MCS_TXAGC1			(RTL8712_RATECTRL_ + 0x61)
#define REG_MCS_TXAGC2			(RTL8712_RATECTRL_ + 0x62)
#define REG_MCS_TXAGC3			(RTL8712_RATECTRL_ + 0x63)
#define REG_MCS_TXAGC4			(RTL8712_RATECTRL_ + 0x64)
#define REG_MCS_TXAGC5			(RTL8712_RATECTRL_ + 0x65)
#define REG_MCS_TXAGC6			(RTL8712_RATECTRL_ + 0x66)
#define REG_MCS_TXAGC7			(RTL8712_RATECTRL_ + 0x67)
#define REG_CCK_TXAGC			(RTL8712_RATECTRL_ + 0x68)

/* ----------------------------------------------------- */
/* 0x102501D0h ~ 0x102501FFh EDCA Configuration */
/* ----------------------------------------------------- */
#define REG_EDCA_VO_PARAM		(RTL8712_EDCASETTING_ + 0x00)
#define REG_EDCA_VI_PARAM		(RTL8712_EDCASETTING_ + 0x04)
#define REG_EDCA_BE_PARAM		(RTL8712_EDCASETTING_ + 0x08)
#define REG_EDCA_BK_PARAM		(RTL8712_EDCASETTING_ + 0x0C)
#define REG_BCNTCFG			(RTL8712_EDCASETTING_ + 0x10)
#define REG_CWRR			(RTL8712_EDCASETTING_ + 0x12)
#define REG_ACMAVG			(RTL8712_EDCASETTING_ + 0x16)
#define REG_ACMHWCTRL			(RTL8712_EDCASETTING_ + 0x17)
#define		AcmHw_HwEn			BIT(0)
#define		AcmHw_BeqEn			BIT(1)
#define		AcmHw_ViqEn			BIT(2)
#define		AcmHw_VoqEn			BIT(3)
#define		AcmHw_BeqStatus			BIT(4)
#define		AcmHw_ViqStatus			BIT(5)
#define		AcmHw_VoqStatus			BIT(6)

#define REG_VO_ADMTIME			(RTL8712_EDCASETTING_ + 0x18)
#define REG_VI_ADMTIME			(RTL8712_EDCASETTING_ + 0x1C)
#define REG_BE_ADMTIME			(RTL8712_EDCASETTING_ + 0x20)
#define REG_RETRY_LIMIT			(RTL8712_EDCASETTING_ + 0x24)
#define		RETRY_LIMIT_SHORT_SHIFT		8
#define		RETRY_LIMIT_LONG_SHIFT		0

#define	REG_SG_RATE			(RTL8712_EDCASETTING_ + 0x26)

/* ----------------------------------------------------- */
/* 0x10250200h ~ 0x1025023Fh Wireless MAC */
/* ----------------------------------------------------- */
#define REG_NAVCTRL			(RTL8712_WMAC_ + 0x00)
#define		NAV_UPPER_EN			BIT(16)
#define		NAV_UPPER			0xFF00
#define		NAV_RTSRST			0xFF

#define REG_BWOPMODE			(RTL8712_WMAC_ + 0x03)
#define		BW_OPMODE_11J			BIT(0)
#define		BW_OPMODE_5G			BIT(1)
#define		BW_OPMODE_20MHZ			BIT(2)

#define REG_BACAMCMD			(RTL8712_WMAC_ + 0x04)
#define REG_BACAMCONTENT		(RTL8712_WMAC_ + 0x08)
#define REG_LBDLY			(RTL8712_WMAC_ + 0x10)
#define REG_FWDLY			(RTL8712_WMAC_ + 0x11)
#define REG_HWPC_RX_CTRL		(RTL8712_WMAC_ + 0x18)
#define REG_MQ				(RTL8712_WMAC_ + 0x20)
#define REG_MA				(RTL8712_WMAC_ + 0x22)
#define REG_MS				(RTL8712_WMAC_ + 0x24)
#define REG_CLM_RESULT			(RTL8712_WMAC_ + 0x27)
#define REG_NHM_RPI_CNT			(RTL8712_WMAC_ + 0x28)
#define REG_RXERR_RPT			(RTL8712_WMAC_ + 0x30)
#define REG_NAV_PROT_LEN		(RTL8712_WMAC_ + 0x34)
#define REG_CFEND_TH			(RTL8712_WMAC_ + 0x36)
#define REG_AMPDU_MIN_SPACE		(RTL8712_WMAC_ + 0x37)
#define		MAX_MSS_OFFSET			3
#define		MAX_MSS_DENSITY_2T		0x13
#define		MAX_MSS_DENSITY_1T		0x0A
#define	REG_TXOP_STALL_CTRL		(RTL8712_WMAC_ + 0x38)

/* ----------------------------------------------------- */
/* 0x10250240h ~ 0x1025025Fh CAM / Security Engine */
/* ----------------------------------------------------- */
#define	REG_RWCAM			(RTL8712_SECURITY_ + 0x00)
#define	REG_WCAMI			(RTL8712_SECURITY_ + 0x04)
#define	REG_RCAMO			(RTL8712_SECURITY_ + 0x08)
#define	REG_CAMDBG			(RTL8712_SECURITY_ + 0x0C)
#define	REG_SECR			(RTL8712_SECURITY_ + 0x10)
#define		SCR_TXUSEDK			BIT(0)
#define		SCR_RXUSEDK			BIT(1)
#define		SCR_TXENCENABLE			BIT(2)
#define		SCR_RXENCENABLE			BIT(3)
#define		SCR_SKBYA2			BIT(4)
#define		SCR_NOSKMC			BIT(5)

/* ----------------------------------------------------- */
/* 0x10250260h ~ 0x102502BFh Powersave Control */
/* ----------------------------------------------------- */
#define REG_WOWCTRL			(RTL8712_POWERSAVE_ + 0x00)
#define REG_PSSTATUS			(RTL8712_POWERSAVE_ + 0x01)
#define REG_PSSWITCH			(RTL8712_POWERSAVE_ + 0x02)
#define REG_MIMOPS_WAITPERIOD		(RTL8712_POWERSAVE_ + 0x03)
#define REG_LPNAV_CTRL			(RTL8712_POWERSAVE_ + 0x04)
#define REG_WFM0			(RTL8712_POWERSAVE_ + 0x10)
#define REG_WFM1			(RTL8712_POWERSAVE_ + 0x20)
#define REG_WFM2			(RTL8712_POWERSAVE_ + 0x30)
#define REG_WFM3			(RTL8712_POWERSAVE_ + 0x40)
#define REG_WFM4			(RTL8712_POWERSAVE_ + 0x50)
#define REG_WFM5			(RTL8712_POWERSAVE_ + 0x60)
#define REG_WFCRC			(RTL8712_POWERSAVE_ + 0x70)
#define REG_RPWM			(RTL8712_POWERSAVE_ + 0x7C)
#define REG_CPWM			(RTL8712_POWERSAVE_ + 0x7D)

/* ----------------------------------------------------- */
/* 0x102502C0h ~ 0x102502CFh Base Band Control */
/* ----------------------------------------------------- */
#define REG_RF_BB_CMD_ADDR		(RTL8712_BB_ + 0x00)
#define REG_RF_BB_CMD_DATA		(RTL8712_BB_ + 0x04)

/* ----------------------------------------------------- */
/* 0x102502D0h ~ 0x102502DFh Offload Control */
/* ----------------------------------------------------- */

/* ----------------------------------------------------- */
/* 0x102502E0h ~ 0x102502FFh GPIO */
/* ----------------------------------------------------- */
#define REG_PSTIMER			(RTL8712_GP_ + 0x00)
#define REG_TIMER1			(RTL8712_GP_ + 0x04)
#define REG_TIMER2			(RTL8712_GP_ + 0x08)
#define REG_GPIO_CTRL			(RTL8712_GP_ + 0x0C)
#define		HAL_8192S_HW_GPIO_OFF_BIT	BIT(3)
#define		HAL_8192S_HW_GPIO_WPS_BIT	BIT(4)

#define REG_GPIO_IO_SEL			(RTL8712_GP_ + 0x0E)
#define		GPIOSEL_PHYDBG			1
#define		GPIOSEL_BT			2
#define		GPIOSEL_WLANDBG			3
#define		GPIOSEL_GPIO_MASK		(~(BIT(0)|BIT(1)))

#define REG_GPIO_INTCTRL		(RTL8712_GP_ + 0x10)
#define REG_MAC_PINMUX_CTRL		(RTL8712_GP_ + 0x11)
#define		GPIOSEL_GPIO			0
#define		GPIOMUX_EN			BIT(3)

#define REG_LEDCFG			(RTL8712_GP_ + 0x12)
#define REG_PHY_REG_RPT			(RTL8712_GP_ + 0x13)
#define REG_PHY_REG_DATA		(RTL8712_GP_ + 0x14)
#define REG_EFUSE_CLK			(RTL8712_GP_ + 0x18)

/* ----------------------------------------------------- */
/* 0x10250300h ~ 0x1025030Fh Interrupt Controller */
/* ----------------------------------------------------- */
#define REG_HIMR			(RTL8712_INTERRUPT_ + 0x08)

/* ----------------------------------------------------- */
/* 0x10250310h ~ 0x1025035Fh Debug */
/* ----------------------------------------------------- */
#define REG_BIST			(RTL8712_DEBUGCTRL_ + 0x00)
#define REG_DBS				(RTL8712_DEBUGCTRL_ + 0x04)
#define REG_LMS				(RTL8712_DEBUGCTRL_ + 0x05)
#define REG_CPUINST			(RTL8712_DEBUGCTRL_ + 0x08)
#define REG_CPUCAUSE			(RTL8712_DEBUGCTRL_ + 0x0C)
#define REG_LBUS_ERR_ADDR		(RTL8712_DEBUGCTRL_ + 0x10)
#define REG_LBUS_ERR_CMD		(RTL8712_DEBUGCTRL_ + 0x14)
#define REG_LBUS_ERR_DATA_L		(RTL8712_DEBUGCTRL_ + 0x18)
#define REG_LBUS_ERR_DATA_H		(RTL8712_DEBUGCTRL_ + 0x1C)
#define REG_LBUS_EXCEPTION_ADDR		(RTL8712_DEBUGCTRL_ + 0x20)
#define REG_WDG_CTRL			(RTL8712_DEBUGCTRL_ + 0x24)
#define REG_INTMTU			(RTL8712_DEBUGCTRL_ + 0x28)
#define REG_INTM			(RTL8712_DEBUGCTRL_ + 0x2A)
#define REG_FDLOCKTURN0			(RTL8712_DEBUGCTRL_ + 0x2C)
#define REG_FDLOCKTURN1			(RTL8712_DEBUGCTRL_ + 0x2D)
#define REG_FDLOCKFLAG0			(RTL8712_DEBUGCTRL_ + 0x2E)
#define REG_FDLOCKFLAG1			(RTL8712_DEBUGCTRL_ + 0x2F)
#define REG_TRXPKTBUF_DBG_DATA		(RTL8712_DEBUGCTRL_ + 0x30)
#define REG_TRXPKTBUF_DBG_CTRL		(RTL8712_DEBUGCTRL_ + 0x38)
#define REG_DPLL_MON			(RTL8712_DEBUGCTRL_ + 0x3A)

/* ----------------------------------------------------- */
/* 0x10250310h ~ 0x1025035Fh IO Command Control */
/* ----------------------------------------------------- */
#define REG_IOCMD_CTRL			(RTL8712_IOCMD_ + 0x00)
#define REG_IOCMD_DATA			(RTL8712_IOCMD_ + 0x04)

/* ----------------------------------------------------- */
/* 0x10250800 ~ 0x10250DFF PHY */
/* ----------------------------------------------------- */
#define REG_RFPGA0_RFMOD		(RTL8712_PHY_ + 0x00)
#define REG_RFPGA0_TXINFO		(RTL8712_PHY_ + 0x04)
#define REG_RFPGA0_PSDFUNCTION		(RTL8712_PHY_ + 0x08)
#define REG_RFPGA0_TXGAINSTAGE		(RTL8712_PHY_ + 0x0C)
#define REG_RFPGA0_RFTIMING1		(RTL8712_PHY_ + 0x10)
#define REG_RFPGA0_RFTIMING2		(RTL8712_PHY_ + 0x14)
#define REG_RFPGA0_XA_HSSIPARAMETER1	(RTL8712_PHY_ + 0x20)
#define REG_RFPGA0_XA_HSSIPARAMETER2	(RTL8712_PHY_ + 0x24)
#define REG_RFPGA0_XB_HSSIPARAMETER1	(RTL8712_PHY_ + 0x28)
#define REG_RFPGA0_XB_HSSIPARAMETER2	(RTL8712_PHY_ + 0x2C)
#define REG_RFPGA0_XC_HSSIPARAMETER1	(RTL8712_PHY_ + 0x30)
#define REG_RFPGA0_XC_HSSIPARAMETER2	(RTL8712_PHY_ + 0x34)
#define REG_RFPGA0_XD_HSSIPARAMETER1	(RTL8712_PHY_ + 0x38)
#define REG_RFPGA0_XD_HSSIPARAMETER2	(RTL8712_PHY_ + 0x3C)
#define REG_RFPGA0_XA_LSSIPARAMETER	(RTL8712_PHY_ + 0x40)
#define REG_RFPGA0_XB_LSSIPARAMETER	(RTL8712_PHY_ + 0x44)
#define REG_RFPGA0_XC_LSSIPARAMETER	(RTL8712_PHY_ + 0x48)
#define REG_RFPGA0_XD_LSSIPARAMETER	(RTL8712_PHY_ + 0x4C)
#define REG_RFPGA0_RFWAKEUP_PARAMETER	(RTL8712_PHY_ + 0x50)
#define REG_RFPGA0_RFSLEEPUP_PARAMETER	(RTL8712_PHY_ + 0x54)
#define REG_RFPGA0_XAB_SWITCHCONTROL	(RTL8712_PHY_ + 0x58)
#define REG_RFPGA0_XCD_SWITCHCONTROL	(RTL8712_PHY_ + 0x5C)
#define REG_RFPGA0_XA_RFINTERFACEOE	(RTL8712_PHY_ + 0x60)
#define REG_RFPGA0_XB_RFINTERFACEOE	(RTL8712_PHY_ + 0x64)
#define REG_RFPGA0_XC_RFINTERFACEOE	(RTL8712_PHY_ + 0x68)
#define REG_RFPGA0_XD_RFINTERFACEOE	(RTL8712_PHY_ + 0x6C)
#define REG_RFPGA0_XAB_RFINTERFACESW	(RTL8712_PHY_ + 0x70)
#define REG_RFPGA0_XCD_RFINTERFACESW	(RTL8712_PHY_ + 0x74)
#define REG_RFPGA0_XAB_RFPARAMETER	(RTL8712_PHY_ + 0x78)
#define REG_RFPGA0_XCD_RFPARAMETER	(RTL8712_PHY_ + 0x7C)
#define REG_RFPGA0_ANALOGPARAMETER1	(RTL8712_PHY_ + 0x80)
#define REG_RFPGA0_ANALOGPARAMETER2	(RTL8712_PHY_ + 0x84)
#define REG_RFPGA0_ANALOGPARAMETER3	(RTL8712_PHY_ + 0x88)
#define REG_RFPGA0_ANALOGPARAMETER4	(RTL8712_PHY_ + 0x8C)
#define REG_RFPGA0_XA_LSSIREADBACK	(RTL8712_PHY_ + 0xa0)
#define REG_RFPGA0_XB_LSSIREADBACK	(RTL8712_PHY_ + 0xA4)
#define REG_RFPGA0_XC_LSSIREADBACK	(RTL8712_PHY_ + 0xA8)
#define REG_RFPGA0_XD_LSSIREADBACK	(RTL8712_PHY_ + 0xAC)
#define REG_RFPGA0_PSDREPORT		(RTL8712_PHY_ + 0xB4)
#define REG_TRANSCEIVERA_HSPI_READBACK	(RTL8712_PHY_ + 0xB8)
#define REG_TRANSCEIVERB_HSPI_READBACK	(RTL8712_PHY_ + 0xBC)
#define REG_RFPGA0_XAB_RFINTERFACERB	(RTL8712_PHY_ + 0xE0)
#define REG_RFPGA0_XCD_RFINTERFACERB	(RTL8712_PHY_ + 0xE4)

/* ----------------------------------------------------- */
/* 0x10250900 ~ 0x102509FF PHY Page 9*/
/* ----------------------------------------------------- */
#define REG_RFPGA1_RFMOD		(RTL8712_PHY_P1_ + 0x00)
#define REG_RFPGA1_TXBLOCK		(RTL8712_PHY_P1_ + 0x04)
#define REG_RFPGA1_DEBUGSELECT		(RTL8712_PHY_P1_ + 0x08)
#define REG_RFPGA1_TXINFO		(RTL8712_PHY_P1_ + 0x0C)

/* ----------------------------------------------------- */
/* 0x10250A00 ~ 0x10250AFF PHY CCK */
/* ----------------------------------------------------- */
#define REG_RCCK0_SYSTEM		(RTL8712_PHY_CCK_ + 0x00)
#define REG_RCCK0_AFESETTING		(RTL8712_PHY_CCK_ + 0x04)
#define REG_RCCK0_CCA			(RTL8712_PHY_CCK_ + 0x08)
#define REG_RCCK0_RXAGC1		(RTL8712_PHY_CCK_ + 0x0C)
#define REG_RCCK0_RXAGC2		(RTL8712_PHY_CCK_ + 0x10)
#define REG_RCCK0_RXHP			(RTL8712_PHY_CCK_ + 0x14)
#define REG_RCCK0_DSPPARAMETER1		(RTL8712_PHY_CCK_ + 0x18)
#define REG_RCCK0_DSPPARAMETER2		(RTL8712_PHY_CCK_ + 0x1C)
#define REG_RCCK0_TXFILTER1		(RTL8712_PHY_CCK_ + 0x20)
#define REG_RCCK0_TXFILTER2		(RTL8712_PHY_CCK_ + 0x24)
#define REG_RCCK0_DEBUGPORT		(RTL8712_PHY_CCK_ + 0x28)
#define REG_RCCK0_FALSEALARMREPORT	(RTL8712_PHY_CCK_ + 0x2C)
#define REG_RCCK0_TRSSIREPORT		(RTL8712_PHY_CCK_ + 0x50)
#define REG_RCCK0_RXREPORT		(RTL8712_PHY_CCK_ + 0x54)
#define REG_RCCK0_FACOUNTERLOWER	(RTL8712_PHY_CCK_ + 0x5C)
#define REG_RCCK0_FACOUNTERUPPER	(RTL8712_PHY_CCK_ + 0x58)

/* ----------------------------------------------------- */
/* 0x10250C00 ~ 0x10250DFF PHY OFDM */
/* ----------------------------------------------------- */
#define REG_ROFDM0_LSTF			(RTL8712_PHY_OFDM_ + 0x00)
#define REG_ROFDM0_TRXPATHENABLE	(RTL8712_PHY_OFDM_ + 0x04)
#define REG_ROFDM0_TRMUXPAR		(RTL8712_PHY_OFDM_ + 0x08)
#define REG_ROFDM0_TRSWISOLATION	(RTL8712_PHY_OFDM_ + 0x0C)
#define REG_ROFDM0_XARXAFE		(RTL8712_PHY_OFDM_ + 0x10)
#define REG_ROFDM0_XARXIQIMBALANCE	(RTL8712_PHY_OFDM_ + 0x14)
#define REG_ROFDM0_XBRXAFE		(RTL8712_PHY_OFDM_ + 0x18)
#define REG_ROFDM0_XBRXIQIMBALANCE	(RTL8712_PHY_OFDM_ + 0x1C)
#define REG_ROFDM0_XCRXAFE		(RTL8712_PHY_OFDM_ + 0x20)
#define REG_ROFDM0_XCRXIQIMBALANCE	(RTL8712_PHY_OFDM_ + 0x24)
#define REG_ROFDM0_XDRXAFE		(RTL8712_PHY_OFDM_ + 0x28)
#define REG_ROFDM0_XDRXIQIMBALANCE	(RTL8712_PHY_OFDM_ + 0x2C)
#define REG_ROFDM0_RXDETECTOR1		(RTL8712_PHY_OFDM_ + 0x30)
#define REG_ROFDM0_RXDETECTOR2		(RTL8712_PHY_OFDM_ + 0x34)
#define REG_ROFDM0_RXDETECTOR3		(RTL8712_PHY_OFDM_ + 0x38)
#define REG_ROFDM0_RXDETECTOR4		(RTL8712_PHY_OFDM_ + 0x3C)
#define REG_ROFDM0_RXDSP		(RTL8712_PHY_OFDM_ + 0x40)
#define REG_ROFDM0_CFO_AND_DAGC		(RTL8712_PHY_OFDM_ + 0x44)
#define REG_ROFDM0_CCADROP_THRESHOLD	(RTL8712_PHY_OFDM_ + 0x48)
#define REG_ROFDM0_ECCA_THRESHOLD	(RTL8712_PHY_OFDM_ + 0x4C)
#define REG_ROFDM0_XAAGCCORE1		(RTL8712_PHY_OFDM_ + 0x50)
#define REG_ROFDM0_XAAGCCORE2		(RTL8712_PHY_OFDM_ + 0x54)
#define REG_ROFDM0_XBAGCCORE1		(RTL8712_PHY_OFDM_ + 0x58)
#define REG_ROFDM0_XBAGCCORE2		(RTL8712_PHY_OFDM_ + 0x5C)
#define REG_ROFDM0_XCAGCCORE1		(RTL8712_PHY_OFDM_ + 0x60)
#define REG_ROFDM0_XCAGCCORE2		(RTL8712_PHY_OFDM_ + 0x64)
#define REG_ROFDM0_XDAGCCORE1		(RTL8712_PHY_OFDM_ + 0x68)
#define REG_ROFDM0_XDAGCCORE2		(RTL8712_PHY_OFDM_ + 0x6C)
#define REG_ROFDM0_AGCPARAMETER1	(RTL8712_PHY_OFDM_ + 0x70)
#define REG_ROFDM0_AGCPARAMETER2	(RTL8712_PHY_OFDM_ + 0x74)
#define REG_ROFDM0_AGCRSSITABLE		(RTL8712_PHY_OFDM_ + 0x78)
#define REG_ROFDM0_HTSTFAGC		(RTL8712_PHY_OFDM_ + 0x7C)
#define REG_ROFDM0_XATXIQIMBALANCE	(RTL8712_PHY_OFDM_ + 0x80)
#define REG_ROFDM0_XATXAFE		(RTL8712_PHY_OFDM_ + 0x84)
#define REG_ROFDM0_XBTXIQIMBALANCE	(RTL8712_PHY_OFDM_ + 0x88)
#define REG_ROFDM0_XBTXAFE		(RTL8712_PHY_OFDM_ + 0x8C)
#define REG_ROFDM0_XCTXIQIMBALANCE	(RTL8712_PHY_OFDM_ + 0x90)
#define REG_ROFDM0_XCTXAFE		(RTL8712_PHY_OFDM_ + 0x94)
#define REG_ROFDM0_XDTXIQIMBALANCE	(RTL8712_PHY_OFDM_ + 0x98)
#define REG_ROFDM0_XDTXAFE		(RTL8712_PHY_OFDM_ + 0x9C)
#define REG_ROFDM0_TXCOEFF1		(RTL8712_PHY_OFDM_ + 0xA4)
#define REG_ROFDM0_TXCOEFF2		(RTL8712_PHY_OFDM_ + 0xA8)
#define REG_ROFDM0_TXCOEFF3		(RTL8712_PHY_OFDM_ + 0xAC)
#define REG_ROFDM0_TXCOEFF4		(RTL8712_PHY_OFDM_ + 0xB0)
#define REG_ROFDM0_TXCOEFF5		(RTL8712_PHY_OFDM_ + 0xB4)
#define REG_ROFDM0_TXCOEFF6		(RTL8712_PHY_OFDM_ + 0xB8)
#define REG_ROFDM0_RXHP_PARAMETER	(RTL8712_PHY_OFDM_ + 0xe0)
#define REG_ROFDM0_TXPSEUDO_NOISE_WGT	(RTL8712_PHY_OFDM_ + 0xe4)
#define REG_ROFDM0_FRAME_SYNC		(RTL8712_PHY_OFDM_ + 0xf0)
#define REG_ROFDM0_DFSREPORT		(RTL8712_PHY_OFDM_ + 0xf4)

#define REG_ROFDM1_LSTF			(RTL8712_PHY_OFDM_ + 0x100)
#define REG_ROFDM1_TRXPATHENABLE	(RTL8712_PHY_OFDM_ + 0x104)
#define REG_ROFDM1_CFO			(RTL8712_PHY_OFDM_ + 0x108)

#define REG_ROFDM1_CSI1			(RTL8712_PHY_OFDM_ + 0x110)
#define REG_ROFDM1_SBD			(RTL8712_PHY_OFDM_ + 0x114)
#define REG_ROFDM1_CSI2			(RTL8712_PHY_OFDM_ + 0x118)

#define REG_ROFDM1_CFOTRACKING		(RTL8712_PHY_OFDM_ + 0x12C)

#define REG_ROFDM1_TRXMESAURE1		(RTL8712_PHY_OFDM_ + 0x134)

#define REG_ROFDM1_INTF_DET		(RTL8712_PHY_OFDM_ + 0x13C)

#define REG_ROFDM1_PSEUDO_NOISESTATEAB	(RTL8712_PHY_OFDM_ + 0x150)
#define REG_ROFDM1_PSEUDO_NOISESTATECD	(RTL8712_PHY_OFDM_ + 0x154)
#define REG_ROFDM1_RX_PSEUDO_NOISE_WGT	(RTL8712_PHY_OFDM_ + 0x158)

#define REG_ROFDM_PHYCOUNTER1		(RTL8712_PHY_OFDM_ + 0x1A0)
#define REG_ROFDM_PHYCOUNTER2		(RTL8712_PHY_OFDM_ + 0x1A4)
#define REG_ROFDM_PHYCOUNTER3		(RTL8712_PHY_OFDM_ + 0x1A8)
#define REG_ROFDM_SHORT_CFOAB		(RTL8712_PHY_OFDM_ + 0x1AC)
#define REG_ROFDM_SHORT_CFOCD		(RTL8712_PHY_OFDM_ + 0x1B0)
#define REG_ROFDM_LONG_CFOAB		(RTL8712_PHY_OFDM_ + 0x1B4)
#define REG_ROFDM_LONG_CFOCD		(RTL8712_PHY_OFDM_ + 0x1B8)
#define REG_ROFDM_TAIL_CFOAB		(RTL8712_PHY_OFDM_ + 0x1BC)
#define REG_ROFDM_TAIL_CFOCD		(RTL8712_PHY_OFDM_ + 0x1C0)
#define REG_ROFDM_PW_MEASURE1		(RTL8712_PHY_OFDM_ + 0x1C4)
#define REG_ROFDM_PW_MEASURE2		(RTL8712_PHY_OFDM_ + 0x1C8)
#define REG_ROFDM_BW_REPORT		(RTL8712_PHY_OFDM_ + 0x1CC)
#define REG_ROFDM_AGC_REPORT		(RTL8712_PHY_OFDM_ + 0x1D0)
#define REG_ROFDM_RXSNR			(RTL8712_PHY_OFDM_ + 0x1D4)
#define REG_ROFDM_RXEVMCSI		(RTL8712_PHY_OFDM_ + 0x1D8)
#define REG_ROFDM_SIG_REPORT		(RTL8712_PHY_OFDM_ + 0x1DC)

/* ----------------------------------------------------- */
/* 0x10250E00 ~ 0x10250EFF PHY Automatic Gain Control */
/* ----------------------------------------------------- */
#define REG_RTXAGC_RATE18_06		(RTL8712_PHY_RXAGC_ + 0x00)
#define REG_RTXAGC_RATE54_24		(RTL8712_PHY_RXAGC_ + 0x04)
#define REG_RTXAGC_CCK_MCS32		(RTL8712_PHY_RXAGC_ + 0x08)
#define REG_RTXAGC_MCS03_MCS00		(RTL8712_PHY_RXAGC_ + 0x10)
#define REG_RTXAGC_MCS07_MCS04		(RTL8712_PHY_RXAGC_ + 0x14)
#define REG_RTXAGC_MCS11_MCS08		(RTL8712_PHY_RXAGC_ + 0x18)
#define REG_RTXAGC_MCS15_MCS12		(RTL8712_PHY_RXAGC_ + 0x1c)

/* ----------------------------------------------------- */
/* 0x1025FE00 ~ 0x1025FEFF USB Configuration */
/* ----------------------------------------------------- */
#define REG_USB_INFO			(RTL8712_USB_ + 0x17)
#define REG_USB_SPECIAL_OPTION		(RTL8712_USB_ + 0x55)
#define REG_USB_HCPWM			(RTL8712_USB_ + 0x57)
#define REG_USB_HRPWM			(RTL8712_USB_ + 0x58)
#define REG_USB_DMA_AGG_TO		(RTL8712_USB_ + 0x5B)
#define REG_USB_AGG_TO			(RTL8712_USB_ + 0x5C)
#define REG_USB_AGG_TH			(RTL8712_USB_ + 0x5D)

#define REG_USB_VID			(RTL8712_USB_ + 0x60)
#define REG_USB_PID			(RTL8712_USB_ + 0x62)
#define REG_USB_OPTIONAL		(RTL8712_USB_ +	0x64)
#define REG_USB_CHIRP_K			(RTL8712_USB_ + 0x65) // REG_USB_EP
#define REG_USB_PHY			(RTL8712_USB_ + 0x66) // 0xFE68?
#define REG_USB_MAC_ADDR		(RTL8712_USB_ + 0x70)
#define REG_USB_STRING			(RTL8712_USB_ + 0x80)

/* ----------------------------------------------------- */
/* 0x10300000 ~ 0x103FFFFF FIFOs for 8712 */
/* ----------------------------------------------------- */
#define REG_RTL8712_DMA_BCNQ		(RTL8712_IOBASE_FF + 0x10000)
#define REG_RTL8712_DMA_MGTQ		(RTL8712_IOBASE_FF + 0x20000)
#define REG_RTL8712_DMA_BMCQ		(RTL8712_IOBASE_FF + 0x30000)
#define REG_RTL8712_DMA_VOQ		(RTL8712_IOBASE_FF + 0x40000)
#define REG_RTL8712_DMA_VIQ		(RTL8712_IOBASE_FF + 0x50000)
#define REG_RTL8712_DMA_BEQ		(RTL8712_IOBASE_FF + 0x60000)
#define REG_RTL8712_DMA_BKQ		(RTL8712_IOBASE_FF + 0x70000)
#define REG_RTL8712_DMA_RX0FF		(RTL8712_IOBASE_FF + 0x80000)
#define REG_RTL8712_DMA_H2CCMD		(RTL8712_IOBASE_FF + 0x90000)
#define REG_RTL8712_DMA_C2HCMD		(RTL8712_IOBASE_FF + 0xA0000)

#if 0

/* PHY */

/*--------------------------Define Parameters-------------------------------*/

/*============================================================
 *       8192S Regsiter offset definition
 *============================================================
 *
 *
 * BB-PHY register PMAC 0x100 PHY 0x800 - 0xEFF
 * 1. PMAC duplicate register due to connection: RF_Mode, TRxRN, NumOf L-STF
 * 2. 0x800/0x900/0xA00/0xC00/0xD00/0xE00
 * 3. RF register 0x00-2E
 * 4. Bit Mask for BB/RF register
 * 5. Other definition for BB/RF R/W
 *
 * 1. PMAC duplicate register due to connection: RF_Mode, TRxRN, NumOf L-STF
 * 1. Page1(0x100)
 */
#define	rPMAC_Reset			0x100
#define	rPMAC_TxStart			0x104
#define	rPMAC_TxLegacySIG		0x108
#define	rPMAC_TxHTSIG1			0x10c
#define	rPMAC_TxHTSIG2			0x110
#define	rPMAC_PHYDebug			0x114
#define	rPMAC_TxPacketNum		0x118
#define	rPMAC_TxIdle			0x11c
#define	rPMAC_TxMACHeader0		0x120
#define	rPMAC_TxMACHeader1		0x124
#define	rPMAC_TxMACHeader2		0x128
#define	rPMAC_TxMACHeader3		0x12c
#define	rPMAC_TxMACHeader4		0x130
#define	rPMAC_TxMACHeader5		0x134
#define	rPMAC_TxDataType		0x138
#define	rPMAC_TxRandomSeed		0x13c
#define	rPMAC_CCKPLCPPreamble		0x140
#define	rPMAC_CCKPLCPHeader		0x144
#define	rPMAC_CCKCRC16			0x148
#define	rPMAC_OFDMRxCRC32OK		0x170
#define	rPMAC_OFDMRxCRC32Er		0x174
#define	rPMAC_OFDMRxParityEr		0x178
#define	rPMAC_OFDMRxCRC8Er		0x17c
#define	rPMAC_CCKCRxRC16Er		0x180
#define	rPMAC_CCKCRxRC32Er		0x184
#define	rPMAC_CCKCRxRC32OK		0x188
#define	rPMAC_TxStatus			0x18c

/*
 * 2. Page2(0x200)
 *
 * The following two definition are only used for USB interface.
 *#define RF_BB_CMD_ADDR	0x02c0	// RF/BB read/write command address.
 *#define RF_BB_CMD_DATA	0x02c4	// RF/BB read/write command data.
 *
 *
 * 3. Page8(0x800)
 */
#define	rFPGA0_RFMOD			0x800	/*RF mode & CCK TxSC RF
						 * BW Setting?? */
#define	rFPGA0_TxInfo			0x804	/* Status report?? */
#define	rFPGA0_PSDFunction		0x808
#define	rFPGA0_TxGainStage		0x80c	/* Set TX PWR init gain? */
#define	rFPGA0_RFTiming1		0x810	/* Useless now */
#define	rFPGA0_RFTiming2		0x814
#define	rFPGA0_XA_HSSIParameter1	0x820	/* RF 3 wire register */
#define	rFPGA0_XA_HSSIParameter2	0x824
#define	rFPGA0_XB_HSSIParameter1	0x828
#define	rFPGA0_XB_HSSIParameter2	0x82c
#define	rFPGA0_XC_HSSIParameter1	0x830
#define	rFPGA0_XC_HSSIParameter2	0x834
#define	rFPGA0_XD_HSSIParameter1	0x838
#define	rFPGA0_XD_HSSIParameter2	0x83c
#define	rFPGA0_XA_LSSIParameter		0x840
#define	rFPGA0_XB_LSSIParameter		0x844
#define	rFPGA0_XC_LSSIParameter		0x848
#define	rFPGA0_XD_LSSIParameter		0x84c

#define	rFPGA0_RFWakeUpParameter	0x850	/* Useless now */
#define	rFPGA0_RFSleepUpParameter	0x854

#define	rFPGA0_XAB_SwitchControl	0x858	/* RF Channel switch */
#define	rFPGA0_XCD_SwitchControl	0x85c

#define	rFPGA0_XA_RFInterfaceOE		0x860	/* RF Channel switch */
#define	rFPGA0_XB_RFInterfaceOE		0x864
#define	rFPGA0_XC_RFInterfaceOE		0x868
#define	rFPGA0_XD_RFInterfaceOE		0x86c
#define	rFPGA0_XAB_RFInterfaceSW	0x870	/* RF Interface Software Ctrl */
#define	rFPGA0_XCD_RFInterfaceSW	0x874

#define	rFPGA0_XAB_RFParameter		0x878	/* RF Parameter */
#define	rFPGA0_XCD_RFParameter		0x87c

#define	rFPGA0_AnalogParameter1		0x880	/* Crystal cap setting
						 * RF-R/W protection
						 * for parameter4?? */
#define	rFPGA0_AnalogParameter2		0x884
#define	rFPGA0_AnalogParameter3		0x888	/* Useless now */
#define	rFPGA0_AnalogParameter4		0x88c

#define	rFPGA0_XA_LSSIReadBack		0x8a0	/* Tranceiver LSSI Readback */
#define	rFPGA0_XB_LSSIReadBack		0x8a4
#define	rFPGA0_XC_LSSIReadBack		0x8a8
#define	rFPGA0_XD_LSSIReadBack		0x8ac

#define	rFPGA0_PSDReport		0x8b4	/* Useless now */
#define	rFPGA0_XAB_RFInterfaceRB	0x8e0	/* Useless now */
#define	rFPGA0_XCD_RFInterfaceRB	0x8e4	/* Useless now */

/*
 * 4. Page9(0x900)
 */
#define	rFPGA1_RFMOD			0x900	/* RF mode & OFDM TxSC */

#define	rFPGA1_TxBlock			0x904	/* Useless now */
#define	rFPGA1_DebugSelect		0x908	/* Useless now */
#define	rFPGA1_TxInfo			0x90c	/* Useless now */

/*
 * 5. PageA(0xA00)
 *
 * Set Control channel to upper or lower.
 * These settings are required only for 40MHz */
#define	rCCK0_System			0xa00

#define	rCCK0_AFESetting		0xa04	/* Disable init gain now */
#define	rCCK0_CCA			0xa08	/* Disable init gain now */

#define	rCCK0_RxAGC1			0xa0c
/* AGC default value, saturation level
 * Antenna Diversity, RX AGC, LNA Threshold, RX LNA Threshold useless now.
 * Not the same as 90 series */
#define	rCCK0_RxAGC2			0xa10	/* AGC & DAGC */

#define	rCCK0_RxHP			0xa14

#define	rCCK0_DSPParameter1		0xa18	/* Timing recovery & Channel
						 * estimation threshold */
#define	rCCK0_DSPParameter2		0xa1c	/* SQ threshold */

#define	rCCK0_TxFilter1			0xa20
#define	rCCK0_TxFilter2			0xa24
#define	rCCK0_DebugPort			0xa28	/* debug port and Tx filter3 */
#define	rCCK0_FalseAlarmReport		0xa2c	/* 0xa2d useless now 0xa30-a4f
						 * channel report */
#define	rCCK0_TRSSIReport		0xa50
#define	rCCK0_RxReport			0xa54   /* 0xa57 */
#define	rCCK0_FACounterLower		0xa5c   /* 0xa5b */
#define	rCCK0_FACounterUpper		0xa58   /* 0xa5c */

/*
 * 6. PageC(0xC00)
 */
#define	rOFDM0_LSTF			0xc00
#define	rOFDM0_TRxPathEnable		0xc04
#define	rOFDM0_TRMuxPar			0xc08
#define	rOFDM0_TRSWIsolation		0xc0c

/*RxIQ DC offset, Rx digital filter, DC notch filter */
#define	rOFDM0_XARxAFE			0xc10
#define	rOFDM0_XARxIQImbalance		0xc14  /* RxIQ imbalance matrix */
#define	rOFDM0_XBRxAFE			0xc18
#define	rOFDM0_XBRxIQImbalance		0xc1c
#define	rOFDM0_XCRxAFE			0xc20
#define	rOFDM0_XCRxIQImbalance		0xc24
#define	rOFDM0_XDRxAFE			0xc28
#define	rOFDM0_XDRxIQImbalance		0xc2c

#define	rOFDM0_RxDetector1		0xc30  /* PD,BW & SBD DM tune
						* init gain */
#define	rOFDM0_RxDetector2		0xc34  /* SBD & Fame Sync. */
#define	rOFDM0_RxDetector3		0xc38  /* Frame Sync. */
#define	rOFDM0_RxDetector4		0xc3c  /* PD, SBD, Frame Sync &
						* Short-GI */

#define	rOFDM0_RxDSP			0xc40  /* Rx Sync Path */
#define	rOFDM0_CFOandDAGC		0xc44  /* CFO & DAGC */
#define	rOFDM0_CCADropThreshold		0xc48 /* CCA Drop threshold */
#define	rOFDM0_ECCAThreshold		0xc4c /* energy CCA */

#define	rOFDM0_XAAGCCore1		0xc50	/* DIG */
#define	rOFDM0_XAAGCCore2		0xc54
#define	rOFDM0_XBAGCCore1		0xc58
#define	rOFDM0_XBAGCCore2		0xc5c
#define	rOFDM0_XCAGCCore1		0xc60
#define	rOFDM0_XCAGCCore2		0xc64
#define	rOFDM0_XDAGCCore1		0xc68
#define	rOFDM0_XDAGCCore2		0xc6c
#define	rOFDM0_AGCParameter1		0xc70
#define	rOFDM0_AGCParameter2		0xc74
#define	rOFDM0_AGCRSSITable		0xc78
#define	rOFDM0_HTSTFAGC			0xc7c

#define	rOFDM0_XATxIQImbalance		0xc80	/* TX PWR TRACK and DIG */
#define	rOFDM0_XATxAFE			0xc84
#define	rOFDM0_XBTxIQImbalance		0xc88
#define	rOFDM0_XBTxAFE			0xc8c
#define	rOFDM0_XCTxIQImbalance		0xc90
#define	rOFDM0_XCTxAFE			0xc94
#define	rOFDM0_XDTxIQImbalance		0xc98
#define	rOFDM0_XDTxAFE			0xc9c

#define	rOFDM0_RxHPParameter		0xce0
#define	rOFDM0_TxPseudoNoiseWgt		0xce4
#define	rOFDM0_FrameSync		0xcf0
#define	rOFDM0_DFSReport		0xcf4
#define	rOFDM0_TxCoeff1			0xca4
#define	rOFDM0_TxCoeff2			0xca8
#define	rOFDM0_TxCoeff3			0xcac
#define	rOFDM0_TxCoeff4			0xcb0
#define	rOFDM0_TxCoeff5			0xcb4
#define	rOFDM0_TxCoeff6			0xcb8

/*
 * 7. PageD(0xD00)
 */
#define	rOFDM1_LSTF			0xd00
#define	rOFDM1_TRxPathEnable		0xd04

#define	rOFDM1_CFO			0xd08	/* No setting now */
#define	rOFDM1_CSI1			0xd10
#define	rOFDM1_SBD			0xd14
#define	rOFDM1_CSI2			0xd18
#define	rOFDM1_CFOTracking		0xd2c
#define	rOFDM1_TRxMesaure1		0xd34
#define	rOFDM1_IntfDet			0xd3c
#define	rOFDM1_PseudoNoiseStateAB	0xd50
#define	rOFDM1_PseudoNoiseStateCD	0xd54
#define	rOFDM1_RxPseudoNoiseWgt		0xd58

#define	rOFDM_PHYCounter1		0xda0  /* cca, parity fail */
#define	rOFDM_PHYCounter2		0xda4  /* rate illegal, crc8 fail */
#define	rOFDM_PHYCounter3		0xda8  /* MCS not support */
#define	rOFDM_ShortCFOAB		0xdac  /* No setting now */
#define	rOFDM_ShortCFOCD		0xdb0
#define	rOFDM_LongCFOAB			0xdb4
#define	rOFDM_LongCFOCD			0xdb8
#define	rOFDM_TailCFOAB			0xdbc
#define	rOFDM_TailCFOCD			0xdc0
#define	rOFDM_PWMeasure1		0xdc4
#define	rOFDM_PWMeasure2		0xdc8
#define	rOFDM_BWReport			0xdcc
#define	rOFDM_AGCReport			0xdd0
#define	rOFDM_RxSNR			0xdd4
#define	rOFDM_RxEVMCSI			0xdd8
#define	rOFDM_SIGReport			0xddc

/*
 * 8. PageE(0xE00)
 */
#define	rTxAGC_Rate18_06		0xe00
#define	rTxAGC_Rate54_24		0xe04
#define	rTxAGC_CCK_Mcs32		0xe08
#define	rTxAGC_Mcs03_Mcs00		0xe10
#define	rTxAGC_Mcs07_Mcs04		0xe14
#define	rTxAGC_Mcs11_Mcs08		0xe18
#define	rTxAGC_Mcs15_Mcs12		0xe1c

/* Analog- control in RX_WAIT_CCA : REG: EE0
 * [Analog- Power & Control Register] */
#define		rRx_Wait_CCCA		0xe70
#define	rAnapar_Ctrl_BB			0xee0

/*
 * 7. RF Register 0x00-0x2E (RF 8256)
 *    RF-0222D 0x00-3F
 *
 * Zebra1
 */
#define	rZebra1_HSSIEnable		0x0	/* Useless now */
#define	rZebra1_TRxEnable1		0x1
#define	rZebra1_TRxEnable2		0x2
#define	rZebra1_AGC			0x4
#define	rZebra1_ChargePump		0x5
#define	rZebra1_Channel			0x7	/* RF channel switch */
#define	rZebra1_TxGain			0x8	/* Useless now */
#define	rZebra1_TxLPF			0x9
#define	rZebra1_RxLPF			0xb
#define	rZebra1_RxHPFCorner		0xc

/* Zebra4 */
#define	rGlobalCtrl			0	/* Useless now */
#define	rRTL8256_TxLPF			19
#define	rRTL8256_RxLPF			11

/* RTL8258 */
#define	rRTL8258_TxLPF			0x11	/* Useless now */
#define	rRTL8258_RxLPF			0x13
#define	rRTL8258_RSSILPF		0xa

/* RL6052 Register definition */
#define	RF_AC				0x00
#define	RF_IQADJ_G1			0x01
#define	RF_IQADJ_G2			0x02
#define	RF_POW_TRSW			0x05

#define	RF_GAIN_RX			0x06
#define	RF_GAIN_TX			0x07

#define	RF_TXM_IDAC			0x08
#define	RF_BS_IQGEN			0x0F

#define	RF_MODE1			0x10
#define	RF_MODE2			0x11

#define	RF_RX_AGC_HP			0x12
#define	RF_TX_AGC			0x13
#define	RF_BIAS				0x14
#define	RF_IPA				0x15
#define	RF_POW_ABILITY			0x17
#define	RF_MODE_AG			0x18
#define	rRfChannel			0x18	/* RF channel and BW switch */
#define	RF_CHNLBW			0x18	/* RF channel and BW switch */
#define	RF_TOP				0x19
#define	RF_RX_G1			0x1A
#define	RF_RX_G2			0x1B
#define	RF_RX_BB2			0x1C
#define	RF_RX_BB1			0x1D

#define	RF_RCK1				0x1E
#define	RF_RCK2				0x1F

#define	RF_TX_G1			0x20
#define	RF_TX_G2			0x21
#define	RF_TX_G3			0x22

#define	RF_TX_BB1			0x23
#define	RF_T_METER			0x24

#define	RF_SYN_G1			0x25	/* RF TX Power control */
#define	RF_SYN_G2			0x26	/* RF TX Power control */
#define	RF_SYN_G3			0x27	/* RF TX Power control */
#define	RF_SYN_G4			0x28	/* RF TX Power control */
#define	RF_SYN_G5			0x29	/* RF TX Power control */
#define	RF_SYN_G6			0x2A	/* RF TX Power control */
#define	RF_SYN_G7			0x2B	/* RF TX Power control */
#define	RF_SYN_G8			0x2C	/* RF TX Power control */

#define	RF_RCK_OS			0x30	/* RF TX PA control */

#define	RF_TXPA_G1			0x31	/* RF TX PA control */
#define	RF_TXPA_G2			0x32	/* RF TX PA control */
#define	RF_TXPA_G3			0x33	/* RF TX PA control */

/*
 * Bit Mask
 *
 * 1. Page1(0x100) */
#define	bBBResetB			0x100	/* Useless now? */
#define	bGlobalResetB			0x200
#define	bOFDMTxStart			0x4
#define	bCCKTxStart			0x8
#define	bCRC32Debug			0x100
#define	bPMACLoopback			0x10
#define	bTxLSIG				0xffffff
#define	bOFDMTxRate			0xf
#define	bOFDMTxReserved			0x10
#define	bOFDMTxLength			0x1ffe0
#define	bOFDMTxParity			0x20000
#define	bTxHTSIG1			0xffffff
#define	bTxHTMCSRate			0x7f
#define	bTxHTBW				0x80
#define	bTxHTLength			0xffff00
#define	bTxHTSIG2			0xffffff
#define	bTxHTSmoothing			0x1
#define	bTxHTSounding			0x2
#define	bTxHTReserved			0x4
#define	bTxHTAggreation			0x8
#define	bTxHTSTBC			0x30
#define	bTxHTAdvanceCoding		0x40
#define	bTxHTShortGI			0x80
#define	bTxHTNumberHT_LTF		0x300
#define	bTxHTCRC8			0x3fc00
#define	bCounterReset			0x10000
#define	bNumOfOFDMTx			0xffff
#define	bNumOfCCKTx			0xffff0000
#define	bTxIdleInterval			0xffff
#define	bOFDMService			0xffff0000
#define	bTxMACHeader			0xffffffff
#define	bTxDataInit			0xff
#define	bTxHTMode			0x100
#define	bTxDataType			0x30000
#define	bTxRandomSeed			0xffffffff
#define	bCCKTxPreamble			0x1
#define	bCCKTxSFD			0xffff0000
#define	bCCKTxSIG			0xff
#define	bCCKTxService			0xff00
#define	bCCKLengthExt			0x8000
#define	bCCKTxLength			0xffff0000
#define	bCCKTxCRC16			0xffff
#define	bCCKTxStatus			0x1
#define	bOFDMTxStatus			0x2
#define IS_BB_REG_OFFSET_92S(_Offset)	((_Offset >= 0x800) && \
					(_Offset <= 0xfff))

/* 2. Page8(0x800) */
#define	bRFMOD			0x1	/* Reg 0x800 rFPGA0_RFMOD */
#define	bJapanMode		0x2
#define	bCCKTxSC		0x30
#define	bCCKEn			0x1000000
#define	bOFDMEn			0x2000000

#define	bOFDMRxADCPhase         0x10000	/* Useless now */
#define	bOFDMTxDACPhase         0x40000
#define	bXATxAGC                0x3f
#define	bXBTxAGC                0xf00	/* Reg 80c rFPGA0_TxGainStage */
#define	bXCTxAGC                0xf000
#define	bXDTxAGC                0xf0000

#define	bPAStart		0xf0000000	/* Useless now */
#define	bTRStart		0x00f00000
#define	bRFStart		0x0000f000
#define	bBBStart		0x000000f0
#define	bBBCCKStart		0x0000000f
#define	bPAEnd			0xf          /* Reg0x814 */
#define	bTREnd			0x0f000000
#define	bRFEnd			0x000f0000
#define	bCCAMask		0x000000f0   /* T2R */
#define	bR2RCCAMask		0x00000f00
#define	bHSSI_R2TDelay		0xf8000000
#define	bHSSI_T2RDelay		0xf80000
#define	bContTxHSSI		0x400     /* change gain at continue Tx */
#define	bIGFromCCK		0x200
#define	bAGCAddress		0x3f
#define	bRxHPTx			0x7000
#define	bRxHPT2R		0x38000
#define	bRxHPCCKIni		0xc0000
#define	bAGCTxCode		0xc00000
#define	bAGCRxCode		0x300000
#define	b3WireDataLength	0x800	/* Reg 0x820~84f rFPGA0_XA_HSSIParm1 */
#define	b3WireAddressLength	0x400
#define	b3WireRFPowerDown	0x1	/* Useless now */
#define	b5GPAPEPolarity		0x40000000
#define	b2GPAPEPolarity		0x80000000
#define	bRFSW_TxDefaultAnt	0x3
#define	bRFSW_TxOptionAnt	0x30
#define	bRFSW_RxDefaultAnt	0x300
#define	bRFSW_RxOptionAnt	0x3000
#define	bRFSI_3WireData		0x1
#define	bRFSI_3WireClock	0x2
#define	bRFSI_3WireLoad		0x4
#define	bRFSI_3WireRW		0x8
#define	bRFSI_3Wire		0xf
#define	bRFSI_RFENV		0x10	/* Reg 0x870 rFPGA0_XAB_RFInterfaceSW */
#define	bRFSI_TRSW		0x20	/* Useless now */
#define	bRFSI_TRSWB		0x40
#define	bRFSI_ANTSW		0x100
#define	bRFSI_ANTSWB		0x200
#define	bRFSI_PAPE		0x400
#define	bRFSI_PAPE5G		0x800
#define	bBandSelect		0x1
#define	bHTSIG2_GI		0x80
#define	bHTSIG2_Smoothing	0x01
#define	bHTSIG2_Sounding	0x02
#define	bHTSIG2_Aggreaton	0x08
#define	bHTSIG2_STBC		0x30
#define	bHTSIG2_AdvCoding	0x40
#define	bHTSIG2_NumOfHTLTF	0x300
#define	bHTSIG2_CRC8		0x3fc
#define	bHTSIG1_MCS		0x7f
#define	bHTSIG1_BandWidth	0x80
#define	bHTSIG1_HTLength	0xffff
#define	bLSIG_Rate		0xf
#define	bLSIG_Reserved		0x10
#define	bLSIG_Length		0x1fffe
#define	bLSIG_Parity		0x20
#define	bCCKRxPhase		0x4
#define	bLSSIReadAddress	0x7f800000   /* T65 RF */
#define	bLSSIReadEdge		0x80000000   /* LSSI "Read" edge signal */
#define	bLSSIReadBackData	0xfffff		/* T65 RF */
#define	bLSSIReadOKFlag		0x1000	/* Useless now */
#define	bCCKSampleRate		0x8       /*0: 44MHz, 1:88MHz*/
#define	bRegulator0Standby	0x1
#define	bRegulatorPLLStandby	0x2
#define	bRegulator1Standby	0x4
#define	bPLLPowerUp		0x8
#define	bDPLLPowerUp		0x10
#define	bDA10PowerUp		0x20
#define	bAD7PowerUp		0x200
#define	bDA6PowerUp		0x2000
#define	bXtalPowerUp		0x4000
#define	b40MDClkPowerUP		0x8000
#define	bDA6DebugMode		0x20000
#define	bDA6Swing		0x380000

/* Reg 0x880 rFPGA0_AnalogParameter1 20/40 CCK support switch 40/80 BB MHZ */
#define	bADClkPhase		0x4000000

#define	b80MClkDelay		0x18000000	/* Useless */
#define	bAFEWatchDogEnable	0x20000000

/* Reg 0x884 rFPGA0_AnalogParameter2 Crystal cap */
#define	bXtalCap01		0xc0000000
#define	bXtalCap23		0x3
#define	bXtalCap92x		0x0f000000
#define bXtalCap		0x0f000000
#define	bIntDifClkEnable	0x400	/* Useless */
#define	bExtSigClkEnable	0x800
#define	bBandgapMbiasPowerUp	0x10000
#define	bAD11SHGain		0xc0000
#define	bAD11InputRange		0x700000
#define	bAD11OPCurrent		0x3800000
#define	bIPathLoopback		0x4000000
#define	bQPathLoopback		0x8000000
#define	bAFELoopback		0x10000000
#define	bDA10Swing		0x7e0
#define	bDA10Reverse		0x800
#define	bDAClkSource		0x1000
#define	bAD7InputRange		0x6000
#define	bAD7Gain		0x38000
#define	bAD7OutputCMMode	0x40000
#define	bAD7InputCMMode		0x380000
#define	bAD7Current		0xc00000
#define	bRegulatorAdjust	0x7000000
#define	bAD11PowerUpAtTx	0x1
#define	bDA10PSAtTx		0x10
#define	bAD11PowerUpAtRx	0x100
#define	bDA10PSAtRx		0x1000
#define	bCCKRxAGCFormat		0x200
#define	bPSDFFTSamplepPoint	0xc000
#define	bPSDAverageNum		0x3000
#define	bIQPathControl		0xc00
#define	bPSDFreq		0x3ff
#define	bPSDAntennaPath		0x30
#define	bPSDIQSwitch		0x40
#define	bPSDRxTrigger		0x400000
#define	bPSDTxTrigger		0x80000000
#define	bPSDSineToneScale	0x7f000000
#define	bPSDReport		0xffff

/* 3. Page9(0x900) */
#define	bOFDMTxSC		0x30000000	/* Useless */
#define	bCCKTxOn		0x1
#define	bOFDMTxOn		0x2
#define	bDebugPage		0xfff  /* reset debug page and HWord, LWord */
#define	bDebugItem		0xff   /* reset debug page and LWord */
#define	bAntL			0x10
#define	bAntNonHT		0x100
#define	bAntHT1			0x1000
#define	bAntHT2			0x10000
#define	bAntHT1S1		0x100000
#define	bAntNonHTS1		0x1000000

/* 4. PageA(0xA00) */
#define	bCCKBBMode		0x3	/* Useless */
#define	bCCKTxPowerSaving	0x80
#define	bCCKRxPowerSaving	0x40

#define	bCCKSideBand		0x10	/* Reg 0xa00 rCCK0_System 20/40 switch*/
#define	bCCKScramble		0x8	/* Useless */
#define	bCCKAntDiversity	0x8000
#define	bCCKCarrierRecovery	0x4000
#define	bCCKTxRate		0x3000
#define	bCCKDCCancel		0x0800
#define	bCCKISICancel		0x0400
#define	bCCKMatchFilter		0x0200
#define	bCCKEqualizer		0x0100
#define	bCCKPreambleDetect	0x800000
#define	bCCKFastFalseCCA	0x400000
#define	bCCKChEstStart		0x300000
#define	bCCKCCACount		0x080000
#define	bCCKcs_lim		0x070000
#define	bCCKBistMode		0x80000000
#define	bCCKCCAMask		0x40000000
#define	bCCKTxDACPhase		0x4
#define	bCCKRxADCPhase		0x20000000   /* r_rx_clk */
#define	bCCKr_cp_mode0		0x0100
#define	bCCKTxDCOffset		0xf0
#define	bCCKRxDCOffset		0xf
#define	bCCKCCAMode		0xc000
#define	bCCKFalseCS_lim		0x3f00
#define	bCCKCS_ratio		0xc00000
#define	bCCKCorgBit_sel		0x300000
#define	bCCKPD_lim		0x0f0000
#define	bCCKNewCCA		0x80000000
#define	bCCKRxHPofIG		0x8000
#define	bCCKRxIG		0x7f00
#define	bCCKLNAPolarity		0x800000
#define	bCCKRx1stGain		0x7f0000
#define	bCCKRFExtend		0x20000000 /* CCK Rx inital gain polarity */
#define	bCCKRxAGCSatLevel	0x1f000000
#define	bCCKRxAGCSatCount       0xe0
#define	bCCKRxRFSettle          0x1f       /* AGCsamp_dly */
#define	bCCKFixedRxAGC          0x8000
#define	bCCKAntennaPolarity     0x2000
#define	bCCKTxFilterType        0x0c00
#define	bCCKRxAGCReportType	0x0300
#define	bCCKRxDAGCEn            0x80000000
#define	bCCKRxDAGCPeriod        0x20000000
#define	bCCKRxDAGCSatLevel	0x1f000000
#define	bCCKTimingRecovery      0x800000
#define	bCCKTxC0                0x3f0000
#define	bCCKTxC1                0x3f000000
#define	bCCKTxC2                0x3f
#define	bCCKTxC3                0x3f00
#define	bCCKTxC4                0x3f0000
#define	bCCKTxC5		0x3f000000
#define	bCCKTxC6		0x3f
#define	bCCKTxC7		0x3f00
#define	bCCKDebugPort		0xff0000
#define	bCCKDACDebug		0x0f000000
#define	bCCKFalseAlarmEnable	0x8000
#define	bCCKFalseAlarmRead	0x4000
#define	bCCKTRSSI		0x7f
#define	bCCKRxAGCReport		0xfe
#define	bCCKRxReport_AntSel	0x80000000
#define	bCCKRxReport_MFOff	0x40000000
#define	bCCKRxRxReport_SQLoss	0x20000000
#define	bCCKRxReport_Pktloss	0x10000000
#define	bCCKRxReport_Lockedbit	0x08000000
#define	bCCKRxReport_RateError	0x04000000
#define	bCCKRxReport_RxRate	0x03000000
#define	bCCKRxFACounterLower	0xff
#define	bCCKRxFACounterUpper	0xff000000
#define	bCCKRxHPAGCStart	0xe000
#define	bCCKRxHPAGCFinal	0x1c00
#define	bCCKRxFalseAlarmEnable	0x8000
#define	bCCKFACounterFreeze	0x4000
#define	bCCKTxPathSel		0x10000000
#define	bCCKDefaultRxPath	0xc000000
#define	bCCKOptionRxPath	0x3000000

/* 5. PageC(0xC00) */
#define	bNumOfSTF		0x3	/* Useless */
#define	bShift_L                0xc0
#define	bGI_TH			0xc
#define	bRxPathA		0x1
#define	bRxPathB		0x2
#define	bRxPathC		0x4
#define	bRxPathD		0x8
#define	bTxPathA		0x1
#define	bTxPathB		0x2
#define	bTxPathC		0x4
#define	bTxPathD		0x8
#define	bTRSSIFreq		0x200
#define	bADCBackoff		0x3000
#define	bDFIRBackoff		0xc000
#define	bTRSSILatchPhase	0x10000
#define	bRxIDCOffset		0xff
#define	bRxQDCOffset		0xff00
#define	bRxDFIRMode		0x1800000
#define	bRxDCNFType		0xe000000
#define	bRXIQImb_A		0x3ff
#define	bRXIQImb_B		0xfc00
#define	bRXIQImb_C		0x3f0000
#define	bRXIQImb_D		0xffc00000
#define	bDC_dc_Notch		0x60000
#define	bRxNBINotch		0x1f000000
#define	bPD_TH			0xf
#define	bPD_TH_Opt2		0xc000
#define	bPWED_TH		0x700
#define	bIfMF_Win_L		0x800
#define	bPD_Option		0x1000
#define	bMF_Win_L		0xe000
#define	bBW_Search_L		0x30000
#define	bwin_enh_L		0xc0000
#define	bBW_TH			0x700000
#define	bED_TH2			0x3800000
#define	bBW_option		0x4000000
#define	bRatio_TH		0x18000000
#define	bWindow_L		0xe0000000
#define	bSBD_Option		0x1
#define	bFrame_TH		0x1c
#define	bFS_Option		0x60
#define	bDC_Slope_check		0x80
#define	bFGuard_Counter_DC_L	0xe00
#define	bFrame_Weight_Short	0x7000
#define	bSub_Tune		0xe00000
#define	bFrame_DC_Length	0xe000000
#define	bSBD_start_offset	0x30000000
#define	bFrame_TH_2		0x7
#define	bFrame_GI2_TH		0x38
#define	bGI2_Sync_en		0x40
#define	bSarch_Short_Early	0x300
#define	bSarch_Short_Late	0xc00
#define	bSarch_GI2_Late		0x70000
#define	bCFOAntSum		0x1
#define	bCFOAcc			0x2
#define	bCFOStartOffset		0xc
#define	bCFOLookBack		0x70
#define	bCFOSumWeight		0x80
#define	bDAGCEnable		0x10000
#define	bTXIQImb_A		0x3ff
#define	bTXIQImb_B		0xfc00
#define	bTXIQImb_C		0x3f0000
#define	bTXIQImb_D		0xffc00000
#define	bTxIDCOffset		0xff
#define	bTxQDCOffset		0xff00
#define	bTxDFIRMode		0x10000
#define	bTxPesudoNoiseOn	0x4000000
#define	bTxPesudoNoise_A	0xff
#define	bTxPesudoNoise_B	0xff00
#define	bTxPesudoNoise_C	0xff0000
#define	bTxPesudoNoise_D	0xff000000
#define	bCCADropOption		0x20000
#define	bCCADropThres		0xfff00000
#define	bEDCCA_H		0xf
#define	bEDCCA_L		0xf0
#define	bLambda_ED              0x300
#define	bRxInitialGain          0x7f
#define	bRxAntDivEn             0x80
#define	bRxAGCAddressForLNA     0x7f00
#define	bRxHighPowerFlow        0x8000
#define	bRxAGCFreezeThres       0xc0000
#define	bRxFreezeStep_AGC1      0x300000
#define	bRxFreezeStep_AGC2      0xc00000
#define	bRxFreezeStep_AGC3      0x3000000
#define	bRxFreezeStep_AGC0      0xc000000
#define	bRxRssi_Cmp_En          0x10000000
#define	bRxQuickAGCEn           0x20000000
#define	bRxAGCFreezeThresMode   0x40000000
#define	bRxOverFlowCheckType    0x80000000
#define	bRxAGCShift             0x7f
#define	bTRSW_Tri_Only          0x80
#define	bPowerThres             0x300
#define	bRxAGCEn                0x1
#define	bRxAGCTogetherEn        0x2
#define	bRxAGCMin               0x4
#define	bRxHP_Ini               0x7
#define	bRxHP_TRLNA             0x70
#define	bRxHP_RSSI              0x700
#define	bRxHP_BBP1              0x7000
#define	bRxHP_BBP2              0x70000
#define	bRxHP_BBP3              0x700000
#define	bRSSI_H                 0x7f0000     /* the threshold for high power */
#define	bRSSI_Gen               0x7f000000   /* the threshold for ant divers */
#define	bRxSettle_TRSW          0x7
#define	bRxSettle_LNA           0x38
#define	bRxSettle_RSSI          0x1c0
#define	bRxSettle_BBP           0xe00
#define	bRxSettle_RxHP          0x7000
#define	bRxSettle_AntSW_RSSI    0x38000
#define	bRxSettle_AntSW         0xc0000
#define	bRxProcessTime_DAGC     0x300000
#define	bRxSettle_HSSI          0x400000
#define	bRxProcessTime_BBPPW    0x800000
#define	bRxAntennaPowerShift    0x3000000
#define	bRSSITableSelect        0xc000000
#define	bRxHP_Final             0x7000000
#define	bRxHTSettle_BBP         0x7
#define	bRxHTSettle_HSSI        0x8
#define	bRxHTSettle_RxHP        0x70
#define	bRxHTSettle_BBPPW       0x80
#define	bRxHTSettle_Idle        0x300
#define	bRxHTSettle_Reserved    0x1c00
#define	bRxHTRxHPEn             0x8000
#define	bRxHTAGCFreezeThres     0x30000
#define	bRxHTAGCTogetherEn      0x40000
#define	bRxHTAGCMin             0x80000
#define	bRxHTAGCEn              0x100000
#define	bRxHTDAGCEn             0x200000
#define	bRxHTRxHP_BBP           0x1c00000
#define	bRxHTRxHP_Final         0xe0000000
#define	bRxPWRatioTH            0x3
#define	bRxPWRatioEn            0x4
#define	bRxMFHold               0x3800
#define	bRxPD_Delay_TH1         0x38
#define	bRxPD_Delay_TH2         0x1c0
#define	bRxPD_DC_COUNT_MAX      0x600
#define	bRxPD_Delay_TH          0x8000
#define	bRxProcess_Delay        0xf0000
#define	bRxSearchrange_GI2_Early 0x700000
#define	bRxFrame_Guard_Counter_L 0x3800000
#define	bRxSGI_Guard_L          0xc000000
#define	bRxSGI_Search_L         0x30000000
#define	bRxSGI_TH               0xc0000000
#define	bDFSCnt0                0xff
#define	bDFSCnt1                0xff00
#define	bDFSFlag                0xf0000
#define	bMFWeightSum            0x300000
#define	bMinIdxTH               0x7f000000
#define	bDAFormat               0x40000
#define	bTxChEmuEnable          0x01000000
#define	bTRSWIsolation_A        0x7f
#define	bTRSWIsolation_B        0x7f00
#define	bTRSWIsolation_C        0x7f0000
#define	bTRSWIsolation_D        0x7f000000
#define	bExtLNAGain             0x7c00

/* 6. PageE(0xE00) */
#define	bSTBCEn                 0x4	/* Useless */
#define	bAntennaMapping         0x10
#define	bNss                    0x20
#define	bCFOAntSumD             0x200
#define	bPHYCounterReset        0x8000000
#define	bCFOReportGet           0x4000000
#define	bOFDMContinueTx         0x10000000
#define	bOFDMSingleCarrier      0x20000000
#define	bOFDMSingleTone         0x40000000
#define	bHTDetect               0x100
#define	bCFOEn                  0x10000
#define	bCFOValue               0xfff00000
#define	bSigTone_Re             0x3f
#define	bSigTone_Im             0x7f00
#define	bCounter_CCA            0xffff
#define	bCounter_ParityFail     0xffff0000
#define	bCounter_RateIllegal    0xffff
#define	bCounter_CRC8Fail       0xffff0000
#define	bCounter_MCSNoSupport   0xffff
#define	bCounter_FastSync       0xffff
#define	bShortCFO               0xfff
#define	bShortCFOTLength        12   /* total */
#define	bShortCFOFLength        11   /* fraction */
#define	bLongCFO                0x7ff
#define	bLongCFOTLength         11
#define	bLongCFOFLength         11
#define	bTailCFO                0x1fff
#define	bTailCFOTLength         13
#define	bTailCFOFLength         12
#define	bmax_en_pwdB            0xffff
#define	bCC_power_dB            0xffff0000
#define	bnoise_pwdB             0xffff
#define	bPowerMeasTLength       10
#define	bPowerMeasFLength       3
#define	bRx_HT_BW               0x1
#define	bRxSC                   0x6
#define	bRx_HT                  0x8
#define	bNB_intf_det_on         0x1
#define	bIntf_win_len_cfg       0x30
#define	bNB_Intf_TH_cfg         0x1c0
#define	bRFGain                 0x3f
#define	bTableSel               0x40
#define	bTRSW                   0x80
#define	bRxSNR_A                0xff
#define	bRxSNR_B                0xff00
#define	bRxSNR_C                0xff0000
#define	bRxSNR_D                0xff000000
#define	bSNREVMTLength          8
#define	bSNREVMFLength          1
#define	bCSI1st                 0xff
#define	bCSI2nd                 0xff00
#define	bRxEVM1st               0xff0000
#define	bRxEVM2nd               0xff000000
#define	bSIGEVM                 0xff
#define	bPWDB                   0xff00
#define	bSGIEN                  0x10000

#define	bSFactorQAM1            0xf	/* Useless */
#define	bSFactorQAM2            0xf0
#define	bSFactorQAM3            0xf00
#define	bSFactorQAM4            0xf000
#define	bSFactorQAM5            0xf0000
#define	bSFactorQAM6            0xf0000
#define	bSFactorQAM7            0xf00000
#define	bSFactorQAM8            0xf000000
#define	bSFactorQAM9            0xf0000000
#define	bCSIScheme              0x100000

#define	bNoiseLvlTopSet         0x3	/* Useless */
#define	bChSmooth               0x4
#define	bChSmoothCfg1           0x38
#define	bChSmoothCfg2           0x1c0
#define	bChSmoothCfg3           0xe00
#define	bChSmoothCfg4           0x7000
#define	bMRCMode                0x800000
#define	bTHEVMCfg               0x7000000

#define	bLoopFitType            0x1	/* Useless */
#define	bUpdCFO                 0x40
#define	bUpdCFOOffData          0x80
#define	bAdvUpdCFO              0x100
#define	bAdvTimeCtrl            0x800
#define	bUpdClko                0x1000
#define	bFC                     0x6000
#define	bTrackingMode           0x8000
#define	bPhCmpEnable            0x10000
#define	bUpdClkoLTF             0x20000
#define	bComChCFO               0x40000
#define	bCSIEstiMode            0x80000
#define	bAdvUpdEqz              0x100000
#define	bUChCfg                 0x7000000
#define	bUpdEqz			0x8000000

#define	bTxAGCRate18_06		0x7f7f7f7f	/* Useless */
#define	bTxAGCRate54_24		0x7f7f7f7f
#define	bTxAGCRateMCS32		0x7f
#define	bTxAGCRateCCK		0x7f00
#define	bTxAGCRateMCS3_MCS0	0x7f7f7f7f
#define	bTxAGCRateMCS7_MCS4	0x7f7f7f7f
#define	bTxAGCRateMCS11_MCS8	0x7f7f7f7f
#define	bTxAGCRateMCS15_MCS12	0x7f7f7f7f

/* Rx Pseduo noise */
#define	bRxPesudoNoiseOn         0x20000000	/* Useless */
#define	bRxPesudoNoise_A         0xff
#define	bRxPesudoNoise_B         0xff00
#define	bRxPesudoNoise_C         0xff0000
#define	bRxPesudoNoise_D         0xff000000
#define	bPesudoNoiseState_A      0xffff
#define	bPesudoNoiseState_B      0xffff0000
#define	bPesudoNoiseState_C      0xffff
#define	bPesudoNoiseState_D      0xffff0000

/* 7. RF Register
 * Zebra1 */
#define	bZebra1_HSSIEnable        0x8		/* Useless */
#define	bZebra1_TRxControl        0xc00
#define	bZebra1_TRxGainSetting    0x07f
#define	bZebra1_RxCorner          0xc00
#define	bZebra1_TxChargePump      0x38
#define	bZebra1_RxChargePump      0x7
#define	bZebra1_ChannelNum        0xf80
#define	bZebra1_TxLPFBW           0x400
#define	bZebra1_RxLPFBW           0x600

/*Zebra4 */
#define	bRTL8256RegModeCtrl1      0x100	/* Useless */
#define	bRTL8256RegModeCtrl0      0x40
#define	bRTL8256_TxLPFBW          0x18
#define	bRTL8256_RxLPFBW          0x600

/* RTL8258 */
#define	bRTL8258_TxLPFBW          0xc	/* Useless */
#define	bRTL8258_RxLPFBW          0xc00
#define	bRTL8258_RSSILPFBW        0xc0

/*
 * Other Definition
 */

/* byte endable for sb_write */
#define	bByte0                    0x1	/* Useless */
#define	bByte1                    0x2
#define	bByte2                    0x4
#define	bByte3                    0x8
#define	bWord0                    0x3
#define	bWord1                    0xc
#define	bDWord                    0xf

/* for PutRegsetting & GetRegSetting BitMask */
#define	bMaskByte0                0xff	/* Reg 0xc50 rOFDM0_XAAGCCore~0xC6f */
#define	bMaskByte1                0xff00
#define	bMaskByte2                0xff0000
#define	bMaskByte3                0xff000000
#define	bMaskHWord                0xffff0000
#define	bMaskLWord                0x0000ffff
#define	bMaskDWord                0xffffffff

/* for PutRFRegsetting & GetRFRegSetting BitMask */
#define	bRFRegOffsetMask	0xfffff
#define	bEnable                   0x1	/* Useless */
#define	bDisable                  0x0

#define	LeftAntenna               0x0	/* Useless */
#define	RightAntenna              0x1

#define	tCheckTxStatus            500   /* 500ms Useless */
#define	tUpdateRxCounter          100   /* 100ms */

#define	rateCCK     0	/* Useless */
#define	rateOFDM    1
#define	rateHT      2

/* define Register-End */
#define	bPMAC_End       0x1ff	/* Useless */
#define	bFPGAPHY0_End   0x8ff
#define	bFPGAPHY1_End   0x9ff
#define	bCCKPHY0_End    0xaff
#define	bOFDMPHY0_End   0xcff
#define	bOFDMPHY1_End   0xdff

#define	bPMACControl	0x0	/* Useless */
#define	bWMACControl	0x1
#define	bWNICControl	0x2

#define	ANTENNA_A	0x1	/* Useless */
#define	ANTENNA_B	0x2
#define	ANTENNA_AB	0x3	/* ANTENNA_A |ANTENNA_B */

#define	ANTENNA_C	0x4
#define	ANTENNA_D	0x8


/* accept all physical address */
#define RCR_AAP		BIT(0)
#define RCR_APM		BIT(1)		/* accept physical match */
#define RCR_AM		BIT(2)		/* accept multicast */
#define RCR_AB		BIT(3)		/* accept broadcast */
#define RCR_ACRC32	BIT(5)		/* accept error packet */
#define RCR_9356SEL	BIT(6)
#define RCR_AICV	BIT(12)		/* Accept ICV error packet */
#define RCR_RXFTH0	(BIT(13)|BIT(14)|BIT(15))	/* Rx FIFO threshold */
#define RCR_ADF		BIT(18)		/* Accept Data(frame type) frame */
#define RCR_ACF		BIT(19)		/* Accept control frame */
#define RCR_AMF		BIT(20)		/* Accept management frame */
#define RCR_ADD3	BIT(21)
#define RCR_APWRMGT	BIT(22)		/* Accept power management packet */
#define RCR_CBSSID	BIT(23)		/* Accept BSSID match packet */
#define RCR_ENMARP	BIT(28)		/* enable mac auto reset phy */
#define RCR_EnCS1	BIT(29)		/* enable carrier sense method 1 */
#define RCR_EnCS2	BIT(30)		/* enable carrier sense method 2 */
/* Rx Early mode is performed for packet size greater than 1536 */
#define RCR_OnlyErlPkt	BIT(31)
#endif

#endif /* __RTL8712_REGDEF_H__*/
