TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

obj-$(CONFIG_SND_AZT3328) += snd-opl3-lib.o
ifeq ($(subst m,y,$(CONFIG_SND_SEQUENCER)),y)
  obj-$(CONFIG_SND_AZT3328) += snd-opl3-synth.o
endif

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/drivers/opl3/Makefile
