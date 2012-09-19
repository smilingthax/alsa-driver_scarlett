TOPDIR = ..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

obj-$(CONFIG_SND_H3600_UDA1341) += snd-pcm.o snd-timer.o snd.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/core/Makefile
