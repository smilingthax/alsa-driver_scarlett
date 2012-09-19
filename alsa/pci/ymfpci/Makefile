include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := ymfpci.c

include $(SND_TOPDIR)/alsa-kernel/pci/ymfpci/Makefile

include $(SND_TOPDIR)/Rules.make

ymfpci.c: ymfpci.patch $(SND_TOPDIR)/alsa-kernel/pci/ymfpci/ymfpci.c
