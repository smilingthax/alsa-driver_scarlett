ifndef SND_TOPDIR
SND_TOPDIR=..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

#
# ALSA extra code
#

clean-files := intel8x0.c

obj-$(CONFIG_SND) += pdplus/

snd-hdspm-objs := hdspm.o
snd-atiixp-modem-objs := atiixp_modem.o

obj-$(CONFIG_SND_HDSPM) +=snd-hdspm.o
obj-$(CONFIG_SND_ATIIXP_MODEM) += snd-atiixp-modem.o

#
#
#

include $(SND_TOPDIR)/alsa-kernel/pci/Makefile

include $(SND_TOPDIR)/Rules.make

intel8x0.c: intel8x0.patch $(SND_TOPDIR)/alsa-kernel/pci/intel8x0.c
