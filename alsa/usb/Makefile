ifndef SND_TOPDIR
SND_TOPDIR=..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := usbaudio.c

# for compatibility
extra-snd-usb-audio-objs := usbcompat.o

# usx2y depends on usb-audio
obj-$(CONFIG_SND_USB_USX2Y) += snd-usb-audio.o

obj-$(CONFIG_SND) += usx2y/

export-objs := usbaudio.o usbmidi.o usbcompat.o

include $(SND_TOPDIR)/alsa-kernel/usb/Makefile

EXTRA_CFLAGS += -I$(SND_TOPDIR)/alsa-kernel/usb

include $(SND_TOPDIR)/Rules.make

usbaudio.c: usbaudio.patch usbaudio.inc usbaudio.inc1 $(SND_TOPDIR)/alsa-kernel/usb/usbaudio.c
