CC     	= gcc
CFLAGS	= -O2 -Wall -pipe
TARGETS = csp_load mmap_test osspcm osspcm1 ossdelay

all: $(TARGETS)

csp_load: csp_load.c
	$(CC) $(CFLAGS) -o csp_load csp_load.c

osspcm: osspcm.c
	$(CC) $(CFLAGS) -o osspcm osspcm.c

osspcm1: osspcm1.c
	$(CC) $(CFLAGS) -o osspcm1 osspcm1.c

ossdelay: ossdelay.c
	$(CC) $(CFLAGS) -o ossdelay ossdelay.c

mmap_test: mmap_test.c
	$(CC) $(CFLAGS) -DVERBOSE -o mmap_test mmap_test.c

clean:
	rm -f *.o $(TARGETS) *~
