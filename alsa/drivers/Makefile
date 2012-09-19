ifndef SND_TOPDIR
SND_TOPDIR=..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

# ALSA extra definitions

snd-serialmidi-objs := serialmidi.o
snd-aloop-objs := aloop.o
snd-portman2x4-objs := portman2x4.o
snd-mts64-objs := mts64.o

obj-$(CONFIG_SND) += pcsp/

obj-$(CONFIG_SND_SERIALMIDI) += snd-serialmidi.o
obj-$(CONFIG_SND_LOOPBACK) += snd-aloop.o
obj-$(CONFIG_SND_PORTMAN2X4) += snd-portman2x4.o
obj-$(CONFIG_SND_MTS64) += snd-mts64.o

include $(SND_TOPDIR)/alsa-kernel/drivers/Makefile

include $(SND_TOPDIR)/Rules.make
