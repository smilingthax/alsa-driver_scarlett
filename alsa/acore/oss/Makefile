TOPDIR = ../..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

EXTRA_CLEAN = mixer_oss.c pcm_oss.c

export-objs  := mixer_oss.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/core/oss/Makefile

EXTRA_CFLAGS = -I$(TOPDIR)/alsa-kernel/core/oss

include $(TOPDIR)/Rules.make

mixer_oss.c: mixer_oss.patch $(TOPDIR)/alsa-kernel/core/oss/mixer_oss.c
pcm_oss.c: pcm_oss.patch $(TOPDIR)/alsa-kernel/core/oss/pcm_oss.c
