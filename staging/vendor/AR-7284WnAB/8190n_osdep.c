/*
 *      Routines to handle OS dependent jobs and interfaces
 *
 *      $Id: 8190n_osdep.c,v 1.19 2009/09/28 13:25:31 cathy Exp $
 */

/*-----------------------------------------------------------------------------
	This file is for OS related functions.
------------------------------------------------------------------------------*/
#define _8190N_OSDEP_C_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/in.h>
#include <linux/if.h>
#include <linux/ip.h>
#include <asm/io.h>
#include <linux/skbuff.h>
#include <linux/socket.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/fcntl.h>
#include <linux/signal.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include "flash_layout.h"
#ifdef __DRAYTEK_OS__
#include <draytek/softimer.h>
#include <draytek/skbuff.h>
#include <draytek/wl_dev.h>
#endif

#include "./8190n_cfg.h"


#include <linux/syscalls.h>
#include <linux/file.h>
#include <asm/unistd.h>
#if defined(RTK_BR_EXT) || defined(BR_SHORTCUT)
#include <linux/syscalls.h>
#include <../net/bridge/br_private.h>
#endif

#include "./8190n.h"
#include "./8190n_hw.h"
#include "./8190n_headers.h"
#include "./8190n_rx.h"
#include "./8190n_debug.h"

#include "./8190n_usb.h"

#ifdef RTL8190_VARIABLE_USED_DMEM
#include "./8190n_dmem.h"
#endif



#ifdef	CONFIG_RTK_MESH
#include "./mesh_ext/mesh_route.h"	// Note : Not include in  #ifdef CONFIG_RTK_MESH, Because use in wlan_device[]
#include "./mesh_ext/mesh_util.h"
#endif	// CONFIG_RTK_MESH

#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
#include <asm/mips16_lib.h>
#endif

#define TBS_MP
#ifdef TBS_MP
#include <flash_layout.h>
#define TXPOWERCCK 		"HW_TX_POWER_CCK"
#define TXPOWEROFDM1S 	"HW_TX_POWER_OFDM_1S"
#define TXPOWEROFDM2S	"HW_TX_POWER_OFDM_2S"
#define LOFDMPWDA		"HW_11N_LOFDMPWDA"
#endif
#ifndef CONFIG_RTK_MESH	//move below from mesh_route.h ; plus 0119
#define MESH_SHIFT			0
#define MESH_NUM_CFG		0
#else
#define MESH_SHIFT			8 // ACCESS_MASK uses 2 bits, WDS_MASK use 4 bits
#define MESH_MASK			0x3
#define MESH_NUM_CFG		NUM_MESH
#endif



#ifdef CONFIG_WIRELESS_LAN_MODULE
extern int (*wirelessnet_hook)(void);
#ifdef BR_SHORTCUT
extern struct net_device* (*wirelessnet_hook_shortcut)(unsigned char *da);
#endif
#ifdef PERF_DUMP
extern int rtl8651_romeperfEnterPoint(unsigned int index);
extern int rtl8651_romeperfExitPoint(unsigned int index);
extern int (*Fn_rtl8651_romeperfEnterPoint)(unsigned int index);
extern int (*Fn_rtl8651_romeperfExitPoint)(unsigned int index);
#endif
#ifdef CONFIG_RTL8190_PRIV_SKB
extern int (*wirelessnet_hook_is_priv_buf)(void);
extern void (*wirelessnet_hook_free_priv_buf)(unsigned char *head);
#endif
#endif // CONFIG_WIRELESS_LAN_MODULE
extern int set_mib(struct rtl8190_priv *priv, unsigned char *data);

struct _device_info_ {
	int type;
	unsigned long conf_addr;
	unsigned long base_addr;
	int irq;
	struct rtl8190_priv *priv;
};

static struct _device_info_ wlan_device[] =
{
#if defined(CONFIG_RTL_EB8186) || defined(USE_RTL8186_SDK)
	//{(MESH_NUM_CFG<<MESH_SHIFT) |(WDS_NUM_CFG<<WDS_SHIFT) | (TYPE_EMBEDDED<<TYPE_SHIFT), 0, 0xbd400000, 2, NULL},
	#ifdef IO_MAPPING
	{(WDS_NUM_CFG<<WDS_SHIFT) | (TYPE_PCI_DIRECT<<TYPE_SHIFT) | ACCESS_SWAP_MEM, PCI_SLOT1_CONFIG_ADDR, 0xb8C00000, 3, NULL},
	#else
#ifdef CONFIG_RTL8196B
	{(MESH_NUM_CFG<<MESH_SHIFT) |(WDS_NUM_CFG<<WDS_SHIFT) | (TYPE_PCI_BIOS<<TYPE_SHIFT) | ACCESS_SWAP_MEM, 0, 1, 0, NULL},
#else
	{(MESH_NUM_CFG<<MESH_SHIFT) | (WDS_NUM_CFG<<WDS_SHIFT) | (TYPE_PCI_DIRECT<<TYPE_SHIFT) | ACCESS_SWAP_MEM, PCI_SLOT1_CONFIG_ADDR, 0xb9000000, 3, NULL},
#endif
	#endif
#else
	{(MESH_NUM_CFG<<MESH_SHIFT) |(WDS_NUM_CFG<<WDS_SHIFT) | (TYPE_PCI_BIOS<<TYPE_SHIFT), 0, 0, 0, NULL}
#endif
};

static int wlan_index=0;

#ifdef CONFIG_RTL865X
/* registration functions for RTL865X platform */
static int rtl865x_wlanRegistration(struct net_device *dev, int index);
static int rtl865x_wlanUnregistration(struct net_device *dev, int index);
#endif	/* CONFIG_RTL865X */

// Added by Mason Yu
// MBSSID Port Mapping
struct port_map wlanDev[5];
static int wlanDevNum=0;

#ifdef PRIV_STA_BUF
static struct rtl8190_hw hw_info;
static struct priv_shared_info shared_info;
static struct wlan_hdr_poll hdr_pool;
static struct wlanllc_hdr_poll llc_pool;
static struct wlanbuf_poll buf_pool;
static struct wlanicv_poll icv_pool;
static struct wlanmic_poll mic_pool;
static unsigned char desc_buf[DESC_DMA_PAGE_SIZE];
#endif

#ifdef RTL867X_DMEM_ENABLE
__DRAM_IN_865X static struct rtl8190_hw hw_info;
__DRAM_IN_865X static struct priv_shared_info shared_info;
//__DRAM_IN_865X static struct wlanllc_hdr_poll llc_pool;
__DRAM_IN_865X static struct wifi_mib mib;
#endif


// init and remove char device
#ifdef CONFIG_WIRELESS_LAN_MODULE
extern int rtl8190_chr_init(void);
extern void rtl8190_chr_exit(void);
#else
int rtl8190_chr_init(void);
void rtl8190_chr_exit(void);
#endif
struct rtl8190_priv *rtl8190_chr_reg(unsigned int minor, struct rtl8190_chr_priv *priv);
void rtl8190_chr_unreg(unsigned int minor);
int rtl8190n_fileopen(const char *filename, int flags, int mode);
struct net_device *get_shortcut_dev(unsigned char *da);

void force_stop_wlan_hw(void);

#ifdef LOOPBACK_MODE //tysu: loopback main code
volatile int flags=0;
volatile int rx_buff0_free=1;
volatile int rx_buff1_free=1;
#define __IRAM	__attribute__ ((__section__(".iram")))


#define MAX_LB_TX_URB 1
volatile struct urb *tx_urb[MAX_LB_TX_URB];

#ifdef LOOPBACK_NORMAL_RX_MODE
#else
#define MAX_LB_RX_URB 1
volatile struct urb *rx_urb[MAX_LB_RX_URB];
#endif

#ifdef PKT_PROCESSOR
unsigned char *txbuff[MAX_LB_TX_URB];
struct sk_buff *skb_pool[MAX_LB_RX_URB];
#else
unsigned char *txbuff[MAX_LB_TX_URB];

#ifdef LOOPBACK_NORMAL_RX_MODE
#else
unsigned char *rxbuff[MAX_LB_RX_URB];
#endif

#endif

volatile int tx_idx=0;
volatile int tx_own[MAX_LB_TX_URB]={0};
volatile int tx_info[MAX_LB_TX_URB]={0};

#ifdef LOOPBACK_NORMAL_RX_MODE
#else
struct usb_priv privs[MAX_LB_RX_URB];
#endif



__IRAM void us_delay(int nus)
{

	unsigned int i,j=0,k;
	k=1333; // 0.005ms  ==>666, 10us==>1333
	
		for(i=0;i<k*nus;i++)
			j++;			

}

__IRAM static void write_complete(struct urb *purb, struct pt_regs *regs)
{
	int *pidx=(int *)purb->context;
//	printk("write_complete=%x\n",(u8)txbuff[*pidx][73]);
//	printk("callback idx=%d\n",*pidx);	
	tx_own[*pidx]=0;
}

#ifdef LOOPBACK_NORMAL_RX_MODE
#else
__IRAM static void read_complete(struct urb *purb, struct pt_regs *regs)
{

		struct usb_priv *ppriv=(struct usb_priv *)purb->context;
		char *pdesc=NULL;

		int idx=ppriv->index;	

//		rtl8192su_rx_isr(purb);
		pdesc = purb->transfer_buffer;
		//memDump(purb->transfer_buffer, purb->actual_length, "RX");
		//if (((u8*)pdesc)[purb->actual_length-4-2] != 0xee)
			memDump(pdesc, purb->actual_length, "RX");
		//printk("--->%x\n", ((u8*)pdesc)[purb->actual_length-1]);

//printk("read_complete=%x\n",rxbuff[idx][97]);


#ifdef PKT_PROCESSOR

		struct sk_buff *ret;
//		int idx=privs->index;
		
//		us_delay(3);  //sim normal 8187 rx time


//		rtl8672_l2learning(&skb->data[6], 15,cp->dev->vlanid);
//		printk("data-head=%d\n",(u32)skb_pool[idx]->data-(u32)skb_pool[idx]->head);

		skb_pool[idx]->data+=58;

		skb_pool[idx]->data[0]=0x00;
		skb_pool[idx]->data[1]=0x11;
		skb_pool[idx]->data[2]=0x22;
		skb_pool[idx]->data[3]=0x33;
		skb_pool[idx]->data[4]=0x44;
		skb_pool[idx]->data[5]=0x66;	

		
		skb_pool[idx]->data[6]=0x00;	
		skb_pool[idx]->data[7]=0x11;	
		skb_pool[idx]->data[8]=0x22;	
		skb_pool[idx]->data[9]=0x33;	
		skb_pool[idx]->data[10]=0x44;	
		skb_pool[idx]->data[11]=0x55;	
		skb_pool[idx]->data[12]=0x08;	
		skb_pool[idx]->data[13]=0x00;	
		skb_pool[idx]->len=purb->actual_length-58;


		dma_cache_wback_inv((unsigned long)&skb_pool[idx]->data[0],skb_pool[idx]->len);

		
		ret = rtl8672_extPortRecv(	      skb_pool[idx],
		                                                        skb_pool[idx]->data,
										skb_pool[idx]->len,
		                                                        9,
		                                                        15,
		                                                        9);
		if (ret != NULL)
		{
			skb_pool[idx]=ret;
			ret->head=(unsigned char *)((u32)ret->head & (~0x20000000));
			ret->data=ret->head+RX_DMA_SHIFT;
			ret->tail=ret->data;
			ret->end=skb_pool[idx]->head+RX_DMA_SHIFT+RX_ALLOC_SIZE;
			ret->fcpu=0;				
			privs->transfer_buffer=ret->data;
			privs->buffer_length=RX_ALLOC_SIZE;
		
		}else
		{
		        /* exception. Drop it. */
			printk("can't put into ext prx\n");
		}



#endif
//		printk("re submit again %d buff=%d\n",privs->index,privs->buffer_length);
//		memDump(purb->transfer_buffer,purb->actual_length,"rx data");

		usb_fill_bulk_urb(purb,
						privs->dev,
						privs->pipe, 
						privs->transfer_buffer,
						privs->buffer_length,
						(usb_complete_t)read_complete,
						privs); 
		usb_submit_urb(purb, GFP_ATOMIC);		
}
#endif

#endif

#if defined(_INCLUDE_PROC_FS_) && defined(PERF_DUMP)
static int read_perf_dump(struct file *file, const char *buffer,
              unsigned long count, void *data)
{
	unsigned long x;
	save_and_cli(x);

	rtl8651_romeperfDump(1, 1);

	restore_flags(x);
    return count;
}


static int flush_perf_dump(struct file *file, const char *buffer,
              unsigned long count, void *data)
{
	unsigned long x;
	save_and_cli(x);

	rtl8651_romeperfReset();

	restore_flags(x);
    return count;
}
#endif // _INCLUDE_PROC_FS_ && PERF_DUMP

#define GPIO_BASE			0xB8003500
#define PEFGHCNR_REG		(0x01C + GPIO_BASE)     /* Port EFGH control */
#define PEFGHPTYPE_REG		(0x020 + GPIO_BASE)     /* Port EFGH type */
#define PEFGHDIR_REG		(0x024 + GPIO_BASE)     /* Port EFGH direction */
#define PEFGHDAT_REG		(0x028 + GPIO_BASE)     /* Port EFGH data */
#ifndef REG32
	#define REG32(reg) 		(*(volatile unsigned int *)(reg))
#endif






static void rtl8190_set_rx_mode(struct net_device *dev)
{

}


static struct net_device_stats *rtl8190_get_stats(struct net_device *dev)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;

#ifdef WDS
	int idx;
	struct stat_info *pstat;

	if (dev->base_addr == 0) {
		idx = getWdsIdxByDev(priv, dev);
		if (idx < 0) {
			memset(&priv->wds_stats[NUM_WDS-1], 0, sizeof(struct net_device_stats));
			return &priv->wds_stats[NUM_WDS-1];
		}

		if (netif_running(dev) && netif_running(priv->dev)) {
			pstat = get_stainfo(priv, priv->pmib->dot11WdsInfo.entry[idx].macAddr);
			if (pstat == NULL) {
				DEBUG_ERR("%s: get_stainfo() wds fail!\n", (char *)__FUNCTION__);
				memset(&priv->wds_stats[idx], 0, sizeof(struct net_device_stats));
			}
			else {
				priv->wds_stats[idx].tx_packets = pstat->tx_pkts;
				priv->wds_stats[idx].tx_errors = pstat->tx_fail;
				priv->wds_stats[idx].tx_bytes = pstat->tx_bytes;
				priv->wds_stats[idx].rx_packets = pstat->rx_pkts;
				priv->wds_stats[idx].rx_bytes = pstat->rx_bytes;
			}
		}

		return &priv->wds_stats[idx];
	}
#endif

#ifdef CONFIG_RTK_MESH

	if (dev->base_addr == 1) {
		if(priv->mesh_dev != dev)
			return NULL;

		return &priv->mesh_stats;
	}
#endif // CONFIG_RTK_MESH
	return &(priv->net_stats);
}


static int rtl8190_init_sw(struct rtl8190_priv *priv)
{
	// All the index/counters should be reset to zero...
	struct rtl8190_hw *phw=NULL;
	static unsigned long offset;
	static unsigned int  i;
	static unsigned char	*page_ptr;
	static struct wlan_hdr_poll	*pwlan_hdr_poll;
	static struct wlanllc_hdr_poll	*pwlanllc_hdr_poll;
	static struct wlanbuf_poll		*pwlanbuf_poll;
	static struct wlanicv_poll		*pwlanicv_poll;
	static struct wlanmic_poll		*pwlanmic_poll;
	static struct wlan_acl_poll	*pwlan_acl_poll;
#ifdef _MESH_ACL_ENABLE_
	static struct mesh_acl_poll	*pmesh_acl_poll;
#endif
	static unsigned long ring_virt_addr;
	static unsigned long ring_dma_addr;
	static unsigned int  ring_buf_len;
	static unsigned char MIMO_TR_hw_support;
	static unsigned int NumTotalRFPath;
#if defined(CLIENT_MODE) && defined(CHECK_HANGUP)
	unsigned char *pbackup=NULL;
	unsigned long backup_len=0;
#endif
#ifdef _11s_TEST_MODE_
	struct Galileo_poll 	*pgalileo_poll;
#endif


#ifdef DFS
	// For JAPAN : prevent switching to channels 52, 56, 60, and 64 in adhoc mode
	if (((priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_MKK) ||
		 (priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_MKK3)) &&
		(OPMODE & WIFI_ADHOC_STATE))
	{
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11A) {
			// block channels 52~64 and place them in NOP_chnl[4]
			if (!timer_pending(&priv->ch52_timer))
				InsertChannel(priv->NOP_chnl, &priv->NOP_chnl_num, 52);
			if (!timer_pending(&priv->ch56_timer))
				InsertChannel(priv->NOP_chnl, &priv->NOP_chnl_num, 56);
			if (!timer_pending(&priv->ch60_timer))
				InsertChannel(priv->NOP_chnl, &priv->NOP_chnl_num, 60);
			if (!timer_pending(&priv->ch64_timer))
				InsertChannel(priv->NOP_chnl, &priv->NOP_chnl_num, 64);
		}

		// if users select an illegal channel, the driver will switch to channel 36~48
		if (priv->pmib->dot11RFEntry.dot11channel >= 52) {
			PRINT_INFO("Channel %d is illegal in ad-hoc mode in Japan!\n", priv->pmib->dot11RFEntry.dot11channel);
			priv->pmib->dot11RFEntry.dot11channel = DFS_SelectChannel();
			PRINT_INFO("Swiching to channel %d!\n", priv->pmib->dot11RFEntry.dot11channel);
		}
	}

	// if users select a blocked channel, the driver will switch to channel 36~48
	if (!priv->pmib->dot11DFSEntry.disable_DFS &&
		((timer_pending(&priv->ch52_timer) && (priv->pmib->dot11RFEntry.dot11channel == 52)) ||
		 (timer_pending(&priv->ch56_timer) && (priv->pmib->dot11RFEntry.dot11channel == 56)) ||
		 (timer_pending(&priv->ch60_timer) && (priv->pmib->dot11RFEntry.dot11channel == 60)) ||
		 (timer_pending(&priv->ch64_timer) && (priv->pmib->dot11RFEntry.dot11channel == 64)))) {
		PRINT_INFO("Channel %d is still in none occupancy period!\n", priv->pmib->dot11RFEntry.dot11channel);
		priv->pmib->dot11RFEntry.dot11channel = DFS_SelectChannel();
		PRINT_INFO("Swiching to channel %d!\n", priv->pmib->dot11RFEntry.dot11channel);
	}

	// disable all of the transmissions during channel availability check
	priv->pmib->dot11DFSEntry.disable_tx = 0;
	if (!priv->pmib->dot11DFSEntry.disable_DFS &&
		(priv->pmib->dot11RFEntry.dot11channel >= 52) &&
		(OPMODE & WIFI_AP_STATE))
		priv->pmib->dot11DFSEntry.disable_tx = 1;
#endif

#ifdef ENABLE_RTL_SKB_STATS
 	rtl_atomic_set(&priv->rtl_tx_skb_cnt, 0);
 	rtl_atomic_set(&priv->rtl_rx_skb_cnt, 0);
#endif

#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
#ifdef DFS
		// will not initialize the tasklet if the driver is rebooting due to the detection of radar
		if (!priv->pmib->dot11DFSEntry.DFS_detected)
#endif
		{
#ifdef CHECK_HANGUP
			if (!priv->reset_hangup)
#endif
			{
				tasklet_init(&priv->pshare->rx_tasklet,(void(*)(unsigned long))rtl8192su_irq_rx_tasklet_new,(unsigned long)priv);
				tasklet_init(&priv->pshare->oneSec_tasklet, rtl8190_expire_timer, (unsigned long)priv);
			}
		}

#ifdef DFS
		if (priv->pmib->dot11DFSEntry.DFS_detected)
			priv->pmib->dot11DFSEntry.DFS_detected = 0;
#endif

		phw = GET_HW(priv);

#ifdef HW_QUICK_INIT
		if (priv->pshare->hw_inited == 0)
#endif
		{
			// save descriptor virtual address before reset, david
			ring_virt_addr = phw->ring_virt_addr;
			ring_dma_addr = phw->ring_dma_addr;
			ring_buf_len = phw->ring_buf_len;

			// save RF related settings before reset
			MIMO_TR_hw_support = phw->MIMO_TR_hw_support;
			NumTotalRFPath = phw->NumTotalRFPath;

			memset((void *)phw, 0, sizeof(struct rtl8190_hw));
			phw->ring_virt_addr = ring_virt_addr;
			phw->ring_buf_len = ring_buf_len;
			phw->MIMO_TR_hw_support = MIMO_TR_hw_support;
			phw->NumTotalRFPath = NumTotalRFPath;
		}
	}

#if defined(CLIENT_MODE) && defined(CHECK_HANGUP)
	if (priv->reset_hangup &&
			(OPMODE & (WIFI_STATION_STATE | WIFI_ADHOC_STATE))) {
		backup_len = ((unsigned long)&((struct rtl8190_priv *)0)->EE_Cached) -
				 ((unsigned long)&((struct rtl8190_priv *)0)->join_res);
		pbackup = kmalloc(backup_len, GFP_ATOMIC);
		if (pbackup)
			memcpy(pbackup, &priv->join_res, backup_len);
	}
#endif

	offset = (unsigned long)(&((struct rtl8190_priv *)0)->net_stats);
	// zero all data members below (including) stats
	memset((void *)((unsigned long)priv + offset), 0, sizeof(struct rtl8190_priv)-offset);

#ifdef CHECK_HANGUP
	if (!priv->reset_hangup)
#endif
		priv->up_time = 0;

#if defined(CLIENT_MODE) && defined(CHECK_HANGUP)
	if (priv->reset_hangup && pbackup) {
		memcpy(&priv->join_res, pbackup, backup_len);
		kfree(pbackup);
	}
#endif

#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
		// zero all data members below (including) LED_Timer of share_info
		offset = (unsigned long)(&((struct priv_shared_info*)0)->LED_Timer);
		memset((void *)((unsigned long)priv->pshare+ offset), 0, sizeof(struct priv_shared_info)-offset);

#ifdef CONFIG_RTK_MESH
		memset((void *)&priv->pshare->meshare, 0, sizeof(struct MESH_Share));
		get_random_bytes((void *)&priv->pshare->meshare.seq, sizeof(priv->pshare->meshare.seq));
#if (MESH_DBG_LV & MESH_DBG_COMPLEX)
		init_timer(&priv->pshare->meshare.mesh_test_sme_timer);
		priv->pshare->meshare.mesh_test_sme_timer.data = (unsigned long) priv;
		priv->pshare->meshare.mesh_test_sme_timer.function = mesh_test_sme_timer;
		mod_timer(&priv->pshare->meshare.mesh_test_sme_timer, jiffies + 200);
#endif // (MESH_DBG_LV & MESH_DBG_COMPLEX)

#if (MESH_DBG_LV & MESH_DBG_TEST)
		init_timer(&priv->pshare->meshare.mesh_test_sme_timer2);
		priv->pshare->meshare.mesh_test_sme_timer2.data = (unsigned long) priv;
		priv->pshare->meshare.mesh_test_sme_timer2.function = mesh_test_sme_timer2;
		mod_timer(&priv->pshare->meshare.mesh_test_sme_timer2, jiffies + 5000);
#endif // (MESH_DBG_LV & MESH_DBG_TEST)

#endif // CONFIG_RTK_MESH

		pwlan_hdr_poll = priv->pshare->pwlan_hdr_poll;
		pwlanllc_hdr_poll = priv->pshare->pwlanllc_hdr_poll;
		pwlanbuf_poll = priv->pshare->pwlanbuf_poll;
		pwlanicv_poll = priv->pshare->pwlanicv_poll;
		pwlanmic_poll = priv->pshare->pwlanmic_poll;

#ifdef _11s_TEST_MODE_
		pgalileo_poll = priv->pshare->galileo_poll;
		pgalileo_poll->count = AODV_RREQ_TABLE_SIZE;
#endif
		pwlan_hdr_poll->count = PRE_ALLOCATED_HDR;
		pwlanllc_hdr_poll->count = PRE_ALLOCATED_HDR;
		pwlanbuf_poll->count = PRE_ALLOCATED_MMPDU;
		pwlanicv_poll->count = PRE_ALLOCATED_HDR;
		pwlanmic_poll->count = PRE_ALLOCATED_HDR;

		// initialize all the hdr/buf node, and list to the poll_list
		INIT_LIST_HEAD(&priv->pshare->wlan_hdrlist);
		INIT_LIST_HEAD(&priv->pshare->wlanllc_hdrlist);
		INIT_LIST_HEAD(&priv->pshare->wlanbuf_list);
		INIT_LIST_HEAD(&priv->pshare->wlanicv_list);
		INIT_LIST_HEAD(&priv->pshare->wlanmic_list);

#ifdef _11s_TEST_MODE_	//Galileo

		memset(priv->rvTestPacket, 0, 3000);

		INIT_LIST_HEAD(&priv->pshare->galileo_list);
		INIT_LIST_HEAD(&priv->mtb_list);

		for(i=0; i< AODV_RREQ_TABLE_SIZE; i++)
		{
			INIT_LIST_HEAD(&(pgalileo_poll->node[i].list));
			list_add_tail(&(pgalileo_poll->node[i].list), &priv->pshare->galileo_list);
			init_timer(&pgalileo_poll->node[i].data.expire_timer);
			pgalileo_poll->node[i].data.priv = priv;
			pgalileo_poll->node[i].data.expire_timer.function = galileo_timer;
		}
#endif

		for(i=0; i< PRE_ALLOCATED_HDR; i++)
		{
			INIT_LIST_HEAD(&(pwlan_hdr_poll->hdrnode[i].list));
			list_add_tail(&(pwlan_hdr_poll->hdrnode[i].list), &priv->pshare->wlan_hdrlist);

			INIT_LIST_HEAD(&(pwlanllc_hdr_poll->hdrnode[i].list));
			list_add_tail( &(pwlanllc_hdr_poll->hdrnode[i].list), &priv->pshare->wlanllc_hdrlist);

			INIT_LIST_HEAD(&(pwlanicv_poll->hdrnode[i].list));
			list_add_tail( &(pwlanicv_poll->hdrnode[i].list), &priv->pshare->wlanicv_list);

			INIT_LIST_HEAD(&(pwlanmic_poll->hdrnode[i].list));
			list_add_tail( &(pwlanmic_poll->hdrnode[i].list), &priv->pshare->wlanmic_list);
		}

		for(i=0; i< PRE_ALLOCATED_MMPDU; i++)
		{
			INIT_LIST_HEAD(&(pwlanbuf_poll->hdrnode[i].list));
			list_add_tail( &(pwlanbuf_poll->hdrnode[i].list), &priv->pshare->wlanbuf_list);
		}

		DEBUG_INFO("hdrlist=%x, llc_hdrlist=%x, buf_list=%x, icv_list=%x, mic_list=%X\n",
			(UINT)&priv->pshare->wlan_hdrlist, (UINT)&priv->pshare->wlanllc_hdrlist, (UINT)&priv->pshare->wlanbuf_list,
			(UINT)&priv->pshare->wlanicv_list, (UINT)&priv->pshare->wlanmic_list);

#ifdef HW_QUICK_INIT
		if (priv->pshare->hw_inited)
			goto skip_init_desc;
#endif

		page_ptr = (unsigned char *)phw->ring_virt_addr;
		memset(page_ptr, 0, phw->ring_buf_len); // this is vital!

			phw->ring_dma_addr = virt_to_bus(page_ptr);

		phw->rx_ring_addr  = phw->ring_dma_addr;
		phw->tx_ringB_addr = phw->ring_dma_addr + NUM_CMD_DESC * sizeof(struct tx_desc);
		phw->tx_descB = (struct tx_desc *)page_ptr;

			int txDescRingIdx;
			struct tx_desc *tx_desc_array[] = {
				phw->tx_descB,
				0
				};

			/* TX RING */
			txDescRingIdx = 0;

			while (tx_desc_array[txDescRingIdx] != 0) {
				txDescRingIdx ++;
			}
#ifdef HW_QUICK_INIT
skip_init_desc:
#endif

		priv->pshare->amsdu_timer_head = priv->pshare->amsdu_timer_tail = 0;
	}

	INIT_LIST_HEAD(&priv->wlan_acl_list);
	INIT_LIST_HEAD(&priv->wlan_aclpolllist);

	pwlan_acl_poll = priv->pwlan_acl_poll;
	for(i=0; i< NUM_ACL; i++)
	{
		INIT_LIST_HEAD(&(pwlan_acl_poll->aclnode[i].list));
		list_add_tail(&(pwlan_acl_poll->aclnode[i].list), &priv->wlan_aclpolllist);
	}

	// copy acl from mib to link list
	for (i=0; i<priv->pmib->dot11StationConfigEntry.dot11AclNum; i++)
	{
		struct list_head *pnewlist;
		struct wlan_acl_node *paclnode;

		pnewlist = priv->wlan_aclpolllist.next;
		list_del_init(pnewlist);

		paclnode = list_entry(pnewlist,	struct wlan_acl_node, list);
		memcpy((void *)paclnode->addr, priv->pmib->dot11StationConfigEntry.dot11AclAddr[i], 6);
		paclnode->mode = (unsigned char)priv->pmib->dot11StationConfigEntry.dot11AclMode;

		list_add_tail(pnewlist, &priv->wlan_acl_list);
	}

	for(i=0; i<NUM_STAT; i++)
		INIT_LIST_HEAD(&(priv->stat_hash[i]));

#ifdef	CONFIG_RTK_MESH
	/*
	 * CAUTION !! These statement meshX(virtual interface) ONLY, Maybe modify....
	*/
#ifdef	_MESH_ACL_ENABLE_	// copy acl from mib to link list (below code copy above ACL code)
	INIT_LIST_HEAD(&priv->mesh_acl_list);
	INIT_LIST_HEAD(&priv->mesh_aclpolllist);

	pmesh_acl_poll = priv->pmesh_acl_poll;
	for(i=0; i< NUM_MESH_ACL; i++)
	{
		INIT_LIST_HEAD(&(pmesh_acl_poll->meshaclnode[i].list));
		list_add_tail(&(pmesh_acl_poll->meshaclnode[i].list), &priv->mesh_aclpolllist);
	}

	for (i=0; i<priv->pmib->dot1180211sInfo.mesh_acl_num; i++)
	{
		struct list_head *pnewlist;
		struct wlan_acl_node *paclnode;

		pnewlist = priv->mesh_aclpolllist.next;
		list_del_init(pnewlist);

		paclnode = list_entry(pnewlist, struct wlan_acl_node, list);
		memcpy((void *)paclnode->addr, priv->pmib->dot1180211sInfo.mesh_acl_addr[i], MACADDRLEN);
		paclnode->mode = (unsigned char)priv->pmib->dot1180211sInfo.mesh_acl_mode;

		list_add_tail(pnewlist, &priv->mesh_acl_list);
	}
#endif

#ifdef MESH_BOOTSEQ_AUTH
	INIT_LIST_HEAD(&(priv->mesh_auth_hdr));
#endif

	INIT_LIST_HEAD(&(priv->mesh_unEstablish_hdr));
	INIT_LIST_HEAD(&(priv->mesh_mp_hdr));

	priv->mesh_profile[0].used = FALSE; // Configure by WEB in the future, Maybe delete, Preservation before delete
#endif

	INIT_LIST_HEAD(&(priv->asoc_list));
	INIT_LIST_HEAD(&(priv->auth_list));
	INIT_LIST_HEAD(&(priv->sleep_list));
	INIT_LIST_HEAD(&(priv->defrag_list));
	INIT_LIST_HEAD(&(priv->wakeup_list));
	INIT_LIST_HEAD(&(priv->rx_datalist));
	INIT_LIST_HEAD(&(priv->rx_mgtlist));
	INIT_LIST_HEAD(&(priv->rx_ctrllist));
	INIT_LIST_HEAD(&(priv->addRAtid_list));	// to avoid add RAtid fail

#ifdef CHECK_HANGUP
	if (priv->reset_hangup) {
		get_available_channel(priv);
		validate_oper_rate(priv);
		get_oper_rate(priv);
		DOT11_InitQueue(priv->pevent_queue);
		return 0;
	}
#endif

	// construct operation and basic rates set
	{
		// validate region domain
		if ((priv->pmib->dot11StationConfigEntry.dot11RegDomain < DOMAIN_FCC) ||
				(priv->pmib->dot11StationConfigEntry.dot11RegDomain >= DOMAIN_MAX)) {
			PRINT_INFO("invalid region domain, use default value [DOMAIN_FCC]!\n");
			priv->pmib->dot11StationConfigEntry.dot11RegDomain = DOMAIN_FCC;
		}

		// validate band
		if (priv->pmib->dot11BssType.net_work_type == 0) {
			PRINT_INFO("operation band is not set, use G+B as default!\n");
			priv->pmib->dot11BssType.net_work_type = WIRELESS_11B | WIRELESS_11G;
		}
		if ((OPMODE & WIFI_AP_STATE) && (priv->pmib->dot11BssType.net_work_type & (WIRELESS_11B | WIRELESS_11G))) {
			if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11A) {
				priv->pmib->dot11BssType.net_work_type &= (WIRELESS_11B | WIRELESS_11G);
				PRINT_INFO("operation band not appropriate, use G/B as default!\n");
			}
		}

		// validate channel number
		if (get_available_channel(priv) == FAIL) {
			PRINT_INFO("can't get operation channels, abort!\n");
			return 1;
		}
		if (priv->pmib->dot11RFEntry.dot11channel != 0) {
			for (i=0; i<priv->available_chnl_num; i++)
				if (priv->pmib->dot11RFEntry.dot11channel == priv->available_chnl[i])
					break;
			if (i == priv->available_chnl_num) {
				priv->pmib->dot11RFEntry.dot11channel = priv->available_chnl[0];

				PRINT_INFO("invalid channel number, use default value [%d]!\n",
					priv->pmib->dot11RFEntry.dot11channel);
			}
			priv->auto_channel = 0;
#ifdef SIMPLE_CH_UNI_PROTOCOL
			SET_PSEUDO_RANDOM_NUMBER(priv->mesh_ChannelPrecedence);
#endif
		}
		else {
#ifdef SIMPLE_CH_UNI_PROTOCOL
			if(GET_MIB(priv)->dot1180211sInfo.mesh_enable)
				priv->auto_channel = 16;
			else
#endif
			{
			if (OPMODE & WIFI_AP_STATE)
				priv->auto_channel = 1;
			else
				priv->auto_channel = 2;
			priv->pmib->dot11RFEntry.dot11channel = priv->available_chnl[0];
			}
		}
		priv->auto_channel_backup = priv->auto_channel;

		// validate hi and low channel
		if (priv->pmib->dot11RFEntry.dot11ch_low != 0) {
			for (i=0; i<priv->available_chnl_num; i++)
				if (priv->pmib->dot11RFEntry.dot11ch_low == priv->available_chnl[i])
					break;
			if (i == priv->available_chnl_num) {
				priv->pmib->dot11RFEntry.dot11ch_low = priv->available_chnl[0];

				PRINT_INFO("invalid low channel number, use default value [%d]!\n",
					priv->pmib->dot11RFEntry.dot11ch_low);
			}
		}
		if (priv->pmib->dot11RFEntry.dot11ch_hi != 0) {
			for (i=0; i<priv->available_chnl_num; i++)
				if (priv->pmib->dot11RFEntry.dot11ch_hi == priv->available_chnl[i])
					break;
			if (i == priv->available_chnl_num) {
				priv->pmib->dot11RFEntry.dot11ch_hi = priv->available_chnl[priv->available_chnl_num-1];

				PRINT_INFO("invalid hi channel number, use default value [%d]!\n",
					priv->pmib->dot11RFEntry.dot11ch_hi);
			}
		}

// Mark the code to auto disable N mode in WEP encrypt
//------------------------------ david+2008-01-11

		// support cck only in channel 14
		if ((priv->pmib->dot11RFEntry.disable_ch14_ofdm) &&
			(priv->pmib->dot11RFEntry.dot11channel == 14)) {
			priv->pmib->dot11BssType.net_work_type = WIRELESS_11B;
			PRINT_INFO("support cck only in channel 14!\n");
		}

		// validate and get support and basic rates
		validate_oper_rate(priv);
		get_oper_rate(priv);

		if (priv->pmib->dot11nConfigEntry.dot11nUse40M &&
			(!(priv->pmib->dot11BssType.net_work_type & WIRELESS_11N))) {
			PRINT_INFO("enable 40M but not in N mode! back to 20M\n");
			priv->pmib->dot11nConfigEntry.dot11nUse40M = 0;
		}

		// check deny band
		if ((priv->pmib->dot11BssType.net_work_type & (~priv->pmib->dot11StationConfigEntry.legacySTADeny)) == 0) {
			PRINT_INFO("legacySTADeny %d not suitable! set to 0\n", priv->pmib->dot11StationConfigEntry.legacySTADeny);
			priv->pmib->dot11StationConfigEntry.legacySTADeny = 0;
		}
	}

	if (!(priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)) {
		if (AMSDU_ENABLE)
			AMSDU_ENABLE = 0;
		if (AMPDU_ENABLE)
			AMPDU_ENABLE = 0;
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
		if (IS_ROOT_INTERFACE(priv))
#endif
		{
			priv->pshare->is_40m_bw = 0;
			priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_DONTCARE;
		}
	}
	else {
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
		if (IS_ROOT_INTERFACE(priv))
#endif
		{
			priv->pshare->is_40m_bw = priv->pmib->dot11nConfigEntry.dot11nUse40M;
			if (priv->pshare->is_40m_bw == 0)
				priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_DONTCARE;
			else {
				if ((priv->pmib->dot11RFEntry.dot11channel < 5) &&
						(priv->pmib->dot11nConfigEntry.dot11n2ndChOffset == HT_2NDCH_OFFSET_BELOW))
					priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_ABOVE;
				else if ((priv->pmib->dot11RFEntry.dot11channel > 9) &&
						(priv->pmib->dot11nConfigEntry.dot11n2ndChOffset == HT_2NDCH_OFFSET_ABOVE))
					priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_BELOW;
				else
					priv->pshare->offset_2nd_chan = priv->pmib->dot11nConfigEntry.dot11n2ndChOffset;
			}
		}

		// force wmm enabled if n mode
		QOS_ENABLE = 1;
	}

#ifdef __ZINWELL__
	if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm >= 10) {
		priv->indicate_auth = 1;
		priv->keep_encrypt_key = 1;
		priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm -= 10;
		PRINT_INFO("Assert indicate_auth and keep_encrypt_key flag!\n");
	}
#endif

	// set wep key length
	if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_)
		priv->pmib->dot1180211AuthEntry.dot11PrivacyKeyLen = 8;
	else if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_)
		priv->pmib->dot1180211AuthEntry.dot11PrivacyKeyLen = 16;

#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
		init_crc32_table();	// for sw encryption
	}

#ifdef SEMI_QOS
	if (QOS_ENABLE) {
		if ((OPMODE & WIFI_AP_STATE)
#ifdef CLIENT_MODE
			|| (OPMODE & WIFI_ADHOC_STATE)
#endif
			) {
			GET_EDCA_PARA_UPDATE = 0;
			//BK
			GET_STA_AC_BK_PARA.AIFSN = 7;
			GET_STA_AC_BK_PARA.TXOPlimit = 0;
			GET_STA_AC_BK_PARA.ACM = 0;
			GET_STA_AC_BK_PARA.ECWmin = 4;
			GET_STA_AC_BK_PARA.ECWmax = 10;
			//BE
			GET_STA_AC_BE_PARA.AIFSN = 3;
			GET_STA_AC_BE_PARA.TXOPlimit = 0;
			GET_STA_AC_BE_PARA.ACM = 0;
			GET_STA_AC_BE_PARA.ECWmin = 4;
			GET_STA_AC_BE_PARA.ECWmax = 10;
			//VI
			GET_STA_AC_VI_PARA.AIFSN = 2;
			if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) ||
				(priv->pmib->dot11BssType.net_work_type & WIRELESS_11A))
				GET_STA_AC_VI_PARA.TXOPlimit = 94; // 3.008ms
			else
				GET_STA_AC_VI_PARA.TXOPlimit = 188; // 6.016ms
			GET_STA_AC_VI_PARA.ACM = 0;
			GET_STA_AC_VI_PARA.ECWmin = 3;
			GET_STA_AC_VI_PARA.ECWmax = 4;
			//VO
			GET_STA_AC_VO_PARA.AIFSN = 2;
			if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) ||
				(priv->pmib->dot11BssType.net_work_type & WIRELESS_11A))
				GET_STA_AC_VO_PARA.TXOPlimit = 47; // 1.504ms
			else
				GET_STA_AC_VO_PARA.TXOPlimit = 102; // 3.264ms
			GET_STA_AC_VO_PARA.ACM = 0;
			GET_STA_AC_VO_PARA.ECWmin = 2;
			GET_STA_AC_VO_PARA.ECWmax = 3;

			//init WMM Para ie in beacon
			init_WMM_Para_Element(priv, priv->pmib->dot11QosEntry.WMM_PARA_IE);
		}
#ifdef CLIENT_MODE
		else if (OPMODE & WIFI_STATION_STATE)
			init_WMM_Para_Element(priv, priv->pmib->dot11QosEntry.WMM_IE);  //  WMM STA
#endif
	}
#endif

	i = priv->pmib->dot11ErpInfo.ctsToSelf;
	memset(&priv->pmib->dot11ErpInfo, '\0', sizeof(struct erp_mib)); // reset ERP mib
	priv->pmib->dot11ErpInfo.ctsToSelf = i;

	if ( (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) ||
			(priv->pmib->dot11BssType.net_work_type & WIRELESS_11A) )
		priv->pmib->dot11ErpInfo.shortSlot = 1;
	else
		priv->pmib->dot11ErpInfo.shortSlot = 0;

	if (OPMODE & WIFI_AP_STATE) {
		memcpy(priv->pmib->dot11StationConfigEntry.dot11Bssid,
				priv->pmib->dot11OperationEntry.hwaddr, 6);
		//priv->oper_band = priv->pmib->dot11BssType.net_work_type;
	}
#ifdef CLIENT_MODE
	else {
		if (priv->pmib->dot11StationConfigEntry.dot11DefaultSSIDLen == 0) {
			priv->pmib->dot11StationConfigEntry.dot11DefaultSSIDLen = 11;
			memcpy(priv->pmib->dot11StationConfigEntry.dot11DefaultSSID, "defaultSSID", 11);
		}
		memset(priv->pmib->dot11StationConfigEntry.dot11Bssid, 0, 6);
		priv->join_res = STATE_Sta_No_Bss;

// Add mac clone address manually ----------
#ifdef RTK_BR_EXT
		if (priv->pmib->ethBrExtInfo.macclone_enable == 2) {
			extern void mac_clone(struct rtl8190_priv *priv, unsigned char *addr);
			mac_clone(priv, priv->pmib->ethBrExtInfo.nat25_dmzMac);
			priv->macclone_completed = 1;
		}
#endif
//------------------------- david+2007-5-31
	}
#endif

	// initialize event queue
	DOT11_InitQueue(priv->pevent_queue);
#ifdef CONFIG_RTL_WAPI_SUPPORT
	DOT11_InitQueue(priv->wapiEvent_queue);
#endif

#ifdef CONFIG_RTK_MESH
	if(GET_MIB(priv)->dot1180211sInfo.mesh_enable == 1)	// plus add 0217, not mesh mode should not do below function
	{
	DOT11_InitQueue2(priv->pathsel_queue, MAXQUEUESIZE2, MAXDATALEN2);
#ifdef	_11s_TEST_MODE_
	DOT11_InitQueue2(priv->receiver_queue, MAXQUEUESIZE2, MAXDATALEN2);
#endif
		//modify by Joule for SECURITY
		i = priv->pmib->dot11sKeysTable.dot11Privacy;
		memset(&priv->pmib->dot11sKeysTable, '\0', sizeof(struct Dot11KeyMappingsEntry)); // reset key
		priv->pmib->dot11sKeysTable.dot11Privacy = i;
	 }	//
#endif

#ifdef __DRAYTEK_OS__
	if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _TKIP_PRIVACY_ &&
		priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _CCMP_PRIVACY_ &&
		priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _WEP_WPA_MIXED_PRIVACY_) {
#ifdef UNIVERSAL_REPEATER
		if (IS_ROOT_INTERFACE(priv))
#endif
		{
			priv->pmib->dot11RsnIE.rsnielen = 0;	// reset RSN IE length
			memset(&priv->pmib->dot11GroupKeysTable, '\0', sizeof(struct Dot11KeyMappingsEntry)); // reset group key
#ifdef UNIVERSAL_REPEATER
			if (GET_VXD_PRIV(priv))
				GET_VXD_PRIV(priv)->pmib->dot11RsnIE.rsnielen = 0;
#endif
		}
	}
#endif

	i = RC_ENTRY_NUM;
	for (;;) {
		if (priv->pmib->reorderCtrlEntry.ReorderCtrlWinSz >= i) {
			priv->pmib->reorderCtrlEntry.ReorderCtrlWinSz = i;
			break;
		}
		else if (i > 8)
			i = i / 2;
		else {
			priv->pmib->reorderCtrlEntry.ReorderCtrlWinSz = 8;
			break;
		}
	}

//#ifndef RTL8192SE
	// Realtek proprietary IE
	memcpy(&(priv->pshare->rtk_ie_buf[0]), Realtek_OUI, 3);
	priv->pshare->rtk_ie_buf[3] = 2;
	priv->pshare->rtk_ie_buf[4] = 1;
	priv->pshare->rtk_ie_buf[5] = 0;
	priv->pshare->rtk_ie_buf[5] |= RTK_CAP_IE_WLAN_8192SE;
#ifdef CLIENT_MODE
	if (OPMODE & WIFI_STATION_STATE)
		priv->pshare->rtk_ie_buf[5] |= RTK_CAP_IE_AP_CLEINT;
#endif
	priv->pshare->rtk_ie_len = 6;

#ifdef A4_TUNNEL
	if (priv->pmib->miscEntry.a4tnl_enable) {
		priv->pmib->ethBrExtInfo.nat25_disable = 1;
		priv->pmib->ethBrExtInfo.macclone_enable = 1;

		for (i=0; i<NUM_STAT; i++)
			INIT_LIST_HEAD(&(priv->a4tnl_table[i]));
	}
#endif

#ifdef INCLUDE_WPA_PSK
// button 2009.05.21
	if (priv->pmib->dot1180211AuthEntry.dot11EnablePSK)
	{
		priv->wpa_global_info->MulticastCipher=0;
		psk_init(priv);
	}

#ifdef WDS
#ifdef UNIVERSAL_REPEATER
	if (IS_ROOT_INTERFACE(priv))
#endif
		if ((priv->pmib->dot11WdsInfo.wdsEnabled) && (priv->pmib->dot11WdsInfo.wdsNum > 0) &&
			((priv->pmib->dot11WdsInfo.wdsPrivacy == _TKIP_PRIVACY_) ||
			 (priv->pmib->dot11WdsInfo.wdsPrivacy == _CCMP_PRIVACY_)))
			wds_psk_init(priv);
#endif
#endif

#ifdef GBWC
	priv->GBWC_tx_queue.head = 0;
	priv->GBWC_tx_queue.tail = 0;
	priv->GBWC_rx_queue.head = 0;
	priv->GBWC_rx_queue.tail = 0;
	priv->GBWC_tx_count = 0;
	priv->GBWC_rx_count = 0;
	priv->GBWC_consuming_Q = 0;
#endif

#ifdef SW_MCAST
	priv->release_mcast = 0;
	priv->queued_mcast = 0;
#endif

#ifdef USB_PKT_RATE_CTRL_SUPPORT //mark_test
	priv->change_toggle = 0;
	priv->pre_pkt_cnt = 0;
	priv->pkt_nsec_diff = 0;
	priv->poll_usb_cnt = 0;
	priv->auto_rate_mask = 0;
#endif

#if defined(RTL8192E) || defined(STA_EXT)
	priv->pshare->fw_free_space = FW_NUM_STAT - 2; // One for MAGANEMENT_AID, one for other STAs
#endif

#if (defined(RTL8192SE)||defined(RTL8192SU)) && defined(EXT_ANT_DVRY)
	priv->pshare->EXT_AD_counter = priv->pshare->rf_ft_var.ext_ad_Ttry;
#endif

#ifdef CONFIG_RTK_VLAN_SUPPORT
	if (priv->pmib->vlan.global_vlan)
		priv->pmib->dot11OperationEntry.disable_brsc = 1;
#endif


#if defined(CONFIG_RTL_WAPI_SUPPORT)
	if (priv->pmib->wapiInfo.wapiType!=wapiDisable)
	{
	/*	set NMK	*/
	GenerateRandomData(priv->wapiNMK, WAPI_KEY_LEN);
	priv->wapiMCastKeyId = 0;
	priv->wapiMCastKeyUpdate = 0;
	wapiInit(priv);
	}
#endif

#ifdef MBSSID
	if (IS_ROOT_INTERFACE(priv) && 
			(OPMODE & WIFI_AP_STATE) &&
			priv->pmib->miscEntry.vap_enable &&
			RTL8190_NUM_VWLAN > 1 &&
			priv->pmib->dot11StationConfigEntry.dot11BeaconPeriod < 100)		
		priv->pmib->dot11StationConfigEntry.dot11BeaconPeriod = 100;		
#endif

	priv->amsdu_skb=NULL;
	priv->amsdu_pkt_remain_size=0;

	return 0;
}


static int rtl8190_stop_sw(struct rtl8190_priv *priv)
{
	struct rtl8190_hw *phw;
	unsigned long	flags;
	int	i;

	// we hope all this can be done in critical section
	SAVE_INT_AND_CLI(flags);

	del_timer_sync(&priv->frag_to_filter);
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
		del_timer_sync(&priv->expire_timer);
		del_timer_sync(&priv->pshare->rc_sys_timer);
	if (timer_pending(&priv->pshare->phw->tpt_timer))
		del_timer_sync(&priv->pshare->phw->tpt_timer);
		if(timer_pending(&priv->poll_usb_timer))
			del_timer_sync(&priv->poll_usb_timer);
#if defined(CONFIG_RTL_WAPI_SUPPORT)
	if (priv->pmib->wapiInfo.wapiType!=wapiDisable
   #ifdef CHECK_HANGUP
		&&(!priv->reset_hangup)
   #endif
		)
	{
		wapiExit(priv);
	}
#endif
#ifdef CONFIG_RTK_MESH
		/*
		 * CAUTION !! These statement meshX(virtual interface) ONLY, Maybe modify....
		*/
		if (timer_pending(&priv->mesh_peer_link_timer))
			del_timer_sync(&priv->mesh_peer_link_timer);

#ifdef MESH_BOOTSEQ_AUTH
		if (timer_pending(&priv->mesh_auth_timer))
			del_timer_sync(&priv->mesh_auth_timer);
#endif
#endif
#ifdef RTL867X_DMEM_ENABLE
		if(priv->pshare->fw_IMEM_buf!=NULL) 
		{
			kfree(priv->pshare->fw_IMEM_buf); 
			priv->pshare->fw_IMEM_buf=NULL;
		}
		if(priv->pshare->fw_EMEM_buf!=NULL)
		{
			kfree(priv->pshare->fw_EMEM_buf);
			priv->pshare->fw_EMEM_buf=NULL;
		}
		if(priv->pshare->fw_DMEM_buf!=NULL)
		{
			kfree(priv->pshare->fw_DMEM_buf);
			priv->pshare->fw_DMEM_buf=NULL;
		}
#endif

	}
#ifdef RTL8192SU_SW_BEACON
	del_timer_sync(&priv->beacon_timer);
#endif
	del_timer_sync(&priv->ss_timer);
	del_timer_sync(&priv->MIC_check_timer);
	del_timer_sync(&priv->assoc_reject_timer);

	// bcm old 11n chipset iot debug

	// to avoid add RAtid fail
	del_timer_sync(&priv->add_RATid_timer);

#ifdef CLIENT_MODE
	del_timer_sync(&priv->reauth_timer);
	del_timer_sync(&priv->reassoc_timer);
	del_timer_sync(&priv->idle_timer);
#endif

#ifdef GBWC
	del_timer_sync(&priv->GBWC_timer);
	while (CIRC_CNT(priv->GBWC_tx_queue.head, priv->GBWC_tx_queue.tail, NUM_TXPKT_QUEUE))
	{
		struct sk_buff *pskb = priv->GBWC_tx_queue.pSkb[priv->GBWC_tx_queue.tail];
#ifdef RTL8190_FASTEXTDEV_FUNCALL
		rtl_actually_kfree_skb(priv, pskb, priv->dev, _SKB_RX_);
#else
		rtl_kfree_skb(priv, pskb, _SKB_TX_);
#endif
		priv->GBWC_tx_queue.tail++;
		priv->GBWC_tx_queue.tail = priv->GBWC_tx_queue.tail & (NUM_TXPKT_QUEUE - 1);
	}
	while (CIRC_CNT(priv->GBWC_rx_queue.head, priv->GBWC_rx_queue.tail, NUM_TXPKT_QUEUE))
	{
		struct sk_buff *pskb = priv->GBWC_rx_queue.pSkb[priv->GBWC_rx_queue.tail];
#ifdef RTL8190_FASTEXTDEV_FUNCALL
		rtl_actually_kfree_skb(priv, pskb, priv->dev, _SKB_RX_);
#else
		rtl_kfree_skb(priv, pskb, _SKB_RX_);
#endif
		priv->GBWC_rx_queue.tail++;
		priv->GBWC_rx_queue.tail = priv->GBWC_rx_queue.tail & (NUM_TXPKT_QUEUE - 1);
	}
#endif

#ifdef INCLUDE_WPA_PSK
	if (timer_pending(&priv->wpa_global_info->GKRekeyTimer))
		del_timer_sync(&priv->wpa_global_info->GKRekeyTimer);
#endif

#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
#ifdef DFS
		if (timer_pending(&priv->DFS_timer))
			del_timer_sync(&priv->DFS_timer);

		if (timer_pending(&priv->ch_avail_chk_timer))
			del_timer_sync(&priv->ch_avail_chk_timer);

		// when we disable the DFS function dynamically, we also remove the channel
		// from NOP_chnl[4] while the driver is rebooting
		if (timer_pending(&priv->ch52_timer) &&
			(priv->pmib->dot11DFSEntry.disable_DFS)) {
			del_timer_sync(&priv->ch52_timer);
			RemoveChannel(priv->NOP_chnl, &priv->NOP_chnl_num, 52);
		}

		if (timer_pending(&priv->ch56_timer) &&
			(priv->pmib->dot11DFSEntry.disable_DFS)) {
			del_timer_sync(&priv->ch56_timer);
			RemoveChannel(priv->NOP_chnl, &priv->NOP_chnl_num, 56);
		}

		if (timer_pending(&priv->ch60_timer) &&
			(priv->pmib->dot11DFSEntry.disable_DFS)) {
			del_timer_sync(&priv->ch60_timer);
			RemoveChannel(priv->NOP_chnl, &priv->NOP_chnl_num, 60);
		}

		if (timer_pending(&priv->ch64_timer) &&
			(priv->pmib->dot11DFSEntry.disable_DFS)) {
			del_timer_sync(&priv->ch64_timer);
			RemoveChannel(priv->NOP_chnl, &priv->NOP_chnl_num, 64);
		}

		// For JAPAN in adhoc mode
		if (((priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_MKK)	||
			 (priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_MKK3)) &&
			 (OPMODE & WIFI_ADHOC_STATE)) {
			if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11A) {
				if (!timer_pending(&priv->ch52_timer))
					RemoveChannel(priv->NOP_chnl, &priv->NOP_chnl_num, 52);
				if (!timer_pending(&priv->ch56_timer))
					RemoveChannel(priv->NOP_chnl, &priv->NOP_chnl_num, 56);
				if (!timer_pending(&priv->ch60_timer))
					RemoveChannel(priv->NOP_chnl, &priv->NOP_chnl_num, 60);
				if (!timer_pending(&priv->ch64_timer))
					RemoveChannel(priv->NOP_chnl, &priv->NOP_chnl_num, 64);
			}
		}
#endif // DFS

		// for SW LED
		if ((LED_TYPE >= LEDTYPE_SW_LINK_TXRX) && (LED_TYPE < LEDTYPE_SW_MAX) || priv->pshare->LED_sw_use)
			disable_sw_LED(priv);

#ifdef DFS
		// prevent killing tasklet issue in interrupt
		if (!priv->pmib->dot11DFSEntry.DFS_detected)
#endif
		{
#ifdef CHECK_HANGUP
			if (!priv->reset_hangup)
#endif
			{
				tasklet_kill(&priv->pshare->rx_tasklet);
				tasklet_kill(&priv->pshare->oneSec_tasklet);
			}
		}

		phw = GET_HW(priv);

#ifdef HW_QUICK_INIT
		if (priv->pshare->hw_inited)
			goto skip_clear_desc;
#endif



	} // if (IS_ROOT_INTERFACE(priv))

#ifdef HW_QUICK_INIT
skip_clear_desc:
#endif

	for (i=0; i<NUM_STAT; i++)
	{
		if (priv->pshare->aidarray[i]) {
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
			if (priv != priv->pshare->aidarray[i]->priv)
				continue;
			else
#endif
			{
				if (priv->pshare->aidarray[i]->used == TRUE)
					if (free_stainfo(priv, &(priv->pshare->aidarray[i]->station)) == FALSE)
					DEBUG_ERR("free station %d fails\n", i);

#if defined(SEMI_QOS) && defined(WMM_APSD)
#ifdef PRIV_STA_BUF
				free_sta_que(priv, priv->pshare->aidarray[i]->station.VO_dz_queue);
				free_sta_que(priv, priv->pshare->aidarray[i]->station.VI_dz_queue);
				free_sta_que(priv, priv->pshare->aidarray[i]->station.BE_dz_queue);
				free_sta_que(priv, priv->pshare->aidarray[i]->station.BK_dz_queue);

#else
				kfree(priv->pshare->aidarray[i]->station.VO_dz_queue);
				kfree(priv->pshare->aidarray[i]->station.VI_dz_queue);
				kfree(priv->pshare->aidarray[i]->station.BE_dz_queue);
				kfree(priv->pshare->aidarray[i]->station.BK_dz_queue);
#endif
#endif
#ifdef INCLUDE_WPA_PSK
#ifdef PRIV_STA_BUF
				free_wpa_buf(priv, priv->pshare->aidarray[i]->station.wpa_sta_info);
#else
				kfree(priv->pshare->aidarray[i]->station.wpa_sta_info);
#endif
#endif
#ifdef RTL8190_VARIABLE_USED_DMEM
			{
				unsigned int index = (unsigned int)i;
				rtl8190_dmem_free(AID_OBJ, &index);
			}
#else
#ifdef PRIV_STA_BUF
				free_sta_obj(priv, priv->pshare->aidarray[i]);
#else
				kfree(priv->pshare->aidarray[i]);
#endif
#endif
				priv->pshare->aidarray[i] = NULL;
			}
		}
	}

#ifndef __DRAYTEK_OS__
	// reset rsnie and group key from open to here, david
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
#ifdef WIFI_SIMPLE_CONFIG
		if (!priv->pmib->dot11OperationEntry.keep_rsnie) {
			priv->pmib->wscEntry.beacon_ielen = 0;
			priv->pmib->wscEntry.probe_rsp_ielen = 0;
			priv->pmib->wscEntry.probe_req_ielen = 0;
			priv->pmib->wscEntry.assoc_ielen = 0;
		}

//		if (!(OPMODE & WIFI_AP_STATE))
//			priv->pmib->dot11OperationEntry.keep_rsnie = 1;
#endif
	}

	if (!priv->pmib->dot11OperationEntry.keep_rsnie) {
		priv->pmib->dot11RsnIE.rsnielen = 0;	// reset RSN IE length
		memset(&priv->pmib->dot11GroupKeysTable, '\0', sizeof(struct Dot11KeyMappingsEntry)); // reset group key
#ifdef UNIVERSAL_REPEATER
		if (GET_VXD_PRIV(priv))
			GET_VXD_PRIV(priv)->pmib->dot11RsnIE.rsnielen = 0;
#endif
		priv->auto_channel_backup = 0;
	}
	else {
		// When wlan scheduling and auto-chan case, it will disable/enable
		// wlan interface directly w/o re-set mib. Therefore, we need use
		// "keep_rsnie" flag to keep auto-chan value

		if (
#ifdef CHECK_HANGUP
			!priv->reset_hangup &&
#endif
			priv->auto_channel_backup)
			priv->pmib->dot11RFEntry.dot11channel = 0;
	}
#endif

#ifdef MBSSID
	if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
	{
		if (IS_VAP_INTERFACE(priv) && !priv->pmib->dot11OperationEntry.keep_rsnie) {
			priv->pmib->dot11RsnIE.rsnielen = 0;	// reset RSN IE length
			memset(&priv->pmib->dot11GroupKeysTable, '\0', sizeof(struct Dot11KeyMappingsEntry)); // reset group key
		}
	}
#endif

#ifdef RTK_BR_EXT
	if (OPMODE & (WIFI_STATION_STATE | WIFI_ADHOC_STATE)) {
#ifdef CHECK_HANGUP
		if (!priv->reset_hangup)
#endif
			nat25_db_cleanup(priv);
	}
#endif

#ifdef A4_TUNNEL
	if (priv->pmib->miscEntry.a4tnl_enable) {
		for (i=0; i<NUM_STAT; i++) {
			while (!list_empty(&(priv->a4tnl_table[i]))) {
				struct list_head *plist;
				struct wlan_a4tnl_node *pbuf;
				plist = priv->a4tnl_table[i].next;
				list_del_init(plist);
				pbuf = list_entry(plist, struct wlan_a4tnl_node, list);
				kfree(pbuf);
			}
		}
	}
#endif

#ifdef SW_MCAST
	{
		int				hd, tl;
		struct sk_buff	*pskb;

		hd = priv->dz_queue.head;
		tl = priv->dz_queue.tail;
		while (CIRC_CNT(hd, tl, NUM_TXPKT_QUEUE))
		{
			pskb = priv->dz_queue.pSkb[tl];
			rtl_kfree_skb(priv, pskb, _SKB_TX_);
			tl++;
			tl = tl & (NUM_TXPKT_QUEUE - 1);
		}
		priv->dz_queue.head = 0;
		priv->dz_queue.tail = 0;
	}
#endif

	if (priv->amsdu_skb != NULL)
		rtl_kfree_skb(priv, priv->amsdu_skb, _SKB_TX_);

	RESTORE_INT(flags);
	printk("stop sw finished!\n");
	return 0;
}


#ifdef MBSSID
static void rtl8190_init_vap_mib(struct rtl8190_priv *priv)
{

	// copy mib_rf from root interface
	memcpy(&priv->pmib->dot11RFEntry, &GET_ROOT_PRIV(priv)->pmib->dot11RFEntry, sizeof(struct Dot11RFEntry));

	// special mib that need to set
#ifdef SEMI_QOS
	//QOS_ENABLE = 0;
#ifdef WMM_APSD
	APSD_ENABLE = 0;
#endif
#endif

#ifdef WDS
	// always disable wds in vap
	priv->pmib->dot11WdsInfo.wdsEnabled = 0;
	priv->pmib->dot11WdsInfo.wdsPure = 0;
#endif
#ifdef CONFIG_RTK_MESH
	// in current release, mesh can be only run upon wlan0, so we disable the following flag in vap
	priv->pmib->dot1180211sInfo.mesh_enable = 0;
#endif
}


static void rtl8190_init_mbssid(struct rtl8190_priv *priv)
{
	int i, j;
	unsigned int camData[2];
	unsigned char *macAddr = GET_MY_HWADDR;

	if (IS_ROOT_INTERFACE(priv))
	{
		camData[0] = 0x00800000 | (macAddr[5] << 8) | macAddr[4];
		camData[1] = (macAddr[3] << 24) | (macAddr[2] << 16) | (macAddr[1] << 8) | macAddr[0];
		for (j=0; j<2; j++) {
			RTL_W32((_MBIDCAMCONTENT_+4)-4*j, camData[j]);
		}
		RTL_W8(_MBIDCAMCFG_, BIT(7) | BIT(6));

		// clear the rest area of CAM
		camData[0] = 0;
		camData[1] = 0;
		for (i=1; i<32; i++) {
			for (j=0; j<2; j++) {
				RTL_W32((_MBIDCAMCONTENT_+4)-4*j, camData[j]);
			}
			RTL_W8(_MBIDCAMCFG_, BIT(7) | BIT(6) | (unsigned char)i);
		}

		// set MBIDCTRL & MBID_BCN_SPACE by cmd
		set_fw_reg(priv, 0xf1000101, 0, 0);
		RTL_W32(_RCR_, RTL_R32(_RCR_) | _ENMBID_);	// MBSSID enable

#ifdef CLIENT_MODE
		if ((OPMODE & WIFI_STATION_STATE) || (OPMODE & WIFI_ADHOC_STATE))
			RTL_W32(_RCR_, RTL_R32(_RCR_) | _CBSSID_);
#endif

	}
	else if (IS_VAP_INTERFACE(priv))
	{
		priv->vap_init_seq = (RTL_R8(_MBIDCTRL_) & (BIT(4) | BIT(5) | BIT(6))) >> 4;
		priv->vap_init_seq++;
		set_fw_reg(priv, 0xf1000001 | ((priv->vap_init_seq + 1)&0xffff)<<8, 0, 0);

		camData[0] = 0x00800000 | (macAddr[5] << 8) | macAddr[4];
		camData[1] = (macAddr[3] << 24) | (macAddr[2] << 16) | (macAddr[1] << 8) | macAddr[0];
		for (j=0; j<2; j++) {
			RTL_W32((_MBIDCAMCONTENT_+4)-4*j, camData[j]);
		}
		RTL_W8(_MBIDCAMCFG_, BIT(7) | BIT(6) | ((unsigned char)priv->vap_init_seq & 0x1f));

		RTL_W32(_RCR_, RTL_R32(_RCR_) & ~_ENMBID_);
		RTL_W32(_RCR_, RTL_R32(_RCR_) | _ENMBID_);	// MBSSID enable
	}
}


static void rtl8190_stop_mbssid(struct rtl8190_priv *priv)
{
	int i, j;

	if (IS_ROOT_INTERFACE(priv))
	{
		// clear the rest area of CAM
		for (i=0; i<32; i++) {
			for (j=0; j<2; j++) {
				RTL_W32((_MBIDCAMCONTENT_+4)-4*j, 0);
			}
			RTL_W8(_MBIDCAMCFG_, BIT(7) | BIT(6) | (unsigned char)i);
		}

		set_fw_reg(priv, 0xf1000001, 0, 0);
		RTL_W32(_RCR_, RTL_R32(_RCR_) & ~_ENMBID_);	// MBSSID disable
	}
	else if (IS_VAP_INTERFACE(priv) && (priv->vap_init_seq >= 0))
	{
		set_fw_reg(priv, 0xf1000001 | (((RTL_R8(_MBIDCTRL_) & (BIT(4) | BIT(5) | BIT(6))) >> 4)&0xffff)<<8, 0, 0);

		for (j=0; j<2; j++) {
			RTL_W32((_MBIDCAMCONTENT_+4)-4*j, 0);
		}
		RTL_W8(_MBIDCAMCFG_, BIT(7) | BIT(6) | ((unsigned char)priv->vap_init_seq & 0x1f));

		RTL_W32(_RCR_, RTL_R32(_RCR_) & ~_ENMBID_);
		RTL_W32(_RCR_, RTL_R32(_RCR_) | _ENMBID_);
		priv->vap_init_seq = -1;
	}
}
#endif


#ifdef WDS
static void create_wds_tbl(struct rtl8190_priv *priv)
{
	int i;
	struct stat_info *pstat;
	DOT11_SET_KEY Set_Key;

#ifdef FAST_RECOVERY
	if (priv->reset_hangup)
		return;
#endif

#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (!IS_ROOT_INTERFACE(priv))
		return;
#endif

	if (priv->pmib->dot11WdsInfo.wdsEnabled)
	{	// add wds into sta table
		for (i=0; i<priv->pmib->dot11WdsInfo.wdsNum; i++)
		{
			printk("%s: dot11WdsInfo\n", __FUNCTION__);
			for(i=0;i<MACADDRLEN;i++)
			printk(" %02x",priv->pmib->dot11WdsInfo.entry[i].macAddr[i]);
			printk("\n");
			pstat = alloc_stainfo(priv, priv->pmib->dot11WdsInfo.entry[i].macAddr, -1);
			if (pstat == NULL) {
				DEBUG_ERR("alloc_stainfo() fail in rtl8190_open!\n");
				break;
			}

			// use self supported rate for wds
			memcpy(pstat->bssrateset, AP_BSSRATE, AP_BSSRATE_LEN);
			pstat->bssratelen = AP_BSSRATE_LEN;

			if (priv->pmib->dot11WdsInfo.entry[i].txRate) {
				if (!(priv->pmib->dot11BssType.net_work_type & WIRELESS_11N))
						priv->pmib->dot11WdsInfo.entry[i].txRate &= 0x0000fff;	// mask HT rates

				if (!(priv->pmib->dot11BssType.net_work_type & WIRELESS_11A) &&
					!(priv->pmib->dot11BssType.net_work_type & WIRELESS_11G))
					priv->pmib->dot11WdsInfo.entry[i].txRate &= 0xffff00f;	// mask OFDM rates

				if (!(priv->pmib->dot11BssType.net_work_type & WIRELESS_11B))
					priv->pmib->dot11WdsInfo.entry[i].txRate &= 0xffffff0;	// mask CCK rates
			}
			pstat->state = WIFI_WDS;

			if (priv->pmib->dot11WdsInfo.wdsPrivacy == _WEP_40_PRIVACY_ ||
					priv->pmib->dot11WdsInfo.wdsPrivacy == _WEP_104_PRIVACY_ ) {
#ifndef CONFIG_RTL8186_KB
				memcpy(Set_Key.MACAddr, priv->pmib->dot11WdsInfo.entry[i].macAddr, 6);
				Set_Key.KeyType = DOT11_KeyType_Pairwise;
				Set_Key.EncType = priv->pmib->dot11WdsInfo.wdsPrivacy;
				Set_Key.KeyIndex = 0;
				DOT11_Process_Set_Key(priv->dev, NULL, &Set_Key, priv->pmib->dot11WdsInfo.wdsWepKey);
#endif
			}
			else if ((priv->pmib->dot11WdsInfo.wdsPrivacy == _TKIP_PRIVACY_ ||
						priv->pmib->dot11WdsInfo.wdsPrivacy == _CCMP_PRIVACY_) &&
// modify to re-init WDS key when tx-hangup reset
//					(priv->pmib->dot11WdsInfo.wdsMappingKeyLen[i] > 0) ) {
					(priv->pmib->dot11WdsInfo.wdsMappingKeyLen[i]&0x80000000) ) {
				priv->pmib->dot11WdsInfo.wdsMappingKeyLen[i] &= ~0x80000000;
//------------------------- david+2006-06-30
				memcpy(Set_Key.MACAddr, priv->pmib->dot11WdsInfo.entry[i].macAddr, 6);
				Set_Key.KeyType = DOT11_KeyType_Pairwise;
				Set_Key.EncType = priv->pmib->dot11WdsInfo.wdsPrivacy;
				Set_Key.KeyIndex = priv->pmib->dot11WdsInfo.wdsKeyId;
				DOT11_Process_Set_Key(priv->dev, NULL, &Set_Key, priv->pmib->dot11WdsInfo.wdsMapingKey[i]);
			}

			pstat->QosEnabled= priv->pmib->dot11QosEntry.dot11QosEnable;
			if (priv->ht_cap_len > 0) {
				memcpy(&pstat->ht_cap_buf, &priv->ht_cap_buf, priv->ht_cap_len);
				pstat->ht_cap_len = priv->ht_cap_len;
//			pstat->ht_cap_buf.ht_cap_info &= ~cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_);
			}

			add_update_RATid(priv, pstat);
			pstat->wds_idx = i;


			assign_tx_rate(priv, pstat, NULL);
			assign_aggre_mthod(priv, pstat);

			list_add_tail(&pstat->asoc_list, &priv->asoc_list);
		}
	}
}
#endif //  WDS


void rtl8192su_rx_initiate(struct rtl8190_priv *priv)
{
	int i;

	struct urb *purb;
	struct sk_buff *pskb;
	//struct r8180_priv *priv = (struct r8180_priv *)netdev_priv(dev);
	struct net_device *dev = priv->dev;

	priv->tx_urb_index = 0;

	priv->rx_urb = (struct urb**) kmalloc (sizeof(struct urb*) * (MAX_RX_URB+1), GFP_KERNEL);
	priv->pp_rxskb = (struct sk_buff **)kmalloc(sizeof(struct sk_buff *) * MAX_RX_URB, GFP_KERNEL);	 
	priv->oldaddr = kmalloc(16, GFP_KERNEL);

	if ((!priv->rx_urb) || (!priv->pp_rxskb)) {
		printk("Cannot intiate RX urb mechanism\n");
		return;
	}
		
	memset(priv->rx_urb, 0, sizeof(struct urb*) * (MAX_RX_URB+1));
	memset(priv->pp_rxskb, 0, sizeof(struct sk_buff*) * MAX_RX_URB);


	priv->pshare->rx_hw_index = 0;
	priv->pshare->rx_schedule_index = 0;
		
	atomic_set(&priv->irt_counter,0);
	for(i = 0;i < MAX_RX_URB; i++)
	{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
		purb = priv->rx_urb[i] = usb_alloc_urb(0,GFP_KERNEL);
#else
		purb = priv->rx_urb[i] = usb_alloc_urb(0);
#endif
		if(!priv->rx_urb[i])
			goto destroy;
		pskb = priv->pp_rxskb[i] =  rtl_dev_alloc_skb(priv, RX_ALLOC_SIZE, _SKB_RX_,TRUE);

		pskb->head=(unsigned char *)((u32)pskb->head & (~0x20000000));
		pskb->data=pskb->head+RX_DMA_SHIFT;
		pskb->tail=pskb->data;
//		if((((u32)pskb->end) | ((u32)pskb->head+RX_DMA_SHIFT+RX_ALLOC_SIZE))!=(u32)pskb->end)
//			printk("BUG at: %s %d - pskb->end=%x,%x\n",__FUNCTION__,__LINE__,(u32)pskb->end,(u32)pskb->head+RX_DMA_SHIFT+RX_ALLOC_SIZE);
#if defined(PKT_PROCESSOR)
		pskb->end=(unsigned char *)((u32)pskb->head+RX_DMA_SHIFT+RX_ALLOC_SIZE);
#endif

#if defined(PKT_PROCESSOR) || defined(PRE_ALLOCATE_SKB)
		pskb->fcpu=0;
		pskb->pptx=NULL;
#endif		
		
		init_rxdesc (pskb, i, priv);

		if (pskb == NULL)
			goto destroy;

		purb->transfer_buffer_length = RX_URB_SIZE;
		purb->transfer_buffer = pskb->data;
		//dma_cache_wback_inv(pskb->data,RX_URB_SIZE);
	}

	for(i=0;i<MAX_RX_URB;i++)
	{
		rtl8192su_rx_urbsubmit(dev,priv->rx_urb[i],i);
	}

	return;

destroy:
	printk("%s %d: Memory is not enough!\n",__FUNCTION__,__LINE__);

	for(i = 0; i < MAX_RX_URB; i++)
	{
		purb = priv->rx_urb[i];

		if (purb)
			usb_free_urb(purb);
		pskb = priv->pp_rxskb[i];
		if (pskb)
			rtl_kfree_skb(priv, pskb, _SKB_RX_);

	}

	return;
}

void rtl8192su_rx_reinitiate(struct rtl8190_priv *priv)
{
	int i;

	struct urb *purb;
	struct sk_buff *pskb;
	//struct r8180_priv *priv = (struct r8180_priv *)netdev_priv(dev);
	struct net_device *dev = priv->dev;

	priv->tx_urb_index = 0;
	priv->pshare->rx_hw_index = 0;
	priv->pshare->rx_schedule_index = 0;
	atomic_set(&priv->irt_counter,0);
	for(i = 0;i < MAX_RX_URB; i++)
	{
		purb = priv->rx_urb[i];
		pskb = priv->pp_rxskb[i];
		pskb->head=(unsigned char *)((u32)pskb->head & (~0x20000000));
		pskb->data=pskb->head+RX_DMA_SHIFT;
		pskb->tail=pskb->data;

//		if((((u32)pskb->end) | ((u32)pskb->head+RX_DMA_SHIFT+RX_ALLOC_SIZE))!=(u32)pskb->end)
//			printk("BUG at: %s %d\n",__FUNCTION__,__LINE__);
#if defined(PKT_PROCESSOR)
		pskb->end=(unsigned char *)((u32)pskb->head+RX_DMA_SHIFT+RX_ALLOC_SIZE);
#endif
#if defined(PKT_PROCESSOR) || defined(PRE_ALLOCATE_SKB)
		pskb->fcpu=0;
		pskb->pptx=NULL;
#endif
		
		init_rxdesc (pskb, i, priv);

		if (pskb == NULL)
		{
			printk("ERR: %s %d\n",__FUNCTION__,__LINE__);
			while(1);
		}

		purb->transfer_buffer_length = RX_URB_SIZE;
		purb->transfer_buffer = pskb->data;
		//dma_cache_wback_inv(pskb->data,RX_URB_SIZE);
	}

	for(i=0;i<MAX_RX_URB;i++)
	{
		rtl8192su_rx_urbsubmit(dev,priv->rx_urb[i],i);
	}

	return;
}

void rtl8192su_rx_free(struct rtl8190_priv *priv)
{
	int i;
	if(priv)
	{
		for(i = 0;i < MAX_RX_URB; i++)
		{
			if(priv->rx_urb)
			{
				if(priv->rx_urb[i]) 
				{
					usb_kill_urb(priv->rx_urb[i]);		
					usb_free_urb( priv->rx_urb[i]);
					priv->rx_urb[i] = NULL;
				}
			}

			if(priv->pp_rxskb)
			{
				if(priv->pp_rxskb[i])
				{
					rtl_kfree_skb(priv, priv->pp_rxskb[i], _SKB_RX_);
					priv->pp_rxskb[i]=NULL;
				}
			}
		}
		priv->tx_urb_index = 0;

		if(priv->rx_urb) 
		{	
			kfree(priv->rx_urb);
			priv->rx_urb=NULL;
		}

		if(priv->pp_rxskb)
		{
			kfree(priv->pp_rxskb);
			priv->pp_rxskb=NULL;
		}

		if(priv->oldaddr)
		{
			kfree(priv->oldaddr);
			priv->oldaddr=NULL;
		}	

		if(priv->pshare)
		{
			priv->pshare->rx_hw_index = 0;
			priv->pshare->rx_schedule_index = 0;
				
			atomic_set(&priv->irt_counter,0);

			//free tx urb
			for(i=0;i<MAX_TX_URB;i++)
			{
				if(priv->pshare->tx_pool.urb[i])
				{
					usb_free_urb(priv->pshare->tx_pool.urb[i]);
					priv->pshare->tx_pool.urb[i]=NULL;
				}

				if(priv->pshare->tx_pool.pdescinfo[i])
				{
					kfree(priv->pshare->tx_pool.pdescinfo[i]);
					priv->pshare->tx_pool.pdescinfo[i]=NULL;
				}		
			}
		}
	}
}

void rtl8192su_rx_enable(struct rtl8190_priv *priv)
{
	struct rtl8190_hw	*hwdesc;
	hwdesc = GET_HW(priv);
	
	//priv->rx_inx=0;
	hwdesc->cur_rx=0;
	RTL_W32SU(priv, RCR, 0);
	delay_ms(10);

	if(priv->rx_urb==NULL)  //first time init
		rtl8192su_rx_initiate(priv);
	else
		rtl8192su_rx_reinitiate(priv);
	delay_ms(10);
}

void rtl8192su_rx_disable(struct rtl8190_priv *priv)
{

	struct rtl8190_hw	*hwdesc;
	hwdesc = GET_HW(priv);
	
	//priv->rx_inx=0;
	hwdesc->cur_rx=0;
	RTL_W32SU(priv, RCR, 0);
	delay_ms(10);

#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (IS_ROOT_INTERFACE(priv))
#endif
	if(priv->rx_urb)
	{
		int prx_inx;
//		int rx_schedule_index=priv->pshare->rx_schedule_index;
		struct urb *rx_urb;
//		struct sk_buff *pskb;

		for(prx_inx=0;prx_inx<MAX_RX_URB;prx_inx++)
		{
			rx_urb = priv->rx_urb[prx_inx];	
			usb_kill_urb(rx_urb);
//			printk("kill urb index=%d\n",prx_inx);
		}
	}
}

void validate_fixed_tx_rate(struct rtl8190_priv *priv)
{
	if (!priv->pmib->dot11StationConfigEntry.autoRate) {
		if (!(priv->pmib->dot11BssType.net_work_type & WIRELESS_11N))
			priv->pmib->dot11StationConfigEntry.fixedTxRate &= 0x0000fff;	// mask HT rates

		if((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
			((get_rf_mimo_mode(priv) == MIMO_1T2R) || (get_rf_mimo_mode(priv) == MIMO_1T1R)))
			priv->pmib->dot11StationConfigEntry.fixedTxRate &= 0x00fffff;	// mask MCS8 - MCS15

		if (!(priv->pmib->dot11BssType.net_work_type & WIRELESS_11A) &&
			!(priv->pmib->dot11BssType.net_work_type & WIRELESS_11G))
			priv->pmib->dot11StationConfigEntry.fixedTxRate &= 0xffff00f;	// mask OFDM rates

		if (!(priv->pmib->dot11BssType.net_work_type & WIRELESS_11B))
			priv->pmib->dot11StationConfigEntry.fixedTxRate &= 0xffffff0;	// mask CCK rates

		if (priv->pmib->dot11StationConfigEntry.fixedTxRate==0) {
			priv->pmib->dot11StationConfigEntry.autoRate=1;
			PRINT_INFO("invalid fixed tx rate, use auto rate!\n");
		}
	}
}

#ifdef LOOPBACK_MODE
int counter=0;
void loopback_test(struct rtl8190_priv *priv, struct net_device *dev)
{
	u16	pktSize=1546;
	struct usb_device *pusbd=priv->udev;
printk("%s %d\n",__FUNCTION__,__LINE__);


	uint pipe_hdl=usb_sndbulkpipe(pusbd,0x4/*VO_ENDPOINT*/);
	uint pipe_in_hdl=usb_rcvbulkpipe(pusbd,0x3);

	int i;

	for(i=0;i<MAX_LB_TX_URB;i++)
	{


		// 24(tx desc)+8(tx fm info)+24(wlan header)+1510(wlan header + data)
		txbuff[i]=kmalloc(1566,GFP_KERNEL);
		memset(txbuff[i],0,1566);
		
		
//---
		txbuff[i][0]=(pktSize-32)&0xff;
		txbuff[i][1]=((pktSize-32)>>8)&0xff;	
		txbuff[i][2]=0x20;
		txbuff[i][3]=0x8c;

		txbuff[i][8]=0x40; // disable retry


		txbuff[i][18]=0x03;		//enable TXHT, SHORT GI, TXBW=40
		
		txbuff[i][19]=0x80;		// USE RATE
//		txbuff[i][21]=0x96;		//OFDM 54
//		txbuff[i][21]=0x9e;		//MCS 3
//		txbuff[i][21]=0xa0;		//MCS 4
//		txbuff[i][21]=0xa2;		//MCS 5
//		txbuff[i][21]=0xa4;		//MCS 6
		txbuff[i][21]=0xa6;		//MCS 7 --> only for BB loopback
//		txbuff[i][21]=0xb6;		//MCS 15

#ifdef LOOPBACK_NORMAL_RX_MODE
		txbuff[i][32]=0x08;
		txbuff[i][33]=0x01; //To AP
		txbuff[i][34]=0;
		txbuff[i][35]=0;

		//BSSID
		txbuff[i][36]=priv->pmib->dot11StationConfigEntry.dot11Bssid[0];
		txbuff[i][37]=priv->pmib->dot11StationConfigEntry.dot11Bssid[1];
		txbuff[i][38]=priv->pmib->dot11StationConfigEntry.dot11Bssid[2];
		txbuff[i][39]=priv->pmib->dot11StationConfigEntry.dot11Bssid[3];
		txbuff[i][40]=priv->pmib->dot11StationConfigEntry.dot11Bssid[4];	
		txbuff[i][41]=priv->pmib->dot11StationConfigEntry.dot11Bssid[5];

		//SA
		txbuff[i][42]=0x00;
		txbuff[i][43]=0x11;
		txbuff[i][44]=0x22;
		txbuff[i][45]=0x33;
		txbuff[i][46]=0x44;
		txbuff[i][47]=0x55;

		//DA
		txbuff[i][48]=0xff;
		txbuff[i][49]=0xff;
		txbuff[i][50]=0xff;
		txbuff[i][51]=0xff;
		txbuff[i][52]=0xff;
		txbuff[i][53]=0xff;

#else
		txbuff[i][32]=0x08;
		txbuff[i][33]=0x02; //from AP
		txbuff[i][34]=0;
		txbuff[i][35]=0;
		
		txbuff[i][36]=0xff;
		txbuff[i][37]=0xff;
		txbuff[i][38]=0xff;
		txbuff[i][39]=0xff;
		txbuff[i][40]=0xff;		
		txbuff[i][41]=0xff;
		
		txbuff[i][42]=0x00;
		txbuff[i][43]=0x11;
		txbuff[i][44]=0x22;
		txbuff[i][45]=0x33;
		txbuff[i][46]=0x44;
		txbuff[i][47]=0x55;
		
		txbuff[i][48]=0x00;
		txbuff[i][49]=0x11;
		txbuff[i][50]=0x22;
		txbuff[i][51]=0x33;
		txbuff[i][52]=0x44;
		txbuff[i][53]=0x55;
#endif		

		tx_urb[i]=usb_alloc_urb(0,GFP_KERNEL);
		tx_own[i]=0;
		tx_info[i]=i;
	}

#ifdef LOOPBACK_NORMAL_RX_MODE
#else

	for(i=0;i<MAX_LB_RX_URB;i++)
	{

#ifdef PKT_PROCESSOR
		skb_pool[i]=prealloc_skb_get();
		if (skb_pool[i] != NULL)
		{
			skb_pool[i]->head=(unsigned char *)((u32)skb_pool[i]->head & (~0x20000000));
			skb_pool[i]->data=skb_pool[i]->head+RX_DMA_SHIFT;
			skb_pool[i]->tail=skb_pool[i]->data;
			skb_pool[i]->end=skb_pool[i]->head+RX_DMA_SHIFT+RX_ALLOC_SIZE;
			skb_pool[i]->fcpu=0;
			skb_pool[i]->pptx=NULL;
		}
		else
		{
			printk("skb pool empty.........\n");

		}
		privs[i].transfer_buffer=skb_pool[i]->data;
#else		
		rxbuff[i]=kmalloc(2048,GFP_KERNEL);
		privs[i].buffer_length=2048;	

		privs[i].transfer_buffer=rxbuff[i];
#endif

		rx_urb[i]=usb_alloc_urb(0,GFP_KERNEL);

		privs[i].dev=pusbd;
		privs[i].pipe=pipe_in_hdl;
		privs[i].netdev=dev;
		privs[i].index=i;

#ifdef PKT_PROCESSOR
		usb_fill_bulk_urb(rx_urb[i],
						pusbd,
						pipe_in_hdl, 
						skb_pool[i]->data,
						1600,
						(usb_complete_t)read_complete,
						&privs[i]);
#else
		usb_fill_bulk_urb(rx_urb[i],
						pusbd,
						pipe_in_hdl, 
						rxbuff[i],
						privs[i].buffer_length,
						(usb_complete_t)read_complete,
						&privs[i]); 
#endif
		usb_submit_urb(rx_urb[i], GFP_ATOMIC);
	}
#endif

#ifdef PKT_PROCESSOR
	//packet come from packet processor
#else
	// packet come from packet genarator 

// set TXOP and NAV protect length
	printk("0x238=%x 0x234=%x 0x235=%x\n",RTL_R8(0x238),RTL_R8(0x234),RTL_R8(0x235));
	RTL_W8(0x238,0);
	RTL_W8(0x234,0);
	RTL_W8(0x235,0);	
	printk("0x238=%x 0x234=%x 0x235=%x\n",RTL_R8(0x238),RTL_R8(0x234),RTL_R8(0x235));	


	// short GI for MCS7
	RTL_W16(0x1f6,0x7777); //MAX MCS Rate available --->MCS7
	RTL_W8(0x237,(10<<3)|RTL_R8(0x237));


	// 40Mhz Mode
	RTL_W8(0x203,0);	
//	PHY_SetBBReg(priv, rFPGA0_RFMOD, bRFMOD, 0x1); // for 40Mhz BW Mode


	while(1)
	{


//		if((tx_own[tx_idx]==0)&&(diff<MAX_LB_TX_URB-1))
#ifdef LOOPBACK_NORMAL_TX_MODE
#else
		if(tx_own[tx_idx]==0)
#endif			
		{

			
			counter++;
//			if((counter%100)==0)
				printk("counter=%d\n",counter);			

//			pipe_hdl=usb_sndbulkpipe(pusbd,0x4+((counter%2)<<1)/*VO_ENDPOINT*/);
//			pipe_hdl=usb_sndbulkpipe(pusbd,0x4+((counter%3))/*VO_ENDPOINT*/);
//			printk("submit urb=%x\n",txbuff[tx_idx][73]);

#ifdef LOOPBACK_NORMAL_TX_MODE
			struct sk_buff *pskb;
			pskb=dev_alloc_skb(2048);
			if(pskb==NULL) printk("error %s %d\n",__FUNCTION__,__LINE__);

			skb_reserve(pskb,128);

			//DA
			pskb->data[0]=0x00;
			pskb->data[1]=0x11;
			pskb->data[2]=0x22;
			pskb->data[3]=0x33;
			pskb->data[4]=0x44;
			pskb->data[5]=0x66;

			//SA
			pskb->data[6]=0x00;
			pskb->data[7]=0x11;
			pskb->data[8]=0x22;
			pskb->data[9]=0x33;
			pskb->data[10]=0x44;
			pskb->data[11]=0x55;

			//EtherType
			pskb->data[12]=0x08;
			pskb->data[13]=0x00;

			memset(&pskb->data[14],0xff,1514);

			pskb->len=1528;
			pskb->tail=pskb->len+pskb->data;
			rtl8190_start_xmit(pskb, dev);

			delay_ms(30);
		
#else
			txbuff[tx_idx][73]++;

			usb_fill_bulk_urb(tx_urb[tx_idx],
							pusbd,
							pipe_hdl, 
							txbuff[tx_idx],
							pktSize,
							(usb_complete_t)write_complete,
							&tx_info[tx_idx]);   //context is xmit_frame
							

			tx_own[tx_idx]=1;	
//			diff++;

//			if(tx_idx==MAX_LB_TX_URB-1)
//				tx_urb[tx_idx]->transfer_flags  &= (~URB_NO_INTERRUPT);
//			else
				tx_urb[tx_idx]->transfer_flags  |= URB_NO_INTERRUPT;


//			us_delay(5);  //sim normal 8187 tx time
			usb_submit_urb(tx_urb[tx_idx], GFP_ATOMIC);
//			printk("submit %d\n",tx_idx);

			tx_idx=(++tx_idx)%MAX_LB_TX_URB;
#endif

			if(counter>=5) break;
		}

	}
	printk("loopback finish\n");	
#endif
}
#endif

#ifdef TBS_MP
void initMPmib(struct rtl8190_priv *priv)
{
	unsigned char power[30]={0};
	unsigned char GetValue[512];
	unsigned short GetLen=sizeof(GetValue);
	unsigned int i;
		
	if(item_get(GetValue,TXPOWERCCK,&GetLen)==ERROR_ITEM_OK)
	{
		get_array_val(power, GetValue, strlen(GetValue));
		DEBUG_MP("[TXPOWERCCK]");
		for(i=0;i<14;i++)
		{
			DEBUG_MP("%02x ",power[i]);
			priv->pmib->dot11RFEntry.pwrlevelCCK[i]=power[i];
		}
		DEBUG_MP("\n");
	}
	else
		DEBUG_MP(":TXPOWERCCK item_get fail");

	if(item_get(GetValue,TXPOWEROFDM1S, &GetLen)==ERROR_ITEM_OK)
	{
		get_array_val(power, GetValue, strlen(GetValue));
		DEBUG_MP("[HW_TX_POWER_OFDM_1S]");
		for(i=0;i<28;i++)
		{
			DEBUG_MP("%02x ",power[i]);
			priv->pmib->dot11RFEntry.pwrlevelOFDM_1SS[i]=power[i];
		}
		DEBUG_MP("\n");
	}
	else
		DEBUG_MP(":HW_TX_POWER_OFDM_1S item_get fail");
	
	if(item_get(GetValue,TXPOWEROFDM2S, &GetLen)==ERROR_ITEM_OK)
	{
		get_array_val(power, GetValue, strlen(GetValue));
		DEBUG_MP("[HW_TX_POWER_OFDM_2S]");
		for(i=0;i<28;i++)
		{
			DEBUG_MP("%02x ",power[i]);
			priv->pmib->dot11RFEntry.pwrlevelOFDM_2SS[i]=power[i];
		}
		DEBUG_MP("\n");
	}
	else
		DEBUG_MP(":TXPOWEROFDM2S item_get fail");
	
	if(item_get(GetValue, LOFDMPWDA , &GetLen)==ERROR_ITEM_OK)
	{
		get_array_val(power, GetValue, strlen(GetValue));
		DEBUG_MP("[HW_11N_LOFDMPWDA]=%02x %02x",power[0],power[1]);
		priv->pmib->dot11RFEntry.LOFDM_pwd_A=power[0];
		priv->pmib->dot11RFEntry.LOFDM_pwd_B=power[1];
		DEBUG_MP("\n");
	}
	else
		DEBUG_MP(":LOFDMPWDA item_get fail");
	priv->pmib->dot11RFEntry.tssi1=12;
	priv->pmib->dot11RFEntry.tssi2=12;
	priv->pmib->dot11RFEntry.ther=16;
}

#endif
int rtl8190_open(struct net_device *dev)
{
	struct rtl8190_priv *priv = dev->priv;	// recuresively used, can't be static
	static int rc, i;
	static unsigned long x;
	unsigned char *macAddr = GET_MY_HWADDR;
#ifdef CONFIG_RTL865X_WTDOG
#ifndef CONFIG_RTL8196B
	static unsigned long wtval;
#endif
#endif
#ifdef CHECK_HANGUP
	static int is_reset;
#endif

	static int init_hw_cnt;
	init_hw_cnt = 0;

	struct wifi_mib	*pmib;

	DBFENTER;

	priv = dev->priv;
#ifdef CHECK_HANGUP
	is_reset = priv->reset_hangup;
#endif

	/* 增加产测MAC地址写入 -chenzanhui*/
	tbs_read_mac( WLAN_MAC, priv->vap_id+1, macAddr);
	pmib = GET_MIB(priv);
#ifdef MBSSID
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
		RTL_W16(CMDR, 0x0000); //disable Base Band
#ifdef PKT_PROCESSOR
		if(dev->name[4]=='0')
		{
			dev->vlanid=VLANID;
			set_netdev_for8672_tx(CONFIG_RTL8672_WLAN_PORT, dev);  /* use port 15 */
		}
#endif
#if defined(LOOPBACK_MODE) && !defined(LOOPBACK_NORMAL_RX_MODE)
#else
		rtl8192su_rx_enable(priv);
	}
#endif

	
#ifdef WDS
	if (dev->base_addr == 0)
	{
#ifdef CONFIG_RTL865X
		{
			int index = dev->name[strlen(dev->name)-1]-'0';
			int _retval = 0;

			index++;
			if ((_retval = rtl865x_wlanRegistration(dev, index /* WDS interface */)) != 0) {
				DEBUG_ERR("865x flatform registration failed!\n");
				return _retval;
			}
		}
#endif

#ifdef BR_SHORTCUT
		extern struct net_device *cached_wds_dev;
		cached_wds_dev = NULL;
#endif

		netif_start_queue(dev);
		return 0;
	}
#endif

#ifdef CONFIG_RTK_MESH
	if (dev->base_addr == 1) {
#ifdef CONFIG_RTL865X
		{
			int _retval = 0;

			if ((_retval = rtl865x_wlanRegistration(dev, 1)) != 0) {
				DEBUG_ERR("865x flatform registration failed!\n");
				return _retval;
			}
		}
#ifdef BR_SHORTCUT
		extern struct net_device *cached_mesh_dev;
		cached_mesh_dev	= NULL;
#endif

#endif // CONFIG_RTL865X
		netif_start_queue(dev);
		return 0;
	}
#endif // CONFIG_RTK_MESH


	// stop h/w in the very beginning
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
#ifdef HW_QUICK_INIT
		if (priv->pshare->hw_inited == 0)
#endif
			rtl819x_stop_hw(priv, 0);
	}

#ifdef UNIVERSAL_REPEATER
	// If vxd interface, see if some mandatory mib is set. If ok, backup these
	// mib, and copy all mib from root interface. Then, restore the backup mib
	// to current.

	if (IS_VXD_INTERFACE(priv)) {
		DEBUG_INFO("Open request from vxd\n");
		if (!IS_DRV_OPEN(GET_ROOT_PRIV(priv))) {
			printk("Open vxd error! Root interface should be opened in advanced.\n");
			return 0;
		}

		if (!(priv->drv_state & DRV_STATE_VXD_INIT)) {
// Mark following code. MIB copy will be executed through ioctl -------------
//---------------------------------------------------------- david+2008-03-17

			// correct RSN IE will be set later for WPA/WPA2
#ifdef CHECK_HANGUP
			if (!is_reset)
#endif
				memset(&priv->pmib->dot11RsnIE, 0, sizeof(struct Dot11RsnIE));

#ifdef WDS
			// always disable wds in vxd
			priv->pmib->dot11WdsInfo.wdsEnabled = 0;
			priv->pmib->dot11WdsInfo.wdsPure = 0;
#endif

#ifdef CONFIG_RTK_MESH
			// always disable mesh in vxd (for dev)
			GET_MIB(priv)->dot1180211sInfo.mesh_enable = 0;
#endif // CONFIG_RTK_MESH


			priv->drv_state |= DRV_STATE_VXD_INIT;	// indicate the mib of vxd driver has been initialized
		}
	}
#endif // UNIVERSAL_REPEATER

#ifdef CHECK_HANGUP
	if (!is_reset)
#endif
	{
		if (OPMODE & WIFI_AP_STATE)
			OPMODE = WIFI_AP_STATE;
#ifdef CLIENT_MODE
		else if (OPMODE & WIFI_STATION_STATE) {
			OPMODE = WIFI_STATION_STATE;
#if defined(SEMI_QOS) && defined(WMM_APSD)
			APSD_ENABLE = 0;
#endif
		}
		else if (OPMODE & WIFI_ADHOC_STATE) {
			OPMODE = WIFI_ADHOC_STATE;
#ifdef SEMI_QOS
#ifdef WMM_APSD
			APSD_ENABLE = 0;
#endif
#endif
		}
#endif
		else {
			printk("Undefined state... using AP mode as default\n");
			OPMODE = WIFI_AP_STATE;
		}
	}

#ifdef MBSSID
	if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
	{
		if (IS_VAP_INTERFACE(priv)) {
			if (!IS_DRV_OPEN(GET_ROOT_PRIV(priv))) {
				printk("Open vap error! Root interface should be opened in advanced.\n");
				return -1;
			}

			if ((GET_ROOT_PRIV(priv)->pmib->dot11OperationEntry.opmode & WIFI_AP_STATE) == 0) {
				printk("Fail to open VAP under non-AP mode!\n");
				return -1;
			}
			else {
#ifdef RTL8192SU_SW_BEACON
				do 
				{
					if (GET_ROOT_PRIV(priv)->pmib->dot11OperationEntry.opmode & WIFI_WAIT_FOR_CHANNEL_SELECT)
					{
						DEBUG_INFO("wait for root interface ss_timer!!\n");
						delay_ms(1);
					}
					else
					{
						DEBUG_INFO("channel=%x\n", GET_ROOT_PRIV(priv)->pmib->dot11RFEntry.dot11channel);
						break;
					}
				}while(1);
#endif			
				rtl8190_init_vap_mib(priv);
			}
		}
	}
#endif

	rc = rtl8190_init_sw(priv);
    if (rc)
        return rc;

#if defined(CONFIG_RTL8196B_TR)
	if (!priv->auto_channel) {
		LOG_START_MSG();
	}
#endif
//#ifdef CONFIG_RTL865X_AC
#if defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
	if (!priv->auto_channel) {
		LOG_START_MSG();
	}
#endif

#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
#ifdef CHECK_HANGUP
		if (!is_reset)
#endif
		{
#ifdef HW_QUICK_INIT
			if (priv->pshare->hw_inited == 0)
#endif
			{
#ifdef CONFIG_RTL865X
				printk("Request IRQ%d, ret=%d\n", dev->irq, rc);
#endif
				if (rc) {
					DEBUG_ERR("some issue in request_irq, rc=%d\n", rc);
				}
			}
		}


#ifdef CONFIG_RTL865X_WTDOG
#ifndef CONFIG_RTL8196B
		wtval = *((volatile unsigned long *)0xB800311C);
		*((volatile unsigned long *)0xB800311C) = 0xA5f00000;
#endif
#endif

do_hw_init:	

#ifdef HW_QUICK_INIT
		if (priv->pshare->hw_inited == 0) {
			rc = rtl819x_init_hw_PCI(priv);
			priv->pshare->hw_inited = 1;
		}
		else {
			rc = 0;
			rtl819x_start_hw_trx(priv);
		}
#else
		rc = rtl819x_init_hw_PCI(priv);
		if (rc==LOADFW_FAIL) //Firmware Download Fail
		{
			printk("[%s, %d]Firmware Download Fail\n", __FUNCTION__, __LINE__);
			rtl819x_stop_hw(priv, 0);
			goto do_hw_init;
		}
#endif

		// write IDR0, IDR4 here
		{
			unsigned long reg = 0;
			reg = *(unsigned long *)(dev->dev_addr);
			RTL_W32(IDR0, (cpu_to_le32(reg)));
			reg = *(unsigned long *)((unsigned long)dev->dev_addr + 4);
			RTL_W32(IDR4, (cpu_to_le32(reg)));
		}

		if (rc && ++init_hw_cnt < 5) {
#ifdef CONFIG_RTL865X_WTDOG
			*((volatile unsigned long *)0xB800311C) |=  1 << 23;
#endif			
			goto do_hw_init;			
		}

#ifdef CONFIG_RTL865X_WTDOG
#ifndef CONFIG_RTL8196B
		*((volatile unsigned long *)0xB800311C) |=  1 << 23;
		*((volatile unsigned long *)0xB800311C) = wtval;
#endif
#endif


		if (rc) {
			DEBUG_ERR("init hw failed!\n");
			force_stop_wlan_hw();
			cli();
			*(volatile unsigned long *)(0xB800311c) = 0; /* enable watchdog reset now */		
			for(;;);
			return rc;
		}
	}
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	else {
		if (get_rf_mimo_mode(priv) == MIMO_1T1R)
			GET_MIB(priv)->dot11nConfigEntry.dot11nSupportedMCS &= 0x00ff;
	}
#endif

	validate_fixed_tx_rate(priv);

#ifdef MBSSID
	if (OPMODE & WIFI_AP_STATE) {
		if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
			rtl8190_init_mbssid(priv);
		if (GET_ROOT(priv)->pmib->miscEntry.vap_enable) {
			// set BcnDmaInt & BcnOk of different VAP in IMR
			if (IS_VAP_INTERFACE(priv)) {
				priv->pshare->InterruptMask |= (IMR_BCNDOK1 << (priv->vap_init_seq-1)) |
					(IMR_BCNDMAINT1 << (priv->vap_init_seq-1));
			}
		}
	}
	if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
	{
		if (IS_VAP_INTERFACE(priv) && (GET_ROOT_PRIV(priv)->auto_channel == 0))
			init_beacon(priv);
	}
#endif

// new added to reset keep_rsnie flag
	if (priv->pmib->dot11OperationEntry.keep_rsnie)
		priv->pmib->dot11OperationEntry.keep_rsnie = 0;
//------------------- david+2006-06-30

	priv->drv_state |= DRV_STATE_OPEN;      // set driver as has been opened, david

	memcpy((void *)dev->dev_addr, priv->pmib->dot11OperationEntry.hwaddr, 6);

#ifdef RTL8192SU_SW_BEACON  //for SW beacon test, chihhsing 20081105
	init_timer(&priv->beacon_timer);
	priv->beacon_timer.data = (unsigned long) priv;
	priv->beacon_timer.function = rtk8192su_beacon_time;
	if (timer_pending(&priv->beacon_timer))
			del_timer_sync(&priv->beacon_timer);
#endif	

	// below is for site_survey timer
	init_timer(&priv->ss_timer);
	priv->ss_timer.data = (unsigned long) priv;
	priv->ss_timer.function = rtl8190_ss_timer;

	// bcm old 11n chipset iot debug

#ifdef CLIENT_MODE
	init_timer(&priv->reauth_timer);
	priv->reauth_timer.data = (unsigned long) priv;
	priv->reauth_timer.function = rtl8190_reauth_timer;

	init_timer(&priv->reassoc_timer);
	priv->reassoc_timer.data = (unsigned long) priv;
	priv->reassoc_timer.function = rtl8190_reassoc_timer;

	init_timer(&priv->idle_timer);
	priv->idle_timer.data = (unsigned long) priv;
	priv->idle_timer.function = rtl8190_idle_timer;
#endif

	priv->frag_to = 0;
	init_timer(&priv->frag_to_filter);
	priv->frag_to_filter.expires = jiffies + FRAG_TO;
	priv->frag_to_filter.data = (unsigned long) priv;
	priv->frag_to_filter.function = rtl8190_frag_timer;
	mod_timer(&priv->frag_to_filter, jiffies + FRAG_TO);

	priv->auth_to = AUTH_TO / 100;
	priv->assoc_to = ASSOC_TO / 100;
	priv->expire_to = (EXPIRETIME > 100)? (EXPIRETIME / 100) : 86400;

#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
		init_timer(&priv->expire_timer);
		priv->expire_timer.expires = jiffies + EXPIRE_TO;
		priv->expire_timer.data = (unsigned long) priv;
		priv->expire_timer.function = rtl8190_1sec_timer;
		mod_timer(&priv->expire_timer, jiffies + EXPIRE_TO);

		init_timer(&priv->pshare->rc_sys_timer);
		priv->pshare->rc_sys_timer.data = (unsigned long) priv;
		priv->pshare->rc_sys_timer.function = reorder_ctrl_timeout;

		init_timer(&priv->pshare->phw->tpt_timer);
		priv->pshare->phw->tpt_timer.data = (unsigned long)priv;
		priv->pshare->phw->tpt_timer.function = rtl8190_tpt_timer;
	}

	// for MIC check
	init_timer(&priv->MIC_check_timer);
	priv->MIC_check_timer.data = (unsigned long) priv;
	priv->MIC_check_timer.function = DOT11_Process_MIC_Timerup;
	init_timer(&priv->assoc_reject_timer);
	priv->assoc_reject_timer.data = (unsigned long) priv;
	priv->assoc_reject_timer.function = DOT11_Process_Reject_Assoc_Timerup;
	priv->MIC_timer_on = FALSE;
	priv->assoc_reject_on = FALSE;

#ifdef GBWC
	init_timer(&priv->GBWC_timer);
	priv->GBWC_timer.data = (unsigned long) priv;
	priv->GBWC_timer.function = rtl8190_GBWC_timer;
	mod_timer(&priv->GBWC_timer, jiffies + GBWC_TO);
#endif

	// to avoid add RAtid fail
	init_timer(&priv->add_RATid_timer);
	priv->add_RATid_timer.data = (unsigned long) priv;
	priv->add_RATid_timer.function = rtl8190_add_RATid_timer;

#ifdef CONFIG_RTK_MESH
	/*
	 * CAUTION !! These statement meshX(virtual interface) ONLY, Maybe modify....
	 * These statment is initial information, (If "ZERO" no need set it, because all cleared to ZERO)
	 */
	init_timer(&priv->mesh_peer_link_timer);
	priv->mesh_peer_link_timer.data = (unsigned long) priv;
	priv->mesh_peer_link_timer.function = mesh_peer_link_timer;

#ifdef MESH_BOOTSEQ_AUTH
	init_timer(&priv->mesh_auth_timer);
	priv->mesh_auth_timer.data = (unsigned long) priv;
	priv->mesh_auth_timer.function = mesh_auth_timer;
#endif

	priv->mesh_Version = 1;
#ifdef	MESH_ESTABLISH_RSSI_THRESHOLD
	priv->mesh_fake_mib.establish_rssi_threshold = DEFAULT_ESTABLISH_RSSI_THRESHOLD;
#endif

#ifdef MESH_USE_METRICOP
	// in next version, the fake_mib related values will be actually recorded in MIB
	priv->mesh_fake_mib.metricID = 1; // 0: very old version,  1: version before 2009/3/10,  2: 11s
	priv->mesh_fake_mib.isPure11s = 0;
	priv->mesh_fake_mib.intervalMetricAuto = 60 * HZ; // 1 Mins
	priv->mesh_fake_mib.spec11kv.defPktTO = 20; // 2 * 100 = 2 secs
	priv->mesh_fake_mib.spec11kv.defPktLen = 1024; // bt=8196 bits
	priv->mesh_fake_mib.spec11kv.defPktCnt = 2;
	priv->mesh_fake_mib.spec11kv.defPktPri = 5;

	// once driver starts up, toMeshMetricAuto will be updated
	// (I think it might be put in "init one" to match our original concept...)
	priv->toMeshMetricAuto = jiffies + priv->mesh_fake_mib.intervalMetricAuto;
#endif // MESH_USE_METRICOP

	mesh_set_PeerLink_CAP(priv, GET_MIB(priv)->dot1180211sInfo.mesh_max_neightbor);

	priv->mesh_PeerCAP_flags = 0x80;		// Bit15(Operation as MP) shall be "1"
	priv->mesh_HeaderFlags = 0;				// NO Address Extension

	// The following info can be saved by FLASH in the future
	priv->mesh_profile[0].used = TRUE;
	priv->mesh_profile[0].PathSelectMetricID.value = 0;
	priv->mesh_profile[0].PathSelectMetricID.OUI[0] = 0x00;
	priv->mesh_profile[0].PathSelectMetricID.OUI[1] = 0x0f;
	priv->mesh_profile[0].PathSelectMetricID.OUI[2] = 0xac;
	priv->mesh_profile[0].PathSelectProtocolID.value = 0;
	priv->mesh_profile[0].PathSelectProtocolID.OUI[0] = 0x00;
	priv->mesh_profile[0].PathSelectProtocolID.OUI[1] = 0x0f;
	priv->mesh_profile[0].PathSelectProtocolID.OUI[2] = 0xac;
#endif


#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
		// for HW/SW LED
		if(priv->pshare->LED_sw_use)
			disable_sw_LED(priv);
		if ((LED_TYPE >= LEDTYPE_HW_TX_RX) && (LED_TYPE <= LEDTYPE_HW_LINKACT_INFRA))
			enable_hw_LED(priv, LED_TYPE);
		else if ((LED_TYPE >= LEDTYPE_SW_LINK_TXRX) && (LED_TYPE < LEDTYPE_SW_MAX))
			enable_sw_LED(priv, 1);

#ifdef DFS
		if (!priv->pmib->dot11DFSEntry.disable_DFS &&
			(OPMODE & WIFI_AP_STATE) &&
			(priv->pmib->dot11RFEntry.dot11channel >= 52)) {
			init_timer(&priv->ch_avail_chk_timer);
			priv->ch_avail_chk_timer.data = (unsigned long) priv;
			priv->ch_avail_chk_timer.function = rtl8190_ch_avail_chk_timer;
			mod_timer(&priv->ch_avail_chk_timer, jiffies + CH_AVAIL_CHK_TO);

			init_timer(&priv->DFS_timer);
			priv->DFS_timer.data = (unsigned long) priv;
			priv->DFS_timer.function = rtl8190_DFS_timer;
			mod_timer(&priv->DFS_timer, jiffies + 500); // DFS activated after 5 sec; prevent switching channel due to DFS false alarm
		}
#endif

#ifdef SUPPORT_SNMP_MIB
		mib_init(priv);
#endif

	}

#if defined(BR_SHORTCUT) && defined(CLIENT_MODE)
	if (OPMODE & WIFI_STATION_STATE) {
		extern struct net_device *cached_sta_dev;
		cached_sta_dev = NULL;
	}
#endif

#if	defined(BR_SHORTCUT)&&defined(RTL_CACHED_BR_STA)
	{
		extern unsigned char cached_br_sta_mac[MACADDRLEN];
		extern struct net_device *cached_br_sta_dev;
		memset(cached_br_sta_mac, 0, MACADDRLEN);
		cached_br_sta_dev = NULL;
	}
#endif

	//if (OPMODE & WIFI_AP_STATE)  //in case of station mode, queue will start only after assoc.
		if(!netif_queue_stopped(dev))
			netif_start_queue(dev);
		else
			netif_wake_queue(dev);

#ifdef CONFIG_RTL865X
	if (rtl865x_wlanRegistration(dev, 0 /* Non-WDS interface*/) != 0) {
		DEBUG_ERR("865x flatform registration failed!\n");
		goto err_out_hw;
	}
#endif

#ifdef WDS
	create_wds_tbl(priv);
#endif

#ifdef CHECK_HANGUP
	if (priv->reset_hangup)
		priv->reset_hangup = 0;
#endif

#if defined(INCLUDE_WPA_PSK) && defined(CLIENT_MODE)
	if (OPMODE & WIFI_ADHOC_STATE)
		if (priv->pmib->dot1180211AuthEntry.dot11EnablePSK)
			ToDrv_SetGTK(priv);
#endif

	check_and_set_ampdu_spacing(priv, NULL);

#ifdef CONFIG_RTL865X
err_out_hw:
#endif

#ifdef MBSSID
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
		if ((OPMODE & WIFI_AP_STATE) && priv->auto_channel) {
#ifdef RTL8192SU_SW_BEACON
			OPMODE |= WIFI_WAIT_FOR_CHANNEL_SELECT;
#endif
			if (((priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _TKIP_PRIVACY_) &&
				  (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _CCMP_PRIVACY_) &&
				  (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _WEP_WPA_MIXED_PRIVACY_)) ||
				 (priv->pmib->dot11RsnIE.rsnielen > 0)) {
				priv->ss_ssidlen = 0;
				DEBUG_INFO("start_clnt_ss, trigger by %s, ss_ssidlen=0\n", (char *)__FUNCTION__);
				start_clnt_ss(priv);
			}
		}
#ifdef RTL8192SU_SW_BEACON
		else
			mod_timer(&priv->beacon_timer, jiffies+pmib->dot11StationConfigEntry.dot11BeaconPeriod/10);
#endif
	}
#if defined(RTL8192SU) && defined(MBSSID)
	else if (GET_ROOT(priv)->auto_channel == 0)
		mod_timer(&priv->beacon_timer, jiffies+pmib->dot11StationConfigEntry.dot11BeaconPeriod/10);
#endif

#ifdef CLIENT_MODE
	if (OPMODE & (WIFI_STATION_STATE | WIFI_ADHOC_STATE)) {
#ifdef RTK_BR_EXT
		if (priv->dev->br_port) {
			memcpy(priv->br_mac, priv->dev->br_port->br->dev->dev_addr, MACADDRLEN);
		}
#endif

		if (!IEEE8021X_FUN || (IEEE8021X_FUN && (priv->pmib->dot11RsnIE.rsnielen > 0))) {
#ifdef CHECK_HANGUP
			if (!is_reset || priv->join_res == STATE_Sta_No_Bss ||
					priv->join_res == STATE_Sta_Roaming_Scan || priv->join_res == 0)
#endif
			{
#ifdef CHECK_HANGUP
				if (is_reset)
					OPMODE &= ~WIFI_SITE_MONITOR;
#endif
				start_clnt_lookup(priv, 1);
			}
		}
	}
#endif

#ifdef UNIVERSAL_REPEATER
	if (IS_ROOT_INTERFACE(priv) &&
		netif_running(GET_VXD_PRIV(priv)->dev)) {
		SAVE_INT_AND_CLI(x);
		rtl8190_open(GET_VXD_PRIV(priv)->dev);
		RESTORE_INT(x);
	}
	if (IS_VXD_INTERFACE(priv) &&
		(GET_ROOT_PRIV(priv)->pmib->dot11OperationEntry.opmode&WIFI_STATION_STATE) &&
		(GET_ROOT_PRIV(priv)->pmib->dot11OperationEntry.opmode&WIFI_ASOC_STATE) &&
#ifdef RTK_BR_EXT
		!(GET_ROOT_PRIV(priv)->pmib->ethBrExtInfo.macclone_enable && !priv->macclone_completed) &&
#endif
		!(priv->drv_state & DRV_STATE_VXD_AP_STARTED) )
		enable_vxd_ap(priv);
#endif

#ifdef MBSSID
	if (IS_ROOT_INTERFACE(priv)) {
		if (priv->pmib->miscEntry.vap_enable)
		{
			for (i=0; i<RTL8190_NUM_VWLAN; i++) {
				if (netif_running(priv->pvap_priv[i]->dev))
					rtl8190_open(priv->pvap_priv[i]->dev);
			}
		}
	}
#endif

#ifdef RTL8192SU_FWBCN
#ifdef MBSSID
	if (IS_ROOT_INTERFACE(priv))
	{
		priv->pshare->root_bcn=0;
		if (priv->pmib->miscEntry.vap_enable)
		{
			int count;
			priv->pshare->max_vapid=0;
			priv->pshare->intf_map=priv->pmib->miscEntry.intf_map;
			for (count=4;count>1;count--)
			{
				if (((priv->pshare->intf_map)>>count)==0x01)
				{
					priv->pshare->max_vapid=count-1;
					break;
				}
			}
		}
		else
		{
			priv->pshare->intf_map=0x01;
			priv->pmib->miscEntry.intf_map=0x01;
		}
	}
#else // vap_enable==0
	{
		priv->pshare->intf_map=0x01;
		priv->pmib->miscEntry.intf_map=0x01;
	}
#endif
#else
#ifdef MBSSID
	if (IS_ROOT_INTERFACE(priv))
#endif	
		priv->pshare->root_bcn=0;
#endif

#ifdef LOOPBACK_MODE //tysu
loopback_loop:
	loopback_test(priv, dev);
#endif

#ifdef RTL8192SU_CHECKTXHANGUP
#ifdef MBSSID
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
		send_probereq(priv);
		delay_ms(100);
		if (priv->pshare->init_success==0)
		{
			if (RTL_R16(0x366)<=priv->pshare->txpkt_count)
			{
				DEBUG_ERR("Init hw failed!\n");
				rtl8190_close(priv->dev);
				rtl8190_open(priv->dev);
			}
		}
	}
#endif

	DBFEXIT;
	return 0;
}


int  rtl8190_set_hwaddr(struct net_device *dev, void *addr)
{
	unsigned long flags;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	unsigned long reg;
	unsigned char *p;
#ifdef WDS
	int i;
#endif

	p = ((struct sockaddr *)addr)->sa_data;

	SAVE_INT_AND_CLI(flags);

	memcpy(priv->dev->dev_addr, p, 6);
	memcpy(GET_MY_HWADDR, p, 6);

#ifdef MBSSID
	if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
	{
		if (IS_VAP_INTERFACE(priv)) {
			RESTORE_INT(flags);
			return 0;
		}
	}
#endif

#ifdef WDS
	for (i=0; i<NUM_WDS; i++)
		if (priv->wds_dev[i])
			memcpy(priv->wds_dev[i]->dev_addr, p, 6);
#endif
#ifdef CONFIG_RTK_MESH
	if(NUM_MESH>0)
		if (priv->mesh_dev)
			memcpy(priv->mesh_dev->dev_addr, p, 6);
#endif

#ifdef UNIVERSAL_REPEATER
	if (IS_ROOT_INTERFACE(priv)) {
		if (GET_VXD_PRIV(priv)) {
			memcpy(GET_VXD_PRIV(priv)->dev->dev_addr, p, 6);
			memcpy(GET_VXD_PRIV(priv)->pmib->dot11OperationEntry.hwaddr, p, 6);
		}
	}
	else if (IS_VXD_INTERFACE(priv)) {
		memcpy(GET_ROOT_PRIV(priv)->dev->dev_addr, p, 6);
		memcpy(GET_ROOT_PRIV(priv)->pmib->dot11OperationEntry.hwaddr, p, 6);
	}
#endif

	reg = *(unsigned long *)(dev->dev_addr);
	RTL_W32(IDR0, (cpu_to_le32(reg)));
	reg = *(unsigned long *)((unsigned long)dev->dev_addr + 4);
	RTL_W32(IDR4, (cpu_to_le32(reg)));
#ifdef MBSSID
	if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
	{
		if (OPMODE & WIFI_AP_STATE)
			rtl8190_init_mbssid(priv);
	}
#endif

	RESTORE_INT(flags);

#ifdef CLIENT_MODE
	if (!(OPMODE & WIFI_AP_STATE) && netif_running(priv->dev)) {
		int link_status = chklink_wkstaQ(priv);
		if (link_status)
			start_clnt_join(priv);
	}
#endif

#ifdef PKT_PROCESSOR
#ifdef MBSSID
	if (IS_ROOT_INTERFACE(priv))
#endif
		set_extDevMac(CONFIG_RTL8672_WLAN_PORT,	p);
#endif //PKT_PROCESSOR

	return 0;
}


int rtl8190_close(struct net_device *dev)
{
    struct rtl8190_priv *priv = dev->priv;
#ifdef UNIVERSAL_REPEATER
	struct rtl8190_priv *priv_vxd;
#endif

	DBFENTER;

	if (!(priv->drv_state & DRV_STATE_OPEN)) {
		DBFEXIT;
		return 0;
	}

	priv->drv_state &= ~DRV_STATE_OPEN;     // set driver as has been closed, david
	printk("intf down.................\n");

	// receive all tx isr
	usb_receive_all_pkts(priv);

	printk("after recycle all tx pool.................\n");	

#ifdef UNIVERSAL_REPEATER
	if (IS_ROOT_INTERFACE(priv)) {
		priv_vxd = GET_VXD_PRIV(priv);

		// if vxd interface is opened, close it first
		if (IS_DRV_OPEN(priv_vxd))
			rtl8190_close(priv_vxd->dev);
	}
	else {
#ifdef MBSSID
/*
#if defined(RTL8192SE) || defined(RTL8192SU)
		if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
#endif
*/
		if (priv->vap_id < 0)
#endif
		disable_vxd_ap(priv);
	}
#endif

    netif_stop_queue(dev);

#ifdef WDS
	if (dev->base_addr == 0)
	{
#ifdef CONFIG_RTL865X
		{
			int index = dev->name[strlen(dev->name)-1]-'0';
			int _retval = 0;

			index++;
			if ((_retval = rtl865x_wlanUnregistration(dev, index /* WDS interface */)) != 0) {
				DEBUG_ERR("865x flatform registration failed!\n");
				DBFEXIT;
				return _retval;
			}
		}
#endif
		DBFEXIT;
		return 0;
	}
#endif

#ifdef CONFIG_RTK_MESH
	if (dev->base_addr == 1)
	{
#ifdef CONFIG_RTL865X
		{
			int _retval = 0;

			if ((_retval = rtl865x_wlanUnregistration(dev, 1)) != 0) {
				DEBUG_ERR("865x flatform registration failed!\n");
				return _retval;
			}
		}
#endif
		return 0;
	}
#endif // CONFIG_RTK_MESH

#ifdef CONFIG_RTL865X
	{
		int index = dev->name[strlen(dev->name)-1]-'0';
		int _retval = 0;

		index++;
		if ((_retval = rtl865x_wlanUnregistration(dev, 0 /* non-WDS interface */)) != 0) {
			DEBUG_ERR("865x flatform registration failed!\n");
			DBFEXIT;
			return _retval;
		}
	}
#endif

#ifdef CHECK_HANGUP
	if (!priv->reset_hangup)
#endif
	if (OPMODE & WIFI_AP_STATE) {
		int i;
		for(i=0; i<NUM_STAT; i++)
		{
			if (priv->pshare->aidarray[i] && (priv->pshare->aidarray[i]->used == TRUE)
#ifdef WDS
				&& !(priv->pshare->aidarray[i]->station.state & WIFI_WDS)
#endif
			) {
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
				if (priv != priv->pshare->aidarray[i]->priv)
					continue;
#endif
				issue_deauth(priv, priv->pshare->aidarray[i]->station.hwaddr, _RSON_DEAUTH_STA_LEAVING_);
			}
		}

		delay_ms(10);
	}

#ifdef MBSSID
	if (IS_ROOT_INTERFACE(priv)) {
		int i;
#ifdef RTL8192SU_FWBCN
		set_fw_reg(priv, 0xf1000003, 0, 0);
#endif
		if (priv->pmib->miscEntry.vap_enable)
		{
			for (i=0; i<RTL8190_NUM_VWLAN; i++) {
				if (IS_DRV_OPEN(priv->pvap_priv[i]))
					rtl8190_close(priv->pvap_priv[i]->dev);
			}
		}
	}

	if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
		rtl8190_stop_mbssid(priv);
#endif

#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
#ifdef RTK_QUE
		free_rtk_queue(priv, &priv->pshare->skb_queue);
#else
		free_skb_queue(priv, &priv->pshare->skb_queue);
#endif

#ifdef HW_QUICK_INIT
		if (priv->pshare->hw_inited == 1) {
			if (priv->up_time > 60) {
				priv->pshare->hw_inited = 0;
				priv->pshare->hw_init_num = 0;
			}
			else {
				priv->pshare->hw_init_num++;
				if (priv->pshare->hw_init_num < 5)
					priv->pshare->hw_inited = 0;
			}
		}
		priv->pshare->last_reinit = priv->pshare->hw_inited;

		if (priv->pshare->hw_inited == 0)
			rtl819x_stop_hw(priv, 1);
		else
			rtl819x_stop_hw_trx(priv);
#else
		rtl819x_stop_hw(priv, 1);
#endif

#ifdef HW_QUICK_INIT
		if (priv->pshare->hw_inited == 0)
#endif
		{
#ifdef CHECK_HANGUP
			if (!priv->reset_hangup)
#endif
				free_irq(dev->irq, dev);
		}

#ifdef UNIVERSAL_REPEATER
		GET_VXD_PRIV(priv)->drv_state &= ~DRV_STATE_VXD_INIT;
#endif
	}

	rtl8190_stop_sw(priv);

#ifdef ENABLE_RTL_SKB_STATS
	DEBUG_INFO("skb_tx_cnt =%d\n", rtl_atomic_read(&priv->rtl_tx_skb_cnt));
	DEBUG_INFO("skb_rx_cnt =%d\n", rtl_atomic_read(&priv->rtl_rx_skb_cnt));
#endif

	rtl8192su_rx_disable(priv);

	DBFEXIT;
    return 0;
}


#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
__NOMIPS16
#endif
static void MDL_DEVINIT set_mib_default(struct rtl8190_priv *priv)
{
	unsigned char *p;
	unsigned char wlanmac[MACADDRLEN];
	struct sockaddr addr;
	p = addr.sa_data;

	priv->pmib->mib_version = MIB_VERSION;
	set_mib_default_tbl(priv);

	// others that are not types of byte and int
	strcpy(priv->pmib->dot11StationConfigEntry.dot11DesiredSSID, "RTL8192su-Porting");
	priv->pmib->dot11StationConfigEntry.dot11DesiredSSIDLen = strlen("RTL8192su-Porting");
	tbs_read_mac( WLAN_MAC, 0, wlanmac);
 
	memcpy(p, wlanmac, MACADDRLEN);
	rtl8190_set_hwaddr(priv->dev, (void *)&addr);

#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (IS_ROOT_INTERFACE(priv))  // is root interface
#endif
	{
#ifdef RTL8192SU_EFUSE
		ReadAdapterInfo8192S(priv);
#else
#endif //RTL8192SU_EFUSE

#ifdef DFS
		init_timer(&priv->ch52_timer);
		priv->ch52_timer.data = (unsigned long) priv;
		priv->ch52_timer.function = rtl8190_ch52_timer;

		init_timer(&priv->ch56_timer);
		priv->ch56_timer.data = (unsigned long) priv;
		priv->ch56_timer.function = rtl8190_ch56_timer;

		init_timer(&priv->ch60_timer);
		priv->ch60_timer.data = (unsigned long) priv;
		priv->ch60_timer.function = rtl8190_ch60_timer;

		init_timer(&priv->ch64_timer);
		priv->ch64_timer.data = (unsigned long) priv;
		priv->ch64_timer.function = rtl8190_ch64_timer;
#endif

		if (((priv->pshare->type>>TYPE_SHIFT) & TYPE_MASK) == TYPE_EMBEDDED) {
			// not implement yet
		}
		else {
			// can't read correct h/w version here
			//GetHardwareVersion(priv);
#ifdef IO_MAPPING
			priv->pshare->io_mapping = 1;
#endif
		}
	}
}

#if defined(PKT_PROCESSOR)&&defined(LOOPBACK_MODE)
//tysu: packet processor ptx to wlan
__IRAM int xmit_entry2(struct sk_buff *pkt, struct net_device * pnetdev)
{
	struct usb_device *pusbd=privs[0].dev;	
	uint pipe_hdl=usb_sndbulkpipe(pusbd,0x4/*VO_ENDPOINT*/);
	uint pipe_in_hdl=usb_rcvbulkpipe(pusbd,0x3);
	

	if(tx_own[tx_idx]==0)
	{

		txbuff[tx_idx][0]=pkt->len&0xff;
		txbuff[tx_idx][1]=(pkt->len>>8)&0xff;	

		txbuff[tx_idx][8]=(pkt->len+8)&0xff;
		txbuff[tx_idx][9]=((pkt->len+8)>>8)&0xff;	


		if(pkt->data-42<pkt->head) printk("error...\n");
		memcpy((pkt->data-42),txbuff[tx_idx],56);
		
		dma_cache_wback_inv((unsigned long)pkt->data-42,pkt->len+42);
		
//		memDump(pkt->data-42,pkt->len+42,"tx data");
		
		usb_fill_bulk_urb(tx_urb[tx_idx],
						pusbd,
						pipe_hdl, 
						(pkt->data-42),
						pkt->len+42,
						(usb_complete_t)write_complete,
						&tx_info[tx_idx]);   //context is xmit_frame

		tx_own[tx_idx]=1;	
//			if(tx_idx==MAX_LB_TX_URB-1)
//				tx_urb[tx_idx]->transfer_flags  &= (~URB_NO_INTERRUPT);
//			else
		tx_urb[tx_idx]->transfer_flags  |= URB_NO_INTERRUPT;


//		us_delay(5);  //sim normal 8187 tx time
		
		usb_submit_urb(tx_urb[tx_idx], GFP_ATOMIC);

		tx_idx=(++tx_idx)%MAX_LB_TX_URB;
	}
	else
	{
//		printk("full\n");
		return -1;
	}
	return 0;
}
#endif



static int MDL_DEVINIT rtl8190_init_one(
                  struct usb_device *pdev,
                  const struct usb_device_id *ent, struct _device_info_ *wdev, int vap_idx)
{
    static struct net_device *dev;
    static struct rtl8190_priv *priv;
    static void *regs;
	static struct wifi_mib 		*pmib;
	static DOT11_QUEUE				*pevent_queue;
#ifdef CONFIG_RTL_WAPI_SUPPORT
	static DOT11_QUEUE				*wapiEvent_queue;	
#endif
	static struct rtl8190_hw		*phw;
	static struct wlan_hdr_poll	*pwlan_hdr_poll;
	static struct wlanllc_hdr_poll	*pwlanllc_hdr_poll;
	static struct wlanbuf_poll		*pwlanbuf_poll;
	static struct wlanicv_poll		*pwlanicv_poll;
	static struct wlanmic_poll		*pwlanmic_poll;
	static struct wlan_acl_poll	*pwlan_acl_poll;
	static DOT11_EAP_PACKET		*Eap_packet;
#ifdef INCLUDE_WPA_PSK
	static WPA_GLOBAL_INFO			*wpa_global_info;
#endif
#ifdef  CONFIG_RTK_MESH
#ifdef _MESH_ACL_ENABLE_
	struct mesh_acl_poll	*pmesh_acl_poll = NULL;
#endif
#ifdef _11s_TEST_MODE_
	struct Galileo_poll		*pgalileo_poll = NULL;
#endif
#ifndef WDS
	char baseDevName[8];
#endif
	static int mesh_num;

	static struct hash_table		*proxy_table;
#ifdef PU_STANDARD
	//pepsi
	static struct hash_table		*proxyupdate_table;
#endif
	static struct mpp_tb			*pann_mpp_tb;
	static struct hash_table		*mesh_rreq_retry_queue;
	// add by chuangch 2007.09.13
	static struct hash_table		*pathsel_table;

	static DOT11_QUEUE2			*pathsel_queue ;
#ifdef	_11s_TEST_MODE_
	DOT11_QUEUE2			*receiver_queue = NULL;
#endif
#endif	// CONFIG_RTK_MESH




#ifdef WDS
	static int wds_num;
	static char baseDevName[8];
#endif
#if defined(WDS)||defined(RTL8192SU_TX_FEW_INT)
	static int i;
#endif

	static unsigned char *page_ptr;
	static struct priv_shared_info *pshare;	// david

	static int rc;

    priv = NULL;
    regs = NULL;
	pmib = NULL;
	pevent_queue = NULL;
#ifdef CONFIG_RTL_WAPI_SUPPORT
	wapiEvent_queue = NULL;
#endif
	phw = NULL;
	pwlan_hdr_poll = NULL;
	pwlanllc_hdr_poll = NULL;
	pwlanbuf_poll = NULL;
	pwlanicv_poll = NULL;
	pwlanmic_poll = NULL;
	pwlan_acl_poll = NULL;
	Eap_packet = NULL;
#ifdef INCLUDE_WPA_PSK
	wpa_global_info = NULL;
#endif

#ifdef PRE_ALLOCATE_SKB
#ifdef MBSSID
	if (vap_idx==-1)
#endif	
	{
		skb_pool_alloc_index=0;
		skb_pool_free_index=0;
		free_cnt=0;
		alloc_skb_pool();
	}
#endif



	pshare = NULL;	// david


	dev = alloc_etherdev(sizeof(struct rtl8190_priv));
	if (!dev) {
		printk(KERN_ERR "alloc_etherdev() error!\n");
       	return -ENOMEM;
	}

	// now, allocating memory for pmib
#ifdef RTL8190_VARIABLE_USED_DMEM
	pmib = (struct wifi_mib *)rtl8190_dmem_alloc(PMIB, NULL);
#else
#ifdef RTL867X_DMEM_ENABLE
	if(vap_idx==-1)
	{
		pmib = &mib;
	}
	else
	{
		pmib = (struct wifi_mib *)kmalloc((sizeof(struct wifi_mib)), GFP_KERNEL);
	}
	
#else
	pmib = (struct wifi_mib *)kmalloc((sizeof(struct wifi_mib)), GFP_KERNEL);
#endif //RTL867X_DMEM_ENABLE
#endif
	if (!pmib) {
		rc = -ENOMEM;
		printk(KERN_ERR "Can't kmalloc for wifi_mib (size %d)\n", sizeof(struct wifi_mib));
		goto err_out_free;
	}
	memset(pmib, 0, sizeof(struct wifi_mib));

	pevent_queue = (DOT11_QUEUE *)kmalloc((sizeof(DOT11_QUEUE)), GFP_KERNEL);
	if (!pevent_queue) {
		rc = -ENOMEM;
		printk(KERN_ERR "Can't kmalloc for DOT11_QUEUE (size %d)\n", sizeof(DOT11_QUEUE));
		goto err_out_free;
	}
	memset((void *)pevent_queue, 0, sizeof(DOT11_QUEUE));
#ifdef CONFIG_RTL_WAPI_SUPPORT
	wapiEvent_queue = (DOT11_QUEUE *)kmalloc((sizeof(DOT11_QUEUE)), GFP_KERNEL);
	if (!wapiEvent_queue) {
		rc = -ENOMEM;
		printk(KERN_ERR "Can't kmalloc for DOT11_QUEUE (size %d)\n", sizeof(DOT11_QUEUE));
		goto err_out_free;
	}
	memset((void *)wapiEvent_queue, 0, sizeof(DOT11_QUEUE));
#endif

#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (wdev->priv == NULL) // root interface
#endif
	{
#if defined(PRIV_STA_BUF) || defined(RTL867X_DMEM_ENABLE)
		phw = &hw_info;
#else
		phw = (struct rtl8190_hw *)kmalloc((sizeof(struct rtl8190_hw)), GFP_KERNEL);
		if (!phw) {
			rc = -ENOMEM;
			printk(KERN_ERR "Can't kmalloc for rtl8190_hw (size %d)\n", sizeof(struct rtl8190_hw));
			goto err_out_free;
		}
#endif
		memset((void *)phw, 0, sizeof(struct rtl8190_hw));

#if defined(PRIV_STA_BUF) || defined(RTL867X_DMEM_ENABLE)
		pshare = &shared_info;
#else
		pshare = (struct priv_shared_info *)kmalloc(sizeof(struct priv_shared_info), GFP_KERNEL);
		if (!pshare) {
			rc = -ENOMEM;
			printk(KERN_ERR "Can't kmalloc for priv_shared_info (size %d)\n", sizeof(struct priv_shared_info));
			goto err_out_free;
		}
#endif
		memset((void *)pshare, 0, sizeof(struct priv_shared_info));

#ifdef PRIV_STA_BUF
		pwlan_hdr_poll = &hdr_pool;
#else
		pwlan_hdr_poll = (struct wlan_hdr_poll *)
						kmalloc((sizeof(struct wlan_hdr_poll)), GFP_KERNEL);
		if (!pwlan_hdr_poll) {
			rc = -ENOMEM;
			printk(KERN_ERR "Can't kmalloc for wlan_hdr_poll (size %d)\n", sizeof(struct wlan_hdr_poll));
			goto err_out_free;
		}
#endif

#ifdef PRIV_STA_BUF
		pwlanllc_hdr_poll = &llc_pool;
#else
		pwlanllc_hdr_poll = (struct wlanllc_hdr_poll *)
						kmalloc((sizeof(struct wlanllc_hdr_poll)), GFP_KERNEL);
		if (!pwlanllc_hdr_poll) {
			rc = -ENOMEM;
			printk(KERN_ERR "Can't kmalloc for wlanllc_hdr_poll (size %d)\n", sizeof(struct wlanllc_hdr_poll));
			goto err_out_free;
		}
#endif

#ifdef PRIV_STA_BUF
		pwlanbuf_poll = &buf_pool;
#else
		pwlanbuf_poll = (struct	wlanbuf_poll *)
						kmalloc((sizeof(struct	wlanbuf_poll)), GFP_KERNEL);
		if (!pwlanbuf_poll) {
			rc = -ENOMEM;
			printk(KERN_ERR "Can't kmalloc for wlanbuf_poll (size %d)\n", sizeof(struct wlanbuf_poll));
			goto err_out_free;
		}
#endif

#ifdef PRIV_STA_BUF
		pwlanicv_poll = &icv_pool;

#else
		pwlanicv_poll = (struct	wlanicv_poll *)
						kmalloc((sizeof(struct	wlanicv_poll)), GFP_KERNEL);
		if (!pwlanicv_poll) {
			rc = -ENOMEM;
			printk(KERN_ERR "Can't kmalloc for wlanicv_poll (size %d)\n", sizeof(struct wlanicv_poll));
			goto err_out_free;
		}
#endif

#ifdef PRIV_STA_BUF
		pwlanmic_poll = &mic_pool;
#else
		pwlanmic_poll = (struct	wlanmic_poll *)
						kmalloc((sizeof(struct	wlanmic_poll)), GFP_KERNEL);
		if (!pwlanmic_poll) {
			rc = -ENOMEM;
			printk(KERN_ERR "Can't kmalloc for wlanmic_poll (size %d)\n", sizeof(struct wlanmic_poll));
			goto err_out_free;
		}
#endif
	}

	pwlan_acl_poll = (struct wlan_acl_poll *)
					kmalloc((sizeof(struct wlan_acl_poll)), GFP_KERNEL);
	if (!pwlan_acl_poll) {
		rc = -ENOMEM;
		printk(KERN_ERR "Can't kmalloc for wlan_acl_poll (size %d)\n", sizeof(struct wlan_acl_poll));
		goto err_out_free;
	}

#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_)	// below code copy above ACL code
	pmesh_acl_poll = (struct mesh_acl_poll *)
					kmalloc((sizeof(struct mesh_acl_poll)), GFP_KERNEL);
	if (!pmesh_acl_poll) {
		rc = -ENOMEM;
		printk(KERN_ERR "Can't kmalloc for Mesh wlan_acl_poll (size %d)\n", sizeof(struct mesh_acl_poll));
		goto err_out_free;
	}
#endif

	Eap_packet = (DOT11_EAP_PACKET *)
					kmalloc((sizeof(DOT11_EAP_PACKET)), GFP_KERNEL);
	if (!Eap_packet) {
		rc = -ENOMEM;
		printk(KERN_ERR "Can't kmalloc for Eap_packet (size %d)\n", sizeof(DOT11_EAP_PACKET));
		goto err_out_free;
	}
	memset((void *)Eap_packet, 0, sizeof(DOT11_EAP_PACKET));

#ifdef INCLUDE_WPA_PSK
	wpa_global_info = (WPA_GLOBAL_INFO *)
					kmalloc((sizeof(WPA_GLOBAL_INFO)), GFP_KERNEL);
	if (!wpa_global_info) {
		rc = -ENOMEM;
		printk(KERN_ERR "Can't kmalloc for wpa_global_info (size %d)\n", sizeof(WPA_GLOBAL_INFO));
		goto err_out_free;
	}
	memset((void *)wpa_global_info, 0, sizeof(WPA_GLOBAL_INFO));
#endif

#ifndef __DRAYTEK_OS__
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (wdev->priv) {
#ifdef UNIVERSAL_REPEATER
		if (vap_idx < 0)
			sprintf(dev->name, "%s-vxd", wdev->priv->dev->name);
#endif
#ifdef MBSSID
		if (vap_idx >= 0)
			sprintf(dev->name, "%s-vap%d", wdev->priv->dev->name, vap_idx);
#endif
	}
	else
#endif
		strcpy(dev->name, "wlan%d");
#endif

	SET_MODULE_OWNER(dev);
	priv = dev->priv;
	priv->pmib = pmib;
#if defined(CONFIG_RTL_WAPI_SUPPORT)
	/*	only for test	*/
	priv->pmib->wapiInfo.wapiType = wapiDisable;
	priv->pmib->wapiInfo.wapiUpdateUCastKeyType = 
		priv->pmib->wapiInfo.wapiUpdateMCastKeyType = wapi_disable_update;
	priv->pmib->wapiInfo.wapiUpdateMCastKeyTimeout = 
		priv->pmib->wapiInfo.wapiUpdateUCastKeyTimeout = WAPI_KEY_UPDATE_PERIOD;
	priv->pmib->wapiInfo.wapiUpdateMCastKeyPktNum = 
		priv->pmib->wapiInfo.wapiUpdateUCastKeyPktNum = WAPI_KEY_UPDATE_PKTCNT;

#endif
	priv->pevent_queue = pevent_queue;
#ifdef CONFIG_RTL_WAPI_SUPPORT
	priv->wapiEvent_queue= wapiEvent_queue;
#endif
	priv->pwlan_acl_poll = pwlan_acl_poll;
	priv->Eap_packet = Eap_packet;
#ifdef INCLUDE_WPA_PSK
	priv->wpa_global_info = wpa_global_info;
#endif
#ifdef MBSSID
	priv->vap_id = -1;
#endif
#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_)	// below code copy above ACL code
	priv->pmesh_acl_poll = pmesh_acl_poll;
#endif
	sema_init(&priv->wx_sem,1);
	priv->udev = pdev;
	priv->rx_urb=NULL;	

#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (wdev->priv) {
		priv->pshare = wdev->priv->pshare;
		GET_ROOT_PRIV(priv) = wdev->priv;
#ifdef UNIVERSAL_REPEATER
		if (vap_idx < 0) // create for vxd
			GET_VXD_PRIV(wdev->priv) = priv;
#endif
#ifdef MBSSID
		if (vap_idx >= 0)  { // create for vap
			GET_ROOT_PRIV(priv)->pvap_priv[vap_idx] = priv;
			priv->vap_id = vap_idx;
			priv->vap_init_seq = -1;
		}
#endif
	}
	else
#endif
	{
		priv->pshare = pshare;	// david
		priv->pshare->phw = phw;
#ifdef RTL867X_DMEM_ENABLE
	priv->pshare->fw_IMEM_buf=NULL;
	priv->pshare->fw_EMEM_buf=NULL;
	priv->pshare->fw_DMEM_buf=NULL;
	priv->pshare->agc_tab_buf=NULL;
	priv->pshare->mac_reg_buf=NULL;
	priv->pshare->phy_reg_buf=NULL;
#endif
		priv->pshare->pwlan_hdr_poll = pwlan_hdr_poll;
		priv->pshare->pwlanllc_hdr_poll = pwlanllc_hdr_poll;
		priv->pshare->pwlanbuf_poll = pwlanbuf_poll;
		priv->pshare->pwlanicv_poll = pwlanicv_poll;
		priv->pshare->pwlanmic_poll = pwlanmic_poll;
		wdev->priv = priv;
		spin_lock_init(&priv->pshare->lock);

#ifdef CONFIG_RTK_MESH
		spin_lock_init(&priv->pshare->lock_queue);
		spin_lock_init(&priv->pshare->lock_Rreq);
#endif
		priv->pshare->type = wdev->type;

#ifdef USE_RTL8186_SDK
#ifdef CONFIG_RTL8196B
		priv->pshare->have_hw_mic = 1;
#else
		priv->pshare->have_hw_mic = 0;
#endif
#else
		priv->pshare->have_hw_mic = 1;//tylo, 8672 has GDMA engine
		if(priv->pshare->have_hw_mic!=0)
		{
			//#define REG32(reg)	*(volatile unsigned int*)(reg) 
			REG32(0xb800330c) |= 0x00000400;	//enable GDMA engine
		}
#endif

//		priv->pshare->is_giga_exist  = is_giga_board();
	}

	priv->dev = dev;

#ifndef __DRAYTEK_OS__
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (GET_ROOT_PRIV(priv)) { // is a vxd or vap
		dev->base_addr = GET_ROOT_PRIV(priv)->dev->base_addr;
		goto register_driver;
	}
#endif
	{
		regs = (void *)wdev->base_addr;
		dev->base_addr = (unsigned long)wdev->base_addr;
		priv->pshare->ioaddr = (UINT)regs;

		if (((priv->pshare->type>>TYPE_SHIFT) & TYPE_MASK) == TYPE_PCI_DIRECT)
		{
			int i ;

			_DEBUG_INFO("INIT PCI config space directly\n");



			{
				u32 vendor_deivce_id, config_base;
				config_base = wdev->conf_addr;
				vendor_deivce_id = *((volatile unsigned long *)(config_base+0));
				//printk("vendor_deivce_id=%x\n", vendor_deivce_id);
				if (vendor_deivce_id !=
						((unsigned long)((0x8192<<16)|PCI_VENDOR_ID_REALTEK)))
				{
					_DEBUG_ERR("vendor_deivce_id=%x not match\n", vendor_deivce_id);
					rc = -EIO;
					goto err_out_free;
				}
			}

			*((volatile unsigned long *)PCI_CONFIG_BASE1) = virt_to_bus((void *)dev->base_addr);
			//DEBUG_INFO("...config_base1 = 0x%08lx\n", *((volatile unsigned long *)PCI_CONFIG_BASE1));
			for(i=0; i<1000000; i++);
			*((volatile unsigned char *)PCI_CONFIG_COMMAND) = 0x07;
			//DEBUG_INFO("...command = 0x%08lx\n", *((volatile unsigned long *)PCI_CONFIG_COMMAND));
			for(i=0; i<1000000; i++);
			*((volatile unsigned short *)PCI_CONFIG_LATENCY) = 0x2000;
			for(i=0; i<1000000; i++);
			//DEBUG_INFO("...latency = 0x%08lx\n", *((volatile unsigned long *)PCI_CONFIG_LATENCY));
			force_stop_wlan_hw();
		}
	}

#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (IS_ROOT_INTERFACE(priv))
#endif
		rtl8192se_ePhyInit(priv);

	{
		dev->irq = wdev->irq;
	}

#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
register_driver:
#endif
	dev->open = rtl8190_open;
	dev->stop = rtl8190_close;
	dev->set_multicast_list = rtl8190_set_rx_mode;
#ifdef PKT_PROCESSOR
	dev->hard_start_xmit = rtl8190_start_xmit_from_cpu;
#if defined(LOOPBACK_MODE)
	dev->hard_start_xmit2 = xmit_entry2;
#else
	dev->hard_start_xmit2 = rtl8190_start_xmit;
#endif
#else
	dev->hard_start_xmit = rtl8190_start_xmit;
#endif
	dev->get_stats = rtl8190_get_stats;
	dev->do_ioctl = rtl8190_ioctl;
	dev->set_mac_address = rtl8190_set_hwaddr;
	dev->priv_flags = IFF_DOMAIN_WLAN;

	rc = register_netdev(dev);
	if (rc)
		goto err_out_iomap;

#ifdef	CONFIG_RTL865X_EXTPORT
	if (cachedWlanDev==NULL)
	{
		cachedWlanDev = dev;
		printk("Cache dev[%s] for extension port.\n", cachedWlanDev->name);
	}
#endif

	// Added by Mason Yu
	// MBSSID Port Mapping
	wlanDev[wlanDevNum].dev_pointer = dev;
	wlanDev[wlanDevNum].dev_ifgrp_member = 0;
	wlanDevNum++;

#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (GET_ROOT_PRIV(priv) == NULL)  // is root interface
#endif
		DEBUG_INFO("Init %s, base_addr=%08x, irq=%d\n",
			dev->name, (UINT)dev->base_addr,  dev->irq);

#else //  __DRAYTEK_OS__
	regs = (void *)wdev->base_addr;
	dev->base_addr = (unsigned long)wdev->base_addr;
	priv->pshare->ioaddr = (UINT)regs;

#ifdef UNIVERSAL_REPEATER
	if (GET_ROOT_PRIV(priv) == NULL)  // is root interface
#endif
		DEBUG_INFO("Init %s, base_addr=%08x\n",
			dev->name, (UINT)dev->base_addr);

#endif // __DRAYTEK_OS__


#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (GET_ROOT_PRIV(priv) == NULL)  // is root interface
#endif
	{
	}

#ifdef _INCLUDE_PROC_FS_
	rtl8190_proc_init(dev);
#ifdef PERF_DUMP
	{
		#include <linux/proc_fs.h>

		struct proc_dir_entry *res;
	    res = create_proc_entry("perf_dump", 0, NULL);
	    if (res) {
    	    res->read_proc = read_perf_dump;
    	    res->write_proc = flush_perf_dump;
	    }
	}
#endif
#endif

	// set some default value of mib
	set_mib_default(priv);

#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (GET_ROOT_PRIV(priv) == NULL)  // is root interface
#endif
	{
#ifndef __DRAYTEK_OS__
#ifdef WDS
		wds_num = (priv->pshare->type>>WDS_SHIFT) & WDS_MASK;
		strcpy(baseDevName, dev->name);

		for (i=0; i<wds_num; i++) {
			dev = alloc_etherdev(0);
			if (!dev) {
				printk(KERN_ERR "alloc_etherdev() wds error!\n");
				rc = -ENOMEM;
	    	   	goto err_out_dev;
			}

			dev->open = rtl8190_open;
		    dev->stop = rtl8190_close;
		    dev->hard_start_xmit = rtl8190_start_xmit;
		    dev->get_stats = rtl8190_get_stats;
		    dev->set_mac_address = rtl8190_set_hwaddr;
			dev->priv_flags = IFF_DOMAIN_WLAN;

			priv->wds_dev[i] = dev;
			strcpy(dev->name, baseDevName);
			strcat(dev->name, "-wds%d");
			dev->priv = priv;
		    rc = register_netdev(dev);
			if (rc) {
				printk(KERN_ERR "register_netdev() wds error!\n");
				goto err_out_dev;
			}
		}
#endif // WDS

#ifdef CONFIG_RTK_MESH
		mesh_num = (priv->pshare->type>>MESH_SHIFT) & MESH_MASK;

#ifndef WDS
		strcpy(baseDevName, dev->name);
#endif
		if(mesh_num>0) {
			GET_MIB(priv)->dot1180211sInfo.mesh_enable = 1;
			dev = alloc_etherdev(0);	// mesh allocate ethernet device BUT don't have priv memory (Because share root priv)
			if (!dev) {
				printk(KERN_ERR "alloc_etherdev() mesh error!\n");
				rc = -ENOMEM;
				goto err_out_iomap;
			}
			dev->base_addr = 1;
			dev->open = rtl8190_open;
			dev->stop = rtl8190_close;
			dev->hard_start_xmit = rtl8190_start_xmit;
			dev->get_stats = rtl8190_get_stats;
			dev->set_mac_address = rtl8190_set_hwaddr;
			dev->do_ioctl = rtl8190_ioctl;

			dev->priv_flags = IFF_DOMAIN_WLAN;

			priv->mesh_dev = dev; // NO priv zone dev
			strcpy(dev->name, baseDevName);
			strcat(dev->name, "-msh%d");
			dev->priv = priv;		// mesh priv pointer to root's priv
			rc = register_netdev(dev);
			if (rc) {
				printk(KERN_ERR "register_netdev() mesh error!\n");
				goto err_out_iomap;
			}
		} // end of if(mesh_num>0)

			priv->RreqEnd = 0;
			priv->RreqBegin = 0;

			pann_mpp_tb = (struct mpp_tb*)kmalloc(sizeof(struct mpp_tb), GFP_KERNEL);
			if(!pann_mpp_tb)
			{
				rc = -ENOMEM;
				printk("allocate pann_mpp_tb error!!\n");
				goto err_out_free;
			}
			init_mpp_pool(pann_mpp_tb);
			proxy_table = (struct hash_table*)kmalloc(sizeof(struct hash_table), GFP_KERNEL);
			if(!proxy_table)
			{
				rc = -ENOMEM;
				printk("allocate proxy_table error!!\n");
				goto err_out_free;
			}
			memset((void*)proxy_table, 0, sizeof(struct hash_table));

#ifdef PU_STANDARD
			//pepsi
			proxyupdate_table = (struct hash_table*)kmalloc(sizeof(struct hash_table), GFP_KERNEL);
			if(!proxyupdate_table)
			{
				rc = -ENOMEM;
				printk("allocate proxyupdate_table error!!\n");
				goto err_out_free;
			}
			memset((void*)proxyupdate_table, 0, sizeof(struct hash_table));
#endif

			pathsel_queue = (DOT11_QUEUE2 *)kmalloc((sizeof(DOT11_QUEUE2)), GFP_KERNEL);
			if (!pathsel_queue) {
				rc = -ENOMEM;
				printk(KERN_ERR "Can't kmalloc for PATHSELECTION_QUEUE (size %d)\n", sizeof(DOT11_QUEUE));
				goto err_out_free;
			}
			memset((void *)pathsel_queue, 0, sizeof (DOT11_QUEUE2));
#ifdef _11s_TEST_MODE_
			receiver_queue = (DOT11_QUEUE2 *)kmalloc((sizeof(DOT11_QUEUE2)), GFP_KERNEL);
			if (!receiver_queue) {
				rc = -ENOMEM;
				printk(KERN_ERR "Can't kmalloc for receiver_queue (size %d)\n", sizeof(DOT11_QUEUE));
				goto err_out_free;
			}
			memset((void *)receiver_queue, 0, sizeof (DOT11_QUEUE2));
			pgalileo_poll = (struct Galileo_poll *)	kmalloc((sizeof( struct Galileo_poll)), GFP_KERNEL);
			if (!pgalileo_poll) {
				rc = -ENOMEM;
				printk(KERN_ERR "Can't kmalloc for pgalileo_poll (size %d)\n", sizeof(struct Galileo_poll));
				goto err_out_free;
			}
#endif
			pathsel_table = (struct hash_table*)kmalloc(sizeof(struct hash_table), GFP_KERNEL);
			if(!pathsel_table)
			{
				rc = -ENOMEM;
				printk("allocate pathsel_table error!!\n");
				goto err_out_free;
			}
			memset((void*)pathsel_table, 0, sizeof(struct hash_table));

			mesh_rreq_retry_queue = (struct hash_table*)kmalloc(sizeof(struct hash_table), GFP_KERNEL);
			if(!mesh_rreq_retry_queue)
			{
				rc = -ENOMEM;
				printk("allocate mesh_rreq_retry_queue error!!\n");
				goto err_out_free;
			}
			memset((void*)mesh_rreq_retry_queue, 0, sizeof(struct hash_table));

			rc = init_hash_table(proxy_table, PROXY_TABLE_SIZE, MACADDRLEN, sizeof(struct proxy_table_entry), crc_hashing, search_default, insert_default, delete_default,traverse_default);
			if(rc == HASH_TABLE_FAILED)
			{
				printk("init_hash_table \"proxy_table\" error!!\n");
			}

#ifdef PU_STANDARD
			//pepsi
			rc = init_hash_table(proxyupdate_table, 8, sizeof(UINT8), sizeof(struct proxyupdate_table_entry), PU_hashing, search_default, insert_default, delete_default,traverse_default);
			if(rc == HASH_TABLE_FAILED)
			{
				printk("init_hash_table \"proxyupdate_table\" error!!\n");
			}
#endif
			rc = init_hash_table(pathsel_table, 8, MACADDRLEN, sizeof(struct path_sel_entry), crc_hashing, search_default, insert_default, delete_default,traverse_default);
			if(rc == HASH_TABLE_FAILED)
			{
				printk("init_hash_table \"pathsel_table\" error!!\n");
			}

			rc = init_hash_table(mesh_rreq_retry_queue, DATA_SKB_BUFFER_SIZE, MACADDRLEN, sizeof(struct mesh_rreq_retry_entry), crc_hashing, search_default, insert_default, delete_default,traverse_default);
			if(rc == HASH_TABLE_FAILED)
			{
				printk("init_hash_table \"mesh_rreq_retry_queue\" error!!\n");
			}

			for(i = 0; i < (1 << mesh_rreq_retry_queue->table_size_power); i++)
			{
				(((struct mesh_rreq_retry_entry*)(mesh_rreq_retry_queue->entry_array[i].data))->ptr) = (struct pkt_queue*)kmalloc(sizeof(struct pkt_queue), GFP_KERNEL);
				if (!(((struct mesh_rreq_retry_entry*)(mesh_rreq_retry_queue->entry_array[i].data))->ptr)) {
					rc = -ENOMEM;
					printk(KERN_ERR "Can't kmalloc for mesh_rreq_retry_entry (size %d)\n", sizeof(struct pkt_queue));
					goto err_out_free;
				}
				memset((void *)((((struct mesh_rreq_retry_entry*)(mesh_rreq_retry_queue->entry_array[i].data))->ptr)), 0, sizeof (struct pkt_queue));
			}

#ifdef PU_STANDARD
			priv->proxyupdate_table = proxyupdate_table;
#endif
#ifdef _11s_TEST_MODE_
			priv->receiver_queue = receiver_queue;
			priv->pshare->galileo_poll = pgalileo_poll ;
#endif
			priv->proxy_table = proxy_table;
			priv->pathsel_queue = pathsel_queue;
			priv->pann_mpp_tb = pann_mpp_tb;
			priv->pathsel_table = pathsel_table;
			priv->mesh_rreq_retry_queue = mesh_rreq_retry_queue;
			//=========================================================
#endif // CONFIG_RTK_MESH


#endif  // __DRAYTEK_OS__

#ifdef PRIV_STA_BUF
		page_ptr = (unsigned char *)
			(((unsigned long)desc_buf) + (PAGE_SIZE - (((unsigned long)desc_buf) & (PAGE_SIZE-1))));
		phw->ring_buf_len = (unsigned long)desc_buf + sizeof(desc_buf) - (unsigned long)page_ptr;
		phw->ring_dma_addr = virt_to_bus(page_ptr);
		page_ptr = (unsigned char *)KSEG1ADDR(page_ptr);
#else

		{
#ifdef __DRAYTEK_OS__
		page_ptr = rtl8185_malloc(DESC_DMA_PAGE_SIZE, 1);	// allocate non-cache buffer
#else
		page_ptr = kmalloc(DESC_DMA_PAGE_SIZE, GFP_KERNEL);
#endif
		}
		phw->ring_buf_len = DESC_DMA_PAGE_SIZE;

		if (page_ptr == NULL) {
			printk(KERN_ERR "can't allocate descriptior page, abort!\n");
			goto err_out_dev;
		}
#endif

		DEBUG_INFO("page_ptr=%lx, size=%ld\n",  (unsigned long)page_ptr, (unsigned long)DESC_DMA_PAGE_SIZE);
		phw->ring_virt_addr = (unsigned long)page_ptr;

#ifdef CONFIG_RTL8190_PRIV_SKB
		init_priv_skb_buf(priv);
#endif

#ifdef PRIV_STA_BUF
		init_priv_sta_buf(priv);
#endif
#ifdef RTL8192SU_TX_FEW_INT
		printk("TX_FEW_INT patch --------- pool alloc\n");

			for(i=0;i<MAX_TX_URB;i++)
			{
				priv->pshare->tx_pool.urb[i] = NULL;
				priv->pshare->tx_pool.pdescinfo[i] = NULL;
				priv->pshare->tx_pool.own[i] = 0;
	
			}

			for(i=0;i<MAX_TX_URB;i++)
			{

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
				priv->pshare->tx_pool.urb[i] = usb_alloc_urb(0,GFP_ATOMIC);
				if(priv->pshare->tx_pool.urb[i]==NULL) goto err_out_free;
#else
				priv->pshare->tx_pool.urb[i] = usb_alloc_urb(0);
				if(priv->pshare->tx_pool.urb[i]==NULL) goto err_out_free;

#endif
				priv->pshare->tx_pool.pdescinfo[i] = kmalloc(sizeof(struct tx_desc_info), GFP_ATOMIC);
				if(priv->pshare->tx_pool.pdescinfo[i]==NULL) goto err_out_free;
	
			}
			priv->pshare->tx_pool_idx=0;
#endif
			init_timer(&priv->poll_usb_timer);
			priv->poll_usb_timer.data = (unsigned long) priv;
			priv->poll_usb_timer.function = usb_poll_func;
#ifdef SW_LED
			priv->led_dr = kmalloc(sizeof(struct usb_ctrlrequest), GFP_NOIO);
			priv->led_data = kmalloc(sizeof(u8), GFP_NOIO);
			priv->led_urb = usb_alloc_urb(0, GFP_ATOMIC);
#endif
			global_rtl8190_priv=priv;
#ifdef RTL8192SU_EFUSE
		//ReadEfuse8192SU(priv);		
		initMPmib(priv);
#endif
	}

	INIT_LIST_HEAD(&priv->asoc_list); // init assoc_list first because webs may get sta_num even it is not open,
																// and it will cause exception if it is not init, david+2008-03-05
	return 0;
goto err_out_dev; //just for fix warning message
err_out_dev:

	unregister_netdev(dev);

err_out_iomap:


err_out_free:

	if (pmib){
#ifdef RTL8190_VARIABLE_USED_DMEM
		rtl8190_dmem_free(PMIB, pmib);
#else
#ifdef RTL867X_DMEM_ENABLE
		if(vap_idx!=-1) kfree(pmib);
#else
		kfree(pmib);
#endif
#endif
	}
#ifdef  CONFIG_RTK_MESH
	if(proxy_table)
	{
		remove_hash_table(proxy_table);
		kfree(proxy_table);
	}
	if(mesh_rreq_retry_queue)
	{
		for (i=0; i< (1 << mesh_rreq_retry_queue->table_size_power); i++)
		{
			if(((struct mesh_rreq_retry_entry*)(mesh_rreq_retry_queue->entry_array[i].data))->ptr)
			{
				kfree(((struct mesh_rreq_retry_entry*)(mesh_rreq_retry_queue->entry_array[i].data))->ptr);
			}
		}
		remove_hash_table(mesh_rreq_retry_queue);
		kfree(mesh_rreq_retry_queue);
	}

	// add by chuangch 2007.09.13
	if(pathsel_table)
	{
		remove_hash_table(pathsel_table);
		kfree(pathsel_table);
	}

	if(pann_mpp_tb)
		kfree(pann_mpp_tb);

	if (pathsel_queue)
		kfree(pathsel_queue);
#ifdef	_11s_TEST_MODE_
	if (receiver_queue)
		kfree(receiver_queue);
#endif
#endif	// CONFIG_RTK_MESH
	if (pevent_queue)
		kfree(pevent_queue);
#ifdef CONFIG_RTL_WAPI_SUPPORT
	if (wapiEvent_queue)
		kfree(wapiEvent_queue);
#endif
#ifndef PRIV_STA_BUF
	if (phw)
		kfree(phw);
	if (pshare)	// david
		kfree(pshare);
	if (pwlan_hdr_poll)
		kfree(pwlan_hdr_poll);
	if (pwlanllc_hdr_poll)
		kfree(pwlanllc_hdr_poll);
	if (pwlanbuf_poll)
		kfree(pwlanbuf_poll);
	if (pwlanicv_poll)
		kfree(pwlanicv_poll);
	if (pwlanmic_poll)
		kfree(pwlanmic_poll);
#endif
	if (pwlan_acl_poll)
		kfree(pwlan_acl_poll);

#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_)	// below code copy above ACL code
	if (pmesh_acl_poll)
		kfree(pmesh_acl_poll);
#endif

	if (Eap_packet)
		kfree(Eap_packet);
#ifdef INCLUDE_WPA_PSK
	if (wpa_global_info)
		kfree(wpa_global_info);
#endif

#ifdef RTL8192SU_TX_FEW_INT
	for(i=0;i<MAX_TX_URB;i++)
	{
		if(priv->pshare->tx_pool.urb[i])  usb_free_urb(priv->pshare->tx_pool.urb[i]);
		if(priv->pshare->tx_pool.pdescinfo[i]) kfree(priv->pshare->tx_pool.pdescinfo[i]);
	}
#endif
#ifdef SW_LED
	if(priv->led_dr)
		kfree(priv->led_dr);
	if(priv->led_data)
		kfree(priv->led_data);
	if(priv->led_urb)
		usb_free_urb(priv->led_urb);
#endif
	free_netdev(dev);
    wdev->priv = NULL;
    return rc;
}




#if defined(__DRAYTEK_OS__) && defined(WDS)
int rtl8190_add_wds(struct net_device *dev, struct net_device *wds_dev, unsigned char *addr)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int wds_num=priv->pmib->dot11WdsInfo.wdsNum;

	priv->pmib->dot11WdsInfo.dev[wds_num] = wds_dev;
	memcpy(priv->pmib->dot11WdsInfo.entry[wds_num].macAddr, addr, 6);
	wds_dev->priv = priv;
	wds_dev->base_addr = 0;
	priv->pmib->dot11WdsInfo.wdsNum++;

	if (!priv->pmib->dot11WdsInfo.wdsEnabled)
		priv->pmib->dot11WdsInfo.wdsEnabled = 1;

	if (netif_running(priv->dev))
		create_wds_tbl(priv);

	DEBUG_INFO("\r\nAdd WDS: %02x%02x	%02x%02x%02x%02x\n",
		addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
}
#endif




#ifdef CONFIG_WIRELESS_LAN_MODULE
int GetCpuCanSuspend(void)
{
	extern int gCpuCanSuspend;
	return gCpuCanSuspend;
}
#endif

int dev_num=0;
static int rtl8192su_usb_probe(struct usb_interface *intf,
			 const struct usb_device_id *ent) 
{
	int ret=0;

	struct usb_device *pdev = interface_to_usbdev(intf);

#ifdef MBSSID
	int i;
#endif

	if(dev_num >= (sizeof(wlan_device)/sizeof(struct _device_info_))){
		printk("USB device %d can't be support\n", dev_num);
		//return -1;
	}
	else {
		ret = rtl8190_init_one(pdev, ent, &wlan_device[dev_num++], -1);
#ifdef UNIVERSAL_REPEATER
		if (ret == 0) {
			ret = rtl8190_init_one(pdev, ent, &wlan_device[--dev_num], -1);
			dev_num++;
		}
#endif
#ifdef MBSSID
		if (ret == 0) {
			dev_num--;
			for (i=0; i<RTL8190_NUM_VWLAN; i++) {
				ret = rtl8190_init_one(pdev, ent, &wlan_device[dev_num], i);
				if (ret != 0) {
					printk("Init fail!\n");
					return ret;
				}
			}
			dev_num++;
		}
#endif
	}
	return ret;
}

static void MDL_EXIT rtl8190_exit (void);

static void rtl8192su_usb_disconnect(struct usb_interface *intf)
{
	//printk("FIXME: must unregister ndetdev here!! %s %d\n",__FUNCTION__,__LINE__);
	rtl8190_exit();
	dev_num=0;	
}

#ifndef USB_VENDOR_ID_REALTEK
#define USB_VENDOR_ID_REALTEK		0x0bda
#endif

static struct usb_device_id rtl8192su_usb_id_tbl[] = {
	{USB_DEVICE(USB_VENDOR_ID_REALTEK, 0x8192)},
	{USB_DEVICE(USB_VENDOR_ID_REALTEK, 0x8172)},		
	{USB_DEVICE(USB_VENDOR_ID_REALTEK, 0x8174)}, // 2T2R	
	{}
};

MODULE_DEVICE_TABLE(usb, rtl8192su_usb_id_tbl);

static struct usb_driver rtl8192su_usb_driver = {

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 16) && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 20)
	.owner		= THIS_MODULE,
#endif
	.name		= DRV_NAME,	          /* Driver name   */
	.id_table	= rtl8192su_usb_id_tbl,	          /* USB_ID table  */
	.probe		= rtl8192su_usb_probe,	          /* probe fn      */
	.disconnect	= rtl8192su_usb_disconnect,			/* remove fn     */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 0)
	.suspend	= NULL,			          /* PM suspend fn */
	.resume     = NULL,			          /* PM resume fn  */
#endif
};



//System identification for CHIP
#undef REG32
#define REG32(reg)	(*(volatile unsigned int *)((unsigned int)reg))
#define CHIP_OEM_ID	0xb8000000
#define DDR_SELECT	0xb8000008
#define C_CUT		2
#define DDR_BOOT	2
int no_ddr_patch;


int MDL_INIT __rtl8190_init(unsigned long base_addr)
{
	int rc;

#ifdef CONFIG_RTL8196B
	//System identification for CHIP
	no_ddr_patch = !((REG32(CHIP_OEM_ID)<C_CUT)&(REG32(DDR_SELECT)&&DDR_BOOT));
#endif

#if defined(EAP_BY_QUEUE) && defined(USE_CHAR_DEV)
// for module, 2005-12-26 -----------
	extern struct rtl8190_priv* (*rtl8190_chr_reg_hook)(unsigned int minor, struct rtl8190_chr_priv *priv);
	extern void (*rtl8190_chr_unreg_hook)(unsigned int minor);
//------------------------------------
#endif
#ifdef CONFIG_WIRELESS_LAN_MODULE
	wirelessnet_hook = GetCpuCanSuspend;
#ifdef BR_SHORTCUT
	wirelessnet_hook_shortcut = get_shortcut_dev;
#endif
#ifdef PERF_DUMP
	Fn_rtl8651_romeperfEnterPoint = rtl8651_romeperfEnterPoint;
	Fn_rtl8651_romeperfExitPoint = rtl8651_romeperfExitPoint;
#endif
#ifdef CONFIG_RTL8190_PRIV_SKB
	wirelessnet_hook_is_priv_buf = is_rtl8190_priv_buf;
	wirelessnet_hook_free_priv_buf = free_rtl8190_priv_buf;
#endif
#endif // CONFIG_WIRELESS_LAN_MODULE

#ifndef GREEN_HILL
	printk("%s driver version %d.%d.%d (%s - %s)\n", DRV_NAME, DRV_VERSION_H, DRV_VERSION_L, DRV_VERSION_SUBL, DRV_RELDATE, DRV_RELDATE_SUB);
#endif

	for (wlan_index=0; wlan_index<sizeof(wlan_device)/sizeof(struct _device_info_); wlan_index++)
	{
		if (((wlan_device[wlan_index].type >> TYPE_SHIFT) & TYPE_MASK) == TYPE_PCI_BIOS) {
			usb_register(&rtl8192su_usb_driver);
		}
		else {

#ifdef __DRAYTEK_OS__
			wlan_device[wlan_index].base_addr = base_addr;
			wlan_device[wlan_index].type = (TYPE_PCI_DIRECT<<TYPE_SHIFT);
#endif

			rc = rtl8190_init_one(NULL, NULL, &wlan_device[wlan_index], -1);

#ifdef UNIVERSAL_REPEATER
			if (rc == 0)
				rc = rtl8190_init_one(NULL, NULL, &wlan_device[wlan_index], -1);
#endif
#ifdef MBSSID
#endif
		}

#ifdef __DRAYTEK_OS__
		if (rc != 0)
			return rc;
#endif

	}


#ifdef _USE_DRAM_
	{
	extern unsigned char *en_cipherstream;
	extern unsigned char *tx_cipherstream;
	extern char *rc4sbox, *rc4kbox;
	extern unsigned char *pTkip_Sbox_Lower, *pTkip_Sbox_Upper;
	extern unsigned char Tkip_Sbox_Lower[256], Tkip_Sbox_Upper[256];

	extern void r3k_enable_DRAM(void);    //6/7/04' hrchen, for 8671 DRAM init
	r3k_enable_DRAM();    //6/7/04' hrchen, for 8671 DRAM init

	en_cipherstream = (unsigned char *)(DRAM_START_ADDR);
	tx_cipherstream = en_cipherstream;

	rc4sbox = (char *)(DRAM_START_ADDR + 2048);
	rc4kbox = (char *)(DRAM_START_ADDR + 2048 + 256);
	pTkip_Sbox_Lower = (unsigned char *)(DRAM_START_ADDR + 2048 + 256*2);
	pTkip_Sbox_Upper = (unsigned char *)(DRAM_START_ADDR + 2048 + 256*3);

	memcpy(pTkip_Sbox_Lower, Tkip_Sbox_Lower, 256);
	memcpy(pTkip_Sbox_Upper, Tkip_Sbox_Upper, 256);
	}
#else
#ifdef CONFIG_RTL865X
	extern unsigned char pTkip_Sbox_Lower[256], pTkip_Sbox_Upper[256];
	extern unsigned char Tkip_Sbox_Lower[256], Tkip_Sbox_Upper[256];

	memcpy(&pTkip_Sbox_Lower[0], &Tkip_Sbox_Lower[0], 256);
	memcpy(&pTkip_Sbox_Upper[0], &Tkip_Sbox_Upper[0], 256);
#endif
#endif

#if defined(EAP_BY_QUEUE) && defined(USE_CHAR_DEV)
// for module, 2005-12-26 -----------
	rtl8190_chr_reg_hook = rtl8190_chr_reg;
	rtl8190_chr_unreg_hook = rtl8190_chr_unreg;
//------------------------------------
	rtl8190_chr_init();
#endif

	//turn off AP LED
	{
		unsigned char wlanreg = *(volatile unsigned char *)0xbd30005e;
		*(volatile unsigned char *)0xbd30005e = (wlanreg | ((1<<5)));
	}

#ifdef PERF_DUMP
	rtl8651_romeperfInit();
#endif

#ifdef USB_PKT_RATE_CTRL_SUPPORT
	register_usb_hook = (register_usb_pkt_cnt_fn)(register_usb_pkt_cnt_f);
#endif

	return 0;
}

//item_get(void * data, char * item_name, unsigned short * len)

#ifdef __DRAYTEK_OS__
int rtl8190_init(unsigned long base_addr)
{
	return __rtl8190_init(base_addr);
}
#else // not __DRAYTEK_OS__

void gpioConfig (int gpio_num, int gpio_func);
void gpioClear(int gpio_num);
void gpioSet(int gpio_num);

int MDL_INIT rtl8190_init(void)
{
	gpioConfig(10,2);
	gpioClear(10);
	delay_ms(10);
	gpioSet(10);

	return __rtl8190_init(0);
}
#endif


static void MDL_EXIT rtl8190_exit (void)
{
	struct net_device *dev;
	struct rtl8190_priv *priv;
	int idx;
#if defined(WDS) || defined(MBSSID)
	int i;
#endif

#ifdef CONFIG_WIRELESS_LAN_MODULE
	wirelessnet_hook = NULL;
#ifdef BR_SHORTCUT
	wirelessnet_hook_shortcut = NULL;
#endif
#ifdef PERF_DUMP
	Fn_rtl8651_romeperfEnterPoint = NULL;
	Fn_rtl8651_romeperfExitPoint = NULL;
 #endif
#ifdef CONFIG_RTL8190_PRIV_SKB
	wirelessnet_hook_is_priv_buf = NULL;
	wirelessnet_hook_free_priv_buf = NULL;
#endif
#endif // CONFIG_WIRELESS_LAN_MODULE

#if defined(EAP_BY_QUEUE) && defined(USE_CHAR_DEV)
// for module, 2005-12-26 ------------
	extern struct rtl8190_priv* (*rtl8190_chr_reg_hook)(unsigned int minor, struct rtl8190_chr_priv *priv);
	extern void (*rtl8190_chr_unreg_hook)(unsigned int minor);
//------------------------------------
#endif

#ifdef WDS
	int num;

	for (idx=0; idx<sizeof(wlan_device)/sizeof(struct _device_info_); idx++) {
		if (wlan_device[idx].priv) {
			num = (wlan_device[idx].type >> WDS_SHIFT) & WDS_MASK;
			for (i=0; i<num; i++) {
				unregister_netdev(wlan_device[idx].priv->wds_dev[i]);
				wlan_device[idx].priv->wds_dev[i]->priv = NULL;
				free_netdev(wlan_device[idx].priv->wds_dev[i]);
			}
		}
	}
#endif

#ifdef CONFIG_RTK_MESH
#ifndef WDS
	int num;
#endif

	for (idx=0; idx<sizeof(wlan_device)/sizeof(struct _device_info_); idx++) {
		num = (wlan_device[idx].type >> MESH_SHIFT) & MESH_MASK;
		if(num > 0) { // num is always 0 or 1 in this time
		// for (i=0; i<num; i++) {
			if (wlan_device[idx].priv) {
				wlan_device[idx].priv->mesh_dev->priv = NULL;
				unregister_netdev(wlan_device[idx].priv->mesh_dev);
				kfree(wlan_device[idx].priv->mesh_dev);
			}
		} // end of if(num > 0)
	}

#endif // CONFIG_RTK_MESH


#ifdef UNIVERSAL_REPEATER
	for (idx=0; idx<sizeof(wlan_device)/sizeof(struct _device_info_); idx++) {
		if (wlan_device[idx].priv) {
			struct rtl8190_priv *vxd_priv = GET_VXD_PRIV(wlan_device[idx].priv);
			if (vxd_priv) {
				unregister_netdev(vxd_priv->dev);
#ifdef RTL8190_VARIABLE_USED_DMEM
				rtl8190_dmem_free(PMIB, vxd_priv->pmib);
#else
				kfree(vxd_priv->pmib);
#endif
				kfree(vxd_priv->pevent_queue);
#ifdef CONFIG_RTL_WAPI_SUPPORT
				kfree(vxd_priv->wapiEvent_queue);
#endif
				kfree(vxd_priv->pwlan_acl_poll);
				kfree(vxd_priv->Eap_packet);
#ifdef INCLUDE_WPA_PSK
				kfree(vxd_priv->wpa_global_info);
#endif
				free_netdev(vxd_priv->dev);
				wlan_device[idx].priv->pvxd_priv = NULL;
			}
		}
	}
#endif

#ifdef MBSSID
	for (idx=0; idx<sizeof(wlan_device)/sizeof(struct _device_info_); idx++) {
		if (wlan_device[idx].priv) {
			for (i=0; i<RTL8190_NUM_VWLAN; i++) {
				struct rtl8190_priv *vap_priv = wlan_device[idx].priv->pvap_priv[i];
				if (vap_priv) {
					unregister_netdev(vap_priv->dev);
#ifdef RTL8190_VARIABLE_USED_DMEM
					rtl8190_dmem_free(PMIB, vap_priv->pmib);
#else
#ifdef RTL867X_DMEM_ENABLE
					if((unsigned int)vap_priv->pmib!=(unsigned int)&mib) kfree(vap_priv->pmib);
#else
					kfree(vap_priv->pmib);
#endif
#endif
					kfree(vap_priv->pevent_queue);
#ifdef CONFIG_RTL_WAPI_SUPPORT
					kfree(vap_priv->wapiEvent_queue);
#endif
					kfree(vap_priv->pwlan_acl_poll);
					kfree(vap_priv->Eap_packet);
#ifdef INCLUDE_WPA_PSK
					kfree(vap_priv->wpa_global_info);
#endif
					free_netdev(vap_priv->dev);
					wlan_device[idx].priv->pvap_priv[i] = NULL;
				}
			}
		}
	}
#endif


	for (idx=0; idx<sizeof(wlan_device)/sizeof(struct _device_info_); idx++)
	{
		if (wlan_device[idx].priv == NULL)
			continue;
		priv = wlan_device[idx].priv;
		dev = priv->dev;

#ifdef _INCLUDE_PROC_FS_
#endif

		unregister_netdev(dev);

#ifndef PRIV_STA_BUF
			kfree((void *)priv->pshare->phw->ring_virt_addr);
#endif

#ifdef RTL8190_VARIABLE_USED_DMEM
		rtl8190_dmem_free(PMIB, priv->pmib);
#else
#ifdef RTL867X_DMEM_ENABLE
		if((unsigned int)priv->pmib!=(unsigned int)&mib) kfree(priv->pmib);
#else
		kfree(priv->pmib);
#endif
#endif
		kfree(priv->pevent_queue);
#ifdef CONFIG_RTL_WAPI_SUPPORT
		kfree(priv->wapiEvent_queue);
#endif

#ifdef CONFIG_RTK_MESH
		kfree(priv->pathsel_queue);
#ifdef _11s_TEST_MODE_
		kfree(priv->receiver_queue);

		for(i=0; i< AODV_RREQ_TABLE_SIZE; i++)
			del_timer(&priv->pshare->galileo_poll->node[i].data.expire_timer);

		kfree(priv->pshare->galileo_poll);
#endif
#ifdef	_MESH_ACL_ENABLE_
		kfree(priv->pmesh_acl_poll);
#endif
#endif

#ifndef PRIV_STA_BUF
		kfree(priv->pshare->phw);
		kfree(priv->pshare->pwlan_hdr_poll);
		kfree(priv->pshare->pwlanllc_hdr_poll);
		kfree(priv->pshare->pwlanbuf_poll);
		kfree(priv->pshare->pwlanicv_poll);
		kfree(priv->pshare->pwlanmic_poll);
#endif
		kfree(priv->pwlan_acl_poll);
		kfree(priv->Eap_packet);
#ifdef INCLUDE_WPA_PSK
		kfree(priv->wpa_global_info);
#endif
#ifndef PRIV_STA_BUF
		kfree(priv->pshare);	// david
#endif

	rtl8192su_rx_free(priv);
#ifdef SW_LED
		if(priv->led_dr)
			kfree(priv->led_dr);
		if(priv->led_data)
			kfree(priv->led_data);
		if(priv->led_urb)
			usb_free_urb(priv->led_urb);
#endif
		free_netdev(dev);
		wlan_device[idx].priv = NULL;
	}

#if defined(EAP_BY_QUEUE) && defined(USE_CHAR_DEV)
// for module, 2005-12-26 ------------
	rtl8190_chr_reg_hook = NULL;
	rtl8190_chr_unreg_hook = NULL;
//------------------------------------

	rtl8190_chr_exit();
#endif
}


#ifdef USE_CHAR_DEV
struct rtl8190_priv *rtl8190_chr_reg(unsigned int minor, struct rtl8190_chr_priv *priv)
{
	if (wlan_device[minor].priv)
		wlan_device[minor].priv->pshare->chr_priv = priv;
	return wlan_device[minor].priv;
}


void rtl8190_chr_unreg(unsigned int minor)
{
	if (wlan_device[minor].priv)
		wlan_device[minor].priv->pshare->chr_priv = NULL;
}
#endif


#ifdef RTL_WPA2_PREAUTH
void wpa2_kill_fasync(void)
{
	int wlan_index = 0;
	struct _device_info_ *wdev = &wlan_device[wlan_index];
	struct rtl8190_priv *priv = wdev->priv;
	event_indicate(priv, NULL, -1);
}


void wpa2_preauth_packet(struct sk_buff	*pskb)
{
	// ****** NOTICE **********
	int wlan_index = 0;
	struct _device_info_ *wdev = &wlan_device[wlan_index];
	// ****** NOTICE **********

	struct rtl8190_priv *priv = wdev->priv;

	unsigned char		szEAPOL[] = {0x02, 0x01, 0x00, 0x00};
	DOT11_EAPOL_START	Eapol_Start;

	if (priv == NULL) {
		PRINT_INFO("%s: priv == NULL\n", (char *)__FUNCTION__);
		return;
	}

#ifndef WITHOUT_ENQUEUE
	if (!memcmp(pskb->data, szEAPOL, sizeof(szEAPOL)))
	{
		Eapol_Start.EventId = DOT11_EVENT_EAPOLSTART_PREAUTH;
		Eapol_Start.IsMoreEvent = FALSE;
#ifdef LINUX_2_6_22_
		memcpy(&Eapol_Start.MACAddr, pskb->mac_header + MACADDRLEN, WLAN_ETHHDR_LEN);
#else
		memcpy(&Eapol_Start.MACAddr, pskb->mac.raw + MACADDRLEN, WLAN_ETHHDR_LEN);
#endif
		DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (unsigned char*)&Eapol_Start, sizeof(DOT11_EAPOL_START));
	}
	else
	{
		unsigned short		pkt_len;

		pkt_len = WLAN_ETHHDR_LEN + pskb->len;
		priv->Eap_packet->EventId = DOT11_EVENT_EAP_PACKET_PREAUTH;
		priv->Eap_packet->IsMoreEvent = FALSE;
		memcpy(&(priv->Eap_packet->packet_len), &pkt_len, sizeof(unsigned short));
#ifdef LINUX_2_6_22_
		memcpy(&(priv->Eap_packet->packet[0]), pskb->mac_header, WLAN_ETHHDR_LEN);
#else
		memcpy(&(priv->Eap_packet->packet[0]), pskb->mac.raw, WLAN_ETHHDR_LEN);
#endif
		memcpy(&(priv->Eap_packet->packet[WLAN_ETHHDR_LEN]), pskb->data, pskb->len);
		DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (unsigned char*)priv->Eap_packet, sizeof(DOT11_EAP_PACKET));
	}
#endif // WITHOUT_ENQUEUE

	event_indicate(priv, NULL, -1);

	// let dsr to free this skb
}
#endif // RTL_WPA2_PREAUTH


#ifdef BR_SHORTCUT
__IRAM_FASTEXTDEV
struct net_device *get_shortcut_dev(unsigned char *da)
{
	int i;
	struct stat_info *pstat;
	struct rtl8190_priv *priv;
//	unsigned long		flags;
	struct net_device	*dev;
#if	defined(RTL_CACHED_BR_STA)
	extern unsigned char cached_br_sta_mac[MACADDRLEN];
	extern struct net_device *cached_br_sta_dev;
#endif

#ifdef  CONFIG_RTK_MESH	//11 mesh no support shortcut now
	extern unsigned char cached_mesh_mac[MACADDRLEN];
	extern struct net_device *cached_mesh_dev;
	if (cached_mesh_dev && !memcmp(da, cached_mesh_mac, MACADDRLEN))
		return cached_mesh_dev;

#endif

#ifdef WDS
	{
		extern unsigned char cached_wds_mac[MACADDRLEN];
		extern struct net_device *cached_wds_dev;
		if (cached_wds_dev && !memcmp(da, cached_wds_mac, MACADDRLEN))
			return cached_wds_dev;
	}
#endif

#ifdef CLIENT_MODE
	{
		extern unsigned char cached_sta_mac[MACADDRLEN];
		extern struct net_device *cached_sta_dev;
		if (cached_sta_dev && !memcmp(da, cached_sta_mac, MACADDRLEN))
			return cached_sta_dev;
	}
#endif

#if	defined(RTL_CACHED_BR_STA)
	if (!memcmp(da, cached_br_sta_mac, MACADDRLEN))
		return cached_br_sta_dev;
#endif
dev = NULL;
	for (i=0; i<sizeof(wlan_device)/sizeof(struct _device_info_) && dev == NULL; i++) {
		if (wlan_device[i].priv && netif_running(wlan_device[i].priv->dev))
		{
			priv = wlan_device[i].priv;
// skylark		 
//SAVE_INT_AND_CLI(flags);

			if ((priv->dev->br_port) &&
				!(priv->dev->br_port->br->stp_enabled) &&
				!(priv->pmib->dot11OperationEntry.disable_brsc))
			{
				pstat = get_stainfo(priv, da);
				if (pstat) 
                {
#ifdef WDS
					if (!(pstat->state & WIFI_WDS))	// if WDS peer
#endif
				    {
#ifdef CONFIG_RTK_MESH
					  if( isMeshPoint(pstat))
						dev = priv->mesh_dev;
					  else
#endif
						dev = priv->dev;
                     }
				}				
			}
//RESTORE_INT(flags);
		}
	}

#if	defined(RTL_CACHED_BR_STA)
	memcpy(cached_br_sta_mac, da, MACADDRLEN);
	cached_br_sta_dev = dev;
#endif

	return dev;
}

void clear_shortcut_cache(void)
{
#ifdef CONFIG_RTK_MESH
	extern struct net_device *cached_mesh_dev;
	extern unsigned char cached_mesh_mac[MACADDRLEN];
	cached_mesh_dev	= NULL;
	memset(cached_mesh_mac,0,MACADDRLEN);
#endif

#ifdef WDS
	extern struct net_device *cached_wds_dev;
	extern unsigned char cached_wds_mac[MACADDRLEN];
	cached_wds_dev= NULL;
	memset(cached_wds_mac,0,MACADDRLEN);
#endif

#ifdef CLIENT_MODE
	extern struct net_device *cached_sta_dev;
	cached_sta_dev = NULL;
	extern unsigned char cached_sta_mac[MACADDRLEN];
	memset(cached_sta_mac,0,MACADDRLEN);
#endif	

#if	defined(RTL_CACHED_BR_STA)
	extern struct net_device *cached_br_sta_dev;
	extern unsigned char cached_br_sta_mac[MACADDRLEN];
	cached_br_sta_dev = NULL;
	memset(cached_br_sta_mac,0,MACADDRLEN);
#endif

	extern 	 unsigned char cached_eth_addr[MACADDRLEN];
	extern struct net_device *cached_dev;
	cached_dev = NULL;
	memset(cached_eth_addr,0,MACADDRLEN);
}
#endif // BR_SHORTCUT


void update_fwtbl_asoclst(struct rtl8190_priv *priv, struct stat_info *pstat)
{
	unsigned char tmpbuf[16];
	int i;

	struct sk_buff *skb = NULL;
	struct wlan_ethhdr_t *e_hdr;
	unsigned char xid_cmd[] = {0, 0, 0xaf, 0x81, 1, 2};

	// update forwarding table of bridge module
#ifdef PKT_PROCESSOR
	rtl8672_l2delete(pstat->hwaddr, CONFIG_RTL8672_WLAN_PORT, VLANID);
#endif
	if (priv->dev->br_port) {
#if defined(PKT_PROCESSOR) || defined(PRE_ALLOCATE_SKB)
		skb = rtl_dev_alloc_skb(priv, RX_ALLOC_SIZE, _SKB_RX_,TRUE);
#else
		skb = dev_alloc_skb(64);
#endif
		if (skb != NULL) {
#ifdef RTL8190_FASTEXTDEV_FUNCALL
			rtl865x_extDev_pktFromProtocolStack(skb);
#endif
			skb->dev = priv->dev;
#if defined(PKT_PROCESSOR) || defined(PRE_ALLOCATE_SKB)
			skb_reserve(skb,NET_SKB_PAD);
#endif
			skb_put(skb, 60);


			e_hdr = (struct wlan_ethhdr_t *)skb->data;
			memset(e_hdr, 0, 64);
			memcpy(e_hdr->daddr, priv->dev->dev_addr, MACADDRLEN);
			memcpy(e_hdr->saddr, pstat->hwaddr, MACADDRLEN);
			e_hdr->type = 8;
			memcpy(&skb->data[14], xid_cmd, sizeof(xid_cmd));
#ifdef CONFIG_RTL865X
			rtl865x_wlanAccelerate(priv, skb);
#else
#if defined(PKT_PROCESSOR) || defined(PRE_ALLOCATE_SKB)
			rtl_netif_rx(priv,skb,pstat);
#else
			skb->protocol = eth_type_trans(skb, priv->dev);
			netif_rx(skb);
#endif
#endif
		}
	}

	// update association lists of the other WLAN interfaces
	for (i=0; i<sizeof(wlan_device)/sizeof(struct _device_info_); i++) {
		if (wlan_device[i].priv && (wlan_device[i].priv != priv)) {
			if (wlan_device[i].priv->pmib->dot11OperationEntry.opmode & WIFI_AP_STATE) {
				sprintf(tmpbuf, "%02x%02x%02x%02x%02x%02x",
					pstat->hwaddr[0],pstat->hwaddr[1],pstat->hwaddr[2],pstat->hwaddr[3],pstat->hwaddr[4],pstat->hwaddr[5]);
				del_sta(wlan_device[i].priv, tmpbuf);
			}
		}
	}

#ifdef MBSSID
	if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
	{
		for (i=0; i<RTL8190_NUM_VWLAN; i++) {
			if (priv->pvap_priv[i] && IS_DRV_OPEN(priv->pvap_priv[i]) && (priv->pvap_priv[i] != priv) &&
				(priv->vap_init_seq >= 0)) {
				if (priv->pvap_priv[i]->pmib->dot11OperationEntry.opmode & WIFI_AP_STATE) {
					sprintf(tmpbuf, "%02x%02x%02x%02x%02x%02x",
						pstat->hwaddr[0],pstat->hwaddr[1],pstat->hwaddr[2],pstat->hwaddr[3],pstat->hwaddr[4],pstat->hwaddr[5]);
					del_sta(priv->pvap_priv[i], tmpbuf);
				}
			}
		}
	}
#endif
}


// quick fix for warn reboot fail issue
void force_stop_wlan_hw(void)
{
	int i;

	for (i=0; i<sizeof(wlan_device)/sizeof(struct _device_info_); i++) {
		if (wlan_device[i].priv) {
			extern	void phy_InitBBRFRegisterDefinition(struct rtl8190_priv *priv);

			phy_InitBBRFRegisterDefinition(wlan_device[i].priv);
			rtl819x_stop_hw(wlan_device[i].priv, 1);
			rtl819x_stop_hw(wlan_device[i].priv, 0);
		}
	}
}


#ifdef CONFIG_RTL865X
/* Register external device for RTL865X platform */
static int rtl865x_wlanRegistration(struct net_device *dev, int index)
{
	struct rtl8190_priv *priv = dev->priv;
// Can remove

	priv->rtl8650extPortNum = CONFIG_RTL865XB_WLAN1_PORT;
	priv->WLAN_VLAN_ID = priv->pmib->miscEntry.set_vlanid;

	if (priv->rtl8650linkNum[index] != 0)
	{
		printk("%s's link ID is already registered = %d.\n", dev->name, (int)(priv->rtl8650linkNum[index]));
	} else if (devglue_regExtDevice(	dev->name,
										priv->WLAN_VLAN_ID,
										priv->rtl8650extPortNum,
										(unsigned long *)(&priv->rtl8650linkNum[index])) != 0)
	{
		printk("XXX Can't register a link ID for device %s on extPort 0x%x!!!\n", dev->name, priv->rtl8650extPortNum);
		return -EACCES;
	} else
	{
		printk("Device %s on vlan ID %d using Link ID %d. Loopback/Ext port is %d\n",
			dev->name, priv->WLAN_VLAN_ID, (int)(priv->rtl8650linkNum[index]), priv->rtl8650extPortNum);
	}

#ifdef RTL8190_FASTEXTDEV_FUNCALL
	rtl865x_extDev_registerUcastTxDev(dev);
	rtl865x_extDev_regCallBack();

#ifdef DRVMAC_LB
	{
		if (	(priv->pmib->miscEntry.drvmac_lb) &&
			(	(priv->pmib->miscEntry.lb_da[0]) ||
				(priv->pmib->miscEntry.lb_da[1]) ||
				(priv->pmib->miscEntry.lb_da[2]) ||
				(priv->pmib->miscEntry.lb_da[3]) ||
				(priv->pmib->miscEntry.lb_da[4]) ||
				(priv->pmib->miscEntry.lb_da[5]))	)
		{
			rtl865x_extDev_addHost(	priv->pmib->miscEntry.lb_da,
									priv->WLAN_VLAN_ID,
									dev,
									(1 << (priv->rtl8650extPortNum)),
									(priv->rtl8650linkNum[index]));
		}
	}
#endif
#endif

	return 0;
}


/* Unregister external device for RTL865X platform */
static int rtl865x_wlanUnregistration(struct net_device *dev, int index)
{
	struct rtl8190_priv *priv = dev->priv;
// Can remove

	if ( priv->rtl8650linkNum[index] == 0 )
	{
		printk("%s's link ID already unregistered\n", dev->name);
	}else if ( devglue_unregExtDevice((unsigned long)(priv->rtl8650linkNum[index])) != 0 )
	{
		printk("XXX Can't unregister link ID %d for device %s !!!\n", (int)(priv->rtl8650linkNum[index]), dev->name);
		return -EACCES;
	}else
	{
		printk("Device %s's link ID %d unregistered.\n", dev->name, (int)(priv->rtl8650linkNum[index]));
		priv->rtl8650linkNum[index] = 0;
	}

#ifdef RTL8190_FASTEXTDEV_FUNCALL
	rtl865x_extDev_unregisterUcastTxDev(dev);
#endif

	return 0;
}
#endif /* CONFIG_RTL865X */



#ifdef _USE_DRAM_
//init DRAM
void r3k_enable_DRAM(void)
{
  int tmp, tmp1;
	//--- initialize and start COP3
	__asm__ __volatile__(
                ".set\tnoreorder\n\t"
                ".set\tnoat\n\t"
		"mfc0	%0,$12\n\t"
		"nop\n\t"
		"lui	%1,0x8000\n\t"
		"or	%1,%0\n\t"
		"mtc0	%1,$12\n\t"
		"nop\n\t"
		"nop\n\t"
                ".set\tat\n\t"
                ".set\treorder"
		: "=r" (tmp), "=r" (tmp1));

	//set base
	__asm__ __volatile__(
                ".set\tnoreorder\n\t"
                ".set\tnoat\n\t"
	 	"mtc3 %0, $4	# $4: d-ram base\n\t"
 	 	"nop\n\t"
	 	"nop\n\t"
                ".set\tat\n\t"
                ".set\treorder"
		:
		: "r" (DRAM_START_ADDR&0x0fffffff));

	//set size
	__asm__ __volatile__(
                ".set\tnoreorder\n\t"
                ".set\tnoat\n\t"
	 	"mtc3 %0, $5    # $5: d-ram top\n\t"
	 	"nop\n\t"
	 	"nop\n\t"
                ".set\tat\n\t"
                ".set\treorder"
		:
		: "r" (R3K_DRAM_SIZE-1));

	//clear DRAM
	__asm__ __volatile__(
"1:\n\t"
	 	"sw	$0,0(%1)\n\t"
	 	"addiu	%1,4\n\t"
	 	"bne	%0,%1,1b\n\t"
	 	"nop\n\t"
                ".set\tat\n\t"
                ".set\treorder"
                :
                : "r" (DRAM_START_ADDR+R3K_DRAM_SIZE), "r" (DRAM_START_ADDR));
}
#endif // _USE_DRAM_


MODULE_LICENSE("GPL");
module_init(rtl8190_init);
module_exit(rtl8190_exit);

