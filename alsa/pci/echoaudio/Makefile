#
# Makefile for ALSA Echoaudio soundcard drivers
# Copyright (c) 2003 by Giuliano Pochini <pochini@shiny.it>
#

ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

snd-darla20-objs := darla20.o
snd-gina20-objs := gina20.o
snd-layla20-objs := layla20.o
snd-darla24-objs := darla24.o
snd-gina24-objs := gina24.o
snd-layla24-objs := layla24.o
snd-mona-objs := mona.o
snd-mia-objs := mia.o
snd-gina3g-objs := gina3g.o
snd-layla3g-objs := layla3g.o
snd-indigo-objs := indigo.o
snd-indigoio-objs := indigoio.o
snd-indigodj-objs := indigodj.o


# Toplevel Module Dependency
obj-$(CONFIG_SND_DARLA20) += snd-darla20.o
obj-$(CONFIG_SND_GINA20) += snd-gina20.o
obj-$(CONFIG_SND_LAYLA20) += snd-layla20.o
obj-$(CONFIG_SND_DARLA24) += snd-darla24.o
obj-$(CONFIG_SND_GINA24) += snd-gina24.o
obj-$(CONFIG_SND_LAYLA24) += snd-layla24.o
obj-$(CONFIG_SND_MONA) += snd-mona.o
obj-$(CONFIG_SND_MIA) += snd-mia.o
obj-$(CONFIG_SND_GINA3G) += snd-gina3g.o
obj-$(CONFIG_SND_LAYLA3G) += snd-layla3g.o
obj-$(CONFIG_SND_INDIGO) += snd-indigo.o
obj-$(CONFIG_SND_INDIGOIO) += snd-indigoio.o
obj-$(CONFIG_SND_INDIGODJ) += snd-indigodj.o

include $(SND_TOPDIR)/Rules.make

