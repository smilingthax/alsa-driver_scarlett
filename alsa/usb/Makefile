TOPDIR = ..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

EXTRA_CLEAN = usbaudio.c

TOPDIR = $(MAINSRCDIR)

# for compatibility
extra-snd-usb-audio-objs := usbcompat.o

include $(TOPDIR)/alsa-kernel/usb/Makefile

EXTRA_CFLAGS = -I$(TOPDIR)/alsa-kernel/usb

include $(TOPDIR)/Rules.make

usbaudio.c: usbaudio.patch usbaudio.inc usbaudio.inc1 $(TOPDIR)/alsa-kernel/usb/usbaudio.c
