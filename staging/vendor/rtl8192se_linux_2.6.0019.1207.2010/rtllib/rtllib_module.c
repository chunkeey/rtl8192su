/*******************************************************************************

  Copyright(c) 2004 Intel Corporation. All rights reserved.

  Portions of this file are based on the WEP enablement code provided by the
  Host AP project hostap-drivers v0.1.3
  Copyright (c) 2001-2002, SSH Communications Security Corp and Jouni Malinen
  <jkmaline@cc.hut.fi>
  Copyright (c) 2002-2003, Jouni Malinen <jkmaline@cc.hut.fi>

  This program is free software; you can redistribute it and/or modify it
  under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 59
  Temple Place - Suite 330, Boston, MA  02111-1307, USA.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.

  Contact Information:
  James P. Ketrenos <ipw2100-admin@linux.intel.com>
  Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497

*******************************************************************************/

#include <linux/compiler.h>
#include <linux/errno.h>
#include <linux/if_arp.h>
#include <linux/in6.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/pci.h>
#include <linux/proc_fs.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/tcp.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/wireless.h>
#include <linux/etherdevice.h>
#include <asm/uaccess.h>
#include <net/arp.h>

#include "rtllib.h"


#ifndef BUILT_IN_RTLLIB
MODULE_DESCRIPTION("802.11 data/management/control stack");
MODULE_AUTHOR("Copyright (C) 2004 Intel Corporation <jketreno@linux.intel.com>");
MODULE_LICENSE("GPL");
#endif

#ifdef RTL8192CE
#define DRV_NAME "rtllib_92ce"
#elif defined RTL8192SE
#define DRV_NAME "rtllib_92se"
#elif defined RTL8192E
#define DRV_NAME "rtllib_92e"
#elif defined RTL8190P
#define DRV_NAME "rtllib_90p"
#elif defined RTL8192SU
#define DRV_NAME "rtllib_92su"
#elif defined RTL8192U
#define DRV_NAME "rtllib_92u"
#else
#define DRV_NAME "rtllib_9x"
#endif

#if defined CONFIG_CFG_80211 && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)  
#ifdef CONFIG_RTL_RFKILL
static inline void rtllib_rfkill_poll(struct wiphy *wiphy)
{
	struct rtllib_device *rtllib = NULL;

	rtllib = (struct rtllib_device *)wiphy_priv(wiphy);

	rtllib = (struct rtllib_device *)netdev_priv_rsl(rtllib->dev);

	if (rtllib->rtllib_rfkill_poll)
		rtllib->rtllib_rfkill_poll(rtllib->dev);
}
#else
static inline void rtllib_rfkill_poll(struct wiphy *wiphy) {}
#endif
struct cfg80211_ops rtllib_config_ops = {.rfkill_poll = rtllib_rfkill_poll };
void *rtllib_wiphy_privid = &rtllib_wiphy_privid;
#endif

void _setup_timer( struct timer_list* ptimer, void* fun, unsigned long data )
{
   ptimer->function = fun;
   ptimer->data = data;
   init_timer( ptimer );
}

#ifdef _RTL8192_EXT_PATCH_
static inline int rtllib_mesh_networks_allocate(struct rtllib_device *ieee)
{
	if (ieee->mesh_networks)
		return 0;

	ieee->mesh_networks = kmalloc(
		MAX_NETWORK_COUNT * sizeof(struct rtllib_network),
		GFP_KERNEL);
	
	if (!ieee->mesh_networks) {
		printk(KERN_WARNING "%s: Out of memory allocating beacons\n",
		       ieee->dev->name);
		return -ENOMEM;
	}

	memset(ieee->mesh_networks, 0,
	       MAX_NETWORK_COUNT * sizeof(struct rtllib_network));

	return 0;
}

static inline void rtllib_mesh_networks_free(struct rtllib_device *ieee)
{
	if (!ieee->mesh_networks)
		return;
	kfree(ieee->mesh_networks);
	ieee->mesh_networks = NULL;
}
#endif

static inline int rtllib_networks_allocate(struct rtllib_device *ieee)
{
	if (ieee->networks)
		return 0;

#ifndef RTK_DMP_PLATFORM
	ieee->networks = kmalloc(
		MAX_NETWORK_COUNT * sizeof(struct rtllib_network),
		GFP_KERNEL);
#else
	ieee->networks = dvr_malloc(MAX_NETWORK_COUNT * sizeof(struct rtllib_network));
#endif
	if (!ieee->networks) {
		printk(KERN_WARNING "%s: Out of memory allocating beacons\n",
		       ieee->dev->name);
		return -ENOMEM;
	}

	memset(ieee->networks, 0,
	       MAX_NETWORK_COUNT * sizeof(struct rtllib_network));

	return 0;
}

static inline void rtllib_networks_free(struct rtllib_device *ieee)
{
	if (!ieee->networks)
		return;
#ifndef RTK_DMP_PLATFORM
	kfree(ieee->networks);
#else
	dvr_free(ieee->networks);
#endif
	ieee->networks = NULL;
}

static inline void rtllib_networks_initialize(struct rtllib_device *ieee)
{
	int i;

	INIT_LIST_HEAD(&ieee->network_free_list);
	INIT_LIST_HEAD(&ieee->network_list);
	for (i = 0; i < MAX_NETWORK_COUNT; i++)
		list_add_tail(&ieee->networks[i].list, &ieee->network_free_list);
#ifdef _RTL8192_EXT_PATCH_
	INIT_LIST_HEAD(&ieee->mesh_network_free_list);
	INIT_LIST_HEAD(&ieee->mesh_network_list);
	for (i = 0; i < MAX_NETWORK_COUNT; i++)
		list_add_tail(&ieee->mesh_networks[i].list, &ieee->mesh_network_free_list);
#endif
}

#if defined CONFIG_CFG_80211 && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)  
static bool rtllib_wdev_alloc(struct rtllib_device *ieee, int sizeof_priv)
{
	int priv_size;
	struct rtllib_device *rtllib = NULL;

	priv_size = ALIGN(sizeof(struct rtllib_device),NETDEV_ALIGN) + sizeof_priv;

	ieee->wdev.wiphy = wiphy_new(&rtllib_config_ops, priv_size);
	if (!ieee->wdev.wiphy) {
		RTLLIB_ERROR("Unable to allocate wiphy.\n");
		goto out_err_new;
	}

	rtllib = (struct rtllib_device *)wiphy_priv(ieee->wdev.wiphy);
	rtllib->dev = ieee->dev;
	
	ieee->dev->ieee80211_ptr = &ieee->wdev;
	ieee->wdev.iftype = NL80211_IFTYPE_STATION;

	/* Fill-out wiphy structure bits we know...  Not enough info
	 *            here to call set_wiphy_dev or set MAC address or channel info
	 *                       -- have to do that in ->ndo_init... */
	ieee->wdev.wiphy->privid = rtllib_wiphy_privid;

	ieee->wdev.wiphy->max_scan_ssids = 1;
	ieee->wdev.wiphy->max_scan_ie_len = 0;
	ieee->wdev.wiphy->interface_modes = BIT(NL80211_IFTYPE_STATION)	| BIT(NL80211_IFTYPE_ADHOC);

	return true;

out_err_new:
	wiphy_free(ieee->wdev.wiphy);
	return false;
}
#endif

struct net_device *alloc_rtllib(int sizeof_priv)
{
	struct rtllib_device *ieee = NULL;
	struct net_device *dev;
	int i,err;
#ifdef ASL
#ifdef ASL_WME
	const __le16 cw_min[QOS_QUEUE_NUM] = {4,4,3,2};
	const __le16 cw_max[QOS_QUEUE_NUM] = {6,10,4,3};
	const u8 aifs[QOS_QUEUE_NUM] = {3,7,2,2};
	const u8 flag[QOS_QUEUE_NUM] = {0,0,0,0};
	const __le16 tx_op_limit[QOS_QUEUE_NUM] = {0,0,3000 / 32+1,1500 / 32+1};
#endif
#endif

	RTLLIB_DEBUG_INFO("Initializing...\n");

	dev = alloc_etherdev(sizeof(struct rtllib_device) + sizeof_priv);
	if (!dev) {
		RTLLIB_ERROR("Unable to network device.\n");
		goto failed;
	}
	ieee = (struct rtllib_device *)netdev_priv_rsl(dev);
	memset(ieee, 0, sizeof(struct rtllib_device)+sizeof_priv);
	ieee->dev = dev;

#if defined CONFIG_CFG_80211 && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)  
	if(!rtllib_wdev_alloc(ieee, sizeof_priv))
		goto failed;
#endif
	err = rtllib_networks_allocate(ieee);
	if (err) {
		RTLLIB_ERROR("Unable to allocate beacon storage: %d\n",
				err);
		goto failed;
	}
#ifdef _RTL8192_EXT_PATCH_
	err = rtllib_mesh_networks_allocate(ieee);
	if (err) {
		RTLLIB_ERROR("Unable to allocate mesh_beacon storage: %d\n",
				err);
		goto failed;
	}
#endif
	rtllib_networks_initialize(ieee);
	ieee->b4ac_Uapsd = 0x00;
	ieee->qos_capability = QOS_WMM;
	/* Default fragmentation threshold is maximum payload size */
	ieee->fts = DEFAULT_FTS;
	ieee->scan_age = DEFAULT_MAX_SCAN_AGE;
	ieee->open_wep = 1;

	/* Default to enabling full open WEP with host based encrypt/decrypt */
	ieee->host_encrypt = 1;
	ieee->host_decrypt = 1;
	ieee->ieee802_1x = 1; /* Default to supporting 802.1x */

	INIT_LIST_HEAD(&ieee->crypt_deinit_list);
	_setup_timer(&ieee->crypt_deinit_timer,
		    rtllib_crypt_deinit_handler,
		    (unsigned long) ieee);
	ieee->rtllib_ap_sec_type = rtllib_ap_sec_type;

	spin_lock_init(&ieee->lock);
	spin_lock_init(&ieee->wpax_suitlist_lock);
	spin_lock_init(&ieee->bw_spinlock);
	spin_lock_init(&ieee->reorder_spinlock);
	ieee->bin_service_period = false;
#ifdef ASL
	ieee->bbeacon_start = false;
	spin_lock_init(&ieee->psQ_lock);
	spin_lock_init(&ieee->wmm_psQ_lock);
	ieee->mDtimPeriod = 2;
	ieee->mDtimCount = 0;
	ieee->PowerSaveStationNum = 0;
	skb_queue_head_init(&ieee->GroupPsQueue);
	ieee->idx_ptk = 0;
	ieee->idx_gtk = 1;
	ieee->iswep = 0;
	for (i=0; i<APDEV_MAX_ASSOC; i++) {
		ieee->stations_crypt[i] = (struct rtllib_crypt_data_list*) kmalloc(sizeof(struct rtllib_crypt_data_list), GFP_KERNEL);
		if (NULL == ieee->stations_crypt) {
			printk("error kmalloc cryptlist\n");
			goto failed;
		}
		memset(ieee->stations_crypt[i], 0, sizeof(struct rtllib_crypt_data_list));
	}
	for(i = 0; i<APDEV_MAX_ASSOC; i++)
		ieee->apdev_assoc_list[i]=NULL;
	ieee->ForcedDataRate = 0;
	spin_lock_init(&ieee->apdev_queue_lock);
	for (i = 0; i < IEEE_AP_MAC_HASH_SIZE; i++)
		INIT_LIST_HEAD(&ieee->ap_mac_hash[i]);
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,13))
	ieee->pAPHTInfo = (RT_HIGH_THROUGHPUT*)kzalloc(sizeof(RT_HIGH_THROUGHPUT), GFP_KERNEL);
#else
	ieee->pAPHTInfo = (RT_HIGH_THROUGHPUT*)kmalloc(sizeof(RT_HIGH_THROUGHPUT), GFP_KERNEL);
	memset(ieee->pAPHTInfo,0,sizeof(RT_HIGH_THROUGHPUT));
#endif
#ifdef RATE_ADAPTIVE_FOR_AP
	ieee->enableRateAdaptive = 1;
#endif
	memset(ieee->swapcamtable,0,sizeof(SW_CAM_TABLE)*32);
#ifdef ASL_WME
	ieee->parameter_set_count = 0;
	ieee->bsupport_wmm_uapsd = true;
	ieee->wme_enabled = 0;
	/*Init WME registers*/	
	memcpy(ieee->current_ap_network.qos_data.parameters.cw_min, cw_min, QOS_QUEUE_NUM);
	memcpy(ieee->current_ap_network.qos_data.parameters.cw_max, cw_max, QOS_QUEUE_NUM);
	memcpy(ieee->current_ap_network.qos_data.parameters.aifs, aifs, QOS_QUEUE_NUM);
	memcpy(ieee->current_ap_network.qos_data.parameters.flag, flag, QOS_QUEUE_NUM);
	memcpy(ieee->current_ap_network.qos_data.parameters.tx_op_limit, tx_op_limit, QOS_QUEUE_NUM);
	
/*init by apdev_tx from  hostapd configure.So here do nothing.*/	
#endif

#endif
	atomic_set(&(ieee->atm_chnlop), 0);
	atomic_set(&(ieee->atm_swbw), 0);

	ieee->bHalfNMode = false;
 	ieee->wpa_enabled = 0;
 	ieee->tkip_countermeasures = 0;
 	ieee->drop_unencrypted = 0;
 	ieee->privacy_invoked = 0;
 	ieee->ieee802_1x = 1;
	ieee->raw_tx = 0;
	ieee->hwsec_active = 0; 

#ifdef _RTL8192_EXT_PATCH_
	for (i=0; i<MAX_MP; i++)
	{
		ieee->cryptlist[i] = (struct rtllib_crypt_data_list*) kmalloc(sizeof(struct rtllib_crypt_data_list), GFP_KERNEL);
		if (NULL == ieee->cryptlist[i])
                {
			printk("error kmalloc cryptlist\n");
			goto failed;
		}
		memset(ieee->cryptlist[i], 0, sizeof(struct rtllib_crypt_data_list));
	}
	memset(ieee->swmeshcamtable,0,sizeof(SW_CAM_TABLE)*32);
#endif	
	memset(ieee->swcamtable,0,sizeof(SW_CAM_TABLE)*32);
	rtllib_softmac_init(ieee);
	
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,13))
	ieee->pHTInfo = (RT_HIGH_THROUGHPUT*)kzalloc(sizeof(RT_HIGH_THROUGHPUT), GFP_KERNEL);
#else
	ieee->pHTInfo = (RT_HIGH_THROUGHPUT*)kmalloc(sizeof(RT_HIGH_THROUGHPUT), GFP_KERNEL);
	memset(ieee->pHTInfo,0,sizeof(RT_HIGH_THROUGHPUT));
#endif
	if (ieee->pHTInfo == NULL)
	{
		RTLLIB_DEBUG(RTLLIB_DL_ERR, "can't alloc memory for HTInfo\n");
		return NULL;
	}	
	HTUpdateDefaultSetting(ieee);
	HTInitializeHTInfo(ieee); 
	TSInitialize(ieee);
#if 0
	INIT_WORK_RSL(&ieee->ht_onAssRsp, (void(*)(void*)) HTOnAssocRsp_wq, ieee);
#endif
	for (i = 0; i < IEEE_IBSS_MAC_HASH_SIZE; i++)
		INIT_LIST_HEAD(&ieee->ibss_mac_hash[i]);

#ifdef _RTL8192_EXT_PATCH_
	for (i = 0; i < IEEE_MESH_MAC_HASH_SIZE; i++)
		INIT_LIST_HEAD(&ieee->mesh_mac_hash[i]);
#endif	
	
	for (i = 0; i < 17; i++) {
	  ieee->last_rxseq_num[i] = -1;
	  ieee->last_rxfrag_num[i] = -1;
	  ieee->last_packet_time[i] = 0;
	}

	rtllib_tkip_null();
	rtllib_wep_null();
	rtllib_ccmp_null();

	return dev;

 failed:
#ifdef _RTL8192_EXT_PATCH_
	for (i=0; i<MAX_MP; i++) {
		if (ieee->cryptlist[i]==NULL){
			continue;
		}
		kfree(ieee->cryptlist[i]);
		ieee->cryptlist[i] = NULL;

	}
#endif
#ifdef ASL
	for (i=0; i<APDEV_MAX_ASSOC; i++) {
		if (ieee->stations_crypt[i]==NULL){
			continue;
		}
		kfree(ieee->stations_crypt[i]);
		ieee->stations_crypt[i] = NULL;

	}
#endif

	if (dev)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))
		free_netdev(dev);
#else
		kfree(dev);
#endif
	return NULL;
}


void free_rtllib(struct net_device *dev)
{
	struct rtllib_device *ieee = (struct rtllib_device *)netdev_priv_rsl(dev);
	int i;
	struct rtllib_crypt_data *crypt = NULL;

#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
	int j;
	struct list_head *p, *q;
#endif	
#if 1	
	if (ieee->pHTInfo != NULL)
	{
		kfree(ieee->pHTInfo);
		ieee->pHTInfo = NULL;
	}
#endif	
#ifdef CONFIG_WPS
	if (ieee->beacon_wps_ie) {
		kfree(ieee->beacon_wps_ie);
		ieee->beacon_wps_ie = NULL;
		ieee->beacon_wps_ie_len = 0;
	}
	if (ieee->probe_rsp_wps_ie) {
		kfree(ieee->probe_rsp_wps_ie);
		ieee->probe_rsp_wps_ie = NULL;
		ieee->probe_rsp_wps_ie_len = 0;
	}
#endif

	rtllib_softmac_free(ieee);
	del_timer_sync(&ieee->crypt_deinit_timer);
	rtllib_crypt_deinit_entries(ieee, 1);

#ifdef _RTL8192_EXT_PATCH_
	for (j=0;j<MAX_MP; j++) {
		if (ieee->cryptlist[j] == NULL)
			continue;
		for (i = 0; i < WEP_KEYS; i++) {
			crypt = ieee->cryptlist[j]->crypt[i];
			
			if (crypt) 
			{
				if (crypt->ops) {
					crypt->ops->deinit(crypt->priv);
					printk("===>%s():j is %d,i is %d\n",__FUNCTION__,j,i);
#ifndef BUILT_IN_RTLLIB
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
					module_put(crypt->ops->owner);
#else
					__MOD_DEC_USE_COUNT(crypt->ops->owner);
#endif				
#endif
				}
				kfree(crypt);
				ieee->cryptlist[j]->crypt[i] = NULL;
			}
		}
		kfree(ieee->cryptlist[j]);
	}
#endif
#ifdef ASL
	for (j=0;j < APDEV_MAX_ASSOC; j++) {
		if (ieee->stations_crypt[j] == NULL)
			continue;
		for (i = 0; i < WEP_KEYS; i++) {
			crypt = ieee->stations_crypt[j]->crypt[i];
			if (crypt) {
				if (crypt->ops) {
					crypt->ops->deinit(crypt->priv);
#ifndef BUILT_IN_RTLLIB
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
					module_put(crypt->ops->owner);
#else
					__MOD_DEC_USE_COUNT(crypt->ops->owner);
#endif
#endif
				}
				kfree(crypt);
				ieee->stations_crypt[j]->crypt[i] = NULL;
			}
		}
		kfree(ieee->stations_crypt[j]);
	}
#endif
#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
	for (i = 0; i < WEP_KEYS; i++) {
		crypt = ieee->sta_crypt[i];
		if (crypt) 	{
			if (crypt->ops) {
				crypt->ops->deinit(crypt->priv);
#ifndef BUILT_IN_RTLLIB
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
				module_put(crypt->ops->owner);
#else
				__MOD_DEC_USE_COUNT(crypt->ops->owner);
#endif				
#endif
			}
			kfree(crypt);
		}
		ieee->sta_crypt[i] = NULL;
	}
#else
	for (i = 0; i < WEP_KEYS; i++) {
		crypt = ieee->crypt[i];
		if (crypt) {
			if (crypt->ops) {
				crypt->ops->deinit(crypt->priv);
#ifndef BUILT_IN_RTLLIB
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
				module_put(crypt->ops->owner);
#else
				__MOD_DEC_USE_COUNT(crypt->ops->owner);
#endif				
#endif
			}
			kfree(crypt);
			ieee->crypt[i] = NULL;
		}
	}
#endif
	rtllib_networks_free(ieee);
#ifdef _RTL8192_EXT_PATCH_
	rtllib_mesh_networks_free(ieee);
#endif
#if 0	
	for (i = 0; i < IEEE_IBSS_MAC_HASH_SIZE; i++) {
		list_for_each_safe(p, q, &ieee->ibss_mac_hash[i]) {
			kfree(list_entry(p, struct ieee_ibss_seq, list));
			list_del(p);
		}
	}

#endif
#ifdef _RTL8192_EXT_PATCH_
	for (i = 0; i < IEEE_MESH_MAC_HASH_SIZE; i++) {
		list_for_each_safe(p, q, &ieee->mesh_mac_hash[i]) {
			kfree(list_entry(p, struct ieee_mesh_seq, list));
			list_del(p);
		}
	}
#endif
#ifdef ASL
	for (i = 0; i < IEEE_AP_MAC_HASH_SIZE; i++) {
		list_for_each_safe(p, q, &ieee->ap_mac_hash[i]) {
			kfree(list_entry(p, struct ieee_ap_seq, list));
			list_del(p);
		}
	}
#endif
#if defined (RTL8192S_WAPI_SUPPORT)
	if (ieee->WapiSupport)
	{
		WapiFreeAllStaInfo(ieee);
	}
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))	
#if defined CONFIG_CFG_80211 && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)  
	wiphy_unregister(ieee->wdev.wiphy);
	wiphy_free(ieee->wdev.wiphy);
#endif
	free_netdev(dev);
#else
	kfree(dev);
#endif
}

#ifdef CONFIG_RTLLIB_DEBUG

u32 rtllib_debug_level = 0;
static int debug = \
			    RTLLIB_DL_ERR	  
			    ;
struct proc_dir_entry *rtllib_proc = NULL;

static int show_debug_level(char *page, char **start, off_t offset,
			    int count, int *eof, void *data)
{
	return snprintf(page, count, "0x%08X\n", rtllib_debug_level);
}

static int store_debug_level(struct file *file, const char *buffer,
			     unsigned long count, void *data)
{
	char buf[] = "0x00000000";
	unsigned long len = min((unsigned long)sizeof(buf) - 1, count);
	char *p = (char *)buf;
	unsigned long val;

	if (copy_from_user(buf, buffer, len))
		return count;
	buf[len] = 0;
	if (p[1] == 'x' || p[1] == 'X' || p[0] == 'x' || p[0] == 'X') {
		p++;
		if (p[0] == 'x' || p[0] == 'X')
			p++;
		val = simple_strtoul(p, &p, 16);
	} else
		val = simple_strtoul(p, &p, 10);
	if (p == buf)
		printk(KERN_INFO DRV_NAME
		       ": %s is not in hex or decimal form.\n", buf);
	else
		rtllib_debug_level = val;

	return strnlen(buf, count);
}

int __init rtllib_init(void)
{
#ifdef CONFIG_RTLLIB_DEBUG
	struct proc_dir_entry *e;

	rtllib_debug_level = debug;
#if(LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
	rtllib_proc = create_proc_entry(DRV_NAME, S_IFDIR, proc_net);
#else
	rtllib_proc = create_proc_entry(DRV_NAME, S_IFDIR, init_net.proc_net);
#endif
	if (rtllib_proc == NULL) {
		RTLLIB_ERROR("Unable to create " DRV_NAME
				" proc directory\n");
		return -EIO;
	}
	e = create_proc_entry("debug_level", S_IFREG | S_IRUGO | S_IWUSR,
			      rtllib_proc);
	if (!e) {
#if(LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
		remove_proc_entry(DRV_NAME, proc_net);
#else
		remove_proc_entry(DRV_NAME, init_net.proc_net);
#endif
		rtllib_proc = NULL;
		return -EIO;
	}
	e->read_proc = show_debug_level;
	e->write_proc = store_debug_level;
	e->data = NULL;
#endif

	return 0;
}

void __exit rtllib_exit(void)
{
#ifdef CONFIG_RTLLIB_DEBUG
	if (rtllib_proc) {
		remove_proc_entry("debug_level", rtllib_proc);
#if(LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
		remove_proc_entry(DRV_NAME, proc_net);
#else
		remove_proc_entry(DRV_NAME, init_net.proc_net);
#endif
		rtllib_proc = NULL;
	}
#endif
}

#ifndef BUILT_IN_RTLLIB
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
#include <linux/moduleparam.h>
module_param(debug, int, 0444);
MODULE_PARM_DESC(debug, "debug output mask");


module_exit(rtllib_exit);
module_init(rtllib_init);
#endif

EXPORT_SYMBOL_RSL(alloc_rtllib);
EXPORT_SYMBOL_RSL(free_rtllib);
EXPORT_SYMBOL_RSL(rtllib_debug_level);
#endif
#endif
