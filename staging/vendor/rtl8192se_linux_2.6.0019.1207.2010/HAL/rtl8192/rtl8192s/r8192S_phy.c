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
#ifdef RTL8192SE

#include "../rtl_core.h"
#include "../rtl_dm.h"

#ifdef ENABLE_DOT11D
#include "../../../rtllib/dot11d.h"
#endif

/*---------------------------Define Local Constant---------------------------*/
#define MAX_PRECMD_CNT 16
#define MAX_RFDEPENDCMD_CNT 16
#define MAX_POSTCMD_CNT 16

#define MAX_DOZE_WAITING_TIMES_9x 64

#define	PHY_STOP_SWITCH_CLKREQ			0
/*---------------------------Define Local Constant---------------------------*/

/*------------------------Define global variable-----------------------------*/

#define Rtl819XMAC_Array			Rtl8192SEMAC_2T_Array
#define Rtl819XAGCTAB_Array			Rtl8192SEAGCTAB_Array
#define Rtl819XPHY_REG_Array			Rtl8192SEPHY_REG_2T2RArray
#define Rtl819XPHY_REG_to1T1R_Array 		Rtl8192SEPHY_ChangeTo_1T1RArray
#define Rtl819XPHY_REG_to1T2R_Array		Rtl8192SEPHY_ChangeTo_1T2RArray
#define Rtl819XPHY_REG_to2T2R_Array		Rtl8192SEPHY_ChangeTo_2T2RArray
#define Rtl819XPHY_REG_Array_PG			Rtl8192SEPHY_REG_Array_PG
#define Rtl819XRadioA_Array			Rtl8192SERadioA_1T_Array
#define Rtl819XRadioB_Array			Rtl8192SERadioB_Array
#define Rtl819XRadioB_GM_Array					Rtl8192SERadioB_GM_Array
#define Rtl819XRadioA_to1T_Array		Rtl8192SERadioA_to1T_Array
#define Rtl819XRadioA_to2T_Array		Rtl8192SERadioA_to2T_Array

/*------------------------Define local variable------------------------------*/
#if 0
static u32	RF_CHANNEL_TABLE_ZEBRA[]={
		0,
		0x085c,
		0x08dc,
		0x095c,
		0x09dc,
		0x0a5c,
		0x0adc,
		0x0b5c,
		0x0bdc,
		0x0c5c,
		0x0cdc,
		0x0d5c,
		0x0ddc,
		0x0e5c,
		0x0f72,
};
#endif

/*------------------------Define local variable------------------------------*/


/*--------------------Define export function prototype-----------------------*/
/*--------------------Define export function prototype-----------------------*/


/*---------------------Define local function prototype-----------------------*/

static  u32 phy_FwRFSerialRead( struct net_device* dev, RF90_RADIO_PATH_E eRFPath, u32 Offset); 
static	void phy_FwRFSerialWrite( struct net_device* dev, RF90_RADIO_PATH_E eRFPath, u32 Offset, u32 Data);

static u32 phy_RFSerialRead(struct net_device* dev,RF90_RADIO_PATH_E eRFPath,u32 Offset);
static void phy_RFSerialWrite( struct net_device* dev, RF90_RADIO_PATH_E eRFPath, u32 Offset, u32 Data);
static	u32 phy_CalculateBitShift(u32 BitMask);
static	bool	phy_BB8190_Config_HardCode(struct net_device* dev);
static	bool	phy_BB8192S_Config_ParaFile(struct net_device* dev);
	
static bool phy_ConfigMACWithHeaderFile(struct net_device* dev);
	
static bool phy_ConfigBBWithHeaderFile(struct net_device* dev,u8 ConfigType);

static bool phy_ConfigBBWithPgHeaderFile(struct net_device* dev,u8 ConfigType);

static bool phy_SetBBtoDiffRFWithHeaderFile(struct net_device* dev,u8 ConfigType);

static void phy_InitBBRFRegisterDefinition(struct net_device* dev);
static bool phy_SetSwChnlCmdArray(	SwChnlCmd*		CmdTable,
		u32			CmdTableIdx,
		u32			CmdTableSz,
		SwChnlCmdID		CmdID,
		u32			Para1,
		u32			Para2,
		u32			msDelay	);

static bool phy_SwChnlStepByStep(
	struct net_device* dev,
	u8		channel,
	u8		*stage,
	u8		*step,
	u32		*delay
	);

static void phy_FinishSwChnlNow(struct net_device* dev,u8 channel);

static u8 phy_DbmToTxPwrIdx( struct net_device* dev, WIRELESS_MODE WirelessMode, long PowerInDbm);
static bool phy_SetRFPowerState8192SE(struct net_device* dev,RT_RF_POWER_STATE eRFPowerState);
static void phy_CheckEphySwitchReady( struct net_device* dev);

static long phy_TxPwrIdxToDbm( struct net_device* dev, WIRELESS_MODE   WirelessMode, u8 TxPwrIdx);
void rtl8192_SetFwCmdIOCallback(struct net_device* dev);

						
/*---------------------Define local function prototype-----------------------*/


/*----------------------------Function Body----------------------------------*/
u32 rtl8192_QueryBBReg(struct net_device* dev, u32 RegAddr, u32 BitMask)
{

  	u32	ReturnValue = 0, OriginalValue, BitShift;

	RT_TRACE(COMP_RF, "--->PHY_QueryBBReg(): RegAddr(%#x), BitMask(%#x)\n", RegAddr, BitMask);

	OriginalValue = read_nic_dword(dev, RegAddr);

	BitShift = phy_CalculateBitShift(BitMask);
	ReturnValue = (OriginalValue & BitMask) >> BitShift;

	RT_TRACE(COMP_RF, "<---PHY_QueryBBReg(): RegAddr(%#x), BitMask(%#x), OriginalValue(%#x)\n", RegAddr, BitMask, OriginalValue);
	return (ReturnValue);
}

void rtl8192_setBBreg(struct net_device* dev, u32 RegAddr, u32 BitMask, u32 Data)
{
	u32	OriginalValue, BitShift, NewValue;

	{
		if(BitMask!= bMaskDWord)
		{
			OriginalValue = read_nic_dword(dev, RegAddr);
			BitShift = phy_CalculateBitShift(BitMask);
			NewValue = (((OriginalValue) & (~BitMask)) | (Data << BitShift));
			write_nic_dword(dev, RegAddr, NewValue);	
		}else
			write_nic_dword(dev, RegAddr, Data);	
	}

	return;
}


u32 rtl8192_phy_QueryRFReg(struct net_device* dev, RF90_RADIO_PATH_E eRFPath, u32 RegAddr, u32 BitMask)
{
	u32 Original_Value, Readback_Value, BitShift;
	struct r8192_priv *priv = rtllib_priv(dev);
        unsigned long flags;
	
	RT_TRACE(COMP_RF, "--->PHY_QueryRFReg(): RegAddr(%#x), eRFPath(%#x), BitMask(%#x)\n", RegAddr, eRFPath,BitMask);
	
	if (!((priv->rf_pathmap >> eRFPath) & 0x1))
		return 0;
	
	if (!rtl8192_phy_CheckIsLegalRFPath(dev, eRFPath))
		return	0;
	
	spin_lock_irqsave(&priv->rf_lock, flags);	
	if (priv->Rf_Mode == RF_OP_By_FW)
	{	
		Original_Value = phy_FwRFSerialRead(dev, eRFPath, RegAddr);
	}
	else
	{	
		Original_Value = phy_RFSerialRead(dev, eRFPath, RegAddr);	   	
	}
	
	BitShift =  phy_CalculateBitShift(BitMask);
	Readback_Value = (Original_Value & BitMask) >> BitShift;	
	spin_unlock_irqrestore(&priv->rf_lock, flags);

	
	return (Readback_Value);
}


void rtl8192_phy_SetRFReg(struct net_device* dev, RF90_RADIO_PATH_E eRFPath, u32 RegAddr, u32 BitMask, u32 Data)
{

	struct r8192_priv *priv = rtllib_priv(dev);
	u32 Original_Value, BitShift, New_Value;
        unsigned long flags;
	
	RT_TRACE(COMP_RF, "--->PHY_SetRFReg(): RegAddr(%#x), BitMask(%#x), Data(%#x), eRFPath(%#x)\n", 
		RegAddr, BitMask, Data, eRFPath);

	if (!((priv->rf_pathmap >> eRFPath) & 0x1))
		return ;
	if (!rtl8192_phy_CheckIsLegalRFPath(dev, eRFPath))
	{
		return;
	}
	
	spin_lock_irqsave(&priv->rf_lock, flags);	
	if (priv->Rf_Mode == RF_OP_By_FW)	
	{		
		if (BitMask != bRFRegOffsetMask) 
		{
			Original_Value = phy_FwRFSerialRead(dev, eRFPath, RegAddr);
			BitShift =  phy_CalculateBitShift(BitMask);
			New_Value = (((Original_Value) & (~BitMask)) | (Data<< BitShift));
		
			phy_FwRFSerialWrite(dev, eRFPath, RegAddr, New_Value);
		}
		else
			phy_FwRFSerialWrite(dev, eRFPath, RegAddr, Data);		
	}
	else
	{		
		if (BitMask != bRFRegOffsetMask) 
		{
			Original_Value = phy_RFSerialRead(dev, eRFPath, RegAddr);
			BitShift =  phy_CalculateBitShift(BitMask);
			New_Value = (((Original_Value) & (~BitMask)) | (Data<< BitShift));
		
			phy_RFSerialWrite(dev, eRFPath, RegAddr, New_Value);
		}
		else
			phy_RFSerialWrite(dev, eRFPath, RegAddr, Data);
	
	}
	spin_unlock_irqrestore(&priv->rf_lock, flags);
	RT_TRACE(COMP_RF, "<---PHY_SetRFReg(): RegAddr(%#x), BitMask(%#x), Data(%#x), eRFPath(%#x)\n", 
			RegAddr, BitMask, Data, eRFPath);
	
}

static	u32
phy_FwRFSerialRead(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset	)
{
	u32		retValue = 0;		
#if 0	
	u32		Data = 0;
	u8		time = 0;
	Data |= ((Offset&0xFF)<<12);
	Data |= ((eRFPath&0x3)<<20);
	Data |= 0x80000000;		
	while (PlatformIORead4Byte(dev, QPNR)&0x80000000)
	{
		if (time++ < 100)
		{
			delay_us(10);
		}
		else
			break;
	}
	PlatformIOWrite4Byte(dev, QPNR, Data);		
	while (PlatformIORead4Byte(dev, QPNR)&0x80000000)
	{
		if (time++ < 100)
		{
			delay_us(10);
		}
		else
			return	(0);
	}
	retValue = PlatformIORead4Byte(dev, RF_DATA);
#endif	
	return	(retValue);

}	/* phy_FwRFSerialRead */

static	void
phy_FwRFSerialWrite(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset,
	u32				Data	)
{
#if 0	
	u8	time = 0;
	DbgPrint("N FW RF CTRL RF-%d OF%02x DATA=%03x\n\r", eRFPath, Offset, Data);
	
	Data |= ((Offset&0xFF)<<12);
	Data |= ((eRFPath&0x3)<<20);
	Data |= 0x400000;
	Data |= 0x80000000;
	
	while (PlatformIORead4Byte(dev, QPNR)&0x80000000)
	{
		if (time++ < 100)
		{
			delay_us(10);
		}
		else
			break;
	}
	PlatformIOWrite4Byte(dev, QPNR, Data);
#endif		
}	/* phy_FwRFSerialWrite */

#if 0
static u32 phy_RFSerialRead(struct net_device* dev,RF90_RADIO_PATH_E eRFPath,u32 Offset)
{

	u32						retValue = 0;
	struct r8192_priv *priv = rtllib_priv(dev);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &priv->PHYRegDef[eRFPath];
	u32						NewOffset;

	Offset &= 0x3f;

	if(	priv->rf_chip == RF_8256 || 
		priv->rf_chip == RF_8225 ||	
		priv->rf_chip == RF_6052)
	{
		rtl8192_setBBreg(dev, rFPGA0_AnalogParameter4, 0xf00, 0x0);

		if(Offset>=31)
		{
			priv->RFReadPageCnt[2]++;
			priv->RfReg0Value[eRFPath] |= 0x140;

			rtl8192_setBBreg(dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);

			NewOffset = Offset - 30;
			
		}else if(Offset>=16)
		{
			priv->RFReadPageCnt[1]++;
			priv->RfReg0Value[eRFPath] |= 0x100;
			priv->RfReg0Value[eRFPath] &= (~0x40);

			rtl8192_setBBreg(dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);

			NewOffset = Offset - 15;
		}
		else
		{
			priv->RFReadPageCnt[0]++;
			NewOffset = Offset;
	}	
	}	
	else
		NewOffset = Offset;

#if 0
	{
		u32	temp1, temp2;

		temp1 = rtl8192_QueryBBReg(dev, rFPGA0_XA_HSSIParameter2, 0xffffffff);
		temp2 = rtl8192_QueryBBReg(dev, pPhyReg->rfHSSIPara2, 0xffffffff);
		temp2 = temp2 & (~bLSSIReadAddress) | (NewOffset<<24) | bLSSIReadEdge;

		rtl8192_setBBreg(dev, rFPGA0_XA_HSSIParameter2, 0xffffffff, temp1&(~bLSSIReadEdge));
		msleep(1);
		rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2, 0xffffffff, temp2);
		msleep(1);
		rtl8192_setBBreg(dev, rFPGA0_XA_HSSIParameter2, 0xffffffff, temp1|bLSSIReadEdge);
		msleep(1);
		
	}
#else
	rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2, bLSSIReadAddress, NewOffset);

	rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2,  bLSSIReadEdge, 0x0);	
	rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2,  bLSSIReadEdge, 0x1);	
#endif
	
	mdelay(1);

	retValue = rtl8192_QueryBBReg(dev, pPhyReg->rfLSSIReadBack, bLSSIReadBackData);
	
	if(	priv->rf_chip == RF_8256 || 
		priv->rf_chip == RF_8225 ||
		priv->rf_chip == RF_6052)
	{
		if (Offset >= 0x10)
		{
			priv->RfReg0Value[eRFPath] &= 0xebf;
		
			rtl8192_setBBreg(
				dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);
		}

		rtl8192_setBBreg(dev, rFPGA0_AnalogParameter4, 0xf00, 0xf);
	}
	
	return retValue;	
}


static void
phy_RFSerialWrite(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset,
	u32				Data
	)
{
	u32					DataAndAddr = 0;
	struct r8192_priv 			*priv = rtllib_priv(dev);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &priv->PHYRegDef[eRFPath];
	u32					NewOffset;

	Offset &= 0x3f;

	PHY_RFShadowWrite(dev, eRFPath, Offset, Data);	

	if( 	priv->rf_chip == RF_8256 || 
		priv->rf_chip == RF_8225 ||
		priv->rf_chip == RF_6052)
	{
		rtl8192_setBBreg(dev, rFPGA0_AnalogParameter4, 0xf00, 0x0);
				
		if(Offset>=31)
		{
			priv->RFWritePageCnt[2]++;
			priv->RfReg0Value[eRFPath] |= 0x140;
			
			rtl8192_setBBreg(dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);

			NewOffset = Offset - 30;
			
		}else if(Offset>=16)
		{
			priv->RFWritePageCnt[1]++;
			priv->RfReg0Value[eRFPath] |= 0x100;
			priv->RfReg0Value[eRFPath] &= (~0x40);			
							

			rtl8192_setBBreg(dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);

			NewOffset = Offset - 15;
		}
		else
		{
			priv->RFWritePageCnt[0]++;
			NewOffset = Offset;
	}
	}
	else
		NewOffset = Offset;

	DataAndAddr = (Data<<16) | (NewOffset&0x3f);

	rtl8192_setBBreg(dev, pPhyReg->rf3wireOffset, bMaskDWord, DataAndAddr);


	if(Offset==0x0)
		priv->RfReg0Value[eRFPath] = Data;

 	if(	priv->rf_chip == RF_8256 || 
		priv->rf_chip == RF_8225 ||
		priv->rf_chip == RF_6052)
	{
		if (Offset >= 0x10)
		{
			if(Offset != 0)
			{
				priv->RfReg0Value[eRFPath] &= 0xebf;
				rtl8192_setBBreg(
				dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);
			}
		}	
		rtl8192_setBBreg(dev, rFPGA0_AnalogParameter4, 0xf00, 0xf);
	}
	
}
#else
static	u32
phy_RFSerialRead(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset
	)
{

	u32					retValue = 0;
	struct r8192_priv 			*priv = rtllib_priv(dev);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &priv->PHYRegDef[eRFPath];
	u32					NewOffset;
	u32 					tmplong,tmplong2;
	u8					RfPiEnable=0;
#if 0
	if(priv->rf_chip == RF_8225 && Offset > 0x24) 
		return	retValue;
	if(priv->rf_chip == RF_8256 && Offset > 0x2D) 
		return	retValue;
#endif
	Offset &= 0x3f;

	NewOffset = Offset;

	tmplong = rtl8192_QueryBBReg(dev, rFPGA0_XA_HSSIParameter2, bMaskDWord);
	if(eRFPath == RF90_PATH_A)
		tmplong2 = tmplong;
	else
	tmplong2 = rtl8192_QueryBBReg(dev, pPhyReg->rfHSSIPara2, bMaskDWord);
	tmplong2 = (tmplong2 & (~bLSSIReadAddress)) | (NewOffset<<23) | bLSSIReadEdge;	

	rtl8192_setBBreg(dev, rFPGA0_XA_HSSIParameter2, bMaskDWord, tmplong&(~bLSSIReadEdge));
	udelay(1000);
	
	rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2, bMaskDWord, tmplong2);
	udelay(1000);
	
	rtl8192_setBBreg(dev, rFPGA0_XA_HSSIParameter2, bMaskDWord, tmplong|bLSSIReadEdge);

	if(eRFPath == RF90_PATH_A)
		RfPiEnable = (u8)rtl8192_QueryBBReg(dev, rFPGA0_XA_HSSIParameter1, BIT8);
	else if(eRFPath == RF90_PATH_B)
		RfPiEnable = (u8)rtl8192_QueryBBReg(dev, rFPGA0_XB_HSSIParameter1, BIT8);
	
	if(RfPiEnable)
	{	
		retValue = rtl8192_QueryBBReg(dev, pPhyReg->rfLSSIReadBackPi, bLSSIReadBackData);
	}
	else
	{	
		retValue = rtl8192_QueryBBReg(dev, pPhyReg->rfLSSIReadBack, bLSSIReadBackData);
	}
	
	retValue = rtl8192_QueryBBReg(dev, pPhyReg->rfLSSIReadBack, bLSSIReadBackData);
	
	return retValue;	
		
}

static	void
phy_RFSerialWrite(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset,
	u32				Data
	)
{
	u32					DataAndAddr = 0;
	struct r8192_priv 			*priv = rtllib_priv(dev);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &priv->PHYRegDef[eRFPath];
	u32					NewOffset;
	
#if 0
	if(priv->rf_chip == RF_8225 && Offset > 0x24) 
		return;
	if(priv->rf_chip == RF_8256 && Offset > 0x2D) 
		return;
#endif

	Offset &= 0x3f;

	PHY_RFShadowWrite(dev, eRFPath, Offset, Data);	

		NewOffset = Offset;

	DataAndAddr = ((NewOffset<<20) | (Data&0x000fffff)) & 0x0fffffff;	

	rtl8192_setBBreg(dev, pPhyReg->rf3wireOffset, bMaskDWord, DataAndAddr);
	
}

#endif

static u32 phy_CalculateBitShift(u32 BitMask)
{
	u32 i;

	for(i=0; i<=31; i++)
	{
		if ( ((BitMask>>i) &  0x1 ) == 1)
			break;
	}

	return (i);
}


extern bool PHY_MACConfig8192S(struct net_device* dev)
{
	bool rtStatus = true;

#if RTL8190_Download_Firmware_From_Header
	rtStatus = phy_ConfigMACWithHeaderFile(dev);
#else
	
	RT_TRACE(COMP_INIT, "Read MACREG.txt\n");
#endif
	return (rtStatus == true) ? 1:0;

}

extern	bool
PHY_BBConfig8192S(struct net_device* dev)
{
	bool rtStatus = true;
	u8 PathMap = 0, index = 0, rf_num = 0;
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u8 bRegHwParaFile = 1;
	
	phy_InitBBRFRegisterDefinition(dev);

	switch(bRegHwParaFile)
	{
		case 0:
			phy_BB8190_Config_HardCode(dev);
			break;

		case 1:
			rtStatus = phy_BB8192S_Config_ParaFile(dev);
			break;

		case 2:
			phy_BB8190_Config_HardCode(dev);
			phy_BB8192S_Config_ParaFile(dev);
			break;

		default:
			phy_BB8190_Config_HardCode(dev);
			break;
	}

	PathMap = (u8)(rtl8192_QueryBBReg(dev, rFPGA0_TxInfo, 0xf) |
				rtl8192_QueryBBReg(dev, rOFDM0_TRxPathEnable, 0xf));
	priv->rf_pathmap = PathMap;
	for(index = 0; index<4; index++)
	{
		if((PathMap>>index)&0x1)
			rf_num++;
	}

	if((priv->rf_type==RF_1T1R && rf_num!=1) ||
		(priv->rf_type==RF_1T2R && rf_num!=2) ||
		(priv->rf_type==RF_2T2R && rf_num!=2) ||
		(priv->rf_type==RF_2T2R_GREEN && rf_num!=2) ||
		(priv->rf_type==RF_2T4R && rf_num!=4))
	{
		RT_TRACE( COMP_INIT, "PHY_BBConfig8192S: RF_Type(%x) does not match RF_Num(%x)!!\n", priv->rf_type, rf_num);
	}
	return rtStatus;
}

extern	bool
PHY_RFConfig8192S(struct net_device* dev)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	bool    		rtStatus = true;


	if (IS_HARDWARE_TYPE_8192SE(dev))
		priv->rf_chip = RF_6052;

	switch(priv->rf_chip)
	{
	case RF_8225:
	case RF_6052:
		rtStatus = PHY_RF6052_Config(dev);
		break;
		
	case RF_8256:			
		break;
		
	case RF_8258:
		break;
		
	case RF_PSEUDO_11N:
		break;
        default:
            break;
	}

	return rtStatus;
}


static bool  
phy_BB8190_Config_HardCode(struct net_device* dev)
{
	return true;
}


static bool 	
phy_BB8192S_Config_ParaFile(struct net_device* dev)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	bool rtStatus = true;	
	
	RT_TRACE(COMP_INIT, "==>phy_BB8192S_Config_ParaFile\n");

#if	RTL8190_Download_Firmware_From_Header
	if (priv->rf_type == RF_1T2R || priv->rf_type == RF_2T2R ||
	    priv->rf_type == RF_1T1R ||priv->rf_type == RF_2T2R_GREEN)
	{
		rtStatus = phy_ConfigBBWithHeaderFile(dev,BaseBand_Config_PHY_REG);
		if(priv->rf_type != RF_2T2R && priv->rf_type != RF_2T2R_GREEN)
		{
			rtStatus = phy_SetBBtoDiffRFWithHeaderFile(dev,BaseBand_Config_PHY_REG);
		}
	}else
		rtStatus = false;
#else
	RT_TRACE(COMP_INIT, "RF_Type == %d\n", priv->rf_type);		
	if (priv->rf_type == RF_1T2R || priv->rf_type == RF_2T2R ||
	    priv->rf_type == RF_1T1R ||priv->rf_type == RF_2T2R_GREEN)
	{
		rtStatus = phy_ConfigBBWithParaFile(dev, (char* )&szBBRegFile);
		if(priv->rf_type != RF_2T2R && priv->rf_type != RF_2T2R_GREEN)
		{
			if(priv->rf_type == RF_1T1R)
				rtStatus = phy_SetBBtoDiffRFWithParaFile(dev, (char* )&szBBRegto1T1RFile);
			else if(priv->rf_type == RF_1T2R)
				rtStatus = phy_SetBBtoDiffRFWithParaFile(dev, (char* )&szBBRegto1T2RFile);
		}

	}else
		rtStatus = false;
#endif

	if(rtStatus != true){
		RT_TRACE(COMP_INIT, "phy_BB8192S_Config_ParaFile():Write BB Reg Fail!!");
		goto phy_BB8190_Config_ParaFile_Fail;
	}

	if (priv->AutoloadFailFlag == false)
	{
		priv->pwrGroupCnt = 0;

#if	RTL8190_Download_Firmware_From_Header
		rtStatus = phy_ConfigBBWithPgHeaderFile(dev,BaseBand_Config_PHY_REG);
#else
		rtStatus = phy_ConfigBBWithPgParaFile(dev, (char* )&szBBRegPgFile);
#endif
	}
	if(rtStatus != true){
		RT_TRACE(COMP_INIT, "phy_BB8192S_Config_ParaFile():BB_PG Reg Fail!!");
		goto phy_BB8190_Config_ParaFile_Fail;
	}
	
#if RTL8190_Download_Firmware_From_Header
	rtStatus = phy_ConfigBBWithHeaderFile(dev,BaseBand_Config_AGC_TAB);
#else
	RT_TRACE(COMP_INIT, "phy_BB8192S_Config_ParaFile AGC_TAB.txt\n");
	rtStatus = phy_ConfigBBWithParaFile(dev, (char* )&szAGCTableFile);
#endif

	if(rtStatus != true){
		printk( "phy_BB8192S_Config_ParaFile():AGC Table Fail\n");
		goto phy_BB8190_Config_ParaFile_Fail;
	}


#if 0	
	if(pHalData->VersionID > VERSION_8190_BD)
	{
		u4RegValue = (  pHalData->AntennaTxPwDiff[2]<<8 | 
						pHalData->AntennaTxPwDiff[1]<<4 | 
						pHalData->AntennaTxPwDiff[0]);
		
		PHY_SetBBReg(dev, rFPGA0_TxGainStage, 
			(bXBTxAGC|bXCTxAGC|bXDTxAGC), u4RegValue);
    	
		u4RegValue = pHalData->CrystalCap;
		PHY_SetBBReg(dev, rFPGA0_AnalogParameter1, bXtalCap92x, u4RegValue);

	}
#endif

	priv->bCckHighPower = (bool)(rtl8192_QueryBBReg(dev, rFPGA0_XA_HSSIParameter2, 0x200));


phy_BB8190_Config_ParaFile_Fail:	
	return rtStatus;
}

static bool	
phy_ConfigMACWithHeaderFile(struct net_device* dev)
{
	u32					i = 0;
	u32					ArrayLength = 0;
	u32*					ptrArray;	
	
	/*if(dev->bInHctTest)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("Rtl819XMACPHY_ArrayDTM\n"));
		ArrayLength = MACPHY_ArrayLengthDTM;
		ptrArray = Rtl819XMACPHY_ArrayDTM;
	}
	else if(pHalData->bTXPowerDataReadFromEEPORM)
	{

	}else*/
	{ 
		RT_TRACE(COMP_INIT, "Read Rtl819XMACPHY_Array\n");
		ArrayLength = MAC_2T_ArrayLength;
		ptrArray = Rtl819XMAC_Array;	
	}
		
	/*for(i = 0 ;i < ArrayLength;i=i+3){
		RT_TRACE(COMP_SEND, DBG_LOUD, ("The Rtl819XMACPHY_Array[0] is %lx Rtl819XMACPHY_Array[1] is %lx Rtl819XMACPHY_Array[2] is %lx\n",ptrArray[i], ptrArray[i+1], ptrArray[i+2]));
		if(ptrArray[i] == 0x318)
		{
			ptrArray[i+2] = 0x00000800;
		}
		PHY_SetBBReg(dev, ptrArray[i], ptrArray[i+1], ptrArray[i+2]);
	}*/
	for(i = 0 ;i < ArrayLength;i=i+2){ 
		write_nic_byte(dev, ptrArray[i], (u8)ptrArray[i+1]);
	}
	return true;
}


static bool 	
phy_ConfigBBWithHeaderFile(struct net_device* dev,u8 ConfigType)
{
	int 		i;
	u32*	Rtl819XPHY_REGArray_Table;
	u32*	Rtl819XAGCTAB_Array_Table;
	u16		PHY_REGArrayLen, AGCTAB_ArrayLen;
	/*if(dev->bInHctTest)
	{	
	
		AGCTAB_ArrayLen = AGCTAB_ArrayLengthDTM;
		Rtl819XAGCTAB_Array_Table = Rtl819XAGCTAB_ArrayDTM;

		if(pHalData->RF_Type == RF_2T4R)
		{
			PHY_REGArrayLen = PHY_REGArrayLengthDTM;
			Rtl819XPHY_REGArray_Table = Rtl819XPHY_REGArrayDTM;
		}
		else if (pHalData->RF_Type == RF_1T2R)
		{
			PHY_REGArrayLen = PHY_REG_1T2RArrayLengthDTM;
			Rtl819XPHY_REGArray_Table = Rtl819XPHY_REG_1T2RArrayDTM;
		}		
	
	}
	else
	*/
	AGCTAB_ArrayLen = AGCTAB_ArrayLength;
	Rtl819XAGCTAB_Array_Table = Rtl819XAGCTAB_Array;
	PHY_REGArrayLen = PHY_REG_2T2RArrayLength; 
	Rtl819XPHY_REGArray_Table = Rtl819XPHY_REG_Array;

	if(ConfigType == BaseBand_Config_PHY_REG)
	{
		for(i=0;i<PHY_REGArrayLen;i=i+2)
		{
			if (Rtl819XPHY_REGArray_Table[i] == 0xfe)
				mdelay(50);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xfd)
				mdelay(5);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xfc)
				mdelay(1);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xfb)
				udelay(50);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xfa)
				udelay(5);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xf9)
				udelay(1);

			udelay(1);

			rtl8192_setBBreg(dev, Rtl819XPHY_REGArray_Table[i], bMaskDWord, Rtl819XPHY_REGArray_Table[i+1]);		

		}
	}
	else if(ConfigType == BaseBand_Config_AGC_TAB){
		for(i=0;i<AGCTAB_ArrayLen;i=i+2)
		{
			rtl8192_setBBreg(dev, Rtl819XAGCTAB_Array_Table[i], bMaskDWord, Rtl819XAGCTAB_Array_Table[i+1]);		
			udelay(1);
		}
	}
	return true;
}


static bool  
phy_SetBBtoDiffRFWithHeaderFile(struct net_device* dev, u8 ConfigType)
{
	int i;
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u32* 			Rtl819XPHY_REGArraytoXTXR_Table;
	u16				PHY_REGArraytoXTXRLen;
	
	if(priv->rf_type == RF_1T1R)
	{
		Rtl819XPHY_REGArraytoXTXR_Table = Rtl819XPHY_REG_to1T1R_Array;
		PHY_REGArraytoXTXRLen = PHY_ChangeTo_1T1RArrayLength;
	} 
	else if(priv->rf_type == RF_1T2R)
	{
		Rtl819XPHY_REGArraytoXTXR_Table = Rtl819XPHY_REG_to1T2R_Array;
		PHY_REGArraytoXTXRLen = PHY_ChangeTo_1T2RArrayLength;
	}
	else
	{
		return false;
	}

	if(ConfigType == BaseBand_Config_PHY_REG)
	{
		for(i=0;i<PHY_REGArraytoXTXRLen;i=i+3)
		{
			if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xfe)
				mdelay(50);
			else if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xfd)
				mdelay(5);
			else if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xfc)
				mdelay(1);
			else if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xfb)
				udelay(50);
			else if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xfa)
				udelay(5);
			else if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xf9)
				udelay(1);
			rtl8192_setBBreg(dev, Rtl819XPHY_REGArraytoXTXR_Table[i], Rtl819XPHY_REGArraytoXTXR_Table[i+1], Rtl819XPHY_REGArraytoXTXR_Table[i+2]);		
		}
	}
	else {
		RT_TRACE(COMP_SEND, "phy_SetBBtoDiffRFWithHeaderFile(): ConfigType != BaseBand_Config_PHY_REG\n");
	}
	return true;
}

void
storePwrIndexDiffRateOffset(
	struct net_device* dev,
	u32		RegAddr,
	u32		BitMask,
	u32		Data
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	if(RegAddr == rTxAGC_Rate18_06)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][0] = Data;
	}
	if(RegAddr == rTxAGC_Rate54_24)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][1] = Data;
	}
	if(RegAddr == rTxAGC_CCK_Mcs32)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][6] = Data;
	}
	if(RegAddr == rTxAGC_Mcs03_Mcs00)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][2] = Data;
	}
	if(RegAddr == rTxAGC_Mcs07_Mcs04)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][3] = Data;
	}
	if(RegAddr == rTxAGC_Mcs11_Mcs08)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][4] = Data;
	}
	if(RegAddr == rTxAGC_Mcs15_Mcs12)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][5] = Data;
		priv->pwrGroupCnt++;
	}
}

static bool 
phy_ConfigBBWithPgHeaderFile(struct net_device* dev,u8 ConfigType)
{
	int i;
	u32*	Rtl819XPHY_REGArray_Table_PG;
	u16	PHY_REGArrayPGLen;

	PHY_REGArrayPGLen = PHY_REG_Array_PGLength;
	Rtl819XPHY_REGArray_Table_PG = Rtl819XPHY_REG_Array_PG;

	if(ConfigType == BaseBand_Config_PHY_REG)
	{
		for(i=0;i<PHY_REGArrayPGLen;i=i+3)
		{
			if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfe)
				mdelay(50);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfd)
				mdelay(5);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfc)
				mdelay(1);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfb)
				udelay(50);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfa)
				udelay(5);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xf9)
				udelay(1);
			storePwrIndexDiffRateOffset(dev, Rtl819XPHY_REGArray_Table_PG[i], 
				Rtl819XPHY_REGArray_Table_PG[i+1], 
				Rtl819XPHY_REGArray_Table_PG[i+2]);
			rtl8192_setBBreg(dev, Rtl819XPHY_REGArray_Table_PG[i], Rtl819XPHY_REGArray_Table_PG[i+1], Rtl819XPHY_REGArray_Table_PG[i+2]);		
		}
	}
	else{
		RT_TRACE(COMP_SEND, "phy_ConfigBBWithPgHeaderFile(): ConfigType != BaseBand_Config_PHY_REG\n");
	}
	return true;
	
}	/* phy_ConfigBBWithPgHeaderFile */

RT_STATUS rtl8192_phy_configRFPABiascurrent(struct net_device *dev, RF90_RADIO_PATH_E eRFPath)
{
	struct r8192_priv  *priv = rtllib_priv(dev);
	RT_STATUS       rtStatus = RT_STATUS_SUCCESS;
	u32          tmpval=0;

	if(priv->IC_Class != IC_INFERIORITY_A)
	{
		tmpval = rtl8192_phy_QueryRFReg(dev, eRFPath, RF_IPA, 0xf);
		rtl8192_phy_SetRFReg(dev, eRFPath, RF_IPA, 0xf, tmpval+1);
	}

	return rtStatus;
}

u8 rtl8192_phy_ConfigRFWithHeaderFile(struct net_device* dev, RF90_RADIO_PATH_E	eRFPath)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	int			i;
	bool 		rtStatus = true;
	u32			*Rtl819XRadioA_Array_Table;
	u32			*Rtl819XRadioB_Array_Table;
	u16			RadioA_ArrayLen,RadioB_ArrayLen;

	RadioA_ArrayLen = RadioA_1T_ArrayLength;
	Rtl819XRadioA_Array_Table=Rtl819XRadioA_Array;

	if(priv->rf_type == RF_2T2R_GREEN)
	{
		Rtl819XRadioB_Array_Table = Rtl819XRadioB_GM_Array;
		RadioB_ArrayLen = RadioB_GM_ArrayLength;
	}
	else
	{		
		Rtl819XRadioB_Array_Table=Rtl819XRadioB_Array;
		RadioB_ArrayLen = RadioB_ArrayLength;	
	}


	RT_TRACE(COMP_INIT, "PHY_ConfigRFWithHeaderFile: Radio No %x\n", eRFPath);
	rtStatus = true;

	switch(eRFPath){
		case RF90_PATH_A:
			for(i = 0;i<RadioA_ArrayLen; i=i+2)
			{
				if(Rtl819XRadioA_Array_Table[i] == 0xfe)
				{ 
					mdelay(50);
				}
				else if (Rtl819XRadioA_Array_Table[i] == 0xfd)
					mdelay(5);
				else if (Rtl819XRadioA_Array_Table[i] == 0xfc)
					mdelay(1);
				else if (Rtl819XRadioA_Array_Table[i] == 0xfb)
					udelay(50);
				else if (Rtl819XRadioA_Array_Table[i] == 0xfa)
					udelay(5);
				else if (Rtl819XRadioA_Array_Table[i] == 0xf9)
					udelay(1);
				else {
					rtl8192_phy_SetRFReg(dev, eRFPath, Rtl819XRadioA_Array_Table[i], 
							bMask20Bits, Rtl819XRadioA_Array_Table[i+1]);
				}

				udelay(1);
			}
			rtl8192_phy_configRFPABiascurrent(dev, eRFPath);
			break;
		case RF90_PATH_B:
			for(i = 0;i<RadioB_ArrayLen; i=i+2){
				if(Rtl819XRadioB_Array_Table[i] == 0xfe)
				{ 
					mdelay(50);
				}
				else if (Rtl819XRadioB_Array_Table[i] == 0xfd)
					mdelay(5);
				else if (Rtl819XRadioB_Array_Table[i] == 0xfc)
					mdelay(1);
				else if (Rtl819XRadioB_Array_Table[i] == 0xfb)
					udelay(50);
				else if (Rtl819XRadioB_Array_Table[i] == 0xfa)
					udelay(5);
				else if (Rtl819XRadioB_Array_Table[i] == 0xf9)
					udelay(1);
				else
				{
					rtl8192_phy_SetRFReg(dev, eRFPath, Rtl819XRadioB_Array_Table[i], bMask20Bits, Rtl819XRadioB_Array_Table[i+1]);
				}
				
			        udelay(1);
			}			
			break;
		case RF90_PATH_C:
			break;
		case RF90_PATH_D:
			break;
		default:
			break;
	}
	return rtStatus;

}

bool rtl8192_phy_checkBBAndRF(
	struct net_device* dev,
	HW90_BLOCK_E		CheckBlock,
	RF90_RADIO_PATH_E	eRFPath
	)
{
	bool rtStatus = true;
	u32				i, CheckTimes = 4,ulRegRead = 0;
	u32				WriteAddr[4];
	u32				WriteData[] = {0xfffff027, 0xaa55a02f, 0x00000027, 0x55aa502f};

	WriteAddr[HW90_BLOCK_MAC] = 0x100;
	WriteAddr[HW90_BLOCK_PHY0] = 0x900;
	WriteAddr[HW90_BLOCK_PHY1] = 0x800;
	WriteAddr[HW90_BLOCK_RF] = 0x3;
	
	for(i=0 ; i < CheckTimes ; i++)
	{

		switch(CheckBlock)
		{
		case HW90_BLOCK_MAC:
			RT_TRACE(COMP_INIT, "PHY_CheckBBRFOK(): Never Write 0x100 here!\n");
			break;
			
		case HW90_BLOCK_PHY0:
		case HW90_BLOCK_PHY1:
			write_nic_dword(dev, WriteAddr[CheckBlock], WriteData[i]);
			ulRegRead = read_nic_dword(dev, WriteAddr[CheckBlock]);
			break;

		case HW90_BLOCK_RF:
			WriteData[i] &= 0xfff;
			rtl8192_phy_SetRFReg(dev, eRFPath, WriteAddr[HW90_BLOCK_RF], bMask20Bits, WriteData[i]);
			mdelay(10);
			ulRegRead = rtl8192_phy_QueryRFReg(dev, eRFPath, WriteAddr[HW90_BLOCK_RF], bMaskDWord);				
			mdelay(10);
			break;
			
		default:
			rtStatus = false;
			break;
		}


		if(ulRegRead != WriteData[i])
		{
			RT_TRACE(COMP_ERR, "read back error(read:%x, write:%x)\n", ulRegRead, WriteData[i]);
			rtStatus = false;			
			break;
		}
	}

	return rtStatus;
}

#ifdef TO_DO_LIST
void
PHY_SetRFPowerState8192SUsb(
	struct net_device* dev,
	RF_POWER_STATE	RFPowerState
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	bool			WaitShutDown = false;
	u32			DWordContent;
	u8				eRFPath;
	BB_REGISTER_DEFINITION_T	*pPhyReg;
	
	if(priv->SetRFPowerStateInProgress == true)
		return;
	
	priv->SetRFPowerStateInProgress = true;
	

	if(RFPowerState==RF_SHUT_DOWN)
	{
		RFPowerState=RF_OFF;
		WaitShutDown=true;
	}
	
	
	priv->RFPowerState = RFPowerState;
	switch( priv->rf_chip )
	{
	case RF_8225:
	case RF_6052:
		switch( RFPowerState )
		{
		case RF_ON:
			break;
	
		case RF_SLEEP:
			break;
	
		case RF_OFF:
			break;
		}
		break;

	case RF_8256:
		switch( RFPowerState )
		{
		case RF_ON:
			break;
	
		case RF_SLEEP:
			break;
	
		case RF_OFF:
			for(eRFPath=(RF90_RADIO_PATH_E)RF90_PATH_A; eRFPath < RF90_PATH_MAX; eRFPath++)
			{
				if (!rtl8192_phy_CheckIsLegalRFPath(dev, eRFPath))		
					continue;	
				
				pPhyReg = &priv->PHYRegDef[eRFPath];
				rtl8192_setBBreg(dev, pPhyReg->rfintfs, bRFSI_RFENV, bRFSI_RFENV);
				rtl8192_setBBreg(dev, pPhyReg->rfintfo, bRFSI_RFENV, 0);
			}
			break;
		}
		break;
		
	case RF_8258:
		break;
	}

	priv->SetRFPowerStateInProgress = false;
}
#endif

void PHY_GetHWRegOriginalValue(struct net_device* dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
#if 0
	priv->MCSTxPowerLevelOriginalOffset[0] =
		rtl8192_QueryBBReg(dev, rTxAGC_Rate18_06, bMaskDWord);
	priv->MCSTxPowerLevelOriginalOffset[1] =
		rtl8192_QueryBBReg(dev, rTxAGC_Rate54_24, bMaskDWord);
	priv->MCSTxPowerLevelOriginalOffset[2] =
		rtl8192_QueryBBReg(dev, rTxAGC_Mcs03_Mcs00, bMaskDWord);
	priv->MCSTxPowerLevelOriginalOffset[3] =
		rtl8192_QueryBBReg(dev, rTxAGC_Mcs07_Mcs04, bMaskDWord);
	priv->MCSTxPowerLevelOriginalOffset[4] =
		rtl8192_QueryBBReg(dev, rTxAGC_Mcs11_Mcs08, bMaskDWord);
	priv->MCSTxPowerLevelOriginalOffset[5] =
		rtl8192_QueryBBReg(dev, rTxAGC_Mcs15_Mcs12, bMaskDWord);

	priv->CCKTxPowerLevelOriginalOffset=
		rtl8192_QueryBBReg(dev, rTxAGC_CCK_Mcs32, bMaskDWord);
	RT_TRACE(COMP_INIT, "Legacy OFDM =%08x/%08x HT_OFDM=%08x/%08x/%08x/%08x\n", 
	priv->MCSTxPowerLevelOriginalOffset[0], priv->MCSTxPowerLevelOriginalOffset[1] ,
	priv->MCSTxPowerLevelOriginalOffset[2], priv->MCSTxPowerLevelOriginalOffset[3] ,
	priv->MCSTxPowerLevelOriginalOffset[4], priv->MCSTxPowerLevelOriginalOffset[5] );
#endif

	priv->DefaultInitialGain[0] = rtl8192_QueryBBReg(dev, rOFDM0_XAAGCCore1, bMaskByte0);
	priv->DefaultInitialGain[1] = rtl8192_QueryBBReg(dev, rOFDM0_XBAGCCore1, bMaskByte0);
	priv->DefaultInitialGain[2] = rtl8192_QueryBBReg(dev, rOFDM0_XCAGCCore1, bMaskByte0);
	priv->DefaultInitialGain[3] = rtl8192_QueryBBReg(dev, rOFDM0_XDAGCCore1, bMaskByte0);
	RT_TRACE(COMP_INIT, "Default initial gain (c50=0x%x, c58=0x%x, c60=0x%x, c68=0x%x) \n", 
			priv->DefaultInitialGain[0], priv->DefaultInitialGain[1], 
			priv->DefaultInitialGain[2], priv->DefaultInitialGain[3]);

	priv->framesync = rtl8192_QueryBBReg(dev, rOFDM0_RxDetector3, bMaskByte0);
	priv->framesyncC34 = rtl8192_QueryBBReg(dev, rOFDM0_RxDetector2, bMaskDWord);
	RT_TRACE(COMP_INIT, "Default framesync (0x%x) = 0x%x \n", 
				rOFDM0_RxDetector3, priv->framesync);

}



static void phy_InitBBRFRegisterDefinition(	struct net_device* dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	priv->PHYRegDef[RF90_PATH_A].rfintfs = rFPGA0_XAB_RFInterfaceSW; 
	priv->PHYRegDef[RF90_PATH_B].rfintfs = rFPGA0_XAB_RFInterfaceSW; 
	priv->PHYRegDef[RF90_PATH_C].rfintfs = rFPGA0_XCD_RFInterfaceSW;
	priv->PHYRegDef[RF90_PATH_D].rfintfs = rFPGA0_XCD_RFInterfaceSW;

	priv->PHYRegDef[RF90_PATH_A].rfintfi = rFPGA0_XAB_RFInterfaceRB; 
	priv->PHYRegDef[RF90_PATH_B].rfintfi = rFPGA0_XAB_RFInterfaceRB;
	priv->PHYRegDef[RF90_PATH_C].rfintfi = rFPGA0_XCD_RFInterfaceRB;
	priv->PHYRegDef[RF90_PATH_D].rfintfi = rFPGA0_XCD_RFInterfaceRB;

	priv->PHYRegDef[RF90_PATH_A].rfintfo = rFPGA0_XA_RFInterfaceOE; 
	priv->PHYRegDef[RF90_PATH_B].rfintfo = rFPGA0_XB_RFInterfaceOE; 
	priv->PHYRegDef[RF90_PATH_C].rfintfo = rFPGA0_XC_RFInterfaceOE;
	priv->PHYRegDef[RF90_PATH_D].rfintfo = rFPGA0_XD_RFInterfaceOE;

	priv->PHYRegDef[RF90_PATH_A].rfintfe = rFPGA0_XA_RFInterfaceOE; 
	priv->PHYRegDef[RF90_PATH_B].rfintfe = rFPGA0_XB_RFInterfaceOE; 
	priv->PHYRegDef[RF90_PATH_C].rfintfe = rFPGA0_XC_RFInterfaceOE;
	priv->PHYRegDef[RF90_PATH_D].rfintfe = rFPGA0_XD_RFInterfaceOE;

	priv->PHYRegDef[RF90_PATH_A].rf3wireOffset = rFPGA0_XA_LSSIParameter; 
	priv->PHYRegDef[RF90_PATH_B].rf3wireOffset = rFPGA0_XB_LSSIParameter;
	priv->PHYRegDef[RF90_PATH_C].rf3wireOffset = rFPGA0_XC_LSSIParameter;
	priv->PHYRegDef[RF90_PATH_D].rf3wireOffset = rFPGA0_XD_LSSIParameter;

	priv->PHYRegDef[RF90_PATH_A].rfLSSI_Select = rFPGA0_XAB_RFParameter;  
	priv->PHYRegDef[RF90_PATH_B].rfLSSI_Select = rFPGA0_XAB_RFParameter;
	priv->PHYRegDef[RF90_PATH_C].rfLSSI_Select = rFPGA0_XCD_RFParameter;
	priv->PHYRegDef[RF90_PATH_D].rfLSSI_Select = rFPGA0_XCD_RFParameter;

	priv->PHYRegDef[RF90_PATH_A].rfTxGainStage = rFPGA0_TxGainStage; 
	priv->PHYRegDef[RF90_PATH_B].rfTxGainStage = rFPGA0_TxGainStage; 
	priv->PHYRegDef[RF90_PATH_C].rfTxGainStage = rFPGA0_TxGainStage; 
	priv->PHYRegDef[RF90_PATH_D].rfTxGainStage = rFPGA0_TxGainStage; 

	priv->PHYRegDef[RF90_PATH_A].rfHSSIPara1 = rFPGA0_XA_HSSIParameter1;  
	priv->PHYRegDef[RF90_PATH_B].rfHSSIPara1 = rFPGA0_XB_HSSIParameter1;  
	priv->PHYRegDef[RF90_PATH_C].rfHSSIPara1 = rFPGA0_XC_HSSIParameter1;  
	priv->PHYRegDef[RF90_PATH_D].rfHSSIPara1 = rFPGA0_XD_HSSIParameter1;  

	priv->PHYRegDef[RF90_PATH_A].rfHSSIPara2 = rFPGA0_XA_HSSIParameter2;  
	priv->PHYRegDef[RF90_PATH_B].rfHSSIPara2 = rFPGA0_XB_HSSIParameter2;  
	priv->PHYRegDef[RF90_PATH_C].rfHSSIPara2 = rFPGA0_XC_HSSIParameter2;  
	priv->PHYRegDef[RF90_PATH_D].rfHSSIPara2 = rFPGA0_XD_HSSIParameter2;  

	priv->PHYRegDef[RF90_PATH_A].rfSwitchControl = rFPGA0_XAB_SwitchControl; 
	priv->PHYRegDef[RF90_PATH_B].rfSwitchControl = rFPGA0_XAB_SwitchControl;
	priv->PHYRegDef[RF90_PATH_C].rfSwitchControl = rFPGA0_XCD_SwitchControl;
	priv->PHYRegDef[RF90_PATH_D].rfSwitchControl = rFPGA0_XCD_SwitchControl;

	priv->PHYRegDef[RF90_PATH_A].rfAGCControl1 = rOFDM0_XAAGCCore1;
	priv->PHYRegDef[RF90_PATH_B].rfAGCControl1 = rOFDM0_XBAGCCore1;
	priv->PHYRegDef[RF90_PATH_C].rfAGCControl1 = rOFDM0_XCAGCCore1;
	priv->PHYRegDef[RF90_PATH_D].rfAGCControl1 = rOFDM0_XDAGCCore1;

	priv->PHYRegDef[RF90_PATH_A].rfAGCControl2 = rOFDM0_XAAGCCore2;
	priv->PHYRegDef[RF90_PATH_B].rfAGCControl2 = rOFDM0_XBAGCCore2;
	priv->PHYRegDef[RF90_PATH_C].rfAGCControl2 = rOFDM0_XCAGCCore2;
	priv->PHYRegDef[RF90_PATH_D].rfAGCControl2 = rOFDM0_XDAGCCore2;

	priv->PHYRegDef[RF90_PATH_A].rfRxIQImbalance = rOFDM0_XARxIQImbalance;
	priv->PHYRegDef[RF90_PATH_B].rfRxIQImbalance = rOFDM0_XBRxIQImbalance;
	priv->PHYRegDef[RF90_PATH_C].rfRxIQImbalance = rOFDM0_XCRxIQImbalance;
	priv->PHYRegDef[RF90_PATH_D].rfRxIQImbalance = rOFDM0_XDRxIQImbalance;	

	priv->PHYRegDef[RF90_PATH_A].rfRxAFE = rOFDM0_XARxAFE;
	priv->PHYRegDef[RF90_PATH_B].rfRxAFE = rOFDM0_XBRxAFE;
	priv->PHYRegDef[RF90_PATH_C].rfRxAFE = rOFDM0_XCRxAFE;
	priv->PHYRegDef[RF90_PATH_D].rfRxAFE = rOFDM0_XDRxAFE;	

	priv->PHYRegDef[RF90_PATH_A].rfTxIQImbalance = rOFDM0_XATxIQImbalance;
	priv->PHYRegDef[RF90_PATH_B].rfTxIQImbalance = rOFDM0_XBTxIQImbalance;
	priv->PHYRegDef[RF90_PATH_C].rfTxIQImbalance = rOFDM0_XCTxIQImbalance;
	priv->PHYRegDef[RF90_PATH_D].rfTxIQImbalance = rOFDM0_XDTxIQImbalance;	

	priv->PHYRegDef[RF90_PATH_A].rfTxAFE = rOFDM0_XATxAFE;
	priv->PHYRegDef[RF90_PATH_B].rfTxAFE = rOFDM0_XBTxAFE;
	priv->PHYRegDef[RF90_PATH_C].rfTxAFE = rOFDM0_XCTxAFE;
	priv->PHYRegDef[RF90_PATH_D].rfTxAFE = rOFDM0_XDTxAFE;	

	priv->PHYRegDef[RF90_PATH_A].rfLSSIReadBack = rFPGA0_XA_LSSIReadBack;
	priv->PHYRegDef[RF90_PATH_B].rfLSSIReadBack = rFPGA0_XB_LSSIReadBack;
	priv->PHYRegDef[RF90_PATH_C].rfLSSIReadBack = rFPGA0_XC_LSSIReadBack;
	priv->PHYRegDef[RF90_PATH_D].rfLSSIReadBack = rFPGA0_XD_LSSIReadBack;	

	priv->PHYRegDef[RF90_PATH_A].rfLSSIReadBackPi = TransceiverA_HSPI_Readback;
	priv->PHYRegDef[RF90_PATH_B].rfLSSIReadBackPi = TransceiverB_HSPI_Readback;

}


bool PHY_SetRFPowerState(struct net_device* dev, RT_RF_POWER_STATE eRFPowerState)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	bool			bResult = false;

	RT_TRACE((COMP_PS | COMP_RF), "---------> PHY_SetRFPowerState(): eRFPowerState(%d)\n", eRFPowerState);

	if(eRFPowerState == priv->rtllib->eRFPowerState)
	{
		;
		return bResult;
	}

	bResult = phy_SetRFPowerState8192SE(dev, eRFPowerState);
	
	RT_TRACE((COMP_PS | COMP_RF), "<--------- PHY_SetRFPowerState(): bResult(%d)\n", bResult);

	return bResult;
}

static bool phy_SetRFPowerState8192SE(struct net_device* dev,RT_RF_POWER_STATE eRFPowerState)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PRT_POWER_SAVE_CONTROL	pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));
	bool 	bResult = true;
	u8		i, QueueID;
	struct rtl8192_tx_ring  *ring = NULL;
	priv->SetRFPowerStateInProgress = true;
	
	switch(priv->rf_chip )
	{
		default:	
		switch( eRFPowerState )
		{
			case eRfOn:
                                RT_TRACE(COMP_PS,"========>%s():eRfOn\n", __func__);
				{
									if((priv->rtllib->eRFPowerState == eRfOff) && RT_IN_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC))
					{ 
						bool rtstatus = true;
						u32 InitializeCount = 0;
						do
						{	
							InitializeCount++;
							rtstatus = NicIFEnableNIC(dev);
						}while( (rtstatus != true) &&(InitializeCount < 10) );
						if(rtstatus != true)
						{
							RT_TRACE(COMP_ERR,"%s():Initialize Adapter fail,return\n",__FUNCTION__);
							priv->SetRFPowerStateInProgress = false;
							return false;
						}
						RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);
					}
					else
					{ 
						write_nic_word(dev, CMDR, 0x37FC);
						write_nic_byte(dev, TXPAUSE, 0x00);
						write_nic_byte(dev, PHY_CCA, 0x3);
					}

#if 1
					if(priv->rtllib->state == RTLLIB_LINKED)
					{
						LedControl8192SE(dev, LED_CTL_LINK); 
					}
					else
					{
						LedControl8192SE(dev, LED_CTL_NO_LINK); 
					}
#endif
				}
				break;
			case eRfOff:  
                                RT_TRACE(COMP_PS,"========>%s():eRfOff\n", __func__);
				{					
					for(QueueID = 0, i = 0; QueueID < MAX_TX_QUEUE; )
					{
						if(QueueID == 5) 
						{
							QueueID++;
							continue;
						}
										
						ring = &priv->tx_ring[QueueID];
						if(skb_queue_len(&ring->queue) == 0)
						{
							QueueID++;
							continue;
						}
					#ifdef TO_DO_LIST
					#if( DEV_BUS_TYPE==PCI_INTERFACE)
						else if(IsLowPowerState(Adapter))
						{
							RT_TRACE(COMP_PS, DBG_LOUD, 
							("eRf Off/Sleep: %d times TcbBusyQueue[%d] !=0 but lower power state!\n", (i+1), QueueID));
							break;
						}
					#endif
					#endif
						else
						{
							RT_TRACE(COMP_PS, "eRf Off/Sleep: %d times TcbBusyQueue[%d] !=0 before doze!\n", (i+1), QueueID);
							udelay(10);
							i++;
						}
						
						if(i >= MAX_DOZE_WAITING_TIMES_9x)
						{
							RT_TRACE(COMP_PS, "\n\n\n %s(): eRfOff: %d times TcbBusyQueue[%d] != 0 !!!\n\n\n", __FUNCTION__,MAX_DOZE_WAITING_TIMES_9x, QueueID);
							break;
						}
					}

					if(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_HALT_NIC && !RT_IN_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC))
					{ 
						NicIFDisableNIC(dev);
						RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);
						if(priv->pwrdown && priv->rtllib->RfOffReason>= RF_CHANGE_BY_HW)
							write_nic_byte(dev,0x03,0x31);
					}
					else  if(!(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_HALT_NIC))
					{ 
						SET_RTL8192SE_RF_SLEEP(dev);

#if 1
						if(priv->rtllib->RfOffReason == RF_CHANGE_BY_IPS )
						{
							LedControl8192SE(dev,LED_CTL_NO_LINK); 
						}
						else
						{
							LedControl8192SE(dev, LED_CTL_POWER_OFF); 
						}
#endif
					}
				}
				break;

			case eRfSleep:
                                RT_TRACE(COMP_PS,"========>%s():eRfSleep\n", __func__);
				{
					if(priv->rtllib->eRFPowerState == eRfOff)
						break;
					
					for(QueueID = 0, i = 0; QueueID < MAX_TX_QUEUE; )
					{
						ring = &priv->tx_ring[QueueID];
						if(skb_queue_len(&ring->queue) == 0)
						{
							QueueID++;
							continue;
						}
						#ifdef TO_DO_LIST
						#if( DEV_BUS_TYPE==PCI_INTERFACE)
							else if(IsLowPowerState(Adapter))
							{
								RT_TRACE(COMP_PS, "eRf Off/Sleep: %d times TcbBusyQueue[%d] !=0 but lower power state!\n", (i+1), QueueID);
								break;
							}
						#endif
						#endif
						else
						{
							RT_TRACE(COMP_PS, "eRf Off/Sleep: %d times TcbBusyQueue[%d] !=0 before doze!\n", (i+1), QueueID);
							udelay(10);
							i++;
						}
						
						if(i >= MAX_DOZE_WAITING_TIMES_9x)
						{
							RT_TRACE(COMP_PS, "\n\n\n %s(): eRfOff: %d times TcbBusyQueue[%d] != 0 !!!\n\n\n", __FUNCTION__,MAX_DOZE_WAITING_TIMES_9x, QueueID);
							break;
						}
					}		

					SET_RTL8192SE_RF_SLEEP(dev);
				}
				break;

			default:
				bResult = false;
				RT_TRACE(COMP_ERR, "phy_SetRFPowerState8192S(): unknow state to set: 0x%X!!!\n", eRFPowerState);
				break;
		} 
		break;
	}

	if(bResult)
	{
		priv->rtllib->eRFPowerState = eRFPowerState;
	}
	
	priv->SetRFPowerStateInProgress = false;

	return bResult;
}


void
PHY_SwitchEphyParameter(struct net_device* dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	write_nic_dword(dev, 0x540, 0x73c11);
	write_nic_dword(dev, 0x548, 0x2407c);
	
	write_nic_word(dev, 0x550, 0x1000);
	write_nic_byte(dev, 0x554, 0x20);	
	phy_CheckEphySwitchReady(dev);

	write_nic_word(dev, 0x550, 0xa0eb);
	write_nic_byte(dev, 0x554, 0x3e);
	phy_CheckEphySwitchReady(dev);
	
	write_nic_word(dev, 0x550, 0xff80);
	write_nic_byte(dev, 0x554, 0x39);
	phy_CheckEphySwitchReady(dev);

	if(priv->bSupportASPM && !priv->bSupportBackDoor)
		write_nic_byte(dev, 0x560, 0x40);
	else
	{
		write_nic_byte(dev, 0x560, 0x00);
	
		if (priv->CustomerID == RT_CID_819x_SAMSUNG ||
			priv->CustomerID == RT_CID_819x_Lenovo)
		{
			if (priv->NdisAdapter.PciBridgeVendor == PCI_BRIDGE_VENDOR_AMD ||
				priv->NdisAdapter.PciBridgeVendor == PCI_BRIDGE_VENDOR_ATI)
			{
				write_nic_byte(dev, 0x560, 0x40);
			}
		}
	}
	
}	


 void
PHY_GetTxPowerLevel8192S(
	struct net_device* dev,
	 long*    		powerlevel
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8			TxPwrLevel = 0;
	long			TxPwrDbm;

	TxPwrLevel = priv->CurrentCckTxPwrIdx;
	TxPwrDbm = phy_TxPwrIdxToDbm(dev, WIRELESS_MODE_B, TxPwrLevel);

	TxPwrLevel = priv->CurrentOfdm24GTxPwrIdx + priv->LegacyHTTxPowerDiff;

	if(phy_TxPwrIdxToDbm(dev, WIRELESS_MODE_G, TxPwrLevel) > TxPwrDbm)
		TxPwrDbm = phy_TxPwrIdxToDbm(dev, WIRELESS_MODE_G, TxPwrLevel);
	TxPwrLevel = priv->CurrentOfdm24GTxPwrIdx;

	if(phy_TxPwrIdxToDbm(dev, WIRELESS_MODE_N_24G, TxPwrLevel) > TxPwrDbm)
		TxPwrDbm = phy_TxPwrIdxToDbm(dev, WIRELESS_MODE_N_24G, TxPwrLevel);
	*powerlevel = TxPwrDbm;
}

#if 1
void getTxPowerIndex(
	struct net_device* 	dev,
	u8				channel,
	u8*				cckPowerLevel,
	u8*				ofdmPowerLevel
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8				index = (channel -1);
	cckPowerLevel[0] = priv->RfTxPwrLevelCck[0][index];	
	cckPowerLevel[1] = priv->RfTxPwrLevelCck[1][index];	

	if (priv->rf_type == RF_1T2R || priv->rf_type == RF_1T1R)
	{
		ofdmPowerLevel[0] = priv->RfTxPwrLevelOfdm1T[0][index];
		ofdmPowerLevel[1] = priv->RfTxPwrLevelOfdm1T[1][index];
	}
	else if (priv->rf_type == RF_2T2R)
	{
		ofdmPowerLevel[0] = priv->RfTxPwrLevelOfdm2T[0][index];
		ofdmPowerLevel[1] = priv->RfTxPwrLevelOfdm2T[1][index];
	}
	RT_TRACE(COMP_POWER,"Channel-%d, set tx power index !!\n", channel);
}

void ccxPowerIndexCheck(
	struct net_device* 	dev,
	u8				channel,
	u8*				cckPowerLevel,
	u8*				ofdmPowerLevel
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
#ifdef TODO 
	if(	priv->rtllib->iw_mode != IW_MODE_INFRA && priv->bWithCcxCellPwr &&
		channel == priv->rtllib->current_network.channel)
	{
		u8	CckCellPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_B, priv->CcxCellPwr);
		u8	LegacyOfdmCellPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_G, priv->CcxCellPwr);
		u8	OfdmCellPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_N_24G, priv->CcxCellPwr);

		RT_TRACE(COMP_TXAGC,  
		"CCX Cell Limit: %d dbm => CCK Tx power index : %d, Legacy OFDM Tx power index : %d, OFDM Tx power index: %d\n", 
		priv->CcxCellPwr, CckCellPwrIdx, LegacyOfdmCellPwrIdx, OfdmCellPwrIdx);
		RT_TRACE(COMP_TXAGC, 
		"EEPROM channel(%d) => CCK Tx power index: %d, Legacy OFDM Tx power index : %d, OFDM Tx power index: %d\n",
		channel, cckPowerLevel[0], ofdmPowerLevel[0] + priv->LegacyHTTxPowerDiff, ofdmPowerLevel[0]); 

		if(cckPowerLevel[0] > CckCellPwrIdx)
			cckPowerLevel[0] = CckCellPwrIdx;
		if(ofdmPowerLevel[0] + priv->LegacyHTTxPowerDiff > LegacyOfdmCellPwrIdx)
		{
			if((OfdmCellPwrIdx - priv->LegacyHTTxPowerDiff) > 0)
			{
				ofdmPowerLevel[0] = OfdmCellPwrIdx - priv->LegacyHTTxPowerDiff;
			}
			else
			{
				ofdmPowerLevel[0] = 0;
			}
		}

		RT_TRACE(COMP_TXAGC,  
		"Altered CCK Tx power index : %d, Legacy OFDM Tx power index: %d, OFDM Tx power index: %d\n", 
		cckPowerLevel[0], ofdmPowerLevel[0] + priv->LegacyHTTxPowerDiff, ofdmPowerLevel[0]);
	}

#endif
	priv->CurrentCckTxPwrIdx = cckPowerLevel[0];
	priv->CurrentOfdm24GTxPwrIdx = ofdmPowerLevel[0];

	RT_TRACE(COMP_TXAGC,  
		"PHY_SetTxPowerLevel8192S(): CCK Tx power index : %d, Legacy OFDM Tx power index: %d, OFDM Tx power index: %d\n", 
		cckPowerLevel[0], ofdmPowerLevel[0] + priv->LegacyHTTxPowerDiff, ofdmPowerLevel[0]);

}
/*-----------------------------------------------------------------------------
 * Function:    SetTxPowerLevel8190()
 *
 * Overview:    This function is export to "HalCommon" moudule
 *			We must consider RF path later!!!!!!!
 *
 * Input:       PADAPTER		Adapter
 *			u1Byte		channel
 *
 * Output:      NONE
 *
 * Return:      NONE
 *	2008/11/04	MHC		We remove EEPROM_93C56.
 *						We need to move CCX relative code to independet file.
 *	2009/01/21	MHC		Support new EEPROM format from SD3 requirement.
 *
 *---------------------------------------------------------------------------*/
void rtl8192_phy_setTxPower(struct net_device* dev, u8	channel)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8	cckPowerLevel[2], ofdmPowerLevel[2];	

	if(priv->bTXPowerDataReadFromEEPORM == false)
		return;
	getTxPowerIndex(dev, channel, &cckPowerLevel[0], &ofdmPowerLevel[0]);
	RT_TRACE(COMP_POWER, "Channel-%d, cckPowerLevel (A / B) = 0x%x / 0x%x,   ofdmPowerLevel (A / B) = 0x%x / 0x%x\n", 
		channel, cckPowerLevel[0], cckPowerLevel[1], ofdmPowerLevel[0], ofdmPowerLevel[1]);

	ccxPowerIndexCheck(dev, channel, &cckPowerLevel[0], &ofdmPowerLevel[0]);

	switch(priv->rf_chip)
	{
		case RF_8225:
		break;

		case RF_8256:
			;
			break;

		case RF_6052:
			PHY_RF6052SetCckTxPower(dev, cckPowerLevel[0]);
			PHY_RF6052SetOFDMTxPower(dev, &ofdmPowerLevel[0], channel);
			break;

		case RF_8258:
			break;
		default:
            		break;
	}
}
#else
void rtl8192_phy_setTxPower(struct net_device* dev, u8	channel)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8	powerlevel = (u8)EEPROM_Default_TxPower, powerlevelOFDM24G = 0x10;
	s8 ant_pwr_diff = 0;
	u32	u4RegValue;
	u8	index = (channel -1);
	u8	pwrdiff[2] = {0};
	u8	ht20pwr[2] = {0}, ht40pwr[2] = {0};
	u8	rfpath = 0, rfpathnum = 2;

	if(priv->bTXPowerDataReadFromEEPORM == false)
		return;


	powerlevel = priv->RfTxPwrLevelCck[0][index];
		
	if (priv->rf_type == RF_1T2R || priv->rf_type == RF_1T1R)
	{
		powerlevelOFDM24G = priv->RfTxPwrLevelOfdm1T[0][index];
		
		
		rfpathnum = 1;
		ht20pwr[0] = ht40pwr[0] = priv->RfTxPwrLevelOfdm1T[0][index];
	}
	else if (priv->rf_type == RF_2T2R)
	{
		powerlevelOFDM24G = priv->RfTxPwrLevelOfdm2T[0][index];
		ant_pwr_diff = 	priv->RfTxPwrLevelOfdm2T[1][index] - 
						priv->RfTxPwrLevelOfdm2T[0][index];
	
		RT_TRACE(COMP_POWER, "CH-%d HT40 A/B Pwr index = %x/%x(%d/%d)\n", 
			channel, priv->RfTxPwrLevelOfdm2T[0][index], 
			priv->RfTxPwrLevelOfdm2T[1][index],
			priv->RfTxPwrLevelOfdm2T[0][index],
			priv->RfTxPwrLevelOfdm2T[1][index]);	

		ht20pwr[0] = ht40pwr[0] = priv->RfTxPwrLevelOfdm2T[0][index];
		ht20pwr[1] = ht40pwr[1] = priv->RfTxPwrLevelOfdm2T[1][index];
	}
	RT_TRACE(COMP_POWER, "Channel-%d, set tx power index\n", channel);
				
	if (priv->eeprom_version >= 2)	
	{		
		if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20)
		{
			for (rfpath = 0; rfpath < rfpathnum; rfpath++)
			{
				pwrdiff[rfpath] = priv->TxPwrHt20Diff[rfpath][index];

				if (pwrdiff[rfpath] < 8)	
				{
					ht20pwr[rfpath] += pwrdiff[rfpath];
				}
				else				
				{
					ht20pwr[rfpath] -= (16-pwrdiff[rfpath]);
				}
			}

			if (priv->rf_type == RF_2T2R)
				ant_pwr_diff = ht20pwr[1] - ht20pwr[0];

			RT_TRACE(COMP_POWER, "HT20 to HT40 pwrdiff[A/B]=%d/%d, ant_pwr_diff=%d(B-A=%d-%d)\n", 
				pwrdiff[0], pwrdiff[1], ant_pwr_diff, ht20pwr[1], ht20pwr[0]);	
		}
	}
				
	if(ant_pwr_diff > 7)
		ant_pwr_diff = 7;
	if(ant_pwr_diff < -8)
		ant_pwr_diff = -8;
				
	RT_TRACE(COMP_POWER, "CCK/HT Power index = %x/%x(%d/%d), ant_pwr_diff=%d\n", 
		powerlevel, powerlevelOFDM24G, powerlevel, powerlevelOFDM24G, ant_pwr_diff);
		
	ant_pwr_diff &= 0xf;	

	priv->AntennaTxPwDiff[2] = 0;
	priv->AntennaTxPwDiff[1] = 0;
	priv->AntennaTxPwDiff[0] = (u8)(ant_pwr_diff);		
	RT_TRACE(COMP_POWER, "pHalData->AntennaTxPwDiff[0]/[1]/[2] = 0x%x/0x%x/0x%x\n", 
		priv->AntennaTxPwDiff[0], priv->AntennaTxPwDiff[1], priv->AntennaTxPwDiff[2]);
	u4RegValue = (	priv->AntennaTxPwDiff[2]<<8 | 
					priv->AntennaTxPwDiff[1]<<4 | 
					priv->AntennaTxPwDiff[0]	);
	RT_TRACE(COMP_POWER, "BCD-Diff=0x%x\n", u4RegValue);

	rtl8192_setBBreg(dev, rFPGA0_TxGainStage, (bXBTxAGC|bXCTxAGC|bXDTxAGC), u4RegValue);
		
#ifdef TODO 
	if(	priv->rtllib->iw_mode != IW_MODE_INFRA && priv->bWithCcxCellPwr &&
		channel == priv->rtllib->current_network.channel)
	{
		u8	CckCellPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_B, priv->CcxCellPwr);
		u8	LegacyOfdmCellPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_G, priv->CcxCellPwr);
		u8	OfdmCellPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_N_24G, priv->CcxCellPwr);

		RT_TRACE(COMP_TXAGC,  
		("CCX Cell Limit: %d dbm => CCK Tx power index : %d, Legacy OFDM Tx power index : %d, OFDM Tx power index: %d\n", 
		priv->CcxCellPwr, CckCellPwrIdx, LegacyOfdmCellPwrIdx, OfdmCellPwrIdx));
		RT_TRACE(COMP_TXAGC,  
		("EEPROM channel(%d) => CCK Tx power index: %d, Legacy OFDM Tx power index : %d, OFDM Tx power index: %d\n",
		channel, powerlevel, powerlevelOFDM24G + priv->LegacyHTTxPowerDiff, powerlevelOFDM24G)); 

		if(powerlevel > CckCellPwrIdx)
			powerlevel = CckCellPwrIdx;
		if(powerlevelOFDM24G + priv->LegacyHTTxPowerDiff > LegacyOfdmCellPwrIdx)
		{
			if((OfdmCellPwrIdx - priv->LegacyHTTxPowerDiff) > 0)
			{
				powerlevelOFDM24G = OfdmCellPwrIdx - priv->LegacyHTTxPowerDiff;
			}
			else
			{
				powerlevelOFDM24G = 0;
			}
		}

		RT_TRACE(COMP_TXAGC, 
		("Altered CCK Tx power index : %d, Legacy OFDM Tx power index: %d, OFDM Tx power index: %d\n", 
		powerlevel, powerlevelOFDM24G + priv->LegacyHTTxPowerDiff, powerlevelOFDM24G));
	}
#endif
	priv->CurrentCckTxPwrIdx = powerlevel;
	priv->CurrentOfdm24GTxPwrIdx = powerlevelOFDM24G;

	RT_TRACE(COMP_POWER, "PHY_SetTxPowerLevel8192S(): CCK Tx power index : %d, Legacy OFDM Tx power index: %d, OFDM Tx power index: %d\n", 
		powerlevel, powerlevelOFDM24G + priv->LegacyHTTxPowerDiff, powerlevelOFDM24G);
	
	switch(priv->rf_chip)
	{
		case RF_8225:
			break;

		case RF_8256:
			break;

		case RF_6052:
			PHY_RF6052SetCckTxPower(dev, powerlevel);
			PHY_RF6052SetOFDMTxPower(dev, powerlevelOFDM24G, channel);
			break;

		case RF_8258:
			break;
		default:
			break;
	}	

}
#endif

bool PHY_UpdateTxPowerDbm8192S(struct net_device* dev, long powerInDbm)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u8				idx;
	u8				rf_path;

	u8	CckTxPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_B, powerInDbm);
	u8	OfdmTxPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_N_24G, powerInDbm);

	if(OfdmTxPwrIdx - priv->LegacyHTTxPowerDiff > 0)
		OfdmTxPwrIdx -= priv->LegacyHTTxPowerDiff;
	else
		OfdmTxPwrIdx = 0;

	RT_TRACE(COMP_POWER, "PHY_UpdateTxPowerDbm8192S(): %ld dBm , CckTxPwrIdx = %d, OfdmTxPwrIdx = %d\n", powerInDbm, CckTxPwrIdx, OfdmTxPwrIdx);

	for(idx = 0; idx < 14; idx++)
	{
		priv->TxPowerLevelCCK[idx] = CckTxPwrIdx;
		priv->TxPowerLevelCCK_A[idx] = CckTxPwrIdx;
		priv->TxPowerLevelCCK_C[idx] = CckTxPwrIdx;
		priv->TxPowerLevelOFDM24G[idx] = OfdmTxPwrIdx;
		priv->TxPowerLevelOFDM24G_A[idx] = OfdmTxPwrIdx;
		priv->TxPowerLevelOFDM24G_C[idx] = OfdmTxPwrIdx;
		for (rf_path = 0; rf_path < 2; rf_path++)
		{
			priv->RfTxPwrLevelCck[rf_path][idx] = CckTxPwrIdx;
			priv->RfTxPwrLevelOfdm1T[rf_path][idx] = 
			priv->RfTxPwrLevelOfdm2T[rf_path][idx] = OfdmTxPwrIdx;
		}
	}

	rtl8192_phy_setTxPower(dev, priv->chan);

	return true;	
}

extern void PHY_SetBeaconHwReg(	struct net_device* dev, u16 BeaconInterval)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u32 NewBeaconNum;	

	if(priv->pFirmware->FirmwareVersion >= 0x33) 
	{
		write_nic_dword(dev,WFM5,0xF1000000|(BeaconInterval<<8));
	}
	else
	{
	NewBeaconNum = BeaconInterval *32 - 64;
	write_nic_dword(dev, WFM3+4, NewBeaconNum);
	write_nic_dword(dev, WFM3, 0xB026007C);
}
}

static u8 phy_DbmToTxPwrIdx(
	struct net_device* dev, 
	WIRELESS_MODE	WirelessMode,
	long			PowerInDbm
	)
{
	u8				TxPwrIdx = 0;
	long				Offset = 0;
	

	switch(WirelessMode)
	{
	case WIRELESS_MODE_B:
		Offset = -7;
		break;

	case WIRELESS_MODE_G:
		Offset = -8;
		break;

	case WIRELESS_MODE_N_24G:
	default:
		Offset = -8;
		break;
	}

	if((PowerInDbm - Offset) > 0)
	{
		TxPwrIdx = (u8)((PowerInDbm - Offset) * 2);
	}
	else
	{
		TxPwrIdx = 0;
	}

	if(TxPwrIdx > MAX_TXPWR_IDX_NMODE_92S)
		TxPwrIdx = MAX_TXPWR_IDX_NMODE_92S;

	return TxPwrIdx;
}

static long phy_TxPwrIdxToDbm(
	struct net_device* dev,
	WIRELESS_MODE	WirelessMode,
	u8			TxPwrIdx
	)
{
	long				Offset = 0;
	long				PwrOutDbm = 0;
	
	switch(WirelessMode)
	{
	case WIRELESS_MODE_B:
		Offset = -7;
		break;

	case WIRELESS_MODE_G:
	case WIRELESS_MODE_N_24G:
		Offset = -8;
		break;
	default:
		Offset = -8;
		break;
	}

	PwrOutDbm = TxPwrIdx / 2 + Offset; 

	return PwrOutDbm;
}

extern	void 
PHY_ScanOperationBackup8192S(
	struct net_device* dev,
	u8 Operation
	)
{
#if 1

	struct r8192_priv *priv = rtllib_priv(dev);

	if(priv->up)
	{
		switch(Operation)
		{
			case SCAN_OPT_BACKUP:
				priv->rtllib->SetFwCmdHandler(dev, FW_CMD_PAUSE_DM_BY_SCAN);
				break;

			case SCAN_OPT_RESTORE:
				priv->rtllib->SetFwCmdHandler(dev, FW_CMD_RESUME_DM_BY_SCAN);
				break;

			default:
				RT_TRACE(COMP_SCAN, "Unknown Scan Backup Operation. \n");
				break;
		}
	}
#endif
}

void PHY_SetBWModeCallback8192S(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8	 			regBwOpMode, regRRSR_RSC;



	RT_TRACE(COMP_SWBW, "==>SetBWModeCallback8192s()  Switch to %s bandwidth\n", \
					priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20?"20MHz":"40MHz");

	if(priv->rf_chip == RF_PSEUDO_11N)
	{
		priv->SetBWModeInProgress= false;
		return;
	}
	if(IS_NIC_DOWN(priv)){
		priv->SwChnlInProgress = priv->SetBWModeInProgress = false;
		return;
	}
		
	regBwOpMode = read_nic_byte(dev, BW_OPMODE);
	regRRSR_RSC = read_nic_byte(dev, RRSR+2);
	
	switch(priv->CurrentChannelBW)
	{
		case HT_CHANNEL_WIDTH_20:
			regBwOpMode |= BW_OPMODE_20MHZ;
			write_nic_byte(dev, BW_OPMODE, regBwOpMode);
			break;
			   
		case HT_CHANNEL_WIDTH_20_40:
			regBwOpMode &= ~BW_OPMODE_20MHZ;
			write_nic_byte(dev, BW_OPMODE, regBwOpMode);

			break;

		default:
			RT_TRACE(COMP_DBG, "SetBWModeCallback8192s():\
						unknown Bandwidth: %#X\n",priv->CurrentChannelBW);
			break;
	}
	
	switch(priv->CurrentChannelBW)
	{
		case HT_CHANNEL_WIDTH_20:
			rtl8192_setBBreg(dev, rFPGA0_RFMOD, bRFMOD, 0x0);
			rtl8192_setBBreg(dev, rFPGA1_RFMOD, bRFMOD, 0x0);

			if(priv->card_8192_version >= VERSION_8192S_BCUT)
				write_nic_byte(dev, rFPGA0_AnalogParameter2, 0x58);
			break;

		case HT_CHANNEL_WIDTH_20_40:
			rtl8192_setBBreg(dev, rFPGA0_RFMOD, bRFMOD, 0x1);
			rtl8192_setBBreg(dev, rFPGA1_RFMOD, bRFMOD, 0x1);
			

			rtl8192_setBBreg(dev, rCCK0_System, bCCKSideBand, (priv->nCur40MhzPrimeSC>>1));
			rtl8192_setBBreg(dev, rOFDM1_LSTF, 0xC00, priv->nCur40MhzPrimeSC);			
			
			if(priv->card_8192_version >= VERSION_8192S_BCUT)
				write_nic_byte(dev, rFPGA0_AnalogParameter2, 0x18);
			break;
			
		default:
			RT_TRACE(COMP_DBG, "SetBWModeCallback8192s(): unknown Bandwidth: %#X\n"\
						,priv->CurrentChannelBW);
			break;
			
	}


	switch( priv->rf_chip )
	{
		case RF_8225:		
			break;	
			
		case RF_8256:
			break;
			
		case RF_8258:
			break;

		case RF_PSEUDO_11N:
			break;

		case RF_6052:
			PHY_RF6052SetBandwidth(dev, priv->CurrentChannelBW);
			break;
		default:
			printk("Unknown rf_chip: %d\n", priv->rf_chip);
			break;
	}

	priv->SetBWModeInProgress= false;

	RT_TRACE(COMP_SWBW, "<==SetBWModeCallback8192s() \n" );
}


void rtl8192_SetBWMode(struct net_device *dev, HT_CHANNEL_WIDTH	Bandwidth, HT_EXTCHNL_OFFSET Offset)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	

	


	if(priv->SetBWModeInProgress)
		return;

	priv->SetBWModeInProgress= true;
	
	priv->CurrentChannelBW = Bandwidth;
	
	if(Offset==HT_EXTCHNL_OFFSET_LOWER)
		priv->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_UPPER;
	else if(Offset==HT_EXTCHNL_OFFSET_UPPER)
		priv->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_LOWER;
	else
		priv->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_DONT_CARE;

#if 0
	if(!priv->bDriverStopped)
	{
#ifdef USE_WORKITEM	
		PlatformScheduleWorkItem(&(priv->SetBWModeWorkItem));
#else
		PlatformSetTimer(dev, &(priv->SetBWModeTimer), 0);
#endif		
	}
#endif
	if(!IS_NIC_DOWN(priv)){
		PHY_SetBWModeCallback8192S(dev);
	} else {
		priv->SetBWModeInProgress= false;
	}
}

void PHY_SwChnlCallback8192S(struct net_device *dev)
{

	struct r8192_priv *priv = rtllib_priv(dev);
	u32		delay;
	
	RT_TRACE(COMP_CH, "==>SwChnlCallback8190Pci(), switch to channel %d\n", priv->chan);
	
	if(IS_NIC_DOWN(priv))
	{	
		printk("%s: driver is not up\n", __FUNCTION__);
		priv->SwChnlInProgress = priv->SetBWModeInProgress = false;
		return;
	}	
	if(priv->rf_chip == RF_PSEUDO_11N)
	{
		printk("%s: rt chip is RF_PSEUDO_11N\n", __FUNCTION__);
		priv->SwChnlInProgress=false;
		return; 								
	}
	
	do{
		if(!priv->SwChnlInProgress)
			break;

		if(!phy_SwChnlStepByStep(dev, priv->chan, &priv->SwChnlStage, &priv->SwChnlStep, &delay))
		{
			if(delay>0)
			{
				mdelay(delay);
			}
			else
			continue;
		}
		else
		{
			priv->SwChnlInProgress=false;
			break;
		}
	}while(true);
}

u8 rtl8192_phy_SwChnl(struct net_device* dev, u8 channel)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	if(IS_NIC_DOWN(priv))
	{
		printk("%s: driver is not up.\n",__FUNCTION__);
		priv->SwChnlInProgress = priv->SetBWModeInProgress = false;
		return false;
	}	
	if(priv->SwChnlInProgress){
		printk("%s: SwChnl is in progress\n",__FUNCTION__);
		return false;
	}

	if(priv->SetBWModeInProgress){
		printk("%s: Set BWMode is in progress\n",__FUNCTION__);
		return false;
	}
	if (0)
	{
		u8 path;
		for(path=0; path<2; path++){
		printk("============>to set channel:%x\n", rtl8192_phy_QueryRFReg(dev, path, 0x18, 0x3ff));
		udelay(10);
		}
	}
	switch(priv->rtllib->mode)
	{
	case WIRELESS_MODE_A:
	case WIRELESS_MODE_N_5G:
		if (channel<=14){
			RT_TRACE(COMP_ERR, "WIRELESS_MODE_A but channel<=14");
			return false;
		}
		break;
		
	case WIRELESS_MODE_B:
		if (channel>14){
			RT_TRACE(COMP_ERR, "WIRELESS_MODE_B but channel>14");
			return false;
		}
		break;
		
	case WIRELESS_MODE_G:
	case WIRELESS_MODE_N_24G:
		if (channel>14){
			RT_TRACE(COMP_ERR, "WIRELESS_MODE_G but channel>14");
			return false;
		}
		break;

	default:
		break;
	}
	
	priv->SwChnlInProgress = true;
	if( channel == 0)
		channel = 1;
	
	priv->chan=channel;

	priv->SwChnlStage=0;
	priv->SwChnlStep=0;

#if 0
	if(!dev->bDriverStopped)
	{
#ifdef USE_WORKITEM	
		PlatformScheduleWorkItem(&(priv->SwChnlWorkItem));
#else
		PlatformSetTimer(dev, &(priv->SwChnlTimer), 0);
#endif
	}
#endif

	if(!IS_NIC_DOWN(priv)){
		PHY_SwChnlCallback8192S(dev);
	} else {
		priv->SwChnlInProgress = false;
	}
	return true;	
}


void PHY_SwChnlPhy8192S(	
	struct net_device* dev,
	u8		channel
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	RT_TRACE(COMP_SCAN, "==>PHY_SwChnlPhy8192S(), switch to channel %d.\n", priv->chan);

#ifdef TO_DO_LIST
	if(RT_CANNOT_IO(dev))
		return;
#endif

	if(priv->SwChnlInProgress)
		return;
	
	if(priv->rf_chip == RF_PSEUDO_11N)
	{
		priv->SwChnlInProgress=false;
		return;
	}
	
	priv->SwChnlInProgress = true;
	if( channel == 0)
		channel = 1;
	
	priv->chan=channel;
	
	priv->SwChnlStage = 0;
	priv->SwChnlStep = 0;
	
	phy_FinishSwChnlNow(dev,channel);
	
	priv->SwChnlInProgress = false;
}

static bool
phy_SwChnlStepByStep(
	struct net_device* dev,
	u8		channel,
	u8		*stage,
	u8		*step,
	u32		*delay
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	SwChnlCmd				PreCommonCmd[MAX_PRECMD_CNT];
	u32					PreCommonCmdCnt;
	SwChnlCmd				PostCommonCmd[MAX_POSTCMD_CNT];
	u32					PostCommonCmdCnt;
	SwChnlCmd				RfDependCmd[MAX_RFDEPENDCMD_CNT];
	u32					RfDependCmdCnt;
	SwChnlCmd				*CurrentCmd = NULL;	
	u8					eRFPath;	
	u16					u2Channel = 0;
	
	RT_TRACE(COMP_CH, "===========>%s(), channel:%d, stage:%d, step:%d\n", __FUNCTION__, channel, *stage, *step);		
	if (!IsLegalChannel(priv->rtllib, channel))
	{
		RT_TRACE(COMP_ERR, "=============>set to illegal channel:%d\n", channel);
		return true; 
	}		

	
	PreCommonCmdCnt = 0;
	phy_SetSwChnlCmdArray(PreCommonCmd, PreCommonCmdCnt++, MAX_PRECMD_CNT, 
				CmdID_SetTxPowerLevel, 0, 0, 0);
	phy_SetSwChnlCmdArray(PreCommonCmd, PreCommonCmdCnt++, MAX_PRECMD_CNT, 
				CmdID_End, 0, 0, 0);
	
	PostCommonCmdCnt = 0;

	phy_SetSwChnlCmdArray(PostCommonCmd, PostCommonCmdCnt++, MAX_POSTCMD_CNT, 
				CmdID_End, 0, 0, 0);
	
	RfDependCmdCnt = 0;
	switch( priv->rf_chip )
	{
		case RF_8225:		
		if (channel < 1 || channel > 14)
			RT_TRACE(COMP_ERR, "illegal channel for zebra:%d\n", channel);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
			CmdID_RF_WriteReg, rRfChannel, channel, 10);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
		CmdID_End, 0, 0, 0);
		break;	
		
	case RF_8256:
		if (channel < 1 || channel > 14)
			RT_TRACE(COMP_ERR, "illegal channel for zebra:%d\n", channel);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
			CmdID_RF_WriteReg, rRfChannel, channel, 10);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
		CmdID_End, 0, 0, 0);
		break;
		
	case RF_6052:
		if (channel < 1 || channel > 14)
			RT_TRACE(COMP_ERR, "illegal channel for zebra:%d\n", channel);
		
		u2Channel = channel;
		
		switch(priv->CurrentChannelBW)
		{
		case HT_CHANNEL_WIDTH_20:
			u2Channel |= BIT10;
			break;
			
		case HT_CHANNEL_WIDTH_20_40:
			u2Channel &= ~BIT10;
			break;
		default:
			u2Channel |= BIT10; 
			break;
		}	
		u2Channel |= BIT12|BIT13|BIT14;
		
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
			CmdID_RF_WriteReg, RF_CHNLBW, u2Channel, 10);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
			CmdID_End, 0, 0, 0);		
		break;

	case RF_8258:
		break;
		
	default:
		return false;
		break;
	}

	
	do{
		switch(*stage)
		{
		case 0:
			CurrentCmd=&PreCommonCmd[*step];
			break;
		case 1:
			CurrentCmd=&RfDependCmd[*step];
			break;
		case 2:
			CurrentCmd=&PostCommonCmd[*step];
			break;
		}
		
		if(CurrentCmd->CmdID==CmdID_End)
		{
			if((*stage)==2)
			{
				return true;
			}
			else
			{
				(*stage)++;
				(*step)=0;
				continue;
			}
		}
		
		switch(CurrentCmd->CmdID)
		{
		case CmdID_SetTxPowerLevel:
#ifndef CONFIG_MP
				rtl8192_phy_setTxPower(dev,channel);
#endif
			break;
		case CmdID_WritePortUlong:
			write_nic_dword(dev, CurrentCmd->Para1, CurrentCmd->Para2);
			break;
		case CmdID_WritePortUshort:
			write_nic_word(dev, CurrentCmd->Para1, (u16)CurrentCmd->Para2);
			break;
		case CmdID_WritePortUchar:
			write_nic_byte(dev, CurrentCmd->Para1, (u8)CurrentCmd->Para2);
			break;
		case CmdID_RF_WriteReg:	
			for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
			{
				if (IS_HARDWARE_TYPE_8192SE(dev)) {
#ifdef CONFIG_FW_SETCHAN
					u32 rf_bw = ((priv->RfRegChnlVal[eRFPath] & 0xfffffc00) | (CurrentCmd->Para2 & 0xFF00));
#endif
					priv->RfRegChnlVal[eRFPath] = ((priv->RfRegChnlVal[eRFPath] & 0xfffffc00) | CurrentCmd->Para2);

#ifdef CONFIG_FW_SETCHAN
					rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, CurrentCmd->Para1, bRFRegOffsetMask, rf_bw);				
#else
					rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, CurrentCmd->Para1, bRFRegOffsetMask, priv->RfRegChnlVal[eRFPath]);				
#endif
				} else {
					rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, CurrentCmd->Para1, bRFRegOffsetMask, (CurrentCmd->Para2));
				}
			}
			break;
                default:
                        break;
		}
		
		break;
	}while(true);

	(*delay)=CurrentCmd->msDelay;
	(*step)++;
	RT_TRACE(COMP_CH, "<===========%s(), channel:%d, stage:%d, step:%d\n", __FUNCTION__, channel, *stage, *step);		
	return false;
}

static bool
phy_SetSwChnlCmdArray(
	SwChnlCmd*		CmdTable,
	u32			CmdTableIdx,
	u32			CmdTableSz,
	SwChnlCmdID		CmdID,
	u32			Para1,
	u32			Para2,
	u32			msDelay
	)
{
	SwChnlCmd* pCmd;

	if(CmdTable == NULL)
	{
		return false;
	}
	if(CmdTableIdx >= CmdTableSz)
	{
		return false;
	}

	pCmd = CmdTable + CmdTableIdx;
	pCmd->CmdID = CmdID;
	pCmd->Para1 = Para1;
	pCmd->Para2 = Para2;
	pCmd->msDelay = msDelay;

	return true;
}

static void
phy_FinishSwChnlNow(	
	struct net_device* dev,
	u8		channel
		)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u32			delay;
  
	while(!phy_SwChnlStepByStep(dev,channel,&priv->SwChnlStage,&priv->SwChnlStep,&delay))
	{
		if(delay>0)
			mdelay(delay);
		if(IS_NIC_DOWN(priv))
			break;
	}
}


u8 rtl8192_phy_CheckIsLegalRFPath(struct net_device* dev, u32 eRFPath)
{
	bool				rtValue = true;

#if 0	
	if (priv->rf_type == RF_1T2R && eRFPath != RF90_PATH_A)
	{		
		rtValue = false;
	}
	if (priv->rf_type == RF_1T2R && eRFPath != RF90_PATH_A)
	{

	}
#endif
	return	rtValue;

}	/* PHY_CheckIsLegalRfPath8192S */



void	
PHY_IQCalibrate(	struct net_device* dev)
{
	u32				i, reg;
	u32				old_value;
	long				X, Y, TX0[4];
	u32				TXA[4];
	

	for (i = 0; i < 10; i++)
	{
		rtl8192_setBBreg(dev, 0xc04, bMaskDWord, 0x00a05430);
		udelay(5);
		rtl8192_setBBreg(dev, 0xc08, bMaskDWord, 0x000800e4);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe28, bMaskDWord, 0x80800000);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe40, bMaskDWord, 0x02140148);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe44, bMaskDWord, 0x681604a2);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe4c, bMaskDWord, 0x000028d1);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe60, bMaskDWord, 0x0214014d);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe64, bMaskDWord, 0x281608ba);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe6c, bMaskDWord, 0x000028d1);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xfb000001);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xf8000001);
		udelay(2000);
		rtl8192_setBBreg(dev, 0xc04, bMaskDWord, 0x00a05433);
		udelay(5);
		rtl8192_setBBreg(dev, 0xc08, bMaskDWord, 0x000000e4);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe28, bMaskDWord, 0x0);		

	
		reg = rtl8192_QueryBBReg(dev, 0xeac, bMaskDWord);

		if (!(reg&(BIT27|BIT28|BIT30|BIT31)))
		{
			old_value = (rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord) & 0x3FF);

			X = (rtl8192_QueryBBReg(dev, 0xe94, bMaskDWord) & 0x03FF0000)>>16;
			TXA[RF90_PATH_A] = (X * old_value)/0x100;
			reg = rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord);
			reg = (reg & 0xFFFFFC00) | (u32)TXA[RF90_PATH_A];
			rtl8192_setBBreg(dev, 0xc80, bMaskDWord, reg);
			udelay(5);

			Y = ( rtl8192_QueryBBReg(dev, 0xe9C, bMaskDWord) & 0x03FF0000)>>16;
			TX0[RF90_PATH_C] = ((Y * old_value)/0x100);
			reg = rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord);
			reg = (reg & 0xffc0ffff) |((u32) (TX0[RF90_PATH_C]&0x3F)<<16);			
			rtl8192_setBBreg(dev, 0xc80, bMaskDWord, reg);
			reg = rtl8192_QueryBBReg(dev, 0xc94, bMaskDWord);		
			reg = (reg & 0x0fffffff) |(((Y&0x3c0)>>6)<<28);
			rtl8192_setBBreg(dev, 0xc94, bMaskDWord, reg);
			udelay(5);

			reg = rtl8192_QueryBBReg(dev, 0xc14, bMaskDWord);
			X = (rtl8192_QueryBBReg(dev, 0xea4, bMaskDWord) & 0x03FF0000)>>16;			
			reg = (reg & 0xFFFFFC00) |X;
			rtl8192_setBBreg(dev, 0xc14, bMaskDWord, reg);
			Y = (rtl8192_QueryBBReg(dev, 0xeac, bMaskDWord) & 0x003F0000)>>16;
			reg = (reg & 0xFFFF03FF) |Y<<10;
			rtl8192_setBBreg(dev, 0xc14, bMaskDWord, reg);
			udelay(5);
			old_value = (rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord) & 0x3FF);
			
			X = (rtl8192_QueryBBReg(dev, 0xeb4, bMaskDWord) & 0x03FF0000)>>16;
			reg = rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord);
			TXA[RF90_PATH_A] = (X * old_value) / 0x100;
			reg = (reg & 0xFFFFFC00) | TXA[RF90_PATH_A];
			rtl8192_setBBreg(dev, 0xc88, bMaskDWord, reg);
			udelay(5);

			Y = (rtl8192_QueryBBReg(dev, 0xebc, bMaskDWord)& 0x03FF0000)>>16;
			TX0[RF90_PATH_C] = ((Y * old_value)/0x100);
			reg = rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord);
			reg = (reg & 0xffc0ffff) |( (TX0[RF90_PATH_C]&0x3F)<<16);
			rtl8192_setBBreg(dev, 0xc88, bMaskDWord, reg);
			reg = rtl8192_QueryBBReg(dev, 0xc9c, bMaskDWord);
			reg = (reg & 0x0fffffff) |(((Y&0x3c0)>>6)<<28);
			rtl8192_setBBreg(dev, 0xc9c, bMaskDWord, reg);
			udelay(5);

			reg = rtl8192_QueryBBReg(dev, 0xc1c, bMaskDWord);
			X = (rtl8192_QueryBBReg(dev, 0xec4, bMaskDWord) & 0x03FF0000)>>16;			
			reg = (reg & 0xFFFFFC00) |X;
			rtl8192_setBBreg(dev, 0xc1c, bMaskDWord, reg);
			
			Y = (rtl8192_QueryBBReg(dev, 0xecc, bMaskDWord) & 0x003F0000)>>16;
			reg = (reg & 0xFFFF03FF) |Y<<10;
			rtl8192_setBBreg(dev, 0xc1c, bMaskDWord, reg);
			udelay(5);
			
			RT_TRACE(COMP_INIT, "PHY_IQCalibrate OK\n");
			break;
		}

	}
	
	


}

extern void PHY_IQCalibrateBcut(struct net_device* dev)
{
	u32				i, reg;
	u32				old_value;
	long				X, Y, TX0[4];
	u32				TXA[4];	
	u32				calibrate_set[13] = {0};
	u32				load_value[13];
	u8			RfPiEnable=0;
	
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
	for (i = 0; i < 13; i++)
	{
		load_value[i] = rtl8192_QueryBBReg(dev, calibrate_set[i], bMaskDWord);
		rtl8192_setBBreg(dev, calibrate_set[i], bMaskDWord, 0x3fed92fb);		
		
	}
	
	RfPiEnable = (u8)rtl8192_QueryBBReg(dev, rFPGA0_XA_HSSIParameter1, BIT8);
	for (i = 0; i < 10; i++)
	{
		RT_TRACE(COMP_INIT, "IQK -%d\n", i);	
		if (!RfPiEnable)	
		{
			rtl8192_setBBreg(dev, 0x820, bMaskDWord, 0x01000100);
			rtl8192_setBBreg(dev, 0x828, bMaskDWord, 0x01000100);
		}
	
		rtl8192_setBBreg(dev, 0xc04, bMaskDWord, 0x00a05430);
		udelay(5);
		rtl8192_setBBreg(dev, 0xc08, bMaskDWord, 0x000800e4);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe28, bMaskDWord, 0x80800000);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe40, bMaskDWord, 0x02140102);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe44, bMaskDWord, 0x681604c2);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe4c, bMaskDWord, 0x000028d1);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe60, bMaskDWord, 0x02140102);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe64, bMaskDWord, 0x28160d05);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xfb000000);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xf8000000);
		udelay(5);

		udelay(2000);

		rtl8192_setBBreg(dev, 0xe6c, bMaskDWord, 0x020028d1);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xfb000000);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xf8000000);

		udelay(2000);

		rtl8192_setBBreg(dev, 0xc04, bMaskDWord, 0x00a05433);
		udelay(5);
		rtl8192_setBBreg(dev, 0xc08, bMaskDWord, 0x000000e4);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe28, bMaskDWord, 0x0);		

		if (!RfPiEnable)	
		{
			rtl8192_setBBreg(dev, 0x820, bMaskDWord, 0x01000000);
			rtl8192_setBBreg(dev, 0x828, bMaskDWord, 0x01000000);
		}

	
		reg = rtl8192_QueryBBReg(dev, 0xeac, bMaskDWord);

		if (!(reg&(BIT27|BIT28|BIT30|BIT31)))
		{			
			old_value = (rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord) & 0x3FF);

			X = (rtl8192_QueryBBReg(dev, 0xe94, bMaskDWord) & 0x03FF0000)>>16;
			TXA[RF90_PATH_A] = (X * old_value)/0x100;
			reg = rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord);
			reg = (reg & 0xFFFFFC00) | (u32)TXA[RF90_PATH_A];
			rtl8192_setBBreg(dev, 0xc80, bMaskDWord, reg);
			udelay(5);

			Y = ( rtl8192_QueryBBReg(dev, 0xe9C, bMaskDWord) & 0x03FF0000)>>16;
			TX0[RF90_PATH_C] = ((Y * old_value)/0x100);
			reg = rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord);
			reg = (reg & 0xffc0ffff) |((u32) (TX0[RF90_PATH_C]&0x3F)<<16);			
			rtl8192_setBBreg(dev, 0xc80, bMaskDWord, reg);
			reg = rtl8192_QueryBBReg(dev, 0xc94, bMaskDWord);		
			reg = (reg & 0x0fffffff) |(((Y&0x3c0)>>6)<<28);
			rtl8192_setBBreg(dev, 0xc94, bMaskDWord, reg);
			udelay(5);

			reg = rtl8192_QueryBBReg(dev, 0xc14, bMaskDWord);
			X = (rtl8192_QueryBBReg(dev, 0xea4, bMaskDWord) & 0x03FF0000)>>16;			
			reg = (reg & 0xFFFFFC00) |X;
			rtl8192_setBBreg(dev, 0xc14, bMaskDWord, reg);
			Y = (rtl8192_QueryBBReg(dev, 0xeac, bMaskDWord) & 0x003F0000)>>16;
			reg = (reg & 0xFFFF03FF) |Y<<10;
			rtl8192_setBBreg(dev, 0xc14, bMaskDWord, reg);
			udelay(5);
			old_value = (rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord) & 0x3FF);
			
			X = (rtl8192_QueryBBReg(dev, 0xeb4, bMaskDWord) & 0x03FF0000)>>16;
			reg = rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord);
			TXA[RF90_PATH_A] = (X * old_value) / 0x100;
			reg = (reg & 0xFFFFFC00) | TXA[RF90_PATH_A];
			rtl8192_setBBreg(dev, 0xc88, bMaskDWord, reg);
			udelay(5);

			Y = (rtl8192_QueryBBReg(dev, 0xebc, bMaskDWord)& 0x03FF0000)>>16;
			TX0[RF90_PATH_C] = ((Y * old_value)/0x100);
			reg = rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord);
			reg = (reg & 0xffc0ffff) |( (TX0[RF90_PATH_C]&0x3F)<<16);
			rtl8192_setBBreg(dev, 0xc88, bMaskDWord, reg);
			reg = rtl8192_QueryBBReg(dev, 0xc9c, bMaskDWord);
			reg = (reg & 0x0fffffff) |(((Y&0x3c0)>>6)<<28);
			rtl8192_setBBreg(dev, 0xc9c, bMaskDWord, reg);
			udelay(5);

			reg = rtl8192_QueryBBReg(dev, 0xc1c, bMaskDWord);
			X = (rtl8192_QueryBBReg(dev, 0xec4, bMaskDWord) & 0x03FF0000)>>16;			
			reg = (reg & 0xFFFFFC00) |X;
			rtl8192_setBBreg(dev, 0xc1c, bMaskDWord, reg);
			
			Y = (rtl8192_QueryBBReg(dev, 0xecc, bMaskDWord) & 0x003F0000)>>16;
			reg = (reg & 0xFFFF03FF) |Y<<10;
			rtl8192_setBBreg(dev, 0xc1c, bMaskDWord, reg);
			udelay(5);
			
			RT_TRACE(COMP_INIT, "PHY_IQCalibrate OK\n");
			break;
		}

	}
	
	for (i = 0; i < 13; i++)
		rtl8192_setBBreg(dev, calibrate_set[i], bMaskDWord, load_value[i]);
	
	
	


}	

#define HalGetFirmwareVerison(priv) (priv->pFirmware->FirmwareVersion )
bool rtl8192se_set_fw_cmd(struct net_device* dev, FW_CMD_IO_TYPE		FwCmdIO)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u32	FwParam = FW_CMD_IO_PARA_QUERY(priv);
	u16	FwCmdMap = FW_CMD_IO_QUERY(priv);
	bool	bPostProcessing = false;

	RT_TRACE(COMP_CMD, "-->HalSetFwCmd8192S(): Set FW Cmd(%#x), SetFwCmdInProgress(%d)\n", FwCmdIO, priv->SetFwCmdInProgress);





	RT_TRACE(COMP_CMD, "-->HalSetFwCmd8192S(): Set FW Cmd(%#x), SetFwCmdInProgress(%d)\n", 
		FwCmdIO, priv->SetFwCmdInProgress);

	do{

		if(HalGetFirmwareVerison(priv) >= 0x35)
	{
			switch(FwCmdIO)
			{
				case FW_CMD_RA_REFRESH_N:
					FwCmdIO = FW_CMD_RA_REFRESH_N_COMB;
					break;
				case FW_CMD_RA_REFRESH_BG:
					FwCmdIO = FW_CMD_RA_REFRESH_BG_COMB;
					break;
				default:
					break;
			}
		}
		else
		{			
			if((FwCmdIO == FW_CMD_IQK_ENABLE) ||
				(FwCmdIO == FW_CMD_RA_REFRESH_N) ||
				(FwCmdIO == FW_CMD_RA_REFRESH_BG))
			{
				bPostProcessing = true;
				break;
			}
	}

		if(HalGetFirmwareVerison(priv) >= 0x3E)
	{
			if(FwCmdIO == FW_CMD_CTRL_DM_BY_DRIVER)
				FwCmdIO = FW_CMD_CTRL_DM_BY_DRIVER_NEW;
	}


		switch(FwCmdIO)
	{

			case FW_CMD_RA_INIT:
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] RA init!!\n");
				FwCmdMap |= FW_RA_INIT_CTL;
				FW_CMD_IO_SET(priv, FwCmdMap);	
				FW_CMD_IO_CLR(priv, FW_RA_INIT_CTL);	 
				break;

			case FW_CMD_DIG_DISABLE:
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set DIG disable!!\n");
				FwCmdMap &= ~FW_DIG_ENABLE_CTL;
				FW_CMD_IO_SET(priv, FwCmdMap);			
				break;
				
			case FW_CMD_DIG_ENABLE:
			case FW_CMD_DIG_RESUME:
				if(!(priv->DMFlag & HAL_DM_DIG_DISABLE))
				{
					RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set DIG enable or resume!!\n");
					FwCmdMap |= (FW_DIG_ENABLE_CTL|FW_SS_CTL);
					FW_CMD_IO_SET(priv, FwCmdMap);	
	}
				break;

			case FW_CMD_DIG_HALT:
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set DIG halt!!\n");
				FwCmdMap &= ~(FW_DIG_ENABLE_CTL|FW_SS_CTL);
				FW_CMD_IO_SET(priv, FwCmdMap);				
				break;
				
			case FW_CMD_TXPWR_TRACK_THERMAL:
	{
					u8	ThermalVal = 0;
					FwCmdMap |= FW_PWR_TRK_CTL;
					FwParam &= FW_PWR_TRK_PARAM_CLR; 
					ThermalVal = priv->ThermalValue;
					FwParam |= ((ThermalVal<<24) |(priv->ThermalMeter[0]<<16));					
					RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set TxPwr tracking!! FwCmdMap(%#x), FwParam(%#x)\n", FwCmdMap, FwParam);
					FW_CMD_PARA_SET(priv, FwParam);
					FW_CMD_IO_SET(priv, FwCmdMap);
					FW_CMD_IO_CLR(priv, FW_PWR_TRK_CTL);	 
	}
				break;
				
			case FW_CMD_RA_REFRESH_N_COMB:
				FwCmdMap |= FW_RA_N_CTL;
				FwCmdMap &= ~(FW_RA_BG_CTL |FW_RA_INIT_CTL);
				FwParam &= FW_RA_PARAM_CLR; 
				if(!(priv->rtllib->pHTInfo->IOTRaFunc & HT_IOT_RAFUNC_DISABLE_ALL))
					FwParam |= ((priv->rtllib->pHTInfo->IOTRaFunc)&0xf);
				FwParam |= ((priv->rtllib->pHTInfo->IOTPeer & 0xf) <<4);
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set RA/IOT Comb in n mode!! FwCmdMap(%#x), FwParam(%#x)\n", FwCmdMap, FwParam);
				FW_CMD_PARA_SET(priv, FwParam);
				FW_CMD_IO_SET(priv, FwCmdMap);	
				FW_CMD_IO_CLR(priv, FW_RA_N_CTL); 
				break;	

			case FW_CMD_RA_REFRESH_BG_COMB:				
				FwCmdMap |= FW_RA_BG_CTL;
				FwCmdMap &= ~(FW_RA_N_CTL|FW_RA_INIT_CTL); 
				FwParam &= FW_RA_PARAM_CLR; 
				if(!(priv->rtllib->pHTInfo->IOTRaFunc & HT_IOT_RAFUNC_DISABLE_ALL))
					FwParam |= ((priv->rtllib->pHTInfo->IOTRaFunc)&0xf);
				FwParam |= ((priv->rtllib->pHTInfo->IOTPeer & 0xf) <<4);
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set RA/IOT Comb in BG mode!! FwCmdMap(%#x), FwParam(%#x)\n", FwCmdMap, FwParam);
				FW_CMD_PARA_SET(priv, FwParam);
				FW_CMD_IO_SET(priv, FwCmdMap);	
				FW_CMD_IO_CLR(priv, FW_RA_BG_CTL); 
				break;

			case FW_CMD_IQK_ENABLE:
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] IQK enable.\n");
				FwCmdMap |= FW_IQK_CTL;
				FW_CMD_IO_SET(priv, FwCmdMap);	
				FW_CMD_IO_CLR(priv, FW_IQK_CTL); 
				break;

			case FW_CMD_CTRL_DM_BY_DRIVER_NEW:
				RT_TRACE(COMP_CMD, "[FW CMD][New Version] Inform FW driver control some DM!! FwCmdMap(%#x), FwParam(%#x)\n", FwCmdMap, FwParam);
				FwCmdMap |= FW_DRIVER_CTRL_DM_CTL;
				FW_CMD_IO_SET(priv, FwCmdMap);
				break;

			case FW_CMD_RESUME_DM_BY_SCAN:
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Resume DM after scan.\n");
				FwCmdMap |= (FW_DIG_ENABLE_CTL|FW_HIGH_PWR_ENABLE_CTL|FW_SS_CTL);
				
				if(priv->DMFlag & HAL_DM_DIG_DISABLE || !dm_digtable.dig_enable_flag)
					FwCmdMap &= ~FW_DIG_ENABLE_CTL;	

				if((priv->DMFlag & HAL_DM_HIPWR_DISABLE) ||
					(priv->rtllib->pHTInfo->IOTAction & HT_IOT_ACT_DISABLE_HIGH_POWER) ||
					(priv->rtllib->bdynamic_txpower_enable == true))
					FwCmdMap &= ~FW_HIGH_PWR_ENABLE_CTL;
				
				if(	(dm_digtable.Dig_Ext_Port_Stage == DIG_EXT_PORT_STAGE_0) ||
					(dm_digtable.Dig_Ext_Port_Stage == DIG_EXT_PORT_STAGE_1))
					FwCmdMap &= ~FW_DIG_ENABLE_CTL;
				
				FW_CMD_IO_SET(priv, FwCmdMap);		
				bPostProcessing = true;
				break;
		
			case FW_CMD_PAUSE_DM_BY_SCAN:
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Pause DM before scan.\n");
				FwCmdMap &= ~(FW_DIG_ENABLE_CTL|FW_HIGH_PWR_ENABLE_CTL|FW_SS_CTL);
				FW_CMD_IO_SET(priv, FwCmdMap);	
				bPostProcessing = true;
				break;

			case FW_CMD_HIGH_PWR_DISABLE:
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version]  Set HighPwr disable!!\n");
				FwCmdMap &= ~FW_HIGH_PWR_ENABLE_CTL;
				FW_CMD_IO_SET(priv, FwCmdMap);		
				bPostProcessing = true;
				break;			

			case FW_CMD_HIGH_PWR_ENABLE:	
				if(((priv->rtllib->pHTInfo->IOTAction & HT_IOT_ACT_DISABLE_HIGH_POWER)==0) &&
					!(priv->DMFlag & HAL_DM_HIPWR_DISABLE) &&
					(priv->rtllib->bdynamic_txpower_enable != true))
				{
					RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set HighPwr enable!!\n");
					FwCmdMap |= (FW_HIGH_PWR_ENABLE_CTL|FW_SS_CTL);
					FW_CMD_IO_SET(priv, FwCmdMap);		
					bPostProcessing = true;
				}
				break;
				
			case FW_CMD_DIG_MODE_FA:
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set DIG Mode to FA.\n");
				FwCmdMap |= FW_FA_CTL;
				FW_CMD_IO_SET(priv, FwCmdMap);
				break;

			case FW_CMD_DIG_MODE_SS:
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set DIG Mode to SS.\n");
				FwCmdMap &= ~FW_FA_CTL;
				FW_CMD_IO_SET(priv, FwCmdMap);
				break;
				
			case FW_CMD_PAPE_CONTROL:
				RT_TRACE(COMP_CMD, "[FW CMD] Set PAPE Control \n");
#ifdef MERGE_TO_DO
				if(pHalData->bt_coexist.BT_PapeCtrl)
				{
					RTPRINT(FBT, BT_TRACE, ("BT set PAPE Control to SW/HW dynamically. \n"));
					FwCmdMap |= FW_PAPE_CTL_BY_SW_HW;
				}
				else
#endif
				{
					printk("BT set PAPE Control to SW\n");
					FwCmdMap &= ~FW_PAPE_CTL_BY_SW_HW;
				}
				FW_CMD_IO_SET(priv, FwCmdMap);
				break;

			default:				
				bPostProcessing = true; 
				break;
		}
	}while(false);

	RT_TRACE(COMP_CMD, "[FW CMD] Current FwCmdMap(%#x)\n", priv->FwCmdIOMap);
	RT_TRACE(COMP_CMD, "[FW CMD] Current FwCmdIOParam(%#x)\n", priv->FwCmdIOParam);

	if(bPostProcessing && !priv->SetFwCmdInProgress)
	{
		priv->SetFwCmdInProgress = true;
		priv->CurrentFwCmdIO = FwCmdIO; 
	}
	else
	{
		return false;
	}

#if 0
#ifdef USE_WORKITEM			
	PlatformScheduleWorkItem(&(pHalData->FwCmdIOWorkItem));
#else
	PlatformSetTimer(Adapter, &(pHalData->SetFwCmdIOTimer), 0);
#endif
#endif
	rtl8192_SetFwCmdIOCallback(dev);
	return true;
}
void ChkFwCmdIoDone(struct net_device* dev)
{
	u16 PollingCnt = 10000;		
	u32 tmpValue;
	
	do
	{
		
		udelay(10); 
		
		tmpValue = read_nic_dword(dev, WFM5);
		if(tmpValue == 0)
		{
			RT_TRACE(COMP_CMD, "[FW CMD] Set FW Cmd success!!\n");
			break;
		}			
		else
		{
			RT_TRACE(COMP_CMD, "[FW CMD] Polling FW Cmd PollingCnt(%d)!!\n", PollingCnt);
		}
	}while( --PollingCnt );

	if(PollingCnt == 0)
	{
		RT_TRACE(COMP_ERR, "[FW CMD] Set FW Cmd fail!!\n");
	}
}
void rtl8192_SetFwCmdIOCallback(struct net_device* dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u32		input,CurrentAID = 0;
	
	if(IS_NIC_DOWN(priv)){
		RT_TRACE(COMP_CMD, "SetFwCmdIOTimerCallback(): driver is going to unload\n");
		return;
	}
	
	RT_TRACE(COMP_CMD, "--->SetFwCmdIOTimerCallback(): Cmd(%#x), SetFwCmdInProgress(%d)\n", priv->CurrentFwCmdIO, priv->SetFwCmdInProgress);

	if(HalGetFirmwareVerison(priv) >= 0x34)
                        {
		switch(priv->CurrentFwCmdIO)
		{
			case FW_CMD_RA_REFRESH_N:
				priv->CurrentFwCmdIO = FW_CMD_RA_REFRESH_N_COMB;
			break;
			case FW_CMD_RA_REFRESH_BG:
				priv->CurrentFwCmdIO = FW_CMD_RA_REFRESH_BG_COMB;
			break;
			default:
			break;
		}
	}
			
	switch(priv->CurrentFwCmdIO)
	{

		case FW_CMD_RA_RESET:
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA Reset!!\n");
			write_nic_dword(dev, WFM5, FW_RA_RESET);	
			ChkFwCmdIoDone(dev);	
			break;
			
		case FW_CMD_RA_ACTIVE:
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA Active!!\n");
			write_nic_dword(dev, WFM5, FW_RA_ACTIVE); 
			ChkFwCmdIoDone(dev);	
			break;

		case FW_CMD_RA_REFRESH_N:
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA n refresh!!\n");
			if(priv->rtllib->pHTInfo->IOTRaFunc & HT_IOT_RAFUNC_DISABLE_ALL)
				input = FW_RA_REFRESH;
			else
				input = FW_RA_REFRESH | (priv->rtllib->pHTInfo->IOTRaFunc << 8);
			write_nic_dword(dev, WFM5, input); 
			ChkFwCmdIoDone(dev);
			write_nic_dword(dev, WFM5, FW_RA_ENABLE_RSSI_MASK); 
			ChkFwCmdIoDone(dev);	
			break;
			
		case FW_CMD_RA_REFRESH_BG:
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA BG refresh!!\n");
			write_nic_dword(dev, WFM5, FW_RA_REFRESH); 
			ChkFwCmdIoDone(dev);
			write_nic_dword(dev, WFM5, FW_RA_DISABLE_RSSI_MASK); 			
			ChkFwCmdIoDone(dev);	
			break;

		case FW_CMD_RA_REFRESH_N_COMB:
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA n Combo refresh!!\n");
			if(priv->rtllib->pHTInfo->IOTRaFunc & HT_IOT_RAFUNC_DISABLE_ALL)
				input = FW_RA_IOT_N_COMB;
			else
				input = FW_RA_IOT_N_COMB | (((priv->rtllib->pHTInfo->IOTRaFunc)&0x0f) << 8);
			input = input |((priv->rtllib->pHTInfo->IOTPeer & 0xf) <<12);
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA/IOT Comb in n mode!! input(%#x)\n", input);
			write_nic_dword(dev, WFM5, input); 			
			ChkFwCmdIoDone(dev);	
			break;		

		case FW_CMD_RA_REFRESH_BG_COMB:		
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA B/G Combo refresh!!\n");
			if(priv->rtllib->pHTInfo->IOTRaFunc & HT_IOT_RAFUNC_DISABLE_ALL)
				input = FW_RA_IOT_BG_COMB;
			else
				input = FW_RA_IOT_BG_COMB | (((priv->rtllib->pHTInfo->IOTRaFunc)&0x0f) << 8);
			input = input |((priv->rtllib->pHTInfo->IOTPeer & 0xf) <<12);
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA/IOT Comb in B/G mode!! input(%#x)\n", input);
			write_nic_dword(dev, WFM5, input); 
			ChkFwCmdIoDone(dev);	
			break;

		case FW_CMD_IQK_ENABLE:
			RT_TRACE(COMP_CMD, "[FW CMD] IQK Enable!!\n");
			write_nic_dword(dev, WFM5, FW_IQK_ENABLE); 
			ChkFwCmdIoDone(dev);	
			break;

		case FW_CMD_PAUSE_DM_BY_SCAN:
			RT_TRACE(COMP_CMD, "[FW CMD] Pause DM by Scan!!\n");
			rtl8192_setBBreg(dev, rOFDM0_XAAGCCore1, bMaskByte0, 0x17);
			rtl8192_setBBreg(dev, rOFDM0_XBAGCCore1, bMaskByte0, 0x17);
			rtl8192_setBBreg(dev, rCCK0_CCA, bMaskByte2, 0x40);
			break;

		case FW_CMD_RESUME_DM_BY_SCAN:		
			RT_TRACE(COMP_CMD, "[FW CMD] Resume DM by Scan!!\n");
			rtl8192_setBBreg(dev, rCCK0_CCA, bMaskByte2, 0xcd);	
			rtl8192_phy_setTxPower(dev, priv->rtllib->current_network.channel);
			break;
		
		case FW_CMD_HIGH_PWR_DISABLE:
			RT_TRACE(COMP_CMD, "[FW CMD] High Pwr Disable!!\n");
			if(priv->DMFlag & HAL_DM_HIPWR_DISABLE)
				break;
			rtl8192_setBBreg(dev, rOFDM0_XAAGCCore1, bMaskByte0, 0x17);
			rtl8192_setBBreg(dev, rOFDM0_XBAGCCore1, bMaskByte0, 0x17);
			rtl8192_setBBreg(dev, rCCK0_CCA, bMaskByte2, 0x40);
			break;
			
		case FW_CMD_HIGH_PWR_ENABLE:
			RT_TRACE(COMP_CMD, "[FW CMD] High Pwr Enable!!\n");
			if((priv->DMFlag & HAL_DM_HIPWR_DISABLE) ||
				(priv->rtllib->bdynamic_txpower_enable == true))
				break;
			rtl8192_setBBreg(dev, rCCK0_CCA, bMaskByte2, 0xcd);				
			break;

		case FW_CMD_LPS_ENTER:
			RT_TRACE(COMP_CMD, "[FW CMD] Enter LPS mode!!\n");
			CurrentAID = priv->rtllib->assoc_id;
			write_nic_dword(dev, WFM5, (FW_LPS_ENTER| ((CurrentAID|0xc000)<<8))    );
			ChkFwCmdIoDone(dev);	
			priv->rtllib->pHTInfo->IOTAction |=  HT_IOT_ACT_DISABLE_EDCA_TURBO;
			break;

		case FW_CMD_LPS_LEAVE:
			RT_TRACE(COMP_CMD, "[FW CMD] Leave LPS mode!!\n");
			write_nic_dword(dev, WFM5, FW_LPS_LEAVE );
			ChkFwCmdIoDone(dev);	
			priv->rtllib->pHTInfo->IOTAction &=  (~HT_IOT_ACT_DISABLE_EDCA_TURBO);
			break;

		case FW_CMD_ADD_A2_ENTRY:
			RT_TRACE(COMP_CMD, "[FW CMD] ADD A2 entry!!\n");
			write_nic_dword(dev, WFM5, FW_ADD_A2_ENTRY);
			ChkFwCmdIoDone(dev);
			break;
		
		case FW_CMD_CTRL_DM_BY_DRIVER:
			RT_TRACE(COMP_CMD, "[FW CMD] Inform fw driver will do some dm at driver\n");
			write_nic_dword(dev, WFM5, FW_CTRL_DM_BY_DRIVER);
	                ChkFwCmdIoDone(dev);		
			break;
#ifdef CONFIG_FW_SETCHAN
		case FW_CMD_CHAN_SET:
			input = FW_CHAN_SET | (((priv->chan)&0xff) << 8);
			RT_TRACE(COMP_CMD, "[FW CMD] Inform fw to set channel to %x!!, input(%#x):\n", priv->chan,input);
			write_nic_dword(dev, WFM5, input);
	                ChkFwCmdIoDone(dev);		
			break;
#endif
			
		default:
			break;
	}
	
			
	ChkFwCmdIoDone(dev);		
	
	
	priv->SetFwCmdInProgress = false;
	RT_TRACE(COMP_CMD, "<---SetFwCmdIOWorkItemCallback()\n");
}

static	void
phy_CheckEphySwitchReady(struct net_device* dev)
{
	u32	delay = 100;	
	u8	regu1;

	regu1 = read_nic_byte(dev, 0x554);
	while ((regu1 & BIT5) && (delay > 0))
	{
		regu1 = read_nic_byte(dev, 0x554);
		delay--;
		udelay(50);
	}	
	RT_TRACE(COMP_INIT, "regu1=%02x  delay = %d\n", regu1, delay);	
	
}	

#ifdef TO_DO_LIST
void
HW_RadioGpioChk92SE(
	IN	PADAPTER	pAdapter
	)
{
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	u1Byte				u1Tmp = 0;
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(pAdapter);
	RT_RF_POWER_STATE	eRfPowerStateToSet;
	BOOLEAN				bActuallySet = false;

#if 0
	if (!RT_IN_PS_LEVEL(pAdapter, RT_RF_OFF_LEVL_PCI_D3) &&
		pMgntInfo->RfOffReason != RF_CHANGE_BY_HW)
	{
		return;
	}
	
		PlatformSwitchClkReq(pAdapter, 0x00);

	if (RT_IN_PS_LEVEL(pAdapter, RT_RF_OFF_LEVL_PCI_D3))
	{		
		RT_LEAVE_D3(pAdapter, false);
		RT_CLEAR_PS_LEVEL(pAdapter, RT_RF_OFF_LEVL_PCI_D3);
		Power_DomainInit92SE(pAdapter);
	}

	PlatformEFIOWrite1Byte(pAdapter, MAC_PINMUX_CFG, (GPIOMUX_EN | GPIOSEL_GPIO));

	u1Tmp = PlatformEFIORead1Byte(pAdapter, GPIO_IO_SEL);
	u1Tmp &= HAL_8192S_HW_GPIO_OFF_MASK;
	PlatformEFIOWrite1Byte(pAdapter, GPIO_IO_SEL, u1Tmp);

	RT_TRACE(COMP_CMD, DBG_LOUD, 
	("HW_RadioGpioChk92SE HW_RadioGpioChk92SE=%02x\n", HW_RadioGpioChk92SE));
	
	u1Tmp = PlatformEFIORead1Byte(pAdapter, GPIO_IN);

	eRfPowerStateToSet = (u1Tmp & HAL_8192S_HW_GPIO_OFF_BIT) ? eRfOn : eRfOff;

	if( (pHalData->bHwRadioOff == true) && (eRfPowerStateToSet == eRfOn))
	{
		RT_TRACE(COMP_RF, DBG_LOUD, ("HW_RadioGpioChk92SE  - HW Radio ON\n"));
		pHalData->bHwRadioOff = false;
		bActuallySet = true;
	}
	else if ( (pHalData->bHwRadioOff == false) && (eRfPowerStateToSet == eRfOff))
	{
		RT_TRACE(COMP_RF, DBG_LOUD, ("HW_RadioGpioChk92SE  - HW Radio OFF\n"));
		pHalData->bHwRadioOff = true;
		bActuallySet = true;
	}
			
	if(bActuallySet)
	{
		pHalData->bHwRfOffAction = 1;
		MgntActSet_RF_State(pAdapter, eRfPowerStateToSet, RF_CHANGE_BY_HW);
		DrvIFIndicateCurrentPhyStatus(pAdapter);

	
		{
			PMP_ADAPTER		pDevice = &(pAdapter->NdisAdapter);
			if(pDevice->RegHwSwRfOffD3 == 1 || pDevice->RegHwSwRfOffD3 == 2) 
				(eRfPowerStateToSet == eRfOff) ? RT_ENABLE_ASPM(pAdapter) : RT_DISABLE_ASPM(pAdapter);
		}
	}
	RT_TRACE(COMP_RF, DBG_TRACE, ("HW_RadioGpioChk92SE() <--------- \n"));
#endif
}/* HW_RadioGpioChk92SE */
#endif
#endif 
