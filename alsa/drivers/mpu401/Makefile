ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

export-objs  := mpu401_uart.o

include $(SND_TOPDIR)/alsa-kernel/drivers/mpu401/Makefile

include $(SND_TOPDIR)/Rules.make
