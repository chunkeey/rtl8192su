/*
 *      Routines to access hardware
 *
 *      $Id: 8190n_hw.c,v 1.12 2009/09/28 13:24:13 cathy Exp $
 */

#define _8190N_HW_C_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <asm/unistd.h>

#include "./8190n_cfg.h"
#include "./8190n.h"
#include "./8190n_hw.h"
#include "./8190n_headers.h"
#include "./8190n_debug.h"
#include "./8190n_usb.h"

#include <linux/syscalls.h>

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
	unsigned int  offset = (RegAddr&0x3);
  	unsigned int ReturnValue = 0, OriginalValue, BitShift;

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
	BitShift = phy_CalculateBitShift(BitMask);
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
	unsigned int OriginalValue, BitShift, NewValue;

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


	if (BitMask != bMaskDWord)
	{//if not "double word" write
		OriginalValue = PHY_QueryBBReg(priv, RegAddr, bMaskDWord);
		priv->pshare->bChangeBBInProgress = TRUE;
		BitShift = phy_CalculateBitShift(BitMask);
		NewValue = ((OriginalValue & (~BitMask)) | (Data << BitShift));
		RTL_W32(RegAddr, NewValue);
	}
	else
	{
		priv->pshare->bChangeBBInProgress = TRUE;
		RTL_W32(RegAddr, Data);
	}


	priv->pshare->bChangeBBInProgress = FALSE;
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
	unsigned char PollingCnt = 100;
	unsigned char RFWaitCounter = 0;
	


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
	unsigned int				retValue = 0;
	unsigned char PollingCnt = 100;
	unsigned char RFWaitCounter = 0;
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
#else
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
	int           fd=0;
	mm_segment_t  old_fs;
	unsigned char *pFileName=NULL;
	unsigned int  u4bRegOffset, u4bRegValue, u4bRegMask;
	unsigned char *mem_ptr, *line_head, *next_head;
	struct PhyRegTable *reg_table=NULL;
	struct MacRegTable *pg_reg_table = NULL;
	unsigned short max_len=0;
	int file_format = TWO_COLUMN;


	switch (reg_file) {
	case AGCTAB:
		pFileName = "/usr/rtl8192su/AGC_TAB.txt";
		reg_table = (struct PhyRegTable *)priv->pshare->agc_tab_buf;
		max_len = AGC_TAB_SIZE;
		break;
	case PHYREG_1T2R:
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
		max_len = PHY_REG_SIZE;
		break;
	case PHYREG_2T4R:
		pFileName = "/usr/rtl8192su/phy_reg.txt";
		reg_table = (struct PhyRegTable *)priv->pshare->phy_reg_buf;
		max_len = PHY_REG_SIZE;
		break;
	case PHYREG_2T2R:
#if defined(MP_TEST)
	if (priv->pshare->rf_ft_var.mp_specific) {
		pFileName = "/usr/rtl8192su/phy_reg_MP.txt";
		reg_table = (struct PhyRegTable *)priv->pshare->phy_reg_mp_buf;
		max_len = PHY_REG_SIZE;
		break; 
	}
	else
#endif
	{
		pFileName = "/usr/rtl8192su/phy_reg.txt";
		reg_table = (struct PhyRegTable *)priv->pshare->phy_reg_buf;
		max_len = PHY_REG_SIZE;
		break;
	}
	case PHYREG_PG:
		pFileName = "/usr/rtl8192su/PHY_REG_PG.txt";
		pg_reg_table = (struct MacRegTable *)priv->pshare->phy_reg_pg_buf;
		max_len = PHY_REG_PG_SIZE;
		file_format = THREE_COLUMN;
		break;
	}

#ifdef CHECK_HANGUP
	if (!priv->reset_hangup)
#endif
	{
		if((mem_ptr = (unsigned char *)kmalloc(MAX_CONFIG_FILE_SIZE, GFP_ATOMIC)) == NULL) {
			printk("PHY_ConfigBBWithParaFile(): not enough memory\n");
			return -1;
		}

		memset(mem_ptr, 0, MAX_CONFIG_FILE_SIZE); // clear memory


		old_fs = get_fs();
		set_fs(KERNEL_DS);

		if ((fd = rtl8190n_fileopen(pFileName, O_RDONLY, 0)) < 0)
		{
			printk("PHY_ConfigBBWithParaFile(): cannot open %s\n", pFileName);
			set_fs(old_fs);
			kfree(mem_ptr);
			return -1;
		}

		read_bytes = sys_read(fd, mem_ptr, MAX_CONFIG_FILE_SIZE);

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

		sys_close(fd);
		set_fs(old_fs);

		kfree(mem_ptr);

		if ((len * sizeof(struct PhyRegTable)) > max_len) {
			printk("PHY REG table buffer not large enough! (%s)\n", pFileName);
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
int PHY_ConfigRFWithParaFile(struct rtl8190_priv *priv, unsigned char *pFileName, RF90_RADIO_PATH_E eRFPath)
{
	int           num;
	int           fd=0, read_bytes;
	mm_segment_t  old_fs;
	unsigned int  u4bRegOffset, u4bRegValue;
	unsigned char *mem_ptr, *line_head, *next_head;

	if((mem_ptr = (unsigned char *)kmalloc(MAX_CONFIG_FILE_SIZE, GFP_ATOMIC)) == NULL) {

		printk("PHY_ConfigRFWithParaFile(): not enough memory\n");
		return -1;
	}

	memset(mem_ptr, 0, MAX_CONFIG_FILE_SIZE); // clear memory

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	if ((fd = rtl8190n_fileopen(pFileName, O_RDONLY, 0)) < 0)
	{
		printk("PHY_ConfigRFWithParaFile(): cannot open %s\n", pFileName);
		set_fs(old_fs);
		kfree(mem_ptr);
		return -1;
	}

	read_bytes = sys_read(fd, mem_ptr, MAX_CONFIG_FILE_SIZE);

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
				PHY_SetRFReg(priv, eRFPath, u4bRegOffset, bMask20Bits, u4bRegValue);
				delay_ms(1);
			}
		}
	}

	sys_close(fd);
	set_fs(old_fs);

	kfree(mem_ptr);

	return 0;
}


int PHY_ConfigMACWithParaFile(struct rtl8190_priv *priv)
{
	int  read_bytes, num, len=0;
	unsigned int  u4bRegOffset, u4bRegMask, u4bRegValue;
	unsigned char *mem_ptr, *line_head, *next_head;
	int fd=0;
	mm_segment_t  old_fs;
	unsigned char *pFileName = "/usr/rtl8192su/macreg.txt";
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


		old_fs = get_fs();
		set_fs(KERNEL_DS);

		if ((fd = rtl8190n_fileopen(pFileName, O_RDONLY, 0)) < 0)
		{
			printk("PHY_ConfigMACWithParaFile(): cannot open %s\n", pFileName);
			set_fs(old_fs);
			kfree(mem_ptr);
			return -1;
		}

		read_bytes = sys_read(fd, mem_ptr, MAX_CONFIG_FILE_SIZE);

		next_head = mem_ptr;
		while (1) {
			line_head = next_head;
			next_head = get_line(&line_head);
			if (line_head == NULL)
				break;

			if (line_head[0] == '/')
				continue;

			u4bRegMask = 0;
			num = get_offset_val_macreg(line_head, &u4bRegOffset, &u4bRegValue);
			if (num > 0) {
				reg_table[len].offset = u4bRegOffset;
				reg_table[len].value = u4bRegValue;
				len++;
				if (u4bRegOffset == 0xff)
					break;
				if ((len * sizeof(struct MacRegTable)) > MAC_REG_SIZE)
					break;
			}
		}

		sys_close(fd);
		set_fs(old_fs);

		kfree(mem_ptr);

		if ((len * sizeof(struct MacRegTable)) > MAC_REG_SIZE) {
			printk("MAC REG table buffer not large enough!\n");
			return -1;
		}
	}

	num = 0;
	while (1) {
		u4bRegOffset = reg_table[num].offset;
		u4bRegValue = reg_table[num].value;

		if (u4bRegOffset == 0xff)
			break;
		else
			RTL_W8(u4bRegOffset, u4bRegValue);
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




void SwChnl(struct rtl8190_priv *priv, unsigned char channel, int offset)
{
	static unsigned int val;

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
}


void SwitchAntenna(struct rtl8190_priv *priv)
{
}



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



void enable_hw_LED(struct rtl8190_priv *priv, unsigned int led_type)
{

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
	phw->PHYRegDef[RF90_PATH_A].rfLSSIReadBack_92S_PI = 0x8b8;
	phw->PHYRegDef[RF90_PATH_B].rfLSSIReadBack_92S_PI = 0x8bc;
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

	if (RTL_R32(PMC_FSM) & BIT(24))
		GET_HW(priv)->MIMO_TR_hw_support = MIMO_2T2R;
	else
		GET_HW(priv)->MIMO_TR_hw_support = MIMO_1T2R;
	return;
}




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

}	/* PHY_RF6052SetOFDMTxPower */


int phy_BB8192SE_Config_ParaFile(struct rtl8190_priv *priv)
{
//	unsigned long ioaddr = priv->pshare->ioaddr;
//	unsigned char u1RegValue;
//	unsigned int  u4RegValue;
	int rtStatus;

	phy_InitBBRFRegisterDefinition(priv);


	/*----Check MIMO TR hw setting----*/
	check_MIMO_TR_status(priv);


	/*----BB Register Initilazation----*/
	rtStatus = 0;
	if (get_rf_mimo_mode(priv) == MIMO_1T2R)
		rtStatus = PHY_ConfigBBWithParaFile(priv, PHYREG_1T2R);
	else if (get_rf_mimo_mode(priv) == MIMO_2T2R)
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

//		RF0_Final_Value = 0;
//		RetryTimes = 5;

		/*----Initialize RF fom connfiguration file----*/
		switch (eRFPath)
		{
			case RF90_PATH_A:
				rtStatus = PHY_ConfigRFWithParaFile(priv, "/usr/rtl8192su/radio_a.txt", eRFPath);				
				break;
			case RF90_PATH_B:
				rtStatus = PHY_ConfigRFWithParaFile(priv, "/usr/rtl8192su/radio_b.txt", eRFPath);
				break;
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
	mm_segment_t old_fs;
//	unsigned int buf_len=0, read_bytes;
	int fd=0;

	swap_arr = kmalloc(RT_8192S_FIRMWARE_HDR_SIZE, GFP_ATOMIC);
	if (swap_arr == NULL)
		return;

	unsigned char *pFileName=NULL;


	old_fs = get_fs();
	set_fs(KERNEL_DS);

	//read from fw img and setup parameters

	if ((fd = rtl8190n_fileopen("/usr/rtl8192Pci/rtl8192sfw.bin", O_RDONLY, 0)) < 0)
	{
		printk("%s cannot be opened\n", pFileName);
		BUG();
	}

	// read from bin file
	sys_read(fd, swap_arr, RT_8192S_FIRMWARE_HDR_SIZE);
	sys_read(fd, priv->pshare->fw_DMEM_buf, FW_DMEM_SIZE);
	rtl_cache_sync_wback(priv, (unsigned int)priv->pshare->fw_DMEM_buf, RT_8192S_FIRMWARE_HDR_SIZE, PCI_DMA_TODEVICE);

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
	pFwPriv->hci_sel=HCISEL_USBAP;
	if (priv->pmib->efuseEntry.usb_epnum==USBEP_SIX)
		pFwPriv->usb_ep_num=USBEP_SIX;
	else
		pFwPriv->usb_ep_num=USBEP_FOUR;
#ifdef RTL8192SU_FWBCN
	pFwPriv->beacon_offload=BCNOFFLOAD_FW;
#endif

	//read IMEM
	sys_read(fd, priv->pshare->fw_IMEM_buf, priv->pshare->fw_IMEM_len);
	rtl_cache_sync_wback(priv, (unsigned int)priv->pshare->fw_IMEM_buf,  priv->pshare->fw_IMEM_len, PCI_DMA_TODEVICE);

	//read EMEM
	sys_read(fd, priv->pshare->fw_EMEM_buf, priv->pshare->fw_EMEM_len);
	rtl_cache_sync_wback(priv, (unsigned int)priv->pshare->fw_EMEM_buf,  priv->pshare->fw_EMEM_len, PCI_DMA_TODEVICE);

	sys_close(fd);
	set_fs(old_fs);
	kfree(swap_arr);
}




static void LoadIMG(struct rtl8190_priv *priv, int fw_file)
{
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

			if(bResult)
				delay_ms(1);

			mem_ptr += LoadPktSize;
			i += LoadPktSize;
		}
		else {
			bResult = rtl8192SE_SetupOneCmdPacket(priv, mem_ptr, (buf_len-i), 1);
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
	volatile
	unsigned int		tmpU4b;
//	unsigned int		bOrgIMREnable;

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
	RTL_W8((SYS_CLKR + 1), Data);
	delay_us(200);
	return TRUE;
}

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
	unsigned char tmpU1b;

#ifdef RTL867X_DMEM_ENABLE
	if(priv->pshare->fw_IMEM_buf!=NULL) {kfree(priv->pshare->fw_IMEM_buf); priv->pshare->fw_IMEM_buf=NULL; }
	if(priv->pshare->fw_EMEM_buf!=NULL) {kfree(priv->pshare->fw_EMEM_buf); priv->pshare->fw_EMEM_buf=NULL; }
	if(priv->pshare->fw_DMEM_buf!=NULL) {kfree(priv->pshare->fw_DMEM_buf); priv->pshare->fw_DMEM_buf=NULL; }
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
	//Set RQPN
#if !defined(RTL8192SU_FWBCN)
	if (!RTL_R8(RQPN3) && (RTL_R8(RQPN3+1)>1)) {
		//default assign bcn q 2 page when pub q page >= 2 and bcn q page == 0
		RTL_W8(RQPN3+1, RTL_R8(RQPN3+1)-2);
		RTL_W8(RQPN3, 2);
	}
#endif	
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
	static unsigned long val32;
	static unsigned short val16;
	static int i;
	static unsigned short fixed_rate;
	static unsigned int ii;
	static unsigned char calc_rate, val8;

	pmib = GET_MIB(priv);
	opmode = priv->pmib->dot11OperationEntry.opmode;

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

	RTL8192SU_MacConfigBeforeFwDownload(priv);

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
		return LOADFW_FAIL;
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
	RTL_W32(RCR, RTL_R32(RCR) | RCR_RXSHFT_EN);
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

#ifdef RTL8192SU_FWBCN
	//RTL_W16(BCN_DRV_EARLY_INT, 0x0000);
#else
	RTL_W16(BCN_DRV_EARLY_INT, 0x8000); //enable software transmit beacon frame
	RTL_W16(BCN_DMATIME, 0);
#endif	

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
		if ((priv->pmib->dot11RFEntry.LOFDM_pwd_A == 0) &&
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
	*(unsigned int *)(&priv->pshare->phw->MCSTxAgcOffset[0])  = PHY_QueryBBReg(priv, rTxAGC_Mcs03_Mcs00, bMaskDWord);
	*(unsigned int *)(&priv->pshare->phw->MCSTxAgcOffset[4])  = PHY_QueryBBReg(priv, rTxAGC_Mcs07_Mcs04, bMaskDWord);
	if (get_rf_mimo_mode(priv) == MIMO_2T2R)
	{
		*(unsigned int *)(&priv->pshare->phw->MCSTxAgcOffset[8])  = PHY_QueryBBReg(priv, rTxAGC_Mcs11_Mcs08, bMaskDWord);
		*(unsigned int *)(&priv->pshare->phw->MCSTxAgcOffset[12]) = PHY_QueryBBReg(priv, rTxAGC_Mcs15_Mcs12, bMaskDWord);
	}
	*(unsigned int *)(&priv->pshare->phw->OFDMTxAgcOffset[0]) = PHY_QueryBBReg(priv, rTxAGC_Rate18_06, bMaskDWord);
	*(unsigned int *)(&priv->pshare->phw->OFDMTxAgcOffset[4]) = PHY_QueryBBReg(priv, rTxAGC_Rate54_24, bMaskDWord);

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




static int LoadFirmware(struct rtl8190_priv *priv)
{

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


	PRINT_INFO("RTL8192 Firmware version: %04x(%d.%d)\n",
		priv->pshare->fw_version, priv->pshare->fw_src_version, priv->pshare->fw_sub_version);

	return TRUE;
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
	static RF90_RADIO_PATH_E eRFPath;
	static BB_REGISTER_DEFINITION_T *pPhyReg;
	static int do_mac_reset;
    do_mac_reset = 1;

	if (reset_bb) {
		for (eRFPath = RF90_PATH_A; eRFPath<priv->pshare->phw->NumTotalRFPath; eRFPath++) {
			pPhyReg = &priv->pshare->phw->PHYRegDef[eRFPath];
			PHY_SetBBReg(priv, pPhyReg->rfintfs, bRFSI_RFENV, bRFSI_RFENV);
			PHY_SetBBReg(priv, pPhyReg->rfintfo, bRFSI_RFENV, 0);
		}

		RTL_W8(MSR, MSR_NOLINK);


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
	}
	if (reset_bb)
		do_mac_reset = 0;

	if (do_mac_reset) {
		delay_us(800); // it is critial!
	}
	printk("stop hw finished!\n");
	return SUCCESS;
}


void SwBWMode(struct rtl8190_priv *priv, unsigned int bandwidth, int offset)
{
	static unsigned char regBwOpMode, nCur40MhzPrimeSC;

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
			//RTL_W8(RRSR+2, 0xe0);
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
			PHY_SetBBReg(priv, rFPGA0_AnalogParameter1, 0x00100000, 1);
			PHY_SetBBReg(priv, rFPGA0_AnalogParameter2, 0x000000ff, 0x58);
						// From SD3 WHChang
			PHY_SetBBReg(priv, rFPGA0_AnalogParameter1, 0x00300000, 3);
			PHY_SetBBReg(priv, rCCK0_TxFilter1, bMaskDWord, 0x1a1b0000);	//suggest by YN
			PHY_SetBBReg(priv, rCCK0_TxFilter2, bMaskDWord, 0x090e1317);	//suggest by YN
			PHY_SetBBReg(priv, rCCK0_DebugPort, bMaskDWord, 0x00000204);	//suggest by YN
			break;
		case HT_CHANNEL_WIDTH_20_40:
			PHY_SetBBReg(priv, rFPGA0_RFMOD, bRFMOD, 0x1);
			PHY_SetBBReg(priv, rFPGA1_RFMOD, bRFMOD, 0x1);
			PHY_SetBBReg(priv, rCCK0_System, bCCKSideBand, (nCur40MhzPrimeSC>>1));
			PHY_SetBBReg(priv, rOFDM1_LSTF, 0xC00, nCur40MhzPrimeSC);
			// From SD3 WHChang
			PHY_SetBBReg(priv, rFPGA0_AnalogParameter1, 0x00300000, 3);
			// Set Control channel to upper or lower. These settings are required only for 40MHz
			PHY_SetBBReg(priv, rFPGA0_AnalogParameter1, 0x00100000, 1);
			PHY_SetBBReg(priv, rFPGA0_AnalogParameter2, 0x000000ff, 0x18);
//			PHY_SetBBReg(priv, rCCK0_System, bCCKSideBand, (nCur40MhzPrimeSC>>1));
//			PHY_SetBBReg(priv, rOFDM1_LSTF, 0xC00, nCur40MhzPrimeSC);
                        PHY_SetBBReg(priv, rCCK0_TxFilter1, bMaskDWord, 0x1a1b0000);    //suggest by YN
                        PHY_SetBBReg(priv, rCCK0_TxFilter2, bMaskDWord, 0x090e1317);    //suggest by YN
                        PHY_SetBBReg(priv, rCCK0_DebugPort, bMaskDWord, 0x00000204);    //suggest by YN
			break;
		default:
			DEBUG_ERR("SwBWMode(): bandwidth mode error!\n");
			return;
			break;
	}


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
		delay_ms(600);
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
		delay_ms(600);
		val_read |= (BIT(10)|BIT(11));
		PHY_SetRFReg(priv, 0, 0x18, bMask20Bits, val_read);

		val_read = PHY_QueryRFReg(priv, 1, 0x18, bMask20Bits, 1);
		val_read |= (BIT(10)|BIT(11));
		PHY_SetRFReg(priv, 1, 0x18, bMask20Bits, val_read);

		PHY_SetBBReg(priv, 0x840,0x0000ffff, 0x7406);
	}
	if (priv->pshare->rf_ft_var.use_frq_2_3G)
		PHY_SetRFReg(priv, RF90_PATH_C, 0x2c, 0x60, 0);
}


void GetHardwareVersion(struct rtl8190_priv *priv)
{

	if (RTL_R8(0x301) == 0x02)
		priv->pshare->VersionID = VERSION_8190_C;
	else
		priv->pshare->VersionID = VERSION_8190_B;
}


void init_EDCA_para(struct rtl8190_priv *priv, int mode)
{
	static unsigned int slot_time, VO_TXOP, VI_TXOP, sifs_time;
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
			RTL_W32(_ACBE_PARM_, (6 << 12) | (4 << 8) | (sifs_time + 3 * slot_time));
	}
	else {
		if (priv->pshare->ht_sta_num
#ifdef WDS
			|| ((OPMODE & WIFI_AP_STATE) && (mode & WIRELESS_11N) &&
			priv->pmib->dot11WdsInfo.wdsEnabled && priv->pmib->dot11WdsInfo.wdsNum)
#endif
			) {
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
			RTL_W32(_ACBE_PARM_, ((TXOP*priv->pshare->txop_enlarge) << 16) |
			(6 << 12) | (4 << 8) | (sifs_time + slot_time));
		}
		else {
			// for intel 4965 IOT issue with 92SE, temporary solution, victoryman 20081201
			RTL_W32(_ACBE_PARM_, (6 << 12) | (4 << 8) | 0x19);
		}

		if (priv->pmib->dot11OperationEntry.wifi_specific == 2) {
			RTL_W16(NAV_PROT_LEN, 0x01C0);
			RTL_W8(CFEND_TH, 0xFF);
			set_fw_reg(priv, 0xfd000ab0, 0, 0);
		}
	}
}
#endif


void setup_timer1(struct rtl8190_priv *priv, int timeout)
{

	RTL_W32(_TIMER1_, timeout);

}


void cancel_timer1(struct rtl8190_priv *priv)
{
}


void setup_timer2(struct rtl8190_priv *priv, unsigned int timeout)
{
	unsigned int current_value=RTL_R32(_TSFTR_L_);

	if (TSF_LESS(timeout, current_value))
		timeout = current_value+20;

	RTL_W32(_TIMER2_, timeout);
}


void cancel_timer2(struct rtl8190_priv *priv)
{
}


// dynamic DC_TH of Fsync in regC38 for non-BCM solution
void check_DC_TH_by_rssi(struct rtl8190_priv *priv, unsigned char rssi_strength)
{

	{
		if ((priv->dc_th_current_state != DC_TH_USE_UPPER) &&
			(rssi_strength >= priv->pshare->rf_ft_var.dcThUpper)) {
			RTL_W8(0xc38, 0x94);
			priv->dc_th_current_state = DC_TH_USE_UPPER;
		}
		else if ((priv->dc_th_current_state != DC_TH_USE_LOWER) &&
			(rssi_strength <= priv->pshare->rf_ft_var.dcThLower)) {
			RTL_W8(0xc38, 0x90);
			priv->dc_th_current_state = DC_TH_USE_LOWER;
		}
		else if (priv->dc_th_current_state == DC_TH_USE_NONE) {
			RTL_W8(0xc38, 0x94);
			priv->dc_th_current_state = DC_TH_USE_UPPER;
		}
	}
}


void check_DIG_by_rssi(struct rtl8190_priv *priv, unsigned char rssi_strength)
{
	unsigned int dig_on = 0;

	if (OPMODE & WIFI_SITE_MONITOR)
		return;


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

	check_DC_TH_by_rssi(priv, rssi_strength);
}


void DIG_for_site_survey(struct rtl8190_priv *priv, int do_ss)
{

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
	}
}



// dynamic CCK CCA enhance by rssi
void CCK_CCA_dynamic_enhance(struct rtl8190_priv *priv, unsigned char rssi_strength)
{

	if (!priv->pshare->phw->CCK_CCA_enhanced && (rssi_strength < 20)) {
		priv->pshare->phw->CCK_CCA_enhanced = TRUE;
		RTL_W8(0xa0a, 0x83);
	}
	else if (priv->pshare->phw->CCK_CCA_enhanced && (rssi_strength > 25)) {
		priv->pshare->phw->CCK_CCA_enhanced = FALSE;
		RTL_W8(0xa0a, 0xcd);
	}
}


// bcm old 11n chipset iot debug

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



}


// dynamic Rx path selection by signal strength
void rx_path_by_rssi(struct rtl8190_priv *priv, struct stat_info *pstat, int enable)
{
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
}


void tx_power_tracking(struct rtl8190_priv *priv)
{

	if (priv->pmib->dot11RFEntry.ther) {
		DEBUG_INFO("TPT: triggered(every %d seconds)\n", priv->pshare->rf_ft_var.tpt_period);

		// enable rf reg 0x24 power and trigger, to get ther value in 1 second
		PHY_SetRFReg(priv, RF90_PATH_A, 0x24, bMask20Bits, 0x60);
		mod_timer(&priv->pshare->phw->tpt_timer, jiffies + 100); // 1000ms
	}
}


void rtl8190_tpt_timer(unsigned long task_priv)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)task_priv;
	unsigned int val32;


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




void CAM_empty_entry(struct rtl8190_priv *priv,UCHAR ucIndex)
{
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
	WritePortUlong(_CAMCMD_, _CAM_CLR_);

	for(ucIndex=0;ucIndex<TOTAL_CAM_ENTRY;ucIndex++) {
		CAM_empty_entry(priv,ucIndex);
	}

	priv->pshare->CamEntryOccupied = 0;
	priv->pmib->dot11GroupKeysTable.keyInCam = 0;
}


void CAM_read_entry(struct rtl8190_priv *priv,UCHAR index, UCHAR* macad,UCHAR* key128, USHORT* config)
{
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

