r92su-objs	+= 						\
	michael.o aes_ccm.o eeprom.o event.o debug.o main.o	\
	sta.o usb.o cmd.o fw.o rx.o hw.o tx.o pwr.o		\


r92su-$(CONFIG_R92SU_DEBUGFS)	+= debugfs.o
r92su-$(CONFIG_R92SU_TRACING)	+= trace.o
obj-$(CONFIG_R92SU)		+= r92su.o
