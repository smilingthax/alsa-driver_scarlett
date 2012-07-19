ifndef SND_TOPDIR
SND_TOPDIR=../..
endif

include $(SND_TOPDIR)/toplevel.config
include $(SND_TOPDIR)/Makefile.conf

clean-files := seq.c seq_clientmgr.c

include $(SND_TOPDIR)/alsa-kernel/core/seq/Makefile

EXTRA_CFLAGS += -I$(SND_TOPDIR)/alsa-kernel/core/seq

include $(SND_TOPDIR)/Rules.make

seq.c: seq.patch $(SND_TOPDIR)/alsa-kernel/core/seq/seq.c
seq_clientmgr.c: seq_clientmgr.patch $(SND_TOPDIR)/alsa-kernel/core/seq/seq_clientmgr.c
