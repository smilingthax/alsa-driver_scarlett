TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

EXTRA_CLEAN = nm256.c

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/pci/nm256/Makefile

include $(TOPDIR)/Rules.make

nm256.c: nm256.patch $(TOPDIR)/alsa-kernel/pci/nm256/nm256.c
