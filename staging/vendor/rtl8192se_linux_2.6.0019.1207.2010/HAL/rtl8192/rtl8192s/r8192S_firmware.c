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
#if defined(RTL8192SE)
#include "../rtl_core.h"
#include "../../../rtllib/rtllib_endianfree.h"

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0) && defined(USE_FW_SOURCE_IMG_FILE)
#include <linux/firmware.h>
#endif
#define   byte(x,n)  ( (x >> (8 * n)) & 0xff  )

static void fw_SetRQPN(struct net_device *dev)
{	

	write_nic_dword(dev,  RQPN, 0xffffffff);
	write_nic_dword(dev,  RQPN+4, 0xffffffff);
	write_nic_byte(dev,  RQPN+8, 0xff);
	write_nic_byte(dev,  RQPN+0xB, 0x80);


}	/* fw_SetRQPN */

bool FirmwareDownloadCode(struct net_device *dev, u8 *	code_virtual_address,u32 buffer_len)
{
	struct r8192_priv   *priv = rtllib_priv(dev);
	bool 		    rt_status = true;
	u16		    frag_threshold = MAX_FIRMWARE_CODE_SIZE; 
	u16		    frag_length, frag_offset = 0;
	struct sk_buff	    *skb;
	unsigned char	    *seg_ptr;
	cb_desc		    *tcb_desc;	
	u8                  	    bLastIniPkt = 0;
	u16 			    ExtraDescOffset = 0;
	
#ifdef RTL8192SE
	fw_SetRQPN(dev);	
#endif

	RT_TRACE(COMP_FIRMWARE, "--->FirmwareDownloadCode()\n" );	

	if(buffer_len >= MAX_FIRMWARE_CODE_SIZE)
	{
		RT_TRACE(COMP_ERR, "Size over FIRMWARE_CODE_SIZE! \n");
		goto cmdsend_downloadcode_fail;
	}
	
	ExtraDescOffset = 0;
	
	do {
		if((buffer_len-frag_offset) > frag_threshold)
		{
			frag_length = frag_threshold + ExtraDescOffset;
		}
		else
		{
			frag_length = (u16)(buffer_len - frag_offset + ExtraDescOffset);
			bLastIniPkt = 1;
		}

		skb  = dev_alloc_skb(frag_length); 
		memcpy((unsigned char *)(skb->cb),&dev,sizeof(dev));
		
		tcb_desc = (cb_desc*)(skb->cb + MAX_DEV_ADDR_SIZE);
		tcb_desc->queue_index = TXCMD_QUEUE;
		tcb_desc->bCmdOrInit = DESC_PACKET_TYPE_INIT;
		tcb_desc->bLastIniPkt = bLastIniPkt;
		
		skb_reserve(skb, ExtraDescOffset);
		seg_ptr = (u8 *)skb_put(skb, (u32)(frag_length-ExtraDescOffset));
		memcpy(seg_ptr, code_virtual_address+frag_offset, (u32)(frag_length-ExtraDescOffset));
		
		tcb_desc->txbuf_size= frag_length;

		if(!priv->rtllib->check_nic_enough_desc(dev,tcb_desc->queue_index)||
			(!skb_queue_empty(&priv->rtllib->skb_waitQ[tcb_desc->queue_index]))||\
			(priv->rtllib->queue_stop) ) 
		{
			RT_TRACE(COMP_FIRMWARE,"=====================================================> tx full!\n");
			skb_queue_tail(&priv->rtllib->skb_waitQ[tcb_desc->queue_index], skb);
		} 
		else 
		{
			priv->rtllib->softmac_hard_start_xmit(skb,dev);
		}
		
		frag_offset += (frag_length - ExtraDescOffset);

	}while(frag_offset < buffer_len);
	write_nic_byte(dev, TPPoll, TPPoll_CQ);
	return rt_status ;


cmdsend_downloadcode_fail:	
	rt_status = false;
	RT_TRACE(COMP_ERR, "CmdSendDownloadCode fail !!\n");
	return rt_status;

}



bool
FirmwareEnableCPU(struct net_device *dev)
{

	bool rtStatus = true;
	u8		tmpU1b, CPUStatus = 0;
	u16		tmpU2b;	
	u32		iCheckTime = 200;	

	RT_TRACE(COMP_FIRMWARE, "-->FirmwareEnableCPU()\n" );

	fw_SetRQPN(dev);	
	
	tmpU1b = read_nic_byte(dev, SYS_CLKR);
	write_nic_byte(dev,  SYS_CLKR, (tmpU1b|SYS_CPU_CLKSEL)); 

	tmpU2b = read_nic_word(dev, SYS_FUNC_EN);	
	write_nic_word(dev, SYS_FUNC_EN, (tmpU2b|FEN_CPUEN));

	do
	{
		CPUStatus = read_nic_byte(dev, TCR);
		if(CPUStatus& IMEM_RDY)
		{
			RT_TRACE(COMP_FIRMWARE, "IMEM Ready after CPU has refilled.\n");	
			break;		
		}

		udelay(100);
	}while(iCheckTime--);

	if(!(CPUStatus & IMEM_RDY))
		return false;

	RT_TRACE(COMP_FIRMWARE, "<--FirmwareEnableCPU(): rtStatus(%#x)\n", rtStatus);
	return rtStatus;			
}

FIRMWARE_8192S_STATUS
FirmwareGetNextStatus(FIRMWARE_8192S_STATUS FWCurrentStatus)
{
	FIRMWARE_8192S_STATUS	NextFWStatus = 0;
	
	switch(FWCurrentStatus)
	{
		case FW_STATUS_INIT:
			NextFWStatus = FW_STATUS_LOAD_IMEM;
			break;

		case FW_STATUS_LOAD_IMEM:
			NextFWStatus = FW_STATUS_LOAD_EMEM;
			break;
		
		case FW_STATUS_LOAD_EMEM:
			NextFWStatus = FW_STATUS_LOAD_DMEM;
			break;

		case FW_STATUS_LOAD_DMEM:
			NextFWStatus = FW_STATUS_READY;
			break;

		default:
			RT_TRACE(COMP_ERR,"Invalid FW Status(%#x)!!\n", FWCurrentStatus);			
			break;
	}	
	return	NextFWStatus;
}

bool FirmwareCheckReady(struct net_device *dev, u8 LoadFWStatus)
{
	struct r8192_priv  *priv = rtllib_priv(dev);
	bool rtStatus = true;
	rt_firmware  *pFirmware = priv->pFirmware;
	short  PollingCnt = 1000;
	u8   CPUStatus = 0;
	u32  tmpU4b;

	RT_TRACE(COMP_FIRMWARE, "--->%s(): LoadStaus(%d),", __FUNCTION__, LoadFWStatus);

	pFirmware->FWStatus = (FIRMWARE_8192S_STATUS)LoadFWStatus;	

	switch (LoadFWStatus) {
	case FW_STATUS_LOAD_IMEM:
		do {
			CPUStatus = read_nic_byte(dev, TCR);
			if(CPUStatus& IMEM_CODE_DONE)
				break;	
			udelay(5);
		} while (PollingCnt--);		
		if (!(CPUStatus & IMEM_CHK_RPT) || (PollingCnt <= 0)) {
			RT_TRACE(COMP_ERR, "FW_STATUS_LOAD_IMEM FAIL CPU, Status=%x\r\n", CPUStatus);
			goto status_check_fail;			
		}
		break;

	case FW_STATUS_LOAD_EMEM:
		do {
			CPUStatus = read_nic_byte(dev, TCR);
			if(CPUStatus& EMEM_CODE_DONE)
				break;
			udelay(5);
		} while(PollingCnt--);		
		if (!(CPUStatus & EMEM_CHK_RPT) || (PollingCnt <= 0)) {
			RT_TRACE(COMP_ERR, "FW_STATUS_LOAD_EMEM FAIL CPU, Status=%x\r\n", CPUStatus);
			goto status_check_fail;
		}

		rtStatus = FirmwareEnableCPU(dev);
		if (rtStatus != true) {
			RT_TRACE(COMP_ERR, "Enable CPU fail ! \n" );
			goto status_check_fail;
		}		
		break;

	case FW_STATUS_LOAD_DMEM:
		do {
			CPUStatus = read_nic_byte(dev, TCR);
			if (CPUStatus& DMEM_CODE_DONE)
				break;		
			udelay(5);
		} while(PollingCnt--);

		if (!(CPUStatus & DMEM_CODE_DONE) || (PollingCnt <= 0)) {
			RT_TRACE(COMP_ERR, "Polling  DMEM code done fail ! CPUStatus(%#x)\n", CPUStatus);
			goto status_check_fail;
		}

		RT_TRACE(COMP_FIRMWARE, "DMEM code download success, CPUStatus(%#x)\n", CPUStatus);		
		PollingCnt = 2000; 
		do {
			CPUStatus = read_nic_byte(dev, TCR);
			if(CPUStatus & FWRDY)
				break;		
			udelay(40);
		} while(PollingCnt--);	

		RT_TRACE(COMP_FIRMWARE, "Polling Load Firmware ready, CPUStatus(%x)\n", CPUStatus);		
		if (((CPUStatus & LOAD_FW_READY) != LOAD_FW_READY) || (PollingCnt <= 0)) {
			RT_TRACE(COMP_ERR, "Polling Load Firmware ready fail ! CPUStatus(%x)\n", CPUStatus);
			goto status_check_fail;
		}

#ifdef RTL8192SE
#endif 

		tmpU4b = read_nic_dword(dev,TCR);
		write_nic_dword(dev, TCR, (tmpU4b&(~TCR_ICV)));

		tmpU4b = read_nic_dword(dev, RCR);
		write_nic_dword(dev, RCR, 
				(tmpU4b|RCR_APPFCS|RCR_APP_ICV|RCR_APP_MIC));

		RT_TRACE(COMP_FIRMWARE, "FirmwareCheckReady(): Current RCR settings(%#x)\n", tmpU4b);

#if 0
		priv->TransmitConfig = read_nic_dword_E(dev, TCR);
		RT_TRACE(COMP_FIRMWARE, "FirmwareCheckReady(): Current TCR settings(%#x)\n", priv->TransmitConfig);
#endif

		write_nic_byte(dev, LBKMD_SEL, LBK_NORMAL);		
		break;	
	default :
		RT_TRACE(COMP_FIRMWARE, "Unknown status check!\n");
		rtStatus = false;
		break;
	}

status_check_fail:
	RT_TRACE(COMP_FIRMWARE, "<---%s: LoadFWStatus(%d), rtStatus(%x)\n", __FUNCTION__,
			LoadFWStatus, rtStatus);
	return rtStatus;
}
u8 FirmwareHeaderMapRfType(struct net_device *dev)
{	
	struct r8192_priv 	*priv = rtllib_priv(dev);
	switch(priv->rf_type)
	{
		case RF_1T1R: 	return 0x11;
		case RF_1T2R: 	return 0x12;
		case RF_2T2R: 	return 0x22;
		case RF_2T2R_GREEN: 	return 0x92;
		default:
			RT_TRACE(COMP_INIT, "Unknown RF type(%x)\n",priv->rf_type);
			break;
	}
	return 0x22;
}


void FirmwareHeaderPriveUpdate(struct net_device *dev, PRT_8192S_FIRMWARE_PRIV 	pFwPriv)
{
	pFwPriv->rf_config = FirmwareHeaderMapRfType(dev);
}



bool FirmwareDownload92S(struct net_device *dev)
{	
	struct r8192_priv 	*priv = rtllib_priv(dev);
	bool			rtStatus = true;
	u8			*pucMappedFile = NULL;
	u32			ulFileLength = 0;	
	u8			FwHdrSize = RT_8192S_FIRMWARE_HDR_SIZE;	
	rt_firmware		*pFirmware = priv->pFirmware;
	u8			FwStatus = FW_STATUS_INIT;
	PRT_8192S_FIRMWARE_HDR	pFwHdr = NULL;
	PRT_8192S_FIRMWARE_PRIV	pFwPriv = NULL;
	
	pFirmware->FWStatus = FW_STATUS_INIT;

	RT_TRACE(COMP_FIRMWARE, " --->FirmwareDownload92S()\n");

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0) && defined(USE_FW_SOURCE_IMG_FILE)
	priv->firmware_source = FW_SOURCE_IMG_FILE;
#else
	priv->firmware_source = FW_SOURCE_HEADER_FILE;
#endif

	switch( priv->firmware_source )
	{
		case FW_SOURCE_IMG_FILE:		
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0) && defined(USE_FW_SOURCE_IMG_FILE)			
			if(pFirmware->szFwTmpBufferLen == 0)
			{
#ifdef _RTL8192_EXT_PATCH_
#ifdef ASL
				const char 		*pFwImageFileName[1] = {"RTL8192SE_SA/rtl8192sfw.bin"};
#else
				const char 		*pFwImageFileName[1] = {"RTL8191SE_MESH/rtl8192sfw.bin"};
#endif
#else
#ifdef ASL
				const char 		*pFwImageFileName[1] = {"RTL8192SE_SA/rtl8192sfw.bin"};
#else
				const char 		*pFwImageFileName[1] = {"RTL8192SE/rtl8192sfw.bin"};
#endif
#endif
				const struct firmware 	*fw_entry = NULL;
				u32 ulInitStep = 0;
				int 			rc = 0;
				u32			file_length = 0;
				rc = request_firmware(&fw_entry, pFwImageFileName[ulInitStep],&priv->pdev->dev);
				if(rc < 0 ) {
					RT_TRACE(COMP_ERR, "request firmware fail!\n");
					goto DownloadFirmware_Fail;
				} 

				if(fw_entry->size > sizeof(pFirmware->szFwTmpBuffer)) {
					RT_TRACE(COMP_ERR, "img file size exceed the container buffer fail!\n");
					release_firmware(fw_entry);
					goto DownloadFirmware_Fail;
				}

				memcpy(pFirmware->szFwTmpBuffer,fw_entry->data,fw_entry->size);
				pFirmware->szFwTmpBufferLen = fw_entry->size;
				release_firmware(fw_entry);

				pucMappedFile = pFirmware->szFwTmpBuffer;
				file_length = pFirmware->szFwTmpBufferLen;

				pFirmware->pFwHeader = (PRT_8192S_FIRMWARE_HDR) pucMappedFile;				
				pFwHdr = pFirmware->pFwHeader;
				RT_TRACE(COMP_FIRMWARE,"signature:%x, version:%x, size:%x, imemsize:%x, sram size:%x\n", \
						pFwHdr->Signature, pFwHdr->Version, pFwHdr->DMEMSize, \
						pFwHdr->IMG_IMEM_SIZE, pFwHdr->IMG_SRAM_SIZE);
				pFirmware->FirmwareVersion =  byte(pFwHdr->Version ,0);			
				pFirmware->pFwHeader->FWPriv.hci_sel = 1;

				if ((pFwHdr->IMG_IMEM_SIZE==0) || (pFwHdr->IMG_IMEM_SIZE > sizeof(pFirmware->FwIMEM)))
				{
					RT_TRACE(COMP_ERR, "%s: memory for data image is less than IMEM required\n",\
							__FUNCTION__);
					goto DownloadFirmware_Fail;
				} else {				
					pucMappedFile+=FwHdrSize;

					memcpy(pFirmware->FwIMEM, pucMappedFile, pFwHdr->IMG_IMEM_SIZE);
					pFirmware->FwIMEMLen = pFwHdr->IMG_IMEM_SIZE;				
				}

				if (pFwHdr->IMG_SRAM_SIZE > sizeof(pFirmware->FwEMEM))
				{
					RT_TRACE(COMP_ERR, "%s: memory for data image is less than EMEM required\n",\
							__FUNCTION__);	
					goto DownloadFirmware_Fail;
				}
				else
				{	
					pucMappedFile += pFirmware->FwIMEMLen;

					memcpy(pFirmware->FwEMEM, pucMappedFile, pFwHdr->IMG_SRAM_SIZE);
					pFirmware->FwEMEMLen = pFwHdr->IMG_SRAM_SIZE;		
				}	
			}
#endif
			break;	

		case FW_SOURCE_HEADER_FILE:	
#if 1
#define Rtl819XFwImageArray Rtl8192SEFwImgArray
			pucMappedFile = Rtl819XFwImageArray;
			ulFileLength = ImgArrayLength;

			RT_TRACE(COMP_INIT,"Fw download from header.\n");
			pFirmware->pFwHeader = (PRT_8192S_FIRMWARE_HDR) pucMappedFile;				
			pFwHdr = pFirmware->pFwHeader;
			RT_TRACE(COMP_FIRMWARE,"signature:%x, version:%x, size:%x, imemsize:%x, sram size:%x\n", \
					pFwHdr->Signature, pFwHdr->Version, pFwHdr->DMEMSize, \
					pFwHdr->IMG_IMEM_SIZE, pFwHdr->IMG_SRAM_SIZE);
			pFirmware->FirmwareVersion =  byte(pFwHdr->Version ,0);			

			if ((pFwHdr->IMG_IMEM_SIZE==0) || (pFwHdr->IMG_IMEM_SIZE > sizeof(pFirmware->FwIMEM)))
			{
				printk("FirmwareDownload92S(): memory for data image is less than IMEM required\n");					
				goto DownloadFirmware_Fail;
			}
			else
			{				
				pucMappedFile+=FwHdrSize;

				memcpy(pFirmware->FwIMEM, pucMappedFile, pFwHdr->IMG_IMEM_SIZE);
				pFirmware->FwIMEMLen = pFwHdr->IMG_IMEM_SIZE;				
			}

			if (pFwHdr->IMG_SRAM_SIZE > sizeof(pFirmware->FwEMEM))
			{
				printk(" FirmwareDownload92S(): memory for data image is less than EMEM required\n");					
				goto DownloadFirmware_Fail;
			} else {	
				pucMappedFile+= pFirmware->FwIMEMLen;

				memcpy(pFirmware->FwEMEM, pucMappedFile, pFwHdr->IMG_SRAM_SIZE);
				pFirmware->FwEMEMLen = pFwHdr->IMG_SRAM_SIZE;		
			}
#endif
			break;
		default:
			break;
	}			

	FwStatus = FirmwareGetNextStatus(pFirmware->FWStatus);
	while(FwStatus!= FW_STATUS_READY)
	{
		switch(FwStatus)
		{
			case FW_STATUS_LOAD_IMEM:				
				pucMappedFile = pFirmware->FwIMEM;
				ulFileLength = pFirmware->FwIMEMLen;					
				break;

			case FW_STATUS_LOAD_EMEM:				
				pucMappedFile = pFirmware->FwEMEM;
				ulFileLength = pFirmware->FwEMEMLen;					
				break;

			case FW_STATUS_LOAD_DMEM:			
				pFwHdr = pFirmware->pFwHeader;
				pFwPriv = (PRT_8192S_FIRMWARE_PRIV)&pFwHdr->FWPriv;
				FirmwareHeaderPriveUpdate(dev, pFwPriv);
				pucMappedFile = (u8*)(pFirmware->pFwHeader)+RT_8192S_FIRMWARE_HDR_EXCLUDE_PRI_SIZE;
				ulFileLength = FwHdrSize-RT_8192S_FIRMWARE_HDR_EXCLUDE_PRI_SIZE;				
				break;
					
			default:
				RT_TRACE(COMP_ERR, "Unexpected Download step!!\n");
				goto DownloadFirmware_Fail;
				break;
		}		
		
		rtStatus = FirmwareDownloadCode(dev, pucMappedFile, ulFileLength);	
		
		if(rtStatus != true)
		{
			RT_TRACE(COMP_ERR, "FirmwareDownloadCode() fail ! \n" );
			goto DownloadFirmware_Fail;
		}	
		
		rtStatus = FirmwareCheckReady(dev, FwStatus);

		if(rtStatus != true)
		{
			RT_TRACE(COMP_ERR, "FirmwareDownloadCode() fail ! \n");
			goto DownloadFirmware_Fail;
		}
		
		FwStatus = FirmwareGetNextStatus(pFirmware->FWStatus);
	}

	RT_TRACE(COMP_FIRMWARE, "Firmware Download Success!!\n");	
	return rtStatus;	
	
	DownloadFirmware_Fail:	
	RT_TRACE(COMP_ERR, "Firmware Download Fail!!%x\n",read_nic_word(dev, TCR));	
	rtStatus = false;
	return rtStatus;	
}
void rtl8192se_dump_skb_data(struct sk_buff *skb)
{
	u8 i = 0;
	u8 *arry = skb->data;

	printk("\nSCAN_CMD/PROBE_REQ==============>\n");
	for(i = 0; i < skb->len; i ++){
		if((i % 4 == 0)&&(i != 0))
			printk("\n");
		printk("%2.2x ", arry[i]);
	}
	printk("\nSCAN_CMD/PROBE_REQ<==============\n");
}

void rtl8192se_dump_cmd_para(u8*SiteSurveyPara)
{
	u8 i = 0;
	u8 desc_size = sizeof(tx_desc_fw);
	u8 para_size = 8+desc_size;
	u8 *arry = SiteSurveyPara;

	printk("\nSCAN_CMD_PARA==============>\n");
	for(i = 0; i < para_size; i ++){
		if((i % 4 == 0)&&(i != 0))
			printk("\n");
		printk("%2.2x ", arry[i]);
	}
	printk("\nSCAN_CMD_PARA<==============\n");
}
#if 1 
RT_STATUS
CmdSendPacket(
	struct net_device	*dev,
	cb_desc			*pTcb,
	struct sk_buff	    	*skb,
	u32				BufferLen,
	u32				PacketType,
	bool				bLastInitPacket 
	)
{
	struct r8192_priv   	*priv = rtllib_priv(dev);

	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
	
	pTcb->queue_index 	= TXCMD_QUEUE;
	pTcb->bCmdOrInit 	= PacketType;
	pTcb->bLastIniPkt	= bLastInitPacket;
	pTcb->txbuf_size	= BufferLen;


	if(!priv->rtllib->check_nic_enough_desc(dev,pTcb->queue_index)||
		(!skb_queue_empty(&priv->rtllib->skb_waitQ[pTcb->queue_index]))||\
		(priv->rtllib->queue_stop) ) 	{
		RT_TRACE(COMP_CMD,"=========================> tx full!\n");
		skb_queue_tail(&priv->rtllib->skb_waitQ[pTcb->queue_index], skb);
	} else {
		priv->rtllib->softmac_hard_start_xmit(skb,dev);
	}
		
	return rtStatus;
}

u32
FillH2CCmd(
	struct sk_buff 	*skb,
	u32			H2CBufferLen,
	u32			CmdNum,
	u32*		pElementID,
	u32*		pCmdLen,
	u8**		pCmbBuffer,
	u8*			CmdStartSeq
	)
{
	u8	i = 0;
	u32	TotalLen = 0, Len = 0, TxDescLen = 0;
	u32	preContinueOffset = 0;

	u8*	pH2CBuffer;
	
	do
	{
		Len = H2C_TX_CMD_HDR_LEN + N_BYTE_ALIGMENT(pCmdLen[i], 8);

		if(H2CBufferLen < TotalLen + Len + TxDescLen)
			break;
		
		pH2CBuffer = (u8 *) skb_put(skb, (u32)Len);
		memset((pH2CBuffer + TotalLen + TxDescLen),0,Len);

		SET_BITS_TO_LE_4BYTE((pH2CBuffer + TotalLen + TxDescLen), 0, 16, pCmdLen[i]);

		SET_BITS_TO_LE_4BYTE((pH2CBuffer + TotalLen + TxDescLen), 16, 8, pElementID[i]);

		*CmdStartSeq = *CmdStartSeq % 0x80;
		SET_BITS_TO_LE_4BYTE((pH2CBuffer + TotalLen + TxDescLen), 24, 7, *CmdStartSeq);
		++ *CmdStartSeq;
		
		memcpy((pH2CBuffer + TotalLen + TxDescLen + H2C_TX_CMD_HDR_LEN), pCmbBuffer[i], pCmdLen[i]);

		if(i < CmdNum - 1) 
			SET_BITS_TO_LE_4BYTE((pH2CBuffer + preContinueOffset), 31, 1, 1);

		preContinueOffset = TotalLen;

		TotalLen += Len;
	}while(++ i < CmdNum);

	return TotalLen;
}

u32
GetH2CCmdLen(
	u32		H2CBufferLen,
	u32		CmdNum,
	u32*	pCmdLen
	)
{
	u8	i = 0;
	u32	TotalLen = 0, Len = 0, TxDescLen = 0;

	do
	{
		Len = H2C_TX_CMD_HDR_LEN + N_BYTE_ALIGMENT(pCmdLen[i], 8);

		if(H2CBufferLen < TotalLen + Len + TxDescLen)
			break;

		TotalLen += Len;
	}while(++ i < CmdNum);

	return TotalLen + TxDescLen;
}

/*-----------------------------------------------------------------------------
 * Function:	FirmwareSetH2CCmd()
 *
 * Overview:	Set FW H2C command (Decide ElementID, cmd content length, and get FW buffer)
 *
 * Input:		H2CCmd: H2C command type. 
 *			pCmdBuffer: Pointer of the H2C command content.
 *
 * Output:		NONE
 *
 * Return:		RT_STATUS
 *
 * Revised History:
 *	When		Who		Remark
 *	2009/1/12	tynli		Create the version 0.
 *
 *---------------------------------------------------------------------------*/
RT_STATUS
FirmwareSetH2CCmd( 
	struct net_device 	*dev, 
	u8				H2CCmd,
	u8*				pCmdBuffer
	)
{	
	struct r8192_priv   *priv = rtllib_priv(dev);
	u32				ElementID;
	u32				Cmd_Len;
	cb_desc			*pTcb;
	struct sk_buff	    	*skb;
	u32				Len;

	RT_STATUS		rtStatus;

	switch(H2CCmd){
	case FW_H2C_SETPWRMODE:
		{
			ElementID = H2C_SetPwrMode_CMD ;
			Cmd_Len = sizeof(H2C_SETPWRMODE_PARM);
		}
		break;
	case FW_H2C_JOINBSSRPT:
		{
			ElementID = H2C_JoinbssRpt_CMD;
			Cmd_Len = sizeof(H2C_JOINBSSRPT_PARM);
		}
		break;
	case FW_H2C_WoWLAN_UPDATE_GTK:
		{
			ElementID = H2C_WoWLAN_UPDATE_GTK_CMD;
			Cmd_Len = sizeof(H2C_WPA_TWO_WAY_PARA);
		}
		break;	
	case FW_H2C_WoWLAN_UPDATE_IV:
		{
			ElementID = H2C_WoWLAN_UPDATE_IV_CMD;
			Cmd_Len = sizeof(unsigned long long);
		}
		break;

	case FW_H2C_WoWLAN_OFFLOAD:
		{
			ElementID = H2C_WoWLAN_FW_OFFLOAD;
			Cmd_Len = sizeof(u8);
		}
		break;
	case FW_H2C_SITESURVEY:
		{
			ElementID = H2C_SiteSurvey_CMD;
			Cmd_Len = sizeof(H2C_SITESURVEY_PARA) + ((PH2C_SITESURVEY_PARA)pCmdBuffer)->probe_req_len - sizeof(tx_desc_fw);
		}
		break;
	default:
		break;
	}

	RT_TRACE(COMP_CMD, "FirmwareSetH2CCmd() HW_VAR_SET_TX_CMD: ElementID = %d, %d+%d=Cmd_Len = %d\n", ElementID, sizeof(H2C_SITESURVEY_PARA),((PH2C_SITESURVEY_PARA)pCmdBuffer)->probe_req_len, Cmd_Len);

	{
		Len = GetH2CCmdLen(MAX_TRANSMIT_BUFFER_SIZE, 1, &Cmd_Len);

		RT_TRACE(COMP_CMD, "---------->%s(), cmdlen:%d,len:%d\n", __func__,Cmd_Len,Len);
		skb  = dev_alloc_skb(Len); 
		memcpy((unsigned char *)(skb->cb),&dev,sizeof(dev));

		pTcb = (cb_desc*)(skb->cb + MAX_DEV_ADDR_SIZE);
		
		FillH2CCmd(skb, MAX_TRANSMIT_BUFFER_SIZE, 1, &ElementID, &Cmd_Len, &pCmdBuffer, &priv->H2CTxCmdSeq);
		

		rtStatus = CmdSendPacket(dev, pTcb, skb, Len, DESC_PACKET_TYPE_NORMAL, false);
		
	}
	
	write_nic_byte(dev, TPPoll, TPPoll_CQ);

	return RT_STATUS_SUCCESS;
}


void
rtl8192se_set_scan_cmd(struct net_device *dev, u32 start_flag)
{
	struct r8192_priv   			*priv = rtllib_priv(dev);
	H2C_SITESURVEY_PARA			*SiteSurveyPara;

	if (!priv->scan_cmd) {
		priv->scan_cmd = kmalloc(sizeof(H2C_SITESURVEY_PARA) +
				RTL_MAX_SCAN_SIZE, GFP_KERNEL);
		if (!priv->scan_cmd) {
			printk("----------->%s() Error!!!\n", __func__);
			return;
		}
	}

	SiteSurveyPara = priv->scan_cmd;
	memset(SiteSurveyPara, 0, sizeof(struct _H2C_SITESURVEY_PARA) + RTL_MAX_SCAN_SIZE);

	if(start_flag){
		struct sk_buff *skb = rtllib_probe_req(priv->rtllib);	
		cb_desc *tcb_desc = (cb_desc *)(skb->cb + 8);


		tcb_desc->queue_index = MGNT_QUEUE;
#ifdef ASL
		tcb_desc->data_rate = MgntQuery_MgntFrameTxRate(priv->rtllib, 0);
#else
		tcb_desc->data_rate = MgntQuery_MgntFrameTxRate(priv->rtllib);	
#endif
		tcb_desc->RATRIndex = 7;
		tcb_desc->bTxDisableRateFallBack = 1;
		tcb_desc->bTxUseDriverAssingedRate = 1;
	
		
		SiteSurveyPara->start_flag 	= start_flag;
		SiteSurveyPara->probe_req_len 	= skb->len + sizeof(tx_desc_fw);
	
		SiteSurveyPara->desc.MacID 	= 0;
		SiteSurveyPara->desc.TXHT		= (tcb_desc->data_rate&0x80)?1:0;	
		SiteSurveyPara->desc.TxRate	= MRateToHwRate8192SE(dev,tcb_desc->data_rate);
		SiteSurveyPara->desc.TxShort	= rtl8192se_QueryIsShort(((tcb_desc->data_rate&0x80)?1:0), MRateToHwRate8192SE(dev,tcb_desc->data_rate), tcb_desc);

		SiteSurveyPara->desc.AggEn 	= 0;
		SiteSurveyPara->desc.Seq 		= 0; 
		SiteSurveyPara->desc.RTSEn	= (tcb_desc->bRTSEnable && tcb_desc->bCTSEnable==false)?1:0;				
		SiteSurveyPara->desc.CTS2Self	= (tcb_desc->bCTSEnable)?1:0;		
		SiteSurveyPara->desc.RTSSTBC	= (tcb_desc->bRTSSTBC)?1:0;
		SiteSurveyPara->desc.RTSHT	= (tcb_desc->rts_rate&0x80)?1:0;
		SiteSurveyPara->desc.RTSRate	= MRateToHwRate8192SE(dev,tcb_desc->rts_rate);
		SiteSurveyPara->desc.RTSRate	= MRateToHwRate8192SE(dev,MGN_24M);
		SiteSurveyPara->desc.RTSBW	= 0;
		SiteSurveyPara->desc.RTSSC	= tcb_desc->RTSSC;
		SiteSurveyPara->desc.RTSShort	= (SiteSurveyPara->desc.RTSHT==0)?(tcb_desc->bRTSUseShortPreamble?1:0):(tcb_desc->bRTSUseShortGI?1:0);

		if(priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40){
			if(tcb_desc->bPacketBW)	{
				SiteSurveyPara->desc.TxBw = 1;
				SiteSurveyPara->desc.TXSC	 = 0;
			} else {				
				SiteSurveyPara->desc.TxBw = 0;
				SiteSurveyPara->desc.TXSC	 = priv->nCur40MhzPrimeSC;
			}
		} else {
			SiteSurveyPara->desc.TxBw	= 0;
			SiteSurveyPara->desc.TXSC		= 0;
		}
	
		SiteSurveyPara->desc.LINIP 		= 0;
		SiteSurveyPara->desc.Offset 		= 32;
		SiteSurveyPara->desc.PktSize 		= (u16)skb->len;		
	
		SiteSurveyPara->desc.RaBRSRID 	= tcb_desc->RATRIndex;
	
		SiteSurveyPara->desc.PktID		= 0x0;		
		SiteSurveyPara->desc.QueueSel		= rtl8192se_MapHwQueueToFirmwareQueue(tcb_desc->queue_index, tcb_desc->priority); 
	
		SiteSurveyPara->desc.DataRateFBLmt= 0x1F;		
		SiteSurveyPara->desc.DISFB		= tcb_desc->bTxDisableRateFallBack;
		SiteSurveyPara->desc.UserRate		= tcb_desc->bTxUseDriverAssingedRate;

	
		SiteSurveyPara->desc.FirstSeg		= 1;
		SiteSurveyPara->desc.LastSeg		= 1;
	
		SiteSurveyPara->desc.TxBufferSize	= (u16)skb->len;	
	
		SiteSurveyPara->desc.OWN			= 1;


		memcpy(&SiteSurveyPara->probe_req[0], skb->data, (u16)skb->len);

		dev_kfree_skb_any(skb);
	} else {
		SiteSurveyPara->start_flag 	= start_flag;
		SiteSurveyPara->probe_req_len 	= 0;
	}
	
	FirmwareSetH2CCmd( dev ,FW_H2C_SITESURVEY, (u8*)SiteSurveyPara);
}

int rtl8192se_send_scan_cmd(struct net_device 	*dev, bool start)
{
	struct r8192_priv   *priv = rtllib_priv(dev);
	
	if(start){
		priv->rtllib->scan_watch_dog =0;
#if 0
		queue_delayed_work_rsl(priv->priv_wq,&priv->hw_scan_simu_wq,0);
#else
		rtl8192se_set_scan_cmd(dev, start);
#endif
	} else {
#if 0
		rtl8192se_rx_surveydone_cmd(dev);
#else
		rtl8192se_set_scan_cmd(dev, start);
#endif
	}
	return true;
}
#endif

#endif
