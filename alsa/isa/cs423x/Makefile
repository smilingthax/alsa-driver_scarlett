TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

TOPDIR = $(MAINSRCDIR)

export-objs  := cs4231_lib.o cs4236_lib.o

isapnp-files := cs4232.isapnp cs4236.isapnp 

include $(TOPDIR)/alsa-kernel/isa/cs423x/Makefile

include $(TOPDIR)/Rules.make
