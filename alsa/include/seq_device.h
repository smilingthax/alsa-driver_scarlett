/*
 *  ALSA sequencer device management
 *  Copyright (c) 1999 by Takashi Iwai <tiwai@suse.de>
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

/*
 * registered device information
 */

#define ID_LEN	32

/* status flag */
#define SND_SEQ_DEVICE_FREE		0
#define SND_SEQ_DEVICE_REGISTERED	1

struct snd_seq_device {
	/* device info */
	snd_card_t *card;	/* sound card */
	int device;		/* device number */
	char id[ID_LEN];	/* driver id */
	char name[80];		/* device name */
	int argsize;		/* size of the argument */
	void *driver_data;	/* private data for driver */
	int status;		/* flag - read only */
	snd_seq_device_t *next; /* link to next device */
};


/* driver operators
 * init_device:
 *	Initialize the device with given parameters.
 *	Typically,
 *		1. call snd_hwdep_new
 *		2. allocate private data and initialize it
 *		3. call snd_hwdep_register
 *		4. store the instance to dev->driver_data pointer.
 *		
 * free_device:
 *	Release the private data.
 *	Typically, call snd_device_free(dev->card, dev->driver_data)
 */
struct snd_seq_dev_ops {
	int (*init_device)(snd_seq_device_t *dev);
	int (*free_device)(snd_seq_device_t *dev);
};

/*
 * prototypes
 */
void snd_seq_device_load_drivers(void);
int snd_seq_device_new(snd_card_t *card, int device, char *id, int argsize, snd_seq_device_t **result);
snd_seq_device_t *snd_seq_device_find(snd_card_t *card, int device, char *id);
int snd_seq_device_register_driver(char *id, snd_seq_dev_ops_t *entry, int argsize);
int snd_seq_device_unregister_driver(char *id);

#define SND_SEQ_DEVICE_ARGPTR(dev) (void *)((char *)(dev) + sizeof(snd_seq_device_t))


/*
 * id strings for generic devices
 */
#define SND_SEQ_DEV_ID_MIDISYNTH	"seq-midi"
#define SND_SEQ_DEV_ID_OPL3		"synth-opl3"


#endif
