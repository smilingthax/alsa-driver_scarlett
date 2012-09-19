ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

obj-$(CONFIG_SND_AU8810) += snd-mpu401-uart.o
obj-$(CONFIG_SND_AU8820) += snd-mpu401-uart.o
obj-$(CONFIG_SND_AU8830) += snd-mpu401-uart.o

export-objs  := mpu401_uart.o

include $(SND_TOPDIR)/alsa-kernel/drivers/mpu401/Makefile

include $(SND_TOPDIR)/Rules.make
