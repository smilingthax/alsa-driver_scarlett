ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := core.c sysfs.c

include $(SND_TOPDIR)/alsa-kernel/aoa/soundbus/Makefile

include $(SND_TOPDIR)/Rules.make

core.c: core.patch $(SND_TOPDIR)/alsa-kernel/aoa/soundbus/core.c
sysfs.c: sysfs.patch $(SND_TOPDIR)/alsa-kernel/aoa/soundbus/sysfs.c
