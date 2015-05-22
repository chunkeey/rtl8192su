# fallback to the current kernel source
KSRC ?= /lib/modules/$(shell uname -r)/build

KMOD_SRC ?= $(CURDIR)/r92su

EXTRA_CFLAGS += -DDEBUG

OPTION_R92SU = CONFIG_R92SU=m
KMOD_OPTIONS = $(OPTION_R92SU)
R92SU_DEP_MODS = cfg80211
EXTRA_CFLAGS += -D$(OPTION_R92SU)

OPTION_WPC = CONFIG_R92SU_WPC=y
KMOD_OPTIONS += $(OPTION_WPC)
R92SU_DEP_MODS += input_core
EXTRA_CFLAGS += -D$(OPTION_WPC)

#OPTION_DEBFS = CONFIG_R92SU_DEBUGFS=y
#KMOD_OPTIONS += $(OPTION_DEBFS)
#R92SU_DEP_MODS += debugfs
#EXTRA_CFLAGS += -D$(OPTION_DEBFS)

all:
	$(MAKE) -C $(KSRC) M=$(KMOD_SRC) $(KMOD_OPTIONS) $(MAKECMDGOALS) EXTRA_CFLAGS="$(EXTRA_CFLAGS)"

.PHONY: all load unload reload clean

clean:
	$(MAKE) -C $(KSRC) M=$(KMOD_SRC) clean $(KMOD_OPTIONS)

#debug trace load
load:
	@for DEP_MOD in $(R92SU_DEP_MODS); do	\
		modprobe "$$DEP_MOD" || echo "continue..."; \
	done
	insmod "$(KMOD_SRC)/r92su.ko"

unload:
	rmmod r92su

reload:	unload load
