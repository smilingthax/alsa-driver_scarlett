TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

# Hack for new dsp flag
EXTRA_CFLAGS = -DCONFIG_SND_CS46XX_NEW_DSP=1
CONFIG_SND_CS46XX_NEW_DSP=y

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/pci/cs46xx/Makefile

include $(TOPDIR)/Rules.make
