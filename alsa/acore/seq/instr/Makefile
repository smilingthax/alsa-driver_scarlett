ifndef SND_TOPDIR
SND_TOPDIR=../../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

export-objs  := ainstr_fm.o ainstr_simple.o ainstr_gf1.o ainstr_iw.o

include $(SND_TOPDIR)/alsa-kernel/core/seq/instr/Makefile

include $(SND_TOPDIR)/Rules.make
