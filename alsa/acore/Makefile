TOPDIR = ..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

obj-$(CONFIG_SND_SERIALMIDI) += snd-rawmidi.o snd.o
obj-$(CONFIG_SND_MSND_PINNACLE) += snd-pcm.o snd-timer.o snd-page-alloc.o snd.o snd-rawmidi.o
obj-$(CONFIG_SND_AZT3328) += snd-pcm.o snd-timer.o snd-page-alloc.o snd.o snd-rawmidi.o snd-hwdep.o

export-objs := sound.o pcm.o pcm_lib.o rawmidi.o timer.o hwdep.o memalloc.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/core/Makefile

snd-page-alloc-objs += memory_wrapper.o

include $(TOPDIR)/Rules.make

info.c: info.inc info.patch $(TOPDIR)/alsa-kernel/core/info.c
pcm_native.c: pcm_native.patch $(TOPDIR)/alsa-kernel/core/pcm_native.c
