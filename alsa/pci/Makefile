TOPDIR = ..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

#
# ALSA extra code
#

extra-subdir-y	:= pdplus
extra-subdir-m	:= $(extra-subdir-y)

snd-azt3328-objs := azt3328.o
extra-obj-$(CONFIG_SND_AZT3328) += snd-azt3328.o

#
#
#

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/pci/Makefile

#
# ALSA extra rules
#
