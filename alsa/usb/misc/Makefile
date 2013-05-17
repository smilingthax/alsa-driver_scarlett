ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := ua101.c

include $(SND_TOPDIR)/alsa-kernel/usb/misc/Makefile

EXTRA_CFLAGS += -I$(SND_TOPDIR)/alsa-kernel/usb/misc

include $(SND_TOPDIR)/Rules.make

ua101.c: ua101.patch $(SND_TOPDIR)/alsa-kernel/usb/misc/ua101.c
