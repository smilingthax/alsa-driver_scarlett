#ifndef __SWITCH_H
#define __SWITCH_H

/*
 *  Abstraction layer for MIXER
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

typedef int (snd_get_switch_t) (void * desc, snd_kswitch_t * kswitch, snd_switch_t * uswitch);
typedef int (snd_set_switch_t) (void * desc, snd_kswitch_t * kswitch, snd_switch_t * uswitch);

struct snd_stru_switch {
	char name[32];
	snd_get_switch_t *get;
	snd_set_switch_t *set;
	unsigned int private_value;
	void *private_data;
	void (*private_free)(void *private_data);
};

struct snd_stru_switch_list {
	int count;
	snd_kswitch_t **switches;
	struct semaphore lock;
	void *desc;
	int iface;
	int device;
	int channel;
	struct snd_stru_switch_list *prev;
	struct snd_stru_switch_list *next;
};

void snd_switch_lock(snd_card_t *card, int up);
int snd_switch_prepare(snd_card_t *card, snd_kswitch_list_t * list, void *desc, int iface, int device, int channel);
snd_kswitch_t *snd_switch_new(snd_kswitch_t * kswitch);
void snd_switch_free_one(snd_kswitch_t * kswitch);
int snd_switch_add(snd_kswitch_list_t * list, snd_kswitch_t * kswitch);
int snd_switch_remove(snd_kswitch_list_t * list, snd_kswitch_t * kswitch);
int snd_switch_free(snd_card_t *card, snd_kswitch_list_t * list);
int snd_switch_list(snd_card_t *card, snd_switch_list_t *_list);
int snd_switch_read(snd_card_t *card, snd_switch_t *_switch);
int snd_switch_write(snd_control_t *control, snd_switch_t *_switch);
int snd_switch_count(snd_kswitch_list_t * list);

#endif				/* __SWITCH_H */
