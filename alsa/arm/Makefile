ifndef SND_TOPDIR
SND_TOPDIR=..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

include $(SND_TOPDIR)/alsa-kernel/arm/Makefile

snd-sa11x0-pcm-objs		:= sa11x0-pcm.o devdma.o

obj-$(CONFIG_SND_S3C2410)	+= s3c24xx-iis.o

include $(SND_TOPDIR)/Rules.make
