#
# Makefile for ALSA low level driver (Linux version)
# Copyright (c) 1994-98 by Jaroslav Kysela <perex@suse.cz>
#

TOPDIR  = ..

include $(TOPDIR)/Makefile.conf

TARGETS = $(SNDVERSIONS)
ifeq (1,$(CONFIG_ISAPNP))
ifeq (0,$(CONFIG_ISAPNP_KERNEL))
TARGETS+= $(TOPDIR)/modules/isapnp.o
EXPORTS+= isapnp.o
endif
endif

ISAPNP  = isapnp.o isapnp_proc.o isapnp_quirks.o

include $(TOPDIR)/Rules0.make

all: $(TARGETS)

include $(TOPDIR)/Rules1.make

$(TOPDIR)/modules/isapnp.o: .depend $(SNDVERSIONS) $(ISAPNP)
	$(LINKER) -o $@ $(ISAPNP)

$(TOPDIR)/modules/isapnp_test.o: .depend $(SNDVERSIONS) isapnp_test.o
	$(LINKER) -o $@ isapnp_test.o

gcompile: $(TARGETS)

clean:
	rm -f .depend *.o *.orig *.old *~
	rm -f core $(SNDVERSIONS)

.depend:
	$(CPP) $(COPTS) $(INCLUDE) *.c > .depend

dep:	.depend

dummy:

#
# include a dependency file if one exists
#
ifeq (.depend,$(wildcard .depend))
include .depend
endif
