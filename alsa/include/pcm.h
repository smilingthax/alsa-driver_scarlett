#ifndef __PCM_H
#define __PCM_H

/*
 *  Digital Audio (PCM) abstract layer
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>
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

#ifdef CONFIG_SND_OSSEMUL
#include "pcm_oss.h"
#endif

typedef struct snd_stru_pcm_file snd_pcm_file_t;
typedef struct snd_stru_pcm_subchn snd_pcm_subchn_t;
typedef struct snd_stru_pcm_channel snd_pcm_channel_t;

/*
 *  Hardware (lowlevel) section
 */

typedef struct snd_stru_pcm_hardware {
	/* -- constants -- */
	unsigned int chninfo;	/* SND_PCM_CHNINFO_* */
	unsigned int formats;	/* SND_PCM_FMT_* */
	unsigned int rates;	/* SND_PCM_RATE_* */
	int min_rate;		/* min rate */
	int max_rate;		/* max rate */
	int min_voices;		/* min voices */
	int max_voices;		/* max voices */
	int min_fragment_size;	/* min fragment size (block mode) */
	int max_fragment_size;	/* max fragment size (block mode) */
	int fragment_align;	/* fragment align value */
	int fifo_size;		/* fifo size in bytes */
	int transfer_block_size; /* bus transfer block size in bytes */
	/* -- functions -- */
	int (*ioctl)(void *private_data, snd_pcm_subchn_t * subchn,
		     unsigned int cmd, unsigned long *arg);
	int (*prepare)(void *private_data, snd_pcm_subchn_t * subchn);
	int (*trigger)(void *private_data, snd_pcm_subchn_t * subchn, int cmd);
	long (*transfer)(void *private_data, snd_pcm_subchn_t * subchn,
		         const char *_buffer, long size);
	unsigned int (*pointer)(void *private_data, snd_pcm_subchn_t * subchn);
} snd_pcm_hardware_t;

/*
 *
 */

#define SND_PCM_DEVICES		16

#define SND_PCM_DEFAULT_RATE	8000

#define SND_PCM_FLG_DMA_OK	(1<<0)
#define SND_PCM_FLG_TIMER	(1<<1)
#define SND_PCM_FLG_TIMESTAMP	(1<<2)
#define SND_PCM_FLG_MMAP	(1<<3)
#define SND_PCM_FLG_NONBLOCK	(1<<4)
#define SND_PCM_FLG_OSS_MMAP	(1<<5)
#define SND_PCM_FLG_TIME	(1<<6)

#define SND_PCM_IOCTL1_FALSE	((unsigned long *)0)
#define SND_PCM_IOCTL1_TRUE	((unsigned long *)0)

#define SND_PCM_IOCTL1_INFO	0
#define SND_PCM_IOCTL1_PARAMS	1
#define SND_PCM_IOCTL1_SETUP	2
#define SND_PCM_IOCTL1_STATUS	3
#define SND_PCM_IOCTL1_MMAP_CTRL 4
#define SND_PCM_IOCTL1_MMAP_SIZE 5
#define SND_PCM_IOCTL1_MMAP_PTR	6

#define SND_PCM_TRIGGER_STOP	0
#define SND_PCM_TRIGGER_GO	1
#define SND_PCM_TRIGGER_SYNC_GO	2

#define snd_pcm_clear_time(channel) \
	((channel)->time.tv_sec = (channel)->time.tv_usec = 0)

struct snd_stru_pcm_file {
	snd_pcm_t *pcm;
	snd_pcm_subchn_t * chn[2];
	struct snd_stru_pcm_file * next;
	/* -- private section */
	void *private_data;
	void (*private_free)(void *private_data);
};

typedef struct snd_stru_pcm_runtime {
	int mode;			/* channel mode */
	volatile int *status;		/* channel status */
	int _sstatus;			/* static status location */
	unsigned int flags;		/* run-time flags - SND_PCM_FLG_* */
	struct timeval stime;		/* time value */
	int start_mode;
	int stop_mode;
	snd_pcm_format_t format;	/* format information */
	int frags;			/* fragments */
	int voice;
	volatile int *frag_head;
	int _sfrag_head;		/* static fragment tail */
	volatile int *frag_tail;
	int _sfrag_tail;		/* static fragment tail */
	int frag_used;
	unsigned int interrupts;
	unsigned int position;
	unsigned int buf_position;
	unsigned int fill_position;
	int underrun;
	int overrun;
	int overrange;
	snd_pcm_digital_t digital;	/* digital format information */
	snd_pcm_sync_t sync_group;	/* synchronization group */
	union {
		struct {
			int queue_size; /* queue size in bytes */
			int fill;       /* fill mode - SND_PCM_FILL_XXXX */
			int max_fill;   /* maximum silence fill in bytes */
		} stream;
		struct {
			int frag_size;  /* requested size of fragment in bytes */
			int frags_min;  /* capture: minimum of filled fragments */
			int frags_max;  /* playback: maximum number of fragments */
		} block;
	} buf;
	snd_pcm_mmap_control_t * mmap_control;
	char * mmap_data;
	snd_vma_t * mmap_control_vma;
	snd_vma_t * mmap_data_vma;
	/* -- locking / scheduling -- */
	spinlock_t lock;
	spinlock_t sleep_lock;
	wait_queue_head_t sleep;
	/* -- own transfer routines -- */
	int (*hw_memcpy)(snd_pcm_subchn_t *subchn, int pos, const void *src, int count);
	int (*hw_memset)(snd_pcm_subchn_t *subchn, int pos, int c, int count);
	/* -- timer -- */
	unsigned int timer_resolution;	/* timer resolution */
	int timer_running;		/* time is running */
	spinlock_t timer_lock;
	/* -- DMA -- */           
        snd_dma_area_t *dma_area;	/* dma area */
#ifdef CONFIG_SND_OSSEMUL
	/* -- OSS things -- */
	snd_pcm_oss_runtime_t oss;
#endif
} snd_pcm_runtime_t;

struct snd_stru_pcm_subchn {
	snd_pcm_t *pcm;
	snd_pcm_channel_t *pchn;
	int number;
	char name[32];			/* subchannel name */
	int channel;			/* channel (direction) */
	/* -- constants -- */
	snd_pcm_hardware_t *hw;
	void (*hw_free)(void *hw);
	snd_pcm_sync_t sync;		/* hardware synchronization ID */
	snd_pcm_digital_t *dig_mask;	/* digital mask */
	int mixer_device;		/* mixer device */
	snd_mixer_eid_t mixer_eid;	/* mixer element identification */	
	/* -- runtime information -- */
	snd_pcm_runtime_t *runtime;
        /* -- timer section -- */
	snd_timer_t *timer;		/* timer */
	/* -- /proc interface -- */
	void *proc_entry;
	void *proc_private;
	spinlock_t proc_lock;
	void *proc_entry1;		/* for second midlevel code */
	/* -- next subchannel -- */
	snd_pcm_subchn_t *next;
	snd_pcm_file_t *file;
	/* -- private section -- */
	void *private_data;
	void (*private_free)(void *private_data);
	/* -- callback -- */
	void (*transfer_ack_begin)(snd_pcm_subchn_t *subchn);
	void (*transfer_ack_end)(snd_pcm_subchn_t *subchn);
#ifdef CONFIG_SND_OSSEMUL
	/* -- OSS things -- */
	snd_pcm_oss_subchn_t oss;
#endif
};

struct snd_stru_pcm_channel {
	int channel;				/* channel (direction) */
	snd_pcm_t *pcm;
	snd_kswitch_list_t switches;
	/* -- lowlevel functions -- */
	int (*open)(void *private_data, snd_pcm_subchn_t *subchn);
	int (*close)(void *private_data, snd_pcm_subchn_t *subchn);
	/* -- subchannels -- */
	unsigned int subchn_count;
	unsigned int subchn_opened;
	snd_pcm_subchn_t *subchn;
	/* -- private section -- */
	void *private_data;
	void (*private_free)(void *private_data);
#ifdef CONFIG_SND_OSSEMUL
	/* -- OSS things -- */
	snd_pcm_oss_channel_t oss;
#endif
};

struct snd_stru_pcm {
	snd_card_t *card;
	unsigned int device;	/* device number */
	unsigned int info_flags;
	int type;		/* PCM type */
	char id[64];
	char name[80];
	snd_pcm_channel_t chn[2];
	snd_pcm_file_t *files;
	snd_minor_t *reg;
	snd_info_entry_t *dev;
	struct semaphore open_mutex;
	int open_prefer_subchn;
	void *private_data;
	void (*private_free) (void *private_data);
#ifdef CONFIG_SND_OSSEMUL
	snd_pcm_oss_t oss;
#endif
};

struct snd_stru_pcm_notify {
	int (*n_register) (unsigned short minor, snd_pcm_t * pcm);
	int (*n_unregister) (unsigned short minor, snd_pcm_t * pcm);
	struct snd_stru_pcm_notify *next;
};

/*
 *  Registering
 */

extern snd_pcm_t *snd_pcm_devices[];

extern void snd_pcm_lock(int unlock);

extern int snd_pcm_new(snd_card_t * card, char *id, int device,
		       int playback_count, int capture_count, snd_pcm_t **rpcm);

extern int snd_pcm_notify(struct snd_stru_pcm_notify *notify, int nfree);

extern int snd_pcm_open(snd_pcm_t * pcm,
			int playback, int capture,
			snd_pcm_file_t ** pcm_file);
extern int snd_pcm_release(snd_pcm_file_t *pcm_file);

extern int snd_pcm_switch_add(snd_pcm_channel_t * pchn, snd_kswitch_t * ksw);
extern int snd_pcm_switch_remove(snd_pcm_channel_t * pchn, snd_kswitch_t * ksw);
extern snd_kswitch_t *snd_pcm_switch_new(snd_pcm_channel_t * pchn, snd_kswitch_t * ksw, void *private_data);
extern int snd_pcm_switch_change(snd_pcm_channel_t * pchn, snd_kswitch_t * ksw);

extern snd_minor_t snd_pcm_reg;

/*
 *  Native I/O
 */

extern int snd_pcm_info(snd_pcm_t * pcm, snd_pcm_info_t * _info);
extern int snd_pcm_channel_info(snd_pcm_subchn_t * subchn, snd_pcm_channel_info_t * _info);
extern int snd_pcm_channel_go_pre(snd_pcm_subchn_t *subchn, int cmd);
extern int snd_pcm_channel_go_post(snd_pcm_subchn_t *go, int cmd, int err);
extern int snd_pcm_kernel_ioctl(snd_pcm_file_t * pcm_file, unsigned int cmd, unsigned long arg);
extern int snd_pcm_open_device(unsigned short minor, int cardnum, int device, struct file *file);
extern int snd_pcm_release_device(unsigned short minor, int cardnum, int device, struct file *file);
extern unsigned int snd_pcm_poll(struct file *file, poll_table * wait);
                           
/*
 *  PCM library
 */
 
extern int snd_pcm_dma_alloc(snd_pcm_subchn_t * subchn, snd_dma_t * dma, char *ident);
extern int snd_pcm_dma_free(snd_pcm_subchn_t * subchn);
extern unsigned int snd_pcm_lib_transfer_size(snd_pcm_subchn_t *subchn);
extern unsigned int snd_pcm_lib_transfer_fragment(snd_pcm_subchn_t *subchn);
extern int snd_pcm_lib_16bit(snd_pcm_subchn_t *subchn);
extern int snd_pcm_lib_unsigned(snd_pcm_subchn_t *subchn);
extern int snd_pcm_lib_big_endian(snd_pcm_subchn_t *subchn);
extern int snd_pcm_lib_neutral_byte(snd_pcm_subchn_t *subchn);
extern int snd_pcm_lib_sample_width(snd_pcm_subchn_t *subchn);
extern int snd_pcm_lib_interleave_len(snd_pcm_subchn_t *subchn);
extern int snd_pcm_lib_set_buffer_size(snd_pcm_subchn_t *subchn, long size);
extern int snd_pcm_lib_mmap_ctrl_ptr(snd_pcm_subchn_t *subchn, char *ptr);
extern int snd_pcm_lib_ioctl(void *private_data, snd_pcm_subchn_t *subchn,
			     unsigned int cmd, unsigned long *arg);                      
extern int snd_pcm_playback_ok(snd_pcm_subchn_t *subchn);
extern int snd_pcm_capture_ok(snd_pcm_subchn_t *subchn);
extern int snd_pcm_playback_data(snd_pcm_subchn_t *subchn);
extern int snd_pcm_playback_empty(snd_pcm_subchn_t *subchn);
extern int snd_pcm_capture_empty(snd_pcm_subchn_t *subchn);
extern void snd_pcm_clear_values(snd_pcm_subchn_t *subchn);
extern void snd_pcm_transfer_stop(snd_pcm_subchn_t *subchn, int status);
extern void snd_pcm_transfer_done(snd_pcm_subchn_t *subchn);
extern long snd_pcm_playback_write(void *private_data, snd_pcm_subchn_t *subchn,
				   const char *buf, long count);
extern long snd_pcm_capture_read(void *private_data, snd_pcm_subchn_t *subchn,
				 const char *buf, long count);

/*
 *  Timer interface
 */

extern void snd_pcm_timer_resolution_change(snd_pcm_subchn_t *subchn);
extern void snd_pcm_timer_init(snd_pcm_subchn_t * subchn);
extern void snd_pcm_timer_done(snd_pcm_subchn_t * subchn);

/*
 *  /proc interface
 */

extern void snd_pcm_proc_init(snd_pcm_t * pcm);
extern void snd_pcm_proc_done(snd_pcm_t * pcm);
extern void snd_pcm_proc_format(snd_pcm_subchn_t * subchn);
extern void snd_pcm_proc_write(snd_pcm_subchn_t * subchn, unsigned int pos,
			       const void *buffer, long count, int kernel);

#endif				/* __PCM_H */
