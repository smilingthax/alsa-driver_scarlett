#
# Makefile for ALSA
#

TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

export-objs := opl4_lib.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/drivers/opl4/Makefile

include $(TOPDIR)/Rules.make
