TOPDIR = ..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

TOPDIR = $(MAINSRCDIR)

#(not used yet) 
##include $(TOPDIR)/alsa-kernel/arm/Makefile

O_TARGET    := arm.o

list-multi  := snd-h3600-uda1341.o 
#snd-sa1100.o

snd-h3600-uda1341-objs := h3600-uda1341.o
#snd-sa1100-objs := sa1100-audio.o

# Toplevel Module Dependency
obj-$(CONFIG_SND_H3600_UDA1341) += snd-h3600-uda1341.o 
#snd-sa1100.o

include $(TOPDIR)/Rules.make

snd-h3600-uda1341.o: $(snd-h3600-uda1341-objs)
	$(LD) $(LD_RFLAG) -r -o $@ $(snd-h3600-uda1341-objs)

#snd-sa1100.o: $(snd-sa1100-objs)
#	$(LD) $(LD_RFLAG) -r -o $@ $(snd-sa1100-objs)
