#
# Makefile for ALSA driver
# Copyright (c) 1994-98 by Jaroslav Kysela <perex@jcu.cz>
#

TOPDIR  = ..

include $(TOPDIR)/Makefile.conf

TARGETS = export-symbols

include $(TOPDIR)/Rules.make

all: $(TARGETS)

export-symbols: .depend
	$(CC) $(COPTS) $(INCLUDE) -o $@ $@.c

clean:
	rm -f core .depend *.o *.orig *~ export-symbols

.depend: $(SYMFILES)
	$(CPP) $(COPTS) $(INCLUDE) *.c > .depend

dummy:

#
# include a dependency file if one exists
#
ifeq (.depend,$(wildcard .depend))
include .depend
endif
