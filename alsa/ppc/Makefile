include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := awacs.c pmac.c

include $(SND_TOPDIR)/alsa-kernel/ppc/Makefile

EXTRA_CFLAGS += -I$(SND_TOPDIR)/alsa-kernel/ppc

include $(SND_TOPDIR)/Rules.make

awacs.c: awacs.patch $(SND_TOPDIR)/alsa-kernel/ppc/awacs.c
pmac.c: pmac.patch $(SND_TOPDIR)/alsa-kernel/ppc/pmac.c
