TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

obj-$(CONFIG_SND_AZT3328) += snd-opl3-lib.o
ifeq ($(subst m,y,$(CONFIG_SND_SEQUENCER)),y)
  obj-$(CONFIG_SND_AZT3328) += snd-opl3-synth.o
endif

obj-$(CONFIG_SND_PC98_CS4232) += $(OPL3_OBJS)

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/drivers/opl3/Makefile

include $(TOPDIR)/Rules.make
