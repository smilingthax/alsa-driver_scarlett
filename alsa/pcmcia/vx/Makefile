TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

#
#
#

TOPDIR = $(MAINSRCDIR)

snd-vxpocket-objs := vxpocket.o vx_entry.o vxp_ops.o vxp_mixer.o
snd-vxp440-objs := vxpocket.o vx_entry.o vxp_ops.o vxp_mixer.o

obj-$(CONFIG_SND_VXPOCKET) += snd-vxpocket.o
obj-$(CONFIG_SND_VXP440) += snd-vxp440.o

include $(TOPDIR)/Rules.make
