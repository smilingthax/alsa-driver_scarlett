#ifndef __PCM_OSS_H
#define __PCM_OSS_H

/*
 *  Digital Audio (PCM) - OSS compatibility abstract layer
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

#include "pcm_plugin.h"

#define SND_PCM_AFMT_QUERY		0
#define SND_PCM_AFMT_MU_LAW		(1<<0)
#define SND_PCM_AFMT_A_LAW		(1<<1)
#define SND_PCM_AFMT_IMA_ADPCM		(1<<2)
#define SND_PCM_AFMT_U8			(1<<3)
#define SND_PCM_AFMT_S16_LE		(1<<4)
#define SND_PCM_AFMT_S16_BE		(1<<5)
#define SND_PCM_AFMT_S8			(1<<6)
#define SND_PCM_AFMT_U16_LE		(1<<7)
#define SND_PCM_AFMT_U16_BE		(1<<8)
#define SND_PCM_AFMT_MPEG		(1<<9)

#define SND_PCM_ENABLE_CAPTURE		0x00000001
#define SND_PCM_ENABLE_PLAYBACK		0x00000002

#define SND_PCM_CAP_REVISION		0x000000ff
#define SND_PCM_CAP_DUPLEX		0x00000100
#define SND_PCM_CAP_REALTIME		0x00000200
#define SND_PCM_CAP_BATCH		0x00000400
#define SND_PCM_CAP_COPROC		0x00000800
#define SND_PCM_CAP_TRIGGER		0x00001000
#define SND_PCM_CAP_MMAP		0x00002000

#define SND_PCM_AFP_NORMAL		0
#define SND_PCM_AFP_NETWORK		1
#define SND_PCM_AFP_CPUINTENS		2

struct snd_pcm_buffer_info {
	int fragments;		/* # of available fragments (partially used ones not counted) */
	int fragstotal;		/* Total # of fragments allocated */
	int fragsize;		/* Size of a fragment in bytes */
	int bytes;		/* Available space in bytes (includes partially used fragments) */
};

struct snd_pcm_count_info {
	int bytes;		/* Total # of bytes processed */
	int blocks;		/* # of fragment transitions since last time */
	int ptr;		/* Current DMA pointer value */
};

struct snd_pcm_buffer_description {
	unsigned char *buffer;
	int size;
};

#define SND_PCM_IOCTL_OSS_RESET		_IO  ('P', 0)
#define SND_PCM_IOCTL_OSS_SYNC		_IO  ('P', 1)
#define SND_PCM_IOCTL_OSS_RATE		_IOWR('P', 2, int)
#define SND_PCM_IOCTL_OSS_GETRATE	_IOR ('P', 2, int)
#define SND_PCM_IOCTL_OSS_STEREO	_IOWR('P', 3, int)
#define SND_PCM_IOCTL_OSS_GETBLKSIZE	_IOWR('P', 4, int)
#define SND_PCM_IOCTL_OSS_FORMAT	_IOWR('P', 5, int)
#define SND_PCM_IOCTL_OSS_GETFORMAT	_IOR ('P', 5, int)
#define SND_PCM_IOCTL_OSS_CHANNELS	_IOWR('P', 6, int)
#define SND_PCM_IOCTL_OSS_GETCHANNELS	_IOR ('P', 6, int)
#define SND_PCM_IOCTL_OSS_FILTER	_IOWR('P', 7, int)
#define SND_PCM_IOCTL_OSS_GETFILTER	_IOR ('P', 7, int)
#define SND_PCM_IOCTL_OSS_POST		_IO  ('P', 8 )
#define SND_PCM_IOCTL_OSS_SUBDIVIDE	_IOWR('P', 9, int)
#define SND_PCM_IOCTL_OSS_SETFRAGMENT	_IOWR('P', 10, int)
#define SND_PCM_IOCTL_OSS_GETFORMATS	_IOR ('P', 11, int)
#define SND_PCM_IOCTL_OSS_GETPBKSPACE	_IOR ('P', 12, struct snd_pcm_buffer_info)
#define SND_PCM_IOCTL_OSS_GETRECSPACE	_IOR ('P', 13, struct snd_pcm_buffer_info)
#define SND_PCM_IOCTL_OSS_NONBLOCK	_IO  ('P', 14)
#define SND_PCM_IOCTL_OSS_GETCAPS	_IOR ('P', 15, int)
#define SND_PCM_IOCTL_OSS_GETTRIGGER	_IOR ('P', 16, int)
#define SND_PCM_IOCTL_OSS_SETTRIGGER	_IOW ('P', 16, int)
#define SND_PCM_IOCTL_OSS_GETRECPTR	_IOR ('P', 17, struct snd_pcm_count_info)
#define SND_PCM_IOCTL_OSS_GETPBKPTR	_IOR ('P', 18, struct snd_pcm_count_info)
#define SND_PCM_IOCTL_OSS_MAPRECBUFFER	_IOR ('P', 19, struct snd_pcm_buffer_description)
#define SND_PCM_IOCTL_OSS_MAPPBKBUFFER	_IOR ('P', 20, struct snd_pcm_buffer_description)
#define SND_PCM_IOCTL_OSS_SYNCRO	_IO  ('P', 21)
#define SND_PCM_IOCTL_OSS_DUPLEX	_IO  ('P', 22)
#define SND_PCM_IOCTL_OSS_GETODELAY	_IOR ('P', 23, int)
#define SND_PCM_IOCTL_OSS_PROFILE	_IOW ('P', 23, int)

typedef struct _snd_pcm_oss_setup snd_pcm_oss_setup_t;

struct _snd_pcm_oss_setup {
	char *task_name;
	unsigned int disable:1,
		     direct:1,
		     block:1,
		     nonblock:1;
	unsigned int fragments;
	unsigned int fragment_size;
	snd_pcm_oss_setup_t *next;
};

typedef struct _snd_pcm_oss_runtime {
	int params: 1,				/* format/parameter change */
            prepare: 1,				/* need to prepare the operation */
            trigger: 1,				/* trigger flag */
            sync_trigger: 1;			/* sync trigger flag */
	int rate;				/* requested rate */
	int format;				/* requested OSS format */
	unsigned int channels;			/* requested channels */
	unsigned int fragshift;
	unsigned int maxfrags;
	unsigned int subdivision;		/* requested subdivision */
	size_t fragment_size;			/* requested fragment size */
	unsigned int fragments;
	size_t buffer_size;			/* requested fragment size */
	size_t bytes;				/* total # bytes processed */
	size_t mmap_size;
	char *buffer;				/* vmallocated fragment */
	size_t buffer_used;			/* used length from buffer */
	snd_pcm_plugin_t *plugin_first;
	snd_pcm_plugin_t *plugin_last;
	unsigned int prev_hw_ptr_interrupt;
} snd_pcm_oss_runtime_t;

typedef struct _snd_pcm_oss_file {
	snd_pcm_substream_t *streams[2];
} snd_pcm_oss_file_t;

typedef struct _snd_pcm_oss_substream {
	int oss: 1;				/* oss mode */
	snd_pcm_oss_setup_t *setup;		/* active setup */
	snd_pcm_oss_file_t *file;
} snd_pcm_oss_substream_t;

typedef struct _snd_pcm_oss_stream {
	snd_pcm_oss_setup_t *setup_list;	/* setup list */
        struct semaphore setup_mutex;
        snd_info_entry_t *proc_entry;
} snd_pcm_oss_stream_t;

typedef struct _snd_pcm_oss {
	int reg;
	snd_info_entry_t *proc_ctrl_entry;
} snd_pcm_oss_t;

#endif /* __PCM_OSS_H */
