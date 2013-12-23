/*
 *      Handle routines for encryption CAM
 *
 *      $Id: 8190n_cam.c,v 1.6 2009/08/06 11:41:28 yachang Exp $
 */

#define _8190N_CAM_C_

#include "./8190n_cfg.h"
#include "./8190n.h"
#include "./8190n_hw.h"
#include "./8190n_util.h"
#include "./8190n_debug.h"
#include "./8190n_headers.h"


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
	ULONG ioaddr = priv->pshare->ioaddr;
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
	ULONG ioaddr = priv->pshare->ioaddr;
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
	ULONG ioaddr = priv->pshare->ioaddr;
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
	ULONG ioaddr = priv->pshare->ioaddr;
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
	ULONG ioaddr = priv->pshare->ioaddr;
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
	ULONG ioaddr = priv->pshare->ioaddr;
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

	ULONG ioaddr = priv->pshare->ioaddr;
	WritePortUlong(_CAMCMD_, _CAM_CLR_);

	for(ucIndex=0;ucIndex<TOTAL_CAM_ENTRY;ucIndex++) {
		CAM_empty_entry(priv,ucIndex);
	}

	priv->pshare->CamEntryOccupied = 0;
	priv->pmib->dot11GroupKeysTable.keyInCam = 0;
}


void CAM_read_entry(struct rtl8190_priv *priv,UCHAR index, UCHAR* macad,UCHAR* key128, USHORT* config)
{
	ULONG ioaddr = priv->pshare->ioaddr;
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

