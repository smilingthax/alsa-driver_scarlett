#ifndef __SEQ_VIRMIDI_H
#define __SEQ_VIRMIDI_H

/*
 *  Virtual Raw MIDI client on Sequencer
 *  Copyright (c) 2000 by Takashi Iwai <tiwai@suse.de>
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

typedef struct snd_virmidi_stream snd_virmidi_stream_t;
typedef struct snd_virmidi snd_virmidi_t;
typedef struct snd_virmidi_dev snd_virmidi_dev_t;

/*
 * read/write channel - idetical with rawmidi stuff
 */
struct snd_virmidi_stream {
	unsigned int flags;
	/* midi stream buffer */
	unsigned char *buffer;	/* buffer for MIDI data */
	size_t buffer_size;	/* size of buffer */
	size_t appl_ptr;	/* application pointer */
	size_t hw_ptr;		/* hardware pointer */
	size_t avail_min;	/* min avail for wakeup */
	size_t avail;		/* max used buffer for wakeup */
	size_t xruns;		/* over/underruns counter */
	/* misc */
	size_t bytes;
	spinlock_t lock;
	wait_queue_head_t sleep;
};

/*
 * device file instance:
 * This instance is created at each time the midi device file is
 * opened.  Each instance has its own input buffer and MIDI parser
 * (buffer), and is associated with the device instance.
 */
struct snd_virmidi {
	snd_virmidi_dev_t *devptr;
	int seq_mode;
	int client;
	int port;
	snd_virmidi_stream_t streams[2];
	snd_midi_event_t *parser;
	unsigned int flags;		/* SND_RAWMIDI_LFLG_XXXX */
	snd_virmidi_t *next;		/* next file instance */
};


typedef void (*snd_virmidi_private_free_t)(snd_virmidi_dev_t *devp);

/*
 * device record:
 * Each virtual midi device has one device instance.  It contains
 * common information and the linked-list of opened files, 
 */
struct snd_virmidi_dev {

	snd_card_t *card;
	unsigned int device;		/* device number */
	unsigned int dev_flags;		/* file permission: SND_RAWMIDI_LFLG_XXXX */
	unsigned int seq_flags;		/* current connection: SND_RAWMIDI_LFLG_XXXX */
	char id[64];
	char name[80];

	int seq_mode;			/* SND_VIRMIDI_XXX */
	int client;			/* created/attached client */
	int port;			/* created/attached port */

	/* linked-list of opened files */
	rwlock_t list_lock;
	int files;
	int read_files;
	int write_files;
	snd_virmidi_t *filelist;

#ifdef CONFIG_SND_OSSEMUL
	int ossreg;
#endif
	void *private_data;
	snd_virmidi_private_free_t private_free;

	struct semaphore open_mutex;

	snd_info_entry_t *proc_entry;
};

/* sequencer mode:
 * ATTACH = input/output events from midi device are routed to the
 *          attached sequencer port.  sequencer port is not created
 *          by virmidi itself.
 * DISPATCH = input/output events are routed to subscribers.
 *            sequencer port is created in virmidi.
 */
#define SND_VIRMIDI_SEQ_NONE		0
#define SND_VIRMIDI_SEQ_ATTACH		1
#define SND_VIRMIDI_SEQ_DISPATCH	2

int snd_virmidi_new(snd_card_t *card, int device, snd_virmidi_dev_t **rmidi);
int snd_virmidi_dev_receive_event(snd_virmidi_dev_t *rdev, snd_seq_event_t *ev);

#endif
