#
# Makefile for ALSA low level driver (Linux version)
# Copyright (c) 1994-98 by Jaroslav Kysela <perex@jcu.cz>
#

$(TOPDIR)/include/sndversions.h: $(SYMFILES)
	@echo updating $(TOPDIR)/include/sndversions.h
ifeq (1,$(newkernel))
	@(echo "#ifndef _LINUX_SNDMODVERSIONS";\
	echo "#define _LINUX_SNDNODVERSIONS";\
	echo "#include <linux/modsetver.h>";\
	cd $(TOPDIR)/include/modules; \
        for f in *.ver; do \
          if [ -f $$f ]; then echo "#include \"modules/$${f}\""; fi; \
        done; \
	echo "#endif") \
	> $(TOPDIR)/include/sndversions.h
else
	@(echo "#ifdef MODVERSIONS";\
	echo "#undef  CONFIG_MODVERSIONS";\
	echo "#define CONFIG_MODVERSIONS";\
	echo "#ifndef _set_ver";\
	echo "#define _set_ver(sym,vers) sym ## _R ## vers";\
	echo "#endif";\
	cd $(TOPDIR)/include/modules; \
        for f in *.ver; do \
          if [ -f $$f ]; then echo "#include \"modules/$${f}\""; fi; \
        done; \
	echo "#undef  CONFIG_MODVERSIONS";\
	echo "#endif") \
	> $(TOPDIR)/include/sndversions.h
endif
