#
# Makefile for ALSA driver
# Copyright (c) 1994-98 by Jaroslav Kysela <perex@jcu.cz>
#

TOPDIR   = .

ifeq (Makefile.conf,$(wildcard Makefile.conf))
include Makefile.conf
else
dummy:
	@echo
	@echo "Please, run configure script as first..."
	@echo
endif

SUBDIRS  = kernel lowlevel cards detect
CSUBDIRS = include test utils

all: compile

compile: $(PEXPORT)
	@for d in $(SUBDIRS); do if ! $(MAKE) -C $$d; then exit 1; fi; done
	@echo
	@echo "ALSA modules were sucessfully compiled."
	@echo

map:
	awk "{ if ( length( $$1 ) != 0 ) print $$1 }" snd.map | sort -o snd.map1
	mv -f snd.map1 snd.map

install: compile
	mkdir -p $(moddir)
	rm -f $(moddir)/snd*.o $(moddir)/persist.o
	cp modules/* $(moddir)
	/sbin/depmod -a $(kversion).$(kpatchlevel).$(ksublevel)
	install -m 644 include/sound.h $(prefix)/include/linux
	install -m 644 include/sounddetect.h $(prefix)/include/linux
	cat WARNING

clean:
	@for d in $(SUBDIRS); do if ! $(MAKE) -C $$d clean; then exit 1; fi; done
	@for d in $(CSUBDIRS); do if ! $(MAKE) -C $$d clean; then exit 1; fi; done
	rm -f core .depend *.o snd.map* *~
	rm -f `find . -name "out.txt"`
	rm -f `find . -name "*.orig"`
	rm -f $(DEXPORT)/*.ver
	rm -f modules/*.o
	rm -f doc/*~

mrproper: clean
	rm -f config.cache config.log config.status Makefile.conf
	rm -f utils/alsa-driver.spec

cvsclean: mrproper
	rm -f configure snddevices include/config.h

pack: mrproper
	chown -R root.root ../alsa-driver
	tar cvz -C .. -f ../alsa-driver-$(SND_VERSION).tar.gz alsa-driver

$(PEXPORT): $(TOPDIR)/utils/export-symbols.c
	make -C $(TOPDIR)/utils export-symbols
