TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

#
#
#

TOPDIR = $(MAINSRCDIR)

snd-pdaudiocf-objs := pdaudiocf.o pdaudiocf_core.o pdaudiocf_irq.o pdaudiocf_pcm.o

obj-$(CONFIG_SND_PDAUDIOCF) += snd-pdaudiocf.o

include $(TOPDIR)/Rules.make
