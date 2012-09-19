TOPDIR = ..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

obj-$(CONFIG_SND_USB_AUDIO) += snd-pcm.o snd-timer.o snd.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/core/Makefile
