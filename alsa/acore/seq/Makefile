TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

obj-$(CONFIG_SND_SERIALMIDI) += snd-seq-midi.o snd-seq.o snd-seq-device.o snd-seq-midi-event.o
obj-$(CONFIG_SND_SSCAPE) += snd-seq-midi.o snd-seq.o snd-seq-device.o snd-seq-midi-event.o
obj-$(CONFIG_SND_MSND_PINNACLE) += snd-seq-midi.o snd-seq.o snd-seq-device.o snd-seq-midi-event.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/core/seq/Makefile
