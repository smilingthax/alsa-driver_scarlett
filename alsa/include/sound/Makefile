INCSRC=../../alsa-kernel/include

ifeq (.includes,$(wildcard .includes))
include .includes
endif

version.h: ../version.h
	ln -s $<

%.h: %.patch
	cp $(INCSRC)/$@ .
	patch -p0 -i $< $@

%.h: $(INCSRC)/%.h
	ln -s $<

.PHONY: prepare1 prepare2 prepare
prepare1:
	@rm -f .includes
	@echo -n "HEADERS=" > .includes
	@echo $(INCSRC)/*.h | sed -es@$(INCSRC)/@@g >> .includes

prepare2: $(HEADERS)

prepare: prepare1
	$(MAKE) prepare2

clean:
	rm -f *.h
	rm -f .includes
