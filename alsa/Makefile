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

SUBDIRS  = support kernel lowlevel cards detect
CSUBDIRS = include test utils

all: compile

include/isapnp.h:
	ln -sf ../support/isapnp.h include/isapnp.h

compile: $(PEXPORT) include/isapnp.h cards.config
	@for d in $(SUBDIRS); do if ! $(MAKE) -C $$d; then exit 1; fi; done
	@echo
	@echo "ALSA modules were sucessfully compiled."
	@echo

cards.config: modules.config
	make -C utils update-deps
	make clean
	./cvscompile
	@echo "You can ignore following error..."
	exit 1

map:
	awk "{ if ( length( $$1 ) != 0 ) print $$1 }" snd.map | sort -o snd.map1
	mv -f snd.map1 snd.map

install: compile
	mkdir -p $(moddir)
	rm -f $(moddir)/snd*.o $(moddir)/persist.o $(moddir)/isapnp.o
	cp modules/*.o $(moddir)
	/sbin/depmod -a $(kaversion)
	install -m 755 -d $(prefix)/include/linux
	install -m 644 include/asound.h $(prefix)/include/linux
	install -m 644 include/asoundid.h $(prefix)/include/linux
	install -m 644 include/asequencer.h $(prefix)/include/linux
	if [ -x /etc/rc.d/init.d/alsasound ]; then \
	  install -m 755 utils/alsasound /etc/rc.d/init.d/alsasound; \
	fi
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
	chmod 755 utils/alsasound
	mv ../alsa-driver ../alsa-driver-$(SND_VERSION)
	tar --exclude=CVS -cvz -C .. -f ../alsa-driver-$(SND_VERSION).tar.gz alsa-driver-$(SND_VERSION)
	mv ../alsa-driver-$(SND_VERSION) ../alsa-driver

$(PEXPORT): $(TOPDIR)/utils/export-symbols.c
	make -C $(TOPDIR)/utils export-symbols
