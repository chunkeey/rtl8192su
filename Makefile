# Convenience wrapper for building by hand; Kbuild has the actual build rules.
#   make                      build against the running kernel's headers
#   make KDIR=<build dir>     build against another kernel
#   make R92SU_WERROR=1       treat warnings as errors
# Installing is left to DKMS (dkms.conf) or to your distribution's packaging.
KDIR ?= /lib/modules/$(shell uname -r)/build

all: modules

modules clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) $@

.PHONY: all modules clean
