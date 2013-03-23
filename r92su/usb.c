/******************************************************************************
 *
 * Copyright(c) 2009-2013  Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
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
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Christian Lamparter <chunkeey@googlemail.com>
 * Joshua Roys <Joshua.Roys@gtri.gatech.edu>
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/export.h>

#include "r92su.h"
#include "usb.h"
#include "rx.h"
#include "tx.h"

static int r92su_sync_write(struct r92su *r92su, __le16 address,
			    const void *data, u16 size)
{
	struct usb_device *udev = r92su->udev;
	void *dma_data;
	u16 *_address = (void *) &address;
	int ret;

	if (!r92su_is_probing(r92su))
		return -EINVAL;

	dma_data = kmemdup(data, size, GFP_ATOMIC);
	if (!dma_data)
		return -ENOMEM;

	ret = usb_control_msg(udev, usb_sndctrlpipe(udev, RTL8712_EP_CTRL),
		REALTEK_USB_VENQT_CMD_REQ, REALTEK_USB_VENQT_WRITE, *_address,
		REALTEK_USB_VENQT_CMD_IDX, dma_data, size,
		USB_CTRL_SET_TIMEOUT);

	kfree(dma_data);
	return (ret < 0) ? ret : ((ret == size) ? 0 : -EMSGSIZE);
}

static int r92su_sync_read(struct r92su *r92su, __le16 address,
			   void *data, u16 size)
{
	struct usb_device *udev = r92su->udev;
	u16 *_address = (void *) &address;
	void *dma_data;
	int ret;

	if (!r92su_is_probing(r92su))
		return -EINVAL;

	dma_data = kmalloc(size, GFP_ATOMIC);
	if (!dma_data)
		return -ENOMEM;

	ret = usb_control_msg(udev, usb_rcvctrlpipe(udev, RTL8712_EP_CTRL),
		REALTEK_USB_VENQT_CMD_REQ, REALTEK_USB_VENQT_READ, *_address,
		REALTEK_USB_VENQT_CMD_IDX, dma_data, size,
		USB_CTRL_GET_TIMEOUT);
	if (ret > 0)
		memcpy(data, dma_data, ret);

	kfree(dma_data);
	return (ret < 0) ? ret : ((ret == size) ? 0 : -EMSGSIZE);
}

static void r92su_read_helper(struct r92su *r92su, const u16 address,
			      void *data, const u16 size)
{
	int ret;
	ret = r92su_sync_read(r92su, cpu_to_le16(address), data, size);
	WARN_ONCE(ret, "unable to read %d bytes from address:0x%x (%d).",
		      size, address, ret);

	if (ret)
		r92su_mark_dead(r92su);
}

u8 r92su_read8(struct r92su *r92su, const u32 address)
{
	u8 data;
	r92su_read_helper(r92su, address, &data, sizeof(data));
	return data;
}

u16 r92su_read16(struct r92su *r92su, const u32 address)
{
	__le16 data;
	r92su_read_helper(r92su, address, &data, sizeof(data));
	return le16_to_cpu(data);
}

u32 r92su_read32(struct r92su *r92su, const u32 address)
{
	__le32 data;
	r92su_read_helper(r92su, address, &data, sizeof(data));
	return le32_to_cpu(data);
}

static void r92su_write_helper(struct r92su *r92su, const u16 address,
			       const void *data, const u16 size)
{
	int ret;
	ret = r92su_sync_write(r92su, cpu_to_le16(address), data, size);
	WARN_ONCE(ret, "unable to write %d bytes to address:0x%x (%d).",
		size, address, ret);

	if (ret)
		r92su_mark_dead(r92su);
}

void r92su_write8(struct r92su *r92su, const u32 address, const u8 data)
{
	u8 tmp = data;
	r92su_write_helper(r92su, address, &tmp, sizeof(tmp));
}

void r92su_write16(struct r92su *r92su, const u32 address, const u16 data)
{
	__le16 tmp = cpu_to_le16(data);
	r92su_write_helper(r92su, address, &tmp, sizeof(tmp));
}

void r92su_write32(struct r92su *r92su, const u32 address, const u32 data)
{
	__le32 tmp = cpu_to_le32(data);
	r92su_write_helper(r92su, address, &tmp, sizeof(tmp));
}

static void r92su_tx_schedule(struct r92su *r92su)
{
	struct urb *urb;
	int err;

	if (atomic_inc_return(&r92su->tx_pending_urbs) >
	    RTL_USB_MAX_TX_URBS_NUM)
		goto err_acc;

	urb = usb_get_from_anchor(&r92su->tx_wait);
	if (!urb)
		goto err_acc;

	usb_anchor_urb(urb, &r92su->tx_submitted);
	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err) {
		WARN_ONCE(err, "can't handle urb submit error %d", err);

		usb_unanchor_urb(urb);
		r92su_mark_dead(r92su);

		dev_kfree_skb_any(urb->context);
	}

	usb_free_urb(urb);
	if (likely(err == 0))
		return;

err_acc:
	atomic_dec(&r92su->tx_pending_urbs);
}

static void r92su_tx_usb_cb(struct urb *urb)
{
	struct r92su *r92su = usb_get_intfdata(usb_ifnum_to_if(urb->dev, 0));
	struct sk_buff *skb = urb->context;

	atomic_dec(&r92su->tx_pending_urbs);

	r92su_tx_cb(r92su, skb);

	dev_kfree_skb_any(skb);

	switch (urb->status) {
	case 0:
		break;

	case -ENOENT:
	case -ECONNRESET:
	case -ENODEV:
	case -ESHUTDOWN:
		break;

	default:
		WARN_ONCE(urb->status, "unhandled urb status %d", urb->status);
		r92su_mark_dead(r92su);
		break;
	}

	r92su_tx_schedule(r92su);
}

static void r92su_rx_usb_cb(struct urb *urb)
{
	struct r92su *r92su = usb_get_intfdata(usb_ifnum_to_if(urb->dev, 0));
	void *buf = urb->context;
	int err;

	switch (urb->status) {
	case 0:
		break;

	case -ENOENT:
	case -ECONNRESET:
	case -ENODEV:
	case -ESHUTDOWN:
		goto err_out;

	default:
		WARN_ONCE(urb->status, "unhandled urb status %d", urb->status);
		r92su_mark_dead(r92su);
		goto err_dead;
	}

	if (!r92su_is_probing(r92su))
		goto err_dead;

	r92su_rx(r92su, buf, urb->actual_length);
	usb_anchor_urb(urb, &r92su->rx_submitted);
	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err) {
		WARN_ONCE(err, "can't handle urb submit error %d", err);
		usb_unanchor_urb(urb);
		goto err_dead;
	}
	return;

err_dead:
	r92su_mark_dead(r92su);
err_out:
	kfree(buf);
	return;
}

static struct urb *r92su_get_rx_urb(struct r92su *r92su, gfp_t flag)
{
	struct urb *urb = NULL;
	void *buf;

	buf = kmalloc(RTL92SU_SIZE_MAX_RX_BUFFER, flag);
	if (!buf)
		goto err_nomem;

	urb = usb_alloc_urb(0, flag);
	if (!urb)
		goto err_nomem;

	usb_fill_bulk_urb(urb, r92su->udev,
			  usb_rcvbulkpipe(r92su->udev, RTL8712_EP_RX),
			  buf, RTL92SU_SIZE_MAX_RX_BUFFER,
			  r92su_rx_usb_cb, buf);
	return urb;

err_nomem:
	kfree(buf);
	usb_free_urb(urb);
	return ERR_PTR(-ENOMEM);
}

static const unsigned int ep4_map[__RTL8712_LAST] = {
	[RTL8712_BKQ] = RTL8712_EP_TX6,
	[RTL8712_BEQ] = RTL8712_EP_TX6,
	[RTL8712_VIQ] = RTL8712_EP_TX4,
	[RTL8712_VOQ] = RTL8712_EP_TX4,
	[RTL8712_H2CCMD] = RTL8712_EP_TX13,
	[RTL8712_BCNQ] = RTL8712_EP_TX13,
	[RTL8712_BMCQ] = RTL8712_EP_TX13,
	[RTL8712_MGTQ] = RTL8712_EP_TX13,
	[RTL8712_RX0FF] = RTL8712_EP_RX,
	[RTL8712_C2HCMD] = RTL8712_EP_RX,
};

static unsigned int r92su_usb_get_pipe(struct r92su *r92su,
				       enum rtl8712_queues_t queue)
{
	BUILD_BUG_ON(ARRAY_SIZE(ep4_map) != __RTL8712_LAST);

	return ep4_map[queue];
}

int r92su_usb_tx(struct r92su *r92su, struct sk_buff *skb,
		 const enum rtl8712_queues_t queue)
{
	struct urb *urb;
	int err = -EINVAL;
	unsigned int pipe;

	if (!r92su_is_probing(r92su))
		goto err_out;

	urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (!urb) {
		err = -ENOMEM;
		goto err_out;
	}

	pipe = r92su_usb_get_pipe(r92su, queue);
	usb_fill_bulk_urb(urb, r92su->udev, usb_sndbulkpipe(r92su->udev, pipe),
			  skb->data, skb->len, r92su_tx_usb_cb, skb);

	urb->transfer_flags |= URB_ZERO_PACKET;

	usb_anchor_urb(urb, &r92su->tx_wait);
	usb_free_urb(urb);
	r92su_tx_schedule(r92su);
	return 0;

err_out:
	dev_kfree_skb_any(skb);
	return err;
}

static int r92su_usb_init(struct r92su *r92su)
{
	struct urb *urb;
	int i, err = -EINVAL;

	for (i = 0; i < RTL_USB_MAX_RX_URBS_NUM; i++) {
		urb = r92su_get_rx_urb(r92su, GFP_KERNEL);
		if (IS_ERR(urb)) {
			err = PTR_ERR(urb);
			break;
		}

		usb_anchor_urb(urb, &r92su->rx_submitted);
		err = usb_submit_urb(urb, GFP_KERNEL);
		if (err) {
			WARN_ONCE(err, "can't handle urb submit error %d",
				  err);

			usb_unanchor_urb(urb);
			dev_kfree_skb_any(urb->context);
			r92su_mark_dead(r92su);
			break;
		}

		usb_free_urb(urb);
	}
	return err;
}

#define USB_VENDER_ID_REALTEK		0x0bda
static struct usb_device_id r92su_usb_product_ids[] = {
	/* RTL8188SU */
	/* Realtek */
	{USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8171)},
	{USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8173)},
	{USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8712)},
	{USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8713)},
	{USB_DEVICE(USB_VENDER_ID_REALTEK, 0xC512)},
	/* Abocom */
	{USB_DEVICE(0x07B8, 0x8188)},
	/* ASUS */
	{USB_DEVICE(0x0B05, 0x1786)},
	{USB_DEVICE(0x0B05, 0x1791)}, /* 11n mode disable */
	/* Belkin */
	{USB_DEVICE(0x050D, 0x945A)},
	/* Corega */
	{USB_DEVICE(0x07AA, 0x0047)},
	/* D-Link */
	{USB_DEVICE(0x2001, 0x3306)},
	{USB_DEVICE(0x07D1, 0x3306)}, /* 11n mode disable */
	/* Edimax */
	{USB_DEVICE(0x7392, 0x7611)},
	/* EnGenius */
	{USB_DEVICE(0x1740, 0x9603)},
	/* Hawking */
	{USB_DEVICE(0x0E66, 0x0016)},
	/* Hercules */
	{USB_DEVICE(0x06F8, 0xE034)},
	{USB_DEVICE(0x06F8, 0xE032)},
	/* Logitec */
	{USB_DEVICE(0x0789, 0x0167)},
	/* PCI */
	{USB_DEVICE(0x2019, 0xAB28)},
	{USB_DEVICE(0x2019, 0xED16)},
	/* Sitecom */
	{USB_DEVICE(0x0DF6, 0x0057)},
	{USB_DEVICE(0x0DF6, 0x0045)},
	{USB_DEVICE(0x0DF6, 0x0059)}, /* 11n mode disable */
	{USB_DEVICE(0x0DF6, 0x004B)},
	{USB_DEVICE(0x0DF6, 0x005B)},
	{USB_DEVICE(0x0DF6, 0x005D)},
	{USB_DEVICE(0x0DF6, 0x0063)},
	/* Sweex */
	{USB_DEVICE(0x177F, 0x0154)},
	/* Thinkware */
	{USB_DEVICE(0x0BDA, 0x5077)},
	/* Toshiba */
	{USB_DEVICE(0x1690, 0x0752)},
	/* - */
	{USB_DEVICE(0x20F4, 0x646B)},
	{USB_DEVICE(0x083A, 0xC512)},

/* RTL8191SU */
	/* Realtek */
	{USB_DEVICE(0x0BDA, 0x8172)},
	{USB_DEVICE(0x0BDA, 0x8192)},
	/* Amigo */
	{USB_DEVICE(0x0EB0, 0x9061)},
	/* ASUS/EKB */
	{USB_DEVICE(0x13D3, 0x3323)},
	{USB_DEVICE(0x13D3, 0x3311)}, /* 11n mode disable */
	{USB_DEVICE(0x13D3, 0x3342)},
	/* ASUS/EKBLenovo */
	{USB_DEVICE(0x13D3, 0x3333)},
	{USB_DEVICE(0x13D3, 0x3334)},
	{USB_DEVICE(0x13D3, 0x3335)}, /* 11n mode disable */
	{USB_DEVICE(0x13D3, 0x3336)}, /* 11n mode disable */
	/* ASUS/Media BOX */
	{USB_DEVICE(0x13D3, 0x3309)},
	/* Belkin */
	{USB_DEVICE(0x050D, 0x815F)},
	/* D-Link */
	{USB_DEVICE(0x07D1, 0x3302)},
	{USB_DEVICE(0x07D1, 0x3300)},
	{USB_DEVICE(0x07D1, 0x3303)},
	/* Edimax */
	{USB_DEVICE(0x7392, 0x7612)},
	/* EnGenius */
	{USB_DEVICE(0x1740, 0x9605)},
	/* Guillemot */
	{USB_DEVICE(0x06F8, 0xE031)},
	/* Hawking */
	{USB_DEVICE(0x0E66, 0x0015)},
	/* Mediao */
	{USB_DEVICE(0x13D3, 0x3306)},
	/* PCI */
	{USB_DEVICE(0x2019, 0xED18)},
	{USB_DEVICE(0x2019, 0x4901)},
	/* Sitecom */
	{USB_DEVICE(0x0DF6, 0x0058)},
	{USB_DEVICE(0x0DF6, 0x0049)},
	{USB_DEVICE(0x0DF6, 0x004C)},
	{USB_DEVICE(0x0DF6, 0x0064)},
	/* Skyworth */
	{USB_DEVICE(0x14b2, 0x3300)},
	{USB_DEVICE(0x14b2, 0x3301)},
	{USB_DEVICE(0x14B2, 0x3302)},
	/* - */
	{USB_DEVICE(0x04F2, 0xAFF2)},
	{USB_DEVICE(0x04F2, 0xAFF5)},
	{USB_DEVICE(0x04F2, 0xAFF6)},
	{USB_DEVICE(0x13D3, 0x3339)},
	{USB_DEVICE(0x13D3, 0x3340)}, /* 11n mode disable */
	{USB_DEVICE(0x13D3, 0x3341)}, /* 11n mode disable */
	{USB_DEVICE(0x13D3, 0x3310)},
	{USB_DEVICE(0x13D3, 0x3325)},

/* RTL8192SU */
	/* Realtek */
	{USB_DEVICE(0x0BDA, 0x8174)},
	/* Belkin */
	{USB_DEVICE(0x050D, 0x845A)},
	/* Corega */
	{USB_DEVICE(0x07AA, 0x0051)},
	/* Edimax */
	{USB_DEVICE(0x7392, 0x7622)},
	/* NEC */
	{USB_DEVICE(0x0409, 0x02B6)},

	{ },
};

static int r92su_usb_probe(struct usb_interface *intf,
			   const struct usb_device_id *id)
{
	struct r92su *r92su;
	int err;

	r92su = r92su_alloc(&intf->dev);
	if (IS_ERR(r92su))
		return PTR_ERR(r92su);

	r92su_set_state(r92su, R92SU_PROBE);

	r92su->udev = interface_to_usbdev(intf);

	usb_set_intfdata(intf, r92su);

	init_usb_anchor(&r92su->rx_submitted);
	init_usb_anchor(&r92su->tx_submitted);
	init_usb_anchor(&r92su->tx_wait);

	err = r92su_usb_init(r92su);
	if (err)
		goto err_out;

	err = r92su_setup(r92su);
	if (err)
		goto err_out;

	err = r92su_register(r92su);
	if (err)
		goto err_out;

	return 0;

err_out:
	r92su_unalloc(r92su);
	return err;
}

static void r92su_usb_disconnect(struct usb_interface *intf)
{
	struct r92su *r92su = usb_get_intfdata(intf);
	struct urb *urb;

	r92su_mark_dead(r92su);

	usb_poison_anchored_urbs(&r92su->tx_submitted);
	usb_poison_anchored_urbs(&r92su->rx_submitted);
	while ((urb = usb_get_from_anchor(&r92su->tx_wait))) {
		kfree_skb(urb->context);
		usb_free_urb(urb);
	}

	r92su_unalloc(r92su);
}

static int r92su_usb_suspend(struct usb_interface *pusb_intf,
			     pm_message_t message)
{
	return 0;
}

void r92su_usb_prepare_firmware(struct r92su *r92su)
{
	struct fw_priv *dmem = &r92su->fw_dmem;

	dmem->hci_sel = RTL8712_HCI_TYPE_72USB;
	dmem->usb_ep_num = 4;
}

static int r92su_usb_resume(struct usb_interface *pusb_intf)
{
	return 0;
}

static struct usb_driver r92su_driver = {
	.name		= R92SU_DRVNAME,
	.id_table	= r92su_usb_product_ids,
	.probe		= r92su_usb_probe,
	.disconnect	= r92su_usb_disconnect,
	.suspend	= r92su_usb_suspend,
	.resume		= r92su_usb_resume,

	.soft_unbind = 1,
	.disable_hub_initiated_lpm = 1,
};

module_usb_driver(r92su_driver);

MODULE_DEVICE_TABLE(usb, r92su_usb_product_ids);
MODULE_FIRMWARE(RTL8192SU_FIRMWARE);
MODULE_AUTHOR("Christian Lamparter <chunkeey@googlemail.com>");
MODULE_LICENSE("GPL");

