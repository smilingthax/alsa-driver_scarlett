ifndef SND_TOPDIR
SND_TOPDIR=..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

#
# ALSA extra code
#

clean-files := bt87x.c intel8x0.c

obj-$(CONFIG_SND) += pdplus/ pcxhr/ echoaudio/

snd-hdspm-objs := hdspm.o
snd-asihpi-objs := asihpi.o

obj-$(CONFIG_SND_HDSPM) +=snd-hdspm.o
obj-$(CONFIG_SND_ASIHPI) += snd-asihpi.o

#
#
#

include $(SND_TOPDIR)/alsa-kernel/pci/Makefile

include $(SND_TOPDIR)/Rules.make

bt87x.c: bt87x.patch $(SND_TOPDIR)/alsa-kernel/pci/bt87x.c
intel8x0.c: intel8x0.patch $(SND_TOPDIR)/alsa-kernel/pci/intel8x0.c
