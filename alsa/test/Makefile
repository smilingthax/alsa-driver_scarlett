CC     	= gcc
CFLAGS	= -O2 -Wall -pipe
TARGETS = mixer pcm mmap_test osspcm

all: $(TARGETS)

mixer: mixer.c
	$(CC) $(CFLAGS) -o mixer mixer.c

pcm: pcm.c
	$(CC) $(CFLAGS) -o pcm pcm.c

osspcm: osspcm.c
	$(CC) $(CFLAGS) -o osspcm osspcm.c

mmap_test: mmap_test.c
	$(CC) $(CFLAGS) -DVERBOSE -o mmap_test mmap_test.c

clean:
	$(MAKE) -C seq clean
	rm -f *.o $(TARGETS) *~
