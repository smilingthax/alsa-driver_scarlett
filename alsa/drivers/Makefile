include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

# ALSA extra definitions

snd-serialmidi-objs := serialmidi.o

obj-$(CONFIG_SND_SERIALMIDI) += snd-serialmidi.o


include $(SND_TOPDIR)/alsa-kernel/drivers/Makefile

include $(SND_TOPDIR)/Rules.make
