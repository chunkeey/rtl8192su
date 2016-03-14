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

#ifdef CONFIG_PM_RTL

#include "rtl_core.h"
#include "rtl_pm.h"
#ifdef _RTL8192_EXT_PATCH_
#include "../../mshclass/msh_class.h"
#endif


int rtl8192E_save_state (struct pci_dev *dev, pm_message_t state)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10))
        printk(KERN_NOTICE "r8192E save state call (state %u).\n", state);
#else
        printk(KERN_NOTICE "r8192E save state call (state %u).\n", state.event);
#endif
	return(-EAGAIN);
}


int rtl8192E_suspend (struct pci_dev *pdev, pm_message_t state)
{
	struct net_device *dev = pci_get_drvdata(pdev);
	struct r8192_priv *priv = rtllib_priv(dev); 
#ifdef _RTL8192_EXT_PATCH_
 	struct net_device *meshdev = priv->rtllib->meshdev;
#endif

#if !(defined RTL8192SE || defined RTL8192CE)		
	u32	ulRegRead;
#endif

        RT_TRACE(COMP_POWER, "============> r8192E suspend call.\n");
        printk("============> r8192E suspend call.\n");
#ifdef ENABLE_GPIO_RADIO_CTL
	del_timer_sync(&priv->gpio_polling_timer);
	cancel_delayed_work(&priv->gpio_change_rf_wq);
	priv->polling_timer_on = 0;
#endif

#ifdef _RTL8192_EXT_PATCH_
        if ((!netif_running(dev)) && (!netif_running(meshdev)))
#else
        if (!netif_running(dev))
#endif
	{
            printk("RTL819XE:UI is open out of suspend function\n");
            goto out_pci_suspend;
        }

#ifdef HAVE_NET_DEVICE_OPS
	if (!(priv->up)) {
		priv->rtllib->wlan_up_before_suspend = 0;
	} else {
		priv->rtllib->wlan_up_before_suspend = 1;
		
		printk("====>%s(2), stop wlan, wlan_up_before_suspend = %d\n", __func__, priv->rtllib->wlan_up_before_suspend);		
	        if (dev->netdev_ops->ndo_stop)
		        dev->netdev_ops->ndo_stop(dev);
	}
#ifdef _RTL8192_EXT_PATCH_
	if (!(priv->mesh_up)) {
		printk("====>%s(1), stop wlan, wlan_up_before_suspend = %d\n", __func__, priv->rtllib->wlan_up_before_suspend);		
		priv->rtllib->mesh_up_before_suspend = 0;
	} else {
		priv->rtllib->mesh_up_before_suspend = 1;
			
		if (meshdev->netdev_ops->ndo_stop)
			meshdev->netdev_ops->ndo_stop(meshdev);
	}
#endif
#else
	if (!(priv->up))
                priv->rtllib->wlan_up_before_suspend = 0;
        else {
                priv->rtllib->wlan_up_before_suspend = 1;
	        dev->stop(dev);
	}
#ifdef _RTL8192_EXT_PATCH_
	if (!(priv->mesh_up))
                priv->rtllib->mesh_up_before_suspend = 0;
        else {
                priv->rtllib->mesh_up_before_suspend = 1;
		meshdev->stop(meshdev);
	}	
#endif
#endif	
	netif_device_detach(dev);

#if !(defined RTL8192SE || defined RTL8192CE)		
	if(!priv->rtllib->bSupportRemoteWakeUp) {
		MgntActSet_RF_State(dev, eRfOff, RF_CHANGE_BY_INIT,true);
		ulRegRead = read_nic_dword(dev, CPU_GEN);	
		ulRegRead|=CPU_GEN_SYSTEM_RESET;
		write_nic_dword(dev, CPU_GEN, ulRegRead);
	} else {
		write_nic_dword(dev, WFCRC0, 0xffffffff);
		write_nic_dword(dev, WFCRC1, 0xffffffff);
		write_nic_dword(dev, WFCRC2, 0xffffffff);
#ifdef RTL8190P
		{
			u8	ucRegRead;	
			ucRegRead = read_nic_byte(dev, GPO);
			ucRegRead |= BIT0;
			write_nic_byte(dev, GPO, ucRegRead);
		}
#endif			
		write_nic_byte(dev, PMR, 0x5);
		write_nic_byte(dev, MacBlkCtrl, 0xa);
	}
#endif
out_pci_suspend:
	RT_TRACE(COMP_POWER, "r8192E support WOL call??????????????????????\n");
	printk("r8192E support WOL call??????????????????????\n");
	if(priv->rtllib->bSupportRemoteWakeUp) {
		RT_TRACE(COMP_POWER, "r8192E support WOL call!!!!!!!!!!!!!!!!!!.\n");
	}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10))
	pci_save_state(pdev,&(priv->pci_state));
        pci_disable_device(pdev);
        pci_enable_wake(pdev, state,\
                        priv->rtllib->bSupportRemoteWakeUp?1:0);
        pci_set_power_state(pdev,state);
#else
	pci_save_state(pdev);
	pci_disable_device(pdev);
	pci_enable_wake(pdev, pci_choose_state(pdev,state),\
			priv->rtllib->bSupportRemoteWakeUp?1:0);
	pci_set_power_state(pdev,pci_choose_state(pdev,state));
#endif

        mdelay(20);

	return 0;
}

int rtl8192E_resume (struct pci_dev *pdev)
{
    struct net_device *dev = pci_get_drvdata(pdev);
    struct r8192_priv *priv = rtllib_priv(dev); 
#ifdef _RTL8192_EXT_PATCH_
	struct net_device *meshdev = priv->rtllib->meshdev;
#endif

    int err;
    u32 val;

    RT_TRACE(COMP_POWER, "================>r8192E resume call.");
    printk("================>r8192E resume call.\n");

    pci_set_power_state(pdev, PCI_D0);

    err = pci_enable_device(pdev);
    if(err) {
        printk(KERN_ERR "%s: pci_enable_device failed on resume\n",
                dev->name);
        return err;
    }
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10))
    pci_restore_state(pdev,&(priv->pci_state));
#else
    pci_restore_state(pdev);
#endif

    pci_read_config_dword(pdev, 0x40, &val);
    if ((val & 0x0000ff00) != 0) {
        pci_write_config_dword(pdev, 0x40, val & 0xffff00ff);
    }

    pci_enable_wake(pdev, PCI_D0, 0);

#ifdef ENABLE_GPIO_RADIO_CTL
    if(priv->polling_timer_on == 0){
        check_rfctrl_gpio_timer((unsigned long)dev);
    }
#endif

#ifdef _RTL8192_EXT_PATCH_
    if ((!netif_running(dev)) && (!netif_running(meshdev)))
#else
    if(!netif_running(dev))
#endif
    {
        printk("RTL819XE:UI is open out of resume function\n");
        goto out;
    }

    netif_device_attach(dev);
#ifdef HAVE_NET_DEVICE_OPS
	if (priv->rtllib->wlan_up_before_suspend) {
                if (dev->netdev_ops->ndo_open)
	                dev->netdev_ops->ndo_open(dev);
	}
#ifdef _RTL8192_EXT_PATCH_
        if (priv->rtllib->mesh_up_before_suspend) { 
		if (meshdev->netdev_ops->ndo_open)
			meshdev->netdev_ops->ndo_open(meshdev);
		netif_carrier_on(meshdev);
	}
#endif
#else
	if (priv->rtllib->wlan_up_before_suspend) {
            dev->open(dev);
	}
#ifdef _RTL8192_EXT_PATCH_
	if (priv->rtllib->mesh_up_before_suspend) {
		meshdev->open(meshdev);
		netif_carrier_on(meshdev);
	}
#endif
#endif    

#if !(defined RTL8192SE || defined RTL8192CE)		
    if(!priv->rtllib->bSupportRemoteWakeUp) {
	    MgntActSet_RF_State(dev, eRfOn, RF_CHANGE_BY_INIT,true);
    }
#endif

out:
    RT_TRACE(COMP_POWER, "<================r8192E resume call.\n");
    return 0;
}


int rtl8192E_enable_wake (struct pci_dev *dev, pm_message_t state, int enable)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10))
        printk(KERN_NOTICE "r8192E enable wake call (state %u, enable %d).\n", 
	       state, enable);
#else
        printk(KERN_NOTICE "r8192E enable wake call (state %u, enable %d).\n", 
	       state.event, enable);
#endif
	return(-EAGAIN);
}

#endif 
