#ifndef __MIDI_H
#define __MIDI_H

/*
 *  Abstract layer for MIDI v1.0 stream
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

#ifdef CONFIG_SND_SEQUENCER
#include "seq_device.h"
#endif

/*
 *  Raw MIDI interface
 */

#define SND_RAWMIDI_DEVICES	8

#define SND_RAWMIDI_FLG_TRIGGER	0x00000001	/* trigger in progress */
#define SND_RAWMIDI_FLG_FLUSH	0x00000002	/* flush */
#define SND_RAWMIDI_FLG_OSS	0x80000000	/* OSS compatible mode */

#define SND_RAWMIDI_LFLG_OUTPUT	0x00000001	/* open for output */
#define SND_RAWMIDI_LFLG_INPUT	0x00000002	/* open for input */
#define SND_RAWMIDI_LFLG_OPEN	0x00000003	/* open */
#define SND_RAWMIDI_LFLG_APPEND	0x00000004	/* append flag for output */

typedef struct snd_stru_rawmidi_stream snd_rawmidi_stream_t;

struct snd_stru_rawmidi_stream_hw {
	void *private_data;
	void (*private_free) (void *private_data);
	int (*open) (snd_rawmidi_t * rmidi);
	int (*close) (snd_rawmidi_t * rmidi);
	void (*trigger) (snd_rawmidi_t * rmidi, int up);
	void (*abort) (snd_rawmidi_t * rmidi);
};

struct snd_stru_rawmidi_stream {
	unsigned int flags;	/* SND_RAWMIDI_FLG_XXXX */
	int use_count;		/* use counter (for output) */
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
	/* event handler (room [output] or new bytes [input]) */
	void (*event)(snd_rawmidi_t *rmidi);
	void *private_data;
	void (*private_free)(void *private_data);
	/* hardware layer */
	struct snd_stru_rawmidi_stream_hw hw;
};

struct snd_stru_rawmidi {
	snd_card_t *card;

	unsigned int device;		/* device number */
	unsigned int flags;		/* SND_RAWMIDI_LFLG_XXXX */
	unsigned int info_flags;	/* SND_RAWMIDI_INFO_XXXX */
	char id[64];
	char name[80];

#ifdef CONFIG_SND_OSSEMUL
	int ossreg;
#endif

	snd_rawmidi_stream_t streams[2];

	void *private_data;
	void (*private_free) (void *private_data);

	struct semaphore open_mutex;
	wait_queue_head_t open_wait;

	snd_info_entry_t *dev;
	snd_info_entry_t *proc_entry;

#ifdef CONFIG_SND_SEQUENCER
	snd_seq_device_t *seq_dev;
#endif
};

/* main rawmidi functions */

extern int snd_rawmidi_new(snd_card_t * card, char *id, int device, snd_rawmidi_t ** rmidi);

/* control functions */

extern int snd_rawmidi_control_ioctl(snd_card_t * card,
				     snd_kctl_t * control,
				     unsigned int cmd,
				     unsigned long arg);

/* callbacks */

void snd_rawmidi_receive_reset(snd_rawmidi_t * rmidi);
int snd_rawmidi_receive(snd_rawmidi_t * rmidi, unsigned char *buffer, int count);
void snd_rawmidi_transmit_reset(snd_rawmidi_t * rmidi);
int snd_rawmidi_transmit_empty(snd_rawmidi_t * rmidi);
int snd_rawmidi_transmit(snd_rawmidi_t * rmidi, unsigned char *buffer, int count);

/* main midi functions */

int snd_rawmidi_kernel_info(int cardnum, int device, snd_rawmidi_info_t *info);
int snd_rawmidi_kernel_open(int cardnum, int device, int mode, snd_rawmidi_t ** out);
int snd_rawmidi_kernel_release(snd_rawmidi_t * rmidi, int mode);
int snd_rawmidi_output_params(snd_rawmidi_t * rmidi, snd_rawmidi_params_t * params);
int snd_rawmidi_input_params(snd_rawmidi_t * rmidi, snd_rawmidi_params_t * params);
int snd_rawmidi_drain_output(snd_rawmidi_t * rmidi);
int snd_rawmidi_flush_output(snd_rawmidi_t * rmidi);
int snd_rawmidi_flush_input(snd_rawmidi_t * rmidi);
long snd_rawmidi_kernel_read(snd_rawmidi_t * rmidi, unsigned char *buf, long count);
long snd_rawmidi_kernel_write(snd_rawmidi_t * rmidi, const unsigned char *buf, long count);

#endif				/* __MIDI_H */
