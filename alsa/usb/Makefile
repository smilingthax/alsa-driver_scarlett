TOPDIR = ..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

TOPDIR = $(MAINSRCDIR)

# for compatibility
extra-snd-usb-audio-objs := usbcompat.o

include $(TOPDIR)/alsa-kernel/usb/Makefile

include $(TOPDIR)/Rules.make
