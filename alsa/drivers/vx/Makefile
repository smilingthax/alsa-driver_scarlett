TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

#
#
#

TOPDIR = $(MAINSRCDIR)

export-objs := vx_core.o

snd-vx-lib-objs := vx_core.o vx_pcm.o vx_mixer.o vx_cmd.o vx_uer.o

obj-$(CONFIG_SND_VXPOCKET) += snd-vx-lib.o
obj-$(CONFIG_SND_VXP440) += snd-vx-lib.o
obj-$(CONFIG_SND_VX222) += snd-vx-lib.o

include $(TOPDIR)/Rules.make

