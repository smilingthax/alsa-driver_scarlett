#
# Makefile for ALSA low level driver (Linux version)
# Copyright (c) 1994-98 by Jaroslav Kysela <perex@suse.cz>
#

$(DEXPORT)/%.ver: %.c
	rm -f $(DEXPORT)/$*.ver
	$(CC) $(INCLUDE) -E -D__GENKSYMS__ $*.c | $(GENKSYMS) > $(DEXPORT)/$*.ver

$(addprefix $(DEXPORT)/,$(EXPORTS:.o=.ver)): $(TOPDIR)/include/config.h $(TOPDIR)/include/config1.h

$(TOPDIR)/include/sndversions.h: $(addprefix $(DEXPORT)/,$(EXPORTS:.o=.ver))
	@echo updating $(TOPDIR)/include/sndversions.h
	@(echo "#ifndef _LINUX_SNDMODVERSIONS";\
	echo "#define _LINUX_SNDNODVERSIONS";\
	echo "#include <linux/modsetver.h>";\
	cd $(TOPDIR)/include/modules; \
        for f in *.ver; do \
          if [ -f $$f ]; then echo "#include \"modules/$${f}\""; fi; \
        done; \
	echo "#endif") \
	> $(TOPDIR)/include/sndversions.h

$(EXPORTS): %.o: %.c
	$(CC) $(COPTS) $(INCLUDE) -DEXPORT_SYMTAB -c $(@:.o=.c)

ifdef SUBDIRS
dep fastdep: $(TOPDIR)/include/sndversions.h
	@for d in $(SUBDIRS); do if ! $(MAKE) -C $$d dep; then exit 1; fi; done
else
dep fastdep: $(TOPDIR)/include/sndversions.h
endif
