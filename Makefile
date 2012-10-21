# fallback to the current kernel source
KSRC ?= /lib/modules/$(shell uname -r)/build

KMOD_SRC ?= $(PWD)/rtlwifi

# Each configuration option enables a list of files.

C ?= 0
CF ?= -D__CHECK_ENDIAN__ 
KMOD_OPTIONS = CONFIG_RTLWIFI=m 
KMOD_OPTIONS += CONFIG_RTLWIFI_DEBUG=m 
KMOD_OPTIONS += CONFIG_RTL8192SU=m

# Don't build any of the other drivers
KMOD_OPTIONS += CONFIG_RTL8192CU=n CONFIG_RTL8192DE=n CONFIG_RTL8192CE=n CONFIG_RTL8192SE=n CONFIG_RTL8192C_COMMON=n


EXTRA_CFLAGS += -DDEBUG

all:
	$(MAKE) -C $(KSRC) M=$(KMOD_SRC) $(KMOD_OPTIONS) C=$(C) CF=$(CF) -j4

clean:
	$(MAKE) -C $(KSRC) M=$(KMOD_SRC) clean $(KMOD_OPTIONS)

#debug trace load
load:	all
	modprobe mac80211
	insmod $(KMOD_SRC)/rtlwifi.ko
	insmod $(KMOD_SRC)/rtl8192su/rtl8192su.ko debug=5

#silent load
ld:	all
	modprobe mac80211
	insmod $(KMOD_SRC)/rtlwifi.ko
	insmod $(KMOD_SRC)/rtl8192su/rtl8192su.ko

unload:
	rmmod $(KMOD_SRC)/rtl8192su/rtl8192su.ko
	rmmod $(KMOD_SRC)/rtlwifi.ko

reload:	unload load
