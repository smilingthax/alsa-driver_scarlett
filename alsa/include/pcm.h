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

#define _snd_pcm_substream_chip(substream) ((substream)->pcm->private_data)
#define snd_pcm_substream_chip(substream) snd_magic_cast1(chip_t, _snd_pcm_substream_chip(substream), return -ENXIO)
#define _snd_pcm_chip(pcm) ((pcm)->private_data)
#define snd_pcm_chip(pcm) snd_magic_cast1(chip_t, _snd_pcm_chip(pcm), return -ENXIO)

typedef struct snd_stru_pcm_file snd_pcm_file_t;
typedef struct snd_stru_pcm_runtime snd_pcm_runtime_t;
typedef struct snd_stru_pcm_substream snd_pcm_substream_t;
typedef struct snd_stru_pcm_stream snd_pcm_stream_t;

#ifdef CONFIG_SND_OSSEMUL
#include "pcm_oss.h"
#endif

/*
 *  Hardware (lowlevel) section
 */

typedef struct snd_stru_pcm_hardware {
	/* -- constants -- */
	unsigned int info;	/* SND_PCM_INFO_* */
	unsigned int formats;	/* SND_PCM_FMT_* */
	unsigned int rates;	/* SND_PCM_RATE_* */
	unsigned int min_rate;	/* min rate */
	unsigned int max_rate;	/* max rate */
	unsigned int min_channels;	/* min channels */
	unsigned int max_channels;	/* max channels */
	size_t min_fragment_size;	/* min fragment size */
	size_t max_fragment_size;	/* max fragment size */
	size_t min_fragments;	/* min # of fragments */
	size_t max_fragments;	/* max # of fragments */
	size_t fragment_align;	/* fragment align value */
	size_t fifo_size;		/* fifo size in bytes */
	size_t transfer_block_size; /* bus transfer block size in bytes */
	/* -- functions -- */
	int (*ioctl)(snd_pcm_substream_t * substream,
		     unsigned int cmd, void *arg);
	int (*prepare)(snd_pcm_substream_t * substream);
	int (*trigger)(snd_pcm_substream_t * substream, int cmd);
	unsigned int (*pointer)(snd_pcm_substream_t * substream);
	int (*copy)(snd_pcm_substream_t *substream, int channel, unsigned int pos,
		    void *buf, size_t count);
	int (*silence)(snd_pcm_substream_t *substream, int channel, 
		       unsigned int pos, size_t count);
} snd_pcm_hardware_t;

/*
 *
 */

#define SND_PCM_DEVICES		8

#define SND_PCM_DEFAULT_RATE	8000

#define SND_PCM_IOCTL1_FALSE	((void *)0)
#define SND_PCM_IOCTL1_TRUE	((void *)1)

#define SND_PCM_IOCTL1_INFO		0
#define SND_PCM_IOCTL1_PARAMS		1
#define SND_PCM_IOCTL1_PARAMS_INFO	2
#define SND_PCM_IOCTL1_SETUP		3
#define SND_PCM_IOCTL1_STATUS		4
#define SND_PCM_IOCTL1_MMAP_BYTES	5
#define SND_PCM_IOCTL1_MMAP_PTR		6
#define SND_PCM_IOCTL1_CHANNEL_INFO	8
#define SND_PCM_IOCTL1_CHANNEL_SETUP	9
#define SND_PCM_IOCTL1_CHANNEL_PARAMS	10
#define SND_PCM_IOCTL1_SYNC		11

#define SND_PCM_TRIGGER_STOP		0
#define SND_PCM_TRIGGER_START		1
#define SND_PCM_TRIGGER_PAUSE_PUSH	3
#define SND_PCM_TRIGGER_PAUSE_RELEASE	4

#define snd_pcm_clear_time(stream) \
	((stream)->time.tv_sec = (stream)->time.tv_usec = 0)

struct snd_stru_pcm_file {
	snd_pcm_substream_t * substream;
	struct snd_stru_pcm_file * next;
};

struct snd_stru_pcm_runtime {
	unsigned int dma_ok: 1,		/* DMA is set up */
		time: 1,		/* Status has timestamp */
		mmap: 1;		/* mmap requested */
	snd_pcm_substream_t *trigger_master;
	snd_timestamp_t trigger_time;	/* start timestamp */
	int start_mode;
	int ready_mode;
	int xrun_mode;
	int mmap_shape;

	snd_pcm_params_info_t params_info;
	snd_pcm_format_t format;	/* format information */
	unsigned int rate_master;
	unsigned int rate_divisor;
	size_t buffer_size;
	size_t frag_size;
	size_t frags;			/* fragments */

	volatile int *state;		/* stream status */
	int _sstate;			/* static status location */
	volatile size_t *hw_ptr;
	size_t _shw_ptr;
	volatile size_t *appl_ptr;
	size_t _sappl_ptr;
	volatile size_t *avail_min;
	size_t _savail_min;
	snd_timestamp_t *tstamp;
	snd_timestamp_t _ststamp;
	size_t hw_ptr_base;		/* Position at buffer restart */
	size_t hw_ptr_interrupt;	/* Position at interrupt time*/
	int interrupt_pending;
	size_t min_align;		/* Min alignment for the format */
	int xfer_mode;
	size_t xfer_min;
	size_t xfer_align;		/* True alignment used */
	size_t avail_max;

	size_t boundary;

	int overrange;
	snd_pcm_digital_t *dig_mask;	/* digital mask */
	void (*dig_mask_free)(void *dig_mask);
	snd_pcm_digital_t digital;	/* digital format information */
	snd_pcm_sync_id_t sync;		/* hardware synchronization ID */
	size_t bits_per_frame;
	size_t bits_per_sample;
	size_t byte_align;

	snd_pcm_mmap_status_t *mmap_status;
	snd_pcm_mmap_control_t *mmap_control;
	char *mmap_data;
	int mmap_status_count;
	int mmap_control_count;
	int mmap_data_count;
	/* -- locking / scheduling -- */
	spinlock_t lock;
	spinlock_t sleep_lock;
	wait_queue_head_t sleep;
	struct timer_list poll_timer;
	struct timer_list xrun_timer;
	struct fasync_struct *fasync;
	/* -- private section -- */
	void *private_data;
	void (*private_free)(snd_pcm_runtime_t *runtime);
	/* -- own hardware routines -- */
	snd_pcm_hardware_t *hw;
	void (*hw_free)(void *hw);
	/* -- interrupt callbacks -- */
	void (*transfer_ack_begin)(snd_pcm_substream_t *substream);
	void (*transfer_ack_end)(snd_pcm_substream_t *substream);
	/* -- timer -- */
	unsigned int timer_resolution;	/* timer resolution */
	int timer_running;		/* time is running */
	spinlock_t timer_lock;
	/* -- DMA -- */           
	unsigned char *dma_area;	/* DMA area */
	dma_addr_t dma_addr;		/* physical bus address (not accessible from main CPU) */
	unsigned long dma_size;		/* size of DMA area */
#ifdef CONFIG_SND_OSSEMUL
	/* -- OSS things -- */
	snd_pcm_oss_runtime_t oss;
#endif
};

struct snd_stru_pcm_substream {
	snd_pcm_t *pcm;
	snd_pcm_stream_t *pstr;
	int number;
	char name[32];			/* substream name */
	int stream;			/* stream (direction) */
	/* -- runtime information -- */
	snd_pcm_runtime_t *runtime;
        /* -- timer section -- */
	snd_timer_t *timer;		/* timer */
	/* -- next substream -- */
	snd_pcm_substream_t *next;
	/* -- linked substreams -- */
	snd_pcm_substream_t *link_next;
	snd_pcm_substream_t *link_prev;
	snd_pcm_file_t *file;
	struct file *ffile;
#ifdef CONFIG_SND_OSSEMUL
	/* -- OSS things -- */
	snd_pcm_oss_substream_t oss;
#endif
};

#ifdef CONFIG_SND_OSSEMUL
#define SUBSTREAM_BUSY(substream) ((substream)->file != NULL || ((substream)->oss.file != NULL))
#else
#define SUBSTREAM_BUSY(substream) ((substream)->file != NULL)
#endif


struct snd_stru_pcm_stream {
	int stream;				/* stream (direction) */
	snd_pcm_t *pcm;
	/* -- lowlevel functions -- */
	int (*open)(snd_pcm_substream_t *substream);
	int (*close)(snd_pcm_substream_t *substream);
	/* -- substreams -- */
	unsigned int substream_count;
	unsigned int substream_opened;
	snd_pcm_substream_t *substream;
#ifdef CONFIG_SND_OSSEMUL
	/* -- OSS things -- */
	snd_pcm_oss_stream_t oss;
#endif
	snd_pcm_file_t *files;
	snd_info_entry_t *dev;
	snd_minor_t *reg;
};

struct snd_stru_pcm {
	snd_card_t *card;
	unsigned int device;	/* device number */
	unsigned int info_flags;
	unsigned short device_class;
	unsigned short device_subclass;
	char id[64];
	char name[80];
	snd_pcm_stream_t streams[2];
	struct semaphore open_mutex;
	wait_queue_head_t open_wait;
	void *private_data;
	void (*private_free) (snd_pcm_t *pcm);
#ifdef CONFIG_SND_OSSEMUL
	snd_pcm_oss_t oss;
#endif
};

struct snd_stru_pcm_notify {
	int (*n_register) (unsigned short minor, snd_pcm_t * pcm);
	int (*n_unregister) (unsigned short minor, snd_pcm_t * pcm);
	struct list_head list;
};

typedef struct {
	struct file *file;
	snd_pcm_substream_t *substream;
	void *arg;
	snd_timestamp_t tstamp;
	int result;
} snd_pcm_ksync_request_t;

typedef struct {
	unsigned int master;
	unsigned int div_min, div_max;
} snd_pcm_clock_t;

/*
 *  Registering
 */

extern snd_pcm_t *snd_pcm_devices[];

extern void snd_pcm_lock(int unlock);

extern int snd_pcm_new(snd_card_t * card, char *id, int device,
		       int playback_count, int capture_count, snd_pcm_t **rpcm);

extern int snd_pcm_notify(struct snd_stru_pcm_notify *notify, int nfree);

extern snd_minor_t snd_pcm_reg[2];

/*
 *  Native I/O
 */

extern int snd_pcm_info(snd_pcm_substream_t * substream, snd_pcm_info_t * _info);
extern int snd_pcm_prepare(snd_pcm_substream_t *substream);
extern int snd_pcm_start(snd_pcm_substream_t *substream);
extern int snd_pcm_stop(snd_pcm_substream_t *substream, int status);
extern int snd_pcm_kernel_playback_ioctl(snd_pcm_substream_t *substream, unsigned int cmd, void *arg);
extern int snd_pcm_kernel_capture_ioctl(snd_pcm_substream_t *substream, unsigned int cmd, void *arg);
extern int snd_pcm_kernel_ioctl(snd_pcm_substream_t *substream, unsigned int cmd, void *arg);
extern int snd_pcm_open(struct inode *inode, struct file *file);
extern int snd_pcm_release(struct inode *inode, struct file *file);
extern unsigned int snd_pcm_playback_poll(struct file *file, poll_table * wait);
extern unsigned int snd_pcm_capture_poll(struct file *file, poll_table * wait);
extern int snd_pcm_open_substream(snd_pcm_t *pcm, int stream, snd_pcm_substream_t **rsubstream);
extern void snd_pcm_release_substream(snd_pcm_substream_t *substream);
extern void snd_pcm_vma_notify_data(void *client, void *data);
extern int snd_pcm_mmap_data(snd_pcm_substream_t *substream, struct file *file,
			     struct vm_area_struct *area);

/*
 *  PCM library
 */

static inline ssize_t bytes_to_samples(snd_pcm_runtime_t *runtime, ssize_t size)
{
	return size * 8 / runtime->bits_per_sample;
}

static inline ssize_t bytes_to_frames(snd_pcm_runtime_t *runtime, ssize_t size)
{
	return size * 8 / runtime->bits_per_frame;
}

static inline ssize_t samples_to_bytes(snd_pcm_runtime_t *runtime, ssize_t size)
{
	return size * runtime->bits_per_sample / 8;
}

static inline ssize_t frames_to_bytes(snd_pcm_runtime_t *runtime, ssize_t size)
{
	return size * runtime->bits_per_frame / 8;
}

static inline int frame_aligned(snd_pcm_runtime_t *runtime, ssize_t bytes)
{
	return bytes % runtime->byte_align == 0;
}

static inline size_t snd_pcm_lib_buffer_bytes(snd_pcm_substream_t *substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	return frames_to_bytes(runtime, runtime->buffer_size);
}

static inline size_t snd_pcm_lib_fragment_bytes(snd_pcm_substream_t *substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	return frames_to_bytes(runtime, runtime->frag_size);
}

/*
 *  result is: 0 ... (frag_boundary - 1)
 */
static inline size_t snd_pcm_playback_avail(snd_pcm_runtime_t *runtime)
{
	ssize_t avail = *runtime->hw_ptr + runtime->buffer_size - *runtime->appl_ptr;
	if (avail < 0)
		avail += runtime->boundary;
	return avail;
}

/*
 *  result is: 0 ... (frag_boundary - 1)
 */
static inline size_t snd_pcm_capture_avail(snd_pcm_runtime_t *runtime)
{
	ssize_t avail = *runtime->hw_ptr - *runtime->appl_ptr;
	if (avail < 0)
		avail += runtime->boundary;
	return avail;
}

static inline void snd_pcm_trigger_done(snd_pcm_substream_t *substream, 
					snd_pcm_substream_t *master)
{
	substream->runtime->trigger_master = master;
}

extern int snd_pcm_nearest_le_clock(unsigned int hertz, 
				    unsigned int clocks_count, 
				    snd_pcm_clock_t *clocks,
				    unsigned int *masterp, 
				    unsigned int *divisorp);
extern int snd_pcm_nearest_ge_clock(unsigned int hertz, 
				    unsigned int clocks_count, 
				    snd_pcm_clock_t *clocks,
				    unsigned int *masterp, 
				    unsigned int *divisorp);
extern int snd_pcm_nearest_clock(unsigned int hertz, 
				 unsigned int clocks_count, 
				 snd_pcm_clock_t *clocks,
				 unsigned int *masterp, 
				 unsigned int *divisorp);
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
 
extern void snd_pcm_set_sync(snd_pcm_substream_t * substream);
extern int snd_pcm_lib_interleave_len(snd_pcm_substream_t *substream);
extern int snd_pcm_lib_set_buffer_size(snd_pcm_substream_t *substream, size_t size);
extern int snd_pcm_lib_ioctl(snd_pcm_substream_t *substream,
			     unsigned int cmd, void *arg);                      
extern void snd_pcm_update_hw_ptr(snd_pcm_substream_t *substream);
extern int snd_pcm_playback_xrun_check(snd_pcm_substream_t *substream);
extern int snd_pcm_capture_xrun_check(snd_pcm_substream_t *substream);
extern int snd_pcm_playback_xrun_asap(snd_pcm_substream_t *substream);
extern int snd_pcm_capture_xrun_asap(snd_pcm_substream_t *substream);
extern int snd_pcm_playback_ready(snd_pcm_substream_t *substream);
extern int snd_pcm_capture_ready(snd_pcm_substream_t *substream);
extern long snd_pcm_playback_ready_jiffies(snd_pcm_substream_t *substream);
extern long snd_pcm_capture_ready_jiffies(snd_pcm_substream_t *substream);
extern int snd_pcm_playback_data(snd_pcm_substream_t *substream);
extern int snd_pcm_playback_empty(snd_pcm_substream_t *substream);
extern int snd_pcm_capture_empty(snd_pcm_substream_t *substream);
extern void snd_pcm_transfer_done(snd_pcm_substream_t *substream);
extern ssize_t snd_pcm_lib_write(snd_pcm_substream_t *substream,
				 const void *buf, size_t frames);
extern ssize_t snd_pcm_lib_read(snd_pcm_substream_t *substream,
				void *buf, size_t frames);
extern ssize_t snd_pcm_lib_writev(snd_pcm_substream_t *substream,
				  void **bufs, size_t frames);
extern ssize_t snd_pcm_lib_readv(snd_pcm_substream_t *substream,
				 void **bufs, size_t frames);

/*
 *  Timer interface
 */

extern void snd_pcm_timer_resolution_change(snd_pcm_substream_t *substream);
extern void snd_pcm_timer_init(snd_pcm_substream_t * substream);
extern void snd_pcm_timer_done(snd_pcm_substream_t * substream);

#endif				/* __PCM_H */
