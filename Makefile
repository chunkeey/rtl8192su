# fallback to the current kernel source
KSRC ?= /lib/modules/$(shell uname -r)/build

KMOD_SRC ?= $(PWD)/r92su

# Each configuration option enables a list of files.

KMOD_OPTIONS += CONFIG_R92SU=m CONFIG_R92SU_DEBUGFS=y CONFIG_R92SU_WPC=y CONFIG_R92SU_TRACING=y

# Don't build any of the other drivers
EXTRA_CFLAGS += -DDEBUG -DCONFIG_R92SU=m -DCONFIG_R92SU_DEBUGFS=y -DCONFIG_R92SU_WPC=y -DCONFIG_R92SU_TRACING=y

all:
	$(MAKE) -C $(KSRC) M=$(KMOD_SRC) $(KMOD_OPTIONS) $(MAKECMDGOALS) EXTRA_CFLAGS="$(EXTRA_CFLAGS)"

.PHONY: all load unload reload clean

clean:
	$(MAKE) -C $(KSRC) M=$(KMOD_SRC) clean $(KMOD_OPTIONS)

#debug trace load
load:	
	insmod $(KMOD_SRC)/r92su.ko

unload:
	rmmod r92su

reload:	unload load
