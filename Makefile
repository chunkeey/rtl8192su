# fallback to the current kernel source
KSRC ?= /lib/modules/$(shell uname -r)/build

KMOD_SRC ?= $(CURDIR)/rtlwifi

KMOD_OPTIONS = CONFIG_RTL_CARDS=y
KMOD_OPTIONS += CONFIG_RTLWIFI=m
KMOD_OPTIONS += CONFIG_RTLWIFI_DEBUG=y
KMOD_OPTIONS += CONFIG_RTLWIFI_USB=m
KMOD_OPTIONS += CONFIG_RTLWIFI_PCI=m
KMOD_OPTIONS += CONFIG_RTL8192SU=m
KMOD_OPTIONS += CONFIG_RTL8192SU_DEBUGFS=m
KMOD_OPTIONS += CONFIG_RTL8192SE=m
KMOD_OPTIONS += CONFIG_RTL8192S_COMMON=m

# Don't build any of the other drivers
KMOD_OPTIONS += CONFIG_RTL8192CU=n CONFIG_RTL8192DE=n CONFIG_RTL8192CE=n CONFIG_RTL8192C_COMMON=n CONFIG_RTL8723AE=n CONFIG_RTL8188EE=n

EXTRA_CFLAGS += -DDEBUG

all:
	$(MAKE) -C $(KSRC) M=$(KMOD_SRC) $(KMOD_OPTIONS) $(MAKECMDGOALS) EXTRA_CFLAGS="$(EXTRA_CFLAGS)"

.PHONY: all clean load unload reload test

clean:
	$(MAKE) -C $(KSRC) M=$(KMOD_SRC) clean $(KMOD_OPTIONS)

load:
	modprobe mac80211
	insmod $(KMOD_SRC)/rtlwifi.ko
	insmod $(KMOD_SRC)/rtl_usb.ko
	insmod $(KMOD_SRC)/rtl8192s/rtl8192s-common.ko
	insmod $(KMOD_SRC)/rtl8192su/rtl8192su.ko

unload:
	rmmod rtl8192su
	rmmod rtl8192s-common
	rmmod rtl_usb
	rmmod rtlwifi

reload: unload load

test: all reload
