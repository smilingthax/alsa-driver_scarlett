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

typedef struct snd_stru_pcm_oss_setup snd_pcm_oss_setup_t;

struct snd_stru_pcm_oss_setup {
	char *task_name;
	unsigned int playback_only:1,
		     block:1,
		     nonblock:1;
	unsigned int fragments;
	unsigned int fragment_size;
	snd_pcm_oss_setup_t *next;
};

typedef struct snd_stru_pcm_oss_runtime {
	int params: 1,				/* format/parameter change */
            prepare: 1,				/* need to prepare the operation */
            trigger: 1,				/* trigger flag */
            sync_trigger: 1;			/* sync trigger flag */
	int rate;				/* requested rate */
	int format;				/* requested OSS format */
	int voices;				/* requested voices */
	int fragment;				/* requested OSS fragment */
	int subdivision;			/* requested subdivision */
	int fragment_size;			/* requested fragment size */
	int fragments;				/* requested fragments */
	int mmap_fragment_size;
	int mmap_fragments;
	char *buffer;				/* vmallocated fragment */
	int buffer_used;			/* used length from buffer */
	snd_pcm_plugin_t *plugin;
	char *xbuffer[2];
	long xbuffer_size[2];
	char xbuffer_lock[2];
} snd_pcm_oss_runtime_t;

typedef struct snd_stru_pcm_oss_subchn {
	int oss: 1;				/* oss mode */
	snd_pcm_oss_setup_t *setup;		/* active setup */
} snd_pcm_oss_subchn_t;

typedef struct snd_stru_pcm_oss_channel {
	snd_pcm_oss_setup_t *setup_list;	/* setup list */
        struct semaphore setup_mutex;
        snd_info_entry_t *proc_entry;
} snd_pcm_oss_channel_t;

typedef struct snd_stru_pcm_oss {
	int reg;
	snd_info_entry_t *proc_ctrl_entry;
} snd_pcm_oss_t;

#endif /* __PCM_OSS_H */
