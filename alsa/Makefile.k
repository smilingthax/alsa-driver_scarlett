#
# Makefile for ALSA
# Copyright (c) 1999 by Jaroslav Kysela <perex@suse.cz>
#

subdir-y     := kernel lowlevel cards
subdir-m     := $(subdir-y)

O_TARGET     := snd.o

ifeq ($(CONFIG_SND),y)
obj-y	     += kernel/snd_snd.o
endif
ifeq ($(CONFIG_SND_OSSEMUL),y)
obj-y	     += kernel/oss/snd_oss.o
endif
ifeq ($(CONFIG_SND_SEQUENCER),y)
obj-y        += kernel/seq/snd_seq.o
obj-y	     += kernel/seq/instr/snd_ainstr.o
endif
ifeq ($(CONFIG_SND_SEQUENCER_OSS),y)
obj-y	     += kernel/seq/oss/snd_seq_oss.o
endif
obj-y	     += lowlevel/generic/snd_generic.o
obj-y	     += lowlevel/isa/snd_isa.o
obj-y	     += lowlevel/i2c/snd_i2c.o
ifeq ($(CONFIG_SND_SB8_DSP),y)
obj-y	     += lowlevel/sb/snd_sb.o
endif
ifeq ($(CONFIG_SND_SB16_DSP),y)
obj-y	     += lowlevel/sb/snd_sb.o
endif
ifeq ($(CONFIG_SND_GUS),y)
obj-y	     += lowlevel/gus/snd_gus.o
endif
ifeq ($(CONFIG_SND_EMU10K1),y)
obj-y	     += lowlevel/emu10k1/snd_emu10k1.o
endif
ifeq ($(CONFIG_PCI),y)
obj-y	     += lowlevel/pci/snd_pci.o
endif
obj-y        += cards/snd_cards.o

include $(TOPDIR)/Rules.make
