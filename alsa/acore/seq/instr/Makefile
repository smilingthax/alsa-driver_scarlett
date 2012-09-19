TOPDIR = ../../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

export-objs  := ainstr_fm.o ainstr_simple.o ainstr_gf1.o ainstr_iw.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/core/seq/instr/Makefile

include $(TOPDIR)/Rules.make
