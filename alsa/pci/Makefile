TOPDIR = ..

include $(TOPDIR)/toplevel.config
include $(TOPDIR)/Makefile.conf

#
# ALSA extra code
#

EXTRA_CLEAN = cs4281.c es1968.c intel8x0.c maestro3.c

extra-subdir-y	:= pdplus
extra-subdir-m	:= $(extra-subdir-y)

#
#
#

TOPDIR = $(MAINSRCDIR)
include $(TOPDIR)/alsa-kernel/pci/Makefile

include $(TOPDIR)/Rules.make

cs4281.c: cs4281.patch $(TOPDIR)/alsa-kernel/pci/cs4281.c
es1968.c: es1968.patch $(TOPDIR)/alsa-kernel/pci/es1968.c
intel8x0.c: intel8x0.patch $(TOPDIR)/alsa-kernel/pci/intel8x0.c
maestro3.c: maestro3.patch $(TOPDIR)/alsa-kernel/pci/maestro3.c

