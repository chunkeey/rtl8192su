NIC_SELECT = RTL8192SE

KVER  := $(shell uname -r)
MODDESTDIR := /lib/modules/$(KVER)/kernel/drivers/net/wireless/$(NIC_SELECT)

CC = gcc
ifneq ($(shell uname -r|cut -d. -f1,2), 2.4)
EXTRA_CFLAGS += -I$(TOPDIR)/drivers/net/wireless
EXTRA_CFLAGS += -O2

#it will fail to compile in suse linux enterprise 10 sp2. This flag is to solve this problem.
ifeq ($(shell uname -r | cut -d. -f1,2,3,4), 2.6.16.60-0)
EXTRA_CFLAGS += -DOPENSUSE_SLED
endif
ifeq ($(NIC_SELECT), RTL8192SE)
	EXTRA_CFLAGS += -DRTL8192S_DISABLE_FW_DM=0
	EXTRA_CFLAGS += -DRTL8192SE
	EXTRA_CFLAGS += -DENABLE_IPS
	EXTRA_CFLAGS += -DENABLE_LPS
endif
ifeq ($(NIC_SELECT), RTL8192SU)
	EXTRA_CFLAGS += -DRTL8192S_DISABLE_FW_DM=0
	EXTRA_CFLAGS += -DRTL8192SU
	EXTRA_CFLAGS += -DENABLE_IPS
	EXTRA_CFLAGS += -DENABLE_LPS
	EXTRA_CFLAGS += -DUSB_RX_AGGREGATION_SUPPORT
	EXTRA_CFLAGS += -DTCP_CSUM_OFFLOAD_RX
endif
ifeq ($(NIC_SELECT), RTL8190P)
	EXTRA_CFLAGS += -DRTL8190P
endif
ifeq ($(NIC_SELECT), RTL8192E)
	EXTRA_CFLAGS += -DRTL8192E
        EXTRA_CFLAGS += -DENABLE_IPS
        EXTRA_CFLAGS += -DENABLE_LPS
endif
ifeq ($(NIC_SELECT),RTL8192U)
EXTRA_CFLAGS += -DRTL8192U
#EXTRA_CFLAGS += -DENABLE_AMSDU
EXTRA_CFLAGS += -DUSB_TX_DRIVER_AGGREGATION_ENABLE
EXTRA_CFLAGS += -DUSB_RX_AGGREGATION_SUPPORT

# For RTK DMP platform
#EXTRA_CFLAGS += -DRTK_DMP_PLATFORM
#EXTRA_CFLAGS += -DWIFI_TEST

endif
ifeq ($(NIC_SELECT), RTL8191SE_MESH)
AUTHTYPE=SAE
#AUTHTYPE=OPEN
EXTRA_CFLAGS += -DRTL8192SE \
		-DRTL8192S_DISABLE_FW_DM=0 \
		-D_RTL8192_EXT_PATCH_\
		-DENABLE_IPS \
		-DENABLE_LPS \
		-DCUSTOMER_ID_INTEL_CMPC
ifeq ($(AUTHTYPE),SAE)
EXTRA_CFLAGS += -DMESH_ABBR_HANDSHK_SUPPORT\
		-DMESH_ATH_2
endif
endif
#EXTRA_CFLAGS += -DJOHN_NOCPY
EXTRA_CFLAGS += -DTHOMAS_TURBO
#flags to enable or disble 80211D feature
EXTRA_CFLAGS += -DENABLE_DOT11D
rtllib-rsl-objs := rtllib_rx.o \
		      rtllib_softmac.o \
		      rtllib_tx.o \
		      rtllib_wx.o \
		      rtllib_module.o \
		      rtllib_softmac_wx.o\
		      rtl819x_HTProc.o\
		      rtl819x_TSProc.o\
		      rtl819x_BAProc.o\
		      dot11d.o

rtllib_crypt-rsl-objs := rtllib_crypt.o
rtllib_crypt_tkip-rsl-objs := rtllib_crypt_tkip.o
rtllib_crypt_ccmp-rsl-objs := rtllib_crypt_ccmp.o
rtllib_crypt_wep-rsl-objs := rtllib_crypt_wep.o

obj-m +=rtllib-rsl.o
obj-m +=rtllib_crypt-rsl.o
obj-m +=rtllib_crypt_wep-rsl.o
obj-m +=rtllib_crypt_tkip-rsl.o
obj-m +=rtllib_crypt_ccmp-rsl.o

KSRC := /lib/modules/$(KVER)/build
INSTALL_PREFIX :=

all: modules

modules:
	$(MAKE) -C $(KSRC) M=$(PWD) CC=$(CC) modules

install: modules
	rm -fr $(MODDESTDIR)
	mkdir -p $(MODDESTDIR)
	@install -p -m 644 rtllib_crypt-rsl.ko $(MODDESTDIR)
	@install -p -m 644 rtllib_crypt_wep-rsl.ko $(MODDESTDIR)
	@install -p -m 644 rtllib_crypt_tkip-rsl.ko $(MODDESTDIR)
	@install -p -m 644 rtllib_crypt_ccmp-rsl.ko $(MODDESTDIR)
	@install -p -m 644 rtllib-rsl.ko $(MODDESTDIR)
	depmod -a
uninstall:
	rm -fr $(MODDESTDIR)
	depmod -a 

else
LD := ld
KSRC := /lib/modules/$(KVER)/build
CONFIG_FILE := $(KSRC)/include/linux/autoconf.h

CFLAGS += -DLINUX -D__KERNEL__ -DMODULE -O2 -pipe -Wall
CFLAGS += -I$(KSRC)/include -I.
#Kernel 2.4.31
CFLAGS += -DMODVERSIONS -DEXPORT_SYMTAB -include $(KSRC)/include/linux/modversions.h
#Kernel 2.4.20
#CFLAGS += -D__NO_VERSION__ -DEXPORT_SYMTAB 
CFLAGS += -DENABLE_DOT11D
CFLAGS += -DTHOMAS_TURBO

ifeq ($(NIC_SELECT),RTL8192U)
CFLAGS += -DRTL8192U
#EXTRA_CFLAGS += -DENABLE_AMSDU
EXTRA_CFLAGS += -DUSB_TX_DRIVER_AGGREGATION_ENABLE
EXTRA_CFLAGS += -DUSB_RX_AGGREGATION_SUPPORT
endif
ifeq ($(NIC_SELECT),RTL8192E)
CFLAGS += -DRTL8192E
endif
ifeq ($(NIC_SELECT),RTL8190P)
CFLAGS += -DRTL8190P
endif
ifeq ($(NIC_SELECT),RTL8192SE)
CFLAGS += -DRTL8192SE
CFLAGS += -DRTL8192S_DISABLE_FW_DM=0
endif
ifeq ($(NIC_SELECT), RTL8191SE_MESH)
AUTHTYPE=SAE
#AUTHTYPE=OPEN
CFLAGS += -DRTL8192SE
CFLAGS += -DRTL8192S_DISABLE_FW_DM=0
CFLAGS += -D_RTL8192_EXT_PATCH_
ifeq ($(AUTHTYPE),SAE)
CFLAGS += -DMESH_ABBR_HANDSHK_SUPPORT\
CFLAGS += -DMESH_ATH_2
endif
endif

SMP := $(shell $(CC) $(MODCFLAGS) -E -dM $(CONFIG_FILE) | \
   grep CONFIG_SMP | awk '{print $$3}')
ifneq ($(SMP),1)
   SMP := 0
endif
ifeq ($(SMP),1)
   CFLAGS    += -D__SMP__
endif

#CFLAGS += -DJOHN_NOCPY

OBJS := ${patsubst %.c, %.o, ${wildcard *.c}}
all:${OBJS} rtllib_crypt-rsl.o michael_mic-rsl.o aes-rsl.o rtllib_crypt_wep-rsl.o rtllib_crypt_tkip-rsl.o rtllib_crypt_ccmp-rsl.o crypto-rsl.o rtllib-rsl.o

rtllib_crypt-rsl.o: rtllib_crypt.o
	mv $^ $@	

michael_mic-rsl.o: michael_mic.o
	mv $^ $@

aes-rsl.o: aes.o
	mv $^ $@

rtllib_crypt_wep-rsl.o: rtllib_crypt_wep.o
	mv $^ $@

rtllib_crypt_tkip-rsl.o: rtllib_crypt_tkip.o
	mv $^ $@

rtllib_crypt_ccmp-rsl.o: rtllib_crypt_ccmp.o
	mv $^ $@

crypto-rsl.o: arc4.o api.o autoload.o cipher.o compress.o digest.o scatterwalk.o proc.o
	$(LD) -r $^ -o $@

rtllib-rsl.o: rtllib_rx.o rtllib_tx.o rtllib_wx.o rtllib_module.o rtllib_softmac_wx.o rtllib_softmac.o rtl819x_HTProc.o rtl819x_TSProc.o rtl819x_BAProc.o dot11d.o
	$(LD) -r $^ -o $@
install:
	rm -fr $(MODDESTDIR)
	mkdir -p $(MODDESTDIR)
	@install -p -m 644 rtllib_crypt-rsl.o $(MODDESTDIR)
	@install -p -m 644 crypto-rsl.o $(MODDESTDIR)
	@install -p -m 644 michael_mic-rsl.o $(MODDESTDIR)
	@install -p -m 644 aes-rsl.o $(MODDESTDIR)
	@install -p -m 644 rtllib_crypt_wep-rsl.o $(MODDESTDIR)
	@install -p -m 644 rtllib_crypt_tkip-rsl.o $(MODDESTDIR)
	@install -p -m 644 rtllib_crypt_ccmp-rsl.o $(MODDESTDIR)
	@install -p -m 644 rtllib-rsl.o $(MODDESTDIR)
	/sbin/depmod -a ${shell uname -r}

uninstall:
	rm -fr $(MODDESTDIR)
	/sbin/depmod -a ${shell uname -r}

endif

.PHONY: clean
clean:
	rm -fr *.mod.c *.mod *.o .*.cmd *.mod.* *.ko *.o *~
	rm -rf .tmp_versions
	rm -rf Module.symvers
	rm -fr Module.markers
	rm -fr modules.order
	rm -fr tags
