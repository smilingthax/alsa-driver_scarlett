ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := cs46xx.c

include $(SND_TOPDIR)/alsa-kernel/pci/cs46xx/Makefile

include $(SND_TOPDIR)/Rules.make

cs46xx.c: cs46xx.patch $(SND_TOPDIR)/alsa-kernel/pci/cs46xx/cs46xx.c
