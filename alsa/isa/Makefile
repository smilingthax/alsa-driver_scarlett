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

#
#
#

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/isa/Makefile
