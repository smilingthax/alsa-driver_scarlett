ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

snd-usb-usx2y-objs := usbusx2y.o usbusx2yaudio.o usX2Yhwdep.o

obj-$(CONFIG_SND_USB_USX2Y) += snd-usb-usx2y.o

include $(SND_TOPDIR)/Rules.make
