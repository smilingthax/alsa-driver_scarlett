ifndef SND_TOPDIR
SND_TOPDIR=..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := info.c pcm_native.c control.c hwdep.c init.c rawmidi.c sound.c timer.c memalloc.c

obj-$(CONFIG_SND_PDPLUS) += snd-pcm.o snd-timer.o snd-page-alloc.o snd.o
obj-$(CONFIG_SND_MIXART) += snd-pcm.o snd-timer.o snd-page-alloc.o snd.o snd-hwdep.o
obj-$(CONFIG_SND_SERIALMIDI) += snd-rawmidi.o snd.o
obj-$(CONFIG_SND_MSND_PINNACLE) += snd-pcm.o snd-timer.o snd-page-alloc.o snd.o snd-rawmidi.o
obj-$(CONFIG_SND_USB_USX2Y) += snd-pcm.o snd-timer.o snd-page-alloc.o snd.o snd-rawmidi.o snd-hwdep.o
obj-$(CONFIG_SND_AU8810) += snd-pcm.o snd-timer.o snd-page-alloc.o snd.o snd-rawmidi.o snd-hwdep.o
obj-$(CONFIG_SND_AU8820) += snd-pcm.o snd-timer.o snd-page-alloc.o snd.o snd-rawmidi.o snd-hwdep.o
obj-$(CONFIG_SND_AU8830) += snd-pcm.o snd-timer.o snd-page-alloc.o snd.o snd-rawmidi.o snd-hwdep.o
obj-$(CONFIG_SND_BT87X) += snd-pcm.o snd-timer.o snd-page-alloc.o snd.o
obj-$(CONFIG_SND_INTEL8X0M) += snd-pcm.o snd-timer.o snd-page-alloc.o snd.o
obj-$(CONFIG_SND_HDSPM) += snd-pcm.o snd-timer.o snd-page-alloc.o snd.o snd-rawmidi.o snd-hwdep.o

export-objs := sound.o pcm.o pcm_lib.o rawmidi.o timer.o hwdep.o memalloc.o
include $(SND_TOPDIR)/alsa-kernel/core/Makefile

snd-page-alloc-objs += memory_wrapper.o

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
