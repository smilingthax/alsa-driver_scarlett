TOPDIR = ..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

EXTRA_CLEAN = info.c pcm_native.c control.c hwdep.c init.c rawmidi.c sound.c timer.c memalloc.c

obj-$(CONFIG_SND_SERIALMIDI) += snd-rawmidi.o snd.o
obj-$(CONFIG_SND_MSND_PINNACLE) += snd-pcm.o snd-timer.o snd-page-alloc.o snd.o snd-rawmidi.o

export-objs := sound.o pcm.o pcm_lib.o rawmidi.o timer.o hwdep.o memalloc.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/core/Makefile

snd-page-alloc-objs += memory_wrapper.o

include $(TOPDIR)/Rules.make

info.c: info.inc info.patch $(TOPDIR)/alsa-kernel/core/info.c
pcm_native.c: pcm_native.patch $(TOPDIR)/alsa-kernel/core/pcm_native.c
control.c: control.patch $(TOPDIR)/alsa-kernel/core/control.c
hwdep.c: hwdep.patch $(TOPDIR)/alsa-kernel/core/hwdep.c
init.c: init.patch $(TOPDIR)/alsa-kernel/core/init.c
rawmidi.c: rawmidi.patch $(TOPDIR)/alsa-kernel/core/rawmidi.c
sound.c: sound.patch $(TOPDIR)/alsa-kernel/core/sound.c
timer.c: timer.patch $(TOPDIR)/alsa-kernel/core/timer.c
memalloc.c: memalloc.patch memalloc.inc memalloc.inc1 $(TOPDIR)/alsa-kernel/core/memalloc.c
