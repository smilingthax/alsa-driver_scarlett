#ifndef __CONTROL_H
#define __CONTROL_H

/*
 *  Header file for control interface
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

struct snd_stru_control {
	snd_card_t *card;
	unsigned int mixer_device;
	unsigned int pcm_device;
	unsigned int rawmidi_device;
};

typedef int (*snd_control_ioctl_t) (snd_card_t * card,
				    snd_control_t * control,
				    unsigned int cmd, unsigned long arg);

extern int snd_control_register(snd_card_t *card);
extern int snd_control_unregister(snd_card_t *card);
extern int snd_control_register_ioctl(snd_control_ioctl_t fcn);
extern int snd_control_unregister_ioctl(snd_control_ioctl_t fcn);
extern int snd_control_switch_add(snd_card_t * card, snd_kswitch_t * ksw);
extern int snd_control_switch_remove(snd_card_t * card, snd_kswitch_t * ksw);
extern snd_kswitch_t *snd_control_switch_new(snd_card_t * card, snd_kswitch_t * ksw, void *private_data);
extern void snd_control_store(snd_card_t *card);
extern void snd_control_restore(snd_card_t *card);

#endif				/* __CONTROL_H */
