#ifndef __SEQ_VIRMIDI_H
#define __SEQ_VIRMIDI_H

/*
 *  Virtual Raw MIDI client on Sequencer
 *  Copyright (c) 2000 by Takashi Iwai <iwai@ww.uni-erlangen.de>
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

#include "rawmidi.h"
#include "seq_midi_event.h"

typedef struct snd_virmidi_channel snd_virmidi_channel_t;
typedef struct snd_virmidi snd_virmidi_t;
typedef struct snd_virmidi_dev snd_virmidi_dev_t;

/*
 * read/write channel
 */
struct snd_virmidi_channel {
	unsigned int flags;
	/* midi stream buffer */
	unsigned char *buffer;	/* buffer for MIDI data */
	unsigned int size;	/* size of buffer */
	unsigned int head;	/* buffer head index */
	unsigned int tail;	/* buffer tail index */
	unsigned int used;	/* buffer used counter */
	unsigned int used_max;	/* max used buffer for wakeup */
	unsigned int used_room;	/* min room in buffer for wakeup */
	unsigned int used_min;	/* min used buffer for wakeup */
	unsigned int xruns;	/* over/underruns counter */
	/* misc */
	unsigned int bytes;
	spinlock_t lock;
	wait_queue_head_t sleep;
};

/*
 * device file instance
 */
struct snd_virmidi {
	snd_virmidi_dev_t *devptr;
	int client;
	int port;
	snd_virmidi_channel_t chn[2];
	snd_midi_event_t *parser;
	unsigned int flags;		/* SND_RAWMIDI_LFLG_XXXX */
	snd_virmidi_t *next;
};


/*
 * device record
 */
struct snd_virmidi_dev {

	snd_card_t *card;
	unsigned int device;		/* device number */
	unsigned int info_flags;	/* SND_RAWMIDI_INFO_XXXX */
	char id[64];
	char name[80];

	int client;
	int port;

	rwlock_t list_lock;
	int files;
	snd_virmidi_t *filelist;

#ifdef CONFIG_SND_OSSEMUL
	int ossreg;
#endif
	/*
	void *private_data;
	void (*private_free) (void *private_data);
	*/

	struct semaphore open_mutex;

	snd_info_entry_t *proc_entry;

	snd_kswitch_list_t switches[2];
};

int snd_virmidi_new(snd_card_t *card, char *id, int device, snd_virmidi_dev_t **rmidi);
#if 0
int snd_virmidi_switch_add(snd_rawmidi_channel_t *dir, snd_kswitch_t *ksw);
int snd_virmidi_switch_remove(snd_rawmidi_channel_t *dir, snd_kswitch_t *ksw);
snd_kswitch_t *snd_virmidi_switch_new(snd_virmidi_channel_t *dir, snd_kswitch_t *ksw, void *private_data);
#endif

#endif
