/*
 * linux/drivers/char/misc.c
 *
 * Generic misc open routine by Johan Myreen
 *
 * Based on code from Linus
 *
 * Teemu Rantanen's Microsoft Busmouse support and Derrick Cole's
 *   changes incorporated into 0.97pl4
 *   by Peter Cervasio (pete%q106fm.uucp@wupost.wustl.edu) (08SEP92)
 *   See busmouse.c for particulars.
 *
 * Made things a lot mode modular - easy to compile in just one or two
 * of the misc drivers, as they are now completely independent. Linus.
 *
 * Support for loadable modules. 8-Sep-95 Philip Blundell <pjb27@cam.ac.uk>
 *
 * Fixed a failing symbol register to free the device registration
 *		Alan Cox <alan@lxorguk.ukuu.org.uk> 21-Jan-96
 *
 * Dynamic minors and /proc/mice by Alessandro Rubini. 26-Mar-96
 *
 * Renamed to misc and miscdevice to be more accurate. Alan Cox 26-Mar-96
 *
 * Handling of mouse minor numbers for kerneld:
 *  Idea by Jacques Gelinas <jack@solucorp.qc.ca>,
 *  adapted by Bjorn Ekwall <bj0rn@blox.se>
 *  corrected by Alan Cox <alan@lxorguk.ukuu.org.uk>
 *
 * Changes for kmod (from kerneld):
 *	Cyrus Durgin <cider@speakeasy.org>
 *
 * Added devfs support. Richard Gooch <rgooch@atnf.csiro.au>  10-Jan-1998
 */

#include <linux/module.h>

#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < 0x020600
#include <linux/devfs_fs_kernel.h>
#endif
#include <linux/stat.h>
#include <linux/init.h>

#include <linux/tty.h>
#include <linux/selection.h>
#include <linux/kmod.h>
#include <linux/spinlock.h>

#include "8190n_cfg.h"

#ifdef USE_CHAR_DEV

/********************************************************************
 *	Move from WLAN driver to flexibly create file system            *
 *******************************************************************/
#define MAX_NUM_WLANIF	4
#define WLAN_MISC_MAJOR	13
//#define DEBUG

#ifdef DEBUG
	#define DEBUG_OUT(fmt, args...)		printk(fmt, ## args)
#else
	#define DEBUG_OUT(fmt, args...)		{}
#endif

#ifdef CONFIG_DEVFS_FS
static devfs_handle_t devfs_handle1, devfs_handle2;
#endif

struct rtl8190_chr_priv {
	unsigned int			major;
	unsigned int			minor;
	struct rtl8190_priv*	wlan_priv;
	struct fasync_struct*	asoc_fasync;	// asynch notification
};

#ifdef __LINUX_2_6__
struct module *owner;
#endif

extern struct rtl8190_priv *rtl8190_chr_reg(unsigned int minor, struct rtl8190_chr_priv *priv);
extern void rtl8190_chr_unreg(unsigned int minor);

// for wlan driver module, 2005-12-26 ---
struct rtl8190_priv* (*rtl8190_chr_reg_hook)(unsigned int minor, struct rtl8190_chr_priv *priv) = NULL;
void (*rtl8190_chr_unreg_hook)(unsigned int minor) = NULL;
//---------------------------------------

static int wlanchr_fasync(int fd, struct file *filp, int mode)
{
	struct rtl8190_chr_priv *pchrdev;
	pchrdev = (struct rtl8190_chr_priv *)filp->private_data;

  	return fasync_helper(fd, filp, mode, &pchrdev->asoc_fasync);
}


static int wlanchr_open(struct inode *inode, struct file *filp)
{
	struct rtl8190_chr_priv *pchrdev;

	int minor = MINOR(inode->i_rdev);

#ifdef __LINUX_2_6__
	if(!try_module_get(owner)) {
		printk(KERN_ALERT "Module increasing error!!\n");
		return -ENODEV;
	}
#else
	MOD_INC_USE_COUNT;
#endif

	if (minor >= MAX_NUM_WLANIF)
	{
		DEBUG_OUT("Sorry, try to open %dth wlan_chr dev is not allowed\n", minor);
		return -ENODEV;
	}

	pchrdev = kmalloc(sizeof(struct rtl8190_chr_priv), GFP_KERNEL);
	if (pchrdev == NULL)
	{
		DEBUG_OUT("Sorry, allowd privdata for %dth chr_dev fails\n", minor);
		return 	-ENODEV;
	}

	memset((void *)pchrdev, 0, sizeof (struct rtl8190_chr_priv));
	pchrdev->minor = minor;
	pchrdev->major = MAJOR(inode->i_rdev);

// for wlan driver module, 2005-12-26 ---
//	pchrdev->wlan_priv = rtl8190_chr_reg(minor, pchrdev);
	if (rtl8190_chr_reg_hook)
		pchrdev->wlan_priv = rtl8190_chr_reg_hook(minor, pchrdev);
	else
		pchrdev->wlan_priv = NULL;
//---------------------------------------

	filp->private_data = (void *)pchrdev;

	DEBUG_OUT("Open wlan_chr dev%d success!\n", minor);

	return  0;
}


static int wlanchr_close(struct inode *inode, struct file *filp)
{
	struct rtl8190_chr_priv *pchrdev;

	int minor = MINOR(inode->i_rdev);

	unsigned long flags;
	spinlock_t lock;

	spin_lock_init (&lock);
	spin_lock_irqsave(&lock, flags);

	// below is to un-register process queue!
	wlanchr_fasync(-1, filp, 0);

	pchrdev = (struct rtl8190_chr_priv *)filp->private_data;

	kfree(pchrdev);

// for wlan driver module, 2005-12-26 ---
//	rtl8190_chr_unreg(minor);
	if (rtl8190_chr_unreg_hook)
		rtl8190_chr_unreg_hook(minor);
//----------------------------------------

	spin_unlock_irqrestore(&lock, flags);

#ifdef __LINUX_2_6__
	module_put(owner);
#else
	MOD_DEC_USE_COUNT;
#endif

	DEBUG_OUT("Close wlan_chr dev%d success!\n", minor);

	return 0;

}


static struct file_operations wlanchr_fops = {
   //     read:           wlanchr_read,
   //     poll:           wlanchr_poll,
   //     ioctl:          wlanchr_ioctl,
          fasync:         wlanchr_fasync,
          open:           wlanchr_open,
		  release:		  wlanchr_close,
};


int __init rtl8190_chr_init(void)
{
	// here we are gonna to register all the wlan_chr dev
#ifdef CONFIG_DEVFS_FS
	if (devfs_register_chrdev(WLAN_MISC_MAJOR, "wlan/chr", &wlanchr_fops)) {
		printk(KERN_NOTICE "Can't allocate major number %d for wl_char Devices.\n", WLAN_MISC_MAJOR);
		return -EAGAIN;
	}

	devfs_handle1 = devfs_register(NULL, "wl_chr0",
								DEVFS_FL_DEFAULT, WLAN_MISC_MAJOR, 0,
								S_IFCHR | S_IRUGO | S_IWUGO,
								&wlanchr_fops, NULL);
	devfs_handle2 = devfs_register(NULL, "wl_chr1",
								DEVFS_FL_DEFAULT, WLAN_MISC_MAJOR, 1,
								S_IFCHR | S_IRUGO | S_IWUGO,
								&wlanchr_fops, NULL);
#else
	int ret;

	if ((ret = register_chrdev(WLAN_MISC_MAJOR, "wlan/chr", &wlanchr_fops)) < 0) {
		printk(KERN_NOTICE "Can't allocate major number %d for wl_char Devices.\n", WLAN_MISC_MAJOR);
        return ret;
	}
#endif

	return 0;
}


void __exit rtl8190_chr_exit(void)
{
#ifdef CONFIG_DEVFS_FS
	devfs_unregister(devfs_handle1);
	devfs_unregister(devfs_handle2);
	devfs_unregister_chrdev(WLAN_MISC_MAJOR, "wlan/chr");
#else
	unregister_chrdev(WLAN_MISC_MAJOR, "wlan/chr");
#endif
}

// for wlan driver module, 2005-12-26 ---
EXPORT_SYMBOL(rtl8190_chr_reg_hook);
EXPORT_SYMBOL(rtl8190_chr_unreg_hook);
//---------------------------------------

#endif // USE_CHAR_DEV

