/*
 *      Header file defines some common inline funtions
 *
 *      $Id: 8190n_util.h,v 1.12 2009/08/06 11:41:29 yachang Exp $
 */

#ifndef _8190N_UTIL_H_
#define _8190N_UTIL_H_
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/circ_buf.h>

#include "./8190n_cfg.h"
#include "./8190n.h"
#include "./wifi.h"
#include "./8190n_hw.h"

#ifdef CONFIG_RTK_MESH
#include "./mesh_ext/mesh_util.h"
#endif

#ifdef PKT_PROCESSOR
#include "./romeperf.h"
#endif

#ifdef GREEN_HILL
#define	SAVE_INT_AND_CLI(x)		{ x = save_and_cli(); }
#define RESTORE_INT(x)			restore_flags(x)
#else
#define SAVE_INT_AND_CLI(x)		{ if (spin_can_lock(&priv->pshare->lock)) spin_lock_irqsave(&priv->pshare->lock, x); else x = 0; }
#define RESTORE_INT(x)			{ if (x != 0) spin_unlock_irqrestore(&priv->pshare->lock, x); }
#endif

#ifdef virt_to_bus
	#undef virt_to_bus
	#define virt_to_bus			CPHYSADDR
#endif

#ifdef USE_IO_OPS

#define get_desc(val)           (val)
#define set_desc(val)           (val)

#define RTL_R8(reg)             inb(((unsigned long)ioaddr) + (reg))
#define RTL_R16(reg)            inw(((unsigned long)ioaddr) + (reg))
#define RTL_R32(reg)            ((unsigned long)inl(((unsigned long)ioaddr) + (reg)))
#define RTL_W8(reg, val8)       outb((val8), ((unsigned long)ioaddr) + (reg))
#define RTL_W16(reg, val16)     outw((val16), ((unsigned long)ioaddr) + (reg))
#define RTL_W32(reg, val32)     outl((val32), ((unsigned long)ioaddr) + (reg))
#define RTL_W8_F                RTL_W8
#define RTL_W16_F               RTL_W16
#define RTL_W32_F               RTL_W32
#undef readb
#undef readw
#undef readl
#undef writeb
#undef writew
#undef writel
#define readb(addr)             inb((unsigned long)(addr))
#define readw(addr)             inw((unsigned long)(addr))
#define readl(addr)             inl((unsigned long)(addr))
#define writeb(val,addr)        outb((val), (unsigned long)(addr))
#define writew(val,addr)        outw((val), (unsigned long)(addr))
#define writel(val,addr)        outl((val), (unsigned long)(addr))

#else // !USE_IO_OPS

#define PAGE_NUM 15

	#define IO_TYPE_CAST	(unsigned char *)

#define PATCH_USB_IN_SCHEDULING
int new_usb_control_msg(struct usb_device *dev, unsigned int pipe, __u8 request, __u8 requesttype, __u16 value, __u16 index, void *data, __u16 size, int timeout);
#ifdef PATCH_USB_IN_SCHEDULING

#define RTL_R32(offset)	RTL_R32SU(priv,((unsigned int)(offset)))
#define RTL_R16(offset)	RTL_R16SU(priv,((unsigned int)(offset)))
#define RTL_R8(reg)		RTL_R8SU (priv ,(unsigned int)((reg)))

#define RTL_W32(offset, val)	RTL_W32SU(priv,((unsigned int)(offset)), ((unsigned int)(val)))
#define RTL_W16(offset, val)	RTL_W16SU(priv,((unsigned int)(offset)), ((unsigned short)(val)))
#define RTL_W8(reg, val)		RTL_W8SU (priv ,(unsigned int)((reg)), ((unsigned char)(val)))

#ifdef PRE_ALLOCATE_SKB

#define MAX_SKB_POOL (128*2+64+128+4)  // 8192su rx = 128*2, nic tx=128, nic rx=64 , 4 buffer
extern int skb_pool_alloc_index;
extern int skb_pool_free_index;
extern int free_cnt;
extern struct sk_buff *skb_pool_ring[MAX_SKB_POOL];
void insert_skb_pool(struct sk_buff *skb);
struct sk_buff *get_free_skb(void);
int alloc_skb_pool();
#endif
#endif

__attribute__((nomips16)) void delay_us(unsigned int t);
__attribute__((nomips16)) void delay_ms(unsigned int t);



#ifdef CHECK_SWAP
#define get_desc(val)	((priv->pshare->type & ACCESS_SWAP_MEM) ? le32_to_cpu(val) : val)
#define set_desc(val)	((priv->pshare->type & ACCESS_SWAP_MEM) ? cpu_to_le32(val) : val)
#else
#define get_desc(val)	(val)
#define set_desc(val)	(val)
#endif

#endif // USE_IO_OPS


#define get_tofr_ds(pframe)	((GetToDs(pframe) << 1) | GetFrDs(pframe))

#define is_qos_data(pframe)	((GetFrameSubType(pframe) & (WIFI_DATA_TYPE | BIT(7))) == (WIFI_DATA_TYPE | BIT(7)))

#define UINT32_DIFF(a, b)		((a >= b)? (a - b):(0xffffffff - b + a + 1))

struct list_head *dequeue_frame(struct rtl8190_priv *priv, struct list_head *head);

static __inline__ int wifi_mac_hash(unsigned char *mac)
{
	unsigned long x;

	x = mac[0];
	x = (x << 2) ^ mac[1];
	x = (x << 2) ^ mac[2];
	x = (x << 2) ^ mac[3];
	x = (x << 2) ^ mac[4];
	x = (x << 2) ^ mac[5];

	x ^= x >> 8;

	return x & (NUM_STAT - 1);
}

static __inline__ struct rx_frinfo *get_pfrinfo(struct sk_buff *pskb)
{
	return (struct rx_frinfo *)((unsigned long)(pskb->end) - sizeof(struct rx_frinfo));
}

static __inline__ struct sk_buff *get_pskb(struct rx_frinfo *pfrinfo)
{
	return (pfrinfo->pskb);
}

UINT8 *get_pframe(struct rx_frinfo *pfrinfo);

static __inline__ UINT8	get_hdrlen(struct rtl8190_priv *priv, UINT8 *pframe)
{
	if (GetFrameType(pframe) == WIFI_DATA_TYPE)
	{
#ifdef CONFIG_RTK_MESH
		if ((get_tofr_ds(pframe) == 0x03) && ( (GetFrameSubType(pframe) == WIFI_11S_MESH) || (GetFrameSubType(pframe) == WIFI_11S_MESH_ACTION)))
		{
			if(GetFrameSubType(pframe) == WIFI_11S_MESH)  // DATA frame, qos might be on (TRUE on 8186)
			{
					return WLAN_HDR_A4_QOS_LEN;
			} // WIFI_11S_MESH
			else // WIFI_11S_MESH_ACTION frame, although qos flag is on, the qos field(2bytes) is not used for 8186
			{
				if(is_mesh_6addr_format_without_qos(pframe)) {
					return WLAN_HDR_A6_MESH_DATA_LEN;
				} else {
					return WLAN_HDR_A4_MESH_DATA_LEN;
				}
			}
		} // end of get_tofr_ds == 0x03 & (MESH DATA or MESH ACTION)
		else
#endif // CONFIG_RTK_MESH
		if (is_qos_data(pframe)) {
			if (get_tofr_ds(pframe) == 0x03)
				return WLAN_HDR_A4_QOS_LEN;
#ifdef A4_TUNNEL
			else if (priv->pmib->miscEntry.a4tnl_enable)
				return WLAN_HDR_A4_QOS_LEN;
#endif
			else
				return WLAN_HDR_A3_QOS_LEN;
		}
		else {
			if (get_tofr_ds(pframe) == 0x03)
				return WLAN_HDR_A4_LEN;
#ifdef A4_TUNNEL
			else if (priv->pmib->miscEntry.a4tnl_enable)
				return WLAN_HDR_A4_LEN;
#endif
			else
				return WLAN_HDR_A3_LEN;
		}
	}
	else if (GetFrameType(pframe) == WIFI_MGT_TYPE)
		return 	WLAN_HDR_A3_LEN;
	else if (GetFrameType(pframe) == WIFI_CTRL_TYPE)
	{
		if (GetFrameSubType(pframe) == WIFI_PSPOLL)
			return 16;
		else if (GetFrameSubType(pframe) == WIFI_BLOCKACK_REQ)
			return 16;
		else if (GetFrameSubType(pframe) == WIFI_BLOCKACK)
			return 16;
		else
		{
#ifdef _DEBUG_RTL8190_
			printk("unallowed control pkt type! 0x%04X\n", GetFrameSubType(pframe));
#endif
			return 0;
		}
	}
	else
	{
#ifdef _DEBUG_RTL8190_
		printk("unallowed pkt type! 0x%04X\n", GetFrameType(pframe));
#endif
		return 0;
	}
}


#define rtl_atomic_inc(ptr_atomic_t)	atomic_inc(ptr_atomic_t)
#define rtl_atomic_dec(ptr_atomic_t)	atomic_dec(ptr_atomic_t)
#define rtl_atomic_read(ptr_atomic_t)	atomic_read(ptr_atomic_t)
#define rtl_atomic_set(ptr_atomic_t, i)	atomic_set(ptr_atomic_t,i)

enum _skb_flag_ {
	_SKB_TX_ = 1,
	_SKB_RX_ = 2,
	_SKB_RX_IRQ_ = 4,
	_SKB_TX_IRQ_ = 8,
	_SKB_TX_SWAP_ = 16 //RTL8192SU
};

#ifdef RTL8190_FASTEXTDEV_FUNCALL
/*
	For Fast Extension Device module, we need to distinguish "Normal socket buffer allocation"
	( use original function call : rtl_dev_alloc_skb() )
	and "Socket buffer allocation for RX Ring" ( use new this new function call : rtl_dev_alloc_rxRing_skb() ).
	It is because we have some additional processes for socket buffer allocated for RX Ring.
*/
static __inline__ struct sk_buff *rtl_dev_alloc_rxRing_skb(	struct rtl8190_priv *priv,
															unsigned int length,
															int flag,
															unsigned int rxRingIdx,
															struct net_device *dev)
{
	struct sk_buff *skb = NULL;

	skb = rtl865x_extDev_alloc_skb(	length,
									RTL865X_EXTDEV_USED_SKB_HEADROOM,
									rxRingIdx,
									dev);

#ifdef ENABLE_RTL_SKB_STATS
	if (flag & (_SKB_TX_ | _SKB_TX_IRQ_))
		rtl_atomic_inc(&priv->rtl_tx_skb_cnt);
	else
		rtl_atomic_inc(&priv->rtl_rx_skb_cnt);
#endif

	return skb;
}
#endif /* RTL8190_FASTEXTDEV_FUNCALL */

// Allocate net device socket buffer

struct sk_buff *rtl_dev_alloc_skb(struct rtl8190_priv *priv,
				unsigned int length, int flag, int could_alloc_from_kerenl);

#ifdef RTL8190_FASTEXTDEV_FUNCALL
/*
	For Fast Extension Device module, we need to distinguish "Normal socket buffer release"
	( use original function call : rtl_kfree_skb() )
	and "Socket buffer release including RX ring socket buffer release" ( use new this new function call : rtl_actually_kfree_skb() ).
	It is because we have some additional processes for socket buffer free for RX Ring.
*/
static __inline__ void rtl_actually_kfree_skb(struct rtl8190_priv *priv, struct sk_buff *skb, struct net_device *dev, int flag)
{
#ifdef ENABLE_RTL_SKB_STATS
	if (flag & (_SKB_TX_ | _SKB_TX_IRQ_))
		rtl_atomic_dec(&priv->rtl_tx_skb_cnt);
	else
		rtl_atomic_dec(&priv->rtl_rx_skb_cnt);
#endif

#ifdef RTL8190_FASTEXTDEV_FUNCALL
	/* Always release this socket buffer, even if this socket buffer is from RX Ring */
	{
		unsigned short skbType;
		unsigned short skbOwner;

		rtl865x_extDev_getSkbProperty(skb, &skbType, &skbOwner, NULL);
		if ((skbType == RTL865X_TYPE_RXRING) && (skbOwner == RTL865X_DRIVER_OWN)) {
			rtl865x_extDev_rxRunoutTxPending(skb, dev);
		}
		else {
			rtl865x_extDev_kfree_skb(skb, TRUE);
		}
	}
#else
	dev_kfree_skb_any(skb);
#endif
}
#endif /* RTL8190_FASTEXTDEV_FUNCALL */

// Free net device socket buffer
void rtl_kfree_skb(struct rtl8190_priv *priv, struct sk_buff *skb, int flag);

static __inline__ int is_CCK_rate(unsigned char rate)
{
	if ((rate == 2) || (rate == 4) || (rate == 11) || (rate == 22))
		return TRUE;
	else
		return FALSE;
}


static __inline__ int is_MCS_rate(unsigned char rate)
{
	if (rate & 0x80)
		return TRUE;
	else
		return FALSE;
}

static __inline__ int is_MCS_1SS_rate(unsigned char rate)
{
	if ((rate & 0x80) && (rate < _MCS8_RATE_))
		return TRUE;
	else
		return FALSE;
}

static __inline__ int is_MCS_2SS_rate(unsigned char rate)
{
	if ((rate & 0x80) && (rate > _MCS7_RATE_))
		return TRUE;
	else
		return FALSE;
}

static __inline__ void rtl_cache_sync_wback(struct rtl8190_priv *priv, unsigned int start,
				unsigned int size, int direction)
{
#if defined(CACHE_WRITEBACK) && defined(RTL8192SU)
	dma_cache_wback_inv((unsigned long)start, size);
	return;
#endif
		dma_cache_wback_inv((unsigned long)bus_to_virt(start), size);
}


static __inline__ unsigned long get_physical_addr(struct rtl8190_priv *priv, void *ptr,
				unsigned int size, int direction)
{
		return virt_to_bus(ptr);
}


static __inline__ int get_rf_mimo_mode(struct rtl8190_priv *priv)
{
	if (GET_MIB(priv)->dot11RFEntry.MIMO_TR_mode == MIMO_1T2R)
		return MIMO_1T2R;
	else if(GET_MIB(priv)->dot11RFEntry.MIMO_TR_mode == MIMO_1T1R)
		return MIMO_1T1R;
	else
		return MIMO_2T2R;
}


static __inline__ void tx_sum_up(struct rtl8190_priv *priv, struct stat_info *pstat, int pktlen)
{
	struct net_device_stats *pnet_stats;

	if (priv) {
		pnet_stats = &(priv->net_stats);
		pnet_stats->tx_packets++;
		pnet_stats->tx_bytes += pktlen;

		priv->ext_stats.tx_byte_cnt += pktlen;

		// bcm old 11n chipset iot debug, and TXOP enlarge
		priv->pshare->current_tx_bytes += pktlen;
	}

	if (pstat) {
		pstat->tx_pkts++;
		pstat->tx_bytes += pktlen;
		pstat->tx_byte_cnt += pktlen;
	}
}


static __inline__ void rx_sum_up(struct rtl8190_priv *priv, struct stat_info *pstat, int pktlen, int retry)
{
	struct net_device_stats *pnet_stats;

	if (priv) {
		pnet_stats = &(priv->net_stats);
		pnet_stats->rx_packets++;
		pnet_stats->rx_bytes += pktlen;

		if (retry)
			priv->ext_stats.rx_retrys++;

		priv->ext_stats.rx_byte_cnt += pktlen;

		// bcm old 11n chipset iot debug
		priv->pshare->current_rx_bytes += pktlen;
	}

	if (pstat) {
		pstat->rx_pkts++;
		pstat->rx_bytes += pktlen;
		pstat->rx_byte_cnt += pktlen;
	}
}


static __inline__ unsigned char get_cck_swing_idx(unsigned int bandwidth, unsigned char ofdm_swing_idx)
{
	unsigned char cck_swing_idx;

	if (bandwidth == HT_CHANNEL_WIDTH_20) {
		if (ofdm_swing_idx >= TxPwrTrk_CCK_SwingTbl_Len)
			cck_swing_idx = TxPwrTrk_CCK_SwingTbl_Len - 1;
		else
			cck_swing_idx = ofdm_swing_idx;
	}
	else {	// 40M bw
		if (ofdm_swing_idx < 12)
			cck_swing_idx = 0;
		else if (ofdm_swing_idx > (TxPwrTrk_CCK_SwingTbl_Len - 1 + 12))
			cck_swing_idx = TxPwrTrk_CCK_SwingTbl_Len - 1;
		else
			cck_swing_idx = ofdm_swing_idx - 12;
	}

	return cck_swing_idx;
}


#define CIRC_CNT_RTK(head,tail,size)	CIRC_CNT(head,tail,size)
#define CIRC_SPACE_RTK(head,tail,size)	CIRC_SPACE(head,tail,size)

void OR_EQUAL(volatile unsigned int *addr,unsigned int val);
void AND_EQUAL(volatile unsigned int *addr,unsigned int val);
void SET_EQUAL(volatile unsigned int *addr,unsigned int val);


#endif // _8190N_UTIL_H_

