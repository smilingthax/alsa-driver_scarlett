/*
 *  MIDI byte <-> sequencer event coder
 *
 *  Copyright (C) 1998,99 Takashi Iwai <iwai@ww.uni-erlangen.de>,
 *                        Jaroslav Kysela <perex@suse.cz>
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
 */

#ifndef __MIDI_CODER_H
#define __MIDI_CODER_H

#ifndef ALSA_BUILD
#include <linux/asequencer.h>
#else
#include "asequencer.h"
#endif

#define MAX_MIDI_EVENT_BUF	256

typedef struct snd_midi_event_t snd_midi_event_t;

/* midi status */
struct snd_midi_event_t {
	int qlen;	/* queue length */
	int read;	/* chars read */
	int status;	/* running status */
	int type;	/* current event type */
	unsigned char lastcmd;
	unsigned char buf[MAX_MIDI_EVENT_BUF];	/* input buffer */
	spinlock_t lock;
};

int snd_midi_event_new(snd_midi_event_t **rdev);
void snd_midi_event_free(snd_midi_event_t *dev);
void snd_midi_event_init(snd_midi_event_t *dev);
void snd_midi_event_reset_encode(snd_midi_event_t *dev);
void snd_midi_event_reset_decode(snd_midi_event_t *dev);
/* encode from byte stream - return number of written bytes if success */
long snd_midi_event_encode(snd_midi_event_t *dev, char *buf, long count, snd_seq_event_t *ev);
int snd_midi_event_encode_byte(snd_midi_event_t *dev, int c, snd_seq_event_t *ev);
/* decode from event to bytes - return number of written bytes if success */
long snd_midi_event_decode(snd_midi_event_t *dev, char *buf, long count, snd_seq_event_t *ev);

#endif
