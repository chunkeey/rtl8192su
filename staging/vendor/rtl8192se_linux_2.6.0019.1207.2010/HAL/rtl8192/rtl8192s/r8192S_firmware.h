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
#ifndef __INC_FIRMWARE_H
#define __INC_FIRMWARE_H

#include "r8192S_def.h"

#define 	RTL8190_MAX_FIRMWARE_CODE_SIZE	64000	
#define 	RTL8190_CPU_START_OFFSET			0x80
#define	MAX_FIRMWARE_CODE_SIZE	0xFF00  

#define		H2C_TX_CMD_HDR_LEN				8

#define		RTL8192S_FW_PKT_FRAG_SIZE		0x4000	

#ifdef RTL8192SE
#define GET_COMMAND_PACKET_FRAG_THRESHOLD(v)	4*(v/4) - 8
#else 
#define GET_COMMAND_PACKET_FRAG_THRESHOLD(v)	(4*(v/4) - 8 - USB_HWDESC_HEADER_LEN)
#endif

typedef enum _DESC_PACKET_TYPE{
	DESC_PACKET_TYPE_INIT = 0,
	DESC_PACKET_TYPE_NORMAL = 1,	
}DESC_PACKET_TYPE;


#ifdef RTL8192S
typedef enum _firmware_init_step{
	FW_INIT_STEP0_IMEM = 0,
	FW_INIT_STEP1_MAIN = 1,
	FW_INIT_STEP2_DATA = 2,
}firmware_init_step_e;
#else
typedef enum _firmware_init_step{
	FW_INIT_STEP0_BOOT = 0,
	FW_INIT_STEP1_MAIN = 1,
	FW_INIT_STEP2_DATA = 2,
}firmware_init_step_e;
#endif

typedef enum _firmware_source{
	FW_SOURCE_IMG_FILE = 0,
	FW_SOURCE_HEADER_FILE = 1,		
}firmware_source_e, *pfirmware_source_e;

typedef enum _opt_rst_type{
	OPT_SYSTEM_RESET = 0,
	OPT_FIRMWARE_RESET = 1,
}opt_rst_type_e;

typedef enum _FIRMWARE_STATUS{
	FW_STATUS_0_INIT = 0,
	FW_STATUS_1_MOVE_BOOT_CODE = 1,
	FW_STATUS_2_MOVE_MAIN_CODE = 2,
	FW_STATUS_3_TURNON_CPU = 3,
	FW_STATUS_4_MOVE_DATA_CODE = 4,
	FW_STATUS_5_READY = 5,
}FIRMWARE_STATUS;

typedef enum _H2C_CMD{
	FW_H2C_SETPWRMODE	= 0,
	FW_H2C_JOINBSSRPT	= 1,
	FW_H2C_WoWLAN_UPDATE_GTK = 2,
	FW_H2C_WoWLAN_UPDATE_IV = 3,
	FW_H2C_WoWLAN_OFFLOAD = 4,
	FW_H2C_SITESURVEY=5,
}H2C_CMD;

 typedef struct _H2C_SETPWRMODE_PARM {
 	u8	mode;
 	u8	flag_low_traffic_en; 
 	u8	flag_lpnav_en;
 	u8	flag_rf_low_snr_en;
 	u8	flag_dps_en; 
 	u8	bcn_rx_en;
 	u8	bcn_pass_cnt;
 	u8	bcn_to;  // beacon TO (ms). ¡§=0¡¨ no limit.
 	u16	bcn_itv;
 	u8	app_itv; 
 	u8	awake_bcn_itvl;
 	u8	smart_ps;
	u8	bcn_pass_period; 
 }H2C_SETPWRMODE_PARM, *PH2C_SETPWRMODE_PARM; 

typedef struct _H2C_JOINBSSRPT_PARM {	
	u8	OpMode;	
	u8	Ps_Qos_Info;
	u8	Bssid[6];	
	u16	BcnItv;	
	u16	Aid;
}H2C_JOINBSSRPT_PARM, *PH2C_JOINBSSRPT_PARM;

typedef struct _H2C_WPA_PTK {
 	u8 	kck[16]; /* EAPOL-Key Key Confirmation Key (KCK) */
 	u8 	kek[16]; /* EAPOL-Key Key Encryption Key (KEK) */
	u8	tk1[16]; /* Temporal Key 1 (TK1) */
	union {
		u8	tk2[16]; 
		struct {
			u8	tx_mic_key[8];
			u8	rx_mic_key[8];
		}Athu;
	}U;
}H2C_WPA_PTK;

typedef struct _H2C_WPA_TWO_WAY_PARA{
	u8			pairwise_en_alg;
	u8			group_en_alg;
	H2C_WPA_PTK		wpa_ptk_value;
}H2C_WPA_TWO_WAY_PARA, *PH2C_WPA_TWO_WAY_PARA;

typedef struct _tx_desc_8192se_fw{

        u32             PktSize:16;
        u32             Offset:8;
        u32             Type:2; 
        u32             LastSeg:1;
        u32             FirstSeg:1;
        u32             LINIP:1;
        u32             AMSDU:1;
        u32             GF:1;
        u32             OWN:1;

        u32             MacID:5;
        u32             MoreData:1;
        u32             MoreFrag:1;
        u32             PIFS:1;
        u32             QueueSel:5;
        u32             AckPolicy:2;
        u32             NoACM:1;
        u32             NonQos:1;
        u32             KeyID:2;
        u32             OUI:1;
        u32             PktType:1;
        u32             EnDescID:1;
        u32             SecType:2;
        u32             HTC:1;  
        u32             WDS:1;  
        u32             PktOffset:5;    
        u32             HWPC:1;

        u32             DataRetryLmt:6;
        u32             RetryLmtEn:1;
        u32             TSFL:5;
        u32             RTSRC:6;        
        u32             DATARC:6;       

        u32             Rsvd1:5;
        u32             AggEn:1;
        u32             BK:1;   
        u32             OwnMAC:1;

        u32             NextHeadPage:8;
        u32             TailPage:8;
        u32             Seq:12;
        u32             Frag:4;

        u32             RTSRate:6;
        u32             DisRTSFB:1;
        u32             RTSRateFBLmt:4;
        u32             CTS2Self:1;
        u32             RTSEn:1;
        u32             RaBRSRID:3;     
        u32             TXHT:1;
        u32             TxShort:1;
        u32             TxBw:1;
        u32             TXSC:2;
        u32             STBC:2;
        u32             RD:1;
        u32             RTSHT:1;
        u32             RTSShort:1;
        u32             RTSBW:1;
        u32             RTSSC:2;
        u32             RTSSTBC:2;
        u32             UserRate:1;

        u32             PktID:9;
        u32             TxRate:6;
        u32             DISFB:1;
        u32             DataRateFBLmt:5;
        u32             TxAGC:11;

        u32             IPChkSum:16;
        u32             TCPChkSum:16;

        u32             TxBufferSize:16;
        u32             IPHdrOffset:8;
        u32             Rsvd3:7;
        u32             TCPEn:1;
} tx_desc_fw, *ptx_desc_fw;

typedef struct _H2C_SITESURVEY_PARA {
    	u32 		start_flag; 
    	u32 		probe_req_len; 
  	tx_desc_fw	desc;		
	u8		probe_req[0];
}H2C_SITESURVEY_PARA, *PH2C_SITESURVEY_PARA;

 typedef enum _FIRMWARE_H2C_CMD{
	H2C_Read_MACREG_CMD ,		/*0*/
	H2C_Write_MACREG_CMD ,		
	H2C_ReadBB_CMD ,	
	H2C_WriteBB_CMD ,		
	H2C_ReadRF_CMD ,		
	H2C_WriteRF_CMD ,		/*5*/
	H2C_Read_EEPROM_CMD ,		
	H2C_Write_EEPROM_CMD ,		
	H2C_Read_EFUSE_CMD ,		
	H2C_Write_EFUSE_CMD ,		
	H2C_Read_CAM_CMD ,			/*10*/
	H2C_Write_CAM_CMD ,			
	H2C_setBCNITV_CMD,
	H2C_setMBIDCFG_CMD,
	H2C_JoinBss_CMD ,				
	H2C_DisConnect_CMD,			/*15*/
	H2C_CreateBss_CMD ,
	H2C_SetOpMode_CMD,	
	H2C_SiteSurvey_CMD,
	H2C_SetAuth_CMD,
	H2C_SetKey_CMD ,				/*20*/
	H2C_SetStaKey_CMD ,
	H2C_SetAssocSta_CMD,
	H2C_DelAssocSta_CMD ,
	H2C_SetStaPwrState_CMD ,
	H2C_SetBasicRate_CMD ,		/*25*/
	H2C_GetBasicRate_CMD ,
	H2C_SetDataRate_CMD ,
	H2C_GetDataRate_CMD ,
	H2C_SetPhyInfo_CMD ,
	H2C_GetPhyInfo_CMD ,			/*30*/
	H2C_SetPhy_CMD ,
	H2C_GetPhy_CMD ,
	H2C_readRssi_CMD ,
	H2C_readGain_CMD ,
	H2C_SetAtim_CMD ,			/*35*/
	H2C_SetPwrMode_CMD ,
	H2C_JoinbssRpt_CMD,
	H2C_SetRaTable_CMD ,	
	H2C_GetRaTable_CMD ,	
	H2C_GetCCXReport_CMD,            /*40*/
	H2C_GetDTMReport_CMD,		
	H2C_GetTXRateStatistics_CMD,
	H2C_SetUsbSuspend_CMD,
	H2C_SetH2cLbk_CMD ,
	H2C_tmp1 ,			/*45*/
	H2C_WoWLAN_UPDATE_GTK_CMD ,	
	H2C_WoWLAN_FW_OFFLOAD , 
	H2C_tmp2 , 
	H2C_tmp3 , 
	H2C_WoWLAN_UPDATE_IV_CMD , 		/*50*/		
	H2C_tmp4,	
	MAX_H2CCMD							/*52*/
}FIRMWARE_H2C_CMD;


typedef  struct _RT_8192S_FIRMWARE_PRIV { 

	u8		signature_0;		
	u8		signature_1;		
	u8		hci_sel;			
	u8		chip_version;	
	u8		customer_ID_0;	
	u8		customer_ID_1;	
	u8		rf_config;		
	u8		usb_ep_num;	
	
	u8		regulatory_class_0;	
	u8		regulatory_class_1;	
	u8		regulatory_class_2;	
	u8		regulatory_class_3;	
	u8		rfintfs;				
	u8		def_nettype;		
	u8		rsvd010;
	u8		rsvd011;
	
	
	u8		lbk_mode;	
	u8		mp_mode;	
	u8		rsvd020;
	u8		rsvd021;
	u8		rsvd022;
	u8		rsvd023;
	u8		rsvd024;
	u8		rsvd025;

	u8		qos_en;				
	u8		bw_40MHz_en;		
	u8		AMSDU2AMPDU_en;	
	u8		AMPDU_en;			
	u8		rate_control_offload;
	u8		aggregation_offload;	
	u8		rsvd030;
	u8		rsvd031;

	
	unsigned char		beacon_offload;			
	unsigned char		MLME_offload;			
	unsigned char		hwpc_offload;			
	unsigned char		tcp_checksum_offload;	
	unsigned char		tcp_offload;				
	unsigned char		ps_control_offload;		
	unsigned char		WWLAN_offload;			
	unsigned char		rsvd040;

	u8		tcp_tx_frame_len_L;		
	u8		tcp_tx_frame_len_H;		
	u8		tcp_rx_frame_len_L;		
	u8		tcp_rx_frame_len_H;		
	u8		rsvd050;
	u8		rsvd051;
	u8		rsvd052;
	u8		rsvd053;
}RT_8192S_FIRMWARE_PRIV, *PRT_8192S_FIRMWARE_PRIV;

typedef struct _RT_8192S_FIRMWARE_HDR {

	u16		Signature;
	u16		Version;		  
	u32		DMEMSize;    


	u32		IMG_IMEM_SIZE;    
	u32		IMG_SRAM_SIZE;    

	u32		FW_PRIV_SIZE;       
	u32		Rsvd0;  

	u32		Rsvd1;
	u32		Rsvd2;

	RT_8192S_FIRMWARE_PRIV	FWPriv;
	
}RT_8192S_FIRMWARE_HDR, *PRT_8192S_FIRMWARE_HDR;

#define	RT_8192S_FIRMWARE_HDR_SIZE	80
#define   RT_8192S_FIRMWARE_HDR_EXCLUDE_PRI_SIZE	32

typedef enum _FIRMWARE_8192S_STATUS{
	FW_STATUS_INIT = 0,
	FW_STATUS_LOAD_IMEM = 1,
	FW_STATUS_LOAD_EMEM = 2,
	FW_STATUS_LOAD_DMEM = 3,
	FW_STATUS_READY = 4,
}FIRMWARE_8192S_STATUS;

typedef struct _rt_firmware{	
	firmware_source_e	eFWSource;	
	PRT_8192S_FIRMWARE_HDR	pFwHeader;
	FIRMWARE_8192S_STATUS	FWStatus;
	u16		FirmwareVersion;
	u8		FwIMEM[RTL8190_MAX_FIRMWARE_CODE_SIZE];
	u8		FwEMEM[RTL8190_MAX_FIRMWARE_CODE_SIZE];
	u32		FwIMEMLen;
	u32		FwEMEMLen;	
	u8		szFwTmpBuffer[164000];	
	u32		szFwTmpBufferLen;
	u16		CmdPacketFragThresold;		
}rt_firmware, *prt_firmware;

#define		FW_DIG_ENABLE_CTL			BIT0
#define		FW_HIGH_PWR_ENABLE_CTL	BIT1
#define		FW_SS_CTL					BIT2
#define		FW_RA_INIT_CTL				BIT3
#define		FW_RA_BG_CTL				BIT4
#define		FW_RA_N_CTL				BIT5
#define		FW_PWR_TRK_CTL			BIT6
#define		FW_IQK_CTL					BIT7
#define		FW_FA_CTL					BIT8
#define		FW_DRIVER_CTRL_DM_CTL		BIT9
#define		FW_PAPE_CTL_BY_SW_HW	BIT10
#define		FW_DISABLE_ALL_DM			0

#define		FW_PWR_TRK_PARAM_CLR		0x0000ffff
#define		FW_RA_PARAM_CLR			0xffff0000

#define FW_CMD_IO_CLR(priv, _Bit)		\
	udelay(1000);	\
	priv->FwCmdIOMap &= (~_Bit);

#define FW_CMD_IO_UPDATE(priv, _val)		\
	priv->FwCmdIOMap = _val;

#define FW_CMD_IO_SET(priv, _val) 	\
	write_nic_word(priv->rtllib->dev, LBUS_MON_ADDR, (u16)_val);	\
	FW_CMD_IO_UPDATE(priv, _val);\

#define FW_CMD_PARA_SET(priv, _val) 		\
	write_nic_dword(priv->rtllib->dev, LBUS_ADDR_MASK, _val);	\
	priv->FwCmdIOParam = _val;

#define FW_CMD_IO_QUERY(priv)	(u16)(priv->FwCmdIOMap)
#define FW_CMD_IO_PARA_QUERY(priv)	((u32)(priv->FwCmdIOParam))

bool FirmwareEnableCPU(struct net_device *dev);
bool FirmwareCheckReady(struct net_device *dev, u8 LoadFWStatus);

bool FirmwareDownload92S(struct net_device *dev);
int rtl8192se_send_scan_cmd(struct net_device 	*dev, bool start);

void rtl8192se_dump_skb_data(struct sk_buff *skb);
#endif

