#ifndef __TIMER_H
#define __TIMER_H

/*
 *  Timer abstract layer
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

#define SND_TIMER_DEVICES	16

#define SND_TIMER_DEV_FLG_PCM	0x10000000

#define SND_TIMER_HW_AUTO	0x00000001	/* auto trigger is supported */
#define SND_TIMER_HW_STOP	0x00000002	/* call stop before start */
#define SND_TIMER_HW_SLAVE	0x00000004	/* only slave timer (variable resolution) */
#define SND_TIMER_HW_FIRST	0x00000008	/* first tick can be incomplete */

#define SND_TIMER_IFLG_SLAVE	0x00000001
#define SND_TIMER_IFLG_RUNNING	0x00000002
#define SND_TIMER_IFLG_START	0x00000004
#define SND_TIMER_IFLG_AUTO	0x00000008	/* auto restart */

#define SND_TIMER_FLG_SYSTEM	0x00000001	/* system timer */
#define SND_TIMER_FLG_CHANGE	0x00000002
#define SND_TIMER_FLG_RESCHED	0x00000004	/* need reschedule */

typedef void (*snd_timer_callback_t) (snd_timer_instance_t * timeri, unsigned long ticks, unsigned long resolution, void *data);

struct snd_stru_timer_hardware {
	/* -- must be filled with low-level driver */
	unsigned int flags;		/* various flags */
	unsigned long resolution;	/* average timer resolution for one tick in nsec */
	unsigned long ticks;		/* max timer ticks per interrupt */
	/* -- low-level functions -- */
	int (*open) (snd_timer_t * timer);
	int (*close) (snd_timer_t * timer);
	unsigned long (*c_resolution) (snd_timer_t * timer);
	void (*start) (snd_timer_t * timer);
	void (*stop) (snd_timer_t * timer);
};

struct snd_stru_timer {
	snd_card_t *card;
	int number;			/* timer number */
	char id[64];
	char name[80];
	unsigned int flags;
	int running;			/* running instances */
	unsigned long sticks;		/* schedule ticks */
	void *private_data;
	void (*private_free) (void *private_data);
	struct snd_stru_timer_hardware hw;
	spinlock_t lock;
	atomic_t in_use;		/* don't free */
	snd_timer_instance_t *first;
	snd_timer_t *next;
};

struct snd_stru_timer_instance {
	snd_timer_t * timer;
	char *owner;
	unsigned int flags;
	void *private_data;
	void (*private_free) (void *private_data);
	snd_timer_callback_t callback;
	void *callback_data;
	unsigned long ticks;
	unsigned long cticks;
	unsigned long lost;		/* lost ticks */
	unsigned int slave_type;
	unsigned int slave_id;
	snd_timer_instance_t *next;
	snd_timer_instance_t *inext;
	snd_timer_instance_t *iprev;
	snd_timer_instance_t *slave;	/* slave list */
	snd_timer_instance_t *master;	/* master link */
};

/*
 *  Registering
 */

extern int snd_timer_new(snd_card_t * card, char *id, int device, snd_timer_t ** rtimer);

extern snd_timer_instance_t *snd_timer_open(char *owner, int timer_no, unsigned int slave_type, unsigned int slave_id);
extern snd_timer_instance_t *snd_timer_open1(char *owner, snd_timer_t *timer, unsigned int slave_type, unsigned int slave_id);
extern snd_timer_instance_t *snd_timer_open_slave(char *owner, unsigned int slave_type, unsigned int slave_id);
extern int snd_timer_close(snd_timer_instance_t * timeri);
extern int snd_timer_set_owner(snd_timer_instance_t * timeri, pid_t pid, gid_t gid);
extern int snd_timer_reset_owner(snd_timer_instance_t * timeri);
extern int snd_timer_set_resolution(snd_timer_instance_t * timeri, unsigned long resolution);
extern unsigned long snd_timer_resolution(snd_timer_instance_t * timeri);
extern int snd_timer_start(snd_timer_instance_t * timeri, unsigned int ticks);
extern int snd_timer_stop(snd_timer_instance_t * timeri);
extern int snd_timer_continue(snd_timer_instance_t * timeri);

extern void snd_timer_interrupt(snd_timer_t * timer, unsigned long ticks_left);

extern unsigned int snd_timer_system_resolution(void);

#endif				/* __TIMER_H */
