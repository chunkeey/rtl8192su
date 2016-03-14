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
/* Check to see if the file has been included already.  */
#ifndef _R8192S_PHY_H
#define _R8192S_PHY_H


/*--------------------------Define Parameters-------------------------------*/
#define LOOP_LIMIT				5
#define MAX_STALL_TIME			50		
#define AntennaDiversityValue		0x80	
#define MAX_TXPWR_IDX_NMODE_92S	63
#define Reset_Cnt_Limit			3


#define MAX_PRECMD_CNT 			16
#define MAX_RFDEPENDCMD_CNT 	16
#define MAX_POSTCMD_CNT 		16
#ifdef RTL8192SE
#define	SET_RTL8192SE_RF_SLEEP(dev)							\
{																	\
	u8		u1bTmp;												\
	u1bTmp = read_nic_byte (dev, LDOV12D_CTRL);		\
	u1bTmp |= BIT0;													\
	write_nic_byte(dev, LDOV12D_CTRL, u1bTmp);		\
	write_nic_byte(dev, SPS1_CTRL, 0x0);				\
	write_nic_byte(dev, TXPAUSE, 0xFF);				\
	write_nic_word(dev, CMDR, 0x57FC);				\
	udelay(100);													\
	write_nic_word(dev, CMDR, 0x77FC);				\
	write_nic_byte(dev, PHY_CCA, 0x0);				\
	udelay(10);													\
	write_nic_word(dev, CMDR, 0x37FC);				\
	udelay(10);													\
	write_nic_word(dev, CMDR, 0x77FC);				\
	udelay(10);													\
	write_nic_word(dev, CMDR, 0x57FC);				\
}


#endif

/*--------------------------Define Parameters-------------------------------*/


/*------------------------------Define structure----------------------------*/ 
typedef enum _SwChnlCmdID{
	CmdID_End,
	CmdID_SetTxPowerLevel,
	CmdID_BBRegWrite10,
	CmdID_WritePortUlong,
	CmdID_WritePortUshort,
	CmdID_WritePortUchar,
	CmdID_RF_WriteReg,
}SwChnlCmdID;


typedef struct _SwChnlCmd{
	SwChnlCmdID	CmdID;
	u32			Para1;
	u32			Para2;
	u32			msDelay;
}__attribute__ ((packed)) SwChnlCmd;

extern u32 rtl819XMACPHY_Array_PG[];
extern u32 rtl819XPHY_REG_1T2RArray[];
extern u32 rtl819XAGCTAB_Array[];
extern u32 rtl819XRadioA_Array[];
extern u32 rtl819XRadioB_Array[];
extern u32 rtl819XRadioC_Array[];
extern u32 rtl819XRadioD_Array[];

typedef enum _HW90_BLOCK{
	HW90_BLOCK_MAC = 0,
	HW90_BLOCK_PHY0 = 1,
	HW90_BLOCK_PHY1 = 2,
	HW90_BLOCK_RF = 3,
	HW90_BLOCK_MAXIMUM = 4, 
}HW90_BLOCK_E, *PHW90_BLOCK_E;

typedef enum _RF90_RADIO_PATH{
	RF90_PATH_A = 0,			
	RF90_PATH_B = 1,			
	RF90_PATH_C = 2,			
	RF90_PATH_D = 3,			
#ifndef _RTL8192_EXT_PATCH_
	RF90_PATH_MAX	= 4,			
#endif
}RF90_RADIO_PATH_E, *PRF90_RADIO_PATH_E;
#ifdef _RTL8192_EXT_PATCH_
#define	RF90_PATH_MAX			2
#endif

#define bMaskByte0                0xff
#define bMaskByte1                0xff00
#define bMaskByte2                0xff0000
#define bMaskByte3                0xff000000
#define bMaskHWord                0xffff0000
#define bMaskLWord                0x0000ffff
#define bMaskDWord                0xffffffff

typedef enum _BaseBand_Config_Type{
	BaseBand_Config_PHY_REG = 0,			
	BaseBand_Config_AGC_TAB = 1,			
}BaseBand_Config_Type, *PBaseBand_Config_Type;

typedef enum _VERSION_8190{
	VERSION_8190_BD=0x3,
	VERSION_8190_BE
}VERSION_8190,*PVERSION_8190;


typedef enum _VERSION_8192S{
	VERSION_8192S_ACUT,
	VERSION_8192S_BCUT,
	VERSION_8192S_CCUT
}VERSION_8192S,*PVERSION_8192S;

typedef enum _PHY_Rate_Tx_Power_Offset_Area{
	RA_OFFSET_LEGACY_OFDM1,
	RA_OFFSET_LEGACY_OFDM2,
	RA_OFFSET_HT_OFDM1,
	RA_OFFSET_HT_OFDM2,
	RA_OFFSET_HT_OFDM3,
	RA_OFFSET_HT_OFDM4,
	RA_OFFSET_HT_CCK,
}RA_OFFSET_AREA,*PRA_OFFSET_AREA;
#if 0
typedef enum _RATR_TABLE_MODE_8192S{
	RATR_INX_WIRELESS_NGB = 0,
	RATR_INX_WIRELESS_NG = 1,
	RATR_INX_WIRELESS_NB = 2,
	RATR_INX_WIRELESS_N = 3,
	RATR_INX_WIRELESS_GB = 4,
	RATR_INX_WIRELESS_G = 5,
	RATR_INX_WIRELESS_B = 6,
	RATR_INX_WIRELESS_MC = 7,
	RATR_INX_WIRELESS_A = 8,
}RATR_TABLE_MODE_8192S, *PRATR_TABLE_MODE_8192S;
#endif
/*------------------------------Define structure----------------------------*/ 


/*------------------------Export global variable----------------------------*/
/*------------------------Export global variable----------------------------*/


/*------------------------Export Marco Definition---------------------------*/
/*------------------------Export Marco Definition---------------------------*/


/*--------------------------Exported Function prototype---------------------*/

extern	u32	rtl8192_QueryBBReg(struct net_device* dev,u32 RegAddr, u32 BitMask);
extern	void	rtl8192_setBBreg(struct net_device* dev,u32 RegAddr, u32 BitMask,u32 Data);
extern	u32	rtl8192_phy_QueryRFReg(struct net_device* dev,RF90_RADIO_PATH_E eRFPath, u32 RegAddr, u32 BitMask);
extern	void	rtl8192_phy_SetRFReg(struct net_device* dev,RF90_RADIO_PATH_E eRFPath, u32 RegAddr,u32 BitMask,u32 Data);

bool rtl8192_phy_checkBBAndRF(struct net_device* dev, HW90_BLOCK_E CheckBlock, RF90_RADIO_PATH_E eRFPath);


extern	bool 	PHY_MACConfig8192S(struct net_device* dev);
extern	bool 	PHY_BBConfig8192S(struct net_device* dev);
extern	bool 	PHY_RFConfig8192S(struct net_device* dev);

extern	u8	rtl8192_phy_ConfigRFWithHeaderFile(struct net_device* dev,RF90_RADIO_PATH_E eRFPath);
extern	void	rtl8192_SetBWMode(struct net_device* dev,HT_CHANNEL_WIDTH ChnlWidth,HT_EXTCHNL_OFFSET Offset	);
extern	u8	rtl8192_phy_SwChnl(struct net_device* dev,u8 channel);	
extern	u8	rtl8192_phy_CheckIsLegalRFPath(struct net_device* dev,u32 eRFPath	);
extern	void    rtl8192_BBConfig(struct net_device* dev);
extern	void 	PHY_IQCalibrateBcut(struct net_device* dev);
extern	void 	PHY_IQCalibrate(struct net_device* dev);
extern	void 	PHY_GetHWRegOriginalValue(struct net_device* dev);

extern void 	InitialGainOperateWorkItemCallBack(void *data);
void rtl8192_phy_setTxPower(struct net_device* dev, u8  channel);

/*--------------------------Exported Function prototype---------------------*/
bool rtl8192se_set_fw_cmd(struct net_device* dev, FW_CMD_IO_TYPE                FwCmdIO);
extern void PHY_SetBeaconHwReg( struct net_device* dev, u16 BeaconInterval);
void ChkFwCmdIoDone(struct net_device* dev);
void PHY_SwitchEphyParameter(struct net_device* dev);
bool PHY_SetRFPowerState(struct net_device* dev, RT_RF_POWER_STATE eRFPowerState);
extern void PHY_ScanOperationBackup8192S(struct net_device* dev,u8 Operation);
#endif	

