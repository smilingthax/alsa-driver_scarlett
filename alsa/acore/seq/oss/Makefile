TOPDIR = ../../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

EXTRA_CLEAN = seq_oss.c

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/core/seq/oss/Makefile

EXTRA_CFLAGS = -I$(TOPDIR)/alsa-kernel/core/seq/oss

include $(TOPDIR)/Rules.make

seq_oss.c: seq_oss.patch $(TOPDIR)/alsa-kernel/core/seq/oss/seq_oss.c
