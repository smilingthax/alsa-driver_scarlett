ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

snd-asihpi-lib-objs := hpimod.o \
	hpiman.o hpifunc.o hpidebug.o hpidspcd.o \
	hpios_linux.o hpi4000.o hpi6000.o hpi56301.o hpi6205.o hpimsgx.o
snd-asihpi-objs := asihpi.o

obj-$(CONFIG_SND_ASIHPI) += snd-asihpi.o snd-asihpi-lib.o

include $(SND_TOPDIR)/Rules.make
