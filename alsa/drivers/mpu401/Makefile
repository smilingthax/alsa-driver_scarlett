TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

obj-$(CONFIG_SND_SSCAPE) += snd-mpu401-uart.o
obj-$(CONFIG_SND_AZT3328) += snd-mpu401-uart.o
obj-$(CONFIG_SND_PC98_CS4232) += snd-mpu401-uart.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/drivers/mpu401/Makefile

include $(TOPDIR)/Rules.make
