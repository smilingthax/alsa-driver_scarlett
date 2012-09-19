include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := trident.c

export-objs  := trident_main.o

include $(SND_TOPDIR)/alsa-kernel/pci/trident/Makefile

include $(SND_TOPDIR)/Rules.make

trident.c: trident.patch $(SND_TOPDIR)/alsa-kernel/pci/trident/trident.c
