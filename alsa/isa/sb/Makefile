TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

# Hack for SB16 CSP
EXTRA_CFLAGS = -DCONFIG_SND_SB16_CSP=1
CONFIG_SND_SB16_CSP=y

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/isa/sb/Makefile

include $(TOPDIR)/Rules.make
