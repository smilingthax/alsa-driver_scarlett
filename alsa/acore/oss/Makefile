TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

export-objs  := mixer_oss.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/core/oss/Makefile

include $(TOPDIR)/Rules.make
