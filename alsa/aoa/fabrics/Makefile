ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := layout.c

include $(SND_TOPDIR)/alsa-kernel/aoa/fabrics/Makefile

include $(SND_TOPDIR)/Rules.make

layout.c: layout.patch $(SND_TOPDIR)/alsa-kernel/aoa/fabrics/layout.c
