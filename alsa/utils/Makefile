#
# Makefile for ALSA driver
# Copyright (c) 1994-98 by Jaroslav Kysela <perex@suse.cz>
#

TOPDIR  = ..

ifeq ($(TOPDIR)/Makefile.conf,$(wildcard $(TOPDIR)/Makefile.conf))
include $(TOPDIR)/Makefile.conf
else
CC = gcc
INCLUDE = ../include
endif

TARGETS = snd-deps

.c.o:
	$(CC) $(INCLUDE) -c -o $*.o $<

all: $(TARGETS)

snd-deps: snd-deps-find.o snd-deps-output.o
	$(CC) $(INCLUDE) snd-deps-find.o snd-deps-output.o -o $@ 

../acinclude.m4: snd-deps ../modules.config
	./snd-deps --acinclude > ../acinclude.m4

../cards.config.in: snd-deps ../modules.config
	./snd-deps --makefile > ../cards.config.in

../include/config1.h.in: snd-deps ../modules.config
	./snd-deps --cinclude > ../include/config1.h.in

update-deps: ../acinclude.m4 ../cards.config.in ../include/config1.h.in

kredirect: kredirect.o
	$(CC) $(INCLUDE) kredirect.o -o $@

clean:
	rm -f core .depend *.o *.orig *~

.depend:
	$(CPP) $(INCLUDE) *.c > .depend

dummy:

#
# include a dependency file if one exists
#
ifeq (.depend,$(wildcard .depend))
include .depend
endif
