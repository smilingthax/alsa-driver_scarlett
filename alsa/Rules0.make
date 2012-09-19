#
# Makefile for ALSA low level driver (Linux version)
# Copyright (c) 1994-98 by Jaroslav Kysela <perex@suse.cz>
#

.SUFFIXES:
.SUFFIXES: .c .s .S .o .sym .nsym

.c.o:
	$(CC) $(COPTS) $(INCLUDE) -c -o $*.o $<

.sym.c:
	$(PEXPORT) $<
	rm -f $(DEXPORT)/$*.ver
	$(CC) $(INCLUDE) -E -D__GENKSYMS__ $*.c | $(GENKSYMS) > $(DEXPORT)/$*.ver

.nsym.c:
	$(PEXPORT) -n $<
