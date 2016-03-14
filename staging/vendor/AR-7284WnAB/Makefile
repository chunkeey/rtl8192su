#
# drivers/net/rtl8190
#
# Makefile for the Linux Wireless network device drivers.
#

EXTRA_CFLAGS += -I$(TOPDIR)/drivers/net/wireless
EXTRA_CFLAGS += -std=gnu89
EXTRA_CFLAGS += -O2
#EXTRA_CFLAGS += -mhard-float -DCONFIG_FORCE_HARD_FLOAT=y
#EXTRA_CFLAGS += -DASL=1
#EXTRA_CFLAGS += -DASL_WME=1
#EXTRA_CFLAGS += -DASL_DEBUG=1
#CFLAGS += -DJACKSON_NEW_8187 -DJACKSON_NEW_RX
#CFLAGS += -DTHOMAS_BEACON -DTHOMAS_TASKLET -DTHOMAS_SKB
CFLAGS += -include $(dir $@)8672port.h
#CFLAGS += -DCONFIG_RTL8192
CFLAGS += -DCONFIG_RTL8671
#CFLAGS += -DRTL8192SE
CFLAGS += -DRTL8192SU
CFLAGS += -DCONFIG_RTL8192SU 
#CFLAGS += -mips16





obj-$(CONFIG_USB_RTL8192SU_SOFTAP) = 8192su.o 8190n_usb.o

#EXTRA_CFLGAS += -Werror

8192su-y := 8190n_tx.o\
	8190n_rx.o\
	8190n_osdep.o\
	8190n_sme.o\
	8190n_util.o\
	8190n_hw.o\
	8190n_ioctl.o\
	8190n_security.o\
	8190n_tkip.o\
	8190n_aes.o\
	8190n_proc.o\
	8190n_br_ext.o\
	8190n_eeprom.o\
	8190n_mp.o\
	8190n_psk.o\
	1x_kmsm_aes.o\
	1x_kmsm_hmac.o\
	1x_md5c.o\
	1x_rc4.o\
	8190n_mib.o\
	8190n_dmem.o\
	romeperf.o\
	rtl_misc.o\
	8672port.o
	
#include $(TOPDIR)/Rules.make
