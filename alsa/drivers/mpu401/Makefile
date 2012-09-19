TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

export-objs  := mpu401_uart.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/drivers/mpu401/Makefile

include $(TOPDIR)/Rules.make
