

#include <linux/config.h>
#include "./8190n_cfg.h"

static int usb_control_cnt=0;

#include "./8190n.h"
#include "./8190n_util.h"
#include "./wifi.h"
#include "./8190n_hw.h"
#include <linux/usb.h>
#include "8190n_usb.h"


#ifdef USB_LOCK_ENABLE
#define USB_LOCK(lock, flags) spin_lock_irqsave(lock, flags)
#define USB_UNLOCK(lock, flags) spin_unlock_irqrestore(lock, flags)
#else
#define USB_LOCK(...)
#define USB_UNLOCK(...)
#endif

#ifdef USB_LOCK_ENABLE
spinlock_t rtl8192su_lock = SPIN_LOCK_UNLOCKED;
#endif

unsigned int PHY_QueryBBReg(struct rtl8190_priv *priv, unsigned int RegAddr, unsigned int BitMask);

static void new_usb_api_blocking_completion(struct urb *urb)
{
	int *done=(int *)urb->context;
//	printk("%s %d *******************************\n",__FUNCTION__,__LINE__);
	*done = 1;
//	wmb(); //FIXME
}

__attribute__((nomips16)) static int new_usb_kill_urb(struct urb *urb)
{
	//might_sleep();
	int retval=0;
	if (!(urb && urb->dev && urb->dev->bus))
		return -ENXIO;
	spin_lock_irq(&urb->lock);
	++urb->reject;
	spin_unlock_irq(&urb->lock);

	retval = usb_hcd_unlink_urb(urb, -ENOENT);
	//wait_event(usb_kill_urb_queue, atomic_read(&urb->use_count) == 0);
retry:	
	if (atomic_read(&urb->use_count) == 0)
		goto out;
	if(!in_atomic()) {
	//	printk("%s, %d, preempt_count = %d\n", __func__, __LINE__, preempt_count());
		schedule();
	}
	goto retry;

out:
	spin_lock_irq(&urb->lock);
	--urb->reject;
	spin_unlock_irq(&urb->lock);
	return retval;
}

 irqreturn_t ehci_irq (struct usb_hcd *hcd);

#if defined(CONFIG_RTK_VOIP) && defined(CONFIG_RTK_DEBUG)
// rock: check usb_free_dev error
struct urb *g_last_urb = NULL;
struct usb_device *g_last_dev = NULL;
unsigned int g_last_pipe = 0;
#endif

#ifdef SW_LED
static int new_usb_control_msg2(struct usb_ctrlrequest *dr, struct urb *urb, struct usb_device *dev, unsigned int pipe, __u8 request, __u8 requesttype,
			 __u16 value, __u16 index, void *data, __u16 size, int timeout)
{
	static volatile int done=1;
	unsigned long expire;
	int status;
	int retval;

	if(done==0) {
		//printk("%s, %d, done=%d, busy!\n", __func__, __LINE__, done);
		return -EBUSY;
	}
	
	dr->bRequestType= requesttype;
	dr->bRequest = request;
	dr->wValue = cpu_to_le16p(&value);
	dr->wIndex = cpu_to_le16p(&index);
	dr->wLength = cpu_to_le16p(&size);
	
  	done = 0;
  	
	usb_fill_control_urb(urb, dev, pipe, (unsigned char *)dr, data,
			     size, new_usb_api_blocking_completion, (void *)&done);
	dma_cache_wback_inv((unsigned long)data, size);
	urb->actual_length = 0;
	urb->status = 0;
	
	status = usb_submit_urb(urb, GFP_ATOMIC);
	
	return status;
}
#endif

#ifdef CONFIG_USB_OTG_HOST_RTL8672
void usb_scan_async(struct usb_bus *bus, unsigned wlan_close)
{
	unsigned long		flags;

	struct usb_hcd *hcd=bus_to_hcd(bus);
	
	dwc_otg_hcd_t *otg_hcd = (struct dwc_otg_hcd_t *)hcd->hcd_priv;
	spin_lock_irqsave (&otg_hcd->lock, flags);
	dwc_otg_async(otg_hcd, wlan_close);
	spin_unlock_irqrestore (&otg_hcd->lock, flags);
}
#endif
int new_usb_control_msg(struct usb_device *dev, unsigned int pipe, __u8 request, __u8 requesttype,
			 __u16 value, __u16 index, void *data, __u16 size, int timeout)
{

	struct usb_ctrlrequest *dr;
	struct urb *urb;
	volatile int done=0;
	unsigned long expire;
	int retry_cnt=0;	
	int status;
	int retval;
#ifdef USB_LOCK_ENABLE
	unsigned long flags;
#endif
	
	dr = kmalloc(sizeof(struct usb_ctrlrequest), GFP_NOIO);
	if (!dr)
	{
		return -ENOMEM;
	}

	dr->bRequestType= requesttype;
	dr->bRequest = request;
	dr->wValue = cpu_to_le16p(&value);
	dr->wIndex = cpu_to_le16p(&index);
	dr->wLength = cpu_to_le16p(&size);

	if((value==0x300)||(value==0x320))
	{
		printk("Can't submit control msg value=%x\n",value);
		if(size==4)
			*(u32*)(data)=0;
		else if(size==1)
			*(u8*)(data)=0;
		else if(size==2)
			*(u16*)(data)=0;
		
		kfree(dr);
		return 0;
	}
	
	urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (!urb)
	{
		kfree(dr);
		return -ENOMEM;
	}

	usb_control_cnt++;
	if(usb_control_cnt>1)
		printk("Error usb_control_cnt=%x\n",usb_control_cnt);	

//  printk("fill urb data=%x indx=%x size=%d\n",*(u8*)data,index,size);
	usb_fill_control_urb(urb, dev, pipe, (unsigned char *)dr, data,
			     size, new_usb_api_blocking_completion, (void *)&done);
	urb->actual_length = 0;

	if(request==RTL8192_REQ_SET_REGS)
		dma_cache_wback_inv((unsigned long)data, size);

	status = usb_submit_urb(urb, GFP_ATOMIC);
	if (unlikely(status))
	{
		goto out;
	}


	if(timeout)		//expire = msecs_to_jiffies(timeout) + jiffies;
		expire = timeout + jiffies;
	else
		expire =  200 + jiffies; //cathy
 
	if(expire<200)
	{
		if(!in_atomic())
		{
			delay_ms(2100);
			expire=jiffies+200;
		}
		else
			expire=0xffffffffL; 
	}
 
retry:
	
	if(done==1) goto out;
	if(jiffies>=expire) goto out;
	
#ifndef USB_LOCK_ENABLE
	if(in_atomic())
#endif
	{
		unsigned long		flags=0;
		retry_cnt++;
		if(retry_cnt>2000) goto out;

#ifdef CONFIG_USB_OTG_Driver
		dwc_otg_hcd_irq(usbhcd, NULL);
		printk("%s %d done=%x jiffies=%x expire=%x\n",__FUNCTION__,__LINE__,done,jiffies,expire);
#elif defined(CONFIG_USB_OTG_HOST_RTL8672)
		usb_scan_async(dev->bus, 0);
#elif defined(CONFIG_USB_EHCI_HCD) || defined(CONFIG_USB_EHCI_HCD_RTL8652)
{
		struct usb_hcd *hcd=bus_to_hcd(dev->bus);
		struct ehci_hcd *ehci=(struct ehci_hcd *)hcd->hcd_priv;
		spin_lock_irqsave (&ehci->lock, flags);
		//scan_async(ehci);
		ehci_irq(hcd);		
		spin_unlock_irqrestore (&ehci->lock, flags);
}
#endif
		
	}
#ifndef USB_LOCK_ENABLE
	else
		schedule();	
#endif

	goto retry;

out:

	if(retry_cnt>2000)
	{
		printk("Timeout! requesttype=%x request=%x value=%x index=%x size=%x retry_cnt=%d\n",requesttype,request,value,index,size,retry_cnt);
	}
	
	if(done==0)
	{
		retval = new_usb_kill_urb(urb);
		status = urb->status == -ENOENT ? -ETIMEDOUT : urb->status;
		if((status != 0) && (status!= -ETIMEDOUT)) {
			printk("[%s,%d], urb->status = %d, retval = %d\n", __func__, __LINE__, urb->status, retval);
		}
	}
	else
	{
//		  printk("index=%x data=%x done\n",index,*(u8*)data); 
		status = urb->status;
	}

	usb_free_urb(urb);
	kfree(dr);

	usb_control_cnt--;
	return status;
}



#define PAGE_NUM 15

	#define IO_TYPE_CAST	(unsigned char *)

#ifdef SW_LED
void RTL_W8SU2(struct rtl8190_priv *priv, unsigned int indx, unsigned char data)
{
	struct usb_device *udev = priv->udev;
	int status;

	if (IS_DRV_CLOSE(priv))
	{
#ifdef CONFIG_RTK_VOIP
		// rock: reduce debug message
#else
		printk("device unlink - skip %s\n",__FUNCTION__);
#endif
		return;
	}
	*(u8 *)priv->led_data = data;
	status = new_usb_control_msg2(priv->led_dr, priv->led_urb, udev, usb_sndctrlpipe(udev, 0),
			       RTL8192_REQ_SET_REGS, RTL8192_REQT_WRITE,
			       (indx&0xffff), 0, priv->led_data, 1, HZ / 2);

}
#endif

//#define DEBUG_CHIHHSING
void RTL_W8SU(struct rtl8190_priv *priv, unsigned int indx, unsigned char data)
{	
	struct usb_device *udev = priv->udev;
	int status;
	unsigned char lcv_Count = 0;
#ifdef DEBUG_CHIHHSING	
	//printk("----->(%s, %d)udev=0x%x, indx=0x%x\n", __FUNCTION__, __LINE__, (u32)udev, indx);
#endif
	if (IS_DRV_CLOSE(priv))
	{
#ifdef CONFIG_RTK_VOIP
		// rock: reduce debug message
#else
		printk("device unlink - skip %s\n",__FUNCTION__);
#endif
		return;
	}

	if (indx==0x320)
		printk("RTL_W8SU!!\n\n");
	status = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			       RTL8192_REQ_SET_REGS, RTL8192_REQT_WRITE,
			       (indx&0xffff), 0, &data, 1, HZ / 2);

	if(status >= 0)
	{
	    return;
	}

	/*if failed, continue*/
	while(1)
	{
	    printk("--------- we try again --------\n");
	    
	    status = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
                                    RTL8192_REQ_SET_REGS, RTL8192_REQT_WRITE,
                                    (indx&0xffff), 0, &data, 1, (HZ * (++lcv_Count)));

	    if(status >= 0)
	    {
		break;
	    }
	}

	if(status < 0)
	{
	    printk("write_nic_word TimeOut! status:%x\n", status);
            dump_stack();
	    panic("TimeOut");
	}


#ifdef DEBUG_CHIHHSING	
	printk("(REG_DUMP): write_nic_byte : idx=0x%x val=0x%x\n", indx, data);
#endif
//	printk("w8 index=%x value=%x\n",indx,data);	
}


void RTL_W16SU(struct rtl8190_priv *priv,unsigned  int indx, unsigned short data)
{	
	struct usb_device *udev = priv->udev;
	int status;
	unsigned char lcv_Count = 0;
	data=cpu_to_le16(data);
#ifdef DEBUG_CHIHHSING	
	//printk("----->(%s, %d)reg=0x%x, data=0x%x\n", __FUNCTION__, __LINE__, indx|0xff00, data);
#endif

	if (IS_DRV_CLOSE(priv))
	{
#ifdef CONFIG_RTK_VOIP
		// rock: reduce debug message
#else
		printk("device unlink - skip %s\n",__FUNCTION__);
#endif
		return;
	}

	if (indx==0x320)
		printk("RTL_W16SU!!\n\n");

	status = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
		                RTL8192_REQ_SET_REGS, RTL8192_REQT_WRITE,
			        (indx&0xffff), 0, &data, 2, HZ / 2);
	
	if(status >= 0)
	{
	    return;
	}

	/*if failed, continue*/
	while(1)
	{
	    printk("--------- we try again --------\n");
	    
	    status = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
                                    RTL8192_REQ_SET_REGS, RTL8192_REQT_WRITE,
                                    (indx&0xffff), 0, &data, 2, (HZ * (++lcv_Count)));

	    if(status >= 0)
	    {
		break;
	    }
	}

	if(status < 0)
	{
	    printk("write_nic_word TimeOut! status:%x\n", status);
            dump_stack();
	    panic("TimeOut");
	}

#ifdef DEBUG_CHIHHSING		
	printk("(REG_DUMP): write_nic_word : idx=0x%x val=0x%x\n", indx, data);
#endif
//	printk("w16 index=%x value=%x\n",indx,data);		
}


void RTL_W32SU(struct rtl8190_priv *priv,unsigned  int indx, unsigned int data)
{
	struct usb_device *udev = priv->udev;
	int status;
	unsigned char lcv_Count = 0;
	data=cpu_to_le32(data);
#ifdef DEBUG_CHIHHSING	
	//printk("----->(%s, %d)udev=0x%x, indx=0x%x\n", __FUNCTION__, __LINE__, (u32)udev, indx);
#endif

	if (IS_DRV_CLOSE(priv))
	{
#ifdef CONFIG_RTK_VOIP
		// rock: reduce debug message
#else
		printk("device unlink - skip %s\n",__FUNCTION__);
#endif
		return;
	}

	if (indx==0x320)
		printk("RTL_W32SU!!\n\n");
	status = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			       RTL8192_REQ_SET_REGS, RTL8192_REQT_WRITE,
			       (indx&0xffff), 0, &data, 4, HZ / 2);

	if(status >= 0)
	{
	    return;
	}

	/*if failed, continue*/
	while(1)
	{
	    printk("--------- we try again --------\n");
	    
	    status = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
                                    RTL8192_REQ_SET_REGS, RTL8192_REQT_WRITE,
                                    (indx&0xffff), 0, &data, 4, (HZ * (++lcv_Count)));

	    if(status >= 0)
	    {
		break;
	    }
	}

	if(status < 0)
	{
	    printk("write_nic_word TimeOut! status:%x\n", status);
            dump_stack();
	    panic("TimeOut");
	}

#ifdef DEBUG_CHIHHSING		
	printk("(REG_DUMP): write_nic_dword : idx=0x%x val=0x%x\n", indx, data);
#endif	
//	printk("w32 index=%x value=%x\n",indx,data);		
}

unsigned char RTL_R8SU(struct rtl8190_priv *priv,unsigned  int indx)
{
	unsigned char data=0;
	struct usb_device *udev = priv->udev;
	int status;
	
#ifdef DEBUG_CHIHHSING	
	//printk("----->(%s, %d)\n", __FUNCTION__, __LINE__);
#endif

	if (IS_DRV_CLOSE(priv))
	{
#ifdef CONFIG_RTK_VOIP
		// rock: reduce debug message
#else
		printk("device unlink - skip %s\n",__FUNCTION__);
#endif
		return 0;
	}

	if (indx>=0x800&&indx<=0xfff)
	{ // read BB register
		data=PHY_QueryBBReg(priv, indx, bMaskByte0);
	}
	else
	{
		status = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
				       RTL8192_REQ_GET_REGS, RTL8192_REQT_READ,
				       (indx&0xffff), 0, &data, 1, HZ / 2);
		
		if (status < 0)
		{
			printk("read_nic_byte TimeOut! status:%x addr=0x%x\n", status,indx);
		}
	}
#ifdef DEBUG_CHIHHSING		
	printk("(REG_DUMP): read_nic_byte : idx=0x%x return=0x%x\n", indx, data);
#endif
	return *(unsigned char *)(((unsigned int)&data) | 0xa0000000);
}

unsigned short RTL_R16SU(struct rtl8190_priv *priv,unsigned  int indx)
{
	unsigned int data=0;
	unsigned short data2=0;

	struct usb_device *udev = priv->udev;
	int status;
#ifdef DEBUG_CHIHHSING	
	//printk("----->(%s, %d)\n", __FUNCTION__, __LINE__);
#endif

	if (IS_DRV_CLOSE(priv))
	{
#ifdef CONFIG_RTK_VOIP
		// rock: reduce debug message
#else
		printk("device unlink - skip %s\n",__FUNCTION__);
#endif
		return 0;
	}

	if (indx>=0x800&&indx<=0xfff)
	{ // read BB register
		data2=PHY_QueryBBReg(priv, indx, bMaskLWord);
	}
	else
	{
		status = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
				       RTL8192_REQ_GET_REGS, RTL8192_REQT_READ,
				       (indx&0xffff), 0, &data, 2, HZ / 2);
		
		data = *(unsigned int *)(((unsigned int)&data) | 0xa0000000);
		
		if (status < 0)
		{
			printk("read_nic_word TimeOut! status:%x addr=0x%x\n", status,indx);
		}
#ifdef __LITTLE_ENDIAN
		data2 = data & 0xffff; // jimmy porting DVR platform problem.
#else
		data2 = data >> 16; //tysu: fixed usb can't read offset 2 , 2bytes data bug	
#endif		
		data2=le16_to_cpu(data2);
	}
#ifdef DEBUG_CHIHHSING		
	printk("(REG_DUMP): read_nic_word : idx=0x%x return=0x%x\n", indx, data2);
#endif
	return data2;
}

unsigned int RTL_R32SU(struct rtl8190_priv *priv, unsigned int indx)
{
	unsigned int data=0;
	struct usb_device *udev = (struct usb_device *)priv->udev;
	int status;
#ifdef DEBUG_CHIHHSING	
	//printk("----->(%s, %d udev=%x)\n", __FUNCTION__, __LINE__, (u32)udev);
#endif

	if (IS_DRV_CLOSE(priv))
	{
#ifdef CONFIG_RTK_VOIP
		// rock: reduce debug message
#else
		printk("device unlink - skip %s\n",__FUNCTION__);
#endif
		return 0;
	}

	if (indx>=0x800&&indx<=0xfff)
	{ // read BB register
		data=PHY_QueryBBReg(priv, indx, bMaskDWord);
	}
	else
	{
		status = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
				       RTL8192_REQ_GET_REGS, RTL8192_REQT_READ,
				       (indx&0xffff), 0, &data, 4, HZ / 2);
		
		data = *(unsigned int *)(((unsigned int)&data) | 0xa0000000);
		
		if (status < 0)
		{
			printk("read_nic_dword TimeOut! status:%d indx=0x%x\n", status,indx);
		}
		data=le32_to_cpu(data);
	}
#ifdef DEBUG_CHIHHSING		
	printk("(REG_DUMP): read_nic_dword : idx=0x%x return=0x%x\n", indx, data);
#endif
	return data;
}

unsigned int RTL_R32BaseBand(struct rtl8190_priv *priv, unsigned int indx)
{
	unsigned int data=0;
	struct usb_device *udev = (struct usb_device *)priv->udev;
	int status;
	if (IS_DRV_CLOSE(priv))
		return 0;
	{
		status = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
				       RTL8192_REQ_GET_REGS, RTL8192_REQT_READ,
				       (indx&0xffff), 0, &data, 4, HZ / 2);

		data = *(unsigned int *)(((unsigned int)&data) | 0xa0000000);
		
		if (status < 0)
		{
			printk("read_nic_dword TimeOut! status:%d indx=0x%x\n", status,indx);
		}
		data=le32_to_cpu(data);
	}
	return data;
}


void usb_receive_all_pkts(struct rtl8190_priv *priv)
{
#ifdef CONFIG_USB_OTG_HOST_RTL8672
	usb_scan_async(priv->udev->bus, 1);
#else
	unsigned long		flags=0;

	struct usb_hcd *hcd=bus_to_hcd(priv->udev->bus);
	struct ehci_hcd *ehci=(struct ehci_hcd *)hcd->hcd_priv;

	spin_lock_irqsave (&ehci->lock, flags);
	ehci_irq(hcd);
	spin_unlock_irqrestore (&ehci->lock, flags);
#endif
}


__IRAM_WIFI_PRI5 int Check_USBPatch(unsigned int* tx, unsigned int urb_len, unsigned char en_pktoffset)
{
	unsigned int straddr=((unsigned int)(tx)), exit;
#ifndef CONFIG_USB_OTG_HOST_RTL8672	
#ifdef RTL867X_USBPATCH
	unsigned int over_size=0;
#endif
#endif
	//urb_len+=(en_pktoffset<<3);
check_usb_bug:
	exit=1;
#ifndef CONFIG_USB_OTG_HOST_RTL8672
#ifdef RTL867X_USBPATCH
	switch((straddr&0x3))
	{
		case 1: //check start address: 4N+1
			over_size = (urb_len&0x3f);
			if ((over_size>=4) && (over_size<=6))
			{
				en_pktoffset++;
				urb_len+=8;
				straddr-=8;
				exit=0;
			}
			break;
		case 2: //check start address: 4N+2
			over_size = (urb_len&0x3f);
			if ((over_size>=3) && (over_size<=5))
			{
				en_pktoffset++;
				urb_len+=8;
				straddr-=8;
				exit=0;
			}
			break;
		case 3: //check start address: 4N+3
			over_size = (urb_len&0x3f);
			if ((over_size>=2) && (over_size<=4))
			{
				en_pktoffset++;
				urb_len+=8;
				straddr-=8;
				exit=0;
			}
			break;
		default:
			exit=1;
	}
#endif //RTL867X_USBPATCH
#endif

	if (0==(urb_len & 0x1ff))
	{
		en_pktoffset++;
		urb_len+=8;
		straddr-=8;
		exit=0;
	}

	if (exit!=1)
	{
		//printk("en_pktoffset=%d, urb_len=%d\n", en_pktoffset, urb_len);
		goto check_usb_bug;
	}

	return en_pktoffset;
}


__IRAM_WIFI_PRI5 int Check1kboundary(unsigned int* tx, unsigned int urb_len)
{
#ifndef CONFIG_USB_OTG_HOST_RTL8672
#ifdef RTL867X_USBPATCH
	unsigned int ret=0, straddr=((unsigned int)(tx));
	unsigned int over_size=0;

	if ((((unsigned int)straddr)&0x3ff) != 0)
	{
		if ((((((unsigned int)straddr)+urb_len)&0x00000f00)-(((unsigned int)straddr)&0x00000f00))>= 1024) // over 1k boundary
		{
			//over_size=(((unsigned int)tx)+urb_len)-((((unsigned int)tx)+urb_len)&0xfffffc00);
			over_size = ((((unsigned int)straddr)+urb_len)&0x3ff);
			if (over_size>=4&&over_size<=7)
			{
				//printk("tx=%x, urb_len=0x%x(%d), straddr=%x, en_pktoffset=%d\n", (unsigned int)straddr, urb_len, urb_len, straddr, en_pktoffset);
				ret=1;
			}
		}
	}
	return ret;
#endif //RTL867X_USBPATCH
#else
	return 0;
#endif
}

__IRAM_WIFI_PRI5 int USB_PATCH(unsigned int** tx, unsigned int* urb_len, unsigned char* en_pktoffset)
{
//	unsigned int check_usb=0; 
	unsigned char pktoffset=*en_pktoffset;
	unsigned int urblen=*urb_len;
#ifndef CONFIG_USB_OTG_HOST_RTL8672
	while(Check1kboundary(*tx, urblen))
	{
		pktoffset++;
		urblen += (pktoffset<<3);
		kfree(*tx);
		*tx = kmalloc(urblen, GFP_ATOMIC);
		if(!(*tx))
		{
			printk("%s, %d\n", __FUNCTION__, __LINE__);
			return 0;
		}
	}
#endif
	*urb_len=urblen;
	*en_pktoffset=pktoffset;
	return 1;
}

void usb_poll(struct usb_bus *bus)
{
#ifdef CONFIG_USB_OTG_HOST_RTL8672
	unsigned long		flags;
	struct usb_hcd *hcd=bus_to_hcd(bus);

	dwc_otg_hcd_t *otg_hcd = (struct dwc_otg_hcd_t *)hcd->hcd_priv;
	spin_lock_irqsave (&otg_hcd->lock, flags);
	dwc_otg_async(otg_hcd, 0);
	dwc_otg_hcd_handle_sof_intr (otg_hcd);
	spin_unlock_irqrestore (&otg_hcd->lock, flags);

	return;
#else
	unsigned long		flags=0;

	struct usb_hcd *hcd=bus_to_hcd(bus);
	struct ehci_hcd *ehci=(struct ehci_hcd *)hcd->hcd_priv;

	spin_lock_irqsave (&ehci->lock, flags);
	ehci_irq(hcd);
	spin_unlock_irqrestore (&ehci->lock, flags);

	return;
#endif
}

int check_align(u32* tx) {
#if defined (CONFIG_USB_OTG_HOST_RTL8672) || defined(CONFIG_USB_OTG_Driver)
	return 	(((((u32)tx)&3)!=0) ? 1:0);
#else
	return 0;
#endif
}

