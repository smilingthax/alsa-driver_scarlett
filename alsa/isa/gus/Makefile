TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

export-objs  := gus_main.o gus_volume.o

isapnp-files := interwave.isapnp interwave-stb.isapnp

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/isa/gus/Makefile

include $(TOPDIR)/Rules.make
