ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := usbusx2y.c usX2Yhwdep.c

include $(SND_TOPDIR)/alsa-kernel/usb/usx2y/Makefile

EXTRA_CFLAGS += -I$(SND_TOPDIR)/alsa-kernel/usb/usx2y

include $(SND_TOPDIR)/Rules.make

usbusx2y.c: usbusx2y.patch $(SND_TOPDIR)/alsa-kernel/usb/usx2y/usbusx2y.c
usX2Yhwdep.c: usX2Yhwdep.patch $(SND_TOPDIR)/alsa-kernel/usb/usx2y/usX2Yhwdep.c
