#ifndef __PCM1_H
#define __PCM1_H

/*
 *  Digital Audio (PCM) abstract layer
 *  Copyright (c) by Jaroslav Kysela <perex@jcu.cz>
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "pcm.h"
#include "timer.h"

#define SND_PCM1_PLAYBACK	0
#define SND_PCM1_RECORD		1

#define SND_PCM1_LFLG_NONE	0x0000
#define SND_PCM1_LFLG_PLAY	0x0001
#define SND_PCM1_LFLG_RECORD	0x0002
#define SND_PCM1_LFLG_BOTH	(SND_PCM1_LFLG_PLAY|SND_PCM1_LFLG_RECORD)
#define SND_PCM1_LFLG_OSS_PLAY	0x0004
#define SND_PCM1_LFLG_OSS_RECORD 0x0008
#define SND_PCM1_LFLG_OSS	(SND_PCM1_LFLG_OSS_PLAY|SND_PCM1_LFLG_OSS_RECORD)

#define SND_PCM1_DEFAULT_RATE	8000

#define SND_PCM1_MODE_MULTI	0x00000010	/* set - multitrack (1-32) operation enabled */
#define SND_PCM1_MODE_VALID	0x00000080	/* unset = not valid, set = valid */
#define SND_PCM1_MODE_U		0x00000100	/* unset = signed, set = unsigned */
#define SND_PCM1_MODE_16	0x00000200	/* unset = 8 bit , set = 16 bit   */
#define SND_PCM1_MODE_BIG	0x00000400	/* big endian (16-bit mode) */
#define SND_PCM1_MODE_ULAW	0x00000800	/* mu-Law */
#define SND_PCM1_MODE_ALAW	0x00001000	/* a-Law */
#define SND_PCM1_MODE_ADPCM	0x00002000	/* IMA ADPCM 4:1 */
#define SND_PCM1_MODE_MPEG	0x00004000	/* MPEG 1/2 */
#define SND_PCM1_MODE_GSM	0x00008000	/* GSM */
#define SND_PCM1_MODE_24	0x00010000	/* unset = nothing, set = 24 bit */
#define SND_PCM1_MODE_32	0x00020000	/* unset = nothing, set = 32 bit */
#define SND_PCM1_MODE_FLOAT	0x00040000	/* unset = nothing, set = Float */
#define SND_PCM1_MODE_FLOAT64	0x00080000	/* unset = nothing, set = Float64 */
#define SND_PCM1_MODE_TYPE	0xffffff00	/* bitmask */

#define SND_PCM1_FLG_NONE	0x00000000
#define SND_PCM1_FLG_ENABLE	0x00000001	/* enable trigger */
#define SND_PCM1_FLG_ABORT	0x00000004	/* user abort */
#define SND_PCM1_FLG_NONBLK	0x00000008	/* non block I/O */
#define SND_PCM1_FLG_MMAP	0x00000010	/* mmaped access */
#define SND_PCM1_FLG_DMAOK	0x00000020	/* DMA is now allocated */
#define SND_PCM1_FLG_NEUTRAL	0x00000040	/* erase DMA buffer (fill with neutral byte) */
#define SND_PCM1_FLG_BUFFERS	0x00000080	/* compute new sizes for buffers */
#define SND_PCM1_FLG_TRIGGER	0x00000100	/* trigger on/off */
#define SND_PCM1_FLG_TRIGGER1	0x00000200	/* prepare ok */
#define SND_PCM1_FLG_TRIGGERA	0x00000300	/* both above flags */
#define SND_PCM1_FLG_SYNC	0x00000400	/* synchronize playback/record */
#define SND_PCM1_FLG_TIME	0x00000800	/* time */
#define SND_PCM1_FLG_PAUSE	0x00001000	/* pause in progress */
#define SND_PCM1_FLG_NEEDEMPTY	0x00002000	/* record buffer needs to be empty */
#define SND_PCM1_FLG_TIMER	0x00004000	/* timer is running */

#define SND_PCM1_HW_BATCH	0x00000001	/* double buffering */
#define SND_PCM1_HW_8BITONLY	0x00000002	/* hardware supports only 8-bit DMA, but does conversions from 16-bit to 8-bit */
#define SND_PCM1_HW_16BITONLY	0x00000004	/* hardware supports only 16-bit DMA, but does conversion from 8-bit to 16-bit */
#define SND_PCM1_HW_COPYMASK	0x00000007	/* flags to copy to info structure */
#define SND_PCM1_HW_AUTODMA	0x10000000	/* hardware supports auto dma - good */
#define SND_PCM1_HW_BLOCKPTR	0x20000000	/* current pointer needs size and returns offset to current block */
#define SND_PCM1_HW_PAUSE	0x40000000	/* for playback - pause is supported */
#define SND_PCM1_HW_OVERRANGE	0x40000000	/* for record - ADC overrange variable is valid */

#define SND_PCM1_IOCTL_FALSE	((void *)0)
#define SND_PCM1_IOCTL_TRUE	((void *)1)

#define SND_PCM1_IOCTL_MODE	0x00000001	/* check mode (format) */
#define SND_PCM1_IOCTL_RATE	0x00000002	/* compute rate */
#define SND_PCM1_IOCTL_VOICES	0x00000003	/* check voices (format) */
#define SND_PCM1_IOCTL_PAUSE	0x00000004	/* pause */
#define SND_PCM1_IOCTL_FRAG	0000000005	/* fragment size */

#define snd_pcm1_lockzero( channel ) \
  ((channel) -> block_lock = -1)
#define snd_pcm1_lock( channel, block ) \
  ((channel) -> block_lock = (block))
#define snd_pcm1_islock( channel, block ) \
  ((channel) -> block_lock >= 0 && (channel) -> block_lock == (block))
#define snd_pcm1_clear_time( channel ) \
  ((channel) -> time.tv_sec = (channel) -> time.tv_usec = 0)

typedef struct snd_stru_pcm1_channel snd_pcm1_channel_t;
typedef struct snd_stru_pcm1 snd_pcm1_t;

struct snd_stru_pcm1_oss_setup {
	char *task_name;
	unsigned int playback_only:1;
	unsigned int fragments;
	unsigned int fragment_size;
	struct snd_stru_pcm1_oss_setup *next;
};

struct snd_stru_pcm1_hardware {
	/* -- these values aren't erased -- */
	void *private_data;		/* pointer to private structure */
	void (*private_free) (void *private_data);
	/* -- must be filled with low-level driver */
	unsigned int flags;		/* see to SND_PCM_HW_XXXX */
	unsigned int formats;		/* supported formats... */
	unsigned int hw_formats;	/* supported formats by hardware... */
	unsigned int align;		/* align value... */
	unsigned short min_fragment;	/* minimal fragment... */
	unsigned short min_rate;	/* minimal rate... */
	unsigned short max_rate;	/* maximal rate... */
	unsigned short max_voices;	/* maximal voices... */
	/* -- low-level functions -- */
	int (*open) (snd_pcm1_t * pcm);
	void (*close) (snd_pcm1_t * pcm);
	int (*ioctl) (snd_pcm1_t * pcm, unsigned int cmd, unsigned long *arg);
	void (*prepare) (snd_pcm1_t * pcm,
			 unsigned char *buffer, unsigned int size,
			 unsigned int offset, unsigned int count);
	void (*trigger) (snd_pcm1_t * pcm, int up);
	unsigned int (*pointer) (snd_pcm1_t * pcm, unsigned int used_size);
	void (*dma) (snd_pcm1_t * pcm,
		     unsigned char *buffer, unsigned int offset,
		     unsigned char *user, unsigned int count);
	void (*dma_move) (snd_pcm1_t * pcm,
			  unsigned char *dbuffer, unsigned int dest_offset,
			  unsigned char *sbuffer, unsigned int src_offset,
			  unsigned int count);
	void (*dma_neutral) (snd_pcm1_t * pcm,
			     unsigned char *buffer, unsigned offset,
			     unsigned int count, unsigned char neutral_byte);
};

struct snd_stru_pcm1_channel {
	/* -- format/buffering -- */
	unsigned short voices;			/* or channels 1-32 */
	unsigned int mode;
	unsigned int format;
	unsigned int rate;
	unsigned int real_rate;
	unsigned int requested_block_size;
	unsigned int requested_blocks;
	unsigned int requested_subdivision;
	volatile unsigned int processed_bytes;
	volatile unsigned int interrupts;
	volatile unsigned int xruns;
	unsigned int lastxruns;
	volatile unsigned int overrange;	/* ADC overrange */
	volatile unsigned int total_discarded;	/* discarded blocks... */
	volatile unsigned int total_xruns;	/* under/overruns */
	snd_timer_t *timer;			/* timer */
	unsigned int timer_resolution;		/* timer resolution */
	/* -- physical/flags -- */
	unsigned int flags;
	unsigned int used_size;		/* used size of audio buffer (logical size) */
	unsigned int mmap_size;		/* for OSS */
	unsigned char neutral_byte;
	unsigned int size;		/* real size of audio buffer */
	unsigned char *buffer;		/* pointer to audio buffer */
	snd_dma_t *dmaptr;		/* dma pointer */
	/* -- ack callback -- */
	void (*ack) (snd_pcm1_t * pcm);	/* acknowledge interrupt to abstract layer */
	/* -- logical blocks -- */
	unsigned short blocks;		/* number of blocks (2-N) */
	unsigned int block_size;	/* size of one block */
	volatile unsigned short used;	/* number of used blocks */
	volatile unsigned int frag_size;/* size of partly used block */
	volatile unsigned short head;	/* fill it... */
	volatile unsigned short tail;	/* remove it... */
	volatile int block_lock;	/* locked block... */
	unsigned int blocks_max;	/* max blocks in queue for wakeup */
	unsigned int blocks_room;	/* min blocks in queue for wakeup */
	unsigned int blocks_min;	/* min blocks in queue for wakeup */
	struct timeval time;		/* time value */
	/* -- hardware -- */
	struct snd_stru_pcm1_hardware hw;
	/* -- OSS things -- */
	struct snd_stru_pcm1_oss_setup *setup_list;	/* setup list */
	struct snd_stru_pcm1_oss_setup *setup;		/* active setup */
	snd_mutex_define(setup_mutex);
	/* misc */
	snd_spin_define(lock);
	snd_spin_define(sleep_lock);
	snd_sleep_define(sleep);
};

struct snd_stru_pcm1 {
	snd_card_t *card;
	snd_pcm_t *pcm;		/* pointer to master PCM structure */
	unsigned int flags;
	unsigned short mask;
	struct snd_stru_pcm1_channel playback;
	struct snd_stru_pcm1_channel record;
	snd_info_entry_t *proc_entry;
	snd_info_entry_t *proc_oss_entry;
	snd_mutex_define(open);
	void *private_data;
	void (*private_free) (void *private_data);
};

extern void snd_pcm1_playback_dma(snd_pcm1_t * pcm,
			      unsigned char *buffer, unsigned int offset,
			      unsigned char *user, unsigned int count);
extern void snd_pcm1_playback_dma_ulaw(snd_pcm1_t * pcm,
			      unsigned char *buffer, unsigned int offset,
			      unsigned char *user, unsigned int count);
extern void snd_pcm1_playback_dma_ulaw_loud(snd_pcm1_t * pcm,
			      unsigned char *buffer, unsigned int offset,
			      unsigned char *user, unsigned int count);
extern void snd_pcm1_playback_dma_neutral(snd_pcm1_t * pcm,
			      unsigned char *buffer, unsigned int offset,
			      unsigned int count, unsigned char neutral_byte);
extern void snd_pcm1_record_dma(snd_pcm1_t * pcm,
			      unsigned char *buffer, unsigned int offset,
			      unsigned char *user, unsigned int count);
extern void snd_pcm1_record_dma_ulaw(snd_pcm1_t * pcm,
			      unsigned char *buffer, unsigned int offset,
			      unsigned char *user, unsigned int count);
extern void snd_pcm1_record_dma_ulaw_loud(snd_pcm1_t * pcm,
			      unsigned char *buffer, unsigned int offset,
			      unsigned char *user, unsigned int count);
extern void snd_pcm1_dma_move(snd_pcm1_t * pcm,
			      unsigned char *dbuffer, unsigned int dest_offset,
			      unsigned char *sbuffer, unsigned int src_offset,
			      unsigned int count);

extern void snd_pcm1_clear_channel(snd_pcm1_channel_t * pchn);
extern unsigned short snd_pcm1_file_flags(struct file *file);
extern void snd_pcm1_fill_with_neutral(snd_pcm1_t * pcm,
				       snd_pcm1_channel_t * pchn);

extern int snd_pcm1_dma_alloc(snd_pcm1_t * pcm, int direction,
			      snd_dma_t * dma, char *ident);
extern int snd_pcm1_dma_free(snd_pcm1_t * pcm, int direction, snd_dma_t * dma);

extern void snd_pcm1_proc_format(snd_pcm_channel_t * pchn,
			         snd_pcm1_channel_t * pchn1);


/*
 *  Registering
 */

extern snd_pcm_t *snd_pcm1_new_device(snd_card_t * card, char *id);

#endif				/* __PCM1_H */
