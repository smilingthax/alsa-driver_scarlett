TOPDIR = ..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

# ALSA extra definitions

extra-subdir-y	:= vx
extra-subdir-m	:= $(extra-subdir-y)

snd-serialmidi-objs := serialmidi.o

obj-$(CONFIG_SND_SERIALMIDI) += snd-serialmidi.o


TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/drivers/Makefile

include $(TOPDIR)/Rules.make
