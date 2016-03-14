NIC_SELECT = RTL8192SE

CC = gcc
KVER  := $(shell uname -r)
MODDESTDIR := /lib/modules/$(KVER)/kernel/drivers/net/wireless/

ifeq ($(shell uname -r|cut -d. -f1,2), 2.6)
###################### KERNEL 2.6 ################################
ifeq ($(NIC_SELECT),RTL8192CE)
sub_folder = rtl8192c
EXTRA_CFLAGS = -DRTL8192CE
EXTRA_CFLAGS += -DRTL8190_Download_Firmware_From_Header=1 \
		-DCONFIG_PM_RTL 			\
		-DENABLE_IPS 				\
		-DENABLE_LPS 				\
		-DCONFIG_ASPM_OR_D3 			\
		-DENABLE_GPIO_RADIO_CTL 		\
		-DENABLE_DOT11D				\
		#					\
		-DENABLE_DYNAMIC_TXPOWER		\
		-DCONFIG_CRDA	 			\
		-DCONFIG_CFG_80211 			\
		-DENABLE_TKIP11N 			\
		-D_ENABLE_SW_BEACON 			\
		-DUSE_FW_SOURCE_IMG_FILE 		\
		-DEFUSE_REPG_WORKAROUND			\
		-DENABLE_AMSDU 				\
		-DADHOC_11N 				\
		-DFOR_ANDROID_X86			\
		-DCONFIG_64BIT_DMA			\
		-DCONFIG_BT_30				\
		#
endif
ifeq ($(NIC_SELECT),RTL8192CE_BT30)
sub_folder = rtl8192c
EXTRA_CFLAGS = -DRTL8192CE
EXTRA_CFLAGS += -DRTL8190_Download_Firmware_From_Header=1 \
		-DCONFIG_PM_RTL 			\
		-DENABLE_GPIO_RADIO_CTL 		\
		-DENABLE_DOT11D				\
		-DCONFIG_BT_30				\
		-DBT_BEFORE_MOTO			\
		-DBT_AES				\
		#					\
		-DENABLE_LPS 				\
		-DCONFIG_ASPM_OR_D3 			\
		-DENABLE_IPS 				\
		-DENABLE_DYNAMIC_TXPOWER		\
		-DCONFIG_CRDA	 			\
		-DCONFIG_CFG_80211 			\
		-DENABLE_TKIP11N 			\
		-D_ENABLE_SW_BEACON 			\
		-DUSE_FW_SOURCE_IMG_FILE 		\
		-DEFUSE_REPG_WORKAROUND			\
		-DENABLE_AMSDU 				\
		-DADHOC_11N 				\
		-DFOR_ANDROID_X86			\
		-DCONFIG_64BIT_DMA			\
		#
endif

ifeq ($(NIC_SELECT),RTL8192SE)
sub_folder = rtl8192s
EXTRA_CFLAGS = -DRTL8192SE
EXTRA_CFLAGS += -DCONFIG_PM_RTL 			\
		-DENABLE_GPIO_RADIO_CTL 		\
		-DRTL8190_Download_Firmware_From_Header=1 \
		-DENABLE_LPS 				\
		-DCONFIG_RX_CMD				\
		-DENABLE_IPS 				\
		-DUSE_FW_SOURCE_IMG_FILE 		\
		-DENABLE_DOT11D 			\
		-DCONFIG_ASPM_OR_D3 			\
		-DRTLLIB_RADIOTAP			\
		#					\
		-DRTL8192S_WAPI_SUPPORT			\
		-DCONFIG_CRDA	 			\
		-DCONFIG_CFG_80211 			\
		-DCONFIG_RTL_RFKILL	 		\
		-DCONFIG_FW_SETCHAN			\
		-DRTL8192S_WAPI_SUPPORT			\
		-D_ENABLE_SW_BEACON 			\
		-DCONFIG_MP 				\
		-DENABLE_AMSDU 				\
		-DADHOC_11N				\
		-DEFUSE_REPG_WORKAROUN			\
		-DENABLE_TKIP11N			\
		-DFOR_ANDROID_X86			\
		#
		#NOTICE:
		#[1] CONFIG_MP is for MP test, 
		#    you should close IPS/LPS/GPIO/ASPM 
		#[2] In order to support CRDA, RFKILL 
      		#    CONFIG_CFG_80211 need to be enabled at first.
		#[3] In order to support CRDA, 
      		#    DOT11D need to be enabled at first.
		#[4] CRDA and RFKILL can be enabled individually. 
                
		
endif
ifeq ($(NIC_SELECT),RTL8192SE_SA)
sub_folder = rtl8192s
EXTRA_CFLAGS = -DRTL8192SE
EXTRA_CFLAGS += -DCONFIG_PM_RTL 			\
		-DENABLE_GPIO_RADIO_CTL 		\
		-DRTL8190_Download_Firmware_From_Header=1 \
		-DCONFIG_RX_CMD				\
		-DENABLE_IPS 				\
		-DUSE_FW_SOURCE_IMG_FILE 		\
		-DENABLE_DOT11D 			\
		-DCONFIG_ASPM_OR_D3 			\
		-DRTLLIB_RADIOTAP			\
		-DENABLE_AMSDU 				\
		-DASL					\
		-DASL_WME				\
		-DCONFIG_WPS				\
		#					\
		-DENABLE_LPS 				\
		-DCONFIG_CRDA	 			\
		-DCONFIG_CFG_80211 			\
		-DCONFIG_RTL_RFKILL	 		\
		-DCONFIG_FW_SETCHAN			\
		-DRTL8192S_WAPI_SUPPORT			\
		-D_ENABLE_SW_BEACON 			\
		-DCONFIG_MP 				\
		-DENABLE_AMSDU 				\
		-DADHOC_11N				\
		-DEFUSE_REPG_WORKAROUN			\
		-DENABLE_TKIP11N			\
		-DFOR_ANDROID_X86			\
		#
		#NOTICE:
		#[1] CONFIG_MP is for MP test, 
		#    you should close IPS/LPS/GPIO/ASPM 
		#[2] In order to support CRDA, RFKILL 
      		#    CONFIG_CFG_80211 need to be enabled at first.
		#[3] In order to support CRDA, 
      		#    DOT11D need to be enabled at first.
		#[4] CRDA and RFKILL can be enabled individually. 
                
		
endif
ifeq ($(NIC_SELECT),RTL8190P)
sub_folder = rtl8192e
EXTRA_CFLAGS += -DRTL8190P				\
		-DUSE_FW_SOURCE_IMG_FILE		\
		-DENABLE_DOT11D 			\
		-DCONFIG_PM_RTL 			\
		#					\
		-DENABLE_TKIP11N			\
		-DFOR_ANDROID_X86			\
		#
endif
ifeq ($(NIC_SELECT),RTL8192E)
sub_folder = rtl8192e
EXTRA_CFLAGS+=-DRTL8192E				\
		-DUSE_FW_SOURCE_IMG_FILE		\
		-DENABLE_IPS				\
		-DENABLE_LPS				\
		-DENABLE_GPIO_RADIO_CTL			\
		-DCONFIG_PM_RTL 			\
		-DENABLE_DOT11D 			
		#					\
		-DCONFIG_RTL8192_IO_MAP			\
		-DCONFIG_ASPM_OR_D3 			\
		-DENABLE_TKIP11N			\
		-DFOR_ANDROID_X86			\
		#
endif
ifeq ($(NIC_SELECT),RTL8191SE_MESH)
sub_folder = rtl8192s
EXTRA_CFLAGS += -DRTL8192SE 				\
		-DRTL8190_Download_Firmware_From_Header=1 \
		-DEFUSE_REPG_WORKAROUND 		\
		-DCONFIG_PM_RTL 			\
		-DENABLE_GPIO_RADIO_CTL			\
		-DENABLE_IPS 				\
		-DENABLE_LPS				\
		-DCONFIG_ASPM_OR_D3 			\
		-DCUSTOMER_ID_INTEL_CMPC 		\
		-DMESH_8192_SPECIFIED 			\
		-D_RTL8192_EXT_PATCH_ 			\
		-D_MESH_DEBUG_ 				\
		-DTREE_BASED_ROUTING_ENABLE 		\
		-DMESH_PATHSEL_DBG_LV=0 		\
		-DMESH_PATHSEL_DBG_SIMPLE=0 		\
		-DMESH_PATHSEL_DBG_TEST=0		\
		-DBUILT_IN_MSHCLASS 			\
		-DENABLE_AMSDU				\
		-DUSE_FW_SOURCE_IMG_FILE 		\
		-DENABLE_DOT11D 			\
		-DCONFIG_RX_CMD				\
		#					\
		-DADHOC_11N				\
		-DCOMPATIBLE_WITH_RALINK_MESH 		\
		-DCONFIG_CRDA	 			\
		-DCONFIG_FW_SETCHAN			\
		-DRTL8192S_WAPI_SUPPORT			\
		-DMESH_AUTO_TEST			\
		-D_ENABLE_SW_BEACON			\
		-DENABLE_TKIP11N			\
		-DFOR_ANDROID_X86			\
		-DCONFIG_MP 				\
		#
endif

ifeq ($(NIC_SELECT),RTL8192CE_MESH)
sub_folder = rtl8192c
EXTRA_CFLAGS = -DRTL8192CE
EXTRA_CFLAGS += -DRTL8190_Download_Firmware_From_Header=1 \
		-DENABLE_TKIP11N 			\
		-DCONFIG_PM_RTL 			\
		-DENABLE_IPS 				\
		-DENABLE_LPS 				\
		-DCONFIG_ASPM_OR_D3 			\
		-DENABLE_GPIO_RADIO_CTL 		\
		-DENABLE_DOT11D				\
		-DCUSTOMER_ID_INTEL_CMPC 		\
		-DMESH_8192_SPECIFIED 			\
		-D_RTL8192_EXT_PATCH_ 			\
		-D_MESH_DEBUG_ 				\
		-DTREE_BASED_ROUTING_ENABLE 		\
		-DMESH_PATHSEL_DBG_LV=0 		\
		-DMESH_PATHSEL_DBG_SIMPLE=0 		\
		-DMESH_PATHSEL_DBG_TEST=0		\
		-DBUILT_IN_MSHCLASS 			\
		-DENABLE_AMSDU				\
		-DENABLE_DYNAMIC_TXPOWER		\
		#					\
		-D_ENABLE_SW_BEACON 			\
		-DUSE_FW_SOURCE_IMG_FILE 		\
		-DCOMPATIBLE_WITH_RALINK_MESH 		\
		-DEFUSE_REPG_WORKAROUND			\
		-DENABLE_AMSDU 				\
		-DADHOC_11N 				\
		-DFOR_ANDROID_X86			\
		-DCONFIG_64BIT_DMA			\
		#
endif

#it will fail to compile in suse linux enterprise 10 sp2. This flag is to solve this problem.
ifeq ($(shell uname -r | cut -d. -f1,2,3,4), 2.6.16.60-0)
	EXTRA_CFLAGS += -DOPENSUSE_SLED=1
else
	EXTRA_CFLAGS += -DOPENSUSE_SLED=0
endif

# Switch AP will fail in moblin2. This flag is to solve this issue.
ifeq ($(shell uname -r | cut -d. -f6 | cut -d- -f1), moblin2)
	EXTRA_CFLAGS += -DFOR_MOBLIN
endif

EXTRA_CFLAGS += -I$(TOPDIR)/drivers/net/wireless
EXTRA_CFLAGS += -std=gnu89
EXTRA_CFLAGS += -O2
EXTRA_CFLAGS += -mhard-float -DCONFIG_FORCE_HARD_FLOAT=y
EXTRA_CFLAGS += -DTHOMAS_TURBO
#flag for single module compile
EXTRA_CFLAGS += -DBUILT_IN_RTLLIB
#EXTRA_CFLAGS += -DCONFIG_RTLWIFI_DEBUGFS
ifeq ($(NIC_SELECT),RTL8190P)
r8190_pci-objs := rtl_core.o 		\
		  rtl_eeprom.o 		\
		  rtl_ps.o 		\
		  rtl_wx.o 		\
		  rtl_cam.o		\
		  rtl_dm.o 		\
		  rtl_pm.o 		\
		  rtl_pci.o 		\
		  rtl_debug.o 		\
		  rtl_ethtool.o		\
		  rtl8192e/r8190P_hwimg.o	\
		  rtl8192e/r8190P_rtl8256.o 	\
		  rtl8192e/r8192E_dev.o		\
		  rtl8192e/r8192E_phy.o 		\
		  rtl8192e/r8192E_firmware.o 	\
		  rtl8192e/r8192E_cmdpkt.o 	\
		  ../../rtllib/rtllib_rx.o                \
		  ../../rtllib/rtllib_softmac.o           \
		  ../../rtllib/rtllib_tx.o                \
		  ../../rtllib/rtllib_wx.o                \
		  ../../rtllib/rtllib_module.o            \
		  ../../rtllib/rtllib_softmac_wx.o        \
		  ../../rtllib/rtl819x_HTProc.o              \
		  ../../rtllib/rtl819x_TSProc.o              \
		  ../../rtllib/rtl819x_BAProc.o              \
		  ../../rtllib/dot11d.o                      \
		  ../../rtllib/rtllib_crypt.o             \
		  ../../rtllib/rtllib_crypt_tkip.o        \
		  ../../rtllib/rtllib_crypt_ccmp.o        \
		  ../../rtllib/rtllib_crypt_wep.o

obj-m := r8190_pci.o
endif
ifeq ($(NIC_SELECT),RTL8192E)
r8192e_pci-objs := rtl_core.o 		\
		  rtl_eeprom.o 	\
		  rtl_ps.o 		\
		  rtl_wx.o   	\
		  rtl_cam.o		\
		  rtl_dm.o 		\
		  rtl_pm.o 		\
		  rtl_pci.o 		\
		  rtl_debug.o		\
		  rtl_ethtool.o		\
		  rtl8192e/r8192E_dev.o		\
		  rtl8192e/r8192E_phy.o 		\
		  rtl8192e/r8192E_firmware.o 	\
		  rtl8192e/r8192E_cmdpkt.o 	\
		  rtl8192e/r8192E_hwimg.o 	\
		  rtl8192e/r8190P_rtl8256.o 	\
		  ../../rtllib/rtllib_rx.o                \
		  ../../rtllib/rtllib_softmac.o           \
		  ../../rtllib/rtllib_tx.o                \
		  ../../rtllib/rtllib_wx.o                \
		  ../../rtllib/rtllib_module.o            \
		  ../../rtllib/rtllib_softmac_wx.o        \
		  ../../rtllib/rtl819x_HTProc.o              \
		  ../../rtllib/rtl819x_TSProc.o              \
		  ../../rtllib/rtl819x_BAProc.o              \
		  ../../rtllib/dot11d.o                      \
		  ../../rtllib/rtllib_crypt.o             \
		  ../../rtllib/rtllib_crypt_tkip.o        \
		  ../../rtllib/rtllib_crypt_ccmp.o        \
		  ../../rtllib/rtllib_crypt_wep.o
obj-m := r8192e_pci.o
endif
ifeq ($(NIC_SELECT),RTL8192SE)
r8192se_pci-objs := rtl_core.o 		\
		  rtl_regd.o		\
		  rtl_rfkill.o		\
		  rtl_eeprom.o 		\
		  rtl_wx.o 		\
		  rtl_cam.o		\
		  rtl_pm.o 		\
		  rtl_pci.o 		\
		  rtl_ps.o 		\
		  rtl_dm.o 		\
		  rtl_debug.o		\
		  rtl_ethtool.o         \
		  rtl8192s/r8192S_dev.o 	\
		  rtl8192s/r8192S_Efuse.o 	\
		  rtl8192s/r8192S_phy.o 	\
		  rtl8192s/r8192S_firmware.o 	\
		  rtl8192s/r8192S_rtl6052.o 	\
		  rtl8192s/r8192S_hwimg.o 	\
		  rtl8192s/r8192S_led.o 	\
		  rtl8192s/r8192S_mp.o		\
		  rtl8192s/r8192S_scan.o	\
		  ../../rtllib/rtllib_rx.o                \
		  ../../rtllib/rtllib_softmac.o           \
		  ../../rtllib/rtllib_tx.o                \
		  ../../rtllib/rtllib_wx.o                \
		  ../../rtllib/rtllib_module.o            \
		  ../../rtllib/rtllib_softmac_wx.o        \
		  ../../rtllib/rtl819x_HTProc.o              \
		  ../../rtllib/rtl819x_TSProc.o              \
		  ../../rtllib/rtl819x_BAProc.o              \
		  ../../rtllib/dot11d.o                      \
		  ../../rtllib/rtllib_crypt.o             \
		  ../../rtllib/rtllib_crypt_tkip.o        \
		  ../../rtllib/rtllib_crypt_ccmp.o        \
		  ../../rtllib/rtllib_crypt_wep.o		\
		  ../../rtllib/wapi.o        			\
		  ../../rtllib/wapi_interface.o
#		  rtl8192e/r8190P_rtl8256.o 	
obj-m := r8192se_pci.o
endif
ifeq ($(NIC_SELECT),RTL8192SE_SA)
r8192se_pci_softap-objs := rtl_core.o 		\
		  rtl_regd.o		\
		  rtl_rfkill.o		\
		  rtl_eeprom.o 		\
		  rtl_wx.o 		\
		  rtl_cam.o		\
		  rtl_pm.o 		\
		  rtl_pci.o 		\
		  rtl_ps.o 		\
		  rtl_dm.o 		\
		  rtl_debug.o		\
		  rtl_ethtool.o         \
		  rtl_softap.o		\
		  rtl8192s/r8192S_dev.o 	\
		  rtl8192s/r8192S_Efuse.o 	\
		  rtl8192s/r8192S_phy.o 	\
		  rtl8192s/r8192S_firmware.o 	\
		  rtl8192s/r8192S_rtl6052.o 	\
		  rtl8192s/r8192S_hwimg.o 	\
		  rtl8192s/r8192S_led.o 	\
		  rtl8192s/r8192S_mp.o		\
		  rtl8192s/r8192S_scan.o	\
		  ../../rtllib/rtllib_rx.o                \
		  ../../rtllib/rtllib_softmac.o           \
		  ../../rtllib/rtllib_tx.o                \
		  ../../rtllib/rtllib_wx.o                \
		  ../../rtllib/rtllib_module.o            \
		  ../../rtllib/rtllib_softmac_wx.o        \
		  ../../rtllib/rtl819x_HTProc.o              \
		  ../../rtllib/rtl819x_TSProc.o              \
		  ../../rtllib/rtl819x_BAProc.o              \
		  ../../rtllib/dot11d.o                      \
		  ../../rtllib/rtllib_crypt.o             \
		  ../../rtllib/rtllib_crypt_tkip.o        \
		  ../../rtllib/rtllib_crypt_ccmp.o        \
		  ../../rtllib/rtllib_crypt_wep.o		\
		  ../../rtllib/wapi.o        			\
		  ../../rtllib/wapi_interface.o
#		  rtl8192e/r8190P_rtl8256.o 	
obj-m := r8192se_pci_softap.o
endif

ifeq ($(NIC_SELECT),RTL8192CE)
r8192ce_pci-objs := rtl_core.o 		\
		  rtl_eeprom.o 		\
		  rtl_wx.o 		\
		  rtl_cam.o		\
		  rtl_pm.o 		\
		  rtl_pci.o 		\
		  rtl_ps.o 		\
		  rtl_debug.o		\
		  rtl_ethtool.o         \
		  rtl_regd.o		\
		  rtl8192c/r8192C_dev.o 	\
		  rtl8192c/r8192C_tx.o 		\
		  rtl8192c/r8192C_rx.o 		\
		  rtl8192c/r8192C_Efuse.o 	\
		  rtl8192c/r8192C_phy.o 	\
		  rtl8192c/r8192C_firmware.o 	\
        	  rtl8192c/r8192C_dmbt.o          \
        	  rtl8192c/r8192C_dmout.o          \
		  rtl8192c/r8192C_dm.o 		\
		  rtl8192c/r8192C_rtl6052.o 	\
		  rtl8192c/r8192C_hwimg.o 	\
		  rtl8192c/r8192C_led.o 	\
		  rtl8192c/r8192C_com.o 	\
		  ../../rtllib/rtllib_rx.o                \
		  ../../rtllib/rtllib_softmac.o           \
		  ../../rtllib/rtllib_tx.o                \
		  ../../rtllib/rtllib_wx.o                \
		  ../../rtllib/rtllib_module.o            \
		  ../../rtllib/rtllib_softmac_wx.o        \
		  ../../rtllib/rtl819x_HTProc.o              \
		  ../../rtllib/rtl819x_TSProc.o              \
		  ../../rtllib/rtl819x_BAProc.o              \
		  ../../rtllib/dot11d.o                      \
		  ../../rtllib/rtllib_crypt.o             \
		  ../../rtllib/rtllib_crypt_tkip.o        \
		  ../../rtllib/rtllib_crypt_ccmp.o        \
		  ../../rtllib/rtllib_crypt_wep.o      

obj-m := r8192ce_pci.o
endif

ifeq ($(NIC_SELECT),RTL8192CE_BT30)
r8192ce_pci-objs := rtl_core.o 		\
		  rtl_eeprom.o 		\
		  rtl_wx.o 		\
		  rtl_cam.o		\
		  rtl_pm.o 		\
		  rtl_pci.o 		\
		  rtl_ps.o 		\
		  rtl_debug.o		\
		  rtl_ethtool.o         \
		  rtl_regd.o		\
		  rtl8192c/r8192C_dev.o 	\
		  rtl8192c/r8192C_tx.o 		\
		  rtl8192c/r8192C_rx.o 		\
		  rtl8192c/r8192C_Efuse.o 	\
		  rtl8192c/r8192C_phy.o 	\
		  rtl8192c/r8192C_firmware.o 	\
        	  rtl8192c/r8192C_dmbt.o          \
        	  rtl8192c/r8192C_dmout.o          \
		  rtl8192c/r8192C_dm.o 		\
		  rtl8192c/r8192C_rtl6052.o 	\
		  rtl8192c/r8192C_hwimg.o 	\
		  rtl8192c/r8192C_led.o 	\
		  rtl8192c/r8192C_com.o 	\
		  ../../rtllib/rtllib_rx.o                \
		  ../../rtllib/rtllib_softmac.o           \
		  ../../rtllib/rtllib_tx.o                \
		  ../../rtllib/rtllib_wx.o                \
		  ../../rtllib/rtllib_module.o            \
		  ../../rtllib/rtllib_softmac_wx.o        \
		  ../../rtllib/rtl819x_HTProc.o              \
		  ../../rtllib/rtl819x_TSProc.o              \
		  ../../rtllib/rtl819x_BAProc.o              \
		  ../../rtllib/dot11d.o                      \
		  ../../rtllib/rtllib_crypt.o             \
		  ../../rtllib/rtllib_crypt_tkip.o        \
		  ../../rtllib/rtllib_crypt_ccmp.o        \
		  ../../rtllib/rtllib_crypt_wep.o      \
		  ../../btlib/bt_aes.o			\
		  ../../btlib/bt_rc4.o			\
		  ../../btlib/bt_md5c.o			\
		  ../../btlib/bt_kmsm_hmac.o		\
		  ../../btlib/bt_kmsm_prf.o		\
		  ../../btlib/bt_kmsm_eapolkey.o	\
		  ../../btlib/bt_hci.o			\
		  ../../btlib/bt_handlePacket.o		\
		  ../../btlib/bt_wifi.o			\
		  ../../btlib/bt_ioctl.o
obj-m := r8192ce_pci.o
endif

ifeq ($(NIC_SELECT),RTL8191SE_MESH)
r8192se_pci_mesh-objs := rtl_core.o 	\
		  rtl_regd.o		\
		  rtl_rfkill.o		\
		  rtl_eeprom.o 		\
		  rtl_ps.o 		\
		  rtl_wx.o 		\
		  rtl_cam.o		\
		  rtl_dm.o 		\
		  rtl_pm.o 		\
		  rtl_pci.o 		\
		  rtl_debug.o		\
		  rtl_ethtool.o         \
		  rtl_mesh.o		\
		  rtl8192s/r8192S_dev.o \
		  rtl8192s/r8192S_Efuse.o 	\
		  rtl8192s/r8192S_phy.o 	\
		  rtl8192s/r8192S_firmware.o 	\
		  rtl8192s/r8192S_rtl6052.o 	\
		  rtl8192s/r8192S_hwimg.o 	\
		  rtl8192s/r8192S_led.o 	\
		  rtl8192s/r8192S_mp.o		\
		  rtl8192s/r8192S_scan.o		\
		  ../../rtllib/rtllib_rx.o                \
		  ../../rtllib/rtllib_softmac.o           \
		  ../../rtllib/rtllib_tx.o                \
		  ../../rtllib/rtllib_wx.o                \
		  ../../rtllib/rtllib_module.o            \
		  ../../rtllib/rtllib_softmac_wx.o        \
		  ../../rtllib/rtl819x_HTProc.o           \
		  ../../rtllib/rtl819x_TSProc.o           \
		  ../../rtllib/rtl819x_BAProc.o           \
		  ../../rtllib/dot11d.o                   \
		  ../../rtllib/rtllib_crypt.o             \
		  ../../rtllib/rtllib_crypt_tkip.o        \
		  ../../rtllib/rtllib_crypt_ccmp.o        \
		  ../../rtllib/rtllib_crypt_wep.o	  \
		  ../../mshclass/msh_class.o 		  \
		  ../../mshclass/mesh_8192_util.o  	  \
		  ../../mshclass/rtl8192_mesh.o 	  \
		  ../../mshclass/rtl8192s_sme.o 	  \
	  	  ../../mshclass/rtl8192_proc.o 	  \
		  ../../mshclass/8192s_pathsel.o 	  \
		  ../../mshclass/hash_table.o 		  \
		  ../../mshclass/rtl8192_sha1.o           \
		  ../../mshclass/rtl8192_sha256.o         \
		  ../../mshclass/rtl8192_aes.o            \
		  ../../mshclass/mesh_abbr_handshk.o      \
		  ../../mshclass/rtl8192_bn_ctx.o         \
		  ../../mshclass/rtl8192_bn_asm.o         \
		  ../../mshclass/rtl8192_bn_gcd.o         \
		  ../../mshclass/rtl8192_bn_mont.o        \
		  ../../mshclass/rtl8192_bn_recp.o        \
		  ../../mshclass/rtl8192_bn_lib.o         \
		  ../../mshclass/mesh_ath.o		  \
		  ../../mshclass/pathselection.o 	  \
		  ../../mshclass/tree_proactive.o
#		  rtl8192e/r8190P_rtl8256.o 	
obj-m := r8192se_pci_mesh.o
endif

ifeq ($(NIC_SELECT),RTL8192CE_MESH)
r8192ce_pci_mesh-objs := rtl_core.o 		\
		  rtl_eeprom.o 		\
		  rtl_wx.o 		\
		  rtl_cam.o		\
		  rtl_pm.o 		\
		  rtl_pci.o 		\
		  rtl_ps.o 		\
		  rtl_debug.o		\
		  rtl_ethtool.o         \
		  rtl_mesh.o		\
		  rtl8192c/r8192C_dev.o 	\
		  rtl8192c/r8192C_tx.o 		\
		  rtl8192c/r8192C_rx.o 		\
		  rtl8192c/r8192C_Efuse.o 	\
		  rtl8192c/r8192C_phy.o 	\
		  rtl8192c/r8192C_firmware.o 	\
        	  rtl8192c/r8192C_dmbt.o          \
        	  rtl8192c/r8192C_dmout.o          \
		  rtl8192c/r8192C_dm.o 		\
		  rtl8192c/r8192C_rtl6052.o 	\
		  rtl8192c/r8192C_hwimg.o 	\
		  rtl8192c/r8192C_led.o 	\
		  rtl8192c/r8192C_com.o 	\
		  rtl8192c/r8192C_PhyParam.o 	\
		  ../../rtllib/rtllib_rx.o                \
		  ../../rtllib/rtllib_softmac.o           \
		  ../../rtllib/rtllib_tx.o                \
		  ../../rtllib/rtllib_wx.o                \
		  ../../rtllib/rtllib_module.o            \
		  ../../rtllib/rtllib_softmac_wx.o        \
		  ../../rtllib/rtl819x_HTProc.o              \
		  ../../rtllib/rtl819x_TSProc.o              \
		  ../../rtllib/rtl819x_BAProc.o              \
		  ../../rtllib/dot11d.o                      \
		  ../../rtllib/rtllib_crypt.o             \
		  ../../rtllib/rtllib_crypt_tkip.o        \
		  ../../rtllib/rtllib_crypt_ccmp.o        \
		  ../../rtllib/rtllib_crypt_wep.o      \
		  ../../mshclass/msh_class.o 		  \
		  ../../mshclass/mesh_8192_util.o  	  \
		  ../../mshclass/rtl8192_mesh.o 	  \
		  ../../mshclass/rtl8192s_sme.o 	  \
	  	  ../../mshclass/rtl8192_proc.o 	  \
		  ../../mshclass/8192s_pathsel.o 	  \
		  ../../mshclass/hash_table.o 		  \
		  ../../mshclass/rtl8192_sha1.o           \
		  ../../mshclass/rtl8192_sha256.o         \
		  ../../mshclass/rtl8192_aes.o            \
		  ../../mshclass/mesh_abbr_handshk.o      \
		  ../../mshclass/rtl8192_bn_ctx.o         \
		  ../../mshclass/rtl8192_bn_asm.o         \
		  ../../mshclass/rtl8192_bn_gcd.o         \
		  ../../mshclass/rtl8192_bn_mont.o        \
		  ../../mshclass/rtl8192_bn_recp.o        \
		  ../../mshclass/rtl8192_bn_lib.o         \
		  ../../mshclass/mesh_ath.o		  \
		  ../../mshclass/pathselection.o 	  \
		  ../../mshclass/tree_proactive.o
		  
obj-m := r8192ce_pci_mesh.o
endif

KSRC := /lib/modules/$(KVER)/build
INSTALL_PREFIX :=
IMG_DIR := /lib/firmware/
all: modules

modules:
	$(MAKE) -C $(KSRC) M=$(PWD) CC=$(CC) modules
install: modules
ifeq ($(NIC_SELECT),RTL8190P)
	find /lib/modules/$(shell uname -r) -name "r8190_*.ko" -exec ls -l {} \;
	find /lib/modules/$(shell uname -r) -name "r8190_*.ko" -exec rm {} \;

	install -p -m 644 r8190_pci.ko $(MODDESTDIR)
endif
ifeq ($(NIC_SELECT),RTL8192E)
	find /lib/modules/$(shell uname -r) -name "r8192e_*.ko" -exec ls -l {} \;
	find /lib/modules/$(shell uname -r) -name "r8192e_*.ko" -exec rm {} \;
	rm -fr /lib/modules/$(shell uname -r)/kernel/ubuntu/rtl8192se

	install -p -m 644 r8192e_pci.ko $(MODDESTDIR)
endif
ifeq ($(NIC_SELECT),RTL8192SE)
	find /lib/modules/$(shell uname -r) -name "r8192se_*.ko" -exec ls -l {} \;
	find /lib/modules/$(shell uname -r) -name "r8192se_*.ko" -exec rm {} \;

	install -p -m 644 r8192se_pci.ko $(MODDESTDIR)
endif
ifeq ($(NIC_SELECT),RTL8192CE)
	find /lib/modules/$(shell uname -r) -name "r8192ce_*.ko" -exec ls -l {} \;
	find /lib/modules/$(shell uname -r) -name "r8192ce_*.ko" -exec rm {} \;

	install -p -m 644 r8192ce_pci.ko $(MODDESTDIR)
endif
ifeq ($(NIC_SELECT),RTL8191SE_MESH)
	find /lib/modules/$(shell uname -r) -name "r8192se_*.ko" -exec ls -l {} \;
	find /lib/modules/$(shell uname -r) -name "r8192se_*.ko" -exec rm {} \;

	mkdir /usr/share/enhanced-easy-network -p
	cp ../../script/mesh-tools /usr/share/enhanced-easy-network/ -rf
	cp ../../script/mesh.conf /etc/
	cp ../../script/wpa_mesh_tkip.conf /etc/
	cp ../../script/wpa_mesh_ccmp.conf /etc/
	chmod 777 ../../script/do_install
	../../script/do_install
	install -p -m 644 r8192se_pci_mesh.ko $(MODDESTDIR)
endif
ifeq ($(NIC_SELECT),RTL8192CE_MESH)
	find /lib/modules/$(shell uname -r) -name "r8192ce_*.ko" -exec ls -l {} \;
	find /lib/modules/$(shell uname -r) -name "r8192ce_*.ko" -exec rm {} \;

	mkdir /usr/share/enhanced-easy-network -p
	cp ../../script/mesh-tools /usr/share/enhanced-easy-network/ -rf
	cp ../../script/mesh.conf /etc/
	cp ../../script/wpa_mesh_tkip.conf /etc/
	cp ../../script/wpa_mesh_ccmp.conf /etc/
	chmod 777 ../../script/do_install
	../../script/do_install
	install -p -m 644 r8192ce_pci_mesh.ko $(MODDESTDIR)
endif
ifeq ($(NIC_SELECT),RTL8192SE_SA)
	find /lib/modules/$(shell uname -r) -name "r8192se_*.ko" -exec ls -l {} \;
	find /lib/modules/$(shell uname -r) -name "r8192se_*.ko" -exec rm {} \;

	install -p -m 644 r8192se_pci_softap.ko $(MODDESTDIR)
endif
ifeq ($(NIC_SELECT),RTL8192CE_BT30)
	find /lib/modules/$(shell uname -r) -name "r8192ce_*.ko" -exec ls -l {} \;
	find /lib/modules/$(shell uname -r) -name "r8192ce_*.ko" -exec rm {} \;

	install -p -m 644 r8192ce_pci.ko $(MODDESTDIR)
endif

	depmod -a
	@#copy firmware img to target fold
	$(shell [ -d "$(IMG_DIR)/`uname -r`" ] && cp -fr $(RTL819x_FIRM_DIR)/$(NIC_SELECT) $(IMG_DIR)/`uname -r`/.)
	$(shell [ ! -d "$(IMG_DIR)/`uname -r`" ] && cp -fr $(RTL819x_FIRM_DIR)/$(NIC_SELECT) $(IMG_DIR)/.)
uninstall:
ifeq ($(NIC_SELECT),RTL8190P)
	$(shell [ -d $(MODDESTDIR) ] && rm -f $(MODDESTDIR)/r8190_pci.ko)
endif
ifeq ($(NIC_SELECT),RTL8192E)
	$(shell [ -d $(MODDESTDIR) ] && rm -f $(MODDESTDIR)/r8192e_pci.ko)
endif
ifeq ($(NIC_SELECT),RTL8192SE)
	$(shell [ -d $(MODDESTDIR) ] && rm -f $(MODDESTDIR)/r8192se_pci.ko)
endif
ifeq ($(NIC_SELECT),RTL8192CE)
	$(shell [ -d $(MODDESTDIR) ] && rm -f $(MODDESTDIR)/r8192ce_pci.ko)
endif
ifeq ($(NIC_SELECT),RTL8191SE_MESH)
	rm -f /etc/wpa_mesh_tkip.conf
	rm -f /etc/wpa_mesh_ccmp.conf
	$(shell [ -d $(MODDESTDIR) ] && rm -f $(MODDESTDIR)/r8192se_pci_mesh.ko)
endif
ifeq ($(NIC_SELECT),RTL8191CE_MESH)
	rm -f /etc/wpa_mesh_tkip.conf
	rm -f /etc/wpa_mesh_ccmp.conf
	$(shell [ -d $(MODDESTDIR) ] && rm -f $(MODDESTDIR)/r8192ce_pci_mesh.ko)
endif
ifeq ($(NIC_SELECT),RTL8192SE_SA)
	$(shell [ -d $(MODDESTDIR) ] && rm -f $(MODDESTDIR)/r8192se_pci_softap.ko)
endif
ifeq ($(NIC_SELECT),RTL8192CE_BT30)
	$(shell [ -d $(MODDESTDIR) ] && rm -f $(MODDESTDIR)/r8192ce_pci.ko)
endif
	depmod -a
	@#delete the firmware img
	@rm -fr /lib/firmware/$(NIC_SELECT)
	@rm -fr /lib/firmware/`uname -r`/$(NIC_SELECT)
else
###################### KERNEL 2.4 ################################
LD := ld
KSRC := /lib/modules/$(KVER)/build
CONFIG_FILE := $(KSRC)/include/linux/autoconf.h
CFLAGS += -DLINUX -D__KERNEL__ -DMODULE -O2 -pipe -Wall
CFLAGS += -I$(KSRC)/include -I.
CFLAGS += -DMODVERSIONS -DEXPORT_SYMTAB -include $(KSRC)/include/linux/modversions.h
#CFLAGS += -D__NO_VERSION__ -DEXPORT_SYMTAB  

ifeq ($(NIC_SELECT),RTL8190P)
sub_folder = rtl8192e
CFLAGS += -DRTL8190P
#CFLAGS +=-DCONFIG_RTLWIFI_DEBUGFS
endif

ifeq ($(NIC_SELECT),RTL8192E)
sub_folder = rtl8192e
CFLAGS += -DRTL8192E
CFLAGS+=-DENABLE_GPIO_RADIO_CTL
#CFLAGS += -DENABLE_IPS
#CFLAGS += -DENABLE_IPS
#CFLAGS +=-DCONFIG_RTLWIFI_DEBUGFS
endif

ifeq ($(NIC_SELECT),RTL8192SE)
sub_folder = rtl8192s
CFLAGS += -DRTL8192SE
CFLAGS += -DRTL8190_Download_Firmware_From_Header=1 
#CFLAGS += -DRTL8192S_WAPI_SUPPORT=1 
#CFLAGS +=-DCONFIG_RTLWIFI_DEBUGFS
#CFLAGS += -DCONFIG_PM_RTL 
#CFLAGS += -DENABLE_GPIO_RADIO_CTL  
#EXTRA_CFLAGS += -DENABLE_IPS
#EXTRA_CFLAGS += -DENABLE_LPS
#EXTRA_CFLAGS += -DCONFIG_ASPM_OR_D3
#CFLAGS += -DENABLE_AMSDU
#CFLAGS += -DADHOC_11N
endif

ifeq ($(NIC_SELECT),RTL8191SE_MESH)
sub_folder = rtl8192s
CFLAGS += -DRTL8192SE
#CFLAGS +=-DCONFIG_RTLWIFI_DEBUGFS
CFLAGS += -DCONFIG_PM_RTL 
CFLAGS += -DENABLE_GPIO_RADIO_CTL  
CFLAGS += -DRTL8190_Download_Firmware_From_Header=1 
CFLAGS += -DENABLE_IPS
CFLAGS += -DENABLE_LPS
CFLAGS += -DCONFIG_ASPM_OR_D3
CFLAGS += -D_RTL8192_EXT_PATCH_ \
	  -DMESH_8192_SPECIFIED \
	  -D_MESH_DEBUG_ \
	  -DTREE_BASED_ROUTING_ENABLE \
	  -DMESH_PATHSEL_DBG_LV=0 \
	  -DMESH_PATHSEL_DBG_SIMPLE=0 \
	  -DMESH_PATHSEL_DBG_TEST=0\
	  -DBUILT_IN_MSHCLASS \
	  -DCUSTOMER_ID_INTEL_CMPC \
	  -DENABLE_AMSDU
endif

ifeq ($(NIC_SELECT),RTL8191CE_MESH)
sub_folder = rtl8192c
CFLAGS += -DRTL8192CE
#CFLAGS +=-DCONFIG_RTLWIFI_DEBUGFS
CFLAGS += -DCONFIG_PM_RTL 
CFLAGS += -DENABLE_GPIO_RADIO_CTL  
CFLAGS += -DRTL8190_Download_Firmware_From_Header=1 
CFLAGS += -DENABLE_IPS
CFLAGS += -DENABLE_LPS
CFLAGS += -DCONFIG_ASPM_OR_D3
CFLAGS += -D_RTL8192_EXT_PATCH_ \
	  -DMESH_8192_SPECIFIED \
	  -D_MESH_DEBUG_ \
	  -DTREE_BASED_ROUTING_ENABLE \
	  -DMESH_PATHSEL_DBG_LV=0 \
	  -DMESH_PATHSEL_DBG_SIMPLE=0 \
	  -DMESH_PATHSEL_DBG_TEST=0\
	  -DBUILT_IN_MSHCLASS \
	  -DCUSTOMER_ID_INTEL_CMPC \
	  -DENABLE_AMSDU
endif
ifeq ($(NIC_SELECT),RTL8192SE_SA)
sub_folder = rtl8192s
CFLAGS += -DRTL8192SE
CFLAGS += -DRTL8190_Download_Firmware_From_Header=1 
CFLAGS += -DASL 
#CFLAGS += -DRTL8192S_WAPI_SUPPORT=1 
#CFLAGS +=-DCONFIG_RTLWIFI_DEBUGFS
#CFLAGS += -DCONFIG_PM_RTL 
#CFLAGS += -DENABLE_GPIO_RADIO_CTL  
#EXTRA_CFLAGS += -DENABLE_IPS
#EXTRA_CFLAGS += -DENABLE_LPS
#EXTRA_CFLAGS += -DCONFIG_ASPM_OR_D3
#CFLAGS += -DENABLE_AMSDU
#CFLAGS += -DADHOC_11N
endif

CFLAGS += -DTHOMAS_TURBO
CFLAGS += -DENABLE_DOT11D

#flag for single module compile
CFLAGS += -DBUILT_IN_RTLLIB
CFLAGS += -DBUILT_IN_CRYPTO
#CFLAGS += -DENABLE_TKIP11N

SMP := $(shell $(CC) $(CFLAGS) -E -dM $(CONFIG_FILE) | \
   grep CONFIG_SMP | awk '{print $$3}')
ifneq ($(SMP),1)
   SMP := 0
endif
ifeq ($(SMP),1)
   CFLAGS    += -D__SMP__
endif
OBJS := ${patsubst %.c, %.o, ${wildcard *.c}}

ifeq ($(NIC_SELECT),RTL8190P)
all:r8190_pci.o
r8190_pci.o: \
	rtl_core.o      \
	rtl_eeprom.o     \
	rtl_ps.o        \
	rtl_dm.o \
	rtl_wx.o \
	rtl_cam.o	 \
	rtl_pm.o \
	rtl_pci.o 		\
	rtl_debug.o \
	rtl_ethtool.o         \
	rtl8192e/r8190P_rtl8256.o \
	rtl8192e/r8190P_hwimg.o \
	rtl8192e/r8192E_dev.o	\
	rtl8192e/r8192E_cmdpkt.o\
	rtl8192e/r8192E_firmware.o \
	rtl8192e/r8192E_phy.o \
	../../rtllib/rtllib_rx.o                \
	../../rtllib/rtllib_softmac.o           \
	../../rtllib/rtllib_tx.o                \
	../../rtllib/rtllib_wx.o                \
	../../rtllib/rtllib_module.o            \
	../../rtllib/rtllib_softmac_wx.o        \
	../../rtllib/rtl819x_HTProc.o              \
	../../rtllib/rtl819x_TSProc.o              \
	../../rtllib/rtl819x_BAProc.o              \
	../../rtllib/dot11d.o                      \
	../../rtllib/rtllib_crypt.o             \
	../../rtllib/rtllib_crypt_tkip.o        \
	../../rtllib/rtllib_crypt_ccmp.o        \
	../../rtllib/rtllib_crypt_wep.o	\
	../../rtllib/api.o			\
	../../rtllib/compress.o		\
	../../rtllib/digest.o		\
	../../rtllib/cipher.o		\
	../../rtllib/scatterwalk.o		\
	../../rtllib/arc4.o			\
	../../rtllib/michael_mic.o		\
	../../rtllib/aes.o			\
	../../rtllib/proc.o

	$(LD) -r $^ -o $@
install:r8190_pci.o
	install -p -m 644 r8190_pci.o  $(MODDESTDIR)
	@depmod -a
uninstall:
	$(shell [ -d $(MODDESTDIR) ] && rm -f $(MODDESTDIR)/r8190_pci.o)
	@depmod -a
endif

ifeq ($(NIC_SELECT),RTL8192E)
all:r8192e_pci.o
r8192e_pci.o: \
	rtl_core.o \
	rtl_eeprom.o \
	rtl_ps.o 		\
	rtl_dm.o \
	rtl_wx.o \
	rtl_cam.o	\
	rtl_pm.o \
	rtl_pci.o 		\
	rtl_debug.o \
	rtl_ethtool.o         \
	rtl8192e/r8192E_dev.o           \
	rtl8192e/r8190P_rtl8256.o \
	rtl8192e/r8192E_hwimg.o \
	rtl8192e/r8192E_cmdpkt.o\
	rtl8192e/r8192E_firmware.o \
	rtl8192e/r8192E_phy.o \
	../../rtllib/rtllib_rx.o                \
	../../rtllib/rtllib_softmac.o           \
	../../rtllib/rtllib_tx.o                \
	../../rtllib/rtllib_wx.o                \
	../../rtllib/rtllib_module.o            \
	../../rtllib/rtllib_softmac_wx.o        \
	../../rtllib/rtl819x_HTProc.o              \
	../../rtllib/rtl819x_TSProc.o              \
	../../rtllib/rtl819x_BAProc.o              \
	../../rtllib/dot11d.o                      \
	../../rtllib/rtllib_crypt.o             \
	../../rtllib/rtllib_crypt_tkip.o        \
	../../rtllib/rtllib_crypt_ccmp.o        \
	../../rtllib/rtllib_crypt_wep.o	\
	../../rtllib/api.o			\
	../../rtllib/compress.o		\
	../../rtllib/digest.o		\
	../../rtllib/cipher.o		\
	../../rtllib/scatterwalk.o		\
	../../rtllib/arc4.o			\
	../../rtllib/michael_mic.o		\
	../../rtllib/aes.o			\
	../../rtllib/proc.o

	$(LD) -r $^ -o $@
install:
	install -p -m 644 r8192e_pci.o  $(MODDESTDIR)
	@depmod -a
uninstall:
	$(shell [ -d $(MODDESTDIR) ] && rm -f $(MODDESTDIR)/r8192e_pci.o)
	@depmod -a
endif

ifeq ($(NIC_SELECT),RTL8192SE)
all:r8192se_pci.o
r8192se_pci.o: rtl_core.o \
	rtl_debug.o \
	rtl_ethtool.o \
	rtl_eeprom.o \
 	rtl_ps.o        \
	rtl_dm.o \
	rtl_wx.o \
	rtl_cam.o		\
	rtl_pm.o \
	rtl_pci.o 		\
	rtl8192s/r8192S_dev.o \
	rtl8192s/r8192S_Efuse.o \
	rtl8192s/r8192S_firmware.o \
	rtl8192s/r8192S_hwimg.o \
	rtl8192s/r8192S_led.o \
	rtl8192s/r8192S_phy.o \
	rtl8192s/r8192S_rtl6052.o \
	../../rtllib/rtllib_rx.o                \
	../../rtllib/rtllib_softmac.o           \
	../../rtllib/rtllib_tx.o                \
	../../rtllib/rtllib_wx.o                \
	../../rtllib/rtllib_module.o            \
	../../rtllib/rtllib_softmac_wx.o        \
	../../rtllib/rtl819x_HTProc.o              \
	../../rtllib/rtl819x_TSProc.o              \
	../../rtllib/rtl819x_BAProc.o              \
	../../rtllib/dot11d.o                      \
	../../rtllib/rtllib_crypt.o             \
	../../rtllib/rtllib_crypt_tkip.o        \
	../../rtllib/rtllib_crypt_ccmp.o        \
	../../rtllib/rtllib_crypt_wep.o	\
	../../rtllib/api.o			\
	../../rtllib/compress.o		\
	../../rtllib/digest.o		\
	../../rtllib/cipher.o		\
	../../rtllib/scatterwalk.o		\
	../../rtllib/arc4.o			\
	../../rtllib/michael_mic.o		\
	../../rtllib/aes.o			\
	../../rtllib/proc.o			\
	../../rtllib/wapi.o			\
	../../rtllib/wapi_interface.o
#	rtl8192e/r8192E_cmdpkt.o 
#	rtl8192e/r8190P_rtl8256.o 
	$(LD) -r $^ -o $@
install:r8192se_pci.o
	install -p -m 644 r8192se_pci.o  $(MODDESTDIR)
	@depmod -a
uninstall:
	$(shell [ -d $(MODDESTDIR) ] && rm -f $(MODDESTDIR)/r8192se_pci.o)
	@depmod -a
endif

ifeq ($(NIC_SELECT),RTL8191SE_MESH )
all:r8192se_pci_mesh.o
r8192se_pci_mesh.o: 		\
	rtl_core.o 		\
	rtl_eeprom.o 		\
	rtl_ps.o 		\
	rtl_dm.o 		\
	rtl_wx.o 		\
	rtl_cam.o		\
	rtl_pm.o 		\
	rtl_pci.o 		\
	rtl_debug.o 		\
	rtl_ethtool.o 		\
	rtl_mesh.o		\
	rtl8192s/r8192S_dev.o 		\
	rtl8192s/r8192S_Efuse.o 	\
	rtl8192s/r8192S_firmware.o 	\
	rtl8192s/r8192S_hwimg.o 	\
	rtl8192s/r8192S_led.o 		\
	rtl8192s/r8192S_phy.o 		\
	rtl8192s/r8192S_rtl6052.o 	\
	../../rtllib/rtllib_rx.o                \
	../../rtllib/rtllib_softmac.o           \
	../../rtllib/rtllib_tx.o                \
	../../rtllib/rtllib_wx.o                \
	../../rtllib/rtllib_module.o            \
	../../rtllib/rtllib_softmac_wx.o        \
	../../rtllib/rtl819x_HTProc.o              \
	../../rtllib/rtl819x_TSProc.o              \
	../../rtllib/rtl819x_BAProc.o              \
	../../rtllib/dot11d.o                      \
	../../rtllib/rtllib_crypt.o             \
	../../rtllib/rtllib_crypt_tkip.o        \
	../../rtllib/rtllib_crypt_ccmp.o        \
	../../rtllib/rtllib_crypt_wep.o		\
	../../mshclass/msh_class.o 			\
	../../mshclass/mesh_8192_util.o  		\
	../../mshclass/rtl8192_mesh.o 			\
	../../mshclass/rtl8192s_sme.o 			\
	../../mshclass/rtl8192_proc.o 			\
	../../mshclass/8192s_pathsel.o 			\
	../../mshclass/hash_table.o 			\
	../../mshclass/pathselection.o 			\
	../../mshclass/tree_proactive.o			\
        ../../mshclass/hash_table.o                   \
        ../../mshclass/sha1.o                         \
        ../../mshclass/sha256.o                       \
        ../../mshclass/aes.o                          \
        ../../mshclass/mesh_abbr_handshk.o            \
        ../../mshclass/bn_ctx.o                       \
        ../../mshclass/bn_asm.o                       \
        ../../mshclass/bn_gcd.o                       \
        ../../mshclass/bn_mont.o                      \
        ../../mshclass/bn_recp.o                      \
        ../../mshclass/bn_lib.o                       \
        ../../mshclass/mesh_ath.o
#	rtl8192e/r8190P_rtl8256.o \	
#	rtl8192e/r8192E_cmdpkt.o 
	$(LD) -r $^ -o $@
install:r8192se_pci_mesh.o
	install -p -m 644 r8192se_pci_mesh.o  $(MODDESTDIR)
	@depmod -a
uninstall:
	$(shell [ -d $(MODDESTDIR) ] && rm -f $(MODDESTDIR)/r8192se_pci_mesh.o)
	@depmod -a
endif

ifeq ($(NIC_SELECT),RTL8191CE_MESH )
all:r8192ce_pci_mesh.o
r8192ce_pci_mesh.o:             \
        rtl_core.o              \
        rtl_eeprom.o          \
        rtl_wx.o              \
        rtl_cam.o             \
        rtl_pm.o              \
        rtl_pci.o             \
        rtl_ps.o              \
        rtl_debug.o           \
        rtl_ethtool.o         \
        rtl_mesh.o            \
        rtl8192c/r8192C_dev.o         \
        rtl8192c/r8192C_tx.o          \
        rtl8192c/r8192C_rx.o          \
        rtl8192c/r8192C_Efuse.o       \
        rtl8192c/r8192C_phy.o         \
        rtl8192c/r8192C_firmware.o    \
        rtl8192c/r8192C_dmbt.o        \
        rtl8192c/r8192C_dmout.o       \
        rtl8192c/r8192C_dm.o          \
        rtl8192c/r8192C_rtl6052.o     \
        rtl8192c/r8192C_hwimg.o       \
        rtl8192c/r8192C_led.o         \
        rtl8192c/r8192C_com.o         \
        rtl8192c/r8192C_PhyParam.o    \
        ../../rtllib/rtllib_rx.o                \
        ../../rtllib/rtllib_softmac.o           \
        ../../rtllib/rtllib_tx.o                \
        ../../rtllib/rtllib_wx.o                \
        ../../rtllib/rtllib_module.o            \
        ../../rtllib/rtllib_softmac_wx.o        \
        ../../rtllib/rtl819x_HTProc.o              \
        ../../rtllib/rtl819x_TSProc.o              \
        ../../rtllib/rtl819x_BAProc.o              \
        ../../rtllib/dot11d.o                      \
        ../../rtllib/rtllib_crypt.o             \
        ../../rtllib/rtllib_crypt_tkip.o        \
        ../../rtllib/rtllib_crypt_ccmp.o        \
        ../../rtllib/rtllib_crypt_wep.o      \
        ../../mshclass/msh_class.o              \
        ../../mshclass/mesh_8192_util.o         \
        ../../mshclass/rtl8192_mesh.o           \
        ../../mshclass/rtl8192s_sme.o           \
        ../../mshclass/rtl8192_proc.o           \
        ../../mshclass/8192s_pathsel.o          \
        ../../mshclass/hash_table.o             \
        ../../mshclass/rtl8192_sha1.o           \
        ../../mshclass/rtl8192_sha256.o         \
        ../../mshclass/rtl8192_aes.o            \
        ../../mshclass/mesh_abbr_handshk.o      \
        
        ../../mshclass/rtl8192_bn_asm.o         \
        ../../mshclass/rtl8192_bn_gcd.o         \
        ../../mshclass/rtl8192_bn_mont.o        \
        ../../mshclass/rtl8192_bn_recp.o        \
        ../../mshclass/rtl8192_bn_lib.o         \
        ../../mshclass/mesh_ath.o               \
        ../../mshclass/pathselection.o          \
        ../../mshclass/tree_proactive.o
	$(LD) -r $^ -o $@
install:r8192ce_pci_mesh.o
        install -p -m 644 r8192ce_pci_mesh.o  $(MODDESTDIR)
        @depmod -a
uninstall:
        $(shell [ -d $(MODDESTDIR) ] && rm -f $(MODDESTDIR)/r8192ce_pci_mesh.o)
        @depmod -a
endif
ifeq ($(NIC_SELECT),RTL8192SE_SA)
all:r8192se_pci_softap.o
r8192se_pci.o: rtl_core.o \
	rtl_debug.o \
	rtl_ethtool.o \
	rtl_eeprom.o \
 	rtl_ps.o        \
	rtl_dm.o \
	rtl_wx.o \
	rtl_cam.o		\
	rtl_pm.o \
	rtl_pci.o 		\
	rtl_softap.o		\
	rtl8192s/r8192S_dev.o \
	rtl8192s/r8192S_Efuse.o \
	rtl8192s/r8192S_firmware.o \
	rtl8192s/r8192S_hwimg.o \
	rtl8192s/r8192S_led.o \
	rtl8192s/r8192S_phy.o \
	rtl8192s/r8192S_rtl6052.o \
	../../rtllib/rtllib_rx.o                \
	../../rtllib/rtllib_softmac.o           \
	../../rtllib/rtllib_tx.o                \
	../../rtllib/rtllib_wx.o                \
	../../rtllib/rtllib_module.o            \
	../../rtllib/rtllib_softmac_wx.o        \
	../../rtllib/rtl819x_HTProc.o              \
	../../rtllib/rtl819x_TSProc.o              \
	../../rtllib/rtl819x_BAProc.o              \
	../../rtllib/dot11d.o                      \
	../../rtllib/rtllib_crypt.o             \
	../../rtllib/rtllib_crypt_tkip.o        \
	../../rtllib/rtllib_crypt_ccmp.o        \
	../../rtllib/rtllib_crypt_wep.o	\
	../../rtllib/api.o			\
	../../rtllib/compress.o		\
	../../rtllib/digest.o		\
	../../rtllib/cipher.o		\
	../../rtllib/scatterwalk.o		\
	../../rtllib/arc4.o			\
	../../rtllib/michael_mic.o		\
	../../rtllib/aes.o			\
	../../rtllib/proc.o			\
	../../rtllib/wapi.o			\
	../../rtllib/wapi_interface.o
#	rtl8192e/r8192E_cmdpkt.o 
#	rtl8192e/r8190P_rtl8256.o 
	$(LD) -r $^ -o $@
install:r8192se_pci_softap.o
	install -p -m 644 r8192se_pci_softap.o  $(MODDESTDIR)
	@depmod -a
uninstall:
	$(shell [ -d $(MODDESTDIR) ] && rm -f $(MODDESTDIR)/r8192se_pci_softap.o)
	@depmod -a
endif
endif

.PHONY:clean
clean:
	rm -fr *.mod.c *.mod *.o .*.cmd *.ko *~
	rm -fr .tmp_versions
	rm -fr Modules.symvers
	rm -fr Module.symvers
	rm -fr Module.markers
	rm -fr modules.order
	rm -fr tags
	@make -C $(sub_folder)/ clean
