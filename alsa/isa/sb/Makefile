TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

# Hack for SB16 CSP
EXTRA_CFLAGS = -DCONFIG_SND_SB16_CSP=1
CONFIG_SND_SB16_CSP=y

export-objs  := emu8000.o sb_common.o sb8_main.o sb16_main.o sb16_csp.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/isa/sb/Makefile

include $(TOPDIR)/Rules.make
