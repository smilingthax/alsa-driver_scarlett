#
# Makefile for ALSA driver
# Copyright (c) 1994-98 by Jaroslav Kysela <perex@suse.cz>
#

include ../Makefile.conf

TARGETS=coding.txt \
        coding.html

all: $(TARGETS)

coding.txt: coding.sgml
	sgml2txt coding.sgml

coding.html: coding.sgml
	sgml2html coding.sgml

clean:
	rm -f core .depend *.orig *~
