#
# Makefile for ALSA low level driver (Linux version)
# Copyright (c) 1994-98 by Jaroslav Kysela <perex@jcu.cz>
#

TOPDIR  = ..

include $(TOPDIR)/Makefile.conf

ifeq (1,$(SNDCFG_MVERSION))
COPTS   += -DMODVERSIONS \
		-include $(SNDCFG_KERNELDIR)/include/linux/modversions.h \
		-include $(TOPDIR)/include/sndversions.h
endif
COPTS   += -D__KERNEL__ -DMODULE -DEXPORT_SYMTAB -DCONFIG_ISAPNP

TARGETS = $(SNDVERSIONS)
ifeq (1,$(SNDCFG_ISAPNP))
TARGETS+= $(TOPDIR)/modules/isapnp.o
endif
ISAPNP	= isapnp.o isapnp_proc.o
ISAPNPE	= $(DEXPORT)/isapnp.ver
SYMFILES = $(ISAPNPE)

include $(TOPDIR)/Rules.make

all: $(TARGETS)
	@echo
	@echo "Support code were sucessfully compiled."
	@echo

include $(TOPDIR)/Rules1.make

$(ISAPNPE): isapnp.c
ifeq (1,$(newkernel))
	$(CC) $(INCLUDE) -E -D__GENKSYMS__ isapnp.c | $(GENKSYMS) > $(DEXPORT)/isapnp.ver
else
	$(CC) $(INCLUDE) -E -D__GENKSYMS__ isapnp.c | $(GENKSYMS) $(DEXPORT)
endif

$(TOPDIR)/modules/isapnp.o: .depend $(SNDVERSIONS) $(ISAPNPE) $(ISAPNP)
	$(LINKER) -o $@ $(ISAPNP)

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
