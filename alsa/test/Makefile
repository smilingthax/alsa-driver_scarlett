CC     	= gcc
CFLAGS	= -O2 -Wall -pipe
TARGETS = mixer pcm mmap_test osspcm osspcm1

all: $(TARGETS)

mixer: mixer.c
	$(CC) $(CFLAGS) -o mixer mixer.c

pcm: pcm.c
	$(CC) $(CFLAGS) -o pcm pcm.c

osspcm: osspcm.c
	$(CC) $(CFLAGS) -o osspcm osspcm.c

osspcm1: osspcm1.c
	$(CC) $(CFLAGS) -o osspcm1 osspcm1.c

mmap_test: mmap_test.c
	$(CC) $(CFLAGS) -DVERBOSE -o mmap_test mmap_test.c

clean:
	rm -f *.o $(TARGETS) *~
