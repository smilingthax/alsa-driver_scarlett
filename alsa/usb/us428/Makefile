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

O_TARGET     := _us428.o

snd-usb-us428-objs := usbus428.o usbus428audio.o usX2Yhwdep.o

# Toplevel Module Dependency
obj-$(CONFIG_SND_USB_US428) += snd-usb-us428.o

include $(TOPDIR)/Rules.make
