TOPDIR = ../..

CONFIG_SND_VXPOCKET=m
CONFIG_SND_VXP440=m

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

#
#
#

TOPDIR = $(MAINSRCDIR)

export-objs := vx_core.o

# vx_entry.o is included in each card module to avoid the dependency
# of snd-vxp-lib to pcmcia stuff

snd-vxpocket-objs := vxpocket.o vx_entry.o
snd-vxp440-objs := vxp440.o vx_entry.o
snd-vxp-lib-objs := vx_core.o vx_pcm.o vx_mixer.o vx_cmd.o vx_irq.o vx_uer.o

obj-$(CONFIG_SND_VXPOCKET) += snd-vxpocket.o snd-vxp-lib.o
obj-$(CONFIG_SND_VXP440) += snd-vxp440.o snd-vxp-lib.o

include $(TOPDIR)/Rules.make



