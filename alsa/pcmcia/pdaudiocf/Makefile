ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

snd-pdaudiocf-objs := pdaudiocf.o pdaudiocf_core.o pdaudiocf_irq.o pdaudiocf_pcm.o

obj-$(CONFIG_SND_PDAUDIOCF) += snd-pdaudiocf.o

include $(SND_TOPDIR)/Rules.make
