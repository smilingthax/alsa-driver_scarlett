#
# Makefile for ALSA
# Copyright (c) 1999 by Jaroslav Kysela <perex@suse.cz>
#

SUB_DIRS     := kernel lowlevel cards
MOD_SUB_DIRS := $(SUB_DIRS)
ALL_SUB_DIRS := $(SUB_DIRS)

O_TARGET     := snd.o

L_OBJS       :=
LX_OBJS      :=
O_OBJS       :=
OX_OBJS      :=
M_OBJS       :=
MX_OBJS      :=
MIX_OBJS     :=

ifeq ($(CONFIG_SND),y)
O_OBJS       += kernel/snd_snd.o
endif
ifeq ($(CONFIG_SND_OSSEMUL),y)
O_OBJS       += kernel/oss/snd_oss.o
endif
ifeq ($(CONFIG_SND_SEQUENCER),y)
O_OBJS       += kernel/seq/snd_seq.o
O_OBJS       += kernel/seq/instr/snd_ainstr.o
endif
ifeq ($(CONFIG_SND_SEQUENCER_OSS),y)
O_OBJS       += kernel/seq/oss/snd_seq_oss.o
endif
O_OBJS       += lowlevel/generic/snd_generic.o
O_OBJS       += lowlevel/isa/snd_isa.o
O_OBJS       += lowlevel/i2c/snd_i2c.o
ifeq ($(CONFIG_SND_SB8_DSP),y)
O_OBJS       += lowlevel/sb/snd_sb.o
endif
ifeq ($(CONFIG_SND_SB16_DSP),y)
O_OBJS       += lowlevel/sb/snd_sb.o
endif
ifeq ($(CONFIG_SND_GUS),y)
O_OBJS       += lowlevel/gus/snd_gus.o
endif
ifeq ($(CONFIG_SND_EMU10K1),y)
O_OBJS       += lowlevel/emu10k1/snd_emu10k1.o
endif
ifeq ($(CONFIG_PCI),y)
O_OBJS       += lowlevel/pci/snd_pci.o
endif
O_OBJS       += cards/snd_cards.o

include $(TOPDIR)/Rules.make
