ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

snd-emu10k1x-objs := emu10k1x.o

obj-$(CONFIG_SND_EMU10K1X) += snd-emu10k1x.o

export-objs  := emu10k1_main.o

include $(SND_TOPDIR)/alsa-kernel/pci/emu10k1/Makefile

include $(SND_TOPDIR)/Rules.make
