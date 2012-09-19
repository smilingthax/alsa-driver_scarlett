TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

EXTRA_CLEAN = vx_entry.c

#
#
#

export-objs = vx_entry.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/pcmcia/vx/Makefile

include $(TOPDIR)/Rules.make

vx_entry.c: vx_entry.patch vx_entry.inc vx_entry.inc1 $(TOPDIR)/alsa-kernel/pcmcia/vx/vx_entry.c
