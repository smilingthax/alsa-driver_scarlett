TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

EXTRA_CLEAN = trident.c

export-objs  := trident_main.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/pci/trident/Makefile

include $(TOPDIR)/Rules.make

trident.c: trident.patch $(TOPDIR)/alsa-kernel/pci/trident/trident.c
