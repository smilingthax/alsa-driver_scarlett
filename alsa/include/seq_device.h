/*
 *  ALSA sequencer device management
 *  Copyright (c) 1999 by Takashi Iwai <iwai@ww.uni-erlangen.de>
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

#ifndef __SND_SEQ_DEVICE_H
#define __SND_SEQ_DEVICE_H

typedef struct snd_seq_device snd_seq_device_t;
typedef struct snd_seq_dev_ops snd_seq_dev_ops_t;
typedef void *snd_seq_dev_entry_t; /* generic pointer */

/* driver operators
 * init_device:
 *	Initialize the device with given parameters.
 *	Typically,
 *		1. call snd_hwdep_new
 *		2. allocate private data and initialize it
 *		3. call snd_hwdep_register
 *		4. store the instance to result pointer.
 *		
 * free_device:
 *	Release the private data.
 *	Typically, call snd_hwdep_unregister(entry)
 */
struct snd_seq_dev_ops {
	int (*init_device)(snd_card_t *card, int device, char *name, char *id, void *arg, int size, snd_seq_dev_entry_t *result);
	int (*free_device)(snd_card_t *card, int device, void *arg, int size, snd_seq_dev_entry_t entry);
};

/*
 * prototypes
 */
int snd_seq_device_register(snd_card_t *card, int device, char *name, char *id, void *arg, int size, snd_seq_device_t **result);
snd_seq_device_t *snd_seq_device_find(snd_card_t *card, int device, char *id);
int snd_seq_device_unregister(snd_seq_device_t *dev);
int snd_seq_device_register_driver(char *id, snd_seq_dev_ops_t *entry);
int snd_seq_device_unregister_driver(char *id);

/*
 * id strings for generic devices
 */
#define SND_SEQ_DEV_MIDISYNTH	"synth-midi"
#define SND_SEQ_DEV_OPL3	"synth-opl3"


#endif
