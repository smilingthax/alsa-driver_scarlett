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

snd-msnd-pinnacle-objs := msnd.o msnd_pinnacle.o msnd_pinnacle_mixer.o msnd_midi.o
# snd-msnd-classic-objs := msnd.o msnd_classic.o msnd_midi.o

# Toplevel Module Dependency
obj-$(CONFIG_SND_MSND_PINNACLE) += snd-msnd-pinnacle.o
# obj-$(CONFIG_SND_MSND_CLASSIC) += snd-msnd-classic.o

include $(TOPDIR)/Rules.make
