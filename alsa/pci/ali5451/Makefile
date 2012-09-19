TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

EXTRA_CLEAN = ali5451.c

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/pci/ali5451/Makefile

include $(TOPDIR)/Rules.make

ali5451.c: ali5451.patch $(TOPDIR)/alsa-kernel/pci/ali5451/ali5451.c

