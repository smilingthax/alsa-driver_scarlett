ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

snd-usb-us428-objs := usbus428.o usbus428audio.o usX2Yhwdep.o

obj-$(CONFIG_SND_USB_US428) += snd-usb-us428.o

include $(SND_TOPDIR)/Rules.make
