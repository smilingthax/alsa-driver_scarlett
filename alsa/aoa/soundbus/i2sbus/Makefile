ifndef SND_TOPDIR
SND_TOPDIR=../../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := control.c core.c

include $(SND_TOPDIR)/alsa-kernel/aoa/soundbus/i2sbus/Makefile

include $(SND_TOPDIR)/Rules.make

control.c: control.patch $(SND_TOPDIR)/alsa-kernel/aoa/soundbus/i2sbus/control.c
core.c: core.patch $(SND_TOPDIR)/alsa-kernel/aoa/soundbus/i2sbus/core.c
