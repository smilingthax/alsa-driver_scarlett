ifndef SND_TOPDIR
SND_TOPDIR=..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

include $(SND_TOPDIR)/alsa-kernel/sparc/Makefile

snd-sun-dbri-objs := dbri.o
obj-$(CONFIG_SND_SUN_DBRI) += snd-sun-dbri.o

include $(SND_TOPDIR)/Rules.make
