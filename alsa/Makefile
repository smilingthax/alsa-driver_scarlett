#
# Makefile for ALSA driver
# Copyright (c) 1994-98 by Jaroslav Kysela <perex@suse.cz>
#

TOPDIR   = .
ALSAKERNELDIR = ../alsa-kernel

ifndef IGROUP
IGROUP = root
endif
ifndef IUSER
IUSER = root
endif

ifeq (Makefile.conf,$(wildcard Makefile.conf))
include Makefile.conf
else
.PHONY: dummy1
dummy1:
	$(MAKE) all-deps
	@echo
	@echo "Please, run the configure script as first..."
	@echo
endif

ifeq (,$(wildcard acinclude.m4))
.PHONY: dummy2
dummy2:
	$(MAKE) all-deps
	@echo
	@echo "Please, run the configure script as first..."
	@echo
endif

SUBDIRS  = support acore i2c drivers isa synth
CSUBDIRS =

ifeq (y,$(CONFIG_PCI))
SUBDIRS  += pci
endif
ifeq (y,$(CONFIG_ARM))
SUBDIRS  += arm
endif
ifeq (y,$(CONFIG_PPC))
SUBDIRS  += ppc
endif
ifeq (y,$(CONFIG_SGI))
SUBDIRS  += hal2
endif
ifeq (y,$(CONFIG_USB))
SUBDIRS  += usb
endif
ifeq (y,$(CONFIG_PCMCIA))
SUBDIRS  += pcmcia
endif
ifeq (y,$(CONFIG_PARISC))
SUBDIRS  += parisc
endif
CSUBDIRS += include test utils

.PHONY: all
all: compile

alsa-kernel/sound_core.c:
	ln -sf $(ALSAKERNELDIR) alsa-kernel
	ln -sf alsa-kernel sound
	ln -sf alsa-kernel/scripts scripts

include/sound/version.h: include/version.h
	if [ ! -d include/sound -a ! -L include/sound ]; then \
	  ln -sf ../alsa-kernel/include include/sound ; \
	fi
	cp -auvf include/version.h include/sound/version.h

utils/mod-deps: alsa-kernel/scripts/mod-deps.c alsa-kernel/scripts/mod-deps.h
	gcc -Ialsa-kernel/scripts alsa-kernel/scripts/mod-deps.c -o utils/mod-deps

toplevel.config.in: alsa-kernel/sound_core.c utils/mod-deps alsa-kernel/scripts/Modules.dep utils/Modules.dep
	cat alsa-kernel/scripts/Modules.dep utils/Modules.dep | utils/mod-deps --makeconf > toplevel.config.in

acinclude.m4: alsa-kernel/sound_core.c utils/mod-deps alsa-kernel/scripts/Modules.dep utils/Modules.dep
	cat alsa-kernel/scripts/Modules.dep utils/Modules.dep | utils/mod-deps --acinclude > acinclude.m4

include/config1.h.in: alsa-kernel/sound_core.c utils/mod-deps alsa-kernel/scripts/Modules.dep utils/Modules.dep
	cat alsa-kernel/scripts/Modules.dep utils/Modules.dep | utils/mod-deps --include > include/config1.h.in

all-deps: toplevel.config.in acinclude.m4 include/config1.h.in

include/sndversions.h:
	make dep

.PHONY: compile
compile: include/sound/version.h include/sndversions.h
	@for d in $(SUBDIRS); do if ! $(MAKE) -C $$d; then exit 1; fi; done
	@echo
	@echo "ALSA modules were successfully compiled."
	@echo

.PHONY: dep
dep: include/sound/version.h
	@for d in $(SUBDIRS); do if ! $(MAKE) -C $$d fastdep; then exit 1; fi; done

.PHONY: map
map:
	awk "{ if ( length( $$1 ) != 0 ) print $$1 }" snd.map | sort -o snd.map1
	mv -f snd.map1 snd.map

.PHONY: install
install: install-modules install-headers install-scripts check-snd-prefix
	@if [ -L /dev/snd ]; then \
		echo "The ALSA devices were removed from /proc/asound/dev directory." ; \
		echo "Creating static device entries in /dev/snd." ; \
		$(TOPDIR)/snddevices ; \
	fi
	cat WARNING

.PHONY: install-headers
install-headers:
	if [ -L $(DESTDIR)$(prefix)/include/sound ]; then \
		rm -f $(DESTDIR)$(prefix)/include/sound; \
		ln -sf $(MAINSRCDIR)/include/sound $(DESTDIR)$(prefix)/include/sound; \
	else \
		rm -rf $(DESTDIR)$(prefix)/include/sound; \
		install -d -m 755 -g $(IGROUP) -o $(IUSER) $(DESTDIR)$(prefix)/include/sound; \
		for f in include/sound/*.h; do \
			install -m 644 -g $(IGROUP) -o $(IUSER) $$f $(DESTDIR)$(prefix)/include/sound; \
		done \
	fi

ifeq ($(CONFIG_SND_KERNELDIR)/System.map,$(wildcard $(CONFIG_SND_KERNELDIR)/System.map))
SYSTEM_MAP_OPT = -F $(CONFIG_SND_KERNELDIR)/System.map
endif

.PHONY: install-modules
install-modules:
ifeq ($(moddir_tree),y)
	rm -rf $(DESTDIR)$(moddir)
else
	rm -f $(DESTDIR)$(moddir)/snd*.o $(DESTDIR)$(moddir)/persist.o $(DESTDIR)$(moddir)/isapnp.o
endif
	@for d in $(SUBDIRS); do if ! $(MAKE) -C $$d modules_install; then exit 1; fi; done
ifeq ($(DESTDIR),)
	-/sbin/depmod -a $(kaversion) $(SYSTEM_MAP_OPT)
else
	-/sbin/depmod -a -b $(DESTDIR)/ $(SYSTEM_MAP_OPT) $(kaversion)
endif

.PHONY: install-scripts
install-scripts:
	if [ -d $(DESTDIR)/sbin/init.d ]; then \
	  install -m 755 -g $(IGROUP) -o $(IUSER) utils/alsasound $(DESTDIR)/sbin/init.d/alsasound; \
	elif [ -d $(DESTDIR)/etc/rc.d/init.d ]; then \
	  install -m 755 -g $(IGROUP) -o $(IUSER) utils/alsasound $(DESTDIR)/etc/rc.d/init.d/alsasound; \
	elif [ -d $(DESTDIR)/etc/init.d ]; then \
	  install -m 755 -g $(IGROUP) -o $(IUSER) utils/alsasound $(DESTDIR)/etc/init.d/alsasound; \
	fi

.PHONY: check-snd-prefix
check-snd-prefix:
	@ if [ x"$(DESTDIR)" = x ]; then \
	  if modprobe -c | grep -s -q snd_; then \
	    echo;\
	    echo "             ===== WARNING =====";\
	    echo;\
	    echo "The options for ALSA modules on your system still include snd_ prefix,";\
	    echo "which is obsoleted now.  Please fix /etc/modules.conf.";\
	    echo "For convenience, you can use utils/module-options script to convert";\
	    echo "the snd_ prefix automatically.";\
	    echo;\
	fi; fi

.PHONY: clean1
clean1:
	rm -f .depend *.o snd.map*
	rm -f $(DEXPORT)/*.ver
	rm -f modules/*.o

.PHONY: clean
clean: clean1
	@for d in $(SUBDIRS); do if ! $(MAKE) -C $$d clean; then exit 1; fi; done
	@for d in $(CSUBDIRS); do if ! $(MAKE) -C $$d clean; then exit 1; fi; done

.PHONY: mrproper
mrproper: clean1
	@for d in $(SUBDIRS); do if ! $(MAKE) -C $$d mrproper; then exit 1; fi; done
	@for d in $(CSUBDIRS); do if ! $(MAKE) -C $$d mrproper; then exit 1; fi; done
	rm -f *~ out.txt *.orig *.rej .#* .gdb_history
	rm -f doc/*~
	rm -f config.cache config.log config.status Makefile.conf
	rm -f utils/alsa-driver.spec
	rm -f `find ../alsa-kernel -name "*~"`
	rm -f `find ../alsa-kernel -name "*.orig"`
	rm -f `find ../alsa-kernel -name "*.rej"`
	rm -f `find ../alsa-kernel -name ".#*"`
	rm -f `find ../alsa-kernel -name "out.txt"`

.PHONY: cvsclean
cvsclean: mrproper
	rm -f configure snddevices aclocal.m4 acinclude.m4 include/config.h include/config1.h \
	include/config1.h.in toplevel.config toplevel.config.in \
	alsa-kernel sound scripts include/sound
	rm -rf include/linux

.PHONY: pack
pack: mrproper
	chmod 755 utils/alsasound
	# big note: use always '--bzip2 -p' where -p is fake argument
	# it seems that some older tar has wrong getopt list
	{ \
		cd .. ; \
		rm -f alsa-driver/alsa-kernel ; \
		mv alsa-kernel alsa-driver ; \
		mv alsa-driver alsa-driver-$(CONFIG_SND_VERSION) ; \
		tar --exclude=CVS --exclude=kchanges \
                    --owner=$(IGROUP) --group=$(IUSER) -cv --bzip2 -p \
                    -f alsa-driver-$(CONFIG_SND_VERSION).tar.bz2 alsa-driver-$(CONFIG_SND_VERSION) ; \
		mv alsa-driver-$(CONFIG_SND_VERSION) alsa-driver ; \
		mv alsa-driver/alsa-kernel . ; \
	}

.PHONY: uninstall
uninstall:
	rm -rf $(DESTDIR)$(prefix)/include/sound
ifeq ($(moddir_tree),y)
	rm -rf $(DESTDIR)$(moddir)
else
	rm -f $(DESTDIR)$(moddir)/snd*.o $(DESTDIR)$(moddir)/persist.o $(DESTDIR)$(moddir)/isapnp.o
endif
	rm -f $(DESTDIR)/sbin/init.d/alsasound
	rm -f $(DESTDIR)/etc/rc.d/init.d/alsasound
	rm -f $(DESTDIR)/etc/init.d/alsasound

.PHONY: TAGS
TAGS:
	find . ../alsa-kernel -name *.h -o -name *.c | xargs etags
