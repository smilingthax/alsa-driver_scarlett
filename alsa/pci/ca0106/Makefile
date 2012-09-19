ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

snd-ca0106-objs := ca0106_main.o ca0106_proc.o ca0106_mixer.o

obj-$(CONFIG_SND_CA0106) += snd-ca0106.o

include $(SND_TOPDIR)/Rules.make
