#
# Makefile for ALSA low level driver (Linux version)
# Copyright (c) 1994-98 by Jaroslav Kysela <perex@jcu.cz>
#

.SUFFIXES:
.SUFFIXES: .c .s .S .o .sym .nsym

.c.o:
	$(CC) $(COPTS) $(INCLUDE) -c -o $*.o $<

.sym.c:
	$(PEXPORT) $<
ifeq (1,$(newkernel))
	$(CC) $(INCLUDE) -E -D__GENKSYMS__ $*.c | $(GENKSYMS) > $(DEXPORT)/$*.ver
else
	$(CC) $(INCLUDE) -E -D__GENKSYMS__ $*.c | $(GENKSYMS) $(DEXPORT)
endif

.nsym.c:
	$(PEXPORT) -n $<
