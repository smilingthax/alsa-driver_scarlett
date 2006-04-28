ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := ac97_codec.c ac97_bus.c
export-objs  := ac97_codec.o ac97_pcm.o ak4531_codec.o

include $(SND_TOPDIR)/alsa-kernel/pci/ac97/Makefile

include $(SND_TOPDIR)/Rules.make

ac97_bus.c: ac97_bus.patch $(SND_TOPDIR)/alsa-kernel/pci/ac97/ac97_bus.c
ac97_codec.c: ac97_codec.patch $(SND_TOPDIR)/alsa-kernel/pci/ac97/ac97_codec.c
