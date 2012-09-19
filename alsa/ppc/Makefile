TOPDIR = ..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

EXTRA_CLEAN = awacs.c pmac.c

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/ppc/Makefile

EXTRA_CFLAGS = -I$(TOPDIR)/alsa-kernel/ppc

include $(TOPDIR)/Rules.make

awacs.c: awacs.patch $(TOPDIR)/alsa-kernel/ppc/awacs.c
pmac.c: pmac.patch $(TOPDIR)/alsa-kernel/ppc/pmac.c
