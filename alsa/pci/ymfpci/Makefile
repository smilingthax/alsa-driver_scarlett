TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

EXTRA_CLEAN = ymfpci.c

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/pci/ymfpci/Makefile

include $(TOPDIR)/Rules.make

ymfpci.c: ymfpci.patch $(TOPDIR)/alsa-kernel/pci/ymfpci/ymfpci.c
