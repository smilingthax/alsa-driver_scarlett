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

#define _snd_kcontrol_chip(kcontrol) ((kcontrol)->private_data)
#define snd_kcontrol_chip(kcontrol) snd_magic_cast1(chip_t, _snd_kcontrol_chip(kcontrol), return -ENXIO)

typedef int (snd_kcontrol_info_t) (snd_kcontrol_t * kcontrol, snd_ctl_element_info_t * uinfo);
typedef int (snd_kcontrol_get_t) (snd_kcontrol_t * kcontrol, snd_ctl_element_t * ucontrol);
typedef int (snd_kcontrol_put_t) (snd_kcontrol_t * kcontrol, snd_ctl_element_t * ucontrol);

typedef struct _snd_kcontrol_new {
	snd_ctl_element_iface_t iface;	/* interface identifier */
	unsigned int device;		/* device/client number */
	unsigned int subdevice;		/* subdevice (substream) number */
	unsigned char *name;		/* ASCII name of item */
	unsigned int index;		/* index of item */
	unsigned int access;		/* access rights */
	snd_kcontrol_info_t *info;
	snd_kcontrol_get_t *get;
	snd_kcontrol_put_t *put;
	unsigned long private_value;
} snd_kcontrol_new_t;

struct _snd_kcontrol {
	struct list_head list;		/* list of controls */
	snd_ctl_element_id_t id;
	pid_t owner;			/* locked */
	unsigned int access;		/* access rights */
	snd_kcontrol_info_t *info;
	snd_kcontrol_get_t *get;
	snd_kcontrol_put_t *put;
	unsigned long private_value;
	void *private_data;
	void (*private_free)(snd_kcontrol_t *kcontrol);
};

#define snd_kcontrol(n) list_entry(n, snd_kcontrol_t, list)

typedef struct _snd_kctl_event {
	struct list_head list;	/* list of events */
	snd_ctl_event_t data;
} snd_kctl_event_t;

#define snd_kctl_event(n) list_entry(n, snd_kctl_event_t, list)

struct _snd_kctl {
	struct list_head list;		/* list of all control files */
	snd_card_t *card;
	pid_t pid;
	int prefer_pcm_subdevice;
	int prefer_rawmidi_subdevice;
	wait_queue_head_t change_sleep;
	spinlock_t read_lock;
	int read_active: 1,		/* read interface is activated */
	    rebuild: 1;			/* rebuild the structure */
	struct list_head events;	/* waiting events for read */
};

#define snd_kctl(n) list_entry(n, snd_kctl_t, list)

typedef int (*snd_kctl_ioctl_func_t) (snd_card_t * card,
				 snd_kctl_t * control,
				 unsigned int cmd, unsigned long arg);

int snd_ctl_busy(snd_kctl_t * ctl);
void snd_ctl_notify_structure_change(snd_card_t * card, snd_ctl_event_type_t etype, snd_ctl_element_id_t * id);
void snd_ctl_notify_value_change(snd_kctl_t * ctl, snd_kcontrol_t * control);
void snd_ctl_notify_value_change_forall(snd_card_t * card, snd_kcontrol_t * control);

snd_kcontrol_t *snd_ctl_new(snd_kcontrol_t * kcontrol);
snd_kcontrol_t *snd_ctl_new1(snd_kcontrol_new_t * kcontrolnew, void * private_data);
void snd_ctl_free_one(snd_kcontrol_t * kcontrol);
int snd_ctl_add(snd_card_t * card, snd_kcontrol_t * kcontrol);
int snd_ctl_remove(snd_card_t * card, snd_kcontrol_t * kcontrol);
int snd_ctl_remove_id(snd_card_t * card, snd_ctl_element_id_t *id);
int snd_ctl_rename_id(snd_card_t * card, snd_ctl_element_id_t *src_id, snd_ctl_element_id_t *dst_id);
snd_kcontrol_t *snd_ctl_find_numid(snd_card_t * card, unsigned int numid);
snd_kcontrol_t *snd_ctl_find_id(snd_card_t * card, snd_ctl_element_id_t *id);

int snd_ctl_register(snd_card_t *card);
int snd_ctl_unregister(snd_card_t *card);
int snd_ctl_register_ioctl(snd_kctl_ioctl_func_t fcn);
int snd_ctl_unregister_ioctl(snd_kctl_ioctl_func_t fcn);

#endif				/* __CONTROL_H */
