TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

obj-$(CONFIG_SND_AU8810) += snd-ac97-codec.o
obj-$(CONFIG_SND_AU8820) += snd-ac97-codec.o
obj-$(CONFIG_SND_AU8830) += snd-ac97-codec.o

export-objs  := ac97_codec.o ak4531_codec.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/pci/ac97/Makefile

include $(TOPDIR)/Rules.make
