#
# Makefile for ALSA
#

TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

TOPDIR = $(MAINSRCDIR)

#
#
#

export-objs := opl4_lib.o

snd-opl4-lib-objs := opl4_lib.o opl4_mixer.o opl4_proc.o
ifeq ($(subst m,y,$(CONFIG_SND_SEQUENCER)),y)
  snd-opl4-synth-objs := opl4_seq.o opl4_synth.o yrw801.o
endif

OPL4_OBJS := snd-opl4-lib.o
ifeq ($(subst m,y,$(CONFIG_SND_SEQUENCER)),y)
  OPL4_OBJS += snd-opl4-synth.o
endif

obj-$(CONFIG_SND_OPTI92X_AD1848) += $(OPL4_OBJS)
obj-$(CONFIG_SND_OPTI92X_CS4231) += $(OPL4_OBJS)

include $(TOPDIR)/Rules.make
