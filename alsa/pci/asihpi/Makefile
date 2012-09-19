ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

snd-asihpi-objs := asihpi.o hpimod.o \
	hpicmn.o hpifunc.o hpidebug.o hpidspcd.o \
	hpios_linux.o hpi4000.o hpi6000.o hpi56301.o hpi6205.o hpimsgx.o

obj-$(CONFIG_SND_ASIHPI) += snd-asihpi.o

include $(SND_TOPDIR)/Rules.make

