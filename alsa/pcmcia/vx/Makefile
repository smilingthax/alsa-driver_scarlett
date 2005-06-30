ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := vxpocket.c

include $(SND_TOPDIR)/alsa-kernel/pcmcia/vx/Makefile

include $(SND_TOPDIR)/Rules.make

vxpocket.c: vxpocket.patch $(SND_TOPDIR)/alsa-kernel/pcmcia/vx/vxpocket.c
