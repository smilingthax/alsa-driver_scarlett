#ifndef __PCM_H
#define __PCM_H

/*
 *  Digital Audio (PCM) abstract layer
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>
 *                   Abramo Bagnara <abramo@alsa-project.org>
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

typedef struct _snd_pcm_file snd_pcm_file_t;
typedef struct _snd_pcm_runtime snd_pcm_runtime_t;
typedef struct _snd_pcm_substream snd_pcm_substream_t;
typedef struct _snd_pcm_stream snd_pcm_stream_t;

#ifdef CONFIG_SND_OSSEMUL
#include "pcm_oss.h"
#endif

/*
 *  Hardware (lowlevel) section
 */

typedef struct _snd_pcm_hardware {
	unsigned int info;		/* SND_PCM_INFO_* */
	unsigned int formats;		/* SND_PCM_FMTBIT_* */
	unsigned int rates;		/* SND_PCM_RATE_* */
	unsigned int min_rate;		/* min rate */
	unsigned int max_rate;		/* max rate */
	unsigned int min_channels;	/* min channels */
	unsigned int max_channels;	/* max channels */
	size_t min_fragment_size;	/* min fragment size */
	size_t max_fragment_size;	/* max fragment size */
	size_t step_fragment_size;	/* fragment size step */
	size_t min_fragments;		/* min # of fragments */
	size_t max_fragments;		/* max # of fragments */
	size_t fifo_size;		/* fifo size in bytes */
	unsigned int dig_groups;	/* digital groups */
} snd_pcm_hardware_t;

typedef struct _snd_pcm_ops {
	int (*open)(snd_pcm_substream_t *substream);
	int (*close)(snd_pcm_substream_t *substream);
	int (*ioctl)(snd_pcm_substream_t * substream,
		     unsigned int cmd, void *arg);
	int (*prepare)(snd_pcm_substream_t * substream);
	int (*trigger)(snd_pcm_substream_t * substream, int cmd);
	unsigned int (*pointer)(snd_pcm_substream_t * substream);
	int (*copy)(snd_pcm_substream_t *substream, int channel, unsigned int pos,
		    void *buf, size_t count);
	int (*silence)(snd_pcm_substream_t *substream, int channel, 
		       unsigned int pos, size_t count);
} snd_pcm_ops_t;

/*
 *
 */

#define SND_PCM_DEVICES		8

#define SND_PCM_IOCTL1_FALSE	((void *)0)
#define SND_PCM_IOCTL1_TRUE	((void *)1)

#define SND_PCM_IOCTL1_RESET		0
#define SND_PCM_IOCTL1_INFO		1
#define SND_PCM_IOCTL1_HW_PARAMS	2
#define SND_PCM_IOCTL1_DIG_INFO		3
#define SND_PCM_IOCTL1_DIG_PARAMS	4
#define SND_PCM_IOCTL1_CHANNEL_INFO	5

#define SND_PCM_TRIGGER_STOP		0
#define SND_PCM_TRIGGER_START		1
#define SND_PCM_TRIGGER_PAUSE_PUSH	3
#define SND_PCM_TRIGGER_PAUSE_RELEASE	4

/* If you change this don't forget to changed snd_pcm_rates table in pcm_lib.c */
#define SND_PCM_RATE_5512		(1<<0)		/* 5512Hz */
#define SND_PCM_RATE_8000		(1<<1)		/* 8000Hz */
#define SND_PCM_RATE_11025		(1<<2)		/* 11025Hz */
#define SND_PCM_RATE_16000		(1<<3)		/* 16000Hz */
#define SND_PCM_RATE_22050		(1<<4)		/* 22050Hz */
#define SND_PCM_RATE_32000		(1<<5)		/* 32000Hz */
#define SND_PCM_RATE_44100		(1<<6)		/* 44100Hz */
#define SND_PCM_RATE_48000		(1<<7)		/* 48000Hz */
#define SND_PCM_RATE_64000		(1<<8)		/* 64000Hz */
#define SND_PCM_RATE_88200		(1<<9)		/* 88200Hz */
#define SND_PCM_RATE_96000		(1<<10)		/* 96000Hz */
#define SND_PCM_RATE_176400		(1<<11)		/* 176400Hz */
#define SND_PCM_RATE_192000		(1<<12)		/* 192000Hz */

#define SND_PCM_RATE_CONTINUOUS		(1<<30)		/* continuous range */
#define SND_PCM_RATE_KNOT		(1<<31)		/* supports more non-continuos rates */

#define SND_PCM_RATE_8000_44100		(SND_PCM_RATE_8000|SND_PCM_RATE_11025|\
					 SND_PCM_RATE_16000|SND_PCM_RATE_22050|\
					 SND_PCM_RATE_32000|SND_PCM_RATE_44100)
#define SND_PCM_RATE_8000_48000		(SND_PCM_RATE_8000_44100|SND_PCM_RATE_48000)
#define SND_PCM_RATE_8000_96000		(SND_PCM_RATE_8000_48000|SND_PCM_RATE_64000|\
					 SND_PCM_RATE_88200|SND_PCM_RATE_96000)
#define SND_PCM_RATE_8000_192000	(SND_PCM_RATE_8000_96000|SND_PCM_RATE_176400|\
					 SND_PCM_RATE_192000)
struct _snd_pcm_file {
	snd_pcm_substream_t * substream;
	struct _snd_pcm_file * next;
};

typedef struct {
	unsigned long min;
	unsigned long max;
	unsigned int openmin:1,
		openmax:1,
		real:1,
		empty:1;
} interval_t;

#define VAR_CHANNELS		0
#define VAR_RATE		1
#define VAR_FRAGMENT_LENGTH	2
#define VAR_FRAGMENTS		3
#define VAR_BUFFER_LENGTH	4
#define VAR_SAMPLE_BITS		5
#define VAR_FRAME_BITS		6
#define VAR_FRAGMENT_SIZE	7
#define VAR_FRAGMENT_BYTES	8
#define VAR_BUFFER_SIZE		9
#define VAR_BUFFER_BYTES	10
#define VAR_LAST_INTERVAL	10
#define VAR_ACCESS		11
#define VAR_FORMAT		12
#define VAR_SUBFORMAT		13
#define VAR_LAST		13

#define VARBIT_CHANNELS		(1<<VAR_CHANNELS)
#define VARBIT_RATE		(1<<VAR_RATE)
#define VARBIT_FRAGMENT_LENGTH	(1<<VAR_FRAGMENT_LENGTH)
#define VARBIT_FRAGMENTS	(1<<VAR_FRAGMENTS)
#define VARBIT_BUFFER_LENGTH	(1<<VAR_BUFFER_LENGTH)
#define VARBIT_SAMPLE_BITS	(1<<VAR_SAMPLE_BITS)
#define VARBIT_FRAME_BITS	(1<<VAR_FRAME_BITS)
#define VARBIT_FRAGMENT_SIZE	(1<<VAR_FRAGMENT_SIZE)
#define VARBIT_FRAGMENT_BYTES	(1<<VAR_FRAGMENT_BYTES)
#define VARBIT_BUFFER_SIZE	(1<<VAR_BUFFER_SIZE)
#define VARBIT_BUFFER_BYTES	(1<<VAR_BUFFER_BYTES)
#define VARBIT_ACCESS		(1<<VAR_ACCESS)
#define VARBIT_FORMAT		(1<<VAR_FORMAT)
#define VARBIT_SUBFORMAT	(1<<VAR_SUBFORMAT)

typedef struct _snd_pcm_hw_infok {
	unsigned int access_mask;
	unsigned int format_mask;
	unsigned int subformat_mask;
	interval_t intervals[VAR_LAST_INTERVAL + 1];
	unsigned int info;		/* R: Info for returned setup */
	unsigned int msbits;		/* R: used most significant bits */
	unsigned int rate_num;		/* R: rate numerator */
	unsigned int rate_den;		/* R: rate denominator */
	size_t fifo_size;		/* R: chip FIFO size in frames */
	unsigned int dig_groups;	/* R: number of channel groups for digital setup */
} snd_pcm_hw_infok_t;

typedef struct _snd_pcm_hw_infoc_constr snd_pcm_hw_infoc_constr_t;

typedef int (*snd_pcm_hw_infoc_func_t)(snd_pcm_hw_infok_t *info,
				       snd_pcm_hw_infoc_constr_t *constr);

struct _snd_pcm_hw_infoc_constr {
	snd_pcm_hw_infoc_func_t func;
	unsigned int var;
	int deps[4];
	void *private;
};

typedef struct _snd_pcm_hw_infoc {
	unsigned int access_mask;
	unsigned int format_mask;
	unsigned int subformat_mask;
	interval_t intervals[VAR_LAST_INTERVAL + 1];
	unsigned int constr_num;
	unsigned int constr_all;
	snd_pcm_hw_infoc_constr_t *constrs;
} snd_pcm_hw_infoc_t;

typedef struct {
	unsigned long num;
	unsigned long den_min, den_max;
} rational_t;

typedef struct {
	int nrats;
	rational_t *rats;
} snd_pcm_hw_infoc_rats_t;

typedef struct {
	unsigned int count;
	unsigned long *list;
	unsigned int mask;
} snd_pcm_hw_infoc_list_t;

struct _snd_pcm_runtime {
	/* -- Status -- */
	snd_pcm_substream_t *trigger_master;
	snd_timestamp_t trigger_time;	/* trigger timestamp */
	int overrange;
	size_t avail_max;
	size_t hw_ptr_base;		/* Position at buffer restart */
	size_t hw_ptr_interrupt;	/* Position at interrupt time*/

	/* -- HW params -- */
	int access;			/* access mode */
	int format;			/* SND_PCM_FORMAT_* */
	int subformat;			/* subformat */
	unsigned int rate;		/* rate in Hz */
	unsigned int channels;		/* channels */
	size_t fragment_size;		/* fragment size */
	size_t fragments;		/* fragments */
	size_t buffer_size;		/* buffer size */
	size_t min_align;		/* Min alignment for the format */
	size_t byte_align;
	size_t bits_per_frame;
	size_t bits_per_sample;
	unsigned int info;
	unsigned int rate_num;
	unsigned int rate_den;

	/* -- SW params -- */
	unsigned int time: 1;		/* mmap timestamp is updated */
	int start_mode;			/* start mode */
	int ready_mode;			/* ready detection mode */
	size_t xfer_min;		/* xfer min size */
	size_t xfer_align;		/* xfer size need to be a multiple */
	int xrun_mode;			/* xrun detection mode */
	size_t boundary;		/* pointers wrap point */

	snd_pcm_sync_id_t sync;		/* hardware synchronization ID */

	/* -- mmap -- */
	snd_pcm_mmap_status_t *status;
	volatile snd_pcm_mmap_control_t *control;
	atomic_t mmap_count;

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

	/* -- hardware description -- */
	snd_pcm_hardware_t hw;
	snd_pcm_hw_infoc_t infoc;

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

struct _snd_pcm_substream {
	snd_pcm_t *pcm;
	snd_pcm_stream_t *pstr;
	int number;
	char name[32];			/* substream name */
	int stream;			/* stream (direction) */
	/* -- hardware operations -- */
	snd_pcm_ops_t *ops;
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


struct _snd_pcm_stream {
	int stream;				/* stream (direction) */
	snd_pcm_t *pcm;
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

struct _snd_pcm {
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

typedef struct _snd_pcm_notify {
	int (*n_register) (unsigned short minor, snd_pcm_t * pcm);
	int (*n_unregister) (unsigned short minor, snd_pcm_t * pcm);
	struct list_head list;
} snd_pcm_notify_t;

/*
 *  Registering
 */

extern snd_pcm_t *snd_pcm_devices[];

extern void snd_pcm_lock(int unlock);

extern int snd_pcm_new(snd_card_t * card, char *id, int device,
		       int playback_count, int capture_count, snd_pcm_t **rpcm);

extern int snd_pcm_notify(snd_pcm_notify_t *notify, int nfree);

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
	return frames_to_bytes(runtime, runtime->fragment_size);
}

/*
 *  result is: 0 ... (frag_boundary - 1)
 */
static inline size_t snd_pcm_playback_avail(snd_pcm_runtime_t *runtime)
{
	ssize_t avail = runtime->status->hw_ptr + runtime->buffer_size - runtime->control->appl_ptr;
	if (avail < 0)
		avail += runtime->boundary;
	return avail;
}

/*
 *  result is: 0 ... (frag_boundary - 1)
 */
static inline size_t snd_pcm_capture_avail(snd_pcm_runtime_t *runtime)
{
	ssize_t avail = runtime->status->hw_ptr - runtime->control->appl_ptr;
	if (avail < 0)
		avail += runtime->boundary;
	return avail;
}

static inline void snd_pcm_trigger_done(snd_pcm_substream_t *substream, 
					snd_pcm_substream_t *master)
{
	substream->runtime->trigger_master = master;
}

extern int interval_refine(interval_t *i, const interval_t *v);
extern int interval_mul(interval_t *a, interval_t *b, interval_t *c);
extern int interval_div(interval_t *a, interval_t *b, interval_t *c);
extern int interval_mul1(interval_t *a, unsigned long k,
			 interval_t *b, interval_t *c);
extern int interval_div1(interval_t *a, unsigned long k,
			 interval_t *b, interval_t *c);
extern int interval_list(interval_t *i, 
			 size_t count, unsigned long *list, unsigned int mask);
extern int interval_step(interval_t *i, unsigned long min, unsigned long step);
extern int interval_rational(interval_t *i,
			     unsigned int rats_count, rational_t *rats,
			     unsigned long *nump, unsigned long *denp);

extern int snd_pcm_hw_infoc_list(snd_pcm_runtime_t *runtime, unsigned int var,
				 snd_pcm_hw_infoc_list_t *l);
extern int snd_pcm_hw_infoc_clocks(snd_pcm_runtime_t *runtime, 
				   snd_pcm_hw_infoc_rats_t *r);
extern int snd_pcm_hw_infoc_msbits(snd_pcm_runtime_t *runtime, 
				   unsigned int width,
				   unsigned int msbits);
extern int snd_pcm_hw_infoc_minmax(snd_pcm_runtime_t *runtime, unsigned int var,
				   unsigned long min, unsigned long max);
extern int snd_pcm_hw_infoc_add(snd_pcm_runtime_t *runtime, unsigned int var,
				snd_pcm_hw_infoc_func_t func, void *private,
				int dep, ...);

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
 
extern void snd_pcm_set_ops(snd_pcm_t * pcm, int direction, snd_pcm_ops_t *ops);
extern void snd_pcm_set_sync(snd_pcm_substream_t * substream);
extern int snd_pcm_lib_interleave_len(snd_pcm_substream_t *substream);
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
