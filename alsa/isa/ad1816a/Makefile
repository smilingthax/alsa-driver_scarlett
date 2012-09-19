TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

export-objs  := ad1816a_lib.o

isapnp-files := ad1816a.isapnp

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/isa/ad1816a/Makefile

include $(TOPDIR)/Rules.make
