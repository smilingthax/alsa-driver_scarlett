TOPDIR = ..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

#
#
#

TOPDIR = $(MAINSRCDIR)

#
#
#

O_TARGET     := _usb.o

snd-usb-audio-objs := usbaudio.o usbmixer.o

# Toplevel Module Dependency
obj-$(CONFIG_SND_USB_AUDIO) += snd-usb-audio.o

include $(TOPDIR)/Rules.make
