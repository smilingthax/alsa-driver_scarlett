TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

obj-$(CONFIG_SND_AU8810) += snd-mpu401-uart.o
obj-$(CONFIG_SND_AU8820) += snd-mpu401-uart.o
obj-$(CONFIG_SND_AU8830) += snd-mpu401-uart.o

export-objs  := mpu401_uart.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/drivers/mpu401/Makefile

include $(TOPDIR)/Rules.make
