ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

# Hack for FM801 && TEA575x
ifeq (y,$(CONFIG_VIDEO_DEV))
ifeq (m,$(CONFIG_SND_FM801))
EXTRA_CFLAGS += -DCONFIG_SND_FM801_TEA575X
CONFIG_SND_FM801_TEA575X=m
endif
endif


export-objs += ak4xxx-adda.o ak4117.o tea575x-tuner.o

snd-ak4117-objs := ak4117.o

# Toplevel Module Dependency
obj-$(CONFIG_SND_PDAUDIOCF) += snd-ak4117.o

include $(SND_TOPDIR)/alsa-kernel/i2c/other/Makefile

include $(SND_TOPDIR)/Rules.make
