# $Id: Makefile,v 1.3 2002/03/28 19:04:56 perex Exp $

TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

TOPDIR = $(MAINSRCDIR)
#include $(TOPDIR)/alsa-kernel/i2c/l3/Makefile

#(Si)

O_TARGET = l3.o

list-multi += snd-uda1341.o

export-objs += uda1341.o

snd-uda1341-objs := uda1341.o

# Module Dependency
obj-$(CONFIG_SND_H3600_UDA1341) += snd-uda1341.o

include $(TOPDIR)/Rules.make

snd-uda1341.o: $(snd-uda1341-objs)
	$(LD) $(LD_RFLAG) -r -o $@ $(snd-uda1341-objs)
