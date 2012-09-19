#
# Makefile for ALSA
#

O_TARGET = l3.o

list-multi += snd-uda1341.o

export-objs += uda1341.o

snd-uda1341-objs := uda1341.o

# Module Dependency
obj-$(CONFIG_SND_SA11XX_UDA1341) += snd-uda1341.o

include $(TOPDIR)/Rules.make

snd-uda1341.o: $(snd-uda1341-objs)
	$(LD) $(LD_RFLAG) -r -o $@ $(snd-uda1341-objs)
