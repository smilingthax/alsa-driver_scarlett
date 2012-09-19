#
# Makefile for ALSA
# Copyright (c) 2001 by Jaroslav Kysela <perex@suse.cz>
#

TOPDIR = ..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

TOPDIR = $(MAINSRCDIR)

O_TARGET     := synth.o

list-multi   := snd-isapnp.o

export-objs  := isapnp.o

snd-isapnp-objs := isapnp.o isapnp_proc.o isapnp_quirks.o

# Toplevel Module Dependency
ifeq (n,$(CONFIG_ISAPNP_KERNEL))
ifeq (y,$(CONFIG_ISAPNP))
obj-m += snd-isapnp.o
endif
endif

include $(TOPDIR)/Rules.make
