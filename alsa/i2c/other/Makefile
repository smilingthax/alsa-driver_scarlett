ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

export-objs += ak4xxx-adda.o ak4117.o

snd-ak4117-objs := ak4117.o

# Toplevel Module Dependency
obj-$(CONFIG_SND_PDAUDIOCF) += snd-ak4117.o

include $(SND_TOPDIR)/alsa-kernel/i2c/other/Makefile

include $(SND_TOPDIR)/Rules.make
