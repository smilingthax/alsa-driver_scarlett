ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

obj-$(CONFIG_SND_AU8810) += snd-ac97-codec.o
obj-$(CONFIG_SND_AU8820) += snd-ac97-codec.o
obj-$(CONFIG_SND_AU8830) += snd-ac97-codec.o
obj-$(CONFIG_SND_ATIIXP) += snd-ac97-codec.o
obj-$(CONFIG_SND_INTEL8X0M) += snd-ac97-codec.o

export-objs  := ac97_codec.o ak4531_codec.o

include $(SND_TOPDIR)/alsa-kernel/pci/ac97/Makefile

include $(SND_TOPDIR)/Rules.make
