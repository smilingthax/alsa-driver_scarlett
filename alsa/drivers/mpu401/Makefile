TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

obj-$(CONFIG_SND_SSCAPE) += snd-mpu401-uart.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/drivers/mpu401/Makefile
