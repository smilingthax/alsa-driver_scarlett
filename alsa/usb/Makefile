TOPDIR = ..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

TOPDIR = $(MAINSRCDIR)

# for compatibility wrappers
export-objs := usbaudio.o

include $(TOPDIR)/alsa-kernel/usb/Makefile

include $(TOPDIR)/Rules.make
