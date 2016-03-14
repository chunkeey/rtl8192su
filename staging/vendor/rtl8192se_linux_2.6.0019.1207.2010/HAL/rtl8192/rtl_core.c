/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
 *
 * Based on the r8180 driver, which is:
 * Copyright 2004-2005 Andrea Merello <andreamrl@tiscali.it>, et al.
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
******************************************************************************/
#undef LOOP_TEST
#undef RX_DONT_PASS_UL
#undef DEBUG_EPROM
#undef DEBUG_RX_VERBOSE
#undef DUMMY_RX
#undef DEBUG_ZERO_RX
#undef DEBUG_RX_SKB
#undef DEBUG_TX_FRAG
#undef DEBUG_RX_FRAG
#undef DEBUG_TX_FILLDESC
#undef DEBUG_TX
#undef DEBUG_IRQ
#undef DEBUG_RX
#undef DEBUG_RXALLOC
#undef DEBUG_REGISTERS
#undef DEBUG_RING
#undef DEBUG_IRQ_TASKLET
#undef DEBUG_TX_ALLOC
#undef DEBUG_TX_DESC

#include <asm/uaccess.h>
#include <linux/pci.h>
#include "rtl_core.h"

#include "rtl_wx.h"
#ifndef RTL8192CE
#include "rtl_dm.h"
#endif

#ifdef CONFIG_PM_RTL
#include "rtl_pm.h"
#endif

#ifdef _RTL8192_EXT_PATCH_
#include "../../mshclass/msh_class.h"
#include "rtl_mesh.h"
#endif

#ifdef ASL
#include "rtl_softap.h"
#endif

#ifdef RTL8192S_WAPI_SUPPORT
#include "../../rtllib/wapi.h"
#include "../../rtllib/wapi_interface.h"
#endif

int hwwep = 1; 
static int channels = 0x3fff;
#ifdef _RTL8192_EXT_PATCH_
#ifdef ASL
char* ifname = "ra0";
#else
char* ifname = "ra%d";
#endif
#else
#ifdef ASL
char* ifname = "wlan0";
#else
char* ifname = "wlan%d";
#endif
#endif


#ifdef RTL8192SE
struct rtl819x_ops rtl8192se_ops = {
	.nic_type = NIC_8192SE,
	.get_eeprom_size 			= rtl8192se_get_eeprom_size,
	.init_adapter_variable 		= rtl8192se_InitializeVariables,
	.init_before_adapter_start 	= MacConfigBeforeFwDownload,
	.initialize_adapter 		= rtl8192se_adapter_start,
	.link_change 			= rtl8192se_link_change,
#ifdef ASL
	.ap_link_change		= rtl8192se_ap_link_change,
#endif
	.tx_fill_descriptor 		= rtl8192se_tx_fill_desc,
	.tx_fill_cmd_descriptor 	= rtl8192se_tx_fill_cmd_desc,
	.rx_query_status_descriptor 	= rtl8192se_rx_query_status_desc,
	.rx_command_packet_handler = rtl8192se_RxCommandPacketHandle,
	.stop_adapter 				= rtl8192se_halt_adapter,
	.update_ratr_table 			= rtl8192se_update_ratr_table,
	.irq_enable				= rtl8192se_EnableInterrupt,
	.irq_disable				= rtl8192se_DisableInterrupt,
	.irq_clear					= rtl8192se_ClearInterrupt,
	.rx_enable				= rtl8192se_enable_rx,
	.tx_enable				= rtl8192se_enable_tx,
	.interrupt_recognized		= rtl8192se_interrupt_recognized,
	.TxCheckStuckHandler		= rtl8192se_HalTxCheckStuck,
	.RxCheckStuckHandler		= rtl8192se_HalRxCheckStuck,
#ifdef ASL
	.rtl819x_hwconfig			= HwConfigureRTL8192SE,
#endif
};
#elif defined RTL8192CE
struct rtl819x_ops rtl8192ce_ops = {
	.nic_type 					= NIC_8192CE,
	.get_eeprom_size 			= rtl8192ce_get_eeprom_size,
	.init_adapter_variable 		= rtl8192ce_InitializeVariables,
	.initialize_adapter 			= rtl8192ce_adapter_start,
	.link_change 				= rtl8192ce_link_change,
	.tx_fill_descriptor 			= rtl8192ce_tx_fill_desc,
	.tx_fill_cmd_descriptor 		= rtl8192ce_tx_fill_cmd_desc,
	.rx_query_status_descriptor 	= rtl8192ce_rx_query_status_desc,
	.rx_command_packet_handler = NULL,
	.stop_adapter 				= rtl8192ce_halt_adapter,
	.update_ratr_table 			= rtl8192ce_UpdateHalRATRTable,
	.irq_enable				= rtl8192ce_EnableInterrupt,
	.irq_disable				= rtl8192ce_DisableInterrupt,
	.irq_clear					= rtl8192ce_ClearInterrupt,
	.rx_enable				= rtl8192ce_enable_rx,
	.tx_enable				= rtl8192ce_enable_tx,
	.interrupt_recognized		= rtl8192ce_interrupt_recognized,
	.TxCheckStuckHandler		= rtl8192ce_HalTxCheckStuck,
	.RxCheckStuckHandler		= rtl8192ce_HalRxCheckStuck,
#ifdef ASL
	.rtl819x_hwconfig			= rtl8192ce_HwConfigure,
#endif
};
#else
struct rtl819x_ops rtl819xp_ops = {
#ifdef RTL8192E
	.nic_type 					= NIC_8192E,
#elif defined RTL8190P
	.nic_type 					= NIC_8190P,
#endif
	.get_eeprom_size 			= rtl8192_get_eeprom_size,
	.init_adapter_variable 		= rtl8192_InitializeVariables,
	.initialize_adapter 			= rtl8192_adapter_start,
	.link_change 				= rtl8192_link_change,
	.tx_fill_descriptor 			= rtl8192_tx_fill_desc,
	.tx_fill_cmd_descriptor 		= rtl8192_tx_fill_cmd_desc,
	.rx_query_status_descriptor 	= rtl8192_rx_query_status_desc,
	.rx_command_packet_handler = NULL,
	.stop_adapter 				= rtl8192_halt_adapter,
	.update_ratr_table 			= rtl8192_update_ratr_table,
	.irq_enable				= rtl8192_EnableInterrupt,
	.irq_disable				= rtl8192_DisableInterrupt,
	.irq_clear					= rtl8192_ClearInterrupt,
	.rx_enable				= rtl8192_enable_rx,
	.tx_enable				= rtl8192_enable_tx,
	.interrupt_recognized		= rtl8192_interrupt_recognized,
	.TxCheckStuckHandler		= rtl8192_HalTxCheckStuck,
	.RxCheckStuckHandler		= rtl8192_HalRxCheckStuck,
#ifdef ASL
	.rtl819x_hwconfig			= rtl8192_hwconfig,
#endif
};
#endif

static struct pci_device_id rtl8192_pci_id_tbl[] __devinitdata = {
#ifdef RTL8190P
	/* Realtek */
	/* Dlink */
	{RTL_PCI_DEVICE(0x10ec, 0x8190, rtl819xp_ops)},
	/* Corega */
	{RTL_PCI_DEVICE(0x07aa, 0x0045, rtl819xp_ops)},
	{RTL_PCI_DEVICE(0x07aa, 0x0046, rtl819xp_ops)},
#elif defined(RTL8192E)
	{RTL_PCI_DEVICE(0x10ec, 0x8192, rtl819xp_ops)},
	{RTL_PCI_DEVICE(0x07aa, 0x0044, rtl819xp_ops)},
	{RTL_PCI_DEVICE(0x07aa, 0x0047, rtl819xp_ops)},
#elif defined(RTL8192SE)	/*8192SE*/
	/* take care of auto load fail case */
	{RTL_PCI_DEVICE(0x10ec, 0x8192, rtl8192se_ops)},
	{RTL_PCI_DEVICE(0x10ec, 0x8171, rtl8192se_ops)},
	{RTL_PCI_DEVICE(0x10ec, 0x8172, rtl8192se_ops)},
	{RTL_PCI_DEVICE(0x10ec, 0x8173, rtl8192se_ops)},
	{RTL_PCI_DEVICE(0x10ec, 0x8174, rtl8192se_ops)},
#elif defined(RTL8192CE)	/*8192CE*/
	{RTL_PCI_DEVICE(0x10ec, 0x8191, rtl8192ce_ops)},
	{RTL_PCI_DEVICE(0x10ec, 0x8178, rtl8192ce_ops)},
	{RTL_PCI_DEVICE(0x10ec, 0x8177, rtl8192ce_ops)},
	{RTL_PCI_DEVICE(0x10ec, 0x8176, rtl8192ce_ops)},
	{RTL_PCI_DEVICE(0x10ec, 0x092D, rtl8192ce_ops)},
#endif
	{}
};
MODULE_DEVICE_TABLE(pci, rtl8192_pci_id_tbl);

static int __devinit rtl8192_pci_probe(struct pci_dev *pdev,
			 const struct pci_device_id *id);
static void __devexit rtl8192_pci_disconnect(struct pci_dev *pdev);

static struct pci_driver rtl8192_pci_driver = {
	.name		= DRV_NAME,	          /* Driver name   */
	.id_table	= rtl8192_pci_id_tbl,	          /* PCI_ID table  */
	.probe		= rtl8192_pci_probe,	          /* probe fn      */
	.remove		= __devexit_p(rtl8192_pci_disconnect),	  /* remove fn     */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 5, 0)
#ifdef CONFIG_PM_RTL
	.suspend	= rtl8192E_suspend,	          /* PM suspend fn */
	.resume		= rtl8192E_resume,                 /* PM resume fn  */
#else
	.suspend	= NULL,			          /* PM suspend fn */
	.resume      	= NULL,			          /* PM resume fn  */
#endif
#endif
};

/****************************************************************************
   -----------------------------IO STUFF-------------------------
*****************************************************************************/
bool
PlatformIOCheckPageLegalAndGetRegMask(
	u32 	u4bPage,
	u8*	pu1bPageMask
)
{
	bool		bReturn = false;
	*pu1bPageMask = 0xfe;

	switch(u4bPage)
	{
		case 1: case 2: case 3: case 4:	
		case 8: case 9: case 10: case 12: case 13:	
			bReturn = true;
			*pu1bPageMask = 0xf0;
			break;
			
		default:
			bReturn = false;
			break;
	}

	return bReturn;
}

void write_nic_io_byte(struct net_device *dev, int x,u8 y)
{
	u32 u4bPage = (x >> 8);
	u8 u1PageMask = 0;
	bool	bIsLegalPage = false;

	if(u4bPage == 0)
	{
		outb(y&0xff,dev->base_addr +x);
	}else
	{
		bIsLegalPage = PlatformIOCheckPageLegalAndGetRegMask(u4bPage, &u1PageMask);
		if(bIsLegalPage)
		{
			u8 u1bPsr = read_nic_io_byte(dev, PSR);

			write_nic_io_byte(dev, PSR, ((u1bPsr & u1PageMask) | (u8)u4bPage)); 
			write_nic_io_byte(dev, (x & 0xff), y);
			write_nic_io_byte(dev, PSR, (u1bPsr & u1PageMask)); 
			
		}else
		{
			;
		}
	}
	
        
}

void write_nic_io_word(struct net_device *dev, int x,u16 y)
{
	u32 u4bPage = (x >> 8);
	u8 u1PageMask = 0;
	bool	bIsLegalPage = false;

	if(u4bPage == 0)
	{
		outw(y,dev->base_addr +x);
	}else
	{
		bIsLegalPage = PlatformIOCheckPageLegalAndGetRegMask(u4bPage, &u1PageMask);
		if(bIsLegalPage)
		{
			u8 u1bPsr = read_nic_io_byte(dev, PSR);

			write_nic_io_byte(dev, PSR, ((u1bPsr & u1PageMask) | (u8)u4bPage)); 
			write_nic_io_word(dev, (x & 0xff), y);
			write_nic_io_byte(dev, PSR, (u1bPsr & u1PageMask)); 
			
		}else
		{
			;
		}
	}
	        
}

void write_nic_io_dword(struct net_device *dev, int x,u32 y)
{
	u32 u4bPage = (x >> 8);
	u8 u1PageMask = 0;
	bool	bIsLegalPage = false;

	if(u4bPage == 0)
	{
		outl(y,dev->base_addr +x);
	}else
	{
		bIsLegalPage = PlatformIOCheckPageLegalAndGetRegMask(u4bPage, &u1PageMask);
		if(bIsLegalPage)
		{
			u8 u1bPsr = read_nic_io_byte(dev, PSR);

			write_nic_io_byte(dev, PSR, ((u1bPsr & u1PageMask) | (u8)u4bPage)); 
			write_nic_io_dword(dev, (x & 0xff), y);
			write_nic_io_byte(dev, PSR, (u1bPsr & u1PageMask)); 
			
		}else
		{
			;
		}
	}
	        
}
u8 read_nic_io_byte(struct net_device *dev, int x) 
{
	u32 u4bPage = (x >> 8);
	u8 u1PageMask = 0;
	bool	bIsLegalPage = false;
	u8	Data = 0;

	if(u4bPage == 0)
	{
		return 0xff&inb(dev->base_addr +x);
	}else
	{
		bIsLegalPage = PlatformIOCheckPageLegalAndGetRegMask(u4bPage, &u1PageMask);
		if(bIsLegalPage)
		{
			u8 u1bPsr = read_nic_io_byte(dev, PSR);

			write_nic_io_byte(dev, PSR, ((u1bPsr & u1PageMask) | (u8)u4bPage)); 
			Data = read_nic_io_byte(dev, (x & 0xff));
			write_nic_io_byte(dev, PSR, (u1bPsr & u1PageMask)); 
			
		}else
		{
			;
		}
	}
	
	return Data;
}

u16 read_nic_io_word(struct net_device *dev, int x) 
{
	u32 u4bPage = (x >> 8);
	u8 u1PageMask = 0;
	bool	bIsLegalPage = false;
	u16	Data = 0;

	if(u4bPage == 0)
	{
		return inw(dev->base_addr +x);
	}else
	{
		bIsLegalPage = PlatformIOCheckPageLegalAndGetRegMask(u4bPage, &u1PageMask);
		if(bIsLegalPage)
		{
			u8 u1bPsr = read_nic_io_byte(dev, PSR);

			write_nic_io_byte(dev, PSR, ((u1bPsr & u1PageMask) | (u8)u4bPage)); 
			Data = read_nic_io_word(dev, (x & 0xff));
			write_nic_io_byte(dev, PSR, (u1bPsr & u1PageMask)); 
			
		}else
		{
			;
		}
	}
	        
	return Data;
}

u32 read_nic_io_dword(struct net_device *dev, int x) 
{
	u32 u4bPage = (x >> 8);
	u8 u1PageMask = 0;
	bool	bIsLegalPage = false;
	u32	Data = 0;

	if(u4bPage == 0)
	{
		return inl(dev->base_addr +x);
	}else
	{
		bIsLegalPage = PlatformIOCheckPageLegalAndGetRegMask(u4bPage, &u1PageMask);
		if(bIsLegalPage)
		{
			u8 u1bPsr = read_nic_io_byte(dev, PSR);

			write_nic_io_byte(dev, PSR, ((u1bPsr & u1PageMask) | (u8)u4bPage)); 
			Data = read_nic_io_dword(dev, (x & 0xff));
			write_nic_io_byte(dev, PSR, (u1bPsr & u1PageMask)); 
			
		}else
		{
			;
		}
	}
	
	return Data;
}

u8 read_nic_byte(struct net_device *dev, int x) 
{
#ifdef CONFIG_RTL8192_IO_MAP
	return read_nic_io_byte(dev, x);
#else
        return 0xff&readb((u8*)dev->mem_start +x);
#endif
}

u32 read_nic_dword(struct net_device *dev, int x) 
{
#ifdef CONFIG_RTL8192_IO_MAP
	return read_nic_io_dword(dev, x);
#else
        return readl((u8*)dev->mem_start +x);
#endif
}

u16 read_nic_word(struct net_device *dev, int x) 
{
#ifdef CONFIG_RTL8192_IO_MAP
	return read_nic_io_word(dev, x);
#else
        return readw((u8*)dev->mem_start +x);
#endif
}

void write_nic_byte(struct net_device *dev, int x,u8 y)
{
#ifdef CONFIG_RTL8192_IO_MAP
	write_nic_io_byte(dev, x, y);
#else
        writeb(y,(u8*)dev->mem_start +x);

#if !(defined RTL8192SE || defined RTL8192CE)	
	udelay(20);
#endif	

#if defined RTL8192CE
		read_nic_byte(dev, x);
#endif

#endif
}

void write_nic_dword(struct net_device *dev, int x,u32 y)
{
#ifdef CONFIG_RTL8192_IO_MAP
	write_nic_io_dword(dev, x, y);
#else
        writel(y,(u8*)dev->mem_start +x);

#if !(defined RTL8192SE || defined RTL8192CE)	
	udelay(20);
#endif	

#if defined RTL8192CE
		read_nic_dword(dev, x);
#endif

#endif
}

void write_nic_word(struct net_device *dev, int x,u16 y) 
{
#ifdef CONFIG_RTL8192_IO_MAP
	write_nic_io_word(dev, x, y);
#else
        writew(y,(u8*)dev->mem_start +x);

#if !(defined RTL8192SE || defined RTL8192CE)	
	udelay(20);
#endif	

#if defined RTL8192CE
		read_nic_word(dev, x);
#endif

#endif
}

/****************************************************************************
   -----------------------------GENERAL FUNCTION-------------------------
*****************************************************************************/
bool
MgntActSet_RF_State(
	struct net_device* dev,
	RT_RF_POWER_STATE	StateToSet,
	RT_RF_CHANGE_SOURCE ChangeSource,
	bool	ProtectOrNot
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device * ieee = priv->rtllib;
	bool 			bActionAllowed = false; 
	bool 			bConnectBySSID = false;
	RT_RF_POWER_STATE	rtState;
	u16			RFWaitCounter = 0;
	unsigned long flag;
	RT_TRACE((COMP_PS | COMP_RF), "===>MgntActSet_RF_State(): StateToSet(%d)\n",StateToSet);



	if(!ProtectOrNot)
	{
	while(true)
	{
		spin_lock_irqsave(&priv->rf_ps_lock,flag);
		if(priv->RFChangeInProgress)
		{
			spin_unlock_irqrestore(&priv->rf_ps_lock,flag);
			RT_TRACE((COMP_PS | COMP_RF), "MgntActSet_RF_State(): RF Change in progress! Wait to set..StateToSet(%d).\n", StateToSet);
			printk("MgntActSet_RF_State(): RF Change in progress! Wait to set..StateToSet(%d).\n", StateToSet);
			#if 1	
			while(priv->RFChangeInProgress)
			{
				RFWaitCounter ++;
				RT_TRACE((COMP_PS | COMP_RF), "MgntActSet_RF_State(): Wait 1 ms (%d times)...\n", RFWaitCounter);
				mdelay(1);

				if(RFWaitCounter > 100)
				{
					RT_TRACE(COMP_ERR, "MgntActSet_RF_State(): Wait too logn to set RF\n");
					return false;
				}
			}
			#endif
		}
		else
		{
			priv->RFChangeInProgress = true;
			spin_unlock_irqrestore(&priv->rf_ps_lock,flag);
			break;
		}
	}
	}

#if 0
	if(!priv->up)
	{

		if(!ProtectOrNot)
		{
			spin_lock_irqsave(&priv->rf_ps_lock,flag);
			priv->RFChangeInProgress = false;
			spin_unlock_irqrestore(&priv->rf_ps_lock,flag);
		}
		return false;
	}
#endif

	rtState = priv->rtllib->eRFPowerState;

	switch(StateToSet) 
	{
	case eRfOn:

		priv->rtllib->RfOffReason &= (~ChangeSource);

		if((ChangeSource == RF_CHANGE_BY_HW) && (priv->bHwRadioOff == true)){
			priv->bHwRadioOff = false;
		}

		if(! priv->rtllib->RfOffReason)
		{
			priv->rtllib->RfOffReason = 0;
			bActionAllowed = true;

			
			if(rtState == eRfOff && ChangeSource >=RF_CHANGE_BY_HW )
			{
				bConnectBySSID = true;
			}
		}
		else{
			RT_TRACE((COMP_PS | COMP_RF), "MgntActSet_RF_State - eRfon reject pMgntInfo->RfOffReason= 0x%x, ChangeSource=0x%X\n", priv->rtllib->RfOffReason, ChangeSource);
                }

		break;

	case eRfOff:

		if((priv->rtllib->iw_mode == IW_MODE_INFRA) || (priv->rtllib->iw_mode == IW_MODE_ADHOC))
		{
			if ((priv->rtllib->RfOffReason > RF_CHANGE_BY_IPS) || (ChangeSource > RF_CHANGE_BY_IPS))
			{
				if(ieee->state == RTLLIB_LINKED)
					priv->blinked_ingpio = true;
				else
					priv->blinked_ingpio = false;	
				rtllib_MgntDisconnect(priv->rtllib,disas_lv_ss);
	
	
				
			}
		}
		if((ChangeSource == RF_CHANGE_BY_HW) && (priv->bHwRadioOff == false)){
			priv->bHwRadioOff = true;
		}
		priv->rtllib->RfOffReason |= ChangeSource;
		bActionAllowed = true;
		break;

	case eRfSleep:
		priv->rtllib->RfOffReason |= ChangeSource;
		bActionAllowed = true;
		break;

	default:
		break;
	}

	if(bActionAllowed)
	{
		RT_TRACE((COMP_PS | COMP_RF), "MgntActSet_RF_State(): Action is allowed.... StateToSet(%d), RfOffReason(%#X)\n", StateToSet, priv->rtllib->RfOffReason);
		PHY_SetRFPowerState(dev, StateToSet);	
		if(StateToSet == eRfOn) 
		{				

			if(bConnectBySSID && (priv->blinked_ingpio == true))
			{				
				queue_delayed_work_rsl(ieee->wq, &ieee->associate_procedure_wq, 0);
				priv->blinked_ingpio = false;

			}
#ifdef _RTL8192_EXT_PATCH_
			if(priv->rtllib->iw_mode == IW_MODE_MESH){
				queue_work_rsl(ieee->wq, &ieee->ext_start_mesh_protocol_wq);

			}
#endif		
		}
		else if(StateToSet == eRfOff)
		{		
		}		
	}
	else
	{
		RT_TRACE((COMP_PS | COMP_RF), "MgntActSet_RF_State(): Action is rejected.... StateToSet(%d), ChangeSource(%#X), RfOffReason(%#X)\n", StateToSet, ChangeSource, priv->rtllib->RfOffReason);
	}

	if(!ProtectOrNot)
	{	
	spin_lock_irqsave(&priv->rf_ps_lock,flag);
	priv->RFChangeInProgress = false;
	spin_unlock_irqrestore(&priv->rf_ps_lock,flag);
	}
	
	RT_TRACE((COMP_PS && COMP_RF), "<===MgntActSet_RF_State()\n");
	return bActionAllowed;
}


short rtl8192_get_nic_desc_num(struct net_device *dev, int prio)
{
    struct r8192_priv *priv = rtllib_priv(dev);
    struct rtl8192_tx_ring *ring = &priv->tx_ring[prio];

    /* For now, we reserved two free descriptor as a safety boundary 
     * between the tail and the head 
     */
    if((prio == MGNT_QUEUE) &&(skb_queue_len(&ring->queue)>10))
    	printk("-----[%d]---------ring->idx=%d queue_len=%d---------\n",
			prio,ring->idx, skb_queue_len(&ring->queue));
    return skb_queue_len(&ring->queue);
}

short rtl8192_check_nic_enough_desc(struct net_device *dev, int prio)
{
    struct r8192_priv *priv = rtllib_priv(dev);
    struct rtl8192_tx_ring *ring = &priv->tx_ring[prio];

    if (ring->entries - skb_queue_len(&ring->queue) >= 2) {
        return 1;
    } else {
        return 0;
    }
}

void rtl8192_tx_timeout(struct net_device *dev)
{
    struct r8192_priv *priv = rtllib_priv(dev);

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
    schedule_work(&priv->reset_wq);
#else
    schedule_task(&priv->reset_wq);
#endif
    printk("TXTIMEOUT");
}

void rtl8192_irq_enable(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);	
	priv->irq_enabled = 1;
	
	priv->ops->irq_enable(dev);
}

void rtl8192_irq_disable(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);	

	priv->ops->irq_disable(dev);

	priv->irq_enabled = 0;
}

void rtl8192_irq_clear(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);	

	priv->ops->irq_clear(dev);
}


void rtl8192_set_chan(struct net_device *dev,short ch)
{
    struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);

    RT_TRACE(COMP_CH, "=====>%s()====ch:%d\n", __FUNCTION__, ch);	
    if (priv->chan_forced)
        return;

    priv->chan = ch;


#ifndef LOOP_TEST	


    if (priv->rf_set_chan)
        priv->rf_set_chan(dev,priv->chan);

#ifdef CONFIG_FW_SETCHAN
    priv->rtllib->SetFwCmdHandler(dev, FW_CMD_CHAN_SET);	
#endif

#endif
}
#ifdef ASL
void rtl8192_update_cap(struct net_device* dev, u16 cap, bool is_ap)
#else
void rtl8192_update_cap(struct net_device* dev, u16 cap)
#endif
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_network *net = NULL;

	
#ifdef ASL
	if (is_ap)
		net = &priv->rtllib->current_ap_network;
	else
#endif
		net = &priv->rtllib->current_network;
	{
		bool		ShortPreamble;
			
		if(cap & WLAN_CAPABILITY_SHORT_PREAMBLE)
		{ 
			if(priv->dot11CurrentPreambleMode != PREAMBLE_SHORT) 
			{
				ShortPreamble = true;
				priv->dot11CurrentPreambleMode = PREAMBLE_SHORT;
				priv->rtllib->SetHwRegHandler( dev, HW_VAR_ACK_PREAMBLE, (unsigned char *)&ShortPreamble );
			}
		}
		else
		{ 
			if(priv->dot11CurrentPreambleMode != PREAMBLE_LONG)  
			{
				ShortPreamble = false;
				priv->dot11CurrentPreambleMode = PREAMBLE_LONG;
				priv->rtllib->SetHwRegHandler( dev, HW_VAR_ACK_PREAMBLE, (unsigned char *)&ShortPreamble );
			}
		}
	}

#ifdef RTL8192CE
	if( net->mode & IEEE_G)
#elif defined RTL8192SE || defined RTL8192E || defined RTL8190P
	if (net->mode & (IEEE_G|IEEE_N_24G)) 
#endif
	{
		u8	slot_time_val;
		u8	CurSlotTime = priv->slot_time;
		
#ifdef RTL8192CE
		if( (cap & WLAN_CAPABILITY_SHORT_SLOT_TIME) && (!(priv->rtllib->pHTInfo->RT2RT_HT_Mode & RT_HT_CAP_USE_LONG_PREAMBLE)))
#elif defined RTL8192SE || defined RTL8192E || defined RTL8190P
		if ((cap & WLAN_CAPABILITY_SHORT_SLOT_TIME) && (!priv->rtllib->pHTInfo->bCurrentRT2RTLongSlotTime)) 
#endif
		{ 
			if(CurSlotTime != SHORT_SLOT_TIME)
			{
				slot_time_val = SHORT_SLOT_TIME;
				priv->rtllib->SetHwRegHandler( dev, HW_VAR_SLOT_TIME, &slot_time_val );
			}
		}
		else
		{ 
			if(CurSlotTime != NON_SHORT_SLOT_TIME)
			{
				slot_time_val = NON_SHORT_SLOT_TIME;
				priv->rtllib->SetHwRegHandler( dev, HW_VAR_SLOT_TIME, &slot_time_val );
			}
		}
	}
}

static struct rtllib_qos_parameters def_qos_parameters = {
        {3,3,3,3},
        {7,7,7,7},
        {2,2,2,2},
        {0,0,0,0},
        {0,0,0,0} 
};

void rtl8192_update_beacon(void *data)
{
#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,20)
	struct r8192_priv *priv = container_of_work_rsl(data, struct r8192_priv, update_beacon_wq.work);
	struct net_device *dev = priv->rtllib->dev;
#else
	struct net_device *dev = (struct net_device *)data;
        struct r8192_priv *priv = rtllib_priv(dev);
#endif
 	struct rtllib_device* ieee = priv->rtllib;
	struct rtllib_network* net = &ieee->current_network;

	if (ieee->pHTInfo->bCurrentHTSupport)
		HTUpdateSelfAndPeerSetting(ieee, net);
	ieee->pHTInfo->bCurrentRT2RTLongSlotTime = net->bssht.bdRT2RTLongSlotTime;
	ieee->pHTInfo->RT2RT_HT_Mode = net->bssht.RT2RT_HT_Mode;
#ifdef ASL
	rtl8192_update_cap(dev, net->capability, false);
#else
	rtl8192_update_cap(dev, net->capability);
#endif
}

#define MOVE_INTO_HANDLER
#ifdef RTL8192CE
int WDCAPARA_ADD[] = {REG_EDCA_BE_PARAM,REG_EDCA_BK_PARAM,REG_EDCA_VI_PARAM,REG_EDCA_VO_PARAM};
#else
int WDCAPARA_ADD[] = {EDCAPARA_BE,EDCAPARA_BK,EDCAPARA_VI,EDCAPARA_VO};
#endif
void rtl8192_qos_activate(void *data)
{
#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,20)
	struct r8192_priv *priv = container_of_work_rsl(data, struct r8192_priv, qos_activate);
	struct net_device *dev = priv->rtllib->dev;
#else
	struct net_device *dev = (struct net_device *)data;
	struct r8192_priv *priv = rtllib_priv(dev);
#endif
#ifndef MOVE_INTO_HANDLER
        struct rtllib_qos_parameters *qos_parameters = &priv->rtllib->current_network.qos_data.parameters;
        u8 mode = priv->rtllib->current_network.mode;
	u8  u1bAIFS;
	u32 u4bAcParam;
#endif
        int i;

        if (priv == NULL)
                return;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16))
	down(&priv->mutex);
#else
        mutex_lock(&priv->mutex);
#endif
#ifdef ASL
	if(priv->rtllib->state != RTLLIB_LINKED && priv->rtllib->ap_state != RTLLIB_LINKED)
#else
        if(priv->rtllib->state != RTLLIB_LINKED)
#endif
		goto success;
#ifdef ASL
#ifndef MOVE_INTO_HANDLER
	if (priv->rtllib->iw_mode == IW_MODE_MASTER || priv->rtllib->iw_mode == IW_MODE_APSTA) {
		qos_parameters = &priv->rtllib->current_ap_network.qos_data.parameters;
		mode = priv->rtllib->apmode;
	}
#endif
#endif
	RT_TRACE(COMP_QOS,"qos active process with associate response received\n");
	
	for (i = 0; i <  QOS_QUEUE_NUM; i++) {
#ifndef MOVE_INTO_HANDLER
#ifdef ASL
		if (priv->rtllib->iw_mode == IW_MODE_MASTER || priv->rtllib->iw_mode == IW_MODE_APSTA)
			u1bAIFS = qos_parameters->aifs[i] * ((mode&(IEEE_G|IEEE_N_24G)) ?9:20) + aAPSifsTime; 
		else
#endif
		u1bAIFS = qos_parameters->aifs[i] * ((mode&(IEEE_G|IEEE_N_24G)) ?9:20) + aSifsTime; 
		u4bAcParam = ((((u32)(qos_parameters->tx_op_limit[i]))<< AC_PARAM_TXOP_LIMIT_OFFSET)|
				(((u32)(qos_parameters->cw_max[i]))<< AC_PARAM_ECW_MAX_OFFSET)|
				(((u32)(qos_parameters->cw_min[i]))<< AC_PARAM_ECW_MIN_OFFSET)|
				((u32)u1bAIFS << AC_PARAM_AIFS_OFFSET));
		printk("===>ACI:%d:u4bAcParam:%x\n", i, u4bAcParam);
		write_nic_dword(dev, WDCAPARA_ADD[i], u4bAcParam);
#else 
		priv->rtllib->SetHwRegHandler(dev, HW_VAR_AC_PARAM, (u8*)(&i));
#endif
	}

success:
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16))
	up(&priv->mutex);
#else
        mutex_unlock(&priv->mutex);
#endif
}

static int rtl8192_qos_handle_probe_response(struct r8192_priv *priv,
		int active_network,
		struct rtllib_network *network)
{
	int ret = 0;
	u32 size = sizeof(struct rtllib_qos_parameters);

	if(priv->rtllib->state !=RTLLIB_LINKED)
                return ret;

#ifdef _RTL8192_EXT_PATCH_
	if (!((priv->rtllib->iw_mode == IW_MODE_INFRA ) || 
	      ((priv->rtllib->iw_mode == IW_MODE_MESH) && (priv->rtllib->only_mesh == 0))))
#else
	if ((priv->rtllib->iw_mode != IW_MODE_INFRA)
#ifdef ASL
		&& (priv->rtllib->iw_mode != IW_MODE_APSTA)
#endif
		) 
#endif
		return ret;

	if (network->flags & NETWORK_HAS_QOS_MASK) {
		if (active_network &&
				(network->flags & NETWORK_HAS_QOS_PARAMETERS))
			network->qos_data.active = network->qos_data.supported;

		if ((network->qos_data.active == 1) && (active_network == 1) &&
				(network->flags & NETWORK_HAS_QOS_PARAMETERS) &&
				(network->qos_data.old_param_count !=
				 network->qos_data.param_count)) {
			network->qos_data.old_param_count =
				network->qos_data.param_count;
                         priv->rtllib->wmm_acm = network->qos_data.wmm_acm;
			queue_work_rsl(priv->priv_wq, &priv->qos_activate);
			RT_TRACE (COMP_QOS, "QoS parameters change call "
					"qos_activate\n");
		}
	} else {
		memcpy(&priv->rtllib->current_network.qos_data.parameters,\
		       &def_qos_parameters, size);

		if ((network->qos_data.active == 1) && (active_network == 1)) {
			queue_work_rsl(priv->priv_wq, &priv->qos_activate);
			RT_TRACE(COMP_QOS, "QoS was disabled call qos_activate \n");
		}
		network->qos_data.active = 0;
		network->qos_data.supported = 0;
	}

	return 0;
}

static int rtl8192_handle_beacon(struct net_device * dev,
                              struct rtllib_beacon * beacon,
                              struct rtllib_network * network)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	rtl8192_qos_handle_probe_response(priv,1,network);

	queue_delayed_work_rsl(priv->priv_wq, &priv->update_beacon_wq, 0);
	return 0;

}

static int rtl8192_qos_association_resp(struct r8192_priv *priv,
                                    struct rtllib_network *network)
{
        int ret = 0;
        unsigned long flags;
        u32 size = sizeof(struct rtllib_qos_parameters);
        int set_qos_param = 0;

        if ((priv == NULL) || (network == NULL))
                return ret;

	if(priv->rtllib->state !=RTLLIB_LINKED)
                return ret;

#ifdef _RTL8192_EXT_PATCH_
	if (!((priv->rtllib->iw_mode == IW_MODE_INFRA ) || 
	      ((priv->rtllib->iw_mode == IW_MODE_MESH) && (priv->rtllib->only_mesh == 0))))
#else
#ifdef ASL
	if ((priv->rtllib->iw_mode != IW_MODE_INFRA) && (priv->rtllib->iw_mode != IW_MODE_APSTA)) 
#else
	if ((priv->rtllib->iw_mode != IW_MODE_INFRA)) 
#endif
#endif
                return ret;

        spin_lock_irqsave(&priv->rtllib->lock, flags);
	if (network->flags & NETWORK_HAS_QOS_PARAMETERS) {
		memcpy(&priv->rtllib->current_network.qos_data.parameters,\
			 &network->qos_data.parameters,\
			sizeof(struct rtllib_qos_parameters));
		priv->rtllib->current_network.qos_data.active = 1;
                priv->rtllib->wmm_acm = network->qos_data.wmm_acm;
#if 0
		if((priv->rtllib->current_network.qos_data.param_count != \
					network->qos_data.param_count))
#endif
		 {
                        set_qos_param = 1;
			priv->rtllib->current_network.qos_data.old_param_count = \
				 priv->rtllib->current_network.qos_data.param_count;
			priv->rtllib->current_network.qos_data.param_count = \
			     	 network->qos_data.param_count;
		}
        } else {
		memcpy(&priv->rtllib->current_network.qos_data.parameters,\
		       &def_qos_parameters, size);
		priv->rtllib->current_network.qos_data.active = 0;
		priv->rtllib->current_network.qos_data.supported = 0;
                set_qos_param = 1;
        }

        spin_unlock_irqrestore(&priv->rtllib->lock, flags);

	RT_TRACE(COMP_QOS, "%s: network->flags = %d,%d\n", __FUNCTION__,
			network->flags ,priv->rtllib->current_network.qos_data.active);
	if (set_qos_param == 1) {
		dm_init_edca_turbo(priv->rtllib->dev);
		queue_work_rsl(priv->priv_wq, &priv->qos_activate);
	}
        return ret;
}

static int rtl8192_handle_assoc_response(struct net_device *dev,
                                     struct rtllib_assoc_response_frame *resp,
                                     struct rtllib_network *network)
{
        struct r8192_priv *priv = rtllib_priv(dev);
        rtl8192_qos_association_resp(priv, network);
        return 0;
}

#if 0  
void rtl8192_prepare_beacon(struct r8192_priv *priv)
{
#ifdef _RTL8192_EXT_PATCH_
	struct net_device *dev = priv->rtllib->dev;
#endif
	struct sk_buff *skb;
	cb_desc *tcb_desc; 

#ifdef _RTL8192_EXT_PATCH_
	if((priv->rtllib->iw_mode == IW_MODE_MESH) && (priv->rtllib->mesh_state == RTLLIB_NOLINK))
		return;
#endif
	skb = rtllib_get_beacon(priv->rtllib);
	tcb_desc = (cb_desc *)(skb->cb + 8);
#ifdef _RTL8192_EXT_PATCH_
	memset(skb->cb, 0, sizeof(skb->cb));
#endif
	tcb_desc->queue_index = BEACON_QUEUE;
	tcb_desc->data_rate = 2;
	tcb_desc->RATRIndex = 7;
	tcb_desc->bTxDisableRateFallBack = 1;
	tcb_desc->bTxUseDriverAssingedRate = 1;
#ifdef _RTL8192_EXT_PATCH_
	tcb_desc->bTxEnableFwCalcDur = 0;
	memcpy((unsigned char *)(skb->cb),&dev,sizeof(dev));
#endif
	skb_push(skb, priv->rtllib->tx_headroom);	
	if(skb){
		rtl8192_tx(priv->rtllib->dev,skb);
	}
}
#else
void rtl8192_prepare_beacon(struct r8192_priv *priv)
{
	struct net_device *dev = priv->rtllib->dev;
	struct sk_buff *pskb = NULL, *pnewskb = NULL;
	cb_desc *tcb_desc = NULL; 
	struct rtl8192_tx_ring *ring = NULL;
	tx_desc *pdesc = NULL;

#ifdef _RTL8192_EXT_PATCH_
	if((priv->rtllib->iw_mode == IW_MODE_MESH) && (priv->rtllib->mesh_state == RTLLIB_NOLINK))
		return;
#endif
	ring = &priv->tx_ring[BEACON_QUEUE];
	pskb = __skb_dequeue(&ring->queue);
	if(pskb)
		kfree_skb(pskb);

	pnewskb = rtllib_get_beacon(priv->rtllib);
	if(!pnewskb)
		return;
#ifdef ASL
#ifdef ASL_DEBUG_2
	if (pnewskb) {
		int i;
		printk("\nSending Beacon+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
		printk("\nPACKET DUMP+++++++++++++++++++++++++++");
		printk("\n----------------------------------------\n");
		for(i = 0; i < pnewskb->len; i++)
			printk("%02X-", pnewskb->data[i] & 0xff);
		printk("\n----------------------------------------\n");
	}
#endif
#endif

#ifdef _RTL8192_EXT_PATCH_
	memset(pnewskb->cb, 0, sizeof(pnewskb->cb));
#endif
	tcb_desc = (cb_desc *)(pnewskb->cb + 8);
	tcb_desc->queue_index = BEACON_QUEUE;
	tcb_desc->data_rate = 2;
	tcb_desc->RATRIndex = 7;
	tcb_desc->bTxDisableRateFallBack = 1;
	tcb_desc->bTxUseDriverAssingedRate = 1;
#ifdef _RTL8192_EXT_PATCH_
	tcb_desc->bTxEnableFwCalcDur = 0;
	memcpy((unsigned char *)(pnewskb->cb),&dev,sizeof(dev));
#endif
	skb_push(pnewskb, priv->rtllib->tx_headroom);	

	pdesc = &ring->desc[0];
	priv->ops->tx_fill_descriptor(dev, pdesc, tcb_desc, pnewskb);
	__skb_queue_tail(&ring->queue, pnewskb);
	pdesc->OWN = 1;
#ifdef ASL
	if (((priv->rtllib->iw_mode == IW_MODE_MASTER || priv->rtllib->iw_mode == IW_MODE_APSTA) && priv->rtllib->ap_state == RTLLIB_LINKED) && priv->rtllib->mDtimCount == 0) { 
		pcb_desc tcb_desc_ps; 
		struct sk_buff * skb_ps = NULL;
		u32 i, len = skb_queue_len(&(priv->rtllib->GroupPsQueue));
		if (len > 0) {
			for (i=0; i<len; i++) {
				skb_ps = skb_dequeue(&(priv->rtllib->GroupPsQueue));
				tcb_desc_ps = (pcb_desc)(skb_ps->cb + MAX_DEV_ADDR_SIZE);
				tcb_desc_ps->ps_flag |= TCB_PS_FLAG_MCAST_DTIM;
				if ( i < (len -1))
					tcb_desc_ps->bMoreData = true;


				rtllib_xmit_inter(skb_ps, priv->rtllib->dev);
			}
		}
	}
#endif

	return;
}
#endif
void rtl8192_stop_beacon(struct net_device *dev)
{
}
#ifdef ASL
void rtl8192_config_rate(struct net_device* dev, u16* rate_config, bool bap)
#else
void rtl8192_config_rate(struct net_device* dev, u16* rate_config)
#endif
{
	 struct r8192_priv *priv = rtllib_priv(dev);
	 struct rtllib_network *net;
	 u8 i=0, basic_rate = 0;
#ifdef _RTL8192_EXT_PATCH_	
	if(priv->rtllib->iw_mode == IW_MODE_MESH)
		net = & priv->rtllib->current_mesh_network;
	else
		net = & priv->rtllib->current_network;
#else	
#ifdef ASL
	if (bap)
		net = & priv->rtllib->current_ap_network;
	else
#endif
	net = & priv->rtllib->current_network;
#endif

	 for (i = 0; i < net->rates_len; i++) {
		 basic_rate = net->rates[i] & 0x7f;
		 switch (basic_rate) {
		 case MGN_1M:	
			 *rate_config |= RRSR_1M;	
			 break;
		 case MGN_2M:	
			 *rate_config |= RRSR_2M;	
			 break;
		 case MGN_5_5M:	
			 *rate_config |= RRSR_5_5M;	
			 break;
		 case MGN_11M:	
			 *rate_config |= RRSR_11M;	
			 break;
		 case MGN_6M:	
			 *rate_config |= RRSR_6M;	
			 break;
		 case MGN_9M:	
			 *rate_config |= RRSR_9M;	
			 break;
		 case MGN_12M:	
			 *rate_config |= RRSR_12M;	
			 break;
		 case MGN_18M:	
			 *rate_config |= RRSR_18M;	
			 break;
		 case MGN_24M:	
			 *rate_config |= RRSR_24M;	
			 break;
		 case MGN_36M:	
			 *rate_config |= RRSR_36M;	
			 break;
		 case MGN_48M:	
			 *rate_config |= RRSR_48M;	
			 break;
		 case MGN_54M:	
			 *rate_config |= RRSR_54M;	
			 break;
		 }
	 }

	 for (i = 0; i < net->rates_ex_len; i++) {
		 basic_rate = net->rates_ex[i] & 0x7f;
		 switch (basic_rate) {
		 case MGN_1M:	
			 *rate_config |= RRSR_1M;	
			 break;
		 case MGN_2M:	
			 *rate_config |= RRSR_2M;	
			 break;
		 case MGN_5_5M:	
			 *rate_config |= RRSR_5_5M;	
			 break;
		 case MGN_11M:	
			 *rate_config |= RRSR_11M;	
			 break;
		 case MGN_6M:	
			 *rate_config |= RRSR_6M;	
			 break;
		 case MGN_9M:	
			 *rate_config |= RRSR_9M;	
			 break;
		 case MGN_12M:	
			 *rate_config |= RRSR_12M;	
			 break;
		 case MGN_18M:	
			 *rate_config |= RRSR_18M;	
			 break;
		 case MGN_24M:	
			 *rate_config |= RRSR_24M;	
			 break;
		 case MGN_36M:	
			 *rate_config |= RRSR_36M;	
			 break;
		 case MGN_48M:	
			 *rate_config |= RRSR_48M;	
			 break;
		 case MGN_54M:	
			 *rate_config |= RRSR_54M;	
			 break;
		 }
	 }
}

#ifdef ASL
void rtl8192_refresh_supportrate(struct r8192_priv* priv, bool is_ap)
#else
void rtl8192_refresh_supportrate(struct r8192_priv* priv)
#endif
{
	struct rtllib_device* ieee = priv->rtllib;
	int mode = 0;
#ifdef ASL
	if (is_ap)
		mode = ieee->apmode;
	else
#endif
		mode = ieee->mode;
	if (mode == WIRELESS_MODE_N_24G || mode == WIRELESS_MODE_N_5G) {
#ifdef ASL
		if (is_ap) {
			memcpy(ieee->ap_Regdot11HTOperationalRateSet, ieee->RegHTSuppRateSet, 16);
			memcpy(ieee->ap_Regdot11TxHTOperationalRateSet, ieee->RegHTSuppRateSet, 16);
		} else 
#endif		
		{
		memcpy(ieee->Regdot11HTOperationalRateSet, ieee->RegHTSuppRateSet, 16);
		memcpy(ieee->Regdot11TxHTOperationalRateSet, ieee->RegHTSuppRateSet, 16);
		}
#ifdef RTL8192CE
		if(priv->rf_type == RF_1T1R) {
			ieee->Regdot11HTOperationalRateSet[1] = 0;
		}
#endif

#ifdef RTL8192SE
		if(priv->rf_type == RF_1T1R) {
#ifdef ASL
			ieee->ap_Regdot11HTOperationalRateSet[1] = 0;
#endif
			ieee->Regdot11HTOperationalRateSet[1] = 0;
		}
		if(priv->rf_type == RF_1T1R || priv->rf_type == RF_1T2R)
		{
#ifdef ASL
			ieee->ap_Regdot11TxHTOperationalRateSet[1] = 0;
#endif
			ieee->Regdot11TxHTOperationalRateSet[1] = 0;
		}

            if(priv->rtllib->b1SSSupport == true) {
#ifdef ASL
			ieee->ap_Regdot11HTOperationalRateSet[1] = 0;
#endif
                ieee->Regdot11HTOperationalRateSet[1] = 0;
            }
#endif
	} else {
#ifdef ASL
		if (is_ap)
			memset(ieee->ap_Regdot11HTOperationalRateSet, 0, 16);
		else
#endif
		memset(ieee->Regdot11HTOperationalRateSet, 0, 16);
	}
	return;
}

u8 rtl8192_getSupportedWireleeMode(struct net_device*dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8 ret = 0;

	switch(priv->rf_chip) {
	case RF_8225:
	case RF_8256:
	case RF_6052:
	case RF_PSEUDO_11N:
		ret = (WIRELESS_MODE_N_24G|WIRELESS_MODE_G | WIRELESS_MODE_B);
		break;
	case RF_8258:
		ret = (WIRELESS_MODE_A | WIRELESS_MODE_N_5G);
		break;
	default:
		ret = WIRELESS_MODE_B;
		break;
	}
	return ret;
}

#ifdef ASL
void rtl8192_SetWirelessMode(struct net_device* dev, u8 wireless_mode, bool is_ap)
#else
void rtl8192_SetWirelessMode(struct net_device* dev, u8 wireless_mode)
#endif
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8 bSupportMode = rtl8192_getSupportedWireleeMode(dev);

#if 0
	if(	(wireless_mode != WIRELESS_MODE_B) && 
		(wireless_mode != WIRELESS_MODE_G) &&
		(wireless_mode != WIRELESS_MODE_A) &&
		(wireless_mode != WIRELESS_MODE_AUTO) &&
		(wireless_mode != WIRELESS_MODE_N_24G) &&
		(wireless_mode != WIRELESS_MODE_N_5G) ) 
	{
		wireless_mode = WIRELESS_MODE_AUTO;
	}
#endif
	if ((wireless_mode == WIRELESS_MODE_AUTO) || ((wireless_mode & bSupportMode) == 0)) {
		if (bSupportMode & WIRELESS_MODE_N_24G) {
			wireless_mode = WIRELESS_MODE_N_24G;
		} else if (bSupportMode & WIRELESS_MODE_N_5G) {
			wireless_mode = WIRELESS_MODE_N_5G;
		} else if((bSupportMode & WIRELESS_MODE_A)) {
			wireless_mode = WIRELESS_MODE_A;
		} else if((bSupportMode & WIRELESS_MODE_G)) {
			wireless_mode = WIRELESS_MODE_G;
		} else if((bSupportMode & WIRELESS_MODE_B)) {
			wireless_mode = WIRELESS_MODE_B;
		} else {
			RT_TRACE(COMP_ERR, "%s(), No valid wireless mode supported (%x)!!!\n",
				       	__FUNCTION__, bSupportMode);
			wireless_mode = WIRELESS_MODE_B;
		}
	}

#ifdef _RTL8192_EXT_PATCH_
	if ((wireless_mode & WIRELESS_MODE_N_24G) == WIRELESS_MODE_N_24G)
		wireless_mode = WIRELESS_MODE_N_24G;
	else if((wireless_mode & WIRELESS_MODE_N_5G) == WIRELESS_MODE_N_5G)
		wireless_mode = WIRELESS_MODE_N_5G;
	else if ((wireless_mode & WIRELESS_MODE_A) == WIRELESS_MODE_A)
		wireless_mode = WIRELESS_MODE_A;
	else if ((wireless_mode & WIRELESS_MODE_G) == WIRELESS_MODE_G)
		wireless_mode = WIRELESS_MODE_G;
	else
		wireless_mode = WIRELESS_MODE_B;
#else
	if ((wireless_mode & (WIRELESS_MODE_B | WIRELESS_MODE_G)) == (WIRELESS_MODE_G | WIRELESS_MODE_B))
		wireless_mode = WIRELESS_MODE_G;
#endif
#ifdef ASL
	if (is_ap)
		priv->rtllib->apmode = wireless_mode;
	else
#endif
	priv->rtllib->mode = wireless_mode;
	
	ActUpdateChannelAccessSetting( dev, wireless_mode, &priv->ChannelAccessSetting);
	
	if ((wireless_mode == WIRELESS_MODE_N_24G) ||  (wireless_mode == WIRELESS_MODE_N_5G)){
		priv->rtllib->pHTInfo->bEnableHT = 1;	
                printk("%s(), wireless_mode:%x, bEnableHT = 1\n", __FUNCTION__,wireless_mode);
        }else{
		priv->rtllib->pHTInfo->bEnableHT = 0;
                printk("%s(), wireless_mode:%x, bEnableHT = 0\n", __FUNCTION__,wireless_mode);
        }
		
	RT_TRACE(COMP_INIT, "Current Wireless Mode is %x\n", wireless_mode);
#ifdef ASL
	rtl8192_refresh_supportrate(priv, is_ap);
#else
	rtl8192_refresh_supportrate(priv);
#endif
}
#ifdef ASL
int _rtl8192_ap_up(struct net_device *dev,bool is_silent_reset)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PRT_POWER_SAVE_CONTROL pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));
	bool init_status = true;
	priv->bDriverIsGoingToUnload = false;
	priv->bdisable_nic = false;  

	RT_TRACE(COMP_INIT, "Bringing up iface");
	if(!is_silent_reset)
		priv->rtllib->iw_mode = IW_MODE_INFRA;
	if(priv->up){
		RT_TRACE(COMP_ERR,"%s():%s is up,return\n",__FUNCTION__,DRV_NAME);
		return -1;
	}
#ifdef RTL8192SE        
	priv->ReceiveConfig =
				RCR_APPFCS | RCR_APWRMGT | /*RCR_ADD3 |*/
				RCR_AMF | RCR_ADF | RCR_APP_MIC | RCR_APP_ICV |
				RCR_AICV | RCR_ACRC32    |                               
				RCR_AB          | RCR_AM                |                               
				RCR_APM         |                                                               
				/*RCR_AAP               |*/                                                     
				RCR_APP_PHYST_STAFF | RCR_APP_PHYST_RXFF |      
				(priv->EarlyRxThreshold<<RCR_FIFO_OFFSET)       ;
#elif defined RTL8192CE
	priv->ReceiveConfig = (\
					RCR_APPFCS	
					| RCR_AMF | RCR_ADF| RCR_APP_MIC| RCR_APP_ICV
					| RCR_AICV | RCR_ACRC32	
					| RCR_AB | RCR_AM			
     					| RCR_APM 					
     					| RCR_APP_PHYST_RXFF		
     					| RCR_HTC_LOC_CTRL
					);
#endif
	if (!priv->apdev_up) {
		RT_TRACE(COMP_INIT, "Bringing up iface");
		priv->bfirst_init = true;
		init_status = priv->ops->initialize_adapter(dev);
		if(init_status != true)
		{
			RT_TRACE(COMP_ERR,"ERR!!! %s(): initialization is failed!\n",__FUNCTION__);
			priv->bfirst_init = false;
			return -1;
		}

		RT_TRACE(COMP_INIT, "start adapter finished\n");
		RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);
		priv->rtllib->ieee_up=1;
		priv->bfirst_init = false;
#if defined RTL8192SE || defined RTL8192CE
		if(priv->rtllib->eRFPowerState!=eRfOn)
			MgntActSet_RF_State(dev, eRfOn, priv->rtllib->RfOffReason,true);	
#endif

#ifdef ENABLE_GPIO_RADIO_CTL
		if(priv->polling_timer_on == 0){
			check_rfctrl_gpio_timer((unsigned long)dev);
		}
#endif
		priv->rtllib->current_network.channel = INIT_DEFAULT_CHAN;
		priv->rtllib->current_ap_network.channel = INIT_DEFAULT_CHAN;
		dm_InitRateAdaptiveMask(dev);
#ifndef CONFIG_MP
		watch_dog_timer_callback((unsigned long) dev);
#endif

	} else {
		rtllib_stop_scan_syncro(priv->rtllib);
		priv->rtllib->iw_mode = IW_MODE_APSTA;
		priv->rtllib->current_network.channel = priv->rtllib->current_ap_network.channel;

	}
	priv->up=1;
	printk("%s():set chan %d\n",__FUNCTION__,INIT_DEFAULT_CHAN);
	priv->rtllib->set_chan(dev, INIT_DEFAULT_CHAN); 
	priv->up_first_time = 0;
	if (!priv->rtllib->proto_started) {
#ifdef RTL8192E
		if(priv->rtllib->eRFPowerState!=eRfOn)
			MgntActSet_RF_State(dev, eRfOn, priv->rtllib->RfOffReason,true);	
#endif
		if(priv->rtllib->state != RTLLIB_LINKED)
#ifndef CONFIG_MP
			rtllib_softmac_start_protocol(priv->rtllib, 0, 0);
#endif
}

	rtllib_reset_queue(priv->rtllib);
	if(!netif_queue_stopped(dev))
		netif_start_queue(dev);
	else
		netif_wake_queue(dev);
	return 0;
}

int rtl8192_ap_down(struct net_device *dev, bool shutdownrf)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	unsigned long flags = 0;
	u8 RFInProgressTimeOut = 0;

	if (priv->up == 0) return -1;

#ifdef ENABLE_IPS
	if(priv->rtllib->rtllib_ips_leave != NULL)
		priv->rtllib->rtllib_ips_leave(dev);
#endif

#ifdef ENABLE_LPS
	if(priv->rtllib->state == RTLLIB_LINKED)
		LeisurePSLeave(dev);
#endif
	if(priv->rtllib->LedControlHandler)
		priv->rtllib->LedControlHandler(dev, LED_CTL_POWER_OFF);

	priv->up=0;
	priv->bfirst_after_down = 1;
	RT_TRACE(COMP_DOWN, "==========>%s()\n", __FUNCTION__);
	if (!netif_queue_stopped(dev))
		netif_stop_queue(dev);

	if (!priv->apdev_up) {
		priv->bDriverIsGoingToUnload = true;
		priv->rtllib->ieee_up = 0;
		priv->rtllib->wpa_ie_len = 0;
		if(priv->rtllib->wpa_ie)
			kfree(priv->rtllib->wpa_ie);
		priv->rtllib->wpa_ie = NULL;
#ifdef RTL8192S_WAPI_SUPPORT	
		priv->rtllib->wapiInfo.wapiTxMsk.bTxEnable = false;
		priv->rtllib->wapiInfo.wapiTxMsk.bSet = false;
#endif	
		CamResetAllEntry(dev);	
		memset(priv->rtllib->swcamtable,0,sizeof(SW_CAM_TABLE)*32);
		rtl8192_irq_disable(dev);

		del_timer_sync(&priv->watch_dog_timer);	
		rtl8192_cancel_deferred_work(priv);
#ifndef RTL8190P
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)	
		cancel_delayed_work(&priv->rtllib->hw_wakeup_wq);
#endif
#endif

		rtllib_softmac_stop_protocol(priv->rtllib, 0, 0, true);
		spin_lock_irqsave(&priv->rf_ps_lock,flags);
		while(priv->RFChangeInProgress)
		{
			spin_unlock_irqrestore(&priv->rf_ps_lock,flags);
			if(RFInProgressTimeOut > 100)
			{
				spin_lock_irqsave(&priv->rf_ps_lock,flags);
				break;
			}
			printk("===>%s():RF is in progress, need to wait until rf chang is done.\n",__FUNCTION__);
			mdelay(1);
			RFInProgressTimeOut ++;
			spin_lock_irqsave(&priv->rf_ps_lock,flags);
		}
		priv->RFChangeInProgress = true;
		spin_unlock_irqrestore(&priv->rf_ps_lock,flags);
		priv->ops->stop_adapter(dev, false);
		spin_lock_irqsave(&priv->rf_ps_lock,flags);
		priv->RFChangeInProgress = false;
		spin_unlock_irqrestore(&priv->rf_ps_lock,flags);
		udelay(100);
		memset(&priv->rtllib->current_network, 0 , offsetof(struct rtllib_network, list));
		priv->rtllib->wap_set = 0;
		priv->rtllib->current_network.channel = INIT_DEFAULT_CHAN;
#ifdef CONFIG_ASPM_OR_D3
		RT_ENABLE_ASPM(dev);
#endif
	}else {
		priv->rtllib->wpa_ie_len = 0;
		if(priv->rtllib->wpa_ie)
			kfree(priv->rtllib->wpa_ie);
		priv->rtllib->wpa_ie = NULL;
#ifdef RTL8192S_WAPI_SUPPORT
		priv->rtllib->wapiInfo.wapiTxMsk.bTxEnable = false;
		priv->rtllib->wapiInfo.wapiTxMsk.bSet = false;
#endif
		CamResetAllEntry(dev);
		CamRestoreEachIFEntry(dev,0,1);	
		memset(priv->rtllib->swcamtable,0,sizeof(SW_CAM_TABLE)*32); 

		rtllib_softmac_stop_protocol(priv->rtllib, 0, 0, true);
		memset(&priv->rtllib->current_network, 0 , offsetof(struct rtllib_network, list));
		priv->rtllib->current_network.channel = INIT_DEFAULT_CHAN;
		priv->rtllib->wap_set = 0; 
		priv->rtllib->iw_mode = IW_MODE_MASTER;
	}

	RT_TRACE(COMP_DOWN, "<==========%s()\n", __FUNCTION__);

	return 0;
}
#endif

#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
#define KEEP_ALIVE_INTERVAL 				20 
#define DEFAULT_KEEP_ALIVE_LEVEL			1

static void MgntLinkKeepAlive(struct r8192_priv *priv )
{
	if (priv->keepAliveLevel == 0)
		return;

	if((priv->rtllib->state == RTLLIB_LINKED) && (!priv->rtllib->is_roaming))
	{
		
		if ( (priv->keepAliveLevel== 2) ||
			(priv->rtllib->LinkDetectInfo.LastNumTxUnicast == priv->rtllib->LinkDetectInfo.NumTxUnicastOkInPeriod && 
			priv->rtllib->LinkDetectInfo.NumRxUnicastOkInPeriod == 0)
			)
		{
			priv->rtllib->LinkDetectInfo.IdleCount++;	

			if(priv->rtllib->LinkDetectInfo.IdleCount >= ((KEEP_ALIVE_INTERVAL / RT_CHECK_FOR_HANG_PERIOD)-1) )
			{
				priv->rtllib->LinkDetectInfo.IdleCount = 0;
				rtllib_sta_ps_send_null_frame(priv->rtllib, false);
			}
		}
		else
		{
			priv->rtllib->LinkDetectInfo.IdleCount = 0;
		}
		priv->rtllib->LinkDetectInfo.LastNumTxUnicast = priv->rtllib->LinkDetectInfo.NumTxUnicastOkInPeriod; 
		priv->rtllib->LinkDetectInfo.LastNumRxUnicast = priv->rtllib->LinkDetectInfo.NumRxUnicastOkInPeriod;
	}
}
#endif
#ifdef _RTL8192_EXT_PATCH_

static void rtl8192_init_mesh_variable(struct net_device* dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8 i;

	priv->rtllib->mesh_security_setting = 0;
	memset(priv->rtllib->swmeshcamtable,0,sizeof(SW_CAM_TABLE)*32);
	priv->rtllib->mesh_sec_type = 0;

	priv->keepAliveLevel = DEFAULT_KEEP_ALIVE_LEVEL;

	priv->FwCmdIOMap = 0;
	priv->FwCmdIOParam = 0;
	priv->ThermalValue = 0;
	priv->DMFlag = 0;
	priv->rssi_level = 0;
	priv->rtllib->bUseRAMask = 0;

	memset(priv->rtllib->swmeshratrtable,0,8*(sizeof(SW_RATR_TABLE)));
	priv->rtllib->mesh_amsdu_in_process = 0;
	priv->rtllib->HwSecCamBitMap = 0;
	memset(priv->rtllib->HwSecCamStaAddr,0,TOTAL_CAM_ENTRY * ETH_ALEN); 
	priv->rtllib->LinkingPeerBitMap = 0;
	memset(priv->rtllib->LinkingPeerAddr,0,(MAX_MP-1) * ETH_ALEN); 
	memset(priv->rtllib->LinkingPeerSecState, 0, (MAX_MP-1));
	memset(priv->rtllib->peer_AID_Addr,0,30 * ETH_ALEN);
	priv->rtllib->peer_AID_bitmap = 0;
	priv->rtllib->backup_channel = 1;
	priv->rtllib->del_hwsec_cam_entry = rtl8192_del_hwsec_cam_entry; 
	priv->rtllib->set_key_for_peer = meshdev_set_key_for_peer;
	priv->rtllib->hostname_len = 0;
	memset(priv->rtllib->hostname, 0, sizeof(priv->rtllib->hostname));
	strcpy(priv->rtllib->hostname, "Crab"); 
	priv->rtllib->hostname_len = strlen(priv->rtllib->hostname);
	priv->rtllib->meshScanMode = 0;
	priv->rtllib->currentRate = 0xffffffff;
	priv->mshobj = alloc_mshobj(priv);
	printk("priv is %p,mshobj is %p\n",priv,priv->mshobj);

	if (priv->mshobj) {
		priv->rtllib->ext_patch_rtllib_start_protocol = 
			priv->mshobj->ext_patch_rtllib_start_protocol;
		priv->rtllib->ext_patch_rtllib_stop_protocol = 
			priv->mshobj->ext_patch_rtllib_stop_protocol;
		priv->rtllib->ext_patch_rtllib_start_mesh = 
			priv->mshobj->ext_patch_rtllib_start_mesh;
		priv->rtllib->ext_patch_rtllib_probe_req_1 = 
			priv->mshobj->ext_patch_rtllib_probe_req_1;
		priv->rtllib->ext_patch_rtllib_probe_req_2 = 
			priv->mshobj->ext_patch_rtllib_probe_req_2;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_auth = 
			priv->mshobj->ext_patch_rtllib_rx_frame_softmac_on_auth;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_deauth = 
			priv->mshobj->ext_patch_rtllib_rx_frame_softmac_on_deauth;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_peerlink_open = 
			priv->mshobj->ext_patch_rtllib_rx_frame_softmac_on_peerlink_open;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_peerlink_confirm = 
			priv->mshobj->ext_patch_rtllib_rx_frame_softmac_on_peerlink_confirm;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_peerlink_close = 
			priv->mshobj->ext_patch_rtllib_rx_frame_softmac_on_peerlink_close;
		priv->rtllib->ext_patch_rtllib_close_all_peerlink = 
			priv->mshobj->ext_patch_rtllib_close_all_peerlink;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_linkmetric_report = 
			priv->mshobj->ext_patch_rtllib_rx_frame_softmac_on_linkmetric_report;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_linkmetric_req = 
			priv->mshobj->ext_patch_rtllib_rx_frame_softmac_on_linkmetric_req;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_pathselect_preq = 
			priv->mshobj->ext_patch_rtllib_rx_frame_softmac_on_pathselect_preq;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_pathselect_prep = 
			priv->mshobj->ext_patch_rtllib_rx_frame_softmac_on_pathselect_prep;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_pathselect_perr = 
			priv->mshobj->ext_patch_rtllib_rx_frame_softmac_on_pathselect_perr;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_pathselect_rann = 
			priv->mshobj->ext_patch_rtllib_rx_frame_softmac_on_pathselect_rann;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_pathselect_pann = 
			priv->mshobj->ext_patch_rtllib_rx_frame_softmac_on_pathselect_pann;
		priv->rtllib->ext_patch_rtllib_ext_stop_scan_wq_set_channel = 
			priv->mshobj->ext_patch_rtllib_ext_stop_scan_wq_set_channel;
		priv->rtllib->ext_patch_r819x_wx_set_mesh_chan = 
			priv->mshobj->ext_patch_r819x_wx_set_mesh_chan;
		priv->rtllib->ext_patch_r819x_wx_set_channel = 
			priv->mshobj->ext_patch_r819x_wx_set_channel;
		priv->rtllib->ext_patch_rtllib_process_probe_response_1 = 
			priv->mshobj->ext_patch_rtllib_process_probe_response_1;
		priv->rtllib->ext_patch_rtllib_rx_mgt_on_probe_req = 
			priv->mshobj->ext_patch_rtllib_rx_mgt_on_probe_req;
		priv->rtllib->ext_patch_rtllib_rx_mgt_update_expire = 
			priv->mshobj->ext_patch_rtllib_rx_mgt_update_expire;
		priv->rtllib->ext_patch_get_beacon_get_probersp = 
			priv->mshobj->ext_patch_get_beacon_get_probersp;
		priv->rtllib->ext_patch_rtllib_rx_on_rx = 
			priv->mshobj->ext_patch_rtllib_rx_on_rx;		
		priv->rtllib->ext_patch_rtllib_rx_frame_get_hdrlen = 
			priv->mshobj->ext_patch_rtllib_rx_frame_get_hdrlen;
		priv->rtllib->ext_patch_rtllib_rx_frame_get_mac_hdrlen = 
			priv->mshobj->ext_patch_rtllib_rx_frame_get_mac_hdrlen;
		priv->rtllib->ext_patch_rtllib_rx_frame_get_mesh_hdrlen_llc = 
			priv->mshobj->ext_patch_rtllib_rx_frame_get_mesh_hdrlen_llc;
		priv->rtllib->ext_patch_rtllib_rx_is_valid_framectl = 
			priv->mshobj->ext_patch_rtllib_rx_is_valid_framectl;
		priv->rtllib->ext_patch_rtllib_softmac_xmit_get_rate = 
			priv->mshobj->ext_patch_rtllib_softmac_xmit_get_rate;
		/* added by david for setting acl dynamically */
		priv->rtllib->ext_patch_rtllib_acl_query = 
			priv->mshobj->ext_patch_rtllib_acl_query;
		priv->rtllib->ext_patch_rtllib_is_mesh = 
			priv->mshobj->ext_patch_rtllib_is_mesh;
		priv->rtllib->ext_patch_rtllib_create_crypt_for_peer = 
			priv->mshobj->ext_patch_rtllib_create_crypt_for_peer;
		priv->rtllib->ext_patch_rtllib_get_peermp_htinfo = 
			priv->mshobj->ext_patch_rtllib_get_peermp_htinfo;
		priv->rtllib->ext_patch_rtllib_send_ath_commit = 
			priv->mshobj->ext_patch_rtllib_send_ath_commit;
		priv->rtllib->ext_patch_rtllib_send_ath_confirm = 
			priv->mshobj->ext_patch_rtllib_send_ath_confirm;
		priv->rtllib->ext_patch_rtllib_rx_ath_commit = 
			priv->mshobj->ext_patch_rtllib_rx_ath_commit;
		priv->rtllib->ext_patch_rtllib_rx_ath_confirm = 
			priv->mshobj->ext_patch_rtllib_rx_ath_confirm;
	}
	for (i = 0; i < MAX_QUEUE_SIZE; i++) {
		skb_queue_head_init(&priv->rtllib->skb_meshaggQ[i]);
	}
}

int _rtl8192_mesh_up(struct net_device *dev,bool is_silent_reset)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PRT_POWER_SAVE_CONTROL pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));
	bool init_status = true;
	priv->bDriverIsGoingToUnload = false;
	priv->bdisable_nic = false;  

	if(priv->mesh_up){
		RT_TRACE(COMP_ERR,"%s(): since mesh0 is already up, ra0 is forbidden to open.\n",__FUNCTION__);
		return 0;
	}
	RT_TRACE(COMP_DOWN, "==========>%s()\n", __FUNCTION__);
	if(!is_silent_reset)
		priv->rtllib->iw_mode = IW_MODE_INFRA;
	if(priv->up){
		RT_TRACE(COMP_ERR,"%s():%s is up,return\n",__FUNCTION__,DRV_NAME);
		return -1;
	}
#ifdef RTL8192SE        
	priv->ReceiveConfig =
				RCR_APPFCS | RCR_APWRMGT | /*RCR_ADD3 |*/
				RCR_AMF | RCR_ADF | RCR_APP_MIC | RCR_APP_ICV |
				RCR_AICV | RCR_ACRC32    |                               
				RCR_AB          | RCR_AM                |                               
				RCR_APM         |                                                               
				/*RCR_AAP               |*/                                                     
				RCR_APP_PHYST_STAFF | RCR_APP_PHYST_RXFF |      
				(priv->EarlyRxThreshold<<RCR_FIFO_OFFSET)       ;
#elif defined RTL8192CE
	priv->ReceiveConfig = (\
					RCR_APPFCS	
					| RCR_AMF | RCR_ADF| RCR_APP_MIC| RCR_APP_ICV
					| RCR_AICV | RCR_ACRC32	
					| RCR_AB | RCR_AM			
     					| RCR_APM 					
     					| RCR_APP_PHYST_RXFF		
     					| RCR_HTC_LOC_CTRL
					);
#endif

	if(!priv->mesh_up)
	{
		RT_TRACE(COMP_INIT, "Bringing up iface");
		priv->bfirst_init = true;
		init_status = priv->ops->initialize_adapter(dev);
		if(init_status != true)
		{
			RT_TRACE(COMP_ERR,"ERR!!! %s(): initialization is failed!\n",__FUNCTION__);
			return -1;
		}
		RT_TRACE(COMP_INIT, "start adapter finished\n");
		RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);
		priv->rtllib->ieee_up=1;
		priv->bfirst_init = false;
#ifdef ENABLE_GPIO_RADIO_CTL
		if(priv->polling_timer_on == 0){
			check_rfctrl_gpio_timer((unsigned long)dev);
		}
#endif
		priv->rtllib->current_network.channel = INIT_DEFAULT_CHAN;
		priv->rtllib->current_mesh_network.channel = INIT_DEFAULT_CHAN;
		if((priv->mshobj->ext_patch_r819x_wx_set_mesh_chan) && (!is_silent_reset))
			priv->mshobj->ext_patch_r819x_wx_set_mesh_chan(dev,INIT_DEFAULT_CHAN);
		if((priv->mshobj->ext_patch_r819x_wx_set_channel) && (!is_silent_reset))
		{
			priv->mshobj->ext_patch_r819x_wx_set_channel(priv->rtllib, INIT_DEFAULT_CHAN);
		}
		dm_InitRateAdaptiveMask(dev);
	}	
	else
	{
		rtllib_stop_scan_syncro(priv->rtllib);
	}
	priv->up=1; 
	printk("%s():set chan %d\n",__FUNCTION__,INIT_DEFAULT_CHAN);
	priv->rtllib->set_chan(dev, INIT_DEFAULT_CHAN); 
	priv->up_first_time = 0;
	if(!priv->rtllib->proto_started) 
	{
#ifdef RTL8192E
		if(priv->rtllib->eRFPowerState!=eRfOn)
			MgntActSet_RF_State(dev, eRfOn, priv->rtllib->RfOffReason,true);	
#endif
		if(priv->rtllib->state != RTLLIB_LINKED)
			rtllib_softmac_start_protocol(priv->rtllib, 0, 0);
	}
	if(!priv->mesh_up)
		watch_dog_timer_callback((unsigned long) dev);
	rtllib_reset_queue(priv->rtllib);
	if(!netif_queue_stopped(dev))
		netif_start_queue(dev);
	else
		netif_wake_queue(dev);
	RT_TRACE(COMP_DOWN, "<==========%s()\n", __FUNCTION__);

	return 0;
}

int rtl8192_mesh_down(struct net_device *dev, bool shutdownrf)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	unsigned long flags = 0;
	u8 RFInProgressTimeOut = 0;

	if (priv->up == 0) 
		return -1;

	RT_TRACE(COMP_DOWN, "==========>%s()\n", __FUNCTION__);
#ifdef ENABLE_LPS
	if(priv->rtllib->state == RTLLIB_LINKED)
		LeisurePSLeave(dev);
#endif
	priv->up=0; 

	/* FIXME */
	if (!netif_queue_stopped(dev))
		netif_stop_queue(dev);  
	if(!priv->mesh_up)
	{
		priv->bDriverIsGoingToUnload = true;	
		priv->rtllib->ieee_up = 0;  
		/* mesh stack has also be closed, then disalbe the hardware function at 
		 * the same time */
		priv->rtllib->wpa_ie_len = 0;
		if(priv->rtllib->wpa_ie)
			kfree(priv->rtllib->wpa_ie);
		priv->rtllib->wpa_ie = NULL;
#ifdef RTL8192S_WAPI_SUPPORT
		priv->rtllib->wapiInfo.wapiTxMsk.bTxEnable = false;
		priv->rtllib->wapiInfo.wapiTxMsk.bSet = false;
#endif		
		CamResetAllEntry(dev);
		memset(priv->rtllib->swcamtable,0,sizeof(SW_CAM_TABLE)*32);
		rtl8192_irq_disable(dev);
		rtl8192_cancel_deferred_work(priv);
#ifndef RTL8190P
		cancel_delayed_work(&priv->rtllib->hw_wakeup_wq);
#endif
		deinit_hal_dm(dev);
		del_timer_sync(&priv->watch_dog_timer);	

		rtllib_softmac_stop_protocol(priv->rtllib, 0, 0, true);
		SPIN_LOCK_PRIV_RFPS(&priv->rf_ps_lock);
		while(priv->RFChangeInProgress)
		{
			SPIN_UNLOCK_PRIV_RFPS(&priv->rf_ps_lock);
			if(RFInProgressTimeOut > 100)
			{
				SPIN_LOCK_PRIV_RFPS(&priv->rf_ps_lock);
				break;
			}
			printk("===>%s():RF is in progress, need to wait until rf chang is done.\n",__FUNCTION__);
			mdelay(1);
			RFInProgressTimeOut ++;
			SPIN_LOCK_PRIV_RFPS(&priv->rf_ps_lock);
		}
		priv->RFChangeInProgress = true;
		SPIN_UNLOCK_PRIV_RFPS(&priv->rf_ps_lock);
		priv->ops->stop_adapter(dev, false);
		SPIN_LOCK_PRIV_RFPS(&priv->rf_ps_lock);
		priv->RFChangeInProgress = false;
		SPIN_UNLOCK_PRIV_RFPS(&priv->rf_ps_lock);
		udelay(100);
		memset(&priv->rtllib->current_network, 0 , offsetof(struct rtllib_network, list));
		priv->rtllib->wap_set = 0; 
		priv->rtllib->current_network.channel = INIT_DEFAULT_CHAN;
#ifdef CONFIG_ASPM_OR_D3
		RT_ENABLE_ASPM(dev);
#endif
	} else {
		priv->rtllib->wpa_ie_len = 0;
		if(priv->rtllib->wpa_ie)
			kfree(priv->rtllib->wpa_ie);
		priv->rtllib->wpa_ie = NULL;
#ifdef RTL8192S_WAPI_SUPPORT
		priv->rtllib->wapiInfo.wapiTxMsk.bTxEnable = false;
		priv->rtllib->wapiInfo.wapiTxMsk.bSet = false;
#endif
		CamResetAllEntry(dev);
		CamRestoreEachIFEntry(dev,1,0);	
		memset(priv->rtllib->swcamtable,0,sizeof(SW_CAM_TABLE)*32);

		rtllib_softmac_stop_protocol(priv->rtllib, 0, 0, true);
		memset(&priv->rtllib->current_network, 0 , offsetof(struct rtllib_network, list));
		priv->rtllib->current_network.channel = INIT_DEFAULT_CHAN;
		priv->rtllib->wap_set = 0; 
	}

	RT_TRACE(COMP_DOWN, "<==========%s()\n", __FUNCTION__);

	return 0;
}

void rtl8192_dinit_mshobj(struct r8192_priv *priv) 
{

	if(priv && priv->mshobj)
	{
		if(priv->mshobj->ext_patch_remove_proc)
			priv->mshobj->ext_patch_remove_proc(priv);
		priv->rtllib->ext_patch_rtllib_start_protocol = 0;
		priv->rtllib->ext_patch_rtllib_stop_protocol = 0;
		priv->rtllib->ext_patch_rtllib_probe_req_1 = 0;
		priv->rtllib->ext_patch_rtllib_probe_req_2 = 0;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_auth =0;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_deauth =0;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_peerlink_open = 0;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_peerlink_confirm = 0;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_peerlink_close = 0;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_linkmetric_report= 0;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_linkmetric_req= 0;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_pathselect_preq = 0;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_pathselect_prep=0;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_pathselect_perr = 0;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_pathselect_rann=0;
		priv->rtllib->ext_patch_rtllib_rx_frame_softmac_on_pathselect_pann=0;
		priv->rtllib->ext_patch_rtllib_ext_stop_scan_wq_set_channel = 0;
		priv->rtllib->ext_patch_rtllib_process_probe_response_1 = 0;
		priv->rtllib->ext_patch_rtllib_rx_mgt_on_probe_req = 0;
		priv->rtllib->ext_patch_rtllib_rx_mgt_update_expire = 0;
		priv->rtllib->ext_patch_rtllib_rx_on_rx = 0;
		priv->rtllib->ext_patch_get_beacon_get_probersp = 0;
		priv->rtllib->ext_patch_rtllib_rx_frame_get_hdrlen = 0;
		priv->rtllib->ext_patch_rtllib_rx_frame_get_mac_hdrlen = 0;
		priv->rtllib->ext_patch_rtllib_rx_frame_get_mesh_hdrlen_llc = 0;
		priv->rtllib->ext_patch_rtllib_rx_is_valid_framectl = 0;
		priv->rtllib->ext_patch_rtllib_softmac_xmit_get_rate = 0;
		priv->rtllib->ext_patch_rtllib_is_mesh = 0;
		priv->rtllib->ext_patch_rtllib_create_crypt_for_peer = 0;
		priv->rtllib->ext_patch_rtllib_get_peermp_htinfo = 0;
		priv->rtllib->ext_patch_rtllib_send_ath_commit = 0;
		priv->rtllib->ext_patch_rtllib_send_ath_confirm = 0;
		priv->rtllib->ext_patch_rtllib_rx_ath_commit = 0;
		priv->rtllib->ext_patch_rtllib_rx_ath_confirm = 0;
                free_mshobj(&priv->mshobj);
	}

}
#else
int _rtl8192_sta_up(struct net_device *dev,bool is_silent_reset)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PRT_POWER_SAVE_CONTROL pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));
	bool init_status = true;
	priv->bDriverIsGoingToUnload = false;
	priv->bdisable_nic = false;  

	priv->up=1;
	priv->rtllib->ieee_up=1;	
	
	priv->up_first_time = 0;
	RT_TRACE(COMP_INIT, "Bringing up iface");
	priv->bfirst_init = true;
	init_status = priv->ops->initialize_adapter(dev);
	if(init_status != true)
	{
		RT_TRACE(COMP_ERR,"ERR!!! %s(): initialization is failed!\n",__FUNCTION__);
		priv->bfirst_init = false;
		return -1;
	}

	RT_TRACE(COMP_INIT, "start adapter finished\n");
	RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);
	priv->bfirst_init = false;
#if defined RTL8192SE || defined RTL8192CE
	if(priv->rtllib->eRFPowerState!=eRfOn)
		MgntActSet_RF_State(dev, eRfOn, priv->rtllib->RfOffReason,true);	
#endif

#ifdef ENABLE_GPIO_RADIO_CTL
	if(priv->polling_timer_on == 0){
		check_rfctrl_gpio_timer((unsigned long)dev);
	}
#endif

	if(priv->rtllib->state != RTLLIB_LINKED)
#ifndef CONFIG_MP
	rtllib_softmac_start_protocol(priv->rtllib, 0, 0);
#endif
	rtllib_reset_queue(priv->rtllib);
#ifndef CONFIG_MP
	watch_dog_timer_callback((unsigned long) dev);
#endif


	if(!netif_queue_stopped(dev))
		netif_start_queue(dev);
	else
		netif_wake_queue(dev);

	return 0;
}

int rtl8192_sta_down(struct net_device *dev, bool shutdownrf)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	unsigned long flags = 0;
	u8 RFInProgressTimeOut = 0;

	if (priv->up == 0) return -1;

#ifdef ENABLE_IPS
	if(priv->rtllib->rtllib_ips_leave != NULL)
		priv->rtllib->rtllib_ips_leave(dev);
#endif

#ifdef ENABLE_LPS
	if(priv->rtllib->state == RTLLIB_LINKED)
		LeisurePSLeave(dev);
#endif
	if(priv->rtllib->LedControlHandler)
		priv->rtllib->LedControlHandler(dev, LED_CTL_POWER_OFF);

	priv->bDriverIsGoingToUnload = true;
	priv->up=0;
	priv->rtllib->ieee_up = 0;
	priv->bfirst_after_down = 1;
	RT_TRACE(COMP_DOWN, "==========>%s()\n", __FUNCTION__);
	if (!netif_queue_stopped(dev))
		netif_stop_queue(dev);

	priv->rtllib->wpa_ie_len = 0;
	if(priv->rtllib->wpa_ie)
		kfree(priv->rtllib->wpa_ie);
	priv->rtllib->wpa_ie = NULL;
#ifdef RTL8192S_WAPI_SUPPORT	
	priv->rtllib->wapiInfo.wapiTxMsk.bTxEnable = false;
	priv->rtllib->wapiInfo.wapiTxMsk.bSet = false;
#endif	
	CamResetAllEntry(dev);	
	memset(priv->rtllib->swcamtable,0,sizeof(SW_CAM_TABLE)*32);
	rtl8192_irq_disable(dev);

	del_timer_sync(&priv->watch_dog_timer);	
	rtl8192_cancel_deferred_work(priv);
#ifndef RTL8190P
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)	
	cancel_delayed_work(&priv->rtllib->hw_wakeup_wq);
#endif
#endif

	rtllib_softmac_stop_protocol(priv->rtllib, 0, 0, true);
	spin_lock_irqsave(&priv->rf_ps_lock,flags);
	while(priv->RFChangeInProgress)
	{
		spin_unlock_irqrestore(&priv->rf_ps_lock,flags);
		if(RFInProgressTimeOut > 100)
		{
			spin_lock_irqsave(&priv->rf_ps_lock,flags);
			break;
		}
		printk("===>%s():RF is in progress, need to wait until rf chang is done.\n",__FUNCTION__);
		mdelay(1);
		RFInProgressTimeOut ++;
		spin_lock_irqsave(&priv->rf_ps_lock,flags);
	}
	priv->RFChangeInProgress = true;
	spin_unlock_irqrestore(&priv->rf_ps_lock,flags);
	priv->ops->stop_adapter(dev, false);
	spin_lock_irqsave(&priv->rf_ps_lock,flags);
	priv->RFChangeInProgress = false;
	spin_unlock_irqrestore(&priv->rf_ps_lock,flags);
	udelay(100);

	memset(&priv->rtllib->current_network, 0 , offsetof(struct rtllib_network, list));
#ifdef CONFIG_ASPM_OR_D3
	RT_ENABLE_ASPM(dev);
#endif
	RT_TRACE(COMP_DOWN, "<==========%s()\n", __FUNCTION__);

	return 0;
}

#endif

static void rtl8192_init_priv_handler(struct net_device* dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	priv->rtllib->softmac_hard_start_xmit 	= rtl8192_hard_start_xmit;
	priv->rtllib->set_chan 				= rtl8192_set_chan;
	priv->rtllib->link_change 			= priv->ops->link_change;
#ifdef ASL
	priv->rtllib->ap_link_change		=priv->ops->ap_link_change;
#endif
	priv->rtllib->softmac_data_hard_start_xmit = rtl8192_hard_data_xmit;
	priv->rtllib->data_hard_stop 		= rtl8192_data_hard_stop;
	priv->rtllib->data_hard_resume 		= rtl8192_data_hard_resume;
	priv->rtllib->check_nic_enough_desc 	= rtl8192_check_nic_enough_desc;	
	priv->rtllib->get_nic_desc_num 		= rtl8192_get_nic_desc_num;	
#ifdef _RTL8192_EXT_PATCH_
	priv->rtllib->set_mesh_key 			= r8192_mesh_set_enc_ext;
#endif
	priv->rtllib->handle_assoc_response 	= rtl8192_handle_assoc_response;
	priv->rtllib->handle_beacon 		= rtl8192_handle_beacon;
	priv->rtllib->SetWirelessMode 		= rtl8192_SetWirelessMode;
	
#ifdef ENABLE_LPS
	priv->rtllib->LeisurePSLeave 		= LeisurePSLeave;
#endif

#ifdef RTL8192CE
	priv->rtllib->SetBWModeHandler 		= PHY_SetBWMode8192C;
	priv->rf_set_chan 						= PHY_SwChnl8192C;
	
#ifdef _ENABLE_SW_BEACON
	priv->rtllib->start_send_beacons 	= NULL;
	priv->rtllib->stop_send_beacons 	= NULL;
#else
	priv->rtllib->start_send_beacons 	= rtl8192ce_SetBeaconRelatedRegisters;
	priv->rtllib->stop_send_beacons 	= rtl8192_stop_beacon;
	priv->rtllib->UpdateBeaconInterruptHandler = rtl8192ce_UpdateBeaconInterruptMask;
	priv->rtllib->UpdateInterruptMaskHandler = rtl8192ce_UpdateInterruptMask;
#endif
	
	priv->rtllib->sta_wake_up 			= rtl8192_hw_wakeup;
	priv->rtllib->enter_sleep_state 		= rtl8192_hw_to_sleep;
	priv->rtllib->ps_is_queue_empty 	= rtl8192_is_tx_queue_empty;
	
	priv->rtllib->GetNmodeSupportBySecCfg = rtl8192ce_GetNmodeSupportBySecCfg;
	priv->rtllib->GetHalfNmodeSupportByAPsHandler = rtl8192ce_GetHalfNmodeSupportByAPs;

	priv->rtllib->SetHwRegHandler 		= rtl8192ce_SetHwReg;
	priv->rtllib->GetHwRegHandler 		= rtl8192ce_GetHwReg;
	priv->rtllib->AllowAllDestAddrHandler = rtl8192ce_AllowAllDestAddr;
	priv->rtllib->SetFwCmdHandler 		= rtl8192ce_phy_SetFwCmdIO;
	priv->rtllib->UpdateHalRAMaskHandler 	= rtl8192ce_UpdateHalRAMask;
	priv->rtllib->rtl_11n_user_show_rates = rtl8192_11n_user_show_rates;
#ifdef ENABLE_IPS
	priv->rtllib->rtllib_ips_leave_wq = rtllib_ips_leave_wq;
	priv->rtllib->rtllib_ips_leave 	= rtllib_ips_leave;
#endif

	priv->rtllib->LedControlHandler 		= LedControl8192CE;
	priv->rtllib->ScanOperationBackupHandler = PHY_ScanOperationBackup8192C;
#endif

#ifdef RTL8192SE
	priv->rtllib->SetBWModeHandler 		= rtl8192_SetBWMode;
	priv->rf_set_chan 						= rtl8192_phy_SwChnl;
	
#ifdef _ENABLE_SW_BEACON
	priv->rtllib->start_send_beacons = NULL;
	priv->rtllib->stop_send_beacons = NULL;
#else
	priv->rtllib->start_send_beacons = rtl8192se_start_beacon;
	priv->rtllib->stop_send_beacons = rtl8192_stop_beacon;
#endif	
	priv->rtllib->sta_wake_up = rtl8192_hw_wakeup;
	priv->rtllib->enter_sleep_state = rtl8192_hw_to_sleep;
	priv->rtllib->ps_is_queue_empty = rtl8192_is_tx_queue_empty;

	priv->rtllib->GetNmodeSupportBySecCfg = rtl8192se_GetNmodeSupportBySecCfg;
	priv->rtllib->GetHalfNmodeSupportByAPsHandler = rtl8192se_GetHalfNmodeSupportByAPs;

	priv->rtllib->SetBeaconRelatedRegistersHandler = SetBeaconRelatedRegisters8192SE;
	priv->rtllib->Adhoc_InitRateAdaptive = Adhoc_InitRateAdaptive;
	priv->rtllib->check_ht_cap = rtl8192se_check_ht_cap;
	priv->rtllib->SetHwRegHandler = SetHwReg8192SE;
	priv->rtllib->GetHwRegHandler = GetHwReg8192SE;
	priv->rtllib->AllowAllDestAddrHandler = rtl8192se_AllowAllDestAddr; 
	priv->rtllib->SetFwCmdHandler = rtl8192se_set_fw_cmd;
	priv->rtllib->UpdateHalRAMaskHandler = UpdateHalRAMask8192SE;
	priv->rtllib->UpdateBeaconInterruptHandler = NULL;
	priv->rtllib->rtl_11n_user_show_rates = rtl8192_11n_user_show_rates;
#ifdef ENABLE_IPS
	priv->rtllib->rtllib_ips_leave_wq = rtllib_ips_leave_wq;
	priv->rtllib->rtllib_ips_leave = rtllib_ips_leave;
#endif

	priv->rtllib->LedControlHandler = LedControl8192SE;
	priv->rtllib->rtllib_start_hw_scan = rtl8192se_hw_scan_initiate;
	priv->rtllib->rtllib_stop_hw_scan = rtl8192se_cancel_hw_scan;

	priv->rtllib->ScanOperationBackupHandler = PHY_ScanOperationBackup8192S;
#endif

#ifdef RTL8192E
	priv->rtllib->SetBWModeHandler 		= rtl8192_SetBWMode;
	priv->rf_set_chan 						= rtl8192_phy_SwChnl;
	
#ifdef _ENABLE_SW_BEACON
	priv->rtllib->start_send_beacons = NULL;
	priv->rtllib->stop_send_beacons = NULL;
#else
	priv->rtllib->start_send_beacons = rtl8192e_start_beacon;
	priv->rtllib->stop_send_beacons = rtl8192_stop_beacon;
#endif
	
	priv->rtllib->sta_wake_up = rtl8192_hw_wakeup;
	priv->rtllib->enter_sleep_state = rtl8192_hw_to_sleep;
	priv->rtllib->ps_is_queue_empty = rtl8192_is_tx_queue_empty;

	priv->rtllib->GetNmodeSupportBySecCfg = rtl8192_GetNmodeSupportBySecCfg;
	priv->rtllib->GetHalfNmodeSupportByAPsHandler = rtl8192_GetHalfNmodeSupportByAPs;

	priv->rtllib->SetHwRegHandler = rtl8192e_SetHwReg;
	priv->rtllib->AllowAllDestAddrHandler = rtl8192_AllowAllDestAddr; 
	priv->rtllib->SetFwCmdHandler = NULL;
	priv->rtllib->InitialGainHandler = InitialGain819xPci;
#ifdef ENABLE_IPS
	priv->rtllib->rtllib_ips_leave_wq = rtllib_ips_leave_wq;
	priv->rtllib->rtllib_ips_leave = rtllib_ips_leave;
#endif

	priv->rtllib->LedControlHandler = NULL;
	priv->rtllib->UpdateBeaconInterruptHandler = NULL;
	
	priv->rtllib->ScanOperationBackupHandler = PHY_ScanOperationBackup8192;
#endif

#ifdef RTL8190P
	priv->rtllib->SetBWModeHandler 		= rtl8192_SetBWMode;
	priv->rf_set_chan 						= rtl8192_phy_SwChnl;
	
#ifdef _ENABLE_SW_BEACON
	priv->rtllib->start_send_beacons = NULL;
	priv->rtllib->stop_send_beacons = NULL;
#else
	priv->rtllib->start_send_beacons = rtl8192e_start_beacon;
	priv->rtllib->stop_send_beacons = rtl8192_stop_beacon;
#endif
	
	priv->rtllib->GetNmodeSupportBySecCfg = rtl8192_GetNmodeSupportBySecCfg;
	priv->rtllib->GetHalfNmodeSupportByAPsHandler = rtl8192_GetHalfNmodeSupportByAPs;

	priv->rtllib->SetHwRegHandler = rtl8192e_SetHwReg;
	priv->rtllib->SetFwCmdHandler = NULL;
	priv->rtllib->InitialGainHandler = InitialGain819xPci;
#ifdef ENABLE_IPS
	priv->rtllib->rtllib_ips_leave_wq = rtllib_ips_leave_wq;
	priv->rtllib->rtllib_ips_leave = rtllib_ips_leave;
#endif

	priv->rtllib->LedControlHandler = NULL;
	priv->rtllib->UpdateBeaconInterruptHandler = NULL;

	priv->rtllib->ScanOperationBackupHandler = PHY_ScanOperationBackup8192;
#endif

#ifdef CONFIG_RTL_RFKILL
	priv->rtllib->rtllib_rfkill_poll = rtl8192_rfkill_poll; 
#else
	priv->rtllib->rtllib_rfkill_poll = NULL;
#endif
}

static void rtl8192_init_priv_constant(struct net_device* dev)
{
#if defined RTL8192SE || defined RTL8192CE || defined RTL8192E	
	struct r8192_priv *priv = rtllib_priv(dev);
	PRT_POWER_SAVE_CONTROL	pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));
#endif

#if defined RTL8192SE || defined RTL8192CE || defined RTL8192E	
	pPSC->RegMaxLPSAwakeIntvl = 5;
#endif
	
#ifdef RTL8192CE
	priv->bWEPinNmodeFromReg = 0;
	priv->bTKIPinNmodeFromReg = 0;

	priv->RegAMDPciASPM = 0;

	priv->RegPciASPM = 3; 

	priv->RegDevicePciASPMSetting = 0x03;

	priv->RegHostPciASPMSetting = 0x02;
	
	priv->RegHwSwRfOffD3 = 0; 
	
	priv->RegSupportPciASPM = 1;	
	
#elif defined RTL8192SE
	priv->RegPciASPM = 2; 

	priv->RegDevicePciASPMSetting = 0x03;

	priv->RegHostPciASPMSetting = 0x02;
	
	priv->RegHwSwRfOffD3 = 2; 
	
	priv->RegSupportPciASPM = 2;	
	
#elif defined RTL8192E
	priv->RegPciASPM = 2; 

	priv->RegDevicePciASPMSetting = 0x03;

	priv->RegHostPciASPMSetting = 0x02;

	priv->RegHwSwRfOffD3 = 2; 
	
	priv->RegSupportPciASPM = 2;
	
#elif defined RTL8190P
#endif	
}


static void rtl8192_init_priv_variable(struct net_device* dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8 i;
	priv->AcmMethod = eAcmWay2_SW;
	priv->dot11CurrentPreambleMode = PREAMBLE_AUTO;
	priv->rtllib->hwscan_sem_up = 1;
	priv->rtllib->status = 0;
	priv->H2CTxCmdSeq = 0;
	priv->bDisableFrameBursting = 0;
	priv->bDMInitialGainEnable = 1;	
	priv->polling_timer_on = 0;
	priv->up_first_time = 1;
	priv->blinked_ingpio = false;
	priv->bDriverIsGoingToUnload = false;
	priv->being_init_adapter = false;
	priv->initialized_at_probe = false;
	priv->sw_radio_on = true;
	priv->bdisable_nic = false;
	priv->bfirst_init = false;
	priv->txringcount = 64;
	priv->rxbuffersize = 9100;
	priv->rxringcount = MAX_RX_COUNT;
	priv->irq_enabled=0;
	priv->chan = 1; 
	priv->RegWirelessMode = WIRELESS_MODE_AUTO;
	priv->RegChannelPlan = 0xf;
	priv->nrxAMPDU_size = 0;
	priv->nrxAMPDU_aggr_num = 0;
	priv->last_rxdesc_tsf_high = 0;
	priv->last_rxdesc_tsf_low = 0;
	priv->rtllib->mode = WIRELESS_MODE_AUTO; 
	priv->rtllib->iw_mode = IW_MODE_INFRA;
	priv->rtllib->bNetPromiscuousMode = false;
	priv->rtllib->IntelPromiscuousModeInfo.bPromiscuousOn = false;
	priv->rtllib->IntelPromiscuousModeInfo.bFilterSourceStationFrame = false;
	priv->rtllib->ieee_up=0;
	priv->retry_rts = DEFAULT_RETRY_RTS;
	priv->retry_data = DEFAULT_RETRY_DATA;
	priv->rtllib->rts = DEFAULT_RTS_THRESHOLD;
	priv->rtllib->rate = 110; 
	priv->rtllib->short_slot = 1;
	priv->promisc = (dev->flags & IFF_PROMISC) ? 1:0;
	priv->bcck_in_ch14 = false;
	priv->bfsync_processing  = false;
	priv->CCKPresentAttentuation = 0;
	priv->rfa_txpowertrackingindex = 0;
	priv->rfc_txpowertrackingindex = 0;
	priv->CckPwEnl = 6;
	priv->ScanDelay = 50;
	priv->ResetProgress = RESET_TYPE_NORESET;
	priv->bForcedSilentReset = 0;
	priv->bDisableNormalResetCheck = false;
	priv->force_reset = false;
	memset(priv->rtllib->swcamtable,0,sizeof(SW_CAM_TABLE)*32);
#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
	priv->rtllib->APExtChlOffset = 0;
	priv->rtllib->set_key_for_AP = rtl8192_set_key_for_AP;
#endif
#ifdef ASL
	priv->RATRTableBitmap = 0;
	priv->rtllib->apdev_queue = (APDEV_QUEUE *)kmalloc((sizeof(APDEV_QUEUE)), GFP_KERNEL);
	if (!priv->rtllib->apdev_queue) {
		return;
	}
	memset((void *)priv->rtllib->apdev_queue, 0, sizeof (APDEV_QUEUE));
	apdev_init_queue(priv->rtllib->apdev_queue, APDEV_MAX_QUEUE_LEN, APDEV_MAX_BUFF_LEN);	
	priv->rtllib->current_ap_network.beacon_interval = DEFAULT_BEACONINTERVAL;	
	memset(priv->rtllib->swapratrtable,0,8*(sizeof(SW_RATR_TABLE)));
	for (i = 0; i < MAX_QUEUE_SIZE; i++) {
		skb_queue_head_init(&priv->rtllib->skb_apaggQ[i]);
	}
#endif
	memset(&priv->InterruptLog,0,sizeof(LOG_INTERRUPT_8190_T));
	priv->RxCounter = 0;
        priv->rtllib->wx_set_enc = 0;
	priv->bHwRadioOff = false;
	priv->RegRfOff = 0;
	priv->isRFOff = false;
	priv->bInPowerSaveMode = false;
	priv->rtllib->RfOffReason = 0;
	priv->RFChangeInProgress = false;
	priv->bHwRfOffAction = 0;
	priv->SetRFPowerStateInProgress = false;
	priv->rtllib->PowerSaveControl.bInactivePs = true;
	priv->rtllib->PowerSaveControl.bIPSModeBackup = false;
	priv->rtllib->PowerSaveControl.bLeisurePs = true;
	priv->rtllib->PowerSaveControl.bFwCtrlLPS = false;
	priv->rtllib->LPSDelayCnt = 0;
	priv->rtllib->sta_sleep = LPS_IS_WAKE;
	priv->rtllib->eRFPowerState = eRfOn;

	priv->txpower_checkcnt = 0;
	priv->thermal_readback_index =0;
	priv->txpower_tracking_callback_cnt = 0;
	priv->ccktxpower_adjustcnt_ch14 = 0;
	priv->ccktxpower_adjustcnt_not_ch14 = 0;

#if defined RTL8192SE 
	for(i = 0; i<PEER_MAX_ASSOC; i++){
		priv->rtllib->peer_assoc_list[i]=NULL;
		priv->rtllib->AvailableAIDTable[i] = 99; 
	}
	priv->RATRTableBitmap = 0;
	priv->rtllib->amsdu_in_process = 0;
#endif	
	
	priv->rtllib->current_network.beacon_interval = DEFAULT_BEACONINTERVAL;	
	priv->rtllib->iw_mode = IW_MODE_INFRA;
	priv->rtllib->active_scan = 1;
	priv->rtllib->be_scan_inprogress = false;
	priv->rtllib->modulation = RTLLIB_CCK_MODULATION | RTLLIB_OFDM_MODULATION;
	priv->rtllib->host_encrypt = 1;
	priv->rtllib->host_decrypt = 1;

	priv->rtllib->dot11PowerSaveMode = eActive;
#if defined (RTL8192S_WAPI_SUPPORT)
	priv->rtllib->wapiInfo.bWapiPSK = false;
#endif
	priv->rtllib->fts = DEFAULT_FRAG_THRESHOLD;
	priv->rtllib->MaxMssDensity = 0;
	priv->rtllib->MinSpaceCfg = 0;
	
	priv->card_type = PCI;

#if 0
	{
		PRT_POWER_SAVE_CONTROL	pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));
	pPSC->RegRfPsLevel |= RT_RF_OFF_LEVL_HALT_NIC;
	pPSC->RegRfPsLevel |= RT_RF_OFF_LEVL_CLK_REQ;
	pPSC->RegRfPsLevel |= RT_RF_OFF_LEVL_ASPM;
	pPSC->RegRfPsLevel |= RT_RF_LPS_LEVEL_ASPM;
		printk("=================>%s():RegRfPsLevel:%#x\n", __func__,pPSC->RegRfPsLevel);
	}
#endif

	priv->AcmControl = 0;	
	priv->pFirmware = (rt_firmware*)vmalloc(sizeof(rt_firmware));
	if (priv->pFirmware)
	memset(priv->pFirmware, 0, sizeof(rt_firmware));

        skb_queue_head_init(&priv->rx_queue);
	skb_queue_head_init(&priv->skb_queue);

	for(i = 0; i < MAX_QUEUE_SIZE; i++) {
		skb_queue_head_init(&priv->rtllib->skb_waitQ [i]);
	}
	for(i = 0; i < MAX_QUEUE_SIZE; i++) {
		skb_queue_head_init(&priv->rtllib->skb_aggQ [i]);
	}

}	

static void rtl8192_init_priv_lock(struct r8192_priv* priv)
{
	spin_lock_init(&priv->fw_scan_lock);
	spin_lock_init(&priv->tx_lock);
	spin_lock_init(&priv->irq_lock);
	spin_lock_init(&priv->irq_th_lock);
	spin_lock_init(&priv->rf_ps_lock);
	spin_lock_init(&priv->ps_lock);
	spin_lock_init(&priv->rf_lock);
	spin_lock_init(&priv->rt_h2c_lock);
#ifdef CONFIG_ASPM_OR_D3
	spin_lock_init(&priv->D3_lock);
#endif
	sema_init(&priv->wx_sem,1);
	sema_init(&priv->rf_sem,1);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16))
	sema_init(&priv->mutex, 1);
#else
	mutex_init(&priv->mutex);
#endif
}

static void rtl8192_init_priv_task(struct net_device* dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);	

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)	
#ifdef PF_SYNCTHREAD
	priv->priv_wq = create_workqueue(DRV_NAME,0);
#else	
	priv->priv_wq = create_workqueue(DRV_NAME);
#endif
#endif
	INIT_WORK_RSL(&priv->reset_wq,  (void*)rtl8192_restart, dev);
#ifdef ENABLE_IPS
	INIT_WORK_RSL(&priv->rtllib->ips_leave_wq, (void*)IPSLeave_wq, dev);  
#endif
	INIT_DELAYED_WORK_RSL(&priv->watch_dog_wq, (void*)rtl819x_watchdog_wqcallback, dev);
	INIT_DELAYED_WORK_RSL(&priv->txpower_tracking_wq,  (void*)dm_txpower_trackingcallback, dev);
	INIT_DELAYED_WORK_RSL(&priv->rfpath_check_wq,  (void*)dm_rf_pathcheck_workitemcallback, dev);
	INIT_DELAYED_WORK_RSL(&priv->update_beacon_wq, (void*)rtl8192_update_beacon, dev);
	INIT_WORK_RSL(&priv->qos_activate, (void*)rtl8192_qos_activate, dev);
#ifndef RTL8190P	
	INIT_DELAYED_WORK_RSL(&priv->rtllib->hw_wakeup_wq,(void*) rtl8192_hw_wakeup_wq, dev);
	INIT_DELAYED_WORK_RSL(&priv->rtllib->hw_sleep_wq,(void*) rtl8192_hw_sleep_wq, dev);
#endif
#if defined RTL8192SE 
	INIT_DELAYED_WORK_RSL(&priv->check_hw_scan_wq, (void*)rtl8192se_check_hw_scan, dev);
	INIT_DELAYED_WORK_RSL(&priv->hw_scan_simu_wq, (void*)rtl8192se_hw_scan_simu, dev);
	INIT_DELAYED_WORK_RSL(&priv->start_hw_scan_wq, (void*)rtl8192se_start_hw_scan, dev);
	INIT_DELAYED_WORK_RSL(&priv->rtllib->check_tsf_wq,(void*)rtl8192se_check_tsf_wq, dev);
	INIT_DELAYED_WORK_RSL(&priv->rtllib->update_assoc_sta_info_wq, 
			(void*)rtl8192se_update_assoc_sta_info_wq, dev);
#ifdef ASL
	INIT_DELAYED_WORK_RSL(&priv->rtllib->clear_sta_hw_sec_wq,(void*)clear_sta_hw_sec, dev);
	INIT_DELAYED_WORK_RSL(&priv->rtllib->apdev_set_wep_crypt_wq,
			(void*)apdev_set_wep_crypt, dev);
#endif
#endif
#ifdef _RTL8192_EXT_PATCH_
	INIT_WORK_RSL(&priv->rtllib->ext_create_crypt_for_peers_wq, (void*)msh_create_crypt_for_peers_wq, dev);
	INIT_WORK_RSL(&priv->rtllib->ext_path_sel_ops_wq,(void*) path_sel_ops_wq, dev);
	INIT_WORK_RSL(&priv->rtllib->ext_update_extchnloffset_wq, 
			(void*) meshdev_update_ext_chnl_offset_as_client, dev);
	INIT_DELAYED_WORK_RSL(&priv->rtllib->ext_wx_set_key_wq, (void*)ext_mesh_set_key_wq,priv->rtllib);
	INIT_WORK_RSL(&priv->rtllib->ext_start_mesh_protocol_wq, 
			(void*) meshdev_start_mesh_protocol_wq, dev);
#endif
	tasklet_init(&priv->irq_rx_tasklet,
	     (void(*)(unsigned long))rtl8192_irq_rx_tasklet,
	     (unsigned long)priv);
	tasklet_init(&priv->irq_tx_tasklet,
	     (void(*)(unsigned long))rtl8192_irq_tx_tasklet,
	     (unsigned long)priv);
        tasklet_init(&priv->irq_prepare_beacon_tasklet,
                (void(*)(unsigned long))rtl8192_prepare_beacon,
                (unsigned long)priv);
}

short rtl8192_get_channel_map(struct net_device * dev)
{
	int i;

#ifdef ENABLE_DOT11D
	struct r8192_priv *priv = rtllib_priv(dev);
	if ((priv->rf_chip != RF_8225) && (priv->rf_chip != RF_8256) 
			&& (priv->rf_chip != RF_6052)) {
		RT_TRACE(COMP_ERR, "%s: unknown rf chip, can't set channel map\n", __FUNCTION__);
		return -1;
	}
		
	if (priv->ChannelPlan > COUNTRY_CODE_MAX) {
		printk("rtl819x_init:Error channel plan! Set to default.\n");
		priv->ChannelPlan= COUNTRY_CODE_FCC;
	}
	RT_TRACE(COMP_INIT, "Channel plan is %d\n",priv->ChannelPlan);
	Dot11d_Init(priv->rtllib);
#if defined CONFIG_CRDA && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30) 
#else
	Dot11d_Channelmap(priv->ChannelPlan, priv->rtllib);
#endif	
#else
	struct r8192_priv *priv = rtllib_priv(dev);
	int ch;
	if(!channels){
		DMESG("No channels, aborting");
		return -1;
	}

	ch = channels;
	priv->ChannelPlan = 0;
	for (i = 1; i <= 14; i++) {
		(priv->rtllib->channel_map)[i] = (u8)(ch & 0x01);
		ch >>= 1;
	}
	priv->rtllib->IbssStartChnl= 10;
	priv->rtllib->ibss_maxjoin_chal = 11;
#endif
	for (i = 1; i <= 11; i++) {
		(priv->rtllib->active_channel_map)[i] = 1;
	}
	(priv->rtllib->active_channel_map)[12] = 2;
	(priv->rtllib->active_channel_map)[13] = 2;
	
	return 0;
}

short rtl8192_init(struct net_device *dev)
{		
	struct r8192_priv *priv = rtllib_priv(dev);

	memset(&(priv->stats),0,sizeof(struct Stats));

#ifdef CONFIG_BT_30
	bt_wifi_init(dev);
#endif

#ifdef CONFIG_MP
	rtl8192_init_mp(dev);
#endif	
	
	rtl8192_dbgp_flag_init(dev);

	rtl8192_init_priv_handler(dev);
	rtl8192_init_priv_constant(dev);
	rtl8192_init_priv_variable(dev);
#ifdef _RTL8192_EXT_PATCH_
	rtl8192_init_mesh_variable(dev);
#endif
	rtl8192_init_priv_lock(priv);
	rtl8192_init_priv_task(dev);
	priv->ops->get_eeprom_size(dev);
	priv->ops->init_adapter_variable(dev);
	rtl8192_get_channel_map(dev);

#if defined CONFIG_CFG_80211 && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)  
	/* channel map setting for the cfg80211 style */
	{
		struct r8192_priv* priv = rtllib_priv(dev);
		rtllib_set_geo(priv);	
	}
#endif

	init_hal_dm(dev);

#if defined RTL8192SE || defined RTL8192CE
	InitSwLeds(dev);
#endif
	init_timer(&priv->watch_dog_timer);
	setup_timer(&priv->watch_dog_timer,
		    watch_dog_timer_callback, 
		    (unsigned long) dev);

	init_timer(&priv->gpio_polling_timer);
	setup_timer(&priv->gpio_polling_timer, 
		    check_rfctrl_gpio_timer,
		    (unsigned long)dev);

#ifdef CONFIG_SW_ANT_SWICH
	init_timer(&priv->SwAntennaSwitchTimer);
	setup_timer(&priv->SwAntennaSwitchTimer, 
		    dm_SW_AntennaSwitchCallback,
		    (unsigned long)dev);
#endif
	
	rtl8192_irq_disable(dev);
#if defined(IRQF_SHARED)
        if (request_irq(dev->irq, (void*)rtl8192_interrupt_rsl, IRQF_SHARED, dev->name, dev)) 
#else
        if (request_irq(dev->irq, (void *)rtl8192_interrupt_rsl, SA_SHIRQ, dev->name, dev)) 
#endif
	{
		printk("Error allocating IRQ %d",dev->irq);
		return -1;
	} else { 
		priv->irq=dev->irq;
		RT_TRACE(COMP_INIT, "IRQ %d\n",dev->irq);
	}

	if (rtl8192_pci_initdescring(dev) != 0) { 
		printk("Endopoints initialization failed");
		return -1;
	}

	return 0;
}

#if defined CONFIG_ASPM_OR_D3
static void
rtl8192_update_default_setting(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PRT_POWER_SAVE_CONTROL	pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));
	
	pPSC->RegRfPsLevel = 0;
	priv->bSupportASPM = 0;


	pPSC->RegAMDPciASPM = priv->RegAMDPciASPM ;
	switch(priv->RegPciASPM)
	{
	case 0: 
		break;
		
	case 1: 
		pPSC->RegRfPsLevel |= RT_RF_LPS_LEVEL_ASPM;
		break;

	case 2: 
		pPSC->RegRfPsLevel |= (RT_RF_LPS_LEVEL_ASPM | RT_RF_OFF_LEVL_CLK_REQ);
		break;

	case 3: 
		pPSC->RegRfPsLevel &= ~(RT_RF_LPS_LEVEL_ASPM);
		pPSC->RegRfPsLevel |= (RT_RF_PS_LEVEL_ALWAYS_ASPM | RT_RF_OFF_LEVL_CLK_REQ);
		break;

	case 4: 
		pPSC->RegRfPsLevel &= ~(RT_RF_LPS_LEVEL_ASPM | RT_RF_OFF_LEVL_CLK_REQ);
		pPSC->RegRfPsLevel |= RT_RF_PS_LEVEL_ALWAYS_ASPM;
		break;
	}

	pPSC->RegRfPsLevel |= RT_RF_OFF_LEVL_HALT_NIC;

	switch(priv->RegHwSwRfOffD3)
	{
	case 1:
		if(pPSC->RegRfPsLevel & RT_RF_LPS_LEVEL_ASPM)
			pPSC->RegRfPsLevel |= RT_RF_OFF_LEVL_ASPM;
		break;

	case 2:
		if(pPSC->RegRfPsLevel & RT_RF_LPS_LEVEL_ASPM)
			pPSC->RegRfPsLevel |= RT_RF_OFF_LEVL_ASPM;
		pPSC->RegRfPsLevel |= RT_RF_OFF_LEVL_HALT_NIC;
		break;

	case 3:
		pPSC->RegRfPsLevel |= RT_RF_OFF_LEVL_PCI_D3;
		break;
	}


	switch(priv->RegSupportPciASPM)
	{
	case 0: 
		{
			bool		bSupportASPM = false;
			priv->bSupportASPM = bSupportASPM;
		}
		break;

	case 1: 
		{
			bool		bSupportASPM = true;
			bool		bSupportBackDoor = true;

			priv->bSupportASPM = bSupportASPM;

			if(priv->CustomerID == RT_CID_TOSHIBA && !priv->NdisAdapter.AMDL1PATCH)
				bSupportBackDoor = false;
			
			priv->bSupportBackDoor = bSupportBackDoor;
		}
		break;

	case 2: 
		if(priv->NdisAdapter.PciBridgeVendor == PCI_BRIDGE_VENDOR_INTEL)
		{
			bool		bSupportASPM = true;
			priv->bSupportASPM = bSupportASPM;
		}
		break;
		
	default:
		break;
	}
}
#endif

#if defined CONFIG_ASPM_OR_D3
static void
rtl8192_initialize_adapter_common(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PRT_POWER_SAVE_CONTROL	pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));

	rtl8192_update_default_setting(dev);

#ifdef CONFIG_ASPM_OR_D3
	if(pPSC->RegRfPsLevel & RT_RF_PS_LEVEL_ALWAYS_ASPM)
	{
		RT_ENABLE_ASPM(dev);
		RT_SET_PS_LEVEL(pPSC, RT_RF_PS_LEVEL_ALWAYS_ASPM);
	}
#endif
}
#endif

/***************************************************************************
    -------------------------------WATCHDOG STUFF---------------------------
***************************************************************************/
short rtl8192_is_tx_queue_empty(struct net_device *dev)
{
	int i=0;
	struct r8192_priv *priv = rtllib_priv(dev);
	for (i=0; i<=MGNT_QUEUE; i++)
	{
		if ((i== TXCMD_QUEUE) || (i == HCCA_QUEUE) )
			continue;
		if (skb_queue_len(&(&priv->tx_ring[i])->queue) > 0){
			printk("===>tx queue is not empty:%d, %d\n", i, skb_queue_len(&(&priv->tx_ring[i])->queue));
			return 0;
		}
	}
	return 1;
}

RESET_TYPE
rtl819x_TxCheckStuck(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8			QueueID;
	u8			ResetThreshold = NIC_SEND_HANG_THRESHOLD_POWERSAVE;
	bool			bCheckFwTxCnt = false;
	struct rtl8192_tx_ring  *ring = NULL;
	struct sk_buff* skb = NULL;
	cb_desc * tcb_desc = NULL;
	unsigned long flags = 0;
	
#if 0
	switch (priv->rtllib->dot11PowerSaveMode)
	{
		case eActive:		
			ResetThreshold = NIC_SEND_HANG_THRESHOLD_NORMAL;
			break;
		case eMaxPs:		
			ResetThreshold = NIC_SEND_HANG_THRESHOLD_POWERSAVE;
			break;
		case eFastPs:	
			ResetThreshold = NIC_SEND_HANG_THRESHOLD_POWERSAVE;
			break;
		default:
			break;
	}
#else
	switch (priv->rtllib->ps)
	{
		case RTLLIB_PS_DISABLED:		
			ResetThreshold = NIC_SEND_HANG_THRESHOLD_NORMAL;
			break;
		case (RTLLIB_PS_MBCAST|RTLLIB_PS_UNICAST):		
			ResetThreshold = NIC_SEND_HANG_THRESHOLD_POWERSAVE;
			break;
		default:
			ResetThreshold = NIC_SEND_HANG_THRESHOLD_POWERSAVE;
			break;
	}
#endif
	spin_lock_irqsave(&priv->irq_th_lock,flags);
	for(QueueID = 0; QueueID < MAX_TX_QUEUE; QueueID++)
	{
		if(QueueID == TXCMD_QUEUE)
			continue;

		if(QueueID == BEACON_QUEUE)
			continue;

		ring = &priv->tx_ring[QueueID];

		if(skb_queue_len(&ring->queue) == 0)
			continue;
		else
		{
			skb = (&ring->queue)->next;
			tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
			tcb_desc->nStuckCount++;
			bCheckFwTxCnt = true;
			if(tcb_desc->nStuckCount > 1)
				printk("%s: QueueID=%d tcb_desc->nStuckCount=%d\n",__func__,QueueID,tcb_desc->nStuckCount);
#if defined RTL8192SE || defined RTL8192CE
			if(tcb_desc->nStuckCount > ResetThreshold)
			{
				RT_TRACE( COMP_RESET, "TxCheckStuck(): Need silent reset because nStuckCount > ResetThreshold.\n" );
                                spin_unlock_irqrestore(&priv->irq_th_lock,flags);				
				return RESET_TYPE_SILENT;
			}
			bCheckFwTxCnt = false;
			#endif
		}
	}
	spin_unlock_irqrestore(&priv->irq_th_lock,flags);
	
	if(bCheckFwTxCnt) {
		if (priv->ops->TxCheckStuckHandler(dev))
		{
			RT_TRACE(COMP_RESET, "TxCheckStuck(): Fw indicates no Tx condition! \n");
			return RESET_TYPE_SILENT;
		}
	}

	return RESET_TYPE_NORESET;
}

RESET_TYPE rtl819x_RxCheckStuck(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	if(priv->ops->RxCheckStuckHandler(dev))
	{
		RT_TRACE(COMP_RESET, "RxStuck Condition\n");
		return RESET_TYPE_SILENT;
	}
	
	return RESET_TYPE_NORESET;
}

RESET_TYPE
rtl819x_ifcheck_resetornot(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	RESET_TYPE	TxResetType = RESET_TYPE_NORESET;
	RESET_TYPE	RxResetType = RESET_TYPE_NORESET;
	RT_RF_POWER_STATE 	rfState;
	
	rfState = priv->rtllib->eRFPowerState;
	
	if(rfState == eRfOn)
		TxResetType = rtl819x_TxCheckStuck(dev);
	
	if( rfState == eRfOn && 
	    (priv->rtllib->iw_mode == IW_MODE_INFRA) && 
	    (priv->rtllib->state == RTLLIB_LINKED)) {


		RxResetType = rtl819x_RxCheckStuck(dev);
	}
	
	if(TxResetType==RESET_TYPE_NORMAL || RxResetType==RESET_TYPE_NORMAL){
		printk("%s(): TxResetType is %d, RxResetType is %d\n",__FUNCTION__,TxResetType,RxResetType);
		return RESET_TYPE_NORMAL;
	} else if(TxResetType==RESET_TYPE_SILENT || RxResetType==RESET_TYPE_SILENT){
		printk("%s(): TxResetType is %d, RxResetType is %d\n",__FUNCTION__,TxResetType,RxResetType);
		return RESET_TYPE_SILENT;
	} else {
		return RESET_TYPE_NORESET;
	}

}

#ifdef _RTL8192_EXT_PATCH_
void rtl819x_silentreset_mesh_start(struct net_device *dev, 
				u8 *pbackup_channel_wlan, 
				u8 *pbackup_channel_mesh,
				u8 *pIsPortal)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;
	
	*pbackup_channel_wlan = ieee->current_network.channel;
	*pbackup_channel_mesh = ieee->current_mesh_network.channel;
	if((ieee->state == RTLLIB_LINKED) && ((ieee->iw_mode == IW_MODE_INFRA) || (ieee->iw_mode == IW_MODE_ADHOC)))
	{
		printk("====>down, infra or adhoc\n");
		SEM_DOWN_IEEE_WX(&ieee->wx_sem);
		printk("ieee->state is RTLLIB_LINKED\n");
		rtllib_stop_send_beacons(priv->rtllib);
		del_timer_sync(&ieee->associate_timer);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)	
		cancel_delayed_work(&ieee->associate_retry_wq);	
#endif	
		rtllib_stop_scan(ieee);
		netif_carrier_off(dev);
		SEM_UP_IEEE_WX(&ieee->wx_sem);
	}
	else if((ieee->iw_mode == IW_MODE_MESH) && (ieee->mesh_state == RTLLIB_MESH_LINKED))
	{
		if(priv->mshobj->ext_patch_r819x_wx_get_AsPortal)
			priv->mshobj->ext_patch_r819x_wx_get_AsPortal(priv, pIsPortal);
		if((!ieee->only_mesh) && (ieee->state == RTLLIB_LINKED)){
			printk("====>down, wlan server\n");
			SEM_DOWN_IEEE_WX(&ieee->wx_sem);
			printk("ieee->state is RTLLIB_LINKED\n");
			rtllib_stop_send_beacons(priv->rtllib);
			del_timer_sync(&ieee->associate_timer);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)	
			cancel_delayed_work(&ieee->associate_retry_wq);	
#endif	
			rtllib_stop_scan(ieee);
			netif_carrier_off(dev);
			SEM_UP_IEEE_WX(&ieee->wx_sem);
			if(priv->mshobj->ext_patch_rtllib_stop_protocol)
				priv->mshobj->ext_patch_rtllib_stop_protocol(ieee,1);
		}
		else if((!ieee->only_mesh) && (ieee->state != RTLLIB_LINKED)){
			printk("====>down, wlan server\n");
			SEM_DOWN_IEEE_WX(&ieee->wx_sem);
			printk("ieee->state is Not RTLLIB_LINKED\n");
			rtllib_stop_send_beacons(priv->rtllib);
			rtllib_stop_scan(ieee);
			netif_carrier_off(dev);
			SEM_UP_IEEE_WX(&ieee->wx_sem);
			if(priv->mshobj->ext_patch_rtllib_stop_protocol)
				priv->mshobj->ext_patch_rtllib_stop_protocol(ieee,1);
		}
		else if(ieee->only_mesh && (*pIsPortal))
		{
			printk("====>down, eth0 server\n");
			if(priv->mshobj->ext_patch_rtllib_stop_protocol)
				priv->mshobj->ext_patch_rtllib_stop_protocol(ieee,1);
		}
		else if(ieee->only_mesh && !(*pIsPortal))
		{
			printk("====>down, p2p or client\n");
			if(priv->mshobj->ext_patch_rtllib_stop_protocol)
				priv->mshobj->ext_patch_rtllib_stop_protocol(ieee,1);
		}
		else{
			printk("====>down, no link\n");
			if(priv->mshobj->ext_patch_rtllib_stop_protocol)
				priv->mshobj->ext_patch_rtllib_stop_protocol(ieee,1);
		}
	}
	else{
		printk("====>down, no link\n");
		printk("ieee->state is NOT LINKED\n");
		rtllib_softmac_stop_protocol(priv->rtllib,0, 0, true);		
	}
}
#endif

void rtl819x_silentreset_mesh_bk(struct net_device *dev, u8 IsPortal)
{
#ifdef _RTL8192_EXT_PATCH_
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;
	u8 updateBW = 0;
	u8 bserverHT = 0;

	if(!ieee->only_mesh) 
	{	
		printk("===>up, wlan0 server\n");
		ieee->set_chan(ieee->dev, ieee->current_network.channel);
	
		queue_work_rsl(ieee->wq, &ieee->associate_complete_wq);
		if (ieee->current_mesh_network.beacon_interval == 0)
			ieee->current_mesh_network.beacon_interval = 100;
		ieee->mesh_state = RTLLIB_MESH_LINKED;
		ieee->link_change(ieee->dev);
		if(priv->mshobj->ext_patch_rtllib_start_protocol)
			priv->mshobj->ext_patch_rtllib_start_protocol(ieee);
	}
	else if(ieee->only_mesh && IsPortal)
	{
		printk("===>up, eth0 server\n");
		if (ieee->current_mesh_network.beacon_interval == 0)
			ieee->current_mesh_network.beacon_interval = 100;
		ieee->mesh_state = RTLLIB_MESH_LINKED;
		ieee->link_change(ieee->dev);
		if(priv->mshobj->ext_patch_rtllib_start_protocol)
			priv->mshobj->ext_patch_rtllib_start_protocol(ieee);
		ieee->current_network.channel = ieee->current_mesh_network.channel; 
		if(ieee->pHTInfo->bCurBW40MHz)
			HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20_40, (ieee->current_mesh_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER);  
		else
			HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20, (ieee->current_mesh_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER);  
	}
	else if(ieee->only_mesh && !IsPortal)
	{
		printk("===>up, p2p or client\n");
		if (ieee->current_mesh_network.beacon_interval == 0)
			ieee->current_mesh_network.beacon_interval = 100;
		ieee->mesh_state = RTLLIB_MESH_LINKED;
		ieee->link_change(ieee->dev);
		if(priv->mshobj->ext_patch_rtllib_start_protocol)
			priv->mshobj->ext_patch_rtllib_start_protocol(ieee);
		if(ieee->p2pmode){
			printk("===>up, p2p\n");
			ieee->current_network.channel = ieee->current_mesh_network.channel; 
			if(ieee->pHTInfo->bCurBW40MHz)
				HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20_40, (ieee->current_mesh_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER);  
			else
				HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20, (ieee->current_mesh_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER);  
		}else{
			printk("===>up, client\n");
			updateBW = priv->mshobj->ext_patch_r819x_wx_update_beacon(ieee->dev,&bserverHT);
			printk("$$$$$$ Cur_networ.chan=%d, cur_mesh_net.chan=%d,bserverHT=%d\n",
			ieee->current_network.channel,ieee->current_mesh_network.channel,bserverHT);
			if (updateBW == 1) {
				if (bserverHT == 0) {
					printk("===>server is not HT supported,set 20M\n");
					HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20, HT_EXTCHNL_OFFSET_NO_EXT);  
				} else {
					printk("===>updateBW is 1,bCurBW40MHz is %d,ieee->serverExtChlOffset is %d\n",
							ieee->pHTInfo->bCurBW40MHz,ieee->serverExtChlOffset);
					if (ieee->pHTInfo->bCurBW40MHz)
						HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20_40, ieee->serverExtChlOffset);  
					else
						HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20, ieee->serverExtChlOffset);  
				}
			} else {
				printk("===>there is no same hostname server, ERR!!!\n");
			}
		}
	}
#endif
}				

void rtl819x_ifsilentreset(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8	reset_times = 0;
	int reset_status = 0;
	struct rtllib_device *ieee = priv->rtllib;
	unsigned long flag;
	
#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
	bool wlansilentreset = false;
       	u8 backup_channel_wlan = 1;	
	int i=0;
#endif
#ifdef _RTL8192_EXT_PATCH_
	bool meshsilentreset = false;
	u8 backup_channel_mesh = 1;
#endif	
#ifdef ASL
	struct sta_info * psta = NULL;
	bool apdevsilentreset = false;
	u8 backup_channel_ap = 1;
#endif
	u8 IsPortal = 0;

		
	if(priv->ResetProgress==RESET_TYPE_NORESET) {

		RT_TRACE(COMP_RESET,"=========>Reset progress!! \n");
		
		priv->ResetProgress = RESET_TYPE_SILENT;
		
		spin_lock_irqsave(&priv->rf_ps_lock,flag);
		if(priv->RFChangeInProgress)
		{
			spin_unlock_irqrestore(&priv->rf_ps_lock,flag);
			goto END;
		}
		priv->RFChangeInProgress = true;
		priv->bResetInProgress = true;
		spin_unlock_irqrestore(&priv->rf_ps_lock,flag);

RESET_START:	
	
		down(&priv->wx_sem);	
		
#ifdef ENABLE_LPS
		if(priv->rtllib->state == RTLLIB_LINKED)
			LeisurePSLeave(dev);
#endif

		if (IS_NIC_DOWN(priv)) {
			RT_TRACE(COMP_ERR,"%s():the driver is not up! return\n",__FUNCTION__);
			up(&priv->wx_sem);
			return ;
		}
#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
		if (priv->up == 1) {
			printk("================>wlansilentreset is true\n");
			wlansilentreset = true;
			priv->up = 0;
		}
#else
		priv->up = 0;
#endif
#ifdef _RTL8192_EXT_PATCH_
		if (priv->mesh_up == 1) {
			printk("================>meshsilentreset is true\n");
			meshsilentreset = true;
			priv->mesh_up = 0;
		}
#endif
#ifdef ASL
		if (priv->apdev_up == 1) {
			printk("================>apdevsilentreset is true\n");
			apdevsilentreset = true;
			priv->apdev_up = 0;
		}
		backup_channel_wlan = ieee->current_network.channel;
		backup_channel_ap = ieee->current_ap_network.channel;
#endif

		RT_TRACE(COMP_RESET,"%s():======>start to down the driver\n",__FUNCTION__);
		mdelay(1000);
		RT_TRACE(COMP_RESET,"%s():111111111111111111111111======>start to down the driver\n",__FUNCTION__);
#ifdef ASL
		if (apdevsilentreset) {
		    if(!netif_queue_stopped(ieee->apdev))
			netif_stop_queue(ieee->apdev);
		}
		if (wlansilentreset) {
			if(!netif_queue_stopped(dev))
				netif_stop_queue(dev);
		}
#else
		if(!netif_queue_stopped(dev))
			netif_stop_queue(dev);
#endif

		rtl8192_irq_disable(dev);
		del_timer_sync(&priv->watch_dog_timer);			
		rtl8192_cancel_deferred_work(priv);
		deinit_hal_dm(dev);
		rtllib_stop_scan_syncro(ieee);
		
#ifdef _RTL8192_EXT_PATCH_
		rtl819x_silentreset_mesh_start(dev, &backup_channel_wlan, 
			&backup_channel_mesh, &IsPortal);				
#else
		if ((ieee->state == RTLLIB_LINKED) && (ieee->iw_mode == IW_MODE_INFRA || ieee->iw_mode == IW_MODE_ADHOC)) {
			SEM_DOWN_IEEE_WX(&ieee->wx_sem);
			printk("ieee->state is RTLLIB_LINKED\n");
			rtllib_stop_send_beacons(priv->rtllib);
			del_timer_sync(&ieee->associate_timer);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)	
			cancel_delayed_work(&ieee->associate_retry_wq);	
#endif	
			rtllib_stop_scan(ieee);
			netif_carrier_off(dev);
			SEM_UP_IEEE_WX(&ieee->wx_sem);
		}
#ifdef ASL
		else if(ieee->ap_state == RTLLIB_LINKED && (ieee->iw_mode == IW_MODE_MASTER || ieee->iw_mode == IW_MODE_APSTA)) {
			SEM_DOWN_IEEE_WX(&ieee->wx_sem);
			printk("ieee->ap_state is RTLLIB_LINKED,and in AP mode\n");
			rtllib_stop_send_beacons(priv->rtllib);
			netif_carrier_off(ieee->apdev);
			if (ieee->state == RTLLIB_LINKED && ieee->iw_mode == IW_MODE_APSTA) {
				del_timer_sync(&ieee->associate_timer);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)	
				cancel_delayed_work(&ieee->associate_retry_wq);	
#endif	
				rtllib_stop_scan(ieee);
			netif_carrier_off(dev);
			}
			SEM_UP_IEEE_WX(&ieee->wx_sem);
		}
#endif
		else if (ieee->state != RTLLIB_LINKED) {
			printk("ieee->state is NOT LINKED\n");
			rtllib_softmac_stop_protocol(priv->rtllib, 0, 0,true);		
		}
#endif

#if !(defined RTL8192SE || defined RTL8192CE)
		dm_backup_dynamic_mechanism_state(dev);
#endif

#ifdef RTL8190P
		priv->ops->stop_adapter(dev, true);
#endif

		up(&priv->wx_sem);
		RT_TRACE(COMP_RESET,"%s():<==========down process is finished\n",__FUNCTION__);	

		RT_TRACE(COMP_RESET,"%s():<===========up process start\n",__FUNCTION__);		
#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
		if(wlansilentreset == true){
			reset_status = _rtl8192_up(dev,true);
		}
#else
		reset_status = _rtl8192_up(dev,true);
#endif	
#ifdef _RTL8192_EXT_PATCH_
		if(meshsilentreset == true)
			reset_status = meshdev_up(ieee->meshdev,true);
#endif	
#ifdef ASL
		if(apdevsilentreset == true){
			reset_status = apdev_up(ieee->apdev, true);
		}
#endif
		RT_TRACE(COMP_RESET,"%s():<===========up process is finished\n",__FUNCTION__);
		if (reset_status == -1) {
			if(reset_times < 3) {
				reset_times++;
				goto RESET_START;
			} else {
				RT_TRACE(COMP_ERR," ERR!!! %s():  Reset Failed!!\n",__FUNCTION__);
			}
		}
		
		ieee->is_silent_reset = 1;
		
		spin_lock_irqsave(&priv->rf_ps_lock,flag);
		priv->RFChangeInProgress = false;
		spin_unlock_irqrestore(&priv->rf_ps_lock,flag);
		
		EnableHWSecurityConfig8192(dev);
		
#ifdef _RTL8192_EXT_PATCH_
		ieee->current_network.channel = backup_channel_wlan;
		ieee->current_mesh_network.channel = backup_channel_mesh;
#endif
#ifdef ASL
		ieee->current_network.channel = backup_channel_wlan;
		ieee->current_ap_network.channel = backup_channel_ap;
#endif
		if (ieee->state == RTLLIB_LINKED && ieee->iw_mode == IW_MODE_INFRA) {
			ieee->set_chan(ieee->dev, ieee->current_network.channel);
		
			queue_work_rsl(ieee->wq, &ieee->associate_complete_wq);
	
		} else if (ieee->state == RTLLIB_LINKED && ieee->iw_mode == IW_MODE_ADHOC) {
			ieee->set_chan(ieee->dev, ieee->current_network.channel);
			ieee->link_change(ieee->dev);
	
			notify_wx_assoc_event(ieee);
	
			rtllib_start_send_beacons(ieee);
	
			if (ieee->data_hard_resume)
				ieee->data_hard_resume(ieee->dev);
			netif_carrier_on(ieee->dev);
		}
#ifdef ASL
		else if (ieee->ap_state == RTLLIB_LINKED && (ieee->iw_mode == IW_MODE_MASTER || ieee->iw_mode == IW_MODE_APSTA)) {
			RT_TRACE(COMP_RESET,"restore the sta info in AP mode\n");
			if (ieee->state == RTLLIB_LINKED && ieee->iw_mode == IW_MODE_APSTA) {
				ieee->set_chan(ieee->dev, ieee->current_network.channel);
				queue_work_rsl(ieee->wq, &ieee->associate_complete_wq);
			}
			ap_start_aprequest(dev);
			queue_delayed_work_rsl(ieee->wq, &ieee->update_assoc_sta_info_wq,0);
		}
#endif
		else if (ieee->iw_mode == IW_MODE_MESH) {
			rtl819x_silentreset_mesh_bk(dev, IsPortal);
#ifdef _RTL8192_EXT_PATCH_
			netif_carrier_on(ieee->meshdev);
#endif
		}
		
#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
		if(wlansilentreset){
			printk("==========>wlansilentreset\n");
			CamRestoreEachIFEntry(dev, 0, 0);
		}
#ifdef _RTL8192_EXT_PATCH_
		if(meshsilentreset){
			printk("==========>meshsilentreset\n");
			CamRestoreEachIFEntry(dev,1,0);
			if (!ieee->bUseRAMask) {
			for(i=0;i<8;i++)
			{
				if(ieee->swmeshratrtable[i].bused == true)
				{
					printk("====>restore ratr table: index=%d,value=%x\n",i,ieee->swmeshratrtable[i].ratr_value);
#ifdef RTL8192SE
					write_nic_dword(dev,ARFR0+i*4,ieee->swmeshratrtable[i].ratr_value);
#elif defined Rtl8192CE
					write_nic_dword(dev,REG_ARFR0+i*4,ieee->swmeshratrtable[i].ratr_value);
#endif
				}
			}
			} else {
			}
		}
#endif
#ifdef ASL
		if(apdevsilentreset){
			printk("==========>apdevsilentreset\n");
			CamRestoreEachIFEntry(dev,0,1);
			if (!ieee->bUseRAMask) {
				for (i=0;i<8;i++) {
					if(ieee->swapratrtable[i].bused == true)
					{
						printk("====>restore ratr table: index=%d,value=%x\n",i,ieee->swapratrtable[i].ratr_value);
#ifdef RTL8192SE
						write_nic_dword(dev,ARFR0+i*4,ieee->swapratrtable[i].ratr_value);
#elif defined Rtl8192CE
						write_nic_dword(dev,REG_ARFR0+i*4,ieee->swapratrtable[i].ratr_value);
#endif
					}
				}
			} else {
				for(i = 0; i < APDEV_MAX_ASSOC; i++) {
					psta = ieee->apdev_assoc_list[i];
					if (psta) {
						if (psta->swratrmask.bused == true) {
							printk("%s(): mask = %x, bitmap = %x\n",__func__, psta->swratrmask.mask, psta->swratrmask.ratr_bitmap);
							write_nic_dword(dev, 0x2c4, psta->swratrmask.ratr_bitmap);
							write_nic_dword(dev, WFM5, (FW_RA_UPDATE_MASK | (psta->swratrmask.mask << 8)));
						}
					}
				}
			}
		}
#endif
#else
		CamRestoreAllEntry(dev);
#endif
#if !(defined RTL8192SE || defined RTL8192CE)
		dm_restore_dynamic_mechanism_state(dev);
#endif
END:
		priv->ResetProgress = RESET_TYPE_NORESET;
		priv->reset_count++;

		priv->bForcedSilentReset =false;
		priv->bResetInProgress = false;

#if !(defined RTL8192SE || defined RTL8192CE)
		write_nic_byte(dev, UFWP, 1);	
#endif
		RT_TRACE(COMP_RESET, "Reset finished!! ====>[%d]\n", priv->reset_count);
	}
}

void rtl819x_update_rxcounts(struct r8192_priv *priv,
			     u32 *TotalRxBcnNum,
			     u32 *TotalRxDataNum)	
{
	u16 			SlotIndex;
	u8			i;

	*TotalRxBcnNum = 0;
	*TotalRxDataNum = 0;

	SlotIndex = (priv->rtllib->LinkDetectInfo.SlotIndex++)%(priv->rtllib->LinkDetectInfo.SlotNum);
	priv->rtllib->LinkDetectInfo.RxBcnNum[SlotIndex] = priv->rtllib->LinkDetectInfo.NumRecvBcnInPeriod;
	priv->rtllib->LinkDetectInfo.RxDataNum[SlotIndex] = priv->rtllib->LinkDetectInfo.NumRecvDataInPeriod;
	for (i = 0; i < priv->rtllib->LinkDetectInfo.SlotNum; i++) {	
		*TotalRxBcnNum += priv->rtllib->LinkDetectInfo.RxBcnNum[i];
		*TotalRxDataNum += priv->rtllib->LinkDetectInfo.RxDataNum[i];
	}
}


void	rtl819x_watchdog_wqcallback(void *data)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
	struct r8192_priv *priv = container_of_dwork_rsl(data,struct r8192_priv,watch_dog_wq);
	struct net_device *dev = priv->rtllib->dev;
#else
	struct net_device *dev = (struct net_device *)data;
	struct r8192_priv *priv = rtllib_priv(dev);
#endif
	struct rtllib_device* ieee = priv->rtllib;
	RESET_TYPE	ResetType = RESET_TYPE_NORESET;
	static u8	check_reset_cnt = 0;
#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
	static u8	last_reset_count = 0;
#endif
	unsigned long flags;
	PRT_POWER_SAVE_CONTROL pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));
	bool bBusyTraffic = false;
	bool	bHigherBusyTraffic = false;
	bool	bHigherBusyRxTraffic = false;
#ifdef ENABLE_LPS
	bool bEnterPS = false;
#endif
	if(IS_NIC_DOWN(priv) || (priv->bHwRadioOff == true))
		return;

	if(priv->rtllib->state >= RTLLIB_LINKED)
	{
		if(priv->rtllib->CntAfterLink<2)
			priv->rtllib->CntAfterLink++;
	}
	else
	{
		priv->rtllib->CntAfterLink = 0;
	}
	
	hal_dm_watchdog(dev);

#ifdef ENABLE_IPS
	if(rtllib_act_scanning(priv->rtllib,false) == false){
		if((ieee->iw_mode == IW_MODE_INFRA) && (ieee->state == RTLLIB_NOLINK) &&\
		    (ieee->eRFPowerState == eRfOn)&&!ieee->is_set_key &&\
		    (!ieee->proto_stoppping) && !ieee->wx_set_enc
#ifdef CONFIG_RTLWIFI_DEBUGFS	    
		    && (!priv->debug->hw_holding)
#endif		    
		 ){
			if((ieee->PowerSaveControl.ReturnPoint == IPS_CALLBACK_NONE)&& 
			    (!ieee->bNetPromiscuousMode))
			{
				RT_TRACE(COMP_PS, "====================>haha:IPSEnter()\n");
				IPSEnter(dev);	
			}
		}
	}
#endif
#ifdef _RTL8192_EXT_PATCH_
	if((ieee->iw_mode == IW_MODE_INFRA) || (ieee->iw_mode == IW_MODE_MESH && ieee->only_mesh == 0))
		MgntLinkKeepAlive(priv);
#endif
#ifdef ASL
	if((ieee->iw_mode == IW_MODE_INFRA) || (ieee->iw_mode == IW_MODE_APSTA ))
		MgntLinkKeepAlive(priv);
#endif
	{
		if((ieee->state == RTLLIB_LINKED) && (ieee->iw_mode == IW_MODE_INFRA) && (!ieee->bNetPromiscuousMode))
		{
			if(	ieee->LinkDetectInfo.NumRxOkInPeriod> 100 ||
				ieee->LinkDetectInfo.NumTxOkInPeriod> 100 ) {
				bBusyTraffic = true;
			}
			

			if( ieee->LinkDetectInfo.NumRxOkInPeriod > 4000 ||
				ieee->LinkDetectInfo.NumTxOkInPeriod > 4000 )
			{
				bHigherBusyTraffic = true;
				if(ieee->LinkDetectInfo.NumRxOkInPeriod > 5000)
					bHigherBusyRxTraffic = true;
				else
					bHigherBusyRxTraffic = false;
			}
				
#ifdef ENABLE_LPS
			if(	((ieee->LinkDetectInfo.NumRxUnicastOkInPeriod + ieee->LinkDetectInfo.NumTxOkInPeriod) > 8 ) ||
				(ieee->LinkDetectInfo.NumRxUnicastOkInPeriod > 2) )
			{
				bEnterPS= false;
			}
			else
			{
				bEnterPS= true;
			}

			if (ieee->current_network.beacon_interval < 95)
				bEnterPS= false;

			if(bEnterPS)
			{
				LeisurePSEnter(dev);
			}
			else
			{
				LeisurePSLeave(dev);
			}
#endif
		
		}
		else
		{
#ifdef ENABLE_LPS
			RT_TRACE(COMP_LPS,"====>no link LPS leave\n");
			LeisurePSLeave(dev);
#endif
		}

	       ieee->LinkDetectInfo.NumRxOkInPeriod = 0;
	       ieee->LinkDetectInfo.NumTxOkInPeriod = 0;
	       ieee->LinkDetectInfo.NumRxUnicastOkInPeriod = 0;
	       ieee->LinkDetectInfo.bBusyTraffic = bBusyTraffic;

		ieee->LinkDetectInfo.bHigherBusyTraffic = bHigherBusyTraffic;
		ieee->LinkDetectInfo.bHigherBusyRxTraffic = bHigherBusyRxTraffic;

	}

	{
#if defined RTL8192SE 
		if(priv->rtllib->iw_mode == IW_MODE_ADHOC)
			IbssAgeFunction(ieee);
#endif
#ifdef ASL
		if(ieee->state == RTLLIB_LINKED && (ieee->iw_mode == IW_MODE_INFRA ||ieee->iw_mode == IW_MODE_APSTA))
			
#else
		if(ieee->state == RTLLIB_LINKED && ieee->iw_mode == IW_MODE_INFRA)
#endif
		{
			u32	TotalRxBcnNum = 0;
			u32	TotalRxDataNum = 0;	

			rtl819x_update_rxcounts(priv, &TotalRxBcnNum, &TotalRxDataNum);

			if((TotalRxBcnNum+TotalRxDataNum) == 0)
				priv->check_roaming_cnt ++;
			else
				priv->check_roaming_cnt = 0;


			if(priv->check_roaming_cnt > 0)
			{
				if( ieee->eRFPowerState == eRfOff)
					RT_TRACE(COMP_ERR,"========>%s()\n",__FUNCTION__);

				printk("===>%s(): AP is power off,chan:%d, connect another one\n",__FUNCTION__, priv->chan);

				ieee->state = RTLLIB_ASSOCIATING;

#if defined(RTL8192S_WAPI_SUPPORT)				
				if ((ieee->WapiSupport) && (ieee->wapiInfo.bWapiEnable))
					WapiReturnOneStaInfo(ieee,priv->rtllib->current_network.bssid,0);
#endif
				RemovePeerTS(priv->rtllib,priv->rtllib->current_network.bssid);
				ieee->is_roaming = true;
				ieee->is_set_key = false;
                                ieee->link_change(dev);
			        if(ieee->LedControlHandler)
				   ieee->LedControlHandler(ieee->dev, LED_CTL_START_TO_LINK); 

				notify_wx_assoc_event(ieee); 

				if(!(ieee->rtllib_ap_sec_type(ieee)&(SEC_ALG_CCMP|SEC_ALG_TKIP)))
					queue_delayed_work_rsl(ieee->wq, &ieee->associate_procedure_wq, 0);

				priv->check_roaming_cnt = 0;
			}
		}
	      ieee->LinkDetectInfo.NumRecvBcnInPeriod=0;
              ieee->LinkDetectInfo.NumRecvDataInPeriod=0;
		
	}

	spin_lock_irqsave(&priv->tx_lock,flags);
	if((check_reset_cnt++ >= 3) && (!ieee->is_roaming) && 
			(!priv->RFChangeInProgress) && (!pPSC->bSwRfProcessing))
	{
    		ResetType = rtl819x_ifcheck_resetornot(dev);
#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
		if(check_reset_cnt == 0xFF)
#endif
		check_reset_cnt = 3;
	}
	spin_unlock_irqrestore(&priv->tx_lock,flags);
	
	if(!priv->bDisableNormalResetCheck && ResetType == RESET_TYPE_NORMAL)
	{
		priv->ResetProgress = RESET_TYPE_NORMAL;
		RT_TRACE(COMP_RESET,"%s(): NOMAL RESET\n",__FUNCTION__);
		return;
	}

#if defined(_RTL8192_EXT_PATCH_) || defined(ASL)
	if( ((priv->force_reset) || (!priv->bDisableNormalResetCheck && ResetType==RESET_TYPE_SILENT))) 
	{
			if((check_reset_cnt != (last_reset_count + 1)) && !priv->force_reset){
				printk("=======================>%s: Resume firmware\n", __FUNCTION__);
#ifdef RTL8192SE
				r8192se_resume_firm(dev);	
#elif defined Rtl8192CE
#endif
				last_reset_count = check_reset_cnt;
			}else{
				printk("=======================>%s: Silent Reset\n", __FUNCTION__);
				rtl819x_ifsilentreset(dev);
			}
	}
#else
	if( ((priv->force_reset) || (!priv->bDisableNormalResetCheck && ResetType==RESET_TYPE_SILENT))) 
	{
		rtl819x_ifsilentreset(dev);
	}
#endif
	priv->force_reset = false;
	priv->bForcedSilentReset = false;
	priv->bResetInProgress = false;
	RT_TRACE(COMP_TRACE, " <==RtUsbCheckForHangWorkItemCallback()\n");
	
}

void watch_dog_timer_callback(unsigned long data)
{
	struct r8192_priv *priv = rtllib_priv((struct net_device *) data);
	queue_delayed_work_rsl(priv->priv_wq,&priv->watch_dog_wq,0);
	mod_timer(&priv->watch_dog_timer, jiffies + MSECS(RTLLIB_WATCH_DOG_TIME));
}

/****************************************************************************
 ---------------------------- NIC TX/RX STUFF---------------------------
*****************************************************************************/
void rtl8192_rx_enable(struct net_device *dev)
{
    struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	priv->ops->rx_enable(dev);
}

void rtl8192_tx_enable(struct net_device *dev)
{
    struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	
	priv->ops->tx_enable(dev);

    rtllib_reset_queue(priv->rtllib);
}


static void rtl8192_free_rx_ring(struct net_device *dev)
{
    struct r8192_priv *priv = rtllib_priv(dev);
    	int i,rx_queue_idx;

	for(rx_queue_idx = 0; rx_queue_idx < MAX_RX_QUEUE; rx_queue_idx ++){
    for (i = 0; i < priv->rxringcount; i++) {
        		struct sk_buff *skb = priv->rx_buf[rx_queue_idx][i];
        if (!skb)
            continue;

        pci_unmap_single(priv->pdev,
                *((dma_addr_t *)skb->cb),
                priv->rxbuffersize, PCI_DMA_FROMDEVICE);
        kfree_skb(skb);
    }

    		pci_free_consistent(priv->pdev, sizeof(*priv->rx_ring[rx_queue_idx]) * priv->rxringcount,
            		priv->rx_ring[rx_queue_idx], priv->rx_ring_dma[rx_queue_idx]);
    		priv->rx_ring[rx_queue_idx] = NULL;
	}
}

static void rtl8192_free_tx_ring(struct net_device *dev, unsigned int prio)
{
    struct r8192_priv *priv = rtllib_priv(dev);
    struct rtl8192_tx_ring *ring = &priv->tx_ring[prio];

    while (skb_queue_len(&ring->queue)) {
        tx_desc *entry = &ring->desc[ring->idx];
        struct sk_buff *skb = __skb_dequeue(&ring->queue);

        pci_unmap_single(priv->pdev, le32_to_cpu(entry->TxBuffAddr),
                skb->len, PCI_DMA_TODEVICE);
        kfree_skb(skb);
        ring->idx = (ring->idx + 1) % ring->entries;
    }

    pci_free_consistent(priv->pdev, sizeof(*ring->desc)*ring->entries,
            ring->desc, ring->dma);
    ring->desc = NULL;
}

void rtl8192_data_hard_stop(struct net_device *dev)
{
	#if 0
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	priv->dma_poll_mask |= (1<<TX_DMA_STOP_LOWPRIORITY_SHIFT);
	rtl8192_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,priv->dma_poll_mask);
	rtl8192_set_mode(dev,EPROM_CMD_NORMAL);
	#endif
}


void rtl8192_data_hard_resume(struct net_device *dev)
{
	#if 0
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	priv->dma_poll_mask &= ~(1<<TX_DMA_STOP_LOWPRIORITY_SHIFT);
	rtl8192_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,priv->dma_poll_mask);
	rtl8192_set_mode(dev,EPROM_CMD_NORMAL);
	#endif
}

void rtl8192_hard_data_xmit(struct sk_buff *skb, struct net_device *dev, int rate)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	int ret;
	cb_desc *tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
	u8 queue_index = tcb_desc->queue_index;
	
	if((priv->rtllib->eRFPowerState == eRfOff) || IS_NIC_DOWN(priv) || priv->bResetInProgress){
		kfree_skb(skb);
		return;	
	}
	
	assert(queue_index != TXCMD_QUEUE);


        memcpy((unsigned char *)(skb->cb),&dev,sizeof(dev));
#if 0
	tcb_desc->RATRIndex = 7;
	tcb_desc->bTxDisableRateFallBack = 1;
	tcb_desc->bTxUseDriverAssingedRate = 1;
	tcb_desc->bTxEnableFwCalcDur = 1;
#endif
	skb_push(skb, priv->rtllib->tx_headroom);
	ret = rtl8192_tx(dev, skb);
	if(ret != 0) {
		kfree_skb(skb);
	};

	if(queue_index!=MGNT_QUEUE) {
		priv->rtllib->stats.tx_bytes+=(skb->len - priv->rtllib->tx_headroom);
		priv->rtllib->stats.tx_packets++;
	}


	return;
}

int rtl8192_hard_start_xmit(struct sk_buff *skb,struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	int ret;
	cb_desc *tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
	u8 queue_index = tcb_desc->queue_index;

	if(queue_index != TXCMD_QUEUE){
		if((priv->rtllib->eRFPowerState == eRfOff) ||IS_NIC_DOWN(priv) || priv->bResetInProgress){
			printk("=====>%s() retrun :%d:%d:%d\n", __func__, (priv->rtllib->eRFPowerState == eRfOff), IS_NIC_DOWN(priv), priv->bResetInProgress);
			kfree_skb(skb);
			return 0;	
		}
	}

	memcpy((unsigned char *)(skb->cb),&dev,sizeof(dev));
	if(queue_index == TXCMD_QUEUE) {
		rtl8192_tx_cmd(dev, skb);
		ret = 0;
		return ret;
	} else {
		tcb_desc->RATRIndex = 7;
		tcb_desc->bTxDisableRateFallBack = 1;
		tcb_desc->bTxUseDriverAssingedRate = 1;
		tcb_desc->bTxEnableFwCalcDur = 1;
		skb_push(skb, priv->rtllib->tx_headroom);
		ret = rtl8192_tx(dev, skb);
		if(ret != 0) {
			kfree_skb(skb);
		};
	}
	
	
	
	return ret;

}

void rtl8192_tx_isr(struct net_device *dev, int prio)
{
    struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);

    struct rtl8192_tx_ring *ring = &priv->tx_ring[prio];

    while (skb_queue_len(&ring->queue)) {
        tx_desc *entry = &ring->desc[ring->idx];
        struct sk_buff *skb;

        if(prio != BEACON_QUEUE) {
            if(entry->OWN)
                return;
            ring->idx = (ring->idx + 1) % ring->entries;
        }

        skb = __skb_dequeue(&ring->queue);
        pci_unmap_single(priv->pdev, le32_to_cpu(entry->TxBuffAddr),
                skb->len, PCI_DMA_TODEVICE);

        kfree_skb(skb);
    }
    if(prio != BEACON_QUEUE) {
        tasklet_schedule(&priv->irq_tx_tasklet);
    }

}

void rtl8192_tx_cmd(struct net_device *dev, struct sk_buff *skb)
{
    struct r8192_priv *priv = rtllib_priv(dev);
    struct rtl8192_tx_ring *ring;
    tx_desc_cmd* entry;
    unsigned int idx;
    cb_desc *tcb_desc;
    unsigned long flags;

    spin_lock_irqsave(&priv->irq_th_lock,flags);
    ring = &priv->tx_ring[TXCMD_QUEUE];

    idx = (ring->idx + skb_queue_len(&ring->queue)) % ring->entries;
    entry = (tx_desc_cmd*) &ring->desc[idx];

    tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
    
    priv->ops->tx_fill_cmd_descriptor(dev, entry, tcb_desc, skb);

    __skb_queue_tail(&ring->queue, skb);
    spin_unlock_irqrestore(&priv->irq_th_lock,flags);

    return;
}

short rtl8192_tx(struct net_device *dev, struct sk_buff* skb)
{
    	struct r8192_priv *priv = rtllib_priv(dev);
    	struct rtl8192_tx_ring  *ring;
    	unsigned long flags;
    	cb_desc *tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
    	tx_desc *pdesc = NULL;
    	struct rtllib_hdr_1addr * header = NULL;
    	u16 fc=0, type=0,stype=0;
    	bool  multi_addr=false,broad_addr=false,uni_addr=false;
    	u8*   pda_addr = NULL;
    	int   idx;
    	u32 fwinfo_size = 0;

    	if(priv->bdisable_nic){
        	RT_TRACE(COMP_ERR,"%s: ERR!! Nic is disabled! Can't tx packet len=%d qidx=%d!!!\n", __FUNCTION__, skb->len, tcb_desc->queue_index);
			return skb->len;
    	}

    	priv->rtllib->bAwakePktSent = true;
	
#if defined RTL8192SE || defined RTL8192CE
    	fwinfo_size = 0;
#else
   	fwinfo_size = sizeof(TX_FWINFO_8190PCI);
#endif

    	header = (struct rtllib_hdr_1addr *)(((u8*)skb->data) + fwinfo_size);
    	fc = header->frame_ctl;
    	type = WLAN_FC_GET_TYPE(fc);
    	stype = WLAN_FC_GET_STYPE(fc);
    	pda_addr = header->addr1;
		
    	if(is_multicast_ether_addr(pda_addr))
        	multi_addr = true;
    	else if(is_broadcast_ether_addr(pda_addr))
        	broad_addr = true;
    	else {
        	uni_addr = true;
    	}
		
    	if(uni_addr)
        	priv->stats.txbytesunicast += skb->len - fwinfo_size;
    	else if(multi_addr)
        	priv->stats.txbytesmulticast += skb->len - fwinfo_size;
    	else 
        	priv->stats.txbytesbroadcast += skb->len - fwinfo_size;

    	spin_lock_irqsave(&priv->irq_th_lock,flags);
    	ring = &priv->tx_ring[tcb_desc->queue_index];
    	if (tcb_desc->queue_index != BEACON_QUEUE) {
        	idx = (ring->idx + skb_queue_len(&ring->queue)) % ring->entries;
    	} else {
#ifdef _ENABLE_SW_BEACON
        	idx = (ring->idx + skb_queue_len(&ring->queue)) % ring->entries;
#else
        	idx = 0;
#endif
    	}

    	pdesc = &ring->desc[idx];
	if((pdesc->OWN == 1) && (tcb_desc->queue_index != BEACON_QUEUE)) {
		RT_TRACE(COMP_ERR,"No more TX desc@%d, ring->idx = %d,idx = %d, skblen = 0x%x queuelen=%d", \
				tcb_desc->queue_index,ring->idx, idx,skb->len, skb_queue_len(&ring->queue));
		spin_unlock_irqrestore(&priv->irq_th_lock,flags);
		return skb->len;
	} 

	if(tcb_desc->queue_index == MGNT_QUEUE){
	}

	if(type == RTLLIB_FTYPE_DATA){
	    	if(priv->rtllib->LedControlHandler)
			priv->rtllib->LedControlHandler(dev, LED_CTL_TX);
    	}
    	priv->ops->tx_fill_descriptor(dev, pdesc, tcb_desc, skb);
    	__skb_queue_tail(&ring->queue, skb);
    	pdesc->OWN = 1;
    	spin_unlock_irqrestore(&priv->irq_th_lock,flags);
    	dev->trans_start = jiffies;

#ifdef RTL8192CE 
	if(tcb_desc->queue_index == BEACON_QUEUE){
		write_nic_word(dev, REG_PCIE_CTRL_REG, BIT4);
	}else{
		write_nic_word(dev, REG_PCIE_CTRL_REG, BIT0<<(tcb_desc->queue_index));
	}
#else
		write_nic_word(dev,TPPoll,0x01<<tcb_desc->queue_index);
#endif
	return 0;	
}

short rtl8192_alloc_rx_desc_ring(struct net_device *dev)
{
    	struct r8192_priv *priv = rtllib_priv(dev);
    	rx_desc *entry = NULL;
    	int i, rx_queue_idx;

	for(rx_queue_idx = 0; rx_queue_idx < MAX_RX_QUEUE; rx_queue_idx ++){
    		priv->rx_ring[rx_queue_idx] = pci_alloc_consistent(priv->pdev,
            	sizeof(*priv->rx_ring[rx_queue_idx]) * priv->rxringcount, &priv->rx_ring_dma[rx_queue_idx]);

    		if (!priv->rx_ring[rx_queue_idx] || (unsigned long)priv->rx_ring[rx_queue_idx] & 0xFF) {
        		RT_TRACE(COMP_ERR,"Cannot allocate RX ring\n");
        		return -ENOMEM;
    		}

    		memset(priv->rx_ring[rx_queue_idx], 0, sizeof(*priv->rx_ring[rx_queue_idx]) * priv->rxringcount);
    		priv->rx_idx[rx_queue_idx] = 0;

    		for (i = 0; i < priv->rxringcount; i++) {
        		struct sk_buff *skb = dev_alloc_skb(priv->rxbuffersize);
        		dma_addr_t *mapping;
        		entry = &priv->rx_ring[rx_queue_idx][i];
        		if (!skb)
            			return 0;
        		skb->dev = dev;
        		priv->rx_buf[rx_queue_idx][i] = skb;
        		mapping = (dma_addr_t *)skb->cb;
        		*mapping = pci_map_single(priv->pdev, skb_tail_pointer_rsl(skb),
                	priv->rxbuffersize, PCI_DMA_FROMDEVICE);

        		entry->BufferAddress = cpu_to_le32(*mapping);

        		entry->Length = priv->rxbuffersize;
        		entry->OWN = 1;
    		}

    		entry->EOR = 1;
	}
    	return 0;
}

static int rtl8192_alloc_tx_desc_ring(struct net_device *dev,
        unsigned int prio, unsigned int entries)
{
    struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
    tx_desc *ring;
    dma_addr_t dma;
    int i;

    ring = pci_alloc_consistent(priv->pdev, sizeof(*ring) * entries, &dma);
    if (!ring || (unsigned long)ring & 0xFF) {
        RT_TRACE(COMP_ERR, "Cannot allocate TX ring (prio = %d)\n", prio);
        return -ENOMEM;
    }

    memset(ring, 0, sizeof(*ring)*entries);
    priv->tx_ring[prio].desc = ring;
    priv->tx_ring[prio].dma = dma;
    priv->tx_ring[prio].idx = 0;
    priv->tx_ring[prio].entries = entries;
    skb_queue_head_init(&priv->tx_ring[prio].queue);

    for (i = 0; i < entries; i++)
        ring[i].NextDescAddress =
            cpu_to_le32((u32)dma + ((i + 1) % entries) * sizeof(*ring));

    return 0;
}

                    
short rtl8192_pci_initdescring(struct net_device *dev)
{
    u32 ret;
    int i;
    struct r8192_priv *priv = rtllib_priv(dev);

    ret = rtl8192_alloc_rx_desc_ring(dev);
    if (ret) {
        return ret;
    }
    

    for (i = 0; i < MAX_TX_QUEUE_COUNT; i++) {
        if ((ret = rtl8192_alloc_tx_desc_ring(dev, i, priv->txringcount)))
            goto err_free_rings;
    }

#if 0
    if ((ret = rtl8192_alloc_tx_desc_ring(dev, MAX_TX_QUEUE_COUNT - 1, 2)))
        goto err_free_rings;
#endif    

    return 0;

err_free_rings:
    rtl8192_free_rx_ring(dev);
    for (i = 0; i < MAX_TX_QUEUE_COUNT; i++)
        if (priv->tx_ring[i].desc)
            rtl8192_free_tx_ring(dev, i);
    return 1;
}

void rtl8192_pci_resetdescring(struct net_device *dev)
{
    struct r8192_priv *priv = rtllib_priv(dev);
    	int i,rx_queue_idx;
    unsigned long flags = 0;

	for(rx_queue_idx = 0; rx_queue_idx < MAX_RX_QUEUE; rx_queue_idx ++){	
    		if(priv->rx_ring[rx_queue_idx]) {
        		rx_desc *entry = NULL;
        		for (i = 0; i < priv->rxringcount; i++) {
            			entry = &priv->rx_ring[rx_queue_idx][i];
            			entry->OWN = 1;
        		}
        		priv->rx_idx[rx_queue_idx] = 0;
    		}
	}

    spin_lock_irqsave(&priv->irq_th_lock,flags);
    for (i = 0; i < MAX_TX_QUEUE_COUNT; i++) {
        if (priv->tx_ring[i].desc) {
            struct rtl8192_tx_ring *ring = &priv->tx_ring[i];

            while (skb_queue_len(&ring->queue)) {
                tx_desc *entry = &ring->desc[ring->idx];
                struct sk_buff *skb = __skb_dequeue(&ring->queue);

                pci_unmap_single(priv->pdev, le32_to_cpu(entry->TxBuffAddr),
                        skb->len, PCI_DMA_TODEVICE);
                kfree_skb(skb);
                ring->idx = (ring->idx + 1) % ring->entries;
            }
            ring->idx = 0;
        }
    }
    spin_unlock_irqrestore(&priv->irq_th_lock,flags);
}

void rtl819x_UpdateRxPktTimeStamp (struct net_device *dev, struct rtllib_rx_stats *stats)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);

	if(stats->bIsAMPDU && !stats->bFirstMPDU) {
		stats->mac_time[0] = priv->LastRxDescTSFLow;
		stats->mac_time[1] = priv->LastRxDescTSFHigh;
	} else {
		priv->LastRxDescTSFLow = stats->mac_time[0];
		priv->LastRxDescTSFHigh = stats->mac_time[1];
	}
}

long rtl819x_translate_todbm(struct r8192_priv * priv, u8 signal_strength_index	)
{
	long	signal_power; 

#ifdef _RTL8192_EXT_PATCH_
	if(priv->CustomerID == RT_CID_819x_Lenovo)
	{	
		signal_power = (long)signal_strength_index;
		if(signal_power >= 45)
			signal_power -= 110;
		else
		{
			signal_power = ((signal_power*6)/10);
			signal_power -= 93;
		}
		return signal_power;
	}
#endif
	signal_power = (long)((signal_strength_index + 1) >> 1); 
	signal_power -= 95; 

	return signal_power;
}


void
rtl819x_update_rxsignalstatistics8190pci(
	struct r8192_priv * priv,
	struct rtllib_rx_stats * pprevious_stats
	)
{
	int weighting = 0;


	if(priv->stats.recv_signal_power == 0)
		priv->stats.recv_signal_power = pprevious_stats->RecvSignalPower;

	if(pprevious_stats->RecvSignalPower > priv->stats.recv_signal_power)
		weighting = 5;
	else if(pprevious_stats->RecvSignalPower < priv->stats.recv_signal_power)
		weighting = (-5);
	priv->stats.recv_signal_power = (priv->stats.recv_signal_power * 5 + pprevious_stats->RecvSignalPower + weighting) / 6;
}

void
rtl819x_process_cck_rxpathsel(
	struct r8192_priv * priv,
	struct rtllib_rx_stats * pprevious_stats
	)
{
#ifdef RTL8190P	
	char				last_cck_adc_pwdb[4]={0,0,0,0};
	u8				i;
		if(priv->rf_type == RF_2T4R && DM_RxPathSelTable.Enable)
		{
			if(pprevious_stats->bIsCCK && 
				(pprevious_stats->bPacketToSelf ||pprevious_stats->bPacketBeacon))
			{
				if(priv->stats.cck_adc_pwdb.TotalNum++ >= PHY_RSSI_SLID_WIN_MAX)
				{
					priv->stats.cck_adc_pwdb.TotalNum = PHY_RSSI_SLID_WIN_MAX;
					for(i=RF90_PATH_A; i<RF90_PATH_MAX; i++)
					{
						last_cck_adc_pwdb[i] = priv->stats.cck_adc_pwdb.elements[i][priv->stats.cck_adc_pwdb.index];
						priv->stats.cck_adc_pwdb.TotalVal[i] -= last_cck_adc_pwdb[i];
					}
				}
				for(i=RF90_PATH_A; i<RF90_PATH_MAX; i++)
				{
					priv->stats.cck_adc_pwdb.TotalVal[i] += pprevious_stats->cck_adc_pwdb[i];
					priv->stats.cck_adc_pwdb.elements[i][priv->stats.cck_adc_pwdb.index] = pprevious_stats->cck_adc_pwdb[i];
				}
				priv->stats.cck_adc_pwdb.index++;
				if(priv->stats.cck_adc_pwdb.index >= PHY_RSSI_SLID_WIN_MAX)
					priv->stats.cck_adc_pwdb.index = 0;

				for(i=RF90_PATH_A; i<RF90_PATH_MAX; i++)
				{
					DM_RxPathSelTable.cck_pwdb_sta[i] = priv->stats.cck_adc_pwdb.TotalVal[i]/priv->stats.cck_adc_pwdb.TotalNum;
				}

				for(i=RF90_PATH_A; i<RF90_PATH_MAX; i++)
				{
					if(pprevious_stats->cck_adc_pwdb[i]  > (char)priv->undecorated_smoothed_cck_adc_pwdb[i])
					{
						priv->undecorated_smoothed_cck_adc_pwdb[i] = 
							( (priv->undecorated_smoothed_cck_adc_pwdb[i]*(Rx_Smooth_Factor-1)) + 
							(pprevious_stats->cck_adc_pwdb[i])) /(Rx_Smooth_Factor);
						priv->undecorated_smoothed_cck_adc_pwdb[i] = priv->undecorated_smoothed_cck_adc_pwdb[i] + 1;
					}
					else
					{
						priv->undecorated_smoothed_cck_adc_pwdb[i] = 
							( (priv->undecorated_smoothed_cck_adc_pwdb[i]*(Rx_Smooth_Factor-1)) + 
							(pprevious_stats->cck_adc_pwdb[i])) /(Rx_Smooth_Factor);
					}
				}
			}
		}
#endif
}


u8 rtl819x_query_rxpwrpercentage(
	char		antpower
	)
{
	if ((antpower <= -100) || (antpower >= 20))
	{
		return	0;
	}
	else if (antpower >= 0)
	{
		return	100;
	}
	else
	{
		return	(100+antpower);
	}
	
}	/* QueryRxPwrPercentage */

u8 
rtl819x_evm_dbtopercentage(
	char value
	)
{
	char ret_val;
	
	ret_val = value;
	
	if(ret_val >= 0)
		ret_val = 0;
	if(ret_val <= -33)
		ret_val = -33;
	ret_val = 0 - ret_val;
	ret_val*=3;
	if(ret_val == 99)
		ret_val = 100;
	return(ret_val);
}

void
rtl8192_record_rxdesc_forlateruse(
	struct rtllib_rx_stats * psrc_stats,
	struct rtllib_rx_stats * ptarget_stats
)
{
	ptarget_stats->bIsAMPDU = psrc_stats->bIsAMPDU;
	ptarget_stats->bFirstMPDU = psrc_stats->bFirstMPDU;
}



void rtl8192_rx_normal(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	struct rtllib_hdr_1addr *rtllib_hdr = NULL;
	bool unicast_packet = false;
	bool bLedBlinking=true;
	u16 fc=0, type=0;
	u32 skb_len = 0;
	int rx_queue_idx = RX_MPDU_QUEUE;
	
	struct rtllib_rx_stats stats = {
		.signal = 0,
		.noise = -98,
		.rate = 0,
		.freq = RTLLIB_24GHZ_BAND,
	};
	unsigned int count = priv->rxringcount;

	stats.nic_type = NIC_8192E;

	while (count--) {
		rx_desc *pdesc = &priv->rx_ring[rx_queue_idx][priv->rx_idx[rx_queue_idx]];
		struct sk_buff *skb = priv->rx_buf[rx_queue_idx][priv->rx_idx[rx_queue_idx]];

		if (pdesc->OWN){
			return;
		} else {

			struct sk_buff *new_skb = NULL;
			if (!priv->ops->rx_query_status_descriptor(dev, &stats, pdesc, skb))
				goto done;

			pci_unmap_single(priv->pdev,
					*((dma_addr_t *)skb->cb), 
					priv->rxbuffersize, 
					PCI_DMA_FROMDEVICE);

			skb_put(skb, pdesc->Length);
			skb_reserve(skb, stats.RxDrvInfoSize + stats.RxBufShift);
			skb_trim(skb, skb->len - 4/*sCrcLng*/);
			rtllib_hdr = (struct rtllib_hdr_1addr *)skb->data;
			if(is_broadcast_ether_addr(rtllib_hdr->addr1)) {
			}else if(is_multicast_ether_addr(rtllib_hdr->addr1)){
			}else {
				/* unicast packet */
				unicast_packet = true;
			}
			fc = le16_to_cpu(rtllib_hdr->frame_ctl);
			type = WLAN_FC_GET_TYPE(fc);
			if(type == RTLLIB_FTYPE_MGMT)
			{
				bLedBlinking = false;
			}
			if(bLedBlinking)
				if(priv->rtllib->LedControlHandler)
				priv->rtllib->LedControlHandler(dev, LED_CTL_RX);

			if(stats.bCRC) {
				if(type != RTLLIB_FTYPE_MGMT)
					priv->stats.rxdatacrcerr ++;
				else
					priv->stats.rxmgmtcrcerr ++;
			}

			skb_len = skb->len;
			
#ifdef RTL8192CE
			if (!stats.bCRC)
#else
			if (1)
#endif
			{
			if(!rtllib_rx(priv->rtllib, skb, &stats)){
				dev_kfree_skb_any(skb);
				skb = NULL;
			} else {
				priv->stats.rxok++;
				if(unicast_packet) {
					priv->stats.rxbytesunicast += skb_len;
				}
			}
			}else{
				dev_kfree_skb_any(skb);
				skb = NULL;
			}
#if 1
			new_skb = dev_alloc_skb(priv->rxbuffersize);
			if (unlikely(!new_skb))
			{
				printk("==========>can't alloc skb for rx\n");
				goto done;
			}
			skb=new_skb;
                        skb->dev = dev;
#endif
			priv->rx_buf[rx_queue_idx][priv->rx_idx[rx_queue_idx]] = skb;
			*((dma_addr_t *) skb->cb) = pci_map_single(priv->pdev, skb_tail_pointer_rsl(skb), priv->rxbuffersize, PCI_DMA_FROMDEVICE);

		}
done:
		pdesc->BufferAddress = cpu_to_le32(*((dma_addr_t *)skb->cb));
		pdesc->OWN = 1;
		pdesc->Length = priv->rxbuffersize;
		if (priv->rx_idx[rx_queue_idx] == priv->rxringcount-1)
			pdesc->EOR = 1;
		priv->rx_idx[rx_queue_idx] = (priv->rx_idx[rx_queue_idx] + 1) % priv->rxringcount;
	}

}

void rtl8192_rx_cmd(struct net_device *dev)
{
#ifdef RTL8192SE
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);

	unsigned int count = priv->rxringcount;
	int rx_queue_idx = RX_CMD_QUEUE;

	struct rtllib_rx_stats stats = {
		.signal = 0,
		.noise = -98,
		.rate = 0,
		.freq = RTLLIB_24GHZ_BAND,
	};
	stats.nic_type = NIC_8192E;

	while (count--) {
		rx_desc *pdesc = &priv->rx_ring[rx_queue_idx][priv->rx_idx[rx_queue_idx]];
		struct sk_buff *skb = priv->rx_buf[rx_queue_idx][priv->rx_idx[rx_queue_idx]];

		if (pdesc->OWN){
			return;
		} else {
			struct sk_buff *new_skb = NULL;

			pci_unmap_single(priv->pdev,
					*((dma_addr_t *)skb->cb), 
					priv->rxbuffersize, 
					PCI_DMA_FROMDEVICE);

			skb_put(skb, pdesc->Length);

			if(pdesc->MACID == 0x1e){

				pdesc->Length = pdesc->Length - 32;
				pdesc->DrvInfoSize = 4; 
				printk(">>>>%s()CMD PKT RX, beacon_len:%d payload_len:%d\n",__func__, pdesc->Length,skb->len);
				
				
				priv->ops->rx_query_status_descriptor(dev, &stats, pdesc, skb);
				skb_reserve(skb, stats.RxDrvInfoSize + stats.RxBufShift);
                                if(0)
				{
					u8 i = 0;
					u8 *arry = (u8*) skb->data;

					printk("\n==============>\n");
					for(i = 0; i < 32; i ++){
						if((i % 4 == 0)&&(i != 0))
							printk("\n");
						printk("%2.2x ", arry[i]);
					}
					printk("\n<==============\n");
				}
			}

			skb_trim(skb, skb->len - 4/*sCrcLng*/);

			if(pdesc->MACID == 0x1e){
				if(!rtllib_rx(priv->rtllib, skb, &stats)){
					dev_kfree_skb_any(skb);
					skb = NULL;
				} 
			}else{
			        if(priv->ops->rx_command_packet_handler != NULL)
				        priv->ops->rx_command_packet_handler(dev, skb, pdesc);
			        dev_kfree_skb_any(skb);
					skb = NULL;
			}

		
			new_skb = dev_alloc_skb(priv->rxbuffersize);
			if (unlikely(!new_skb))
			{
				printk("==========>can't alloc skb for rx\n");
				goto done;
			}
			skb=new_skb;
                        skb->dev = dev;

			priv->rx_buf[rx_queue_idx][priv->rx_idx[rx_queue_idx]] = skb;
			*((dma_addr_t *) skb->cb) = pci_map_single(priv->pdev, skb_tail_pointer_rsl(skb), priv->rxbuffersize, PCI_DMA_FROMDEVICE);

		}
done:
		pdesc->BufferAddress = cpu_to_le32(*((dma_addr_t *)skb->cb));
		pdesc->OWN = 1;
		pdesc->Length = priv->rxbuffersize;
		if (priv->rx_idx[rx_queue_idx] == priv->rxringcount-1)
			pdesc->EOR = 1;
		priv->rx_idx[rx_queue_idx] = (priv->rx_idx[rx_queue_idx] + 1) % priv->rxringcount;
	}
#endif
}


void rtl8192_tx_resume(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;
	struct sk_buff *skb;
	int queue_index;

	for(queue_index = BK_QUEUE; queue_index < MAX_QUEUE_SIZE;queue_index++) {
		while((!skb_queue_empty(&ieee->skb_waitQ[queue_index]))&&
				(priv->rtllib->check_nic_enough_desc(dev,queue_index) > 0)) {
			skb = skb_dequeue(&ieee->skb_waitQ[queue_index]);
			ieee->softmac_data_hard_start_xmit(skb,dev,0/* rate useless now*/);
			#if 0
			if(queue_index!=MGNT_QUEUE) {
				ieee->stats.tx_packets++;
				ieee->stats.tx_bytes += skb->len;
			}
			#endif
		}
#ifdef ENABLE_AMSDU
		while((skb_queue_len(&priv->rtllib->skb_aggQ[queue_index]) > 0)&&\
				(!(priv->rtllib->queue_stop)) && \
				(priv->rtllib->check_nic_enough_desc(dev,queue_index) > 0)){

			struct sk_buff_head pSendList;
			u8 dst[ETH_ALEN];
			cb_desc *tcb_desc = NULL;
			int qos_actived = priv->rtllib->current_network.qos_data.active;
			struct sta_info *psta = NULL;
			u8 bIsSptAmsdu = false;

#ifdef WIFI_TEST 
			if (queue_index <= VO_QUEUE)
				queue_index = wmm_queue_select(priv, queue_index, priv->rtllib->skb_aggQ);
#endif
			priv->rtllib->amsdu_in_process = true;

			skb = skb_dequeue(&(priv->rtllib->skb_aggQ[queue_index]));
			if (skb == NULL) {
				printk("In %s:Skb is NULL\n",__FUNCTION__);
				priv->rtllib->amsdu_in_process = false;
				return;
			}
			tcb_desc = (pcb_desc)(skb->cb + MAX_DEV_ADDR_SIZE);
			if(tcb_desc->bFromAggrQ)
			{
				rtllib_xmit_inter(skb, dev);
				priv->rtllib->amsdu_in_process = false;
				return;
			}

			memcpy(dst, skb->data, ETH_ALEN);
			if(priv->rtllib->iw_mode == IW_MODE_ADHOC)
			{
				psta = GetStaInfo(priv->rtllib, dst);
				if(psta) {
					if(psta->htinfo.bEnableHT)
						bIsSptAmsdu = true;
				}
			}
			else if(priv->rtllib->iw_mode == IW_MODE_INFRA)
				bIsSptAmsdu = true;
			else
				bIsSptAmsdu = true;

			bIsSptAmsdu = bIsSptAmsdu && priv->rtllib->pHTInfo->bCurrent_AMSDU_Support && qos_actived;

			if(qos_actived &&       !is_broadcast_ether_addr(dst) &&
					!is_multicast_ether_addr(dst) &&
					bIsSptAmsdu)
			{
				skb_queue_head_init(&pSendList);
				if(AMSDU_GetAggregatibleList(priv->rtllib, skb, &pSendList,queue_index,false))
				{
					struct sk_buff * pAggrSkb = AMSDU_Aggregation(priv->rtllib, &pSendList);
					if(NULL != pAggrSkb) {
						rtllib_xmit_inter(pAggrSkb, dev);
				}
			}
			}
			else
			{
				memset(skb->cb,0,sizeof(skb->cb));
				tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
				tcb_desc->bFromAggrQ = true;
				rtllib_xmit_inter(skb, dev);
			}
		}
#endif
#ifdef ASL
		while((skb_queue_len(&priv->rtllib->skb_apaggQ[queue_index]) > 0)&&\
				(!(priv->rtllib->queue_stop)) && \
				(priv->rtllib->check_nic_enough_desc(dev,queue_index) > 0)){

			struct sk_buff_head pSendList;
			u8 dst[ETH_ALEN];
			cb_desc *tcb_desc = NULL;
			int qos_actived = priv->rtllib->current_ap_network.qos_data.active;
			struct sta_info *psta = NULL;
			u8 bIsSptAmsdu = false;

#ifdef WIFI_TEST 
			if (queue_index <= VO_QUEUE)
				queue_index = wmm_queue_select(priv, queue_index, priv->rtllib->skb_apaggQ);
#endif
			priv->rtllib->ap_amsdu_in_process = true;

			skb = skb_dequeue(&(priv->rtllib->skb_apaggQ[queue_index]));
			if (skb == NULL) {
				printk("In %s:Skb is NULL\n",__FUNCTION__);
				priv->rtllib->ap_amsdu_in_process = false;
				return;
			}
			tcb_desc = (pcb_desc)(skb->cb + MAX_DEV_ADDR_SIZE);
			if(tcb_desc->bFromAggrQ)
			{
				apdev_xmit_inter(skb, dev);
				priv->rtllib->ap_amsdu_in_process = false;
				return;
			}

			memcpy(dst, skb->data, ETH_ALEN);
				psta = ap_get_stainfo(priv->rtllib, dst);
				if(psta) {
					if(psta->htinfo.bEnableHT)
				               bIsSptAmsdu = true;
					qos_actived = qos_actived & psta->wme_enable;
				}
			

			bIsSptAmsdu = bIsSptAmsdu && priv->rtllib->pAPHTInfo->bCurrent_AMSDU_Support && qos_actived;

			if(qos_actived &&       !is_broadcast_ether_addr(dst) &&
					!is_multicast_ether_addr(dst) &&
					bIsSptAmsdu)
			{
				skb_queue_head_init(&pSendList);
				if(AMSDU_GetAggregatibleList(priv->rtllib, skb, &pSendList,queue_index,true))
				{
					struct sk_buff * pAggrSkb = AMSDU_Aggregation(priv->rtllib, &pSendList);
					if(NULL != pAggrSkb) {
						apdev_xmit_inter(pAggrSkb, dev);
				}
			}
			}
			else
			{
				memset(skb->cb,0,sizeof(skb->cb));
				tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
				tcb_desc->bFromAggrQ = true;
				apdev_xmit_inter(skb, dev);
			}
		}
#endif

#ifdef _RTL8192_EXT_PATCH_
		while((!skb_queue_empty(&priv->rtllib->skb_meshaggQ[queue_index]) )&&\
			(priv->rtllib->check_nic_enough_desc(dev,queue_index)> 0))
		{
			u8 dst[ETH_ALEN];
			cb_desc *tcb_desc = NULL;
			u8 IsHTEnable = false;
#ifdef ENABLE_AMSDU
            struct sk_buff_head pSendList;
			int qos_actived = 1; 
#endif
			priv->rtllib->mesh_amsdu_in_process = true;
			skb = skb_dequeue(&(priv->rtllib->skb_meshaggQ[queue_index]));
			if(skb == NULL)
			{
				priv->rtllib->mesh_amsdu_in_process = false;
				printk("In %s:Skb is NULL\n",__FUNCTION__);
				return;
			}
			tcb_desc = (pcb_desc)(skb->cb + MAX_DEV_ADDR_SIZE);
			if(tcb_desc->bFromAggrQ)
			{
				rtllib_mesh_xmit(skb, dev);
				priv->rtllib->mesh_amsdu_in_process = false;
				continue;
			}
			memcpy(dst, skb->data, ETH_ALEN);
			
#if 0
			ppeerMP_htinfo phtinfo = NULL;
			bool is_mesh = false;
			if(priv->mshobj->ext_patch_rtllib_is_mesh)
				is_mesh = priv->mshobj->ext_patch_rtllib_is_mesh(priv->rtllib,dst);
			if(is_mesh){
				if(priv->rtllib->ext_patch_rtllib_get_peermp_htinfo)
				{
					phtinfo = priv->rtllib->ext_patch_rtllib_get_peermp_htinfo(ieee,dst);
					if(phtinfo == NULL)
					{
						RT_TRACE(COMP_ERR,"%s(): No htinfo\n",__FUNCTION__);
					}
					else
					{
						if(phtinfo->bEnableHT)
							IsHTEnable = true;
					}	
				}
			}
			else 
			{
				printk("===>%s():tx AMSDU data has not entry,dst: "MAC_FMT"\n",__FUNCTION__,MAC_ARG(dst));
				IsHTEnable = true;
			}
#else
			IsHTEnable = true;
#endif
#ifdef ENABLE_AMSDU
			IsHTEnable = (IsHTEnable && ieee->pHTInfo->bCurrent_Mesh_AMSDU_Support && qos_actived);
			if( !is_broadcast_ether_addr(dst) && 
				!is_multicast_ether_addr(dst) &&
				IsHTEnable) 
			{
				skb_queue_head_init(&pSendList);
				if(msh_AMSDU_GetAggregatibleList(priv->rtllib, skb, &pSendList,queue_index))
				{				
					struct sk_buff * pAggrSkb = msh_AMSDU_Aggregation(priv->rtllib, &pSendList);
					if(NULL != pAggrSkb)
						rtllib_mesh_xmit(pAggrSkb, dev);
				}else{
					priv->rtllib->mesh_amsdu_in_process = false;
					return;
				}
			}
			else
#endif
			{
				memset(skb->cb,0,sizeof(skb->cb));
				tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
				tcb_desc->bFromAggrQ = true;
				rtllib_mesh_xmit(skb, dev);
			}
		}
#endif		
	}
}

void rtl8192_irq_tx_tasklet(struct r8192_priv *priv)
{
       rtl8192_tx_resume(priv->rtllib->dev);
}

void rtl8192_irq_rx_tasklet(struct r8192_priv *priv)
{
       	rtl8192_rx_normal(priv->rtllib->dev);

	if(MAX_RX_QUEUE > 1)
		rtl8192_rx_cmd(priv->rtllib->dev);
	
#ifndef RTL8192CE
       write_nic_dword(priv->rtllib->dev, INTA_MASK,read_nic_dword(priv->rtllib->dev, INTA_MASK) | IMR_RDU); 
#endif
}

/****************************************************************************
 ---------------------------- NIC START/CLOSE STUFF---------------------------
*****************************************************************************/
void rtl8192_cancel_deferred_work(struct r8192_priv* priv)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	cancel_delayed_work(&priv->watch_dog_wq);
	cancel_delayed_work(&priv->update_beacon_wq);
#ifndef RTL8190P
	cancel_delayed_work(&priv->rtllib->hw_sleep_wq);
#endif
#if defined RTL8192SE 
	cancel_delayed_work(&priv->check_hw_scan_wq);
	cancel_delayed_work(&priv->hw_scan_simu_wq);
	cancel_delayed_work(&priv->start_hw_scan_wq);
	cancel_delayed_work(&priv->rtllib->update_assoc_sta_info_wq);
	cancel_delayed_work(&priv->rtllib->check_tsf_wq);
#endif
#endif

#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,22)
	cancel_work_sync(&priv->reset_wq);
	cancel_work_sync(&priv->qos_activate);
#elif ((LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)))
	cancel_delayed_work(&priv->reset_wq);
	cancel_delayed_work(&priv->qos_activate);
#if defined RTL8192SE 
	cancel_delayed_work(&priv->rtllib->update_assoc_sta_info_wq);
	cancel_delayed_work(&priv->rtllib->check_tsf_wq);
#endif
#endif

}

int _rtl8192_up(struct net_device *dev,bool is_silent_reset)
{
#ifdef _RTL8192_EXT_PATCH_
	if(_rtl8192_mesh_up(dev, is_silent_reset) == -1)
		return -1;
#else
#ifdef ASL
	if(_rtl8192_ap_up(dev, is_silent_reset) == -1)
		return -1;
#else
	if(_rtl8192_sta_up(dev, is_silent_reset) == -1) 
		return -1;
#endif
#endif

#ifdef CONFIG_BT_30
	bt_ioctl_up(dev);
#endif

	return 0;
}


int rtl8192_open(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	int ret;
	
	down(&priv->wx_sem);
	ret = rtl8192_up(dev);
	up(&priv->wx_sem);
	return ret;
	
}


int rtl8192_up(struct net_device *dev)
{
#ifndef _RTL8192_EXT_PATCH_
	struct r8192_priv *priv = rtllib_priv(dev);

	if (priv->up == 1) return -1;
	return _rtl8192_up(dev,false);
#else	
	return _rtl8192_up(dev,false);
#endif	
}


int rtl8192_close(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	int ret;
	
	if ((rtllib_act_scanning(priv->rtllib, false)) && 
		!(priv->rtllib->softmac_features & IEEE_SOFTMAC_SCAN)){
		rtllib_stop_scan(priv->rtllib);
	}

	down(&priv->wx_sem);

	ret = rtl8192_down(dev,true);
	
	up(&priv->wx_sem);
	
	return ret;

}

int rtl8192_down(struct net_device *dev, bool shutdownrf)
{
#ifdef CONFIG_MP
	struct r8192_priv *priv = rtllib_priv(dev);
#endif

#ifdef _RTL8192_EXT_PATCH_
	if(rtl8192_mesh_down(dev, shutdownrf) == -1)
		return -1;
#else
#ifdef ASL
	if(rtl8192_ap_down(dev, shutdownrf) == -1)
		return -1;
#else
	if(rtl8192_sta_down(dev, shutdownrf) == -1)
		return -1;
#endif
#endif

#ifdef CONFIG_BT_30
	bt_ioctl_down(dev);
#endif

#ifdef CONFIG_MP
	if (priv->bCckContTx) {
		printk("####RTL819X MP####stop single cck continious TX\n");
		mpt_StopCckCoNtTx(dev);
	} 
	if (priv->bOfdmContTx) {
		printk("####RTL819X MP####stop single ofdm continious TX\n");
		mpt_StopOfdmContTx(dev);
	} 
	if (priv->bSingleCarrier) {
		printk("####RTL819X MP####stop single carrier mode\n");
		MPT_ProSetSingleCarrier(dev, false);
	}
#endif
	return 0;
}

void rtl8192_commit(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);

#ifdef _RTL8192_EXT_PATCH_
	if (IS_NIC_DOWN(priv)) 
		return ;
	rtllib_softmac_stop_protocol(priv->rtllib,0,0,true);
	rtl8192_irq_disable(dev);
	priv->ops->stop_adapter(dev, true);
	priv->up = 0;
	_rtl8192_up(dev,false);
#else
	if (priv->up == 0) return ;
	rtllib_softmac_stop_protocol(priv->rtllib, 0 ,0, true);
	rtl8192_irq_disable(dev);
	priv->ops->stop_adapter(dev, true);
	_rtl8192_up(dev,false);
#endif
	
}

void rtl8192_restart(void *data)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
	struct r8192_priv *priv = container_of_work_rsl(data, struct r8192_priv, reset_wq);
	struct net_device *dev = priv->rtllib->dev;
#else
	struct net_device *dev = (struct net_device *)data;
        struct r8192_priv *priv = rtllib_priv(dev);
#endif

	down(&priv->wx_sem);
	
	rtl8192_commit(dev);
	
	up(&priv->wx_sem);
}

static void r8192_set_multicast(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	short promisc;

	
	
	promisc = (dev->flags & IFF_PROMISC) ? 1:0;
	
	if (promisc != priv->promisc) {
		;
	}
	
	priv->promisc = promisc;
	
}


int r8192_set_mac_adr(struct net_device *dev, void *mac)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct sockaddr *addr = mac;
	
	down(&priv->wx_sem);
	
	memcpy(dev->dev_addr, addr->sa_data, ETH_ALEN);
		
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
	schedule_work(&priv->reset_wq);
#else
	schedule_task(&priv->reset_wq);
#endif	
	up(&priv->wx_sem);
	
	return 0;
}

#if defined (RTL8192S_WAPI_SUPPORT)
extern int wapi_ioctl_set_mode(struct net_device *dev, struct iw_request_info *a,union iwreq_data *wrqu, char *b);
int WAPI_Ioctl(struct net_device *dev, struct iwreq *wrq, int cmd)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;
	static u8 QueueData[WAPI_MAX_BUFF_LEN];
#define DATAQUEUE_EMPTY	"Queue is empty"
	int ret = 0;

	switch(cmd){
	    	case WAPI_CMD_GET_WAPI_SUPPORT:
			WAPI_TRACE(WAPI_API, "%s(): WAPI_CMD_GET_WAPI_SUPPORT!\n",__FUNCTION__);
			if(copy_to_user((u8 *)(wrq->u.data.pointer), &ieee->WapiSupport, sizeof(u8))==0)
				wrq->u.data.length = sizeof(u8);
			break;
	    	case WAPI_CMD_SET_WAPI_ENABLE:
		{
			u8 wapi_enable = *(u8 *)wrq->u.data.pointer;
			WAPI_TRACE(WAPI_API, "%s(): WAPI_CMD_SET_WAPI_ENABLE!\n",__FUNCTION__);
			if((ieee->WapiSupport) && (wapi_enable == 1)){
				ieee->wapiInfo.bWapiEnable = true;
				ieee->pairwise_key_type = KEY_TYPE_SMS4;
				ieee->group_key_type = KEY_TYPE_SMS4;
				ieee->wapiInfo.extra_prefix_len = WAPI_EXT_LEN;
				ieee->wapiInfo.extra_postfix_len = SMS4_MIC_LEN;
			}
			else if (wapi_enable == 0) {
				ieee->wapiInfo.bWapiEnable = false;
				ieee->pairwise_key_type = KEY_TYPE_NA;
				ieee->group_key_type = KEY_TYPE_NA;
			}
			break;
	    	}
	    	case WAPI_CMD_SET_WAPI_PSK:
		{
			u8 wapi_psk = *(u8 *)wrq->u.data.pointer;
			WAPI_TRACE(WAPI_API, "%s(): WAPI_CMD_SET_WAPI_PSK!\n",__FUNCTION__);
			if(ieee->wapiInfo.bWapiEnable){
				ieee->wapiInfo.bWapiPSK = (wapi_psk > 0)?1:0; 
				WAPI_TRACE(WAPI_API, "%s(): bWapiPSK=%d!\n",__FUNCTION__, ieee->wapiInfo.bWapiPSK);
			}
			break;
	    	}
	    	case WAPI_CMD_SET_KEY:
		{ /*Data: keyType(1) + bTxEnable(1) + bAuthenticator(1) + bUpdate(1) + PeerAddr(6) + DataKey(16) + MicKey(16) + KeyId(1)*/
			PRT_WAPI_T			pWapiInfo = &ieee->wapiInfo;
			PRT_WAPI_BKID		pWapiBkid;
			PRT_WAPI_STA_INFO	pWapiSta;
			u8					data[43];
			bool					bTxEnable;
			bool					bUpdate;
			bool					bAuthenticator;
			u8					PeerAddr[6];
			u8					WapiAEPNInitialValueSrc[16] = {0x37,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C} ;
			u8					WapiASUEPNInitialValueSrc[16] = {0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C} ;
			u8					WapiAEMultiCastPNInitialValueSrc[16] = {0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C,0x36,0x5C} ;		

			WAPI_TRACE(WAPI_API, "%s(): WAPI_CMD_SET_KEY!\n",__FUNCTION__);
			if (!ieee->WapiSupport){
				ret = -1;
				break;    
			}
			if(wrq->u.data.length < 26){
				ret = -1;
				break;
			}

			copy_from_user(data, (u8 *)wrq->u.data.pointer, wrq->u.data.length);
			bTxEnable = data[1];
			bAuthenticator = data[2];
			bUpdate = data[3];
		 	memcpy(PeerAddr,data+4,6);
				
			if(data[0] == 0x3){
				if(!list_empty(&(pWapiInfo->wapiBKIDIdleList))){
					pWapiBkid = (PRT_WAPI_BKID)list_entry(pWapiInfo->wapiBKIDIdleList.next, RT_WAPI_BKID,list);
					list_del_init(&pWapiBkid->list);
					memcpy(pWapiBkid->bkid, data+10, 16);
					list_add_tail(&pWapiBkid->list, &pWapiInfo->wapiBKIDStoreList);
					WAPI_DATA(WAPI_API, "SetKey - BKID", pWapiBkid->bkid, 16);
				}
			} else if (data[0] == 0x04){
				list_for_each_entry(pWapiSta, &pWapiInfo->wapiSTAUsedList, list) {
					if(!memcmp(pWapiSta->PeerMacAddr,PeerAddr,6)){
						if(bUpdate){
							if(bAuthenticator) {
								memcpy(pWapiSta->lastTxUnicastPN,WapiASUEPNInitialValueSrc,16);
								memcpy(pWapiSta->lastRxUnicastPNBEQueue,WapiAEPNInitialValueSrc,16);
								memcpy(pWapiSta->lastRxUnicastPNBKQueue,WapiAEPNInitialValueSrc,16);
								memcpy(pWapiSta->lastRxUnicastPNVIQueue,WapiAEPNInitialValueSrc,16);
								memcpy(pWapiSta->lastRxUnicastPNVOQueue,WapiAEPNInitialValueSrc,16);
								memcpy(pWapiSta->lastRxUnicastPN,WapiAEPNInitialValueSrc,16);
							} else {
								memcpy(pWapiSta->lastTxUnicastPN,WapiAEPNInitialValueSrc,16);
								memcpy(pWapiSta->lastRxUnicastPNBEQueue,WapiASUEPNInitialValueSrc,16);
								memcpy(pWapiSta->lastRxUnicastPNBKQueue,WapiASUEPNInitialValueSrc,16);
								memcpy(pWapiSta->lastRxUnicastPNVIQueue,WapiASUEPNInitialValueSrc,16);
								memcpy(pWapiSta->lastRxUnicastPNVOQueue,WapiASUEPNInitialValueSrc,16);
								memcpy(pWapiSta->lastRxUnicastPN,WapiASUEPNInitialValueSrc,16);
							}
						}
					}
				}
			}
			else{
				list_for_each_entry(pWapiSta, &pWapiInfo->wapiSTAUsedList, list) {
					if(!memcmp(pWapiSta->PeerMacAddr,PeerAddr,6)){
						pWapiSta->bAuthenticatorInUpdata = false;
						switch(data[0]){
						case 1:              
							if(bAuthenticator){         
								memcpy(pWapiSta->lastTxUnicastPN,WapiAEPNInitialValueSrc,16);
								if(!bUpdate) {     
									WAPI_TRACE(WAPI_API, "%s(): AE fisrt set usk!\n",__FUNCTION__);
									pWapiSta->wapiUsk.bSet = true;
									memcpy(pWapiSta->wapiUsk.dataKey,data+10,16);
									memcpy(pWapiSta->wapiUsk.micKey,data+26,16);
									pWapiSta->wapiUsk.keyId = *(data+42);
									pWapiSta->wapiUsk.bTxEnable = true;
									WAPI_DATA(WAPI_API, "SetKey - AE Usk Data Key", pWapiSta->wapiUsk.dataKey, 16);
									WAPI_DATA(WAPI_API, "SetKey - AE Usk Mic Key", pWapiSta->wapiUsk.micKey, 16);
								}
								else               
								{
									WAPI_TRACE(WAPI_API, "%s(): AE update usk!\n",__FUNCTION__);
									pWapiSta->wapiUskUpdate.bSet = true;
									pWapiSta->bAuthenticatorInUpdata = true;
									memcpy(pWapiSta->wapiUskUpdate.dataKey,data+10,16);
									memcpy(pWapiSta->wapiUskUpdate.micKey,data+26,16);
									memcpy(pWapiSta->lastRxUnicastPNBEQueue,WapiASUEPNInitialValueSrc,16);
									memcpy(pWapiSta->lastRxUnicastPNBKQueue,WapiASUEPNInitialValueSrc,16);
									memcpy(pWapiSta->lastRxUnicastPNVIQueue,WapiASUEPNInitialValueSrc,16);
									memcpy(pWapiSta->lastRxUnicastPNVOQueue,WapiASUEPNInitialValueSrc,16);
									memcpy(pWapiSta->lastRxUnicastPN,WapiASUEPNInitialValueSrc,16);
									pWapiSta->wapiUskUpdate.keyId = *(data+42);
									pWapiSta->wapiUskUpdate.bTxEnable = true;
									WAPI_TRACE(WAPI_API, "%s(): keyid=%d\n",__FUNCTION__, pWapiSta->wapiUskUpdate.keyId);
									WAPI_DATA(WAPI_API, "SetKey - AE Usk Data Key", pWapiSta->wapiUskUpdate.dataKey, 16);
									WAPI_DATA(WAPI_API, "SetKey - AE Usk Mic Key", pWapiSta->wapiUskUpdate.micKey, 16);
								}
							}
							else{
								if(!bUpdate){
									WAPI_TRACE(WAPI_API, "%s(): ASUE fisrt set usk!\n",__FUNCTION__);
									if(bTxEnable){
										pWapiSta->wapiUsk.bTxEnable = true;
										memcpy(pWapiSta->lastTxUnicastPN,WapiASUEPNInitialValueSrc,16);
										WAPI_TRACE(WAPI_API, "%s(): Tx enable!\n",__FUNCTION__);
									}else{
										pWapiSta->wapiUsk.bSet = true;
										memcpy(pWapiSta->wapiUsk.dataKey,data+10,16);
										memcpy(pWapiSta->wapiUsk.micKey,data+26,16);
										pWapiSta->wapiUsk.keyId = *(data+42);
										pWapiSta->wapiUsk.bTxEnable = false;
										WAPI_TRACE(WAPI_API, "%s(): Tx disable!\n",__FUNCTION__);
										WAPI_DATA(WAPI_API, "SetKey - AE Usk Data Key", pWapiSta->wapiUsk.dataKey, 16);
										WAPI_DATA(WAPI_API, "SetKey - AE Usk Mic Key", pWapiSta->wapiUsk.micKey, 16);
									}
								}else{
									WAPI_TRACE(WAPI_API, "%s(): ASUE update usk!\n",__FUNCTION__);
									if(bTxEnable){
										pWapiSta->wapiUskUpdate.bTxEnable = true;
										if(pWapiSta->wapiUskUpdate.bSet){
											WAPI_TRACE(WAPI_API, "%s(): ASUE set usk!\n",__FUNCTION__);
											memcpy(pWapiSta->wapiUsk.dataKey,pWapiSta->wapiUskUpdate.dataKey,16);
											memcpy(pWapiSta->wapiUsk.micKey,pWapiSta->wapiUskUpdate.micKey,16);
											pWapiSta->wapiUsk.keyId=pWapiSta->wapiUskUpdate.keyId;
											memcpy(pWapiSta->lastRxUnicastPNBEQueue,WapiAEPNInitialValueSrc,16);
											memcpy(pWapiSta->lastRxUnicastPNBKQueue,WapiAEPNInitialValueSrc,16);
											memcpy(pWapiSta->lastRxUnicastPNVIQueue,WapiAEPNInitialValueSrc,16);
											memcpy(pWapiSta->lastRxUnicastPNVOQueue,WapiAEPNInitialValueSrc,16);
											memcpy(pWapiSta->lastRxUnicastPN,WapiAEPNInitialValueSrc,16);
											pWapiSta->wapiUskUpdate.bTxEnable = false;
											pWapiSta->wapiUskUpdate.bSet = false;
										}
										memcpy(pWapiSta->lastTxUnicastPN,WapiASUEPNInitialValueSrc,16);
									}else{
										WAPI_TRACE(WAPI_API, "%s(): ASUE set update usk!\n",__FUNCTION__);
										pWapiSta->wapiUskUpdate.bSet = true;
										memcpy(pWapiSta->wapiUskUpdate.dataKey,data+10,16);
										memcpy(pWapiSta->wapiUskUpdate.micKey,data+26,16);
										pWapiSta->wapiUskUpdate.keyId = *(data+42);
										pWapiSta->wapiUskUpdate.bTxEnable = false;
										WAPI_TRACE(WAPI_API, "%s(): keyid=%d\n",__FUNCTION__, pWapiSta->wapiUskUpdate.keyId);
										WAPI_DATA(WAPI_API, "SetKey - AE Usk Data Key", pWapiSta->wapiUskUpdate.dataKey, 16);
										WAPI_DATA(WAPI_API, "SetKey - AE Usk Mic Key", pWapiSta->wapiUskUpdate.micKey, 16);
									}
								}
							}
							break;
						case 2:		
							if(bAuthenticator){          
								pWapiInfo->wapiTxMsk.bSet = true;
								memcpy(pWapiInfo->wapiTxMsk.dataKey,data+10,16);													
								memcpy(pWapiInfo->wapiTxMsk.micKey,data+26,16);
								pWapiInfo->wapiTxMsk.keyId = *(data+42);
								pWapiInfo->wapiTxMsk.bTxEnable = true;
								memcpy(pWapiInfo->lastTxMulticastPN,WapiAEMultiCastPNInitialValueSrc,16);
								if(!bUpdate){      
									WAPI_TRACE(WAPI_API, "%s(): AE fisrt set msk!\n",__FUNCTION__);
									if(!pWapiSta->bSetkeyOk)
										pWapiSta->bSetkeyOk = true;
									pWapiInfo->bFirstAuthentiateInProgress= false;
								}else{               
									WAPI_TRACE(WAPI_API, "%s():AE update msk!\n",__FUNCTION__);
								}
								WAPI_TRACE(WAPI_API, "%s(): keyid=%d\n",__FUNCTION__, pWapiInfo->wapiTxMsk.keyId);
								WAPI_DATA(WAPI_API, "SetKey - AE Msk Data Key", pWapiInfo->wapiTxMsk.dataKey, 16);
								WAPI_DATA(WAPI_API, "SetKey - AE Msk Mic Key", pWapiInfo->wapiTxMsk.micKey, 16);
							}
							else{
								if(!bUpdate){
									WAPI_TRACE(WAPI_API, "%s(): ASUE fisrt set msk!\n",__FUNCTION__);
									pWapiSta->wapiMsk.bSet = true;
									memcpy(pWapiSta->wapiMsk.dataKey,data+10,16);													
									memcpy(pWapiSta->wapiMsk.micKey,data+26,16);
									pWapiSta->wapiMsk.keyId = *(data+42);
									pWapiSta->wapiMsk.bTxEnable = false;
									if(!pWapiSta->bSetkeyOk)
										pWapiSta->bSetkeyOk = true;
									pWapiInfo->bFirstAuthentiateInProgress= false;
									WAPI_DATA(WAPI_API, "SetKey - ASUE Msk Data Key", pWapiSta->wapiMsk.dataKey, 16);
									WAPI_DATA(WAPI_API, "SetKey - ASUE Msk Mic Key", pWapiSta->wapiMsk.micKey, 16);
								}else{
									WAPI_TRACE(WAPI_API, "%s(): ASUE update msk!\n",__FUNCTION__);
									pWapiSta->wapiMskUpdate.bSet = true;
									memcpy(pWapiSta->wapiMskUpdate.dataKey,data+10,16);													
									memcpy(pWapiSta->wapiMskUpdate.micKey,data+26,16);
									pWapiSta->wapiMskUpdate.keyId = *(data+42);
									pWapiSta->wapiMskUpdate.bTxEnable = false;
									WAPI_TRACE(WAPI_API, "%s(): keyid=%d\n",__FUNCTION__, pWapiSta->wapiMskUpdate.keyId);
									WAPI_DATA(WAPI_API, "SetKey - ASUE Msk Data Key", pWapiSta->wapiMskUpdate.dataKey, 16);
									WAPI_DATA(WAPI_API, "SetKey - ASUE Msk Mic Key", pWapiSta->wapiMskUpdate.micKey, 16);
								}
							}
							break;
						default:
							WAPI_TRACE(WAPI_ERR, "%s(): Unknown Flag!\n",__FUNCTION__);
							break;
						}
					}
				}
			}
			break;
		}
	    	case WAPI_CMD_SET_MULTICAST_PN:
		{
			PRT_WAPI_T			pWapiInfo = &ieee->wapiInfo;
			PRT_WAPI_STA_INFO	pWapiSta;
			u8				data[22];
			u8				k;

			WAPI_TRACE(WAPI_API, "%s(): WAPI_CMD_SET_MULTICAST_PN!\n",__FUNCTION__);
			if (!ieee->WapiSupport)
			{
				ret = -1;
				break;    
			}
			if(wrq->u.data.length < 22){
				ret = -1;
				break;
			}

			memcpy(data, (u8 *)wrq->u.data.pointer, 22);
			list_for_each_entry(pWapiSta, &pWapiInfo->wapiSTAUsedList, list) {
				if(!memcmp(pWapiSta->PeerMacAddr,data,6))
				{
					for(k=0;k<16;k++)
						pWapiSta->lastRxMulticastPN[k] = data[21-k];
					break;
				}
			}
			WAPI_DATA(WAPI_API, "Multicast Rx PN:", pWapiSta->lastRxMulticastPN, 16);
			break;
		}
	    	case WAPI_CMD_GET_PN:
		{
			PRT_WAPI_T		pWapiInfo = &ieee->wapiInfo;
			
			WAPI_TRACE(WAPI_API, "%s(): WAPI_CMD_GET_PN!\n",__FUNCTION__);
			if (!ieee->WapiSupport){
				ret = -1;
				break;    
			}
			if(wrq->u.data.length<16){
				ret = -1;
				break;
			}
			if(copy_to_user((void *)(wrq->u.data.pointer), pWapiInfo->lastTxMulticastPN, 16)==0)
				wrq->u.data.length = 16;
			break;
		}
	    	case WAPI_CMD_GET_WAPIIE:
		{
			PRT_WAPI_T		pWapiInfo = &ieee->wapiInfo;
			u8 				data[512];
			u8				Length = 0;
			
			WAPI_TRACE(WAPI_API, "%s(): WAPI_CMD_GET_WAPIIE!\n",__FUNCTION__);
			if (!ieee->WapiSupport){
				ret = -1;
				break;    
			}
			if(wrq->u.data.length<257){
				ret = -1;
				break;
			}
#ifdef ASL
			if(ieee->iw_mode == IW_MODE_INFRA || ieee->iw_mode == IW_MODE_APSTA)
#else
			if(ieee->iw_mode == IW_MODE_INFRA)
#endif
			{	
				data[0]= pWapiInfo->assoReqWapiIELength;
				Length ++;
				memcpy(data+Length,pWapiInfo->assoReqWapiIE,pWapiInfo->assoReqWapiIELength);
				Length += pWapiInfo->assoReqWapiIELength;
			}
			else if(ieee->iw_mode == IW_MODE_ADHOC){
				WAPI_DATA(WAPI_API, "GetWapiIE - ADHOC", pWapiInfo->sendbeaconWapiIE, pWapiInfo->sendbeaconWapiIELength);
				data[0] = pWapiInfo->sendbeaconWapiIELength;
				Length++;
				memcpy(data+Length,pWapiInfo->sendbeaconWapiIE,pWapiInfo->sendbeaconWapiIELength);
				Length += pWapiInfo->sendbeaconWapiIELength;
			}
			
			if(copy_to_user((void *)(wrq->u.data.pointer), data, Length)==0)
				wrq->u.data.length = Length;
			break;
	    	}
		case WAPI_CMD_SET_SSID:
		{
			u8 ssid[IW_ESSID_MAX_SIZE + 1];
			
			WAPI_TRACE(WAPI_API, "%s(): WAPI_CMD_SET_SSID!\n",__FUNCTION__);
			if (!ieee->WapiSupport){
				ret = -1;
				break;    
			}
			if((wrq->u.data.length == 0) || (NULL == wrq->u.data.pointer)){
				ret = -1;
				break;
			}
			if(wrq->u.data.length > IW_ESSID_MAX_SIZE){
				ret = -1;
				break;
			}
			printk("===>%s(): wrq->u.essid.flags is %d\n",__FUNCTION__,wrq->u.essid.flags);
			copy_from_user(ssid, (u8 *)wrq->u.essid.pointer, wrq->u.essid.length);

#ifdef TO_DO_LIST
			if(!ieee->wapiInfo.bWapiEnable)
				SetEncryptState(dev);
#endif			
			if((priv->bHwRadioOff == true) || (!priv->up)){
				ret = -1;
				break;
			}
			ret = rtllib_wx_set_essid(ieee,NULL,(union iwreq_data *)&(wrq->u),ssid);
			
			break;
		}
		case WAPI_CMD_GET_BSSID:
			WAPI_TRACE(WAPI_API, "%s(): WAPI_CMD_GET_BSSID!\n",__FUNCTION__);
			if (!ieee->WapiSupport){
				ret = -1;
				break;    
			}
			if((ieee->iw_mode == IW_MODE_INFRA) 
#ifdef ASL
				|| (ieee->iw_mode == IW_MODE_APSTA)
#endif
				|| (ieee->iw_mode == IW_MODE_ADHOC) ){
				if(copy_to_user((void *)(wrq->u.data.pointer), ieee->current_network.bssid, ETH_ALEN)==0)
					wrq->u.data.length = ETH_ALEN;
			}else{
				ret = -1;
			}
			break;
#if 0			
		case WAPI_CMD_GET_LINK_STATUS:
		{
			u8 connect_status = 0;  
			
			WAPI_TRACE(WAPI_API, "%s(): WAPI_CMD_GET_LINK_STATUS!\n",__FUNCTION__);
			if(ieee->state == IEEE80211_LINKED)
				connect_status = 1;
			if(copy_to_user((void *)(wrq->u.data.pointer), &connect_status, sizeof(u8))==0)
				wrq->u.data.length = sizeof(u8);
			break;
		}
#endif
		case WAPI_CMD_SET_IW_MODE:
		{
			union iwreq_data *wrqu = (union iwreq_data *)&(wrq->u);
			WAPI_TRACE(WAPI_API, "%s(): WAPI_CMD_SET_IW_MODE!\n",__FUNCTION__);
			if (!ieee->WapiSupport){
				ret = -1;
				break;    
			}
			WAPI_TRACE(WAPI_API, "%s(): Set iw_mode %d!\n",__FUNCTION__, wrqu->mode);
			wapi_ioctl_set_mode(dev, NULL, wrqu, NULL);
			if(wrqu->mode == IW_MODE_ADHOC)
				ConstructWapiIEForInit(ieee);
			break;
		}
		case WAPI_CMD_SET_DISASSOCIATE:
		{
			u8 destAddr[6];
			
			WAPI_TRACE(WAPI_API, "%s(): WAPI_CMD_SET_DISASSOCIATE!\n",__FUNCTION__);
			if (!ieee->WapiSupport){
				ret = -1;
				break;    
			}
			if(wrq->u.data.length != 6){
				ret = -1;
				break;
			}
			copy_from_user(destAddr, (u8 *)(wrq->u.data.pointer), wrq->u.data.length);

			if(ieee->iw_mode == IW_MODE_INFRA
#ifdef ASL
				|| ieee->iw_mode == IW_MODE_APSTA
#endif
				){
				SendDisassociation(ieee,1,deauth_lv_ss);
				ieee80211_disassociate_from_app(ieee);
			}else if(ieee->iw_mode == IW_MODE_ADHOC){
				WAPI_TRACE(WAPI_API, "%s(): Disassociate "MAC_FMT"!\n",__FUNCTION__, MAC_ARG(destAddr));
				WapiReturnOneStaInfo(ieee, destAddr, 1);
				DelStaInfo(ieee, destAddr);
			}
			break;
		}
		case WAPI_CMD_SAVE_PID:
		{
			u8 strPID[10];
			u32 len = 0;
			int i = 0;

			WAPI_TRACE(WAPI_API, "%s(): WAPI_CMD_SAVE_PID!\n",__FUNCTION__);
			if (!ieee->WapiSupport){
				ret = -1;
				break;    
			}
			if ( !wrq->u.data.pointer ){
				ret = -1;
				break;
			}

			len = wrq->u.data.length;
			memset(strPID, 0, sizeof(strPID));
			if(copy_from_user(strPID, (void *)wrq->u.data.pointer, len)){
				ret = -1;
				break;
			}

			pid_wapi = 0;
			for(i = 0; i < len; i++) {
				pid_wapi = pid_wapi * 10 + (strPID[i] - 48);
			}		
			WAPI_TRACE(WAPI_API, "%s(): strPID=%s pid_wapi=%d!\n",__FUNCTION__, strPID, pid_wapi);
			break;	
		}
	    	case WAPI_CMD_DEQUEUE:
		{
			int QueueDataLen = 0;

			WAPI_TRACE(WAPI_API, "%s(): WAPI_CMD_DEQUEUE!\n",__FUNCTION__);
			if((ret = WAPI_DeQueue(&ieee->wapi_queue_lock, ieee->wapi_queue, QueueData, &QueueDataLen)) != 0){
				if(copy_to_user((void *)(wrq->u.data.pointer), DATAQUEUE_EMPTY, sizeof(DATAQUEUE_EMPTY))==0)
					wrq->u.data.length = sizeof(DATAQUEUE_EMPTY);
			}else{
				if(copy_to_user((void *)wrq->u.data.pointer, (void *)QueueData, QueueDataLen)==0)
					wrq->u.data.length = QueueDataLen;
			}	
			break;
		}
		default:
			WAPI_TRACE(WAPI_ERR, "%s(): Error CMD!\n",__FUNCTION__);
			break;
	}
	
	return ret;
}
#endif

/* based on ipw2200 driver */
int rtl8192_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	struct iwreq *wrq = (struct iwreq *)rq;
	int ret=-1;
	struct rtllib_device *ieee = priv->rtllib;
	u32 key[4];
	u8 broadcast_addr[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
	u8 zero_addr[6] = {0};
	struct iw_point *p = &wrq->u.data;

	down(&priv->wx_sem);

	switch (cmd) {
		case RTL_IOCTL_WPA_SUPPLICANT:
		{
			struct ieee_param *ipw = NULL;
	if (p->length < sizeof(struct ieee_param) || !p->pointer){
		ret = -EINVAL;
		goto out;
	}

	ipw = (struct ieee_param *)kmalloc(p->length, GFP_KERNEL);
	if (ipw == NULL){
		ret = -ENOMEM;
		goto out;
	}
	if (copy_from_user(ipw, p->pointer, p->length)) {
		kfree(ipw);
		ret = -EFAULT;
		goto out;
	}

			if (ipw->cmd == IEEE_CMD_SET_ENCRYPTION)
			{
				if (ipw->u.crypt.set_tx)
				{
					if (strcmp(ipw->u.crypt.alg, "CCMP") == 0)
						ieee->pairwise_key_type = KEY_TYPE_CCMP;
					else if (strcmp(ipw->u.crypt.alg, "TKIP") == 0)
						ieee->pairwise_key_type = KEY_TYPE_TKIP;
					else if (strcmp(ipw->u.crypt.alg, "WEP") == 0)
					{
						if (ipw->u.crypt.key_len == 13)
							ieee->pairwise_key_type = KEY_TYPE_WEP104;
						else if (ipw->u.crypt.key_len == 5)
							ieee->pairwise_key_type = KEY_TYPE_WEP40;
					}
					else
						ieee->pairwise_key_type = KEY_TYPE_NA;

					if (ieee->pairwise_key_type)
					{
						if (memcmp(ieee->ap_mac_addr, zero_addr, 6) == 0)
							ieee->iw_mode = IW_MODE_ADHOC;

						memcpy((u8*)key, ipw->u.crypt.key, 16);
						EnableHWSecurityConfig8192(dev);
						set_swcam(dev, 4, ipw->u.crypt.idx, ieee->pairwise_key_type, (u8*)ieee->ap_mac_addr, 0, key,0,0);
						setKey(dev, 4, ipw->u.crypt.idx, ieee->pairwise_key_type, (u8*)ieee->ap_mac_addr, 0, key);
						if (ieee->iw_mode == IW_MODE_ADHOC){  
							set_swcam(dev, ipw->u.crypt.idx, ipw->u.crypt.idx, ieee->pairwise_key_type, (u8*)ieee->ap_mac_addr, 0, key,0,0);
							setKey(dev, ipw->u.crypt.idx, ipw->u.crypt.idx, ieee->pairwise_key_type, (u8*)ieee->ap_mac_addr, 0, key);
						}
					}
#ifdef RTL8192E
					if ((ieee->pairwise_key_type == KEY_TYPE_CCMP) && ieee->pHTInfo->bCurrentHTSupport){
						write_nic_byte(dev, 0x173, 1); 
					}
#endif

				}
				else 
				{
					memcpy((u8*)key, ipw->u.crypt.key, 16);
					if (strcmp(ipw->u.crypt.alg, "CCMP") == 0)
						ieee->group_key_type= KEY_TYPE_CCMP;
					else if (strcmp(ipw->u.crypt.alg, "TKIP") == 0)
						ieee->group_key_type = KEY_TYPE_TKIP;
					else if (strcmp(ipw->u.crypt.alg, "WEP") == 0)
					{
						if (ipw->u.crypt.key_len == 13)
							ieee->group_key_type = KEY_TYPE_WEP104;
						else if (ipw->u.crypt.key_len == 5)
							ieee->group_key_type = KEY_TYPE_WEP40;
					}
					else
						ieee->group_key_type = KEY_TYPE_NA;

					if (ieee->group_key_type)
					{
						set_swcam(	dev, 
								ipw->u.crypt.idx,
								ipw->u.crypt.idx,		
								ieee->group_key_type,	
								broadcast_addr,	
								0,		
								key,		
								0,
								0);
						setKey(	dev, 
								ipw->u.crypt.idx,
								ipw->u.crypt.idx,		
								ieee->group_key_type,	
								broadcast_addr,	
								0,		
								key);		
					}
				}
			}
#ifdef JOHN_DEBUG
			{
				int i;
				printk("@@ wrq->u pointer = ");
				for(i=0;i<wrq->u.data.length;i++){
					if(i%10==0) printk("\n");
					printk( "%8x|", ((u32*)wrq->u.data.pointer)[i] );
				}
				printk("\n");
			}
#endif 
			ret = rtllib_wpa_supplicant_ioctl(priv->rtllib, &wrq->u.data,0,0);

			kfree(ipw);
			break; 
		}
#if defined (RTL8192S_WAPI_SUPPORT)
	    	case WAPI_CMD_GET_WAPI_SUPPORT:
	    	case WAPI_CMD_SET_WAPI_ENABLE:
	    	case WAPI_CMD_SET_WAPI_PSK:
	    	case WAPI_CMD_SET_KEY:
	    	case WAPI_CMD_SET_MULTICAST_PN:
	    	case WAPI_CMD_GET_PN:
	    	case WAPI_CMD_GET_WAPIIE:
	    	case WAPI_CMD_SET_SSID:
	    	case WAPI_CMD_GET_BSSID:
	    	case WAPI_CMD_SET_IW_MODE:
	    	case WAPI_CMD_SET_DISASSOCIATE:
		case WAPI_CMD_SAVE_PID:
	    	case WAPI_CMD_DEQUEUE:
			ret = WAPI_Ioctl(dev, wrq, cmd);
			break;
#endif
		default:
			ret = -EOPNOTSUPP;
			break;
	}

out:
	up(&priv->wx_sem);

	return ret;
}

void FairBeacon(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	struct rtllib_network *net = &priv->rtllib->current_network;
	static u8 i=100;
	static u8 forceturn =0;
	u16		beaconint = net->beacon_interval;

	if (priv->rtllib->iw_mode == IW_MODE_ADHOC)
		net = &priv->rtllib->current_network;
#ifdef ASL
	else 
		net = &priv->rtllib->current_ap_network; 
#endif
	beaconint = net->beacon_interval;
	if(priv->bIbssCoordinator){
		i--;

		if(forceturn ==2){
			forceturn =0;
			priv->rtllib->SetHwRegHandler(dev, HW_VAR_BEACON_INTERVAL, (u8*)(&beaconint));
			i=100;
		}

		if(i<=94){
			beaconint=beaconint+2;
			priv->rtllib->SetHwRegHandler(dev, HW_VAR_BEACON_INTERVAL, (u8*)(&beaconint));
			forceturn =1;
		}
	} else {
		i++;

		if(forceturn ==1){
			forceturn =0;
			priv->rtllib->SetHwRegHandler(dev, HW_VAR_BEACON_INTERVAL, (u8*)(&beaconint));
			i=100;
		}

		if(i>=106){
			beaconint=beaconint-2;
			priv->rtllib->SetHwRegHandler(dev, HW_VAR_BEACON_INTERVAL, (u8*)(&beaconint));
			forceturn =2;
		}
	}
}


irqreturn_type rtl8192_interrupt(int irq, void *netdev, struct pt_regs *regs)
{
	struct net_device *dev = (struct net_device *) netdev;
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
#ifdef _RTL8192_EXT_PATCH_
	struct net_device *meshdev = ((struct rtllib_device *)netdev_priv_rsl(dev))->meshdev;
#endif
#ifdef ASL 
	struct net_device *apdev = ((struct rtllib_device *)netdev_priv_rsl(dev))->apdev;
#endif
	unsigned long flags;
	u32 inta;
	u32 intb;
	intb = 0;
	
	if(priv->irq_enabled == 0){
		goto done;
	}
	spin_lock_irqsave(&priv->irq_th_lock,flags);

	priv->ops->interrupt_recognized(dev, &inta, &intb);
	priv->stats.shints++;

	if (!inta) {
		spin_unlock_irqrestore(&priv->irq_th_lock,flags);
		goto done;
	}

	if (inta == 0xffff) {
		spin_unlock_irqrestore(&priv->irq_th_lock,flags);
		goto done;
	}

	priv->stats.ints++;

#ifdef _RTL8192_EXT_PATCH_
#ifdef ASL
	if (!netif_running(dev) && !netif_running(meshdev) && !netif_running(apdev)) 
#else
	if (!netif_running(dev) && !netif_running(meshdev)) 
#endif
#else
#ifdef ASL
	if (!netif_running(dev) && !netif_running(apdev)) 
#else
	if (!netif_running(dev)) 
#endif
#endif
	{
		spin_unlock_irqrestore(&priv->irq_th_lock,flags);
		goto done;
	}

#if defined RTL8192SE 
	if(intb & IMR_TBDOK){
		RT_TRACE(COMP_INTR, "beacon ok interrupt!\n");
		priv->stats.txbeaconokint++;
		priv->bIbssCoordinator = true;
	}

	if(intb & IMR_TBDER){
		RT_TRACE(COMP_INTR, "beacon error interrupt!\n");
		priv->stats.txbeaconerr++;
		priv->bIbssCoordinator = false;
	}

	if ((intb & IMR_TBDOK) ||(intb & IMR_TBDER)) {
		if((priv->rtllib->iw_mode == IW_MODE_ADHOC) 
#ifdef ASL
			|| (((priv->rtllib->iw_mode == IW_MODE_MASTER) || (priv->rtllib->iw_mode == IW_MODE_APSTA)) && priv->rtllib->ap_state == RTLLIB_LINKED)
#endif
	) {
		FairBeacon(dev);
		}
	}
#else
	if(inta & IMR_TBDOK){
		RT_TRACE(COMP_INTR, "beacon ok interrupt!\n");
		priv->stats.txbeaconokint++;
	}

	if(inta & IMR_TBDER){
		RT_TRACE(COMP_INTR, "beacon ok interrupt!\n");
		priv->stats.txbeaconerr++;
	}
#endif

	if(inta & IMR_BDOK) {
		RT_TRACE(COMP_INTR, "beacon interrupt!\n");
#ifdef _ENABLE_SW_BEACON
		rtl8192_tx_isr(dev, BEACON_QUEUE); 
#endif
	}

	if(inta  & IMR_MGNTDOK ) {
		RT_TRACE(COMP_INTR, "Manage ok interrupt!\n");
		priv->stats.txmanageokint++;
		rtl8192_tx_isr(dev,MGNT_QUEUE);
		spin_unlock_irqrestore(&priv->irq_th_lock,flags);
		if (priv->rtllib->ack_tx_to_ieee){
			if (rtl8192_is_tx_queue_empty(dev)){
				priv->rtllib->ack_tx_to_ieee = 0;
				rtllib_ps_tx_ack(priv->rtllib, 1);
			}
		}
		spin_lock_irqsave(&priv->irq_th_lock,flags);
	}

#ifndef RTL8192CE
	if (inta & IMR_COMDOK) {
		priv->stats.txcmdpktokint++;
		rtl8192_tx_isr(dev,TXCMD_QUEUE);
	}
#endif

	if (inta & IMR_HIGHDOK) {
		rtl8192_tx_isr(dev,HIGH_QUEUE);
	}

#ifdef RTL8192SE
	if ((inta & IMR_ROK) || (inta & IMR_RXCMDOK))
#else
	if (inta & IMR_ROK) 
#endif
	{
		priv->stats.rxint++;
		priv->InterruptLog.nIMR_ROK++;
		tasklet_schedule(&priv->irq_rx_tasklet);
	}

	if (inta & IMR_BcnInt) {
		RT_TRACE(COMP_INTR, "prepare beacon for interrupt!\n");
#ifndef _ENABLE_SW_BEACON
		tasklet_schedule(&priv->irq_prepare_beacon_tasklet);
#endif
	}

	if (inta & IMR_RDU) {
		RT_TRACE(COMP_INTR, "rx descriptor unavailable!\n");
		priv->stats.rxrdu++;
#ifndef RTL8192CE
		write_nic_dword(dev,INTA_MASK,read_nic_dword(dev, INTA_MASK) & ~IMR_RDU); 
#endif    
		tasklet_schedule(&priv->irq_rx_tasklet);
	}

	if (inta & IMR_RXFOVW) {
		RT_TRACE(COMP_INTR, "rx overflow !\n");
		priv->stats.rxoverflow++;
		tasklet_schedule(&priv->irq_rx_tasklet);
	}

	if (inta & IMR_TXFOVW) priv->stats.txoverflow++;

	if (inta & IMR_BKDOK) { 
		RT_TRACE(COMP_INTR, "BK Tx OK interrupt!\n");
		priv->stats.txbkokint++;
		priv->rtllib->LinkDetectInfo.NumTxOkInPeriod++;
		rtl8192_tx_isr(dev,BK_QUEUE);
	}

	if (inta & IMR_BEDOK) { 
		RT_TRACE(COMP_INTR, "BE TX OK interrupt!\n");
		priv->stats.txbeokint++;
		priv->rtllib->LinkDetectInfo.NumTxOkInPeriod++;
		rtl8192_tx_isr(dev,BE_QUEUE);
	}

	if (inta & IMR_VIDOK) { 
		RT_TRACE(COMP_INTR, "VI TX OK interrupt!\n");
		priv->stats.txviokint++;
		priv->rtllib->LinkDetectInfo.NumTxOkInPeriod++;
		rtl8192_tx_isr(dev,VI_QUEUE);
	}

	if (inta & IMR_VODOK) { 
		priv->stats.txvookint++;
		RT_TRACE(COMP_INTR, "Vo TX OK interrupt!\n");
		priv->rtllib->LinkDetectInfo.NumTxOkInPeriod++;
		rtl8192_tx_isr(dev,VO_QUEUE);
	}

	spin_unlock_irqrestore(&priv->irq_th_lock,flags);

done:

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	return;
#else
	return IRQ_HANDLED;
#endif
}



/****************************************************************************
     ---------------------------- PCI_STUFF---------------------------
*****************************************************************************/
#ifdef HAVE_NET_DEVICE_OPS
static const struct net_device_ops rtl8192_netdev_ops = {
	.ndo_open = rtl8192_open,
	.ndo_stop = rtl8192_close,
	.ndo_tx_timeout = rtl8192_tx_timeout,
	.ndo_do_ioctl = rtl8192_ioctl,
	.ndo_set_multicast_list = r8192_set_multicast,
	.ndo_set_mac_address = r8192_set_mac_adr,
	.ndo_validate_addr = eth_validate_addr,
	.ndo_change_mtu = eth_change_mtu,
	.ndo_start_xmit = rtllib_xmit,
};
#endif

static int __devinit rtl8192_pci_probe(struct pci_dev *pdev,
			 const struct pci_device_id *id)
{
	unsigned long ioaddr = 0;
	struct net_device *dev = NULL;
	struct r8192_priv *priv= NULL;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	struct rtl819x_ops *ops = (struct rtl819x_ops *)(id->driver_data);
#endif
	
#ifdef CONFIG_RTL8192_IO_MAP
	unsigned long pio_start, pio_len, pio_flags;
#else
	unsigned long pmem_start, pmem_len, pmem_flags;
#endif 
	int err = 0;
#ifdef _RTL8192_EXT_PATCH_
	int result;
	struct net_device *meshdev = NULL;
	struct meshdev_priv *mpriv;
	char meshifname[]="mesh0";
#endif	
#ifdef ASL
	int result;
	struct net_device *apdev = NULL;
	struct apdev_priv *appriv;
	char apifname[]="softap0";
#endif
	bool bdma64 = false;

	RT_TRACE(COMP_INIT,"Configuring chip resources");
	
#ifdef RTL8192SE
	if((pdev->device == 0x8192) && (pdev->revision == HAL_HW_PCI_REVISION_ID_8192PCIE)){
		printk("it's 8192se driver Please select proper driver before install!!!!\n");
		return -ENODEV;
	}
#endif
#ifdef RTL8192E
	if((pdev->device == 0x8192) && (pdev->revision == HAL_HW_PCI_REVISION_ID_8192SE)){
		printk("it's 8192e driver Please select proper driver before install!!!!\n");
		return -ENODEV;
	}
#endif
	
	if( pci_enable_device (pdev) ){
		RT_TRACE(COMP_ERR,"Failed to enable PCI device");
		return -EIO;
	}

	pci_set_master(pdev);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10))
#define DMA_BIT_MASK(n)	(((n) == 64) ? ~0ULL : ((1ULL < (n)) -1))
#endif

#ifdef CONFIG_64BIT_DMA
	if (!pci_set_dma_mask(pdev, DMA_BIT_MASK(64))) {
		printk("RTL819xCE: Using 64bit DMA\n");
		if (pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(64))) {
			printk( "Unable to obtain 64bit DMA for consistent allocations\n");
			pci_disable_device(pdev);
			return -ENOMEM;
		}
		bdma64 = true;
	} else 
#endif
	{
		if (!pci_set_dma_mask(pdev, DMA_BIT_MASK(32))) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
			if (pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32))) {
				printk( "Unable to obtain 32bit DMA for consistent allocations\n");
				pci_disable_device(pdev);
				return -ENOMEM;
			} 
#endif	
		}
	}

	dev = alloc_rtllib(sizeof(struct r8192_priv));
	if (!dev) {
		pci_disable_device(pdev);
		return -ENOMEM;
	}
	
	if(bdma64){
		dev->features |= NETIF_F_HIGHDMA;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
	SET_MODULE_OWNER(dev);
#endif

	pci_set_drvdata(pdev, dev);	
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	SET_NETDEV_DEV(dev, &pdev->dev);
#endif
	priv = rtllib_priv(dev);
	priv->rtllib = (struct rtllib_device *)netdev_priv_rsl(dev);
	priv->pdev=pdev;
	priv->rtllib->pdev=pdev;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	if((pdev->subsystem_vendor == PCI_VENDOR_ID_DLINK)&&(pdev->subsystem_device == 0x3304)){
		priv->rtllib->bSupportRemoteWakeUp = 1;
	} else 
#endif
	{
		priv->rtllib->bSupportRemoteWakeUp = 0;
	}

#ifdef CONFIG_RTL8192_IO_MAP
	pio_start = (unsigned long)pci_resource_start (pdev, 0);
	pio_len = (unsigned long)pci_resource_len (pdev, 0);
	pio_flags = (unsigned long)pci_resource_flags (pdev, 0);
 	
      	if (!(pio_flags & IORESOURCE_IO)) {
		RT_TRACE(COMP_ERR,"region #0 not a PIO resource, aborting");
		goto fail_free_dev;
	}
	
	printk("Io mapped space start: 0x%08lx \n", pio_start );
	if( ! request_region( pio_start, pio_len, DRV_NAME ) ){
		RT_TRACE(COMP_ERR,"request_region failed!");
		goto fail_free_dev;
	}
	
	ioaddr = pio_start;
	dev->base_addr = ioaddr; 
#else
#ifdef RTL8192CE
	pmem_start = pci_resource_start(pdev, 2);
	pmem_len = pci_resource_len(pdev, 2);
	pmem_flags = pci_resource_flags (pdev, 2);
#else
	pmem_start = pci_resource_start(pdev, 1);
	pmem_len = pci_resource_len(pdev, 1);
	pmem_flags = pci_resource_flags (pdev, 1);
#endif
	
	if (!(pmem_flags & IORESOURCE_MEM)) {
		RT_TRACE(COMP_ERR,"region #1 not a MMIO resource, aborting");
		goto fail_free_dev;
	}
	
	printk("Memory mapped space start: 0x%08lx \n", pmem_start);
	if (!request_mem_region(pmem_start, pmem_len, DRV_NAME)) {
		RT_TRACE(COMP_ERR,"request_mem_region failed!");
		goto fail_free_dev;
	}
	
	
	ioaddr = (unsigned long)ioremap_nocache( pmem_start, pmem_len);	
	if( ioaddr == (unsigned long)NULL ){
		RT_TRACE(COMP_ERR,"ioremap failed!");
		goto fail_release_region;
	}
	
	dev->mem_start = ioaddr; 
	dev->mem_end = ioaddr + pci_resource_len(pdev, 0); 
#endif 

#if defined RTL8192SE || defined RTL8192CE
        pci_write_config_byte(pdev, 0x81,0);
        pci_write_config_byte(pdev,0x44,0);
        pci_write_config_byte(pdev,0x04,0x06);
        pci_write_config_byte(pdev,0x04,0x07);
#endif        

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	priv->ops = ops;
#else
#if defined RTL8190P || defined RTL8192E
	priv->ops = &rtl819xp_ops;
#else
	priv->ops = &rtl8192se_ops;
#endif
#endif

	if(rtl8192_pci_findadapter(pdev, dev) == false)
		goto fail_iounmap;

	dev->irq = pdev->irq;
	priv->irq = 0;

#ifdef HAVE_NET_DEVICE_OPS
	dev->netdev_ops = &rtl8192_netdev_ops;
#else	
	dev->open = rtl8192_open;
	dev->stop = rtl8192_close;
	dev->tx_timeout = rtl8192_tx_timeout;
	dev->do_ioctl = rtl8192_ioctl;
	dev->set_multicast_list = r8192_set_multicast;
	dev->set_mac_address = r8192_set_mac_adr;
	dev->hard_start_xmit = rtllib_xmit;
#endif	

#if WIRELESS_EXT >= 12
#if WIRELESS_EXT < 17
        dev->get_wireless_stats = r8192_get_wireless_stats;
#endif
        dev->wireless_handlers = (struct iw_handler_def *) &r8192_wx_handlers_def;
#endif
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)	
	dev->ethtool_ops = &rtl819x_ethtool_ops;
#endif

	dev->type = ARPHRD_ETHER;
	dev->watchdog_timeo = HZ*3;	

	if (dev_alloc_name(dev, ifname) < 0){
                RT_TRACE(COMP_INIT, "Oops: devname already taken! Trying wlan%%d...\n");
		dev_alloc_name(dev, ifname);
        }
	
	RT_TRACE(COMP_INIT, "Driver probe completed1\n");
	if(rtl8192_init(dev)!=0){ 
		RT_TRACE(COMP_ERR, "Initialization failed");
		goto fail_iounmap;
	}

#if defined CONFIG_CFG_80211 && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)  
	if(!rtl8192_register_wiphy_dev(dev))
		goto fail_iounmap;
#endif

	netif_carrier_off(dev);
	
	register_netdev(dev);
	RT_TRACE(COMP_INIT, "dev name: %s\n",dev->name);

	netif_stop_queue(dev);

	err = rtl_debug_module_init(priv, dev->name);
	if (err) {
		RT_TRACE(COMP_DBG, "failed to create debugfs files. Ignoring error: %d\n", err);	
	}
	rtl8192_proc_init_one(dev);
	
#ifdef ENABLE_GPIO_RADIO_CTL
	if(priv->polling_timer_on == 0){
		check_rfctrl_gpio_timer((unsigned long)dev);
	}
#endif
#ifdef _RTL8192_EXT_PATCH_
	meshdev = alloc_netdev(sizeof(struct meshdev_priv),meshifname,meshdev_init);
	netif_stop_queue(meshdev);
	memcpy(meshdev->dev_addr, dev->dev_addr, ETH_ALEN);
	DMESG("Card MAC address for meshdev is "MAC_FMT, MAC_ARG(meshdev->dev_addr));

	meshdev->base_addr = dev->base_addr;
	meshdev->irq = dev->irq;
	meshdev->mem_start = dev->mem_start;
	meshdev->mem_end = dev->mem_end;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21)
	SET_NETDEV_DEV(meshdev, dev->dev.parent);
#endif

	if ((result = register_netdev(meshdev)))
	{
		printk("Error %i registering device %s",result, meshdev->name);
		goto fail_free_dev;	
	}
	else
	{
		mpriv = (struct meshdev_priv *)netdev_priv_rsl(meshdev);
		priv->rtllib->meshdev=meshdev;
		priv->rtllib->meshstats=meshdev_stats(meshdev);
		priv->rtllib->only_mesh = 1;  
		priv->rtllib->p2pmode = 0;
		priv->rtllib->serverExtChlOffset = 0;
		mpriv->rtllib = priv->rtllib;
		mpriv->priv = priv;
	}
#endif
#ifdef ASL
	apdev = alloc_netdev(sizeof(struct apdev_priv),apifname,apdev_init);
	netif_stop_queue(apdev);
	memcpy(apdev->dev_addr, dev->dev_addr, ETH_ALEN);
	DMESG("Card MAC address for meshdev is "MAC_FMT, MAC_ARG(apdev->dev_addr));


	apdev->base_addr = dev->base_addr;
	apdev->irq = dev->irq;
	apdev->mem_start = dev->mem_start;
	apdev->mem_end = dev->mem_end;
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21)
	SET_NETDEV_DEV(apdev, dev->dev.parent);
#endif
	
	if ((result = register_netdev(apdev))) {
		printk("Error %i registering device %s",result,apdev->name);
		goto fail_free_dev;	
	} else {
		appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
		priv->rtllib->apdev=apdev;
		priv->rtllib->apstats=apdev_stats(apdev);
		appriv->rtllib = priv->rtllib;
		appriv->priv = priv;
		spin_lock_init(&appriv->lock);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
#endif
	}
#endif
	
#ifdef RTL8192CE
	mdelay(1000);
#endif

#ifdef CONFIG_RTL_RFKILL
	if (priv->ops->init_before_adapter_start) {
		priv->ops->init_before_adapter_start(dev);
		priv->initialized_at_probe = true;
	}
	if (!rtl8192_rfkill_init(dev)) {
		goto fail_iounmap;
	}
#endif	

#if defined CONFIG_ASPM_OR_D3
	rtl8192_initialize_adapter_common(dev);
#endif

	RT_TRACE(COMP_INIT, "Driver probe completed\n");
	return 0;	

fail_iounmap:
#ifdef CONFIG_RTL8192_IO_MAP
#else
	if( dev->mem_start != (unsigned long)NULL ){
		iounmap( (void *)dev->mem_start );
	}
#endif 
		
fail_release_region:
#ifdef CONFIG_RTL8192_IO_MAP
	if( dev->base_addr != 0 ){
		release_region(dev->base_addr, 
	       pci_resource_len(pdev, 0) );
	}
#else
#ifdef RTL8192CE
		release_mem_region( pci_resource_start(pdev, 2), 
				    pci_resource_len(pdev, 2) );
#else
		release_mem_region( pci_resource_start(pdev, 1), 
				    pci_resource_len(pdev, 1) );
#endif
#endif
	
fail_free_dev:
	if(dev){
		unregister_netdev(dev);
#ifdef ASL
                if(apdev)
                      	unregister_netdev(apdev);	
#endif
#ifdef _RTL8192_EXT_PATCH_
                if(meshdev)
                      	unregister_netdev(meshdev);
#endif
		if (priv->irq) {
			free_irq(dev->irq, dev);
			dev->irq=0;
		}
		free_rtllib(dev);

	}
	
	pci_set_drvdata(pdev, NULL);
	pci_disable_device(pdev);
	
	DMESG("wlan driver load failed\n");
	return -ENODEV;
	
}

static void __devexit rtl8192_pci_disconnect(struct pci_dev *pdev)
{
	struct net_device *dev = pci_get_drvdata(pdev);
	struct r8192_priv *priv ;
#ifdef _RTL8192_EXT_PATCH_
	struct net_device *meshdev;
#endif
#ifdef ASL
	struct net_device *apdev;
#endif
	if(dev){
#ifdef CONFIG_RTL_RFKILL
		rtl8192_rfkill_exit(dev);
#endif
		unregister_netdev(dev);
#ifdef ASL
		priv=rtllib_priv(dev);
		if(priv->rtllib->apdev)
		{
			apdev = priv->rtllib->apdev;
			priv->rtllib->apdev = NULL;
#ifdef ASL_DEBUG
			printk("\nFreeing wlan0ap.........");
#endif
		
			if(apdev){
				unregister_netdev(apdev);
			}
#ifdef ASL_DEBUG
			printk("\nFreed wlan0ap");
#endif
		}
#endif		
		
		priv = rtllib_priv(dev);
		
#ifdef _RTL8192_EXT_PATCH_
		rtl8192_dinit_mshobj(priv);	
#endif 

#ifdef CONFIG_BT_30
	bt_wifi_deinit(dev);
#endif

#ifdef ENABLE_GPIO_RADIO_CTL
		del_timer_sync(&priv->gpio_polling_timer);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)	
		cancel_delayed_work(&priv->gpio_change_rf_wq);
#endif
		priv->polling_timer_on = 0;
#endif
		rtl_debug_module_remove(priv);
		rtl8192_proc_remove_one(dev);
#ifdef _RTL8192_EXT_PATCH_
		rtl8192_down(dev,true);
		if(priv && priv->rtllib->meshdev)
		{
			meshdev = priv->rtllib->meshdev;
			priv->rtllib->meshdev = NULL;
		
			if(meshdev){
				meshdev_down(meshdev);
				unregister_netdev(meshdev);
			}
		}
#else		
		rtl8192_down(dev,true);
#endif
		deinit_hal_dm(dev);
#ifdef CONFIG_ASPM_OR_D3
		;
#endif
#ifdef RTL8192SE
		DeInitSwLeds(dev);
#endif
		if (priv->pFirmware)
		{
			vfree(priv->pFirmware);
			priv->pFirmware = NULL;
		}
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
		destroy_workqueue(priv->priv_wq);
#endif
                {
                    u32 i;
                    rtl8192_free_rx_ring(dev);
                    for (i = 0; i < MAX_TX_QUEUE_COUNT; i++) {
                        rtl8192_free_tx_ring(dev, i);
                    }
                }

		if(priv->irq){
			
			printk("Freeing irq %d\n",dev->irq);
			free_irq(dev->irq, dev);
			priv->irq=0;
			
		}
#ifdef CONFIG_RTL8192_IO_MAP
		if( dev->base_addr != 0 ){
			
			release_region(dev->base_addr, 
				       pci_resource_len(pdev, 0) );
		}
#else
		if( dev->mem_start != (unsigned long)NULL ){
			iounmap( (void *)dev->mem_start );
#ifdef RTL8192CE
			release_mem_region( pci_resource_start(pdev, 2), 
				    pci_resource_len(pdev, 2) );
#else
			release_mem_region( pci_resource_start(pdev, 1), 
					    pci_resource_len(pdev, 1) );
#endif
		}
#endif /*end #ifdef RTL_IO_MAP*/
		free_rtllib(dev);

		if(priv->scan_cmd)
			kfree(priv->scan_cmd);

	} else{
		priv=rtllib_priv(dev);
        }

	pci_disable_device(pdev);
#ifdef RTL8192SE   
        pci_write_config_byte(pdev, 0x81,1);
        pci_write_config_byte(pdev,0x44,3);
#endif
	RT_TRACE(COMP_DOWN, "wlan driver removed\n");
}

bool NicIFEnableNIC(struct net_device* dev)
{
	bool init_status = true;
	struct r8192_priv* priv = rtllib_priv(dev);
	PRT_POWER_SAVE_CONTROL pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));

	if(IS_NIC_DOWN(priv)){
		RT_TRACE(COMP_ERR, "ERR!!! %s(): Driver is already down!\n",__FUNCTION__);
		priv->bdisable_nic = false;  
		return RT_STATUS_FAILURE;
	}

	RT_TRACE(COMP_PS, "===========>%s()\n",__FUNCTION__);
	priv->bfirst_init = true;
	init_status = priv->ops->initialize_adapter(dev);
	if (init_status != true) {
		RT_TRACE(COMP_ERR,"ERR!!! %s(): initialization is failed!\n",__FUNCTION__);
		priv->bdisable_nic = false;  
		return -1;
	}
	RT_TRACE(COMP_INIT, "start adapter finished\n");
	RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);
	priv->bfirst_init = false;

	rtl8192_irq_enable(dev);
	priv->bdisable_nic = false;
	RT_TRACE(COMP_PS,"<===========%s()\n",__FUNCTION__);
	return init_status;
}
bool NicIFDisableNIC(struct net_device* dev)
{
	bool	status = true;
	struct r8192_priv* priv = rtllib_priv(dev);
	u8 tmp_state = 0;
	RT_TRACE(COMP_PS, "=========>%s()\n",__FUNCTION__);
	priv->bdisable_nic = true;	
	tmp_state = priv->rtllib->state;
#ifdef _RTL8192_EXT_PATCH_
	if((priv->rtllib->iw_mode == IW_MODE_MESH) && (priv->rtllib->only_mesh)) 
		rtllib_softmac_stop_protocol(priv->rtllib, 1, 0, false);
	else
		rtllib_softmac_stop_protocol(priv->rtllib, 0, 0, false);
#else
	rtllib_softmac_stop_protocol(priv->rtllib, 0, 0, false);
#endif
	priv->rtllib->state = tmp_state;
	rtl8192_cancel_deferred_work(priv);
	rtl8192_irq_disable(dev);

	priv->ops->stop_adapter(dev, false);
	RT_TRACE(COMP_PS, "<=========%s()\n",__FUNCTION__);

	return status;
}

static int __init rtl8192_pci_module_init(void)
{
	int ret;
	int error;

#ifdef BUILT_IN_CRYPTO
        ret = arc4_init();
        if (ret) {
                printk(KERN_ERR "arc4_init() failed %d\n", ret);
                return ret;
        }


        ret = michael_mic_init();
        if (ret) {
                printk(KERN_ERR "michael_mic_init() failed %d\n", ret);
                return ret;
        }

        ret = aes_init();
        if (ret) {
                printk(KERN_ERR "aes_init() failed %d\n", ret);
                return ret;
        }
#endif
#ifdef BUILT_IN_RTLLIB
	ret = rtllib_init();
	if (ret) {
		printk(KERN_ERR "rtllib_init() failed %d\n", ret);
		return ret;
	}
	ret = rtllib_crypto_init();
	if (ret) {
		printk(KERN_ERR "rtllib_crypto_init() failed %d\n", ret);
		return ret;
	}
	ret = rtllib_crypto_tkip_init();
	if (ret) {
		printk(KERN_ERR "rtllib_crypto_tkip_init() failed %d\n", ret);
		return ret;
	}
	ret = rtllib_crypto_ccmp_init();
	if (ret) {
		printk(KERN_ERR "rtllib_crypto_ccmp_init() failed %d\n", ret);
		return ret;
	}
	ret = rtllib_crypto_wep_init();
	if (ret) {
		printk(KERN_ERR "rtllib_crypto_wep_init() failed %d\n", ret);
		return ret;
	}
#endif
#ifdef BUILT_IN_MSHCLASS
	ret = msh_init();
	if (ret) {
		printk(KERN_ERR "msh_init() failed %d\n", ret);
		return ret;
	}
#endif
	printk(KERN_INFO "\nLinux kernel driver for RTL8192 based WLAN cards\n");
	printk(KERN_INFO "Copyright (c) 2007-2008, Realsil Wlan Driver\n");
	RT_TRACE(COMP_INIT, "Initializing module");
	RT_TRACE(COMP_INIT, "Wireless extensions version %d", WIRELESS_EXT);

	error = rtl_create_debugfs_root();
	if (error) {
		RT_TRACE(COMP_DBG, "Create debugfs root fail: %d\n", error);
		goto err_out;
	}

	rtl8192_proc_module_init();
#if(LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22))
      if(0!=pci_module_init(&rtl8192_pci_driver))
#else
      if(0!=pci_register_driver(&rtl8192_pci_driver))
#endif
	{
		DMESG("No device found");
		/*pci_unregister_driver (&rtl8192_pci_driver);*/
		return -ENODEV;
	}
	return 0;
err_out:
        return error;

}

static void __exit rtl8192_pci_module_exit(void)
{
	pci_unregister_driver(&rtl8192_pci_driver);

	RT_TRACE(COMP_DOWN, "Exiting");
	rtl8192_proc_module_remove();
	rtl_remove_debugfs_root();
#ifdef BUILT_IN_RTLLIB
	rtllib_crypto_tkip_exit();
	rtllib_crypto_ccmp_exit();
	rtllib_crypto_wep_exit();
	rtllib_crypto_deinit();
	rtllib_exit();
#endif	
#ifdef BUILT_IN_CRYPTO
        arc4_exit();
        michael_mic_exit();
        aes_fini();
#endif
#ifdef BUILT_IN_MSHCLASS
	msh_exit();
#endif

}

void check_rfctrl_gpio_timer(unsigned long data)
{
	struct r8192_priv* priv = rtllib_priv((struct net_device *)data);

	priv->polling_timer_on = 1;

	queue_delayed_work_rsl(priv->priv_wq,&priv->gpio_change_rf_wq,0);

	mod_timer(&priv->gpio_polling_timer, jiffies + MSECS(RTLLIB_WATCH_DOG_TIME));
}

/***************************************************************************
     ------------------- module init / exit stubs ----------------
****************************************************************************/
module_init(rtl8192_pci_module_init);
module_exit(rtl8192_pci_module_exit);

MODULE_DESCRIPTION("Linux driver for Realtek RTL819x WiFi cards");
MODULE_AUTHOR(DRV_COPYRIGHT " " DRV_AUTHOR);
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
MODULE_VERSION(DRV_VERSION);
#endif
MODULE_LICENSE("GPL");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 9)
module_param(ifname, charp, S_IRUGO|S_IWUSR );
module_param(hwwep,int, S_IRUGO|S_IWUSR);
module_param(channels,int, S_IRUGO|S_IWUSR);
#else
MODULE_PARM(ifname, "s");
MODULE_PARM(hwwep,"i");
MODULE_PARM(channels,"i");
#endif

MODULE_PARM_DESC(ifname," Net interface name, wlan%d=default");
MODULE_PARM_DESC(hwwep," Try to use hardware WEP support(default use hw. set 0 to use software security)");
MODULE_PARM_DESC(channels," Channel bitmask for specific locales. NYI");
