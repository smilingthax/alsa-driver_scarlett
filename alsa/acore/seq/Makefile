ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := seq_clientmgr.c seq_memory.c

obj-$(CONFIG_SND_SERIALMIDI) += snd-seq-midi.o snd-seq.o snd-seq-device.o snd-seq-midi-event.o
obj-$(CONFIG_SND_MSND_PINNACLE) += snd-seq-midi.o snd-seq.o snd-seq-device.o snd-seq-midi-event.o
obj-$(CONFIG_SND_AU8810) += snd-seq-midi.o snd-seq.o snd-seq-device.o snd-seq-midi-event.o
obj-$(CONFIG_SND_AU8820) += snd-seq-midi.o snd-seq.o snd-seq-device.o snd-seq-midi-event.o
obj-$(CONFIG_SND_AU8830) += snd-seq-midi.o snd-seq.o snd-seq-device.o snd-seq-midi-event.o
obj-$(CONFIG_SND_USB_USX2Y) += snd-seq-midi.o snd-seq.o snd-seq-device.o snd-seq-midi-event.o
obj-$(CONFIG_SND_MPU401_ACPI) += snd-seq-midi.o snd-seq.o snd-seq-device.o snd-seq-midi-event.o

export-objs  := seq_device.o seq.o seq_instr.o seq_midi_emul.o \
		seq_midi_event.o seq_virmidi.o

include $(SND_TOPDIR)/alsa-kernel/core/seq/Makefile

EXTRA_CFLAGS += -I$(SND_TOPDIR)/alsa-kernel/core/seq

include $(SND_TOPDIR)/Rules.make

seq_clientmgr.c: seq_clientmgr.patch $(SND_TOPDIR)/alsa-kernel/core/seq/seq_clientmgr.c
seq_memory.c: seq_memory.patch $(SND_TOPDIR)/alsa-kernel/core/seq/seq_memory.c
