TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

obj-$(CONFIG_SND_USB_MIDI) += snd-seq-virmidi.o snd-seq.o snd-seq-device.o snd-seq-midi-event.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/core/seq/Makefile
