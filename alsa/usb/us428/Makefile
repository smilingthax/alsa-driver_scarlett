#
# Makefile for ALSA
# Copyright (c) 2001 by Jaroslav Kysela <perex@suse.cz>
#

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

#
#
#

snd-usb-us428-objs := usbus428.o usbus428audio.o usX2Yhwdep.o

# Toplevel Module Dependency
obj-$(CONFIG_SND_USB_US428) += snd-usb-us428.o

include $(SND_TOPDIR)/Rules.make
