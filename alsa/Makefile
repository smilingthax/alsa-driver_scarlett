#
# Makefile for ALSA driver
# Copyright (c) 1994-98 by Jaroslav Kysela <perex@suse.cz>
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

SUBDIRS  =
CSUBDIRS =

ifeq (0,$(CONFIG_ISAPNP_KERNEL))
SUBDIRS  += support
endif

SUBDIRS  += kernel lowlevel cards
CSUBDIRS += include test utils


all: compile

include/sndversions.h:
	make dep

include/isapnp.h:
	ln -sf ../support/isapnp.h include/isapnp.h

/usr/include/byteswap.h:
	if [ ! -r /usr/include/byteswap.h ]; then \
	  cp utils/patches/byteswap.h /usr/include ; \
	fi

compile: /usr/include/byteswap.h include/sndversions.h include/isapnp.h cards.config
	@for d in $(SUBDIRS); do if ! $(MAKE) -C $$d; then exit 1; fi; done
	@echo
	@echo "ALSA modules were sucessfully compiled."
	@echo

dep: /usr/include/byteswap.h include/isapnp.h cards.config
	@for d in $(SUBDIRS); do if ! $(MAKE) -C $$d dep; then exit 1; fi; done

cards.config: modules.config
	make -C utils update-deps
	make clean
	./cvscompile
	@echo "You may ignore following error..."
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
	install -m 644 include/ainstr_simple.h $(prefix)/include/linux
	install -m 644 include/ainstr_gf1.h $(prefix)/include/linux
	install -m 644 include/ainstr_iw.h $(prefix)/include/linux
	if [ -d /sbin/init.d ]; then \
	  install -m 755 utils/alsasound /sbin/init.d/alsasound; \
	elif [ -d /etc/rc.d/init.d ]; then \
	  install -m 755 utils/alsasound /etc/rc.d/init.d/alsasound; \
        elif [ -d /etc/init.d ]; then \
	  install -m 755 utils/alsasound /etc/init.d/alsasound; \
        fi
	cat WARNING

clean:
	rm -f `find . -name ".depend"`
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
	rm -f configure snddevices aclocal.m4 include/config.h include/isapnp.h

pack: mrproper
	chown -R root.root ../alsa-driver
	chmod 755 utils/alsasound
	mv ../alsa-driver ../alsa-driver-$(CONFIG_SND_VERSION)
	tar --exclude=CVS -cvI -C .. -f ../alsa-driver-$(CONFIG_SND_VERSION).tar.bz2 alsa-driver-$(CONFIG_SND_VERSION)
	mv ../alsa-driver-$(CONFIG_SND_VERSION) ../alsa-driver
