TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

#
#
#

TOPDIR = $(MAINSRCDIR)

snd-mixart-objs := mixart.o mixart_core.o mixart_hwdep.o mixart_mixer.o

obj-$(CONFIG_SND_MIXART) += snd-mixart.o

include $(TOPDIR)/Rules.make
