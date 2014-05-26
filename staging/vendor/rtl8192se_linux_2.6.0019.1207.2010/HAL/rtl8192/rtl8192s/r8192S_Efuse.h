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

#ifndef __INC_EFUSE_H
#define __INC_EFUSE_H

#define		EFUSE_FOR_92SU		1

/*--------------------------Define Parameters-------------------------------*/
#define		EFUSE_REAL_CONTENT_LEN			512
#define		EFUSE_MAP_LEN				128
#define		EFUSE_MAX_SECTION			16
#define		EFUSE_MAX_WORD_UNIT			4
#define 	       EFUSE_IC_ID_OFFSET			506 

#define		EFUSE_INIT_MAP				0
#define		EFUSE_MODIFY_MAP				1

#define		EFUSE_CLK_CTRL			EFUSE_CTRL
#define 	EFUSE_BIT(x)  (1 << (x))

#define		PG_STATE_HEADER 	0x01
#define		PG_STATE_WORD_0		0x02
#define		PG_STATE_WORD_1		0x04
#define		PG_STATE_WORD_2		0x08
#define		PG_STATE_WORD_3		0x10
#define		PG_STATE_DATA		0x20

#define		PG_SWBYTE_H			0x01
#define		PG_SWBYTE_L			0x02

/*--------------------------Define Parameters-------------------------------*/


/*------------------------------Define structure----------------------------*/ 

/*------------------------------Define structure----------------------------*/ 


/*------------------------Export global variable----------------------------*/
/*------------------------Export global variable----------------------------*/

/*------------------------Export Marco Definition---------------------------*/

/*------------------------Export Marco Definition---------------------------*/


/*--------------------------Exported Function prototype---------------------*/
extern	void	
EFUSE_Initialize(struct net_device* dev);
extern	u8	
EFUSE_Read1Byte(struct net_device* dev,u16 Address);
extern	void	
EFUSE_Write1Byte(struct net_device* dev,u16 Address,u8 Value);

#ifdef EFUSE_FOR_92SU 
extern 	void 
ReadEFuse(struct net_device* dev,u16 _offset,u16 _size_byte,u8* pbuf);
extern	void
ReadEFuseByte(struct net_device* dev,u16  _offset,u8  *pbuf);
#endif	

extern	void
EFUSE_ShadowRead(struct net_device* dev,unsigned char Type,unsigned short Offset,u32 *Value);
extern	void
EFUSE_ShadowWrite(struct net_device* dev,unsigned char Type,unsigned short Offset,u32 Value);
extern	bool
EFUSE_ShadowUpdate(struct net_device* dev);
extern	bool
EFUSE_ShadowUpdateChk(struct net_device* dev);
extern	void 
EFUSE_ShadowMapUpdate(struct net_device* dev);
extern	void 
EFUSE_RePgSection1(struct net_device* dev);

extern	bool	
EFUSE_ProgramMap(struct net_device* dev,char* pFileName, u8 TableType);		
/*--------------------------Exported Function prototype---------------------*/

/* End of Efuse.h */

#endif 
