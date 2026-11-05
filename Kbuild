# Out-of-tree build: make -C <kernel build dir> M=$PWD modules (see Makefile, dkms.conf).
obj-m := r92su.o
r92su-y := michael.o aes_ccm.o eeprom.o event.o debug.o main.o \
	tkip.o wep.o sta.o usb.o pwr.o cmd.o fw.o rx.o hw.o tx.o

# cfg80211 after 6.18 passes some callbacks a wireless_dev (see r92su.h).
# Detected from the kernel's header rather than a version number. The pattern
# is in a variable because its ")(" would end a $(shell ...) written inline.
r92su_wdev_pattern := (*get_station)(struct wiphy *wiphy, struct wireless_dev
ifneq ($(shell grep -sF '$(r92su_wdev_pattern)' $(srctree)/include/net/cfg80211.h),)
ccflags-y += -DR92SU_CFG80211_WDEV
endif

# Warnings as errors only when asked (R92SU_WERROR=1): every new compiler
# brings new warnings, and those mustn't break a DKMS build on a user's
# machine. Our own kernel builds (the appliance) set it, so kernel bumps fail
# loudly there instead of as odd runtime behaviour.
ifeq ($(R92SU_WERROR),1)
ccflags-y += -Werror
endif
