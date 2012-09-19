TOPDIR = ..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

#
# ALSA extra code
#
ifeq (y,$(CONFIG_X86))
extra-subdir-y	:= msnd
else
ifeq (y,$(CONFIG_ALPHA))
extra-subdir-y	:= msnd
endif
endif
extra-subdir-m	:= $(extra-subdir-y)

isapnp-files := als100.isapnp azt2320.isapnp dt019x.isapnp opl3sa2.isapnp \
		cmi8330.isapnp es18xx.isapnp sscape.isapnp

#
#
#

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/isa/Makefile

include $(TOPDIR)/Rules.make
