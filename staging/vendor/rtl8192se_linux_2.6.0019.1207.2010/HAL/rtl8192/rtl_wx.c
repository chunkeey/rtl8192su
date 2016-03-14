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

#include <linux/string.h>
#include "rtl_core.h"
#ifdef ENABLE_DOT11D
#include "../../rtllib/dot11d.h"
#endif
#ifdef _RTL8192_EXT_PATCH_
#include "../../mshclass/msh_class.h"
#endif
#ifdef ASL
#include "rtl_softap.h"
#endif
#ifdef CONFIG_MP
#include "r8192S_mp.h"
#endif

#define RATE_COUNT 12
u32 rtl8192_rates[] = {1000000,2000000,5500000,11000000,
	6000000,9000000,12000000,18000000,24000000,36000000,48000000,54000000};
	

#ifndef ENETDOWN
#define ENETDOWN 1
#endif
extern int  hwwep;
#ifdef _RTL8192_EXT_PATCH_
int r8192_wx_set_channel(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int r8192_wx_mesh_scan(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int r8192_wx_get_mesh_list(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
#endif

static int r8192_wx_get_freq(struct net_device *dev,
			     struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
#ifdef _RTL8192_EXT_PATCH_
	return rtllib_wx_get_freq(priv->rtllib,a,wrqu,b,0);
#else
#ifdef ASL
	return rtllib_wx_get_freq(priv->rtllib,a,wrqu,b, 0);
#else
	return rtllib_wx_get_freq(priv->rtllib,a,wrqu,b);
#endif
#endif
}


#if 0

static int r8192_wx_set_beaconinterval(struct net_device *dev, struct iw_request_info *aa,
			  union iwreq_data *wrqu, char *b)
{
	int *parms = (int *)b;
	int bi = parms[0];
	
	struct r8192_priv *priv = rtllib_priv(dev);
	
	down(&priv->wx_sem);
	DMESG("setting beacon interval to %x",bi);
	
	priv->rtllib->beacon_interval=bi;
	rtl8192_commit(dev);
	up(&priv->wx_sem);
		
	return 0;	
}


static int r8192_wx_set_forceassociate(struct net_device *dev, struct iw_request_info *aa,
			  union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv=rtllib_priv(dev);	
	int *parms = (int *)extra;
	
	priv->rtllib->force_associate = (parms[0] > 0);
	

	return 0;
}

#endif
static int r8192_wx_get_mode(struct net_device *dev, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	struct r8192_priv *priv=rtllib_priv(dev);	

	return rtllib_wx_get_mode(priv->rtllib,a,wrqu,b);
}

static int r8192_wx_get_rate(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	return rtllib_wx_get_rate(priv->rtllib,info,wrqu,extra);
}



static int r8192_wx_set_rate(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	int ret;
	struct r8192_priv *priv = rtllib_priv(dev);	
	
	if(priv->bHwRadioOff == true)
		return 0;	
	
	down(&priv->wx_sem);

	ret = rtllib_wx_set_rate(priv->rtllib,info,wrqu,extra);
	
	up(&priv->wx_sem);
	
	return ret;
}


static int r8192_wx_set_rts(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	int ret;
	struct r8192_priv *priv = rtllib_priv(dev);	
	
	if(priv->bHwRadioOff == true)
		return 0;	
	
	down(&priv->wx_sem);

	ret = rtllib_wx_set_rts(priv->rtllib,info,wrqu,extra);
	
	up(&priv->wx_sem);
	
	return ret;
}

static int r8192_wx_get_rts(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	return rtllib_wx_get_rts(priv->rtllib,info,wrqu,extra);
}

static int r8192_wx_set_power(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	int ret;
	struct r8192_priv *priv = rtllib_priv(dev);	
	
	if(priv->bHwRadioOff == true){
		RT_TRACE(COMP_ERR,"%s():Hw is Radio Off, we can't set Power,return\n",__FUNCTION__);
		return 0;
	}
	down(&priv->wx_sem);

	ret = rtllib_wx_set_power(priv->rtllib,info,wrqu,extra);
	
	up(&priv->wx_sem);
	
	return ret;
}

static int r8192_wx_get_power(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	return rtllib_wx_get_power(priv->rtllib,info,wrqu,extra);
}

static int r8192_wx_set_rawtx(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	int ret;
	
	if(priv->bHwRadioOff == true)
		return 0;
	
	down(&priv->wx_sem);
	
	ret = rtllib_wx_set_rawtx(priv->rtllib, info, wrqu, extra);
	
	up(&priv->wx_sem);
	
	return ret;
	 
}

static int r8192_wx_force_reset(struct net_device *dev,
		struct iw_request_info *info,
		union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	down(&priv->wx_sem);

	printk("%s(): force reset ! extra is %d\n",__FUNCTION__, *extra);
	priv->force_reset = *extra;
	up(&priv->wx_sem);
	return 0;

}

static int r8192_wx_force_mic_error(struct net_device *dev,
		struct iw_request_info *info,
		union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;
	
	down(&priv->wx_sem);

	printk("%s(): force mic error ! \n",__FUNCTION__);
	ieee->force_mic_error = true;
	up(&priv->wx_sem);
	return 0;

}
static int r8192_wx_set_wmm_uapsd(struct net_device *dev,
		struct iw_request_info *info,
		union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	down(&priv->wx_sem);

	printk("%s(): set wmm uapsd ! extra is %d\n",__FUNCTION__, *extra);
	if (*extra)
		priv->rtllib->b4ac_Uapsd = 0x0F;
	else
		priv->rtllib->b4ac_Uapsd = 0x00;
	    	
	if (priv->rtllib->b4ac_Uapsd & 0x0F) {
		priv->rtllib->qos_capability |= QOS_WMM_UAPSD;
	} else {
		priv->rtllib->qos_capability &= ~QOS_WMM_UAPSD;
	}
	priv->rtllib->state = RTLLIB_ASSOCIATING;

	RemovePeerTS(priv->rtllib,priv->rtllib->current_network.bssid);
	priv->rtllib->is_set_key = false;
	priv->rtllib->link_change(dev);
	if(priv->rtllib->LedControlHandler)
		priv->rtllib->LedControlHandler(priv->rtllib->dev, LED_CTL_START_TO_LINK); 

	notify_wx_assoc_event(priv->rtllib); 

	if(!(priv->rtllib->rtllib_ap_sec_type(priv->rtllib) & (SEC_ALG_CCMP|SEC_ALG_TKIP)))
		queue_delayed_work_rsl(priv->rtllib->wq, &priv->rtllib->associate_procedure_wq, 0);

	up(&priv->wx_sem);
	return 0;

}
#define MAX_ADHOC_PEER_NUM 64 

typedef struct 
{
	unsigned char MacAddr[ETH_ALEN];
	unsigned char WirelessMode;
	unsigned char bCurTxBW40MHz;		
} adhoc_peer_entry_t, *p_adhoc_peer_entry_t;

typedef struct 
{
	adhoc_peer_entry_t Entry[MAX_ADHOC_PEER_NUM];
	unsigned char num;
} adhoc_peers_info_t, *p_adhoc_peers_info_t;

int r8192_wx_get_adhoc_peers(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
#ifndef RTL8192SE 
	return 0;
#else
	struct r8192_priv *priv = rtllib_priv(dev);
	struct sta_info * psta = NULL;
	adhoc_peers_info_t adhoc_peers_info;
	p_adhoc_peers_info_t  padhoc_peers_info = &adhoc_peers_info; 
	p_adhoc_peer_entry_t padhoc_peer_entry = NULL;
	int k=0;


	memset(extra, 0, 1024);
	padhoc_peers_info->num = 0;

	down(&priv->wx_sem);

	for(k=0; k<PEER_MAX_ASSOC; k++)
	{
		psta = priv->rtllib->peer_assoc_list[k];
		if(NULL != psta)
		{
			if(1024 - strlen(extra) < 100){
				printk("====> we can not show so many peers\n");
				break;
			}
					
			padhoc_peer_entry = &padhoc_peers_info->Entry[padhoc_peers_info->num];
			memset(padhoc_peer_entry,0, sizeof(adhoc_peer_entry_t));
			memcpy(padhoc_peer_entry->MacAddr, psta->macaddr, ETH_ALEN);
			padhoc_peer_entry->WirelessMode = psta->wireless_mode;
			padhoc_peer_entry->bCurTxBW40MHz = psta->htinfo.bCurTxBW40MHz;
			padhoc_peers_info->num ++;
			printk("[%d] MacAddr:"MAC_FMT" \tWirelessMode:%d \tBW40MHz:%d \n", \
				k, MAC_ARG(padhoc_peer_entry->MacAddr), padhoc_peer_entry->WirelessMode, padhoc_peer_entry->bCurTxBW40MHz);
			sprintf(extra, "[%d] MacAddr:"MAC_FMT" \tWirelessMode:%d \tBW40MHz:%d \n",  \
				k, MAC_ARG(padhoc_peer_entry->MacAddr), padhoc_peer_entry->WirelessMode, padhoc_peer_entry->bCurTxBW40MHz);
		}
	}

	up(&priv->wx_sem);

	wrqu->data.length = strlen(extra);
	wrqu->data.flags = 0;
	return 0;

#endif
}


static int r8191se_wx_get_firm_version(struct net_device *dev,
		struct iw_request_info *info,
		struct iw_param *wrqu, char *extra)
{
#if defined RTL8192SE || defined RTL8192CE
        struct r8192_priv *priv = rtllib_priv(dev);
	u16 firmware_version;

	down(&priv->wx_sem);
	printk("%s(): Just Support 92SE tmp\n", __FUNCTION__);
#ifdef RTL8192CE
	firmware_version = priv->firmware_version;
#else
	firmware_version = priv->pFirmware->FirmwareVersion;
#endif
	wrqu->value = firmware_version;
	wrqu->fixed = 1;

	up(&priv->wx_sem);
#endif
	return 0;
}

static int r8192_wx_adapter_power_status(struct net_device *dev,
		struct iw_request_info *info,
		union iwreq_data *wrqu, char *extra)
{
#ifdef ENABLE_LPS
	struct r8192_priv *priv = rtllib_priv(dev);
	PRT_POWER_SAVE_CONTROL pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));
	struct rtllib_device* ieee = priv->rtllib;

	down(&priv->wx_sem);

	RT_TRACE(COMP_POWER, "%s(): %s\n",__FUNCTION__, (*extra ==  6)?"DC power":"AC power");
	if(*extra || priv->force_lps) {
		priv->ps_force = false;
		pPSC->bLeisurePs = true;
	} else {	
		if(priv->rtllib->state == RTLLIB_LINKED)
			LeisurePSLeave(dev);
	
		priv->ps_force = true;
		pPSC->bLeisurePs = false;
		ieee->ps = *extra;	
	}

	up(&priv->wx_sem);
#endif

	return 0;

}

#ifdef _RTL8192_EXT_PATCH_
static int r8192_wx_print_reg(struct net_device *dev,
		struct iw_request_info *info,
		union iwreq_data *wrqu, char *extra)
{
	u8 reg1=0,reg2=0,reg3=0,reg4=0;
	u32 reg5 = 0, len = 0;
	
	memset(extra, 0, 512);
	sprintf(extra,"\nStart Log: Set 0x08000000 to 0x230\n");
	write_nic_dword(dev, 0x230 , 0x08000000);
	reg1 = read_nic_byte(dev, 0xf0);
	reg2 = read_nic_byte(dev, 0xf4);
	reg3 = read_nic_byte(dev, 0x140);
	reg4 = read_nic_byte(dev, 0x124);
	mdelay(10);
	reg5 = read_nic_dword(dev, 0x230);
	len = strlen(extra);
	sprintf(extra+len, "0xf0: %2.2x\n0xf4: %2.2x\n0x140: %2.2x\n0x124: %2.2x\n", reg1,reg2,reg3,reg4);
	len = strlen(extra);
	sprintf(extra+len,"After delay 10ms, read 0x230: %8.8x\n", reg5);

	write_nic_dword(dev, 0x230 , 0x40000000);
	reg5 = read_nic_dword(dev, 0x230);
	len = strlen(extra);
	sprintf(extra+len,"Set 0x40000000 to 0x230. Read 0x230: %8.8x\n", reg5);

	write_nic_dword(dev, 0x230 , 0x80000000);
	reg5 = read_nic_dword(dev, 0x230);
	len = strlen(extra);
	sprintf(extra+len,"Set 0x80000000 to 0x230. Read 0x230: %8.8x\n", reg5);

	wrqu->data.length = strlen(extra);
	return 0;
}

static int r8192_wx_resume_firm(struct net_device *dev,
		struct iw_request_info *info,
		union iwreq_data *wrqu, char *extra)
{

	write_nic_byte(dev, 0x42, 0xFF);
	write_nic_word(dev, 0x40, 0x77FC);
	write_nic_word(dev, 0x40, 0x57FC);
	write_nic_word(dev, 0x40, 0x37FC);
	write_nic_word(dev, 0x40, 0x77FC);
	
	udelay(100);

	write_nic_word(dev, 0x40, 0x57FC);
	write_nic_word(dev, 0x40, 0x37FC);
	write_nic_byte(dev, 0x42, 0x00);

	return 0;
}
#endif
static int r8192se_wx_set_radio(struct net_device *dev,
        struct iw_request_info *info,
        union iwreq_data *wrqu, char *extra)
{
    struct r8192_priv *priv = rtllib_priv(dev);

    down(&priv->wx_sem);

    printk("%s(): set radio ! extra is %d\n",__FUNCTION__, *extra);
    if((*extra != 0) && (*extra != 1))
    {
        RT_TRACE(COMP_ERR, "%s(): set radio an err value,must 0(radio off) or 1(radio on)\n",__FUNCTION__);
        return -1;
    }
    priv->sw_radio_on = *extra;
    up(&priv->wx_sem);
    return 0;

}

static int r8192se_wx_set_lps_awake_interval(struct net_device *dev,
        struct iw_request_info *info,
        union iwreq_data *wrqu, char *extra)
{
    struct r8192_priv *priv = rtllib_priv(dev);
    PRT_POWER_SAVE_CONTROL	pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));

    down(&priv->wx_sem);

    printk("%s(): set lps awake interval ! extra is %d\n",__FUNCTION__, *extra);

    pPSC->RegMaxLPSAwakeIntvl = *extra;
    up(&priv->wx_sem);
    return 0;

}

static int r8192se_wx_set_force_lps(struct net_device *dev,
		struct iw_request_info *info,
		union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	down(&priv->wx_sem);

	printk("%s(): force LPS ! extra is %d (1 is open 0 is close)\n",__FUNCTION__, *extra);
	priv->force_lps = *extra;
	up(&priv->wx_sem);
	return 0;

}

#ifdef _RTL8192_EXT_PATCH_
static int r8192_wx_get_drv_version(struct net_device *dev,
		struct iw_request_info *info,
		union iwreq_data *wrqu, char *extra)
{
	memset(extra, 0, 64);
	sprintf(extra, "Support Mesh");

	((struct iw_point *)wrqu)->length = strlen(extra);
	return 0;
}
#endif

static int r8192_wx_set_debugflag(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8 c = *extra;
	
	if(priv->bHwRadioOff == true)
		return 0;
	
	printk("=====>%s(), *extra:%x, debugflag:%x\n", __FUNCTION__, *extra, rt_global_debug_component);
	if (c > 0)  {
		rt_global_debug_component |= (1<<c);
	} else {
		rt_global_debug_component &= BIT31; 
	}
	return 0;
}

static int r8192_wx_set_mode(struct net_device *dev, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	struct r8192_priv *priv = rtllib_priv(dev);
#ifdef ENABLE_IPS	
	struct rtllib_device* ieee = netdev_priv_rsl(dev);
#endif

	RT_RF_POWER_STATE	rtState;
	int ret;

	if(priv->bHwRadioOff == true)
		return 0;
#ifdef _RTL8192_EXT_PATCH_
	if (priv->mshobj && (priv->rtllib->iw_mode==IW_MODE_MESH)) {
		return 0;	
	}
#endif
#ifdef ASL
	if (priv->rtllib->iw_mode == IW_MODE_APSTA) {
		printk("==>%s(): It is APSTA mode , can't change mode\n",__func__);
		return 0;
	}
#endif
	rtState = priv->rtllib->eRFPowerState;
	down(&priv->wx_sem);
#ifdef ENABLE_IPS	
	if(wrqu->mode == IW_MODE_ADHOC || wrqu->mode == IW_MODE_MONITOR
#ifdef ASL
		|| wrqu->mode == IW_MODE_MASTER || wrqu->mode == IW_MODE_APSTA
#endif
		|| ieee->bNetPromiscuousMode )
	{
		if(priv->rtllib->PowerSaveControl.bInactivePs){ 
			if(rtState == eRfOff){
				if(priv->rtllib->RfOffReason > RF_CHANGE_BY_IPS)
				{
					RT_TRACE(COMP_ERR, "%s(): RF is OFF.\n",__FUNCTION__);
					up(&priv->wx_sem);
					return -1;
				} else {
					printk("=========>%s(): IPSLeave\n",__FUNCTION__);
					down(&priv->rtllib->ips_sem);
					IPSLeave(dev);
					up(&priv->rtllib->ips_sem);				
				}
			}
		}
	}
#endif
	ret = rtllib_wx_set_mode(priv->rtllib,a,wrqu,b);

	up(&priv->wx_sem);
	return ret;
}
#if defined (RTL8192S_WAPI_SUPPORT)
int wapi_ioctl_set_mode(struct net_device *dev, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	RT_RF_POWER_STATE	rtState;
	int ret;

	printk("===============================>%s\n", __FUNCTION__);	
	if(priv->bHwRadioOff == true)
		return 0;
#ifdef _RTL8192_EXT_PATCH_
	if (priv->mshobj && (priv->rtllib->iw_mode==IW_MODE_MESH)) {
		return 0;	
	}
#endif
#ifdef ASL
	if (priv->rtllib->iw_mode == IW_MODE_APSTA) {
		printk("======>%s(): it is AP mode ,cant change mode\n",__func__);
		return 0;
	}
#endif
	rtState = priv->rtllib->eRFPowerState;
#ifdef ENABLE_IPS	
	if(wrqu->mode == IW_MODE_ADHOC){
		if(priv->rtllib->PowerSaveControl.bInactivePs){ 
			if(rtState == eRfOff){
				if(priv->rtllib->RfOffReason > RF_CHANGE_BY_IPS)
				{
					RT_TRACE(COMP_ERR, "%s(): RF is OFF.\n",__FUNCTION__);
					up(&priv->wx_sem);
					return -1;
				} else {
					printk("=========>%s(): IPSLeave\n",__FUNCTION__);
					down(&priv->rtllib->ips_sem);
					IPSLeave(dev);
					up(&priv->rtllib->ips_sem);				
				}
			}
		}
	}
#endif
	ret = rtllib_wx_set_mode(priv->rtllib,a,wrqu,b);

	return ret;
}
#endif
struct  iw_range_with_scan_capa
{
        /* Informative stuff (to choose between different interface) */
        __u32           throughput;     /* To give an idea... */
        /* In theory this value should be the maximum benchmarked
         * TCP/IP throughput, because with most of these devices the
         * bit rate is meaningless (overhead an co) to estimate how
         * fast the connection will go and pick the fastest one.
         * I suggest people to play with Netperf or any benchmark...
         */

        /* NWID (or domain id) */
        __u32           min_nwid;       /* Minimal NWID we are able to set */
        __u32           max_nwid;       /* Maximal NWID we are able to set */

        /* Old Frequency (backward compat - moved lower ) */
        __u16           old_num_channels;
        __u8            old_num_frequency;

        /* Scan capabilities */
        __u8            scan_capa;       
};

static int rtl8192_wx_get_range(struct net_device *dev, 
				struct iw_request_info *info, 
				union iwreq_data *wrqu, char *extra)
{
	struct iw_range *range = (struct iw_range *)extra;
	struct r8192_priv *priv = rtllib_priv(dev);
	u16 val;
	int i;

	wrqu->data.length = sizeof(*range);
	memset(range, 0, sizeof(*range));

	/* ~130 Mb/s real (802.11n) */
	range->throughput = 130 * 1000 * 1000;     

	if(priv->rf_set_sens != NULL)
		range->sensitivity = priv->max_sens;	/* signal level threshold range */
	
	range->max_qual.qual = 100;
	range->max_qual.level = 0;
	range->max_qual.noise = 0;
	range->max_qual.updated = 7; /* Updated all three */

	range->avg_qual.qual = 70; /* > 8% missed beacons is 'bad' */
	range->avg_qual.level = 0;
	range->avg_qual.noise = 0;
	range->avg_qual.updated = 7; /* Updated all three */

	range->num_bitrates = min(RATE_COUNT, IW_MAX_BITRATES);
	
	for (i = 0; i < range->num_bitrates; i++) {
		range->bitrate[i] = rtl8192_rates[i];
	}
	
	range->max_rts = DEFAULT_RTS_THRESHOLD;
	range->min_frag = MIN_FRAG_THRESHOLD;
	range->max_frag = MAX_FRAG_THRESHOLD;
	
	range->min_pmp = 0;
	range->max_pmp = 5000000;
	range->min_pmt = 0;
	range->max_pmt = 65535*1000;	
	range->pmp_flags = IW_POWER_PERIOD;
	range->pmt_flags = IW_POWER_TIMEOUT;
	range->pm_capa = IW_POWER_PERIOD | IW_POWER_TIMEOUT | IW_POWER_ALL_R;
	range->we_version_compiled = WIRELESS_EXT;
	range->we_version_source = 18;

	for (i = 0, val = 0; i < 14; i++) {
		if ((priv->rtllib->active_channel_map)[i+1]) {
		        range->freq[val].i = i + 1;
			range->freq[val].m = rtllib_wlan_frequencies[i] * 100000;
			range->freq[val].e = 1;
			val++;
		} else {
		}
		
		if (val == IW_MAX_FREQUENCIES)
		break;
	}
	range->num_frequency = val;
	range->num_channels = val;
#if WIRELESS_EXT > 17
	range->enc_capa = IW_ENC_CAPA_WPA|IW_ENC_CAPA_WPA2|
			  IW_ENC_CAPA_CIPHER_TKIP|IW_ENC_CAPA_CIPHER_CCMP;
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25)
	{
		struct iw_range_with_scan_capa* tmp = (struct iw_range_with_scan_capa*)range;
		tmp->scan_capa = 0x01;
	}
#else
	range->scan_capa = IW_SCAN_CAPA_ESSID | IW_SCAN_CAPA_TYPE;
#endif

        /* Event capability (kernel + driver) */ 

	return 0;
}

static int r8192_wx_set_scan(struct net_device *dev, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;
	RT_RF_POWER_STATE	rtState;
	int ret;
	
#ifdef CONFIG_MP
	printk("######################%s(): In MP Test Can not Scan\n",__FUNCTION__);
	return 0;
#endif
	if (!(ieee->softmac_features & IEEE_SOFTMAC_SCAN)){
		if((ieee->state >= RTLLIB_ASSOCIATING) && (ieee->state <= RTLLIB_ASSOCIATING_AUTHENTICATED)){
			return 0;
		}
		if((priv->rtllib->state == RTLLIB_LINKED) && (priv->rtllib->CntAfterLink<2)){
			return 0;
		}
	}

	if(priv->bHwRadioOff == true){
		printk("================>%s(): hwradio off\n",__FUNCTION__);
		return 0;
	}
	rtState = priv->rtllib->eRFPowerState;
	if(!priv->up) return -ENETDOWN;
	if (priv->rtllib->LinkDetectInfo.bBusyTraffic == true)
		return -EAGAIN;

#ifdef _RTL8192_EXT_PATCH_	
	if((ieee->iw_mode == IW_MODE_MESH)&&(ieee->mesh_state == RTLLIB_MESH_LINKED)
					&&(ieee->only_mesh == 1))
	{
		return 0;
	}
#endif
#if WIRELESS_EXT > 17
	if (wrqu->data.flags & IW_SCAN_THIS_ESSID)
	{
		struct iw_scan_req* req = (struct iw_scan_req*)b;
		if (req->essid_len)
		{
			ieee->current_network.ssid_len = req->essid_len;
			memcpy(ieee->current_network.ssid, req->essid, req->essid_len); 
		}
	}
#endif	

	down(&priv->wx_sem);

	priv->rtllib->FirstIe_InScan = true;

	if(priv->rtllib->state != RTLLIB_LINKED){
#ifdef ENABLE_IPS
		if(priv->rtllib->PowerSaveControl.bInactivePs){ 
			if(rtState == eRfOff){
				if(priv->rtllib->RfOffReason > RF_CHANGE_BY_IPS){
					RT_TRACE(COMP_ERR, "%s(): RF is OFF.\n",__FUNCTION__);
					up(&priv->wx_sem);
					return -1;
				}else{
					RT_TRACE(COMP_PS, "=========>%s(): IPSLeave\n",__FUNCTION__);
					down(&priv->rtllib->ips_sem);
					IPSLeave(dev);
					up(&priv->rtllib->ips_sem);				
				}
			}
		}
#endif
		rtllib_stop_scan(priv->rtllib);
		if(priv->rtllib->LedControlHandler)
			priv->rtllib->LedControlHandler(dev, LED_CTL_SITE_SURVEY);
		
                if(priv->rtllib->eRFPowerState != eRfOff){
			priv->rtllib->actscanning = true;

			if(ieee->ScanOperationBackupHandler)
				ieee->ScanOperationBackupHandler(ieee->dev,SCAN_OPT_BACKUP);
	
			rtllib_start_scan_syncro(priv->rtllib, 0);

			if(ieee->ScanOperationBackupHandler)
				ieee->ScanOperationBackupHandler(ieee->dev,SCAN_OPT_RESTORE);
                }
		ret = 0;
	} else {
		priv->rtllib->actscanning = true;
	        ret = rtllib_wx_set_scan(priv->rtllib,a,wrqu,b);
	}
	
	up(&priv->wx_sem);
	return ret;
}


static int r8192_wx_get_scan(struct net_device *dev, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{

	int ret;
	struct r8192_priv *priv = rtllib_priv(dev);
	
	if(!priv->up) return -ENETDOWN;
			
	if(priv->bHwRadioOff == true)
		return 0;
        
			
	down(&priv->wx_sem);

	ret = rtllib_wx_get_scan(priv->rtllib,a,wrqu,b);
		
	up(&priv->wx_sem);
	
	return ret;
}

static int r8192_wx_set_essid(struct net_device *dev, 
			      struct iw_request_info *a,
			      union iwreq_data *wrqu, char *b)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	int ret;
		
	if ((rtllib_act_scanning(priv->rtllib, false)) && !(priv->rtllib->softmac_features & IEEE_SOFTMAC_SCAN)){
		;
	}
#ifdef CONFIG_MP
	printk("######################%s(): In MP Test Can not Set Essid\n",__FUNCTION__);
	return 0;
#endif
	if(priv->bHwRadioOff == true){
		printk("=========>%s():hw radio off,or Rf state is eRfOff, return\n",__FUNCTION__);
		return 0;
	}
	down(&priv->wx_sem);
	ret = rtllib_wx_set_essid(priv->rtllib,a,wrqu,b);

	up(&priv->wx_sem);

	return ret;
}

static int r8192_wx_get_essid(struct net_device *dev, 
			      struct iw_request_info *a,
			      union iwreq_data *wrqu, char *b)
{
	int ret;
	struct r8192_priv *priv = rtllib_priv(dev);
	
	down(&priv->wx_sem);
	
	ret = rtllib_wx_get_essid(priv->rtllib, a, wrqu, b);

	up(&priv->wx_sem);
	
	return ret;
}

static int r8192_wx_set_nick(struct net_device *dev,
			   struct iw_request_info *info,
			   union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	if (wrqu->data.length > IW_ESSID_MAX_SIZE)
		return -E2BIG;
	down(&priv->wx_sem);
	wrqu->data.length = min((size_t) wrqu->data.length, sizeof(priv->nick));
	memset(priv->nick, 0, sizeof(priv->nick));
	memcpy(priv->nick, extra, wrqu->data.length);
	up(&priv->wx_sem);
	return 0;

}

static int r8192_wx_get_nick(struct net_device *dev,
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	down(&priv->wx_sem);
	wrqu->data.length = strlen(priv->nick);
	memcpy(extra, priv->nick, wrqu->data.length);
	wrqu->data.flags = 1;   /* active */
	up(&priv->wx_sem);
	return 0;
}

static int r8192_wx_set_freq(struct net_device *dev, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	int ret;
	struct r8192_priv *priv = rtllib_priv(dev);
	
	if(priv->bHwRadioOff == true)
		return 0;
	
	down(&priv->wx_sem);
	
	ret = rtllib_wx_set_freq(priv->rtllib, a, wrqu, b);
	
	up(&priv->wx_sem);
	return ret;
}

static int r8192_wx_get_name(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	return rtllib_wx_get_name(priv->rtllib, info, wrqu, extra);
}


static int r8192_wx_set_frag(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	if(priv->bHwRadioOff == true)
		return 0;
	
	if (wrqu->frag.disabled)
		priv->rtllib->fts = DEFAULT_FRAG_THRESHOLD;
	else {
		if (wrqu->frag.value < MIN_FRAG_THRESHOLD ||
		    wrqu->frag.value > MAX_FRAG_THRESHOLD)
			return -EINVAL;
		
		priv->rtllib->fts = wrqu->frag.value & ~0x1;
	}

	return 0;
}


static int r8192_wx_get_frag(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	wrqu->frag.value = priv->rtllib->fts;
	wrqu->frag.fixed = 0;	/* no auto select */
	wrqu->frag.disabled = (wrqu->frag.value == DEFAULT_FRAG_THRESHOLD);

	return 0;
}


static int r8192_wx_set_wap(struct net_device *dev,
			 struct iw_request_info *info,
			 union iwreq_data *awrq,
			 char *extra)
{

	int ret;
	struct r8192_priv *priv = rtllib_priv(dev);
	
	if ((rtllib_act_scanning(priv->rtllib, false)) && !(priv->rtllib->softmac_features & IEEE_SOFTMAC_SCAN)){
		;
	}

	if(priv->bHwRadioOff == true)
		return 0;
	
#ifdef _RTL8192_EXT_PATCH_
	if (priv->mshobj && (priv->rtllib->iw_mode==IW_MODE_MESH)){
		return 0;
	}
#endif
	down(&priv->wx_sem);
	
	ret = rtllib_wx_set_wap(priv->rtllib,info,awrq,extra);

	up(&priv->wx_sem);

	return ret;
	
}
	

static int r8192_wx_get_wap(struct net_device *dev, 
			    struct iw_request_info *info, 
			    union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	return rtllib_wx_get_wap(priv->rtllib,info,wrqu,extra);
}


static int r8192_wx_get_enc(struct net_device *dev, 
			    struct iw_request_info *info, 
			    union iwreq_data *wrqu, char *key)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
#ifdef _RTL8192_EXT_PATCH_
	return rtllib_wx_get_encode(priv->rtllib, info, wrqu, key,0);
#else
	return rtllib_wx_get_encode(priv->rtllib, info, wrqu, key);
#endif
}

static int r8192_wx_set_enc(struct net_device *dev, 
			    struct iw_request_info *info, 
			    union iwreq_data *wrqu, char *key)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	int ret;

	struct rtllib_device *ieee = priv->rtllib;
	u32 hwkey[4]={0,0,0,0};
	u8 mask=0xff;
	u32 key_idx=0;
	u8 zero_addr[4][6] ={	{0x00,0x00,0x00,0x00,0x00,0x00},
				{0x00,0x00,0x00,0x00,0x00,0x01}, 
				{0x00,0x00,0x00,0x00,0x00,0x02}, 
				{0x00,0x00,0x00,0x00,0x00,0x03} };
	int i;

	if ((rtllib_act_scanning(priv->rtllib, false)) && !(priv->rtllib->softmac_features & IEEE_SOFTMAC_SCAN)){
		;
	}
#ifdef CONFIG_MP
	printk("######################%s(): In MP Test Can not Set Enc\n",__FUNCTION__);
	return 0;
#endif
	if(priv->bHwRadioOff == true)
		return 0;
	
       if(!priv->up) return -ENETDOWN;

        priv->rtllib->wx_set_enc = 1;
#ifdef ENABLE_IPS
        down(&priv->rtllib->ips_sem);
        IPSLeave(dev);
        up(&priv->rtllib->ips_sem);			
#endif
	down(&priv->wx_sem);
	
	RT_TRACE(COMP_SEC, "Setting SW wep key");
#ifdef _RTL8192_EXT_PATCH_
	ret = rtllib_wx_set_encode(priv->rtllib,info,wrqu,key,0);
#else	
	ret = rtllib_wx_set_encode(priv->rtllib,info,wrqu,key);
#endif
	up(&priv->wx_sem);


	if (wrqu->encoding.flags & IW_ENCODE_DISABLED) {
		ieee->pairwise_key_type = ieee->group_key_type = KEY_TYPE_NA;
		CamResetAllEntry(dev);
#ifdef _RTL8192_EXT_PATCH_
		CamRestoreEachIFEntry(dev,1, 0);
		reset_IFswcam(dev,0,0);
		priv->rtllib->wx_set_enc = 0;
#else
#ifdef ASL
		CamRestoreEachIFEntry(dev,0, 1);
		reset_IFswcam(dev,0,0);
#else
		memset(priv->rtllib->swcamtable,0,sizeof(SW_CAM_TABLE)*32);
#endif
#endif
		goto end_hw_sec;
	}
	if(wrqu->encoding.length!=0){

		for(i=0 ; i<4 ; i++){
			hwkey[i] |=  key[4*i+0]&mask;
			if(i==1&&(4*i+1)==wrqu->encoding.length) mask=0x00;
			if(i==3&&(4*i+1)==wrqu->encoding.length) mask=0x00;
			hwkey[i] |= (key[4*i+1]&mask)<<8;
			hwkey[i] |= (key[4*i+2]&mask)<<16;
			hwkey[i] |= (key[4*i+3]&mask)<<24;
		}

		#define CONF_WEP40  0x4
		#define CONF_WEP104 0x14

		switch(wrqu->encoding.flags & IW_ENCODE_INDEX){
			case 0: key_idx = ieee->tx_keyidx; break;
			case 1:	key_idx = 0; break;
			case 2:	key_idx = 1; break;
			case 3:	key_idx = 2; break;
			case 4:	key_idx	= 3; break;
			default: break;
		}
		if(wrqu->encoding.length==0x5){
		ieee->pairwise_key_type = KEY_TYPE_WEP40;
			EnableHWSecurityConfig8192(dev);
#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
#else
			setKey( dev,
				key_idx,                
				key_idx,                
				KEY_TYPE_WEP40,         
				zero_addr[key_idx],
				0,                      
				hwkey);                 
			set_swcam( dev,
				key_idx,                
				key_idx,                
				KEY_TYPE_WEP40,         
				zero_addr[key_idx],
				0,                      
				hwkey,                 
				0,				
				0);				

#endif
		}

		else if(wrqu->encoding.length==0xd){
			ieee->pairwise_key_type = KEY_TYPE_WEP104;
				EnableHWSecurityConfig8192(dev);
#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
#else
			setKey( dev,
				key_idx,                
				key_idx,                
				KEY_TYPE_WEP104,        
				zero_addr[key_idx],
				0,                      
				hwkey);                 
			set_swcam( dev,
				key_idx,                
				key_idx,                
				KEY_TYPE_WEP104,        
				zero_addr[key_idx],
				0,                      
				hwkey,                 
				0,				
				0);				
#endif
		}
		else printk("wrong type in WEP, not WEP40 and WEP104\n");
#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
		if(ieee->state == RTLLIB_LINKED){
			if(ieee->iw_mode == IW_MODE_ADHOC)
			{
				
				setKey( dev,
						key_idx,                      
						key_idx,                      
						ieee->pairwise_key_type,        
						zero_addr[key_idx],		
						0,                      
						hwkey);                 
				set_swcam( dev,
						key_idx,                      
						key_idx,                      
						ieee->pairwise_key_type,        
						zero_addr[key_idx],		
						0,                      
						hwkey,               
						0,				
						0);				
			}
			else{
			setKey( dev,
					31,                      
					key_idx,                      
						ieee->pairwise_key_type,        
					ieee->ap_mac_addr,         
					0,                      
					hwkey);                 
			set_swcam( dev,
					31,                      
					key_idx,                      
						ieee->pairwise_key_type,        
					ieee->ap_mac_addr,         
					0,                      
					hwkey,               
					0,				
					0);				
		}
	}
#endif
	}

#if 0
	if(wrqu->encoding.length==0 && (wrqu->encoding.flags >>8) == 0x8 ){
		printk("===>1\n");		
		EnableHWSecurityConfig8192(dev);
		key_idx = (wrqu->encoding.flags & 0xf)-1 ;
		write_cam(dev, (4*6),   0xffff0000|read_cam(dev, key_idx*6) );
		write_cam(dev, (4*6)+1, 0xffffffff);
		write_cam(dev, (4*6)+2, read_cam(dev, (key_idx*6)+2) );
		write_cam(dev, (4*6)+3, read_cam(dev, (key_idx*6)+3) );
		write_cam(dev, (4*6)+4, read_cam(dev, (key_idx*6)+4) );
		write_cam(dev, (4*6)+5, read_cam(dev, (key_idx*6)+5) );
	}
#endif
#ifdef _RTL8192_EXT_PATCH_
	priv->rtllib->wx_set_enc = 0;
	printk("===================>%s():set ieee->wx_set_enc 0\n",__FUNCTION__);
end_hw_sec:
#else
end_hw_sec:
	priv->rtllib->wx_set_enc = 0;
#endif
	return ret;
}


static int r8192_wx_set_scan_type(struct net_device *dev, struct iw_request_info *aa, union
 iwreq_data *wrqu, char *p){
  
 	struct r8192_priv *priv = rtllib_priv(dev);
	int *parms=(int*)p;
	int mode=parms[0];
	
	if(priv->bHwRadioOff == true)
		return 0;
	
	priv->rtllib->active_scan = mode;
	
	return 1;
}



#define R8192_MAX_RETRY 255
static int r8192_wx_set_retry(struct net_device *dev, 
				struct iw_request_info *info, 
				union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	int err = 0;
	
	if(priv->bHwRadioOff == true)
		return 0;
	
	down(&priv->wx_sem);
	
	if (wrqu->retry.flags & IW_RETRY_LIFETIME || 
	    wrqu->retry.disabled){
		err = -EINVAL;
		goto exit;
	}
	if (!(wrqu->retry.flags & IW_RETRY_LIMIT)){
		err = -EINVAL;
		goto exit;
	}

	if(wrqu->retry.value > R8192_MAX_RETRY){
		err= -EINVAL;
		goto exit;
	}
	if (wrqu->retry.flags & IW_RETRY_MAX) {
		priv->retry_rts = wrqu->retry.value;
		DMESG("Setting retry for RTS/CTS data to %d", wrqu->retry.value);
	
	}else {
		priv->retry_data = wrqu->retry.value;
		DMESG("Setting retry for non RTS/CTS data to %d", wrqu->retry.value);
	}
	

 	rtl8192_commit(dev);
	/*
	if(priv->up){
		rtl8180_halt_adapter(dev);
		rtl8180_rx_enable(dev);
		rtl8180_tx_enable(dev);
			
	}
	*/
exit:
	up(&priv->wx_sem);
	
	return err;
}

static int r8192_wx_get_retry(struct net_device *dev, 
				struct iw_request_info *info, 
				union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	

	wrqu->retry.disabled = 0; /* can't be disabled */

	if ((wrqu->retry.flags & IW_RETRY_TYPE) == 
	    IW_RETRY_LIFETIME) 
		return -EINVAL;
	
	if (wrqu->retry.flags & IW_RETRY_MAX) {
		wrqu->retry.flags = IW_RETRY_LIMIT & IW_RETRY_MAX;
		wrqu->retry.value = priv->retry_rts;
	} else {
		wrqu->retry.flags = IW_RETRY_LIMIT & IW_RETRY_MIN;
		wrqu->retry.value = priv->retry_data;
	}
	

	return 0;
}

static int r8192_wx_get_sens(struct net_device *dev, 
				struct iw_request_info *info, 
				union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	if(priv->rf_set_sens == NULL) 
		return -1; /* we have not this support for this radio */
	wrqu->sens.value = priv->sens;
	return 0;
}


static int r8192_wx_set_sens(struct net_device *dev, 
				struct iw_request_info *info, 
				union iwreq_data *wrqu, char *extra)
{
	
	struct r8192_priv *priv = rtllib_priv(dev);
	
	short err = 0;
	
	if(priv->bHwRadioOff == true)
		return 0;
	
	down(&priv->wx_sem);
	if(priv->rf_set_sens == NULL) {
		err= -1; /* we have not this support for this radio */
		goto exit;
	}
	if(priv->rf_set_sens(dev, wrqu->sens.value) == 0)
		priv->sens = wrqu->sens.value;
	else
		err= -EINVAL;

exit:
	up(&priv->wx_sem);
	
	return err;
}

#if (WIRELESS_EXT >= 18)
#if 0
static int r8192_wx_get_enc_ext(struct net_device *dev,
                                        struct iw_request_info *info,
                                        union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	int ret = 0;
#ifdef _RTL8192_EXT_PATCH_
	ret = rtllib_wx_get_encode_ext(priv->rtllib, info, wrqu, extra,0);
#else
	ret = rtllib_wx_get_encode_ext(priv->rtllib, info, wrqu, extra);
#endif
	return ret;
}
#endif

#ifdef _RTL8192_EXT_PATCH_	
static int meshdev_set_key_for_linked_peers(struct net_device *dev, u8 KeyIndex,u16 KeyType, u32 *KeyContent );
/*
 * set key for mesh, not a wireless extension handler. 
 * place it here because of porting from r8192_wx_set_enc_ext().  
 */ 
int r8192_mesh_set_enc_ext(struct net_device *dev,
                        struct iw_point *encoding, struct iw_encode_ext *ext, u8 *addr)
{
	int ret=0;
	int i=0;
	u8 broadcast_addr[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;
	u8 entry_idx = 0;
	down(&priv->wx_sem);
	if(memcmp(addr,broadcast_addr,6))
	{
		if ((i=rtllib_find_MP(ieee, addr, 0)) < 0) 
		{
			i = rtllib_find_MP(ieee, addr, 1); 
			if (i<0) 
				return -1;
		}
	}
	ret = rtllib_mesh_set_encode_ext(ieee, encoding, ext, i);
	if ((-EINVAL == ret) || (-ENOMEM == ret)) {
		goto end_hw_sec;
	}
	{
#if 0		
		u8 zero[6] = {0};
#endif
		u32 key[4] = {0};
		u8 idx = 0, alg = 0, group = 0;
		if ((encoding->flags & IW_ENCODE_DISABLED) ||
			ext->alg == IW_ENCODE_ALG_NONE) 
		{
			CamResetAllEntry(dev);
			CamRestoreEachIFEntry(dev,0,0);
			reset_IFswcam(dev,1,0);
			goto end_hw_sec;
		}
		alg =  (ext->alg == IW_ENCODE_ALG_CCMP)?KEY_TYPE_CCMP:ext->alg; 
		idx = encoding->flags & IW_ENCODE_INDEX;
		if (idx)
			idx --;
		group = ext->ext_flags & IW_ENCODE_EXT_GROUP_KEY;

		if (!group)
		{
			ieee->mesh_pairwise_key_type = alg;
			EnableHWSecurityConfig8192(dev);	
		}
		
		memcpy((u8*)key, ext->key, 16); 

		if(group)
		{
			ieee->mesh_group_key_type = alg;
#if 0
			setKey( dev,
					idx,
					idx, 
					alg,  
					broadcast_addr, 
					0,              
					key);           
#endif
		}
		else 
		{
#if 0	
			if ((ieee->mesh_pairwise_key_type == KEY_TYPE_CCMP) && ieee->pHTInfo->bCurrentHTSupport){
				write_nic_byte(dev, 0x173, 1); 
			}
#endif			
			entry_idx = rtl8192_get_free_hwsec_cam_entry(ieee,addr);
#if 0 
			printk("%s(): Can't find  free hw security cam entry\n",__FUNCTION__);
			ret = -1;
#else
			if (entry_idx >= TOTAL_CAM_ENTRY-1) {
				printk("%s(): Can't find  free hw security cam entry\n",__FUNCTION__);
				ret = -1;
			} else {
				set_swcam( dev,
					entry_idx,		
					idx, 		
					alg,  		
					(u8*)addr, 	
					0,              
					key,		
					1,
					0);           	
				setKey( dev,
					entry_idx,		
					idx, 		
					alg,  		
					(u8*)addr, 	
					0,              
					key);           
				ret = 0;
			}
#endif
		}


	}

end_hw_sec:
	up(&priv->wx_sem);
#endif
	return ret;	

}
#endif
#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
static int r8192_set_hw_enc(struct net_device *dev,
                                        struct iw_request_info *info,
                                        union iwreq_data *wrqu, char *extra, u8 is_mesh)
{
	int ret=0;
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;
	u32 key[4] = {0};
	struct iw_encode_ext *ext = (struct iw_encode_ext *)extra;
	struct iw_point *encoding = &wrqu->encoding;
	u8 idx = 0, alg = 0;
	bool group = false;
	bool wep_nogroup = false;
	u8 broadcast_addr[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
	u8 zero[6] = {0};
        priv->rtllib->wx_set_enc = 1;
#ifdef ENABLE_IPS
        down(&priv->rtllib->ips_sem);
        IPSLeave(dev);
        up(&priv->rtllib->ips_sem);			
#endif
#if 0
	static u8 CAM_CONST_ADDR[4][6] = {
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x02},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x03}};
#endif
	if ((encoding->flags & IW_ENCODE_DISABLED) ||
		ext->alg == IW_ENCODE_ALG_NONE) 
	{
		if(is_mesh) {
#ifdef _RTL8192_EXT_PATCH_
			ieee->mesh_pairwise_key_type = ieee->mesh_pairwise_key_type = KEY_TYPE_NA;
#endif
		} else
			ieee->pairwise_key_type = ieee->group_key_type = KEY_TYPE_NA;
		CamResetAllEntry(dev);
		if(is_mesh) {
			CamRestoreEachIFEntry(dev, 0, 0);
		}
		else {
			CamRestoreEachIFEntry(dev, 1, 0);
			CamRestoreEachIFEntry(dev, 0, 1);
		}
		reset_IFswcam(dev,is_mesh,0);
		goto end_hw_sec;
	}
	alg =  (ext->alg == IW_ENCODE_ALG_CCMP)?KEY_TYPE_CCMP:ext->alg; 
	idx = encoding->flags & IW_ENCODE_INDEX;
	if (idx)
		idx --;
	
	group = (ext->ext_flags & IW_ENCODE_EXT_GROUP_KEY) > 0 ? true:false;
	printk("%s() group:%d, alg:%d, last_pairwise_key_type:%d, key_len:%d\n", 
			__func__, group, alg, ieee->pairwise_key_type, ext->key_len);

	
	if ((!is_mesh) && ((!group) || (IW_MODE_ADHOC == ieee->iw_mode) || (ieee->pairwise_key_type == KEY_TYPE_NA)))
	{
		if(ieee->pairwise_key_type == KEY_TYPE_NA && alg == KEY_TYPE_WEP40)
				wep_nogroup = true;
		
		if ((ext->key_len == 13) && (alg == KEY_TYPE_WEP40) )
			alg = KEY_TYPE_WEP104;
		
		ieee->pairwise_key_type = alg;
		EnableHWSecurityConfig8192(dev);
	}

#ifdef _RTL8192_EXT_PATCH_
	if (is_mesh && (IW_MODE_MESH == ieee->iw_mode)){
		ieee->mesh_pairwise_key_type = alg;
		EnableHWSecurityConfig8192(dev);
	}
#endif
	memcpy((u8*)key, ext->key, 16); 
	if (wep_nogroup && (ieee->auth_mode !=2)) 
	{
		printk("=====>set WEP key\n");
		if (ext->key_len == 13){
			if(is_mesh) {
#ifdef _RTL8192_EXT_PATCH_
				ieee->mesh_pairwise_key_type = alg = KEY_TYPE_WEP104;
#endif
			}
			else
				ieee->pairwise_key_type = alg = KEY_TYPE_WEP104;
		}
		if(ieee->iw_mode == IW_MODE_ADHOC){
			set_swcam( dev,
					idx,
					idx, 
					alg,  
					zero, 
					0,              
					key,		   
					is_mesh,   
					0);
			setKey( dev,
					idx,
					idx, 
					alg,  
					zero, 
					0,              
					key);           
		}
			
		if(!is_mesh){ 
			if(ieee->state == RTLLIB_LINKED){
				setKey( dev,
						31,                      
						idx,                      
						ieee->pairwise_key_type,        
						ieee->ap_mac_addr,         
						0,                      
						key);                 
				set_swcam( dev,
						31,                      
						idx,                      
						ieee->pairwise_key_type,        
						ieee->ap_mac_addr,         
						0,                      
						key,               
						0,			
						0);			
			}
		}
	}
	else if (group)
	{
		printk("set group key\n");

		if ((ext->key_len == 13) && (alg == KEY_TYPE_WEP40) )
				alg = KEY_TYPE_WEP104;
		
		if(is_mesh) {
#ifdef _RTL8192_EXT_PATCH_
			ieee->mesh_group_key_type = alg;
#endif
		}
		else
			ieee->group_key_type = alg;
		if(ieee->iw_mode == IW_MODE_ADHOC){
			set_swcam(  dev,
					idx,
					idx, 
					alg,  
					broadcast_addr, 
					0,              
					key,           
					is_mesh,	   
					0);		   
			setKey(  dev,
					idx,
					idx, 
					alg,  
					broadcast_addr, 
					0,              
					key);           
		}
#ifdef _RTL8192_EXT_PATCH_
		if(is_mesh)
			meshdev_set_key_for_linked_peers(dev,
				idx, 
				alg,  
				key); 
#endif
	}
	else 
	{
		printk("=============>set pairwise key\n");
#ifdef RTL8192E
		if ((ieee->pairwise_key_type == KEY_TYPE_CCMP) && ieee->pHTInfo->bCurrentHTSupport){
			write_nic_byte(dev, 0x173, 1); 
		}
#endif
		set_swcam( dev,
				31, 
				idx, 
				alg,  
				(u8*)ieee->ap_mac_addr, 
				0,              
				key,           
				is_mesh,	   
				0);		   
		setKey( dev,
				31, 
				idx, 
				alg,  
				(u8*)ieee->ap_mac_addr, 
				0,              
				key);           
	}

end_hw_sec:
        priv->rtllib->wx_set_enc = 0;
	return ret;	
}
static int r8192_wx_set_enc_ext(struct net_device *dev,
                                        struct iw_request_info *info,
                                        union iwreq_data *wrqu, char *extra)
{
	int ret=0;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;

	if(priv->bHwRadioOff == true)
		return 0;
	
	SEM_DOWN_PRIV_WX(&priv->wx_sem);	
#ifdef _RTL8192_EXT_PATCH_
	ret = rtllib_wx_set_encode_ext(ieee, info, wrqu, extra, 0);
#else
	ret = rtllib_wx_set_encode_ext(ieee, info, wrqu, extra);
#endif
	ret |= r8192_set_hw_enc(dev,info,wrqu,extra, 0);
	SEM_UP_PRIV_WX(&priv->wx_sem);	
#endif

	return ret;	
}
int rtl8192_set_key_for_AP(struct rtllib_device *ieee)
{
	struct rtllib_crypt_data **crypt;
	int key_len=0;
	char key[32]; 
	u16 keytype = IW_ENCODE_ALG_NONE;
	crypt = &ieee->sta_crypt[ieee->tx_keyidx];
	if (*crypt == NULL || (*crypt)->ops == NULL)
	{
		printk("%s():no encrypt now\n",__FUNCTION__);
		return 0;
	}
	if (!((*crypt)->ops->set_key && (*crypt)->ops->get_key)) 
		return -1;
	
	key_len = (*crypt)->ops->get_key(key, 32, NULL, (*crypt)->priv);
	if (strcmp((*crypt)->ops->name, "WEP") == 0 )
	{
		if(key_len == 5)
			keytype = KEY_TYPE_WEP40;
		else
			keytype = KEY_TYPE_WEP104;
	}
	else if (strcmp((*crypt)->ops->name, "TKIP") == 0)
		return 0;
	else if (strcmp((*crypt)->ops->name, "CCMP") == 0)
		return 0;

	set_swcam( ieee->dev,
				31,
				ieee->tx_keyidx, 
				keytype,  
				ieee->ap_mac_addr, 
				0,              
				(u32 *)key ,          
				0,
				0);
	setKey( ieee->dev,
			31,
			ieee->tx_keyidx, 
			keytype,  
			ieee->ap_mac_addr, 
			0,              
			(u32 *)key);           
	
	
	return 0;
}
#else
static int r8192_wx_set_enc_ext(struct net_device *dev,
                                        struct iw_request_info *info,
                                        union iwreq_data *wrqu, char *extra)
{
	int ret=0;
	
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;

	if(priv->bHwRadioOff == true)
		return 0;

	down(&priv->wx_sem);

        priv->rtllib->wx_set_enc = 1;
#ifdef ENABLE_IPS
        down(&priv->rtllib->ips_sem);
        IPSLeave(dev);
        up(&priv->rtllib->ips_sem);			
#endif

	ret = rtllib_wx_set_encode_ext(ieee, info, wrqu, extra);

	{
		u8 broadcast_addr[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
		u8 zero[6] = {0};
		u32 key[4] = {0};
		struct iw_encode_ext *ext = (struct iw_encode_ext *)extra;
		struct iw_point *encoding = &wrqu->encoding;
#if 0
		static u8 CAM_CONST_ADDR[4][6] = {
			{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
			{0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
			{0x00, 0x00, 0x00, 0x00, 0x00, 0x02},
			{0x00, 0x00, 0x00, 0x00, 0x00, 0x03}};
#endif
		u8 idx = 0, alg = 0;
		bool group = 0;
		bool wep_nogroup = false;
		if ((encoding->flags & IW_ENCODE_DISABLED) ||
		ext->alg == IW_ENCODE_ALG_NONE) 
		{
			ieee->pairwise_key_type = ieee->group_key_type = KEY_TYPE_NA;
			CamResetAllEntry(dev);
			memset(priv->rtllib->swcamtable,0,sizeof(SW_CAM_TABLE)*32);
			goto end_hw_sec;
		}
		alg =  (ext->alg == IW_ENCODE_ALG_CCMP)?KEY_TYPE_CCMP:ext->alg; 
		idx = encoding->flags & IW_ENCODE_INDEX;
		if (idx)
			idx --;
		
		group = (ext->ext_flags & IW_ENCODE_EXT_GROUP_KEY) > 0 ? true:false;
		printk("%s() group:%d, alg:%d, last_pairwise_key_type:%d, key_len:%d\n", 
			__func__, group, alg, ieee->pairwise_key_type, ext->key_len);

		if ((!group) || (IW_MODE_ADHOC == ieee->iw_mode) || ieee->pairwise_key_type == KEY_TYPE_NA)
		{
			if(ieee->pairwise_key_type == KEY_TYPE_NA && alg == KEY_TYPE_WEP40)
				wep_nogroup = true;

			if ((ext->key_len == 13) && (alg == KEY_TYPE_WEP40) )
				alg = KEY_TYPE_WEP104;

			ieee->pairwise_key_type = alg;
			EnableHWSecurityConfig8192(dev);
		}
		memcpy((u8*)key, ext->key, 16); 
		
		if(wep_nogroup && (ieee->auth_mode !=2))
		{
			if (ext->key_len == 13)
				ieee->pairwise_key_type = alg = KEY_TYPE_WEP104;
			setKey( dev,
					idx,
					idx, 
					alg,  
					zero, 
					0,              
					key);           
			set_swcam( dev,
					idx,
					idx, 
					alg,  
					zero, 
					0,              
					key,          
					0,
					0);
		}
		else if (group)
		{
			if ((ext->key_len == 13) && (alg == KEY_TYPE_WEP40) )
				alg = KEY_TYPE_WEP104;
			
			ieee->group_key_type = alg;
			setKey( dev,
					idx,
					idx, 
					alg,  
					broadcast_addr, 
					0,              
					key);           
			set_swcam( dev,
					idx,
					idx, 
					alg,  
					broadcast_addr, 
					0,              
					key,                 
					0,
					0);
		}
		else 
		{
			#ifdef RTL8192E
			if ((ieee->pairwise_key_type == KEY_TYPE_CCMP) && ieee->pHTInfo->bCurrentHTSupport){
							write_nic_byte(dev, 0x173, 1); 
			}
			#endif
			setKey( dev,
					4,
					idx, 
					alg,  
					(u8*)ieee->ap_mac_addr, 
					0,              
					key);           
			set_swcam( dev,
					4,
					idx, 
					alg,  
					(u8*)ieee->ap_mac_addr, 
					0,              
					key,                 
					0,
					0);
		}


	}

end_hw_sec:
        priv->rtllib->wx_set_enc = 0;
	up(&priv->wx_sem);
#endif
	return ret;	

}
#endif

static int r8192_wx_set_auth(struct net_device *dev,
                                        struct iw_request_info *info,
                                        union iwreq_data *data, char *extra)
{
	int ret=0;
	
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	struct r8192_priv *priv = rtllib_priv(dev);

	if(priv->bHwRadioOff == true)
		return 0;

	down(&priv->wx_sem);
	ret = rtllib_wx_set_auth(priv->rtllib, info, &(data->param), extra);
	up(&priv->wx_sem);
#endif
	return ret;
}

static int r8192_wx_set_mlme(struct net_device *dev,
                                        struct iw_request_info *info,
                                        union iwreq_data *wrqu, char *extra)
{

	int ret=0;
	
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	struct r8192_priv *priv = rtllib_priv(dev);

	if(priv->bHwRadioOff == true)
		return 0;

	down(&priv->wx_sem);
	ret = rtllib_wx_set_mlme(priv->rtllib, info, wrqu, extra);
	up(&priv->wx_sem);
#endif
	return ret;
}
#endif

static int r8192_wx_set_gen_ie(struct net_device *dev,
                                        struct iw_request_info *info,
                                        union iwreq_data *data, char *extra)
{
	int ret = 0;
	
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
        struct r8192_priv *priv = rtllib_priv(dev);

	if(priv->bHwRadioOff == true)
		return 0;

        down(&priv->wx_sem);
        ret = rtllib_wx_set_gen_ie(priv->rtllib, extra, data->data.length);
        up(&priv->wx_sem);
#endif
        return ret;
}

static int r8192_wx_get_gen_ie(struct net_device *dev,
                               struct iw_request_info *info, 
			       union iwreq_data *data, char *extra)
{
	int ret = 0;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
        struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;

	if (ieee->wpa_ie_len == 0 || ieee->wpa_ie == NULL) {
		data->data.length = 0;
		return 0;
	}

	if (data->data.length < ieee->wpa_ie_len) {
		return -E2BIG;
	}

	data->data.length = ieee->wpa_ie_len;
	memcpy(extra, ieee->wpa_ie, ieee->wpa_ie_len);
#endif
        return ret;
}

#ifdef _RTL8192_EXT_PATCH_
/*
   Output:
     (case 1) Mesh: Enable. MESHID=[%s] (max length of %s is 32 bytes). 
     (case 2) Mesh: Disable.
*/
static int r8192_wx_get_meshinfo(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	if( ! priv->mshobj || !priv->mshobj->ext_patch_r819x_wx_get_meshinfo )
		return 0;
	return priv->mshobj->ext_patch_r819x_wx_get_meshinfo(dev, info, wrqu, extra);
}


static int r8192_wx_enable_mesh(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;
	RT_RF_POWER_STATE       rtState;
	int ret = 0;
        rtState = priv->rtllib->eRFPowerState;
	
	if( ! priv->mshobj || !priv->mshobj->ext_patch_r819x_wx_enable_mesh )
		return 0;

	SEM_DOWN_PRIV_WX(&priv->wx_sem);	
	if(priv->mshobj->ext_patch_r819x_wx_enable_mesh(dev))
	{
		union iwreq_data tmprqu;
#ifdef ENABLE_IPS
		if(priv->rtllib->PowerSaveControl.bInactivePs){
			if(rtState == eRfOff){
				if(priv->rtllib->RfOffReason > RF_CHANGE_BY_IPS)
				{
					RT_TRACE(COMP_ERR, "%s(): RF is OFF.\n",__FUNCTION__);
					SEM_UP_PRIV_WX(&priv->wx_sem);	
					return -1;
				}
				else{
					printk("=========>%s(): IPSLeave\n",__FUNCTION__);
					IPSLeave(dev);
				}
			}
		}
#endif
		if(ieee->only_mesh == 0) 
		{
			tmprqu.mode = ieee->iw_mode;
			ieee->iw_mode = 0; 
			ret = rtllib_wx_set_mode(ieee, info, &tmprqu, extra);
		}
	}

	SEM_UP_PRIV_WX(&priv->wx_sem);	
	
	return ret;
	
}

static int r8192_wx_disable_mesh(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;

	int ret = 0;
	if( ! priv->mshobj || !priv->mshobj->ext_patch_r819x_wx_disable_mesh )
		return 0;

	SEM_DOWN_PRIV_WX(&priv->wx_sem);	
	if(priv->mshobj->ext_patch_r819x_wx_disable_mesh(dev))
	{
		union iwreq_data tmprqu;
		tmprqu.mode = ieee->iw_mode;
		ieee->iw_mode = 999;
		ret = rtllib_wx_set_mode(ieee, info, &tmprqu, extra);
	}

	SEM_UP_PRIV_WX(&priv->wx_sem);	
	
	return ret;
}


int r8192_wx_set_channel(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	int ch = *extra;
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;
	
	if (!priv->mshobj || (ieee->iw_mode != IW_MODE_MESH) || !priv->mshobj->ext_patch_r819x_wx_set_channel || !ieee->only_mesh)
		return 0;	
			
	if ( ch < 0 ) 	
	{
		rtllib_start_scan(ieee);			
		ieee->meshScanMode =2;
	}
	else	
	{	
		ieee->meshScanMode =0;		
		if(priv->mshobj->ext_patch_r819x_wx_set_channel)
		{
			priv->mshobj->ext_patch_r819x_wx_set_channel(ieee, ch);
			priv->mshobj->ext_patch_r819x_wx_set_mesh_chan(dev,ch);
		}
		queue_work_rsl(ieee->wq, &ieee->ext_stop_scan_wq);
		ieee->set_chan(ieee->dev, ch);
printk("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!set current mesh network channel %d\n", ch);
		ieee->current_mesh_network.channel = ch;
		if(ieee->only_mesh)
			ieee->current_network.channel = ch;
		
		ieee->current_network.channel = ieee->current_mesh_network.channel; 
		if(ieee->pHTInfo->bCurBW40MHz)
			HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20_40, (ieee->current_mesh_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER);  
		else
			HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20, (ieee->current_mesh_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER);  
		if(rtllib_act_scanning(ieee,true) == true)
			rtllib_stop_scan_syncro(ieee);
	}
		
	return 0;
}

static int r8192_wx_set_meshID(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{	
	struct r8192_priv *priv = rtllib_priv(dev);	
	if( ! priv->mshobj || !priv->mshobj->ext_patch_r819x_wx_set_meshID )
		return 0;

	return priv->mshobj->ext_patch_r819x_wx_set_meshID(dev, wrqu->data.pointer);	
}


/* reserved for future
static int r8192_wx_add_mac_deny(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	if( ! priv->mshobj || !priv->mshobj->ext_patch_r819x_wx_set_add_mac_deny )
		return 0;

	return priv->mshobj->ext_patch_r819x_wx_set_add_mac_deny(dev, info, wrqu, extra);
}

static int r8192_wx_del_mac_deny(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	if( ! priv->mshobj || !priv->mshobj->ext_patch_r819x_wx_set_del_mac_deny )
		return 0;

	return priv->mshobj->ext_patch_r819x_wx_set_del_mac_deny(dev, info, wrqu, extra);
}
*/
/* reserved for future
static int r8192_wx_get_mac_deny(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	if( ! priv->mshobj || !priv->mshobj->ext_patch_r819x_wx_get_mac_deny )
		return 0;

	return priv->mshobj->ext_patch_r819x_wx_get_mac_deny(dev, info, wrqu, extra);
}
static int r8192_wx_join_mesh(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{	
	struct r8192_priv *priv = rtllib_priv(dev);	
	int ret=0;
	char ch;
	if(priv->rtllib->iw_mode == IW_MODE_MESH) {
		printk("join mesh %s\n",extra);
		if (wrqu->essid.length > IW_ESSID_MAX_SIZE){
			ret= -E2BIG;
			goto out;
		}
		if((wrqu->essid.length == 1) && (wrqu->essid.flags == 1)){
			ret = 0;
			goto out;
		}
		if (wrqu->essid.flags && wrqu->essid.length) {
			if(priv->mshobj->ext_patch_r819x_wx_get_selected_mesh_channel(dev, extra, &ch))
			{
				priv->mshobj->ext_patch_r819x_wx_set_meshID(dev, extra); 
				priv->mshobj->ext_patch_r819x_wx_set_mesh_chan(dev,ch); 
				r8192_wx_set_channel(dev, NULL, NULL, &ch);
			}
			else
				printk("invalid mesh #\n");
		}
	}
out:
	return ret;
}
*/

static int r8192_wx_get_mesh_list(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	if( ! priv->mshobj || !priv->mshobj->ext_patch_r819x_wx_get_mesh_list )
		return 0;
	return priv->mshobj->ext_patch_r819x_wx_get_mesh_list(dev, info, wrqu, extra);
}

static int r8192_wx_mesh_scan(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	if( ! priv->mshobj || !priv->mshobj->ext_patch_r819x_wx_mesh_scan )
		return 0;
	return priv->mshobj->ext_patch_r819x_wx_mesh_scan(dev, info, wrqu, extra);
}

static int r8192_wx_set_meshmode(struct net_device *dev,
		struct iw_request_info *info,
		union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	SEM_DOWN_PRIV_WX(&priv->wx_sem);	

	printk("%s(): set mesh mode ! extra is %d\n",__FUNCTION__, *extra);
	if((*extra != WIRELESS_MODE_A) && (*extra != WIRELESS_MODE_B) && 
		(*extra != WIRELESS_MODE_G) && (*extra != WIRELESS_MODE_AUTO) &&
		(*extra != WIRELESS_MODE_N_24G) && (*extra != WIRELESS_MODE_N_5G))
	{
		printk("ERR!! you should input 1 | 2 | 4 | 8 | 16 | 32\n");
		SEM_UP_PRIV_WX(&priv->wx_sem);	
		return -1;
	}
	if(priv->rtllib->state == RTLLIB_LINKED)
	{
		if((priv->rtllib->mode != WIRELESS_MODE_N_5G) && (priv->rtllib->mode != WIRELESS_MODE_N_24G)){
			printk("===>wlan0 is linked,and ieee->mode is not N mode ,do not need to set mode,return\n");
			SEM_UP_PRIV_WX(&priv->wx_sem);	
			return 0;
		}
	}
	priv->rtllib->mode = *extra;
	if(priv->ResetProgress == RESET_TYPE_NORESET)
		rtl8192_SetWirelessMode(dev, priv->rtllib->mode);
	 HTUseDefaultSetting(priv->rtllib);
	SEM_UP_PRIV_WX(&priv->wx_sem);	
	return 0;

}

static int r8192_wx_set_meshBW(struct net_device *dev,
                struct iw_request_info *info,
                union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	SEM_DOWN_PRIV_WX(&priv->wx_sem);	

	printk("%s(): set mesh BW ! extra is %d\n",__FUNCTION__, *extra);
	priv->rtllib->pHTInfo->bRegBW40MHz = *extra;
	SEM_UP_PRIV_WX(&priv->wx_sem);	
	return 0;
}
static int r8192_wx_set_mesh_security(struct net_device *dev,
                struct iw_request_info *info,
                union iwreq_data *wrqu, char *extra)
{
        struct r8192_priv *priv = rtllib_priv(dev);
        struct rtllib_device* ieee = priv->rtllib;

        down(&priv->wx_sem);

        printk("%s(): set mesh security! extra is %d\n",__FUNCTION__, *extra);
        ieee->mesh_security_setting = *extra;

        if (0 == ieee->mesh_security_setting)
        {
		ieee->mesh_pairwise_key_type = ieee->mesh_group_key_type = KEY_TYPE_NA;
		CamResetAllEntry(dev);
		CamRestoreEachIFEntry(dev,0,0);
		reset_IFswcam(dev,1,0);
        }
	else
	{
		ieee->mesh_pairwise_key_type = KEY_TYPE_CCMP;
		ieee->mesh_group_key_type = KEY_TYPE_CCMP;
	}
        up(&priv->wx_sem);
        return 0;

}

static int r8192_wx_set_mkdd_id(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{	
	struct r8192_priv *priv = rtllib_priv(dev);	
	printk("===>%s()\n",__FUNCTION__);	
	if( ! priv->mshobj || !priv->mshobj->ext_patch_r819x_wx_set_mkdd_id)
		return 0;

	return priv->mshobj->ext_patch_r819x_wx_set_mkdd_id(dev, wrqu->data.pointer);	
}

static int r8192_wx_set_mesh_key(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{	
	struct r8192_priv *priv = rtllib_priv(dev);	
	printk("===>%s()\n",__FUNCTION__);	
	if( ! priv->mshobj || !priv->mshobj->ext_patch_r819x_wx_set_mesh_key)
		return 0;
	return priv->mshobj->ext_patch_r819x_wx_set_mesh_key(dev, wrqu->data.pointer);	
}

static int r8192_wx_set_mesh_sec_type(struct net_device *dev,
                struct iw_request_info *info,
                union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;

	printk("%s(): set mesh security type! extra is %d\n",__FUNCTION__, *extra);
	if (ieee->mesh_sec_type == 1 && *extra != 1 && ieee->mesh_security_setting == 3) {
		rtl8192_abbr_handshk_disable_key(ieee); 
	
		if(priv->mshobj->ext_patch_r819x_wx_release_sae_info)
			priv->mshobj->ext_patch_r819x_wx_release_sae_info(ieee);
	}
	down(&priv->wx_sem);

	ieee->mesh_sec_type = *extra;

	if(ieee->mesh_sec_type == 0)
		ieee->mesh_pairwise_key_type = ieee->mesh_group_key_type = KEY_TYPE_NA;

	up(&priv->wx_sem);
	return 0;
}

#endif 

#define OID_RT_INTEL_PROMISCUOUS_MODE	0xFF0101F6

static int r8192_wx_set_PromiscuousMode(struct net_device *dev,
                struct iw_request_info *info,
                union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;

	u32 *info_buf = (u32*)(wrqu->data.pointer);

	u32 oid = info_buf[0];
	u32 bPromiscuousOn = info_buf[1];
	u32 bFilterSourceStationFrame = info_buf[2];

	if (OID_RT_INTEL_PROMISCUOUS_MODE == oid)
	{
		ieee->IntelPromiscuousModeInfo.bPromiscuousOn = 
					(bPromiscuousOn)? (true) : (false);
		ieee->IntelPromiscuousModeInfo.bFilterSourceStationFrame = 
					(bFilterSourceStationFrame)? (true) : (false);

		(bPromiscuousOn) ? (rtllib_EnableIntelPromiscuousMode(dev, false)) : 
				(rtllib_DisableIntelPromiscuousMode(dev, false));	

		printk("=======>%s(), on = %d, filter src sta = %d\n", __FUNCTION__, 
			bPromiscuousOn, bFilterSourceStationFrame);
	} else {
		return -1;
	}
	
	return 0;
}


static int r8192_wx_get_PromiscuousMode(struct net_device *dev,
                               struct iw_request_info *info,
                               union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;

        down(&priv->wx_sem);

        snprintf(extra, 45, "PromiscuousMode:%d, FilterSrcSTAFrame:%d",\
			ieee->IntelPromiscuousModeInfo.bPromiscuousOn,\
			ieee->IntelPromiscuousModeInfo.bFilterSourceStationFrame);
        wrqu->data.length = strlen(extra) + 1;

        up(&priv->wx_sem);

	return 0;
}

#ifdef CONFIG_BT_30
static int r8192_wx_creat_physical_link(
		struct net_device 	*dev,
                struct iw_request_info 	*info,
                union iwreq_data 	*wrqu, 
		char 			*extra)
{
	u32	*info_buf = (u32*)(wrqu->data.pointer);
	
	struct r8192_priv* priv = rtllib_priv(dev);
	PPACKET_IRP_HCICMD_DATA	pHciCmd = NULL;
	u8	joinaddr[6] = {0x00, 0xe0, 0x4c, 0x76, 0x00, 0xa4};
	u8	i = 0;

	if (priv->bt_close)
		return 1;

	for(i=0; i<6; i++)
		joinaddr[i] = (u8)(info_buf[i]);

	printk("===++===++===> RemoteJoinerAddr: %02x:%02x:%02x:%02x:%02x:%02x\n", 
		joinaddr[0],joinaddr[1],joinaddr[2],joinaddr[3],joinaddr[4],joinaddr[5]);

	pHciCmd = (PPACKET_IRP_HCICMD_DATA)kmalloc(sizeof(PACKET_IRP_HCICMD_DATA)+4, GFP_KERNEL);

	BT_HCI_RESET(dev, false);
	if(priv->rtllib->state >= RTLLIB_LINKED)
		priv->BtInfo.BTChannel = priv->rtllib->current_network.channel;
	BT_HCI_CREATE_PHYSICAL_LINK(dev, pHciCmd);
	
	memcpy(priv->BtInfo.BtAsocEntry[0].BTRemoteMACAddr, joinaddr, 6);
	printk("%s Rm MAC:" MAC_FMT "\n", __func__, MAC_ARG(priv->BtInfo.BtAsocEntry[0].BTRemoteMACAddr));

	BT_HCI_StartBeaconAndConnect(dev, pHciCmd, 0);
	priv->BtInfo.BtAsocEntry[0].bUsed = true;
	priv->BtInfo.BtAsocEntry[0].LogLinkCmdData[0].BtLogLinkhandle = 0x201;

	kfree(pHciCmd);

	return 1;
}

static int r8192_wx_accept_physical_link(
		struct net_device 	*dev,
                struct iw_request_info 	*info,
                union iwreq_data 	*wrqu, 
		char 			*extra)
{
	struct r8192_priv* priv = rtllib_priv(dev);
	PBT30Info	pBTInfo = &priv->BtInfo;
	PPACKET_IRP_HCICMD_DATA	pHciCmd = NULL;
	u8 amp_ssid[32] = {0};
	u8 amp_len = strlen(wrqu->data.pointer);

	if (priv->bt_close)
		return 1;

	memcpy(amp_ssid, wrqu->data.pointer, amp_len);

	pHciCmd = (PPACKET_IRP_HCICMD_DATA)kmalloc(sizeof(PACKET_IRP_HCICMD_DATA)+4, GFP_KERNEL);

	BT_HCI_RESET(dev, false);
	BT_HCI_ACCEPT_PHYSICAL_LINK(dev, pHciCmd);

	snprintf(pBTInfo->BtAsocEntry[0].BTSsidBuf, 32, "AMP-%02x-%02x-%02x-%02x-%02x-%02x", 0x00,0xe0,0x4c,0x76,0x00,0xa1);
	pBTInfo->BtAsocEntry[0].BTSsid.Octet = pBTInfo->BtAsocEntry[0].BTSsidBuf;
	pBTInfo->BtAsocEntry[0].BTSsid.Length = 21;
	printk("==++==++==++==>%s() AMP-SSID:%s:%s, len:%d\n", __func__, amp_ssid, pBTInfo->BtAsocEntry[0].BTSsid.Octet, amp_len);

#if 1 
	{
		unsigned long flags;
		struct rtllib_network *target;

		spin_lock_irqsave(&priv->rtllib->lock, flags);
				
		list_for_each_entry(target, &priv->rtllib->network_list, list) {

			if(!CompareSSID(pBTInfo->BtAsocEntry[0].BTSsidBuf, pBTInfo->BtAsocEntry[0].BTSsid.Length, 
				target->ssid,target->ssid_len)){
				continue;
			}

			printk("===++===++===> CreaterBssid:  %02x:%02x:%02x:%02x:%02x:%02x\n", 
				target->bssid[0],target->bssid[1],target->bssid[2],
				target->bssid[3],target->bssid[4],target->bssid[5]);
			memcpy(pBTInfo->BtAsocEntry[0].BTRemoteMACAddr, target->bssid, 6);
		}
		
		spin_unlock_irqrestore(&priv->rtllib->lock, flags);
	}
#endif
	BT_HCI_StartBeaconAndConnect(dev, pHciCmd, 0);
	

	mdelay(100);

	snprintf(pBTInfo->BtAsocEntry[0].BTSsidBuf, 32, "AMP-%02x-%02x-%02x-%02x-%02x-%02x", 0x00,0xe0,0x4c,0x76,0x00,0xa1);
	pBTInfo->BtAsocEntry[0].BTSsid.Octet = pBTInfo->BtAsocEntry[0].BTSsidBuf;
	pBTInfo->BtAsocEntry[0].BTSsid.Length = 21;
	pBTInfo->BtAsocEntry[0].bUsed = 1;

	BT_ConnectProcedure(dev, 0);

	kfree(pHciCmd);

	return 1;
}
static int r8192_wx_get_data_or_evn(struct net_device *dev,
        struct iw_request_info *info,
        union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	if (priv->bt_close)
		return 1;

	down(&priv->wx_sem);

	printk("%s(): %d\n",__FUNCTION__, *extra);
	if((*extra != 0) && (*extra != 1))
	{
		RT_TRACE(COMP_ERR, "%s(): set radio an err value,must 0(radio off) or 1(radio on)\n",__FUNCTION__);
		return -1;
	}

	if (*extra == 0) {
		u8 data[32] = {8,8,8,8,1,4,7,9,0,0,0,8,8,8,8};

		BT_RX_ACLData(dev, data, 32, 0);
		printk("hci get data \n");
	} else if (*extra == 1) {
		u8 event[32] = {9,9,9,9,1,4,7,9,0,0,0,9,9,9,9};

		BT_ISSUE_EVENT(dev, event, 32);
		printk("hci get event \n");
	}

	up(&priv->wx_sem);
	return 0;

}
#endif


#define IW_IOCTL(x) [(x)-SIOCSIWCOMMIT]
static iw_handler r8192_wx_handlers[] =
{
        IW_IOCTL(SIOCGIWNAME) = r8192_wx_get_name,   	  
        IW_IOCTL(SIOCSIWFREQ) = r8192_wx_set_freq,        
        IW_IOCTL(SIOCGIWFREQ) = r8192_wx_get_freq,        
        IW_IOCTL(SIOCSIWMODE) = r8192_wx_set_mode,        
        IW_IOCTL(SIOCGIWMODE) = r8192_wx_get_mode,        
        IW_IOCTL(SIOCSIWSENS) = r8192_wx_set_sens,        
        IW_IOCTL(SIOCGIWSENS) = r8192_wx_get_sens,        
        IW_IOCTL(SIOCGIWRANGE) = rtl8192_wx_get_range,	  
        IW_IOCTL(SIOCSIWAP) = r8192_wx_set_wap,      	  
        IW_IOCTL(SIOCGIWAP) = r8192_wx_get_wap,           
        IW_IOCTL(SIOCSIWSCAN) = r8192_wx_set_scan,        
        IW_IOCTL(SIOCGIWSCAN) = r8192_wx_get_scan,        
        IW_IOCTL(SIOCSIWESSID) = r8192_wx_set_essid,      
        IW_IOCTL(SIOCGIWESSID) = r8192_wx_get_essid,      
        IW_IOCTL(SIOCSIWNICKN) = r8192_wx_set_nick,
		IW_IOCTL(SIOCGIWNICKN) = r8192_wx_get_nick,
        IW_IOCTL(SIOCSIWRATE) = r8192_wx_set_rate,        
        IW_IOCTL(SIOCGIWRATE) = r8192_wx_get_rate,        
        IW_IOCTL(SIOCSIWRTS) = r8192_wx_set_rts,          
        IW_IOCTL(SIOCGIWRTS) = r8192_wx_get_rts,          
        IW_IOCTL(SIOCSIWFRAG) = r8192_wx_set_frag,        
        IW_IOCTL(SIOCGIWFRAG) = r8192_wx_get_frag,        
        IW_IOCTL(SIOCSIWRETRY) = r8192_wx_set_retry,      
        IW_IOCTL(SIOCGIWRETRY) = r8192_wx_get_retry,      
        IW_IOCTL(SIOCSIWENCODE) = r8192_wx_set_enc,       
        IW_IOCTL(SIOCGIWENCODE) = r8192_wx_get_enc,       
        IW_IOCTL(SIOCSIWPOWER) = r8192_wx_set_power,         
        IW_IOCTL(SIOCGIWPOWER) = r8192_wx_get_power,         
#if (WIRELESS_EXT >= 18)
		IW_IOCTL(SIOCSIWGENIE) = r8192_wx_set_gen_ie, 	     
		IW_IOCTL(SIOCGIWGENIE) = r8192_wx_get_gen_ie,
		IW_IOCTL(SIOCSIWMLME) = r8192_wx_set_mlme,        
		IW_IOCTL(SIOCSIWAUTH) = r8192_wx_set_auth,	    
		IW_IOCTL(SIOCSIWENCODEEXT) = r8192_wx_set_enc_ext,  
#endif
}; 

/* 
 * the following rule need to be follwing,
 * Odd : get (world access), 
 * even : set (root access) 
 * */
static const struct iw_priv_args r8192_private_args[] = { 
	{
		SIOCIWFIRSTPRIV + 0x0, 
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "set_debugflag" 
	}, 
	{
		SIOCIWFIRSTPRIV + 0x1,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "activescan"
	},
	{
		SIOCIWFIRSTPRIV + 0x2, 
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "rawtx" 
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x3,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "forcereset"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x4,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "force_mic_error"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x5,
		IW_PRIV_TYPE_NONE, IW_PRIV_TYPE_INT|IW_PRIV_SIZE_FIXED|1, 
		"firm_ver"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x6,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED|1, IW_PRIV_TYPE_NONE, 
		"set_power"
	}
#ifdef _RTL8192_EXT_PATCH_
	,
	{
		SIOCIWFIRSTPRIV + 0x7,
		IW_PRIV_TYPE_NONE, IW_PRIV_TYPE_CHAR|512, 
		"print_reg"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x8,
		IW_PRIV_TYPE_NONE, IW_PRIV_TYPE_CHAR|64, 
		"resume_firm"
	}
#endif
	,
	{
		SIOCIWFIRSTPRIV + 0x9,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED|1, IW_PRIV_TYPE_NONE, 
		"radio"
	}
        ,
	{
		SIOCIWFIRSTPRIV + 0xa,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED|1, IW_PRIV_TYPE_NONE, 
		"lps_interv"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0xb,
		0, IW_PRIV_TYPE_CHAR|1024, "adhoc_peer_list" 
	}
        ,
	{
		SIOCIWFIRSTPRIV + 0xc,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED|1, IW_PRIV_TYPE_NONE, 
		"lps_force"
	}
#ifdef _RTL8192_EXT_PATCH_
	,
	{
		SIOCIWFIRSTPRIV + 0xd,
		IW_PRIV_TYPE_NONE, IW_PRIV_TYPE_CHAR|64, 
		"driverVer"
	}
#endif
#ifdef CONFIG_MP	
	,
	{
		SIOCIWFIRSTPRIV + 0xe,
                IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "SetChan"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0xf,
                IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "SetRate"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x10,
                IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "SetTxPower"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x11,
                IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "SetBW"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x12,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "TxStart"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x13,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,0, "SetSingleCarrier"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x14,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 3, 0, "WriteRF"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x15,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 3, 0, "WriteMAC"
	}
#endif
	,
	{
		SIOCIWFIRSTPRIV + 0x16,
                IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 3, 0, "setpromisc"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x17,
		0,IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_FIXED | 45, "getpromisc"
	}
#ifdef CONFIG_BT_30
	,
	{
		SIOCIWFIRSTPRIV + 0x18,
                IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 6, 0, "amp_creater"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x19,
		IW_PRIV_TYPE_CHAR | 64, 0, "amp_joiner"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x1a,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED|1, IW_PRIV_TYPE_NONE, 
		"amp_get_data"
	}
#endif
	,
	{
		SIOCIWFIRSTPRIV + 0x1b,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "set_uapsd"
	}
};

static iw_handler r8192_private_handler[] = {
	(iw_handler)r8192_wx_set_debugflag,   /*SIOCIWSECONDPRIV*/
	(iw_handler)r8192_wx_set_scan_type,
	(iw_handler)r8192_wx_set_rawtx,
	(iw_handler)r8192_wx_force_reset,
	(iw_handler)r8192_wx_force_mic_error,
	(iw_handler)r8191se_wx_get_firm_version,
	(iw_handler)r8192_wx_adapter_power_status,	    
#ifdef _RTL8192_EXT_PATCH_
	(iw_handler)r8192_wx_print_reg,	    
	(iw_handler)r8192_wx_resume_firm,
#else
	(iw_handler)NULL, 
	(iw_handler)NULL,
#endif
	(iw_handler)r8192se_wx_set_radio,
	(iw_handler)r8192se_wx_set_lps_awake_interval,
	(iw_handler)r8192_wx_get_adhoc_peers,
	(iw_handler)r8192se_wx_set_force_lps,
#ifdef _RTL8192_EXT_PATCH_
	(iw_handler)r8192_wx_get_drv_version,
#else
	(iw_handler)NULL,
#endif
#ifdef CONFIG_MP
	(iw_handler)r8192_wx_mp_set_chan,
	(iw_handler)r8192_wx_mp_set_txrate,
	(iw_handler)r8192_wx_mp_set_txpower,
	(iw_handler)r8192_wx_mp_set_bw,
        (iw_handler)r8192_wx_mp_set_txstart,
        (iw_handler)r8192_wx_mp_set_singlecarrier,
        (iw_handler)r8192_wx_mp_write_rf,
	(iw_handler)r8192_wx_mp_write_mac,
#else
	(iw_handler)NULL,
	(iw_handler)NULL,
	(iw_handler)NULL,
	(iw_handler)NULL,
	(iw_handler)NULL,
 	(iw_handler)NULL,
	(iw_handler)NULL,
	(iw_handler)NULL,
#endif
	(iw_handler)r8192_wx_set_PromiscuousMode,
	(iw_handler)r8192_wx_get_PromiscuousMode,
#ifdef CONFIG_BT_30
	(iw_handler)r8192_wx_creat_physical_link,
	(iw_handler)r8192_wx_accept_physical_link,
	(iw_handler)r8192_wx_get_data_or_evn,
#else
	(iw_handler)NULL,
	(iw_handler)NULL,
	(iw_handler)NULL,
#endif
	(iw_handler)r8192_wx_set_wmm_uapsd,
};

struct iw_statistics *r8192_get_wireless_stats(struct net_device *dev)
{
       struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;
	struct iw_statistics* wstats = &priv->wstats;
	int tmp_level = 0;
	int tmp_qual = 0;
	int tmp_noise = 0;
	if(ieee->state < RTLLIB_LINKED)
	{
		wstats->qual.qual = 10;
		wstats->qual.level = 0;
		wstats->qual.noise = -100;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14)) 
		wstats->qual.updated = IW_QUAL_ALL_UPDATED | IW_QUAL_DBM;
#else
		wstats->qual.updated = 0x0f;
#endif
		return wstats;
	}
	
       tmp_level = (&ieee->current_network)->stats.rssi;
	tmp_qual = (&ieee->current_network)->stats.signal;
	tmp_noise = (&ieee->current_network)->stats.noise;			

	wstats->qual.level = tmp_level;
	wstats->qual.qual = tmp_qual;
	wstats->qual.noise = tmp_noise;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14)) 
	wstats->qual.updated = IW_QUAL_ALL_UPDATED | IW_QUAL_DBM;
#else
	wstats->qual.updated = 0x0f;
#endif
	return wstats;
}

#if defined RTL8192SE || defined RTL8192CE
u8 SS_Rate_Map_G[6][2] = {{40, MGN_54M}, {30, MGN_48M}, {20, MGN_36M},
	{12, MGN_24M}, {7, MGN_18M}, {0, MGN_12M}};
u8 SS_Rate_Map_G_MRC_OFF[6][2]= {{17, MGN_54M}, {15, MGN_48M}, 
	{13, MGN_36M}, {10, MGN_24M}, {3, MGN_18M}, {0, MGN_12M}};
u8 MSI_SS_Rate_Map_G[6][2] = {{40, MGN_54M}, {30, MGN_54M}, {20, MGN_54M},
	{12, MGN_48M}, {7, MGN_36M}, {0, MGN_24M}};
u8 SS_Rate_Map_B[2][2] = {{7, MGN_11M}, {0, MGN_5_5M}};
u8 SS_Rate_Map_N_MCS7[7][2] = {{40, MGN_MCS7}, {30, MGN_MCS5}, {25, MGN_MCS4},
	{23, MGN_MCS3}, {19, MGN_MCS2}, {8, MGN_MCS1}, {0, MGN_MCS0}};
u8 SS_Rate_Map_N_MCS15[7][2] = {{40, MGN_MCS15}, {35, MGN_MCS14}, {31, MGN_MCS12},
	{28, MGN_MCS7}, {25, MGN_MCS5}, {23, MGN_MCS3}, {20, MGN_MCS0}};
#define TxRateTypeNormal        0
#define TxRateTypeCurrent       1
#define TxRateTypeStartRate     2

u8 rtl8192_decorate_txrate_by_singalstrength(u32 SignalStrength, u8 *SS_Rate_Map, u8 MapSize)
{
	u8 index = 0;

	for (index = 0; index < (MapSize * 2); index += 2) {
		if (SignalStrength > SS_Rate_Map[index])
			return SS_Rate_Map[index+1];
	}

	return MGN_1M;
}

u16 rtl8192_11n_user_show_rates(struct net_device* dev)
{
	struct r8192_priv  *priv = rtllib_priv(dev);
	static u8  TimesForReportingFullRxSpeedAfterConnected = 100;
	u8  rate = MGN_1M;
	u32  Sgstrength;
	bool TxorRx = priv->rtllib->bForcedShowRxRate;          
	PRT_HIGH_THROUGHPUT pHTInfo = priv->rtllib->pHTInfo;
	u8 bCurrentMRC = 0;

	priv->rtllib->GetHwRegHandler(dev, HW_VAR_MRC, (u8*)(&bCurrentMRC));


	if (!TxorRx) {
		{
			return CONVERT_RATE(priv->rtllib, priv->rtllib->softmac_stats.CurrentShowTxate);
		}
	}

	Sgstrength = 100;

	if (priv->rtllib->mode == WIRELESS_MODE_A ||
			priv->rtllib->mode == WIRELESS_MODE_G ||
			priv->rtllib->mode == (WIRELESS_MODE_G | WIRELESS_MODE_B )) {
		if (priv->CustomerID == RT_CID_819x_MSI) {
			rate = rtl8192_decorate_txrate_by_singalstrength(Sgstrength,
					(u8*)MSI_SS_Rate_Map_G, sizeof(MSI_SS_Rate_Map_G)/2);
		} else {
			if(!bCurrentMRC) 
				rate = rtl8192_decorate_txrate_by_singalstrength(Sgstrength, 
					(u8*)SS_Rate_Map_G_MRC_OFF, sizeof(SS_Rate_Map_G_MRC_OFF)/2);
			else				
			rate = rtl8192_decorate_txrate_by_singalstrength(Sgstrength,
					(u8*)SS_Rate_Map_G, sizeof(SS_Rate_Map_G)/2);
		}
	} else if (priv->rtllib->mode == WIRELESS_MODE_B) {
		rate = rtl8192_decorate_txrate_by_singalstrength(Sgstrength, (u8*)SS_Rate_Map_B,
				sizeof(SS_Rate_Map_B)/2);
	} else if(priv->rtllib->mode == WIRELESS_MODE_N_24G) {
		bool  bMaxRateMcs15;
		bool  b1SSSupport = priv->rtllib->b1x1RecvCombine;
		u8    rftype = priv->rf_type;
		if (((!TxorRx) && (rftype==RF_1T1R || rftype==RF_1T2R)) ||
				(TxorRx && (rftype==RF_1T1R || (rftype==RF_1T2R && b1SSSupport))) ||
				(rftype==RF_2T2R && priv->rtllib->HTHighestOperaRate<=MGN_MCS7))
			bMaxRateMcs15 = false;
		else
			bMaxRateMcs15 = true;


		if(priv->rtllib->state != RTLLIB_LINKED)
			priv->rtllib->SystemQueryDataRateCount = 0;
		if (TimesForReportingFullRxSpeedAfterConnected > priv->rtllib->SystemQueryDataRateCount) {
			priv->rtllib->SystemQueryDataRateCount++;

			if(bMaxRateMcs15){
				if(pHTInfo->bCurBW40MHz){
					if(pHTInfo->bCurShortGI40MHz){ 
						return 600;
					}else{
						return 540;
					}
				} else {
					if(pHTInfo->bCurShortGI20MHz){ 
						return 289;
					}else{
						return 260;
					}
				}
			}else{
				if(pHTInfo->bCurBW40MHz){
					if(pHTInfo->bCurShortGI40MHz){ 
						return 300;
					}else{
						return 270;
					}
				} else {
					if(pHTInfo->bCurShortGI20MHz){ 
						return 144;
					}else{
						return 130;
					}
				}
			}
		}

		if (bMaxRateMcs15)
			rate = rtl8192_decorate_txrate_by_singalstrength(Sgstrength, (u8*)SS_Rate_Map_N_MCS15,
					sizeof(SS_Rate_Map_N_MCS15)/2);
		else
			rate = rtl8192_decorate_txrate_by_singalstrength(Sgstrength, (u8*)SS_Rate_Map_N_MCS7,
					sizeof(SS_Rate_Map_N_MCS7)/2);
	} else if (priv->rtllib->mode == WIRELESS_MODE_N_5G) {
		return 580;
	} else {
		return 2;
	}

	if (priv->rtllib->GetHalfNmodeSupportByAPsHandler(dev)) {
		if (rate < 0x80)
			return rate;
		else
			return HTHalfMcsToDataRate(priv->rtllib, rate);
	} else {
                return CONVERT_RATE(priv->rtllib, rate);
        }
}
#endif

struct iw_handler_def  r8192_wx_handlers_def={
	.standard = r8192_wx_handlers,
	.num_standard = sizeof(r8192_wx_handlers) / sizeof(iw_handler),
	.private = r8192_private_handler,
	.num_private = sizeof(r8192_private_handler) / sizeof(iw_handler),
 	.num_private_args = sizeof(r8192_private_args) / sizeof(struct iw_priv_args),
#if WIRELESS_EXT >= 17	
	.get_wireless_stats = r8192_get_wireless_stats,
#endif
	.private_args = (struct iw_priv_args *)r8192_private_args,	
};
#ifdef ASL
static int apdev_wx_get_name(struct net_device *apdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	strcpy(wrqu->name, "802.11");

	if (ieee->modulation & RTLLIB_CCK_MODULATION)
		strcat(wrqu->name, "b");
	if (ieee->modulation & RTLLIB_OFDM_MODULATION)
		strcat(wrqu->name, "g");
	if (ieee->apmode & (IEEE_N_24G | IEEE_N_5G))
		strcat(wrqu->name, "n");
	return 0;
}
static int apdev_wx_set_freq(struct net_device *apdev, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;
	struct r8192_priv *priv = rtllib_priv(dev);
	int ret;

	printk("====>%s()\n",__FUNCTION__);

	if(priv->bHwRadioOff == true)
		return 0;
	
	down(&priv->wx_sem);
	
	ret = apdev_rtllib_wx_set_freq(priv->rtllib, a, wrqu, b);

	up(&priv->wx_sem);
	return ret;
}
static int apdev_wx_get_freq(struct net_device *apdev,
			     struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
#ifdef ASL
	return rtllib_wx_get_freq(ieee ,a,wrqu,b,1);
#else
	return rtllib_wx_get_freq(ieee ,a,wrqu,b);
#endif
}
static int apdev_wx_set_mode(struct net_device *apdev, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_set_mode(dev,a,wrqu,b);
}
static int apdev_wx_get_mode(struct net_device *apdev, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	wrqu->mode = IW_MODE_MASTER;
	return 0;
}
static int apdev_wx_set_sens(struct net_device *apdev, 
				struct iw_request_info *info, 
				union iwreq_data *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_set_sens(dev,info,wrqu,extra);
}
static int apdev_wx_get_sens(struct net_device *apdev, 
				struct iw_request_info *info, 
				union iwreq_data *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_get_sens(dev,info,wrqu,extra);
}
static int apdev_wx_get_range(struct net_device *apdev, 
				struct iw_request_info *info, 
				union iwreq_data *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;

	return rtl8192_wx_get_range(dev,info,wrqu,extra);
}
static int apdev_wx_set_wap(struct net_device *apdev,
			 struct iw_request_info *info,
			 union iwreq_data *awrq,
			 char *extra)
{
	printk("====>%s(): AP mode don't support set wap\n",__FUNCTION__);
	return -1;
}
static int apdev_wx_get_wap(struct net_device *apdev, 
			    struct iw_request_info *info, 
			    union iwreq_data *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;

	return apdev_rtllib_wx_get_wap(ieee,info,wrqu,extra);
}
static int apdev_wx_set_essid(struct net_device *apdev, 
			      struct iw_request_info *a,
			      union iwreq_data *wrqu, char *b)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;
	struct r8192_priv *priv = rtllib_priv(dev);
	int ret;
	
#ifdef CONFIG_MP
	printk("######################%s(): In MP Test Can not Set Essid\n",__FUNCTION__);
	return 0;
#endif
	if(priv->bHwRadioOff == true){
		printk("=========>%s():hw radio off,or Rf state is eRfOff, return\n",__FUNCTION__);
		return 0;
	}
	down(&priv->wx_sem);
	ret = apdev_rtllib_wx_set_essid(priv->rtllib,a,wrqu,b);

	up(&priv->wx_sem);

	return ret;
}
static int apdev_wx_get_essid(struct net_device *apdev, 
			      struct iw_request_info *a,
			      union iwreq_data *wrqu, char *b)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;
	struct r8192_priv *priv = rtllib_priv(dev);
	int ret;
	down(&priv->wx_sem);
	ret = apdev_rtllib_wx_get_essid(ieee,a,wrqu,b);
	up(&priv->wx_sem);
	return ret;
}
static int apdev_wx_set_nick(struct net_device *apdev,
			   struct iw_request_info *info,
			   union iwreq_data *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_set_nick(dev,info,wrqu,extra);
}
static int apdev_wx_get_nick(struct net_device *apdev,
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_get_nick(dev,info,wrqu,extra);
}
static int apdev_wx_set_rate(struct net_device *apdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_set_rate(dev,info,wrqu,extra);
}
static int apdev_wx_get_rate(struct net_device *apdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_get_rate(dev,info,wrqu,extra);
}
static int apdev_wx_set_rts(struct net_device *apdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;
	
	return r8192_wx_set_rts(dev,info,wrqu,extra);
}
static int apdev_wx_get_rts(struct net_device *apdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_get_rts(dev,info,wrqu,extra);
}
static int apdev_wx_set_frag(struct net_device *apdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_set_frag(dev,info,wrqu,extra);
}
static int apdev_wx_get_frag(struct net_device *apdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_get_frag(dev,info,wrqu,extra);
}
static int apdev_wx_set_retry(struct net_device *apdev, 
				struct iw_request_info *info, 
				union iwreq_data *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_set_retry(dev,info,wrqu,extra);
}
static int apdev_wx_get_retry(struct net_device *apdev, 
				struct iw_request_info *info, 
				union iwreq_data *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_get_retry(dev,info,wrqu,extra);
}
static int apdev_wx_get_enc(struct net_device *apdev, 
			    struct iw_request_info *info, 
			    union iwreq_data *wrqu, char *key)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_get_enc(dev,info,wrqu,key);
}
static int apdev_wx_set_mlme(struct net_device *apdev,
                                        struct iw_request_info *info,
                                        union iwreq_data *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_set_mlme(dev,info,wrqu,extra);
}
static iw_handler apdev_wx_handlers[] =
{
        IW_IOCTL(SIOCGIWNAME) = apdev_wx_get_name,   	  
        IW_IOCTL(SIOCSIWFREQ) = apdev_wx_set_freq,        
        IW_IOCTL(SIOCGIWFREQ) = apdev_wx_get_freq,        
        IW_IOCTL(SIOCSIWMODE) = apdev_wx_set_mode,        
        IW_IOCTL(SIOCGIWMODE) = apdev_wx_get_mode,        
        IW_IOCTL(SIOCSIWSENS) = apdev_wx_set_sens,        
        IW_IOCTL(SIOCGIWSENS) = apdev_wx_get_sens,        
        IW_IOCTL(SIOCGIWRANGE) = apdev_wx_get_range,	  
        IW_IOCTL(SIOCSIWAP) = apdev_wx_set_wap,      	  
        IW_IOCTL(SIOCGIWAP) = apdev_wx_get_wap,           
        IW_IOCTL(SIOCSIWSCAN) = NULL,        
        IW_IOCTL(SIOCGIWSCAN) = NULL,        
        IW_IOCTL(SIOCSIWESSID) = apdev_wx_set_essid,      
        IW_IOCTL(SIOCGIWESSID) = apdev_wx_get_essid,      
        IW_IOCTL(SIOCSIWNICKN) = apdev_wx_set_nick,
	 IW_IOCTL(SIOCGIWNICKN) = apdev_wx_get_nick,
        IW_IOCTL(SIOCSIWRATE) = apdev_wx_set_rate,        
        IW_IOCTL(SIOCGIWRATE) = apdev_wx_get_rate,        
        IW_IOCTL(SIOCSIWRTS) = apdev_wx_set_rts,          
        IW_IOCTL(SIOCGIWRTS) = apdev_wx_get_rts,          
        IW_IOCTL(SIOCSIWFRAG) = apdev_wx_set_frag,        
        IW_IOCTL(SIOCGIWFRAG) = apdev_wx_get_frag,        
        IW_IOCTL(SIOCSIWRETRY) = apdev_wx_set_retry,      
        IW_IOCTL(SIOCGIWRETRY) = apdev_wx_get_retry,      
        IW_IOCTL(SIOCSIWENCODE) = NULL,       
        IW_IOCTL(SIOCGIWENCODE) = apdev_wx_get_enc,       
        IW_IOCTL(SIOCSIWPOWER) = NULL,         
        IW_IOCTL(SIOCGIWPOWER) = NULL,         
#if (WIRELESS_EXT >= 18)
		IW_IOCTL(SIOCSIWGENIE) = NULL, 	     
		IW_IOCTL(SIOCGIWGENIE) = NULL,
		IW_IOCTL(SIOCSIWMLME) = apdev_wx_set_mlme,        
		IW_IOCTL(SIOCSIWAUTH) = NULL,	    
		IW_IOCTL(SIOCSIWENCODEEXT) = NULL,  
#endif
}; 
static int apdev_wx_set_debugflag(struct net_device *apdev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_set_debugflag(dev,info,wrqu,extra);
}
static int apdev_wx_force_reset(struct net_device *apdev,
		struct iw_request_info *info,
		union iwreq_data *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;
	printk("====>%s()\n",__FUNCTION__);

	return r8192_wx_force_reset(dev,info,wrqu,extra);
}
static int apdev_wx_force_mic_error(struct net_device *apdev,
		struct iw_request_info *info,
		union iwreq_data *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_force_mic_error(dev,info,wrqu,extra);
}
static int apdev_wx_get_firm_version(struct net_device *apdev,
		struct iw_request_info *info,
		struct iw_param *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8191se_wx_get_firm_version(dev,info,wrqu,extra);
}
static int apdev_wx_set_PromiscuousMode(struct net_device *apdev,
                struct iw_request_info *info,
                union iwreq_data *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_set_PromiscuousMode(dev,info,wrqu,extra);
}
static int apdev_wx_get_PromiscuousMode(struct net_device *apdev,
                               struct iw_request_info *info,
                               union iwreq_data *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_get_PromiscuousMode(dev,info,wrqu,extra);
}
#ifdef RATE_ADAPTIVE_FOR_AP
static int apdev_wx_setrate_adaptive(struct net_device *apdev,
                               struct iw_request_info *info,
                               union iwreq_data *wrqu, char *extra)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct r8192_priv * priv = appriv->priv;
	u8 flag;

	down(&priv->wx_sem);
	flag = *extra;
	
	priv->rtllib->enableRateAdaptive = flag;
	if(flag)
		printk("\nEnable Rate Adaptive Mechanism.");
	else
		printk("\nDisable Rate Adaptive Mechanism.");
		
        up(&priv->wx_sem);
        return 0;

}
#endif

/* 
 * the following rule need to be follwing,
 * Odd : get (world access), 
 * even : set (root access) 
 * */
static const struct iw_priv_args apdev_private_args[] = { 
	{
		SIOCIWFIRSTPRIV + 0x2, 
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "set_debugflag" 
	}
	, 
	{
		SIOCIWFIRSTPRIV + 0x3,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "forcereset"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x4,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "force_mic_error"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x5,
		IW_PRIV_TYPE_NONE, IW_PRIV_TYPE_INT|IW_PRIV_SIZE_FIXED|1, 
		"firm_ver"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x16,
                IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 3, 0, "setpromisc"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x17,
		0,IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_FIXED | 45, "getpromisc"
	}
#ifdef RATE_ADAPTIVE_FOR_AP
	,
	{
		SIOCIWFIRSTPRIV + 0x1a,
                IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "set_ra"
	}
#endif
};

static iw_handler apdev_private_handler[] = {
	
	(iw_handler)NULL,
	(iw_handler)NULL,
	(iw_handler)apdev_wx_set_debugflag,   /*SIOCIWSECONDPRIV*/	
	(iw_handler)apdev_wx_force_reset,
	(iw_handler)apdev_wx_force_mic_error,
	(iw_handler)apdev_wx_get_firm_version,
	(iw_handler)NULL,	    
	(iw_handler)NULL, 
	(iw_handler)NULL,
	(iw_handler)NULL,
	(iw_handler)NULL,
	(iw_handler)NULL,
	(iw_handler)NULL,
	(iw_handler)NULL,
	(iw_handler)NULL,
	(iw_handler)NULL,
	(iw_handler)NULL,
	(iw_handler)NULL,
	(iw_handler)NULL,
 	(iw_handler)NULL,
	(iw_handler)NULL,
	(iw_handler)NULL,
	(iw_handler)apdev_wx_set_PromiscuousMode,
	(iw_handler)apdev_wx_get_PromiscuousMode,
	(iw_handler)NULL,
	(iw_handler)NULL,
#ifdef RATE_ADAPTIVE_FOR_AP
	(iw_handler)apdev_wx_set_rate_adaptive,
#else
	(iw_handler)NULL,
#endif
};
struct iw_statistics *apdev_get_wireless_stats(struct net_device *apdev)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct r8192_priv * priv = appriv->priv;
	struct rtllib_device *ieee = appriv->rtllib;
	struct iw_statistics* wstats = &priv->ap_wstats;
	int tmp_level = 0;
	int tmp_qual = 0;
	int tmp_noise = 0;
	if(ieee->state < RTLLIB_LINKED)
	{
		wstats->qual.qual = 10;
		wstats->qual.level = 0;
		wstats->qual.noise = -100;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14)) 
		wstats->qual.updated = IW_QUAL_ALL_UPDATED | IW_QUAL_DBM;
#else
		wstats->qual.updated = 0x0f;
#endif
		return wstats;
	}
	
       tmp_level = (&ieee->current_ap_network)->stats.rssi;
	tmp_qual = (&ieee->current_ap_network)->stats.signal;
	tmp_noise = (&ieee->current_ap_network)->stats.noise;			
	
	wstats->qual.level = tmp_level;
	wstats->qual.qual = tmp_qual;
	wstats->qual.noise = tmp_noise;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14)) 
	wstats->qual.updated = IW_QUAL_ALL_UPDATED | IW_QUAL_DBM;
#else
	wstats->qual.updated = 0x0f;
#endif
	return wstats;
}
struct iw_handler_def  apdev_wx_handlers_def={
	.standard = apdev_wx_handlers,
	.num_standard = sizeof(apdev_wx_handlers) / sizeof(iw_handler),
	.private = apdev_private_handler,
	.num_private = sizeof(apdev_private_handler) / sizeof(iw_handler),
 	.num_private_args = sizeof(apdev_private_args) / sizeof(struct iw_priv_args),
#if WIRELESS_EXT >= 17	
	.get_wireless_stats = apdev_get_wireless_stats,
#endif
	.private_args = (struct iw_priv_args *)apdev_private_args,	
};
#endif
#ifdef _RTL8192_EXT_PATCH_
#define OID_802_11_MESH_SECURITY_INFO 0x0651
#define OID_802_11_MESH_ID 0x0652
#define OID_802_11_MESH_AUTO_LINK 0x0653
#define OID_802_11_MESH_LINK_STATUS 0x0654
#define OID_802_11_MESH_LIST 0x0655
#define OID_802_11_MESH_ROUTE_LIST 0x0656
#define OID_802_11_MESH_ADD_LINK 0x0657
#define OID_802_11_MESH_DEL_LINK 0x0658
#define OID_802_11_MESH_MAX_TX_RATE 0x0659
#define OID_802_11_MESH_CHANNEL 0x065A
#define OID_802_11_MESH_HOSTNAME	0x065B
#define OID_802_11_MESH_ONLY_MODE  0x065C

#define OID_GET_SET_TOGGLE 0x8000
#define RTL_OID_802_11_MESH_SECURITY_INFO (OID_GET_SET_TOGGLE + OID_802_11_MESH_SECURITY_INFO)
#define RTL_OID_802_11_MESH_ID (OID_GET_SET_TOGGLE + OID_802_11_MESH_ID)
#define RTL_OID_802_11_MESH_AUTO_LINK (OID_GET_SET_TOGGLE + OID_802_11_MESH_AUTO_LINK)
#define RTL_OID_802_11_MESH_ADD_LINK (OID_GET_SET_TOGGLE + OID_802_11_MESH_ADD_LINK)
#define RTL_OID_802_11_MESH_DEL_LINK (OID_GET_SET_TOGGLE + OID_802_11_MESH_DEL_LINK)
#define RTL_OID_802_11_MESH_MAX_TX_RATE (OID_GET_SET_TOGGLE + OID_802_11_MESH_MAX_TX_RATE)
#define RTL_OID_802_11_MESH_CHANNEL (OID_GET_SET_TOGGLE + OID_802_11_MESH_CHANNEL)
#define RTL_OID_802_11_MESH_HOSTNAME		(OID_GET_SET_TOGGLE + OID_802_11_MESH_HOSTNAME)
#define RTL_OID_802_11_MESH_ONLY_MODE    (OID_GET_SET_TOGGLE + OID_802_11_MESH_ONLY_MODE)

#define MAX_NEIGHBOR_NUM 64 
typedef struct _MESH_NEIGHBOR_ENTRY
{
	char Rssi;
	unsigned char HostName[MAX_HOST_NAME_LENGTH];
	unsigned char MacAddr[ETH_ALEN];
	unsigned char MeshId[MAX_MESH_ID_LEN];
	unsigned char Channel;
	unsigned char Status; 
	unsigned char MeshEncrypType;		
} MESH_NEIGHBOR_ENTRY, *PMESH_NEIGHBOR_ENTRY;
typedef struct _MESH_NEIGHBOR_INFO
{
	MESH_NEIGHBOR_ENTRY Entry[MAX_NEIGHBOR_NUM];
	unsigned char num;
} MESH_NEIGHBOR_INFO, *PMESH_NEIGHBOR_INFO;

static int meshdev_set_key_for_linked_peers(struct net_device *dev, u8 KeyIndex, 
		                            u16 KeyType, u32 *KeyContent )
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;
	struct mshclass *mshobj = priv->mshobj;
	PMESH_NEIGHBOR_INFO  pmesh_neighbor = NULL;
	PMESH_NEIGHBOR_ENTRY pneighbor_entry = NULL;
	u8 entry_idx = 0;
	int i = 0;
	int found_idx = MAX_MP-1;
	pmesh_neighbor = (PMESH_NEIGHBOR_INFO)kmalloc(sizeof(MESH_NEIGHBOR_INFO), GFP_KERNEL);
	if(NULL == pmesh_neighbor)
		return -1;
	
	if(mshobj->ext_patch_r819x_get_peers)
		mshobj->ext_patch_r819x_get_peers(dev, (void*)pmesh_neighbor);
	
	for (i=0; i<pmesh_neighbor->num; i++) {
		pneighbor_entry = (PMESH_NEIGHBOR_ENTRY)&pmesh_neighbor->Entry[i];
		if (mshobj->ext_patch_r819x_insert_linking_crypt_peer_queue)
			found_idx = mshobj->ext_patch_r819x_insert_linking_crypt_peer_queue(ieee,pneighbor_entry->MacAddr);
		if (found_idx == -1) {
			printk("%s(): found_idx is -1 , something is wrong, return\n",__FUNCTION__);
			return -1;
		} else if (found_idx == (MAX_MP - 1)) {
			printk("%s(): found_idx is MAX_MP-1, peer entry is full, return\n",__FUNCTION__);
			return -1;
		}
		if ((((ieee->LinkingPeerBitMap>>found_idx) & (BIT0)) == BIT0) && ((ieee->LinkingPeerSecState[found_idx] == USED) )) {
			entry_idx = rtl8192_get_free_hwsec_cam_entry(ieee, pneighbor_entry->MacAddr);
#if 0
			printk("%s: Can not find free hw security cam entry, use software encryption entry(%d)\n", __FUNCTION__,entry_idx);
			if (mshobj->ext_patch_r819x_set_msh_peer_entry_sec_info)
				mshobj->ext_patch_r819x_set_msh_peer_entry_sec_info(ieee,pneighbor_entry->MacAddr,SW_SEC);
			ieee->LinkingPeerSecState[found_idx] = SW_SEC; 
#else
			if (entry_idx >=  TOTAL_CAM_ENTRY-1) {
				printk("%s: Can not find free hw security cam entry, use software encryption entry(%d)\n", __FUNCTION__,entry_idx);
				if (mshobj->ext_patch_r819x_set_msh_peer_entry_sec_info)
					mshobj->ext_patch_r819x_set_msh_peer_entry_sec_info(ieee,pneighbor_entry->MacAddr,SW_SEC);
				ieee->LinkingPeerSecState[found_idx] = SW_SEC; 
			} else {
				printk("==========>%s():entry_idx is %d,set HW CAM\n",__FUNCTION__,entry_idx);
				set_swcam( dev,
						entry_idx,
						KeyIndex, 
						KeyType,  
						pneighbor_entry->MacAddr, 
						0,              
						KeyContent,           
						1,
						0);
				setKey( dev,
						entry_idx,
						KeyIndex, 
						KeyType,  
						pneighbor_entry->MacAddr, 
						0,              
						KeyContent);           
						if (mshobj->ext_patch_r819x_set_msh_peer_entry_sec_info)
							mshobj->ext_patch_r819x_set_msh_peer_entry_sec_info(ieee,pneighbor_entry->MacAddr,HW_SEC);
						ieee->LinkingPeerSecState[found_idx] = HW_SEC; 
			}
#endif
		}
	}
	if(pmesh_neighbor)
		kfree(pmesh_neighbor);
	return 0;
}

int meshdev_set_key_for_peer(struct net_device *dev, 
								u8 *Addr,
								u8 KeyIndex, 
								u16 KeyType, 
								u32 *KeyContent )
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;
	u8 entry_idx = 0;
	
	entry_idx = rtl8192_get_free_hwsec_cam_entry(ieee, Addr);
#if 0
	printk("%s: Can not find free hw security cam entry\n", __FUNCTION__);
	return -1;
#else
	if (entry_idx >=  TOTAL_CAM_ENTRY-1) {
		printk("%s: Can not find free hw security cam entry\n", __FUNCTION__);
		return -1;
	} else {
		set_swcam(dev,
			entry_idx,
			KeyIndex, 
			KeyType,  
			Addr, 
			0,              
			KeyContent,           
			1,
			0);
		setKey(dev,
			entry_idx,
			KeyIndex, 
			KeyType,  
			Addr, 
			0,              
			KeyContent);           
	}
	return 0;
#endif
}

static struct net_device_stats *meshdev_stats(struct net_device *meshdev)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0) 
	return &((struct meshdev_priv*)netdev_priv(meshdev))->stats;
#else
	return &((struct meshdev_priv*)meshdev->priv)->stats;
#endif
}

static int meshdev_wx_get_name(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_get_name(dev, info, wrqu, extra);
}
static int meshdev_wx_get_freq(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;
	struct r8192_priv *priv = rtllib_priv(dev);

	return rtllib_wx_get_freq(priv->rtllib,info,wrqu,extra,1);
}
static int meshdev_wx_get_mode(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

	if(0)
		return r8192_wx_get_mode(dev, info, wrqu, extra);
	else
		return -1;
}	
static int meshdev_wx_get_sens(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;
	
	return r8192_wx_get_sens(dev, info, wrqu, extra);
}
static int meshdev_wx_get_range(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

	return rtl8192_wx_get_range(dev, info, wrqu, extra);
}
static int meshdev_wx_get_wap(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_get_wap(dev, info, wrqu, extra);
}
static int meshdev_wx_get_essid(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;
	struct r8192_priv *priv = rtllib_priv(dev);
	struct mshclass *mshobj = priv->mshobj;
	int ret = 0;	

	if(mshobj->ext_patch_r819x_wx_get_meshid)
		ret = mshobj->ext_patch_r819x_wx_get_meshid(dev, info, wrqu, extra);

	return ret;
}
static int meshdev_wx_get_rate(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_get_rate(dev, info, wrqu, extra);
}
#if 0
static int meshdev_wx_set_freq(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;
	
	return r8192_wx_set_freq(dev, info, wrqu, extra);
}
static int meshdev_wx_set_rate(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;
	
	return r8192_wx_set_rate(dev, info, wrqu, extra);
}
static int meshdev_wx_set_sens(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_set_sens(dev, info, wrqu, extra);
}
static int meshdev_wx_set_scan(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_set_scan(dev, info, wrqu, extra);
}
static int meshdev_wx_get_scan(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_get_scan(dev, info, wrqu, extra);
}
#endif
static int meshdev_wx_set_enc(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *key)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;
	struct r8192_priv *priv = (struct r8192_priv *)ieee->priv;
	int ret;
	u32 hwkey[4]={0,0,0,0};
	u8 mask=0xff;
	u32 key_idx=0;
#if 0
	u8 zero_addr[4][6] ={	{0x00,0x00,0x00,0x00,0x00,0x00},
				{0x00,0x00,0x00,0x00,0x00,0x01}, 
				{0x00,0x00,0x00,0x00,0x00,0x02}, 
				{0x00,0x00,0x00,0x00,0x00,0x03} };
#endif
	int i;

       if(!priv->mesh_up) return -ENETDOWN;

	SEM_DOWN_PRIV_WX(&priv->wx_sem);	
	
	RT_TRACE(COMP_SEC, "Setting SW wep key");
	ret = rtllib_wx_set_encode(priv->rtllib,info,wrqu,key,1);

	SEM_UP_PRIV_WX(&priv->wx_sem);	


	if(wrqu->encoding.length!=0){

		for(i=0 ; i<4 ; i++){
			hwkey[i] |=  key[4*i+0]&mask;
			if(i==1&&(4*i+1)==wrqu->encoding.length) mask=0x00;
			if(i==3&&(4*i+1)==wrqu->encoding.length) mask=0x00;
			hwkey[i] |= (key[4*i+1]&mask)<<8;
			hwkey[i] |= (key[4*i+2]&mask)<<16;
			hwkey[i] |= (key[4*i+3]&mask)<<24;
		}

		#define CONF_WEP40  0x4
		#define CONF_WEP104 0x14

		switch(wrqu->encoding.flags & IW_ENCODE_INDEX){
			case 0: key_idx = ieee->mesh_txkeyidx; break;
			case 1:	key_idx = 0; break;
			case 2:	key_idx = 1; break;
			case 3:	key_idx = 2; break;
			case 4:	key_idx	= 3; break;
			default: break;
		}

		if(wrqu->encoding.length==0x5){
		ieee->mesh_pairwise_key_type = KEY_TYPE_WEP40;
			EnableHWSecurityConfig8192(dev);
		}

		else if(wrqu->encoding.length==0xd){
			ieee->mesh_pairwise_key_type = KEY_TYPE_WEP104;
				EnableHWSecurityConfig8192(dev);
		}
		else 
			printk("wrong type in WEP, not WEP40 and WEP104\n");

		meshdev_set_key_for_linked_peers(dev,
						key_idx, 
						ieee->mesh_pairwise_key_type,  
						hwkey); 

	}

#if 0
	if(wrqu->encoding.length==0 && (wrqu->encoding.flags >>8) == 0x8 ){
		printk("===>1\n");		
		EnableHWSecurityConfig8192(dev);
		key_idx = (wrqu->encoding.flags & 0xf)-1 ;
		write_cam(dev, (4*6),   0xffff0000|read_cam(dev, key_idx*6) );
		write_cam(dev, (4*6)+1, 0xffffffff);
		write_cam(dev, (4*6)+2, read_cam(dev, (key_idx*6)+2) );
		write_cam(dev, (4*6)+3, read_cam(dev, (key_idx*6)+3) );
		write_cam(dev, (4*6)+4, read_cam(dev, (key_idx*6)+4) );
		write_cam(dev, (4*6)+5, read_cam(dev, (key_idx*6)+5) );
	}
#endif

	return ret;
}
static int meshdev_wx_get_enc(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device * dev = ieee->dev;
	struct r8192_priv* priv = rtllib_priv(dev);

	if(!priv->mesh_up){
		return -ENETDOWN;
	}
	return rtllib_wx_get_encode(ieee, info, wrqu, extra,1);
}
#if 0
static int meshdev_wx_set_frag(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_set_frag(dev, info, wrqu, extra);
}
static int meshdev_wx_get_frag(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_get_frag(dev, info, wrqu, extra);
}
static int meshdev_wx_set_retry(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_set_retry(dev, info, wrqu, extra);
}
static int meshdev_wx_get_retry(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_get_retry(dev, info, wrqu, extra);
}
#endif
static int meshdev_wx_set_mode(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
#if 0
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;
	
	return r8192_wx_set_mode(dev, info, wrqu, extra);
#endif
	return 0;
}
static int meshdev_wx_set_wap(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
#if 0
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_set_wap(dev, info, wrqu, extra);
#endif
	return 0;
}
static int meshdev_wx_set_mlme(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
#if 0
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_set_mlme(dev, info, wrqu, extra);
#endif
	return 0;
}
static int meshdev_wx_set_essid(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
#if 0
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_set_essid(dev, info, wrqu, extra);
#endif
	return 0;
}
static int meshdev_wx_set_gen_ie(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
#if 0
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_set_gen_ie(dev, info, wrqu, extra);
#endif
	return 0;
}
static int meshdev_wx_set_auth(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
#if 0
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

	return r8192_wx_set_auth(dev, info, wrqu, extra);
#endif
	return 0;
}
static int meshdev_wx_set_enc_ext(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	int ret=0;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;
	struct r8192_priv *priv = (struct r8192_priv *)ieee->priv;

	printk("============================================================>%s\n", __FUNCTION__);
	SEM_DOWN_PRIV_WX(&priv->wx_sem);	
	ret = rtllib_wx_set_encode_ext(ieee, info, wrqu, extra, 1);

	ret |= r8192_set_hw_enc(dev,info,wrqu,extra, 1);
	SEM_UP_PRIV_WX(&priv->wx_sem);	
#endif

	return ret;	
}

static int dummy(struct net_device *dev, struct iw_request_info *a,
		 union iwreq_data *wrqu,char *b)
{
	return -1;
}

int rt_ioctl_siwpmksa(struct net_device *dev,
			   struct iw_request_info *info,
			   union iwreq_data *wrqu,
			   char *extra)
{
	return 0;
}

static iw_handler meshdev_wx_handlers[] =
{
	NULL,                     /* SIOCSIWCOMMIT */
	meshdev_wx_get_name,   	  /* SIOCGIWNAME */
	dummy,                    /* SIOCSIWNWID */
	dummy,                    /* SIOCGIWNWID */
	NULL,  
	meshdev_wx_get_freq,        /* SIOCGIWFREQ */
	meshdev_wx_set_mode,        /* SIOCSIWMODE */
	meshdev_wx_get_mode,        /* SIOCGIWMODE */
	NULL,  
	meshdev_wx_get_sens,        /* SIOCGIWSENS */
	NULL,                     /* SIOCSIWRANGE */
	meshdev_wx_get_range,	  /* SIOCGIWRANGE */
	NULL,                     /* SIOCSIWPRIV */
	NULL,                     /* SIOCGIWPRIV */
	NULL,                     /* SIOCSIWSTATS */
	NULL,                     /* SIOCGIWSTATS */
	dummy,                    /* SIOCSIWSPY */
	dummy,                    /* SIOCGIWSPY */
	NULL,                     /* SIOCGIWTHRSPY */
	NULL,                     /* SIOCWIWTHRSPY */
	meshdev_wx_set_wap,      	  /* SIOCSIWAP */
	meshdev_wx_get_wap,         /* SIOCGIWAP */
	meshdev_wx_set_mlme, 
	dummy,                     /* SIOCGIWAPLIST -- depricated */
	NULL,  
	NULL,  
	meshdev_wx_set_essid,       /* SIOCSIWESSID */
	meshdev_wx_get_essid,       /* SIOCGIWESSID */
	dummy,                    /* SIOCSIWNICKN */
	dummy,                    /* SIOCGIWNICKN */
	NULL,                     /* -- hole -- */
	NULL,                     /* -- hole -- */
	NULL,  
	meshdev_wx_get_rate,        /* SIOCGIWRATE */
	dummy,                    /* SIOCSIWRTS */
	dummy,                    /* SIOCGIWRTS */
	NULL,  
	NULL,  
	dummy,                    /* SIOCSIWTXPOW */
	dummy,                    /* SIOCGIWTXPOW */
	NULL,  
	NULL,  
	meshdev_wx_set_enc,         /* SIOCSIWENCODE */
	meshdev_wx_get_enc,         /* SIOCGIWENCODE */
	dummy,                    /* SIOCSIWPOWER */
	dummy,                    /* SIOCGIWPOWER */
	NULL,			/*---hole---*/
	NULL, 			/*---hole---*/
	meshdev_wx_set_gen_ie,
	NULL, 			/* SIOCSIWGENIE */
	meshdev_wx_set_auth,
	NULL, 
	meshdev_wx_set_enc_ext, 			/* SIOCSIWENCODEEXT */
	NULL, 
	(iw_handler) rt_ioctl_siwpmksa,
	NULL, 			 /*---hole---*/
}; 

static struct iw_priv_args meshdev_private_args[] = { 
	{
		SIOCIWFIRSTPRIV + 0x0, 
		0, 0, "enablemesh" 
 	},
	{
		SIOCIWFIRSTPRIV + 0x1, 
		0, IW_PRIV_TYPE_CHAR | 64, "getmeshinfo"
 	},
	{
		SIOCIWFIRSTPRIV + 0x2, 
		0, 0, "disablemesh"
 	},
 	{
		SIOCIWFIRSTPRIV + 0x3, 
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "setch"
 	},
 	{
		SIOCIWFIRSTPRIV + 0x4, 
		IW_PRIV_TYPE_CHAR | 64  , 0, "setmeshid"
 	},
        {	SIOCIWFIRSTPRIV + 0x5, 
		0,IW_PRIV_TYPE_CHAR | 64 , "getmeshlist"
 	},
	{	SIOCIWFIRSTPRIV + 0x6, 
		IW_PRIV_TYPE_CHAR | 64,0 , "meshscan"
 	},
	{
		SIOCIWFIRSTPRIV + 0x7,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "setmode"

	},
 	{
		SIOCIWFIRSTPRIV + 0x8, 
		IW_PRIV_TYPE_CHAR | 64, 0, "sethostname"
 	},
	{
		SIOCIWFIRSTPRIV + 0x9,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "setBW"

	},
	{
		SIOCIWFIRSTPRIV + 0xa, 
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "updateBW"
 	}, 
 	{	SIOCIWFIRSTPRIV + 0xb, 
		0,IW_PRIV_TYPE_CHAR | 256 , "macdenyget"
 	},
 	{	SIOCIWFIRSTPRIV + 0xc, 
		IW_PRIV_TYPE_CHAR | 64,0 , "macdenyadd"
 	},
	/*
	{
		SIOCIWFIRSTPRIV + 0xe, 
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK, IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK, ""
 	},*/
        {	SIOCIWFIRSTPRIV + 0xf, 
		0, IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK , "getneighborlist"
 	},
	/* Sub-ioctls definitions*/
	/*
	{
		OID_802_11_MESH_ID, 
		IW_PRIV_TYPE_INT | 2047, IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK, "meshid"
	},
	{
		OID_802_11_MESH_LIST, 
		IW_PRIV_TYPE_INT | 2047, IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK, "meshlist"
	},*/
	{	SIOCIWFIRSTPRIV + 0x10, 
		IW_PRIV_TYPE_CHAR | 64, 0, "set"
 	},
	{	SIOCIWFIRSTPRIV + 0x12, 
		IW_PRIV_TYPE_CHAR | 64,0 , "macdenydel"
 	},
	{
		SIOCIWFIRSTPRIV + 0x14,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "setmeshsec"
	},
	{
		SIOCIWFIRSTPRIV + 0x15,
		IW_PRIV_TYPE_CHAR | 6, 0, "setmkddid"
	},
	{
		SIOCIWFIRSTPRIV + 0x16,
		IW_PRIV_TYPE_CHAR | 64, 0, "setkey"
	},
	{
		SIOCIWFIRSTPRIV + 0x17,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "setsectype"
	},
};

int meshdev_wx_mesh(struct net_device *meshdev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *meshpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device * ieee = meshpriv->rtllib;
	struct net_device *dev = ieee->dev;
	struct r8192_priv *priv = rtllib_priv(dev);
	struct mshclass *mshobj = priv->mshobj;
	int ret = 0;

	printk("@@@@@%s: ", __FUNCTION__);
	if(mshobj)
	{
		switch(wrqu->data.flags)
		{
			case OID_802_11_MESH_SECURITY_INFO:
			{
				printk("OID_802_11_MESH_SECURITY_INFO \n");
				if(mshobj->ext_patch_r819x_wx_get_security_info)
					ret = mshobj->ext_patch_r819x_wx_get_security_info(dev, info, wrqu, extra);
				break;
			}
			case OID_802_11_MESH_ID:
			{
				printk("OID_802_11_MESH_ID \n");
				if(mshobj->ext_patch_r819x_wx_get_meshid)
					ret = mshobj->ext_patch_r819x_wx_get_meshid(dev, info, wrqu, extra);
				break;
			}
			case OID_802_11_MESH_AUTO_LINK:
			{
				printk("OID_802_11_MESH_AUTO_LINK \n");
				if(mshobj->ext_patch_r819x_wx_get_auto_link)
					ret = mshobj->ext_patch_r819x_wx_get_auto_link(dev, info, wrqu, extra);
				break;
			}
			case OID_802_11_MESH_LINK_STATUS:
			{
				printk("OID_802_11_MESH_LINK_STATUS \n");
				if(mshobj->ext_patch_r819x_wx_get_link_status)
					ret = mshobj->ext_patch_r819x_wx_get_link_status(dev, info, wrqu, extra);
				break;
			}
			case OID_802_11_MESH_LIST:
			{
				printk("OID_802_11_MESH_LIST \n");
				if(mshobj->ext_patch_r819x_wx_get_neighbor_list)
					ret = mshobj->ext_patch_r819x_wx_get_neighbor_list(dev, info, wrqu, extra);
				break;
			}
			case OID_802_11_MESH_ROUTE_LIST:
			{
				printk("OID_802_11_MESH_ROUTE_LIST \n");
				if(mshobj->ext_patch_r819x_wx_get_route_list)
					ret = mshobj->ext_patch_r819x_wx_get_route_list(dev, info, wrqu, extra);
				break;
			}
			case OID_802_11_MESH_MAX_TX_RATE:
			{
				printk("OID_802_11_MESH_MAX_TX_RATE \n");
				if(mshobj->ext_patch_r819x_wx_get_maxrate)
					ret = mshobj->ext_patch_r819x_wx_get_maxrate(dev, info, wrqu, extra);
				break;
			}
			case OID_802_11_MESH_CHANNEL:
			{
				printk("OID_802_11_MESH_CHANNEL \n");
				if(mshobj->ext_patch_r819x_wx_get_channel)
					ret = mshobj->ext_patch_r819x_wx_get_channel(dev, info, wrqu, extra);
				break;
			}
			case OID_802_11_MESH_HOSTNAME:
			{
				printk("OID_802_11_MESH_HOSTNAME \n");
				if(mshobj->ext_patch_r819x_wx_get_host_name)
					ret = mshobj->ext_patch_r819x_wx_get_host_name(dev, info, wrqu, extra);
				break;
			}
			case OID_802_11_MESH_ONLY_MODE:
			{
				printk("OID_802_11_MESH_ONLY_MODE \n");
				if(mshobj->ext_patch_r819x_wx_get_mesh_only_mode)
					ret = mshobj->ext_patch_r819x_wx_get_mesh_only_mode(dev, info, wrqu, extra);
				break;
			}
			case RTL_OID_802_11_MESH_SECURITY_INFO:
			{
				printk("RTL_OID_802_11_MESH_SECURITY_INFO \n");
				if(mshobj->ext_patch_r819x_wx_set_security_info)
					ret = mshobj->ext_patch_r819x_wx_set_security_info(dev, info, wrqu, extra);
				break;
			}
			case RTL_OID_802_11_MESH_ID:
			{
				printk("RTL_OID_802_11_MESH_ID \n");
				if(mshobj->ext_patch_r819x_wx_set_meshID)
					ret = mshobj->ext_patch_r819x_wx_set_meshID(dev, (u8*)wrqu->data.pointer);
				break;
			}
			case RTL_OID_802_11_MESH_AUTO_LINK:
			{
				printk("RTL_OID_802_11_MESH_AUTO_LINK \n");
				if(mshobj->ext_patch_r819x_wx_set_auto_link)
					ret = mshobj->ext_patch_r819x_wx_set_auto_link(dev, info, wrqu, extra);
				break;
			}
			case RTL_OID_802_11_MESH_ADD_LINK:
			{
				printk("RTL_OID_802_11_MESH_ADD_LINK \n");
				if(mshobj->ext_patch_r819x_wx_set_add_link)
					ret = mshobj->ext_patch_r819x_wx_set_add_link(dev, info, wrqu, extra);
				break;
			}
			case RTL_OID_802_11_MESH_DEL_LINK:
			{
				printk("RTL_OID_802_11_MESH_DEL_LINK \n");
				if(mshobj->ext_patch_r819x_wx_set_del_link)
					ret = mshobj->ext_patch_r819x_wx_set_del_link(dev, info, wrqu, extra);
				break;
			}
			case RTL_OID_802_11_MESH_MAX_TX_RATE:
			{
				printk("RTL_OID_802_11_MESH_MAX_TX_RATE \n");
				if(mshobj->ext_patch_r819x_wx_set_maxrate)
					ret = mshobj->ext_patch_r819x_wx_set_maxrate(dev, info, wrqu, extra);
				break;
			}
			case RTL_OID_802_11_MESH_CHANNEL:
			{
				printk("RTL_OID_802_11_MESH_CHANNEL \n");
				printk("channel = %d\n",*(u8*)wrqu->data.pointer);
				r8192_wx_set_channel(dev, info, wrqu, wrqu->data.pointer);
				break;
			}
			case RTL_OID_802_11_MESH_HOSTNAME:
			{
				printk("RTL_OID_802_11_MESH_HOSTNAME \n");
				if(mshobj->ext_patch_r819x_wx_set_host_name)
					ret = mshobj->ext_patch_r819x_wx_set_host_name(dev, info, wrqu, extra);
				break;
			}
			case RTL_OID_802_11_MESH_ONLY_MODE:
			{
				printk("RTL_OID_802_11_MESH_ONLY_MODE \n");
				if(mshobj->ext_patch_r819x_wx_set_mesh_only_mode)
					ret = mshobj->ext_patch_r819x_wx_set_mesh_only_mode(dev, info, wrqu, extra);
				break;
			}
			default:
				printk("Default \n");
				break;
		}
	}
	return ret;
}

static int meshdev_wx_get_meshinfo(struct net_device *meshdev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct net_device *dev = mpriv->rtllib->dev;
	
printk("++++++======%s: dev=%p length=%d extra=%p\n", __FUNCTION__, dev, wrqu->data.length,extra);
	return r8192_wx_get_meshinfo(dev, info, wrqu, extra);
}

static int meshdev_wx_enable_mesh(struct net_device *meshdev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;
	
printk("++++++======%s\n", __FUNCTION__);
	return r8192_wx_enable_mesh(dev, info, wrqu, extra);
	
}

static int meshdev_wx_disable_mesh(struct net_device *meshdev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;
	
printk("++++++======%s\n", __FUNCTION__);
	return r8192_wx_disable_mesh(dev, info, wrqu, extra);
}


int meshdev_wx_set_channel(struct net_device *meshdev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;
printk("++++++======%s\n", __FUNCTION__);

	return r8192_wx_set_channel(dev, info, wrqu, extra);
}

static int meshdev_wx_set_meshid(struct net_device *meshdev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{	
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;
printk("++++++======%s\n", __FUNCTION__);

	return r8192_wx_set_meshID(dev, info, wrqu, extra);	
}

static int meshdev_wx_mesh_scan(struct net_device *meshdev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

printk("++++++======%s\n", __FUNCTION__);
	return r8192_wx_mesh_scan(dev, info, wrqu, extra);
}
static int meshdev_wx_get_mesh_list(struct net_device *meshdev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;
printk("++++++======%s\n", __FUNCTION__);

	return r8192_wx_get_mesh_list(dev, info, wrqu, extra);
}
static int meshdev_wx_set_meshmode(struct net_device *meshdev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

printk("++++++======%s\n", __FUNCTION__);
	return r8192_wx_set_meshmode(dev, info, wrqu, extra);
}
static int meshdev_wx_set_meshbw(struct net_device *meshdev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

printk("++++++======%s\n", __FUNCTION__);
	return r8192_wx_set_meshBW(dev, info, wrqu, extra);
}
static int meshdev_wx_update_beacon(struct net_device *meshdev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;
	struct r8192_priv *priv = rtllib_priv(dev);
	struct mshclass *mshobj= (priv)->mshobj;
	u8 updateBW = 0;
	u8 bserverHT = 0;	
	
	printk("++++++======%s\n", __FUNCTION__);
	if(*extra == 0)
	{
		ieee->p2pmode = 1;
		ieee->current_network.channel = ieee->current_mesh_network.channel; 
		if(ieee->state!=RTLLIB_LINKED){
			if(ieee->pHTInfo->bCurBW40MHz)
				HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20_40, (ieee->current_mesh_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER);  
			else
				HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20, (ieee->current_mesh_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER);  
		}
	}
	else
	{
		ieee->p2pmode = 0;
		updateBW=mshobj->ext_patch_r819x_wx_update_beacon(dev,&bserverHT);
		printk("$$$$$$ Cur_networ.chan=%d, cur_mesh_net.chan=%d,bserverHT=%d\n", ieee->current_network.channel,ieee->current_mesh_network.channel,bserverHT);
		if(updateBW == 1)
		{
			if(bserverHT == 0)
			{
				printk("===>server is not HT supported,set 20M\n");
				HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20, HT_EXTCHNL_OFFSET_NO_EXT);  
			}
			else
			{
				printk("===>updateBW is 1,bCurBW40MHz is %d,ieee->serverExtChlOffset is %d\n",ieee->pHTInfo->bCurBW40MHz,ieee->serverExtChlOffset);		
				if(ieee->pHTInfo->bCurBW40MHz)
					HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20_40, ieee->serverExtChlOffset);  
				else
					HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20, ieee->serverExtChlOffset);  
			}
		}
		else
		{
			printk("===>there is no same hostname server, ERR!!!\n");
			return -1;
		}
	}
#ifdef RTL8192SE
	write_nic_dword(dev,BSSIDR,((u32*)priv->rtllib->current_mesh_network.bssid)[0]);
	write_nic_word(dev,BSSIDR+4,((u16*)priv->rtllib->current_mesh_network.bssid)[2]);
#elif defined RTL8192CE
	write_nic_dword(dev,REG_BSSID,((u32*)priv->rtllib->current_mesh_network.bssid)[0]);
	write_nic_word(dev,REG_BSSID+4,((u16*)priv->rtllib->current_mesh_network.bssid)[2]);
#endif

	return 0;
}
static int meshdev_wx_add_mac_deny(struct net_device *meshdev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;
	struct r8192_priv *priv = rtllib_priv(dev);
	
	
	if( ! priv->mshobj || !priv->mshobj->ext_patch_r819x_wx_set_add_mac_deny )
		return 0;

	return priv->mshobj->ext_patch_r819x_wx_set_add_mac_deny(dev, info, wrqu, extra);
}

static int meshdev_wx_del_mac_deny(struct net_device *meshdev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;
	struct r8192_priv *priv = rtllib_priv(dev);
	
	if( ! priv->mshobj || !priv->mshobj->ext_patch_r819x_wx_set_del_mac_deny )
		return 0;

	return priv->mshobj->ext_patch_r819x_wx_set_del_mac_deny(dev, info, wrqu, extra);
}

static int meshdev_wx_get_neighbor_list(struct net_device *meshdev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;
	struct r8192_priv *priv = rtllib_priv(dev);
	int ret = 0;

	if( ! priv->mshobj || !priv->mshobj->ext_patch_r819x_wx_get_neighbor_list )
		return 0;
	ret = priv->mshobj->ext_patch_r819x_wx_get_neighbor_list(dev, info, wrqu, extra);
#ifdef MESH_AUTO_TEST
	ret |= r8192_wx_set_channel(dev, info, wrqu, &ieee->current_network.channel);
#endif
	return ret;
}

/* reserved for future
static int r819x_wx_get_mac_allow(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
		return 0;

	return priv->mshobj->ext_patch_r819x_wx_get_mac_allow(dev, info, wrqu, extra);
}
*/

static int meshdev_wx_get_mac_deny(struct net_device *meshdev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;
	struct r8192_priv *priv = rtllib_priv(dev);
	
	if( ! priv->mshobj || !priv->mshobj->ext_patch_r819x_wx_get_mac_deny )
		return 0;

	return priv->mshobj->ext_patch_r819x_wx_get_mac_deny(dev, info, wrqu, extra);
}


static int meshdev_wx_set_hostname(struct net_device *meshdev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;

	if(wrqu->data.length > MAX_HOST_NAME_LENGTH)
	{
		printk("%s: Host name is too long. len=%d\n", __FUNCTION__, wrqu->data.length);
		return -1;
	}

	ieee->hostname_len = wrqu->data.length;
	memcpy(ieee->hostname, extra, wrqu->data.length);
	
printk("++++++======%s: %s\n", __FUNCTION__, ieee->hostname);

	return 0;
}
static int meshdev_wx_set_mesh_security(struct net_device *meshdev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = netdev_priv(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

	printk("++++++======%s\n", __FUNCTION__);
	return r8192_wx_set_mesh_security(dev, info, wrqu, extra);
}
static int meshdev_wx_set_mkdd_id(struct net_device *meshdev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = netdev_priv(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;
	

	printk("++++++======%s\n", __FUNCTION__);
	return r8192_wx_set_mkdd_id(dev, info, wrqu, extra);
}
static int meshdev_wx_set_mesh_key(struct net_device *meshdev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = netdev_priv(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

	printk("++++++======%s\n", __FUNCTION__);
	return r8192_wx_set_mesh_key(dev, info, wrqu, extra);
}
static int meshdev_wx_set_sec_type(struct net_device *meshdev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct meshdev_priv *mpriv = netdev_priv(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;

	printk("++++++======%s\n", __FUNCTION__);
	return r8192_wx_set_mesh_sec_type(dev, info, wrqu, extra);
}
static u8 my_atoi(const char *arg)
{
	u8 val = 0;
	for(; ; arg++){
		switch (*arg){
			case '0'...'9':
				val = 10*val + (*arg-'0');
				break;
			default:
				return val;
		}
	}
	return val;
}

static int Set_Channel_Proc(struct net_device *meshdev, char *arg)
{
	int ch = my_atoi(arg);
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct r8192_priv *priv = (void *)ieee->priv;
	struct net_device *dev = ieee->dev;

	if (!priv->mshobj || !priv->mshobj->ext_patch_r819x_wx_set_channel || !ieee->only_mesh)
		return 0;	
			
printk("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!set current mesh network channel %d\n", ch);
	if ( ch < 0 ) 	
	{
		rtllib_start_scan(ieee);			
		ieee->meshScanMode =2;
	}
	else	
	{	
		ieee->meshScanMode =0;		
		if(priv->mshobj->ext_patch_r819x_wx_set_channel)
		{
			priv->mshobj->ext_patch_r819x_wx_set_channel(ieee, ch);
			priv->mshobj->ext_patch_r819x_wx_set_mesh_chan(dev,ch);
		}
		queue_work_rsl(ieee->wq, &ieee->ext_stop_scan_wq);
		ieee->set_chan(ieee->dev, ch);
		ieee->current_mesh_network.channel = ch;
		if(ieee->only_mesh)
			ieee->current_network.channel = ch;
		ieee->current_network.channel = ieee->current_mesh_network.channel; 
		if(ieee->pHTInfo->bCurBW40MHz)
			HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20_40, (ieee->current_mesh_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER);  
		else
			HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20, (ieee->current_mesh_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER);  
		if(rtllib_act_scanning(ieee,true) == true)
			rtllib_stop_scan_syncro(ieee);
	}
	return 0;		
}
static int Set_MeshID_Proc(struct net_device *meshdev, char *arg)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;
	struct r8192_priv *priv = rtllib_priv(dev);
	RT_RF_POWER_STATE       rtState;
	int ret = 0;
        rtState = priv->rtllib->eRFPowerState;
	
	if(!priv->mshobj || !priv->mshobj->ext_patch_r819x_wx_enable_mesh || !priv->mshobj->ext_patch_r819x_wx_set_meshID)
		return 0;

	if(rtllib_act_scanning(ieee,true) == true)
		rtllib_stop_scan_syncro(ieee);
	
	/* Set Mesh ID */
	ret = priv->mshobj->ext_patch_r819x_wx_set_meshID(dev, arg);
	if(ret)
		goto End;
	
	/* Enable Mesh */
	SEM_DOWN_PRIV_WX(&priv->wx_sem);	
	ret = priv->mshobj->ext_patch_r819x_wx_enable_mesh(dev);
	if(!ret)
	{
#ifdef ENABLE_IPS
		if(priv->rtllib->PowerSaveControl.bInactivePs){
			if(rtState == eRfOff){
				if(priv->rtllib->RfOffReason > RF_CHANGE_BY_IPS)
				{
					RT_TRACE(COMP_ERR, "%s(): RF is OFF.\n",__FUNCTION__);
					SEM_UP_PRIV_WX(&priv->wx_sem);	
					return -1;
				}
				else{
					printk("=========>%s(): IPSLeave\n",__FUNCTION__);
					IPSLeave(dev);
				}
			}
		}
#endif
	}
	SEM_UP_PRIV_WX(&priv->wx_sem);	
#if 1
	if (ieee->mesh_sec_type == 1) {	
		rtl8192_abbr_handshk_set_GTK(ieee,1);
	}
#else
	if (ieee->mesh_sec_type == 1) 	
		rtl8192_abbr_handshk_set_key(ieee); 
#endif
End:
	return ret;	
}
static int Set_Bw40MHz_Proc(struct net_device *meshdev, char *arg)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct r8192_priv *priv = (void *)ieee->priv;
	u8 bBw40MHz = my_atoi(arg);

	SEM_DOWN_PRIV_WX(&priv->wx_sem);	
	printk("%s(): set mesh BW ! extra is %d\n",__FUNCTION__, bBw40MHz);
	priv->rtllib->pHTInfo->bRegBW40MHz = bBw40MHz;
	SEM_UP_PRIV_WX(&priv->wx_sem);	

	return 0;
}
static int Set_WirelessMode_Proc(struct net_device *meshdev, char *arg)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct r8192_priv *priv = (void *)ieee->priv;
	struct net_device *dev = ieee->dev;
	u8 wirelessmode = my_atoi(arg);

	SEM_DOWN_PRIV_WX(&priv->wx_sem);	

	printk("%s(): set mesh mode ! extra is %d\n",__FUNCTION__, wirelessmode);
	if((wirelessmode != WIRELESS_MODE_A) && (wirelessmode != WIRELESS_MODE_B) && 
		(wirelessmode != WIRELESS_MODE_G) && (wirelessmode != WIRELESS_MODE_AUTO) &&
		(wirelessmode != WIRELESS_MODE_N_24G) && (wirelessmode != WIRELESS_MODE_N_5G))
	{
		printk("ERR!! you should input 1 | 2 | 4 | 8 | 16 | 32\n");
		SEM_UP_PRIV_WX(&priv->wx_sem);	
		return -1;
	}
	if(priv->rtllib->state == RTLLIB_LINKED)
	{
		if((priv->rtllib->mode != WIRELESS_MODE_N_5G) && (priv->rtllib->mode != WIRELESS_MODE_N_24G)){
			printk("===>wlan0 is linked,and ieee->mode is not N mode ,do not need to set mode,return\n");
			SEM_UP_PRIV_WX(&priv->wx_sem);	
			return 0;
		}
	}
	priv->rtllib->mode = wirelessmode;
	if(priv->ResetProgress == RESET_TYPE_NORESET)
		rtl8192_SetWirelessMode(dev, priv->rtllib->mode);
	HTUseDefaultSetting(priv->rtllib);
	SEM_UP_PRIV_WX(&priv->wx_sem);	
	return 0;
}
static int Set_ExtChnOffset_Proc(struct net_device *meshdev, char *arg)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
#if 0
	struct r8192_priv *priv = (void *)ieee->priv;
	struct net_device *dev = ieee->dev;
	struct mshclass *mshobj= priv->mshobj;
	u8 updateBW = 0;
	u8 bserverHT = 0;	
#endif
	ieee->p2pmode = 0;	
#if 0
	updateBW=mshobj->ext_patch_r819x_wx_update_beacon(dev,&bserverHT);
	printk("$$$$$$ Cur_networ.chan=%d, cur_mesh_net.chan=%d,bserverHT=%d\n", ieee->current_network.channel,ieee->current_mesh_network.channel,bserverHT);
	if(updateBW == 1)
	{
		if(bserverHT == 0)
		{
			printk("===>server is not HT supported,set 20M\n");
			HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20, HT_EXTCHNL_OFFSET_NO_EXT);  
		}
		else
		{
			printk("===>updateBW is 1,bCurBW40MHz is %d,ieee->serverExtChlOffset is %d\n",ieee->pHTInfo->bCurBW40MHz,ieee->serverExtChlOffset);		
			if(ieee->pHTInfo->bCurBW40MHz)
				HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20_40, ieee->serverExtChlOffset);  
			else
				HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20, ieee->serverExtChlOffset);  
		}
	}
	else
	{
		printk("===>there is no same hostname server, ERR!!!\n");
		return -1;
	}
#endif
	return 0;
}
static int Set_OnlyMesh_Proc(struct net_device *meshdev, char *arg)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	union iwreq_data tmprqu;
	int ret = 0;

	ieee->p2pmode = 1;	
	ieee->only_mesh = my_atoi(arg);
printk("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!mesh only = %d, p2pmode = %d\n", ieee->only_mesh, ieee->p2pmode);
	if(ieee->only_mesh)
		ieee->current_network.channel = ieee->current_mesh_network.channel;
	if(ieee->only_mesh == 0)
	{
		tmprqu.mode = ieee->iw_mode;
		ieee->iw_mode = IW_MODE_INFRA; 
		ret = rtllib_wx_set_mode(ieee, NULL, &tmprqu, NULL);
	}
	return ret;
}
static int Set_AsPortal_Proc(struct net_device *meshdev, char *arg)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;
	struct r8192_priv *priv = (void *)ieee->priv;
	u8 val = my_atoi(arg);
	int ret = 0;

	SEM_DOWN_PRIV_WX(&priv->wx_sem);	
	ret = priv->mshobj->ext_patch_r819x_wx_set_AsPortal(dev, val);
	SEM_UP_PRIV_WX(&priv->wx_sem);	

	return ret;
}
static int Set_AsRoot_Proc(struct net_device *meshdev, char *arg)
{
	struct meshdev_priv *mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
	struct rtllib_device *ieee = mpriv->rtllib;
	struct net_device *dev = ieee->dev;
	struct r8192_priv *priv = (void *)ieee->priv;
	u8 val = my_atoi(arg);
	int ret = 0;

	SEM_DOWN_PRIV_WX(&priv->wx_sem);	
	ret = priv->mshobj->ext_patch_r819x_wx_set_AsRoot(dev, val);
	SEM_UP_PRIV_WX(&priv->wx_sem);	

	return ret;
}

static struct {
	char* name;
	int (*set_proc)(struct net_device *dev, char *arg);
} *private_set_proc, private_support_proc[] = {
	{"Debug",				NULL},
	{"Channel",				Set_Channel_Proc},            
	{"MeshId",				Set_MeshID_Proc},
	{"Bw40MHz",				Set_Bw40MHz_Proc},
	{"WirelessMode",			Set_WirelessMode_Proc},
	{"ExtChnOffset",			Set_ExtChnOffset_Proc},
	{"OnlyMesh",				Set_OnlyMesh_Proc},
	{"AsPortal",				Set_AsPortal_Proc},
	{"AsRoot",				Set_AsRoot_Proc},
	{"MeshAuthMode",			NULL},       
	{"MeshEncrypType",			NULL},  
	{"<NULL>",				NULL},
};

static char *rtlstrchr(const char *s, int c)
{
	for(; *s!=(char)c; ++s)
		if(*s == '\0')
			return NULL;
	return (char *)s;
}

static int meshdev_wx_set_param(struct net_device *dev, struct iw_request_info *info,
			       union iwreq_data *w, char *extra)
{
	char * this_char = extra;
	char *value = NULL;
	int  Status=0;

	printk("=======>%s: extra=%s\n", __FUNCTION__,extra);
	if (!*this_char)
		return -EINVAL; 
	
	if ((value = rtlstrchr(this_char, '=')) != NULL)
		*value++ = 0;

	for (private_set_proc = private_support_proc; strcmp(private_set_proc->name, "<NULL>"); private_set_proc++)            
	{
		if (strcmp(this_char, private_set_proc->name) == 0)
		{
			if(private_set_proc->set_proc)
			{
				if(private_set_proc->set_proc(dev, value))
				{	
					Status = -EINVAL;
				}
			}
			break;	
		}
	}
	
	if(strcmp(private_set_proc->name, "<NULL>") == 0)
	{  
		Status = -EINVAL;
		printk("===>%s: (iwpriv) Not Support Set Command [%s]", __FUNCTION__, this_char);
		if(value != NULL)
			printk(" value=%s\n", value);
		else
			printk("\n");
	}
	
	return Status;
}


static iw_handler meshdev_private_handler[] = {
	meshdev_wx_enable_mesh,		
	meshdev_wx_get_meshinfo, 		
	meshdev_wx_disable_mesh,		
	meshdev_wx_set_channel, 		
	meshdev_wx_set_meshid,			
	meshdev_wx_get_mesh_list, 		
	meshdev_wx_mesh_scan, 			
	meshdev_wx_set_meshmode,		
	meshdev_wx_set_hostname,		
	meshdev_wx_set_meshbw,			
	meshdev_wx_update_beacon,		
	meshdev_wx_get_mac_deny,		
	meshdev_wx_add_mac_deny,		
	NULL,			  		
	NULL,
	meshdev_wx_get_neighbor_list,		
	meshdev_wx_set_param,			
	NULL,			  		
	meshdev_wx_del_mac_deny,		
	NULL,							
	meshdev_wx_set_mesh_security,		
	meshdev_wx_set_mkdd_id,			
	meshdev_wx_set_mesh_key,			
	meshdev_wx_set_sec_type,		
};

struct iw_handler_def  meshdev_wx_handlers_def={
	.standard = meshdev_wx_handlers,
	.num_standard = sizeof(meshdev_wx_handlers) / sizeof(iw_handler),
	.private = meshdev_private_handler,
	.num_private = sizeof(meshdev_private_handler) / sizeof(iw_handler),
 	.num_private_args = sizeof(meshdev_private_args) / sizeof(struct iw_priv_args),
#if WIRELESS_EXT >= 17	
	.get_wireless_stats = (void*)meshdev_stats,
#endif
	.private_args = (struct iw_priv_args *)meshdev_private_args,	
};
#endif

