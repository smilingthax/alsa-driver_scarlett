TOPDIR = ..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

subdir-$(CONFIG_SND_ARM) := l3
subdir-m = $(subdir-y)

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/i2c/Makefile

