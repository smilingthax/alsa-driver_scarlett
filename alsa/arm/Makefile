ifndef SND_TOPDIR
SND_TOPDIR=..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

include $(SND_TOPDIR)/alsa-kernel/arm/Makefile

snd-pxa2xx-pcm-objs := pxa2xx-pcm.o
snd-pxa2xx-ac97-objs := pxa2xx-ac97.o

obj-$(CONFIG_SND_PXA2XX_PCM) += snd-pxa2xx-pcm.o
obj-$(CONFIG_SND_PXA2XX_AC97) += snd-pxa2xx-ac97.o

include $(SND_TOPDIR)/Rules.make
