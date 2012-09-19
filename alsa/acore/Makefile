TOPDIR = ..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

obj-$(CONFIG_SND_SERIALMIDI) += snd-rawmidi.o snd.o
obj-$(CONFIG_SND_SSCAPE) += snd-pcm.o snd-timer.o snd.o snd-rawmidi.o snd-hwdep.o
obj-$(CONFIG_SND_MSND_PINNACLE) += snd-pcm.o snd-timer.o snd.o snd-rawmidi.o
obj-$(CONFIG_SND_AZT3328) += snd-pcm.o snd-timer.o snd.o snd-rawmidi.o snd-hwdep.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/core/Makefile
