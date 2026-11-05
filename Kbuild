# Out-of-tree build (scripts/cross-build-kernel.sh, M=drivers/r92su).
obj-m := r92su.o
r92su-y := michael.o aes_ccm.o eeprom.o event.o debug.o main.o \
	tkip.o wep.o sta.o usb.o pwr.o cmd.o fw.o rx.o hw.o tx.o
# Kernel bumps should fail loudly here, not as odd runtime behaviour.
ccflags-y += -Werror
