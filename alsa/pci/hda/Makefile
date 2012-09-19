ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := hda_intel.c hda_codec.c hda_beep.c

export-objs  := hda_codec.o

EXTRA_CFLAGS += -I$(SND_TOPDIR)/alsa-kernel/pci/hda

include $(SND_TOPDIR)/alsa-kernel/pci/hda/Makefile

include $(SND_TOPDIR)/Rules.make

hda_intel.c: hda_intel.patch $(SND_TOPDIR)/alsa-kernel/pci/hda/hda_intel.c
hda_codec.c: hda_codec.patch $(SND_TOPDIR)/alsa-kernel/pci/hda/hda_codec.c
hda_beep.c: hda_beep.patch $(SND_TOPDIR)/alsa-kernel/pci/hda/hda_beep.c
