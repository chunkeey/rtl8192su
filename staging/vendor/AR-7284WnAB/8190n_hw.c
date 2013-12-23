/*
 *      Routines to access hardware
 *
 *      $Id: 8190n_hw.c,v 1.12 2009/09/28 13:24:13 cathy Exp $
 */

#define _8190N_HW_C_

#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <asm/unistd.h>
#endif

#include "./8190n_cfg.h"
#include "./8190n.h"
#include "./8190n_hw.h"
#include "./8190n_headers.h"
#include "./8190n_debug.h"
#ifdef RTL8192SU
#include "./8190n_usb.h"
#endif

#ifdef __KERNEL__
#ifdef __LINUX_2_6__
#include <linux/syscalls.h>
#else
#include <linux/fs.h>
#endif
#endif

#define MAX_CONFIG_FILE_SIZE (20*1024) // for 8192, added to 20k

int rtl8190n_fileopen(const char *filename, int flags, int mode);
void PHY_RF6052SetOFDMTxPower(struct rtl8190_priv *priv, unsigned char powerlevel_1ss,
	unsigned char powerlevel_2ss);
static int LoadFirmware(struct rtl8190_priv *priv);
static void ReadEFuseByte(struct rtl8190_priv *priv, unsigned short _offset, unsigned char *pbuf);


unsigned int TxPwrTrk_OFDM_SwingTbl[TxPwrTrk_OFDM_SwingTbl_Len] = {
	/*  +6.0dB */ 0x7f8001fe,
	/*  +5.5dB */ 0x788001e2,
	/*  +5.0dB */ 0x71c001c7,
	/*  +4.5dB */ 0x6b8001ae,
	/*  +4.0dB */ 0x65400195,
	/*  +3.5dB */ 0x5fc0017f,
	/*  +3.0dB */ 0x5a400169,
	/*  +2.5dB */ 0x55400155,
	/*  +2.0dB */ 0x50800142,
	/*  +1.5dB */ 0x4c000130,
	/*  +1.0dB */ 0x47c0011f,
	/*  +0.5dB */ 0x43c0010f,
	/*   0.0dB */ 0x40000100,
	/*  -0.5dB */ 0x3c8000f2,
	/*  -1.0dB */ 0x390000e4,
	/*  -1.5dB */ 0x35c000d7,
	/*  -2.0dB */ 0x32c000cb,
	/*  -2.5dB */ 0x300000c0,
	/*  -3.0dB */ 0x2d4000b5,
	/*  -3.5dB */ 0x2ac000ab,
	/*  -4.0dB */ 0x288000a2,
	/*  -4.5dB */ 0x26000098,
	/*  -5.0dB */ 0x24000090,
	/*  -5.5dB */ 0x22000088,
	/*  -6.0dB */ 0x20000080,
	/*  -6.5dB */ 0x1a00006c,
	/*  -7.0dB */ 0x1c800072,
	/*  -7.5dB */ 0x18000060,
	/*  -8.0dB */ 0x19800066,
	/*  -8.5dB */ 0x15800056,
	/*  -9.0dB */ 0x26c0005b,
	/*  -9.5dB */ 0x14400051,
	/* -10.0dB */ 0x24400051,
	/* -10.5dB */ 0x1300004c,
	/* -11.0dB */ 0x12000048,
	/* -11.5dB */ 0x11000044,
	/* -12.0dB */ 0x10000040
};

unsigned char TxPwrTrk_CCK_SwingTbl[TxPwrTrk_CCK_SwingTbl_Len][8] = {
	/*   0.0dB */ {0x36, 0x35, 0x2e, 0x25, 0x1c, 0x12, 0x09, 0x04},
	/*   0.5dB */ {0x33, 0x32, 0x2b, 0x23, 0x1a, 0x11, 0x08, 0x04},
	/*   1.0dB */ {0x30, 0x2f, 0x29, 0x21, 0x19, 0x10, 0x08, 0x03},
	/*   1.5dB */ {0x2d, 0x2d, 0x27, 0x1f, 0x18, 0x0f, 0x08, 0x03},
	/*   2.0dB */ {0x2b, 0x2a, 0x25, 0x1e, 0x16, 0x0e, 0x07, 0x03},
	/*   2.5dB */ {0x28, 0x28, 0x22, 0x1c, 0x15, 0x0d, 0x07, 0x03},
	/*   3.0dB */ {0x26, 0x25, 0x21, 0x1b, 0x14, 0x0d, 0x06, 0x03},
	/*   3.5dB */ {0x24, 0x23, 0x1f, 0x19, 0x13, 0x0c, 0x06, 0x03},
	/*   4.0dB */ {0x22, 0x21, 0x1d, 0x18, 0x11, 0x0b, 0x06, 0x02},
	/*   4.5dB */ {0x20, 0x20, 0x1b, 0x16, 0x11, 0x08, 0x05, 0x02},
	/*   5.0dB */ {0x1f, 0x1e, 0x1a, 0x15, 0x10, 0x0a, 0x05, 0x02},
	/*   5.5dB */ {0x1d, 0x1c, 0x18, 0x14, 0x0f, 0x0a, 0x05, 0x02},
	/*   6.0dB */ {0x1b, 0x1a, 0x17, 0x13, 0x0e, 0x09, 0x04, 0x02},
	/*   6.5dB */ {0x1a, 0x19, 0x16, 0x12, 0x0d, 0x09, 0x04, 0x02},
	/*   7.0dB */ {0x18, 0x17, 0x15, 0x11, 0x0c, 0x08, 0x04, 0x02},
	/*   7.5dB */ {0x17, 0x16, 0x13, 0x10, 0x0c, 0x08, 0x04, 0x02},
	/*   8.0dB */ {0x16, 0x15, 0x12, 0x0f, 0x0b, 0x07, 0x04, 0x01},
	/*   8.5dB */ {0x14, 0x14, 0x11, 0x0e, 0x0b, 0x07, 0x03, 0x02},
	/*   9.0dB */ {0x13, 0x13, 0x10, 0x0d, 0x0a, 0x06, 0x03, 0x01},
	/*   9.5dB */ {0x12, 0x12, 0x0f, 0x0c, 0x09, 0x06, 0x03, 0x01},
	/*  10.0dB */ {0x11, 0x11, 0x0f, 0x0c, 0x09, 0x06, 0x03, 0x01},
	/*  10.5dB */ {0x10, 0x10, 0x0e, 0x0b, 0x08, 0x05, 0x03, 0x01},
	/*  11.0dB */ {0x0f, 0x0f, 0x0d, 0x0b, 0x08, 0x05, 0x03, 0x01}
};

unsigned char TxPwrTrk_CCK_SwingTbl_CH14[TxPwrTrk_CCK_SwingTbl_Len][8] = {
	/*   0.0dB */ {0x36, 0x35, 0x2e, 0x1b, 0x00, 0x00, 0x00, 0x00},
	/*   0.5dB */ {0x33, 0x32, 0x2b, 0x19, 0x00, 0x00, 0x00, 0x00},
	/*   1.0dB */ {0x30, 0x2f, 0x29, 0x18, 0x00, 0x00, 0x00, 0x00},
	/*   1.5dB */ {0x2d, 0x2d, 0x27, 0x17, 0x00, 0x00, 0x00, 0x00},
	/*   2.0dB */ {0x2b, 0x2a, 0x25, 0x15, 0x00, 0x00, 0x00, 0x00},
	/*   2.5dB */ {0x28, 0x28, 0x22, 0x14, 0x00, 0x00, 0x00, 0x00},
	/*   3.0dB */ {0x26, 0x25, 0x21, 0x13, 0x00, 0x00, 0x00, 0x00},
	/*   3.5dB */ {0x24, 0x23, 0x1f, 0x12, 0x00, 0x00, 0x00, 0x00},
	/*   4.0dB */ {0x22, 0x21, 0x1d, 0x11, 0x00, 0x00, 0x00, 0x00},
	/*   4.5dB */ {0x20, 0x20, 0x1b, 0x10, 0x00, 0x00, 0x00, 0x00},
	/*   5.0dB */ {0x1f, 0x1e, 0x1a, 0x0f, 0x00, 0x00, 0x00, 0x00},
	/*   5.5dB */ {0x1d, 0x1c, 0x18, 0x0e, 0x00, 0x00, 0x00, 0x00},
	/*   6.0dB */ {0x1b, 0x1a, 0x17, 0x0e, 0x00, 0x00, 0x00, 0x00},
	/*   6.5dB */ {0x1a, 0x19, 0x16, 0x0d, 0x00, 0x00, 0x00, 0x00},
	/*   7.0dB */ {0x18, 0x17, 0x15, 0x0c, 0x00, 0x00, 0x00, 0x00},
	/*   7.5dB */ {0x17, 0x16, 0x13, 0x0b, 0x00, 0x00, 0x00, 0x00},
	/*   8.0dB */ {0x16, 0x15, 0x12, 0x0b, 0x00, 0x00, 0x00, 0x00},
	/*   8.5dB */ {0x14, 0x14, 0x11, 0x0a, 0x00, 0x00, 0x00, 0x00},
	/*   9.0dB */ {0x13, 0x13, 0x10, 0x0a, 0x00, 0x00, 0x00, 0x00},
	/*   9.5dB */ {0x12, 0x12, 0x0f, 0x09, 0x00, 0x00, 0x00, 0x00},
	/*  10.0dB */ {0x11, 0x11, 0x0f, 0x09, 0x00, 0x00, 0x00, 0x00},
	/*  10.5dB */ {0x10, 0x10, 0x0e, 0x08, 0x00, 0x00, 0x00, 0x00},
	/*  11.0dB */ {0x0f, 0x0f, 0x0d, 0x08, 0x00, 0x00, 0x00, 0x00}
};

#ifdef ADD_TX_POWER_BY_CMD
#define ASSIGN_TX_POWER_OFFSET(offset, setting) { \
	if (setting != 0xff) \
		offset = setting; \
}
#endif

/*-----------------------------------------------------------------------------
 * Function:	PHYCheckIsLegalRfPath8190Pci()
 *
 * Overview:	Check different RF type to execute legal judgement. If RF Path is illegal
 *			We will return false.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	11/15/2007	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
int PHYCheckIsLegalRfPath8190Pci(struct rtl8190_priv *priv, unsigned int eRFPath)
{
	unsigned int rtValue = TRUE;

#ifdef RTL8190
	if (get_rf_mimo_mode(priv) == MIMO_2T4R)
	{
		rtValue = TRUE;
	}
	else if (get_rf_mimo_mode(priv) == MIMO_1T2R)
	{
		if ((eRFPath == RF90_PATH_A) || (eRFPath == RF90_PATH_B))
			rtValue = FALSE;
		else
			rtValue = TRUE;
	}
	else if (get_rf_mimo_mode(priv) == MIMO_2T2R)
	{
		if ((eRFPath == RF90_PATH_B) || (eRFPath == RF90_PATH_D))
			rtValue = FALSE;
		else
			rtValue = TRUE;
	}
	else // MIMO_1T1R
	{
		if (eRFPath == RF90_PATH_C)
			rtValue = TRUE;
		else
			rtValue = FALSE;
	}
#elif defined(RTL8192E)
	if (get_rf_mimo_mode(priv) == MIMO_1T2R) {
		if ((eRFPath == RF90_PATH_A) || (eRFPath == RF90_PATH_B))
			rtValue = TRUE;
		else
			rtValue = FALSE;
	}
	else {
		rtValue = FALSE;
	}
#endif

	return rtValue;
}


/**
* Function:	phy_CalculateBitShift
*
* OverView:	Get shifted position of the BitMask
*
* Input:
*			u4Byte		BitMask,
*
* Output:	none
* Return:		u4Byte		Return the shift bit bit position of the mask
*/
unsigned int phy_CalculateBitShift(unsigned int BitMask)
{
	unsigned int i;

	for (i=0; i<=31; i++) {
		if (((BitMask>>i) & 0x1) == 1)
			break;
	}

	return (i);
}


/**
* Function:	PHY_QueryBBReg
*
* OverView:	Read "sepcific bits" from BB register
*
* Input:
*			PADAPTER		Adapter,
*			u4Byte			RegAddr,		//The target address to be readback
*			u4Byte			BitMask		//The target bit position in the target address
*										//to be readback
* Output:	None
* Return:		u4Byte			Data			//The readback register value
* Note:		This function is equal to "GetRegSetting" in PHY programming guide
*/
unsigned int PHY_QueryBBReg(struct rtl8190_priv *priv, unsigned int RegAddr, unsigned int BitMask)
{
#if !defined(RTL8192SU)
	unsigned long ioaddr = priv->pshare->ioaddr;
#else //RTL8192SU
	unsigned int  offset = (RegAddr&0x3);
#endif	
  	unsigned int ReturnValue = 0, OriginalValue, BitShift;

#if !defined(RTL8192SU)
	OriginalValue = RTL_R32(RegAddr);
#else
  	unsigned char BBWaitCounter = 0, PollingCnt = 50;

  	while(priv->pshare->bChangeBBInProgress)
	{
		BBWaitCounter ++;
		delay_ms(1); // 1 ms

		// Wait too long, return FALSE to avoid to be stuck here.
		if(BBWaitCounter > 200)
		{
			DEBUG_ERR("phy_QueryUsbBBReg(RegAddr=%x): (%d) Wait too long to query BB!!\n", RegAddr, BBWaitCounter);
			return ReturnValue;
		}
	}

	priv->pshare->bChangeBBInProgress = TRUE;
	OriginalValue = RTL_R32BaseBand(priv, (RegAddr&0xffc));
#endif	
	BitShift = phy_CalculateBitShift(BitMask);
#ifdef RTL8192SU
	do
	{// Make sure that access could be done.
		if((RTL_R8(PHY_REG)&BIT(0)) == 0)
			break;
	}while( --PollingCnt );

	if(PollingCnt == 0)
	{
		DEBUG_ERR("Fail!!!phy_QueryUsbBBReg(): RegAddr(%#x) = %#x\n", RegAddr, ReturnValue);
	}
	else
	{
		// Data FW read back.
		OriginalValue = RTL_R32(PHY_REG_DATA);	
		//printk("phy_QueryUsbBBReg(): RegAddr(%#x) = %#x, PollingCnt(%d)\n", RegAddr, ReturnValue, PollingCnt);
	}
	
	priv->pshare->bChangeBBInProgress = FALSE;
	if (offset!=0)
	{
		unsigned int  nextRegAddr=(RegAddr&0xffc)+4;
		unsigned int  nextRegVal;

		switch(offset)
		{
			case 1:
				if (BitMask<=0xffffff)
				{
					ReturnValue = (OriginalValue >> 8) & BitMask;
				}
				else
				{
					nextRegVal = PHY_QueryBBReg(priv, nextRegAddr, bMaskDWord);
					ReturnValue = ((OriginalValue>>8)|((nextRegVal&0xff)<<24)) & BitMask;
				}
				break;
			case 2:
				if (BitMask<=0xffff)
					ReturnValue = (OriginalValue >> 16) & BitMask;
				else
				{
					nextRegVal = PHY_QueryBBReg(priv, nextRegAddr, bMaskDWord);
					ReturnValue = ((OriginalValue>>16)|((nextRegVal&0xffff)<<16)) & BitMask;
				}
				break;
			case 3:
				if (BitMask<=0xff)
					ReturnValue = (OriginalValue >> 24) & BitMask;
				else
				{
					nextRegVal = PHY_QueryBBReg(priv, nextRegAddr, bMaskDWord);
					ReturnValue = ((OriginalValue>>24)|((nextRegVal&0xffffff)<<8)) & BitMask;
				}
				break;
			default:
				ReturnValue = (OriginalValue & BitMask) >> BitShift;
		}
	}
	else
#endif
	ReturnValue = (OriginalValue & BitMask) >> BitShift;

	return (ReturnValue);
}


/**
* Function:	PHY_SetBBReg
*
* OverView:	Write "Specific bits" to BB register (page 8~)
*
* Input:
*			PADAPTER		Adapter,
*			u4Byte			RegAddr,		//The target address to be modified
*			u4Byte			BitMask		//The target bit position in the target address
*										//to be modified
*			u4Byte			Data			//The new register value in the target bit position
*										//of the target address
*
* Output:	None
* Return:		None
* Note:		This function is equal to "PutRegSetting" in PHY programming guide
*/
void PHY_SetBBReg(struct rtl8190_priv *priv, unsigned int RegAddr, unsigned int BitMask, unsigned int Data)
{
#if !defined(RTL8192SU)
	unsigned long ioaddr = priv->pshare->ioaddr;
#endif	
	unsigned int OriginalValue, BitShift, NewValue;

#ifdef RTL8192SU
	unsigned char BBWaitCounter = 0;

	while(priv->pshare->bChangeBBInProgress)
	{
		BBWaitCounter ++;
		delay_ms(10); // 1 ms?
	
		if(BBWaitCounter > 100)
		{
			DEBUG_ERR("phy_SetUsbBBReg(): (%d) Wait too logn to query BB!!\n", BBWaitCounter);
			return;
		}
	}

#endif	

	if (BitMask != bMaskDWord)
	{//if not "double word" write
#if !defined(RTL8192SU)
		OriginalValue = RTL_R32(RegAddr);
#else
		OriginalValue = PHY_QueryBBReg(priv, RegAddr, bMaskDWord);
		priv->pshare->bChangeBBInProgress = TRUE;
#endif
		BitShift = phy_CalculateBitShift(BitMask);
		NewValue = ((OriginalValue & (~BitMask)) | (Data << BitShift));
		RTL_W32(RegAddr, NewValue);
	}
	else
	{
#ifdef RTL8192SU	
		priv->pshare->bChangeBBInProgress = TRUE;
#endif		
		RTL_W32(RegAddr, Data);
	}

#if defined(RTL8192E) && defined(CONFIG_X86)
	delay_us(100);
#endif

#ifdef RTL8192SU	
	priv->pshare->bChangeBBInProgress = FALSE;
#endif	
	return;
}


/**
* Function:	phy_RFSerialWrite
*
* OverView:	Write data to RF register (page 8~)
*
* Input:
*			PADAPTER		Adapter,
*			RF90_RADIO_PATH_E	eRFPath,	//Radio path of A/B/C/D
*			u4Byte			Offset,		//The target address to be read
*			u4Byte			Data			//The new register Data in the target bit position
*										//of the target to be read
*
* Output:	None
* Return:		None
* Note:		Threre are three types of serial operations: (1) Software serial write
*			(2) Hardware LSSI-Low Speed Serial Interface (3) Hardware HSSI-High speed
*			serial write. Driver need to implement (1) and (2).
*			This function is equal to the combination of RF_ReadReg() and  RFLSSIRead()
 *
 * Note: 		  For RF8256 only
 *			 The total count of RTL8256(Zebra4) register is around 36 bit it only employs
 *			 4-bit RF address. RTL8256 uses "register mode control bit" (Reg00[12], Reg00[10])
 *			 to access register address bigger than 0xf. See "Appendix-4 in PHY Configuration
 *			 programming guide" for more details.
 *			 Thus, we define a sub-finction for RTL8526 register address conversion
 *		       ===========================================================
 *			 Register Mode		RegCTL[1]		RegCTL[0]		Note
 *								(Reg00[12])		(Reg00[10])
 *		       ===========================================================
 *			 Reg_Mode0				0				x			Reg 0 ~15(0x0 ~ 0xf)
 *		       ------------------------------------------------------------------
 *			 Reg_Mode1				1				0			Reg 16 ~30(0x1 ~ 0xf)
 *		       ------------------------------------------------------------------
 *			 Reg_Mode2				1				1			Reg 31 ~ 45(0x1 ~ 0xf)
 *		       ------------------------------------------------------------------
*/
void phy_RFSerialWrite(struct rtl8190_priv *priv, RF90_RADIO_PATH_E eRFPath, unsigned int Offset, unsigned int Data)
{
#if !defined(RTL8192SU)
	struct rtl8190_hw			*phw = GET_HW(priv);
	unsigned int				DataAndAddr = 0;
	BB_REGISTER_DEFINITION_T	*pPhyReg = &phw->PHYRegDef[eRFPath];
#else
	unsigned char PollingCnt = 100;
	unsigned char RFWaitCounter = 0;
#endif	
	
#if	defined(RTL8190) || defined(RTL8192E)
	unsigned long ioaddr = priv->pshare->ioaddr;
	unsigned int				BBReg88C_OriginalValue = 0;
#endif

#if !defined(RTL8192SU)
	// Joseph test
	unsigned int				NewOffset;
#endif	

#if	defined(RTL8190) || defined(RTL8192E)
	Offset &= 0x3f;

	//
	// Switch page for 8256 RF IC
	//
	if (priv->pmib->dot11RFEntry.dot11RFType == _11BGN_RF_8256_) {
		BBReg88C_OriginalValue = RTL_R32(rFPGA0_AnalogParameter4);
		RTL_W32(rFPGA0_AnalogParameter4, (BBReg88C_OriginalValue & 0xfffff0ff));

 		if (Offset >= 31) {
			phw->RfReg0Value[eRFPath] |= 0x140;
			PHY_SetBBReg(priv, pPhyReg->rf3wireOffset, bMaskDWord, phw->RfReg0Value[eRFPath]<<16);

			NewOffset = Offset - 30;
		}
		else if (Offset >= 16) {
			phw->RfReg0Value[eRFPath] |= 0x100;
			phw->RfReg0Value[eRFPath] &= (~0x40);
			PHY_SetBBReg(priv, pPhyReg->rf3wireOffset, bMaskDWord, phw->RfReg0Value[eRFPath]<<16);

			NewOffset = Offset - 15;
		}
		else
			NewOffset = Offset;
	}
	else
		NewOffset = Offset;

	//
	// Put write addr in [5:0]  and write data in [31:16]
	//
	DataAndAddr = (Data<<16) | (NewOffset&0x3f);

	//
	// Write Operation
	//
	PHY_SetBBReg(priv, pPhyReg->rf3wireOffset, bMaskDWord, DataAndAddr);

	if (Offset == 0)
		phw->RfReg0Value[eRFPath] = Data;

	// Switch back to Reg_Mode0;
	if ((priv->pmib->dot11RFEntry.dot11RFType == _11BGN_RF_8256_) && (Offset>=16)) {
			phw->RfReg0Value[eRFPath] &= 0xebf;
			PHY_SetBBReg(priv, pPhyReg->rf3wireOffset, bMaskDWord, phw->RfReg0Value[eRFPath]<<16);
	}

	if (priv->pmib->dot11RFEntry.dot11RFType == _11BGN_RF_8256_)
		RTL_W32(rFPGA0_AnalogParameter4, BBReg88C_OriginalValue);

#elif	defined(RTL8192SE)

	Offset &= 0x3f;

	//
	// Switch page for 8256 RF IC
	//
		NewOffset = Offset;

	//
	// Put write addr in [5:0]  and write data in [31:16]
	//
	//DataAndAddr = (Data<<16) | (NewOffset&0x3f);
	DataAndAddr = ((NewOffset<<20) | (Data&0x000fffff)) & 0x0fffffff;	// T65 RF

	//
	// Write Operation
	//
	PHY_SetBBReg(priv, pPhyReg->rf3wireOffset, bMaskDWord, DataAndAddr);

#elif defined(RTL8192SU)
	Offset &= 0x3f;

	while(priv->pshare->bChangeRFInProgress)
	{
		RFWaitCounter ++;
		delay_ms(1);	 // 1 ms
	
		if(RFWaitCounter > 100)
		{
			DEBUG_ERR("phy_SetUsbRFReg(): (%d) Wait too logn to query BB!!\n", RFWaitCounter);
			return;
		}
	}

	priv->pshare->bChangeRFInProgress = TRUE;

	RTL_W32(RF_BB_CMD_DATA, Data);
	RTL_W32(RF_BB_CMD_ADDR, 0xF0000003|
					(Offset<<8)| //RF_Offset= 0x00~0x3F
					(eRFPath<<16));  //RF_Path = 0(A) or 1(B)
	
	do
	{// Make sure that access could be done.
		if(RTL_R32(RF_BB_CMD_ADDR) == 0)
				break;
	}while( --PollingCnt );		

	if(PollingCnt == 0)
	{		
		DEBUG_ERR("phy_SetUsbRFReg(): Set RegAddr(%#x) = %#x Fail!!!\n", Offset, Data);
	}

	priv->pshare->bChangeRFInProgress = FALSE;

#endif
}


/**
* Function:	phy_RFSerialRead
*
* OverView:	Read regster from RF chips
*
* Input:
*			PADAPTER		Adapter,
*			RF90_RADIO_PATH_E	eRFPath,	//Radio path of A/B/C/D
*			u4Byte			Offset,		//The target address to be read
*			u4Byte			dbg_avoid,	//set bitmask in reg 0 to prevent RF switchs to debug mode
*
* Output:	None
* Return:		u4Byte			reback value
* Note:		Threre are three types of serial operations: (1) Software serial write
*			(2) Hardware LSSI-Low Speed Serial Interface (3) Hardware HSSI-High speed
*			serial write. Driver need to implement (1) and (2).
*			This function is equal to the combination of RF_ReadReg() and  RFLSSIRead()
*/
unsigned int phy_RFSerialRead(struct rtl8190_priv *priv, RF90_RADIO_PATH_E eRFPath, unsigned int Offset, unsigned int dbg_avoid)
{
#if !defined(RTL8192SU)
	struct rtl8190_hw			*phw = GET_HW(priv);
#endif	
#if	defined(RTL8192SE)
	unsigned int 				tmplong, tmplong2;
#endif
	unsigned int				retValue = 0;
#if !defined(RTL8192SU)
	BB_REGISTER_DEFINITION_T	*pPhyReg = &phw->PHYRegDef[eRFPath];
	unsigned long ioaddr = priv->pshare->ioaddr;

	// Joseph test
	unsigned int				NewOffset;
#else //RTL8192SU
	unsigned char PollingCnt = 100;
	unsigned char RFWaitCounter = 0;
#endif
#if	defined(RTL8190) || defined(RTL8192E)
	unsigned int				BBReg88C_OriginalValue = 0;

	//
	// Make sure RF register offset is correct
	//
	Offset &= 0x3f;

	//
	// Switch page for 8256 RF IC
	//
	if (priv->pmib->dot11RFEntry.dot11RFType == _11BGN_RF_8256_) {
		BBReg88C_OriginalValue = RTL_R32(rFPGA0_AnalogParameter4);
		RTL_W32(rFPGA0_AnalogParameter4, (BBReg88C_OriginalValue & 0xfffff0ff));

		if (Offset >= 31) {
			phw->RfReg0Value[eRFPath] |= 0x140;

			// Switch to Reg_Mode2 for Reg31~45
			PHY_SetBBReg(priv, pPhyReg->rf3wireOffset, bMaskDWord, phw->RfReg0Value[eRFPath]<<16);

			// Modified Offset
			NewOffset = Offset - 30;
		}
		else if (Offset >= 16) {
			phw->RfReg0Value[eRFPath] |= 0x100;
			phw->RfReg0Value[eRFPath] &= (~0x40);

			// Switch to Reg_Mode1 for Reg16~30
			PHY_SetBBReg(priv, pPhyReg->rf3wireOffset, bMaskDWord, phw->RfReg0Value[eRFPath]<<16);

			// Modified Offset
			NewOffset = Offset - 15;
		}
		else
			NewOffset = Offset;
	}
	else
		NewOffset = Offset;

	//
	// Put desired read address to LSSI control register
	//
	PHY_SetBBReg(priv, pPhyReg->rfHSSIPara2, bLSSIReadAddress, NewOffset);

	// TODO: we should not delay such a  long time. Ask help from SD3
	delay_ms(1);

	//
	// Issue a posedge trigger
	//
	PHY_SetBBReg(priv, pPhyReg->rfHSSIPara2,  bLSSIReadEdge, 0x0);
	PHY_SetBBReg(priv, pPhyReg->rfHSSIPara2,  bLSSIReadEdge, 0x1);


	// TODO: we should not delay such a  long time. Ask help from SD3
	delay_ms(10);

	retValue = PHY_QueryBBReg(priv, pPhyReg->rfLSSIReadBack, bLSSIReadBackData);

	if ((priv->pmib->dot11RFEntry.dot11RFType == _11BGN_RF_8256_) && (Offset==0)) {
		// for 8256, in case rf switchs to debug mode
		// have to be refined when register 0 format has been changed
		if (dbg_avoid)
			retValue |= (BIT(11) | BIT(10) | BIT(7) | BIT(4) |BIT(3));
		phw->RfReg0Value[eRFPath] = retValue;
		PHY_SetBBReg(priv, pPhyReg->rf3wireOffset, bMaskDWord, phw->RfReg0Value[eRFPath]<<16);
	}

	// Switch back to Reg_Mode0;
	if ((priv->pmib->dot11RFEntry.dot11RFType == _11BGN_RF_8256_) && (Offset>=16)) {
		phw->RfReg0Value[eRFPath] &= 0xebf;
		PHY_SetBBReg(priv, pPhyReg->rf3wireOffset, bMaskDWord, phw->RfReg0Value[eRFPath]<<16);
	}

	if (priv->pmib->dot11RFEntry.dot11RFType == _11BGN_RF_8256_)
		RTL_W32(rFPGA0_AnalogParameter4, BBReg88C_OriginalValue);

#elif defined(RTL8192SE)

	//
	// Make sure RF register offset is correct
	//
	Offset &= 0x3f;

	//
	// Switch page for 8256 RF IC
	//
	NewOffset = Offset;


	// For 92S LSSI Read RFLSSIRead
	// For RF A/B write 0x824/82c(does not work in the future)
	// We must use 0x824 for RF A and B to execute read trigger
	tmplong = RTL_R32(rFPGA0_XA_HSSIParameter2);
	tmplong2 = RTL_R32(pPhyReg->rfHSSIPara2);
	tmplong2 = (tmplong2 & (~bLSSIReadAddress)) | ((NewOffset<<23) | bLSSIReadEdge);	//T65 RF

	RTL_W32(rFPGA0_XA_HSSIParameter2,tmplong&(~bLSSIReadEdge));
	delay_ms(1);
	RTL_W32(pPhyReg->rfHSSIPara2,tmplong2);
	delay_ms(1);
	RTL_W32(rFPGA0_XA_HSSIParameter2,tmplong|bLSSIReadEdge);
	delay_ms(1);

	//Read from BBreg8a0, 12 bits for 8190, 20 bits for T65 RF
	if ((RTL_R32(0x820)&BIT(8)) && (RTL_R32(0x828)&BIT(8)))
		retValue = PHY_QueryBBReg(priv, pPhyReg->rfLSSIReadBack_92S_PI, bLSSIReadBackData);
	else
		retValue = PHY_QueryBBReg(priv, pPhyReg->rfLSSIReadBack, bLSSIReadBackData);

#elif defined(RTL8192SU)
	//
	// Make sure RF register offset is correct
	//
	Offset &= 0x3f;
	while(priv->pshare->bChangeRFInProgress)
	{
		RFWaitCounter ++;
		delay_ms(1);	 // 1 ms

		if(RFWaitCounter > 100)
		{
			DEBUG_ERR("phy_QueryUsbRFReg(): (%d) Wait too logn to query BB!!\n", RFWaitCounter);
			return 0xffffffff;
		}
	}

	priv->pshare->bChangeRFInProgress = TRUE;

	RTL_W32(RF_BB_CMD_ADDR, 0xF0000002|
						(Offset<<8)|	//RF_Offset= 0x00~0x3F
						(eRFPath<<16)); 	//RF_Path = 0(A) or 1(B)
	
	do
	{// Make sure that access could be done.
		if(RTL_R32(RF_BB_CMD_ADDR) == 0)
			break;
	}while( --PollingCnt );

	// Data FW read back.
	retValue = RTL_R32(RF_BB_CMD_DATA);
	
	priv->pshare->bChangeRFInProgress = FALSE;
	
	//printk("phy_QueryUsbRFReg(): eRFPath(%d), Offset(%#x) = %#x\n", eRFPath, Offset, retValue);
#endif

	return retValue;
}


/**
* Function:	PHY_QueryRFReg
*
* OverView:	Query "Specific bits" to RF register (page 8~)
*
* Input:
*			PADAPTER		Adapter,
*			RF90_RADIO_PATH_E	eRFPath,	//Radio path of A/B/C/D
*			u4Byte			RegAddr,		//The target address to be read
*			u4Byte			BitMask		//The target bit position in the target address
*										//to be read
*			u4Byte			dbg_avoid	//set bitmask in reg 0 to prevent RF switchs to debug mode
*
* Output:	None
* Return:		u4Byte			Readback value
* Note:		This function is equal to "GetRFRegSetting" in PHY programming guide
*/
unsigned int PHY_QueryRFReg(struct rtl8190_priv *priv, RF90_RADIO_PATH_E eRFPath,
				unsigned int RegAddr, unsigned int BitMask, unsigned int dbg_avoid)
{
	unsigned int	Original_Value, Readback_Value, BitShift;

	Original_Value = phy_RFSerialRead(priv, eRFPath, RegAddr, dbg_avoid);
   	BitShift =  phy_CalculateBitShift(BitMask);
   	Readback_Value = (Original_Value & BitMask) >> BitShift;

	return (Readback_Value);
}


/**
* Function:	PHY_SetRFReg
*
* OverView:	Write "Specific bits" to RF register (page 8~)
*
* Input:
*			PADAPTER		Adapter,
*			RF90_RADIO_PATH_E	eRFPath,	//Radio path of A/B/C/D
*			u4Byte			RegAddr,		//The target address to be modified
*			u4Byte			BitMask		//The target bit position in the target address
*										//to be modified
*			u4Byte			Data			//The new register Data in the target bit position
*										//of the target address
*
* Output:	None
* Return:		None
* Note:		This function is equal to "PutRFRegSetting" in PHY programming guide
*/
#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
__NOMIPS16
#endif
void PHY_SetRFReg(struct rtl8190_priv *priv, RF90_RADIO_PATH_E eRFPath, unsigned int RegAddr,
				unsigned int BitMask, unsigned int Data)
{
	unsigned int Original_Value, BitShift, New_Value;
	unsigned long flags;

	SAVE_INT_AND_CLI(flags);

#if	defined(RTL8190) || defined(RTL8192)
   	if (BitMask != bMask12Bits) // RF data is 12 bits only
#elif	defined(RTL8192SE) || defined(RTL8192SU)
   	if (BitMask != bMask20Bits) // RF data is 12 bits only
#endif
   	{
		Original_Value = phy_RFSerialRead(priv, eRFPath, RegAddr, 1);
		BitShift = phy_CalculateBitShift(BitMask);
		New_Value = ((Original_Value & (~BitMask)) | (Data<< BitShift));

		phy_RFSerialWrite(priv, eRFPath, RegAddr, New_Value);
	}
	else
		phy_RFSerialWrite(priv, eRFPath, RegAddr, Data);

	RESTORE_INT(flags);
}


static int is_hex(char s)
{
	if (( s >= '0' && s <= '9') || ( s >= 'a' && s <= 'f') || (s >= 'A' && s <= 'F') || (s == 'x' || s == 'X'))
		return 1;
	else
		return 0;
}


static unsigned char *get_digit(unsigned char **data)
{
	unsigned char *buf=*data;
	int i=0;

	while(buf[i] && ((buf[i] == ' ') || (buf[i] == '\t')))
		i++;
	*data = &buf[i];

	while(buf[i]) {
		if ((buf[i] == ' ') || (buf[i] == '\t')) {
			buf[i] = '\0';
			break;
		}
		if (!is_hex(buf[i]))
			return NULL;
		i++;
	}
	if (i == 0)
		return NULL;
	else
		return &buf[i+1];
}


static int get_offset_val(unsigned char *line_head, unsigned int *u4bRegOffset, unsigned int *u4bRegValue)
{
	unsigned char *next;
	int base, idx;
	int num=0;
	extern int _atoi(char *s, int base);

	*u4bRegOffset = *u4bRegValue = '\0';

	next = get_digit(&line_head);
	if (next == NULL)
		return num;
	num++;
	if ((!memcmp(line_head, "0x", 2)) || (!memcmp(line_head, "0X", 2))) {
		base = 16;
		idx = 2;
	}
	else {
		base = 10;
		idx = 0;
	}
	*u4bRegOffset = _atoi(&line_head[idx], base);

	if (next) {
		if (!get_digit(&next))
			return num;
		num++;
		if ((!memcmp(next, "0x", 2)) || (!memcmp(next, "0X", 2))) {
			base = 16;
			idx = 2;
		}
		else {
			base = 10;
			idx = 0;
		}
		*u4bRegValue = _atoi(&next[idx], base);
	}
	else
		*u4bRegValue = 0;

	return num;
}

#ifdef RTL8192SU
static int get_offset_val_macreg(unsigned char *line_head, unsigned int *u4bRegOffset, unsigned int *u4bRegValue)
{
	unsigned char *next;
	int base, idx;
	int num=0;
	extern int _atoi(char *s, int base);

	*u4bRegOffset = *u4bRegValue = '\0';

	next = get_digit(&line_head);
	if (next == NULL)
		return num;
	num++;
	base = 16;
	idx = 0;
	if (memcmp(&line_head[idx], "0xff", 4)==0)
		*u4bRegOffset=0xff;
	else	
		*u4bRegOffset = _atoi(&line_head[idx], base);
		
	if (next) {
		if (!get_digit(&next) )
			return num;
		num++;
		base = 16;
		idx = 0;
		*u4bRegValue = _atoi(&next[idx], base);
	}
	else
		*u4bRegValue = 0;

	return num;
}
#endif

static int get_offset_mask_val(unsigned char *line_head, unsigned int *u4bRegOffset, unsigned int *u4bMask, unsigned int *u4bRegValue)
{
	unsigned char *next, *n1;
	int base, idx;
	extern int _atoi(char *s, int base);
	int num=0;

	*u4bRegOffset = *u4bRegValue = *u4bMask = '\0';

	next = get_digit(&line_head);
	if (next == NULL)
		return num;
	num++;
	if (!memcmp(line_head, "0x", 2)) {
		base = 16;
		idx = 2;
	}
	else {
		base = 10;
		idx = 0;
	}
	*u4bRegOffset = _atoi(&line_head[idx], base);

	if (next) {
		n1 = get_digit(&next);
		if (n1 == NULL)
			return num;
		num++;
		if (!memcmp(next, "0x", 2)) {
			base = 16;
			idx = 2;
		}
		else {
			base = 10;
			idx = 0;
		}
		*u4bMask = _atoi(&next[idx], base);

		if (n1) {
			if (!get_digit(&n1))
				return num;
			num++;
			if (!memcmp(n1, "0x", 2)) {
				base = 16;
				idx = 2;
			}
			else {
				base = 10;
				idx = 0;
			}
			*u4bRegValue = _atoi(&n1[idx], base);
		}
		else
			*u4bRegValue = 0;
	}
	else
		*u4bMask = 0;

	return num;
}


unsigned char *get_line(unsigned char **line)
{
	unsigned char *p=*line;

	while (*p && ((*p == '\n') || (*p == '\r')))
		p++;

	if (*p == '\0') {
		*line = NULL;
		return NULL;
	}
	*line = p;

	while (*p && *p != '\n' && *p != '\r' )
			p++;

	*p = '\0';
	return p+1;
}


/*-----------------------------------------------------------------------------
 * Function:    PHY_ConfigBBWithParaFile()
 *
 * Overview:    This function read BB parameters from general file format, and do register
 *			  Read/Write
 *
 * Input:      	PADAPTER		Adapter
 *			ps1Byte 			pFileName
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: configuration file exist
 *
 *---------------------------------------------------------------------------*/
int PHY_ConfigBBWithParaFile(struct rtl8190_priv *priv, int reg_file)
{
	int           read_bytes, num, len=0;
#ifndef MERGE_FW
	int           fd=0;
	mm_segment_t  old_fs;
	unsigned char *pFileName=NULL;
#endif
	unsigned int  u4bRegOffset, u4bRegValue, u4bRegMask;
	unsigned char *mem_ptr, *line_head, *next_head;
	struct PhyRegTable *reg_table=NULL;
	struct MacRegTable *pg_reg_table = NULL;
	unsigned short max_len=0;
	int file_format = TWO_COLUMN;
#if !defined(CONFIG_X86) && !defined(MERGE_FW)
	extern ssize_t sys_read(unsigned int fd, char * buf, size_t count);
#endif

#ifdef MERGE_FW
	if (reg_file == AGCTAB) {
		reg_table = (struct PhyRegTable *)priv->pshare->agc_tab_buf;
		next_head = __AGC_TAB_start;
		read_bytes = (int)( __AGC_TAB_end - __AGC_TAB_start);
		max_len = AGC_TAB_SIZE;
	}
	else if (reg_file == PHYREG_PG){
		pg_reg_table = (struct MacRegTable *)priv->pshare->phy_reg_pg_buf;
		next_head = __PHY_REG_PG_start;
		read_bytes = (int)( __PHY_REG_PG_end - __PHY_REG_PG_start);
		max_len = PHY_REG_PG_SIZE;
		file_format = THREE_COLUMN;
	}
	else if (reg_file == PHYREG_1T2R){
		if(priv->pshare->rf_ft_var.pathB_1T == 0){//PATH A
			pg_reg_table = (struct MacRegTable *)priv->pshare->phy_reg_2to1;
			next_head = __PHY_to1T2R_start;
			read_bytes = (int)( __PHY_to1T2R_end - __PHY_to1T2R_start);
			max_len = PHY_REG_1T2R;
			file_format = THREE_COLUMN;
		}else{//PATH B

			pg_reg_table = (struct MacRegTable *)priv->pshare->phy_reg_2to1;
			next_head = __PHY_to1T2R_b_start;
			read_bytes = (int)( __PHY_to1T2R_b_end - __PHY_to1T2R_b_start);
			max_len = PHY_REG_1T2R;
			file_format = THREE_COLUMN;
			printk("PHY_to1T2R_b\n");
		}
	}else if(reg_file == PHYREG_1T1R){
		if(priv->pshare->rf_ft_var.pathB_1T == 0){
			pg_reg_table = (struct MacRegTable *)priv->pshare->phy_reg_2to1;
			next_head = __PHY_to1T1R_start;
			read_bytes = (int)( __PHY_to1T1R_end - __PHY_to1T1R_start);
			max_len = PHY_REG_1T1R;
			file_format = THREE_COLUMN;
			printk("PHY_to1T1R\n");
		}else{//PATH B
			pg_reg_table = (struct MacRegTable *)priv->pshare->phy_reg_2to1;
			next_head = __PHY_to1T1R_b_start;
			read_bytes = (int)( __PHY_to1T1R_b_end - __PHY_to1T1R_b_start);
			max_len = PHY_REG_1T1R;
			file_format = THREE_COLUMN;
			printk("PHY_to1T1R_b\n");
		}
	}
	else {
#if (defined(RTL8192SE)||defined(RTL8192SU)) && defined(MP_TEST)
		if (priv->pshare->rf_ft_var.mp_specific) {
			reg_table = (struct PhyRegTable *)priv->pshare->phy_reg_mp_buf;
			next_head = __phy_reg_MP_start;
			read_bytes = (int)( __phy_reg_MP_end - __phy_reg_MP_start);
		}
		else
#endif
		{
		reg_table = (struct PhyRegTable *)priv->pshare->phy_reg_buf;
		next_head = __phy_reg_start;
		read_bytes = (int)( __phy_reg_end - __phy_reg_start);
		}
		max_len = PHY_REG_SIZE;
	}
#else

	switch (reg_file) {
	case AGCTAB:
#if	defined(RTL8190) || defined(RTL8192E)
		pFileName = "/usr/rtl8190Pci/AGC_TAB.txt";
#elif defined(RTL8192SE)
		pFileName = "/usr/rtl8192Pci/AGC_TAB.txt";
#elif defined(RTL8192SU)
		pFileName = "/usr/rtl8192su/AGC_TAB.txt";
#endif
		reg_table = (struct PhyRegTable *)priv->pshare->agc_tab_buf;
		max_len = AGC_TAB_SIZE;
		break;
	case PHYREG_1T2R:
#if	defined(RTL8190) || defined(RTL8192E)
		pFileName = "/usr/rtl8190Pci/PHY_REG_1T2R.txt";
		reg_table = (struct PhyRegTable *)priv->pshare->phy_reg_buf;
#elif defined(RTL8192SE)
		if(priv->pshare->rf_ft_var.pathB_1T == 0)
			pFileName = "/usr/rtl8192Pci/PHY_to1T2R.txt";
		else
			pFileName = "/usr/rtl8192Pci/PHY_to1T2R_b.txt";
		pg_reg_table = (struct MacRegTable *)priv->pshare->phy_reg_pg_buf;
		file_format = THREE_COLUMN;
#elif defined(RTL8192SU)
#if defined(MP_TEST)
	if (priv->pshare->rf_ft_var.mp_specific) {
		pFileName = "/usr/rtl8192su/phy_reg_MP.txt";
		reg_table = (struct PhyRegTable *)priv->pshare->phy_reg_mp_buf;
		max_len = PHY_REG_SIZE;
		break; 
	}
	else
#endif

		pFileName = "/usr/rtl8192su/PHY_REG_1T2R.txt";
		reg_table = (struct PhyRegTable *)priv->pshare->phy_reg_buf;
#endif
		max_len = PHY_REG_SIZE;
		break;
	case PHYREG_2T4R:
#if	defined(RTL8190) || defined(RTL8192E)
		pFileName = "/usr/rtl8190Pci/PHY_REG.txt";
#elif defined(RTL8192SE)
		pFileName = "/usr/rtl8192Pci/phy_reg.txt";
#elif defined(RTL8192SU)
		pFileName = "/usr/rtl8192su/phy_reg.txt";
#endif
		reg_table = (struct PhyRegTable *)priv->pshare->phy_reg_buf;
		max_len = PHY_REG_SIZE;
		break;
	case PHYREG_2T2R:
#if	defined(RTL8190) || defined(RTL8192E)
		pFileName = "/usr/rtl8190Pci/PHY_REG_2T2R.txt";
#elif defined(RTL8192SE) || defined(RTL8192SU)
#if defined(MP_TEST)
	if (priv->pshare->rf_ft_var.mp_specific) {
#if !defined(RTL8192SU)
		pFileName = "/usr/rtl8192Pci/phy_reg_MP.txt";
#else
		pFileName = "/usr/rtl8192su/phy_reg_MP.txt";
#endif
		reg_table = (struct PhyRegTable *)priv->pshare->phy_reg_mp_buf;
		max_len = PHY_REG_SIZE;
		break; 
	}
	else
#endif
	{
#ifdef RTL8192SU
		pFileName = "/usr/rtl8192su/phy_reg.txt";
#else
		pFileName = "/usr/rtl8192Pci/phy_reg.txt";
#endif
#endif
		reg_table = (struct PhyRegTable *)priv->pshare->phy_reg_buf;
		max_len = PHY_REG_SIZE;
		break;
	}
#if !defined(RTL8192SU)
	case PHYREG_1T1R:
#if defined(RTL8190) || defined(RTL8192E)
		pFileName = "/usr/rtl8192Pci/PHY_REG_1T1R.txt";
		reg_table = (struct PhyRegTable *)priv->pshare->phy_reg_buf;
#elif defined(RTL8192SE)
		if(priv->pshare->rf_ft_var.pathB_1T == 0)
			pFileName = "/usr/rtl8192Pci/PHY_to1T1R.txt";
		else
			pFileName = "/usr/rtl8192Pci/PHY_to1T1R_b.txt";
		pg_reg_table = (struct MacRegTable *)priv->pshare->phy_reg_pg_buf;
		file_format = THREE_COLUMN;
#endif
		max_len = PHY_REG_SIZE;
		break;
#endif
#if defined(RTL8192SE) || defined(RTL8192SU)
	case PHYREG_PG:
#if !defined(RTL8192SU)
		pFileName = "/usr/rtl8192Pci/PHY_REG_PG.txt";
#else
		pFileName = "/usr/rtl8192su/PHY_REG_PG.txt";
#endif
		pg_reg_table = (struct MacRegTable *)priv->pshare->phy_reg_pg_buf;
		max_len = PHY_REG_PG_SIZE;
		file_format = THREE_COLUMN;
		break;
#endif
	}
#endif // MERGE_FW

#ifdef CHECK_HANGUP
	if (!priv->reset_hangup)
#endif
	{
		if((mem_ptr = (unsigned char *)kmalloc(MAX_CONFIG_FILE_SIZE, GFP_ATOMIC)) == NULL) {
			printk("PHY_ConfigBBWithParaFile(): not enough memory\n");
			return -1;
		}

		memset(mem_ptr, 0, MAX_CONFIG_FILE_SIZE); // clear memory

#ifdef MERGE_FW
		memcpy(mem_ptr, next_head, read_bytes);
#else

		old_fs = get_fs();
		set_fs(KERNEL_DS);

#if	defined(CONFIG_X86)
		if ((fd = rtl8190n_fileopen(pFileName, O_RDONLY, 0)) < 0)
#else
		if ((fd = sys_open(pFileName, O_RDONLY, 0)) < 0)
#endif
		{
			printk("PHY_ConfigBBWithParaFile(): cannot open %s\n", pFileName);
			set_fs(old_fs);
			kfree(mem_ptr);
			return -1;
		}

		read_bytes = sys_read(fd, mem_ptr, MAX_CONFIG_FILE_SIZE);
#endif // MERGE_FW

		next_head = mem_ptr;
		while (1) {
			line_head = next_head;
			next_head = get_line(&line_head);
			if (line_head == NULL)
				break;

			if (line_head[0] == '/')
				continue;

			if (file_format == TWO_COLUMN) {
				num = get_offset_val(line_head, &u4bRegOffset, &u4bRegValue);
				if (num > 0) {
					reg_table[len].offset = u4bRegOffset;
					reg_table[len].value = u4bRegValue;
					len++;
					if (u4bRegOffset == 0xff)
						break;
					if ((len * sizeof(struct PhyRegTable)) > max_len)
						break;
				}
			}
			else {
				num = get_offset_mask_val(line_head, &u4bRegOffset, &u4bRegMask ,&u4bRegValue);
				if (num > 0) {
					pg_reg_table[len].offset = u4bRegOffset;
					pg_reg_table[len].mask = u4bRegMask;
					pg_reg_table[len].value = u4bRegValue;
					len++;
					if (u4bRegOffset == 0xff)
						break;
					if ((len * sizeof(struct MacRegTable)) > max_len)
						break;
				}
			}
		}

#ifndef MERGE_FW
		sys_close(fd);
		set_fs(old_fs);
#endif

		kfree(mem_ptr);

		if ((len * sizeof(struct PhyRegTable)) > max_len) {
#ifdef MERGE_FW
			printk("PHY REG table buffer not large enough!\n");
#else
			printk("PHY REG table buffer not large enough! (%s)\n", pFileName);
#endif
			return -1;
		}
	}

	num = 0;
	while (1) {
		if (file_format == THREE_COLUMN) {
			u4bRegOffset = pg_reg_table[num].offset;
			u4bRegValue = pg_reg_table[num].value;
			u4bRegMask = pg_reg_table[num].mask;
		}
		else {
			u4bRegOffset = reg_table[num].offset;
			u4bRegValue = reg_table[num].value;
		}

		if (u4bRegOffset == 0xff)
			break;
#ifdef RTL8192SU			
		else if (u4bRegOffset == 0xfe)
			delay_ms(50);
		else if (u4bRegOffset == 0xfd)
			delay_ms(5);
		else if (u4bRegOffset == 0xfc)
			delay_ms(1);
		else if (u4bRegOffset == 0xfb)
			delay_us(50);
		else if (u4bRegOffset == 0xfa)
			delay_us(5);
		else if (u4bRegOffset == 0xf9)
			delay_us(1);
#endif
		else if (file_format == THREE_COLUMN)
			PHY_SetBBReg(priv, u4bRegOffset, u4bRegMask, u4bRegValue);
		else
			PHY_SetBBReg(priv, u4bRegOffset, bMaskDWord, u4bRegValue);
		num++;
	}

	return 0;
}


/*-----------------------------------------------------------------------------
 * Function:    PHY_ConfigRFWithParaFile()
 *
 * Overview:    This function read RF parameters from general file format, and do RF 3-wire
 *
 * Input:      	PADAPTER			Adapter
 *			ps1Byte 				pFileName
 *			RF90_RADIO_PATH_E	eRFPath
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: configuration file exist
 *
 * Note:		Delay may be required for RF configuration
 *---------------------------------------------------------------------------*/
#ifdef MERGE_FW
int PHY_ConfigRFWithParaFile(struct rtl8190_priv *priv, unsigned char *start, int read_bytes, RF90_RADIO_PATH_E eRFPath)
#else
int PHY_ConfigRFWithParaFile(struct rtl8190_priv *priv, unsigned char *pFileName, RF90_RADIO_PATH_E eRFPath)
#endif
{
	int           num;
#ifndef MERGE_FW
	int           fd=0, read_bytes;
	mm_segment_t  old_fs;
#endif
	unsigned int  u4bRegOffset, u4bRegValue;
	unsigned char *mem_ptr, *line_head, *next_head;
#if !defined(CONFIG_X86) && !defined(MERGE_FW)
	extern ssize_t sys_read(unsigned int fd, char * buf, size_t count);
#endif

	if((mem_ptr = (unsigned char *)kmalloc(MAX_CONFIG_FILE_SIZE, GFP_ATOMIC)) == NULL) {

		printk("PHY_ConfigRFWithParaFile(): not enough memory\n");
		return -1;
	}

	memset(mem_ptr, 0, MAX_CONFIG_FILE_SIZE); // clear memory

#ifdef MERGE_FW
	memcpy(mem_ptr, start, read_bytes);
#else
	old_fs = get_fs();
	set_fs(KERNEL_DS);

#if	defined(CONFIG_X86)
	if ((fd = rtl8190n_fileopen(pFileName, O_RDONLY, 0)) < 0)
#else
	if ((fd = sys_open(pFileName, O_RDONLY, 0)) < 0)
#endif
	{
		printk("PHY_ConfigRFWithParaFile(): cannot open %s\n", pFileName);
		set_fs(old_fs);
		kfree(mem_ptr);
		return -1;
	}

	read_bytes = sys_read(fd, mem_ptr, MAX_CONFIG_FILE_SIZE);
#endif // MERGE_FW

	next_head = mem_ptr;
	while (1) {
		line_head = next_head;
		next_head = get_line(&line_head);
		if (line_head == NULL)
			break;

		if (line_head[0] == '/')
			continue;

		num = get_offset_val(line_head, &u4bRegOffset, &u4bRegValue);
		if (num > 0) {
			if (u4bRegOffset == 0xff)
				break;
			else if (u4bRegOffset == 0xfe)
				delay_ms(50);	// Delay 50 ms. Only RF configuration require delay
			else if (num == 2) {
#if	defined(RTL8190) || defined(RTL8192E)
				PHY_SetRFReg(priv, eRFPath, u4bRegOffset, bMask12Bits, u4bRegValue);
#elif defined(RTL8192SE) || defined(RTL8192SU)
				PHY_SetRFReg(priv, eRFPath, u4bRegOffset, bMask20Bits, u4bRegValue);
#endif
				delay_ms(1);
			}
		}
	}

#ifndef MERGE_FW
	sys_close(fd);
	set_fs(old_fs);
#endif

	kfree(mem_ptr);

	return 0;
}


int PHY_ConfigMACWithParaFile(struct rtl8190_priv *priv)
{
	int  read_bytes, num, len=0;
	unsigned int  u4bRegOffset, u4bRegMask, u4bRegValue;
	unsigned char *mem_ptr, *line_head, *next_head;
#ifdef MERGE_FW
#ifdef RTL8192SE
	ULONG ioaddr = priv->pshare->ioaddr;
#endif
#else
	int fd=0;
	mm_segment_t  old_fs;
#if	defined(RTL8190) || defined(RTL8192E)
	unsigned char *pFileName = "/usr/rtl8190Pci/MACPHY_REG.txt";
#elif	defined(RTL8192SE)
	unsigned char *pFileName = "/usr/rtl8192Pci/MACPHY_REG.txt";
	ULONG ioaddr = priv->pshare->ioaddr;
#elif	defined(RTL8192SU)
	unsigned char *pFileName = "/usr/rtl8192su/macreg.txt";
#endif
#ifndef CONFIG_X86
	extern ssize_t sys_read(unsigned int fd, char * buf, size_t count);
#endif
#endif
	struct MacRegTable *reg_table = (struct MacRegTable *)priv->pshare->mac_reg_buf;

#ifdef CHECK_HANGUP
	if (!priv->reset_hangup)
#endif
	{
		if((mem_ptr = (unsigned char *)kmalloc(MAX_CONFIG_FILE_SIZE, GFP_ATOMIC)) == NULL) {
			printk("PHY_ConfigMACWithParaFile(): not enough memory\n");
			return -1;
		}

		memset(mem_ptr, 0, MAX_CONFIG_FILE_SIZE); // clear memory

#ifdef MERGE_FW
		read_bytes = (int)(__MACPHY_REG_end - __MACPHY_REG_start);
		memcpy(mem_ptr, __MACPHY_REG_start, read_bytes);
#else

		old_fs = get_fs();
		set_fs(KERNEL_DS);

#if	defined(CONFIG_X86)
		if ((fd = rtl8190n_fileopen(pFileName, O_RDONLY, 0)) < 0)
#else
		if ((fd = sys_open(pFileName, O_RDONLY, 0)) < 0)
#endif
		{
			printk("PHY_ConfigMACWithParaFile(): cannot open %s\n", pFileName);
			set_fs(old_fs);
			kfree(mem_ptr);
			return -1;
		}

		read_bytes = sys_read(fd, mem_ptr, MAX_CONFIG_FILE_SIZE);
#endif // MERGE_FW

		next_head = mem_ptr;
		while (1) {
			line_head = next_head;
			next_head = get_line(&line_head);
			if (line_head == NULL)
				break;

			if (line_head[0] == '/')
				continue;

#if !defined(RTL8192SU)
			num = get_offset_mask_val(line_head, &u4bRegOffset, &u4bRegMask, &u4bRegValue);
#else
			u4bRegMask = 0;
			num = get_offset_val_macreg(line_head, &u4bRegOffset, &u4bRegValue);
#endif		
			if (num > 0) {
				reg_table[len].offset = u4bRegOffset;
#if !defined(RTL8192SU)
				reg_table[len].mask = u4bRegMask;
#endif			
				reg_table[len].value = u4bRegValue;
				len++;
				if (u4bRegOffset == 0xff)
					break;
				if ((len * sizeof(struct MacRegTable)) > MAC_REG_SIZE)
					break;
			}
		}

#ifndef MERGE_FW
		sys_close(fd);
		set_fs(old_fs);
#endif

		kfree(mem_ptr);

		if ((len * sizeof(struct MacRegTable)) > MAC_REG_SIZE) {
			printk("MAC REG table buffer not large enough!\n");
			return -1;
		}
	}

	num = 0;
	while (1) {
		u4bRegOffset = reg_table[num].offset;
#if !defined(RTL8192SU)
		u4bRegMask = reg_table[num].mask;
#endif		
		u4bRegValue = reg_table[num].value;

		if (u4bRegOffset == 0xff)
			break;
		else
#if defined(RTL8190) || defined(RTL8192E)
			PHY_SetBBReg(priv, u4bRegOffset, u4bRegMask, u4bRegValue);
#elif defined(RTL8192SE) || defined(RTL8192SU)
			RTL_W8(u4bRegOffset, u4bRegValue);
#endif
		num++;
	}

	return 0;
}


#ifdef UNIVERSAL_REPEATER
static struct rtl8190_priv *get_another_interface_priv(struct rtl8190_priv *priv)
{
	if (IS_DRV_OPEN(GET_VXD_PRIV(priv)))
		return GET_VXD_PRIV(priv);
	else if (IS_DRV_OPEN(GET_ROOT_PRIV(priv)))
		return GET_ROOT_PRIV(priv);
	else
		return NULL;
}


static int get_shortslot_for_another_interface(struct rtl8190_priv *priv)
{
	struct rtl8190_priv *p_priv;

	p_priv = get_another_interface_priv(priv);
	if (p_priv) {
		if (p_priv->pmib->dot11OperationEntry.opmode & WIFI_AP_STATE)
			return (p_priv->pmib->dot11ErpInfo.shortSlot);
		else {
			if (p_priv->pmib->dot11OperationEntry.opmode & WIFI_ASOC_STATE)
				return (p_priv->pmib->dot11ErpInfo.shortSlot);
		}
	}
	return -1;
}
#endif // UNIVERSAL_REPEATER


void set_slot_time(struct rtl8190_priv *priv, int use_short)
{
#if !defined(RTL8192SU)
	ULONG ioaddr = priv->pshare->ioaddr;
#endif	

#ifdef UNIVERSAL_REPEATER
	int is_short;
	is_short = get_shortslot_for_another_interface(priv);
	if (is_short != -1) { // not abtained
		use_short &= is_short;
	}
#endif

	if (use_short)
		RTL_W8(_SLOT_, 0x09);
	else
		RTL_W8(_SLOT_, 0x14);
}


#if defined(RTL8190) || defined(RTL8192E)
void SetTxPowerLevel(struct rtl8190_priv *priv)
{
#ifdef RTL8190
	ULONG ioaddr = priv->pshare->ioaddr;
#endif
	unsigned char pwrlevel, val8;
	unsigned int val32;
	unsigned char *mcs_enhance_array;

	mcs_enhance_array = priv->pshare->phw->MCSTxAgcOffset;

	if (!priv->pshare->use_default_para) {
		pwrlevel = priv->pmib->dot11RFEntry.pwrlevelOFDM[priv->pshare->working_channel-1];
		val32 = 0;
		// MCS11, MCS3, 24
		val8 = pwrlevel + mcs_enhance_array[4];
		if (val8 > 36)	val8 = 36;
		val32 |= (val8 << 24);
		// MCS10, MCS2, 18
		val8 = pwrlevel + mcs_enhance_array[5];
		if (val8 > 36)	val8 = 36;
		val32 |= (val8 << 16);
		// MCS9, MCS1, 12
		val8 = pwrlevel + mcs_enhance_array[6];
		if (val8 > 36)	val8 = 36;
		val32 |= (val8 << 8);
		// MCS8, MCS0, 9, 6
		val8 = pwrlevel + mcs_enhance_array[7];
		if (val8 > 36)	val8 = 36;
		val32 |= val8;
		priv->pshare->phw->MCSTxAgc0 = val32;

		val32 = 0;
		// MCS15, MCS7
		val8 = pwrlevel + mcs_enhance_array[0];
		if (val8 > 36)	val8 = 36;
		val32 |= (val8 << 24);
		// MCS14, MCS6, 54
		val8 = pwrlevel + mcs_enhance_array[1];
		if (val8 > 36)	val8 = 36;
		val32 |= (val8 << 16);
		// MCS13, MCS5, 48
		val8 = pwrlevel + mcs_enhance_array[2];
		if (val8 > 36)	val8 = 36;
		val32 |= (val8 << 8);
		// MCS12, MCS4, 36
		val8 = pwrlevel + mcs_enhance_array[3];
		if (val8 > 36)	val8 = 36;
		val32 |= val8;
		priv->pshare->phw->MCSTxAgc1 = val32;

		pwrlevel = priv->pmib->dot11RFEntry.pwrlevelCCK[priv->pshare->working_channel-1];
		val32 = 0;
		// 11, 5.5
		val8 = pwrlevel;
		if (val8 > 36)	val8 = 36;
		val32 |= (val8 << 8);
		// 2, 1
		val8 = pwrlevel + 0;
		if (val8 > 36)	val8 = 36;
		val32 |= val8;
		priv->pshare->phw->CCKTxAgc = val32;

#ifdef RTL8190
		if (priv->pmib->dot11RFEntry.pwrlevelOFDM[priv->pshare->working_channel-1+14] != 0)
			priv->pshare->channelAC_pwrdiff =
			        (int)priv->pmib->dot11RFEntry.pwrlevelOFDM[priv->pshare->working_channel-1+14] -
			        (int)priv->pmib->dot11RFEntry.pwrlevelOFDM[priv->pshare->working_channel-1];
		else
			priv->pshare->channelAC_pwrdiff = (int)priv->pmib->dot11RFEntry.antC_pwrdiff - 32;
		PHY_SetBBReg(priv, rFPGA0_TxGainStage, bXCTxAGC, (priv->pshare->channelAC_pwrdiff & 0x0f));
#endif
	}

#ifdef RTL8190
	RTL_W32(_MCS_TXAGC_0_, priv->pshare->phw->MCSTxAgc0);
	RTL_W32(_MCS_TXAGC_1_, priv->pshare->phw->MCSTxAgc1);
	RTL_W32(_CCK_TXAGC_, priv->pshare->phw->CCKTxAgc);
#elif defined(RTL8192E)
	PHY_SetBBReg(priv, 0xe10, 0x7f7f7f7f, priv->pshare->phw->MCSTxAgc0);
	PHY_SetBBReg(priv, 0xe18, 0x7f7f7f7f, priv->pshare->phw->MCSTxAgc0);
	PHY_SetBBReg(priv, 0xe14, 0x7f7f7f7f, priv->pshare->phw->MCSTxAgc1);
	PHY_SetBBReg(priv, 0xe1c, 0x7f7f7f7f, priv->pshare->phw->MCSTxAgc1);
	PHY_SetBBReg(priv, rTxAGC_CCK_Mcs32, bTxAGCRateCCK, priv->pshare->phw->CCKTxAgc);
#endif
}
#endif


void SwChnl(struct rtl8190_priv *priv, unsigned char channel, int offset)
{
#if defined(RTL8190) || defined(RTL8192E)
	static RF90_RADIO_PATH_E eRFPath;
	static unsigned int val, RetryCnt, RfRetVal;
#elif defined(RTL8192SE) || defined(RTL8192SU)
	static unsigned int val;
#endif

	val = channel;

	if (channel > 14)
		priv->pshare->curr_band = BAND_5G;
	else
		priv->pshare->curr_band = BAND_2G;

	if (priv->pshare->CurrentChannelBW) {
		if (offset == 1)
			val -= 2;
		else
			val += 2;
	}

	if (priv->pshare->rf_ft_var.use_frq_2_3G)
		val += 14;
#if	defined(RTL8192SE) || defined(RTL8192SU)
	{
//		return ; // for 40Mhz
		unsigned int val_read;
		val_read = PHY_QueryRFReg(priv, 0, 0x18, bMask12Bits, 1);
		val_read &= 0xfffffff0;
		PHY_SetRFReg(priv, 0, 0x18, bMask12Bits, val_read | val);
		PHY_SetRFReg(priv, 1, 0x18, bMask12Bits, val_read | val);

	}

	if (priv->pshare->rf_ft_var.use_frq_2_3G)
		val -= 14;

	priv->pshare->working_channel = val;
	PHY_RF6052SetOFDMTxPower(priv, priv->pmib->dot11RFEntry.pwrlevelOFDM_1SS[priv->pshare->working_channel-1],
		priv->pmib->dot11RFEntry.pwrlevelOFDM_2SS[priv->pshare->working_channel-1]);

	if (!priv->pshare->use_default_para) {
#ifdef ADD_TX_POWER_BY_CMD
		if (priv->pshare->rf_ft_var.txPowerPlus_cck != 0xff)
			PHY_SetBBReg(priv, rTxAGC_CCK_Mcs32, bTxAGCRateCCK, priv->pmib->dot11RFEntry.pwrlevelCCK[priv->pshare->working_channel-1]+priv->pshare->rf_ft_var.txPowerPlus_cck);
		else
#endif
			PHY_SetBBReg(priv, rTxAGC_CCK_Mcs32, bTxAGCRateCCK, priv->pmib->dot11RFEntry.pwrlevelCCK[priv->pshare->working_channel-1]);
	}

	return;
#else

	for (eRFPath = RF90_PATH_A; eRFPath<priv->pshare->phw->NumTotalRFPath; eRFPath++) {
#ifdef RTL8190
		if ((get_rf_mimo_mode(priv) == MIMO_1T2R) || (get_rf_mimo_mode(priv) == MIMO_1T1R)) {
			if ((eRFPath == RF90_PATH_A) || (eRFPath == RF90_PATH_B) || (eRFPath == RF90_PATH_D))
				continue;
		}
		else if ((get_rf_mimo_mode(priv) == MIMO_2T4R) || (get_rf_mimo_mode(priv) == MIMO_2T2R)) {
			if ((eRFPath == RF90_PATH_B) || (eRFPath == RF90_PATH_D))
				continue;
		}
#elif defined(RTL8192E)
		if ((eRFPath == RF90_PATH_B) || (eRFPath == RF90_PATH_C) || (eRFPath == RF90_PATH_D))
			continue;
#endif
		RetryCnt = 3;
		do {
			PHY_SetRFReg(priv, eRFPath, rZebra1_Channel, bZebra1_ChannelNum, val);
			delay_ms(10);

			RfRetVal = PHY_QueryRFReg(priv, eRFPath, rZebra1_Channel, bZebra1_ChannelNum, 1);
			RetryCnt--;
		} while ((val!=RfRetVal) && (RetryCnt!=0));
	}

	if (priv->pshare->rf_ft_var.use_frq_2_3G)
		val -= 14;

	priv->pshare->working_channel = val;
	SetTxPowerLevel(priv);

	/*----------To ensure PHY operates correctly----------*/
	if (priv->pmib->dot11RFEntry.dot11RFType == _11BGN_RF_8256_) {
		// for 8256, in case rf switchs to debug mode
		// have to be refined when register 0 format has been changed
		PHY_SetRFReg(priv, RF90_PATH_A, 0, bMask12Bits, 0xc9f);
		PHY_SetRFReg(priv, RF90_PATH_B, 0, bMask12Bits, 0xcbf);
		PHY_SetRFReg(priv, RF90_PATH_C, 0, bMask12Bits, 0xc9f);
		PHY_SetRFReg(priv, RF90_PATH_D, 0, bMask12Bits, 0xcbf);
	}
#endif
}


void SwitchAntenna(struct rtl8190_priv *priv)
{
}

#if defined(RTL8192SE) || defined(RTL8192SU)


#ifdef EXT_ANT_DVRY
// SwitchExtAnt(), EXT_ANT_PATH specifies which external antennna is been used.
// EXT_ANT_PATH: 0 -- default, 1 -- extra ant
void SwitchExtAnt(struct rtl8190_priv *priv, unsigned char EXT_ANT_PATH){

	ULONG ioaddr = priv->pshare->ioaddr;
//	panic_printk("EXT_ANT_PATH = %d\n", EXT_ANT_PATH);

	if(EXT_ANT_PATH == 0){
		RTL_W32(0x2ec, 0xff0400);
		priv->pshare->EXT_AD_selection = 0;
	}
	else{
		RTL_W32(0x2ec, 0xff0800);
		priv->pshare->EXT_AD_selection = 1;
	}
}
#endif

// switch 1 spatial stream path
//antPath: 01 for PathA,10 for PathB, 11for Path AB
void Switch_1SS_Antenna(struct rtl8190_priv *priv, unsigned int antPath )
{
#if !defined(RTL8192SU)
	ULONG ioaddr = priv->pshare->ioaddr;
#endif	
	unsigned int dword = 0;
	if(get_rf_mimo_mode(priv) != MIMO_2T2R)
		return;

	switch(antPath){
	case 1:
		dword = RTL_R32(0x90C);
		if((dword & 0x0ff00000) == 0x01100000)
			goto switch_1ss_end;
		dword &= 0xf00fffff;
		dword |= 0x01100000; // Path A
		RTL_W32(0x90C, dword);
		break;
	case 2:
		dword = RTL_R32(0x90C);
		if((dword & 0x0ff00000) == 0x02200000)
			goto switch_1ss_end;
		dword &= 0xf00fffff;
		dword |= 0x02200000;	// Path B
		RTL_W32(0x90C, dword);
		break;

	case 3:
		if(priv->pshare->rf_ft_var.ofdm_1ss_oneAnt == 1)// use one ANT for 1ss
			goto switch_1ss_end;// do nothing
		dword = RTL_R32(0x90C);
		if((dword & 0x0ff00000) == 0x03300000)
			goto switch_1ss_end;
		dword &= 0xf00fffff;
		dword |= 0x03300000; // Path A,B
		RTL_W32(0x90C, dword);
		break;

	default:// do nothing
		break;
	}
switch_1ss_end:
	return;

}

// switch OFDM path
//antPath: 01 for PathA,10 for PathB, 11for Path AB
void Switch_OFDM_Antenna(struct rtl8190_priv *priv, unsigned int antPath )
{
#if !defined(RTL8192SU)
	ULONG ioaddr = priv->pshare->ioaddr;
#endif	
	unsigned int dword = 0;
	if(get_rf_mimo_mode(priv) != MIMO_2T2R)
		return;

	switch(antPath){
	case 1:
		dword = RTL_R32(0x90C);
		if((dword & 0x000000f0) == 0x00000010)
			goto switch_OFDM_end;
		dword &= 0xffffff0f;
		dword |= 0x00000010; // Path A
		RTL_W32(0x90C, dword);
		break;
	case 2:
		dword = RTL_R32(0x90C);
		if((dword & 0x000000f0) == 0x00000020)
			goto switch_OFDM_end;
		dword &= 0xffffff0f;
		dword |= 0x00000020;	// Path B
		RTL_W32(0x90C, dword);
		break;

	case 3:
		if(priv->pshare->rf_ft_var.ofdm_1ss_oneAnt == 1)// use one ANT for 1ss
			goto switch_OFDM_end;// do nothing
		dword = RTL_R32(0x90C);
		if((dword & 0x000000f0) == 0x00000030)
			goto switch_OFDM_end;
		dword &= 0xffffff0f;
		dword |= 0x00000030; // Path A,B
		RTL_W32(0x90C, dword);
		break;

	default:// do nothing
		break;
	}
switch_OFDM_end:
	return;

}


#endif

void enable_hw_LED(struct rtl8190_priv *priv, unsigned int led_type)
{
#if !defined(RTL8192SU)
	ULONG ioaddr = priv->pshare->ioaddr;
#endif	

#if defined(RTL8190) || defined(RTL8192E)
	switch (led_type) {
	case LEDTYPE_HW_TX_RX:
		RTL_W8(_LED0CFG_, 0x04);
		RTL_W8(_LED1CFG_, 0x06);
		break;
	case LEDTYPE_HW_LINKACT_INFRA:
		RTL_W8(_LED0CFG_, 0x02);
		if ((OPMODE & WIFI_AP_STATE) || (OPMODE & WIFI_STATION_STATE))
			RTL_W8(_LED1CFG_, 0x00);
		else
			RTL_W8(_LED1CFG_, 0x80);
		break;
	default:
		break;
	}
#elif defined(RTL8192SE) || defined(RTL8192SU)
	switch (led_type) {
	case LEDTYPE_HW_TX_RX:
		RTL_W8(LEDCFG, 0x64);
		break;
	case LEDTYPE_HW_LINKACT_INFRA:
		RTL_W8(LEDCFG, 0x02);
		if ((OPMODE & WIFI_AP_STATE) || (OPMODE & WIFI_STATION_STATE))
			RTL_W8(LEDCFG, RTL_R8(LEDCFG) & 0x0f);
		else
			RTL_W8(LEDCFG, RTL_R8(LEDCFG) | 0x80);
		break;
	default:
		break;
	}
#endif
}


/**
* Function:	phy_InitBBRFRegisterDefinition
*
* OverView:	Initialize Register definition offset for Radio Path A/B/C/D
*
* Input:
*			PADAPTER		Adapter,
*
* Output:	None
* Return:		None
* Note:		The initialization value is constant and it should never be changes
*/
void phy_InitBBRFRegisterDefinition(struct rtl8190_priv *priv)
{
	struct rtl8190_hw *phw = GET_HW(priv);

	// RF Interface Sowrtware Control
	phw->PHYRegDef[RF90_PATH_A].rfintfs = rFPGA0_XAB_RFInterfaceSW; // 16 LSBs if read 32-bit from 0x870
	phw->PHYRegDef[RF90_PATH_B].rfintfs = rFPGA0_XAB_RFInterfaceSW; // 16 MSBs if read 32-bit from 0x870 (16-bit for 0x872)
	phw->PHYRegDef[RF90_PATH_C].rfintfs = rFPGA0_XCD_RFInterfaceSW;// 16 LSBs if read 32-bit from 0x874
	phw->PHYRegDef[RF90_PATH_D].rfintfs = rFPGA0_XCD_RFInterfaceSW;// 16 MSBs if read 32-bit from 0x874 (16-bit for 0x876)

	// RF Interface Readback Value
	phw->PHYRegDef[RF90_PATH_A].rfintfi = rFPGA0_XAB_RFInterfaceRB; // 16 LSBs if read 32-bit from 0x8E0
	phw->PHYRegDef[RF90_PATH_B].rfintfi = rFPGA0_XAB_RFInterfaceRB;// 16 MSBs if read 32-bit from 0x8E0 (16-bit for 0x8E2)
	phw->PHYRegDef[RF90_PATH_C].rfintfi = rFPGA0_XCD_RFInterfaceRB;// 16 LSBs if read 32-bit from 0x8E4
	phw->PHYRegDef[RF90_PATH_D].rfintfi = rFPGA0_XCD_RFInterfaceRB;// 16 MSBs if read 32-bit from 0x8E4 (16-bit for 0x8E6)

	// RF Interface Output (and Enable)
	phw->PHYRegDef[RF90_PATH_A].rfintfo = rFPGA0_XA_RFInterfaceOE; // 16 LSBs if read 32-bit from 0x860
	phw->PHYRegDef[RF90_PATH_B].rfintfo = rFPGA0_XB_RFInterfaceOE; // 16 LSBs if read 32-bit from 0x864
	phw->PHYRegDef[RF90_PATH_C].rfintfo = rFPGA0_XC_RFInterfaceOE;// 16 LSBs if read 32-bit from 0x868
	phw->PHYRegDef[RF90_PATH_D].rfintfo = rFPGA0_XD_RFInterfaceOE;// 16 LSBs if read 32-bit from 0x86C

	// RF Interface (Output and)  Enable
	phw->PHYRegDef[RF90_PATH_A].rfintfe = rFPGA0_XA_RFInterfaceOE; // 16 MSBs if read 32-bit from 0x860 (16-bit for 0x862)
	phw->PHYRegDef[RF90_PATH_B].rfintfe = rFPGA0_XB_RFInterfaceOE; // 16 MSBs if read 32-bit from 0x864 (16-bit for 0x866)
	phw->PHYRegDef[RF90_PATH_C].rfintfe = rFPGA0_XC_RFInterfaceOE;// 16 MSBs if read 32-bit from 0x86A (16-bit for 0x86A)
	phw->PHYRegDef[RF90_PATH_D].rfintfe = rFPGA0_XD_RFInterfaceOE;// 16 MSBs if read 32-bit from 0x86C (16-bit for 0x86E)

	//Addr of LSSI. Wirte RF register by driver
	phw->PHYRegDef[RF90_PATH_A].rf3wireOffset = rFPGA0_XA_LSSIParameter; //LSSI Parameter
	phw->PHYRegDef[RF90_PATH_B].rf3wireOffset = rFPGA0_XB_LSSIParameter;
	phw->PHYRegDef[RF90_PATH_C].rf3wireOffset = rFPGA0_XC_LSSIParameter;
	phw->PHYRegDef[RF90_PATH_D].rf3wireOffset = rFPGA0_XD_LSSIParameter;

	// RF parameter
	phw->PHYRegDef[RF90_PATH_A].rfLSSI_Select = rFPGA0_XAB_RFParameter;  //BB Band Select
	phw->PHYRegDef[RF90_PATH_B].rfLSSI_Select = rFPGA0_XAB_RFParameter;
	phw->PHYRegDef[RF90_PATH_C].rfLSSI_Select = rFPGA0_XCD_RFParameter;
	phw->PHYRegDef[RF90_PATH_D].rfLSSI_Select = rFPGA0_XCD_RFParameter;

	// Tx AGC Gain Stage (same for all path. Should we remove this?)
	phw->PHYRegDef[RF90_PATH_A].rfTxGainStage = rFPGA0_TxGainStage; //Tx gain stage
	phw->PHYRegDef[RF90_PATH_B].rfTxGainStage = rFPGA0_TxGainStage; //Tx gain stage
	phw->PHYRegDef[RF90_PATH_C].rfTxGainStage = rFPGA0_TxGainStage; //Tx gain stage
	phw->PHYRegDef[RF90_PATH_D].rfTxGainStage = rFPGA0_TxGainStage; //Tx gain stage

	// Tranceiver A~D HSSI Parameter-1
	phw->PHYRegDef[RF90_PATH_A].rfHSSIPara1 = rFPGA0_XA_HSSIParameter1;  //wire control parameter1
	phw->PHYRegDef[RF90_PATH_B].rfHSSIPara1 = rFPGA0_XB_HSSIParameter1;  //wire control parameter1
	phw->PHYRegDef[RF90_PATH_C].rfHSSIPara1 = rFPGA0_XC_HSSIParameter1;  //wire control parameter1
	phw->PHYRegDef[RF90_PATH_D].rfHSSIPara1 = rFPGA0_XD_HSSIParameter1;  //wire control parameter1

	// Tranceiver A~D HSSI Parameter-2
	phw->PHYRegDef[RF90_PATH_A].rfHSSIPara2 = rFPGA0_XA_HSSIParameter2;  //wire control parameter2
	phw->PHYRegDef[RF90_PATH_B].rfHSSIPara2 = rFPGA0_XB_HSSIParameter2;  //wire control parameter2
	phw->PHYRegDef[RF90_PATH_C].rfHSSIPara2 = rFPGA0_XC_HSSIParameter2;  //wire control parameter2
	phw->PHYRegDef[RF90_PATH_D].rfHSSIPara2 = rFPGA0_XD_HSSIParameter2;  //wire control parameter1

	// RF switch Control
	phw->PHYRegDef[RF90_PATH_A].rfSwitchControl = rFPGA0_XAB_SwitchControl; //TR/Ant switch control
	phw->PHYRegDef[RF90_PATH_B].rfSwitchControl = rFPGA0_XAB_SwitchControl;
	phw->PHYRegDef[RF90_PATH_C].rfSwitchControl = rFPGA0_XCD_SwitchControl;
	phw->PHYRegDef[RF90_PATH_D].rfSwitchControl = rFPGA0_XCD_SwitchControl;

	// AGC control 1
	phw->PHYRegDef[RF90_PATH_A].rfAGCControl1 = rOFDM0_XAAGCCore1;
	phw->PHYRegDef[RF90_PATH_B].rfAGCControl1 = rOFDM0_XBAGCCore1;
	phw->PHYRegDef[RF90_PATH_C].rfAGCControl1 = rOFDM0_XCAGCCore1;
	phw->PHYRegDef[RF90_PATH_D].rfAGCControl1 = rOFDM0_XDAGCCore1;

	// AGC control 2
	phw->PHYRegDef[RF90_PATH_A].rfAGCControl2 = rOFDM0_XAAGCCore2;
	phw->PHYRegDef[RF90_PATH_B].rfAGCControl2 = rOFDM0_XBAGCCore2;
	phw->PHYRegDef[RF90_PATH_C].rfAGCControl2 = rOFDM0_XCAGCCore2;
	phw->PHYRegDef[RF90_PATH_D].rfAGCControl2 = rOFDM0_XDAGCCore2;

	// RX AFE control 1
	phw->PHYRegDef[RF90_PATH_A].rfRxIQImbalance = rOFDM0_XARxIQImbalance;
	phw->PHYRegDef[RF90_PATH_B].rfRxIQImbalance = rOFDM0_XBRxIQImbalance;
	phw->PHYRegDef[RF90_PATH_C].rfRxIQImbalance = rOFDM0_XCRxIQImbalance;
	phw->PHYRegDef[RF90_PATH_D].rfRxIQImbalance = rOFDM0_XDRxIQImbalance;

	// RX AFE control 1
	phw->PHYRegDef[RF90_PATH_A].rfRxAFE = rOFDM0_XARxAFE;
	phw->PHYRegDef[RF90_PATH_B].rfRxAFE = rOFDM0_XBRxAFE;
	phw->PHYRegDef[RF90_PATH_C].rfRxAFE = rOFDM0_XCRxAFE;
	phw->PHYRegDef[RF90_PATH_D].rfRxAFE = rOFDM0_XDRxAFE;

	// Tx AFE control 1
	phw->PHYRegDef[RF90_PATH_A].rfTxIQImbalance = rOFDM0_XATxIQImbalance;
	phw->PHYRegDef[RF90_PATH_B].rfTxIQImbalance = rOFDM0_XBTxIQImbalance;
	phw->PHYRegDef[RF90_PATH_C].rfTxIQImbalance = rOFDM0_XCTxIQImbalance;
	phw->PHYRegDef[RF90_PATH_D].rfTxIQImbalance = rOFDM0_XDTxIQImbalance;

	// Tx AFE control 2
	phw->PHYRegDef[RF90_PATH_A].rfTxAFE = rOFDM0_XATxAFE;
	phw->PHYRegDef[RF90_PATH_B].rfTxAFE = rOFDM0_XBTxAFE;
	phw->PHYRegDef[RF90_PATH_C].rfTxAFE = rOFDM0_XCTxAFE;
	phw->PHYRegDef[RF90_PATH_D].rfTxAFE = rOFDM0_XDTxAFE;

	// Tranceiver LSSI Readback
	phw->PHYRegDef[RF90_PATH_A].rfLSSIReadBack = rFPGA0_XA_LSSIReadBack;
	phw->PHYRegDef[RF90_PATH_B].rfLSSIReadBack = rFPGA0_XB_LSSIReadBack;
	phw->PHYRegDef[RF90_PATH_C].rfLSSIReadBack = rFPGA0_XC_LSSIReadBack;
	phw->PHYRegDef[RF90_PATH_D].rfLSSIReadBack = rFPGA0_XD_LSSIReadBack;
#if defined(RTL8192SE) || defined(RTL8192SU)
	phw->PHYRegDef[RF90_PATH_A].rfLSSIReadBack_92S_PI = 0x8b8;
	phw->PHYRegDef[RF90_PATH_B].rfLSSIReadBack_92S_PI = 0x8bc;
#endif
}


/*-----------------------------------------------------------------------------
 * Function:    PHY_CheckBBAndRFOK()
 *
 * Overview:    This function is write register and then readback to make sure whether
 *			  BB[PHY0, PHY1], RF[Patha, path b, path c, path d] is Ok
 *
 * Input:      	PADAPTER			Adapter
 *			HW90_BLOCK_E		CheckBlock
 *			RF90_RADIO_PATH_E	eRFPath		// it is used only when CheckBlock is HW90_BLOCK_RF
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: PHY is OK
 *
 * Note:		This function may be removed in the ASIC
 *---------------------------------------------------------------------------*/
int PHY_CheckBBAndRFOK(struct rtl8190_priv *priv, HW90_BLOCK_E CheckBlock, RF90_RADIO_PATH_E eRFPath)
{
#if !defined(RTL8192SU)
	unsigned long ioaddr = priv->pshare->ioaddr;
#endif	
	int rtStatus = 0;
	unsigned int i, CheckTimes = 4, ulRegRead = 0;
	unsigned int WriteAddr[4];
	unsigned int WriteData[] = {0xfffff027, 0xaa55a02f, 0x00000027, 0x55aa502f};

	// Initialize register address offset to be checked
	WriteAddr[HW90_BLOCK_MAC] = 0x100;
	WriteAddr[HW90_BLOCK_PHY0] = 0x900;
	WriteAddr[HW90_BLOCK_PHY1] = 0x800;
	WriteAddr[HW90_BLOCK_RF] = 0x3;

	for(i=0; i<CheckTimes; i++)
	{
		//
		// Write Data to register and readback
		//
		switch(CheckBlock)
		{
		case HW90_BLOCK_MAC:
			break;

		case HW90_BLOCK_PHY0:
		case HW90_BLOCK_PHY1:
			RTL_W32(WriteAddr[CheckBlock], WriteData[i]);
			ulRegRead = RTL_R32(WriteAddr[CheckBlock]);
			break;

		case HW90_BLOCK_RF:
			WriteData[i] &= 0xfff;
			PHY_SetRFReg(priv, eRFPath, WriteAddr[HW90_BLOCK_RF], bMaskDWord, WriteData[i]);
			// TODO: we should not delay for such a long time. Ask SD3
			delay_ms(10);
			ulRegRead = PHY_QueryRFReg(priv, eRFPath, WriteAddr[HW90_BLOCK_RF], bMaskDWord, 1);
			delay_ms(10);
			break;

		default:
			rtStatus = -1;
			break;
		}

		//
		// Check whether readback data is correct
		//
		if(ulRegRead != WriteData[i])
		{
			printk("ulRegRead: %x, WriteData: %x \n", ulRegRead, WriteData[i]);
			rtStatus = -1;
			break;
		}
	}

	return rtStatus;
}


void check_MIMO_TR_status(struct rtl8190_priv *priv)
{
#ifdef RTL8190
	unsigned long ioaddr = priv->pshare->ioaddr;
	int MIMO_TR_status;

	if ((GET_HW(priv)->MIMO_TR_hw_support == MIMO_2T4R) ||
		(GET_HW(priv)->MIMO_TR_hw_support == MIMO_1T2R))
		return;

	RTL_W8(_BBGLBRESET_, RTL_R8(_BBGLBRESET_)|0x01);
	RTL_W32(_CPURST_, RTL_R32(_CPURST_)|0x100);
	RTL_W32(0x860, 0x001f0010);
	RTL_W32(0x864, 0x007f0010);
	PHY_SetRFReg(priv, RF90_PATH_A, 0, 0xfff, 0xbf);
	MIMO_TR_status = PHY_QueryRFReg(priv, RF90_PATH_A, 0, 0xff, 0);
	if ((MIMO_TR_status == 0xbf) || (MIMO_TR_status == 0xb8) || (MIMO_TR_status == 0x98) || (MIMO_TR_status == 0x9f))
		GET_HW(priv)->MIMO_TR_hw_support = MIMO_2T4R;
	else
		GET_HW(priv)->MIMO_TR_hw_support = MIMO_1T2R;
	PHY_QueryRFReg(priv, RF90_PATH_A, 0, 0xff, 1);
#elif defined(RTL8192E)
	GET_HW(priv)->MIMO_TR_hw_support = MIMO_1T2R;
#elif defined(RTL8192SE) || defined(RTL8192SU)
#if !defined(RTL8192SU)
	unsigned long ioaddr = priv->pshare->ioaddr;
#endif

	if (RTL_R32(PMC_FSM) & BIT(24))
		GET_HW(priv)->MIMO_TR_hw_support = MIMO_2T2R;
	else
		GET_HW(priv)->MIMO_TR_hw_support = MIMO_1T2R;
	return;
#endif
}


#if	defined(RTL8192SE) || defined(RTL8192SU)


/*-----------------------------------------------------------------------------
 * Function:	PHY_IQCalibrateBcut()
 *
 * Overview:	After all MAC/PHY/RF is configued. We must execute IQ calibration
 *			to improve RF EVM!!?
 *
 * Input:		IN	PADAPTER	pAdapter
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	11/18/2008	MHC		Create. Document from SD3 RFSI Jenyu.
 *						92S B-cut QFN 68 pin IQ calibration procedure.doc
 *
 *---------------------------------------------------------------------------*/
#if 0
void PHY_IQCalibrateBcut(	struct rtl8190_priv *priv)
{
//	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
//	PMGNT_INFO		pMgntInfo = &pAdapter->MgntInfo;
	unsigned int	i, reg;
	unsigned int	old_value;
	int				X, Y, TX0[4];
	unsigned int	TXA[4];
	unsigned int	calibrate_set[13] = {0};
	unsigned int	load_value[13];

	// 0. Check QFN68 or 64 92S (Read from EEPROM/EFUSE)

	//
	// 1. Save e70~ee0 register setting, and load calibration setting
	//
	/*
	0xee0[31:0]=0x3fed92fb;
	0xedc[31:0] =0x3fed92fb;
	0xe70[31:0] =0x3fed92fb;
	0xe74[31:0] =0x3fed92fb;
	0xe78[31:0] =0x3fed92fb;
	0xe7c[31:0]= 0x3fed92fb;
	0xe80[31:0]= 0x3fed92fb;
	0xe84[31:0]= 0x3fed92fb;
	0xe88[31:0]= 0x3fed92fb;
	0xe8c[31:0]= 0x3fed92fb;
	0xed0[31:0]= 0x3fed92fb;
	0xed4[31:0]= 0x3fed92fb;
	0xed8[31:0]= 0x3fed92fb;
	*/
	calibrate_set [0] = 0xee0;
	calibrate_set [1] = 0xedc;
	calibrate_set [2] = 0xe70;
	calibrate_set [3] = 0xe74;
	calibrate_set [4] = 0xe78;
	calibrate_set [5] = 0xe7c;
	calibrate_set [6] = 0xe80;
	calibrate_set [7] = 0xe84;
	calibrate_set [8] = 0xe88;
	calibrate_set [9] = 0xe8c;
	calibrate_set [10] = 0xed0;
	calibrate_set [11] = 0xed4;
	calibrate_set [12] = 0xed8;
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Save e70~ee0 register setting\n"));
	for (i = 0; i < 13; i++)
	{
		load_value[i] = PHY_QueryBBReg(priv, calibrate_set[i], bMaskDWord);
		PHY_SetBBReg(priv, calibrate_set[i], bMaskDWord, 0x3fed92fb);

	}

	//
	// 2. QFN 68
	//
	// For 1T2R IQK only now !!!
	for (i = 0; i < 10; i++)
	{
		//BB switch to PI mode. If default is PI mode, ignoring 2 commands below.
//if (pMgntInfo->bRFSiOrPi == 0)	// SI
		{
			PHY_SetBBReg(priv, 0x820, bMaskDWord, 0x01000100);
			PHY_SetBBReg(priv, 0x828, bMaskDWord, 0x01000100);
		}

		// IQK
		// 2. IQ calibration & LO leakage calibration
		PHY_SetBBReg(priv, 0xc04, bMaskDWord, 0x00a05430);
		delay_us(5);
		PHY_SetBBReg(priv, 0xc08, bMaskDWord, 0x000800e4);
		delay_us(5);
		PHY_SetBBReg(priv, 0xe28, bMaskDWord, 0x80800000);
		delay_us(5);
		//path-A IQ K and LO K gain setting
		PHY_SetBBReg(priv, 0xe40, bMaskDWord, 0x02140102);
		delay_us(5);
		PHY_SetBBReg(priv, 0xe44, bMaskDWord, 0x681604c2);
		delay_us(5);
		//set LO calibration
		PHY_SetBBReg(priv, 0xe4c, bMaskDWord, 0x000028d1);
		delay_us(5);
		//path-B IQ K and LO K gain setting
		PHY_SetBBReg(priv, 0xe60, bMaskDWord, 0x02140102);
		delay_us(5);
		PHY_SetBBReg(priv, 0xe64, bMaskDWord, 0x28160d05);
		delay_us(5);
		//K idac_I & IQ
		PHY_SetBBReg(priv, 0xe48, bMaskDWord, 0xfb000000);
		delay_us(5);
		PHY_SetBBReg(priv, 0xe48, bMaskDWord, 0xf8000000);
		delay_us(5);

		// delay 2ms
		delay_ms(2);

		//idac_Q setting
		PHY_SetBBReg(priv, 0xe6c, bMaskDWord, 0x020028d1);
		delay_us(5);
		//K idac_Q & IQ
		PHY_SetBBReg(priv, 0xe48, bMaskDWord, 0xfb000000);
		delay_us(5);
		PHY_SetBBReg(priv, 0xe48, bMaskDWord, 0xf8000000);

		// delay 2ms
		delay_ms(2);

		PHY_SetBBReg(priv, 0xc04, bMaskDWord, 0x00a05433);
		delay_us(5);
		PHY_SetBBReg(priv, 0xc08, bMaskDWord, 0x000000e4);
		delay_us(5);
		PHY_SetBBReg(priv, 0xe28, bMaskDWord, 0x0);

//		if (pMgntInfo->bRFSiOrPi == 0)	// SI
		{
			PHY_SetBBReg(priv, 0x820, bMaskDWord, 0x01000000);
			PHY_SetBBReg(priv, 0x828, bMaskDWord, 0x01000000);
		}


		reg = PHY_QueryBBReg(priv, 0xeac, bMaskDWord);

		// 3.	check fail bit, and fill BB IQ matrix
		// Readback IQK value and rewrite
		if (!(reg&(BIT(27)|BIT(28)|BIT(30)|BIT(31))))
		{
			old_value = (PHY_QueryBBReg(priv, 0xc80, bMaskDWord) & 0x3FF);

			// Calibrate init gain for A path for TX0
			X = (PHY_QueryBBReg(priv, 0xe94, bMaskDWord) & 0x03FF0000)>>16;
			TXA[RF90_PATH_A] = (X * old_value)/0x100;
			reg = PHY_QueryBBReg(priv, 0xc80, bMaskDWord);
			reg = (reg & 0xFFFFFC00) | (unsigned int)TXA[RF90_PATH_A];
			PHY_SetBBReg(priv, 0xc80, bMaskDWord, reg);
			delay_us(5);

			// Calibrate init gain for C path for TX0
			Y = ( PHY_QueryBBReg(priv, 0xe9C, bMaskDWord) & 0x03FF0000)>>16;
			TX0[RF90_PATH_C] = ((Y * old_value)/0x100);
			reg = PHY_QueryBBReg(priv, 0xc80, bMaskDWord);
			reg = (reg & 0xffc0ffff) |((unsigned int) (TX0[RF90_PATH_C]&0x3F)<<16);
			PHY_SetBBReg(priv, 0xc80, bMaskDWord, reg);
			reg = PHY_QueryBBReg(priv, 0xc94, bMaskDWord);
			reg = (reg & 0x0fffffff) |(((Y&0x3c0)>>6)<<28);
			PHY_SetBBReg(priv, 0xc94, bMaskDWord, reg);
			delay_us(5);

			// Calibrate RX A and B for RX0
			reg = PHY_QueryBBReg(priv, 0xc14, bMaskDWord);
			X = (PHY_QueryBBReg(priv, 0xea4, bMaskDWord) & 0x03FF0000)>>16;
			reg = (reg & 0xFFFFFC00) |X;
			PHY_SetBBReg(priv, 0xc14, bMaskDWord, reg);
			Y = (PHY_QueryBBReg(priv, 0xeac, bMaskDWord) & 0x003F0000)>>16;
			reg = (reg & 0xFFFF03FF) |Y<<10;
			PHY_SetBBReg(priv, 0xc14, bMaskDWord, reg);
			delay_us(5);
			old_value = (PHY_QueryBBReg(priv, 0xc88, bMaskDWord) & 0x3FF);

			// Calibrate init gain for A path for TX1 !!!!!!
			X = (PHY_QueryBBReg(priv, 0xeb4, bMaskDWord) & 0x03FF0000)>>16;
			reg = PHY_QueryBBReg(priv, 0xc88, bMaskDWord);
			TXA[RF90_PATH_A] = (X * old_value) / 0x100;
			reg = (reg & 0xFFFFFC00) | TXA[RF90_PATH_A];
			PHY_SetBBReg(priv, 0xc88, bMaskDWord, reg);
			delay_us(5);

			// Calibrate init gain for C path for TX1
			Y = (PHY_QueryBBReg(priv, 0xebc, bMaskDWord)& 0x03FF0000)>>16;
			TX0[RF90_PATH_C] = ((Y * old_value)/0x100);
			reg = PHY_QueryBBReg(priv, 0xc88, bMaskDWord);
			reg = (reg & 0xffc0ffff) |( (TX0[RF90_PATH_C]&0x3F)<<16);
			PHY_SetBBReg(priv, 0xc88, bMaskDWord, reg);
			reg = PHY_QueryBBReg(priv, 0xc9c, bMaskDWord);
			reg = (reg & 0x0fffffff) |(((Y&0x3c0)>>6)<<28);
			PHY_SetBBReg(priv, 0xc9c, bMaskDWord, reg);
			delay_us(5);

			// Calibrate RX A and B for RX1
			reg = PHY_QueryBBReg(priv, 0xc1c, bMaskDWord);
			X = (PHY_QueryBBReg(priv, 0xec4, bMaskDWord) & 0x03FF0000)>>16;
			reg = (reg & 0xFFFFFC00) |X;
			PHY_SetBBReg(priv, 0xc1c, bMaskDWord, reg);

			Y = (PHY_QueryBBReg(priv, 0xecc, bMaskDWord) & 0x003F0000)>>16;
			reg = (reg & 0xFFFF03FF) |Y<<10;
			PHY_SetBBReg(priv, 0xc1c, bMaskDWord, reg);
			delay_us(5);

			break;
		}

	}

	//
	// 4. Reload e70~ee0 register setting.
	//
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Reload e70~ee0 register setting.\n"));
	for (i = 0; i < 13; i++)
		PHY_SetBBReg(priv, calibrate_set[i], bMaskDWord, load_value[i]);


	//
	// 3. QFN64. Not enabled now !!! We must use different gain table for 1T2R.
	//



}	// PHY_IQCalibrateBcut
#endif


/*-----------------------------------------------------------------------------
 * Function:	PHY_RF6052SetOFDMTxPower
 *
 * Overview:	For legacy and HY OFDM, we must read EEPROM TX power index for
 *			different channel and read original value in TX power register area from
 *			0xe00. We increase offset and original value to be correct tx pwr.
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/05/2008 	MHC		Simulate 8192 series method.
 *
 *---------------------------------------------------------------------------*/

void PHY_RF6052SetOFDMTxPower(struct rtl8190_priv *priv, unsigned char powerlevel_1ss, unsigned char powerlevel_2ss)
{
//	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
#if !defined(RTL8192SU)
	unsigned long ioaddr = priv->pshare->ioaddr;
#endif	
	unsigned int writeVal, powerBase0, powerBase1;
	unsigned char index = 0;
	unsigned short RegOffset1[2] = {rTxAGC_Rate18_06, rTxAGC_Rate54_24};
	unsigned short RegOffset2[4] = {rTxAGC_Mcs03_Mcs00, rTxAGC_Mcs07_Mcs04, rTxAGC_Mcs11_Mcs08, rTxAGC_Mcs15_Mcs12};
	unsigned char  byte0, byte1, byte2, byte3;
	unsigned char RF6052_MAX_TX_PWR = 0x3F;


	if(priv->pshare->use_default_para){
		RTL_W32(rTxAGC_Rate18_06, 0x28282828);
		RTL_W32(rTxAGC_Rate54_24, 0x28282828);
		RTL_W16(rTxAGC_CCK_Mcs32,0x2424);
		RTL_W32(rTxAGC_Mcs03_Mcs00, 0x28282828);
		RTL_W32(rTxAGC_Mcs07_Mcs04, 0x28282828);
		RTL_W32(rTxAGC_Mcs11_Mcs08, 0x28282828);
		RTL_W32(rTxAGC_Mcs15_Mcs12, 0x28282828);

#ifdef ADD_TX_POWER_BY_CMD
		byte0 = byte1 = byte2 = byte3 = 0;
		ASSIGN_TX_POWER_OFFSET(byte0, priv->pshare->rf_ft_var.txPowerPlus_ofdm_18);
		ASSIGN_TX_POWER_OFFSET(byte1, priv->pshare->rf_ft_var.txPowerPlus_ofdm_12);
		ASSIGN_TX_POWER_OFFSET(byte2, priv->pshare->rf_ft_var.txPowerPlus_ofdm_9);
		ASSIGN_TX_POWER_OFFSET(byte3, priv->pshare->rf_ft_var.txPowerPlus_ofdm_6);
		writeVal = (byte0<<24) | (byte1<<16) |(byte2<<8) | byte3;
		if (writeVal)
			RTL_W32(0xe00, RTL_R32(0xe00)+writeVal);

		byte0 = byte1 = byte2 = byte3 = 0;
		ASSIGN_TX_POWER_OFFSET(byte0, priv->pshare->rf_ft_var.txPowerPlus_ofdm_54);
		ASSIGN_TX_POWER_OFFSET(byte1, priv->pshare->rf_ft_var.txPowerPlus_ofdm_48);
		ASSIGN_TX_POWER_OFFSET(byte2, priv->pshare->rf_ft_var.txPowerPlus_ofdm_36);
		ASSIGN_TX_POWER_OFFSET(byte3, priv->pshare->rf_ft_var.txPowerPlus_ofdm_24);
		writeVal = (byte0<<24) | (byte1<<16) |(byte2<<8) | byte3;
		if (writeVal)
			RTL_W32(0xe04, RTL_R32(0xe04)+writeVal);

		byte0 = 0;
		ASSIGN_TX_POWER_OFFSET(byte0, priv->pshare->rf_ft_var.txPowerPlus_cck);
		writeVal = (byte0<<8);
		if (writeVal)
			RTL_W16(0xe08, RTL_R16(0xe08)+(unsigned short)writeVal);

		byte0 = byte1 = byte2 = byte3 = 0;
		ASSIGN_TX_POWER_OFFSET(byte0, priv->pshare->rf_ft_var.txPowerPlus_mcs_3);
		ASSIGN_TX_POWER_OFFSET(byte1, priv->pshare->rf_ft_var.txPowerPlus_mcs_2);
		ASSIGN_TX_POWER_OFFSET(byte2, priv->pshare->rf_ft_var.txPowerPlus_mcs_1);
		ASSIGN_TX_POWER_OFFSET(byte3, priv->pshare->rf_ft_var.txPowerPlus_mcs_0);
		writeVal = (byte0<<24) | (byte1<<16) |(byte2<<8) | byte3;
		if (writeVal)
			RTL_W32(0xe10, RTL_R32(0xe10)+writeVal);

		byte0 = byte1 = byte2 = byte3 = 0;
		ASSIGN_TX_POWER_OFFSET(byte0, priv->pshare->rf_ft_var.txPowerPlus_mcs_7);
		ASSIGN_TX_POWER_OFFSET(byte1, priv->pshare->rf_ft_var.txPowerPlus_mcs_6);
		ASSIGN_TX_POWER_OFFSET(byte2, priv->pshare->rf_ft_var.txPowerPlus_mcs_5);
		ASSIGN_TX_POWER_OFFSET(byte3, priv->pshare->rf_ft_var.txPowerPlus_mcs_4);
		writeVal = (byte0<<24) | (byte1<<16) |(byte2<<8) | byte3;
		if (writeVal)
			RTL_W32(0xe14, RTL_R32(0xe14)+writeVal);

		byte0 = byte1 = byte2 = byte3 = 0;
		ASSIGN_TX_POWER_OFFSET(byte0, priv->pshare->rf_ft_var.txPowerPlus_mcs_11);
		ASSIGN_TX_POWER_OFFSET(byte1, priv->pshare->rf_ft_var.txPowerPlus_mcs_10);
		ASSIGN_TX_POWER_OFFSET(byte2, priv->pshare->rf_ft_var.txPowerPlus_mcs_9);
		ASSIGN_TX_POWER_OFFSET(byte3, priv->pshare->rf_ft_var.txPowerPlus_mcs_8);
		writeVal = (byte0<<24) | (byte1<<16) |(byte2<<8) | byte3;
		if (writeVal)
			RTL_W32(0xe18, RTL_R32(0xe18)+writeVal);

		byte0 = byte1 = byte2 = byte3 = 0;
		ASSIGN_TX_POWER_OFFSET(byte0, priv->pshare->rf_ft_var.txPowerPlus_mcs_15);
		ASSIGN_TX_POWER_OFFSET(byte1, priv->pshare->rf_ft_var.txPowerPlus_mcs_14);
		ASSIGN_TX_POWER_OFFSET(byte2, priv->pshare->rf_ft_var.txPowerPlus_mcs_13);
		ASSIGN_TX_POWER_OFFSET(byte3, priv->pshare->rf_ft_var.txPowerPlus_mcs_12);
		writeVal = (byte0<<24) | (byte1<<16) |(byte2<<8) | byte3;
		if (writeVal)
			RTL_W32(0xe1c, RTL_R32(0xe1c)+writeVal);
#endif // ADD_TX_POWER_BY_CMD
		return; // use default
	}

	if (priv->pshare->legacyOFDM_pwrdiff_A & 0x80)
		powerBase0 = powerlevel_1ss - (priv->pshare->legacyOFDM_pwrdiff_A & 0x0f);
	else
		powerBase0 = powerlevel_1ss + (priv->pshare->legacyOFDM_pwrdiff_A & 0x0f);

	for(index=0; index<2; index++)
	{
		byte0 = powerBase0 + priv->pshare->phw->OFDMTxAgcOffset[0+index*4];
		byte1 = powerBase0 + priv->pshare->phw->OFDMTxAgcOffset[1+index*4];
		byte2 = powerBase0 + priv->pshare->phw->OFDMTxAgcOffset[2+index*4];
		byte3 = powerBase0 + priv->pshare->phw->OFDMTxAgcOffset[3+index*4];

		// Max power index = 0x3F Range = 0-0x3F
		if (byte0 > RF6052_MAX_TX_PWR)
			byte0 = RF6052_MAX_TX_PWR;
		if (byte1 > RF6052_MAX_TX_PWR)
			byte1 = RF6052_MAX_TX_PWR;
		if (byte2 > RF6052_MAX_TX_PWR)
			byte2 = RF6052_MAX_TX_PWR;
		if (byte3 > RF6052_MAX_TX_PWR)
			byte3 = RF6052_MAX_TX_PWR;
		writeVal = (byte0<<24) | (byte1<<16) |(byte2<<8) | byte3;
		PHY_SetBBReg(priv, RegOffset1[index], 0x7f7f7f7f, writeVal);
	}

	for(index=0; index<4; index++)
	{
		if (index <= 1)
			powerBase1 = powerlevel_1ss;
		else
			powerBase1 = powerlevel_2ss;

		byte0 = powerBase1 + priv->pshare->phw->MCSTxAgcOffset[0+index*4];
		byte1 = powerBase1 + priv->pshare->phw->MCSTxAgcOffset[1+index*4];
		byte2 = powerBase1 + priv->pshare->phw->MCSTxAgcOffset[2+index*4];
		byte3 = powerBase1 + priv->pshare->phw->MCSTxAgcOffset[3+index*4];

		// Max power index = 0x3F Range = 0-0x3F
		if (byte0 > RF6052_MAX_TX_PWR)
			byte0 = RF6052_MAX_TX_PWR;
		if (byte1 > RF6052_MAX_TX_PWR)
			byte1 = RF6052_MAX_TX_PWR;
		if (byte2 > RF6052_MAX_TX_PWR)
			byte2 = RF6052_MAX_TX_PWR;
		if (byte3 > RF6052_MAX_TX_PWR)
			byte3 = RF6052_MAX_TX_PWR;
		writeVal = (byte0<<24) | (byte1<<16) |(byte2<<8) | byte3;
		PHY_SetBBReg(priv, RegOffset2[index], 0x7f7f7f7f, writeVal);
	}

	if (priv->pmib->dot11RFEntry.pwrlevelOFDM_2SS[priv->pshare->working_channel-1+14] != 0)
		priv->pshare->channelAB_pwrdiff =
		        (int)priv->pmib->dot11RFEntry.pwrlevelOFDM_2SS[priv->pshare->working_channel-1+14] -
		        (int)priv->pmib->dot11RFEntry.pwrlevelOFDM_2SS[priv->pshare->working_channel-1];
	else
		priv->pshare->channelAB_pwrdiff = 0;
	PHY_SetBBReg(priv, rFPGA0_TxGainStage, bXBTxAGC, (priv->pshare->channelAB_pwrdiff & 0x0f));

#if 0
	//Legacy OFDM rates
	powerBase0 = powerlevel + priv->pshare->legacyOFDM_pwrdiff;
	powerBase0 = (powerBase0<<24) | (powerBase0<<16) |(powerBase0<<8) |powerBase0;

	//MCS rates HT OFDM
	powerBase1 = powerlevel;
	powerBase1 = (powerBase1<<24) | (powerBase1<<16) |(powerBase1<<8) |powerBase1;

	for(index=0; index<6; index++)
	{
		//
		// Index 0 & 1= legacy OFDM, 2-5=HT_MCS rate
		//
		writeVal = 	RTL_R32(RegOffset[index])/*priv->MCSTxPowerLevelOriginalOffset[index]*/
					+ ((index<2)?powerBase0:powerBase1);
		byte0 = (writeVal & 0x7f);
		byte1 = ((writeVal & 0x7f00)>>8);
		byte2 = ((writeVal & 0x7f0000)>>16);
		byte3 = ((writeVal & 0x7f000000)>>24);

		// Max power index = 0x3F Range = 0-0x3F
		if(byte0 > RF6052_MAX_TX_PWR)
			byte0 = RF6052_MAX_TX_PWR;
		if(byte1 > RF6052_MAX_TX_PWR)
			byte1 = RF6052_MAX_TX_PWR;
		if(byte2 > RF6052_MAX_TX_PWR)
			byte2 = RF6052_MAX_TX_PWR;
		if(byte3 > RF6052_MAX_TX_PWR)
			byte3 = RF6052_MAX_TX_PWR;
		//
		// Add description: PWDB > threshold!!!High power issue!!
		// We must decrease tx power !! Why is the value ???
		//
/*		if(priv->bDynamicTxHighPower == TRUE)
		{
			// For MCS rate
			if(index > 1)
			{
				writeVal = 0x03030303;
			}
			// For Legacy rate
			else
			{
				writeVal = (byte3<<24) | (byte2<<16) |(byte1<<8) |byte0;
			}
		}
		else
		{
			writeVal = (byte3<<24) | (byte2<<16) |(byte1<<8) |byte0;
		}
*/
		//
		// Write different rate set tx power index.
		//
		writeVal = (byte3<<24) | (byte2<<16) |(byte1<<8) |byte0;
		PHY_SetBBReg(priv, RegOffset[index], 0x7f7f7f7f, writeVal);

	}
#endif
}	/* PHY_RF6052SetOFDMTxPower */


int phy_BB8192SE_Config_ParaFile(struct rtl8190_priv *priv)
{
//	unsigned long ioaddr = priv->pshare->ioaddr;
//	unsigned char u1RegValue;
//	unsigned int  u4RegValue;
	int rtStatus;

	phy_InitBBRFRegisterDefinition(priv);

#if 0	// For 92SE we do not check FPGA board now!!!!!????? Enable will reboot~~~ when write IDR0!!!
	/*----Set BB Global Reset----*/
	u1RegValue = RTL_R8(_BBGLBRESET_);
	RTL_W8(_BBGLBRESET_, (u1RegValue|BIT(0)));

	/*----Set BB reset Active----*/
	u4RegValue = RTL_R32(_CPURST_);
	RTL_W32(_CPURST_, (u4RegValue&(~CPURST_BBRst)));
#endif

	/*----Check MIMO TR hw setting----*/
	check_MIMO_TR_status(priv);

#if 0
	/*---- Set CCK and OFDM Block "OFF"----*/
	PHY_SetBBReg(priv, rFPGA0_RFMOD, bCCKEn|bOFDMEn, 0x0);
#endif

	/*----BB Register Initilazation----*/
#ifdef RTL8192SU
	rtStatus = 0;
	if (get_rf_mimo_mode(priv) == MIMO_1T2R)
		rtStatus = PHY_ConfigBBWithParaFile(priv, PHYREG_1T2R);
	else if (get_rf_mimo_mode(priv) == MIMO_2T2R)
#endif
	rtStatus = PHY_ConfigBBWithParaFile(priv, PHYREG_2T2R);
	if (rtStatus) {
		printk("phy_BB8192SE_Config_ParaFile(): Write BB Reg Fail!!\n");
		return rtStatus;
	}

	/*----If EEPROM or EFUSE autoload OK, We must config by PHY_REG_PG.txt----*/

	if (!priv->EE_AutoloadFail)
	{
//		printk("need PG setup\n");
		rtStatus = PHY_ConfigBBWithParaFile(priv, PHYREG_PG);
	}
	if(rtStatus){
		printk("phy_BB8192SE_Config_ParaFile():BB_PG Reg Fail!!\n");
		return rtStatus;
	}

	/*----BB AGC table Initialization----*/
	rtStatus = PHY_ConfigBBWithParaFile(priv, AGCTAB);
	if (rtStatus) {
		printk("phy_BB8192SE_Config_ParaFile(): Write BB AGC Table Fail!!\n");
		return rtStatus;
	}

#if !defined(RTL8192SU)
	/*----For 1T2R Config----*/
	if (get_rf_mimo_mode(priv) == MIMO_1T2R) {
		rtStatus = PHY_ConfigBBWithParaFile(priv, PHYREG_1T2R);
		if (rtStatus) {
			printk("phy_BB8192SE_Config_ParaFile(): Write BB Reg for 1T2R Fail!!\n");
			return rtStatus;
		}
	}else if (get_rf_mimo_mode(priv) == MIMO_1T1R){
		rtStatus = PHY_ConfigBBWithParaFile(priv, PHYREG_1T1R);
		if (rtStatus) {
			printk("phy_BB8192SE_Config_ParaFile(): Write BB Reg for 1T1R Fail!!\n");
			return rtStatus;
		}
	}
#endif
	DEBUG_INFO("PHY-BB Initialization Success\n");
	return 0;
}


// 8192SE use RF8225 as single chip phy
// So far, since only 8192SE use RF8225, we just write the funciton for 8192SE only
int phy_RF8225_Config_ParaFile(struct rtl8190_priv *priv)
{
	int rtStatus=0;
	RF90_RADIO_PATH_E eRFPath;
	BB_REGISTER_DEFINITION_T *pPhyReg;
//	unsigned int  RF0_Final_Value;
	unsigned int  u4RegValue = 0;
//	unsigned char RetryTimes;


	priv->pshare->phw->NumTotalRFPath = 2; // 8192SE only 1T2R or 2T2R, joshua

	for (eRFPath = RF90_PATH_A; eRFPath<priv->pshare->phw->NumTotalRFPath; eRFPath++)
	{
//		if (!PHYCheckIsLegalRfPath8190Pci(priv, eRFPath))
//			continue;
/*
  		if (get_rf_mimo_mode(priv) == MIMO_1T2R) {
			if ((eRFPath == RF90_PATH_A) || (eRFPath == RF90_PATH_B))
				continue;
  		}
		else if (get_rf_mimo_mode(priv) == MIMO_2T2R) {
			if ((eRFPath == RF90_PATH_B) || (eRFPath == RF90_PATH_D))
				continue;
		}
*/

		pPhyReg = &priv->pshare->phw->PHYRegDef[eRFPath];

		/*----Store original RFENV control type----*/
		switch(eRFPath)
		{
		case RF90_PATH_A:
		case RF90_PATH_C:
			u4RegValue = PHY_QueryBBReg(priv, pPhyReg->rfintfs, bRFSI_RFENV);
			break;
		case RF90_PATH_B :
		case RF90_PATH_D:
			u4RegValue = PHY_QueryBBReg(priv, pPhyReg->rfintfs, bRFSI_RFENV<<16);
			break;
		case RF90_PATH_MAX:
			break;
		}

		/*----Set RF_ENV enable----*/
		PHY_SetBBReg(priv, pPhyReg->rfintfe, bRFSI_RFENV<<16, 0x1);

		/*----Set RF_ENV output high----*/
		PHY_SetBBReg(priv, pPhyReg->rfintfo, bRFSI_RFENV, 0x1);

		/* Set bit number of Address and Data for RF register */
		PHY_SetBBReg(priv, pPhyReg->rfHSSIPara2, b3WireAddressLength, 0x0); 	// Set 0 to 4 bits for Z-serial and set 1 to 6 bits for 8258
		PHY_SetBBReg(priv, pPhyReg->rfHSSIPara2, b3WireDataLength, 0x0);	// Set 0 to 12 bits for Z-serial and 8258, and set 1 to 14 bits for ???

#if 0
		rtStatus  = PHY_CheckBBAndRFOK(priv, HW90_BLOCK_RF, eRFPath);
		if(rtStatus!= 0)
		{
			DEBUG_INFO("PHY_RF8256_Config():Check Radio[%d] Fail!!\n", eRFPath);
			goto phy_RF8256_Config_ParaFile_Fail;
		}
#endif
//		RF0_Final_Value = 0;
//		RetryTimes = 5;

		/*----Initialize RF fom connfiguration file----*/
		switch (eRFPath)
		{
#ifdef MERGE_FW
			case RF90_PATH_A:
				rtStatus = PHY_ConfigRFWithParaFile(priv, __radio_a_start,
												(int)(__radio_a_end -__radio_a_start),  eRFPath);
#ifdef HIGH_POWER_EXT_PA
				if(priv->pshare->rf_ft_var.use_ext_pa)
					PHY_ConfigRFWithParaFile(priv, __radio_a_hp_start,	(int)(__radio_a_hp_end -__radio_a_hp_start),  RF90_PATH_A);
#endif
				break;
			case RF90_PATH_B:
				rtStatus = PHY_ConfigRFWithParaFile(priv, __radio_b_start,
												(int)(__radio_b_end -__radio_b_start),  eRFPath);
				break;
#else
			case RF90_PATH_A:
#if !defined(RTL8192SU)
				rtStatus = PHY_ConfigRFWithParaFile(priv, "/usr/rtl8192Pci/radio_a.txt", eRFPath);
#else
				rtStatus = PHY_ConfigRFWithParaFile(priv, "/usr/rtl8192su/radio_a.txt", eRFPath);				
#endif				
				break;
			case RF90_PATH_B:
#if !defined(RTL8192SU)
				rtStatus = PHY_ConfigRFWithParaFile(priv, "/usr/rtl8192Pci/radio_b.txt", eRFPath);
#else				
				rtStatus = PHY_ConfigRFWithParaFile(priv, "/usr/rtl8192su/radio_b.txt", eRFPath);
#endif				
				break;
#endif
			case RF90_PATH_C:
			case RF90_PATH_D:
			default:
				break;
		}

		/*----Restore RFENV control type----*/;
		switch(eRFPath)
		{
		case RF90_PATH_A:
		case RF90_PATH_C:
			PHY_SetBBReg(priv, pPhyReg->rfintfs, bRFSI_RFENV, u4RegValue);
				break;
		case RF90_PATH_B :
		case RF90_PATH_D:
			PHY_SetBBReg(priv, pPhyReg->rfintfs, bRFSI_RFENV<<16, u4RegValue);
			break;
		case RF90_PATH_MAX:
			break;
		}
	}

	DEBUG_INFO("PHY-RF Initialization Success\n");

//phy_RF8256_Config_ParaFile_Fail:

	return rtStatus;
}


void check_and_set_ampdu_spacing(struct rtl8190_priv *priv, struct stat_info *pstat)
{
#if !defined(RTL8192SU)
	unsigned long ioaddr = priv->pshare->ioaddr;
#endif	
	unsigned char ampdu_des;

	// set MCS15-Short GI spacing (8192SE hardware restriction),
	// which is used while spacing is 16us and rate is 300M

	if (priv->pshare->min_ampdu_spacing < _HTCAP_AMPDU_SPC_16_US_) {
		if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != 0) {
			priv->pshare->min_ampdu_spacing = _HTCAP_AMPDU_SPC_16_US_;
			RTL_W8(AMPDU_MIN_SPACE, _HTCAP_AMPDU_SPC_16_US_ | (0x13<<3));
		}
		else if (priv->pmib->dot11WdsInfo.wdsEnabled && (priv->pmib->dot11WdsInfo.wdsNum > 0)) {
			if (priv->pmib->dot11WdsInfo.wdsPrivacy != 0) {
				priv->pshare->min_ampdu_spacing = _HTCAP_AMPDU_SPC_16_US_;
				RTL_W8(AMPDU_MIN_SPACE, _HTCAP_AMPDU_SPC_16_US_ | (0x13<<3));
			}
		}
		else if (pstat) {
			ampdu_des = (pstat->ht_cap_buf.ampdu_para & _HTCAP_AMPDU_SPC_MASK_) >> _HTCAP_AMPDU_SPC_SHIFT_;
			if (ampdu_des > priv->pshare->min_ampdu_spacing) {
				priv->pshare->min_ampdu_spacing = ampdu_des;
				RTL_W8(AMPDU_MIN_SPACE, ampdu_des | (0x13<<3));
			}
		}
	}
}


static void rtl8192SE_FW_hdr_swap(void *data)
{
#ifdef _BIG_ENDIAN_
	struct _RT_8192S_FIRMWARE_HDR *pFwHdr = (struct _RT_8192S_FIRMWARE_HDR *)data;

	pFwHdr->Signature        = le16_to_cpu(pFwHdr->Signature);
	pFwHdr->Version          = le16_to_cpu(pFwHdr->Version);
	pFwHdr->DMEMSize         = le32_to_cpu(pFwHdr->DMEMSize);
	pFwHdr->IMG_IMEM_SIZE    = le32_to_cpu(pFwHdr->IMG_IMEM_SIZE);
	pFwHdr->IMG_SRAM_SIZE    = le32_to_cpu(pFwHdr->IMG_SRAM_SIZE);
	pFwHdr->FW_PRIV_SIZE     = le32_to_cpu(pFwHdr->FW_PRIV_SIZE);
	pFwHdr->efuse_addr       = le16_to_cpu(pFwHdr->efuse_addr);
	pFwHdr->h2ccmd_resp_addr = le16_to_cpu(pFwHdr->h2ccmd_resp_addr);
	pFwHdr->source_version   = le16_to_cpu(pFwHdr->source_version);
	pFwHdr->release_time     = le32_to_cpu(pFwHdr->release_time);
#endif
}


static void rtl8192SE_ReadIMG(struct rtl8190_priv *priv)
{
	unsigned char *swap_arr;
	struct _RT_8192S_FIRMWARE_HDR *pFwHdr = NULL;
	struct _RT_8192S_FIRMWARE_PRIV *pFwPriv = NULL;
#ifdef MERGE_FW
	unsigned long fw_ptr = (unsigned long)__fw_start;
#else
	mm_segment_t old_fs;
//	unsigned int buf_len=0, read_bytes;
	int fd=0;
#ifndef	CONFIG_X86
	extern ssize_t sys_read(unsigned int fd, char * buf, size_t count);
#endif
#endif

	swap_arr = kmalloc(RT_8192S_FIRMWARE_HDR_SIZE, GFP_ATOMIC);
	if (swap_arr == NULL)
		return;

#ifdef RTL8192SU
	unsigned char *pFileName=NULL;
#endif	

#ifdef MERGE_FW
	memcpy(swap_arr, (unsigned char *)fw_ptr, RT_8192S_FIRMWARE_HDR_SIZE);
	fw_ptr += RT_8192S_FIRMWARE_HDR_SIZE;
	priv->pshare->fw_DMEM_buf = (unsigned char *)fw_ptr;
	fw_ptr += FW_DMEM_SIZE;
#else

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	//read from fw img and setup parameters

#ifdef CONFIG_X86
	if ((fd = rtl8190n_fileopen("/usr/rtl8192Pci/rtl8192sfw.bin", O_RDONLY, 0)) < 0)
#else
#if !defined(RTL8192SU)
	if ((fd = sys_open("/usr/rtl8192Pci/rtl8192sfw.bin", O_RDONLY, 0)) < 0)
#else
	pFileName = "/usr/rtl8192su/rtl8192sfw.bin";
	if ((fd = sys_open(pFileName, O_RDONLY, 0)) < 0)
#endif	
#endif
	{
#if !defined(RTL8192SU)
		printk("/usr/rtl8192Pci/rtl8192sfw.bin cannot be opened\n");
#else
		printk("%s cannot be opened\n", pFileName);
#endif			
		BUG();
	}

	// read from bin file
	sys_read(fd, swap_arr, RT_8192S_FIRMWARE_HDR_SIZE);
	sys_read(fd, priv->pshare->fw_DMEM_buf, FW_DMEM_SIZE);
	rtl_cache_sync_wback(priv, (unsigned int)priv->pshare->fw_DMEM_buf, RT_8192S_FIRMWARE_HDR_SIZE, PCI_DMA_TODEVICE);
#endif // MERGE_FW

	rtl8192SE_FW_hdr_swap(swap_arr);
	pFwHdr = (struct _RT_8192S_FIRMWARE_HDR *)swap_arr;

	//set parameters
	priv->pshare->fw_EMEM_len = pFwHdr->IMG_SRAM_SIZE;
	priv->pshare->fw_IMEM_len = pFwHdr->IMG_IMEM_SIZE;
	priv->pshare->fw_DMEM_len = pFwHdr->DMEMSize;
	priv->pshare->fw_version     = pFwHdr->Version;
	priv->pshare->fw_src_version = pFwHdr->source_version;
	priv->pshare->fw_sub_version = pFwHdr->subversion;

	printk("fw_version: %04x(%d.%d), fw_EMEM_len: %d, fw_IMEM_len: %d, fw_DMEM_len: %d\n",
		priv->pshare->fw_version, priv->pshare->fw_src_version, priv->pshare->fw_sub_version,
		priv->pshare->fw_EMEM_len, priv->pshare->fw_IMEM_len, priv->pshare->fw_DMEM_len);

	pFwPriv = (struct _RT_8192S_FIRMWARE_PRIV *)priv->pshare->fw_DMEM_buf;
	// for rf_config filed
	if (get_rf_mimo_mode(priv) == MIMO_1T2R ||get_rf_mimo_mode(priv) == MIMO_1T1R )
		pFwPriv->rf_config = 0x12;
	else // 2T2R
		pFwPriv->rf_config = 0x22;
#ifdef RTL8192SU
	pFwPriv->hci_sel=HCISEL_USBAP;
#if 0	
	if (get_rf_mimo_mode(priv) == MIMO_1T2R ||get_rf_mimo_mode(priv) == MIMO_1T1R )
		pFwPriv->rf_config=RFCONFIG_1T2R;
	else
		pFwPriv->rf_config=RFCONFIG_2T2R;
#endif		
	if (priv->pmib->efuseEntry.usb_epnum==USBEP_SIX)
		pFwPriv->usb_ep_num=USBEP_SIX;
	else
		pFwPriv->usb_ep_num=USBEP_FOUR;
#ifdef RTL8192SU_FWBCN
	pFwPriv->beacon_offload=BCNOFFLOAD_FW;
#endif
#else

	// root Fw beacon
	if (priv->pshare->rf_ft_var.rootFwBeacon)
		pFwPriv->beacon_offload = 3;
	else
		pFwPriv->beacon_offload = 0;
#endif //RTL8192SU

	//read IMEM
#ifdef MERGE_FW
	priv->pshare->fw_IMEM_buf = (unsigned char *)fw_ptr;
	fw_ptr += priv->pshare->fw_IMEM_len;
#else
	sys_read(fd, priv->pshare->fw_IMEM_buf, priv->pshare->fw_IMEM_len);
	rtl_cache_sync_wback(priv, (unsigned int)priv->pshare->fw_IMEM_buf,  priv->pshare->fw_IMEM_len, PCI_DMA_TODEVICE);
#endif

	//read EMEM
#ifdef MERGE_FW
	priv->pshare->fw_EMEM_buf = (unsigned char *)fw_ptr;
	fw_ptr += priv->pshare->fw_EMEM_len;
#else
	sys_read(fd, priv->pshare->fw_EMEM_buf, priv->pshare->fw_EMEM_len);
	rtl_cache_sync_wback(priv, (unsigned int)priv->pshare->fw_EMEM_buf,  priv->pshare->fw_EMEM_len, PCI_DMA_TODEVICE);
#endif

#ifndef MERGE_FW
	sys_close(fd);
	set_fs(old_fs);
#endif
	kfree(swap_arr);
}


#if 0
/*-----------------------------------------------------------------------------
 * Function:	fw_SetRQPN()
 *
 * Overview:
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	04/30/2008	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
static	void	fw_SetRQPN(struct rtl8190_priv *priv)
{
	// Only for 92SE HW bug, we have to set RAPN before every FW download
	// We can remove the code later.
	unsigned long ioaddr = priv->pshare->ioaddr;

	RTL_W32(RQPN, 0xffffffff);
	RTL_W32(RQPN+4, 0xffffffff);
	RTL_W8(RQPN+8, 0xff);
	RTL_W8(RQPN+0xB, 0x80);

}	/* fw_SetRQPN */
#endif


static void LoadIMG(struct rtl8190_priv *priv, int fw_file)
{
#if !defined(RTL8192SU)
	unsigned long ioaddr=priv->pshare->ioaddr;
#endif	
	int bResult=0;
	unsigned char *mem_ptr=NULL;
	unsigned int buf_len=0;
	int i=0;

//	fw_SetRQPN(priv);	// For 92SE only

	switch(fw_file) {
	case LOAD_IMEM:
		mem_ptr = priv->pshare->fw_IMEM_buf;
		buf_len = priv->pshare->fw_IMEM_len;
		break;
	case LOAD_EMEM:
		mem_ptr = priv->pshare->fw_EMEM_buf;
		buf_len = priv->pshare->fw_EMEM_len;
		break;
	case LOAD_DMEM:
		mem_ptr = priv->pshare->fw_DMEM_buf;
		buf_len = priv->pshare->fw_DMEM_len;
		break;
	default:
		printk("ERROR!, not such loading option\n");
		break;
	}
	while(1)
	{
		if ((i+LoadPktSize) < buf_len) {
			//for test
			bResult = rtl8192SE_SetupOneCmdPacket(priv, mem_ptr, LoadPktSize, 0);
#if !defined(RTL8192SU)
			RTL_W16(TPPoll, TPPoll_CQ);
#endif			

			if(bResult)
				delay_ms(1);

			mem_ptr += LoadPktSize;
			i += LoadPktSize;
		}
		else {
			bResult = rtl8192SE_SetupOneCmdPacket(priv, mem_ptr, (buf_len-i), 1);
#if !defined(RTL8192SU)			
			if(bResult)
				RTL_W8(TPPoll, POLL_CMD);
#endif				
			break;
		}
	}

	if (bResult == 0)
		printk("desc not available, firmware cannot be loaded \n");
}


//
// Description:   CPU register locates in different page against general register.
//			    Switch to CPU register in the begin and switch back before return
//
// Arguments:   The pointer of the adapter
// Created by Roger, 2008.04.10.
//
unsigned int FirmwareEnableCPU(struct rtl8190_priv *priv)
{

	unsigned int	rtStatus = SUCCESS;
	unsigned char	tmpU1b, CPUStatus = 0;
	unsigned short	tmpU2b;
	unsigned int iCheckTime = 200;
#if !defined(RTL8192SU)	
	unsigned long ioaddr = priv->pshare->ioaddr;
#endif	

//	RT_TRACE(COMP_INIT, DBG_LOUD, ("-->FirmwareEnableCPU()\n") );

//	fw_SetRQPN(priv);	// For 92SE only

	// Enable CPU.
	tmpU1b = RTL_R8(SYS_CLKR);
	delay_ms(2);
	RTL_W8(SYS_CLKR, (tmpU1b|SYS_CPU_CLKSEL)); //AFE source
	delay_ms(2);

	tmpU2b = RTL_R16(SYS_FUNC_EN);
	delay_ms(2);

	RTL_W16(SYS_FUNC_EN, (tmpU2b|FEN_CPUEN));
	delay_ms(2);


	//Polling IMEM Ready after CPU has refilled.
	do
	{
		CPUStatus = RTL_R8(TCR);
		if(CPUStatus & IMEM_RDY)
		{
//			RT_TRACE(COMP_INIT, DBG_LOUD, ("IMEM Ready after CPU has refilled.\n"));
			break;
		}

		delay_ms(10);
	}while(iCheckTime--);

	if(!(CPUStatus & IMEM_RDY))
		return FAIL;

//	RT_TRACE(COMP_INIT, DBG_LOUD, ("<--FirmwareEnableCPU(): rtStatus(%#x)\n", rtStatus));
	return rtStatus;
}


static int FirmwareCheckReady(struct rtl8190_priv *priv,int fw_file)
{
	unsigned int rtStatus = SUCCESS;
	int			PollingCnt = 100;
	unsigned char	 	/*tmpU1b, */ CPUStatus = 0;
#ifdef RTL8192SU
	volatile
#endif
	unsigned int		tmpU4b;
//	unsigned int		bOrgIMREnable;
#if !defined(RTL8192SU)
	unsigned long ioaddr = priv->pshare->ioaddr;
#endif	

//	DEBUG_INFO(("--->FirmwareCheckReady(): LoadStaus(%d)\n", LoadFWStatus));

	if( fw_file == LOAD_IMEM)
	{
		do
		{//Polling IMEM code done.
			CPUStatus = RTL_R8(TCR);
			if(CPUStatus& IMEM_CODE_DONE)
				break;

			delay_ms(10);
		}while(PollingCnt--);
		if(!(CPUStatus & IMEM_CHK_RPT) || PollingCnt <= 0)
		{
//			RT_TRACE(COMP_INIT, DBG_LOUD, ("FW_STATUS_LOAD_IMEM FAIL CPUStatus=%x\r\n", CPUStatus));
			printk("Check IMEM fail!, PollingCnt: %x, CPU stats: %x\n",PollingCnt,CPUStatus);
			return FAIL;
		}
		DEBUG_INFO("Load IMEM ok\n");
	}
	else if( fw_file == LOAD_EMEM)
	{//Check Put Code OK and Turn On CPU

		do
		{//Polling EMEM code done.
			CPUStatus = RTL_R8(TCR);
			if(CPUStatus & EMEM_CODE_DONE)
				break;

			delay_ms(10);
		}while(PollingCnt--);
		if(!(CPUStatus & EMEM_CHK_RPT))
		{
//			RT_TRACE(COMP_INIT, DBG_LOUD, ("FW_STATUS_LOAD_EMEM FAIL CPUStatus=%x\r\n", CPUStatus));
			printk("Check EMEM fail!, CPU Stats = %x, PollingCnt = %x\n", CPUStatus, PollingCnt);
			return FAIL;
		}

		// Turn On CPU
		rtStatus = FirmwareEnableCPU(priv);
		if(rtStatus != SUCCESS)
		{
//			RT_TRACE(COMP_INIT, DBG_LOUD, ("Enable CPU fail ! \n") );
			printk("CPU Enable Fail\n");
			return FAIL;
		}
		DEBUG_INFO("Load EMEM ok, CPU Enabled\n");
	}
	else if( fw_file == LOAD_DMEM)
	{
		do
		{//Polling DMEM code done
			CPUStatus = RTL_R8(TCR);
			if(CPUStatus& DMEM_CODE_DONE)
				break;

			delay_ms(10);
		}while(PollingCnt--);
		if(!(CPUStatus & DMEM_CODE_DONE)){
			printk("Check DMEM fail!\n");
			return FAIL;
		}

		do
		{//Polling Load Firmware ready
			CPUStatus = RTL_R8(TCR);
			if(CPUStatus & LOAD_FW_READY)
				break;

			delay_ms(10);
		}while(PollingCnt--);

//		RTL_W32(RQPN, 0x10101010);
//		RTL_W8(LD_RQPN, 0x80);

		tmpU4b = RTL_R32(TCR);
		RTL_W32(TCR, (tmpU4b&(~TCR_ICV)));

		tmpU4b = RTL_R32(RCR);
		RTL_W32(RCR,
			(tmpU4b|RCR_APPFCS|RCR_APP_ICV|RCR_APP_MIC));

		// Set to normal mode
		// 2008/05/26 MH For firmware operation, we must wait BIT 7 check is OK
		// We can set Loopback  mode.

		PollingCnt = 0;
		tmpU4b = RTL_R32(TCR);
		while (!(tmpU4b & FWRDY) && PollingCnt++ < 10)
		{
			tmpU4b = RTL_R32(TCR);
			delay_ms(10);
		}
		if (PollingCnt > 10)
			return FAIL;

		// for test
		RTL_W8(LBKMD_SEL, LBK_NORMAL);
		DEBUG_INFO("Load DMEM ok. FW is ready and RCR, TCR initialized\n");
	}

//	RT_TRACE(COMP_INIT, DBG_LOUD, ("<---FirmwareCheckReady(): rtStatus(%#x)\n", rtStatus));
	return rtStatus;
}


int LoadIMEMIMG(struct rtl8190_priv *priv)
{
	LoadIMG(priv, LOAD_IMEM);
	return FirmwareCheckReady(priv, LOAD_IMEM);
}


int LoadDMEMIMG(struct rtl8190_priv *priv)
{
	LoadIMG(priv, LOAD_DMEM);
	return FirmwareCheckReady(priv, LOAD_DMEM);
}


int LoadEMEMIMG(struct rtl8190_priv *priv)
{
	LoadIMG(priv, LOAD_EMEM);
	return FirmwareCheckReady(priv, LOAD_EMEM);
}


#if 0 // FPGA Version
/*-----------------------------------------------------------------------------
 * Function:	MacConfigBeforeFwDownload()
 *
 * Overview:	Copy from WMAc for init setting. MAC config before FW download
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	04/29/2008	MHC		The same as 92SE MAC initialization.
 *	07/11/2008	MHC		MAC config before FW download
 *
 *---------------------------------------------------------------------------*/
static void  MacConfigBeforeFwDownload(struct rtl8190_priv *priv)
{
//	HAL_DATA_TYPE		*pHalData	= GET_HAL_DATA(Adapter);
	unsigned char		i;
	unsigned char		tmpU1b;
	unsigned short		tmpU2b;
	unsigned int		addr;
	unsigned long	ioaddr = priv->pshare->ioaddr;
	struct rtl8192SE_hw *phw = GET_HW(priv);

//	RT_TRACE(COMP_INIT, DBG_LOUD, ("--->MacConfig8192SE()\n"));

	// 2008/05/14 MH For 92SE rest before init MAC data sheet not defined now.
	// HW provide the method to prevent press reset bbutton every time.
//	RT_TRACE(COMP_INIT, DBG_LOUD, ("Set some register before enable NIC\r\n"));

	tmpU1b = RTL_R8(0x562);
	tmpU1b |= 0x08;
	RTL_W8(0x562, tmpU1b);
	tmpU1b &= ~(BIT(3));
	RTL_W8(0x562, tmpU1b);

	tmpU1b = RTL_R8(SYS_FUNC_EN+1);
	tmpU1b &= 0x73;
	RTL_W8(SYS_FUNC_EN+1, tmpU1b);

	tmpU1b = RTL_R8(SYS_CLKR);
	tmpU1b &= 0xfa;
	RTL_W8(SYS_CLKR, tmpU1b);

//	RT_TRACE(COMP_INIT, DBG_LOUD,
//	("Delay 1000ms before reset NIC. I dont know how long should we delay!!!!!\r\n"));
	delay_ms(1000);

	// Switch to 80M clock
	RTL_W8(SYS_CLKR, SYS_CLKSEL_80M);

	 // Enable VSPS12 LDO Macro block
	tmpU1b = RTL_R8(SPS1_CTRL);
	RTL_W8(SPS1_CTRL, (tmpU1b|SPS1_LDEN));

	//Enable AFE Macro Block's Bandgap
	tmpU1b = RTL_R8(AFE_MISC);
	RTL_W8(AFE_MISC, (tmpU1b|AFE_BGEN));

	//Enable LDOA15 block
	tmpU1b = RTL_R8(LDOA15_CTRL);
	RTL_W8(LDOA15_CTRL, (tmpU1b|LDA15_EN));

	 //Enable VSPS12_SW Macro Block
	tmpU1b = RTL_R8(SPS1_CTRL);
	RTL_W8(SPS1_CTRL, (tmpU1b|SPS1_SWEN));

	//Enable AFE Macro Block's Mbias
	tmpU1b = RTL_R8(AFE_MISC);
	RTL_W8(AFE_MISC, (tmpU1b|AFE_MBEN));

	// Set Digital Vdd to Retention isolation Path.
	tmpU2b = RTL_R16(SYS_ISO_CTRL);
	RTL_W16(SYS_ISO_CTRL, (tmpU2b|ISO_PWC_DV2RP));

	// Attatch AFE PLL to MACTOP/BB/PCIe Digital
	tmpU2b = RTL_R16(SYS_ISO_CTRL);
	RTL_W16(SYS_ISO_CTRL, (tmpU2b &(~ISO_PWC_RV2RP)));

	//Enable AFE clock
	tmpU2b = RTL_R16(AFE_XTAL_CTRL);
	RTL_W16(AFE_XTAL_CTRL, (tmpU2b &(~XTAL_GATE_AFE)));

	//Enable AFE PLL Macro Block
	tmpU1b = RTL_R8(AFE_PLL_CTRL);
	RTL_W8(AFE_PLL_CTRL, (tmpU1b|APLL_EN));

	// Release isolation AFE PLL & MD
	RTL_W8(SYS_ISO_CTRL, 0xEE);

	//Enable MAC clock
	tmpU2b = RTL_R16(SYS_CLKR);
	RTL_W16(SYS_CLKR, (tmpU2b|SYS_MAC_CLK_EN));

	//Enable Core digital and enable IOREG R/W
	tmpU2b = RTL_R16(SYS_FUNC_EN);
	RTL_W16(SYS_FUNC_EN, (tmpU2b|FEN_DCORE|FEN_MREGEN));

	 //Switch the control path.
	tmpU2b = RTL_R16(SYS_CLKR);
	RTL_W16(SYS_CLKR, ((tmpU2b|SYS_FWHW_SEL)&(~SYS_SWHW_SEL)));


	RTL_W16(CMDR, 0x37FC);

#if 1
	// Load MAC register from WMAc temporarily We simulate macreg.txt HW will provide
	// MAC txt later
	RTL_W8(0x6, 0x30);
	RTL_W8(0x48, 0x2f);
	RTL_W8(0x49, 0xf0);

	RTL_W8(0x4b, 0x81);

	RTL_W8(0xb5, 0x21);

	RTL_W8(0xdc, 0xff);
	RTL_W8(0xdd, 0xff);
	RTL_W8(0xde, 0xff);
	RTL_W8(0xdf, 0xff);

	RTL_W8(0x11a, 0x00);
	RTL_W8(0x11b, 0x00);

	for (i = 0; i < 32; i++)
		RTL_W8(INIMCS_SEL+i, 0x1b);	 // MCS 15

	RTL_W8(0x236, 0xff);

	RTL_W8(0x503, 0x22);
	RTL_W8(0x560, 0x09);

	RTL_W8(DBG_PORT, 0x91);
#endif
	//in this time, RQPN will be set before fw download procedure
/*
	if (pmib->dot11OperationEntry.wifi_specific) {
		RTL_W32(RQPN1, (NUM_OF_PAGE_IN_FW_QUEUE_BK+(NUM_OF_PAGE_IN_FW_QUEUE_PUB/4)) << RSVD_FW_QUEUE_PAGE_BK_SHIFT | \
						 (NUM_OF_PAGE_IN_FW_QUEUE_BE+(NUM_OF_PAGE_IN_FW_QUEUE_PUB/4)) << RSVD_FW_QUEUE_PAGE_BE_SHIFT | \
						 (NUM_OF_PAGE_IN_FW_QUEUE_VI+(NUM_OF_PAGE_IN_FW_QUEUE_PUB/4)) << RSVD_FW_QUEUE_PAGE_VI_SHIFT | \
						 (NUM_OF_PAGE_IN_FW_QUEUE_VO+(NUM_OF_PAGE_IN_FW_QUEUE_PUB/4)) << RSVD_FW_QUEUE_PAGE_VO_SHIFT);
	}
	else {
		RTL_W32(RQPN1, 0 << RSVD_FW_QUEUE_PAGE_BK_SHIFT | \
						 (NUM_OF_PAGE_IN_FW_QUEUE_BE +NUM_OF_PAGE_IN_FW_QUEUE_PUB) << RSVD_FW_QUEUE_PAGE_BE_SHIFT | \
						 (NUM_OF_PAGE_IN_FW_QUEUE_VI + NUM_OF_PAGE_IN_FW_QUEUE_VO + NUM_OF_PAGE_IN_FW_QUEUE_BK)<< RSVD_FW_QUEUE_PAGE_VI_SHIFT | \
						 0 << RSVD_FW_QUEUE_PAGE_VO_SHIFT);
	}

	RTL_W32(RQPN2, NUM_OF_PAGE_IN_FW_QUEUE_MGNT << RSVD_FW_QUEUE_PAGE_MGNT_SHIFT);
	RTL_W32(RQPN3, APPLIED_RESERVED_QUEUE_IN_FW | \
					 NUM_OF_PAGE_IN_FW_QUEUE_BCN << RSVD_FW_QUEUE_PAGE_BCN_SHIFT|\
					 0 << RSVD_FW_QUEUE_PAGE_PUB_SHIFT);
*/
	RTL_W32(RDSA, phw->ring_dma_addr);
	RTL_W32(TMDA, phw->tx_ring0_addr);
	RTL_W32(TBKDA, phw->tx_ring1_addr);
	RTL_W32(TBEDA, phw->tx_ring2_addr);
	RTL_W32(TVIDA, phw->tx_ring3_addr);
	RTL_W32(TVODA, phw->tx_ring4_addr);
	RTL_W32(THPDA, phw->tx_ring5_addr);
	RTL_W32(TBDA, phw->tx_ringB_addr);
	RTL_W32(RCDA, phw->rxcmd_ring_addr);
	RTL_W32(TCDA, phw->txcmd_ring_addr);
//	RTL_W32(TCDA, phw->tx_ring5_addr);

/*
	//Set RX Desc Address
	PlatformEFIOWrite4Byte(Adapter, RDSA,
	pHalData->RxDescMemory[RX_MPDU_QUEUE].PhysicalAddressLow);
	PlatformEFIOWrite4Byte(Adapter, RCDA,
	pHalData->RxDescMemory[RX_CMD_QUEUE].PhysicalAddressLow);

	//Set TX Desc Address
	PlatformEFIOWrite4Byte(Adapter, TCDA,
	pHalData->TxDescMemory[TXCMD_QUEUE].PhysicalAddressLow);
	PlatformEFIOWrite4Byte(Adapter, TMDA,
	pHalData->TxDescMemory[MGNT_QUEUE].PhysicalAddressLow);
	PlatformEFIOWrite4Byte(Adapter, TVODA,
	pHalData->TxDescMemory[VO_QUEUE].PhysicalAddressLow);
	PlatformEFIOWrite4Byte(Adapter, TVIDA,
	pHalData->TxDescMemory[VI_QUEUE].PhysicalAddressLow);
	PlatformEFIOWrite4Byte(Adapter, TBEDA,
	pHalData->TxDescMemory[BE_QUEUE].PhysicalAddressLow);
	PlatformEFIOWrite4Byte(Adapter,
	TBKDA, pHalData->TxDescMemory[BK_QUEUE].PhysicalAddressLow);
	PlatformEFIOWrite4Byte(Adapter, TBDA,
	pHalData->TxDescMemory[BEACON_QUEUE].PhysicalAddressLow);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("<---MacConfig8192SE()\n"));
*/
}	/* MacConfigBeforeFwDownload */


/*-----------------------------------------------------------------------------
 * Function:	MacConfigAfterFwDownload()
 *
 * Overview:	After download Firmware, we must set some MAC register for some dedicaed
 *			purpose.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	07/02/2008	MHC		Create.
 *
 *---------------------------------------------------------------------------*/
static void MacConfigAfterFwDownload(struct rtl8190_priv *priv)
{
	unsigned char	i;
	unsigned char	tmpU1b;
	unsigned short	tmpU2b;
	unsigned long	ioaddr = priv->pshare->ioaddr;
	//
	// 1. System Configure Register (Offset: 0x0000 - 0x003F)
	//

	//
	// 2. Command Control Register (Offset: 0x0040 - 0x004F)
	//
	// Turn on 0x40 Command register


	// for sending beacon in current stage, disable RX
	RTL_W16(CMDR, BBRSTn|BB_GLB_RSTn|SCHEDULE_EN|MACRXEN|MACTXEN|DDMA_EN|FW2HW_EN|	RXDMA_EN|TXDMA_EN|HCI_RXDMA_EN|HCI_TXDMA_EN);

	// Set TCR TX DMA pre 2 FULL enable bit
	RTL_W32(TCR, RTL_R32(TCR)|TXDMAPRE2FULL);
	// Set RCR ... disable RX now, will enable later

	RTL_W32(RCR, RCR_APPFCS |RCR_APP_PHYST_STAFF | RCR_APWRMGT | RCR_ADD3 |	RCR_AMF	| RCR_ADF | RCR_APP_MIC | RCR_APP_ICV |       RCR_AICV	/*| RCR_ACRC32*/	|				// Accept ICV error, CRC32 Error
	RCR_AB 		| RCR_AM		|				// Accept Broadcast, Multicast
    	RCR_APM 	|  								// Accept Physical match
     	RCR_AAP		|     						// Accept Destination Address packets
	(7/*pHalData->EarlyRxThreshold*/<<RCR_FIFO_OFFSET));

//	RTL_W32(RCR, 0); //just dont receive anything

	//
	// 3. MACID Setting Register (Offset: 0x0050 - 0x007F)
	//

	//
	// 4. Timing Control Register  (Offset: 0x0080 - 0x009F)
	//
	// Set CCK/OFDM SIFS
	RTL_W16(SIFS_CCK, 0x0e0e);//0x1010
	RTL_W16(SIFS_OFDM, 0x0e0e);// 0x1010
	//3
	//Set AckTimeout
	RTL_W8(ACK_TIMEOUT, 0x40);

	//Beacon related
	RTL_W16(BCN_INTERVAL, 100);
	RTL_W16(ATIMWND, 2);
	//PlatformEFIOWrite2Byte(Adapter, BCN_DRV_EARLY_INT, 0x0022);
	//PlatformEFIOWrite2Byte(Adapter, BCN_DMATIME, 0xC8);
	//PlatformEFIOWrite2Byte(Adapter, BCN_ERR_THRESH, 0xFF);
	//PlatformEFIOWrite1Byte(Adapter, MLT, 8);

	//
	// 5. FIFO Control Register (Offset: 0x00A0 - 0x015F)
	//
	// Initialize Number of Reserved Pages in Firmware Queue
#if 0
	PlatformEFIOWrite4Byte(Adapter, RQPN1,
	NUM_OF_PAGE_IN_FW_QUEUE_BK<<0 | NUM_OF_PAGE_IN_FW_QUEUE_BE<<8 |\
	NUM_OF_PAGE_IN_FW_QUEUE_VI<<16 | NUM_OF_PAGE_IN_FW_QUEUE_VO<<24);
	PlatformEFIOWrite4Byte(Adapter, RQPN2,
	NUM_OF_PAGE_IN_FW_QUEUE_HCCA << 0 | NUM_OF_PAGE_IN_FW_QUEUE_CMD << 8|\
	NUM_OF_PAGE_IN_FW_QUEUE_MGNT << 16 |NUM_OF_PAGE_IN_FW_QUEUE_HIGH << 24);
	PlatformEFIOWrite4Byte(Adapter, RQPN3,
	NUM_OF_PAGE_IN_FW_QUEUE_BCN<<0 | NUM_OF_PAGE_IN_FW_QUEUE_PUB<<8);
	// Active FW/MAC to load the new RQPN setting
	PlatformEFIOWrite1Byte(Adapter, LD_RQPN, BIT7);
#endif
	// Setting TX/RX page size 0/1/2/3/4=64/128/256/512/1024
	//PlatformEFIOWrite1Byte(Adapter, PBP, 0x22);


	// Set RXDMA arbitration to control RXDMA/MAC/FW R/W for RXFIFO
	RTL_W8( RXDMA_, RTL_R8(RXDMA_)|BIT(6));

	//
	// 6. Adaptive Control Register  (Offset: 0x0160 - 0x01CF)
	//
	// Set RRSR to all legacy rate and HT rate
	// CCK rate is supported by default.
	// CCK rate will be filtered out only when associated AP does not support it.
	RTL_W8(RRSR, 0xff);
	RTL_W8(RRSR+1, 0x01);
//	RTL_W8(RRSR+1, 0xff);
	RTL_W8(RRSR+2, 0x0);
#if 0
	//for (i = 0; i < 8; i++)
		//PlatformEFIOWrite4Byte(Adapter, ARFR0+i*4, 0x1ffffff);
	// MCS32/ MCS15_SG use max AMPDU size 15*2=30K
	PlatformEFIOWrite1Byte(Adapter, AGGLEN_LMT_H, 0xff);
	// MCS0/1/2/3 use max AMPDU size 4*2=8K
	PlatformEFIOWrite2Byte(Adapter, AGGLEN_LMT_L, 0x4444);
	// MCS4/5 use max AMPDU size 8*2=16K 6/7 use 10*2=20K
	PlatformEFIOWrite2Byte(Adapter, AGGLEN_LMT_L+2, 0xAA88);
	// MCS8/9 use max AMPDU size 8*2=16K 10/11 use 10*2=20K
	PlatformEFIOWrite2Byte(Adapter, AGGLEN_LMT_L+4, 0xAA88);
	// MCS12/13/14/15 use max AMPDU size 15*2=30K
	PlatformEFIOWrite2Byte(Adapter, AGGLEN_LMT_L+4, 0xFFFF);
#endif

	// Set Data / Response auto rate fallack retry count
	//PlatformEFIOWrite4Byte(Adapter, DARFRC, 0x04030201);
	//PlatformEFIOWrite4Byte(Adapter, DARFRC+4, 0x08070605);
	//PlatformEFIOWrite4Byte(Adapter, RARFRC, 0x04030201);
	//PlatformEFIOWrite4Byte(Adapter, RARFRC+4, 0x08070605);

	// MCS/CCK TX Auto Gain Control Register
	//PlatformEFIOWrite4Byte(Adapter, MCS_TXAGC, 0x0D0C0C0C);
	//PlatformEFIOWrite4Byte(Adapter, MCS_TXAGC+4, 0x0F0E0E0D);
	//PlatformEFIOWrite2Byte(Adapter, CCK_TXAGC, 0x0000);


	//
	// 7. EDCA Setting Register (Offset: 0x01D0 - 0x01FF)
	//
	// BCNTCFG
	//PlatformEFIOWrite2Byte(Adapter, BCNTCFG, 0x0000);
	// Set all rate to support SG
	RTL_W16(SG_RATE, 0xFFFF);


	//
	// 8. WMAC, BA, and CCX related Register (Offset: 0x0200 - 0x023F)
	//
	// Set NAV protection length
	RTL_W16(NAV_PROT_LEN, 0x01C0);
	// CF-END Threshold
	RTL_W8(CFEND_TH, 0xFF);
	// Set AMPSU minimum space
	RTL_W8(AMPDU_MIN_SPACE, 0xFF);
	// Set TXOP stall control for several queue/HI/BCN/MGT/
	RTL_W8(TXOP_STALL_CTRL, 0x00);

	RTL_W32(EDCAPARA_BE, 0xa44f);
	// 9. Security Control Register (Offset: 0x0240 - 0x025F)
	// 10. Power Save Control Register (Offset: 0x0260 - 0x02DF)
	// 11. General Purpose Register (Offset: 0x02E0 - 0x02FF)
	// 12. Host Interrupt Status Register (Offset: 0x0300 - 0x030F)
	// 13. Test Mode and Debug Control Register (Offset: 0x0310 - 0x034F)

	//
	// 13. HOST/MAC PCIE Interface Setting
	//
	// For 92Se if we can not assign as 0x77 in init step~~~~ other wise TP is bad!!!!???,
	// Set Tx dma burst
	/*PlatformEFIOWrite1Byte(Adapter, PCIF, ((MXDMA2_NoLimit<<MXDMA2_RX_SHIFT) | \
											(MXDMA2_NoLimit<<MXDMA2_TX_SHIFT) | \
											(1<<MULRW_SHIFT)));*/
	//3 Set Tx/Rx Desc Address
	/*PlatformEFIOWrite4Byte(Adapter, RDQDA, pHalData->RxDescMemory[RX_MPDU_QUEUE].PhysicalAddressLow);
	PlatformEFIOWrite4Byte(Adapter, RCQDA, pHalData->RxDescMemory[RX_CMD_QUEUE].PhysicalAddressLow);

	PlatformEFIOWrite4Byte(Adapter, TCDA	, pHalData->TxDescMemory[TXCMD_QUEUE].PhysicalAddressLow);  // Command queue address borrowed VI descriptors
	PlatformEFIOWrite4Byte(Adapter, TMDA, 	pHalData->TxDescMemory[MGNT_QUEUE].PhysicalAddressLow);
	PlatformEFIOWrite4Byte(Adapter, TVODA, pHalData->TxDescMemory[VO_QUEUE].PhysicalAddressLow);
	PlatformEFIOWrite4Byte(Adapter, TVIDA, 	pHalData->TxDescMemory[VI_QUEUE].PhysicalAddressLow);
	PlatformEFIOWrite4Byte(Adapter, TBEDA, pHalData->TxDescMemory[BE_QUEUE].PhysicalAddressLow);
	PlatformEFIOWrite4Byte(Adapter, TBKDA, pHalData->TxDescMemory[BK_QUEUE].PhysicalAddressLow);
	PlatformEFIOWrite4Byte(Adapter, TBDA, pHalData->TxDescMemory[BEACON_QUEUE].PhysicalAddressLow);
	*/

}	/* MacConfigAfterFwDownload */
#endif


//
// Description:
//	Set the MAC offset [0x09] and prevent all I/O for a while (about 20us~200us, suggested from SD4 Scott).
//	If the protection is not performed well or the value is not set complete, the next I/O will cause the system hang.
// Note:
//	This procudure is designed specifically for 8192S and references the platform based variables
//	which violates the stucture of multi-platform.
//	Thus, we shall not extend this procedure to common handler.
// By Bruce, 2009-01-08.
//
unsigned char
HalSetSysClk8192SE(	struct rtl8190_priv *priv,	unsigned char Data)
{
#if !defined(RTL8192SU)
	unsigned long	ioaddr = priv->pshare->ioaddr;
#endif	
	RTL_W8((SYS_CLKR + 1), Data);
	delay_us(200);
	return TRUE;
}

#ifdef RTL8192SU
static void  RTL8192SU_MacConfigBeforeFwDownload(struct rtl8190_priv *priv)
{
	//unsigned char		i;
	volatile unsigned char		tmpU1b;
	unsigned char		PollingCnt = 20;

	tmpU1b = RTL_R8(SYS_CLKR+1);
	if(tmpU1b & BIT(7))
	{
		tmpU1b &= ~(BIT(6)|BIT(7));
		if(!HalSetSysClk8192SE(priv, tmpU1b))
			return; // Set failed, return to prevent hang.
	}

	RTL_W8(LDOA15_CTRL, 0x34); 
	tmpU1b = RTL_R8(AFE_PLL_CTRL);
	RTL_W8(AFE_PLL_CTRL, (tmpU1b&0xee));

	tmpU1b = RTL_R8(SYS_FUNC_EN+1);
	tmpU1b &= 0x73;
	RTL_W8(SYS_FUNC_EN+1, tmpU1b);
	delay_ms(1000);

	//Revised POS, suggested by SD1 Alex, 2008.09.27.
	RTL_W8(SPS0_CTRL+1, 0x53);
	RTL_W8(SPS0_CTRL, 0x57);

	//Enable AFE Macro Block's Bandgap adn Enable AFE Macro Block's Mbias
	tmpU1b = RTL_R8(AFE_MISC);
	RTL_W8(AFE_MISC, (tmpU1b|AFE_BGEN)); //Bandgap //Bandgap
	RTL_W8(AFE_MISC, (tmpU1b|AFE_BGEN|AFE_MBEN));//Mbios

	//Enable PLL Power (LDOA15V)
	RTL_W8(LDOA15_CTRL, 0x35);

	//Enable LDOV12D block
	tmpU1b = RTL_R8(LDOV12D_CTRL);	
	RTL_W8(LDOV12D_CTRL, (tmpU1b|LDV12_EN));	
	
	tmpU1b = RTL_R8(SYS_ISO_CTRL+1);	
	RTL_W8(SYS_ISO_CTRL+1, (tmpU1b|0x08));

	//Engineer Packet CP test Enable
	tmpU1b = RTL_R8(SYS_FUNC_EN+1);	
	RTL_W8(SYS_FUNC_EN+1, (tmpU1b|0x20));

	//Support 64k IMEM, suggested by SD1 Alex.
	tmpU1b = RTL_R8(SYS_ISO_CTRL+1);	
	RTL_W8(SYS_ISO_CTRL+1, (tmpU1b& 0x68));

	//Enable AFE clock
	tmpU1b = RTL_R8(AFE_XTAL_CTRL+1);	
	RTL_W8(AFE_XTAL_CTRL+1, (tmpU1b& 0xfb));

	//Enable AFE PLL Macro Block
	tmpU1b = RTL_R8(AFE_PLL_CTRL);	
	RTL_W8(AFE_PLL_CTRL, (tmpU1b|0x11));

	delay_us(100);
	RTL_W8(AFE_PLL_CTRL, 0x51);
	delay_us(1);
	RTL_W8(AFE_PLL_CTRL, 0x11);
	delay_us(1);


	//Attatch AFE PLL to MACTOP/BB/PCIe Digital
	tmpU1b = RTL_R8(SYS_ISO_CTRL);	
	RTL_W8(SYS_ISO_CTRL, (tmpU1b&0xEE));

	// Switch to 40M clock
	//RTL_W8(SYS_CLKR, 0x00);

	//CPU Clock and 80M Clock SSC Disable to overcome FW download fail timing issue.
	tmpU1b = RTL_R8(SYS_CLKR);	
	RTL_W8(SYS_CLKR, (tmpU1b|0xa0));

	//Enable MAC clock
	tmpU1b = RTL_R8(SYS_CLKR+1);	
	RTL_W8(SYS_CLKR+1, (tmpU1b|0x18));

	//Revised POS, suggested by SD1 Alex, 2008.09.27.
	RTL_W8(PMC_FSM, 0x02);
	
	//Enable Core digital and enable IOREG R/W
	tmpU1b = RTL_R8(SYS_FUNC_EN+1);	
	RTL_W8(SYS_FUNC_EN+1, (tmpU1b|0x08));

	//Enable REG_EN
	tmpU1b = RTL_R8(SYS_FUNC_EN+1);	
	RTL_W8(SYS_FUNC_EN+1, (tmpU1b|0x80));

	//Switch the control path to FW
	tmpU1b = RTL_R8(SYS_CLKR+1);	
	RTL_W8(SYS_CLKR+1, (tmpU1b|0x80)& 0xBF);

	RTL_W8(CMDR, 0xFC);
	RTL_W8(CMDR+1, 0x37);

	//Fix the RX FIFO issue(usb error), 970410
	tmpU1b = RTL_R8(0xFE5c);	
	RTL_W8(0xFE5c, (tmpU1b|BIT(7)));
	
	 //For power save, used this in the bit file after 970621
	tmpU1b = RTL_R8(SYS_CLKR);	
	RTL_W8(SYS_CLKR, tmpU1b&(~SYS_CPU_CLKSEL));	
	
	// Revised for 8051 ROM code wrong operation. Added by Roger. 2008.10.16. 
	RTL_W8(0xfe1c, 0x80);

	if (priv->pmib->efuseEntry.usb_epnum==USBEP_SIX)
	{
		tmpU1b = RTL_R8(0x00ab);	
		RTL_W8(0x00ab, (tmpU1b|BIT(6)|BIT(7)));
	}
	else //USBEP_FOUR
	{
		tmpU1b = RTL_R8( 0xab); // RQPN 
		RTL_W8( 0xab, (tmpU1b|BIT(5)|BIT(6)|BIT(7)));
	}
	//
	// <Roger_EXP> To make sure that TxDMA can ready to download FW.
	// We should reset TxDMA if IMEM RPT was not ready.
	// Suggested by SD1 Alex. 2008.10.23.
	//
	do
	{
		tmpU1b = RTL_R8(TCR);
		if((tmpU1b & (IMEM_CHK_RPT|EXT_IMEM_CHK_RPT)) == (IMEM_CHK_RPT|EXT_IMEM_CHK_RPT))
			break;

		delay_ms(5);
	}while(PollingCnt--);	// Delay 1ms

	if(PollingCnt <= 0 )
	{
		DEBUG_ERR("MacConfigBeforeFwDownloadASIC(): Polling TXDMA_INIT_VALUE timeout!! Current TCR(%x)\n", tmpU1b);
		tmpU1b = RTL_R8(CMDR);
		RTL_W8(CMDR, tmpU1b&(~TXDMA_EN));
		delay_ms(2);
		RTL_W8(CMDR, tmpU1b|TXDMA_EN);// Reset TxDMA
	}


#ifdef RTL867X_DMEM_ENABLE
	if(priv->pshare->fw_IMEM_buf==NULL) priv->pshare->fw_IMEM_buf=kmalloc(FW_IMEM_SIZE,GFP_KERNEL);
	if(priv->pshare->fw_EMEM_buf==NULL) priv->pshare->fw_EMEM_buf=kmalloc(FW_EMEM_SIZE,GFP_KERNEL);
	if(priv->pshare->fw_DMEM_buf==NULL) priv->pshare->fw_DMEM_buf=kmalloc(FW_DMEM_SIZE,GFP_KERNEL);
	if(priv->pshare->agc_tab_buf==NULL) priv->pshare->agc_tab_buf=kmalloc(AGC_TAB_SIZE,GFP_KERNEL);
	if(priv->pshare->mac_reg_buf==NULL) priv->pshare->mac_reg_buf=kmalloc(MAC_REG_SIZE,GFP_KERNEL);
	if(priv->pshare->phy_reg_buf==NULL) priv->pshare->phy_reg_buf=kmalloc(PHY_REG_SIZE,GFP_KERNEL);
#endif

#ifdef RTL8192SU_FWBCN
	RTL_W16(BCN_DRV_EARLY_INT, 0x0000);
#endif
//	RT_TRACE(COMP_INIT, DBG_LOUD, ("<---MacConfig8192SE()\n"));

}
#else
// Chip version
/*-----------------------------------------------------------------------------
 * Function:	MacConfigBeforeFwDownload()
 *
 * Overview:	Copy from WMAc for init setting. MAC config before FW download
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	04/29/2008	MHC		The same as 92SE MAC initialization.
 *	07/11/2008	MHC		MAC config before FW download
 *	09/02/2008	MHC		The initialize sequence can preven NIC reload fail
 *						NIC will not disappear when power domain init twice.
 *
 *---------------------------------------------------------------------------*/
static void  MacConfigBeforeFwDownload(struct rtl8190_priv *priv)
{
	unsigned char		i;
	unsigned char		tmpU1b;
	unsigned short		tmpU2b;
//	unsigned int		addr;
	unsigned long	ioaddr = priv->pshare->ioaddr;
	unsigned char PollingCnt = 20;
	struct rtl8190_hw *phw = GET_HW(priv);

//	RT_TRACE(COMP_INIT, DBG_LOUD, ("--->MacConfig8192SE()\n"));

	// 2008/05/14 MH For 92SE rest before init MAC data sheet not defined now.
	// HW provide the method to prevent press reset bbutton every time.
//	RT_TRACE(COMP_INIT, DBG_LOUD, ("Set some register before enable NIC\r\n"));

	// 2009/03/24 MH Reset PCIE Digital
	tmpU1b = RTL_R8(SYS_FUNC_EN+1);
	tmpU1b &= 0xFE;
	RTL_W8(SYS_FUNC_EN+1, tmpU1b);
	delay_ms(1);
	RTL_W8(SYS_FUNC_EN+1, tmpU1b|BIT(0));

	// Switch to SW IO control
	tmpU1b = RTL_R8((SYS_CLKR + 1));
	if(tmpU1b & BIT(7))
	{
		tmpU1b &= ~(BIT(6)|BIT(7));
		if(!HalSetSysClk8192SE(priv, tmpU1b))
			return; // Set failed, return to prevent hang.
	}

	// Clear FW RPWM for FW control LPS. by tynli. 2009.02.23
	RTL_W8(RPWM, 0x0);

	tmpU1b = RTL_R8(SYS_FUNC_EN+1);

//	tmpU1b &= 0xfb;
	tmpU1b &= 0x73;

	RTL_W8(SYS_FUNC_EN+1, tmpU1b);
	// wait for BIT 10/11/15 to pull high automatically!!
	delay_ms(1000);
	RTL_W16( CMDR, 0);
	RTL_W32( TCR, 0);
/*
	tmpU1b = RTL_R8(SPS1_CTRL);
	tmpU1b &= 0xfc;
	RTL_W8(SPS1_CTRL, tmpU1b);
*/

	tmpU1b = RTL_R8(0x562);
	tmpU1b |= 0x08;
	RTL_W8(0x562, tmpU1b);
	tmpU1b &= ~(BIT(3));
	RTL_W8(0x562, tmpU1b);

	//tmpU1b = PlatformEFIORead1Byte(Adapter, SYS_FUNC_EN+1);
	//tmpU1b &= 0x73;
	//PlatformEFIOWrite1Byte(Adapter, SYS_FUNC_EN+1, tmpU1b);

	//tmpU1b = PlatformEFIORead1Byte(Adapter, SYS_CLKR);
	//tmpU1b &= 0xfa;
	//PlatformEFIOWrite1Byte(Adapter, SYS_CLKR, tmpU1b);

	// Switch to 80M clock
	//PlatformEFIOWrite1Byte(Adapter, SYS_CLKR, SYS_CLKSEL_80M);

	 // Enable VSPS12 LDO Macro block
	//tmpU1b = PlatformEFIORead1Byte(Adapter, SPS1_CTRL);
	//PlatformEFIOWrite1Byte(Adapter, SPS1_CTRL, (tmpU1b|SPS1_LDEN));

	//Enable AFE clock source
//	RT_TRACE(COMP_INIT, DBG_LOUD, 	("Enable AFE clock source\r\n"));
	tmpU1b = RTL_R8(AFE_XTAL_CTRL);
	RTL_W8(AFE_XTAL_CTRL, (tmpU1b|0x01));
	// Delay 1.5ms
	delay_ms(2);
	tmpU1b = RTL_R8(AFE_XTAL_CTRL+1);
	RTL_W8(AFE_XTAL_CTRL+1, (tmpU1b&0xfb));



	//Enable AFE Macro Block's Bandgap
//	RT_TRACE(COMP_INIT, DBG_LOUD, 	("Enable AFE Macro Block's Bandgap\r\n"));
	tmpU1b = RTL_R8(AFE_MISC);
	RTL_W8(AFE_MISC, (tmpU1b|BIT(0)));
	delay_ms(2);

	//Enable AFE Mbias
//	RT_TRACE(COMP_INIT, DBG_LOUD, 	("Enable AFE Mbias\r\n"));
	tmpU1b = RTL_R8(AFE_MISC);
	RTL_W8(AFE_MISC, (tmpU1b|0x02));
	delay_ms(2);

	//Enable LDOA15 block
	tmpU1b = RTL_R8(LDOA15_CTRL);
	RTL_W8(LDOA15_CTRL, (tmpU1b|BIT(0)));


	 //Enable VSPS12_SW Macro Block
	//tmpU1b = PlatformEFIORead1Byte(Adapter, SPS1_CTRL);
	//PlatformEFIOWrite1Byte(Adapter, SPS1_CTRL, (tmpU1b|SPS1_SWEN));

	//Enable AFE Macro Block's Mbias
	//tmpU1b = PlatformEFIORead1Byte(Adapter, AFE_MISC);
	//PlatformEFIOWrite1Byte(Adapter, AFE_MISC, (tmpU1b|AFE_MBEN));

	// Set Digital Vdd to Retention isolation Path.
	tmpU2b =RTL_R16(SYS_ISO_CTRL);
	RTL_W16(SYS_ISO_CTRL, (tmpU2b|BIT(11)));

	// 2008/09/25 MH From SD1 Jong, For warm reboot NIC disappera bug.
	tmpU2b = RTL_R16(SYS_FUNC_EN);
	RTL_W16( SYS_FUNC_EN, tmpU2b |= BIT(13));

	RTL_W8(SYS_ISO_CTRL+1, 0x68);

	tmpU1b = RTL_R8(AFE_PLL_CTRL);
	RTL_W8(AFE_PLL_CTRL, (tmpU1b|BIT(0)|BIT(4)));
	// Enable MAC 80MHZ clock
	tmpU1b = RTL_R8(AFE_PLL_CTRL+1);
	RTL_W8( AFE_PLL_CTRL+1, (tmpU1b|BIT(0)));
	delay_ms(2);
	// Attatch AFE PLL to MACTOP/BB/PCIe Digital
//	tmpU2b = RTL_R16(SYS_ISO_CTRL);
//	RTL_W16(SYS_ISO_CTRL, (tmpU2b &(~ BIT(12))));

	//Enable AFE clock
	//tmpU2b = PlatformEFIORead2Byte(Adapter, AFE_XTAL_CTRL);
	//PlatformEFIOWrite2Byte(Adapter, AFE_XTAL_CTRL, (tmpU2b &(~XTAL_GATE_AFE)));

	// Release isolation AFE PLL & MD
	RTL_W8(SYS_ISO_CTRL, 0xA6);



	//Enable MAC clock
	tmpU2b = RTL_R16( SYS_CLKR);
	//PlatformEFIOWrite2Byte(Adapter, SYS_CLKR, (tmpU2b|SYS_MAC_CLK_EN));
	RTL_W16(SYS_CLKR, (tmpU2b|BIT(12)|BIT(11)));


	//Enable Core digital and enable IOREG R/W
	tmpU2b = RTL_R16(SYS_FUNC_EN);
	RTL_W16(SYS_FUNC_EN, (tmpU2b|BIT(11)));

	tmpU1b = RTL_R8(SYS_FUNC_EN+1);
	RTL_W8(SYS_FUNC_EN+1, tmpU1b&~(BIT(7)));

	// 2008/09/25 MH From SD1 Jong, For warm reboot NIC disappera bug.
// 	tmpU2b = RTL_R16(SYS_FUNC_EN);
//	RTL_W16(SYS_FUNC_EN, tmpU2b | BIT(13));

	//enable REG_EN
	RTL_W16(SYS_FUNC_EN, (tmpU2b|BIT(11)|BIT(15)));

	 //Switch the control path.
	 tmpU2b = RTL_R16(SYS_CLKR);
	RTL_W16(SYS_CLKR, (tmpU2b&(~BIT(2))));

	tmpU1b = RTL_R8((SYS_CLKR + 1));
	tmpU1b = ((tmpU1b | BIT(7)) & (~BIT(6)));
	if(!HalSetSysClk8192SE(priv, tmpU1b))
		return; // Set failed, return to prevent hang.

	RTL_W16(CMDR, 0x07FC);
#if 1
	// 2008/09/02 MH We must enable the section of code to prevent load IMEM fail.
	// Load MAC register from WMAc temporarily We simulate macreg.txt HW will provide
	// MAC txt later
	RTL_W8(0x6, 0x30);
//	RTL_W8(0x48, 0x3f);
	RTL_W8(0x49, 0xf0);

	RTL_W8(0x4b, 0x81);

	RTL_W8(0xb5, 0x21);

	RTL_W8(0xdc, 0xff);
	RTL_W8(0xdd, 0xff);
	RTL_W8(0xde, 0xff);
	RTL_W8(0xdf, 0xff);

	RTL_W8(0x11a, 0x00);
	RTL_W8(0x11b, 0x00);

	for (i = 0; i < 32; i++)
		RTL_W8(INIMCS_SEL+i, 0x1b); // MCS15

	RTL_W8(CFEND_TH, 0xff);

	RTL_W8( 0x503, 0x22);
//	RTL_W8( 0x560, 0x09);
	RTL_W8( 0x560, 0x40);

	RTL_W8( DBG_PORT, 0x91);
#endif

	RTL_W32(RDSA, phw->ring_dma_addr);
	RTL_W32(TMDA, phw->tx_ring0_addr);
	RTL_W32(TBKDA, phw->tx_ring1_addr);
	RTL_W32(TBEDA, phw->tx_ring2_addr);
	RTL_W32(TVIDA, phw->tx_ring3_addr);
	RTL_W32(TVODA, phw->tx_ring4_addr);
	RTL_W32(THPDA, phw->tx_ring5_addr);
	RTL_W32(TBDA, phw->tx_ringB_addr);
	RTL_W32(RCDA, phw->rxcmd_ring_addr);
	RTL_W32(TCDA, phw->txcmd_ring_addr);
//	RTL_W32(TCDA, phw->tx_ring5_addr);

	// 2009/03/13 MH Prevent incorrect DMA write after accident reset !!!
	RTL_W16(CMDR, 0x37FC);

	//
	// <Roger_EXP> To make sure that TxDMA can ready to download FW.
	// We should reset TxDMA if IMEM RPT was not ready.
	// Suggested by SD1 Alex. 2008.10.23.
	//
	do
	{
		tmpU1b = RTL_R8( TCR);
		if((tmpU1b & (IMEM_CHK_RPT|EXT_IMEM_CHK_RPT)) == (IMEM_CHK_RPT|EXT_IMEM_CHK_RPT))
			break;

		delay_ms(5);
	}while(PollingCnt--);	// Delay 1ms

	if(PollingCnt <= 0 )
	{
		printk("MacConfigBeforeFwDownloadASIC(): Polling TXDMA_INIT_VALUE timeout!! Current TCR(%x)\n", tmpU1b);
		tmpU1b = RTL_R8(CMDR);
		RTL_W8(CMDR, tmpU1b&(~TXDMA_EN));
		delay_ms(10);
		RTL_W8(CMDR, tmpU1b|TXDMA_EN);// Reset TxDMA
	}


//	RT_TRACE(COMP_INIT, DBG_LOUD, ("<---MacConfig8192SE()\n"));

	RTL_W16(BCN_DRV_EARLY_INT, (10<<4)); // 2
#ifdef FW_SW_BEACON
	if (priv->pmib->miscEntry.vap_enable)
		RTL_W16(BCN_DRV_EARLY_INT, RTL_R16(BCN_DRV_EARLY_INT)|BIT(15)); // sw beacon
#endif
}	/* MacConfigBeforeFwDownload */
#endif //RTL8192SU

/*-----------------------------------------------------------------------------
 * Function:	MacConfigAfterFwDownload()
 *
 * Overview:	After download Firmware, we must set some MAC register for some dedicaed
 *			purpose.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	07/02/2008	MHC		Create.
 *
 *---------------------------------------------------------------------------*/
static void MacConfigAfterFwDownload(struct rtl8190_priv *priv)
{
#if !defined(RTL8192SU)
	unsigned long	ioaddr = priv->pshare->ioaddr;
#else	
	unsigned char tmpU1b;

#ifdef RTL867X_DMEM_ENABLE
	if(priv->pshare->fw_IMEM_buf!=NULL) {kfree(priv->pshare->fw_IMEM_buf); priv->pshare->fw_IMEM_buf=NULL; }
	if(priv->pshare->fw_EMEM_buf!=NULL) {kfree(priv->pshare->fw_EMEM_buf); priv->pshare->fw_EMEM_buf=NULL; }
	if(priv->pshare->fw_DMEM_buf!=NULL) {kfree(priv->pshare->fw_DMEM_buf); priv->pshare->fw_DMEM_buf=NULL; }
#endif
#endif
	//
	// 1. System Configure Register (Offset: 0x0000 - 0x003F)
	//

	//
	// 2. Command Control Register (Offset: 0x0040 - 0x004F)
	//
	// Turn on 0x40 Command register
	// Original is RTL_W8 ... but should be RTL_W16?
	RTL_W16(CMDR, BBRSTn|BB_GLB_RSTn|SCHEDULE_EN|MACRXEN|MACTXEN|DDMA_EN|FW2HW_EN|	RXDMA_EN|TXDMA_EN|HCI_RXDMA_EN|HCI_TXDMA_EN);

	// for sending beacon in current stage, disable RX
//	RTL_W16(CMDR, BBRSTn|BB_GLB_RSTn|SCHEDULE_EN|MACRXEN|MACTXEN|DDMA_EN|FW2HW_EN|	RXDMA_EN|TXDMA_EN|HCI_RXDMA_EN|HCI_TXDMA_EN);

	// Set TCR TX DMA pre 2 FULL enable bit
	RTL_W32(TCR, RTL_R32(TCR)|TXDMAPRE2FULL);
	// Set RCR ... disable RX now, will enable later

	RTL_W32(RCR, RCR_APPFCS |RCR_APP_PHYST_RXFF | RCR_APP_PHYST_RXFF | RCR_APWRMGT /*| RCR_ADD3*/ |	RCR_AMF	| RCR_ADF | RCR_APP_MIC | RCR_APP_ICV |       RCR_AICV	| /*RCR_ACRC32	|*/				// Accept ICV error, CRC32 Error
	RCR_AB 		| RCR_AM		|				// Accept Broadcast, Multicast
    	RCR_APM 	|  								// Accept Physical match
    /* 	RCR_AAP		|     	*/					// Accept Destination Address packets
	(7/*pHalData->EarlyRxThreshold*/<<RCR_FIFO_OFFSET));

	//
	// 3. MACID Setting Register (Offset: 0x0050 - 0x007F)
	//


	//
	// 4. Timing Control Register  (Offset: 0x0080 - 0x009F)
	//
	// Set CCK/OFDM SIFS

	RTL_W16(SIFS_CCK, 0x0a0a);//0x1010
	RTL_W16(SIFS_OFDM, 0x0a0a);// 0x1010
	//3
	//Set AckTimeout
	RTL_W8(ACK_TIMEOUT, 0x40);

	//Beacon related
	set_fw_reg(priv, 0xf1006400, 0, 0);
//	RTL_W32(0x2A4, 0x00006300);
//	RTL_W32(0x2A0, 0xb026007C);
//	delay_ms(1);
//	while(RTL_R32(0x2A0) != 0){};
	RTL_W16(ATIMWND, 2);
	//PlatformEFIOWrite2Byte(Adapter, BCN_DRV_EARLY_INT, 0x0022);
	//PlatformEFIOWrite2Byte(Adapter, BCN_DMATIME, 0xC8);
	//PlatformEFIOWrite2Byte(Adapter, BCN_ERR_THRESH, 0xFF);
	//PlatformEFIOWrite1Byte(Adapter, MLT, 8);

	//
	// 5. FIFO Control Register (Offset: 0x00A0 - 0x015F)
	//
	// Initialize Number of Reserved Pages in Firmware Queue
#if 0
	PlatformEFIOWrite4Byte(Adapter, RQPN1,
	NUM_OF_PAGE_IN_FW_QUEUE_BK<<0 | NUM_OF_PAGE_IN_FW_QUEUE_BE<<8 |\
	NUM_OF_PAGE_IN_FW_QUEUE_VI<<16 | NUM_OF_PAGE_IN_FW_QUEUE_VO<<24);
	PlatformEFIOWrite4Byte(Adapter, RQPN2,
	NUM_OF_PAGE_IN_FW_QUEUE_HCCA << 0 | NUM_OF_PAGE_IN_FW_QUEUE_CMD << 8|\
	NUM_OF_PAGE_IN_FW_QUEUE_MGNT << 16 |NUM_OF_PAGE_IN_FW_QUEUE_HIGH << 24);
	PlatformEFIOWrite4Byte(Adapter, RQPN3,
	NUM_OF_PAGE_IN_FW_QUEUE_BCN<<0 | NUM_OF_PAGE_IN_FW_QUEUE_PUB<<8);
	// Active FW/MAC to load the new RQPN setting
	PlatformEFIOWrite1Byte(Adapter, LD_RQPN, BIT7);
#endif
	// Setting TX/RX page size 0/1/2/3/4=64/128/256/512/1024
	//PlatformEFIOWrite1Byte(Adapter, PBP, 0x22);


	// Set RXDMA arbitration to control RXDMA/MAC/FW R/W for RXFIFO
	// Set RXDMA arbitration to control RXDMA/MAC/FW R/W for RXFIFO
	RTL_W8( RXDMA_, RTL_R8(RXDMA_)|BIT(6));

	//
	// 6. Adaptive Control Register  (Offset: 0x0160 - 0x01CF)
	//
	// Set RRSR to all legacy rate and HT rate
	// CCK rate is supported by default.
	// CCK rate will be filtered out only when associated AP does not support it.
	// Only enable ACK rate to OFDM 24M
#ifdef RTL8192SE_ACUT
	RTL_W8(RRSR, 0xf0);
#else
	RTL_W8(RRSR, 0x5f);
#endif
	RTL_W8(RRSR+1, 0x01);
//	RTL_W8(RRSR+1, 0xff);
	RTL_W8(RRSR+2, 0x0);
	//
	// 6. Adaptive Control Register  (Offset: 0x0160 - 0x01CF)
	//
	// Set RRSR to all legacy rate and HT rate
	// CCK rate is supported by default.
	// CCK rate will be filtered out only when associated AP does not support it.
	// Only enable ACK rate to OFDM 24M

	// ARFR table set by f/w, 20081130
#if 0
	RTL_W32(ARFR0, 0x1f0ff0f0);
	RTL_W32(ARFR1, 0x1f0ff0f0);
	RTL_W32(ARFR2, 0x1f0ff0f0);
	RTL_W32(ARFR3, 0x1f0ff0f0);
	RTL_W32(ARFR4, 0x1f0ff0f0);
	RTL_W32(ARFR5, 0x1f0ff0f0);
	RTL_W32(ARFR6, 0x1f0ff0f0);
	RTL_W32(ARFR7, 0x1f0ff0f0);
#endif
	// Different rate use different AMPDU size
	// MCS32/ MCS15_SG use max AMPDU size 15*2=30K
	RTL_W8(AGGLEN_LMT_H, 0x0f);
	// MCS0/1/2/3 use max AMPDU size 4*2=8K
	RTL_W16(AGGLEN_LMT_L, 0x5221);
	// MCS4/5 use max AMPDU size 8*2=16K 6/7 use 10*2=20K
	RTL_W16(AGGLEN_LMT_L+2, 0xBBB5);
	// MCS8/9 use max AMPDU size 8*2=16K 10/11 use 10*2=20K
	RTL_W16(AGGLEN_LMT_L+4, 0xB551);
	// MCS12/13/14/15 use max AMPDU size 15*2=30K
	RTL_W16(AGGLEN_LMT_L+6, 0xFFFB);

	// WHChang's suggestion
	RTL_W32(DARFRC, 0x01000000);
	RTL_W32(DARFRC+4, 0x07060504);
	RTL_W32(RARFRC, 0x01000000);
	RTL_W32(RARFRC+4, 0x07060504);

	set_fw_reg(priv, 0xfd0000af, 0 ,0);
	set_fw_reg(priv, 0xfd0000a6, 0 ,0);
	set_fw_reg(priv, 0xfd0000a0, 0 ,0);

//	RTL_W32(0x2c0, 0xd0000000);
	RTL_W32(0x1d8, 0xa44f);


	//
	// 7. EDCA Setting Register (Offset: 0x01D0 - 0x01FF)
	//
	// BCNTCFG
	//PlatformEFIOWrite2Byte(Adapter, BCNTCFG, 0x0000);
	// Set all rate to support SG
//	RTL_W16(SG_RATE, 0xFFFF);	// set by fw, 20090708


	//
	// 8. WMAC, BA, and CCX related Register (Offset: 0x0200 - 0x023F)
	//
#if 0
	// Set NAV protection length
	RTL_W16(NAV_PROT_LEN, 0x01C0);
	// CF-END Threshold
	RTL_W8(CFEND_TH, 0xFF);
#endif

#ifdef CHECK_HANGUP
	if (!priv->reset_hangup)
#endif
	{
		// Set AMPSU minimum space
		priv->pshare->min_ampdu_spacing = 0;

		// for RF reset
		RTL_W8(0x1F, 0x0);
		delay_ms(1);
		RTL_W8(0x1F, 0x07);
	}

	RTL_W8(AMPDU_MIN_SPACE, priv->pshare->min_ampdu_spacing);
	// Set TXOP stall control for several queue/HI/BCN/MGT/
	RTL_W8(TXOP_STALL_CTRL, 0x00);

    //Set Driver INFO size
	RTL_W8(RXDRVINFO_SZ, 0x4);

#ifdef RTL8192SU
	if (priv->pmib->efuseEntry.usb_epnum==USBEP_SIX)
	{
		tmpU1b = RTL_R8( 0xab); // RQPN
		RTL_W8( 0xab, tmpU1b|BIT(7)|BIT(6));// reduce to 6 endpoints.
	}
	else //USBEP_FOUR
	{
		tmpU1b = RTL_R8( 0xab); // RQPN 
		RTL_W8( 0xab, (tmpU1b|BIT(5)|BIT(6)|BIT(7)));
	}
#endif
	//Set RQPN
#if 0
	RTL_W32(RQPN1,0x15150707);
	RTL_W32(RQPN2,0x03030000);
	RTL_W32(RQPN3,0x8000bc00);
#endif
#if !defined(RTL8192SU_FWBCN)
	if (!RTL_R8(RQPN3) && (RTL_R8(RQPN3+1)>1)) {
		//default assign bcn q 2 page when pub q page >= 2 and bcn q page == 0
		RTL_W8(RQPN3+1, RTL_R8(RQPN3+1)-2);
		RTL_W8(RQPN3, 2);
	}
#endif	
#ifdef RTL8192SU
	// Fix the RX FIFO issue(USB error), Rivesed by Roger, 2008-06-14
	tmpU1b = RTL_R8( 0xfe5C);
	RTL_W8(0xfe5C, tmpU1b|BIT(7));

	//
	// Revise USB PHY to solve the issue of Rx payload error, Rivesed by Roger,  2008-04-10
	// Suggest by SD1 Alex.
	//
	RTL_W8( 0xfe41,0xf4); 
	RTL_W8( 0xfe40,0x00); 
	RTL_W8( 0xfe42,0x00);
	RTL_W8( 0xfe42,0x01);
	RTL_W8( 0xfe40,0x0f);
	RTL_W8( 0xfe42,0x00); 
	RTL_W8( 0xfe42,0x01);
#endif

	// for warm reboot.
        RTL_W8(0x03, RTL_R8(0x03) | BIT(4));
        RTL_W8(0x01, RTL_R8(0x01) & 0xFE);

	//Make sure DIG is done by false alarm
	set_fw_reg(priv, 0xfd00ff00, 0 ,0);

	// Disable DIG as default
//	set_fw_reg(priv, 0xfd000001, 0, 0);
	RTL_W8(0x364, RTL_R8(0x364) &  ~FW_REG364_DIG);

#if defined(EXT_ANT_DVRY) && (defined(RTL8192SE)||defined(RTL8192SU))
//for giga 2T3R
	RTL_W8(0x2f1, RTL_R8(0x2f1)|BIT(3));
	RTL_W32(0x2ec, 0xff0400);
#endif

  // Loopback mode or not 
#ifdef LOOPBACK_MODE
	RTL_W8(MSR, MSR_NOLINK);
	RTL_W8(LBKMD_SEL,LBK_MAC_DLB); 
#endif

	DEBUG_INFO("<---MacConfigAfterFwDownload()\n");
}	/* MacConfigAfterFwDownload */


int rtl819x_init_hw_PCI(struct rtl8190_priv *priv)
{
	static struct wifi_mib *pmib;
	static unsigned int opmode;
#if !defined(RTL8192SU)	
	static unsigned long ioaddr;
#endif	
	static unsigned long val32;
	static unsigned short val16;
	static int i;
	static unsigned short fixed_rate;
	static unsigned int ii;
	static unsigned char calc_rate, val8;

	pmib = GET_MIB(priv);
	opmode = priv->pmib->dot11OperationEntry.opmode;
#if !defined(RTL8192SU)	
	ioaddr = priv->pshare->ioaddr;
#endif

	DBFENTER;

//	GetHardwareVersion(priv); //we don't need to get hw version under 8192SE yet

#ifdef DW_FW_BY_MALLOC_BUF
	if ((priv->pshare->fw_IMEM_buf = kmalloc(FW_IMEM_SIZE, GFP_ATOMIC)) == NULL) {
		DEBUG_ERR("alloc fw_IMEM_buf failed!\n");
		return -1;
	}
	if ((priv->pshare->fw_EMEM_buf = kmalloc(FW_EMEM_SIZE, GFP_ATOMIC)) == NULL) {
		DEBUG_ERR("alloc fw_EMEM_buf failed!\n");
		return -1;
	}
	if ((priv->pshare->fw_DMEM_buf = kmalloc(FW_DMEM_SIZE, GFP_ATOMIC)) == NULL) {
		DEBUG_ERR("alloc fw_DMEM_buf failed!\n");
		return -1;
	}
#endif

	ReadEFuseByte(priv, EFUSE_IC_ID_OFFSET, &val8);
	if (val8 == 0xfe) {
		priv->pshare->IC_Class = IC_INFERIORITY_B;
		printk("B class\n");
	}
	else {
		priv->pshare->IC_Class = IC_INFERIORITY_A;
		printk("A class\n");
	}


//1 For Test, Firmware Downloading

#if !defined(RTL8192SU)
	MacConfigBeforeFwDownload(priv);
#else
	RTL8192SU_MacConfigBeforeFwDownload(priv);
#endif	

	DEBUG_INFO("MacConfigBeforeFwDownload\n");

	// ===========================================================================================
	// Download Firmware
	// allocate memory for tx cmd packet
	if((priv->pshare->txcmd_buf = (unsigned char *)kmalloc((LoadPktSize), GFP_ATOMIC)) == NULL) {
		printk("not enough memory for txcmd_buf\n");
		return -1;
	}

	priv->pshare->cmdbuf_phyaddr = get_physical_addr(priv, priv->pshare->txcmd_buf,
			LoadPktSize, PCI_DMA_TODEVICE);

	if(LoadFirmware(priv) == FALSE){
//		panic_printk("Load Firmware Fail!\n");
#ifdef CONFIG_PRINTK_DISABLED
		panic_printk
#else
		printk
#endif
                ("Load Firmware check!\n");
#if !defined(RTL8192SU)
		return -1;
#else
		return LOADFW_FAIL;
#endif
	}else {
//		delay_ms(20);
		PRINT_INFO("Load firmware successful! \n");
	}

	MacConfigAfterFwDownload(priv);



	//
	// 2. Initialize MAC/PHY Config by MACPHY_reg.txt
	//

	PHY_ConfigMACWithParaFile(priv);// this should be opened in the future

	DEBUG_INFO("finish config MAC\n");
#ifdef LOOPBACK_MODE
	return 0;
#endif

	//
	// 3. Initialize BB After MAC Config PHY_reg.txt, AGC_Tab.txt
	//
#ifdef DRVMAC_LB
	if (!priv->pmib->miscEntry.drvmac_lb)
#endif
	{
		val32 = phy_BB8192SE_Config_ParaFile(priv);
		if (val32)
			return -1;
		DEBUG_INFO("finish config BB\n");
	}

	// support up to MCS7 for 1T1R, modify rx capability here
	if (get_rf_mimo_mode(priv) == MIMO_1T1R)
		pmib->dot11nConfigEntry.dot11nSupportedMCS &= 0x00ff;

	// Set NAV protection length
	// CF-END Threshold
	if (priv->pmib->dot11OperationEntry.wifi_specific) {
		RTL_W16(NAV_PROT_LEN, 0x80);
		RTL_W8(CFEND_TH, 0x2);
	}
	else {
		RTL_W16(NAV_PROT_LEN, 0x01C0);
		RTL_W8(CFEND_TH, 0xFF);
	}

#ifdef BB_LOOPBACK
	{
		RTL_W8(LBKMD_SEL,0x0);
		RTL_W8(MSR,0x0);
		RTL_W8(0xf1c,0x1f);
		RTL_W8(0x803,0x0a);
		RTL_W8(0x804,0x01);
		RTL_W32(0x870,0xffffffff);
		RTL_W32(0x874,0xffffffff);
		RTL_W32(0x90c,0x0110b811);
		RTL_W8(0xc03,0x10);
		RTL_W8(0xc04,0x22);
		RTL_W32(0xc08,0x000100e0);
		RTL_W8(0xd03,0x01);
		RTL_W8(0xd04,0x02);
	return 0;
		//SetBWModeCallback8192SUsb(priv,HT_CHANNEL_WIDTH_20_40,1);
	}

#endif

	//
	// 4. Initiailze RF RAIO_A.txt RF RAIO_B.txt
	//
	// 2007/11/02 MH Before initalizing RF. We can not use FW to do RF-R/W.
	// close loopback, normal mode

	// For RF test only from Scott's suggestion
	RTL_W8(0x27, 0xDB);
//	RTL_W8(0x1B, 0x07); // ACUT


	// set RCR: RX_SHIFT and disable ACF
#if !defined(RTL8192SU)
	RTL_W8(0x48, 0x3e);
#else	
	RTL_W32(RCR, RTL_R32(RCR) | RCR_RXSHFT_EN);
#endif
	RTL_W32(0x48, RTL_R32(0x48) & ~ RCR_ACF  & ~RCR_ACRC32);
	// for debug by victoryman, 20081119
	RTL_W32(RCR, RTL_R32(RCR) | RCR_APP_PHYST_RXFF);

#ifdef DRVMAC_LB
	if (priv->pmib->miscEntry.drvmac_lb) {
		RTL_W8(MSR, MSR_NOLINK);
		drvmac_loopback(priv);
	}
	else
#endif
	{
#ifdef CHECK_HANGUP
		if (!priv->reset_hangup
#ifdef CLIENT_MODE
				|| (!(OPMODE & WIFI_AP_STATE) &&
						(priv->join_res != STATE_Sta_Bss) && (priv->join_res != STATE_Sta_Ibss_Active))
#endif
			)
#endif
		{
			// so far, 8192SE does not need to initialize BB or RF
			val32 = phy_RF8225_Config_ParaFile(priv);
			if (val32)
				return -1;
		}
	}

	if (priv->pshare->IC_Class != IC_INFERIORITY_A) {
		val32 = PHY_QueryRFReg(priv, 0, 0x15, bMask20Bits, 1);
		val32++;
		PHY_SetRFReg(priv, 0, 0x15, bMask20Bits, val32);
	}
	
#ifdef HIGH_POWER_EXT_PA
	if(priv->pshare->rf_ft_var.use_ext_pa)
	{
		PHY_SetBBReg(priv, 0xc80, bMaskDWord, 0x20000080);
		PHY_SetBBReg(priv, 0xc88, bMaskDWord, 0x40000100);
		PHY_SetBBReg(priv, 0xee8, bMaskDWord, 0x31555448);
		PHY_SetBBReg(priv, 0x860, bMaskDWord, 0x0f7f0730);
		PHY_SetBBReg(priv, 0x870,0x00000400, 0x0);

		if (get_rf_mimo_mode(priv) == MIMO_1T2R || get_rf_mimo_mode(priv) == MIMO_1T1R)
		{
			if(priv->pshare->rf_ft_var.pathB_1T == 0)	// turn off PATH B
			{
				PHY_SetBBReg(priv, 0x870,0x04000000, 0x1);
				PHY_SetBBReg(priv, 0x864,0x400, 0x0);			
				PHY_SetBBReg(priv, 0x878,0x000f0000, 0x0);
			} 
			else										// turn off PATH A
			{
				PHY_SetBBReg(priv, 0x878,0xf, 0x0);
				PHY_SetBBReg(priv, 0x870,0x00000400, 0x1);
				PHY_SetBBReg(priv, 0x860,0x400, 0x0);
				PHY_SetBBReg(priv, 0xee8,0x10000000, 0x0);
			}
		}

#ifdef MP_TEST
		if (OPMODE & WIFI_MP_STATE)
		{
			PHY_SetBBReg(priv, 0x878,0xf, 0x0);
			PHY_SetBBReg(priv, 0x878,0x000f0000, 0x0);
			PHY_SetBBReg(priv, 0x870,0x00000400, 0x1);
			PHY_SetBBReg(priv, 0x860,0x400, 0x0);		//Path A PAPE low
			PHY_SetBBReg(priv, 0x870,0x04000000, 0x1);
			PHY_SetBBReg(priv, 0x864,0x400, 0x0);		//Path B PAPE low
			PHY_SetBBReg(priv, 0xee8,0x10000000, 0x0);
		}
#endif		
	}
#endif

#ifdef DW_FW_BY_MALLOC_BUF
	kfree(priv->pshare->fw_IMEM_buf);
	kfree(priv->pshare->fw_EMEM_buf);
	kfree(priv->pshare->fw_DMEM_buf);
#endif

/*
	{
		// for test, switch to 40Mhz mode
		unsigned int val_read;
		val_read = PHY_QueryRFReg(priv, 0, 18, bMask20Bits, 1);
		val_read &= ~(BIT(10)|BIT(11));
		PHY_SetRFReg(priv, 0, 18, bMask20Bits, val_read);
		val_read = PHY_QueryRFReg(priv, 1, 18, bMask20Bits, 1);
		val_read &= ~(BIT(10)|BIT(11));
		PHY_SetRFReg(priv, 1, 18, bMask20Bits, val_read);

		RTL_W8(0x800,RTL_R8(0x800)|0x1);
		RTL_W8(0x800,RTL_R8(0x900)|0x1);

		RTL_W8(0xc04 , 0x33);
		RTL_W8(0xd04, 0x33);

	}
*/

	/*---- Set CCK and OFDM Block "ON"----*/
//	PHY_SetBBReg(priv, rFPGA0_RFMOD, bCCKEn, 0x1);
//	PHY_SetBBReg(priv, rFPGA0_RFMOD, bOFDMEn, 0x1);


//	RTL_W8(BW_OPMODE, BIT(2)); // 40Mhz:0 20Mhz:1
//	RTL_W32(MACIDR,0x0);

	// under loopback mode
//	RTL_W32(MACIDR,0xffffffff);
/*
#ifdef CONFIG_NET_PCI
	if (IS_PCIBIOS_TYPE)
		pci_unmap_single(priv->pshare->pdev, priv->pshare->cmdbuf_phyaddr,
			(LoadPktSize), PCI_DMA_TODEVICE);
#endif
*/
#if	0
//	RTL_W32(0x230, 0x40000000);	//for test
////////////////////////////////////////////////////////////

	printk("init_hw: 1\n");

	RTL_W16(SIFS_OFDM, 0x1010);
	RTL_W8(SLOT_TIME, 0x09);

	RTL_W8(MSR, MSR_AP);

//	RTL_W8(MSR,MSR_INFRA);
	// for test, loopback
//	RTL_W8(MSR, MSR_NOLINK);
//	RTL_W8(LBKMD_SEL, BIT(0)| BIT(1) );
//	RTL_W16(LBDLY, 0xffff);

	//beacon related
	RTL_W16(BCN_INTERVAL, pmib->dot11StationConfigEntry.dot11BeaconPeriod);
	RTL_W16(ATIMWND, 2); //0
	RTL_W16(BCN_DRV_EARLY_INT, 10<<4); // 2
	RTL_W16(BCN_DMATIME, 256); // 0xf
	RTL_W16(SIFS_OFDM, 0x0e0e);
	RTL_W8(SLOT_TIME, 0x10);

//	CamResetAllEntry(priv);
	RTL_W16(SECR, 0x0000);

// By H.Y. advice
//	RTL_W16(_BCNTCFG_, 0x060a);
//	if (OPMODE & WIFI_AP_STATE)
//		RTL_W16(BCNTCFG, 0x000a);
//	else
// for debug
//	RTL_W16(_BCNTCFG_, 0x060a);
//	RTL_W16(BCNTCFG, 0x0204);


	init_beacon(priv);

	priv->pshare->InterruptMask = (IMR_ROK | IMR_VODOK | IMR_VIDOK | IMR_BEDOK | IMR_BKDOK |		\
	IMR_HCCADOK | IMR_MGNTDOK | IMR_COMDOK | IMR_HIGHDOK | 					\
	IMR_BDOK | /*IMR_RXCMDOK | IMR_TIMEOUT0 |*/ IMR_RDU | IMR_RXFOVW	|			\
	IMR_BcnInt/* | IMR_TXFOVW*/ /*| IMR_TBDOK | IMR_TBDER*/) ;// IMR_ROK | IMR_BcnInt | IMR_RDU | IMR_RXFOVW | IMR_RXCMDOK;

//	priv->pshare->InterruptMask = IMR_ROK| IMR_BDOK | IMR_BcnInt | IMR_MGNTDOK | IMR_TBDOK | IMR_RDU ;
//	priv->pshare->InterruptMask  = 0;
	priv->pshare->InterruptMaskExt = 0;
	RTL_W32(IMR, priv->pshare->InterruptMask);
	RTL_W32(IMR+4, priv->pshare->InterruptMaskExt);

//////////////////////////////////////////////////////////////
	printk("end of init hw\n");

	return 0;

#endif
// clear TPPoll
//	RTL_W16(TPPoll, 0x0);
// Should 8192SE do this initialize? I don't know yet, 080812. Joshua
	// PJ 1-5-2007 Reset PHY parameter counters
//	RTL_W32(0xD00, RTL_R32(0xD00)|BIT(27));
//	RTL_W32(0xD00, RTL_R32(0xD00)&(~(BIT(27))));

	// configure timing parameter
	RTL_W8(ACK_TIMEOUT, 0x30);
	RTL_W8(PIFS_TIME,0x13);
//	RTL_W16(LBDLY, 0x060F);
//	RTL_W16(SIFS_OFDM, 0x0e0e);
//	RTL_W8(SLOT_TIME, 0x10);
	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
		RTL_W16(SIFS_OFDM, 0x0a0a);
		RTL_W8(SLOT_TIME, 0x09);
	}
	else if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11A) {
		RTL_W16(SIFS_OFDM, 0x0a0a);
		RTL_W8(SLOT_TIME, 0x09);
	}
	else if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) {
		RTL_W16(SIFS_OFDM, 0x0a0a);
		RTL_W8(SLOT_TIME, 0x09);
	}
	else { // WIRELESS_11B
		RTL_W16(SIFS_OFDM, 0x0a0a);
		RTL_W8(SLOT_TIME, 0x14);
	}

	init_EDCA_para(priv, pmib->dot11BssType.net_work_type);


	// we don't have EEPROM yet, Mark this for FPGA Platform
//	RTL_W8(_9346CR_, CR9346_CFGRW);

//	92SE Windows driver does not set the PCIF as 0x77, seems HW bug?
	// Set Tx and Rx DMA burst size
//	RTL_W8(PCIF, 0x77);
	// Enable byte shift
//	RTL_W8(_PCIF_+2, 0x01);


	// Retry Limit
	if (priv->pmib->dot11OperationEntry.dot11LongRetryLimit)
		val16 = priv->pmib->dot11OperationEntry.dot11LongRetryLimit & 0xff;
	else {
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)
			val16 = 0x20;
		else
			val16 = 0x06;
	}
	if (priv->pmib->dot11OperationEntry.dot11ShortRetryLimit)
		val16 |= ((priv->pmib->dot11OperationEntry.dot11ShortRetryLimit & 0xff) << 8);
	else {
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)
			val16 |= (0x20 << 8);
		else
			val16 |= (0x06 << 8);
	}
	RTL_W16(RETRY_LIMIT, val16);

/*	This should be done later, but Windows Driver not done yet.
	// Response Rate Set
	// let ACK sent by highest of 24Mbps
	val32 = 0x1ff;
	if (pmib->dot11RFEntry.shortpreamble)
		val32 |= BIT(23);
	RTL_W32(_RRSR_, val32);
*/

	// Adaptive Rate Table for Basic Rate
	val32 = 0;
	for (i=0; i<32; i++) {
		if (AP_BSSRATE[i]) {
			if (AP_BSSRATE[i] & 0x80)
				val32 |= get_bit_value_from_ieee_value(AP_BSSRATE[i] & 0x7f);
		}
	}
	val32 |= (priv->pmib->dot11nConfigEntry.dot11nBasicMCS << 12);

	set_fw_reg(priv, (0xfd00002a | (1<<9 | ARFR_BMC)<<8), val32 ,1);

//	panic_printk("0x2c4 = bitmap = 0x%08x\n", (unsigned int)val32);
//	panic_printk("0x2c0 = cmd | macid | band = 0x%08x\n", 0xfd0000a2 | (1<<9 | (sta_band & 0xf))<<8);
//	panic_printk("Add id %d val %08x to ratr\n", 0, (unsigned int)val32);

/*	for (i = 0; i < 8; i++)
#ifdef RTL8192SE_ACUT
		RTL_W32(ARFR0+i*4, val32 & 0x1f0ff0f0);
#else
		RTL_W32(ARFR0+i*4, val32 & 0x1f0ff0f0);
#endif
*/

	//settting initial rate for control frame to 24M
	RTL_W8(INIRTSMCS_SEL, 8);

	//setting MAR
	RTL_W32(MAR, 0xffffffff);
	RTL_W32((MAR+4), 0xffffffff);

	//setting BSSID, not matter AH/AP/station
	memcpy((void *)&val32, (pmib->dot11OperationEntry.hwaddr), 4);
	memcpy((void *)&val16, (pmib->dot11OperationEntry.hwaddr + 4), 2);
	RTL_W32(BSSIDR, cpu_to_le32(val32));
	RTL_W16((BSSIDR + 4), cpu_to_le16(val16));
//	RTL_W32(BSSIDR, 0x814ce000);
//	RTL_W16((BSSIDR + 4), 0xee92);

	//setting TCR
	//TCR, use default value

	//setting RCR // set in MacConfigAfterFwDownload
//	RTL_W32(_RCR_, _APWRMGT_ | _AMF_ | _ADF_ | _AICV_ | _ACRC32_ | _AB_ | _AM_ | _APM_);
//	if (pmib->dot11OperationEntry.crc_log)
//		RTL_W32(_RCR_, RTL_R32(_RCR_) | _ACRC32_);

	//setting MSR
	if (opmode & WIFI_AP_STATE)
	{
		DEBUG_INFO("AP-mode enabled...\n");

		if (priv->pmib->dot11WdsInfo.wdsPure)
			RTL_W8(MSR, MSR_NOLINK);
		else
			RTL_W8(MSR, MSR_AP);
// Move init beacon after f/w download
#if 0
		if (priv->auto_channel == 0) {
			DEBUG_INFO("going to init beacon\n");
			init_beacon(priv);
		}
#endif
	}
#ifdef CLIENT_MODE
	else if (opmode & WIFI_STATION_STATE)
	{
		DEBUG_INFO("Station-mode enabled...\n");
		RTL_W8(MSR, MSR_INFRA);
	}
	else if (opmode & WIFI_ADHOC_STATE)
	{
		DEBUG_INFO("Adhoc-mode enabled...\n");
		RTL_W8(MSR, MSR_ADHOC);
	}
#endif
	else
	{
		printk("Operation mode error!\n");
		return 2;
	}

	CamResetAllEntry(priv);
	RTL_W16(SECR, 0x0000);
	if ((OPMODE & (WIFI_AP_STATE|WIFI_STATION_STATE|WIFI_ADHOC_STATE)) &&
		!priv->pmib->dot118021xAuthEntry.dot118021xAlgrthm &&
			(pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_ ||
				pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_)) {
		pmib->dot11GroupKeysTable.dot11Privacy = pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm;
		if (pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_)
			i = 5;
		else
			i = 13;
//#if defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
#ifdef USE_WEP_DEFAULT_KEY
		memcpy(pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKey.skey,
			&priv->pmib->dot11DefaultKeysTable.keytype[pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex].skey[0], i);
		pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKeyLen = i;
		pmib->dot11GroupKeysTable.keyid = pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex;
		pmib->dot11GroupKeysTable.keyInCam = 0;
		RTL_W16(SECR, RTL_R16(SECR)|BIT(5));	// no search multicast
#else
#ifdef CONFIG_RTL8196B_KLD
		memcpy(pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKey.skey,
							&priv->pmib->dot11DefaultKeysTable.keytype[pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex].skey[0], i);
#else
		memcpy(pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKey.skey,
							&priv->pmib->dot11DefaultKeysTable.keytype[0].skey[0], i);
#endif
		pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKeyLen = i;
		pmib->dot11GroupKeysTable.keyid = pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex;
		pmib->dot11GroupKeysTable.keyInCam = 0;
#endif
	}

// for debug
#if 0
// when hangup reset, re-init TKIP/AES key in ad-hoc mode
#ifdef CLIENT_MODE
	if ((OPMODE & WIFI_ADHOC_STATE) && pmib->dot11OperationEntry.keep_rsnie &&
		(pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _TKIP_PRIVACY_ ||
			pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _CCMP_PRIVACY_)) {
		DOT11_SET_KEY Set_Key;
		Set_Key.KeyType = DOT11_KeyType_Group;
		Set_Key.EncType = pmib->dot11GroupKeysTable.dot11Privacy;
		DOT11_Process_Set_Key(priv->dev, NULL, &Set_Key, pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKey.skey);
	}
	else
#endif
//-------------------------------------- david+2006-06-30
	// restore group key if it has been set before open, david
	if (pmib->dot11GroupKeysTable.keyInCam) {
		int retVal;
		retVal = CamAddOneEntry(priv, (unsigned char *)"\xff\xff\xff\xff\xff\xff",
					pmib->dot11GroupKeysTable.keyid,
					pmib->dot11GroupKeysTable.dot11Privacy<<2,
					0, pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKey.skey);
		if (retVal)
			priv->pshare->CamEntryOccupied++;
		else {
			DEBUG_ERR("Add group key failed!\n");
		}
	}
#endif
	//here add if legacy WEP
	// if 1x is enabled, do not set default key, david
//#if 0	// marked by victoryman, use pairwise key at present, 20070627
//#if defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
#ifdef USE_WEP_DEFAULT_KEY
#ifdef MBSSID
	if (!(OPMODE & WIFI_AP_STATE) || !priv->pmib->miscEntry.vap_enable)
#endif
	{
		if(!SWCRYPTO && !IEEE8021X_FUN &&
			(pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_ ||
			 pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_))
			init_DefaultKey_Enc(priv, NULL, 0);
	}
#endif


	//Setup Beacon Interval/interrupt interval, ATIM-WIND ATIM-Interrupt
//	RTL_W16(BCN_INTERVAL, pmib->dot11StationConfigEntry.dot11BeaconPeriod);
	//Setting BCNITV is done by firmware now
	set_fw_reg(priv, (0xF1000000 | (pmib->dot11StationConfigEntry.dot11BeaconPeriod << 8)), 0, 0);
	// Set max AMPDU aggregation time
	int max_aggre_time = 0xc0; // in 4us
	set_fw_reg(priv, (0xFD0000B1|((max_aggre_time << 8) & 0xff00)), 0 ,0);

//	RTL_W32(0x2A4, 0x00006300);
//	RTL_W32(0x2A0, 0xb026007C);
//	delay_ms(1);
//	while(RTL_R32(0x2A0) != 0){};
//	RTL_W16(ATIMWND, 0); //0

#if !defined(RTL8192SU)
	RTL_W16(BCN_DRV_EARLY_INT, (10<<4)); // 2
#ifdef WDS
	if (!((OPMODE & WIFI_AP_STATE) && priv->pmib->dot11WdsInfo.wdsEnabled && priv->pmib->dot11WdsInfo.wdsPure))
#endif
		RTL_W16(BCN_DRV_EARLY_INT, RTL_R16(BCN_DRV_EARLY_INT)|BIT(15)); // sw beacon

#ifdef MBSSID
	if (priv->pmib->miscEntry.vap_enable && RTL8190_NUM_VWLAN == 1 &&
					priv->pmib->dot11StationConfigEntry.dot11BeaconPeriod < 30)
		RTL_W16(BCN_DRV_EARLY_INT, (RTL_R16(BCN_DRV_EARLY_INT)&0xf00f) | ((6<<4)&0xff0));
#endif

	RTL_W16(BCN_DMATIME, 0x400); // 1ms
#else //RTl8192SU
#ifdef RTL8192SU_FWBCN
	//RTL_W16(BCN_DRV_EARLY_INT, 0x0000);
#else
	RTL_W16(BCN_DRV_EARLY_INT, 0x8000); //enable software transmit beacon frame
	RTL_W16(BCN_DMATIME, 0);
#endif	
#endif //!RTL8192SU

// for debug
#ifdef CLIENT_MODE
	if (OPMODE & WIFI_ADHOC_STATE)
		RTL_W8(BCN_ERR_THRESH, 100);
#endif
//--------------

// By H.Y. advice
//	RTL_W16(_BCNTCFG_, 0x060a);
	if (OPMODE & WIFI_AP_STATE)
		RTL_W16(BCNTCFG, 0x000a);
	else
// for debug
//	RTL_W16(_BCNTCFG_, 0x060a);
	RTL_W16(BCNTCFG, 0x0204);


	// Ack ISR, and then unmask IMR
#if 0
	RTL_W32(ISR, RTL_R32(ISR));
	RTL_W32(ISR+4, RTL_R16(ISR+4));
	RTL_W32(IMR, 0x0);
	RTL_W32(IMR+4, 0x0);
	priv->pshare->InterruptMask = _ROK_ | _BCNDMAINT_ | _RDU_ | _RXFOVW_ | _RXCMDOK_;
	priv->pshare->InterruptMask = (IMR_ROK | IMR_VODOK | IMR_VIDOK | IMR_BEDOK | IMR_BKDOK |		\
	IMR_HCCADOK | IMR_MGNTDOK | IMR_COMDOK | IMR_HIGHDOK | 					\
	IMR_BDOK | IMR_RXCMDOK | /*IMR_TIMEOUT0 |*/ IMR_RDU | IMR_RXFOVW	|			\
	IMR_BcnInt/* | IMR_TXFOVW*/ /*| IMR_TBDOK | IMR_TBDER*/);// IMR_ROK | IMR_BcnInt | IMR_RDU | IMR_RXFOVW | IMR_RXCMDOK;
#endif
	priv->pshare->InterruptMask = IMR_ROK | IMR_BcnInt |IMR_RDU | IMR_RXFOVW ;
#ifdef MP_TEST
	priv->pshare->InterruptMask	|= IMR_BEDOK;
#endif
	priv->pshare->InterruptMaskExt = 0;

	if (opmode & WIFI_AP_STATE)
		priv->pshare->InterruptMask |= IMR_BDOK;
#ifdef CLIENT_MODE
	else if (opmode & WIFI_ADHOC_STATE)
		priv->pshare->InterruptMaskExt |= (IMR_TBDER | IMR_TBDOK);
#endif

	if (priv->pmib->miscEntry.ack_timeout && (priv->pmib->miscEntry.ack_timeout < 0xff))
		RTL_W8(ACK_TIMEOUT, priv->pmib->miscEntry.ack_timeout);

	// FGPA does not have eeprom now
//	RTL_W8(_9346CR_, 0);
/*
	// ===========================================================================================
	// Download Firmware
	// allocate memory for tx cmd packet
	if((priv->pshare->txcmd_buf = (unsigned char *)kmalloc((LoadPktSize), GFP_ATOMIC)) == NULL) {
		printk("not enough memory for txcmd_buf\n");
		return -1;
	}

	priv->pshare->cmdbuf_phyaddr = get_physical_addr(priv, priv->pshare->txcmd_buf,
			LoadPktSize, PCI_DMA_TODEVICE);

	if(LoadFirmware(priv) == FALSE){
		printk("Load Firmware Fail!\n");
		return -1;
	}else {
		delay_ms(20);
		PRINT_INFO("Load firmware successful! \n");
	}

	MacConfigAfterFwDownload(priv);
*/

	kfree(priv->pshare->txcmd_buf);

#if !defined(LOOPBACK_MODE)
	if (opmode & WIFI_AP_STATE)
	{
		if (priv->auto_channel == 0) {
			DEBUG_INFO("going to init beacon\n");
			init_beacon(priv);
		}
	}
#endif	

	//enable interrupt
#if !defined(RTL8192SU) // usb interface don't need IMR setting.
	RTL_W32(IMR, priv->pshare->InterruptMask);
	RTL_W32(IMR+4, priv->pshare->InterruptMaskExt);
#endif	
//	RTL_W32(IMR, 0xffffffff);
//	RTL_W8(IMR+4, 0x3f);

	// ===========================================================================================


		// for test, loopback
//		RTL_W8(MSR, MSR_NOLINK);
//		RTL_W8(LBKMD_SEL, BIT(0)| BIT(1) | BIT(3));


#ifdef CHECK_HANGUP
	if (priv->reset_hangup)
		priv->pshare->CurrentChannelBW = priv->pshare->is_40m_bw;
	else
#endif
	{
		if (opmode & WIFI_AP_STATE)
			priv->pshare->CurrentChannelBW = priv->pshare->is_40m_bw;
		else
			priv->pshare->CurrentChannelBW = HT_CHANNEL_WIDTH_20;
	}

	if (get_rf_mimo_mode(priv) == MIMO_2T2R)
	{
#if !defined(RTL8192SU)
		if ((priv->pmib->dot11RFEntry.LOFDM_pwd_A == 0) ||
#else
		if ((priv->pmib->dot11RFEntry.LOFDM_pwd_A == 0) &&
#endif
			(priv->pmib->dot11RFEntry.LOFDM_pwd_B == 0)) {
			priv->pshare->use_default_para = 1;
			priv->pshare->legacyOFDM_pwrdiff_A = 0;
			priv->pshare->legacyOFDM_pwrdiff_B = 0;
		}
		else
		{
			priv->pshare->use_default_para = 0;

			if (priv->pmib->dot11RFEntry.LOFDM_pwd_A >= 32)
				priv->pshare->legacyOFDM_pwrdiff_A = (priv->pmib->dot11RFEntry.LOFDM_pwd_A - 32) & 0x0f;
			else {
				priv->pshare->legacyOFDM_pwrdiff_A = (32 - priv->pmib->dot11RFEntry.LOFDM_pwd_A) & 0x0f;
				priv->pshare->legacyOFDM_pwrdiff_A |= 0x80;
			}

			if (priv->pmib->dot11RFEntry.LOFDM_pwd_B >= 32)
				priv->pshare->legacyOFDM_pwrdiff_B = (priv->pmib->dot11RFEntry.LOFDM_pwd_B - 32) & 0x0f;
			else {
				priv->pshare->legacyOFDM_pwrdiff_B = (32 - priv->pmib->dot11RFEntry.LOFDM_pwd_B) & 0x0f;
				priv->pshare->legacyOFDM_pwrdiff_B |= 0x80;
			}
		}
	}
	else
	{
		if (priv->pmib->dot11RFEntry.LOFDM_pwd_A == 0) {
			priv->pshare->use_default_para = 1;
			priv->pshare->legacyOFDM_pwrdiff_A = 0;
			priv->pshare->legacyOFDM_pwrdiff_B = 0;
		}
		else
		{
			priv->pshare->use_default_para = 0;

			if (priv->pmib->dot11RFEntry.LOFDM_pwd_A >= 32)
				priv->pshare->legacyOFDM_pwrdiff_A = (priv->pmib->dot11RFEntry.LOFDM_pwd_A - 32) & 0x0f;
			else {
				priv->pshare->legacyOFDM_pwrdiff_A = (32 - priv->pmib->dot11RFEntry.LOFDM_pwd_A) & 0x0f;
				priv->pshare->legacyOFDM_pwrdiff_A |= 0x80;
			}

			if (priv->pmib->dot11RFEntry.LOFDM_pwd_B != 0) {
				if (priv->pmib->dot11RFEntry.LOFDM_pwd_B >= 32)
					priv->pshare->legacyOFDM_pwrdiff_B = (priv->pmib->dot11RFEntry.LOFDM_pwd_B - 32) & 0x0f;
				else {
					priv->pshare->legacyOFDM_pwrdiff_B = (32 - priv->pmib->dot11RFEntry.LOFDM_pwd_B) & 0x0f;
					priv->pshare->legacyOFDM_pwrdiff_B |= 0x80;
				}
			}
			else
				priv->pshare->legacyOFDM_pwrdiff_B = priv->pshare->legacyOFDM_pwrdiff_A;
		}
	}

	// get default Tx AGC offset
#if !defined(RTL8192SU)
	*(unsigned int *)(&priv->pshare->phw->MCSTxAgcOffset[0])  = RTL_R32(rTxAGC_Mcs03_Mcs00);
	*(unsigned int *)(&priv->pshare->phw->MCSTxAgcOffset[4])  = RTL_R32(rTxAGC_Mcs07_Mcs04);
	*(unsigned int *)(&priv->pshare->phw->MCSTxAgcOffset[8])  = RTL_R32(rTxAGC_Mcs11_Mcs08);
	*(unsigned int *)(&priv->pshare->phw->MCSTxAgcOffset[12]) = RTL_R32(rTxAGC_Mcs15_Mcs12);
	*(unsigned int *)(&priv->pshare->phw->OFDMTxAgcOffset[0]) = RTL_R32(rTxAGC_Rate18_06);
	*(unsigned int *)(&priv->pshare->phw->OFDMTxAgcOffset[4]) = RTL_R32(rTxAGC_Rate54_24);
#else
	*(unsigned int *)(&priv->pshare->phw->MCSTxAgcOffset[0])  = PHY_QueryBBReg(priv, rTxAGC_Mcs03_Mcs00, bMaskDWord);
	*(unsigned int *)(&priv->pshare->phw->MCSTxAgcOffset[4])  = PHY_QueryBBReg(priv, rTxAGC_Mcs07_Mcs04, bMaskDWord);
	if (get_rf_mimo_mode(priv) == MIMO_2T2R)
	{
		*(unsigned int *)(&priv->pshare->phw->MCSTxAgcOffset[8])  = PHY_QueryBBReg(priv, rTxAGC_Mcs11_Mcs08, bMaskDWord);
		*(unsigned int *)(&priv->pshare->phw->MCSTxAgcOffset[12]) = PHY_QueryBBReg(priv, rTxAGC_Mcs15_Mcs12, bMaskDWord);
	}
	*(unsigned int *)(&priv->pshare->phw->OFDMTxAgcOffset[0]) = PHY_QueryBBReg(priv, rTxAGC_Rate18_06, bMaskDWord);
	*(unsigned int *)(&priv->pshare->phw->OFDMTxAgcOffset[4]) = PHY_QueryBBReg(priv, rTxAGC_Rate54_24, bMaskDWord);
#endif //RTL8192SU

#ifdef ADD_TX_POWER_BY_CMD
	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->OFDMTxAgcOffset[0], priv->pshare->rf_ft_var.txPowerPlus_ofdm_18);
	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->OFDMTxAgcOffset[1], priv->pshare->rf_ft_var.txPowerPlus_ofdm_12);
	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->OFDMTxAgcOffset[2], priv->pshare->rf_ft_var.txPowerPlus_ofdm_9);
	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->OFDMTxAgcOffset[3], priv->pshare->rf_ft_var.txPowerPlus_ofdm_6);

	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->OFDMTxAgcOffset[4], priv->pshare->rf_ft_var.txPowerPlus_ofdm_54);
	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->OFDMTxAgcOffset[5], priv->pshare->rf_ft_var.txPowerPlus_ofdm_48);
	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->OFDMTxAgcOffset[6], priv->pshare->rf_ft_var.txPowerPlus_ofdm_36);
	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->OFDMTxAgcOffset[7], priv->pshare->rf_ft_var.txPowerPlus_ofdm_24);

	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->MCSTxAgcOffset[0], priv->pshare->rf_ft_var.txPowerPlus_mcs_3);
	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->MCSTxAgcOffset[1], priv->pshare->rf_ft_var.txPowerPlus_mcs_2);
	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->MCSTxAgcOffset[2], priv->pshare->rf_ft_var.txPowerPlus_mcs_1);
	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->MCSTxAgcOffset[3], priv->pshare->rf_ft_var.txPowerPlus_mcs_0);

	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->MCSTxAgcOffset[4], priv->pshare->rf_ft_var.txPowerPlus_mcs_7);
	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->MCSTxAgcOffset[5], priv->pshare->rf_ft_var.txPowerPlus_mcs_6);
	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->MCSTxAgcOffset[6], priv->pshare->rf_ft_var.txPowerPlus_mcs_5);
	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->MCSTxAgcOffset[7], priv->pshare->rf_ft_var.txPowerPlus_mcs_4);

	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->MCSTxAgcOffset[8], priv->pshare->rf_ft_var.txPowerPlus_mcs_11);
	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->MCSTxAgcOffset[9], priv->pshare->rf_ft_var.txPowerPlus_mcs_10);
	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->MCSTxAgcOffset[10], priv->pshare->rf_ft_var.txPowerPlus_mcs_9);
	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->MCSTxAgcOffset[11], priv->pshare->rf_ft_var.txPowerPlus_mcs_8);

	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->MCSTxAgcOffset[12], priv->pshare->rf_ft_var.txPowerPlus_mcs_15);
	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->MCSTxAgcOffset[13], priv->pshare->rf_ft_var.txPowerPlus_mcs_14);
	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->MCSTxAgcOffset[14], priv->pshare->rf_ft_var.txPowerPlus_mcs_13);
	ASSIGN_TX_POWER_OFFSET(priv->pshare->phw->MCSTxAgcOffset[15], priv->pshare->rf_ft_var.txPowerPlus_mcs_12);
#endif // ADD_TX_POWER_BY_CMD

	if ((priv->pmib->dot11RFEntry.ther < 0x07) || (priv->pmib->dot11RFEntry.ther > 0x1d)) {
		DEBUG_ERR("TPT: unreasonable target ther %d, disable tpt\n", priv->pmib->dot11RFEntry.ther);
		priv->pmib->dot11RFEntry.ther = 0;
	}

/*
	if (opmode & WIFI_AP_STATE)
	{
		if (priv->auto_channel == 0) {
			DEBUG_INFO("going to init beacon\n");
			init_beacon(priv);
		}
	}
*/
	/*---- Set CCK and OFDM Block "ON"----*/
	PHY_SetBBReg(priv, rFPGA0_RFMOD, bCCKEn, 0x1);
	PHY_SetBBReg(priv, rFPGA0_RFMOD, bOFDMEn, 0x1);
	delay_ms(2);

#if	defined(RTL8192)
	//Turn on the RF8256, joshua
	PHY_SetBBReg(priv, rFPGA0_XA_RFInterfaceOE, 1<<4, 0x1);		// 0x860[4]
	PHY_SetBBReg(priv, rFPGA0_AnalogParameter4, 0x300, 0x3);		// 0x88c[4]
	PHY_SetBBReg(priv, rFPGA0_AnalogParameter1, 0x60, 0x3); 		// 0x880[6:5]
	PHY_SetBBReg(priv, rOFDM0_TRxPathEnable, 0xf, 0x3);			// 0xc04[3:0]
	PHY_SetBBReg(priv, rOFDM1_TRxPathEnable, 0xf, 0x3);			// 0xd04[3:0]
	PHY_SetBBReg(priv, rFPGA0_AnalogParameter2, 0x7000, 0x3);

	//Enable LED
	RTL_W32(0x87, 0x0);
#endif

	SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);
	SwChnl(priv, priv->pmib->dot11RFEntry.dot11channel, priv->pshare->offset_2nd_chan);
#ifdef RTL8192SU_USE_FW_TPT
	if (priv->pmib->dot11RFEntry.ther!=0)
		set_fw_reg(priv, 0xfd000017 | (((unsigned char)priv->pmib->dot11RFEntry.ther) << 8), 0, 0);
#endif
	if(priv->pshare->rf_ft_var.ofdm_1ss_oneAnt == 1){// use one PATH for ofdm and 1SS
		Switch_1SS_Antenna(priv, 2);
		Switch_OFDM_Antenna(priv, 2);
	}

	// set Max Short GI Rate
	if (priv->pmib->dot11StationConfigEntry.autoRate == 1) {
		// set by fw, 20090707
#if 0
		if (get_rf_mimo_mode(priv) == MIMO_2T2R)
			RTL_W16(SG_RATE, 0xffff);
		else // 1T2R, 1T1R?
			RTL_W16(SG_RATE, 0x7777);
#endif
	}
	else { // fixed rate
		fixed_rate = (priv->pmib->dot11StationConfigEntry.fixedTxRate >> 12) & 0xffff;
		calc_rate = 0;
		for (ii = 0; ii < 16; ii++) {
			if ((fixed_rate & 0x1) == 0)
				calc_rate += 1;
			else
				break;
			fixed_rate >>= 1;
		}
		RTL_W16(SG_RATE, (calc_rate)|(calc_rate << 4) | (calc_rate << 8) | (calc_rate<<12));
	}

	// disable tx brust for wifi
	if (priv->pmib->dot11OperationEntry.wifi_specific)
		set_fw_reg(priv, 0xfd0001b0, 0, 0);

	// enable RIFS function for wifi
	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
		priv->pmib->dot11OperationEntry.wifi_specific)
		PHY_SetBBReg(priv, 0x818, 0xffff, 0x03c5);


	// IQCal
	set_fw_reg(priv, 0xf0000020, 0, 0);
#ifdef _DEBUG_RTL8190_
	{
		delay_ms(150);
		unsigned int reply = RTL_R32(0x2c4);
		reply &= 0xffff;
		if (reply == 0xdddd) {
			DEBUG_INFO("IQK: Cal success!\n");
		}
		else if (reply == 0xffff) {
			DEBUG_WARN("IQK: Cal fail!\n");
		}
		else {
			DEBUG_ERR("IQK: returned unknown 0x%04x\n", reply);
		}
	}
#endif

	delay_ms(100);

	//RTL_W32(0x100, RTL_R32(0x100) | BIT(14)); //for 8190 fw debug

	DBFEXIT;

	return 0;

}


// finetune PCIe analog PHY characteristic, by Pei-Si Wu, 20090227
void rtl8192se_ePhyInit(struct rtl8190_priv *priv)
{
#if !defined(RTL8192SU)
	unsigned int ioaddr = priv->pshare->ioaddr;
#endif	

	RTL_W16(MDIO_DATA, 0x1000);
	RTL_W8(MDIO_CTRL, 0x20);
	delay_ms(1);
	RTL_W8(MDIO_CTRL, 0);
	delay_ms(10);

	RTL_W16(MDIO_DATA, 0xc49a);
	RTL_W8(MDIO_CTRL, 0x23);
	delay_ms(1);
	RTL_W8(MDIO_CTRL, 0);
	delay_ms(10);

	RTL_W16(MDIO_DATA, 0xa5bc);
	RTL_W8(MDIO_CTRL, 0x26);
	delay_ms(1);
	RTL_W8(MDIO_CTRL, 0);
	delay_ms(10);

	RTL_W16(MDIO_DATA, 0x1a80);
	RTL_W8(MDIO_CTRL, 0x27);
	delay_ms(1);
	RTL_W8(MDIO_CTRL, 0);
	delay_ms(10);

	RTL_W16(MDIO_DATA, 0xff80);
	RTL_W8(MDIO_CTRL, 0x39);
	delay_ms(1);
	RTL_W8(MDIO_CTRL, 0);
	delay_ms(10);

	RTL_W16(MDIO_DATA, 0xa0eb);
	RTL_W8(MDIO_CTRL, 0x3e);
	delay_ms(1);
	RTL_W8(MDIO_CTRL, 0);
	delay_ms(10);

	// debug print
#if 0
	RTL_W8(MDIO_CTRL, 0x40);
	delay_ms(1);
	printk("ePhyReg0x%02x=0x%04x\n", 0x00, RTL_R16(MDIO_DATA+2));
	RTL_W8(MDIO_CTRL, 0);
	delay_ms(10);

	RTL_W8(MDIO_CTRL, 0x43);
	delay_ms(1);
	printk("ePhyReg0x%02x=0x%04x\n", 0x03, RTL_R16(MDIO_DATA+2));
	RTL_W8(MDIO_CTRL, 0);
	delay_ms(10);

	RTL_W8(MDIO_CTRL, 0x46);
	delay_ms(1);
	printk("ePhyReg0x%02x=0x%04x\n", 0x06, RTL_R16(MDIO_DATA+2));
	RTL_W8(MDIO_CTRL, 0);
	delay_ms(10);

	RTL_W8(MDIO_CTRL, 0x47);
	delay_ms(1);
	printk("ePhyReg0x%02x=0x%04x\n", 0x07, RTL_R16(MDIO_DATA+2));
	RTL_W8(MDIO_CTRL, 0);
	delay_ms(10);

	RTL_W8(MDIO_CTRL, 0x59);
	delay_ms(1);
	printk("ePhyReg0x%02x=0x%04x\n", 0x19, RTL_R16(MDIO_DATA+2));
	RTL_W8(MDIO_CTRL, 0);
	delay_ms(10);

	RTL_W8(MDIO_CTRL, 0x5e);
	delay_ms(1);
	printk("ePhyReg0x%02x=0x%04x\n", 0x1e, RTL_R16(MDIO_DATA+2));
	RTL_W8(MDIO_CTRL, 0);
	delay_ms(10);
#endif
}


#ifdef HW_QUICK_INIT
void rtl819x_start_hw_trx(struct rtl8190_priv *priv)
{
	static struct wifi_mib *pmib;
	static unsigned int opmode;
	static unsigned long ioaddr;
	static unsigned long val32;
	static unsigned short val16;
	static int i;
	static unsigned short fixed_rate;
	static unsigned int ii;
	static unsigned char calc_rate;

	pmib = GET_MIB(priv);
	opmode = priv->pmib->dot11OperationEntry.opmode;
	ioaddr = priv->pshare->ioaddr;

#ifdef WDS
	if (!((OPMODE & WIFI_AP_STATE) && priv->pmib->dot11WdsInfo.wdsEnabled && priv->pmib->dot11WdsInfo.wdsPure))
#endif
		RTL_W16(BCN_DRV_EARLY_INT, RTL_R16(BCN_DRV_EARLY_INT)|BIT(15)); // sw beacon

	// support up to MCS7 for 1T1R, modify rx capability here
	if (get_rf_mimo_mode(priv) == MIMO_1T1R)
		pmib->dot11nConfigEntry.dot11nSupportedMCS &= 0x00ff;

	// Set NAV protection length
	// CF-END Threshold
	if (priv->pmib->dot11OperationEntry.wifi_specific) {
		RTL_W16(NAV_PROT_LEN, 0x80);
		RTL_W8(CFEND_TH, 0x2);
	}
	else {
		RTL_W16(NAV_PROT_LEN, 0x01C0);
		RTL_W8(CFEND_TH, 0xFF);
	}

#ifdef DRVMAC_LB
	if (priv->pmib->miscEntry.drvmac_lb) {
		RTL_W8(MSR, MSR_NOLINK);
		drvmac_loopback(priv);
	}
#endif

	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
		RTL_W16(SIFS_OFDM, 0x0a0a);
		RTL_W8(SLOT_TIME, 0x09);
	}
	else if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11A) {
		RTL_W16(SIFS_OFDM, 0x0a0a);
		RTL_W8(SLOT_TIME, 0x09);
	}
	else if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) {
		RTL_W16(SIFS_OFDM, 0x0a0a);
		RTL_W8(SLOT_TIME, 0x09);
	}
	else { // WIRELESS_11B
		RTL_W16(SIFS_OFDM, 0x0a0a);
		RTL_W8(SLOT_TIME, 0x14);
	}

	init_EDCA_para(priv, pmib->dot11BssType.net_work_type);

	// Retry Limit
	if (priv->pmib->dot11OperationEntry.dot11LongRetryLimit)
		val16 = priv->pmib->dot11OperationEntry.dot11LongRetryLimit & 0xff;
	else {
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)
			val16 = 0x20;
		else
			val16 = 0x06;
	}
	if (priv->pmib->dot11OperationEntry.dot11ShortRetryLimit)
		val16 |= ((priv->pmib->dot11OperationEntry.dot11ShortRetryLimit & 0xff) << 8);
	else {
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)
			val16 |= (0x20 << 8);
		else
			val16 |= (0x06 << 8);
	}
	RTL_W16(RETRY_LIMIT, val16);

	// Adaptive Rate Table for Basic Rate
	val32 = 0;
	for (i=0; i<32; i++) {
		if (AP_BSSRATE[i]) {
			if (AP_BSSRATE[i] & 0x80)
				val32 |= get_bit_value_from_ieee_value(AP_BSSRATE[i] & 0x7f);
		}
	}
	val32 |= (priv->pmib->dot11nConfigEntry.dot11nBasicMCS << 12);

	set_fw_reg(priv, (0xfd00002a | (1<<9 | ARFR_BMC)<<8), val32 ,1);

	//setting BSSID, not matter AH/AP/station
	memcpy((void *)&val32, (pmib->dot11OperationEntry.hwaddr), 4);
	memcpy((void *)&val16, (pmib->dot11OperationEntry.hwaddr + 4), 2);
	RTL_W32(BSSIDR, cpu_to_le32(val32));
	RTL_W16((BSSIDR + 4), cpu_to_le16(val16));

	//setting MSR
	if (opmode & WIFI_AP_STATE)
	{
		DEBUG_INFO("AP-mode enabled...\n");

		if (priv->pmib->dot11WdsInfo.wdsPure)
			RTL_W8(MSR, MSR_NOLINK);
		else
			RTL_W8(MSR, MSR_AP);
	}
#ifdef CLIENT_MODE
	else if (opmode & WIFI_STATION_STATE)
	{
		DEBUG_INFO("Station-mode enabled...\n");
		RTL_W8(MSR, MSR_INFRA);
	}
	else if (opmode & WIFI_ADHOC_STATE)
	{
		DEBUG_INFO("Adhoc-mode enabled...\n");
		RTL_W8(MSR, MSR_ADHOC);
	}
#endif
	else
	{
		printk("Operation mode error!\n");
		return;
	}

	CamResetAllEntry(priv);
	RTL_W16(SECR, 0x0000);
	if ((OPMODE & (WIFI_AP_STATE|WIFI_STATION_STATE|WIFI_ADHOC_STATE)) &&
		!priv->pmib->dot118021xAuthEntry.dot118021xAlgrthm &&
			(pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_ ||
				pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_)) {
		pmib->dot11GroupKeysTable.dot11Privacy = pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm;
		if (pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_)
			i = 5;
		else
			i = 13;
//#if defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
#ifdef USE_WEP_DEFAULT_KEY
		memcpy(pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKey.skey,
			&priv->pmib->dot11DefaultKeysTable.keytype[pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex].skey[0], i);
		pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKeyLen = i;
		pmib->dot11GroupKeysTable.keyid = pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex;
		pmib->dot11GroupKeysTable.keyInCam = 0;
		RTL_W16(SECR, RTL_R16(SECR)|BIT(5));	// no search multicast
#else
#ifdef CONFIG_RTL8196B_KLD
		memcpy(pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKey.skey,
							&priv->pmib->dot11DefaultKeysTable.keytype[pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex].skey[0], i);
#else
		memcpy(pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKey.skey,
							&priv->pmib->dot11DefaultKeysTable.keytype[0].skey[0], i);
#endif
		pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKeyLen = i;
		pmib->dot11GroupKeysTable.keyid = pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex;
		pmib->dot11GroupKeysTable.keyInCam = 0;
#endif
	}

//#if defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
#ifdef USE_WEP_DEFAULT_KEY
#ifdef MBSSID
	if (!(OPMODE & WIFI_AP_STATE) || !priv->pmib->miscEntry.vap_enable)
#endif
	{
		if(!SWCRYPTO && !IEEE8021X_FUN &&
			(pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_ ||
			 pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_))
			init_DefaultKey_Enc(priv, NULL, 0);
	}
#endif

	//Setting BCNITV is done by firmware now
	set_fw_reg(priv, (0xF1000000 | (pmib->dot11StationConfigEntry.dot11BeaconPeriod << 8)), 0, 0);

#ifdef WDS
	if (!((OPMODE & WIFI_AP_STATE) && priv->pmib->dot11WdsInfo.wdsEnabled && priv->pmib->dot11WdsInfo.wdsPure))
#endif
		RTL_W16(BCN_DRV_EARLY_INT, RTL_R16(BCN_DRV_EARLY_INT)|BIT(15)); // sw beacon

#ifdef CLIENT_MODE
	if (OPMODE & WIFI_ADHOC_STATE)
		RTL_W8(BCN_ERR_THRESH, 100);
#endif

	if (OPMODE & WIFI_AP_STATE)
		RTL_W16(BCNTCFG, 0x000a);
	else
		RTL_W16(BCNTCFG, 0x0204);

	priv->pshare->InterruptMask = IMR_ROK | IMR_BcnInt |IMR_RDU | IMR_RXFOVW ;
#ifdef MP_TEST
	priv->pshare->InterruptMask	|= IMR_BEDOK;
#endif
	priv->pshare->InterruptMaskExt = 0;

	if (opmode & WIFI_AP_STATE)
		priv->pshare->InterruptMask |= IMR_BDOK;
#ifdef CLIENT_MODE
	else if (opmode & WIFI_ADHOC_STATE)
		priv->pshare->InterruptMaskExt |= (IMR_TBDER | IMR_TBDOK);
#endif

	if (priv->pmib->miscEntry.ack_timeout && (priv->pmib->miscEntry.ack_timeout < 0xff))
		RTL_W8(ACK_TIMEOUT, priv->pmib->miscEntry.ack_timeout);

	if (opmode & WIFI_AP_STATE)
	{
		if (priv->auto_channel == 0) {
			DEBUG_INFO("going to init beacon\n");
			init_beacon(priv);
		}
	}

	//enable interrupt
	RTL_W32(IMR, priv->pshare->InterruptMask);
	RTL_W32(IMR+4, priv->pshare->InterruptMaskExt);

	{
		if (opmode & WIFI_AP_STATE)
			priv->pshare->CurrentChannelBW = priv->pshare->is_40m_bw;
		else
			priv->pshare->CurrentChannelBW = HT_CHANNEL_WIDTH_20;
	}

	if (get_rf_mimo_mode(priv) == MIMO_2T2R)
	{
		if ((priv->pmib->dot11RFEntry.LOFDM_pwd_A == 0) ||
			(priv->pmib->dot11RFEntry.LOFDM_pwd_B == 0)) {
			priv->pshare->use_default_para = 1;
			priv->pshare->legacyOFDM_pwrdiff_A = 0;
			priv->pshare->legacyOFDM_pwrdiff_B = 0;
		}
		else
		{
			priv->pshare->use_default_para = 0;

			if (priv->pmib->dot11RFEntry.LOFDM_pwd_A >= 32)
				priv->pshare->legacyOFDM_pwrdiff_A = (priv->pmib->dot11RFEntry.LOFDM_pwd_A - 32) & 0x0f;
			else {
				priv->pshare->legacyOFDM_pwrdiff_A = (32 - priv->pmib->dot11RFEntry.LOFDM_pwd_A) & 0x0f;
				priv->pshare->legacyOFDM_pwrdiff_A |= 0x80;
			}

			if (priv->pmib->dot11RFEntry.LOFDM_pwd_B >= 32)
				priv->pshare->legacyOFDM_pwrdiff_B = (priv->pmib->dot11RFEntry.LOFDM_pwd_B - 32) & 0x0f;
			else {
				priv->pshare->legacyOFDM_pwrdiff_B = (32 - priv->pmib->dot11RFEntry.LOFDM_pwd_B) & 0x0f;
				priv->pshare->legacyOFDM_pwrdiff_B |= 0x80;
			}
		}
	}
	else
	{
		if (priv->pmib->dot11RFEntry.LOFDM_pwd_A == 0) {
			priv->pshare->use_default_para = 1;
			priv->pshare->legacyOFDM_pwrdiff_A = 0;
			priv->pshare->legacyOFDM_pwrdiff_B = 0;
		}
		else
		{
			priv->pshare->use_default_para = 0;

			if (priv->pmib->dot11RFEntry.LOFDM_pwd_A >= 32)
				priv->pshare->legacyOFDM_pwrdiff_A = (priv->pmib->dot11RFEntry.LOFDM_pwd_A - 32) & 0x0f;
			else {
				priv->pshare->legacyOFDM_pwrdiff_A = (32 - priv->pmib->dot11RFEntry.LOFDM_pwd_A) & 0x0f;
				priv->pshare->legacyOFDM_pwrdiff_A |= 0x80;
			}

			if (priv->pmib->dot11RFEntry.LOFDM_pwd_B != 0) {
				if (priv->pmib->dot11RFEntry.LOFDM_pwd_B >= 32)
					priv->pshare->legacyOFDM_pwrdiff_B = (priv->pmib->dot11RFEntry.LOFDM_pwd_B - 32) & 0x0f;
				else {
					priv->pshare->legacyOFDM_pwrdiff_B = (32 - priv->pmib->dot11RFEntry.LOFDM_pwd_B) & 0x0f;
					priv->pshare->legacyOFDM_pwrdiff_B |= 0x80;
				}
			}
			else
				priv->pshare->legacyOFDM_pwrdiff_B = priv->pshare->legacyOFDM_pwrdiff_A;
		}
	}

	// get default Tx AGC offset
	*(unsigned int *)(&priv->pshare->phw->MCSTxAgcOffset[0])  = RTL_R32(rTxAGC_Mcs03_Mcs00);
	*(unsigned int *)(&priv->pshare->phw->MCSTxAgcOffset[4])  = RTL_R32(rTxAGC_Mcs07_Mcs04);
	*(unsigned int *)(&priv->pshare->phw->MCSTxAgcOffset[8])  = RTL_R32(rTxAGC_Mcs11_Mcs08);
	*(unsigned int *)(&priv->pshare->phw->MCSTxAgcOffset[12]) = RTL_R32(rTxAGC_Mcs15_Mcs12);
	*(unsigned int *)(&priv->pshare->phw->OFDMTxAgcOffset[0]) = RTL_R32(rTxAGC_Rate18_06);
	*(unsigned int *)(&priv->pshare->phw->OFDMTxAgcOffset[4]) = RTL_R32(rTxAGC_Rate54_24);

	SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);
	SwChnl(priv, priv->pmib->dot11RFEntry.dot11channel, priv->pshare->offset_2nd_chan);

	// set Max Short GI Rate
	if (priv->pmib->dot11StationConfigEntry.autoRate == 1) {
		// set by fw, 20090707
#if 0
		if (get_rf_mimo_mode(priv) == MIMO_2T2R)
			RTL_W16(SG_RATE, 0xffff);
		else // 1T2R, 1T1R?
			RTL_W16(SG_RATE, 0x7777);
#endif
	}
	else { // fixed rate
		fixed_rate = (priv->pmib->dot11StationConfigEntry.fixedTxRate >> 12) & 0xffff;
		calc_rate = 0;
		for (ii = 0; ii < 16; ii++) {
			if ((fixed_rate & 0x1) == 0)
				calc_rate += 1;
			else
				break;
			fixed_rate >>= 1;
		}
		RTL_W16(SG_RATE, (calc_rate)|(calc_rate << 4) | (calc_rate << 8) | (calc_rate<<12));
	}

	// disable tx brust for wifi
	if (priv->pmib->dot11OperationEntry.wifi_specific)
		set_fw_reg(priv, 0xfd0001b0, 0, 0);

	delay_ms(100);
}


void rtl819x_stop_hw_trx(struct rtl8190_priv *priv)
{
	unsigned long ioaddr = priv->pshare->ioaddr;

	RTL_W8(MSR, MSR_NOLINK);
}
#endif // HW_QUICK_INIT
#endif


#if defined(RTL8190) || defined(RTL8192E)
int phy_BB8190_Config_ParaFile(struct rtl8190_priv *priv)
{
	unsigned long ioaddr = priv->pshare->ioaddr;
	unsigned char u1RegValue;
	unsigned int  u4RegValue;
	int rtStatus;

	phy_InitBBRFRegisterDefinition(priv);

	/*----Set BB Global Reset----*/
	u1RegValue = RTL_R8(_BBGLBRESET_);
	RTL_W8(_BBGLBRESET_, (u1RegValue|BIT(0)));

	/*----Set BB reset Active----*/
	u4RegValue = RTL_R32(_CPURST_);
	RTL_W32(_CPURST_, (u4RegValue&(~CPURST_BBRst)));

	/*----Check MIMO TR hw setting----*/
	check_MIMO_TR_status(priv);

	/*---- Set CCK and OFDM Block "OFF"----*/
	PHY_SetBBReg(priv, rFPGA0_RFMOD, bCCKEn|bOFDMEn, 0x0);

	/*----BB Register Initilazation----*/
	if (get_rf_mimo_mode(priv) == MIMO_1T2R)
		rtStatus = PHY_ConfigBBWithParaFile(priv, PHYREG_1T2R);
	else if (get_rf_mimo_mode(priv) == MIMO_1T1R)
		rtStatus = PHY_ConfigBBWithParaFile(priv, PHYREG_1T1R);
	else if (get_rf_mimo_mode(priv) == MIMO_2T2R)
		rtStatus = PHY_ConfigBBWithParaFile(priv, PHYREG_2T2R);
	else
		rtStatus = PHY_ConfigBBWithParaFile(priv, PHYREG_2T4R);

	if (rtStatus) {
		printk("phy_BB8190_Config_ParaFile(): Write BB Reg Fail!!\n");
		return rtStatus;
	}

	/*----Set BB reset de-Active----*/
	u4RegValue = RTL_R32(_CPURST_);
	RTL_W32(_CPURST_, u4RegValue|CPURST_BBRst);

	/*----BB AGC table Initialization----*/
	rtStatus = PHY_ConfigBBWithParaFile(priv, AGCTAB);
	if (rtStatus) {
		printk("phy_BB8190_Config_ParaFile(): Write BB AGC Table Fail!!\n");
		return rtStatus;
	}

#ifdef RTL8192E
	// if the cut version is bigger than BD, we are BE now
	// FIXME should detect automatically
	u4RegValue = 0x0;
	PHY_SetBBReg(priv, rFPGA0_TxGainStage,(bXBTxAGC|bXCTxAGC|bXDTxAGC), u4RegValue);

	// set Crystal Cap
	u4RegValue = priv->EE_CrystalCap;
	PHY_SetBBReg(priv, rFPGA0_AnalogParameter1, bXtalCap92x, u4RegValue);
#endif

	DEBUG_INFO("PHY-BB Initialization Success\n");
	return 0;
}


int phy_RF8256_Config_ParaFile(struct rtl8190_priv *priv)
{
	int rtStatus=0;
	RF90_RADIO_PATH_E eRFPath;
	BB_REGISTER_DEFINITION_T *pPhyReg;
	unsigned int  RF0_Final_Value;
#ifdef RTL8192E
	unsigned int  u4RegValue = 0;
#endif
	unsigned char RetryTimes;

#ifdef RTL8190
	priv->pshare->phw->NumTotalRFPath = 4;
#elif defined(RTL8192E)
	priv->pshare->phw->NumTotalRFPath = 2; // 8192e only 1T2R, joshua
#endif

	for (eRFPath = RF90_PATH_A; eRFPath<priv->pshare->phw->NumTotalRFPath; eRFPath++)
	{
		if (!PHYCheckIsLegalRfPath8190Pci(priv, eRFPath))
			continue;
/*
  		if (get_rf_mimo_mode(priv) == MIMO_1T2R) {
			if ((eRFPath == RF90_PATH_A) || (eRFPath == RF90_PATH_B))
				continue;
  		}
		else if (get_rf_mimo_mode(priv) == MIMO_2T2R) {
			if ((eRFPath == RF90_PATH_B) || (eRFPath == RF90_PATH_D))
				continue;
		}
*/

		pPhyReg = &priv->pshare->phw->PHYRegDef[eRFPath];

#ifdef RTL8190
		// Joseph test for shorten RF config
		priv->pshare->phw->RfReg0Value[eRFPath] = PHY_QueryRFReg(priv, eRFPath, rGlobalCtrl, bMaskDWord, 1);
#elif defined(RTL8192E)
		/*----Store original RFENV control type----*/
		switch(eRFPath) {
			case RF90_PATH_A:
				u4RegValue = PHY_QueryBBReg(priv, pPhyReg->rfintfs, bRFSI_RFENV);
				break;
			case RF90_PATH_B:
				u4RegValue = PHY_QueryBBReg(priv, pPhyReg->rfintfs, bRFSI_RFENV<<16);
				break;
			default:
				break;
		}

		// Joseph test for shorten RF config
		priv->pshare->phw->RfReg0Value[eRFPath] = PHY_QueryRFReg(priv, eRFPath, rGlobalCtrl, bMaskDWord,0);
		/*----Set RF_ENV enable----*/
		PHY_SetBBReg(priv, pPhyReg->rfintfe, bRFSI_RFENV<<16, 0x1);

		/*----Set RF_ENV output high----*/
		PHY_SetBBReg(priv, pPhyReg->rfintfo, bRFSI_RFENV, 0x1);

		/* Set bit number of Address and Data for RF register */
		PHY_SetBBReg(priv, pPhyReg->rfHSSIPara2, b3WireAddressLength, 0x0); 	// Set 0 to 4 bits for Z-serial and set 1 to 6 bits for 8258
		PHY_SetBBReg(priv, pPhyReg->rfHSSIPara2, b3WireDataLength, 0x0);	// Set 0 to 12 bits for Z-serial and 8258, and set 1 to 14 bits for ???
		/*----Make sure RF is configurable----*/
		PHY_SetRFReg(priv, eRFPath, 0x0, bMask12Bits, 0xbf);

		rtStatus  = PHY_CheckBBAndRFOK(priv, HW90_BLOCK_RF, eRFPath);
		if(rtStatus!= 0)
		{
			DEBUG_INFO("PHY_RF8256_Config():Check Radio[%d] Fail!!\n", eRFPath);
			goto phy_RF8256_Config_ParaFile_Fail;
		}
#endif

		RF0_Final_Value = 0;
		RetryTimes = 5;

		/*----Initialize RF fom connfiguration file----*/
		switch (eRFPath)
		{
			case RF90_PATH_A:
				while ((RF0_Final_Value!=0x7f1) && (RetryTimes!=0)) {
					rtStatus = PHY_ConfigRFWithParaFile(priv, "/usr/rtl8190Pci/radio_a.txt", eRFPath);
					RF0_Final_Value = PHY_QueryRFReg(priv, eRFPath, 3, bMask12Bits, 1);
					PRINT_INFO("RF %d 3 register final value: %x\n", eRFPath, RF0_Final_Value);
					RetryTimes--;
				}
				break;
			case RF90_PATH_B:
				while ((RF0_Final_Value!=0x7f1) && (RetryTimes!=0)) {
					rtStatus = PHY_ConfigRFWithParaFile(priv, "/usr/rtl8190Pci/radio_b.txt", eRFPath);
					RF0_Final_Value = PHY_QueryRFReg(priv, eRFPath, 3, bMask12Bits, 1);
					PRINT_INFO("RF %d 3 register final value: %x\n", eRFPath, RF0_Final_Value);
					RetryTimes--;
				}
				break;
#ifdef RTL8190
			case RF90_PATH_C:
				while ((RF0_Final_Value!=0x7f1) && (RetryTimes!=0)) {
					rtStatus = PHY_ConfigRFWithParaFile(priv, "/usr/rtl8190Pci/radio_c.txt", eRFPath);
					RF0_Final_Value = PHY_QueryRFReg(priv, eRFPath, 3, bMask12Bits, 1);
					PRINT_INFO("RF %d 3 register final value: %x\n", eRFPath, RF0_Final_Value);
					RetryTimes--;
				}
				break;
			case RF90_PATH_D:
				while ((RF0_Final_Value!=0x7f1) && (RetryTimes!=0)) {
					rtStatus = PHY_ConfigRFWithParaFile(priv, "/usr/rtl8190Pci/radio_d.txt", eRFPath);
					RF0_Final_Value = PHY_QueryRFReg(priv, eRFPath, 3, bMask12Bits, 1);
					PRINT_INFO("RF %d 3 register final value: %x\n", eRFPath, RF0_Final_Value);
					RetryTimes--;
				}
				break;
#endif
			default:
				break;
		}

#ifdef RTL8192E
		/*----Restore RFENV control type----*/;
		switch(eRFPath) {
			case RF90_PATH_A:
				PHY_SetBBReg(priv, pPhyReg->rfintfs, bRFSI_RFENV, u4RegValue);
					break;
			case RF90_PATH_B:
				PHY_SetBBReg(priv, pPhyReg->rfintfs, bRFSI_RFENV<<16, u4RegValue);
				break;
			default:
				break;
		}
#endif
	}

	DEBUG_INFO("PHY-RF Initialization Success\n");
#ifdef RTL8192E
phy_RF8256_Config_ParaFile_Fail:
#endif
	return rtStatus;
}


static unsigned int RegScan(struct rtl8190_priv *priv, unsigned int Offset,
				unsigned long BitPosition, unsigned int CheckIteration)
{
	unsigned long ioaddr = priv->pshare->ioaddr;
	unsigned long templong;
	unsigned int i = 0;

	while (1)
	{
		delay_ms(1);
		templong = RTL_R32(Offset);
		if ((templong & BitPosition) == BitPosition)
			break;

		if (i > CheckIteration)
			break;

		i++;
	}

	return i;
}


static void ReadIMG(struct rtl8190_priv *priv, int fw_file)
{
	unsigned int buf_len=0, read_bytes;
	int fd=0, i = 0;
	unsigned char *mem_ptr=NULL, tmpbuf[4];
	mm_segment_t old_fs;
#ifndef	CONFIG_X86
	extern ssize_t sys_read(unsigned int fd, char * buf, size_t count);
#endif

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	// open *.img to read
	switch(fw_file) {
	case BOOT:
#ifdef CONFIG_X86
		if ((fd = rtl8190n_fileopen("/usr/rtl8190Pci/boot.img", O_RDONLY, 0)) < 0)
#else
		if ((fd = sys_open("/usr/rtl8190Pci/boot.img", O_RDONLY, 0)) < 0)
#endif
		{
			printk("boot.img cannot be opened\n");
			BUG();
		}
		mem_ptr = priv->pshare->fw_boot_buf;
		buf_len = FW_BOOT_SIZE;
		break;
	case MAIN:
#ifdef CONFIG_X86
		if ((fd = rtl8190n_fileopen("/usr/rtl8190Pci/main.img", O_RDONLY, 0)) < 0)
#else
		if ((fd = sys_open("/usr/rtl8190Pci/main.img", O_RDONLY, 0)) < 0)
#endif
		{
			printk("main.img cannot be opened\n");
			BUG();
		}
		mem_ptr = priv->pshare->fw_main_buf;
		buf_len = FW_MAIN_SIZE;
#ifdef RTL8192E
		i = 128;
#endif
		break;
	case DATA:
#ifdef CONFIG_X86
		if ((fd = rtl8190n_fileopen("/usr/rtl8190Pci/data.img", O_RDONLY, 0)) < 0)
#else
		if ((fd = sys_open("/usr/rtl8190Pci/data.img", O_RDONLY, 0)) < 0)
#endif
		{
			printk("data.img cannot be opened\n");
			BUG();
		}
		mem_ptr = priv->pshare->fw_data_buf;
		buf_len = FW_DATA_SIZE;
		break;
	}

	memset(mem_ptr, 0, buf_len); // clear memory

	read_bytes = sys_read(fd, mem_ptr+i, buf_len-i);

	sys_close(fd);
	set_fs(old_fs);

	if (read_bytes >= buf_len) {
		printk("Firmware buffer not large enough!\n");
		BUG();
	}
	else {
		switch(fw_file) {
		case BOOT:
			priv->pshare->fw_boot_len = read_bytes;
			break;
		case MAIN:
			priv->pshare->fw_main_len = read_bytes+i;
			read_bytes = priv->pshare->fw_main_len;
			break;
		case DATA:
			priv->pshare->fw_data_len = read_bytes;
			break;
		}
	}

	// do byte swap
	for (; i<read_bytes;i+=4) {
		tmpbuf[3]    = mem_ptr[i];
		tmpbuf[2]    = mem_ptr[i+1];
		tmpbuf[1]    = mem_ptr[i+2];
		tmpbuf[0]    = mem_ptr[i+3];
		mem_ptr[i]   = tmpbuf[0];
		mem_ptr[i+1] = tmpbuf[1];
		mem_ptr[i+2] = tmpbuf[2];
		mem_ptr[i+3] = tmpbuf[3];
	}
}


static void LoadIMG(struct rtl8190_priv *priv, int fw_file)
{
	unsigned long ioaddr=priv->pshare->ioaddr;
	int bResult=0, i=0;
	unsigned char *mem_ptr=NULL;
	unsigned int buf_len=0;

#ifdef CHECK_HANGUP
	if (!priv->reset_hangup)
#endif
		ReadIMG(priv, fw_file);

	switch(fw_file) {
	case BOOT:
		mem_ptr = priv->pshare->fw_boot_buf;
		buf_len = priv->pshare->fw_boot_len;
		break;
	case MAIN:
		mem_ptr = priv->pshare->fw_main_buf;
		buf_len = priv->pshare->fw_main_len;
		break;
	case DATA:
		mem_ptr = priv->pshare->fw_data_buf;
		buf_len = priv->pshare->fw_data_len;
		break;
	}

	while(1)
	{
		if ((i+LoadPktSize) < buf_len) {
			bResult = SetupOneCmdPacket(priv, mem_ptr, LoadPktSize, 0);
			if(bResult)
				RTL_W8(_TXPOLL_, POLL_CMD);
			delay_ms(1);
			mem_ptr += LoadPktSize;
			i += LoadPktSize;
		}
		else {
			bResult = SetupOneCmdPacket(priv, mem_ptr, (buf_len-i), 1);
			if(bResult)
				RTL_W8(_TXPOLL_, POLL_CMD);
			break;
		}
	}

	if (bResult == 0)
		printk("desc not available, firmware cannot be loaded ");
}


static int LoadBOOTIMG(struct rtl8190_priv *priv)
{
#ifdef RTL8190
	unsigned long ioaddr = priv->pshare->ioaddr;
	unsigned char *mem_ptr;
	int bResult = 0;
#endif

	LoadIMG(priv, BOOT);

#ifdef RTL8190
	// allocate memory for storing binary files
	if((mem_ptr=(unsigned char *)kmalloc((LoadPktSize), GFP_ATOMIC)) == NULL) 	{
		printk("not enough memory\n");
		return FALSE;
	}

	memset(mem_ptr, 0, LoadPktSize); // clear memory
	bResult = SetupOneCmdPacket(priv, mem_ptr, 128, 0); // 128 = 0x80, offset for CPU read from 0x800080
	if (bResult)
		RTL_W8(_TXPOLL_, POLL_CMD);

	kfree(mem_ptr);
#endif
	return TRUE;
}


static int LoadMAINIMG(struct rtl8190_priv *priv)
{
	unsigned long ioaddr = priv->pshare->ioaddr;
	unsigned int RegScanIteration;
	unsigned int RegScanLimit = 1000; // number of times to scan for bit ready, the speed varies depending on processor speed

	LoadIMG(priv, MAIN);

	RegScanIteration = RegScan(priv, _CPURST_, CPURST_PutCode, RegScanLimit);
	if(RegScanIteration < RegScanLimit) {
		//printk("Put Code OK!\n");
	}
	else {
		printk("Put Code Fail!!!\n");
		return FALSE;
	}

	//Turn on CPU
	RTL_W8(_CPURST_, RTL_R8(_CPURST_) | CPURST_Pwron);

	RegScanIteration = RegScan(priv, _CPURST_, CPURST_Brdy, RegScanLimit);
	if(RegScanIteration < RegScanLimit) {
		//printk("Boot Ready!\n");
	}
	else {
		printk("Boot Fail!!!\n");
		return FALSE;
	}

	return TRUE;
}


static int LoadDATAIMG(struct rtl8190_priv *priv)
{
	unsigned int RegScanIteration;
	unsigned int RegScanLimit = 1000;

	LoadIMG(priv, DATA);

	RegScanIteration = RegScan(priv, _CPURST_, CPURST_FwRdy, RegScanLimit);
	if (RegScanIteration < RegScanLimit) {
		//printk("Firmware Ready!\n");
		return TRUE;
	}
	else {
		printk("Firmware Not Ready!!!\n");
		return FALSE;
	}
}


int rtl819x_init_hw_PCI(struct rtl8190_priv *priv)
{
	struct wifi_mib	*pmib = GET_MIB(priv);
	struct rtl8190_hw *phw = GET_HW(priv);
	unsigned int	opmode = priv->pmib->dot11OperationEntry.opmode;
	unsigned long	ioaddr = priv->pshare->ioaddr;
	static unsigned long	val32;
	static unsigned short	val16;
	int i = 0;
#ifdef RTL8192E
	unsigned int ulRegRead;
#endif

	DBFENTER;

	GetHardwareVersion(priv);

#ifdef DRVMAC_LB
	if (!priv->pmib->miscEntry.drvmac_lb)
#endif
	{
		val32 = phy_BB8190_Config_ParaFile(priv);
		if (val32)
			return -1;
	}

	// support up to MCS13 for 1T2R, modify rx capability here
	if ((get_rf_mimo_mode(priv) == MIMO_1T2R) || (get_rf_mimo_mode(priv) == MIMO_2T2R))
		GET_MIB(priv)->dot11nConfigEntry.dot11nSupportedMCS &= 0x3fff;
	else if (get_rf_mimo_mode(priv) == MIMO_1T1R)
		GET_MIB(priv)->dot11nConfigEntry.dot11nSupportedMCS &= 0x00ff;

#ifdef RTL8192E
	//3//
	//3//Set Loopback mode or Normal mode
	//3//
	//2006.12.13 by emily. Note!We should not merge these two CPU_GEN register writings
	//      because setting of System_Reset bit reset MAC to default transmission mode.
	ulRegRead = RTL_R32(_CPURST_);
	ulRegRead = ((ulRegRead & 0xFFF8FFFF) | 0x00080000);
	RTL_W32(_CPURST_, ulRegRead);
	// 2006.11.29. After reset cpu, we sholud wait for a second, otherwise, it may fail to write registers. Emily
	delay_us(500);
        //3//
        //3 //Fix the issue of E-cut high temperature issue
        //3//
        // TODO: E cut only
        {
                unsigned char ICVersion = RTL_R8(0x301);// here is IC version
                unsigned char SwitchingRegulatorOutput;
                if(ICVersion >= 0x4) //E-cut only
                {
                 // HW SD suggest that we should not wirte this register too often, so driver
                 // should readback this register. This register will be modified only when
                 // power on reset
                        SwitchingRegulatorOutput = RTL_R8(0xb8);
                        if(SwitchingRegulatorOutput  != 0xb8)
                        {
                                RTL_W8(0xb8, 0xa8);
                                delay_ms(100);
                                RTL_W8(0xb8, 0xb8);
                        }
                }
        }

#endif

	PHY_ConfigMACWithParaFile(priv);

	// PJ 1-5-2007 Reset PHY parameter counters
	RTL_W32(0xD00, RTL_R32(0xD00)|BIT(27));
	RTL_W32(0xD00, RTL_R32(0xD00)&(~(BIT(27))));

// By H.Y. advice
//	RTL_W16(_BCNTCFG_, 0x060a);
	if (OPMODE & WIFI_AP_STATE)
		RTL_W16(_BCNTCFG_, 0x000a);
	else
// for debug
//	RTL_W16(_BCNTCFG_, 0x060a);
	RTL_W16(_BCNTCFG_, 0x0204);


	// Initialize Number of Reserved Pages in Firmware Queue
// for debug ----------------------
#if 0
	RTL_W32(_RQPN1_, NUM_OF_PAGE_IN_FW_QUEUE_BK << RSVD_FW_QUEUE_PAGE_BK_SHIFT | \
					 NUM_OF_PAGE_IN_FW_QUEUE_BE << RSVD_FW_QUEUE_PAGE_BE_SHIFT | \
					 NUM_OF_PAGE_IN_FW_QUEUE_VI << RSVD_FW_QUEUE_PAGE_VI_SHIFT | \
					 NUM_OF_PAGE_IN_FW_QUEUE_VO << RSVD_FW_QUEUE_PAGE_VO_SHIFT);
	RTL_W32(_RQPN2_, NUM_OF_PAGE_IN_FW_QUEUE_MGNT << RSVD_FW_QUEUE_PAGE_MGNT_SHIFT);
	RTL_W32(_RQPN3_, APPLIED_RESERVED_QUEUE_IN_FW | \
					 NUM_OF_PAGE_IN_FW_QUEUE_BCN << RSVD_FW_QUEUE_PAGE_BCN_SHIFT|\
					 NUM_OF_PAGE_IN_FW_QUEUE_PUB << RSVD_FW_QUEUE_PAGE_PUB_SHIFT);
#else
	if (pmib->dot11OperationEntry.wifi_specific) {
		RTL_W32(_RQPN1_, (NUM_OF_PAGE_IN_FW_QUEUE_BK+(NUM_OF_PAGE_IN_FW_QUEUE_PUB/4)) << RSVD_FW_QUEUE_PAGE_BK_SHIFT | \
						 (NUM_OF_PAGE_IN_FW_QUEUE_BE+(NUM_OF_PAGE_IN_FW_QUEUE_PUB/4)) << RSVD_FW_QUEUE_PAGE_BE_SHIFT | \
						 (NUM_OF_PAGE_IN_FW_QUEUE_VI+(NUM_OF_PAGE_IN_FW_QUEUE_PUB/4)) << RSVD_FW_QUEUE_PAGE_VI_SHIFT | \
						 (NUM_OF_PAGE_IN_FW_QUEUE_VO+(NUM_OF_PAGE_IN_FW_QUEUE_PUB/4)) << RSVD_FW_QUEUE_PAGE_VO_SHIFT);
	}
	else {
		RTL_W32(_RQPN1_, 0 << RSVD_FW_QUEUE_PAGE_BK_SHIFT | \
						 (NUM_OF_PAGE_IN_FW_QUEUE_BE +NUM_OF_PAGE_IN_FW_QUEUE_PUB) << RSVD_FW_QUEUE_PAGE_BE_SHIFT | \
						 (NUM_OF_PAGE_IN_FW_QUEUE_VI + NUM_OF_PAGE_IN_FW_QUEUE_VO + NUM_OF_PAGE_IN_FW_QUEUE_BK)<< RSVD_FW_QUEUE_PAGE_VI_SHIFT | \
						 0 << RSVD_FW_QUEUE_PAGE_VO_SHIFT);
	}

	RTL_W32(_RQPN2_, NUM_OF_PAGE_IN_FW_QUEUE_MGNT << RSVD_FW_QUEUE_PAGE_MGNT_SHIFT);
	RTL_W32(_RQPN3_, APPLIED_RESERVED_QUEUE_IN_FW | \
					 NUM_OF_PAGE_IN_FW_QUEUE_BCN << RSVD_FW_QUEUE_PAGE_BCN_SHIFT|\
					 0 << RSVD_FW_QUEUE_PAGE_PUB_SHIFT);
#endif
//-----------------------------


	// configure timing parameter
	RTL_W32(0x4C, 0x00501330);
	RTL_W16(0x306, 0x060F);
	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
		RTL_W16(_SIFS_OFDM_, 0x1010);
		RTL_W8(_SLOT_, 0x09);
	}
	else if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11A) {
		RTL_W16(_SIFS_OFDM_, 0x1010);
		RTL_W8(_SLOT_, 0x09);
	}
	else if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) {
		RTL_W16(_SIFS_OFDM_, 0x0a0a);
		RTL_W8(_SLOT_, 0x09);
	}
	else { // WIRELESS_11B
		RTL_W16(_SIFS_OFDM_, 0x0a0a);
		RTL_W8(_SLOT_, 0x14);
	}

	init_EDCA_para(priv, pmib->dot11BssType.net_work_type);

	RTL_W8(0x1fc, 0x0f);
	RTL_W8(0xb2, 0x8f);
	RTL_W8(0xb3, 0xc7);		//c7 for fix P-bit Mask, d7 for P-bit valid

	RTL_W8(_9346CR_, CR9346_CFGRW);


	// Set Tx and Rx DMA burst size
	RTL_W8(_PCIF_, 0x77);
	// Enable byte shift
	RTL_W8(_PCIF_+2, 0x01);


	RTL_W32(_RDSAR_, phw->ring_dma_addr);
	RTL_W32(_TMGDA_, phw->tx_ring0_addr);
	RTL_W32(_TBKDA_, phw->tx_ring1_addr);
	RTL_W32(_TBEDA_, phw->tx_ring2_addr);
	RTL_W32(_TLPDA_, phw->tx_ring3_addr);
	RTL_W32(_TNPDA_, phw->tx_ring4_addr);
	RTL_W32(_THPDA_, phw->tx_ring5_addr);
	RTL_W32(_TBDA_, phw->tx_ringB_addr);
	RTL_W32(_RCDSA_, phw->rxcmd_ring_addr);
	RTL_W32(_TCDA_, phw->txcmd_ring_addr);

	// Retry Limit
	if (priv->pmib->dot11OperationEntry.dot11LongRetryLimit)
		val16 = priv->pmib->dot11OperationEntry.dot11LongRetryLimit & 0xff;
	else {
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)
			val16 = 0x20;
		else
			val16 = 0x06;
	}
	if (priv->pmib->dot11OperationEntry.dot11ShortRetryLimit)
		val16 |= ((priv->pmib->dot11OperationEntry.dot11ShortRetryLimit & 0xff) << 8);
	else {
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)
			val16 |= (0x20 << 8);
		else
			val16 |= (0x06 << 8);
	}
	RTL_W16(_RETRYCNT_, val16);

	// Response Rate Set
	// let ACK sent by highest of 24Mbps
	val32 = 0x1ff;
	if (pmib->dot11RFEntry.shortpreamble)
		val32 |= BIT(23);
	RTL_W32(_RRSR_, val32);

	// Adaptive Rate Table for Basic Rate
	val32 = 0;
	for (i=0; i<32; i++) {
		if (AP_BSSRATE[i]) {
			if (AP_BSSRATE[i] & 0x80)
				val32 |= get_bit_value_from_ieee_value(AP_BSSRATE[i] & 0x7f);
		}
	}
	val32 |= (priv->pmib->dot11nConfigEntry.dot11nBasicMCS << 12);
	RTL_W32(_RATR7_, val32);

	//setting MAR
	RTL_W32(_MAR0_, 0xffffffff);
	RTL_W32(_MAR4_, 0xffffffff);

	//setting BSSID, not matter AH/AP/station
	memcpy((void *)&val16, (pmib->dot11OperationEntry.hwaddr), 2);
	memcpy((void *)&val32, (pmib->dot11OperationEntry.hwaddr + 2), 4);
	RTL_W16(_BSSID_, cpu_to_le16(val16));
	RTL_W32(_BSSID_ + 2, cpu_to_le32(val32));

	//setting TCR
	//TCR, use default value

	//setting RCR
	RTL_W32(_RCR_, _APWRMGT_ | _AMF_ | _ADF_ | _AICV_ | _ACRC32_ | _AB_ | _AM_ | _APM_);
//	if (pmib->dot11OperationEntry.crc_log)
//		RTL_W32(_RCR_, RTL_R32(_RCR_) | _ACRC32_);

	//setting MSR
	if (opmode & WIFI_AP_STATE)
	{
		DEBUG_INFO("AP-mode enabled...\n");

		if (priv->pmib->dot11WdsInfo.wdsPure)
			RTL_W8(_MSR_, _HW_STATE_NOLINK_);
		else
			RTL_W8(_MSR_, _HW_STATE_AP_);
// Move init beacon after f/w download
#if 0
		if (priv->auto_channel == 0) {
			DEBUG_INFO("going to init beacon\n");
			init_beacon(priv);
		}
#endif
	}
#ifdef CLIENT_MODE
	else if (opmode & WIFI_STATION_STATE)
	{
		DEBUG_INFO("Station-mode enabled...\n");
		RTL_W8(_MSR_, _HW_STATE_STATION_);
	}
	else if (opmode & WIFI_ADHOC_STATE)
	{
		DEBUG_INFO("Adhoc-mode enabled...\n");
		RTL_W8(_MSR_, _HW_STATE_ADHOC_);
	}
#endif
	else
	{
		printk("Operation mode error!\n");
		return 2;
	}

	CamResetAllEntry(priv);
	RTL_W16(_WPACFG_, 0x0000);
	if ((OPMODE & (WIFI_AP_STATE|WIFI_STATION_STATE|WIFI_ADHOC_STATE)) &&
		!priv->pmib->dot118021xAuthEntry.dot118021xAlgrthm &&
			(pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_ ||
				pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_)) {
		pmib->dot11GroupKeysTable.dot11Privacy = pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm;
		if (pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_)
			i = 5;
		else
			i = 13;
//#if defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
#ifdef USE_WEP_DEFAULT_KEY
		memcpy(pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKey.skey,
			&priv->pmib->dot11DefaultKeysTable.keytype[pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex].skey[0], i);
		pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKeyLen = i;
		pmib->dot11GroupKeysTable.keyid = pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex;
		pmib->dot11GroupKeysTable.keyInCam = 1;
#else
		memcpy(pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKey.skey,
							&priv->pmib->dot11DefaultKeysTable.keytype[0].skey[0], i);
		pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKeyLen = i;
		pmib->dot11GroupKeysTable.keyid = pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex;
		pmib->dot11GroupKeysTable.keyInCam = 0;
#endif
	}

// for debug
#if 0
// when hangup reset, re-init TKIP/AES key in ad-hoc mode
#ifdef CLIENT_MODE
	if ((OPMODE & WIFI_ADHOC_STATE) && pmib->dot11OperationEntry.keep_rsnie &&
		(pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _TKIP_PRIVACY_ ||
			pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _CCMP_PRIVACY_)) {
		DOT11_SET_KEY Set_Key;
		Set_Key.KeyType = DOT11_KeyType_Group;
		Set_Key.EncType = pmib->dot11GroupKeysTable.dot11Privacy;
		DOT11_Process_Set_Key(priv->dev, NULL, &Set_Key, pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKey.skey);
	}
	else
#endif
//-------------------------------------- david+2006-06-30
	// restore group key if it has been set before open, david
	if (pmib->dot11GroupKeysTable.keyInCam) {
		int retVal;
		retVal = CamAddOneEntry(priv, (unsigned char *)"\xff\xff\xff\xff\xff\xff",
					pmib->dot11GroupKeysTable.keyid,
					pmib->dot11GroupKeysTable.dot11Privacy<<2,
					0, pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKey.skey);
		if (retVal)
			priv->pshare->CamEntryOccupied++;
		else {
			DEBUG_ERR("Add group key failed!\n");
		}
	}
#endif
	//here add if legacy WEP
	// if 1x is enabled, do not set default key, david
//#if 0	// marked by victoryman, use pairwise key at present, 20070627
//#if defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
#ifdef USE_WEP_DEFAULT_KEY
#ifdef MBSSID
	if (!(OPMODE & WIFI_AP_STATE))
#endif
	if(!SWCRYPTO && !IEEE8021X_FUN &&
		(pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_ ||
		 pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_))
		init_DefaultKey_Enc(priv, NULL, 0);
#endif


	//Setup Beacon Interval/interrupt interval, ATIM-WIND ATIM-Interrupt
	RTL_W16(_BCNITV_, pmib->dot11StationConfigEntry.dot11BeaconPeriod);
	RTL_W16(_ATIMWIN_, 0);
	RTL_W16(_DRVERLYINT_, 2);
	RTL_W16(_BCNDMA_, 0xf);

// for debug
#ifdef CLIENT_MODE
	if (OPMODE & WIFI_ADHOC_STATE)
		RTL_W8(0x78, 100);
#endif
//--------------


	// Ack ISR, and then unmask IMR
	RTL_W32(_ISR_, RTL_R32(_ISR_));
	RTL_W32(_IMR_, 0x0);
	priv->pshare->InterruptMask = _ROK_ | _BCNDMAINT_ | _RDU_ | _RXFOVW_ | _RXCMDOK_;

	if (opmode & WIFI_AP_STATE)
		priv->pshare->InterruptMask |= _BCNDOK_;
#ifdef CLIENT_MODE
	else if (opmode & WIFI_ADHOC_STATE)
		priv->pshare->InterruptMask |= (_TBDER_ | _TBDOK_);
#endif

	if (priv->pmib->miscEntry.ack_timeout && (priv->pmib->miscEntry.ack_timeout < 0xff))
		RTL_W8(_ACKTIMEOUT_, priv->pmib->miscEntry.ack_timeout);

	RTL_W8(_9346CR_, 0);

	// ===========================================================================================
	// Download Firmware
	// allocate memory for tx cmd packet
	if((priv->pshare->txcmd_buf = (unsigned char *)kmalloc((LoadPktSize), GFP_ATOMIC)) == NULL) {
		printk("not enough memory for txcmd_buf\n");
		return -1;
	}

	RTL_W8(_CR_, BIT(2) | BIT(3));
	delay_ms(10);

	priv->pshare->cmdbuf_phyaddr = get_physical_addr(priv, priv->pshare->txcmd_buf,
			LoadPktSize, PCI_DMA_TODEVICE);

	if(LoadFirmware(priv) == FALSE) {
		printk("Load firmware fail!\n");
		return -1;
	}
	else {
//		delay_ms(20);
		PRINT_INFO("Load firmware successful! \n");
	}

#ifdef CONFIG_NET_PCI
	if (IS_PCIBIOS_TYPE)
		pci_unmap_single(priv->pshare->pdev, priv->pshare->cmdbuf_phyaddr,
			(LoadPktSize), PCI_DMA_TODEVICE);
#endif
	kfree(priv->pshare->txcmd_buf);
	//enable interrupt
	RTL_W32(_IMR_, priv->pshare->InterruptMask);
	// ===========================================================================================


#ifdef DRVMAC_LB
	if (priv->pmib->miscEntry.drvmac_lb) {
		RTL_W8(_MSR_, _HW_STATE_NOLINK_);
		drvmac_loopback(priv);
	}
	else
#endif
	{
#ifdef CHECK_HANGUP
		if (!priv->reset_hangup
#ifdef CLIENT_MODE
				|| (!(OPMODE & WIFI_AP_STATE) &&
						(priv->join_res != STATE_Sta_Bss) && (priv->join_res != STATE_Sta_Ibss_Active))
#endif
			)
#endif
		{
			val32 = phy_RF8256_Config_ParaFile(priv);
			if (val32)
				return -1;
		}
	}

#ifdef CHECK_HANGUP
	if (priv->reset_hangup)
		priv->pshare->CurrentChannelBW = priv->pshare->is_40m_bw;
	else
#endif
	{
		if (opmode & WIFI_AP_STATE)
			priv->pshare->CurrentChannelBW = priv->pshare->is_40m_bw;
		else
			priv->pshare->CurrentChannelBW = HT_CHANNEL_WIDTH_20;
	}

#if	defined(RTL8190)
	if ((priv->pmib->dot11RFEntry.legacyOFDM_pwrdiff == 0) ||
		(priv->pmib->dot11RFEntry.crystalCap == 0)) {
		priv->pshare->use_default_para = 1;
		priv->pshare->legacyOFDM_pwrdiff = 0;
	}
	else
#endif
	{
		int value;

		priv->pshare->use_default_para = 0;
		if (priv->pmib->dot11RFEntry.legacyOFDM_pwrdiff >= 32)
			priv->pshare->legacyOFDM_pwrdiff = (priv->pmib->dot11RFEntry.legacyOFDM_pwrdiff - 32) & 0x0f;
		else {
			priv->pshare->legacyOFDM_pwrdiff = (32 - priv->pmib->dot11RFEntry.legacyOFDM_pwrdiff) & 0x0f;
			priv->pshare->legacyOFDM_pwrdiff |= 0x80;
		}

#ifdef RTL8190
		value = (int)priv->pmib->dot11RFEntry.crystalCap - 32;
		PHY_SetBBReg(priv, rFPGA0_AnalogParameter1, bXtalCap01, (value & 0x00000003));
		PHY_SetBBReg(priv, rFPGA0_AnalogParameter2, bXtalCap23, ((value & 0x0000000c) >> 2));
#elif defined(RTL8192E)
		value = priv->EE_CrystalCap;
		PHY_SetBBReg(priv, rFPGA0_AnalogParameter1, bXtalCap92x, value);
#endif
	}

#ifdef RTL8190
	// get default Tx AGC offset
	*(unsigned int *)(&priv->pshare->phw->MCSTxAgcOffset[4]) = RTL_R32(_MCS_TXAGC_0_);
	*(unsigned int *)(&priv->pshare->phw->MCSTxAgcOffset[0]) = RTL_R32(_MCS_TXAGC_1_);
#endif

	if ((priv->pmib->dot11RFEntry.ther_rfic < 10) || (priv->pmib->dot11RFEntry.ther_rfic > 100)) {
		//printk("TPT: unreasonable target TSSI %d\n", priv->pmib->dot11RFEntry.ther_rfic);
		priv->pmib->dot11RFEntry.ther_rfic = 0;
	}

	// get bandwidth power difference (40M to 20M)
	if ((priv->pmib->dot11RFEntry.bw_pwrdiff >= 24) && (priv->pmib->dot11RFEntry.bw_pwrdiff <= 39)) {
		if (priv->pmib->dot11RFEntry.bw_pwrdiff >= 32) {
			priv->pshare->bw_pwrdiff_sign = 0;
			priv->pshare->bw_pwrdiff_ofst = priv->pmib->dot11RFEntry.bw_pwrdiff - 32;
		}
		else {
			priv->pshare->bw_pwrdiff_sign = 1;
			priv->pshare->bw_pwrdiff_ofst = 32 - priv->pmib->dot11RFEntry.bw_pwrdiff;
		}
	}
	else {
		priv->pshare->bw_pwrdiff_sign = 0;
		priv->pshare->bw_pwrdiff_ofst = 0;
	}

	if (opmode & WIFI_AP_STATE)
	{
		if (priv->auto_channel == 0) {
			DEBUG_INFO("going to init beacon\n");
			init_beacon(priv);
		}
	}

	/*---- Set CCK and OFDM Block "ON"----*/
	PHY_SetBBReg(priv, rFPGA0_RFMOD, bCCKEn, 0x1);
	PHY_SetBBReg(priv, rFPGA0_RFMOD, bOFDMEn, 0x1);
	delay_ms(2);

#ifdef RTL8192E
	//Turn on the RF8256, joshua
	PHY_SetBBReg(priv, rFPGA0_XA_RFInterfaceOE, 1<<4, 0x1);		// 0x860[4]
	PHY_SetBBReg(priv, rFPGA0_AnalogParameter4, 0x300, 0x3);		// 0x88c[4]
	PHY_SetBBReg(priv, rFPGA0_AnalogParameter1, 0x60, 0x3); 		// 0x880[6:5]
	PHY_SetBBReg(priv, rOFDM0_TRxPathEnable, 0xf, 0x3);			// 0xc04[3:0]
	PHY_SetBBReg(priv, rOFDM1_TRxPathEnable, 0xf, 0x3);			// 0xd04[3:0]
	PHY_SetBBReg(priv, rFPGA0_AnalogParameter2, 0x7000, 0x3);

	//Enable LED
	RTL_W32(0x87, 0x0);
#endif

	SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);
	SwChnl(priv, priv->pmib->dot11RFEntry.dot11channel, priv->pshare->offset_2nd_chan);
	delay_ms(100);

	//RTL_W32(0x100, RTL_R32(0x100) | BIT(14)); //for 8190 fw debug

	DBFEXIT;

	return 0;
}
#endif // defined(RTL8190) || defined(RTL8192E)


static int LoadFirmware(struct rtl8190_priv *priv)
{
#if defined(RTL8190) || defined(RTL8192E)
	unsigned long ioaddr = priv->pshare->ioaddr;
	unsigned int val32, year, month, day, hour, min;

	if (LoadBOOTIMG(priv) == FALSE)
		return FALSE;

	if (LoadMAINIMG(priv) == FALSE)
		return FALSE;

	if (LoadDATAIMG(priv) == FALSE)
		return FALSE;

	val32 = RTL_R32(_BLDTIME_);
	year = val32 & 0xff;
	month = (val32 >> 8) & 0x0f;
	day = (val32 >> 12) & 0x3f;
	hour = (val32 >> 18) & 0x3f;
	min = (val32 >> 24) & 0x3f;
	PRINT_INFO("RTL8190 Firmware buildtime: %02d-%02d-%02d %02d:%02d\n", year, month, day, hour, min);
	priv->pshare->fw_version = val32;

#elif defined(RTL8192SE) || defined(RTL8192SU)

	//we should read header first
	rtl8192SE_ReadIMG(priv);

	DEBUG_INFO("Load IMEM\n");
	if(LoadIMEMIMG(priv) == FALSE)
		return FALSE;

	DEBUG_INFO("Load EMEM\n");
	if(LoadEMEMIMG(priv) == FALSE)
		return FALSE;
	
	DEBUG_INFO("Load DMEM\n");
	if(LoadDMEMIMG(priv) == FALSE)
		return FALSE;

#if 0
	val32 = RTL_R32(BUILDTIME);
	year = val32 & 0xff;
	month = (val32 >> 8) & 0x0f;
	day = (val32 >> 12) & 0x3f;
	hour = (val32 >> 18) & 0x3f;
	min = (val32 >> 24) & 0x3f;
	PRINT_INFO("RTL8190 Firmware buildtime: %02d-%02d-%02d %02d:%02d\n", year, month, day, hour, min);
	priv->pshare->fw_version = val32;
#endif

	PRINT_INFO("RTL8192 Firmware version: %04x(%d.%d)\n",
		priv->pshare->fw_version, priv->pshare->fw_src_version, priv->pshare->fw_sub_version);

	return TRUE;
#endif
	return TRUE;
}


#define	SET_RTL8192SE_RF_HALT(priv)						\
{ 														\
	unsigned char u1bTmp;								\
	unsigned long ioaddr=priv->pshare->ioaddr;			\
														\
	do													\
	{													\
		u1bTmp = RTL_R8(LDOV12D_CTRL);					\
		u1bTmp |= BIT(0); 								\
		RTL_W8(LDOV12D_CTRL, u1bTmp);					\
		RTL_W8(SPS1_CTRL, 0x0);							\
		RTL_W8(TXPAUSE, 0xFF);							\
		RTL_W16(CMDR, 0x57FC);							\
		delay_us(100);									\
		RTL_W16(CMDR, 0x77FC);							\
		RTL_W8(PHY_CCA, 0x0);							\
		delay_us(10);									\
		RTL_W16(CMDR, 0x37FC);							\
		delay_us(10);									\
		RTL_W16(CMDR, 0x77FC);							\
		delay_us(10);									\
		RTL_W16(CMDR, 0x57FC);							\
		RTL_W16(CMDR, 0x0000);							\
		u1bTmp = RTL_R8((SYS_CLKR + 1));				\
		if (u1bTmp & BIT(7))							\
		{												\
			u1bTmp &= ~(BIT(6) | BIT(7));				\
			if (!HalSetSysClk8192SE(priv, u1bTmp))		\
			break;										\
		}												\
		RTL_W8(0x03, 0x71);								\
		RTL_W8(0x09, 0x70);								\
		RTL_W8(0x29, 0x68);								\
		RTL_W8(0x28, 0x00);								\
		RTL_W8(0x20, 0x50);								\
		RTL_W8(0x26, 0x0E);								\
	} while (FALSE);									\
}


int rtl819x_stop_hw(struct rtl8190_priv *priv, int reset_bb)
{
#if !defined(RTL8192SU)
	static unsigned long ioaddr;
#endif
	static RF90_RADIO_PATH_E eRFPath;
	static BB_REGISTER_DEFINITION_T *pPhyReg;
	static int do_mac_reset;
    do_mac_reset = 1;
#if !defined(RTL8192SU)
	ioaddr=priv->pshare->ioaddr;
#endif

	if (reset_bb) {
		for (eRFPath = RF90_PATH_A; eRFPath<priv->pshare->phw->NumTotalRFPath; eRFPath++) {
			pPhyReg = &priv->pshare->phw->PHYRegDef[eRFPath];
			PHY_SetBBReg(priv, pPhyReg->rfintfs, bRFSI_RFENV, bRFSI_RFENV);
			PHY_SetBBReg(priv, pPhyReg->rfintfo, bRFSI_RFENV, 0);
		}

#if	defined(RTL8190) || defined(RTL8192E)
		RTL_W32(_IMR_, 0);
		RTL_W8(_MSR_, _HW_STATE_NOLINK_);

#ifdef CHECK_HANGUP
		if (!priv->reset_hangup)
#endif
		{
			RTL_W8(_CR_, 0);
			delay_ms(1);
		}

#elif defined(RTL8192SE) || defined(RTL8192SU)
#if !defined(RTL8192SU)
		RTL_W32(IMR, 0);
		RTL_W32(IMR+4, 0);
#endif		
		RTL_W8(MSR, MSR_NOLINK);

#if !defined(RTL8192SU)
		SET_RTL8192SE_RF_HALT(priv);
		delay_us(120);
#endif		

#ifdef CHECK_HANGUP
		if (!priv->reset_hangup)
#endif
		{
			RTL_W16(CMDR, 0);
			delay_ms(1);
		}

/*
		// 8192SE specific setting
		RTL_W16(0x0, RTL_R16(0x0) & ~BIT(8));
		delay_ms(1);
		RTL_W16(0x2, RTL_R16(0x2)| BIT(13));
		delay_ms(1);
		RTL_W16(0x8, (RTL_R16(0x8) & ~BIT(14)) & ~BIT(15));
		delay_ms(1);
		RTL_W16(0x2, RTL_R16(0x2) & ~BIT(11));
		delay_ms(1);
		RTL_W16(0x2, RTL_R16(0x2) | BIT(11));
		delay_ms(1);
		RTL_W16(0x2, RTL_R16(0x2) & ~BIT(15));
		delay_ms(1);
		RTL_W16(0x2, RTL_R16(0x2) | BIT(15));
		delay_ms(1);
*/
#endif // if(8190, 8192) else 8192SE
	}
	if (reset_bb)
		do_mac_reset = 0;

	if (do_mac_reset) {
#ifdef RTL8190
		RTL_W8(_CPURST_, RTL_R8(_CPURST_) | CPURST_SysRst);
#elif defined(RTL8192E)
		//reference to 8129e driver, must write 32bits
		unsigned int Val;
		Val = RTL_R32(_CPURST_);
		//Val &= ~(0x00000100);
		Val |= CPURST_SysRst;
		RTL_W32(_CPURST_, Val);
#endif
		delay_us(800); // it is critial!
	}
	printk("stop hw finished!\n");
	return SUCCESS;
}


void SwBWMode(struct rtl8190_priv *priv, unsigned int bandwidth, int offset)
{
#if !defined(RTL8192SU)
	static unsigned long ioaddr;
#endif
	static unsigned char regBwOpMode, nCur40MhzPrimeSC;
#if	defined(RTL8190) || defined(RTL8192E)
	static RF90_RADIO_PATH_E eRFPath;
#endif
#if !defined(RTL8192SU)
	ioaddr = priv->pshare->ioaddr;
#endif

	DEBUG_INFO("SwBWMode(): Switch to %s bandwidth\n", bandwidth?"40MHz":"20MHz");

	//3 <1> Set MAC register
	regBwOpMode = RTL_R8(_BWOPMODE_);

	switch (bandwidth)
	{
		case HT_CHANNEL_WIDTH_20:
			regBwOpMode |= BW_OPMODE_20MHZ;
			RTL_W8(_BWOPMODE_, regBwOpMode);
			break;
		case HT_CHANNEL_WIDTH_20_40:
			regBwOpMode &= ~BW_OPMODE_20MHZ;
			RTL_W8(_BWOPMODE_, regBwOpMode);
#ifdef 	RTL8192SU
			//RTL_W8(RRSR+2, 0xe0);
#endif
			break;
		default:
			DEBUG_ERR("SwBWMode(): bandwidth mode error!\n");
			return;
			break;
	}

	//3 <2> Set PHY related register
	if (offset == 1)
		nCur40MhzPrimeSC = 2;
	else
		nCur40MhzPrimeSC = 1;
	switch (bandwidth)
	{
		case HT_CHANNEL_WIDTH_20:
			PHY_SetBBReg(priv, rFPGA0_RFMOD, bRFMOD, 0x0);
			PHY_SetBBReg(priv, rFPGA1_RFMOD, bRFMOD, 0x0);
#ifdef RTL8190
			PHY_SetBBReg(priv, rFPGA0_AnalogParameter1, bADClkPhase, 1);
			PHY_SetBBReg(priv, rOFDM0_RxDetector1, bMaskByte0, 0x44);		//suggest by jerry, by emily
#elif defined(RTL8192E)
			PHY_SetBBReg(priv, rFPGA0_AnalogParameter1, 0x00100000, 1);
#elif defined(RTL8192SE) || defined(RTL8192SU)
			PHY_SetBBReg(priv, rFPGA0_AnalogParameter1, 0x00100000, 1);
			PHY_SetBBReg(priv, rFPGA0_AnalogParameter2, 0x000000ff, 0x58);
						// From SD3 WHChang
			PHY_SetBBReg(priv, rFPGA0_AnalogParameter1, 0x00300000, 3);
#endif
			PHY_SetBBReg(priv, rCCK0_TxFilter1, bMaskDWord, 0x1a1b0000);	//suggest by YN
			PHY_SetBBReg(priv, rCCK0_TxFilter2, bMaskDWord, 0x090e1317);	//suggest by YN
			PHY_SetBBReg(priv, rCCK0_DebugPort, bMaskDWord, 0x00000204);	//suggest by YN
			break;
		case HT_CHANNEL_WIDTH_20_40:
			PHY_SetBBReg(priv, rFPGA0_RFMOD, bRFMOD, 0x1);
			PHY_SetBBReg(priv, rFPGA1_RFMOD, bRFMOD, 0x1);
			PHY_SetBBReg(priv, rCCK0_System, bCCKSideBand, (nCur40MhzPrimeSC>>1));
#if defined(RTL8192SE) || defined(RTL8192SU)
			PHY_SetBBReg(priv, rOFDM1_LSTF, 0xC00, nCur40MhzPrimeSC);
#endif
#ifdef RTL8190
			PHY_SetBBReg(priv, rFPGA0_AnalogParameter1, bADClkPhase, 0);
			PHY_SetBBReg(priv, rOFDM1_LSTF, 0xC00, nCur40MhzPrimeSC);
			PHY_SetBBReg(priv, rOFDM0_RxDetector1, bMaskByte0, 0x42);		//suggest by jerry, by emily
			PHY_SetBBReg(priv, rFPGA0_RFMOD, 0x00000060, offset);			//suggest by YN
#elif defined(RTL8192E)
			PHY_SetBBReg(priv, rFPGA0_AnalogParameter1, 0x00100000, 0);
			PHY_SetBBReg(priv, rOFDM1_LSTF, 0xC00, nCur40MhzPrimeSC);
#elif defined(RTL8192SE) || defined(RTL8192SU)
			// From SD3 WHChang
			PHY_SetBBReg(priv, rFPGA0_AnalogParameter1, 0x00300000, 3);
			// Set Control channel to upper or lower. These settings are required only for 40MHz
			PHY_SetBBReg(priv, rFPGA0_AnalogParameter1, 0x00100000, 1);
			PHY_SetBBReg(priv, rFPGA0_AnalogParameter2, 0x000000ff, 0x18);
//			PHY_SetBBReg(priv, rCCK0_System, bCCKSideBand, (nCur40MhzPrimeSC>>1));
//			PHY_SetBBReg(priv, rOFDM1_LSTF, 0xC00, nCur40MhzPrimeSC);
#endif
                        PHY_SetBBReg(priv, rCCK0_TxFilter1, bMaskDWord, 0x1a1b0000);    //suggest by YN
                        PHY_SetBBReg(priv, rCCK0_TxFilter2, bMaskDWord, 0x090e1317);    //suggest by YN
                        PHY_SetBBReg(priv, rCCK0_DebugPort, bMaskDWord, 0x00000204);    //suggest by YN
			break;
		default:
			DEBUG_ERR("SwBWMode(): bandwidth mode error!\n");
			return;
			break;
	}

#if	defined(RTL8190) || defined(RTL8192E)

	//3 <3> Set RF related register
	for (eRFPath = RF90_PATH_A; eRFPath<priv->pshare->phw->NumTotalRFPath; eRFPath++)
	{
		if (!PHYCheckIsLegalRfPath8190Pci(priv, eRFPath))
			continue;
/*
		if (get_rf_mimo_mode(priv) == MIMO_1T2R) {
			if ((eRFPath == RF90_PATH_A) || (eRFPath == RF90_PATH_B))
				continue;
		}
		else if (get_rf_mimo_mode(priv) == MIMO_2T2R) {
			if ((eRFPath == RF90_PATH_B) || (eRFPath == RF90_PATH_D))
				continue;
		}
*/
		switch(bandwidth)
		{
		case HT_CHANNEL_WIDTH_20:
			PHY_SetRFReg(priv, (RF90_RADIO_PATH_E)eRFPath, 0x0b, bMask12Bits, 0x100); //phy para:1ba
			PHY_SetRFReg(priv, (RF90_RADIO_PATH_E)eRFPath, 0x2c, bMask12Bits, 0x3d7);
			PHY_SetRFReg(priv, (RF90_RADIO_PATH_E)eRFPath, 0x0e, bMask12Bits, 0x021);
			break;
		case HT_CHANNEL_WIDTH_20_40:
			PHY_SetRFReg(priv, (RF90_RADIO_PATH_E)eRFPath, 0x0b, bMask12Bits, 0x300); //phy para:3ba
			PHY_SetRFReg(priv, (RF90_RADIO_PATH_E)eRFPath, 0x2c, bMask12Bits, 0x3ff); // suggested by Bryant 20080125
			PHY_SetRFReg(priv, (RF90_RADIO_PATH_E)eRFPath, 0x0e, bMask12Bits, 0x0e1); // suggested by Bryant 20080125
			break;
		default:
			DEBUG_ERR("SwBWMode(): bandwidth mode error!\n");
			return;
			break;
		}
	}

#elif	defined(RTL8192SE) || defined(RTL8192SU)

/*
	delay_us(100);

	{
		unsigned int val_read;
		val_read = PHY_QueryRFReg(priv, 0, 0x18, bMask12Bits, 1);
		//switch to channel 3
		PHY_SetRFReg(priv, 0, 0x18, bMask12Bits, val_read|0x3);
	}
*/
	if(bandwidth == HT_CHANNEL_WIDTH_20_40)// switch rf to 40Mhz
	{
		unsigned int val_read;

		val_read = PHY_QueryRFReg(priv, 0, 0x18, bMask20Bits, 1);
#ifdef RTL8192SU
		delay_ms(600);
#endif
		val_read &= ~(BIT(10)|BIT(11));
		PHY_SetRFReg(priv, 0, 0x18, bMask20Bits, val_read);
		delay_ms(10);

		val_read = PHY_QueryRFReg(priv, 1, 0x18, bMask20Bits, 1);
		val_read &= ~(BIT(10)|BIT(11));
		PHY_SetRFReg(priv, 1, 0x18, bMask20Bits, val_read);
		delay_ms(10);

	}else{ // 20Mhz mode
		unsigned int val_read;
		val_read = PHY_QueryRFReg(priv, 0, 0x18, bMask20Bits, 1);
#ifdef RTL8192SU
		delay_ms(600);
#endif		
		val_read |= (BIT(10)|BIT(11));
		PHY_SetRFReg(priv, 0, 0x18, bMask20Bits, val_read);

		val_read = PHY_QueryRFReg(priv, 1, 0x18, bMask20Bits, 1);
		val_read |= (BIT(10)|BIT(11));
		PHY_SetRFReg(priv, 1, 0x18, bMask20Bits, val_read);

		PHY_SetBBReg(priv, 0x840,0x0000ffff, 0x7406);
	}
#endif
	if (priv->pshare->rf_ft_var.use_frq_2_3G)
		PHY_SetRFReg(priv, RF90_PATH_C, 0x2c, 0x60, 0);
}


void GetHardwareVersion(struct rtl8190_priv *priv)
{
#if !defined(RTL8192SU)
	ULONG ioaddr = priv->pshare->ioaddr;
#endif	

	if (RTL_R8(0x301) == 0x02)
		priv->pshare->VersionID = VERSION_8190_C;
	else
		priv->pshare->VersionID = VERSION_8190_B;
}


void init_EDCA_para(struct rtl8190_priv *priv, int mode)
{
#if !defined(RTL8192SU)
	static ULONG ioaddr;
#endif	
	static unsigned int slot_time, VO_TXOP, VI_TXOP, sifs_time;
#if !defined(RTL8192SU)
	ioaddr = priv->pshare->ioaddr;
#endif
	slot_time = 20;
	VO_TXOP = 47;
	VI_TXOP = 94;
	sifs_time = 10;

	if (mode & WIRELESS_11N)
		sifs_time = 16;

	if ((mode & WIRELESS_11N) ||
		(mode & WIRELESS_11G)) {
		slot_time = 9;
	}
	else {
		VO_TXOP = 102;
		VI_TXOP = 188;
	}

	RTL_W32(_ACVO_PARM_, (VO_TXOP << 16) | (3 << 12) | (2 << 8) | (sifs_time + ((OPMODE & WIFI_AP_STATE)?1:2) * slot_time));
#ifdef SEMI_QOS
	if (QOS_ENABLE)
		RTL_W32(_ACVI_PARM_, (VI_TXOP << 16) | (4 << 12) | (3 << 8) | (sifs_time + ((OPMODE & WIFI_AP_STATE)?1:2) * slot_time));
	else
#endif
		RTL_W32(_ACVI_PARM_, (10 << 12) | (4 << 8) | (sifs_time + 2 * slot_time));

	RTL_W32(_ACBE_PARM_, (((OPMODE & WIFI_AP_STATE)?6:10) << 12) | (4 << 8) | (sifs_time + 3 * slot_time));

	RTL_W32(_ACBK_PARM_, (10 << 12) | (4 << 8) | (sifs_time + 7 * slot_time));

	RTL_W8(_ACM_CTRL_, 0x00);
}


#ifdef SEMI_QOS
void BE_switch_to_VI(struct rtl8190_priv *priv, int mode, char enable)
{
#if !defined(RTL8192SU)
	ULONG ioaddr = priv->pshare->ioaddr;
#endif	
	unsigned int slot_time = 20, TXOP = 47, sifs_time = 10;

	if ((mode & WIRELESS_11N) && (priv->pshare->ht_sta_num
#ifdef WDS
		|| ((OPMODE & WIFI_AP_STATE) && priv->pmib->dot11WdsInfo.wdsEnabled && priv->pmib->dot11WdsInfo.wdsNum)
#endif
		))
		sifs_time = 16;

	if ((mode & WIRELESS_11N) ||
		(mode & WIRELESS_11G)) {
		slot_time = 9;
	}
	else {
		TXOP = 94;
	}

	if (!enable) {
#if defined(RTL8192SE) || defined(RTL8192SU)
		if (priv->pmib->dot11OperationEntry.wifi_specific == 2) {
			RTL_W16(NAV_PROT_LEN, 0x80);
			RTL_W8(CFEND_TH, 0x2);
			set_fw_reg(priv, 0xfd0001b0, 0, 0);// turn off tx burst
			if (priv->BE_wifi_EDCA_enhance)
				RTL_W32(_ACBE_PARM_, (6 << 12) | (3 << 8) | (sifs_time + 3 * slot_time));
			else
				RTL_W32(_ACBE_PARM_, (6 << 12) | (4 << 8) | (sifs_time + 3 * slot_time));
		}
		else
#endif
			RTL_W32(_ACBE_PARM_, (6 << 12) | (4 << 8) | (sifs_time + 3 * slot_time));
	}
	else {
		if (priv->pshare->ht_sta_num
#ifdef WDS
			|| ((OPMODE & WIFI_AP_STATE) && (mode & WIRELESS_11N) &&
			priv->pmib->dot11WdsInfo.wdsEnabled && priv->pmib->dot11WdsInfo.wdsNum)
#endif
			) {
#if defined(RTL8192SE) || defined(RTL8192SU)
			if (priv->pshare->txop_enlarge == 0xf) {
				// is 8192S client
				RTL_W32(_ACBE_PARM_, ((TXOP*2) << 16) |
							(6 << 12) | (4 << 8) | (sifs_time + slot_time+ 0xf)); // 0xf is 92s circuit delay
				priv->pshare->txop_enlarge = 2;
			}
			else if (priv->pshare->txop_enlarge == 0xe) {
				// is intel client, use a different edca value
				RTL_W32(_ACBE_PARM_, (TXOP << 16) | (6 << 12) | (4 << 8) | 0x2b);
				priv->pshare->txop_enlarge = 1;
			}
			else
#endif
			RTL_W32(_ACBE_PARM_, ((TXOP*priv->pshare->txop_enlarge) << 16) |
			(6 << 12) | (4 << 8) | (sifs_time + slot_time));
		}
		else {
			// for intel 4965 IOT issue with 92SE, temporary solution, victoryman 20081201
#if defined(RTL8190) || defined(RTL8192E)
			RTL_W32(_ACBE_PARM_, ((TXOP*2) << 16) | (4 << 12) | (3 << 8) | (sifs_time + slot_time));
#else	// RTL8192SE
			RTL_W32(_ACBE_PARM_, (6 << 12) | (4 << 8) | 0x19);
#endif
		}

#if defined(RTL8192SE) || defined(RTL8192SU)
		if (priv->pmib->dot11OperationEntry.wifi_specific == 2) {
			RTL_W16(NAV_PROT_LEN, 0x01C0);
			RTL_W8(CFEND_TH, 0xFF);
			set_fw_reg(priv, 0xfd000ab0, 0, 0);
		}
#endif
	}
}
#endif


void setup_timer1(struct rtl8190_priv *priv, int timeout)
{
#if !defined(RTL8192SU)
	ULONG ioaddr = priv->pshare->ioaddr;
#endif	

	RTL_W32(_TIMER1_, timeout);

#if !defined(RTL8192SU)
	RTL_W32(_IMR_, RTL_R32(_IMR_) | _TIMEOUT1_);
#endif
}


void cancel_timer1(struct rtl8190_priv *priv)
{
#if !defined(RTL8192SU)
	ULONG ioaddr = priv->pshare->ioaddr;

	RTL_W32(_IMR_, RTL_R32(_IMR_) & ~_TIMEOUT1_);
#endif
}


void setup_timer2(struct rtl8190_priv *priv, unsigned int timeout)
{
#if !defined(RTL8192SU)
	ULONG ioaddr = priv->pshare->ioaddr;
#endif	
	unsigned int current_value=RTL_R32(_TSFTR_L_);

	if (TSF_LESS(timeout, current_value))
		timeout = current_value+20;

	RTL_W32(_TIMER2_, timeout);
#if !defined(RTL8192SU)	
	RTL_W32(_IMR_, RTL_R32(_IMR_) | _TIMEOUT2_);
#endif
}


void cancel_timer2(struct rtl8190_priv *priv)
{
#if !defined(RTL8192SU)
	ULONG ioaddr = priv->pshare->ioaddr;

	RTL_W32(_IMR_, RTL_R32(_IMR_) & ~_TIMEOUT2_);
#endif
}


// dynamic DC_TH of Fsync in regC38 for non-BCM solution
void check_DC_TH_by_rssi(struct rtl8190_priv *priv, unsigned char rssi_strength)
{
#if !defined(RTL8192SU)
	ULONG ioaddr = priv->pshare->ioaddr;
#endif	

#if defined(RTL8190) || defined(RTL8192E)
	if (!priv->fsync_monitor_pstat)
#endif
	{
		if ((priv->dc_th_current_state != DC_TH_USE_UPPER) &&
			(rssi_strength >= priv->pshare->rf_ft_var.dcThUpper)) {
#if	defined(RTL8192SE) || defined(RTL8192SU)
			RTL_W8(0xc38, 0x94);
#else
			RTL_W8(0xc38, 0x14);
#endif
			priv->dc_th_current_state = DC_TH_USE_UPPER;
		}
		else if ((priv->dc_th_current_state != DC_TH_USE_LOWER) &&
			(rssi_strength <= priv->pshare->rf_ft_var.dcThLower)) {
#if	defined(RTL8192SE) || defined(RTL8192SU)
			RTL_W8(0xc38, 0x90);
#else
			RTL_W8(0xc38, 0x10);
#endif
			priv->dc_th_current_state = DC_TH_USE_LOWER;
		}
		else if (priv->dc_th_current_state == DC_TH_USE_NONE) {
#if	defined(RTL8192SE) || defined(RTL8192SU)
			RTL_W8(0xc38, 0x94);
#else
			RTL_W8(0xc38, 0x14);
#endif
			priv->dc_th_current_state = DC_TH_USE_UPPER;
		}
	}
}


void check_DIG_by_rssi(struct rtl8190_priv *priv, unsigned char rssi_strength)
{
#if !defined(RTL8192SU)
	ULONG ioaddr = priv->pshare->ioaddr;
#endif	
	unsigned int dig_on = 0;
#if defined(CLIENT_MODE) && (defined(RTL8190) || defined(RTL8192E))
	unsigned char val8;
	unsigned int data_delta, mlcst_delta;
#endif

	if (OPMODE & WIFI_SITE_MONITOR)
		return;

#if defined(RTL8190) || defined(RTL8192E)
#ifdef CLIENT_MODE
	if ((OPMODE & (WIFI_STATION_STATE | WIFI_ASOC_STATE)) == (WIFI_STATION_STATE | WIFI_ASOC_STATE))
	{
		data_delta = UINT32_DIFF(priv->rxDataNumInPeriod, priv->rxDataNumInPeriod_pre);
		mlcst_delta = UINT32_DIFF(priv->rxMlcstDataNumInPeriod, priv->rxMlcstDataNumInPeriod_pre);
		priv->rxDataNumInPeriod_pre = priv->rxDataNumInPeriod;
		priv->rxMlcstDataNumInPeriod_pre = priv->rxMlcstDataNumInPeriod;

		if (((mlcst_delta * 10) > (data_delta * 9)) || (priv->pshare->phw->rxmlcst_rssi != 0))
		{
			if ((mlcst_delta * 10) > (data_delta * 9)) {
				if (priv->pshare->phw->rxmlcst_rssi == 0) {
					if (priv->pshare->phw->signal_strength != 0) {
						if (priv->pshare->phw->signal_strength == 1) {
							// set CS_ratio for CCK
							RTL_W8(0xa0a, 0xcd);

							// degrade ED_CCA
							RTL_W32(0xc4c, 0x314);
						}
						RTL_W8(0xc30, 0x44);
						priv->pshare->phw->signal_strength = 0;
					}

					// DIG off
					RTL_W32(_RATR_POLL_, 0x800);
					RTL_W8(0xa09, 0x86);
				}

				if (priv->pshare->phw->rxmlcst_rssi != rssi_strength) {
					priv->pshare->phw->rxmlcst_rssi = rssi_strength;
					val8 = 110 - (100 - rssi_strength) - 15;
					if (val8 > priv->pshare->rf_ft_var.mlcstRxIgUpperBound)
						val8 = priv->pshare->rf_ft_var.mlcstRxIgUpperBound;
					else if (val8 < 0x20)
						val8 = 0x20;

					if (priv->pshare->phw->initial_gain != val8) {
						priv->pshare->phw->initial_gain = val8;

						// set initial gain
						RTL_W8(0xc50, val8);
						RTL_W8(0xc58, val8);
						RTL_W8(0xc60, val8);
						RTL_W8(0xc68, val8);
					}
				}
			}
			else {
				priv->pshare->phw->rxmlcst_rssi = 0;
				priv->pshare->phw->initial_gain = 0;

				// set initial gain
				RTL_W8(0xc50, 0x20);
				RTL_W8(0xc58, 0x20);
				RTL_W8(0xc60, 0x20);
				RTL_W8(0xc68, 0x20);

				RTL_W8(0xa09, 0x83);

				// DIG on
				RTL_W32(_RATR_POLL_, 0x100);
			}

			return;
		}
	}
#endif

	if ((rssi_strength < priv->pshare->rf_ft_var.digGoLowerLevel)
		&& (priv->pshare->phw->signal_strength != 1)) {
		// DIG off
		RTL_W32(_RATR_POLL_, 0x800);

		// set initial gain
		RTL_W8(0xc50, 0x20);
		RTL_W8(0xc58, 0x20);
		RTL_W8(0xc60, 0x20);
		RTL_W8(0xc68, 0x20);

		// degrade PD_TH for OFDM
		if (priv->pshare->is_40m_bw)
			RTL_W8(0xc30, 0x40);
		else
			RTL_W8(0xc30, 0x42);

		// degrade CS_ratio for CCK
		RTL_W8(0xa0a, 0x8);

		// enhance ED_CCA
		RTL_W32(0xc4c, 0x300);

		priv->pshare->phw->signal_strength = 1;
	}
	else if ((rssi_strength > priv->pshare->rf_ft_var.digGoUpperLevel)
		&& (rssi_strength < 71) && (priv->pshare->phw->signal_strength != 2)) {
		// set PD_TH for OFDM
		if (priv->pshare->is_40m_bw)
			RTL_W8(0xc30, 0x42);
		else
			RTL_W8(0xc30, 0x44);

		if (priv->pshare->phw->signal_strength != 3)
			dig_on++;

		priv->pshare->phw->signal_strength = 2;
	}
	else if ((rssi_strength > 75) && (priv->pshare->phw->signal_strength != 3)) {
		// set PD_TH for OFDM
		if (priv->pshare->is_40m_bw)
			RTL_W8(0xc30, 0x41);
		else
			RTL_W8(0xc30, 0x43);

		if (priv->pshare->phw->signal_strength != 2)
			dig_on++;

		priv->pshare->phw->signal_strength = 3;
	}

	if (dig_on) {
		// set initial gain
		RTL_W8(0xc50, 0x20);
		RTL_W8(0xc58, 0x20);
		RTL_W8(0xc60, 0x20);
		RTL_W8(0xc68, 0x20);

		// set CS_ratio for CCK
		RTL_W8(0xa0a, 0xcd);

		// degrade ED_CCA
		RTL_W32(0xc4c, 0x314);

		// DIG on
		RTL_W32(_RATR_POLL_, 0x100);
	}

#else	// RTL8192SE

	if ((rssi_strength > priv->pshare->rf_ft_var.digGoUpperLevel)
		&& (rssi_strength < 71) && (priv->pshare->phw->signal_strength != 2)) {
		if (priv->pshare->is_40m_bw)
			RTL_W8(0xc87, 0x20);
		else
			RTL_W8(0xc30, 0x44);

		if (priv->pshare->phw->signal_strength != 3)
			dig_on++;

		priv->pshare->phw->signal_strength = 2;
	}
	else if ((rssi_strength > 75) && (priv->pshare->phw->signal_strength != 3)) {
		if (priv->pshare->is_40m_bw)
			RTL_W8(0xc87, 0x10);
		else
			RTL_W8(0xc30, 0x43);

		if (priv->pshare->phw->signal_strength != 2)
			dig_on++;

		priv->pshare->phw->signal_strength = 3;
	}
	else if (((rssi_strength < priv->pshare->rf_ft_var.digGoLowerLevel)
		&& (priv->pshare->phw->signal_strength != 1)) || !priv->pshare->phw->signal_strength) {
		// DIG off
//		set_fw_reg(priv, 0xfd000001, 0, 0); //old form of fw
		RTL_W8(0x364, RTL_R8(0x364) & ~FW_REG364_DIG);

		if (priv->pshare->is_40m_bw)
			RTL_W8(0xc87, 0);
		else
			RTL_W8(0xc30, 0x42);

		priv->pshare->phw->signal_strength = 1;
	}

	if (dig_on) {
		// DIG on
//		set_fw_reg(priv, 0xfd000002, 0, 0);
		RTL_W8(0x364, RTL_R8(0x364) | FW_REG364_DIG);
	}
#endif

	check_DC_TH_by_rssi(priv, rssi_strength);
}


void DIG_for_site_survey(struct rtl8190_priv *priv, int do_ss)
{
#if !defined(RTL8192SU)
	ULONG ioaddr = priv->pshare->ioaddr;
#endif	

#if defined(RTL8190) || defined(RTL8192E)
	if (do_ss) {
		if (priv->pshare->phw->rxmlcst_rssi) {
			RTL_W8(0xa09, 0x83);
			priv->pshare->phw->rxmlcst_rssi = 0;
			priv->pshare->phw->initial_gain = 0;
		}
		else if (priv->pshare->phw->signal_strength != 0) {
			if (priv->pshare->phw->signal_strength == 1) {
				// set CS_ratio for CCK
				RTL_W8(0xa0a, 0xcd);

				// degrade ED_CCA
				RTL_W32(0xc4c, 0x314);
			}
			RTL_W8(0xc30, 0x44);
			priv->pshare->phw->signal_strength = 0;
		}

		// DIG off
		RTL_W32(_RATR_POLL_, 0x800);

		// set initial gain
		RTL_W8(0xc50, 0x20);
		RTL_W8(0xc58, 0x20);
		RTL_W8(0xc60, 0x20);
		RTL_W8(0xc68, 0x20);
	}
	else {
		// DIG on
		RTL_W32(_RATR_POLL_, 0x100);
	}
#else	// RTL8192SE
	if (do_ss) {
		if (priv->pshare->phw->signal_strength) {
			if (priv->pshare->is_40m_bw)
				RTL_W8(0xc87, 0x20);
			else
				RTL_W8(0xc30, 0x44);
			priv->pshare->phw->signal_strength = 0;
		}

		// DIG off
		//set_fw_reg(priv, 0xfd000001, 0, 0);
		RTL_W8(0x364, RTL_R8(0x364) & ~FW_REG364_DIG);
	}
	else {
		// let check_DIG_by_rssi() re-init DIG state again
		// and do nothing here
#if 0
		// DIG on
//		set_fw_reg(priv, 0xfd000002, 0, 0);
		RTL_W8(0x364, RTL_R8(0x364) | FW_REG364_DIG);
#endif
	}
#endif
}

#if defined(RTL8190)
// dynamic CCK Tx power by rssi
void CCK_txpower_by_rssi(struct rtl8190_priv *priv, unsigned char rssi_strength)
{
	ULONG ioaddr = priv->pshare->ioaddr;
	unsigned int val32;

	if (priv->pshare->phw->CCKTxAgc_enhanced) {
		if (rssi_strength > priv->pshare->rf_ft_var.digGoUpperLevel) {
			priv->pshare->phw->CCKTxAgc_enhanced = 0;
			RTL_W32(_CCK_TXAGC_, priv->pshare->phw->CCKTxAgc);
			DEBUG_INFO("Return CCK Tx power to 0x%08x\n", priv->pshare->phw->CCKTxAgc);
		}
	}
	else {
		if (rssi_strength < priv->pshare->rf_ft_var.digGoLowerLevel) {
			priv->pshare->phw->CCKTxAgc_enhanced = 1;
			if (priv->pshare->rf_ft_var.cck_enhance == 1) {
				if (priv->pshare->channelAC_pwrdiff <= 2)
					val32 = (0x22 << 8) | 0x22;
				else
					val32 = ((0x24 - priv->pshare->channelAC_pwrdiff) << 8) | (0x24 - priv->pshare->channelAC_pwrdiff);
			}
			else {
				if (priv->pshare->rf_ft_var.cck_enhance == 6)
					val32 = (priv->pshare->phw->CCKTxAgc & 0xff) + 6;
				else if (priv->pshare->rf_ft_var.cck_enhance == 8)
					val32 = (priv->pshare->phw->CCKTxAgc & 0xff) + 8;
				else if (priv->pshare->rf_ft_var.cck_enhance == 10)
					val32 = (priv->pshare->phw->CCKTxAgc & 0xff) + 10;
				else {
					printk("cck_enhance=%d not acceptable! set to 6\n", priv->pshare->rf_ft_var.cck_enhance);
					priv->pshare->rf_ft_var.cck_enhance = 6;
					val32 = (priv->pshare->phw->CCKTxAgc & 0xff) + 6;
				}
				if (val32 > 0x22)
					val32 = 0x22;
				if ((priv->pshare->channelAC_pwrdiff > 2) && (val32 == 0x22))
					val32 = 0x24 - priv->pshare->channelAC_pwrdiff;
				val32 = (val32 << 8) | val32;
			}
			RTL_W32(_CCK_TXAGC_, val32);
			DEBUG_INFO("Enchance CCK Tx power to 0x%08x\n", val32);
		}
	}
}
#endif


#if defined(RTL8192SE) || defined(RTL8192SU)
// dynamic CCK CCA enhance by rssi
void CCK_CCA_dynamic_enhance(struct rtl8190_priv *priv, unsigned char rssi_strength)
{
#if !defined(RTL8192SU)
	unsigned long ioaddr = priv->pshare->ioaddr;
#endif

	if (!priv->pshare->phw->CCK_CCA_enhanced && (rssi_strength < 20)) {
		priv->pshare->phw->CCK_CCA_enhanced = TRUE;
		RTL_W8(0xa0a, 0x83);
	}
	else if (priv->pshare->phw->CCK_CCA_enhanced && (rssi_strength > 25)) {
		priv->pshare->phw->CCK_CCA_enhanced = FALSE;
		RTL_W8(0xa0a, 0xcd);
	}
}
#endif


// bcm old 11n chipset iot debug
#if defined(RTL8190) || defined(RTL8192E)
void fsync_refine_switch(struct rtl8190_priv *priv, unsigned int refine_on)
{
	ULONG ioaddr = priv->pshare->ioaddr;

	if (refine_on) {
		// late
#ifdef RTL8190
		RTL_W8(0xc36, 0x00);
#elif defined(RTL8192E)
		RTL_W8(0xc36, 0x14);
#endif
		RTL_W8(0xc3e, 0x90);
	}
	else {
		// default
#ifdef RTL8190
		RTL_W8(0xc36, 0x40);
#elif defined(RTL8192E)
		RTL_W8(0xc36, 0x54);
#endif
		RTL_W8(0xc3e, 0x96);
	}
}


// bcm old 11n chipset iot debug
void rtl8190_fsync_timer(unsigned long task_priv)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)task_priv;
	if (timer_pending(&priv->fsync_timer))
		del_timer_sync(&priv->fsync_timer);

	if ((priv->pshare->rf_ft_var.fsync_func_on) && (priv->fsync_monitor_pstat)) {
		if ((priv->fsync_monitor_pstat->rssi > priv->pshare->rf_ft_var.fsync_rssi_th) &&
			((!(priv->fsync_monitor_pstat->rx_rate_bitmap)) ||
			(!(priv->fsync_monitor_pstat->rx_rate_bitmap >> (priv->pshare->rf_ft_var.fsync_mcs_th+1))))) {
			if (priv->pshare->fsync_refine_on)
				priv->pshare->fsync_refine_on = 0;
			else
				priv->pshare->fsync_refine_on++;
			fsync_refine_switch(priv, priv->pshare->fsync_refine_on);
		}
		else if (priv->fsync_monitor_pstat->rssi <= priv->pshare->rf_ft_var.fsync_rssi_th) {
			if (priv->pshare->fsync_refine_on) {
				priv->pshare->fsync_refine_on = 0;
				fsync_refine_switch(priv, priv->pshare->fsync_refine_on);
			}
		}

		mod_timer(&priv->fsync_timer, jiffies + 50);	// 500 ms
	}
	else {
		if (priv->pshare->fsync_refine_on) {
			priv->pshare->fsync_refine_on = 0;
			fsync_refine_switch(priv, priv->pshare->fsync_refine_on);
		}
	}

	if (priv->fsync_monitor_pstat)
		priv->fsync_monitor_pstat->rx_rate_bitmap = 0;
}
#endif

#if defined(RTL8192SE) || defined(RTL8192SU)
void tx_path_by_rssi(struct rtl8190_priv *priv, struct stat_info *pstat, unsigned char enable){

	if((get_rf_mimo_mode(priv) != MIMO_2T2R))
		return; // 1T2R, 1T1R; do nothing

	if(pstat == NULL)
		return;

#ifdef	STA_EXT
	if ((pstat->remapped_aid == FW_NUM_STAT-1) || 
		(priv->pshare->has_2r_sta & BIT(pstat->remapped_aid)))// 2r STA
#else
	if (priv->pshare->has_2r_sta & BIT(pstat->aid))// 2r STA
#endif
		return; // do nothing

	// for debug, by victoryman 20090623
	if (pstat->tx_ra_bitmap & 0xff00000) {
		// this should be a 2r station!!!
		return;
	}

	if (pstat->tx_ra_bitmap & 0xffff000){// 11n 1R client
		if(enable){
			if(pstat->rf_info.mimorssi[0] > pstat->rf_info.mimorssi[1])
				Switch_1SS_Antenna(priv, 1);
			else
				Switch_1SS_Antenna(priv, 2);
		}
		else
			Switch_1SS_Antenna(priv, 3);
  }
	else if (pstat->tx_ra_bitmap & 0xff0){// 11bg client
		if(enable){
			if(pstat->rf_info.mimorssi[0] > pstat->rf_info.mimorssi[1])
				Switch_OFDM_Antenna(priv, 1);
			else
				Switch_OFDM_Antenna(priv, 2);
		}
		else
			Switch_OFDM_Antenna(priv, 3);
  }

#if 0  // original  setup
	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N){ // for 11n 1ss sta
		if(enable){
			if(pstat->rf_info.mimorssi[0] > pstat->rf_info.mimorssi[1])
				Switch_1SS_Antenna(priv, 1);
			else
				Switch_1SS_Antenna(priv, 2);
		}
		else
			Switch_1SS_Antenna(priv, 3);
	}
	else if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G){ // for 11g
		if(enable){
			if(pstat->rf_info.mimorssi[0] > pstat->rf_info.mimorssi[1])
				Switch_OFDM_Antenna(priv, 1);
			else
				Switch_OFDM_Antenna(priv, 2);
		}
		else
			Switch_OFDM_Antenna(priv, 3);
	}
#endif


}
#endif


// dynamic Rx path selection by signal strength
void rx_path_by_rssi(struct rtl8190_priv *priv, struct stat_info *pstat, int enable)
{
#if !defined(RTL8192SU)
	ULONG ioaddr = priv->pshare->ioaddr;
#endif	
	unsigned char highest_rssi=0, higher_rssi=0, under_ss_th_low=0;
	RF90_RADIO_PATH_E eRFPath, eRFPath_highest=0, eRFPath_higher=0;
	int ant_on_processing=0;
#ifdef _DEBUG_RTL8190_
	char path_name[] = {'A', 'B', 'C', 'D'};
#endif

	if (enable == FALSE) {
		if (priv->pshare->phw->ant_off_num) {
			priv->pshare->phw->ant_off_num = 0;
			priv->pshare->phw->ant_off_bitmap = 0;
			RTL_W8(rOFDM0_TRxPathEnable, 0x0f);
			RTL_W8(rOFDM1_TRxPathEnable, 0x0f);
			DEBUG_INFO("More than 1 sta, turn on all path\n");
		}
		return;
	}

	for (eRFPath = RF90_PATH_A; eRFPath<priv->pshare->phw->NumTotalRFPath; eRFPath++) {
		if (priv->pshare->phw->ant_off_bitmap & BIT(eRFPath))
			continue;

		if (pstat->rf_info.mimorssi[eRFPath] > highest_rssi) {
			higher_rssi = highest_rssi;
			eRFPath_higher = eRFPath_highest;
			highest_rssi = pstat->rf_info.mimorssi[eRFPath];
			eRFPath_highest = eRFPath;
		}

		else if (pstat->rf_info.mimorssi[eRFPath] > higher_rssi) {
			higher_rssi = pstat->rf_info.mimorssi[eRFPath];
			eRFPath_higher = eRFPath;
		}

		if (pstat->rf_info.mimorssi[eRFPath] < priv->pshare->rf_ft_var.ss_th_low)
			under_ss_th_low = 1;
	}

	// for OFDM
	if (priv->pshare->phw->ant_off_num > 0)
	{
		for (eRFPath = RF90_PATH_A; eRFPath<priv->pshare->phw->NumTotalRFPath; eRFPath++) {
			if (!(priv->pshare->phw->ant_off_bitmap & BIT(eRFPath)))
				continue;

			if (highest_rssi >= priv->pshare->phw->ant_on_criteria[eRFPath]) {
				priv->pshare->phw->ant_off_num--;
				priv->pshare->phw->ant_off_bitmap &= (~BIT(eRFPath));
				RTL_W8(rOFDM0_TRxPathEnable, ~(priv->pshare->phw->ant_off_bitmap) & 0x0f);
				RTL_W8(rOFDM1_TRxPathEnable, ~(priv->pshare->phw->ant_off_bitmap) & 0x0f);
				DEBUG_INFO("Path %c is on due to >= %d%%\n",
					path_name[eRFPath], priv->pshare->phw->ant_on_criteria[eRFPath]);
				ant_on_processing = 1;
			}
		}
	}

	if (!ant_on_processing)
	{
		if (priv->pshare->phw->ant_off_num < 2) {
			for (eRFPath = RF90_PATH_A; eRFPath<priv->pshare->phw->NumTotalRFPath; eRFPath++) {
				if ((eRFPath == eRFPath_highest) || (priv->pshare->phw->ant_off_bitmap & BIT(eRFPath)))
					continue;

				if ((pstat->rf_info.mimorssi[eRFPath] < priv->pshare->rf_ft_var.ss_th_low) &&
					((highest_rssi - pstat->rf_info.mimorssi[eRFPath]) > priv->pshare->rf_ft_var.diff_th)) {
					priv->pshare->phw->ant_off_num++;
					priv->pshare->phw->ant_off_bitmap |= BIT(eRFPath);
					priv->pshare->phw->ant_on_criteria[eRFPath] = highest_rssi + 5;
					RTL_W8(rOFDM0_TRxPathEnable, ~(priv->pshare->phw->ant_off_bitmap) & 0x0f);
					RTL_W8(rOFDM1_TRxPathEnable, ~(priv->pshare->phw->ant_off_bitmap) & 0x0f);
					DEBUG_INFO("Path %c is off due to under th_low %d%% and diff %d%%, will be on at %d%%\n",
						path_name[eRFPath], priv->pshare->rf_ft_var.ss_th_low,
						(highest_rssi - pstat->rf_info.mimorssi[eRFPath]),
						priv->pshare->phw->ant_on_criteria[eRFPath]);
					break;
				}
			}
		}
	}

	// For CCK
	if (priv->pshare->rf_ft_var.cck_sel_ver == 1) {
		if (under_ss_th_low && (pstat->rx_pkts > 20)) {
			if (priv->pshare->phw->ant_cck_sel != ((eRFPath_highest << 2) | eRFPath_higher)) {
				priv->pshare->phw->ant_cck_sel = ((eRFPath_highest << 2) | eRFPath_higher);
				RTL_W8(0xa07, (RTL_R8(0xa07) & 0xf0) | priv->pshare->phw->ant_cck_sel);
				DEBUG_INFO("CCK select default: path %c, optional: path %c\n",
					path_name[eRFPath_highest], path_name[eRFPath_higher]);
			}
		}
	}
}


// dynamic Rx path selection by signal strength
void rx_path_by_rssi_cck_v2(struct rtl8190_priv *priv, struct stat_info *pstat)
{
#if !defined(RTL8192SU)
	ULONG ioaddr = priv->pshare->ioaddr;
#endif	
	int highest_rssi=-1000, higher_rssi=-1000;
	RF90_RADIO_PATH_E eRFPath, eRFPath_highest=0, eRFPath_higher=0;
#ifdef _DEBUG_RTL8190_
	char path_name[] = {'A', 'B', 'C', 'D'};
#endif

	for (eRFPath = RF90_PATH_A; eRFPath<priv->pshare->phw->NumTotalRFPath; eRFPath++) {
		if (pstat->cck_mimorssi_total[eRFPath] > highest_rssi) {
			higher_rssi = highest_rssi;
			eRFPath_higher = eRFPath_highest;
			highest_rssi = pstat->cck_mimorssi_total[eRFPath];
			eRFPath_highest = eRFPath;
		}

		else if (pstat->cck_mimorssi_total[eRFPath] > higher_rssi) {
			higher_rssi = pstat->cck_mimorssi_total[eRFPath];
			eRFPath_higher = eRFPath;
		}
	}

	if (priv->pshare->phw->ant_cck_sel != ((eRFPath_highest << 2) | eRFPath_higher)) {
		priv->pshare->phw->ant_cck_sel = ((eRFPath_highest << 2) | eRFPath_higher);
		RTL_W8(0xa07, (RTL_R8(0xa07) & 0xf0) | priv->pshare->phw->ant_cck_sel);
		DEBUG_INFO("CCK rssi A:%d B:%d C:%d D:%d accu %d pkts\n", pstat->cck_mimorssi_total[0],
			pstat->cck_mimorssi_total[1], pstat->cck_mimorssi_total[2], pstat->cck_mimorssi_total[3], pstat->cck_rssi_num);
		DEBUG_INFO("CCK select default: path %c, optional: path %c\n",
			path_name[eRFPath_highest], path_name[eRFPath_higher]);
	}
}


// Tx power control
void tx_power_control(struct rtl8190_priv *priv, struct stat_info *pstat, int enable)
{
#if defined(RTL8190) || defined(RTL8192E)
#ifdef RTL8190
	ULONG ioaddr = priv->pshare->ioaddr;
#endif

	if (enable == FALSE) {
		if (priv->pshare->phw->lower_tx_power) {
			priv->pshare->phw->lower_tx_power = 0;
#ifdef RTL8190
			RTL_W32(_MCS_TXAGC_1_, priv->pshare->phw->MCSTxAgc1);
#elif defined(RTL8192E)
			PHY_SetBBReg(priv, 0xe14, 0x7f7f7f7f,  priv->pshare->phw->MCSTxAgc1);
			PHY_SetBBReg(priv, 0xe1c, 0x7f7f7f7f,  priv->pshare->phw->MCSTxAgc1);
#endif
			DEBUG_INFO("More than 1 sta, back to normal power state\n");
		}
		return;
	}

	if (priv->pshare->phw->lower_tx_power == 0) {
		if (pstat->rssi > 75) {
			priv->pshare->phw->lower_tx_power = 1;
#ifdef RTL8190
			RTL_W32(_MCS_TXAGC_1_, 0x03030303);
#elif defined(RTL8192E)
			PHY_SetBBReg(priv, 0xe14, 0x7f7f7f7f,  0x03030303);
			PHY_SetBBReg(priv, 0xe1c, 0x7f7f7f7f,  0x03030303);
#endif
			DEBUG_INFO("Rssi > 75, enter high power state\n");
		}
	}
	else {
		if (pstat->rssi < 70) {
			priv->pshare->phw->lower_tx_power = 0;
#ifdef RTL8190
			RTL_W32(_MCS_TXAGC_1_, priv->pshare->phw->MCSTxAgc1);
#elif defined(RTL8192E)
			PHY_SetBBReg(priv, 0xe14, 0x7f7f7f7f,  priv->pshare->phw->MCSTxAgc1);
			PHY_SetBBReg(priv, 0xe1c, 0x7f7f7f7f,  priv->pshare->phw->MCSTxAgc1);
#endif
			DEBUG_INFO("Rssi < 70, back to normal power state\n");
		}
	}
#else	// RTL8192SE
#if !defined(RTL8192SU)
	ULONG ioaddr = priv->pshare->ioaddr;
#endif	
	if (enable) {
		if (!priv->pshare->phw->lower_tx_power) {
			// TX High power enable
//			set_fw_reg(priv, 0xfd000009, 0, 0);
			RTL_W8(0x364, RTL_R8(0x364) | FW_REG364_HP);
			priv->pshare->phw->lower_tx_power++;
		}
	}
	else {
		if (priv->pshare->phw->lower_tx_power) {
			//TX High power disable
//			set_fw_reg(priv, 0xfd000008, 0, 0);
			RTL_W8(0x364, RTL_R8(0x364) & ~FW_REG364_HP);
			priv->pshare->phw->lower_tx_power = 0;
		}
	}
#endif
}


void tx_power_tracking(struct rtl8190_priv *priv)
{
#if defined(RTL8190) || defined(RTL8192E)
	ULONG ioaddr = priv->pshare->ioaddr;
	unsigned int regval;
	int i;

	if (!priv->pshare->phw->tpt_inited)
	{
		regval = PHY_QueryBBReg(priv, rOFDM0_XCTxIQImbalance, bMaskDWord);
		for (i=0; i<TxPwrTrk_OFDM_SwingTbl_Len; i++) {
			if (regval == TxPwrTrk_OFDM_SwingTbl[i]) {
				priv->pshare->phw->tpt_ofdm_swing_idx = (unsigned char)i;
				break;
			}
		}

		priv->pshare->phw->tpt_cck_swing_idx = get_cck_swing_idx(priv->pshare->CurrentChannelBW, priv->pshare->phw->tpt_ofdm_swing_idx);

		DEBUG_INFO("TPT: started, target_tssi=%d, ofdm_swing_idx=%d, cck_swing_idx=%d\n",
			priv->pmib->dot11RFEntry.ther_rfic, priv->pshare->phw->tpt_ofdm_swing_idx, priv->pshare->phw->tpt_cck_swing_idx);

		priv->pshare->phw->tpt_inited = TRUE;
	}

	if (priv->pshare->phw->tpt_tracking_num == 0) {
		priv->pshare->phw->tpt_tssi_total = 0;
		priv->pshare->phw->tpt_tssi_num = 0;
		if (priv->pshare->phw->lower_tx_power) {
			RTL_W32(_MCS_TXAGC_1_, priv->pshare->phw->MCSTxAgc1);
			DEBUG_INFO("TPT: back to normal power state\n");
		}
	}

	if (priv->pshare->phw->tpt_tracking_num < 15) {
		priv->pshare->phw->tpt_tracking_num++;
		issue_tpt_tstpkt(priv);
		mod_timer(&priv->pshare->phw->tpt_timer, jiffies + 1);
	}
	else {
		DEBUG_INFO("TPT: too busy, ofdm_swing_idx=%d, cck_swing_idx=%d\n",
			priv->pshare->phw->tpt_ofdm_swing_idx,  priv->pshare->phw->tpt_cck_swing_idx);
		priv->pshare->phw->tpt_tracking_num = 0;
		if (priv->pshare->phw->lower_tx_power) {
			RTL_W32(_MCS_TXAGC_1_, 0x03030303);
			DEBUG_INFO("TPT: enter high power state\n");
		}
	}

#elif defined(RTL8192SE) || defined(RTL8192SU)

	if (priv->pmib->dot11RFEntry.ther) {
		DEBUG_INFO("TPT: triggered(every %d seconds)\n", priv->pshare->rf_ft_var.tpt_period);

		// enable rf reg 0x24 power and trigger, to get ther value in 1 second
		PHY_SetRFReg(priv, RF90_PATH_A, 0x24, bMask20Bits, 0x60);
		mod_timer(&priv->pshare->phw->tpt_timer, jiffies + 100); // 1000ms
	}
#endif
}


void rtl8190_tpt_timer(unsigned long task_priv)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)task_priv;
	unsigned int val32;

#if defined(RTL8190) || defined(RTL8192E)
	ULONG ioaddr = priv->pshare->ioaddr;
	unsigned short tssi, target_tssi=priv->pmib->dot11RFEntry.ther_rfic;
	unsigned char pwr_flag, *CCK_SwingEntry;

	pwr_flag = RTL_R8(0x136);
	if (pwr_flag == 0) {
		if (priv->pshare->phw->tpt_tssi_waiting < 20) {
			priv->pshare->phw->tpt_tssi_waiting++;
			mod_timer(&priv->pshare->phw->tpt_timer, jiffies + 1);
			return;
		}
		else {
			DEBUG_INFO("TPT: waiting too long\n");
			priv->pshare->phw->tpt_tssi_waiting = 0;
			tx_power_tracking(priv);
			return;
		}
	}

	priv->pshare->phw->tpt_tssi_waiting = 0;
	tssi = RTL_R16(0x134) / 100;
	RTL_W8(0x136, 0);
	DEBUG_INFO("TPT: tssi = %d (%d)\n", tssi, priv->pshare->phw->tpt_tssi_num);
	if (tssi == 0) {
		tx_power_tracking(priv);
		return;
	}
	else {
		priv->pshare->phw->tpt_tssi_total += tssi;
		priv->pshare->phw->tpt_tssi_num++;

		if (priv->pshare->phw->tpt_tssi_num < 3) {
			tx_power_tracking(priv);
			return;
		}
		else
			tssi = priv->pshare->phw->tpt_tssi_total / 3;
	}

	if (tssi >= target_tssi) {
		if ((tssi - target_tssi) <= TxPwrTrk_E_Val) {
			DEBUG_INFO("TPT: done, ofdm_swing_idx=%d, cck_swing_idx=%d\n",
				priv->pshare->phw->tpt_ofdm_swing_idx,  priv->pshare->phw->tpt_cck_swing_idx);
			priv->pshare->phw->tpt_tracking_num = 0;
			if (priv->pshare->phw->lower_tx_power) {
				RTL_W32(_MCS_TXAGC_1_, 0x03030303);
				DEBUG_INFO("TPT: enter high power state\n");
			}
			return;
		}
		else {
			if (priv->pshare->phw->tpt_ofdm_swing_idx < (TxPwrTrk_OFDM_SwingTbl_Len-1)) {
				priv->pshare->phw->tpt_ofdm_swing_idx++;
				priv->pshare->phw->tpt_cck_swing_idx = get_cck_swing_idx(priv->pshare->CurrentChannelBW, priv->pshare->phw->tpt_ofdm_swing_idx);
			}
			else {
				DEBUG_INFO("TPT: limited, ofdm_swing_idx=%d, cck_swing_idx=%d\n",
					priv->pshare->phw->tpt_ofdm_swing_idx,  priv->pshare->phw->tpt_cck_swing_idx);
				priv->pshare->phw->tpt_tracking_num = 0;
				if (priv->pshare->phw->lower_tx_power) {
					RTL_W32(_MCS_TXAGC_1_, 0x03030303);
					DEBUG_INFO("TPT: enter high power state\n");
				}
				return;
			}
		}
	}
	else {
		if ((target_tssi - tssi) <= TxPwrTrk_E_Val) {
			DEBUG_INFO("TPT: done, ofdm_swing_idx=%d, cck_swing_idx=%d\n",
				priv->pshare->phw->tpt_ofdm_swing_idx,  priv->pshare->phw->tpt_cck_swing_idx);
			priv->pshare->phw->tpt_tracking_num = 0;
			if (priv->pshare->phw->lower_tx_power) {
				RTL_W32(_MCS_TXAGC_1_, 0x03030303);
				DEBUG_INFO("TPT: enter high power state\n");
			}
			return;
		}
		else {
			if (priv->pshare->phw->tpt_ofdm_swing_idx > 0) {
				priv->pshare->phw->tpt_ofdm_swing_idx--;
				priv->pshare->phw->tpt_cck_swing_idx = get_cck_swing_idx(priv->pshare->CurrentChannelBW, priv->pshare->phw->tpt_ofdm_swing_idx);
			}
			else {
				DEBUG_INFO("TPT: limited, ofdm_swing_idx=%d, cck_swing_idx=%d\n",
					priv->pshare->phw->tpt_ofdm_swing_idx,  priv->pshare->phw->tpt_cck_swing_idx);
				priv->pshare->phw->tpt_tracking_num = 0;
				if (priv->pshare->phw->lower_tx_power) {
					RTL_W32(_MCS_TXAGC_1_, 0x03030303);
					DEBUG_INFO("TPT: enter high power state\n");
				}
				return;
			}
		}
	}

	val32 = TxPwrTrk_OFDM_SwingTbl[priv->pshare->phw->tpt_ofdm_swing_idx];
	PHY_SetBBReg(priv, rOFDM0_XATxIQImbalance, bMaskDWord, val32);
	PHY_SetBBReg(priv, rOFDM0_XBTxIQImbalance, bMaskDWord, val32);
	PHY_SetBBReg(priv, rOFDM0_XCTxIQImbalance, bMaskDWord, val32);
	PHY_SetBBReg(priv, rOFDM0_XDTxIQImbalance, bMaskDWord, val32);
	if (priv->pmib->dot11RFEntry.dot11channel == 14)
		CCK_SwingEntry = &TxPwrTrk_CCK_SwingTbl_CH14[priv->pshare->phw->tpt_cck_swing_idx][0];
	else
		CCK_SwingEntry = &TxPwrTrk_CCK_SwingTbl[priv->pshare->phw->tpt_cck_swing_idx][0];
	val32 = CCK_SwingEntry[0] | (CCK_SwingEntry[1] << 8);
	PHY_SetBBReg(priv, rCCK0_TxFilter1, bMaskHWord, val32);
	val32 = CCK_SwingEntry[2] | (CCK_SwingEntry[3] << 8) | (CCK_SwingEntry[4] << 16) | (CCK_SwingEntry[5] << 24);
	PHY_SetBBReg(priv, rCCK0_TxFilter2, bMaskDWord, val32);
	val32 = CCK_SwingEntry[6] | (CCK_SwingEntry[7] << 8);
	PHY_SetBBReg(priv, rCCK0_DebugPort, bMaskLWord, val32);

	DEBUG_INFO("TPT: finished once\n");

	priv->pshare->phw->tpt_tssi_total = 0;
	priv->pshare->phw->tpt_tssi_num = 0;
	tx_power_tracking(priv);

#elif defined(RTL8192SE) || defined(RTL8192SU)

	if (timer_pending(&priv->pshare->phw->tpt_timer))
		del_timer_sync(&priv->pshare->phw->tpt_timer);

	if (priv->pmib->dot11RFEntry.ther) {
		// query rf reg 0x24[4:0], for thermal meter value
		val32 = PHY_QueryRFReg(priv, RF90_PATH_A, 0x24, bMask20Bits, 1) & 0x01f;

		if (val32) {
			set_fw_reg(priv, 0xfd000019|(priv->pmib->dot11RFEntry.ther & 0xff)<<8|val32<<16, 0, 0);
			DEBUG_INFO("TPT: finished once (ther: current=0x%02x, target=0x%02x)\n",
				val32, priv->pmib->dot11RFEntry.ther);
		}
		else{
			DEBUG_WARN("TPT: cannot finish, since wrong current ther value report\n");
		}
	}
#endif
}




/*
 *
 * Move from 8190n_cam.c to here for open source consideration
 *
 */

#define  WritePortUlong  RTL_W32
#define  WritePortUshort  RTL_W16
#define  WritePortUchar  RTL_W8

#define ReadPortUchar(offset,value)	do{*value=RTL_R8(offset);}while(0)
#define ReadPortUshort(offset,value)	do{*value=RTL_R16(offset);}while(0)
#define ReadPortUlong(offset,value)	do{*value=RTL_R32(offset);}while(0)

/*******************************************************/
/*CAM related utility                                  */
/*CamAddOneEntry                                       */
/*CamDeleteOneEntry                                    */
/*CamResetAllEntry                                     */
/*******************************************************/
#define TOTAL_CAM_ENTRY 32

#define CAM_CONTENT_COUNT 8
#define CAM_CONTENT_USABLE_COUNT 6

#define CFG_VALID        BIT(15)


//return first not invalid entry back.
static UCHAR CAM_find_usable(struct rtl8190_priv *priv)
{
#if !defined(RTL8192SU)
	ULONG ioaddr = priv->pshare->ioaddr;
#endif	
	ULONG ulCommand=0;
	ULONG ulContent=0;
	UCHAR ucIndex;
	int for_begin = 4;

	for(ucIndex=for_begin; ucIndex<TOTAL_CAM_ENTRY; ucIndex++) {
		// polling bit, and No Write enable, and address
		ulCommand= CAM_CONTENT_COUNT*ucIndex;
		WritePortUlong(_CAMCMD_, (_CAM_POLL_| ulCommand));

	   	//Check polling bit is clear
		while(1) {
			ReadPortUlong(_CAMCMD_, &ulCommand);
			if(ulCommand & _CAM_POLL_)
				continue;
			else
				break;
		}
		ReadPortUlong(_CAM_R_, &ulContent);

		//check valid bit. if not valid,
		if((ulContent & CFG_VALID)==0) {
			return ucIndex;
		}
	}
	return TOTAL_CAM_ENTRY;
}


static void CAM_program_entry(struct rtl8190_priv *priv,UCHAR index, UCHAR* macad,UCHAR* key128, USHORT config)
{
#if !defined(RTL8192SU)
	ULONG ioaddr = priv->pshare->ioaddr;
#endif	
	ULONG target_command=0;
	ULONG target_content=0;
	UCHAR entry_i=0;
	struct stat_info	*pstat;

	for(entry_i=0; entry_i<CAM_CONTENT_USABLE_COUNT; entry_i++)
	{
		// polling bit, and write enable, and address
		target_command= entry_i+CAM_CONTENT_COUNT*index;
		target_command= target_command |_CAM_POLL_ | _CAM_WE_;
		if(entry_i == 0) {
		    //first 32-bit is MAC address and CFG field
		    target_content= (ULONG)(*(macad+0))<<16
							|(ULONG)(*(macad+1))<<24
							|(ULONG)config;
		    target_content=target_content|config;
	    }
		else if(entry_i == 1) {
			//second 32-bit is MAC address
			target_content= (ULONG)(*(macad+5))<<24
							|(ULONG)(*(macad+4))<<16
							|(ULONG)(*(macad+3))<<8
							|(ULONG)(*(macad+2));
		}
		else {
			target_content= (ULONG)(*(key128+(entry_i*4-8)+3))<<24
							|(ULONG)(*(key128+(entry_i*4-8)+2))<<16
							|(ULONG)(*(key128+(entry_i*4-8)+1))<<8
							|(ULONG)(*(key128+(entry_i*4-8)+0));
		}

		WritePortUlong(_CAM_W_, target_content);
		WritePortUlong(_CAMCMD_, target_command);
	}

	pstat = get_stainfo(priv, macad);
	if (pstat) {
		pstat->cam_id = index;
	}
}


int CamAddOneEntry(struct rtl8190_priv *priv,UCHAR *pucMacAddr, ULONG ulKeyId, ULONG ulEncAlg, ULONG ulUseDK, UCHAR *pucKey)
{
	UCHAR retVal = 0;
    UCHAR ucCamIndex = 0;
    USHORT usConfig = 0;
#if !defined(RTL8192SU)    
	ULONG ioaddr = priv->pshare->ioaddr;
#endif	
	UCHAR wpaContent = 0;

    //use Hardware Polling to check the valid bit.
    //in reality it should be done by software link-list
	if ((!memcmp(pucMacAddr, "\xff\xff\xff\xff\xff\xff", 6)) || (ulUseDK))
		ucCamIndex = ulKeyId;
	else
		ucCamIndex = CAM_find_usable(priv);

    if(ucCamIndex==TOTAL_CAM_ENTRY)
    	return retVal;

	usConfig=usConfig|CFG_VALID|((USHORT)(ulEncAlg))|(UCHAR)ulKeyId;

    CAM_program_entry(priv,ucCamIndex,pucMacAddr,pucKey,usConfig);

	if (priv->pshare->CamEntryOccupied == 0) {
		if (ulUseDK == 1)
			wpaContent =  _RX_USE_DK_ | _TX_USE_DK_;
		RTL_W8(_WPACFG_, RTL_R8(_WPACFG_) | _RX_DEC_ | _TX_ENC_ | wpaContent);
	}

    return 1;
}


void CAM_read_mac_config(struct rtl8190_priv *priv,UCHAR ucIndex, UCHAR* pucMacad, USHORT* pusTempConfig)
{
#if !defined(RTL8192SU)
	ULONG ioaddr = priv->pshare->ioaddr;
#endif	
	ULONG ulCommand=0;
	ULONG ulContent=0;

	// polling bit, and No Write enable, and address
	// cam address...
	// first 32-bit
	ulCommand= CAM_CONTENT_COUNT*ucIndex+0;
	ulCommand= ulCommand | _CAM_POLL_;
	WritePortUlong(_CAMCMD_, ulCommand);

   	//Check polling bit is clear
	while(1) {
		ReadPortUlong(_CAMCMD_, &ulCommand);
		if(ulCommand & _CAM_POLL_)
			continue;
		else
			break;
	}
	ReadPortUlong(_CAM_R_, &ulContent);

	//first 32-bit is MAC address and CFG field
	*(pucMacad+0)= (UCHAR)((ulContent>>16)&0x000000FF);
	*(pucMacad+1)= (UCHAR)((ulContent>>24)&0x000000FF);
	*pusTempConfig  = (USHORT)(ulContent&0x0000FFFF);

	ulCommand= CAM_CONTENT_COUNT*ucIndex+1;
	ulCommand= ulCommand | _CAM_POLL_;
	WritePortUlong(_CAMCMD_, ulCommand);

   	//Check polling bit is clear
	while(1) {
		ReadPortUlong(_CAMCMD_, &ulCommand);
		if(ulCommand & _CAM_POLL_)
			continue;
		else
			break;
	}
	ReadPortUlong(_CAM_R_, &ulContent);

	*(pucMacad+5)= (UCHAR)((ulContent>>24)&0x000000FF);
	*(pucMacad+4)= (UCHAR)((ulContent>>16)&0x000000FF);
	*(pucMacad+3)= (UCHAR)((ulContent>>8)&0x000000FF);
	*(pucMacad+2)= (UCHAR)((ulContent)&0x000000FF);
}


#if 0
void CAM_mark_invalid(struct rtl8190_priv *priv,UCHAR ucIndex)
{
	ULONG ioaddr = priv->pshare->ioaddr;
	ULONG ulCommand=0;
	ULONG ulContent=0;

	// polling bit, and No Write enable, and address
	ulCommand= CAM_CONTENT_COUNT*ucIndex;
	ulCommand= ulCommand | _CAM_POLL_ |_CAM_WE_;
	// write content 0 is equall to mark invalid
	WritePortUlong(_CAM_W_, ulContent);
	WritePortUlong(_CAMCMD_, ulCommand);
}
#endif


void CAM_empty_entry(struct rtl8190_priv *priv,UCHAR ucIndex)
{
#if !defined(RTL8192SU)
	ULONG ioaddr = priv->pshare->ioaddr;
#endif	
	ULONG ulCommand=0;
	ULONG ulContent=0;
	int i;

	for(i=0;i<CAM_CONTENT_COUNT;i++) {
		// polling bit, and No Write enable, and address
		ulCommand= CAM_CONTENT_COUNT*ucIndex+i;
		ulCommand= ulCommand | _CAM_POLL_ |_CAM_WE_;
		// write content 0 is equal to mark invalid
		WritePortUlong(_CAM_W_, ulContent);
		WritePortUlong(_CAMCMD_, ulCommand);
	}
}


int CamDeleteOneEntry(struct rtl8190_priv *priv,UCHAR *pucMacAddr, ULONG ulKeyId, unsigned int useDK)
{
	UCHAR ucIndex;
	UCHAR ucTempMAC[6];
	USHORT usTempConfig=0;
#if !defined(RTL8192SU)	
	ULONG ioaddr = priv->pshare->ioaddr;
#endif	
	int for_begin=0;

	// group key processing for RTL8190
	if ((!memcmp(pucMacAddr, "\xff\xff\xff\xff\xff\xff", 6)) || (useDK)) {
		CAM_read_mac_config(priv,ulKeyId,ucTempMAC,&usTempConfig);
		if (usTempConfig&CFG_VALID) {
			CAM_empty_entry(priv, ulKeyId);
			if (priv->pshare->CamEntryOccupied == 1)
				RTL_W8(_WPACFG_, 0);
			return 1;
		}
		else
			return 0;
	}

	for_begin = 4;

	// unicast key processing for RTL8190
	// key processing for RTL818X(B) series
	for(ucIndex = for_begin; ucIndex < TOTAL_CAM_ENTRY; ucIndex++) {
		CAM_read_mac_config(priv,ucIndex,ucTempMAC,&usTempConfig);
		if(!memcmp(pucMacAddr,ucTempMAC,6)) {

			CAM_empty_entry(priv,ucIndex);	// reset MAC address, david+2007-1-15

			if (priv->pshare->CamEntryOccupied == 1)
				RTL_W8(_WPACFG_, 0);

			return 1;
		}
	}
	return 0;
}


/*now use empty to fill in the first 4 entries*/
void CamResetAllEntry(struct rtl8190_priv *priv)
{
	UCHAR ucIndex;
#if !defined(RTL8192SU)
	ULONG ioaddr = priv->pshare->ioaddr;
#endif	
	WritePortUlong(_CAMCMD_, _CAM_CLR_);

	for(ucIndex=0;ucIndex<TOTAL_CAM_ENTRY;ucIndex++) {
		CAM_empty_entry(priv,ucIndex);
	}

	priv->pshare->CamEntryOccupied = 0;
	priv->pmib->dot11GroupKeysTable.keyInCam = 0;
}


void CAM_read_entry(struct rtl8190_priv *priv,UCHAR index, UCHAR* macad,UCHAR* key128, USHORT* config)
{
#if !defined(RTL8192SU)
	ULONG ioaddr = priv->pshare->ioaddr;
#endif	
	ULONG target_command=0;
	ULONG target_content=0;
	unsigned char entry_i=0;
	ULONG ulStatus;

	for(entry_i=0; entry_i<CAM_CONTENT_USABLE_COUNT; entry_i++)
	{
		// polling bit, and No Write enable, and address
		target_command= (ULONG)(entry_i+CAM_CONTENT_COUNT*index);
		target_command= target_command | _CAM_POLL_;

		WritePortUlong(_CAMCMD_, target_command);
	   	//Check polling bit is clear
		while(1) {
			ReadPortUlong(_CAMCMD_, &ulStatus);
			if(ulStatus & _CAM_POLL_)
				continue;
			else
				break;
		}
		ReadPortUlong(_CAM_R_, &target_content);

		if(entry_i==0) {
			//first 32-bit is MAC address and CFG field
		    *(config)= (USHORT)((target_content)&0x0000FFFF);
		    *(macad+0)= (UCHAR)((target_content>>16)&0x000000FF);
		    *(macad+1)= (UCHAR)((target_content>>24)&0x000000FF);
		}
		else if(entry_i==1) {
			*(macad+5)= (unsigned char)((target_content>>24)&0x000000FF);
		    *(macad+4)= (unsigned char)((target_content>>16)&0x000000FF);
	    	*(macad+3)= (unsigned char)((target_content>>8)&0x000000FF);
	    	*(macad+2)= (unsigned char)((target_content)&0x000000FF);
    		}
		else {
	    	*(key128+(entry_i*4-8)+3)= (unsigned char)((target_content>>24)&0x000000FF);
	    	*(key128+(entry_i*4-8)+2)= (unsigned char)((target_content>>16)&0x000000FF);
	    	*(key128+(entry_i*4-8)+1)= (unsigned char)((target_content>>8)&0x000000FF);
	    	*(key128+(entry_i*4-8)+0)= (unsigned char)(target_content&0x000000FF);
		}

		target_content = 0;
	}
}


void debug_cam(UCHAR*TempOutputMac,UCHAR*TempOutputKey,USHORT TempOutputCfg)
{
	printk("MAC Address\n");
	printk(" %X %X %X %X %X %X\n",*TempOutputMac
					    ,*(TempOutputMac+1)
					    ,*(TempOutputMac+2)
					    ,*(TempOutputMac+3)
					    ,*(TempOutputMac+4)
					    ,*(TempOutputMac+5));
	printk("Config:\n");
	printk(" %X\n",TempOutputCfg);

	printk("Key:\n");
	printk("%X %X %X %X,%X %X %X %X,\n%X %X %X %X,%X %X %X %X\n"
	      ,*TempOutputKey,*(TempOutputKey+1),*(TempOutputKey+2)
	      ,*(TempOutputKey+3),*(TempOutputKey+4),*(TempOutputKey+5)
	      ,*(TempOutputKey+6),*(TempOutputKey+7),*(TempOutputKey+8)
	      ,*(TempOutputKey+9),*(TempOutputKey+10),*(TempOutputKey+11)
	      ,*(TempOutputKey+12),*(TempOutputKey+13),*(TempOutputKey+14)
	      ,*(TempOutputKey+15));
}


#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
__NOMIPS16
#endif
void CamDumpAll(struct rtl8190_priv *priv)
{
	UCHAR TempOutputMac[6];
	UCHAR TempOutputKey[16];
	USHORT TempOutputCfg=0;
	unsigned long flags;
	int i;

	SAVE_INT_AND_CLI(flags);
	for(i=0;i<TOTAL_CAM_ENTRY;i++)
	{
		printk("%X-",i);
		CAM_read_entry(priv,i,TempOutputMac,TempOutputKey,&TempOutputCfg);
		debug_cam(TempOutputMac,TempOutputKey,TempOutputCfg);
		printk("\n\n");
	}
	RESTORE_INT(flags);
}


#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
__NOMIPS16
#endif
void CamDump4(struct rtl8190_priv *priv)
{
	UCHAR TempOutputMac[6];
	UCHAR TempOutputKey[16];
	USHORT TempOutputCfg=0;
	unsigned long flags;
	int i;

	SAVE_INT_AND_CLI(flags);
	for(i=0;i<4;i++)
	{
		printk("%X",i);
		CAM_read_entry(priv,i,TempOutputMac,TempOutputKey,&TempOutputCfg);
		debug_cam(TempOutputMac,TempOutputKey,TempOutputCfg);
		printk("\n\n");
	}
	RESTORE_INT(flags);
}

#ifdef RTL8192SU_EFUSE
/*-----------------------------------------------------------------------------
 * Function:	efuse_PowerSwitch
 *
 * Overview:	When we want to enable write operation, we should change to 
 *				pwr on state. When we stop write, we should switch to 500k mode
 *				and disable LDO 2.5V.
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/17/2008 	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
static	void efuse_PowerSwitch(struct rtl8190_priv *priv, unsigned char PwrState)
{
	unsigned char	tempval;
	if (PwrState == TRUE)
	{
		// Enable LDO 2.5V before read/write action
		tempval = RTL_R8(EFUSE_TEST+3);
		RTL_W8(EFUSE_TEST+3, (tempval | 0x80));

		// Change Efuse Clock for write action to 40MHZ
		RTL_W8(EFUSE_CLK, 0x03);
	}
	else
	{
		// Disable LDO 2.5V after read/write action
		tempval = RTL_R8(EFUSE_TEST+3);
		RTL_W8(EFUSE_TEST+3, (tempval & 0x7F));

		// Change Efuse Clock for write action to 500K
		RTL_W8(EFUSE_CLK, 0x02);
	}	
	
}	/* efuse_PowerSwitch */

static	void ReadEFuseByte(struct rtl8190_priv *priv, unsigned short _offset, unsigned char *pbuf) 
{

	//unsigned short 	indexk=0;
	unsigned int   	value32;
	unsigned char 	readbyte;
	unsigned short 	retry;
	

	//Write Address
	RTL_W8(EFUSE_CTRL+1, (_offset & 0xff));  		
	readbyte = RTL_R8(EFUSE_CTRL+2);
	RTL_W8(EFUSE_CTRL+2, ((_offset >> 8) & 0x03) | (readbyte & 0xfc));  		

	//Write bit 32 0
	readbyte = RTL_R8(EFUSE_CTRL+3);		
	RTL_W8(EFUSE_CTRL+3, (readbyte & 0x7f));  	
	
	//Check bit 32 read-ready
	retry = 0;
	value32 = RTL_R32(EFUSE_CTRL);
	//while(!(((value32 >> 24) & 0xff) & 0x80)  && (retry<10))
	while(!(((value32 >> 24) & 0xff) & 0x80)  && (retry<10000))
	{
		value32 = RTL_R32(EFUSE_CTRL);
		retry++;
	}
	*pbuf = (unsigned char)(value32 & 0xff);
}

//
//	Description:
//		1. Execute E-Fuse read byte operation according as map offset and 
//		    save to E-Fuse table.
//		2. Refered from SD1 Richard.
//
//	Assumption:
//		1. Boot from E-Fuse and successfully auto-load.
//		2. PASSIVE_LEVEL (USB interface)
//
//	Created by Roger, 2008.10.21.
//	2008/12/12 MH 	1. Reorganize code flow and reserve bytes. and add description.
//					2. Add efuse utilization collect.
//	2008/12/22 MH	Read Efuse must check if we write section 1 data again!!! Sec1
//					write addr must be after sec5.
//
void ReadEFuse(struct rtl8190_priv *priv, unsigned short _offset, unsigned char _size_byte, unsigned char *pbuf)
{
	unsigned char  	efuseTbl[EFUSE_MAP_LEN];
	unsigned char  	rtemp8[1];
	unsigned short 	eFuse_Addr = 0;
	unsigned char  	offset, wren;
	unsigned short  	i, j;
	unsigned short 	eFuseWord[EFUSE_MAX_SECTION][EFUSE_MAX_WORD_UNIT];
//	unsigned short 	kindex = 0;
	unsigned short	efuse_utilized = 0;
	unsigned short	efuse_usage = 0;

	//
	// Do NOT excess total size of EFuse table. Added by Roger, 2008.11.10.
	//
	if((_offset + _size_byte)>EFUSE_MAP_LEN)
	{// total E-Fuse table is 128bytes
		DEBUG_ERR("ReadEFuse(): Invalid offset(%#x) with read bytes(%#x)!!\n",_offset, _size_byte);
		return;
	}

	// 0. Refresh efuse init map as all oxFF.
	for (i = 0; i < EFUSE_MAX_SECTION; i++)
		for (j = 0; j < EFUSE_MAX_WORD_UNIT; j++)
			eFuseWord[i][j] = 0xFFFF;


	//
	// 1. Read the first byte to check if efuse is empty!!!
	// 
	//
	ReadEFuseByte(priv, eFuse_Addr, rtemp8);
	if(*rtemp8 != 0xFF)
	{
		efuse_utilized++;
		//printk("EFUSE_READ_ALL, Addr=%d\n", eFuse_Addr);
		eFuse_Addr++;
	}

	//
	// 2. Read real efuse content. Filter PG header and every section data.
	//
	while((*rtemp8 != 0xFF) && (eFuse_Addr < EFUSE_REAL_CONTENT_LEN))
	{
		// Check PG header for section num.
		offset = ((*rtemp8 >> 4) & 0x0f);
		
		if(offset < EFUSE_MAX_SECTION)
		{
			// Get word enable value from PG header
			wren = (*rtemp8 & 0x0f);
			//printk("EFUSE_READ_ALL, Offset-%d Worden=%x\n", offset, wren);

			for(i=0; i<EFUSE_MAX_WORD_UNIT; i++)
			{
				// Check word enable condition in the section				
				if(!(wren & 0x01))
				{
					//printk("EFUSE_READ_ALL, Addr=%d\n", eFuse_Addr);
					ReadEFuseByte(priv, eFuse_Addr, rtemp8);
					eFuse_Addr++;
					efuse_utilized++;
					eFuseWord[offset][i] = (*rtemp8 & 0xff);
					

					if(eFuse_Addr >= EFUSE_REAL_CONTENT_LEN) 
						break;

					//printk("EFUSE_READ_ALL, Addr=%d\n", eFuse_Addr);
					ReadEFuseByte(priv, eFuse_Addr, rtemp8);
					eFuse_Addr++;
					efuse_utilized++;
					eFuseWord[offset][i] |= (((UINT16)*rtemp8 << 8) & 0xff00);

					if(eFuse_Addr >= EFUSE_REAL_CONTENT_LEN) 
						break;
				}
				
				wren >>= 1;
				
			}
		}

		// Read next PG header
		ReadEFuseByte(priv, eFuse_Addr, rtemp8);	
		if(*rtemp8 != 0xFF && (eFuse_Addr < 512))
		{
			efuse_utilized++;
			eFuse_Addr++;
		}
	}

	//
	// 3. Collect 16 sections and 4 word unit into Efuse map.
	//
	for(i=0; i<EFUSE_MAX_SECTION; i++)
	{
		for(j=0; j<EFUSE_MAX_WORD_UNIT; j++)
		{
			efuseTbl[(i*8)+(j*2)]=(eFuseWord[i][j] & 0xff);
			efuseTbl[(i*8)+((j*2)+1)]=((eFuseWord[i][j] >> 8) & 0xff);
		}
	}

	//
	// 4. Copy from Efuse map to output pointer memory!!!
	//
	for(i=0; i<_size_byte; i++)
		pbuf[i] = efuseTbl[_offset+i];

	//
	// 5. Calculate Efuse utilization.
	//
	priv->EfuseUsedBytes = efuse_utilized;
	efuse_usage = (unsigned char)((efuse_utilized*100)/EFUSE_REAL_CONTENT_LEN);
}

/*-----------------------------------------------------------------------------
 * Function:	efuse_ReadAllMap
 *
 * Overview:	Read All Efuse content
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/11/2008 	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
static	void efuse_ReadAllMap(struct rtl8190_priv *priv, unsigned char *Efuse)
{	
	//unsigned char pg_data[8];
	//unsigned char offset = 0;
	//unsigned char tmpidx;

	//
	// We must enable clock and LDO 2.5V otherwise, read all map will be fail!!!!
	//
	efuse_PowerSwitch(priv, TRUE);

	ReadEFuse(priv, 0, 128, Efuse);

#if 0	// Error !!!!!!
	for(offset = 0;offset<16;offset++)	// For 8192SE
	{		
		PlatformFillMemory((PVOID)pg_data, 8, 0xff);
		efuse_PgPacketRead(pAdapter,offset,pg_data);

		PlatformMoveMemory((PVOID)&Efuse[offset*8], (PVOID)pg_data, 8);
	}
#endif
	efuse_PowerSwitch(priv, FALSE);

}	// efuse_ReadAllMap

/*-----------------------------------------------------------------------------
 * Function:	EFUSE_ShadowMapUpdate
 *
 * Overview:	Transfer current EFUSE content to shadow init and modify map.
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/13/2008 	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
void EFUSE_ShadowMapUpdate(struct rtl8190_priv *priv)
{	
	
	if (priv->AutoloadFailFlag == TRUE )
	{
		memset((PVOID)(&priv->EfuseMap[EFUSE_INIT_MAP][0]), 128, 0xFF);
	}
	else
	{
		//efuse_ReadAllMap(priv);
		efuse_ReadAllMap(priv, &priv->EfuseMap[EFUSE_INIT_MAP][0]);
	}

	memcpy((PVOID)&priv->EfuseMap[EFUSE_MODIFY_MAP][0], 
	(PVOID)&priv->EfuseMap[EFUSE_INIT_MAP][0], HWSET_MAX_SIZE_92S);
}	// EFUSE_ShadowMapUpdate

//
//	Description:
//		Read HW adapter information by E-Fuse or EEPROM according CR9346 reported.
//
//	Assumption:
//		1. CR9346 regiser has verified.
//		2. PASSIVE_LEVEL (USB interface)
//
//	Created by Roger, 2008.10.21.
//
int ReadAdapterInfo8192SUsb(struct rtl8190_priv *priv)
{
	unsigned char			tmpU1b;	    
	unsigned char			hwinfo[HWSET_MAX_SIZE_92S];
	    
	//
	// <Roger_Note> The following operation are prevent Efuse leakage by turn on 2.5V.
	// 2008.11.25.
	//
	tmpU1b = RTL_R8(EFUSE_TEST+3);
	RTL_W8(EFUSE_TEST+3, tmpU1b|0x80);
	delay_us(1000);
	RTL_W8(EFUSE_TEST+3, (tmpU1b&(~(BIT(7)))));
	
	// Retrieve Chip version.
	priv->pshare->VersionID = 
			(VERSION_8192S)((RTL_R32(PMC_FSM)>>16)&0xF);
	switch(priv->pshare->VersionID)
	{
		case 0:
			printk("Chip Version ID: VERSION_8192S_ACUT.\n");
			break;
		case 1:
			printk("Chip Version ID: VERSION_8192S_BCUT.\n");
			break;
		case 2:
			printk("Chip Version ID: VERSION_8192S_CCUT.\n");
			break;
		default:
			printk("Unknown Chip Version!!\n");
			priv->pshare->VersionID = VERSION_8192S_BCUT;
			break;
	}	

	// IS_BOOT_FROM_EFUSE(Adapter)
	{	// Read from EFUSE	

		//
		// <Roger_Notes> We set Isolation signals from Loader and reset EEPROM after system resuming 
		// from suspend mode.
		// 2008.10.21. 
		//
		//PlatformEFIOWrite1Byte(Adapter, SYS_ISO_CTRL+1, 0xE8); // Isolation signals from Loader
		//PlatformStallExecution(10000);
		//PlatformEFIOWrite1Byte(Adapter, SYS_FUNC_EN+1, 0x40); 
		//PlatformEFIOWrite1Byte(Adapter, SYS_FUNC_EN+1, 0x50); 
		
		//tmpU1b = PlatformEFIORead1Byte(Adapter, EFUSE_TEST+3);
		//PlatformEFIOWrite1Byte(Adapter, EFUSE_TEST+3, (tmpU1b | 0x80));
		//PlatformEFIOWrite1Byte(Adapter, EFUSE_TEST+3, 0x72); 
		//PlatformEFIOWrite1Byte(Adapter, EFUSE_CLK, 0x03);

		//pHalData->EEType = EEPROM_BOOT_EFUSE;
		
		// Read EFUSE real map to shadow.
		EFUSE_ShadowMapUpdate(priv);
		memcpy((PVOID) hwinfo, (PVOID)&priv->EfuseMap[EFUSE_INIT_MAP][0], HWSET_MAX_SIZE_92S);		
	}
	return RT_STATUS_SUCCESS;
}

//
//	Description:
//		Read HW adapter information by E-Fuse or EEPROM according CR9346 reported.
//
//	Assumption:
//		1. CR9346 regiser has verified.
//		2. PASSIVE_LEVEL (USB interface)
//
//	Created by Roger, 2008.10.21.
//
int ReadAdapterInfo8192S(struct rtl8190_priv *priv)
{
	unsigned char			tmpU1b;	    

	tmpU1b = RTL_R8(CR9346);	

	// To check system boot selection.
	if (tmpU1b & CmdEERPOMSEL)
	{
		printk("Boot from EEPROM,  ");
		priv->EepromOrEfuse = TRUE;
	}
	else 
	{
		printk("Boot from EFUSE,  ");
		priv->EepromOrEfuse = FALSE;
	}	

	// To check autoload success or not.
	if (tmpU1b & CmdEEPROM_En)
	{
		printk("Autoload OK!!\n"); 
		priv->AutoloadFailFlag=FALSE;		
		ReadAdapterInfo8192SUsb(priv);
	}	
	else
	{ // Auto load fail.		
		printk("AutoLoad Fail reported from CR9346!!\n");
		priv->AutoloadFailFlag=TRUE;
	}
	return RT_STATUS_SUCCESS;
}

void ReadEfuse8192SU(struct rtl8190_priv *priv)
{
	if (priv->AutoloadFailFlag==FALSE && priv->pmib->efuseEntry.enable_efuse==1)
	{
		unsigned int usb_ep_num;
		int i;//, rf_path;

		for(i=0;i<14;i++)
		{
			if (i < 3)
				priv->pmib->dot11RFEntry.pwrlevelCCK[i]=priv->EfuseMap[0][EEPROM_TxPwIndex];
			else if (i < 9)
				priv->pmib->dot11RFEntry.pwrlevelCCK[i]=priv->EfuseMap[0][EEPROM_TxPwIndex+1];
			else
				priv->pmib->dot11RFEntry.pwrlevelCCK[i]=priv->EfuseMap[0][EEPROM_TxPwIndex+2];
		}
		for(i=0;i<14;i++)
		{
			if (i < 3)
			{
				priv->pmib->dot11RFEntry.pwrlevelOFDM_1SS[i]=priv->EfuseMap[0][EEPROM_TxPwIndex+6];
				priv->pmib->dot11RFEntry.pwrlevelOFDM_1SS[i+14]=priv->EfuseMap[0][EEPROM_TxPwIndex+9];
				priv->pmib->dot11RFEntry.pwrlevelOFDM_2SS[i]=priv->EfuseMap[0][EEPROM_TxPwIndex+12];
				priv->pmib->dot11RFEntry.pwrlevelOFDM_2SS[i+14]=priv->EfuseMap[0][EEPROM_TxPwIndex+15];
			}
			else if (i < 9)
			{
				priv->pmib->dot11RFEntry.pwrlevelOFDM_1SS[i]=priv->EfuseMap[0][EEPROM_TxPwIndex+7];
				priv->pmib->dot11RFEntry.pwrlevelOFDM_1SS[i+14]=priv->EfuseMap[0][EEPROM_TxPwIndex+10];
				priv->pmib->dot11RFEntry.pwrlevelOFDM_2SS[i]=priv->EfuseMap[0][EEPROM_TxPwIndex+13];
				priv->pmib->dot11RFEntry.pwrlevelOFDM_2SS[i+14]=priv->EfuseMap[0][EEPROM_TxPwIndex+16];
			}
			else
			{
				priv->pmib->dot11RFEntry.pwrlevelOFDM_1SS[i]=priv->EfuseMap[0][EEPROM_TxPwIndex+8];
				priv->pmib->dot11RFEntry.pwrlevelOFDM_1SS[i+14]=priv->EfuseMap[0][EEPROM_TxPwIndex+11];
				priv->pmib->dot11RFEntry.pwrlevelOFDM_2SS[i]=priv->EfuseMap[0][EEPROM_TxPwIndex+14];
				priv->pmib->dot11RFEntry.pwrlevelOFDM_2SS[i+14]=priv->EfuseMap[0][EEPROM_TxPwIndex+17];
			}
		}
		priv->pmib->dot11RFEntry.LOFDM_pwd_A=priv->EfuseMap[0][EEPROM_PwDiff]&0x0f;
		priv->pmib->dot11RFEntry.LOFDM_pwd_A+=32;
		priv->pmib->dot11RFEntry.LOFDM_pwd_B=(priv->EfuseMap[0][EEPROM_PwDiff]&0xf0)>>4;
		priv->pmib->dot11RFEntry.LOFDM_pwd_B+=32;

		priv->pmib->dot11RFEntry.ther=priv->EfuseMap[0][EEPROM_ThermalMeter];

		//USB endpoint number
		usb_ep_num=((priv->EfuseMap[0][EEPROM_USB_OPTIONAL])>>3)&0x03;
		switch(usb_ep_num)
		{
			case 0:
				priv->pmib->efuseEntry.usb_epnum=USBEP_SIX;
				break;
			case 2:
				priv->pmib->efuseEntry.usb_epnum=USBEP_FOUR;
				break;
			default:
				priv->pmib->efuseEntry.usb_epnum=USBEP_FOUR;				
		}

		//RF board type
		switch(priv->EfuseMap[0][EEPROM_BoardType])
		{

			case 0: //  1x1
			priv->pmib->dot11RFEntry.MIMO_TR_mode=MIMO_1T1R;
				break;
			case 1://  1x2
				priv->pmib->dot11RFEntry.MIMO_TR_mode=MIMO_1T2R;
				break;
			case 2://  2x2
			default:
				priv->pmib->dot11RFEntry.MIMO_TR_mode=MIMO_2T2R;
		}
	}
}
#endif

