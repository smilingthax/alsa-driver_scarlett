ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

snd-cmi8788-objs := cmi8788.o cmi_lib.o cmi_pcm.o cmi_mixer.o \
		    patch_ak4396.o patch_wm8785.o patch_cmi9780.o

obj-$(CONFIG_SND_CMI8788) += snd-cmi8788.o

include $(SND_TOPDIR)/Rules.make
