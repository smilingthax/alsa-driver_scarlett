ifndef SND_TOPDIR
SND_TOPDIR=..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

#
# ALSA extra code
#

clean-files := intel8x0.c

obj-$(CONFIG_SND) += pdplus/ azx/

snd-hdspm-objs := hdspm.o
snd-via82xx-modem-objs := via82xx_modem.o
snd-asihpi-objs := asihpi.o

obj-$(CONFIG_SND_HDSPM) +=snd-hdspm.o
obj-$(CONFIG_SND_VIA82XX_MODEM) += snd-via82xx-modem.o
obj-$(CONFIG_SND_ASIHPI) += snd-asihpi.o

#
#
#

include $(SND_TOPDIR)/alsa-kernel/pci/Makefile

include $(SND_TOPDIR)/Rules.make

intel8x0.c: intel8x0.patch $(SND_TOPDIR)/alsa-kernel/pci/intel8x0.c
