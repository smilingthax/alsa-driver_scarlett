ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

snd-au8810-objs := au8810.o au8810_core.o au8810_pcm.o au8810_mixer.o au8810_mpu401.o au8810_game.o au8810_eq.o
snd-au8820-objs := au8820.o au8820_core.o au8820_pcm.o au8820_mixer.o au8820_mpu401.o au8820_game.o au8820_synth.o
snd-au8830-objs := au8830.o au8830_core.o au8830_pcm.o au8830_mixer.o au8830_mpu401.o au8830_game.o au8830_eq.o au8830_synth.o

obj-$(CONFIG_SND_AU8810) += snd-au8810.o
obj-$(CONFIG_SND_AU8820) += snd-au8820.o
obj-$(CONFIG_SND_AU8830) += snd-au8830.o

include $(SND_TOPDIR)/Rules.make
