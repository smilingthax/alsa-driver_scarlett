TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

# Hack for new dsp flag
EXTRA_CFLAGS = -DCONFIG_SND_CS46XX_NEW_DSP=1
CONFIG_SND_CS46XX_NEW_DSP=y

EXTRA_CLEAN = cs46xx.c

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/pci/cs46xx/Makefile

include $(TOPDIR)/Rules.make

cs46xx.c: cs46xx.patch $(TOPDIR)/alsa-kernel/pci/cs46xx/cs46xx.c
