INCSRC=../../alsa-kernel/include

ifeq (.includes,$(wildcard .includes))
include .includes
endif

version.h: ../version.h
	ln -s $<

%.h: %.patch
	cp $(INCSRC)/$@ .
	patch -p0 -i $< $@

%.h:
	ln -s $(INCSRC)/$@

.includes.tmp:
	@echo -n "HEADERS=" > $@
	@echo $(INCSRC)/*.h | sed -es@$(INCSRC)/@@g >> $@

.includes: .includes.tmp
	@if [ -f $@ ]; then \
	 cmp -s $@ $< || mv -f $< $@; \
	else \
	 mv -f $< $@; \
	fi; \
	rm -f $<

.PHONY: prepare prepare2
prepare2: $(HEADERS)

prepare: .includes
	$(MAKE) prepare2

clean:
	rm -f *.h
	rm -f .includes*
