#
# This file contains rules which are shared between multiple Makefiles.
#
# It's a stripped and modified version of /usr/src/linux/Rules.make. [--jk]
#

MODCURDIR = $(subst $(MAINSRCDIR)/,,$(shell /bin/pwd))

ifdef KBUILD_MODULES

# clean obsolete definitions
export-objs :=

# apply patches beforehand
.PHONY: cleanup
cleanup:
	rm -f *.[oas] *.ko .*.cmd .*.d .*.tmp *.mod.c $(clean-files)
	@for d in $(patsubst %/,%,$(filter %/, $(obj-y))) \
	          $(patsubst %/,%,$(filter %/, $(obj-m))) DUMMY; do \
	 if [ $$d != DUMMY ]; then $(MAKE) -C $$d cleanup || exit 1; fi; \
	done

else # ! KBUILD_MODULES

first_rule: modules

include $(MAINSRCDIR)/Rules.make1

%.c: %.patch
	@SND_TOPDIR="$(MAINSRCDIR)" $(SND_TOPDIR)/utils/patch-alsa $@

# apply patches beforehand
.PHONY: prepare
prepare: $(clean-files)
	@for d in $(ALL_SUB_DIRS) DUMMY; do \
	 if [ $$d != DUMMY ]; then $(MAKE) -C $$d prepare || exit 1; fi; \
	done

modules:
	$(MAKE) prepare
	$(MAKE) -C $(CONFIG_SND_KERNELDIR) SUBDIRS=$(shell /bin/pwd) $(MAKE_ADDS) SND_TOPDIR=$(MAINSRCDIR) modules
	$(SND_TOPDIR)/utils/link-modules $(MODCURDIR)

ALL_MOBJS := $(filter-out $(obj-y), $(obj-m))
ALL_MOBJS := $(filter-out %/, $(ALL_MOBJS))
modules_install:
ifneq "$(strip $(ALL_MOBJS))" ""
	mkdir -p $(DESTDIR)$(moddir)/$(MODCURDIR)
	cp $(sort $(ALL_MOBJS:.o=.ko)) $(DESTDIR)$(moddir)/$(MODCURDIR)
endif
	@for d in $(ALL_SUB_DIRS) DUMMY; do \
	 if [ $$d != DUMMY ]; then $(MAKE) -C $$d modules_install || exit 1; fi; \
	done

endif # KBUILD_MODULES
