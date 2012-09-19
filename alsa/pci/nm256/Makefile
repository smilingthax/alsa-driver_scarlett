ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := nm256.c

include $(SND_TOPDIR)/alsa-kernel/pci/nm256/Makefile

include $(SND_TOPDIR)/Rules.make

nm256.c: nm256.patch $(SND_TOPDIR)/alsa-kernel/pci/nm256/nm256.c
