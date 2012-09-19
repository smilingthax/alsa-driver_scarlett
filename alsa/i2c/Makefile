ifndef SND_TOPDIR
SND_TOPDIR=..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

extra-subdir-y  := other
extra-subdir-m  := $(extra-subdir-y)

snd-uda1380-objs := uda1380.o
obj-$(CONFIG_SND_UDA1380) += snd-uda1380.o

include $(SND_TOPDIR)/alsa-kernel/i2c/Makefile

obj-$(CONFIG_L3) += l3/

include $(SND_TOPDIR)/Rules.make
