#ifndef __PMIX_H
#define __PMIX_H

/*
 *  Digital Audio (PCM) abstract layer / Mixing devices
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
 
typedef struct snd_pmix snd_pmix_t;
typedef struct snd_pmix_data snd_pmix_data_t;

#define SND_PMIX_FLG_MMAP	0x00000001	/* mmaped access? */
#define SND_PMIX_FLG_BUFFERS	0x00000002	/* need computer buffers/fragments? */
#define SND_PMIX_FLG_NEUTRAL	0x00000004	/* need neutral fill */
#define SND_PMIX_FLG_TRIGGER1	0x00000010	/* output is active (kernel level) */
#define SND_PMIX_FLG_TRIGGER2	0x00000020	/* output is active (user level) */
#define SND_PMIX_FLG_TRIGGERA	(SND_PMIX_FLG_TRIGGER1|SND_PMIX_FLG_TRIGGER2)
#define SND_PMIX_FLG_ABORT	0x00000040	/* abort in progress */
#define SND_PMIX_FLG_SLEEP	0x00000080	/* process is sleeping */

struct snd_pmix {
  struct snd_pmix *next;
  snd_pmix_data_t *pdata;
  unsigned char *buffer;
  unsigned int size;		/* buffer size */
  unsigned int flags;		/* flags */
  unsigned int rate;		/* playback rate */
  unsigned int format;		/* playback format (ALSA) */
  unsigned int channels;	/* channels */
  unsigned int used_size;	/* used buffer size */
  unsigned int bsize;		/* block size */
  unsigned int blocks;		/* total blocks */
  unsigned int used;		/* used blocks */
  unsigned int head;		/* head block */
  unsigned int tail;		/* tail block */
  unsigned int frag_size;	/* size of partly filled block */
  unsigned int interrupts;	/* interrupts (blocks played) */
  unsigned int processed_bytes;
  unsigned int requested_block_size;
  unsigned int requested_blocks;
  unsigned int neutral_byte;
  snd_spin_define( playback );
  snd_spin_define( sleep );
  snd_sleep_define( sleep );
};
    
struct snd_pmix_data {
  snd_card_t *card;
  int device;
  snd_pcm_t *pcm;
  unsigned int rate;		/* rate in Hz */
  unsigned int format;		/* playback format */
  unsigned int channels;	/* playback channels */
  unsigned int fsize;		/* fragment size */
  unsigned int bsize;		/* buffer size */
  unsigned int ssize;		/* subprocess buffer size */
  unsigned char *buffer;	/* mixing buffer (for one fragment) */
  int quit;			/* end mixing thread */
  int running;			/* thread is running */
  int pid;			/* pid of mixing thread */
  int inputs;			/* number of input processes */
  snd_pmix_t *first;
  struct file file;             /* needed for file -> private.data and permissions */
  snd_info_entry_t *dev;
  snd_info_entry_t *oss_dev;
  snd_sleep_define( tstart );
  snd_sleep_define( tsleep );
  snd_mutex_define( open );
  snd_mutex_define( add );
};

int snd_pmix_open_device( snd_pmix_data_t *pdata, snd_pmix_t *pmix );
int snd_pmix_release_device( snd_pmix_data_t *pdata, snd_pmix_t *pmix );

#endif /* __PMIX_H */
