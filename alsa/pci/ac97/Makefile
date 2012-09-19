TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

export-objs  := ac97_codec.o ak4531_codec.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/pci/ac97/Makefile

include $(TOPDIR)/Rules.make
