/*
 * This is a simple program which demonstrates use of mmapped DMA buffer
 * of the sound driver directly from application program.
 *
 * This sample program works (currently) only with Linux, FreeBSD and BSD/OS
 * (FreeBSD and BSD/OS require OSS version 3.8-beta16 or later.
 *
 * Note! Don't use mmapped DMA buffers (direct audio) unless you have
 * very good reasons to do it. Programs using this feature will not
 * work with all soundcards. GUS (GF1) is one of them (GUS MAX works).
 *
 * This program requires version 3.5-beta7 or later of OSS
 * (3.8-beta16 or later in FreeBSD and BSD/OS).
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/soundcard.h>
#include <sys/time.h>

main()
{
	int fd, sz, fsz, i, tmp, n, l, have_data=0, nfrag;
        int caps, idx;

	int sd, sl=0, sp;

	unsigned char data[500000], *dp = data;

	struct buffmem_desc imemd, omemd;
        caddr_t buf;
	struct timeval tim;

	unsigned char *op;
	
        struct audio_buf_info info;

	int frag = 0xffff000c;	/* Max # periods of 2^13=8k bytes */

	fd_set writeset;

	close(0);
	if ((fd=open("/dev/dsp", O_RDWR, 0))==-1)
	{
		perror("/dev/dsp");
		exit(-1);
	}
/*
 * Then setup sampling parameters. Just sampling rate in this case.
 */

	tmp = 48000;
	ioctl(fd, SNDCTL_DSP_SPEED, &tmp);
	printf("Speed set to %d\n", tmp);

/*
 * Load some test data.
 */

  sl = sp = 0;
  if ((sd=open("smpl", O_RDONLY, 0))!=-1)
  {
	sl = read(sd, data, sizeof(data));
	printf("%d bytes read from file.\n", sl);
	close(sd);
  }
  else perror("smpl");

	if (ioctl(fd, SNDCTL_DSP_GETCAPS, &caps)==-1)
	{
		perror("/dev/dsp");
		fprintf(stderr, "Sorry but your sound driver is too old\n");
		exit(-1);
	}

/*
 * Check that the device has capability to do this. Currently just
 * CS4231 based cards will work.
 *
 * The application should also check for DSP_CAP_MMAP bit but this
 * version of driver doesn't have it yet.
 */
/*	ioctl(fd, SNDCTL_DSP_SETSYNCRO, 0); */

/*
 * You need version 3.5-beta7 or later of the sound driver before next
 * two lines compile. There is no point to modify this program to
 * compile with older driver versions since they don't have working
 * mmap() support.
 */
	if (!(caps & DSP_CAP_TRIGGER) ||
	    !(caps & DSP_CAP_MMAP))
	{
		fprintf(stderr, "Sorry but your soundcard can't do this\n");
		exit(-1);
	}

/*
 * Select the period size. This is propably important only when
 * the program uses select(). Period size defines how often
 * select call returns.
 */

	ioctl(fd, SNDCTL_DSP_SETPERIOD, &frag);

/*
 * Compute total size of the buffer. It's important to use this value
 * in mmap() call.
 */

	if (ioctl(fd, SNDCTL_DSP_GETOSPACE, &info)==-1)
	{
		perror("GETOSPACE");
		exit(-1);
	}

	sz = info.fragstotal * info.fragsize;
	fsz = info.fragsize;
	printf( "info.fragstotal = %i\n", info.fragstotal );
	printf( "info.fragsize = %i\n", info.fragsize );
	printf( "info.periods = %i\n", info.periods );
	printf( "info.bytes = %i\n", info.bytes );

/*
 * Call mmap().
 * 
 * IMPORTANT NOTE!!!!!!!!!!!
 *
 * Full duplex audio devices have separate input and output buffers. 
 * It is not possible to map both of them at the same mmap() call. The buffer
 * is selected based on the prot argument in the following way:
 *
 * - PROT_READ (alone) selects the input buffer.
 * - PROT_WRITE (alone) selects the output buffer.
 * - PROT_WRITE|PROT_READ together select the output buffer. This combination
 *   is required in BSD to make the buffer accessible. With just PROT_WRITE
 *   every attempt to access the returned buffer will result in segmentation/bus
 *   error. PROT_READ|PROT_WRITE is also permitted in Linux with OSS version
 *   3.8-beta16 and later (earlier versions don't accept it).
 *
 * Non duplex devices have just one buffer. When an application wants to do both
 * input and output it's recommended that the device is closed and re-opened when
 * switching between modes. PROT_READ|PROT_WRITE can be used to open the buffer
 * for both input and output (with OSS 3.8-beta16 and later) but the result may be
 * unpredictable.
 */

#if 1
	if ((buf=mmap(NULL, sz, PROT_WRITE, MAP_FILE|MAP_SHARED, fd, 0))==(caddr_t)-1)
  	{
		perror("mmap (write)");
		exit(-1);
	}
	printf("mmap (out) returned %08x\n", buf);
#else
        buf=data;
#endif
	op=buf;

/*
 * op contains now a pointer to the DMA buffer
 */

/*
 * Then it's time to start the engine. The driver doesn't allow read() and/or
 * write() when the buffer is mapped. So the only way to start operation is
 * to togle device's enable bits. First set them off. Setting them on enables
 * recording and/or playback.
 */

	tmp = 0;
	ioctl(fd, SNDCTL_DSP_SETTRIGGER, &tmp);
	printf("Trigger set to %08x\n", tmp);

/*
 * It might be usefull to write some data to the buffer before starting.
 */

	tmp = PCM_ENABLE_OUTPUT;
	ioctl(fd, SNDCTL_DSP_SETTRIGGER, &tmp);
	printf("Trigger set to %08x\n", tmp);

/*
 * The machine is up and running now. Use SNDCTL_DSP_GETOPTR to get the
 * buffer status.
 *
 * NOTE! The driver empties each buffer fragmen after they have been
 * played. This prevents looping sound if there are some performance problems
 * in the application side. For similar reasons it recommended that the
 * application uses some amout of play ahead. It can rewrite the unplayed
 * data later if necessary.
 */

	nfrag = 0;
	for (idx=0; idx<40; idx++)
	{
		struct count_info count;
		int p, l, extra;

		FD_ZERO(&writeset);
		FD_SET(fd, &writeset);

		tim.tv_sec = 10;
		tim.tv_usec= 0;

		select(fd+1, NULL, &writeset, NULL, NULL);
/*
 * SNDCTL_DSP_GETOPTR (and GETIPTR as well) return three items. The
 * bytes field returns number of bytes played since start. It can be used
 * as a real time clock.
 *
 * The blocks field returns number of period transitions (interrupts) since
 * previous GETOPTR call. It can be used as a method to detect underrun 
 * situations.
 *
 * The ptr field is the DMA pointer inside the buffer area (in bytes from
 * the beginning of total buffer area).
 */

		if (ioctl(fd, SNDCTL_DSP_GETOPTR, &count)==-1)
		{
			perror("GETOPTR");
			exit(-1);
		}

		nfrag += count.blocks;

#ifdef VERBOSE

		printf("Total: %09d, Period: %03d, Ptr: %06d",
			count.bytes, nfrag, count.ptr);
		fflush(stdout);
#endif

/*
 * Caution! This version doesn't check for bounds of the DMA
 * memory area. It's possible that the returned pointer value is not aligned
 * to period boundaries. It may be several samples behind the boundary
 * in case there was extra delay between the actual hardware interrupt and
 * the time when DSP_GETOPTR was called.
 *
 * Don't just call memcpy() with length set to 'period_size' without
 * first checking that the transfer really fits to the buffer area.
 * A mistake of just one byte causes seg fault. It may be easiest just
 * to align the returned pointer value to period boundary before using it.
 *
 * It would be very good idea to write few extra samples to next period
 * too. Otherwise several (uninitialized) samples from next period
 * will get played before your program gets chance to initialize them.
 * Take in count the fact thaat there are other processes batling about
 * the same CPU. This effect is likely to be very annoying if period
 * size is decreased too much.
 */

/*
 * Just a minor clarification to the above. The following line alings
 * the pointer to period boundaries. Note! Don't trust that period
 * size is always a power of 2. It may not be so in future.
 */
		count.ptr = (count.ptr/fsz)*fsz;

#ifdef VERBOSE
		printf(" memcpy(%6d, %4d)\n", (dp-data), fsz);
		fflush(stdout);
#endif

/*
 * Set few bytes in the beginning of next period too.
 */
		if ((count.ptr+fsz+16) < sz)	/* Last period? */
		   extra = 16;
		else
		   extra = 0;

		memcpy(op+count.ptr, dp, fsz+extra);
		
		dp += fsz;
		if (dp > (data+sl-fsz))
		   dp = data;
	}

	close(fd);

	printf( ">>>> open (2)\n" ); fflush( stdout );

	if ((fd=open("/dev/dsp", O_RDWR, 0))==-1)
	{
		perror("/dev/dsp");
		exit(-1);
	}
	close( fd );        

	exit(0);
}
