#
# Makefile for ALSA driver
# Copyright (c) 1994-98 by Jaroslav Kysela <perex@suse.cz>
#

TOPDIR  = ..

ifeq ($(TOPDIR)/Makefile.conf,$(wildcard $(TOPDIR)/Makefile.conf))
include $(TOPDIR)/Makefile.conf
else
CC = gcc
INCLUDE = -I../include
endif

TARGETS = dummy

.c.o:
	$(CC) $(INCLUDE) -c -o $*.o $<

all: $(TARGETS)

.PHONY: dummy
dummy:

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
