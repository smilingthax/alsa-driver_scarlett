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

#ifndef __MIXER_H
#include "mixer.h"
#endif

typedef struct snd_stru_pcm_file snd_pcm_file_t;
typedef struct snd_stru_pcm_runtime snd_pcm_runtime_t;
typedef struct snd_stru_pcm_subchn snd_pcm_subchn_t;
typedef struct snd_stru_pcm_channel snd_pcm_channel_t;

#ifdef CONFIG_SND_OSSEMUL
#include "pcm_oss.h"
#endif

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
	unsigned int (*pointer)(void *private_data, snd_pcm_subchn_t * subchn);
} snd_pcm_hardware_t;

/*
 *
 */

#define SND_PCM_DEVICES		8

#define SND_PCM_DEFAULT_RATE	8000

#define SND_PCM_FLG_DMA_OK	(1<<0)
#define SND_PCM_FLG_TIMER	(1<<1)
#define SND_PCM_FLG_TIME	(1<<2)
#define SND_PCM_FLG_MMAP	(1<<3)

#define SND_PCM_IOCTL1_FALSE	((unsigned long *)0)
#define SND_PCM_IOCTL1_TRUE	((unsigned long *)1)

#define SND_PCM_IOCTL1_INFO		0
#define SND_PCM_IOCTL1_PARAMS		1
#define SND_PCM_IOCTL1_SETUP		2
#define SND_PCM_IOCTL1_STATUS		3
#define SND_PCM_IOCTL1_MMAP_CTRL	4
#define SND_PCM_IOCTL1_MMAP_SIZE	5
#define SND_PCM_IOCTL1_MMAP_PTR		6
#define SND_PCM_IOCTL1_PAUSE		7
#define SND_PCM_IOCTL1_VOICE_INFO	8
#define SND_PCM_IOCTL1_VOICE_SETUP	9
#define SND_PCM_IOCTL1_VOICE_PARAMS	10

#define SND_PCM_TRIGGER_STOP		0
#define SND_PCM_TRIGGER_GO		1
#define SND_PCM_TRIGGER_SYNC_GO		2
#define SND_PCM_TRIGGER_PAUSE_PUSH	3
#define SND_PCM_TRIGGER_PAUSE_RELEASE	4

#define snd_pcm_clear_time(channel) \
	((channel)->time.tv_sec = (channel)->time.tv_usec = 0)

struct snd_stru_pcm_file {
	snd_pcm_subchn_t * subchn;
	struct snd_stru_pcm_file * next;
	/* -- private section */
	void *private_data;
	void (*private_free)(void *private_data);
};

struct snd_stru_pcm_runtime {
	int mode;			/* channel mode */
	unsigned int flags;		/* run-time flags - SND_PCM_FLG_* */
	struct timeval stime;		/* time value */
	int start_mode;
	int xrun_mode;

	snd_pcm_format_t format;	/* format information */
	size_t buffer_size;
	size_t frag_size;
	size_t frags;			/* fragments */

	volatile int *status;		/* channel status */
	int _sstatus;			/* static status location */
	volatile size_t *byte_io;
	size_t _sbyte_io;
	volatile size_t *byte_data;
	size_t _sbyte_data;
	size_t byte_io_base;		/* Position at buffer restart */
	size_t byte_io_interrupt;	/* Position at interrupt time*/
	size_t frag_io_mod;		/* Fragment under I/O */
	size_t bytes_per_second;
	size_t bytes_per_frame;
	size_t bytes_avail_max;

	size_t byte_boundary;

	int xruns;
	int overrange;
	snd_pcm_digital_t *dig_mask;	/* digital mask */
	void (*dig_mask_free)(void *dig_mask);
	snd_pcm_digital_t digital;	/* digital format information */
	snd_pcm_sync_t sync;		/* hardware synchronization ID */
	snd_pcm_sync_t sync_group;	/* synchronization group */
	int mixer_device;		/* mixer device */
	snd_mixer_eid_t mixer_eid;	/* mixer element identification */	
	size_t bytes_min;	/* min available bytes for wakeup */
	size_t bytes_align;
	unsigned int bytes_xrun_max;
	int fill_mode;       /* fill mode - SND_PCM_FILL_XXXX */
	size_t bytes_fill_max;   /* maximum silence fill in bytes */
	size_t byte_fill;

	snd_pcm_mmap_control_t *mmap_control;
	char *mmap_data;
	unsigned long mmap_data_user;		/* User space address */
	snd_vma_t *mmap_control_vma;
	snd_vma_t *mmap_data_vma;
	/* -- locking / scheduling -- */
	spinlock_t lock;
	spinlock_t sleep_lock;
	wait_queue_head_t sleep;
	struct timer_list poll_timer;
	struct timer_list xrun_timer;
	int flushing;
	/* -- private section -- */
	void *private_data;
	void (*private_free)(void *private_data);
	/* -- own hardware routines -- */
	snd_pcm_hardware_t *hw;
	void (*hw_free)(void *hw);
	int (*hw_memcpy)(snd_pcm_subchn_t *subchn, int voice, unsigned int pos,
			 void *buf, size_t count);
	int (*hw_silence)(snd_pcm_subchn_t *subchn, int voice, 
			  unsigned int pos, size_t count);
	/* -- interrupt callbacks -- */
	void (*transfer_ack_begin)(snd_pcm_subchn_t *subchn);
	void (*transfer_ack_end)(snd_pcm_subchn_t *subchn);
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
};

struct snd_stru_pcm_subchn {
	snd_pcm_t *pcm;
	snd_pcm_channel_t *pchn;
	int number;
	char name[32];			/* subchannel name */
	int channel;			/* channel (direction) */
	/* -- runtime information -- */
	snd_pcm_runtime_t *runtime;
        /* -- timer section -- */
	snd_timer_t *timer;		/* timer */
	/* -- next subchannel -- */
	snd_pcm_subchn_t *next;
	snd_pcm_file_t *file;
	struct file *ffile;
#ifdef CONFIG_SND_OSSEMUL
	/* -- OSS things -- */
	snd_pcm_oss_subchn_t oss;
#endif
};

#ifdef CONFIG_SND_OSSEMUL
#define SUBCHN_BUSY(subchn) ((subchn)->file != NULL || ((subchn)->oss.file != NULL))
#else
#define SUBCHN_BUSY(subchn) ((subchn)->file != NULL)
#endif


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
	int open_prefer_subchn;
	snd_pcm_file_t *files;
	snd_info_entry_t *dev;
	snd_minor_t *reg;
};

struct snd_stru_pcm {
	snd_card_t *card;
	unsigned int device;	/* device number */
	unsigned int info_flags;
	unsigned short pcm_class;
	unsigned short pcm_subclass;
	char id[64];
	char name[80];
	snd_pcm_channel_t chn[2];
	struct semaphore open_mutex;
	wait_queue_head_t open_wait;
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

extern int snd_pcm_switch_add(snd_pcm_channel_t * pchn, snd_kswitch_t * ksw);
extern int snd_pcm_switch_remove(snd_pcm_channel_t * pchn, snd_kswitch_t * ksw);
extern snd_kswitch_t *snd_pcm_switch_new(snd_pcm_channel_t * pchn, snd_kswitch_t * ksw, void *private_data);
extern int snd_pcm_switch_change(snd_pcm_channel_t * pchn, snd_kswitch_t * ksw);

extern snd_minor_t snd_pcm_reg[2];

/*
 *  Native I/O
 */

extern int snd_pcm_info(snd_pcm_t * pcm, snd_pcm_info_t * _info);
extern int snd_pcm_channel_info(snd_pcm_subchn_t * subchn, snd_pcm_channel_info_t * _info);
extern int snd_pcm_channel_go(snd_pcm_subchn_t *subchn);
extern int snd_pcm_channel_go_pre(snd_pcm_subchn_t *subchn);
extern int snd_pcm_channel_go_post(snd_pcm_subchn_t *subchn, int err);
extern void snd_pcm_channel_stop(snd_pcm_subchn_t *subchn, int status);
extern int snd_pcm_kernel_playback_ioctl(snd_pcm_subchn_t *subchn, unsigned int cmd, unsigned long arg);
extern int snd_pcm_kernel_capture_ioctl(snd_pcm_subchn_t *subchn, unsigned int cmd, unsigned long arg);
extern int snd_pcm_kernel_ioctl(snd_pcm_subchn_t *subchn, unsigned int cmd, unsigned long arg);
extern int snd_pcm_open(unsigned short minor, int cardnum, int device, struct file *file, int channel);
extern int snd_pcm_release(unsigned short minor, int cardnum, int device, struct file *file);
extern unsigned int snd_pcm_playback_poll(struct file *file, poll_table * wait);
extern unsigned int snd_pcm_capture_poll(struct file *file, poll_table * wait);
extern int snd_pcm_open_subchn(snd_pcm_t *pcm, int channel, snd_pcm_subchn_t **rsubchn);
extern void snd_pcm_release_subchn(snd_pcm_subchn_t *subchn);
extern void snd_pcm_vma_notify_data(void *client, void *data);
extern int snd_pcm_mmap_data(snd_pcm_subchn_t *subchn, struct file *file,
			     struct vm_area_struct *area);

/*
 *  PCM library
 */

static inline size_t snd_pcm_lib_transfer_size(snd_pcm_subchn_t *subchn)
{
	return subchn->runtime->buffer_size;
}

static inline size_t snd_pcm_lib_transfer_fragment(snd_pcm_subchn_t *subchn)
{
	return subchn->runtime->frag_size;
}

extern int snd_pcm_format_signed(int format);
extern int snd_pcm_format_unsigned(int format);
extern int snd_pcm_format_linear(int format);
extern int snd_pcm_format_little_endian(int format);
extern int snd_pcm_format_big_endian(int format);
extern int snd_pcm_format_width(int format);			/* in bits */
extern int snd_pcm_format_physical_width(int format);		/* in bits */
extern u_int64_t snd_pcm_format_silence_64(int format);
extern ssize_t snd_pcm_format_set_silence(int format, void *buf, size_t count);
extern int snd_pcm_build_linear_format(int width, int unsignd, int big_endian);
extern ssize_t snd_pcm_format_size(int format, size_t samples);
extern ssize_t snd_pcm_format_bytes_per_second(snd_pcm_format_t *format);
 
extern int snd_pcm_dma_alloc(snd_pcm_subchn_t * subchn, snd_dma_t * dma, char *ident);
extern int snd_pcm_dma_setup(snd_pcm_subchn_t * subchn, snd_dma_area_t * area);
extern int snd_pcm_dma_free(snd_pcm_subchn_t * subchn);
extern void snd_pcm_set_sync(snd_pcm_subchn_t * subchn);
extern void snd_pcm_set_mixer(snd_pcm_subchn_t * subchn, int mixer_device, snd_kmixer_element_t * element);
extern int snd_pcm_lib_interleave_len(snd_pcm_subchn_t *subchn);
extern int snd_pcm_lib_set_buffer_size(snd_pcm_subchn_t *subchn, size_t size);
extern int snd_pcm_lib_mmap_ctrl_ptr(snd_pcm_subchn_t *subchn, char *ptr);
extern int snd_pcm_lib_ioctl(void *private_data, snd_pcm_subchn_t *subchn,
			     unsigned int cmd, unsigned long *arg);                      
extern void snd_pcm_update_byte_io(snd_pcm_subchn_t *subchn);
extern int snd_pcm_playback_xrun_check(snd_pcm_subchn_t *subchn);
extern int snd_pcm_capture_xrun_check(snd_pcm_subchn_t *subchn);
extern int snd_pcm_playback_ready(snd_pcm_subchn_t *subchn);
extern int snd_pcm_capture_ready(snd_pcm_subchn_t *subchn);
extern long snd_pcm_playback_ready_jiffies(snd_pcm_subchn_t *subchn);
extern long snd_pcm_capture_ready_jiffies(snd_pcm_subchn_t *subchn);
extern long snd_pcm_playback_xrun_jiffies(snd_pcm_subchn_t *subchn);
extern long snd_pcm_capture_xrun_jiffies(snd_pcm_subchn_t *subchn);
extern int snd_pcm_playback_data(snd_pcm_subchn_t *subchn);
extern int snd_pcm_playback_empty(snd_pcm_subchn_t *subchn);
extern int snd_pcm_capture_empty(snd_pcm_subchn_t *subchn);
extern void snd_pcm_transfer_done(snd_pcm_subchn_t *subchn);
extern ssize_t snd_pcm_lib_write(snd_pcm_subchn_t *subchn,
				 const char *buf, size_t count);
extern ssize_t snd_pcm_lib_read(snd_pcm_subchn_t *subchn,
				char *buf, size_t count);
extern ssize_t snd_pcm_lib_writev(snd_pcm_subchn_t *subchn,
				  const struct iovec *vector, unsigned long count);
extern ssize_t snd_pcm_lib_readv(snd_pcm_subchn_t *subchn,
				 const struct iovec *vector, unsigned long count);

/*
 *  Timer interface
 */

extern void snd_pcm_timer_resolution_change(snd_pcm_subchn_t *subchn);
extern void snd_pcm_timer_init(snd_pcm_subchn_t * subchn);
extern void snd_pcm_timer_done(snd_pcm_subchn_t * subchn);

#endif				/* __PCM_H */
