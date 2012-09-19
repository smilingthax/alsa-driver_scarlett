ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := opl3_lib.c

include $(SND_TOPDIR)/alsa-kernel/drivers/opl3/Makefile

include $(SND_TOPDIR)/Rules.make

opl3_lib.c: opl3_lib.patch $(SND_TOPDIR)/alsa-kernel/drivers/opl3/opl3_lib.c
