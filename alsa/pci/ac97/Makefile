ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

export-objs  := ac97_codec.o ak4531_codec.o

clean-files := ac97_proc.c

include $(SND_TOPDIR)/alsa-kernel/pci/ac97/Makefile

include $(SND_TOPDIR)/Rules.make

ac97_proc.c: ac97_proc.patch $(SND_TOPDIR)/alsa-kernel/pci/ac97/ac97_proc.c
