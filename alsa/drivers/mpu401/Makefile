ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

snd-mpu401-acpi-objs := mpu401_acpi.o

obj-$(CONFIG_SND_MPU401_ACPI) += snd-mpu401-acpi.o snd-mpu401-uart.o

export-objs  := mpu401_uart.o

include $(SND_TOPDIR)/alsa-kernel/drivers/mpu401/Makefile

include $(SND_TOPDIR)/Rules.make
