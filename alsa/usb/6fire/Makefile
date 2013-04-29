ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := pcm.c

include $(SND_TOPDIR)/alsa-kernel/usb/6fire/Makefile

EXTRA_CFLAGS += -I$(SND_TOPDIR)/alsa-kernel/usb/6fire

include $(SND_TOPDIR)/Rules.make

pcm.c: pcm.patch $(SND_TOPDIR)/alsa-kernel/usb/6fire/pcm.c
