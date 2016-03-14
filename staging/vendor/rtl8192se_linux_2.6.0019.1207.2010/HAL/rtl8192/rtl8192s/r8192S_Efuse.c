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
#include "../rtl_core.h"
#include <linux/types.h>

#ifdef RTL8192SE
/*---------------------------Define Local Constant---------------------------*/
#define 	_POWERON_DELAY_
#define 	_PRE_EXECUTE_READ_CMD_

#define		EFUSE_REPEAT_THRESHOLD_		3
#define		EFUSE_ERROE_HANDLE		1


typedef struct _EFUSE_MAP_A{
	u8 offset;		
	u8 word_start;	
	u8 byte_start;	
	u8 byte_cnts;
	
}EFUSE_MAP, *PEFUSE_MAP;

typedef struct PG_PKT_STRUCT_A{
	u8 offset;
	u8 word_en;
	u8 data[8];	
}PGPKT_STRUCT,*PPGPKT_STRUCT;

typedef enum _EFUSE_DATA_ITEM{
	EFUSE_CHIP_ID=0,
	EFUSE_LDO_SETTING,
	EFUSE_CLK_SETTING,
	EFUSE_SDIO_SETTING,
	EFUSE_CCCR,
	EFUSE_SDIO_MODE,
	EFUSE_OCR,
	EFUSE_F0CIS,
	EFUSE_F1CIS,
	EFUSE_MAC_ADDR,
	EFUSE_EEPROM_VER,
	EFUSE_CHAN_PLAN,
	EFUSE_TXPW_TAB
} EFUSE_DATA_ITEM;

struct efuse_priv 
{    
	u8		id[2];
	u8		ldo_setting[2];
	u8		clk_setting[2];
	u8		cccr;
	u8		sdio_mode;
	u8		ocr[3];
	u8		cis0[17];
	u8		cis1[48];	
	u8		mac_addr[6];
	u8		eeprom_verno;	
	u8		channel_plan;
	u8		tx_power_b[14];
	u8		tx_power_g[14];	
};

/*---------------------------Define Local Constant---------------------------*/


/*------------------------Define global variable-----------------------------*/
const u8 MAX_PGPKT_SIZE = 9; 
const u8 PGPKT_DATA_SIZE = 8; 
const u32 EFUSE_MAX_SIZE = 512;

const u8 EFUSE_OOB_PROTECT_BYTES = 15; 


const EFUSE_MAP RTL8712_SDIO_EFUSE_TABLE[]={
/*ID*/		{0		,0		,0			,2	}, 
/*LDO Setting*/	{0		,1		,0			,2	}, 
/*CLK Setting*/	{0		,2		,0			,2	}, 
/*SDIO Setting*/{1		,0		,0			,1	}, 
/*CCCR*/	{1		,0		,1			,1	}, 
/*SDIO MODE*/	{1		,1		,0			,1	}, 
/*OCR*/		{1		,1		,1			,3	}, 
/*CCIS*/	{1		,3		,0			,17	}, 
/*F1CIS*/	{3		,3		,1			,48	}, 
/*MAC Addr*/	{10		,0		,0			,6	}, 
/*EEPROM ver*/	{10		,3		,0			,1	}, 
/*Channel plan*/{10		,3		,1			,1	}, 
/*TxPwIndex */	{11		,0		,0			,28	}  
};

/*------------------------Define global variable-----------------------------*/


/*------------------------Define local variable------------------------------*/

/*------------------------Define local variable------------------------------*/


/*--------------------Define function prototype-----------------------*/
extern	void	
EFUSE_Initialize(struct net_device* dev);
extern	u8	
EFUSE_Read1Byte(struct net_device* dev, u16 Address);
extern	void	
EFUSE_Write1Byte(struct net_device* dev, u16 Address,u8 Value);

static	void
efuse_ShadowRead1Byte(struct net_device* dev,u16 Offset,u8 *Value);
static	void
efuse_ShadowRead2Byte(struct net_device* dev,	u16 Offset,u16 *Value	);
static	void
efuse_ShadowRead4Byte(struct net_device* dev,	u16 Offset,u32 *Value	);
static	void
efuse_ShadowWrite1Byte(struct net_device* dev,	u16 Offset, u8 Value);
static	void
efuse_ShadowWrite2Byte(struct net_device* dev,	u16 Offset,u16 Value);
static	void
efuse_ShadowWrite4Byte(struct net_device* dev,	u16 Offset,u32 Value);

static	u8
efuse_OneByteRead(struct net_device* dev,u16 addr,u8 *data);
static	u8
efuse_OneByteWrite(struct net_device* dev,u16 addr, u8 data);

static	void 
efuse_ReadAllMap(struct net_device* dev,u8 *Efuse);
#ifdef TO_DO_LIST
static	void
efuse_WriteAllMap(struct net_device* dev,u8 *eeprom,u32 eeprom_size);
static	bool	
efuse_ParsingMap(char* szStr,u32* pu4bVal,u32* pu4bMove);
#endif
static	u8
efuse_PgPacketRead(	struct net_device* dev,u8	offset,u8 *data);
static	u8 
efuse_PgPacketWrite(struct net_device* dev,u8 offset,u8 word_en,u8	*data);
static	void
efuse_WordEnableDataRead(	u8 word_en,u8 *sourdata,u8 *targetdata);
static	u8
efuse_WordEnableDataWrite(	struct net_device* dev, u16 efuse_addr, u8 word_en, u8 *data);
static	void
efuse_PowerSwitch(struct net_device* dev,u8 PwrState);
static	u16
efuse_GetCurrentSize(struct net_device* dev);
static u8
efuse_CalculateWordCnts(u8 word_en);
#if 0
static	void
efuse_ResetLoader(struct net_device* dev);
#endif
#ifdef TO_DO_LIST
static void efuse_reg_ctrl(struct net_device* dev, u8 bPowerOn);
#endif
/*--------------------Define function prototype-----------------------*/



extern	void	
EFUSE_Initialize(struct net_device* dev)
{
	u8	Bytetemp = {0x00};
	u8	temp = {0x00};

	Bytetemp = read_nic_byte(dev, SYS_FUNC_EN+1);
	temp = Bytetemp | 0x20;
	write_nic_byte(dev, SYS_FUNC_EN+1, temp);

	Bytetemp = read_nic_byte(dev, SYS_ISO_CTRL+1);
	temp = Bytetemp & 0xFE;
	write_nic_byte(dev, SYS_ISO_CTRL+1, temp);

	
	Bytetemp = read_nic_byte(dev, EFUSE_TEST+3);
	temp = Bytetemp | 0x80;
	write_nic_byte(dev, EFUSE_TEST+3, temp);

	write_nic_byte(dev, 0x2F8, 0x3);
	
	write_nic_byte(dev, EFUSE_CTRL+3, 0x72);
	
}	/* EFUSE_Initialize */


extern	u8	
EFUSE_Read1Byte(struct net_device* dev, u16	Address)
{
	u8	data;
	u8	Bytetemp = {0x00};
	u8	temp = {0x00};
	u32	k=0;

	if (Address < EFUSE_REAL_CONTENT_LEN)	
	{
		temp = Address & 0xFF;	
		write_nic_byte(dev, EFUSE_CTRL+1, temp);	
		Bytetemp = read_nic_byte(dev, EFUSE_CTRL+2);	
		temp = ((Address >> 8) & 0x03) | (Bytetemp & 0xFC);	
		write_nic_byte(dev, EFUSE_CTRL+2, temp);	

		Bytetemp = read_nic_byte(dev, EFUSE_CTRL+3);
		temp = Bytetemp & 0x7F;
		write_nic_byte(dev, EFUSE_CTRL+3, temp);

		Bytetemp = read_nic_byte(dev, EFUSE_CTRL+3);
		while(!(Bytetemp & 0x80))
		{				
			Bytetemp = read_nic_byte(dev, EFUSE_CTRL+3);
			k++;
			if(k==1000)
			{
				k=0;
				break;
			}
		}
		data=read_nic_byte(dev, EFUSE_CTRL);
		return data;
	}
	else
		return 0xFF;
	
}	/* EFUSE_Read1Byte */


extern	void	
EFUSE_Write1Byte(struct net_device* dev, u16 Address,u8 Value)
{
	u8	Bytetemp = {0x00};
	u8	temp = {0x00};
	u32	k=0;

	
	if( Address < EFUSE_REAL_CONTENT_LEN)	
	{
		write_nic_byte(dev, EFUSE_CTRL, Value);

		temp = Address & 0xFF;	
		write_nic_byte(dev, EFUSE_CTRL+1, temp);	
		Bytetemp = read_nic_byte(dev, EFUSE_CTRL+2);	
		
		temp = ((Address >> 8) & 0x03) | (Bytetemp & 0xFC);	
		write_nic_byte(dev, EFUSE_CTRL+2, temp);	

		Bytetemp = read_nic_byte(dev, EFUSE_CTRL+3);
		temp = Bytetemp | 0x80;
		write_nic_byte(dev, EFUSE_CTRL+3, temp);

		Bytetemp = read_nic_byte(dev, EFUSE_CTRL+3);
		while(Bytetemp & 0x80)
		{
			Bytetemp = read_nic_byte(dev, EFUSE_CTRL+3);			
			k++;
			if(k==100)
			{
				k=0;
				break;
			}
		}
	}
	
}	/* EFUSE_Write1Byte */


#ifdef EFUSE_FOR_92SU
void do_93c46(struct net_device* dev,  u8 addorvalue)
{
	u8  cs[1] = {0x88};        
	u8  cssk[1] = {0x8c};      
	u8  csdi[1] = {0x8a};      
    	u8  csskdi[1] = {0x8e};    
    	u8  count;
	
    	for(count=0 ; count<8 ; count++)
	{
		if((addorvalue&0x80)!=0)
		{
			write_nic_byte(dev, EPROM_CMD, csdi[0]);  		
			write_nic_byte(dev, EPROM_CMD, csskdi[0]);  		
		}
		else
		{
			write_nic_byte(dev, EPROM_CMD, cs[0]);  		
			write_nic_byte(dev, EPROM_CMD, cssk[0]);  		
		}
		addorvalue = addorvalue << 1;
	}
}


u16 Read93C46(struct net_device*	dev,	u16	Reg	)
{

   	u8  	clear[1] = {0x0};      
	u8  	cs[1] = {0x88};        
	u8  	cssk[1] = {0x8c};      
	u8  	csdi[1] = {0x8a};      
   	u8  	csskdi[1] = {0x8e};    
	u8  	EepromSEL[1]={0x00};
	u8  	address;
	
	u16   	storedataF[1] = {0x0};   
	u8   	t,data[1],storedata[1];
	

	address = (u8)Reg;
	
	*EepromSEL= read_nic_byte(dev, EPROM_CMD);

	if((*EepromSEL & 0x10) == 0x10) 
	{
		address = address | 0x80;

		write_nic_byte(dev, EPROM_CMD, csdi[0]);  		
		write_nic_byte(dev, EPROM_CMD, csskdi[0]);  		
   		do_93c46(dev, address);
	}


	for(t=0 ; t<16 ; t++)      
	{
		write_nic_byte(dev, EPROM_CMD, cs[0]);  		
		write_nic_byte(dev, EPROM_CMD, cssk[0]); 
		*data= read_nic_byte(dev, EPROM_CMD);		
		
		if(*data & 0x8d) 
		{
			*data = *data & 0x01;
			*storedata = *data;
		}
		else 
		{
			*data = *data & 0x01 ; 
			*storedata = *data;   
		} 
		*storedataF = (*storedataF << 1 ) + *storedata;
	}
	write_nic_byte(dev, EPROM_CMD, cs[0]);     
	write_nic_byte(dev, EPROM_CMD, clear[0]);     

	return *storedataF;
}


void
ReadEFuseByte(struct net_device* dev,u16 _offset, u8 *pbuf) 
{

	u32  value32;
	u8 	readbyte;
	u16 	retry;
	

	write_nic_byte(dev, EFUSE_CTRL+1, (_offset & 0xff));  		
	readbyte = read_nic_byte(dev, EFUSE_CTRL+2);
	write_nic_byte(dev, EFUSE_CTRL+2, ((_offset >> 8) & 0x03) | (readbyte & 0xfc));  		

	readbyte = read_nic_byte(dev, EFUSE_CTRL+3);		
	write_nic_byte(dev, EFUSE_CTRL+3, (readbyte & 0x7f));  	
	
	retry = 0;
	value32 = read_nic_dword(dev, EFUSE_CTRL);
	while(!(((value32 >> 24) & 0xff) & 0x80)  && (retry<10000))
	{
		value32 = read_nic_dword(dev, EFUSE_CTRL);
		retry++;
	}	
	*pbuf = (u8)(value32 & 0xff);
}


#define		EFUSE_READ_SWITCH		1
void
ReadEFuse(struct net_device* dev, u16	 _offset, u16 _size_byte, u8 *pbuf)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8  	efuseTbl[EFUSE_MAP_LEN];
	u8  	rtemp8[1];
	u16 	eFuse_Addr = 0;
	u8  	offset, wren;
	u16  	i, j;
	u16 	eFuseWord[EFUSE_MAX_SECTION][EFUSE_MAX_WORD_UNIT];
	u16	efuse_utilized = 0;
	u16	efuse_usage = 0;
#ifdef EFUSE_REPG_WORKAROUND
	u16	sec1_addr = 0, sec5_addr = 0;
#endif

	if((_offset + _size_byte)>EFUSE_MAP_LEN)
	{
		printk("ReadEFuse(): Invalid offset with read bytes!!\n");
		return;
	}

	for (i = 0; i < EFUSE_MAX_SECTION; i++)
		for (j = 0; j < EFUSE_MAX_WORD_UNIT; j++)
			eFuseWord[i][j]=0xFFFF;
	
	for (i = 0; i < EFUSE_MAP_LEN; i++)
		efuseTbl[i] = 0xFF;

	ReadEFuseByte(dev, eFuse_Addr, rtemp8);	
	if(*rtemp8 != 0xFF)		
	{
		efuse_utilized++;
		eFuse_Addr++;
	}
	while((*rtemp8 != 0xFF) && (eFuse_Addr < EFUSE_REAL_CONTENT_LEN))
	{
		offset = ((*rtemp8 >> 4) & 0x0f);
#ifdef EFUSE_REPG_WORKAROUND
		if (IS_HARDWARE_TYPE_8192SE(dev))
		{
			if (offset == 1)
			{
				sec1_addr = eFuse_Addr;	
			}
			else if (offset == 5)
			{
				sec5_addr = eFuse_Addr;
			}
		
			if (sec1_addr > sec5_addr)
			{
				priv->efuse_RePGSec1Flag = false;
			}
			else
			{
				priv->efuse_RePGSec1Flag = true;
		       }
		}
#endif
		if(offset < EFUSE_MAX_SECTION){
			wren = (*rtemp8 & 0x0f);
			for(i=0; i<4; i++){
				if(!(wren & 0x01)){
					ReadEFuseByte(dev, eFuse_Addr, rtemp8);	eFuse_Addr++;
					efuse_utilized++;
					eFuseWord[offset][i] = (*rtemp8 & 0xff);
					if(eFuse_Addr >= EFUSE_REAL_CONTENT_LEN) break;
					ReadEFuseByte(dev, eFuse_Addr, rtemp8);	eFuse_Addr++;
					efuse_utilized++;
					eFuseWord[offset][i] |= (((u16)*rtemp8 << 8) & 0xff00);
					if(eFuse_Addr >= EFUSE_REAL_CONTENT_LEN) break;
				}
				wren >>= 1;
			}
		}
		ReadEFuseByte(dev, eFuse_Addr, rtemp8);	
		if(*rtemp8 != 0xFF && (eFuse_Addr < 512))	
		{
			efuse_utilized++;
			eFuse_Addr++;
		}
	}

	for(i=0; i<EFUSE_MAX_SECTION; i++){
		for(j=0; j<EFUSE_MAX_WORD_UNIT; j++){
			efuseTbl[(i*8)+(j*2)]=(eFuseWord[i][j] & 0xff);
			efuseTbl[(i*8)+((j*2)+1)]=((eFuseWord[i][j] >> 8) & 0xff);
#ifdef FUSE_REPG_WORKAROUND
			if ((i == 1) && (priv->efuse_RePGSec1Flag == true))
			{
				priv->efuse_RePGData[j*2]=(eFuseWord[i][j] & 0xff);
				priv->efuse_RePGData[(j*2)+1]=((eFuseWord[i][j] >> 8) & 0xff);
			}
#endif
		}
	}
	for(i=0; i<_size_byte; i++)
		pbuf[i] = efuseTbl[_offset+i];
	priv->EfuseUsedBytes = efuse_utilized;
	efuse_usage = (u8)((efuse_utilized*100)/EFUSE_REAL_CONTENT_LEN);
	priv->EfuseUsedPercentage = efuse_usage;	
}
#endif	

extern bool EFUSE_ShadowUpdateChk(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8   SectionIdx, i, Base;
	u16  WordsNeed = 0, HdrNum = 0, TotalBytes = 0, EfuseUsed = 0;
	bool bWordChanged, bResult = true;

	for (SectionIdx = 0; SectionIdx < 16; SectionIdx++) {
		Base = SectionIdx * 8;
		bWordChanged = false;

		for (i = 0; i < 8; i=i+2) {
			if((priv->EfuseMap[EFUSE_INIT_MAP][Base+i] !=
						priv->EfuseMap[EFUSE_MODIFY_MAP][Base+i]) ||
					(priv->EfuseMap[EFUSE_INIT_MAP][Base+i+1] !=
					 priv->EfuseMap[EFUSE_MODIFY_MAP][Base+i+1])) {
				WordsNeed++;
				bWordChanged = true;
			}
		}

		if( bWordChanged == true )
			HdrNum++;
	}

	TotalBytes = HdrNum + WordsNeed * 2;
	EfuseUsed = priv->EfuseUsedBytes;

	if ((TotalBytes + EfuseUsed) >= (EFUSE_MAX_SIZE - EFUSE_OOB_PROTECT_BYTES))
		bResult = false;

	return bResult;
}

extern void
EFUSE_ShadowRead(	struct net_device*	dev,	u8 Type, u16 Offset, u32 *Value)
{

	if (Type == 1)
		efuse_ShadowRead1Byte(dev, Offset, (u8 *)Value);
	else if (Type == 2)
		efuse_ShadowRead2Byte(dev, Offset, (u16 *)Value);
	else if (Type == 4)
		efuse_ShadowRead4Byte(dev, Offset, (u32 *)Value);
	
}	


extern	void
EFUSE_ShadowWrite(	struct net_device*	dev,	u8 Type, u16 Offset,u32	Value)
{

	if (Offset >= 0x18 && Offset <= 0x1F)
		return;

	if (Type == 1)
		efuse_ShadowWrite1Byte(dev, Offset, (u8)Value);
	else if (Type == 2)
		efuse_ShadowWrite2Byte(dev, Offset, (u16)Value);
	else if (Type == 4)
		efuse_ShadowWrite4Byte(dev, Offset, (u32)Value);
	
}	


extern bool EFUSE_ShadowUpdate(struct net_device* dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u16  i, offset, base = 0;
	u8   word_en = 0x0F;
	bool first_pg = false;

	if (!EFUSE_ShadowUpdateChk(dev)) {
		efuse_ReadAllMap(dev, &priv->EfuseMap[EFUSE_INIT_MAP][0]);
		memcpy(&priv->EfuseMap[EFUSE_MODIFY_MAP][0],
				(void *)&priv->EfuseMap[EFUSE_INIT_MAP][0], HWSET_MAX_SIZE_92S);

		printk("<---EFUSE_ShadowUpdate(): Efuse out of capacity!!\n");
		return false;
	}

	efuse_PowerSwitch(dev, true);

	for (offset = 0; offset < 16; offset++)
	{
		if(IS_HARDWARE_TYPE_8192SE(dev)){
			if (offset == 3)
				continue;
		}
		word_en = 0x0F;		
		base = offset * 8;

		for (i = 0; i < 8; i++)
		{
			if (offset == 0 && priv->EfuseMap[EFUSE_INIT_MAP][base+i] == 0xFF)
			{
				first_pg = true;				
			}

			if (first_pg == true && offset == 1 && IS_HARDWARE_TYPE_8192SE(dev))
			{
				continue;
			}

			if (first_pg == true)
			{
				word_en &= ~(BIT(i/2));	
				priv->EfuseMap[EFUSE_INIT_MAP][base+i] = 
				priv->EfuseMap[EFUSE_MODIFY_MAP][base+i];
			}else
			{
				if (	priv->EfuseMap[EFUSE_INIT_MAP][base+i] != 
					priv->EfuseMap[EFUSE_MODIFY_MAP][base+i])
				{
					word_en &= ~(EFUSE_BIT(i/2));

					priv->EfuseMap[EFUSE_INIT_MAP][base+i] = 
					priv->EfuseMap[EFUSE_MODIFY_MAP][base+i];
				}
			}
		}
		
		if (word_en != 0x0F)
		{
			u8	tmpdata[8];

			memcpy(tmpdata, &(priv->EfuseMap[EFUSE_MODIFY_MAP][base]), 8);
			efuse_PgPacketWrite(dev,(u8)offset,word_en,tmpdata);
		}
		
	}
	if (first_pg == true && IS_HARDWARE_TYPE_8192SE(dev))
	{
		u8	tmpdata[8];

		memcpy(tmpdata, (&priv->EfuseMap[EFUSE_MODIFY_MAP][8]), 8);
		efuse_PgPacketWrite(dev, 1, 0x0, tmpdata);		
#if 0
		u1Byte	tmpdata[8] = {0xFF, 0xFF, 0xEC, 0x10, 0xFF, 0xFF, 0xFF, 0xFF};
		
		efuse_PgPacketWrite(pAdapter, 1, 0xD, tmpdata);
#endif		
	}


	efuse_PowerSwitch(dev, false);
	efuse_ReadAllMap(dev, &priv->EfuseMap[EFUSE_INIT_MAP][0]);
	memcpy(&priv->EfuseMap[EFUSE_MODIFY_MAP][0],
		(void *)&priv->EfuseMap[EFUSE_INIT_MAP][0], HWSET_MAX_SIZE_92S);
		
	return true;	
}	


extern void EFUSE_ShadowMapUpdate(struct net_device* dev)
{	
	struct r8192_priv *priv = rtllib_priv(dev);
	
	if (priv->AutoloadFailFlag == true){
		memset(&(priv->EfuseMap[EFUSE_INIT_MAP][0]), 0xff, 128);
	}else{	
		efuse_ReadAllMap(dev, &priv->EfuseMap[EFUSE_INIT_MAP][0]);
	}
	memcpy(&priv->EfuseMap[EFUSE_MODIFY_MAP][0], 
		&priv->EfuseMap[EFUSE_INIT_MAP][0], HWSET_MAX_SIZE_92S);
	
}	


extern	void 
EFUSE_ForceWriteVendorId( struct net_device* dev)
{
	u8 tmpdata[8] = {0xFF, 0xFF, 0xEC, 0x10, 0xFF, 0xFF, 0xFF, 0xFF};

	efuse_PowerSwitch(dev, true);
		
	efuse_PgPacketWrite(dev, 1, 0xD, tmpdata);

	efuse_PowerSwitch(dev, false);
	
}	

extern	void 
EFUSE_RePgSection1(struct net_device* dev)
{
#ifdef EFUSE_REPG_WORKAROUND

	struct r8192_priv *priv = rtllib_priv(dev);
	
	if (priv->efuse_RePGSec1Flag == true)
	{
		efuse_PowerSwitch(dev, true);		
		RT_TRACE(COMP_INIT, "EFUSE REPG: %p, %u\n", priv->efuse_RePGData, 8);
				
		efuse_PgPacketWrite(dev, 1, 0x0d, priv->efuse_RePGData);	
		efuse_PowerSwitch(dev, false);
		priv->efuse_RePGSec1Flag = false;
	}
#endif	
}	


static	void
efuse_ShadowRead1Byte(struct net_device*	dev,	u16 Offset,	u8 *Value)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	*Value = priv->EfuseMap[EFUSE_MODIFY_MAP][Offset];
	
}	

static	void
efuse_ShadowRead2Byte(struct net_device*	dev,	u16 Offset,	u16 *Value)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	*Value = priv->EfuseMap[EFUSE_MODIFY_MAP][Offset];
	*Value |= priv->EfuseMap[EFUSE_MODIFY_MAP][Offset+1]<<8;
	
}	

static	void
efuse_ShadowRead4Byte(struct net_device*	dev,	u16 Offset,	u32 *Value)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	*Value = priv->EfuseMap[EFUSE_MODIFY_MAP][Offset];
	*Value |= priv->EfuseMap[EFUSE_MODIFY_MAP][Offset+1]<<8;
	*Value |= priv->EfuseMap[EFUSE_MODIFY_MAP][Offset+2]<<16;
	*Value |= priv->EfuseMap[EFUSE_MODIFY_MAP][Offset+3]<<24;
	
}	



static	void
efuse_ShadowWrite1Byte(struct net_device*	dev,	u16 Offset,	u8 Value)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	priv->EfuseMap[EFUSE_MODIFY_MAP][Offset] = Value;
	
}	

static	void
efuse_ShadowWrite2Byte(struct net_device*	dev,	u16 Offset,	u16 Value)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	priv->EfuseMap[EFUSE_MODIFY_MAP][Offset] = Value&0x00FF;
	priv->EfuseMap[EFUSE_MODIFY_MAP][Offset+1] = Value>>8;
	
}	

static	void
efuse_ShadowWrite4Byte(struct net_device*	dev,	u16 Offset,	u32 Value)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	priv->EfuseMap[EFUSE_MODIFY_MAP][Offset] = (u8)(Value&0x000000FF);
	priv->EfuseMap[EFUSE_MODIFY_MAP][Offset+1] = (u8)((Value>>8)&0x0000FF);
	priv->EfuseMap[EFUSE_MODIFY_MAP][Offset+2] = (u8)((Value>>16)&0x00FF);
	priv->EfuseMap[EFUSE_MODIFY_MAP][Offset+3] = (u8)((Value>>24)&0xFF);
	
}	


static	u8
efuse_OneByteRead(struct net_device* dev, u16 addr,u8 *data)
{
	u8 tmpidx = 0;
	u8 bResult;
	
	write_nic_byte(dev, EFUSE_CTRL+1, (u8)(addr&0xff));		
	write_nic_byte(dev, EFUSE_CTRL+2, ((u8)((addr>>8) &0x03) ) | 
	(read_nic_byte(dev, EFUSE_CTRL+2)&0xFC ));	

	write_nic_byte(dev, EFUSE_CTRL+3,  0x72);

	while(!(0x80 &read_nic_byte(dev, EFUSE_CTRL+3))&&(tmpidx<100))
	{
		tmpidx++;
	}
	if(tmpidx<100)
	{			
		*data=read_nic_byte(dev, EFUSE_CTRL);		
		bResult = true;
	}
	else
	{
		*data = 0xff;	
		bResult = false;
	}
	return bResult;
}	
		
static	u8
efuse_OneByteWrite(struct net_device* dev,  u16 addr, u8 data)
{
	u8 tmpidx = 0;
	u8 bResult;
	


	write_nic_byte(dev, EFUSE_CTRL+1, (u8)(addr&0xff));		
	write_nic_byte(dev, EFUSE_CTRL+2, 
	read_nic_byte(dev, EFUSE_CTRL+2)|(u8)((addr>>8)&0x03) );

	write_nic_byte(dev, EFUSE_CTRL, data);
	write_nic_byte(dev, EFUSE_CTRL+3, 0xF2);
		
	while((0x80 &  read_nic_byte(dev, EFUSE_CTRL+3)) && (tmpidx<100) ){
		tmpidx++;
	}
	
	if(tmpidx<100)
	{					
		bResult = true;
	}
	else
	{			
		bResult = false;
	}		
	
	return bResult;	
}	


static	void 
efuse_ReadAllMap(struct net_device*	dev, u8	*Efuse)
{	
		
	efuse_PowerSwitch(dev, true);
	ReadEFuse(dev, 0, 128, Efuse);
	efuse_PowerSwitch(dev, false);
#if 0
	RT_TRACE(COMP_INIT, "efuse_ResetLoader\n");
	efuse_ResetLoader(dev);

	write_nic_byte(dev, EFUSE_CLK, 0x03);
	
	ReadEFuse(dev, 0, 128, Efuse);

	write_nic_byte(dev, EFUSE_CLK, 0x02);
#if 0	
	for(offset = 0;offset<16;offset++)	
	{		
		PlatformFillMemory((PVOID)pg_data, 8, 0xff);
		efuse_PgPacketRead(pAdapter,offset,pg_data);

		PlatformMoveMemory((PVOID)&Efuse[offset*8], (PVOID)pg_data, 8);
	}
#endif

	if (Efuse[0] != 0x29 || Efuse[1] != 0x81)
	{
		if (index ++ < 5)
		{
			RT_TRACE(COMP_INIT, "EFUSE R FAIL %d\n", index);
			efuse_ReadAllMap(dev, Efuse);
		}
	}
	else
	{
		index = 0;
	}

#endif	
}	


#ifdef TO_DO_LIST
static	void
efuse_WriteAllMap(struct net_device* dev,u8 *eeprom, u32 eeprom_size)
{	
	unsigned char word_en = 0x00;

	unsigned char tmpdata[8];
	unsigned char offset;

	efuse_PowerSwitch(dev, true);

	for(offset=0 ; offset< eeprom_size/PGPKT_DATA_SIZE ; offset++)
	{
		if (IS_HARDWARE_TYPE_8192SE(dev))
		{
			if (offset == 3/* || offset > 9*/)	
				continue;
			else
				word_en = 0x00;
		}
		
		memcpy(tmpdata, (eeprom+(offset*PGPKT_DATA_SIZE)), 8);

	
		efuse_PgPacketWrite(dev,offset,word_en,tmpdata);

		
	}

	efuse_PowerSwitch(dev, false);
	
}	
#endif
static	u8
efuse_PgPacketRead(	struct net_device*	dev,	u8 offset, u8	*data)
{	
	u8 ReadState = PG_STATE_HEADER;	
	
	bool bContinual = true;
	bool  bDataEmpty = true ;	

	u8 efuse_data,word_cnts=0;
	u16 efuse_addr = 0;
	u8 hoffset=0,hworden=0;	
	u8 tmpidx=0;
	u8 tmpdata[8];
	
	if(data==NULL)	return false;
	if(offset>15)		return false;	
	
	memset(data, 0xff, sizeof(u8)*PGPKT_DATA_SIZE);
	memset(tmpdata, 0xff, sizeof(u8)*PGPKT_DATA_SIZE);
	
	
	while(bContinual && (efuse_addr  < EFUSE_MAX_SIZE) )
	{			
		if(ReadState & PG_STATE_HEADER)
		{
			if(efuse_OneByteRead(dev, efuse_addr ,&efuse_data)&&(efuse_data!=0xFF)){				
				hoffset = (efuse_data>>4) & 0x0F;
				hworden =  efuse_data & 0x0F;									
				word_cnts = efuse_CalculateWordCnts(hworden);
				bDataEmpty = true ;

				if(hoffset==offset){
					for(tmpidx = 0;tmpidx< word_cnts*2 ;tmpidx++){
						if(efuse_OneByteRead(dev, efuse_addr+1+tmpidx ,&efuse_data) ){
							tmpdata[tmpidx] = efuse_data;
							if(efuse_data!=0xff){						
								bDataEmpty = false;
							}
						}					
					}
					if(bDataEmpty==false){
						ReadState = PG_STATE_DATA;							
					}else{
						efuse_addr = efuse_addr + (word_cnts*2)+1;
						ReadState = PG_STATE_HEADER; 
					}
				}
				else{
					efuse_addr = efuse_addr + (word_cnts*2)+1;
					ReadState = PG_STATE_HEADER; 
				}
				
			}
			else{
				bContinual = false ;
			}
		}		
		else if(ReadState & PG_STATE_DATA)
		{
			efuse_WordEnableDataRead(hworden,tmpdata,data);
			efuse_addr = efuse_addr + (word_cnts*2)+1;
			ReadState = PG_STATE_HEADER; 
		}
		
	}			
	
	
	if(	(data[0]==0xff) &&(data[1]==0xff) && (data[2]==0xff)  && (data[3]==0xff) &&
		(data[4]==0xff) &&(data[5]==0xff) && (data[6]==0xff)  && (data[7]==0xff))
		return false;
	else
		return true;

}	


static u8 efuse_PgPacketWrite(struct net_device* dev, u8 offset, u8 word_en,u8 *data)
{
	u8 WriteState = PG_STATE_HEADER;		
	bool bContinual = true,bDataEmpty=true, bResult = true;
	u16 efuse_addr = 0;
	u8 efuse_data;

	u8 pg_header = 0;

	u8 tmp_word_cnts=0,target_word_cnts=0;
	u8 tmp_header,match_word_en,tmp_word_en;

	PGPKT_STRUCT target_pkt;	
	PGPKT_STRUCT tmp_pkt;

	u8 originaldata[sizeof(u8)*8];
	u8 tmpindex = 0,badworden = 0x0F;

	static u32 repeat_times = 0;

	if (efuse_GetCurrentSize(dev) >= EFUSE_MAX_SIZE - EFUSE_OOB_PROTECT_BYTES) {
		printk("efuse_PgPacketWrite error \n");
		return false;
	}

	target_pkt.offset = offset;
	target_pkt.word_en= word_en;

	memset(target_pkt.data,0xFF,sizeof(u8)*8);
	
	efuse_WordEnableDataRead(word_en,data,target_pkt.data);
	target_word_cnts = efuse_CalculateWordCnts(target_pkt.word_en);

	printk("EFUSE Power ON\n");

	while (bContinual && (efuse_addr  < (EFUSE_MAX_SIZE - EFUSE_OOB_PROTECT_BYTES)))
	{
		if(WriteState==PG_STATE_HEADER)
		{	
			bDataEmpty=true;
			badworden = 0x0F;		
			printk("EFUSE PG_STATE_HEADER\n");
			if (	efuse_OneByteRead(dev, efuse_addr ,&efuse_data) &&
				(efuse_data!=0xFF))
			{ 	
				tmp_header  =  efuse_data;
				
				tmp_pkt.offset 	= (tmp_header>>4) & 0x0F;
				tmp_pkt.word_en 	= tmp_header & 0x0F;					
				tmp_word_cnts =  efuse_CalculateWordCnts(tmp_pkt.word_en);

				if(tmp_pkt.offset  != target_pkt.offset)
				{
					efuse_addr = efuse_addr + (tmp_word_cnts*2) +1; 
					#if (EFUSE_ERROE_HANDLE == 1)
					WriteState = PG_STATE_HEADER;
					#endif
				}
				else
				{	
					for(tmpindex=0 ; tmpindex<(tmp_word_cnts*2) ; tmpindex++)
					{
						if(efuse_OneByteRead(dev, (efuse_addr+1+tmpindex) ,&efuse_data)&&(efuse_data != 0xFF)){
							bDataEmpty = false;	
						}
					}	
					if(bDataEmpty == false)
					{						
						efuse_addr = efuse_addr + (tmp_word_cnts*2) +1; 
						#if (EFUSE_ERROE_HANDLE == 1)
						WriteState=PG_STATE_HEADER;
						#endif
					}
					else
					{
						match_word_en = 0x0F;
						if(   !( (target_pkt.word_en&BIT0)|(tmp_pkt.word_en&BIT0)  ))
						{
							 match_word_en &= (~BIT0);
						}	
						if(   !( (target_pkt.word_en&BIT1)|(tmp_pkt.word_en&BIT1)  ))
						{
							 match_word_en &= (~BIT1);
						}
						if(   !( (target_pkt.word_en&BIT2)|(tmp_pkt.word_en&BIT2)  ))
						{
							 match_word_en &= (~BIT2);
						}
						if(   !( (target_pkt.word_en&BIT3)|(tmp_pkt.word_en&BIT3)  ))
						{
							 match_word_en &= (~BIT3);
						}					
												
						if((match_word_en&0x0F)!=0x0F)
						{							
							badworden = efuse_WordEnableDataWrite(dev,efuse_addr+1, tmp_pkt.word_en ,target_pkt.data);
							
							if(0x0F != (badworden&0x0F))
							{														
								u8 reorg_offset = offset;
								u8 reorg_worden=badworden;								
								efuse_PgPacketWrite(dev,reorg_offset,reorg_worden,originaldata);	
							}	

							tmp_word_en = 0x0F;	
							if(  (target_pkt.word_en&BIT0)^(match_word_en&BIT0)  )
							{
								tmp_word_en &= (~BIT0);
							}
							if(   (target_pkt.word_en&BIT1)^(match_word_en&BIT1) )
							{
								tmp_word_en &=  (~BIT1);
							}
							if(   (target_pkt.word_en&BIT2)^(match_word_en&BIT2) )
							{
								tmp_word_en &= (~BIT2);
							}						
							if(   (target_pkt.word_en&BIT3)^(match_word_en&BIT3) )
							{
								tmp_word_en &=(~BIT3);
							}							
						
							if((tmp_word_en&0x0F)!=0x0F){
								efuse_addr = efuse_GetCurrentSize(dev);
								target_pkt.offset = offset;
								target_pkt.word_en= tmp_word_en;					
							}else{								
								bContinual = false;
							}
							#if (EFUSE_ERROE_HANDLE == 1)
							WriteState=PG_STATE_HEADER;
							repeat_times++;
							if(repeat_times>EFUSE_REPEAT_THRESHOLD_){
								bContinual = false;
								bResult = false;
							}
							#endif
						}
						else{
							efuse_addr = efuse_addr + (2*tmp_word_cnts) +1;
							target_pkt.offset = offset;
							target_pkt.word_en= target_pkt.word_en;					
							#if (EFUSE_ERROE_HANDLE == 1)
							WriteState=PG_STATE_HEADER;
							#endif
						}		
					}				
				}				
				printk("EFUSE PG_STATE_HEADER-1\n");
			}
			else		
			{
				pg_header = ((target_pkt.offset << 4)&0xf0) |target_pkt.word_en;

				efuse_OneByteWrite(dev,efuse_addr, pg_header);
				efuse_OneByteRead(dev,efuse_addr, &tmp_header);		
		
				if(tmp_header == pg_header)
				{ 
					WriteState = PG_STATE_DATA;						
				}				
				#if (EFUSE_ERROE_HANDLE == 1)
				else if(tmp_header == 0xFF){
					WriteState = PG_STATE_HEADER;					
					repeat_times++;
					if(repeat_times>EFUSE_REPEAT_THRESHOLD_){
						bContinual = false;
						bResult = false;
					}
				}
				#endif
				else
				{
					tmp_pkt.offset = (tmp_header>>4) & 0x0F;
					tmp_pkt.word_en=  tmp_header & 0x0F;					
					tmp_word_cnts =  efuse_CalculateWordCnts(tmp_pkt.word_en);
																											
					memset(originaldata,0xff,sizeof(u8)*8);
					
					if(efuse_PgPacketRead( dev, tmp_pkt.offset,originaldata))
					{	
						badworden = efuse_WordEnableDataWrite(dev,efuse_addr+1,tmp_pkt.word_en,originaldata);	
						if(0x0F != (badworden&0x0F))
						{														
							u8 reorg_offset = tmp_pkt.offset;
							u8 reorg_worden=badworden;								
							efuse_PgPacketWrite(dev,reorg_offset,reorg_worden,originaldata);	
							efuse_addr = efuse_GetCurrentSize(dev);	 
						}
						else{
							efuse_addr = efuse_addr + (tmp_word_cnts*2) +1; 
						}
					}
					else
					{
						efuse_addr = efuse_addr + (tmp_word_cnts*2) +1; 
					}

					#if (EFUSE_ERROE_HANDLE == 1)
					WriteState=PG_STATE_HEADER;	
					repeat_times++;
					if(repeat_times>EFUSE_REPEAT_THRESHOLD_){
						bContinual = false;
						bResult = false;
					}
					#endif
					
					printk("EFUSE PG_STATE_HEADER-2\n");
				}

			}

		}
		else if(WriteState==PG_STATE_DATA) 
		{	
			printk("EFUSE PG_STATE_DATA\n");
			badworden = 0x0f;
			badworden = efuse_WordEnableDataWrite(dev,efuse_addr+1,target_pkt.word_en,target_pkt.data);	
			if((badworden&0x0F)==0x0F)
			{ 
				bContinual = false;
			}
			else
			{
				efuse_addr = efuse_addr + (2*target_word_cnts) +1;
										
				target_pkt.offset = offset;
				target_pkt.word_en= badworden;		
				target_word_cnts =  efuse_CalculateWordCnts(target_pkt.word_en); 
				#if (EFUSE_ERROE_HANDLE == 1)
				WriteState=PG_STATE_HEADER;	
				repeat_times++;
				if(repeat_times>EFUSE_REPEAT_THRESHOLD_){
					bContinual = false;
					bResult = false;
				}
				#endif
				printk("EFUSE PG_STATE_HEADER-3\n");
			}
		}
	}

	
	return true;
}	


static	void
efuse_WordEnableDataRead(	u8 word_en,u8 *sourdata,u8 *targetdata)
{	



	if (!(word_en&BIT0))
	{
		targetdata[0] = sourdata[0];
		targetdata[1] = sourdata[1];
	}
	if (!(word_en&BIT1))
	{
		targetdata[2] = sourdata[2];
		targetdata[3] = sourdata[3];
	}
	if (!(word_en&BIT2))
	{
		targetdata[4] = sourdata[4];
		targetdata[5] = sourdata[5];
	}
	if (!(word_en&BIT3))
	{
		targetdata[6] = sourdata[6];
		targetdata[7] = sourdata[7];
	}
}	


static	u8
efuse_WordEnableDataWrite(	struct net_device*	dev,	u16 efuse_addr, u8 word_en, u8 *data)
{		
	u16 tmpaddr = 0;
	u16 start_addr = efuse_addr;
	u8 badworden = 0x0F;
	u8 tmpdata[8]; 
	
	memset(tmpdata,0xff,PGPKT_DATA_SIZE);
	


	if(!(word_en&BIT0))
	{
		tmpaddr = start_addr;
		efuse_OneByteWrite(dev,start_addr++, data[0]);
		efuse_OneByteWrite(dev,start_addr++, data[1]);

		efuse_OneByteRead(dev,tmpaddr, &tmpdata[0]);
		efuse_OneByteRead(dev,tmpaddr+1, &tmpdata[1]);
		if((data[0]!=tmpdata[0])||(data[1]!=tmpdata[1])){
			badworden &= (~BIT0);
		}
	}
	if(!(word_en&BIT1))
	{
		tmpaddr = start_addr;
		efuse_OneByteWrite(dev,start_addr++, data[2]);
		efuse_OneByteWrite(dev,start_addr++, data[3]);
				
		efuse_OneByteRead(dev,tmpaddr    , &tmpdata[2]);
		efuse_OneByteRead(dev,tmpaddr+1, &tmpdata[3]);
		if((data[2]!=tmpdata[2])||(data[3]!=tmpdata[3])){
			badworden &=( ~BIT1);
		}
	}
	if(!(word_en&BIT2))
	{
		tmpaddr = start_addr;
		efuse_OneByteWrite(dev,start_addr++, data[4]);
		efuse_OneByteWrite(dev,start_addr++, data[5]);

		efuse_OneByteRead(dev,tmpaddr, &tmpdata[4]);							
		efuse_OneByteRead(dev,tmpaddr+1, &tmpdata[5]);
		if((data[4]!=tmpdata[4])||(data[5]!=tmpdata[5])){
			badworden &=( ~BIT2);
		}
	}
	if(!(word_en&BIT3))
	{
		tmpaddr = start_addr;
		efuse_OneByteWrite(dev,start_addr++, data[6]);
		efuse_OneByteWrite(dev,start_addr++, data[7]);

		efuse_OneByteRead(dev,tmpaddr, &tmpdata[6]);
		efuse_OneByteRead(dev,tmpaddr+1, &tmpdata[7]);
		if((data[6]!=tmpdata[6])||(data[7]!=tmpdata[7])){
			badworden &=( ~BIT3);
		}
	}
	return badworden;
}	


static	void
efuse_PowerSwitch(struct net_device* dev, u8 PwrState)
{
	u8	tempval;
	if (PwrState == true)
	{
		tempval = read_nic_byte(dev, EFUSE_TEST+3);
		write_nic_byte(dev, EFUSE_TEST+3, (tempval | 0x80));

		write_nic_byte(dev, EFUSE_CLK, 0x03);
	}
	else
	{
		tempval = read_nic_byte(dev, EFUSE_TEST+3);
		write_nic_byte(dev, EFUSE_TEST+3, (tempval & 0x7F));

		write_nic_byte(dev, EFUSE_CLK, 0x02);
	}	
	
}	/* efuse_PowerSwitch */


static	u16
efuse_GetCurrentSize(struct net_device*	dev)
{
	bool bContinual = true;

	u16 efuse_addr = 0;
	u8 hoffset=0,hworden=0;	
	u8 efuse_data,word_cnts=0;
		
	
	while (	bContinual && 
			efuse_OneByteRead(dev, efuse_addr ,&efuse_data) && 
			(efuse_addr  < EFUSE_MAX_SIZE) )
	{			
		if(efuse_data!=0xFF)
		{					
			hoffset = (efuse_data>>4) & 0x0F;
			hworden =  efuse_data & 0x0F;									
			word_cnts = efuse_CalculateWordCnts(hworden);
			efuse_addr = efuse_addr + (word_cnts*2)+1;						
		}
		else
		{
			bContinual = false ;			
		}
	}

	
	return efuse_addr;
	
}	


static u8
efuse_CalculateWordCnts(u8	word_en)
{
	u8 word_cnts = 0;
	if(!(word_en & BIT0))	word_cnts++; 
	if(!(word_en & BIT1))	word_cnts++;
	if(!(word_en & BIT2))	word_cnts++;
	if(!(word_en & BIT3))	word_cnts++;
	return word_cnts;
}	


#if 0
static void efuse_ResetLoader(struct net_device* dev)
{
	u16	tmpU2b;

	tmpU2b = read_nic_word(dev, SYS_FUNC_EN);
	write_nic_word(dev, SYS_FUNC_EN, (tmpU2b&~(BIT12)));
	mdelay(10);
	write_nic_word(dev, SYS_FUNC_EN, (tmpU2b|BIT12));
	mdelay(10);
	
}	
#endif

 #ifdef TO_DO_LIST
extern	bool	
EFUSE_ProgramMap(struct net_device* dev, char* pFileName,u8	TableType)		
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	s4Byte			nLinesRead, ithLine;
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
	char* 			szLine;
	u32			u4bRegValue, u4RegMask;
	u32			u4bMove;
	u16			index = 0;
	u16			i;
	u8			eeprom[HWSET_MAX_SIZE_92S];	
	
	rtStatus = PlatformReadFile(
					dev, 
					pFileName,
					(u8*)(priv->BufOfLines),
					MAX_LINES_HWCONFIG_TXT,
					MAX_BYTES_LINE_HWCONFIG_TXT,
					&nLinesRead
					);

	if(rtStatus == RT_STATUS_SUCCESS)
	{	
		memcp(pHalData->BufOfLines3, pHalData->BufOfLines,
			nLinesRead*MAX_BYTES_LINE_HWCONFIG_TXT);
		pHalData->nLinesRead3 = nLinesRead;
	}

	if(rtStatus == RT_STATUS_SUCCESS)
	{		
		printk("szEepromFile(): read %s ok\n", pFileName);
		for(ithLine = 0; ithLine < nLinesRead; ithLine++)
		{
			szLine = pHalData->BufOfLines[ithLine];
			printk("Line-%d String =%s\n", ithLine, szLine);
			
			if(!IsCommentString(szLine))
			{
				for (i = 0; i < 8; i++)
				{
					u32	j;

					efuse_ParsingMap(szLine, &u4bRegValue, &u4bMove);

					szLine += u4bMove;
					eeprom[index++] = (u8)(u4bRegValue&0xff);
					eeprom[index++] = (u8)((u4bRegValue>>8)&0xff);
					
					printk("Addr-%d = %x\n", (ithLine*8+i), u4bRegValue);
				}
			}
			
		}
	
	}
	else
	{
		printk("szEepromFile(): Fail read%s\n", pFileName);
		return	RT_STATUS_FAILURE;
	}



	if (TableType == 1)
	{
		efuse_WriteAllMap(dev, eeprom, HWSET_MAX_SIZE_92S);
	}
	else
	{
		for (i = 0; i < HWSET_MAX_SIZE_92S; i++)
			EFUSE_ShadowWrite(dev, 1, i, (u32)eeprom[i]);
	}
	
	return	rtStatus;
}	/* EFUSE_ProgramMap */

#endif

bool IsHexDigit(	char chTmp)
{
	if( (chTmp >= '0' && chTmp <= '9') ||  
		(chTmp >= 'a' && chTmp <= 'f') ||
		(chTmp >= 'A' && chTmp <= 'F') )
	{
		return true;
	}
	else
	{
		return false;
	}
}

u32 MapCharToHexDigit(char chTmp)
{
	if(chTmp >= '0' && chTmp <= '9')
		return (chTmp - '0');
	else if(chTmp >= 'a' && chTmp <= 'f')
		return (10 + (chTmp - 'a'));
	else if(chTmp >= 'A' && chTmp <= 'F') 
		return (10 + (chTmp - 'A'));
	else
		return 0;	
}

#ifdef TO_DO_LIST
static	bool	
efuse_ParsingMap(char* szStr,u32* pu4bVal,u32* pu4bMove)
{
	char* 		szScan = szStr;

	if(szStr == NULL || pu4bVal == NULL || pu4bMove == NULL)
	{
		return false;
	}

	*pu4bMove = 0;
	*pu4bVal = 0;

	while(	*szScan != '\0' && 
			(*szScan == ' ' || *szScan == '\t') )
	{
		szScan++;
		(*pu4bMove)++;
	}

	if(!IsHexDigit(*szScan))
	{
		return false;
	}

	do
	{
		(*pu4bVal) <<= 4;
		*pu4bVal += MapCharToHexDigit(*szScan);

		szScan++;
		(*pu4bMove)++;
	} while(IsHexDigit(*szScan));

	return true;
	
}	/* efuse_ParsingMap */
#endif

int efuse_one_byte_rw(struct net_device* dev, u8 bRead, u16 addr, u8 *data)
{
	u32 bResult;
	u8 tmpidx = 0;
	u8 tmpv8=0;
	

	write_nic_byte(dev, EFUSE_CTRL+1, (u8)(addr&0xff));		
	tmpv8 = ((u8)((addr>>8) &0x03) ) | (read_nic_byte(dev, EFUSE_CTRL+2)&0xFC );	
	write_nic_byte(dev, EFUSE_CTRL+2, tmpv8);

	if(true==bRead){
		
		write_nic_byte(dev, EFUSE_CTRL+3,  0x72);

		while(!(0x80 & read_nic_byte(dev, EFUSE_CTRL+3)) && (tmpidx<100) ){
			tmpidx++;
		}
		if(tmpidx<100){			
			*data=read_nic_byte(dev, EFUSE_CTRL);
			bResult = true;
		}
		else
		{
			*data = 0;
			bResult = false;
		}
		
	}
	else{
		write_nic_byte(dev, EFUSE_CTRL, *data);
		
		write_nic_byte(dev, EFUSE_CTRL+3, 0xF2);
		
		while((0x80 & read_nic_byte(dev, EFUSE_CTRL+3)) && (tmpidx<100) ){
			tmpidx++;
		}
		if(tmpidx<100)
		{			
			*data=read_nic_byte(dev, EFUSE_CTRL);
			bResult = true;
		}
		else
		{
			*data = 0;
			bResult = false;
		}
		
	}
	return bResult;	
}
void efuse_access(struct net_device* dev, u8 bRead,u16 start_addr, u8 cnts, u8 *data)
{
	u8	efuse_clk_ori,efuse_clk_new;
	u32 i = 0;

	if(start_addr>0x200) return;
	efuse_clk_ori = read_nic_byte(dev,SYS_FUNC_EN+1);
	efuse_clk_new = efuse_clk_ori|0x20;
	
	if(efuse_clk_new!= efuse_clk_ori){
		write_nic_byte(dev, SYS_FUNC_EN+1, efuse_clk_new);
	}
#ifdef _POWERON_DELAY_
	mdelay(10);
#endif	
	write_nic_byte(dev, EFUSE_TEST+3, (read_nic_byte(dev, EFUSE_TEST+3)|0x80));
	write_nic_byte(dev, EFUSE_CLK_CTRL, (read_nic_byte(dev, EFUSE_CLK_CTRL)|0x03));
	
#ifdef _PRE_EXECUTE_READ_CMD_
	{
		unsigned char tmpdata;
		efuse_OneByteRead(dev, 0,&tmpdata);
	}		
#endif	
	
	for(i=0;i<cnts;i++){		
		efuse_one_byte_rw(dev,bRead, start_addr+i , data+i);		
	}
	write_nic_byte(dev, EFUSE_TEST+3, read_nic_byte(dev, EFUSE_TEST+3)&0x7f);
	write_nic_byte(dev, EFUSE_CLK_CTRL, read_nic_byte(dev, EFUSE_CLK_CTRL)&0xfd);
			
	if(efuse_clk_new != efuse_clk_ori)	write_nic_byte(dev, 0x10250003, efuse_clk_ori);
	
}

#ifdef TO_DO_LIST
static	void efuse_reg_ctrl(struct net_device* dev, u8 bPowerOn)
{		
	if(true==bPowerOn){
		write_nic_byte(dev, SYS_FUNC_EN+1,  read_nic_byte(dev,SYS_FUNC_EN+1)|0x20);
#ifdef _POWERON_DELAY_	
		mdelay(10);	
#endif
		write_nic_byte(dev, EFUSE_TEST+4, (read_nic_byte(dev, EFUSE_TEST+4)|0x80));
		write_nic_byte(dev, EFUSE_CLK_CTRL, (read_nic_byte(dev, EFUSE_CLK_CTRL)|0x03));
#ifdef _PRE_EXECUTE_READ_CMD_
		{
			unsigned char tmpdata;
			efuse_OneByteRead(dev, 0,&tmpdata);
		}
		
#endif
	}
	else{
		write_nic_byte(dev, EFUSE_TEST+4, read_nic_byte(dev, EFUSE_TEST+4)&0x7f);
		write_nic_byte(dev, EFUSE_CLK_CTRL, read_nic_byte(dev, EFUSE_CLK_CTRL)&0xfd);

	}

	
}
#endif

void efuse_read_data(struct net_device* dev,u8 efuse_read_item,u8 *data,u32 data_size)
{
	u8 offset, word_start,byte_start,byte_cnts;
	u8	efusedata[EFUSE_REAL_CONTENT_LEN];
	u8 *tmpdata = NULL;

	u8 pg_pkt_cnts ;
	
	u8 tmpidx;
	u8 pg_data[8];

	if(efuse_read_item>  (sizeof(RTL8712_SDIO_EFUSE_TABLE)/sizeof(EFUSE_MAP))){
		return ;
	}
		
	offset		= RTL8712_SDIO_EFUSE_TABLE[efuse_read_item].offset ;
	word_start	= RTL8712_SDIO_EFUSE_TABLE[efuse_read_item].word_start;
	byte_start 	= RTL8712_SDIO_EFUSE_TABLE[efuse_read_item].byte_start;
	byte_cnts   	= RTL8712_SDIO_EFUSE_TABLE[efuse_read_item].byte_cnts;

	if(data_size!=byte_cnts){
		return;		
	}
	
	pg_pkt_cnts = (byte_cnts /PGPKT_DATA_SIZE) +1;		
	
	if(pg_pkt_cnts > 1){
		tmpdata = efusedata;

		if(tmpdata!=NULL)
		{
			memset(tmpdata,0xff,pg_pkt_cnts*PGPKT_DATA_SIZE);	
						
			for(tmpidx=0;tmpidx<pg_pkt_cnts;tmpidx++)
			{				
				memset(pg_data,0xff,PGPKT_DATA_SIZE);
				if(true== efuse_PgPacketRead(dev,offset+tmpidx,pg_data))
				{	
					memcpy(tmpdata+(PGPKT_DATA_SIZE*tmpidx),pg_data,PGPKT_DATA_SIZE);
				}
			}
			memcpy(data,(tmpdata+ (2*word_start)+byte_start ),data_size);					
		}		
	}
	else
	{
		memset(pg_data,0xff,PGPKT_DATA_SIZE);
		if(true==efuse_PgPacketRead(dev,offset,pg_data)){	
			memcpy(data,pg_data+ (2*word_start)+byte_start ,data_size);
		}
	}	
	
}
void efuse_write_data(struct net_device* dev,u8 efuse_write_item,u8 *data,u32 data_size,u32 bWordUnit)
{
	u8 offset, word_start,byte_start,byte_cnts;	
	u8 word_en = 0x0f,word_cnts;
	u8 pg_pkt_cnts ;
	
	u8 tmpidx,tmpbitmask;
	u8 pg_data[8],tmpbytes=0;

	if(efuse_write_item>  (sizeof(RTL8712_SDIO_EFUSE_TABLE)/sizeof(EFUSE_MAP))){
		return ;
	}
	
	offset		= RTL8712_SDIO_EFUSE_TABLE[efuse_write_item].offset ;
	word_start	= RTL8712_SDIO_EFUSE_TABLE[efuse_write_item].word_start;
	byte_start 	= RTL8712_SDIO_EFUSE_TABLE[efuse_write_item].byte_start;
	byte_cnts   	= RTL8712_SDIO_EFUSE_TABLE[efuse_write_item].byte_cnts;
	
	if(data_size >  byte_cnts){
		return;		
	}
	pg_pkt_cnts = (byte_cnts /PGPKT_DATA_SIZE) +1;	
	word_cnts = byte_cnts /2 ;

	if(byte_cnts %2){
		word_cnts+=1;
	}		
	if((byte_start==1)||((byte_cnts%2)==1)){

		if((efuse_write_item==EFUSE_F0CIS)||(efuse_write_item==EFUSE_F1CIS)){
			memset(pg_data,0xff,PGPKT_DATA_SIZE);
			efuse_PgPacketRead(dev,offset,pg_data);		
			
			if(efuse_write_item==EFUSE_F0CIS){
				word_en = 0x07;
				memcpy(pg_data+word_start*2+byte_start,data,sizeof(u8)*2);
				efuse_PgPacketWrite(dev,offset,word_en,pg_data+(word_start*2));	

				word_en = 0x00;
				efuse_PgPacketWrite(dev,(offset+1),word_en,data+2);

				word_en = 0x00;
				efuse_PgPacketRead(dev,offset+2,pg_data);
				memcpy(pg_data,data+2+8,sizeof(u8)*7);
				
				efuse_PgPacketWrite(dev,(offset+2),word_en,pg_data);
			}
			else if(efuse_write_item==EFUSE_F1CIS){
				word_en = 0x07;
				efuse_PgPacketRead(dev,offset,pg_data);
				pg_data[7] = data[0];
				efuse_PgPacketWrite(dev,offset,word_en,pg_data+(word_start*2));	
				
				word_en = 0x00;
				for(tmpidx = 0 ;tmpidx<(word_cnts/4);tmpidx++){
					efuse_PgPacketWrite(dev,(offset+1+tmpidx),word_en,data+1+(tmpidx*PGPKT_DATA_SIZE));
				}				
			}
			
		}
		else{			
			memset(pg_data,0xff,PGPKT_DATA_SIZE);
			if((efuse_write_item==EFUSE_SDIO_SETTING)||(efuse_write_item==EFUSE_CCCR)){
				word_en = 0x0e ;
				tmpbytes = 2;
			}
			else if(efuse_write_item == EFUSE_SDIO_MODE){
				word_en = 0x0d ;
				tmpbytes = 2;
			}
			else if(efuse_write_item == EFUSE_OCR){
				word_en = 0x09 ;
				tmpbytes = 4;
			}
			else if((efuse_write_item == EFUSE_EEPROM_VER)||(efuse_write_item==EFUSE_CHAN_PLAN)){
				word_en = 0x07 ;
				tmpbytes = 2;
			}
			if(bWordUnit==true){
				memcpy(pg_data+word_start*2 ,data,sizeof(u8)*tmpbytes);
			}
			else{																
				efuse_PgPacketRead(dev,offset,pg_data);
				memcpy(pg_data+(2*word_start)+byte_start,data,sizeof(u8)*byte_cnts);			
			}
					
			efuse_PgPacketWrite(dev,offset,word_en,pg_data+(word_start*2));	
			
		}		
		
	}
	else if(pg_pkt_cnts>1){
		if(word_start==0){
			word_en = 0x00;
			for(tmpidx = 0 ;tmpidx<(word_cnts/4);tmpidx++)
			{
				efuse_PgPacketWrite(dev,(offset+tmpidx),word_en,data+(tmpidx*PGPKT_DATA_SIZE));
			}
			word_en = 0x0f;
			for(tmpidx= 0; tmpidx<(word_cnts%4) ; tmpidx++)
			{
				tmpbitmask =tmpidx;
				word_en &= (~(EFUSE_BIT(tmpbitmask)));
			}		
			efuse_PgPacketWrite(dev,offset+(word_cnts/4),word_en,data+((word_cnts/4)*PGPKT_DATA_SIZE));
		}else
		{

		}
	}
	else{
		word_en = 0x0f;
		for(tmpidx= 0; tmpidx<word_cnts ; tmpidx++)
		{			
			tmpbitmask = word_start + tmpidx ;
			word_en &= (~(EFUSE_BIT(tmpbitmask)));
		}
		efuse_PgPacketWrite(dev,offset,word_en,data);
	}
		
}

void efuset_test_func_read(struct net_device* dev)
{
	u8 chipid[2];
	u8 ocr[3];
	u8 macaddr[6];	
	u8 txpowertable[28];
	
	memset(chipid,0,sizeof(u8)*2);	
	efuse_read_data(dev,EFUSE_CHIP_ID,chipid,sizeof(chipid));
	
	memset(ocr,0,sizeof(u8)*3);	
	efuse_read_data(dev,EFUSE_CCCR,ocr,sizeof(ocr));

	memset(macaddr,0,sizeof(u8)*6);
	efuse_read_data(dev,EFUSE_MAC_ADDR,macaddr,sizeof(macaddr));
	
	memset(txpowertable,0,sizeof(u8)*28);
	efuse_read_data(dev,EFUSE_TXPW_TAB,txpowertable,sizeof(txpowertable));
}

void efuset_test_func_write(struct net_device* dev)
{
	u32 bWordUnit = true;	
	u8 CCCR=0x02,SDIO_SETTING = 0xFF;
	u8 tmpdata[2];
	
	u8 macaddr[6] = {0x00,0xe0,0x4c,0x87,0x12,0x66};	
	efuse_write_data(dev,EFUSE_MAC_ADDR,macaddr,sizeof(macaddr),bWordUnit);

	bWordUnit = false;
	efuse_write_data(dev,EFUSE_CCCR,&CCCR,sizeof(u8),bWordUnit);
	
	bWordUnit = false;
	efuse_write_data(dev,EFUSE_SDIO_SETTING,&SDIO_SETTING,sizeof(u8),bWordUnit);

	bWordUnit = true;	
	tmpdata[0] =SDIO_SETTING ;
	tmpdata[1] =CCCR ;
	efuse_write_data(dev,EFUSE_SDIO_SETTING,tmpdata,sizeof(tmpdata),bWordUnit);
	
}
#endif	
