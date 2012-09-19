ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

# Hack for new dsp flag
EXTRA_CFLAGS += -DCONFIG_SND_CS46XX_NEW_DSP=1
CONFIG_SND_CS46XX_NEW_DSP=y

clean-files := cs46xx.c

include $(SND_TOPDIR)/alsa-kernel/pci/cs46xx/Makefile

include $(SND_TOPDIR)/Rules.make

cs46xx.c: cs46xx.patch $(SND_TOPDIR)/alsa-kernel/pci/cs46xx/cs46xx.c
