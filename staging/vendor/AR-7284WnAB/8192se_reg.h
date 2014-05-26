#ifndef _8192SE_REG_H_
#define _8192SE_REG_H_

//============================================================
//       8192S Regsiter offset definition
//============================================================

//
// MAC register 0x0 - 0x5xx
// 1. System configuration registers.
// 2. Command Control Registers
// 3. MACID Setting Registers
// 4. Timing Control Registers
// 5. FIFO Control Registers
// 6. Adaptive Control Registers
// 7. EDCA Setting Registers
// 8. WMAC, BA and CCX related Register.
// 9. Security Control Registers
// 10. Power Save Control Registers
// 11. General Purpose Registers
// 12. Host Interrupt Status Registers
// 13. Test Mode and Debug Control Registers
// 14. PCIE config register
//


//
// 1. System Configuration Registers	 (Offset: 0x0000 - 0x003F)
//
#define		SYS_ISO_CTRL		0x0000	// System Isolation Interface Control.
#define		SYS_FUNC_EN			0x0002	// System Function Enable.
#define		PMC_FSM				0x0004	// Power Sequence Control.
#define		SYS_CLKR			0x0008	// System Clock.
#define		CR9346				0x000A	// 93C46/93C56 Command Register.
#define		EE_VPD				0x000C	// EEPROM VPD Data.
#define		AFE_MISC			0x0010	// AFE Misc.
#define		SPS0_CTRL			0x0011	// Switching Power Supply 0 Control.
#define		SPS1_CTRL			0x0018	// Switching Power Supply 1 Control.
#define		RF_CTRL				0x001F	// RF Block Control.
#define		LDOA15_CTRL			0x0020	// V15 Digital LDO Control.
#define		LDOV12D_CTRL		0x0021	// V12 Digital LDO Control.
#define		LDOHCI12_CTRL		0x0022	// V12 Digital LDO Control. 
#define		LDO_USB_SDIO		0x0023	// LDO USB Control.
#define		LPLDO_CTRL			0x0024	// Low Power LDO Control.
#define		AFE_XTAL_CTRL		0x0026	// AFE Crystal Control.
#define		AFE_PLL_CTRL		0x0028	// System Function Enable.
#define		EFUSE_CTRL			0x0030	// E-Fuse Control.
#define		EFUSE_TEST			0x0034	// E-Fuse Test.
#define		PWR_DATA			0x0038	// Power on date.
#define		DBG_PORT			0x003A	// MAC debug port select
#define		DPS_TIMER			0x003C	// Deep Power Save Timer Register.
#define		RCLK_MON			0x003E	// Retention Clock Monitor.

//
// 2. Command Control Registers	 (Offset: 0x0040 - 0x004F)
//
#define		CMDR				0x0040	// MAC Command Register.
#define		TXPAUSE				0x0042	// Transmission Pause Register.
#define		LBKMD_SEL			0x0043	// Loopback Mode Select Register.
#define		TCR					0x0044	// Transmit Configuration Register
#define		RCR					0x0048	// Receive Configuration Register
#define		MSR					0x004C	// Media Status register
#define		SYSF_CFG			0x004D	// System Function Configuration.
#define		RX_PKY_LIMIT		0x004E	// RX packet length limit
#define		MBIDCTRL			0x004F	// MBSSID Control.

//
// 3. MACID Setting Registers	(Offset: 0x0050 - 0x007F)
//
#define		MACIDR				0x0050	// MAC ID Register, Offset 0x0050-0x0055
#define		MACIDR0				0x0050	// MAC ID Register, Offset 0x0050-0x0053
#define		MACIDR4				0x0054	// MAC ID Register, Offset 0x0054-0x0055
#define		BSSIDR				0x0058	// BSSID Register, Offset 0x0058-0x005D
#define		HWVID				0x005E	// HW Version ID.
#define		MAR					0x0060	// Multicase Address.
#define		MBIDCAMCONTENT		0x0068	// MBSSID CAM Content.
#define		MBIDCAMCFG		0x006F // 0x005E // 0x0070	// MBSSID CAM Configuration.
#define		BUILDTIME			0x0074	// Build Time Register.
#define		BUILDUSER			0x0078	// Build User Register.   
// Redifine MACID register
#define		IDR0				MACIDR0
#define		IDR4				MACIDR4

//
// 4. Timing Control Registers	(Offset: 0x0080 - 0x009F)
//
#define		TSFR				0x0080	// Timing Sync Function Timer Register.
#define		SLOT_TIME			0x0089	// Slot Time Register, in us.
#define		USTIME				0x008A	// EDCA/TSF clock unit time us unit.
//#define 	TUBASE				0x008A	// Time Unit Base Reg, default TU 1024us
#define		SIFS_CCK			0x008C	// SIFS for CCK, in us.
#define		SIFS_OFDM			0x008E	// SIFS for OFDM, in us.
#define		PIFS_TIME			0x0090	// PIFS time register.
#define		ACK_TIMEOUT			0x0091	// Ack Timeout Register
#define		EIFSTR				0x0092	// EIFS time regiser.
#define		BCN_INTERVAL		0x0094	// Beacon Interval, in TU.
#define		ATIMWND				0x0096	// ATIM Window width, in TU.
#define		BCN_DRV_EARLY_INT	0x0098	// Driver Early Interrupt 
#define		BCN_DMATIME			0x009A	// Beacon DMA and ATIM INT Time.
#define		BCN_ERR_THRESH		0x009C	// Beacon Error Threshold.
#define		MLT					0x009D	// MSDU Lifetime.
//#define		RSVD_MAC_TUNE_US	0x009E	// MAC Internal USE.
#define		MBID_BCN_SPACE	0x009E	// MBSSID beacon space register

//
// 5. FIFO Control Registers	 (Offset: 0x00A0 - 0x015F)
//
#define		RQPN1				0x00A0	// Reserved Que Page Num, Vo Vi, Be, Bk
#define		RQPN2				0x00A4	// RQPN, HCCA, Cmd, Mgnt, High
#define		RQPN3				0x00A8	// RQPN, Bcn, Public, 
#define		RQPN				RQPN1	//
#define		LD_RQPN				0x00AB  //
#define		RXFF_BNDY			0x00AC  //
#define		RXRPT_BNDY			0x00B0  //
#define		TXPKTBUF_PGBNDY		0x00B4  //
#define		PBP					0x00B5  //
#define		RXDRVINFO_SZ		0x00B6  //
#define		TXFF_STATUS			0x00B7  //
#define		RXFF_STATUS			0x00B8  //
#define		TXFF_EMPTY_TH		0x00B9  //
#define		SDIO_RX_BLKSZ		0x00BC  //
#define		RXDMA_				0x00BD  //
#define		RXPKT_NUM			0x00BE  //
#define		C2HCMD_UDT_SIZE		0x00C0  //
#define		C2HCMD_UDT_ADDR		0x00C2  //
#define		FIFOPAGE1			0x00C4  //PUB/BCN
#define		FIFOPAGE2			0x00C8  //
#define		FIFOPAGE3			0x00CC  //
#define		FIFOPAGE4			0x00D0  //
#define		FIFOPAGE5			0x00D4  //
#define		FW_RSVD_PG_CRTL		0x00D8  //
#define		RXDMA_AGG_PG_TH		0x00D9  //
#define		TXRPTFF_RDPTR		0x00E0  //
#define		TXRPTFF_WTPTR		0x00E4  //
#define		C2HFF_RDPTR			0x00E8	//FIFO Read pointer register.
#define		C2HFF_WTPTR			0x00EC	//FIFO Write pointer register.
#define		RXFF0_RDPTR			0x00F0	//
#define		RXFF0_WTPTR			0x00F4  //
#define		RXFF1_RDPTR			0x00F8  //
#define		RXFF1_WTPTR			0x00FC  //
#define		RXRPT0_RDPTR		0x0100  //
#define		RXRPT0_WTPTR		0x0104  //
#define		RXRPT1_RDPTR		0x0108  //
#define		RXRPT1_WTPTR		0x010C  //
#define		RX0_UDT_SIZE		0x0110  //
#define		RX1PKTNUM			0x0114  //
#define		RXFILTERMAP			0x0116  //
#define		RXFILTERMAP_GP1		0x0118  //
#define		RXFILTERMAP_GP2		0x011A  //
#define		RXFILTERMAP_GP3		0x011C  //
#define		BCNQ_CTRL			0x0120  //
#define		MGTQ_CTRL			0x0124  //
#define		HIQ_CTRL			0x0128  //
#define		VOTID7_CTRL			0x012c  //
#define		VOTID6_CTRL			0x0130  //
#define		VITID5_CTRL			0x0134  //
#define		VITID4_CTRL			0x0138  //
#define		BETID3_CTRL			0x013c  //
#define		BETID0_CTRL			0x0140  //
#define		BKTID2_CTRL			0x0144  //
#define		BKTID1_CTRL			0x0148  //
#define		CMDQ_CTRL			0x014c  //                                          
#define		TXPKT_NUM_CTRL		0x0150  //
#define		TXQ_PGADD			0x0152  //
#define		TXFF_PG_NUM			0x0154  //
#define		TRXDMA_STATUS		0x0156  //

//
// 6. Adaptive Control Registers  (Offset: 0x0160 - 0x01CF)
//
#define		INIMCS_SEL			0x0160	// Init MCSrate for 32 MACID 0x160-17f
#define		INIRTSMCS_SEL		0x0180	// Init RTSMCSrate 
#define		RRSR				0x0181	// Response rate setting.
#define		ARFR0				0x0184	// Auto Rate Fallback 0 Register.
#define		ARFR1				0x0188	//
#define		ARFR2				0x018C  //
#define		ARFR3				0x0190  //
#define		ARFR4				0x0194  //
#define		ARFR5				0x0198  //
#define		ARFR6				0x019C  //
#define		ARFR7				0x01A0  //
#define		AGGLEN_LMT_H		0x01A7	// Aggregation Length Limit for High-MCS
#define		AGGLEN_LMT_L		0x01A8	// Aggregation Length Limit for Low-MCS.
#define		DARFRC				0x01B0	// Data Auto Rate Fallback Retry Count.
#define		RARFRC				0x01B8	// Response Auto Rate Fallback Count.
#define		MCS_TXAGC			0x01C0
#define		CCK_TXAGC			0x01C8

//
// 7. EDCA Setting Registers	 (Offset: 0x01D0 - 0x01FF)
//
#define		EDCAPARA_VO 		0x01D0	// EDCA Parameter Register for VO queue.
#define		EDCAPARA_VI			0x01D4	// EDCA Parameter Register for VI queue.
#define		EDCAPARA_BE			0x01D8	// EDCA Parameter Register for BE queue.
#define		EDCAPARA_BK			0x01DC	// EDCA Parameter Register for BK queue.
#define		BCNTCFG				0x01E0	// Beacon Time Configuration Register.
#define		CWRR				0x01E2	// Contention Window Report Register.
#define		ACMAVG				0x01E4	// ACM Average Register.
#define		ACMHWCTRL			0x01E7	
#define		VO_ADMTM			0x01E8	// Admission Time Register.
#define		VI_ADMTM			0x01EC
#define		BE_ADMTM			0x01F0
#define		RETRY_LIMIT			0x01F4	// Retry Limit [15:8]-short, [7:0]-long
#define		SG_RATE				0x01F6	// Set the hightst SG rate

//
// 8. WMAC, BA and CCX related Register.	 (Offset: 0x0200 - 0x023F)
//
#define		NAV_CTRL			0x0200
#define		BW_OPMODE			0x0203
#define		BACAMCMD			0x0204
#define		BACAMCONTENT		0x0208	// Block ACK Content Register.
// Roger had defined the 0x2xx register WMAC definition
#define		BACAMRW				0x0204	// BA CAM R/W Reg
#define		BACAMCon			0x0208	// BA CAM Content Reg
#define		LBDLY				0x0210	// Loopback Delay Register.
#define		FWDLY				0x0211	// FW Delay Register.
#define		HWPC_RX_CTRL		0x0218	// HW Packet Conversion RX Control Reg
#define		MQIR				0x0220	// Mesh Qos Type Indication Register.
#define		MAIR				0x0222	// Mesh ACK.
#define		MSIR				0x0224	// Mesh HW Security Req Indication Reg
#define		CLM_RESULT			0x0227	// CCA Busy Fraction(Channel Load)
#define		NHM_RPI_CNT			0x0228	// NHM RPI Report.
#define		RXERR_RPT			0x0230	// Rx Error Report.
#define		NAV_PROT_LEN		0x0234	// NAV Protection Length.
#define		CFEND_TH			0x0236	// CF-End Threshold.
#define		AMPDU_MIN_SPACE		0x0237	// AMPDU Min Space.
#define		TXOP_STALL_CTRL		0x0238

//
// 9. Security Control Registers	(Offset: 0x0240 - 0x025F)
//
#define		RWCAM				0x0240	//IN 8190 Data Sheet is called CAMcmd
#define		WCAMI				0x0244	// Software write CAM input content
#define		RCAMO				0x0248	// Software read/write CAM config
#define		CAMDBG				0x024C
#define		SECR				0x0250	//Security Configuration Register

//
// 10. Power Save Control Registers	 (Offset: 0x0260 - 0x02DF)
//
#define		WOW_CTRL			0x0260	//Wake On WLAN Control.
#define		PSSTATUS			0x0261	// Power Save Status.
#define		PSSWITCH			0x0262	// Power Save Switch.
#define		MIMOPS_WAIT_PERIOD	0x0263
#define		LPNAV_CTRL			0x0264
#define		WFM0				0x0270	// Wakeup Frame Mask.
#define		WFM1				0x0280	//
#define		WFM2				0x0290  //
#define		WFM3				0x02A0  //
#define		WFM4				0x02B0  //
#define		WFM5				0x02C0  //

// The following two definition are only used for USB interface.
#define		RF_BB_CMD_ADDR		0x02c0	// RF/BB read/write command address.
#define		RF_BB_CMD_DATA		0x02c4	// RF/BB read/write command data.

#define		WFCRC				0x02D0	// Wakeup Frame CRC.
//#define		RPWM				0x02DC	// Host Request Power Mode.
#define		RPWM				PCI_RPWM
//#define		CPWM				0x02DD	// Current Power Mode.

//
// 11. General Purpose Registers	(Offset: 0x02E0 - 0x02FF)
//
#define		PSTIME				0x02E0  // Power Save Timer Register
#define		TIMER0				0x02E4  // 
#define		TIMER1				0x02E8  // 
#define		GPIO_CTRL			0x02EC  // 
#define		MAC_PINMUX_CFG		0x02F0  // MAC PINMUX Configuration Reg[15:0]
#define		LEDCFG				0x02F2  // System PINMUX Configuration Reg[7:0]
#define		PHY_REG				0x02F3  // RPT: PHY REG Access Report Reg[7:0]
#define		PHY_REG_DATA		0x02F4  // PHY REG Read DATA Register [31:0]
#define		EFUSE_CLK			0x02F8  // CTRL: E-FUSE Clock Control Reg[7:0]
#define		GPIO_INTCTRL		0x02F9  // GPIO Interrupt Control Register[7:0]

//
// 12. Host Interrupt Status Registers	 (Offset: 0x0300 - 0x030F)
//
#define		IMR					0x0300	// Interrupt Mask Register
#define		ISR					0x0308	// Interrupt Status Register

//
// 13. Test Mode and Debug Control Registers	(Offset: 0x0310 - 0x034F)
//
#define		DBG_PORT_SWITCH		0x003A
#define		BIST				0x0310	// Bist reg definition
#define		DBS					0x0314	// Debug Select ???
#define		CPUINST				0x0318	// CPU Instruction Read Register
#define		CPUCAUSE			0x031C	// CPU Cause Register
#define		LBUS_ERR_ADDR		0x0320	// Lexra Bus Error Address Register
#define		LBUS_ERR_CMD		0x0324	// Lexra Bus Error Command Register
#define		LBUS_ERR_DATA_L		0x0328	// Lexra Bus Error Data Low DW Register
#define		LBUS_ERR_DATA_H		0x032C	// 
#define		LX_EXCEPTION_ADDR	0x0330	// Lexra Bus Exception Address Register
#define		WDG_CTRL			0x0334	// Watch Dog Control Register
#define		INTMTU				0x0338	// Interrupt Mitigation Time Unit Reg
#define		INTM				0x033A	// Interrupt Mitigation Register
#define		FDLOCKTURN0			0x033C	// FW/DRV Lock Turn 0 Register
#define		FDLOCKTURN1			0x033D	// FW/DRV Lock Turn 1 Register
//#define		FDLOCKTURN0			0x033E	// FW/DRV Lock Turn 0 Register
//#define		FDLOCKTURN1			0x033F	// FW/DRV Lock Turn 1 Register
#define		TRXPKTBUF_DBG_DATA	0x0340	// TRX Packet Buffer Debug Data Register
#define		TRXPKTBUF_DBG_CTRL	0x0348	// TRX Packet Buffer Debug Control Reg
#define		DPLL				0x034A	// DPLL Monitor Register [15:0]
#define		CBUS_ERR_ADDR		0x0350	// CPU Bus Error Address Register
#define		CBUS_ERR_CMD		0x0354	// CPU Bus Error Command Register
#define		CBUS_ERR_DATA_L		0x0358	// CPU Bus Error Data Low DW Register
#define		CBUS_ERR_DATA_H 	0x035C	// 
#define		USB_SIE_INTF_ADDR	0x0360	// USB SIE Access Interface Address Reg
#define		USB_SIE_INTF_WD		0x0361	// USB SIE Access Interface WData Reg
#define		USB_SIE_INTF_RD		0x0362	// USB SIE Access Interface RData Reg
#define		USB_SIE_INTF_CTRL	0x0363	// USB SIE Access Interface Control Reg

// Boundary is 0x37F

//
// 14. PCIE config register	(Offset 0x500-)
//
#define		TPPoll				0x0500	// Transmit Polling
#define		PM_CTRL				0x0502	// PCIE power management control Reg
#define		PCIF				0x0503	// PCI Function Register 0x0009h~0x000bh

#define		THPDA				0x0514	// Transmit High Priority Desc Addr
#define		TCDA				0x051C	// Transmit Command Desc Addr
#define		TMDA				0x0518	// Transmit Management Desc Addr
#define		HDA					0x0520	// HCCA Desc Addr
#define		TVODA				0x0524	// Transmit VO Desc Addr
#define		TVIDA				0x0528	// Transmit VI Desc Addr
#define		TBEDA				0x052C	// Transmit BE Desc Addr
#define		TBKDA				0x0530	// Transmit BK Desc Addr
#define		TBDA				0x0534	// Transmit Beacon Desc Addr
#define		RCDA				0x0538	// Receive Command Desc Addr
#define		RDSA				0x053C	// Receive Desc Starting Addr
#define		DBI_WDATA			0x0540	// DBI write data Register
#define		DBI_RDATA			0x0544	// DBI read data Register
#define		DBI_CTRL			0x0548	// PCIE DBI control Register
#define		MDIO_DATA			0x0550	// PCIE MDIO data Register
#define		MDIO_CTRL			0x0554	// PCIE MDIO control Register
#define		PCI_RPWM			0x0561	// PCIE RPWM register

// Config register	(Offset 0x800-)
//
#define		PHY_CCA			0x803	// CCA related register

//============================================================
// CCX Related Register
//============================================================
#define		CCX_COMMAND_REG			0x890	
// CCX Measurement Command Register. 4 Bytes.
// Bit[0]: R_CLM_En, 1=enable, 0=disable. Enable or disable "Channel Load 
// Measurement (CLM)".
// Bit[1]: R_NHM_En, 1=enable, 0=disable. Enable or disalbe "Noise Histogram 
// Measurement (NHM)".
// Bit[2~7]: Reserved
// Bit[8]: R_CCX_En: 1=enable, 0=disable. Enable or disable CCX function. 
// Note: After clearing this bit, all the result of all NHM_Result and CLM_
// Result are cleared concurrently.
// Bit[9]: R_Ignore_CCA: 1=enable, 0=disable. Enable means that treat CCA 
// period as idle time for NHM.
// Bit[10]: R_Ignore_TXON: 1=enable, 0=disable. Enable means that treat TXON 
// period as idle time for NHM.
// Bit[11~31]: Reserved.
#define		CLM_PERIOD_REG			0x894	
// CCX Measurement Period Register, in unit of 4us. 2 Bytes.
#define		NHM_PERIOD_REG			0x896	
// Noise Histogram Measurement Period Register, in unit of 4us. 2Bytes. 
#define		NHM_THRESHOLD0			0x898	// Noise Histogram Meashorement0
#define		NHM_THRESHOLD1			0x899	// Noise Histogram Meashorement1
#define		NHM_THRESHOLD2			0x89A	// Noise Histogram Meashorement2
#define		NHM_THRESHOLD3			0x89B	// Noise Histogram Meashorement3
#define		NHM_THRESHOLD4			0x89C	// Noise Histogram Meashorement4
#define		NHM_THRESHOLD5			0x89D	// Noise Histogram Meashorement5
#define		NHM_THRESHOLD6			0x89E	// Noise Histogram Meashorement6
#define		CLM_RESULT_REG			0x8D0	
// Channel Load result register. 4 Bytes.
// Bit[0~15]: Total measured duration of CLM. The CCA busy fraction is caculate 
// by CLM_RESULT_REG/CLM_PERIOD_REG.
// Bit[16]: Indicate the CLM result is ready.
// Bit[17~31]: Reserved.
#define		NHM_RESULT_REG			0x8D4	
// Noise Histogram result register. 4 Bytes.
// Bit[0~15]: Total measured duration of NHM. If R_Ignore_CCA=1 or 
// R_Ignore_TXON=1, this value will be less than NHM_PERIOD_REG.
// Bit[16]: Indicate the NHM result is ready.
// Bit[17~31]: Reserved.
#define		NHM_RPI_COUNTER0		0x8D8	
// NHM RPI counter0, the fraction of signal strength < NHM_THRESHOLD0. 
#define		NHM_RPI_COUNTER1		0x8D9	
// NHM RPI counter1, the fraction of signal stren in NHM_THRESH0, NHM_THRESH1 
#define		NHM_RPI_COUNTER2		0x8DA	      
// NHM RPI counter2, the fraction of signal stren in NHM_THRESH2, NHM_THRESH3
#define		NHM_RPI_COUNTER3		0x8DB	      
// NHM RPI counter3, the fraction of signal stren in NHM_THRESH4, NHM_THRESH5
#define		NHM_RPI_COUNTER4		0x8DC	      
// NHM RPI counter4, the fraction of signal stren in NHM_THRESH6, NHM_THRESH7
#define		NHM_RPI_COUNTER5		0x8DD	      
// NHM RPI counter5, the fraction of signal stren in NHM_THRESH8, NHM_THRESH9
#define		NHM_RPI_COUNTER6		0x8DE	      
// NHM RPI counter6, the fraction of signal stren in NHM_THRESH10, NHM_THRESH11 
#define		NHM_RPI_COUNTER7		0x8DF	      
// NHM RPI counter7, the fraction of signal stren in NHM_THRESH12, NHM_THRESH13

//============================================================================
//       8190 Regsiter offset definition
//============================================================================
#define		AFR					0x010	// AutoLoad Function Register
#define		BCN_TCFG			0x062	// Beacon Time Configuration
#define		RATR0				0x320	// Rate Adaptive Table register1
// TODO: Remove unused register, We must declare backward compatiable
//Undefined register set in 8192S. 0x320/350 DW is useless
#define		UnusedRegister		0x0320		
#define		PSR					UnusedRegister	// Page Select Register
//Security Related                              
#define		DCAM				UnusedRegister	// Debug CAM Interface
//PHY Configuration related
#define		BBAddr				UnusedRegister	// Phy register address register
#define		PhyDataR			UnusedRegister	// Phy register data read       		
#define		UFWP				UnusedRegister


//============================================================================
//       8192S Regsiter Bit and Content definition 
//============================================================================

//
// 1. System Configuration Registers	 (Offset: 0x0000 - 0x003F)
//
//----------------------------------------------------------------------------
//       8192S SYS_ISO_CTRL bits					(Offset 0x0, 16bit)
//----------------------------------------------------------------------------
#define		ISO_PWC_RV2RP		BIT(12)	// LPLDOR12 to Retenrion Path 
										// 1: isolation, 0: attach.
#define		ISO_PWC_DV2RP		BIT(11)	// Digital Vdd to Retention Path 
										// 1: isolation, 0: attach
#define		ISO_PLL2MD			BIT(4)	// AFE PLL to MACTOP/BB/PCIe Digital.
#define		ISO_PA2PCIE			BIT(3)	// PCIe Analog 1.2V to PCIe 3.3V
#define		ISO_MD2PP			BIT(0)	// MACTOP/BB/PCIe Digital to Power On.

//----------------------------------------------------------------------------
//       8192S SYS_FUNC_EN bits					(Offset 0x2, 16bit)
//----------------------------------------------------------------------------
#define		FEN_MREGEN			BIT(15)	// MAC I/O Registers Enable.
#define		FEN_DCORE			BIT(11)	// Enable Core Digital.
#define		FEN_CPUEN			BIT(10)	// Enable CPU Core Digital.

//----------------------------------------------------------------------------
//       8192S SYS_CLKR bits					(Offset 0x8, 16bit)
//----------------------------------------------------------------------------
#define		SYS_FWHW_SEL		BIT(15)	// Sleep exit, control path swith. 
#define		SYS_SWHW_SEL		BIT(14)	// Load done, control path seitch.
#define		SYS_CPU_CLKSEL		BIT(2)	// System Clock select, 
										// 1: AFE source, 0: System clock(L-Bus)
#define		SYS_MAC_CLK_EN		BIT(11)	// MAC Clock Enable.
#define		SYS_CLKSEL_80M		BIT(0)	// System Clock 80MHz

//----------------------------------------------------------------------------
//       8192S Cmd9346CR bits					(Offset 0xA, 16bit)
//----------------------------------------------------------------------------
#define		Cmd9346CR_9356SEL					BIT(4)
#ifdef RTL8192SU_EFUSE
#define		CmdEEPROM_En						BIT(5)	 // EEPROM enable when set 1
#define		CmdEERPOMSEL						BIT(4) // System EEPROM select, 0: boot from E-FUSE, 1: The EEPROM used is 9346
#define		AutoLoadEEPROM						(CmdEEPROM_En|CmdEERPOMSEL)
#define		AutoLoadEFUSE						CmdEEPROM_En
#endif

//----------------------------------------------------------------------------
//       8192S AFE_MISC bits		AFE Misc			(Offset 0x10, 8bits)
//----------------------------------------------------------------------------
#define		AFE_MBEN			BIT(1)	// Enable AFE Macro Block's Mbias.
#define		AFE_BGEN			BIT(0)	// Enable AFE Macro Block's Bandgap.

//----------------------------------------------------------------------------
//       8192S SPS1_CTRL bits					(Offset 0x18-1E, 56bits)
//----------------------------------------------------------------------------
#define		SPS1_SWEN			BIT(1)	// Enable vsps18 SW Macro Block.
#define		SPS1_LDEN			BIT(0)	// Enable VSPS12 LDO Macro block. 

//----------------------------------------------------------------------------
//       8192S LDOA15_CTRL bits					(Offset 0x20, 8bits)
//----------------------------------------------------------------------------
#define		LDA15_EN			BIT(0)	// Enable LDOA15 Macro Block

//----------------------------------------------------------------------------
//       8192S AFE_XTAL_CTRL bits	AFE Crystal Control.	(Offset 0x26,16bits)
//----------------------------------------------------------------------------
#define		XTAL_GATE_AFE		BIT(10)	
// Gated Control. 1: AFE Clock source gated, 0: Clock enable.

//----------------------------------------------------------------------------
//       8192S AFE_PLL_CTRL bits	System Function Enable	(Offset 0x28,64bits)
//----------------------------------------------------------------------------
#define		APLL_EN				BIT(0)	// Enable AFE PLL Macro Block.

// Find which card bus type 
#define		AFR_CardBEn			BIT(0)
#define		AFR_CLKRUN_SEL		BIT(1)
#define		AFR_FuncRegEn		BIT(2)

//
// 2. Command Control Registers	 (Offset: 0x0040 - 0x004F)
//
//----------------------------------------------------------------------------
//       8192S (CMD) command register bits		(Offset 0x40, 16 bits)
//----------------------------------------------------------------------------
#define		APSDOFF_STATUS		BIT(15)	//
#define		APSDOFF				BIT(14)   //
#define		BBRSTn				BIT(13)   //Enable OFDM/CCK
#define		BB_GLB_RSTn			BIT(12)   //Enable BB
#define		SCHEDULE_EN			BIT(10)   //Enable MAC scheduler
#define		MACRXEN				BIT(9)    //
#define		MACTXEN				BIT(8)    //
#define		DDMA_EN				BIT(7)    //FW off load function enable
#define		FW2HW_EN			BIT(6)    //MAC every module reset as below
#define		RXDMA_EN			BIT(5)    //
#define		TXDMA_EN			BIT(4)    //
#define		HCI_RXDMA_EN		BIT(3)    //
#define		HCI_TXDMA_EN		BIT(2)    //

//----------------------------------------------------------------------------
//       8192S (TXPAUSE) transmission pause		(Offset 0x42, 8 bits)
//----------------------------------------------------------------------------
#define		StopHCCA			BIT(6)
#define		StopHigh			BIT(5)
#define		StopMgt				BIT(4)
#define		StopVO				BIT(3)
#define		StopVI				BIT(2)
#define		StopBE				BIT(1)
#define		StopBK				BIT(0)

//----------------------------------------------------------------------------
//       8192S (LBKMD) LoopBack Mode Select 		(Offset 0x43, 8 bits)
//----------------------------------------------------------------------------
/*
[3] no buffer, 1: no delay, 0: delay; [2] dmalbk, [1] no_txphy, [0] diglbk.
0000: Normal
1011: MAC loopback (involving CPU)
0011: MAC Delay Loopback
0001: PHY loopback (not yet implemented)
0111: DMA loopback (only uses TxPktBuffer and DMA engine)
All other combinations are reserved.
Default: 0000b.
*/
#define		LBK_NORMAL		0x00
#define		LBK_MAC_LB		(BIT(0)|BIT(1)|BIT(3))
#define		LBK_MAC_DLB		(BIT(0)|BIT(1))
#define		LBK_DMA_LB		(BIT(0)|BIT(1)|BIT(2))	

//----------------------------------------------------------------------------
//       8192S (TCR) transmission Configuration Register (Offset 0x44, 32 bits)
//----------------------------------------------------------------------------
#define		TCP_OFDL_EN				BIT(25)	//For CE packet conversion
#define		HWPC_TX_EN				BIT(24)   //""	
#define		TXDMAPRE2FULL			BIT(23)   //TXDMA enable pre2full sync
#define		DISCW					BIT(20)   //CW disable
#define		TCRICV					BIT(19)   //Append ICV or not
#define		CfendForm				BIT(17)   //AP mode 
#define		TCRCRC					BIT(16)   //Append CRC32
#define		FAKE_IMEM_EN			BIT(15)   //
#define		TSFRST					BIT(9)    //
#define		TSFEN					BIT(8)    //
// For TCR FW download ready --> write by FW  Bit0-7 must all one
#define		FWALLRDY				(BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(4)|BIT(5)|BIT(6)|BIT(7))
#define		FWRDY					BIT(7)
#define		BASECHG					BIT(6)
#define		IMEM					BIT(5)
#define		DMEM_CODE_DONE			BIT(4)
#define		EXT_IMEM_CHK_RPT		BIT(3)
#define		EXT_IMEM_CODE_DONE		BIT(2)
#define		IMEM_CHK_RPT			BIT(1)
#define		IMEM_CODE_DONE			BIT(0)
// Copy fomr 92SU definition
#define		IMEM_CODE_DONE		BIT(0)	
#define		IMEM_CHK_RPT		BIT(1)
#define		EMEM_CODE_DONE		BIT(2)
#define		EMEM_CHK_RPT		BIT(3)
#define		DMEM_CODE_DONE		BIT(4)
#define		IMEM_RDY			BIT(5)
#define		BASECHG				BIT(6)
#define		FWRDY				BIT(7)
#define		LOAD_FW_READY		(IMEM_CODE_DONE|IMEM_CHK_RPT|EMEM_CODE_DONE|\
								EMEM_CHK_RPT|DMEM_CODE_DONE|IMEM_RDY|BASECHG|\
								FWRDY)
#define		TCR_TSFEN			BIT(8)		// TSF function on or off.
#define		TCR_TSFRST			BIT(9)		// Reset TSF function to zero.
#define		TCR_FAKE_IMEM_EN	BIT(15)
#define		TCR_CRC				BIT(16)
#define		TCR_ICV				BIT(19)	// Integrity Check Value.
#define		TCR_DISCW			BIT(20)	// Disable Contention Windows Backoff.
#define		TCR_HWPC_TX_EN		BIT(24)
#define		TCR_TCP_OFDL_EN		BIT(25)

//----------------------------------------------------------------------------
//       8192S (RCR) Receive Configuration Register	(Offset 0x48, 32 bits)
//----------------------------------------------------------------------------
#define		RCR_APPFCS			BIT(31)		//WMAC append FCS after pauload
#define		RCR_DIS_ENC_2BYTE	BIT(30)       //HW encrypt 2 or 1 byte mode
#define		RCR_DIS_AES_2BYTE	BIT(29)       //
#define		RCR_HTC_LOC_CTRL	BIT(28)       //MFC<--HTC=1 MFC-->HTC=0
#define		RCR_ENMBID			BIT(27)		//Enable Multiple BssId.
#define		RCR_RX_TCPOFDL_EN	BIT(26)		//
#define		RCR_APP_PHYST_RXFF	BIT(25)       //
#define		RCR_APP_PHYST_STAFF	BIT(24)       //
#define		RCR_CBSSID			BIT(23)		//Accept BSSID match packet
#define		RCR_APWRMGT			BIT(22)		//Accept power management packet
#define		RCR_ADD3			BIT(21)		//Accept address 3 match packet
#define		RCR_AMF				BIT(20)		//Accept management type frame
#define		RCR_ACF				BIT(19)		//Accept control type frame
#define		RCR_ADF				BIT(18)		//Accept data type frame
#define		RCR_APP_MIC			BIT(17)		//
#define		RCR_APP_ICV			BIT(16)       //
#define		RCR_RXFTH			BIT(13)		//Rx FIFO Threshold Bot 13 - 15
#define		RCR_AICV			BIT(12)		//Accept ICV error packet
#define		RCR_RXDESC_LK_EN	BIT(11)		//Accept to update rx desc length
#define		RCR_APP_BA_SSN		BIT(6)		//Accept BA SSN
#define		RCR_ACRC32			BIT(5)		//Accept CRC32 error packet 
#define		RCR_RXSHFT_EN		BIT(4)		//Accept broadcast packet 
#define		RCR_AB				BIT(3)		//Accept broadcast packet 
#define		RCR_AM				BIT(2)		//Accept multicast packet 
#define		RCR_APM				BIT(1)		//Accept physical match packet
#define		RCR_AAP				BIT(0)		//Accept all unicast packet 
#define		RCR_MXDMA_OFFSET	8
#define		RCR_FIFO_OFFSET		13

//----------------------------------------------------------------------------
//       8192S (MSR) Media Status Register	(Offset 0x4C, 8 bits)  
//----------------------------------------------------------------------------
/*
Network Type
00: No link
01: Link in ad hoc network
10: Link in infrastructure network
11: AP mode
Default: 00b.
*/
#define		MSR_NOLINK				0x00
#define		MSR_ADHOC				0x01
#define		MSR_INFRA				0x02
#define		MSR_AP					0x03

//----------------------------------------------------------------------------
//       8192S (SYSF_CFG) system Fucntion Config Reg	(Offset 0x4D, 8 bits)  
//----------------------------------------------------------------------------
#define		ENUART					BIT(7)
#define		ENJTAG					BIT(3)
#define		BTMODE					(BIT(2)|BIT(1))
#define		ENBT					BIT(0)

//----------------------------------------------------------------------------
//       8192S (MBIDCTRL) MBSSID Control Register	(Offset 0x4F, 8 bits)  
//----------------------------------------------------------------------------
#define		ENMBID					BIT(7)
#define		BCNUM					(BIT(6)|BIT(5)|BIT(4))

//
// 3. MACID Setting Registers	(Offset: 0x0050 - 0x007F)
//

//
// 4. Timing Control Registers	(Offset: 0x0080 - 0x009F)
//
//----------------------------------------------------------------------------
//       8192S (USTIME) US Time Tunning Register	(Offset 0x8A, 16 bits)  
//----------------------------------------------------------------------------
#define		USTIME_EDCA				0xFF00
#define		USTIME_TSF				0x00FF

//----------------------------------------------------------------------------
//       8192S (SIFS_CCK/OFDM) US Time Tunning Register	(Offset 0x8C/8E,16 bits)  
//----------------------------------------------------------------------------
#define		SIFS_TRX				0xFF00
#define		SIFS_CTX				0x00FF

//----------------------------------------------------------------------------
//       8192S (DRVERLYINT)	Driver Early Interrupt Reg		(Offset 0x98, 16bit)
//----------------------------------------------------------------------------
#define		ENSWBCN					BIT(15)
#define		DRVERLY_TU				0x0FF0
#define		DRVERLY_US				0x000F
#define		BCN_TCFG_CW_SHIFT		8
#define		BCN_TCFG_IFS			0	

//
// 5. FIFO Control Registers	 (Offset: 0x00A0 - 0x015F)
//

//
// 6. Adaptive Control Registers  (Offset: 0x0160 - 0x01CF)
//
//----------------------------------------------------------------------------
//       8192S Response Rate Set Register	(offset 0x181, 24bits)
//----------------------------------------------------------------------------
#define		RRSR_RSC_OFFSET			21
#define		RRSR_SHORT_OFFSET		23
#define		RRSR_RSC_DUPLICATE		0x600000
#define		RRSR_RSC_UPSUBCHNL		0x400000
#define		RRSR_RSC_LOWSUBCHNL		0x200000
#define		RRSR_SHORT				0x800000
#define		RRSR_1M					BIT(0)
#define		RRSR_2M					BIT(1) 
#define		RRSR_5_5M				BIT(2) 
#define		RRSR_11M				BIT(3) 
#define		RRSR_6M					BIT(4) 
#define		RRSR_9M					BIT(5) 
#define		RRSR_12M				BIT(6) 
#define		RRSR_18M				BIT(7) 
#define		RRSR_24M				BIT(8) 
#define		RRSR_36M				BIT(9) 
#define		RRSR_48M				BIT(10) 
#define		RRSR_54M				BIT(11)
#define		RRSR_MCS0				BIT(12)
#define		RRSR_MCS1				BIT(13)
#define		RRSR_MCS2				BIT(14)
#define		RRSR_MCS3				BIT(15)
#define		RRSR_MCS4				BIT(16)
#define		RRSR_MCS5				BIT(17)
#define		RRSR_MCS6				BIT(18)
#define		RRSR_MCS7				BIT(19)
#define		BRSR_AckShortPmb		BIT(23)	
// CCK ACK: use Short Preamble or not

//----------------------------------------------------------------------------
//       8192S Rate Definition
//----------------------------------------------------------------------------
//CCK
#define		RATR_1M					0x00000001
#define		RATR_2M					0x00000002
#define		RATR_55M				0x00000004
#define		RATR_11M				0x00000008
//OFDM 		
#define		RATR_6M					0x00000010
#define		RATR_9M					0x00000020
#define		RATR_12M				0x00000040
#define		RATR_18M				0x00000080
#define		RATR_24M				0x00000100
#define		RATR_36M				0x00000200
#define		RATR_48M				0x00000400
#define		RATR_54M				0x00000800
//MCS 1 Spatial Stream	
#define		RATR_MCS0				0x00001000
#define		RATR_MCS1				0x00002000
#define		RATR_MCS2				0x00004000
#define		RATR_MCS3				0x00008000
#define		RATR_MCS4				0x00010000
#define		RATR_MCS5				0x00020000
#define		RATR_MCS6				0x00040000
#define		RATR_MCS7				0x00080000
//MCS 2 Spatial Stream
#define		RATR_MCS8				0x00100000
#define		RATR_MCS9				0x00200000
#define		RATR_MCS10				0x00400000
#define		RATR_MCS11				0x00800000
#define		RATR_MCS12				0x01000000
#define		RATR_MCS13				0x02000000
#define		RATR_MCS14				0x04000000
#define		RATR_MCS15				0x08000000
// ALL CCK Rate
#define	RATE_ALL_CCK				RATR_1M|RATR_2M|RATR_55M|RATR_11M 
#define	RATE_ALL_OFDM_AG			RATR_6M|RATR_9M|RATR_12M|RATR_18M|RATR_24M|\
									RATR_36M|RATR_48M|RATR_54M	
#define	RATE_ALL_OFDM_1SS			RATR_MCS0|RATR_MCS1|RATR_MCS2|RATR_MCS3 |\
									RATR_MCS4|RATR_MCS5|RATR_MCS6	|RATR_MCS7	
#define	RATE_ALL_OFDM_2SS			RATR_MCS8|RATR_MCS9	|RATR_MCS10|RATR_MCS11|\
									RATR_MCS12|RATR_MCS13|RATR_MCS14|RATR_MCS15
									
//
// 7. EDCA Setting Registers	 (Offset: 0x01D0 - 0x01FF)
//
//----------------------------------------------------------------------------
//       8192S EDCA Setting 	(offset 0x1D0-1DF, 4DW VO/VI/BE/BK)
//----------------------------------------------------------------------------
#define		AC_PARAM_TXOP_LIMIT_OFFSET		16
#define		AC_PARAM_ECW_MAX_OFFSET			12
#define		AC_PARAM_ECW_MIN_OFFSET			8
#define		AC_PARAM_AIFS_OFFSET			0

//----------------------------------------------------------------------------
//       8192S AcmHwCtrl bits 					(offset 0x1E7, 1 byte)
//----------------------------------------------------------------------------
#define		AcmHw_HwEn				BIT(0)
#define		AcmHw_BeqEn				BIT(1)
#define		AcmHw_ViqEn				BIT(2)
#define		AcmHw_VoqEn				BIT(3)
#define		AcmHw_BeqStatus			BIT(4)
#define		AcmHw_ViqStatus			BIT(5)
#define		AcmHw_VoqStatus			BIT(6)

//----------------------------------------------------------------------------
//       8192S Retry Limit					(Offset 0x1F4, 16bit)
//----------------------------------------------------------------------------
#define		RETRY_LIMIT_SHORT_SHIFT	8
#define		RETRY_LIMIT_LONG_SHIFT	0

//
// 8. WMAC, BA and CCX related Register.	 (Offset: 0x0200 - 0x023F)
//
//----------------------------------------------------------------------------
//       8192S NAV_CTRL bits					(Offset 0x200, 24bit)
//----------------------------------------------------------------------------
#define		NAV_UPPER_EN			BIT(16)
#define		NAV_UPPER				0xFF00
#define		NAV_RTSRST				0xFF
//----------------------------------------------------------------------------
//       8192S BW_OPMODE bits					(Offset 0x203, 8bit)
//----------------------------------------------------------------------------
#define		BW_OPMODE_20MHZ			BIT(2)
#define		BW_OPMODE_5G			BIT(1)
#define		BW_OPMODE_11J			BIT(0)

//
// 9. Security Control Registers	(Offset: 0x0240 - 0x025F)
//
//----------------------------------------------------------------------------
//       8192S RWCAM CAM Command Register     		(offset 0x240, 4 byte)
//----------------------------------------------------------------------------
#define		CAM_CM_SecCAMPolling	BIT(31)		//Security CAM Polling		
#define		CAM_CM_SecCAMClr		BIT(30)		//Clear all bits in CAM
#define		CAM_CM_SecCAMWE			BIT(16)		//Security CAM enable
#define		CAM_ADDR				0xFF		//CAM Address Offset

//----------------------------------------------------------------------------
//       8192S CAMDBG Debug CAM Register	 			(offset 0x24C, 4 byte)
//----------------------------------------------------------------------------
#define		Dbg_CAM_TXSecCAMInfo	BIT(31)		//Retrieve lastest Tx Info
#define		Dbg_CAM_SecKeyFound		BIT(30)		//Security KEY Found


//----------------------------------------------------------------------------
//       8192S SECR Security Configuration Register	(offset 0x250, 1 byte)
//----------------------------------------------------------------------------
#define		SCR_TxUseDK				BIT(0)			//Force Tx Use Default Key
#define		SCR_RxUseDK				BIT(1)			//Force Rx Use Default Key
#define		SCR_TxEncEnable			BIT(2)			//Enable Tx Encryption
#define		SCR_RxDecEnable			BIT(3)			//Enable Rx Decryption
#define		SCR_SKByA2				BIT(4)			//Search kEY BY A2
#define		SCR_NoSKMC				BIT(5)			//No Key Search Multicast
//----------------------------------------------------------------------------
//       8192S CAM Config Setting (offset 0x250, 1 byte)
//----------------------------------------------------------------------------
#define		CAM_VALID				BIT(15)
#define		CAM_NOTVALID			0x0000
#define		CAM_USEDK				BIT(5)
       	       		
#define		CAM_NONE				0x0
#define		CAM_WEP40				0x01
#define		CAM_TKIP				0x02
#define		CAM_AES					0x04
#define		CAM_WEP104				0x05
        		
#define		TOTAL_CAM_ENTRY			32
       		
#define		CAM_CONFIG_USEDK		TRUE
#define		CAM_CONFIG_NO_USEDK		FALSE
       		
#define		CAM_WRITE				BIT(16)
#define		CAM_READ				0x00000000
#define		CAM_POLLINIG			BIT(31)
       		
#define		SCR_UseDK				0x01
#define		SCR_TxSecEnable		0x02
#define		SCR_RxSecEnable		0x04

//
// 10. Power Save Control Registers	 (Offset: 0x0260 - 0x02DF)
//

//
// 11. General Purpose Registers	(Offset: 0x02E0 - 0x02FF)
//

//
// 12. Host Interrupt Status Registers	 (Offset: 0x0300 - 0x030F)
//
//----------------------------------------------------------------------------
//       8190 IMR/ISR bits						(offset 0xfd,  8bits)
//----------------------------------------------------------------------------
#define		IMR8190_DISABLED	0x0

#if (INTERRUPT_2DW == 0)
#define		IMR_CPUERR			BIT(35)		// ATIM Window End Interrupt	
#define		IMR_ATIMEND			BIT(34)		// Transmit Beacon OK Interrupt
#define		IMR_TBDOK			BIT(33)		// Transmit Beacon Error Interrupt
#define		IMR_TBDER			BIT(32)		// Beacon DMA Interrupt 8
//#define		IMR_BCNDMAINT7		BIT(32)		// Beacon DMA Interrupt 7
#else
#define		IMR_CPUERR			BIT(3)		// Transmit Beacon OK Interrupt
#define		IMR_ATIMEND			BIT(2)		// Transmit Beacon Error Interrupt
#define		IMR_TBDOK		BIT(1)		// Beacon DMA Interrupt 8
#define		IMR_TBDER		BIT(0)		// Beacon DMA Interrupt 7
#endif
#define		IMR_BCNDMAINT7		BIT(31)
#define		IMR_BCNDMAINT6		BIT(30)		// Beacon DMA Interrupt 6
#define		IMR_BCNDMAINT5		BIT(29)		// Beacon DMA Interrupt 5
#define		IMR_BCNDMAINT4		BIT(28)		// Beacon DMA Interrupt 4
#define		IMR_BCNDMAINT3		BIT(27)		// Beacon DMA Interrupt 3
#define		IMR_BCNDMAINT2		BIT(26)		// Beacon DMA Interrupt 2
#define		IMR_BCNDMAINT1		BIT(25)		// Beacon DMA Interrupt 1
#define		IMR_BCNDOK7			BIT(24)		// Beacon Queue DMA OK Interrup 7
#define		IMR_BCNDOK6			BIT(23)		// Beacon Queue DMA OK Interrup 6
#define		IMR_BCNDOK5			BIT(22)		// Beacon Queue DMA OK Interrup 5
#define		IMR_BCNDOK4			BIT(21)		// Beacon Queue DMA OK Interrup 4
#define		IMR_BCNDOK3			BIT(20)		// Beacon Queue DMA OK Interrup 3
#define		IMR_BCNDOK2			BIT(19)		// Beacon Queue DMA OK Interrup 2
#define		IMR_BCNDOK1			BIT(18)		// Beacon Queue DMA OK Interrup 1
#define		IMR_TIMEOUT2		BIT(17)		// Timeout interrupt 2
#define		IMR_TIMEOUT1		BIT(16)		// Timeout interrupt 1
#define		IMR_TXFOVW			BIT(15)		// Transmit FIFO Overflow
#define		IMR_PSTIMEOUT		BIT(14)		// Power save time out interrupt 
#define		IMR_BcnInt			BIT(13)		// Beacon DMA Interrupt 0
#define		IMR_RXFOVW			BIT(12)		// Receive FIFO Overflow
#define		IMR_RDU				BIT(11)		// Receive Descriptor Unavailable
#define		IMR_RXCMDOK			BIT(10)		// Receive Command Packet OK
#define		IMR_BDOK			BIT(9)		// Beacon Queue DMA OK Interrup
#define		IMR_HIGHDOK			BIT(8)		// High Queue DMA OK Interrupt
#define		IMR_COMDOK			BIT(7)		// Command Queue DMA OK Interrupt
#define		IMR_MGNTDOK			BIT(6)		// Management Queue DMA OK Interrupt
#define		IMR_HCCADOK			BIT(5)		// HCCA Queue DMA OK Interrupt
#define		IMR_BKDOK			BIT(4)		// AC_BK DMA OK Interrupt
#define		IMR_BEDOK			BIT(3)		// AC_BE DMA OK Interrupt
#define		IMR_VIDOK			BIT(2)		// AC_VI DMA OK Interrupt
#define		IMR_VODOK			BIT(1)		// AC_VO DMA Interrupt
#define		IMR_ROK				BIT(0)		// Receive DMA OK Interrupt

//
// 13. Test Mode and Debug Control Registers	(Offset: 0x0310 - 0x034F)
//

//
// 14. PCIE config register	(Offset 0x500-)
//
//----------------------------------------------------------------------------
//       8190 TPPool bits 					(offset 0xd9, 2 byte)
//----------------------------------------------------------------------------
#define		TPPoll_BKQ			BIT(0)			// BK queue polling
#define		TPPoll_BEQ			BIT(1)			// BE queue polling
#define		TPPoll_VIQ			BIT(2)			// VI queue polling
#define		TPPoll_VOQ			BIT(3)			// VO queue polling
#define		TPPoll_BQ			BIT(4)			// Beacon queue polling
#define		TPPoll_CQ			BIT(5)			// Command queue polling
#define		TPPoll_MQ			BIT(6)			// Management queue polling
#define		TPPoll_HQ			BIT(7)			// High queue polling
#define		TPPoll_HCCAQ		BIT(8)			// HCCA queue polling
#define		TPPoll_StopBK		BIT(9)			// Stop BK queue
#define		TPPoll_StopBE		BIT(10)			// Stop BE queue
#define		TPPoll_StopVI		BIT(11)			// Stop VI queue
#define		TPPoll_StopVO		BIT(12)			// Stop VO queue
#define		TPPoll_StopMgt		BIT(13)			// Stop Mgnt queue
#define		TPPoll_StopHigh		BIT(14)			// Stop High queue
#define		TPPoll_StopHCCA		BIT(15)			// Stop HCCA queue
#define		TPPoll_SHIFT		8				// Queue ID mapping 

//----------------------------------------------------------------------------
//       8192S PCIF 							(Offset 0x500, 32bit)
//----------------------------------------------------------------------------
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

//============================================================================
//       8192S USB specific Regsiter Offset and Content definition, 
//       2008.08.28, added by Roger.
//============================================================================
// Rx Aggregation time-out reg.
#define	USB_RX_AGG_TIMEOUT	0xFE5B	

// Firware reserved Tx page control.
#define	FW_OFFLOAD_EN		BIT(7)

// Min Spacing related settings.
#define	MAX_MSS_DENSITY 		0x13

// Rx DMA Control related settings
#define	RXDMA_AGG_EN		BIT(7)

// USB Rx Aggregation TimeOut settings
#define	RXDMA_AGG_TIMEOUT_DISABLE		0x00
#define	RXDMA_AGG_TIMEOUT_17MS  			0x01
#define	RXDMA_AGG_TIMEOUT_17_2_MS  		0x02
#define	RXDMA_AGG_TIMEOUT_17_4_MS  		0x04
#define	RXDMA_AGG_TIMEOUT_17_10_MS  	0x0a

//----------------------------------------------------------------------------
//       8192S LDOV12_CTRL bits					(Offset 0x21, 8bits)
//----------------------------------------------------------------------------
#define		LDV12_EN			BIT(0) 	// Enable LDOV12 Macro Block
#define		LDOV12D_CTRL		0x0021	// V12 Digital LDO Control.


//----------------------------------------------------------------------------
//       8190 CCX_COMMAND_REG Setting (offset 0x25A, 1 byte)
//----------------------------------------------------------------------------
#define		CCX_CMD_CLM_ENABLE				BIT(0)	// Enable Channel Load
#define		CCX_CMD_NHM_ENABLE				BIT(1)	// Enable Noise Histogram
#define		CCX_CMD_FUNCTION_ENABLE			BIT(8)	
// CCX function (Channel Load/RPI/Noise Histogram).
#define		CCX_CMD_IGNORE_CCA				BIT(9)	
// Treat CCA period as IDLE time for NHM.
#define		CCX_CMD_IGNORE_TXON				BIT(10)	
// Treat TXON period as IDLE time for NHM.
#define		CCX_CLM_RESULT_READY			BIT(16)	
// 1: Indicate the result of Channel Load is ready.
#define		CCX_NHM_RESULT_READY			BIT(16)	
// 1: Indicate the result of Noise histogram is ready.
#define		CCX_CMD_RESET					0x0		
// Clear all the result of CCX measurement and disable the CCX function.			

//----------------------------------------------------------------------------
//       8190 EEROM
//----------------------------------------------------------------------------
#define		RTL8190_EEPROM_ID					0x8129
#define		EEPROM_NODE_ADDRESS_BYTE_0			0x0C
       		                                	
#define		EEPROM_RFInd_PowerDiff				0x28
#if !defined(RTL8192SU_EFUSE)
#define		EEPROM_ThermalMeter					0x29
#endif
#define		EEPROM_TxPwDiff_CrystalCap			0x2A	//0x2A~0x2B
#define		EEPROM_TxPwIndex_CCK				0x2C	//0x2C~0x39
#define		EEPROM_TxPwIndex_OFDM_24G			0x3A	//0x3A~0x47
#define		EEPROM_TxPwIndex_OFDM_5G			0x34	//0x34~0x7B

//The following definition is for eeprom 93c56......modified 20080220
#define		EEPROM_C56_CrystalCap				0x17	//0x17
#define		EEPROM_C56_RfA_CCK_Chnl1_TxPwIndex	0x80	//0x80
#define		EEPROM_C56_RfA_HT_OFDM_TxPwIndex	0x81	//0x81~0x83
#define		EEPROM_C56_RfC_CCK_Chnl1_TxPwIndex	0xbc	//0xb8
#define		EEPROM_C56_RfC_HT_OFDM_TxPwIndex	0xb9	//0xb9~0xbb
#define		EEPROM_Customer_ID					0x7B	//0x7B:CustomerID
#define		EEPROM_ICVersion_ChannelPlan		0x7C	//0x7C:ChnlPlan, 
														//0x7D:IC_Ver
#define		EEPROM_CRC							0x7E	//0x7E~0x7F
       		
#define		EEPROM_Default_LegacyHTTxPowerDiff	0x4
#define		EEPROM_Default_ThermalMeter			0x77
#define		EEPROM_Default_AntTxPowerDiff		0x0
#define		EEPROM_Default_TxPwDiff_CrystalCap	0x5
#define		EEPROM_Default_TxPower				0x1010
#define		EEPROM_Default_TxPowerLevel			0x10

//
// Define Different EEPROM type for customer
//
#define		EEPROM_CID_DEFAULT					0x0
#define		EEPROM_CID_CAMEO					0x1
#define		EEPROM_CID_RUNTOP					0x2
#define		EEPROM_CID_Senao					0x3
#define		EEPROM_CID_TOSHIBA					0x4	
#define		EEPROM_CID_NetCore					0x5
#define		EEPROM_CID_Nettronix				0x6
#define		EEPROM_CID_Pronet					0x7


#endif // #ifndef __INC_HAL8192SEREG_H

