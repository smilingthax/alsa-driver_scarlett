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

/*
 *  Raw MIDI interface
 */

#define SND_RAWMIDI_DEVICES	4

#define SND_RAWMIDI_HW_POLL	0x00000001	/* polled mode */

#define SND_RAWMIDI_MODE_STREAM	0x00000000	/* stream mode */
#define SND_RAWMIDI_MODE_SEQ	0x00000001	/* sequencer mode */

#define SND_RAWMIDI_FLG_TRIGGER	0x00000001	/* trigger in progress */
#define SND_RAWMIDI_FLG_TIMER	0x00000002	/* polling timer armed */
#define SND_RAWMIDI_FLG_OSS	0x80000000	/* OSS compatible mode */

#define SND_RAWMIDI_LFLG_OUTPUT	0x00000001	/* open for output */
#define SND_RAWMIDI_LFLG_INPUT	0x00000002	/* open for input */
#define SND_RAWMIDI_LFLG_OPEN	0x00000003	/* open */

typedef struct snd_stru_rawmidi_direction snd_rawmidi_direction_t;

struct snd_stru_rawmidi_direction_hw {
	unsigned int flags;	/* SND_RAWMIDI_HW_XXXX */
	void *private_data;
	void (*private_free) (void *private_data);
	int (*open) (snd_rawmidi_t * rmidi);
	int (*close) (snd_rawmidi_t * rmidi);
	void (*trigger) (snd_rawmidi_t * rmidi, int up);
	union {
		void (*read) (snd_rawmidi_t * rmidi);
		void (*write) (snd_rawmidi_t * rmidi);
	} io;
	void (*abort) (snd_rawmidi_t * rmidi);
};

struct snd_stru_rawmidi_direction {
	unsigned int mode;	/* SND_RAWMIDI_MODE_XXXX */
	unsigned int flags;	/* SND_RAWMIDI_FLG_XXXX */
	struct {
		struct {
			/* midi stream buffer */
			unsigned char *buffer;	/* buffer for MIDI data */
			unsigned int size;	/* size of buffer */
			unsigned int head;	/* buffer head index */
			unsigned int tail;	/* buffer tail index */
			unsigned int used;	/* buffer used index */
			unsigned int used_max;	/* max used buffer for wakeup */
			unsigned int used_room;	/* min room in buffer for wakeup */
			unsigned int used_min;	/* min used buffer for wakeup */
			unsigned int xruns;	/* over/underruns counter */
		} s;
		struct {
			/* upper layer - parses MIDI v1.0 data */
			unsigned char *cbuffer;	/* command buffer */
			unsigned int csize;	/* command buffer size */
			unsigned int cused;	/* command buffer used */
			unsigned int cleft;	/* command buffer left */
			unsigned char cprev;	/* previous command */
			void *cmd_private_data;	/* private data for command */
			void (*command) (snd_rawmidi_t * rmidi,
					 void *cmd_private_data,
					 unsigned char *command,
					 int count);
		} p;
	} u;
	/* misc */
	unsigned int bytes;
	struct timer_list timer;	/* poll timer */
	/* callback */
	int (*reset) (snd_rawmidi_t * rmidi);	/* reset MIDI command!!! */
	int (*data) (snd_rawmidi_t * rmidi, char *buffer, int count);
	/* switches */
	snd_kswitch_list_t switches;
	/* hardware layer */
	struct snd_stru_rawmidi_direction_hw hw;
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

	snd_rawmidi_direction_t input;
	snd_rawmidi_direction_t output;

	void *private_data;
	void (*private_free) (void *private_data);

	spinlock_t input_lock;
	spinlock_t output_lock;
	wait_queue_head_t input_sleep;
	wait_queue_head_t output_sleep;
	struct semaphore open_mutex;

	snd_info_entry_t *dev;
	snd_info_entry_t *proc_entry;
};

/* main rawmidi functions */

extern snd_rawmidi_t *snd_rawmidi_new_device(snd_card_t * card, char *id);
extern int snd_rawmidi_free(snd_rawmidi_t * rmidi);
extern int __snd_rawmidi_register(snd_rawmidi_t * rmidi, int rawmidi_device);
extern int __snd_rawmidi_unregister(snd_rawmidi_t * rmidi);
#ifdef CONFIG_SND_SEQUENCER
#include "seq_device.h"
static inline int snd_rawmidi_register(snd_rawmidi_t * rmidi, int rawmidi_device)
{
	int err;

	if ((err = __snd_rawmidi_register(rmidi, rawmidi_device))<0)
		return err;
	snd_seq_device_register(rmidi->card, rmidi->device, NULL,
				SND_SEQ_DEV_MIDISYNTH, NULL, 0, NULL);
	return err;
}
static inline int snd_rawmidi_unregister(snd_rawmidi_t * rmidi)
{
	snd_seq_device_t *dev;
	dev = snd_seq_device_find(rmidi->card, rmidi->device, SND_SEQ_DEV_MIDISYNTH);
	if (dev)
		snd_seq_device_unregister(dev);
	return __snd_rawmidi_unregister(rmidi);
}
#else
#define snd_rawmidi_register __snd_rawmidi_register
#define snd_rawmidi_unregister __snd_rawmidi_unregister
#endif
extern int snd_rawmidi_switch_add(snd_rawmidi_direction_t * dir, snd_kswitch_t * ksw);
extern int snd_rawmidi_switch_remove(snd_rawmidi_direction_t * dir, snd_kswitch_t * ksw);
extern snd_kswitch_t *snd_rawmidi_switch_new(snd_rawmidi_direction_t * dir, snd_kswitch_t * ksw, void *private_data);

/* control functions */

extern int snd_rawmidi_control_ioctl(snd_card_t * card,
				     snd_control_t * control,
				     unsigned int cmd,
				     unsigned long arg);

/* main midi functions */

extern int snd_midi_info(int cardnum, int device, snd_rawmidi_info_t *info);
extern int snd_midi_open(int cardnum, int device, int mode, snd_rawmidi_t ** out);
extern int snd_midi_close(int cardnum, int device, int mode);
extern int snd_midi_drain_output(snd_rawmidi_t * rmidi);
extern int snd_midi_flush_output(snd_rawmidi_t * rmidi);
extern int snd_midi_stop_input(snd_rawmidi_t * rmidi);
extern int snd_midi_start_input(snd_rawmidi_t * rmidi);
extern int snd_midi_transmit(snd_rawmidi_t * rmidi, char *buf, int count);

#endif				/* __MIDI_H */
