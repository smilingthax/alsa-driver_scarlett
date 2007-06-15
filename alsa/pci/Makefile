ifndef SND_TOPDIR
SND_TOPDIR=..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

#
# ALSA extra code
#

clean-files := ad1889.c bt87x.c intel8x0.c maestro3.c

obj-$(CONFIG_SND) += pdplus/ asihpi/ cmi8788/

#
#
#

include $(SND_TOPDIR)/alsa-kernel/pci/Makefile

include $(SND_TOPDIR)/Rules.make

ad1889.c: ad1889.patch $(SND_TOPDIR)/alsa-kernel/pci/ad1889.c
bt87x.c: bt87x.patch $(SND_TOPDIR)/alsa-kernel/pci/bt87x.c
intel8x0.c: intel8x0.patch $(SND_TOPDIR)/alsa-kernel/pci/intel8x0.c
maestro3.c: maestro3.patch $(SND_TOPDIR)/alsa-kernel/pci/maestro3.c
