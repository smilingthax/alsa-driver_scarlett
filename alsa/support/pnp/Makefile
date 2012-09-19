#
# Makefile for ALSA
# Copyright (c) 2001 by Jaroslav Kysela <perex@suse.cz>
#

TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

TOPDIR = $(MAINSRCDIR)

export-objs  += pnp.o

snd-pnp-objs := pnp.o

# Toplevel Module Dependency
ifndef CONFIG_PNP_KERNEL
ifeq (y,$(CONFIG_ISAPNP))
obj-m += snd-pnp.o
endif
endif

include $(TOPDIR)/Rules.make
