TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

EXTRA_CLEAN = hammerfall_mem.c

export-objs  := hammerfall_mem.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/pci/rme9652/Makefile

include $(TOPDIR)/Rules.make

hammerfall_mem.c: hammerfall_mem.patch $(TOPDIR)/alsa-kernel/pci/rme9652/hammerfall_mem.c
