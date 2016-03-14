#include "./8190n_cfg.h"
#include "./8190n.h"
#include "./8190n_hw.h"
#include "./8190n_headers.h"
#include "./8190n_rx.h"
#include "./8190n_debug.h"
#include "./8190n_usb.h"

#if !defined(PKT_PROCESSOR) && !defined(PRE_ALLOCATE_SKB)
extern atomic_t txskb_from_wifi;
extern void checkTXDesc(void);
#endif
extern struct port_map wlanDev[5];
extern int g_port_mapping;
struct rtl8190_priv *global_rtl8190_priv=NULL;

DEFINE_SPINLOCK(gdma_lock);

#if !defined(PKT_PROCESSOR) && !defined(PRE_ALLOCATE_SKB)
void checkNICTX(void){
	if((WIFI_MAX_RX_NUM-1) == atomic_read(&txskb_from_wifi)) {
		checkTXDesc();
	}
}
#endif

void set_wlan_vlan_member(struct sk_buff *newskb){
	int k;
	if ( g_port_mapping == TRUE ) {
		for (k=0; k<5; k++) {
			if ( newskb->dev == wlanDev[k].dev_pointer ) {
				newskb->vlan_member = wlanDev[k].dev_ifgrp_member;
				break;
			}
		}
	}
}

int get_wlan_member(void){
	return wlanDev[0].dev_ifgrp_member;
}
void set_wlan_vid(int vid){
	extern struct rtl8190_priv *global_rtl8190_priv;
	global_rtl8190_priv->dev->vlanid=vid;
}


int wifi_poll=0;
void usb_poll_func(unsigned long task_priv)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)task_priv;
	struct usb_device *dev = priv->udev;
	int i;
	for(i=0;i<5;i++)
		usb_poll(dev->bus);
	if(wifi_poll)
		mod_timer(&priv->poll_usb_timer, jiffies + 1);
	else
		printk("wifi stop polling mode\n");
	return;
}
void kick_usb_poll_timer(void)
{
	if(global_rtl8190_priv!=NULL)
		mod_timer(&global_rtl8190_priv->poll_usb_timer, jiffies + 1);
	return;
}


void gdma_memcpy(UINT32* d, UINT32* s, UINT32 len)
{
#define GDMACNR_ENABLE		0x80000000
#define GDMACNR_MEMCPY	0x00000000
#define GDMABL_LDB	0x80000000
#define GDMACNR_POLL		0x40000000
#define REG32(reg)	*(volatile unsigned int*)(reg) 
	UINT32 tmp;
	//int count=0;		
	unsigned long flags;
	spin_lock_irqsave(&gdma_lock, flags);
	dma_cache_wback_inv((unsigned long)s,len);
	dma_cache_wback_inv((unsigned long)d,len);
	REG32(GDMACNR) = 0;
	REG32(GDMACNR) = GDMACNR_ENABLE | GDMACNR_MEMCPY;
	REG32(GDMAIMR) = 0;
	//tmp = REG32(GDMAISR);
	//REG32(GDMAISR) = tmp;
	REG32(GDMAICVL) = 0;
	REG32(GDMAICVR) = 0;
	REG32(GDMASBP0) = s;
	REG32(GDMASBL0) = GDMABL_LDB | len;
	REG32(GDMADBP0) = d;
	REG32(GDMADBL0) = GDMABL_LDB | len;
	REG32(GDMACNR) = GDMACNR_ENABLE | GDMACNR_POLL | GDMACNR_MEMCPY;
	
	do {
		tmp = REG32(GDMAISR);
		//count++;

	}while( !(tmp&0x80000000) );
	REG32(GDMAISR) = tmp;
	REG32(GDMACNR) = 0;
	spin_unlock_irqrestore(&gdma_lock, flags);
	//while(!(REG32(GDMAISR)&0x80000000));
}
