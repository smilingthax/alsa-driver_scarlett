TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

export-objs += ak4xxx-adda.o ak4117.o

snd-ak4117-objs := ak4117.o

# Toplevel Module Dependency
obj-$(CONFIG_SND_PDAUDIOCF) += snd-ak4117.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/i2c/other/Makefile

include $(TOPDIR)/Rules.make
