ifndef SND_TOPDIR
SND_TOPDIR=..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

include $(SND_TOPDIR)/alsa-kernel/firewire/Makefile

EXTRA_CFLAGS += -I$(SND_TOPDIR)/alsa-kernel/firewire

include $(SND_TOPDIR)/Rules.make
