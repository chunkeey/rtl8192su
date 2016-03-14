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

/*---------------------------Define Local Constant---------------------------*/
typedef struct RF_Shadow_Compare_Map {
	u32		Value;
	u8		Compare;
	u8		ErrorOrNot;
	u8		Recorver;
	u8		Driver_Write;
}RF_SHADOW_T;
/*---------------------------Define Local Constant---------------------------*/


/*------------------------Define global variable-----------------------------*/
/*------------------------Define global variable-----------------------------*/




/*---------------------Define local function prototype-----------------------*/
void phy_RF6052_Config_HardCode(struct net_device* dev);

bool phy_RF6052_Config_ParaFile(struct net_device* dev);
/*---------------------Define local function prototype-----------------------*/

/*------------------------Define function prototype--------------------------*/
extern void RF_ChangeTxPath(struct net_device* dev,  u16 DataRate);

/*------------------------Define function prototype--------------------------*/

/*------------------------Define local variable------------------------------*/
static	RF_SHADOW_T	RF_Shadow[RF6052_MAX_PATH][RF6052_MAX_REG];
/*------------------------Define local variable------------------------------*/

/*------------------------Define function prototype--------------------------*/
extern void RF_ChangeTxPath(struct net_device* dev,  u16 DataRate)
{
}	/* RF_ChangeTxPath */


void PHY_RF6052SetBandwidth(struct net_device* dev, HT_CHANNEL_WIDTH Bandwidth)	
{	
	u8				eRFPath;	
	struct r8192_priv 	*priv = rtllib_priv(dev);
	

	if (IS_HARDWARE_TYPE_8192SE(dev))
	{		
#if 1
		switch(Bandwidth)
		{
			case HT_CHANNEL_WIDTH_20:
#if 1
				priv->RfRegChnlVal[0] = ((priv->RfRegChnlVal[0] & 0xfffff3ff) | 0x0400);
				rtl8192_phy_SetRFReg(dev, RF90_PATH_A, RF_CHNLBW, bRFRegOffsetMask, priv->RfRegChnlVal[0]);
#else
				rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A, RF_CHNLBW, BIT10|BIT11, 0x01);
#endif
				break;
			case HT_CHANNEL_WIDTH_20_40:
#if 1
				priv->RfRegChnlVal[0] = ((priv->RfRegChnlVal[0] & 0xfffff3ff));
				rtl8192_phy_SetRFReg(dev, RF90_PATH_A, RF_CHNLBW, bRFRegOffsetMask, priv->RfRegChnlVal[0]);
#else
				rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A, RF_CHNLBW, BIT10|BIT11, 0x00);
#endif
				break;
			default:
				RT_TRACE(COMP_DBG, "PHY_SetRF6052Bandwidth(): unknown Bandwidth: %#X\n",Bandwidth);
				break;			
		}
#endif	
	}
	else
	{
		for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
		{
			switch(Bandwidth)
			{
				case HT_CHANNEL_WIDTH_20:
					break;
				case HT_CHANNEL_WIDTH_20_40:
					break;
				default:
					RT_TRACE(COMP_DBG, "PHY_SetRF8225Bandwidth(): unknown Bandwidth: %#X\n",Bandwidth );
					break;
					
			}
		}
	}
}


extern void PHY_RF6052SetCckTxPower(struct net_device* dev, u8	powerlevel)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u32				TxAGC=0;
	bool				dontIncCCKOrTurboScanOff=false;

	if (((priv->eeprom_version >= 2) && (priv->TxPwrSafetyFlag == 1)) ||
	     ((priv->eeprom_version >= 2) && (priv->EEPROMRegulatory != 0))) {
		dontIncCCKOrTurboScanOff = true;
	}

	if(rtllib_act_scanning(priv->rtllib,true) == true){
		TxAGC = 0x3f;

		if(dontIncCCKOrTurboScanOff )
			TxAGC = powerlevel;
	} else {
		TxAGC = powerlevel;

		if(priv->DynamicTxHighPowerLvl == TxHighPwrLevel_Level1)
			TxAGC = 0x10;
		else if(priv->DynamicTxHighPowerLvl == TxHighPwrLevel_Level2)
			TxAGC = 0x0;
	}
	
	if(TxAGC > RF6052_MAX_TX_PWR)
		TxAGC = RF6052_MAX_TX_PWR;

	rtl8192_setBBreg(dev, rTxAGC_CCK_Mcs32, bTxAGCRateCCK, TxAGC);

}	/* PHY_RF6052SetCckTxPower */


#if 0
extern void PHY_RF6052SetOFDMTxPower(struct net_device* dev, u8 powerlevel, u8 Channel)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u32 	writeVal, powerBase0, powerBase1;
	u8 	index = 0;
	u16 	RegOffset[6] = {0xe00, 0xe04, 0xe10, 0xe14, 0xe18, 0xe1c};
	u8	rfa_pwr[4];
	u8 	rfa_lower_bound = 0, rfa_upper_bound = 0;
	u8	i;
	u8	rf_pwr_diff = 0, chnlGroup = 0;
	u8	Legacy_pwrdiff=0, HT20_pwrdiff=0;

	if (priv->eeprom_version < 2)
		powerBase0 = powerlevel + (priv->LegacyHTTxPowerDiff & 0xf); 
	else if (priv->eeprom_version >= 2)	
	{
		Legacy_pwrdiff = priv->TxPwrLegacyHtDiff[RF90_PATH_A][Channel-1];
		powerBase0 = powerlevel + Legacy_pwrdiff; 
		RT_TRACE(COMP_POWER, " [LagacyToHT40 pwr diff = %d]\n", Legacy_pwrdiff);		
		RT_TRACE(COMP_POWER, " [OFDM power base index = 0x%x]\n", powerBase0);
	}
	powerBase0 = (powerBase0<<24) | (powerBase0<<16) |(powerBase0<<8) |powerBase0;
	
	if(priv->eeprom_version >= 2)
	{	
	
		if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20)
		{
			HT20_pwrdiff = priv->TxPwrHt20Diff[RF90_PATH_A][Channel-1];

			if (HT20_pwrdiff < 8)	
				powerlevel += HT20_pwrdiff;	
			else				
				powerlevel -= (16-HT20_pwrdiff);

			RT_TRACE(COMP_POWER, " [HT20 to HT40 pwrdiff = %d]\n", HT20_pwrdiff);
			RT_TRACE(COMP_POWER, " [MCS power base index = 0x%x]\n", powerlevel);
		}
	}
	powerBase1 = powerlevel;							
	powerBase1 = (powerBase1<<24) | (powerBase1<<16) |(powerBase1<<8) |powerBase1;

	RT_TRACE(COMP_POWER, " [Legacy/HT power index= %x/%x]\n", powerBase0, powerBase1);
	
	for(index=0; index<6; index++)
	{

		if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
		{
			writeVal = ((index<2)?powerBase0:powerBase1);
		}
		else
		{
			if(priv->pwrGroupCnt == 0)
				chnlGroup = 0;
			if(priv->pwrGroupCnt >= 3)
			{
				if(Channel <= 3)
					chnlGroup = 0;
				else if(Channel >= 4 && Channel <= 9)
					chnlGroup = 1;
				else if(Channel >= 10)
					chnlGroup = 2;
				if(priv->pwrGroupCnt == 4)
					chnlGroup ++;
			}
		else
				chnlGroup = 0;
			writeVal = priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index] + 
					((index<2)?powerBase0:powerBase1);
		}
		RT_TRACE(COMP_POWER, "Reg 0x%x, chnlGroup = %d, Original=%x writeVal=%x\n", 
			RegOffset[index], chnlGroup, priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index], 
			writeVal);

		if (priv->rf_type == RF_2T2R)
		{			
			rf_pwr_diff = priv->AntennaTxPwDiff[0];
			RT_TRACE(COMP_POWER, "2T2R RF-B to RF-A PWR DIFF=%d\n", rf_pwr_diff);

			if (rf_pwr_diff >= 8)		
			{	
				rfa_lower_bound = 0x10-rf_pwr_diff;
				RT_TRACE(COMP_POWER, "rfa_lower_bound= %d\n", rfa_lower_bound);
			}
			else if (rf_pwr_diff >= 0)	
			{
				rfa_upper_bound = RF6052_MAX_TX_PWR-rf_pwr_diff;
				RT_TRACE(COMP_POWER, "rfa_upper_bound= %d\n", rfa_upper_bound);
			}			
		}

		for (i=  0; i <4; i++)
		{
			rfa_pwr[i] = (u8)((writeVal & (0x7f<<(i*8)))>>(i*8));
			if (rfa_pwr[i]  > RF6052_MAX_TX_PWR)
				rfa_pwr[i]  = RF6052_MAX_TX_PWR;

			if (priv->rf_type == RF_2T2R)
			{
				if (rf_pwr_diff >= 8)		
				{	
					if (rfa_pwr[i] <rfa_lower_bound)
					{
						RT_TRACE(COMP_POWER, "Underflow");
						rfa_pwr[i] = rfa_lower_bound;
					}
				}
				else if (rf_pwr_diff >= 1)	
				{	
					if (rfa_pwr[i] > rfa_upper_bound)
					{
						RT_TRACE(COMP_POWER, "Overflow");
						rfa_pwr[i] = rfa_upper_bound;
					}
				}
				RT_TRACE(COMP_POWER, "rfa_pwr[%d]=%x\n", i, rfa_pwr[i]);
			}

		}

#if 1
		writeVal = (rfa_pwr[3]<<24) | (rfa_pwr[2]<<16) |(rfa_pwr[1]<<8) |rfa_pwr[0];
		RT_TRACE(COMP_POWER, "WritePower=%08x\n", writeVal);
#else
		if(priv->bDynamicTxHighPower == true)     
		{	
			if(index > 1)	
			{
				writeVal = 0x03030303;
			}
			else
			{
				writeVal = (rfa_pwr[3]<<24) | (rfa_pwr[2]<<16) |(rfa_pwr[1]<<8) |rfa_pwr[0];
			}
			RT_TRACE(COMP_POWER, "HighPower=%08x\n", writeVal);
		}
		else
		{
			writeVal = (rfa_pwr[3]<<24) | (rfa_pwr[2]<<16) |(rfa_pwr[1]<<8) |rfa_pwr[0];
			RT_TRACE(COMP_POWER, "NormalPower=%08x\n", writeVal);
		}
#endif
		rtl8192_setBBreg(dev, RegOffset[index], 0x7f7f7f7f, writeVal);
	}

}	/* PHY_RF6052SetOFDMTxPower */
#endif

void getPowerBase(
	struct net_device* dev,
	u8*		pPowerLevel,
	u8		Channel,
	u32*	OfdmBase,
	u32*	MCSBase,
	u8*		pFinalPowerIndex
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u32			powerBase0, powerBase1;
	u8			Legacy_pwrdiff=0, HT20_pwrdiff=0;
	u8			i, powerlevel[4];
	
	for(i=0; i<2; i++)
		powerlevel[i] = pPowerLevel[i];
	if (priv->eeprom_version < 2)
		powerBase0 = powerlevel[0] + (priv->LegacyHTTxPowerDiff & 0xf); 
	else if (priv->eeprom_version >= 2)
	{
		Legacy_pwrdiff = priv->TxPwrLegacyHtDiff[RF90_PATH_A][Channel-1];
		powerBase0 = powerlevel[0] + Legacy_pwrdiff; 
	}
	powerBase0 = (powerBase0<<24) | (powerBase0<<16) |(powerBase0<<8) |powerBase0;
	*OfdmBase = powerBase0;
	RT_TRACE(COMP_POWER, " [OFDM power base index = 0x%x]\n", powerBase0);
	
	if(priv->eeprom_version >= 2)
	{
		if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20)
		{
			for(i=0; i<2; i++)	
			{	
				HT20_pwrdiff = priv->TxPwrHt20Diff[i][Channel-1];
				if (HT20_pwrdiff < 8)	
					powerlevel[i] += HT20_pwrdiff;	
				else				
					powerlevel[i] -= (16-HT20_pwrdiff);
			}
		}
	}
	powerBase1 = powerlevel[0];	
	powerBase1 = (powerBase1<<24) | (powerBase1<<16) |(powerBase1<<8) |powerBase1;
	*MCSBase = powerBase1;
	
	RT_TRACE(COMP_POWER, " [MCS power base index = 0x%x]\n", powerBase1);

	pFinalPowerIndex[0] = powerlevel[0];
	pFinalPowerIndex[1] = powerlevel[1];
	switch(priv->EEPROMRegulatory)
	{
		case 3:
			if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
			{
				pFinalPowerIndex[0] += priv->PwrGroupHT40[RF90_PATH_A][Channel-1];
				pFinalPowerIndex[1] += priv->PwrGroupHT40[RF90_PATH_B][Channel-1];
			}
			else
			{
				pFinalPowerIndex[0] += priv->PwrGroupHT20[RF90_PATH_A][Channel-1];
				pFinalPowerIndex[1] += priv->PwrGroupHT20[RF90_PATH_B][Channel-1];
			}
			break;
		default:
			break;
	}
	if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
	{
		RT_TRACE(COMP_POWER, "40MHz finalPowerIndex (A / B) = 0x%x / 0x%x\n", 
			pFinalPowerIndex[0], pFinalPowerIndex[1]);
	}
	else
	{
		RT_TRACE(COMP_POWER, "20MHz finalPowerIndex (A / B) = 0x%x / 0x%x\n", 
			pFinalPowerIndex[0], pFinalPowerIndex[1]);
	}
}

void getTxPowerWriteValByRegulatory(
	struct net_device* dev,
	u8		Channel,
	u8		index,
	u32		powerBase0,
	u32		powerBase1,
	u32*	pOutWriteVal
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8		i, chnlGroup, pwr_diff_limit[4];
	u32 		writeVal, customer_limit;
	
	switch(priv->EEPROMRegulatory)
	{
		case 0:	
			chnlGroup = 0;
			RT_TRACE(COMP_POWER,"MCSTxPowerLevelOriginalOffset[%d][%d] = 0x%x\n", 
				chnlGroup, index, priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index]);
			writeVal = priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index] + 
				((index<2)?powerBase0:powerBase1);
			RT_TRACE(COMP_POWER,"RTK better performance, writeVal = 0x%x\n", writeVal);
			break;
		case 1:	
			if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
			{
				writeVal = ((index<2)?powerBase0:powerBase1);
				RT_TRACE(COMP_POWER,"Realtek regulatory, 40MHz, writeVal = 0x%x\n", writeVal);
			}
			else
			{
				if(priv->pwrGroupCnt == 1)
					chnlGroup = 0;
				if(priv->pwrGroupCnt >= 3)
				{
					if(Channel <= 3)
						chnlGroup = 0;
					else if(Channel >= 4 && Channel <= 8)
						chnlGroup = 1;
					else if(Channel > 8)
						chnlGroup = 2;
					if(priv->pwrGroupCnt == 4)
						chnlGroup++;
				}
				RT_TRACE(COMP_POWER,"MCSTxPowerLevelOriginalOffset[%d][%d] = 0x%x\n", 
				chnlGroup, index, priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index]);
				writeVal = priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index] + 
						((index<2)?powerBase0:powerBase1);
				RT_TRACE(COMP_POWER,"Realtek regulatory, 20MHz, writeVal = 0x%x\n", writeVal);
			}
			break;
		case 2:	
			writeVal = ((index<2)?powerBase0:powerBase1);
			RT_TRACE(COMP_POWER,"Better regulatory, writeVal = 0x%x\n", writeVal);
			break;
		case 3:	
			chnlGroup = 0;
			RT_TRACE(COMP_POWER,"MCSTxPowerLevelOriginalOffset[%d][%d] = 0x%x\n", 
				chnlGroup, index, priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index]);

			if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
			{
				RT_TRACE(COMP_POWER,"customer's limit, 40MHz = 0x%x\n", 
					priv->PwrGroupHT40[RF90_PATH_A][Channel-1]);
			}
			else
			{
				RT_TRACE(COMP_POWER,"customer's limit, 20MHz = 0x%x\n", 
					priv->PwrGroupHT20[RF90_PATH_A][Channel-1]);
			}
			for (i=0; i<4; i++)
			{
				pwr_diff_limit[i] = (u8)((priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index]&(0x7f<<(i*8)))>>(i*8));
				if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
				{
					if(pwr_diff_limit[i] > priv->PwrGroupHT40[RF90_PATH_A][Channel-1])
					{
						pwr_diff_limit[i] = priv->PwrGroupHT40[RF90_PATH_A][Channel-1];
					}
				}
				else
				{
					if(pwr_diff_limit[i] > priv->PwrGroupHT20[RF90_PATH_A][Channel-1])
					{
						pwr_diff_limit[i] = priv->PwrGroupHT20[RF90_PATH_A][Channel-1];
					}
				}
			}
			customer_limit = (pwr_diff_limit[3]<<24) | (pwr_diff_limit[2]<<16) |
							(pwr_diff_limit[1]<<8) | (pwr_diff_limit[0]);
			RT_TRACE(COMP_POWER,"Customer's limit = 0x%x\n", customer_limit);

			writeVal = customer_limit + ((index<2)?powerBase0:powerBase1);
			RT_TRACE(COMP_POWER,"Customer, writeVal = 0x%x\n", writeVal);
			break;
		default:
			chnlGroup = 0;
			writeVal = priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index] + 
				((index<2)?powerBase0:powerBase1);
			RT_TRACE(COMP_POWER,"RTK better performance, writeVal = 0x%x\n", writeVal);
			break;
	}

	if(priv->DynamicTxHighPowerLvl == TxHighPwrLevel_Level1)
		writeVal = 0x10101010;
	else if(priv->DynamicTxHighPowerLvl == TxHighPwrLevel_Level2)
		writeVal = 0x0;

	*pOutWriteVal = writeVal;

}

void setAntennaDiff(
	struct net_device* dev,
	u8*		pFinalPowerIndex
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	char	ant_pwr_diff=0;
	u32	u4RegValue=0;

	if (priv->rf_type == RF_2T2R)
	{
		ant_pwr_diff = pFinalPowerIndex[1] - pFinalPowerIndex[0];
		
		if(ant_pwr_diff > 7)
			ant_pwr_diff = 7;
		if(ant_pwr_diff < -8)
			ant_pwr_diff = -8;
		RT_TRACE(COMP_POWER,"Antenna Diff from RF-B to RF-A = %d (0x%x)\n", 
			ant_pwr_diff, ant_pwr_diff&0xf);
		ant_pwr_diff &= 0xf;
	}
	priv->AntennaTxPwDiff[2] = 0;
	priv->AntennaTxPwDiff[1] = 0;
	priv->AntennaTxPwDiff[0] = (u8)(ant_pwr_diff);		

	u4RegValue = (priv->AntennaTxPwDiff[2]<<8 | 
				priv->AntennaTxPwDiff[1]<<4 | 
				priv->AntennaTxPwDiff[0]	);

	rtl8192_setBBreg(dev, rFPGA0_TxGainStage, 
		(bXBTxAGC|bXCTxAGC|bXDTxAGC), u4RegValue);

	RT_TRACE(COMP_POWER,"Write BCD-Diff(0x%x) = 0x%x\n", 
		rFPGA0_TxGainStage, u4RegValue);
}

void writeOFDMPowerReg(
	struct net_device* dev,
	u8		index,
	u32		Value
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u16 RegOffset[6] = {0xe00, 0xe04, 0xe10, 0xe14, 0xe18, 0xe1c};
	u8 i, rfa_pwr[4];
	u8 rfa_lower_bound = 0, rfa_upper_bound = 0, rf_pwr_diff = 0;
	u32 writeVal=Value;
	if (priv->rf_type == RF_2T2R)
	{			
		rf_pwr_diff = priv->AntennaTxPwDiff[0];

		if (rf_pwr_diff >= 8)		
		{	
			rfa_lower_bound = 0x10-rf_pwr_diff;
			RT_TRACE(COMP_POWER,"rfa_lower_bound= %d\n", rfa_lower_bound);
		}
		else
		{
			rfa_upper_bound = RF6052_MAX_TX_PWR-rf_pwr_diff;
			RT_TRACE(COMP_POWER,"rfa_upper_bound= %d\n", rfa_upper_bound);
		}			
	}

	for (i=0; i<4; i++)
	{
		rfa_pwr[i] = (u8)((writeVal & (0x7f<<(i*8)))>>(i*8));
		if (rfa_pwr[i]  > RF6052_MAX_TX_PWR)
			rfa_pwr[i]  = RF6052_MAX_TX_PWR;

		if (priv->rf_type == RF_2T2R)
		{
			if (rf_pwr_diff >= 8)		
			{	
				if (rfa_pwr[i] <rfa_lower_bound)
				{
					RT_TRACE(COMP_POWER,"Underflow");
					rfa_pwr[i] = rfa_lower_bound;
				}
			}
			else if (rf_pwr_diff >= 1)	
			{	
				if (rfa_pwr[i] > rfa_upper_bound)
				{
					RT_TRACE(COMP_POWER,"Overflow");
					rfa_pwr[i] = rfa_upper_bound;
				}
			}
			RT_TRACE(COMP_POWER,"rfa_pwr[%d]=%x\n", i, rfa_pwr[i]);
		}

	}

	writeVal = (rfa_pwr[3]<<24) | (rfa_pwr[2]<<16) |(rfa_pwr[1]<<8) |rfa_pwr[0];

	rtl8192_setBBreg(dev, RegOffset[index], 0x7f7f7f7f, writeVal);
	RT_TRACE(COMP_POWER,"Set 0x%x = %08x\n",RegOffset[index], writeVal);
}

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
 * 01/06/2009	MHC		1. Prevent Path B tx power overflow or underflow dure to
 *						A/B pwr difference or legacy/HT pwr diff.
 *						2. We concern with path B legacy/HT OFDM difference.
 * 01/22/2009	MHC		Support new EPRO format from SD3.
 *
 *---------------------------------------------------------------------------*/
extern	void 
PHY_RF6052SetOFDMTxPower(struct net_device* dev, u8* pPowerLevel, u8 Channel)
{
	u32 writeVal, powerBase0, powerBase1;
	u8 index = 0;
	u8 finalPowerIndex[4];

	getPowerBase(dev, pPowerLevel, Channel, &powerBase0, &powerBase1, &finalPowerIndex[0]);
	setAntennaDiff(dev, &finalPowerIndex[0]	);
	
	for(index=0; index<6; index++)
	{
		getTxPowerWriteValByRegulatory(dev, Channel, index, 
			powerBase0, powerBase1, &writeVal);

		writeOFDMPowerReg(dev, index, writeVal);
	}
}

bool PHY_RF6052_Config(struct net_device* dev)
{
	struct r8192_priv 			*priv = rtllib_priv(dev);
	bool rtStatus = true;	
	u8 bRegHwParaFile = 1;
	
	if(priv->rf_type == RF_1T1R)
		priv->NumTotalRFPath = 1;
	else
	priv->NumTotalRFPath = 2;

	switch(bRegHwParaFile)
	{
		case 0:
			phy_RF6052_Config_HardCode(dev);
			break;

		case 1:
			rtStatus = phy_RF6052_Config_ParaFile(dev);
			break;

		case 2:
			phy_RF6052_Config_HardCode(dev);
			phy_RF6052_Config_ParaFile(dev);
			break;

		default:
			phy_RF6052_Config_HardCode(dev);
			break;
	}
	return rtStatus;
		
}

void phy_RF6052_Config_HardCode(struct net_device* dev)
{
	

	
}

bool phy_RF6052_Config_ParaFile(struct net_device* dev)
{
	u32			u4RegValue = 0;
	u8			eRFPath;
	bool rtStatus = true;
	struct r8192_priv 	*priv = rtllib_priv(dev);
	BB_REGISTER_DEFINITION_T	*pPhyReg;	


	for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
	{

		pPhyReg = &priv->PHYRegDef[eRFPath];
		
		switch(eRFPath)
		{
		case RF90_PATH_A:
		case RF90_PATH_C:
			u4RegValue = rtl8192_QueryBBReg(dev, pPhyReg->rfintfs, bRFSI_RFENV);
			break;
		case RF90_PATH_B :
		case RF90_PATH_D:
			u4RegValue = rtl8192_QueryBBReg(dev, pPhyReg->rfintfs, bRFSI_RFENV<<16);
			break;
		}

		rtl8192_setBBreg(dev, pPhyReg->rfintfe, bRFSI_RFENV<<16, 0x1);
		
		rtl8192_setBBreg(dev, pPhyReg->rfintfo, bRFSI_RFENV, 0x1);

		rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2, b3WireAddressLength, 0x0); 	
		rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2, b3WireDataLength, 0x0);	


		switch(eRFPath)
		{
		case RF90_PATH_A:
#if	RTL8190_Download_Firmware_From_Header
			rtStatus= rtl8192_phy_ConfigRFWithHeaderFile(dev,(RF90_RADIO_PATH_E)eRFPath);
#else
			rtStatus = PHY_ConfigRFWithParaFile(dev, (char* )&szRadioAFile, (RF90_RADIO_PATH_E)eRFPath);
#endif
			break;
		case RF90_PATH_B:
#if	RTL8190_Download_Firmware_From_Header
			rtStatus= rtl8192_phy_ConfigRFWithHeaderFile(dev,(RF90_RADIO_PATH_E)eRFPath);
#else
			if(priv->rf_type == RF_2T2R_GREEN)
				rtStatus = PHY_ConfigRFWithParaFile(dev, (char *)&szRadioBGMFile, (RF90_RADIO_PATH_E)eRFPath);
			else
				rtStatus = PHY_ConfigRFWithParaFile(dev, (char* )&szRadioBFile, (RF90_RADIO_PATH_E)eRFPath);
#endif
			break;
		case RF90_PATH_C:
			break;
		case RF90_PATH_D:
			break;
		}

		switch(eRFPath)
		{
		case RF90_PATH_A:
		case RF90_PATH_C:
			rtl8192_setBBreg(dev, pPhyReg->rfintfs, bRFSI_RFENV, u4RegValue);
			break;
		case RF90_PATH_B :
		case RF90_PATH_D:
			rtl8192_setBBreg(dev, pPhyReg->rfintfs, bRFSI_RFENV<<16, u4RegValue);
			break;
		}

		if(rtStatus != true){
			printk("phy_RF6052_Config_ParaFile():Radio[%d] Fail!!", eRFPath);
			goto phy_RF6052_Config_ParaFile_Fail;
		}

	}

	RT_TRACE(COMP_INIT, "<---phy_RF6052_Config_ParaFile()\n");
	return rtStatus;
	
phy_RF6052_Config_ParaFile_Fail:	
	return rtStatus;
}


extern u32 PHY_RFShadowRead(
	struct net_device		* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32					Offset)
{
	return	RF_Shadow[eRFPath][Offset].Value;
	
}	/* PHY_RFShadowRead */


extern void PHY_RFShadowWrite(
	struct net_device		* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32					Offset,
	u32					Data)
{
	RF_Shadow[eRFPath][Offset].Value = (Data & bMask20Bits);
	RF_Shadow[eRFPath][Offset].Driver_Write = true;
	
}	/* PHY_RFShadowWrite */


extern bool PHY_RFShadowCompare(
	struct net_device		* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32					Offset)
{
	u32	reg;
	
	if (RF_Shadow[eRFPath][Offset].Compare == true)
	{
		reg = rtl8192_phy_QueryRFReg(dev, eRFPath, Offset, bMask20Bits);
		if (RF_Shadow[eRFPath][Offset].Value != reg)
		{
			RF_Shadow[eRFPath][Offset].ErrorOrNot = true;
			RT_TRACE(COMP_INIT, "PHY_RFShadowCompare RF-%d Addr%02xErr = %05x", eRFPath, Offset, reg);
		}
		else
		{
			RT_TRACE(COMP_INIT, "PHY_RFShadowCompare RF-%d Addr%02x Err = %05x return false\n", eRFPath, Offset, reg);
			RF_Shadow[eRFPath][Offset].ErrorOrNot = false;
		}
		return RF_Shadow[eRFPath][Offset].ErrorOrNot;
	}
	return false;
}	/* PHY_RFShadowCompare */

extern void PHY_RFShadowRecorver(
	struct net_device		* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32					Offset)
{
	if (RF_Shadow[eRFPath][Offset].ErrorOrNot == true)
	{
		if (RF_Shadow[eRFPath][Offset].Recorver == true)
		{
			rtl8192_phy_SetRFReg(dev, eRFPath, Offset, bMask20Bits, RF_Shadow[eRFPath][Offset].Value);
			RT_TRACE(COMP_INIT, "PHY_RFShadowRecorver RF-%d Addr%02x=%05x", 
			eRFPath, Offset, RF_Shadow[eRFPath][Offset].Value);
		}
	}
	
}	/* PHY_RFShadowRecorver */


extern void PHY_RFShadowCompareAll(struct net_device * dev)
{
	u32		eRFPath;
	u32		Offset;

	for (eRFPath = 0; eRFPath < RF6052_MAX_PATH; eRFPath++)
	{
		for (Offset = 0; Offset <= RF6052_MAX_REG; Offset++)
		{
			PHY_RFShadowCompare(dev, (RF90_RADIO_PATH_E)eRFPath, Offset);
		}
	}
	
}	/* PHY_RFShadowCompareAll */


extern void PHY_RFShadowRecorverAll(struct net_device * dev)
{
	u32		eRFPath;
	u32		Offset;

	for (eRFPath = 0; eRFPath < RF6052_MAX_PATH; eRFPath++)
	{
		for (Offset = 0; Offset <= RF6052_MAX_REG; Offset++)
		{
			PHY_RFShadowRecorver(dev, (RF90_RADIO_PATH_E)eRFPath, Offset);
		}
	}
	
}	/* PHY_RFShadowRecorverAll */


extern void PHY_RFShadowCompareFlagSet(
	struct net_device 		* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32					Offset,
	u8					Type)
{
	RF_Shadow[eRFPath][Offset].Compare = Type;
		
}	/* PHY_RFShadowCompareFlagSet */


extern void PHY_RFShadowRecorverFlagSet(
	struct net_device 		* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32					Offset,
	u8					Type)
{
	RF_Shadow[eRFPath][Offset].Recorver= Type;
		
}	/* PHY_RFShadowRecorverFlagSet */


extern void PHY_RFShadowCompareFlagSetAll(struct net_device  * dev)
{
	u32		eRFPath;
	u32		Offset;

	for (eRFPath = 0; eRFPath < RF6052_MAX_PATH; eRFPath++)
	{
		for (Offset = 0; Offset <= RF6052_MAX_REG; Offset++)
		{
			if (Offset != 0x26 && Offset != 0x27)
				PHY_RFShadowCompareFlagSet(dev, (RF90_RADIO_PATH_E)eRFPath, Offset, false);
			else
				PHY_RFShadowCompareFlagSet(dev, (RF90_RADIO_PATH_E)eRFPath, Offset, true);
		}
	}
		
}	/* PHY_RFShadowCompareFlagSetAll */


extern void PHY_RFShadowRecorverFlagSetAll(struct net_device  * dev)
{
	u32		eRFPath;
	u32		Offset;

	for (eRFPath = 0; eRFPath < RF6052_MAX_PATH; eRFPath++)
	{
		for (Offset = 0; Offset <= RF6052_MAX_REG; Offset++)
		{
			if (Offset != 0x26 && Offset != 0x27)
				PHY_RFShadowRecorverFlagSet(dev, (RF90_RADIO_PATH_E)eRFPath, Offset, false);
			else
				PHY_RFShadowRecorverFlagSet(dev, (RF90_RADIO_PATH_E)eRFPath, Offset, true);
		}
	}
		
}	/* PHY_RFShadowCompareFlagSetAll */



extern void PHY_RFShadowRefresh(struct net_device  * dev)
{
	u32		eRFPath;
	u32		Offset;

	for (eRFPath = 0; eRFPath < RF6052_MAX_PATH; eRFPath++)
	{
		for (Offset = 0; Offset <= RF6052_MAX_REG; Offset++)
		{
			RF_Shadow[eRFPath][Offset].Value = 0;
			RF_Shadow[eRFPath][Offset].Compare = false;
			RF_Shadow[eRFPath][Offset].Recorver  = false;
			RF_Shadow[eRFPath][Offset].ErrorOrNot = false;
			RF_Shadow[eRFPath][Offset].Driver_Write = false;
		}
	}
	
}	/* PHY_RFShadowRead */

/* End of HalRf6052.c */

#endif
