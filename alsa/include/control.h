#ifndef __CONTROL_H
#define __CONTROL_H

/*
 *  Header file for control interface
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

typedef struct snd_stru_ctl_read {
	snd_ctl_read_t data;
	struct snd_stru_ctl_read *next;
} snd_kctl_read_t;

struct snd_stru_control {
	snd_card_t *card;
	int hwdep_device;
	int mixer_device;
	int pcm_device;
	int pcm_channel;
	int pcm_subdevice;
	int rawmidi_device;
	wait_queue_head_t change_sleep;
	spinlock_t read_lock;
	int read_active: 1,		/* read interface is activated */
	    rebuild: 1;			/* rebuild the structure */
	snd_kctl_read_t *first_item;
	snd_kctl_read_t *last_item;
	struct snd_stru_control *next;
};

typedef int (*snd_control_ioctl_t) (snd_card_t * card,
				    snd_control_t * control,
				    unsigned int cmd, unsigned long arg);

extern int snd_control_busy(snd_control_t * control);
extern void snd_control_notify_structure_change(snd_control_t * control, snd_ctl_read_t * read);
extern void snd_control_notify_value_change(snd_control_t * control, snd_ctl_read_t * read, int atomic);
extern void snd_control_notify_switch_change(snd_card_t * card, int cmd, int iface, char *name);
extern void snd_control_notify_switch_value_change(snd_control_t * control, int iface, char *name, int atomic);

extern int snd_control_register(snd_card_t *card);
extern int snd_control_unregister(snd_card_t *card);
extern int snd_control_register_ioctl(snd_control_ioctl_t fcn);
extern int snd_control_unregister_ioctl(snd_control_ioctl_t fcn);
extern int snd_control_switch_add(snd_card_t * card, snd_kswitch_t * ksw);
extern int snd_control_switch_remove(snd_card_t * card, snd_kswitch_t * ksw);
extern snd_kswitch_t *snd_control_switch_new(snd_card_t * card, snd_kswitch_t * ksw, void *private_data);
extern int snd_control_switch_change(snd_card_t * card, snd_kswitch_t * ksw);

#endif				/* __CONTROL_H */
