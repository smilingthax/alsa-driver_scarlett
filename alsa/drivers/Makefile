TOPDIR = ..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

# ALSA extra definitions

extra-list-multi := snd-serialmidi.o

snd-serialmidi-objs := serialmidi.o

obj-$(CONFIG_SND_SERIALMIDI) += snd-serialmidi.o


TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/drivers/Makefile

# ALSA extra rules

snd-serialmidi.o: $(snd-serialmidi-objs)
	$(LD) $(LD_RFLAG) -r -o $@ $(snd-serialmidi-objs)
