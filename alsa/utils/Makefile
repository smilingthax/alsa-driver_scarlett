#
# Makefile for ALSA driver
# Copyright (c) 1994-98 by Jaroslav Kysela <perex@suse.cz>
#

TOPDIR  = ..

include $(TOPDIR)/Makefile.conf

TARGETS = export-symbols snd-deps

.c.o:
	$(CC) $(INCLUDE) -c -o $*.o $<

all: $(TARGETS)

export-symbols: .depend
	$(CC) $(INCLUDE) -o $@ $@.c

snd-deps: snd-deps-find.o snd-deps-output.o
	$(CC) $(INCLUDE) snd-deps-find.o snd-deps-output.o -o $@ 

update-deps: snd-deps
	./update-deps

clean:
	rm -f core .depend *.o *.orig *~ export-symbols

.depend: $(SYMFILES)
	$(CPP) $(INCLUDE) *.c > .depend

dummy:

#
# include a dependency file if one exists
#
ifeq (.depend,$(wildcard .depend))
include .depend
endif
