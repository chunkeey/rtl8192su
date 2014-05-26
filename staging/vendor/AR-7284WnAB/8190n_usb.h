
#ifndef _8190N_USB_H_
#define _8190N_USB_H_

#include "../../core/hcd.h" //(for mips16 __delay issue, must disable)


#define usb_control_msg new_usb_control_msg

#ifdef CONFIG_USB_EHCI_HCD

struct ehci_hcd {			/* one per controller */
	/* glue to PCI and HCD framework */
	struct ehci_caps __iomem *caps;
	struct ehci_regs __iomem *regs;
	struct ehci_dbg_port __iomem *debug;

	__u32			hcs_params;	/* cached register copy */
	spinlock_t		lock;

	/* async schedule support */
	struct ehci_qh		*async;
	struct ehci_qh		*reclaim;
	unsigned		reclaim_ready : 1;
	unsigned		scanning : 1;

	/* periodic schedule support */
#define	DEFAULT_I_TDPS		1024		/* some HCs can do less */
	unsigned		periodic_size;
	__le32			*periodic;	/* hw periodic table */
	dma_addr_t		periodic_dma;
	unsigned		i_thresh;	/* uframes HC might cache */

	union ehci_shadow	*pshadow;	/* mirror hw periodic table */
	int			next_uframe;	/* scan periodic, start here */
	unsigned		periodic_sched;	/* periodic activity count */

	/* per root hub port */
	unsigned long		reset_done [15];

	/* per-HC memory pools (could be per-bus, but ...) */
	struct dma_pool		*qh_pool;	/* qh per active urb */
	struct dma_pool		*qtd_pool;	/* one or more per qh */
	struct dma_pool		*itd_pool;	/* itd per iso urb */
	struct dma_pool		*sitd_pool;	/* sitd per split iso urb */

	struct timer_list	watchdog;
	unsigned long		actions;
	unsigned		stamp;
	unsigned long		next_statechange;
	u32			command;

	/* SILICON QUIRKS */
	unsigned		is_tdi_rh_tt:1;	/* TDI roothub with TT */
	unsigned		no_selective_suspend:1;
	unsigned		has_fsl_port_bug:1; /* FreeScale */

	u8			sbrn;		/* packed release number */

	/* irq statistics */
#ifdef EHCI_STATS
	struct ehci_stats	stats;
	#define COUNT(x) do { (x)++; } while (0)
#else
	#define COUNT(x) do {} while (0)
#endif
};

#endif

#ifdef CONFIG_USB_OTG_Driver
//#error "1"
extern struct usb_hcd *usbhcd;	
#endif

#ifdef CONFIG_USB_OTG_HOST_RTL8672
#include "../../otg/dwc_otg_regs.h"
#include "../../otg/dwc_otg_cil_def.h"
#include "../../otg/dwc_otg_hcd_def.h"

int32_t dwc_otg_hcd_handle_intr (dwc_otg_hcd_t *_dwc_otg_hcd);
#endif
void RTL_W8SU(struct rtl8190_priv *priv, unsigned int indx, unsigned char data);
void RTL_W16SU(struct rtl8190_priv *priv, unsigned int indx, unsigned short data);
void RTL_W32SU(struct rtl8190_priv *priv, unsigned int indx, unsigned int data);
unsigned char RTL_R8SU(struct rtl8190_priv *priv, unsigned  int indx);
unsigned short RTL_R16SU(struct rtl8190_priv *priv, unsigned  int indx);
unsigned int RTL_R32SU(struct rtl8190_priv *priv, unsigned int indx);
unsigned int RTL_R32BaseBand(struct rtl8190_priv *priv, unsigned int indx);
void usb_receive_all_pkts(struct rtl8190_priv *priv);
void usb_poll(struct usb_bus *bus);


#endif
