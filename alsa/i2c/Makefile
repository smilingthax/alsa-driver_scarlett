TOPDIR = ..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

ifeq ($(CONFIG_SND_ARM),y)
  extra-subdir-y := l3
  extra-subdir-m := $(extra-subdir-y)
endif

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/i2c/Makefile

