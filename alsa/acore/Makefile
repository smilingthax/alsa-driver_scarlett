ifndef SND_TOPDIR
SND_TOPDIR=..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := info.c pcm_native.c control.c hwdep.c init.c rawmidi.c sound.c timer.c memalloc.c

export-objs := sound.o pcm.o pcm_lib.o rawmidi.o timer.o hwdep.o memalloc.o
include $(SND_TOPDIR)/alsa-kernel/core/Makefile

snd-page-alloc-objs += memory_wrapper.o

obj-$(CONFIG_SND_BIT32_EMUL) += ioctl32/

include $(SND_TOPDIR)/Rules.make

info.c: info.inc info.patch $(SND_TOPDIR)/alsa-kernel/core/info.c
pcm_native.c: pcm_native.patch $(SND_TOPDIR)/alsa-kernel/core/pcm_native.c
control.c: control.patch $(SND_TOPDIR)/alsa-kernel/core/control.c
hwdep.c: hwdep.patch $(SND_TOPDIR)/alsa-kernel/core/hwdep.c
init.c: init.patch $(SND_TOPDIR)/alsa-kernel/core/init.c
rawmidi.c: rawmidi.patch $(SND_TOPDIR)/alsa-kernel/core/rawmidi.c
sound.c: sound.patch $(SND_TOPDIR)/alsa-kernel/core/sound.c
timer.c: timer.patch $(SND_TOPDIR)/alsa-kernel/core/timer.c
memalloc.c: memalloc.patch memalloc.inc memalloc.inc1 $(SND_TOPDIR)/alsa-kernel/core/memalloc.c
