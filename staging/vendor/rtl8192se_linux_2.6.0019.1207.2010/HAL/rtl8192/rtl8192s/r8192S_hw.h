/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
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
#ifndef __INC_HAL8192SREG_H
#define __INC_HAL8192SREG_H


#if 0
typedef enum _RT_RF_TYPE_DEF
{
	RF_1T2R = 0,
	RF_2T4R,
	RF_2T2R,
	RF_1T1R,
	RF_2T2R_GREEN,
	RF_819X_MAX_TYPE
}RT_RF_TYPE_DEF;
#endif

#define		SYS_ISO_CTRL			0x0000	
#define		SYS_FUNC_EN			0x0002	
#define		PMC_FSM				0x0004	
#define		SYS_CLKR			0x0008	
#define		EPROM_CMD			0x000A	
#define		EE_VPD				0x000C	
#define		AFE_MISC			0x0010	
#define		SPS0_CTRL			0x0011	
#define		SPS1_CTRL			0x0018	
#define		RF_CTRL				0x001F	
#define		LDOA15_CTRL			0x0020	
#define		LDOV12D_CTRL			0x0021	
#define		LDOHCI12_CTRL			0x0022	
#define		LDO_USB_SDIO			0x0023	
#define		LPLDO_CTRL			0x0024	
#define		AFE_XTAL_CTRL			0x0026	
#define		AFE_PLL_CTRL			0x0028	
#define		EFUSE_CTRL			0x0030	
#define		EFUSE_TEST			0x0034	
#define		PWR_DATA			0x0038	
#define		DBG_PORT			0x003A	
#define		DPS_TIMER			0x003C	
#define		RCLK_MON			0x003E	

#define		CMDR				0x0040	
#define		TXPAUSE				0x0042	
#define		LBKMD_SEL			0x0043	
#define		TCR				0x0044	
#define		RCR				0x0048	
#define		MSR				0x004C	
#define		SYSF_CFG			0x004D	
#define		RX_PKY_LIMIT			0x004E	
#define		MBIDCTRL			0x004F	

#define		MACIDR				0x0050	
#define		MACIDR0				0x0050	
#define		MACIDR4				0x0054	
#define		BSSIDR				0x0058	
#define		HWVID				0x005E	
#define		MAR				0x0060	
#define		MBIDCAMCONTENT			0x0068	
#define		MBIDCAMCFG			0x0070	
#define		BUILDTIME			0x0074	
#define		BUILDUSER			0x0078	

#define		IDR0				MACIDR0
#define		IDR4				MACIDR4

#define		TSFR				0x0080	
#define		SLOT_TIME			0x0089	
#define		USTIME				0x008A	
#define		SIFS_CCK			0x008C	
#define		SIFS_OFDM			0x008E	
#define		PIFS_TIME			0x0090	
#define		ACK_TIMEOUT			0x0091	
#define		EIFSTR				0x0092	
#define		BCN_INTERVAL			0x0094	
#define		ATIMWND				0x0096	
#define		BCN_DRV_EARLY_INT		0x0098	
#define		BCN_DMATIME			0x009A	
#define		BCN_ERR_THRESH			0x009C	
#define		MLT				0x009D	
#define		RSVD_MAC_TUNE_US		0x009E	

#define 	RQPN				0x00A0
#define		RQPN1				0x00A0	
#define		RQPN2				0x00A1	
#define		RQPN3				0x00A2	
#define		RQPN4				0x00A3	
#define		RQPN5				0x00A4	
#define		RQPN6				0x00A5	
#define		RQPN7				0x00A6	
#define		RQPN8				0x00A7	
#define		RQPN9				0x00A8	
#define		RQPN10				0x00A9	
#define		LD_RQPN				0x00AB  
#define		RXFF_BNDY			0x00AC  
#define		RXRPT_BNDY			0x00B0  
#define		TXPKTBUF_PGBNDY			0x00B4  
#define		PBP				0x00B5  
#define		RXDRVINFO_SZ			0x00B6  
#define		TXFF_STATUS			0x00B7  
#define		RXFF_STATUS			0x00B8  
#define		TXFF_EMPTY_TH			0x00B9  
#define		SDIO_RX_BLKSZ			0x00BC  
#define		RXDMA				0x00BD  
#define		RXPKT_NUM			0x00BE  
#define		C2HCMD_UDT_SIZE			0x00C0  
#define		C2HCMD_UDT_ADDR			0x00C2  
#define		FIFOPAGE1			0x00C4  
#define		FIFOPAGE2			0x00C8  
#define		FIFOPAGE3			0x00CC  
#define		FIFOPAGE4			0x00D0  
#define		FIFOPAGE5			0x00D4  
#define		FW_RSVD_PG_CRTL			0x00D8  
#define		RXDMA_AGG_PG_TH			0x00D9  
#define		TXDESC_MSK 			0x00DC  
#define		TXRPTFF_RDPTR			0x00E0  
#define		TXRPTFF_WTPTR			0x00E4  
#define		C2HFF_RDPTR			0x00E8	
#define		C2HFF_WTPTR			0x00EC	
#define		RXFF0_RDPTR			0x00F0	
#define		RXFF0_WTPTR			0x00F4  
#define		RXFF1_RDPTR			0x00F8  
#define		RXFF1_WTPTR			0x00FC  
#define		RXRPT0_RDPTR			0x0100  
#define		RXRPT0_WTPTR			0x0104  
#define		RXRPT1_RDPTR			0x0108  
#define		RXRPT1_WTPTR			0x010C  
#define		RX0_UDT_SIZE			0x0110  
#define		RX1PKTNUM			0x0114  
#define		RXFILTERMAP			0x0116  
#define		RXFILTERMAP_GP1			0x0118  
#define		RXFILTERMAP_GP2			0x011A  
#define		RXFILTERMAP_GP3			0x011C  
#define		BCNQ_CTRL			0x0120  
#define		MGTQ_CTRL			0x0124  
#define		HIQ_CTRL			0x0128  
#define		VOTID7_CTRL			0x012c  
#define		VOTID6_CTRL			0x0130  
#define		VITID5_CTRL			0x0134  
#define		VITID4_CTRL			0x0138  
#define		BETID3_CTRL			0x013c  
#define		BETID0_CTRL			0x0140  
#define		BKTID2_CTRL			0x0144  
#define		BKTID1_CTRL			0x0148  
#define		CMDQ_CTRL			0x014c  
#define		TXPKT_NUM_CTRL			0x0150  
#define		TXQ_PGADD			0x0152  
#define		TXFF_PG_NUM			0x0154  
#define		TRXDMA_STATUS			0x0156  

#define		INIMCS_SEL			0x0160	
#define		TX_RATE_REG			INIMCS_SEL 
#define		INIRTSMCS_SEL			0x0180	
#define		RRSR				0x0181	
#define		ARFR0				0x0184	
#define		ARFR1				0x0188	
#define		ARFR2				0x018C  
#define		ARFR3				0x0190  
#define		ARFR4				0x0194  
#define		ARFR5				0x0198  
#define		ARFR6				0x019C  
#define		ARFR7				0x01A0  
#define		AGGLEN_LMT_H			0x01A7	
#define		AGGLEN_LMT_L			0x01A8	
#define		DARFRC				0x01B0	
#define		RARFRC				0x01B8	
#define		MCS_TXAGC			0x01C0
#define		CCK_TXAGC			0x01C8

#define		EDCAPARA_VO 			0x01D0	
#define		EDCAPARA_VI			0x01D4	
#define		EDCAPARA_BE			0x01D8	
#define		EDCAPARA_BK			0x01DC	
#define		BCNTCFG				0x01E0	
#define		CWRR				0x01E2	
#define		ACMAVG				0x01E4	
#define		AcmHwCtrl			0x01E7	
#define		VO_ADMTM			0x01E8	
#define		VI_ADMTM			0x01EC
#define		BE_ADMTM			0x01F0
#define		RETRY_LIMIT			0x01F4	
#define		SG_RATE				0x01F6	

#define		NAV_CTRL			0x0200
#define		BW_OPMODE			0x0203
#define		BACAMCMD			0x0204
#define		BACAMCONTENT			0x0208	

#define		LBDLY				0x0210	
#define		FWDLY				0x0211	
#define		HWPC_RX_CTRL			0x0218	
#define		MQIR				0x0220	
#define		MAIR				0x0222	
#define		MSIR				0x0224	
#define		CLM_RESULT			0x0227	
#define		NHM_RPI_CNT			0x0228	
#define		RXERR_RPT			0x0230	
#define		NAV_PROT_LEN			0x0234	
#define		CFEND_TH			0x0236	
#define		AMPDU_MIN_SPACE			0x0237	
#define		TXOP_STALL_CTRL			0x0238

#define		RWCAM				0x0240	
#define		WCAMI				0x0244	
#define		RCAMO				0x0248	
#define		CAMDBG				0x024C
#define		SECR				0x0250	

#define		WOW_CTRL			0x0260	
#define		PSSTATUS			0x0261	
#define		PSSWITCH			0x0262	
#define		MIMOPS_WAIT_PERIOD		0x0263
#define		LPNAV_CTRL			0x0264
#define		WFM0				0x0270	
#define		WFM1				0x0280	
#define		WFM2				0x0290  
#define		WFM3				0x02A0  
#define		WFM4				0x02B0  
#define		WFM5				0x02C0  
#define		WFCRC				0x02D0	
#define		FW_RPT_REG			0x02c4

#define		PSTIME				0x02E0  
#define		TIMER0				0x02E4  
#define		TIMER1				0x02E8  
#define		GPIO_CTRL			0x02EC  
#define		GPIO_IN				0x02EC	
#define		GPIO_OUT			0x02ED	
#define		GPIO_IO_SEL			0x02EE	
#define		GPIO_MOD			0x02EF	
#define		GPIO_INTCTRL			0x02F0  
#define		MAC_PINMUX_CFG			0x02F1  
#define		LEDCFG				0x02F2  
#define		PHY_REG				0x02F3  
#define		PHY_REG_DATA			0x02F4  
#define		EFUSE_CLK			0x02F8  

#define		INTA_MASK				0x0300	
#define		ISR				0x0308	

#define		DBG_PORT_SWITCH			0x003A
#define		BIST				0x0310	
#define		DBS				0x0314	
#define		CPUINST				0x0318	
#define		CPUCAUSE			0x031C	
#define		LBUS_ERR_ADDR			0x0320	
#define		LBUS_ERR_CMD			0x0324	
#define		LBUS_ERR_DATA_L			0x0328	
#define		LBUS_ERR_DATA_H			0x032C	
#define		LX_EXCEPTION_ADDR		0x0330	
#define		WDG_CTRL			0x0334	
#define		INTMTU				0x0338	
#define		INTM				0x033A	
#define		FDLOCKTURN0			0x033C	
#define		FDLOCKTURN1			0x033D	
#define		TRXPKTBUF_DBG_DATA		0x0340	
#define		TRXPKTBUF_DBG_CTRL		0x0348	
#define		DPLL				0x034A	
#define		CBUS_ERR_ADDR			0x0350	
#define		CBUS_ERR_CMD			0x0354	
#define		CBUS_ERR_DATA_L			0x0358	
#define		CBUS_ERR_DATA_H 		0x035C	
#define		USB_SIE_INTF_ADDR		0x0360	
#define		USB_SIE_INTF_WD			0x0361	
#define		USB_SIE_INTF_RD			0x0362	
#define		USB_SIE_INTF_CTRL		0x0363	
#define 		LBUS_MON_ADDR 		0x0364	
#define 		LBUS_ADDR_MASK 		0x0368	


#define		TPPoll				0x0500	
#define		PM_CTRL				0x0502	
#define		PCIF				0x0503	

#define		THPDA				0x0514	
#define		TMDA				0x0518	
#define		TCDA				0x051C	
#define		HDA				0x0520	
#define		TVODA				0x0524	
#define		TVIDA				0x0528	
#define		TBEDA				0x052C	
#define		TBKDA				0x0530	
#define		TBDA				0x0534	
#define		RCDA				0x0538	
#define		RDQDA				0x053C	
#define		DBI_WDATA			0x0540	
#define		DBI_RDATA			0x0544	
#define		DBI_CTRL			0x0548	
#define		MDIO_DATA			0x0550	
#define		MDIO_CTRL			0x0554	
#define		PCI_RPWM			0x0561	
#define		PCI_CPWM				0x0563	


#define		PHY_CCA			0x803	


#define	USB_RX_AGG_TIMEOUT			0xFE5B	

#define	FW_OFFLOAD_EN				BIT7

#define	MAX_MSS_DENSITY_2T 			0x13
#define	MAX_MSS_DENSITY_1T 			0x0A

#define	RXDMA_AGG_EN				BIT7

#define	RXDMA_AGG_TIMEOUT_DISABLE		0x00
#define	RXDMA_AGG_TIMEOUT_17MS  		0x01
#define	RXDMA_AGG_TIMEOUT_17_2_MS  		0x02
#define	RXDMA_AGG_TIMEOUT_17_4_MS  		0x04
#define	RXDMA_AGG_TIMEOUT_17_10_MS  		0x0A

#define	InvalidBBRFValue		0x12345678

#define	USB_RPWM			0xFE58

#ifdef RTL8192SE 
#define	RPWM		PCI_RPWM
#endif
#ifdef RTL8192SU 
#define	RPWM		USB_RPWM
#endif

#if 1	
#define		AFR				0x010	
#define		BCN_TCFG			0x062	
#define		RATR0				0x320	
#endif
#define		UnusedRegister			0x0320		
#define		PSR				UnusedRegister	
#define		DCAM				UnusedRegister	
#define		BBAddr				UnusedRegister	
#define		PhyDataR			UnusedRegister	
#define		UFWP				UnusedRegister



#define		ISO_MD2PP			BIT0	
#define		ISO_PA2PCIE			BIT3	
#define		ISO_PLL2MD			BIT4	
#define		ISO_PWC_DV2RP		BIT11	
#define		ISO_PWC_RV2RP		BIT12	

#define		FEN_MREGEN			BIT15	
#define		FEN_DCORE			BIT11	
#define		FEN_CPUEN			BIT10	

#define		PAD_HWPD_IDN		BIT22	

#define		SYS_CLKSEL_80M		BIT0	
#define		SYS_PS_CLKSEL		BIT1	 
#define		SYS_CPU_CLKSEL		BIT2	
#define		SYS_MAC_CLK_EN		BIT11	
#define		SYS_SWHW_SEL		BIT14	
#define		SYS_FWHW_SEL		BIT15	


#define		CmdEEPROM_En						BIT5	 
#define		CmdEERPOMSEL						BIT4 
#define		Cmd9346CR_9356SEL					BIT4
#define		AutoLoadEEPROM					(CmdEEPROM_En|CmdEERPOMSEL)
#define		AutoLoadEFUSE						CmdEEPROM_En
#define EPROM_CMD_OPERATING_MODE_SHIFT 6
#define EPROM_CMD_OPERATING_MODE_MASK ((1<<7)|(1<<6))
#define EPROM_CMD_CONFIG 0x3
#define EPROM_CMD_NORMAL 0 
#define EPROM_CMD_LOAD 1
#define EPROM_CMD_PROGRAM 2
#define EPROM_CS_SHIFT 3
#define EPROM_CK_SHIFT 2
#define EPROM_W_SHIFT 1
#define EPROM_R_SHIFT 0

#define		AFE_MBEN			BIT1	
#define		AFE_BGEN			BIT0	

#define		SPS1_SWEN			BIT1	
#define		SPS1_LDEN			BIT0	

#define		RF_EN				BIT0 
#define		RF_RSTB			BIT1 
#define		RF_SDMRSTB			BIT2 

#define		LDA15_EN			BIT0	

#define		LDV12_EN			BIT0	
#define		LDV12_SDBY			BIT1	

#define		XTAL_GATE_AFE		BIT10	

#define		APLL_EN				BIT0	

#define		AFR_CardBEn			BIT0
#define		AFR_CLKRUN_SEL		BIT1
#define		AFR_FuncRegEn		BIT2

#define		APSDOFF_STATUS		BIT15	
#define		APSDOFF				BIT14   
#define		BBRSTn				BIT13   
#define		BB_GLB_RSTn			BIT12   
#define		SCHEDULE_EN			BIT10   
#define		MACRXEN				BIT9    
#define		MACTXEN				BIT8    
#define		DDMA_EN				BIT7    
#define		FW2HW_EN			BIT6    
#define		RXDMA_EN			BIT5    
#define		TXDMA_EN			BIT4    
#define		HCI_RXDMA_EN		BIT3    
#define		HCI_TXDMA_EN		BIT2    

#define		StopHCCA			BIT6
#define		StopHigh			BIT5
#define		StopMgt				BIT4
#define		StopVO				BIT3
#define		StopVI				BIT2
#define		StopBE				BIT1
#define		StopBK				BIT0

#define		LBK_NORMAL		0x00
#define		LBK_MAC_LB		(BIT0|BIT1|BIT3)
#define		LBK_MAC_DLB		(BIT0|BIT1)
#define		LBK_DMA_LB		(BIT0|BIT1|BIT2)	

#define		TCP_OFDL_EN				BIT25	
#define		HWPC_TX_EN				BIT24   
#define		TXDMAPRE2FULL			BIT23   
#define		DISCW					BIT20   
#define		TCRICV					BIT19   
#define		CfendForm				BIT17   
#define		TCRCRC					BIT16   
#define		FAKE_IMEM_EN			BIT15   
#define		TSFRST					BIT9    
#define		TSFEN					BIT8    
#define		FWALLRDY				(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7)
#define		FWRDY					BIT7
#define		BASECHG					BIT6
#define		IMEM					BIT5
#define		DMEM_CODE_DONE			BIT4
#define		EXT_IMEM_CHK_RPT		BIT3
#define		EXT_IMEM_CODE_DONE		BIT2
#define		IMEM_CHK_RPT			BIT1
#define		IMEM_CODE_DONE			BIT0
#define		IMEM_CODE_DONE		BIT0	
#define		IMEM_CHK_RPT		BIT1
#define		EMEM_CODE_DONE		BIT2
#define		EMEM_CHK_RPT		BIT3
#define		DMEM_CODE_DONE		BIT4
#define		IMEM_RDY			BIT5
#define		BASECHG				BIT6
#define		FWRDY				BIT7
#define		LOAD_FW_READY	(IMEM_CODE_DONE|IMEM_CHK_RPT|EMEM_CODE_DONE|\
								EMEM_CHK_RPT|DMEM_CODE_DONE|IMEM_RDY|BASECHG|\
								FWRDY)
#define		TCR_TSFEN			BIT8		
#define		TCR_TSFRST			BIT9		
#define		TCR_FAKE_IMEM_EN	BIT15
#define		TCR_CRC				BIT16
#define		TCR_ICV				BIT19	
#define		TCR_DISCW			BIT20	
#define		TCR_HWPC_TX_EN		BIT24
#define		TCR_TCP_OFDL_EN		BIT25
#define		TXDMA_INIT_VALUE	(IMEM_CHK_RPT|EXT_IMEM_CHK_RPT)

#define		RCR_APPFCS			BIT31		
#define		RCR_DIS_ENC_2BYTE	BIT30       
#define		RCR_DIS_AES_2BYTE	BIT29       
#define		RCR_HTC_LOC_CTRL	BIT28       
#define		RCR_ENMBID			BIT27		
#define		RCR_RX_TCPOFDL_EN	BIT26		
#define		RCR_APP_PHYST_RXFF	BIT25       
#define		RCR_APP_PHYST_STAFF	BIT24       
#define		RCR_CBSSID			BIT23		
#define		RCR_APWRMGT			BIT22		
#define		RCR_ADD3			BIT21		
#define		RCR_AMF				BIT20		
#define		RCR_ACF				BIT19		
#define		RCR_ADF				BIT18		
#define		RCR_APP_MIC			BIT17		
#define		RCR_APP_ICV			BIT16       
#define		RCR_RXFTH			BIT13		
#define		RCR_AICV			BIT12		
#define		RCR_RXDESC_LK_EN	BIT11		
#define		RCR_APP_BA_SSN		BIT6		
#define		RCR_ACRC32			BIT5		
#define		RCR_RXSHFT_EN		BIT4		
#define		RCR_AB				BIT3		
#define		RCR_AM				BIT2		
#define		RCR_APM				BIT1		
#define		RCR_AAP				BIT0		
#define		RCR_MXDMA_OFFSET	8
#define		RCR_FIFO_OFFSET		13

/*
Network Type
00: No link
01: Link in ad hoc network
10: Link in infrastructure network
11: AP mode
Default: 00b.
*/
#define MSR_LINK_MASK      ((1<<0)|(1<<1))
#define MSR_LINK_MANAGED   2
#define MSR_LINK_NONE      0
#define MSR_LINK_SHIFT     0
#define MSR_LINK_ADHOC     1
#define MSR_LINK_MASTER    3
#if 1
#define		MSR_NOLINK				0x00
#define		MSR_ADHOC				0x01
#define		MSR_INFRA				0x02
#define		MSR_AP					0x03
#endif
#define		ENUART					BIT7
#define		ENJTAG					BIT3
#define		BTMODE					(BIT2|BIT1)
#define		ENBT					BIT0

#define		ENMBID					BIT7
#define		BCNUM					(BIT6|BIT5|BIT4)


#define		USTIME_EDCA				0xFF00
#define		USTIME_TSF				0x00FF

#define		SIFS_TRX				0xFF00
#define		SIFS_CTX				0x00FF

#define		ENSWBCN					BIT15
#define		DRVERLY_TU				0x0FF0
#define		DRVERLY_US				0x000F
#define		BCN_TCFG_CW_SHIFT		8
#define		BCN_TCFG_IFS			0	


#define		RRSR_RSC_OFFSET			21
#define		RRSR_SHORT_OFFSET		23
#define		RRSR_RSC_BW_40M		0x600000
#define		RRSR_RSC_UPSUBCHNL		0x400000
#define		RRSR_RSC_LOWSUBCHNL		0x200000
#define		RRSR_SHORT				0x800000
#define		RRSR_1M					BIT0
#define		RRSR_2M					BIT1 
#define		RRSR_5_5M				BIT2 
#define		RRSR_11M				BIT3 
#define		RRSR_6M					BIT4 
#define		RRSR_9M					BIT5 
#define		RRSR_12M				BIT6 
#define		RRSR_18M				BIT7 
#define		RRSR_24M				BIT8 
#define		RRSR_36M				BIT9 
#define		RRSR_48M				BIT10 
#define		RRSR_54M				BIT11
#define		RRSR_MCS0				BIT12
#define		RRSR_MCS1				BIT13
#define		RRSR_MCS2				BIT14
#define		RRSR_MCS3				BIT15
#define		RRSR_MCS4				BIT16
#define		RRSR_MCS5				BIT17
#define		RRSR_MCS6				BIT18
#define		RRSR_MCS7				BIT19
#define		BRSR_AckShortPmb		BIT23	

#define		RATR_1M					0x00000001
#define		RATR_2M					0x00000002
#define		RATR_55M				0x00000004
#define		RATR_11M				0x00000008
#define		RATR_6M					0x00000010
#define		RATR_9M					0x00000020
#define		RATR_12M				0x00000040
#define		RATR_18M				0x00000080
#define		RATR_24M				0x00000100
#define		RATR_36M				0x00000200
#define		RATR_48M				0x00000400
#define		RATR_54M				0x00000800
#define		RATR_MCS0				0x00001000
#define		RATR_MCS1				0x00002000
#define		RATR_MCS2				0x00004000
#define		RATR_MCS3				0x00008000
#define		RATR_MCS4				0x00010000
#define		RATR_MCS5				0x00020000
#define		RATR_MCS6				0x00040000
#define		RATR_MCS7				0x00080000
#define		RATR_MCS8				0x00100000
#define		RATR_MCS9				0x00200000
#define		RATR_MCS10				0x00400000
#define		RATR_MCS11				0x00800000
#define		RATR_MCS12				0x01000000
#define		RATR_MCS13				0x02000000
#define		RATR_MCS14				0x04000000
#define		RATR_MCS15				0x08000000
#define	RATE_ALL_CCK				RATR_1M|RATR_2M|RATR_55M|RATR_11M 
#define	RATE_ALL_OFDM_AG			RATR_6M|RATR_9M|RATR_12M|RATR_18M|RATR_24M|\
									RATR_36M|RATR_48M|RATR_54M	
#define	RATE_ALL_OFDM_1SS			RATR_MCS0|RATR_MCS1|RATR_MCS2|RATR_MCS3 |\
									RATR_MCS4|RATR_MCS5|RATR_MCS6	|RATR_MCS7	
#define	RATE_ALL_OFDM_2SS			RATR_MCS8|RATR_MCS9	|RATR_MCS10|RATR_MCS11|\
									RATR_MCS12|RATR_MCS13|RATR_MCS14|RATR_MCS15
									
#define		AC_PARAM_TXOP_LIMIT_OFFSET		16
#define		AC_PARAM_ECW_MAX_OFFSET			12
#define		AC_PARAM_ECW_MIN_OFFSET			8
#define		AC_PARAM_AIFS_OFFSET			0

#define		AcmHw_HwEn				BIT0
#define		AcmHw_BeqEn				BIT1
#define		AcmHw_ViqEn				BIT2
#define		AcmHw_VoqEn				BIT3
#define		AcmHw_BeqStatus			BIT4
#define		AcmHw_ViqStatus			BIT5
#define		AcmHw_VoqStatus			BIT6

#define		RETRY_LIMIT_SHORT_SHIFT	8
#define		RETRY_LIMIT_LONG_SHIFT	0

#define		NAV_UPPER_EN			BIT16
#define		NAV_UPPER				0xFF00
#define		NAV_RTSRST				0xFF
#define		BW_OPMODE_20MHZ			BIT2
#define		BW_OPMODE_5G			BIT1
#define		BW_OPMODE_11J			BIT0

#define		RXERR_RPT_RST			BIT27 
#define		RXERR_OFDM_PPDU			0
#define		RXERR_OFDM_FALSE_ALARM	1
#define		RXERR_OFDM_MPDU_OK		2
#define		RXERR_OFDM_MPDU_FAIL	3
#define		RXERR_CCK_PPDU			4
#define		RXERR_CCK_FALSE_ALARM	5
#define		RXERR_CCK_MPDU_OK		6
#define		RXERR_CCK_MPDU_FAIL		7
#define		RXERR_HT_PPDU			8
#define		RXERR_HT_FALSE_ALARM	9
#define		RXERR_HT_MPDU_TOTAL		10
#define		RXERR_HT_MPDU_OK		11
#define		RXERR_HT_MPDU_FAIL		12
#define		RXERR_RX_FULL_DROP		15


#define		CAM_CM_SecCAMPolling	BIT31		
#define		CAM_CM_SecCAMClr		BIT30		
#define		CAM_CM_SecCAMWE			BIT16		
#define		CAM_ADDR				0xFF		

#define		Dbg_CAM_TXSecCAMInfo	BIT31		
#define		Dbg_CAM_SecKeyFound		BIT30		


#define		SCR_TxUseDK				BIT0			
#define		SCR_RxUseDK				BIT1			
#define		SCR_TxEncEnable			BIT2			
#define		SCR_RxDecEnable			BIT3			
#define		SCR_SKByA2				BIT4			
#define		SCR_NoSKMC				BIT5			
#define		CAM_VALID				BIT15
#define		CAM_NOTVALID			0x0000
#define		CAM_USEDK				BIT5
       	       		
#define		CAM_NONE				0x0
#define		CAM_WEP40				0x01
#define		CAM_TKIP				0x02
#define		CAM_AES					0x04
#define		CAM_WEP104				0x05
        		
#define		TOTAL_CAM_ENTRY			32
#define		HALF_CAM_ENTRY				16
       		
#define		CAM_CONFIG_USEDK		true
#define		CAM_CONFIG_NO_USEDK		false
       		
#define		CAM_WRITE				BIT16
#define		CAM_READ				0x00000000
#define		CAM_POLLINIG			BIT31
       		
#define		SCR_UseDK				0x01
#define		SCR_TxSecEnable		0x02
#define		SCR_RxSecEnable		0x04

#define		WOW_PMEN				BIT0 
#define		WOW_WOMEN			BIT1 
#define		WOW_MAGIC				BIT2 
#define		WOW_UWF				BIT3 

#define		GPIOMUX_EN			BIT3 
#define		GPIOSEL_GPIO		0	
#define		GPIOSEL_PHYDBG		1	
#define		GPIOSEL_BT			2	
#define		GPIOSEL_WLANDBG		3	
#define		GPIOSEL_GPIO_MASK	~(BIT0|BIT1)

#define		HST_RDBUSY				BIT0
#define		CPU_WTBUSY			BIT1

#define		IMR8190_DISABLED	0x0
#define		IMR_CPUERR			BIT5		
#define		IMR_ATIMEND			BIT4		
#define		IMR_TBDOK			BIT3		
#define		IMR_TBDER			BIT2		
#define		IMR_BCNDMAINT8		BIT1		
#define		IMR_BCNDMAINT7		BIT0		
#define		IMR_BCNDMAINT6		BIT31		
#define		IMR_BCNDMAINT5		BIT30		
#define		IMR_BCNDMAINT4		BIT29		
#define		IMR_BCNDMAINT3		BIT28		
#define		IMR_BCNDMAINT2		BIT27		
#define		IMR_BCNDMAINT1		BIT26		
#define		IMR_BCNDOK8			BIT25		
#define		IMR_BCNDOK7			BIT24		
#define		IMR_BCNDOK6			BIT23		
#define		IMR_BCNDOK5			BIT22		
#define		IMR_BCNDOK4			BIT21		
#define		IMR_BCNDOK3			BIT20		
#define		IMR_BCNDOK2			BIT19		
#define		IMR_BCNDOK1			BIT18		
#define		IMR_TIMEOUT2		BIT17		
#define		IMR_TIMEOUT1		BIT16		
#define		IMR_TXFOVW			BIT15		
#define		IMR_PSTIMEOUT		BIT14		
#define		IMR_BcnInt			BIT13		
#define		IMR_RXFOVW			BIT12		
#define		IMR_RDU				BIT11		
#define		IMR_RXCMDOK			BIT10		
#define		IMR_BDOK			BIT9		
#define		IMR_HIGHDOK			BIT8		
#define		IMR_COMDOK			BIT7		
#define		IMR_MGNTDOK			BIT6		
#define		IMR_HCCADOK			BIT5		
#define		IMR_BKDOK			BIT4		
#define		IMR_BEDOK			BIT3		
#define		IMR_VIDOK			BIT2		
#define		IMR_VODOK			BIT1		
#define		IMR_ROK				BIT0		


#define		TPPoll_BKQ			BIT0			
#define		TPPoll_BEQ			BIT1			
#define		TPPoll_VIQ			BIT2			
#define		TPPoll_VOQ			BIT3			
#define		TPPoll_BQ			BIT4			
#define		TPPoll_CQ			BIT5			
#define		TPPoll_MQ			BIT6			
#define		TPPoll_HQ			BIT7			
#define		TPPoll_HCCAQ		BIT8			
#define		TPPoll_StopBK		BIT9			
#define		TPPoll_StopBE		BIT10			
#define		TPPoll_StopVI		BIT11			
#define		TPPoll_StopVO		BIT12			
#define		TPPoll_StopMgt		BIT13			
#define		TPPoll_StopHigh		BIT14			
#define		TPPoll_StopHCCA		BIT15			
#define		TPPoll_SHIFT		8				

#define		MXDMA2_16bytes		0x000
#define		MXDMA2_32bytes		0x001
#define		MXDMA2_64bytes		0x010
#define		MXDMA2_128bytes		0x011
#define		MXDMA2_256bytes		0x100
#define		MXDMA2_512bytes		0x101
#define		MXDMA2_1024bytes	0x110
#define		MXDMA2_NoLimit		0x7
       		
#define		MULRW_SHIFT			3
#define		MXDMA2_RX_SHIFT		4
#define		MXDMA2_TX_SHIFT		0

#define		CCX_CMD_CLM_ENABLE				BIT0	
#define		CCX_CMD_NHM_ENABLE				BIT1	
#define		CCX_CMD_FUNCTION_ENABLE			BIT8	
#define		CCX_CMD_IGNORE_CCA				BIT9	
#define		CCX_CMD_IGNORE_TXON				BIT10	
#define		CCX_CLM_RESULT_READY			BIT16	
#define		CCX_NHM_RESULT_READY			BIT16	
#define		CCX_CMD_RESET					0x0		


#define		HWSET_MAX_SIZE_92S				128



#ifdef RTL8192SE 
#define 	RTL8190_EEPROM_ID		0x8129	
#define 	EEPROM_HPON			0x02 
#define 	EEPROM_CLK			0x06 
#define 	EEPROM_TESTR			0x08 

#define 	EEPROM_VID			0x0A 
#define 	EEPROM_DID			0x0C 
#define 	EEPROM_SVID			0x0E 
#define 	EEPROM_SMID			0x10 

#define 	EEPROM_MAC_ADDR			0x12 
#define 	EEPROM_NODE_ADDRESS_BYTE_0	0x12 

#define 	EEPROM_PwDiff			0x54 

#define 	EEPROM_TxPowerBase			0x50 
#define		EEPROM_TX_PWR_INDEX_RANGE	28		

#define 	EEPROM_TX_PWR_HT20_DIFF		0x62
#define 	DEFAULT_HT20_TXPWR_DIFF		2	
#define 	EEPROM_TX_PWR_OFDM_DIFF		0x65

#define	EEPROM_TxPWRGroup			0x67
#define 	EEPROM_Regulatory				0x6D

#define 	TX_PWR_SAFETY_CHK		0x6D
#define 	EEPROM_TxPwIndex_CCK_24G	0x5D 
#define 	EEPROM_TxPwIndex_OFDM_24G	0x6B 
#define 	EEPROM_HT2T_CH1_A		0x6c 
#define 	EEPROM_HT2T_CH7_A		0x6d 
#define 	EEPROM_HT2T_CH13_A		0x6e 
#define 	EEPROM_HT2T_CH1_B		0x6f 
#define 	EEPROM_HT2T_CH7_B		0x70 
#define 	EEPROM_HT2T_CH13_B		0x71 
#define 	EEPROM_TSSI_A			0x74 
#define 	EEPROM_TSSI_B			0x75 
#define		EEPROM_RFInd_PowerDiff			0x76
#define		EEPROM_Default_LegacyHTTxPowerDiff	0x3
#define 	EEPROM_ThermalMeter			0x77 
#define		EEPROM_BLUETOOTH_COEXIST		0x78 
#define		EEPROM_BLUETOOTH_TYPE		0x4f 

#define	EEPROM_Optional	0x78 
#ifdef RTL8192S_WAPI_SUPPORT
#define		EEPROM_WAPI_SUPPORT			0x78
#endif
#define		EEPROM_WoWLAN				0x78 

#define 	EEPROM_CrystalCap			0x79 
#define 	EEPROM_ChannelPlan			0x7B 
#define 	EEPROM_Version				0x7C 
#define		EEPROM_CustomID				0x7A
#define 	EEPROM_BoardType			0x7E 

#define		EEPROM_Default_TSSI			0x0
#define 	EEPROM_Default_TxPowerDiff		0x0
#define 	EEPROM_Default_CrystalCap		0x5
#define 	EEPROM_Default_BoardType		0x02 
#define 	EEPROM_Default_TxPower			0x1010
#define		EEPROM_Default_HT2T_TxPwr		0x10

#define		EEPROM_Default_LegacyHTTxPowerDiff	0x3
#define		EEPROM_Default_ThermalMeter		0x12
#define		EEPROM_Default_BlueToothCoexist		0x0
#define		EEPROM_Default_BlueToothAntNum	0x0
#define		EEPROM_Default_BlueToothAntIso		0x0
#define		EEPROM_Default_BlueToothType		0x0
#define		EEPROM_Default_AntTxPowerDiff		0x0
#define		EEPROM_Default_TxPwDiff_CrystalCap	0x5
#define		EEPROM_Default_TxPowerLevel		0x22

#define		EEPROM_CHANNEL_PLAN_FCC			0x0
#define		EEPROM_CHANNEL_PLAN_IC			0x1
#define		EEPROM_CHANNEL_PLAN_ETSI		0x2
#define		EEPROM_CHANNEL_PLAN_SPAIN		0x3
#define		EEPROM_CHANNEL_PLAN_FRANCE		0x4
#define		EEPROM_CHANNEL_PLAN_MKK			0x5
#define		EEPROM_CHANNEL_PLAN_MKK1		0x6
#define		EEPROM_CHANNEL_PLAN_ISRAEL		0x7
#define		EEPROM_CHANNEL_PLAN_TELEC		0x8
#define		EEPROM_CHANNEL_PLAN_GLOBAL_DOMAIN	0x9
#define		EEPROM_CHANNEL_PLAN_WORLD_WIDE_13	0xA
#define		EEPROM_CHANNEL_PLAN_NCC				0xB
#define		EEPROM_CHANNEL_PLAN_BY_HW_MASK	0x80


#define 	EEPROM_CID_DEFAULT		0x0
#define 	EEPROM_CID_TOSHIBA		0x4
#define	EEPROM_CID_CCX				0x10 
#define	EEPROM_CID_QMI				0x0D
#define 	EEPROM_CID_WHQL 			0xFE 

#else 
#define 	RTL8190_EEPROM_ID		0x8129
#define 	EEPROM_HPON			0x02 
#define 	EEPROM_VID			0x08 
#define 	EEPROM_PID			0x0A 
#define 	EEPROM_USB_OPTIONAL		0x0C 
#define 	EEPROM_USB_PHY_PARA1		0x0D 
#define 	EEPROM_NODE_ADDRESS_BYTE_0	0x12 
#define 	EEPROM_Version			0x50	
#define 	EEPROM_ChannelPlan		0x51 
#define		EEPROM_CustomID			0x52
#define 	EEPROM_SubCustomID		0x53 

#define 	EEPROM_BoardType			0x54 
#define		EEPROM_TxPwIndex			0x55 
#define 	EEPROM_PwDiff				0x67 
#define 	EEPROM_ThermalMeter			0x68 
#define 	EEPROM_CrystalCap			0x69 
#define 	EEPROM_TxPowerBase			0x6a 
#define 	EEPROM_TSSI_A				0x6b 
#define 	EEPROM_TSSI_B				0x6c 
#define 	EEPROM_TxPwTkMode			0x6d 

#define 	EEPROM_TX_PWR_HT20_DIFF			0x6e
#define 	DEFAULT_HT20_TXPWR_DIFF			2
#define 	EEPROM_TX_PWR_OFDM_DIFF			0x71
#define 	TX_PWR_SAFETY_CHK			0x79

#define		EEPROM_USB_Default_OPTIONAL_FUNC	0x8
#define		EEPROM_USB_Default_PHY_PARAM		0x0
#define		EEPROM_Default_TSSI			0x0
#define		EEPROM_Default_TxPwrTkMode		0x0
#define 	EEPROM_Default_TxPowerDiff		0x0
#define 	EEPROM_Default_TxPowerBase		0x0
#define 	EEPROM_Default_ThermalMeter		0x7
#define 	EEPROM_Default_PwDiff			0x4
#define 	EEPROM_Default_CrystalCap		0x5
#define 	EEPROM_Default_TxPower			0x1010
#define 	EEPROM_Default_BoardType		0x02 
#define		EEPROM_Default_HT2T_TxPwr			0x10
#define		EEPROM_USB_SN					BIT0
#define		EEPROM_USB_REMOTE_WAKEUP	BIT1
#define		EEPROM_USB_DEVICE_PWR		BIT2
#define		EEPROM_EP_NUMBER				(BIT3|BIT4)


#define		EEPROM_CHANNEL_PLAN_FCC				0x0
#define		EEPROM_CHANNEL_PLAN_IC				0x1
#define		EEPROM_CHANNEL_PLAN_ETSI			0x2
#define		EEPROM_CHANNEL_PLAN_SPAIN			0x3
#define		EEPROM_CHANNEL_PLAN_FRANCE			0x4
#define		EEPROM_CHANNEL_PLAN_MKK				0x5
#define		EEPROM_CHANNEL_PLAN_MKK1			0x6
#define		EEPROM_CHANNEL_PLAN_ISRAEL			0x7
#define		EEPROM_CHANNEL_PLAN_TELEC			0x8
#define		EEPROM_CHANNEL_PLAN_GLOBAL_DOMAIN	0x9
#define		EEPROM_CHANNEL_PLAN_WORLD_WIDE_13	0xA
#define		EEPROM_CHANNEL_PLAN_BY_HW_MASK	0x80

#define 	EEPROM_CID_DEFAULT				0x0
#define 	EEPROM_CID_ALPHA				0x1
#define 	EEPROM_CID_TOSHIBA				0x4
#define 	EEPROM_CID_WHQL 				0xFE 
#endif


#define		FW_DIG_DISABLE				0xfd00cc00
#define		FW_DIG_ENABLE					0xfd000000
#define		FW_DIG_HALT					0xfd000001
#define		FW_DIG_RESUME					0xfd000002
#define		FW_HIGH_PWR_DISABLE			0xfd000008
#define		FW_HIGH_PWR_ENABLE			0xfd000009
#define		FW_ADD_A2_ENTRY				0xfd000016
#define		FW_TXPWR_TRACK_ENABLE		0xfd000017
#define		FW_TXPWR_TRACK_DISABLE		0xfd000018
#define		FW_TXPWR_TRACK_THERMAL		0xfd000019
#define		FW_TXANT_SWITCH_ENABLE		0xfd000023
#define		FW_TXANT_SWITCH_DISABLE		0xfd000024
#define		FW_RA_INIT						0xfd000026 
#define		FW_CTRL_DM_BY_DRIVER			0Xfd00002a 
#define		FW_RA_IOT_BG_COMB			0xfd000030 
#define		FW_RA_IOT_N_COMB				0xfd000031 
#define		FW_RA_REFRESH					0xfd0000a0
#define		FW_RA_UPDATE_MASK				0xfd0000a2
#define		FW_RA_DISABLE					0xfd0000a4 
#define		FW_RA_ACTIVE					0xfd0000a6
#define		FW_RA_DISABLE_RSSI_MASK		0xfd0000ac 
#define		FW_RA_ENABLE_RSSI_MASK		0xfd0000ad 
#define		FW_RA_RESET					0xfd0000af
#define		FW_DM_DISABLE					0xfd00aa00
#define		FW_IQK_ENABLE					0xf0000020
#define		FW_IQK_SUCCESS				0x0000dddd
#define		FW_IQK_FAIL					0x0000ffff
#define		FW_OP_FAILURE					0xffffffff
#define		FW_TX_FEEDBACK_NONE			0xfb000000					
#define		FW_TX_FEEDBACK_DTM_ENABLE	(FW_TX_FEEDBACK_NONE | 0x1)	
#define		FW_TX_FEEDBACK_CCX_ENABLE	(FW_TX_FEEDBACK_NONE | 0x2) 
#define		FW_BB_RESET_ENABLE			0xff00000d
#define		FW_BB_RESET_DISABLE			0xff00000e
#define		FW_CCA_CHK_ENABLE				0xff000011		
#define		FW_CCK_RESET_CNT				0xff000013		
#define		FW_LPS_ENTER					0xfe000010
#define		FW_LPS_LEAVE					0xfe000011
#define		FW_INDIRECT_READ				0xf2000000
#define		FW_INDIRECT_WRITE				0xf2000001
#define		FW_CHAN_SET					0xf3000001 







#define RFPC					0x5F			
#define RCR_9356SEL			BIT6
#define TCR_LRL_OFFSET		0
#define TCR_SRL_OFFSET		8
#define TCR_MXDMA_OFFSET	21
#define TCR_SAT				BIT24		
#define RCR_MXDMA_OFFSET	8
#define RCR_FIFO_OFFSET		13
#define RCR_OnlyErlPkt		BIT31				
#define CWR					0xDC			
#define RetryCTR				0xDE			


#define		LED1Cfg				UnusedRegister	
#define 	LED0Cfg				UnusedRegister	
#define 	GPI					UnusedRegister	
#define 	BRSR				UnusedRegister	
#define 	CPU_GEN				UnusedRegister	
#define 	SIFS				UnusedRegister	

#define 	CPU_GEN_SYSTEM_RESET		0x00000001




#define		CCX_COMMAND_REG			0x890	
#define		CLM_PERIOD_REG			0x894	
#define		NHM_PERIOD_REG			0x896	
#define		NHM_THRESHOLD0			0x898	
#define		NHM_THRESHOLD1			0x899	
#define		NHM_THRESHOLD2			0x89A	
#define		NHM_THRESHOLD3			0x89B	
#define		NHM_THRESHOLD4			0x89C	
#define		NHM_THRESHOLD5			0x89D	
#define		NHM_THRESHOLD6			0x89E	
#define		CLM_RESULT_REG			0x8D0	
#define		NHM_RESULT_REG			0x8D4	
#define		NHM_RPI_COUNTER0		0x8D8	
#define		NHM_RPI_COUNTER1		0x8D9	
#define		NHM_RPI_COUNTER2		0x8DA	      
#define		NHM_RPI_COUNTER3		0x8DB	      
#define		NHM_RPI_COUNTER4		0x8DC	      
#define		NHM_RPI_COUNTER5		0x8DD	      
#define		NHM_RPI_COUNTER6		0x8DE	      
#define		NHM_RPI_COUNTER7		0x8DF	      


#define		HAL_8192S_HW_GPIO_OFF_BIT	BIT3
#define		HAL_8192S_HW_GPIO_OFF_MASK	0xF7
#define		HAL_8192S_HW_GPIO_WPS_BIT	BIT4

#endif 

