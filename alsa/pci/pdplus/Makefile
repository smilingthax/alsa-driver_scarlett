#
# Makefile for ALSA
# Copyright (c) 2001 by Jaroslav Kysela <perex@suse.cz>
#

TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

TOPDIR = $(MAINSRCDIR)

#
#
#

O_TARGET     := _pdplus.o

snd-pdplus-objs := pdplus.o

# Toplevel Module Dependency
obj-$(CONFIG_SND_PDPLUS) += snd-pdplus.o

include $(TOPDIR)/Rules.make
