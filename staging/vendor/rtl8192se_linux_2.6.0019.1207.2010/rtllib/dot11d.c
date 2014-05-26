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
#ifdef ENABLE_DOT11D
#include "dot11d.h"

typedef struct _CHANNEL_LIST
{
	u8      Channel[32];
	u8      Len;
}CHANNEL_LIST, *PCHANNEL_LIST;

static CHANNEL_LIST ChannelPlan[] = {
	{{1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,52,56,60,64,149,153,157,161,165},24},
	{{1,2,3,4,5,6,7,8,9,10,11},11},    
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48,52,56,60,64},21},
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},   
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},   
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64},22}, 
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64},22}, 
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},   
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64},22}, 
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64},22}, 
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14},14}, 
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},    
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48,52,56,60,64},21}    
};

void Dot11d_Init(struct rtllib_device *ieee)
{
	PRT_DOT11D_INFO pDot11dInfo = GET_DOT11D_INFO(ieee);
#if defined CONFIG_CRDA && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)  
	ieee->bGlobalDomain = true;
	pDot11dInfo->bEnabled = true;
#else
	pDot11dInfo->bEnabled = false;
#endif	

	pDot11dInfo->State = DOT11D_STATE_NONE;
	pDot11dInfo->CountryIeLen = 0;
	memset(pDot11dInfo->channel_map, 0, MAX_CHANNEL_NUMBER+1);  
	memset(pDot11dInfo->MaxTxPwrDbmList, 0xFF, MAX_CHANNEL_NUMBER+1);
	RESET_CIE_WATCHDOG(ieee);

}

void Dot11d_Channelmap(u8 channel_plan, struct rtllib_device* ieee)
{
	int i, max_chan = 14, min_chan = 1;

	ieee->bGlobalDomain = false;

	if (ChannelPlan[channel_plan].Len != 0) {
		memset(GET_DOT11D_INFO(ieee)->channel_map, 0, sizeof(GET_DOT11D_INFO(ieee)->channel_map));
		for (i = 0; i < ChannelPlan[channel_plan].Len; i++) {
			if (ChannelPlan[channel_plan].Channel[i] < min_chan ||
					ChannelPlan[channel_plan].Channel[i] > max_chan)
				break;
			GET_DOT11D_INFO(ieee)->channel_map[ChannelPlan[channel_plan].Channel[i]] = 1;
		}
	}

	switch (channel_plan) {
		case COUNTRY_CODE_GLOBAL_DOMAIN:
			ieee->bGlobalDomain = true;
			for (i = 12; i <= 14; i++) {
				GET_DOT11D_INFO(ieee)->channel_map[i] = 2;
			}
			ieee->IbssStartChnl= 10;
			ieee->ibss_maxjoin_chal = 11;
			break;

		case COUNTRY_CODE_WORLD_WIDE_13:
			for (i = 12; i <= 13; i++) {
				GET_DOT11D_INFO(ieee)->channel_map[i] = 2;
			}
			ieee->IbssStartChnl = 10;
			ieee->ibss_maxjoin_chal = 11;
			break;

		default:
			ieee->IbssStartChnl = 1;
			ieee->ibss_maxjoin_chal = 14;
			break;
	}
}
						

void Dot11d_Reset(struct rtllib_device *ieee)
{
	PRT_DOT11D_INFO pDot11dInfo = GET_DOT11D_INFO(ieee);
#if 0
	if(!pDot11dInfo->bEnabled)
		return;
#endif

#if defined CONFIG_CRDA && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30) 
#else
	u32 i;
	memset(pDot11dInfo->channel_map, 0, MAX_CHANNEL_NUMBER+1);
	memset(pDot11dInfo->MaxTxPwrDbmList, 0xFF, MAX_CHANNEL_NUMBER+1);
	for (i=1; i<=11; i++) {
		(pDot11dInfo->channel_map)[i] = 1;
	}
	for (i=12; i<=14; i++) {
		(pDot11dInfo->channel_map)[i] = 2;
	}
#endif
	pDot11dInfo->State = DOT11D_STATE_NONE;
	pDot11dInfo->CountryIeLen = 0;
	RESET_CIE_WATCHDOG(ieee);

}

void Dot11d_UpdateCountryIe(struct rtllib_device *dev, u8 *pTaddr,
	                    u16 CoutryIeLen, u8* pCoutryIe)
{
	PRT_DOT11D_INFO pDot11dInfo = GET_DOT11D_INFO(dev);
	
#if defined CONFIG_CRDA && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30) 
	
#else	
	u8 i, j, NumTriples, MaxChnlNum;
	PCHNL_TXPOWER_TRIPLE pTriple;

	memset(pDot11dInfo->channel_map, 0, MAX_CHANNEL_NUMBER+1);
	memset(pDot11dInfo->MaxTxPwrDbmList, 0xFF, MAX_CHANNEL_NUMBER+1);
	MaxChnlNum = 0;
	NumTriples = (CoutryIeLen - 3) / 3; 
	pTriple = (PCHNL_TXPOWER_TRIPLE)(pCoutryIe + 3);
	for(i = 0; i < NumTriples; i++)
	{
		if(MaxChnlNum >= pTriple->FirstChnl)
		{ 
			printk("Dot11d_UpdateCountryIe(): Invalid country IE, skip it........1\n");
			return; 
		}
		if(MAX_CHANNEL_NUMBER < (pTriple->FirstChnl + pTriple->NumChnls))
		{ 
			printk("Dot11d_UpdateCountryIe(): Invalid country IE, skip it........2\n");
			return; 
		}

		for(j = 0 ; j < pTriple->NumChnls; j++)
		{
			pDot11dInfo->channel_map[pTriple->FirstChnl + j] = 1;
			pDot11dInfo->MaxTxPwrDbmList[pTriple->FirstChnl + j] = pTriple->MaxTxPowerInDbm;
			MaxChnlNum = pTriple->FirstChnl + j;
		}	

		pTriple = (PCHNL_TXPOWER_TRIPLE)((u8*)pTriple + 3);
	}
#if 0
	printk("Channel List:");
	for(i=1; i<= MAX_CHANNEL_NUMBER; i++)
		if(pDot11dInfo->channel_map[i] > 0)
			printk(" %d", i);
	printk("\n");
#endif
#endif	

	UPDATE_CIE_SRC(dev, pTaddr);

	pDot11dInfo->CountryIeLen = CoutryIeLen;
	memcpy(pDot11dInfo->CountryIeBuf, pCoutryIe,CoutryIeLen);
	pDot11dInfo->State = DOT11D_STATE_LEARNED;

#if defined CONFIG_CRDA && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)  
   	queue_delayed_work_rsl(dev->wq, &dev->softmac_hint11d_wq, 0);
#endif	
}

u8 DOT11D_GetMaxTxPwrInDbm( struct rtllib_device *dev, u8 Channel)
{
	PRT_DOT11D_INFO pDot11dInfo = GET_DOT11D_INFO(dev);
	u8 MaxTxPwrInDbm = 255;

	if(MAX_CHANNEL_NUMBER < Channel)
	{ 
		printk("DOT11D_GetMaxTxPwrInDbm(): Invalid Channel\n");
		return MaxTxPwrInDbm; 
	}
	if(pDot11dInfo->channel_map[Channel])
	{
		MaxTxPwrInDbm = pDot11dInfo->MaxTxPwrDbmList[Channel];	
	}

	return MaxTxPwrInDbm;
}

void DOT11D_ScanComplete( struct rtllib_device * dev)
{
	PRT_DOT11D_INFO pDot11dInfo = GET_DOT11D_INFO(dev);

	switch(pDot11dInfo->State)
	{
	case DOT11D_STATE_LEARNED:
		pDot11dInfo->State = DOT11D_STATE_DONE;
		break;

	case DOT11D_STATE_DONE:
		{ 
			Dot11d_Reset(dev); 
		}
		break;
	case DOT11D_STATE_NONE:
		break;
	}
}

int ToLegalChannel( struct rtllib_device * dev, u8 channel)
{
	PRT_DOT11D_INFO pDot11dInfo = GET_DOT11D_INFO(dev);
	u8 default_chn = 0;
	u32 i = 0;

	for (i=1; i<= MAX_CHANNEL_NUMBER; i++)
	{
		if(pDot11dInfo->channel_map[i] > 0)
		{
			default_chn = i;
			break;
		}
	}

	if(MAX_CHANNEL_NUMBER < channel)
	{ 
		printk("%s(): Invalid Channel\n", __FUNCTION__);
		return default_chn; 
	}
	
	if(pDot11dInfo->channel_map[channel] > 0)
		return channel;
	
	return default_chn;
}

#ifndef BUILT_IN_RTLLIB
EXPORT_SYMBOL_RSL(Dot11d_Init);
EXPORT_SYMBOL_RSL(Dot11d_Channelmap);
EXPORT_SYMBOL_RSL(Dot11d_Reset);
EXPORT_SYMBOL_RSL(Dot11d_UpdateCountryIe);
EXPORT_SYMBOL_RSL(DOT11D_GetMaxTxPwrInDbm);
EXPORT_SYMBOL_RSL(DOT11D_ScanComplete);
EXPORT_SYMBOL_RSL(ToLegalChannel);
#endif

#endif
