TOPDIR = ..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

export-objs  := i2c.o cs8427.o tea6330t.o

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/i2c/Makefile

include $(TOPDIR)/Rules.make
