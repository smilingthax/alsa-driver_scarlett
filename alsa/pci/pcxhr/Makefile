ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

snd-pcxhr-objs := pcxhr.o pcxhr_hwdep.o pcxhr_mixer.o pcxhr_core.o

obj-$(CONFIG_SND_PCXHR) += snd-pcxhr.o

include $(SND_TOPDIR)/Rules.make
