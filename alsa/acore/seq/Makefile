TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

EXTRA_CLEAN = seq_clientmgr.c seq_memory.c

obj-$(CONFIG_SND_SERIALMIDI) += snd-seq-midi.o snd-seq.o snd-seq-device.o snd-seq-midi-event.o
obj-$(CONFIG_SND_MSND_PINNACLE) += snd-seq-midi.o snd-seq.o snd-seq-device.o snd-seq-midi-event.o

export-objs  := seq_device.o seq.o seq_instr.o seq_midi_emul.o \
		seq_midi_event.o seq_virmidi.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/core/seq/Makefile

EXTRA_CFLAGS = -I$(TOPDIR)/alsa-kernel/core/seq

include $(TOPDIR)/Rules.make

seq_clientmgr.c: seq_clientmgr.patch $(TOPDIR)/alsa-kernel/core/seq/seq_clientmgr.c
seq_memory.c: seq_memory.patch $(TOPDIR)/alsa-kernel/core/seq/seq_memory.c
