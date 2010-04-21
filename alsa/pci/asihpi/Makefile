ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := hpidspcd.c hpios.c

EXTRA_CFLAGS = -I$(SND_TOPDIR)/alsa-kernel/pci/asihpi

include $(SND_TOPDIR)/alsa-kernel/pci/asihpi/Makefile

include $(SND_TOPDIR)/Rules.make

hpidspcd.c: hpidspcd.patch $(SND_TOPDIR)/alsa-kernel/pci/asihpi/hpidspcd.c
hpios.c: hpios.patch $(SND_TOPDIR)/alsa-kernel/pci/asihpi/hpios.c
