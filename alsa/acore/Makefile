TOPDIR = ..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

obj-$(CONFIG_SND_SERIALMIDI) += snd-rawmidi.o snd.o
obj-$(CONFIG_SND_SSCAPE) += snd-pcm.o snd-timer.o snd.o snd-rawmidi.o snd-hwdep.o
obj-$(CONFIG_SND_MSND_PINNACLE) += snd-pcm.o snd-timer.o snd.o snd-rawmidi.o
obj-$(CONFIG_SND_AZT3328) += snd-pcm.o snd-timer.o snd.o snd-rawmidi.o snd-hwdep.o
obj-$(CONFIG_SND_VXPOCKET) += snd-pcm.o snd-timer.o snd.o
obj-$(CONFIG_SND_VXP440) += snd-pcm.o snd-timer.o snd.o
obj-$(CONFIG_SND_VX222) += snd-pcm.o snd-timer.o snd.o
obj-$(CONFIG_SND_HARMONY) += snd-pcm.o snd-timer.o snd.o

export-objs := sound.o pcm.o pcm_lib.o rawmidi.o timer.o hwdep.o

ifeq ($(CONFIG_PCI),y)
export-objs += pcm_sgbuf.o
endif

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/core/Makefile

include $(TOPDIR)/Rules.make
