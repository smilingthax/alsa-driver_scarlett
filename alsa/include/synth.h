#ifndef __SYNTH_H
#define __SYNTH_H

/*
 *  Synthesizer abstract layer 
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

#define SND_SYNTH_DEVICES	16

struct snd_stru_synth_hardware {
	void (*event) (snd_synth_t * synth, unsigned char *buffer);	/* process sequencer event */
	void (*ioctl) (snd_synth_t * synth, unsigned int cmd, unsigned long arg);
};

struct snd_stru_synth {
	snd_card_t *card;
	char id[32];
	char name[80];
	struct snd_stru_synth_hardware hw;
	void *private_data;
	void (*private_free) (void *private_data);
};

extern snd_synth_t *snd_synth_new_device(snd_card_t * card, char *id);
extern int snd_synth_free(snd_synth_t * synth);
extern int snd_synth_register(snd_synth_t * synth);
extern int snd_synth_unregister(snd_synth_t * synth);

#endif				/* __SYNTH_H */
