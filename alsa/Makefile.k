#
# Makefile for ALSA
# Copyright (c) 1999 by Jaroslav Kysela <perex@suse.cz>
#

SUB_DIRS     := kernel lowlevel cards
MOD_SUB_DIRS := $(SUB_DIRS)
ALL_SUB_DIRS := $(SUB_DIRS)

L_TARGET     := snd.a
L_OBJS       := kernel/snd_snd.o
LX_OBJS      :=
O_OBJS       :=
OX_OBJS      :=
M_OBJS       :=
MX_OBJS      :=
MIX_OBJS     :=

ifeq ($(CONFIG_SND_PCM_OSS),y)
L_OBJS       += kernel/plugin/snd_plugin.o
endif
ifeq ($(CONFIG_SND_SEQUENCER),y)
L_OBJS       += kernel/seq/snd_seq.o
L_OBJS       += kernel/seq/instr/snd_ainstr.o
endif
ifeq ($(CONFIG_SND_SEQUENCER_OSS),y)
L_OBJS       += kernel/seq/oss/snd_seq_oss.o
endif
L_OBJS       += lowlevel/generic/snd_generic.o
L_OBJS       += lowlevel/isa/snd_isa.o
L_OBJS       += lowlevel/i2c/snd_i2c.o
ifeq ($(CONFIG_SND_SB8_DSP),y)
L_OBJS       += lowlevel/sb/snd_sb.o
endif
ifeq ($(CONFIG_SND_SB16_DSP),y)
L_OBJS       += lowlevel/sb/snd_sb.o
endif
ifeq ($(CONFIG_SND_GUS),y)
L_OBJS       += lowlevel/gus/snd_gus.o
endif
ifeq ($(CONFIG_SND_EMU10K1),y)
L_OBJS       += lowlevel/emu10k1/snd_emu10k1.o
endif
ifeq ($(CONFIG_PCI),y)
L_OBJS       += lowlevel/pci/snd_pci.o
endif
L_OBJS       += cards/snd_cards.o

include $(TOPDIR)/Rules.make
