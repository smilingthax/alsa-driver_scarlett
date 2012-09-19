ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

export-objs  := memory.o voice.o io.c

include $(SND_TOPDIR)/alsa-kernel/pci/emu10k1/Makefile

include $(SND_TOPDIR)/Rules.make
