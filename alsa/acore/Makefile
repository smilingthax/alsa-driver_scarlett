ifndef SND_TOPDIR
SND_TOPDIR=..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := info.c info_oss.c pcm.c pcm_native.c pcm_memory.c control.c \
	       hwdep.c init.c rawmidi.c sound.c timer.c memalloc.c misc.c \
	       hrtimer.c

include $(SND_TOPDIR)/alsa-kernel/core/Makefile

snd-objs += wrappers.o misc_driver.o
ifeq ($(CONFIG_SND_DEBUG_MEMORY),y)
snd-objs += memory_debug.o
endif
snd-page-alloc-objs += memory_wrapper.o
snd-hpet-objs := hpetimer.o

obj-$(CONFIG_SND_HPET) += snd-hpet.o
obj-$(CONFIG_SND_BIT32_EMUL) += ioctl32/

include $(SND_TOPDIR)/Rules.make

info.c: info.patch $(SND_TOPDIR)/alsa-kernel/core/info.c
info_oss.c: info_oss.patch $(SND_TOPDIR)/alsa-kernel/core/info_oss.c
pcm.c: pcm.patch $(SND_TOPDIR)/alsa-kernel/core/pcm.c
pcm_native.c: pcm_native.patch $(SND_TOPDIR)/alsa-kernel/core/pcm_native.c
pcm_memory.c: pcm_memory.patch $(SND_TOPDIR)/alsa-kernel/core/pcm_memory.c
control.c: control.patch $(SND_TOPDIR)/alsa-kernel/core/control.c
hwdep.c: hwdep.patch $(SND_TOPDIR)/alsa-kernel/core/hwdep.c
init.c: init.patch $(SND_TOPDIR)/alsa-kernel/core/init.c
rawmidi.c: rawmidi.patch $(SND_TOPDIR)/alsa-kernel/core/rawmidi.c
sound.c: sound.patch $(SND_TOPDIR)/alsa-kernel/core/sound.c
timer.c: timer.patch $(SND_TOPDIR)/alsa-kernel/core/timer.c
memalloc.c: memalloc.patch memalloc.inc memalloc.inc1 $(SND_TOPDIR)/alsa-kernel/core/memalloc.c
misc.c: misc.patch $(SND_TOPDIR)/alsa-kernel/core/misc.c
hrtimer.c: hrtimer.patch $(SND_TOPDIR)/alsa-kernel/core/hrtimer.c
