#
# Makefile for ALSA driver
# Copyright (c) 1994-98 by Jaroslav Kysela <perex@suse.cz>
#

TOPDIR   = .

ifeq (Makefile.conf,$(wildcard Makefile.conf))
include Makefile.conf
else
dummy:
	make -C utils update-deps
	@echo
	@echo "Please, run the configure script as first..."
	@echo
endif

SUBDIRS  =
CSUBDIRS =

ifeq (0,$(CONFIG_ISAPNP_KERNEL))
ifeq (1,$(CONFIG_SND_ISA))
SUBDIRS  += support
endif
endif

SUBDIRS  += kernel lowlevel cards
CSUBDIRS += include test utils


all: compile

sound:
	ln -sf include sound

include/sndversions.h:
	make dep

include/isapnp.h:
	ln -sf ../support/isapnp.h include/isapnp.h

/usr/include/byteswap.h:
	if [ ! -r /usr/include/byteswap.h ]; then \
	  cp utils/patches/byteswap.h /usr/include ; \
	fi

compile: /usr/include/byteswap.h sound include/sndversions.h include/isapnp.h cards.config
	@for d in $(SUBDIRS); do if ! $(MAKE) -C $$d; then exit 1; fi; done
	@echo
	@echo "ALSA modules were successfully compiled."
	@echo

dep: /usr/include/byteswap.h sound include/isapnp.h cards.config
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

install: install-modules install-headers install-scripts
	cat WARNING

install-headers:
	if [ -L $(DESTDIR)$(prefix)/include/sound ]; then \
		ln -sf $(SRCDIR)/include $(DESTDIR)$(prefix)/include/sound; \
	else \
		rm -rf $(DESTDIR)$(prefix)/include/sound; \
		install -d -m 755 -g root -o root $(DESTDIR)$(prefix)/include/sound; \
		for f in include/*.h; do \
			install -m 755 -g root -o root $$f $(DESTDIR)$(prefix)/include/sound; \
		done \
	fi

install-modules: compile
	mkdir -p $(DESTDIR)$(moddir)
	rm -f $(DESTDIR)$(moddir)/snd*.o $(DESTDIR)$(moddir)/persist.o $(DESTDIR)$(moddir)/isapnp.o
	cp modules/*.o $(DESTDIR)$(moddir)
ifeq ($(DESTDIR),)
	/sbin/depmod -a
else
	/sbin/depmod -a -b $(DESTDIR)/
endif

install-scripts:
	if [ -d /sbin/init.d ]; then \
	  install -m 755 -g root -o root utils/alsasound $(DESTDIR)/sbin/init.d/alsasound; \
	elif [ -d /etc/rc.d/init.d ]; then \
	  install -m 755 -g root -o root utils/alsasound $(DESTDIR)/etc/rc.d/init.d/alsasound; \
	elif [ -d /etc/init.d ]; then \
	  install -m 755 -g root -o root utils/alsasound $(DESTDIR)/etc/init.d/alsasound; \
	fi

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
	chmod 755 utils/alsasound
	mv ../alsa-driver ../alsa-driver-$(CONFIG_SND_VERSION)
	tar --exclude=CVS --owner=root --group=root -cvI -C .. -f ../alsa-driver-$(CONFIG_SND_VERSION).tar.bz2 alsa-driver-$(CONFIG_SND_VERSION)
	mv ../alsa-driver-$(CONFIG_SND_VERSION) ../alsa-driver

uninstall:
	rm -rf $(DESTDIR)$(prefix)/include/sound
	rm -f $(DESTDIR)$(moddir)/snd*.o $(DESTDIR)$(moddir)/persist.o $(DESTDIR)$(moddir)/isapnp.o
	rm -f $(DESTDIR)/sbin/init.d/alsasound
	rm -f $(DESTDIR)/etc/rc.d/init.d/alsasound
	rm -f $(DESTDIR)/etc/init.d/alsasound

