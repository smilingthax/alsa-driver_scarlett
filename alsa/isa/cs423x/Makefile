TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

TOPDIR = $(MAINSRCDIR)

snd-pc98-cs4232-objs := pc98.o
obj-$(CONFIG_SND_PC98_CS4232) += snd-pc98-cs4232.o snd-cs4231-lib.o
 
include $(TOPDIR)/alsa-kernel/isa/cs423x/Makefile
