ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := pdaudiocf.c

include $(SND_TOPDIR)/alsa-kernel/pcmcia/pdaudiocf/Makefile

include $(SND_TOPDIR)/Rules.make

pdaudiocf.c: pdaudiocf.patch $(SND_TOPDIR)/alsa-kernel/pcmcia/pdaudiocf/pdaudiocf.c
