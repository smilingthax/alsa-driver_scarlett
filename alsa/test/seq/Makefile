#
# Makefile for ALSA Sequencer test programs
# Copyright (c) 1998 by Frank van de Pol <F.K.W.van.de.Pol@inter.nl.net>
#

# let this point to the directory you ALSA driver source 
TOPDIR  = ../..

include $(TOPDIR)/Makefile.conf

TARGETS = $(SNDVERSIONS) \
	opn2 opn3 \
	usertest1 usertest2 \
	readtime \
	snd-client1.o \
	playmidi

include $(TOPDIR)/Rules.make

all: $(TARGETS) tags
	@echo
	@echo "ALSA sequencer test programs were sucessfully compiled."
	@echo

include $(TOPDIR)/Rules1.make

gcompile: $(TARGETS)


opn2: 	opn2.c
	cc $(COPTS) $(INCLUDE) -o $@ opn2.c

opn3: 	opn3.c
	cc $(COPTS) $(INCLUDE) -o $@ opn3.c

usertest1: 	usertest1.c
	cc $(COPTS) $(INCLUDE) -o $@ usertest1.c

usertest2: 	usertest2.c
	cc $(COPTS) $(INCLUDE)  -o $@ usertest2.c

readtime: 	readtime.c
	cc $(COPTS) $(INCLUDE)  -o $@ readtime.c -lcurses

snd-client1.o: .depend $(SNDVERSIONS) kernel-client1.o
	$(LINKER) -o $@ kernel-client1.o

playmidi:	playmidi.c midifile.c midifile.h
	cc -O2 $(INCLUDE) -o $@ playmidi.c midifile.c

ARCH_SOURCES = \
	Makefile\
	*.c *.h *.3 *.mid

snapshot: 
	tar czvf ../../../archive/snapshot-test-`date +%d%b%Y`.tar.gz $(ARCH_SOURCES)
	ls -l  ../../../archive/snapshot-*-`date +%d%b%Y`.tar.gz

ci:
	ci -l $(ARCH_SOURCES)


tags:	*.c
	ctags *.c

clean:
	rm -f .depend *.o *.orig snd.map snd.map? *.old *~
	rm -f core $(SNDVERSIONS) $(SYMFILES)

.depend: $(SYMFILES)
	$(CPP) $(COPTS) $(INCLUDE) *.c > .depend

dep:	.depend

dummy:

#
# include a dependency file if one exists
#
ifeq (.depend,$(wildcard .depend))
include .depend
endif
