NIC_SELECT = RTL8192SE
LINUX_KSRC_MODULE = /lib/modules/$(shell uname -r)/kernel/drivers/net/wireless/
RTL819x_DIR = $(shell pwd)
KVER  = $(shell uname -r)
KSRC = /lib/modules/$(KVER)/build
RTL819x_FIRM_DIR = $(RTL819x_DIR)/firmware
HAL_SUB_DIR = rtl8192
MODULE_FILE = $(RTL819x_DIR)/rtllib/Module.symvers
SCRIPT_FOLDER = /etc/realtek

export LINUX_KSRC_MODULE_DIR RTL819x_FIRM_DIR

all: 
ifeq ($(shell uname -r|cut -d. -f1,2), 2.4)
#	@make -C $(RTL819x_DIR)/rtllib
	@make -C $(RTL819x_DIR)/HAL/$(HAL_SUB_DIR) 
	@make -C $(RTL819x_DIR)/wapi_supplicant
else	
#	@make -C $(KSRC) SUBDIRS=$(RTL819x_DIR)/rtllib modules 
#	@test -f $(MODULE_FILE) && cp $(MODULE_FILE) $(RTL819x_DIR)/HAL/$(HAL_SUB_DIR) || echo > /dev/null 
ifeq ($(NIC_SELECT), RTL8192SE_SA)
	@make -C $(RTL819x_DIR)/hostapd-0.6.10/hostapd
endif
	@make -C $(KSRC) SUBDIRS=$(RTL819x_DIR)/HAL/$(HAL_SUB_DIR) modules 
#	@make -C $(RTL819x_DIR)/wapi_supplicant
endif
install: all
	@rm -f /etc/acpi/wireless-rtl-ac-dc-power.sh
	@rm -f /etc/acpi/events/RadioPower.sh
	@cp -fr realtek/ /etc
	@make -C HAL/$(HAL_SUB_DIR)/ install
#	@cp $(RTL819x_DIR)/wapi_supplicant/wapi_supplicant /usr/bin/
uninstall:
	@make -C HAL/$(HAL_SUB_DIR)/ uninstall
	@rm -fr $(SCRIPT_FOLDER)
	@rm -f /usr/bin/wapi_supplicant
clean:
	@make -C HAL/$(HAL_SUB_DIR)/ clean
	@make -C rtllib/ clean
	@test -r mshclass/ && make -C mshclass/ clean|| echo > /dev/null 
	@test -r wapi_supplicant/ && make -C wapi_supplicant/ clean|| echo > /dev/null 
	@rm -fr Modules.symvers
	@rm -fr Module.symvers
	@rm -fr Module.markers
	@rm -fr modules.order
	@rm -fr .tmp_versions
	@rm -fr *~
	@rm -f tags
ifeq ($(NIC_SELECT), RTL8192SE_SA)
	@make -C $(RTL819x_DIR)/hostapd-0.6.10/hostapd clean
endif
ifeq ($(NIC_SELECT), RTL8192CE_BT30)
	@make -C btlib clean
endif
