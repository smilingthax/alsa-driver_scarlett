ifndef SND_TOPDIR
SND_TOPDIR=..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

#
# ALSA extra code
#

clean-files := cs4281.c es1968.c intel8x0.c maestro3.c

obj-$(CONFIG_SND) += pdplus/ mixart/ au88x0/

snd-bt87x-objs := bt87x.o

obj-$(CONFIG_SND_BT87X) += snd-bt87x.o

#
#
#

include $(SND_TOPDIR)/alsa-kernel/pci/Makefile

include $(SND_TOPDIR)/Rules.make

cs4281.c: cs4281.patch $(SND_TOPDIR)/alsa-kernel/pci/cs4281.c
es1968.c: es1968.patch $(SND_TOPDIR)/alsa-kernel/pci/es1968.c
intel8x0.c: intel8x0.patch $(SND_TOPDIR)/alsa-kernel/pci/intel8x0.c
maestro3.c: maestro3.patch $(SND_TOPDIR)/alsa-kernel/pci/maestro3.c

