/*
 * PC-Speaker driver for Linux
 *
 * Copyright (C) 1993-1997  Michael Beck
 * Copyright (C) 1997-2001  David Woodhouse
 * Copyright (C) 2001-2004  Stas Sergeev
 */

#ifndef __PCSP_H__
#define __PCSP_H__

#define PCSP_SOUND_VERSION	0x200	/* read 2.00 */

#define PCSP_DEBUG 		0

#define PCSP_DEFAULT_GAIN	4
/* PCSP internal maximum volume, it's hardcoded */
#define PCSP_MAX_VOLUME		256

#if PCSP_DEBUG
#define assert(expr) \
        if(!(expr)) { \
        printk( "Assertion failed! %s, %s, %s, line=%d\n", \
        #expr,__FILE__,__FUNCTION__,__LINE__); \
        }
#else
#define assert(expr)
#endif

/* the timer stuff */
#define TIMER_IRQ 0

/* default timer freq for PC-Speaker: 18643 Hz */
#define DIV_18KHZ	64
#define MAX_DIV 	DIV_18KHZ
#define MIN_DIV		(MAX_DIV >> chip->treble)
#define PIT_COUNTER	(MIN_DIV + chip->bass)

/* max timer freq & default sampling rate for PC-Speaker: 37286 Hz */
#define PCSP_MAX_POSS_TREBLE 1
#define PCSP_DEFAULT_SDIV (MAX_DIV >> PCSP_MAX_POSS_TREBLE)
#define PCSP_DEFAULT_RATE (CLOCK_TICK_RATE / PCSP_DEFAULT_SDIV)
#define PCSP_INDEX_INC (1 << (PCSP_MAX_POSS_TREBLE - chip->treble))
#define PCSP_RATE (CLOCK_TICK_RATE / MIN_DIV)

#define PCSP_MAX_PERIOD_SIZE	(64*1024)
#define PCSP_MAX_PERIODS	512
#define PCSP_BUFFER_SIZE	(128*1024)

/* Macros to emulate the DMA fragmentation */
#define PCSP_BUF(i) (runtime->dma_area + i * snd_pcm_lib_period_bytes(substream))
#define PCSP_CUR_BUF (PCSP_BUF(chip->cur_buf))

struct snd_pcsp {
	spinlock_t lock;
	struct snd_card *card;
	struct input_dev *input_dev;
	unsigned short port, irq, dma;
	struct snd_pcm *pcm;
	struct snd_pcm_substream *playback_substream;
	volatile int last_clocks;
	volatile int index;
	unsigned int volume;	/* volume for pc-speaker */
	unsigned int gain;		/* output gain */
	volatile int timer_active;
	volatile int timer_latch;
	volatile int clockticks;
	volatile int reset_timer;
	volatile unsigned int cur_buf;	/* fragment currently playing */
	unsigned char e;
	int max_treble;
	int treble;
	int bass;
	int enable;
	int pcspkr;
	unsigned char vl_tab[256];
};

extern struct snd_pcsp *snd_pcsp_chip;

extern int pcsp_set_timer_hook(struct snd_pcsp * chip, int (*func) (struct snd_pcsp * chip));
extern void pcsp_release_timer_hook(struct snd_pcsp * chip);

extern int snd_pcsp_new_pcm(struct snd_pcsp * chip);
extern int snd_pcsp_new_mixer(struct snd_pcsp * chip);
extern void pcsp_start_timer(struct snd_pcsp * chip);
extern void pcsp_stop_timer(struct snd_pcsp * chip);
extern void pcsp_calc_voltab(struct snd_pcsp * chip);

#endif
