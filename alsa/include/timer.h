#ifndef __TIMER_H
#define __TIMER_H

/*
 *  Timer abstract layer
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

#define SND_TIMER_DEVICES	16

#define SND_TIMER_FLG_USED	0x00000001
#define SND_TIMER_FLG_SYSTEM	0x00000002	/* system timer */
#define SND_TIMER_FLG_RUNNING	0x00000004
#define SND_TIMER_FLG_AUTO	0x00000008	/* auto trigger */

typedef void (*snd_timer_callback_t) (snd_timer_t * timer, void *data);

struct snd_stru_timer_hardware {
	/* -- must be filled with low-level driver */
	unsigned int resolution;	/* average timer resolution for one tick in nsec */
	unsigned int low_ticks;		/* low timer ticks (usually 1) */
	unsigned int high_ticks;	/* high timer ticks */
	/* -- low-level functions -- */
	int (*open) (snd_timer_t * timer);
	void (*close) (snd_timer_t * timer);
	unsigned int (*c_resolution) (snd_timer_t * timer);
	void (*start) (snd_timer_t * timer);
	void (*stop) (snd_timer_t * timer);
	void (*t_continue) (snd_timer_t * timer);
};

struct snd_stru_timer {
	snd_card_t *card;
	char id[32];
	char name[80];
	unsigned int flags;
	char *owner;
	struct snd_stru_timer_hardware hw;
	void *private_data;
	void (*private_free) (void *private_data);
	snd_timer_callback_t callback;
	void *callback_data;
	unsigned int ticks;
	unsigned int cticks;
};

/*
 *  Registering
 */

extern snd_timer_t *snd_timer_new_device(snd_card_t * card, char *id);
extern int snd_timer_free(snd_timer_t * timer);
extern int snd_timer_register(snd_timer_t * timer);
extern int snd_timer_unregister(snd_timer_t * timer);
extern int snd_timer_change(int *last);

extern snd_timer_t *snd_timer_open(char *owner, unsigned int resolution);
extern int snd_timer_close(snd_timer_t * timer);
extern unsigned int snd_timer_resolution(snd_timer_t * timer);
extern void snd_timer_start(snd_timer_t * timer, unsigned int ticks);
extern void snd_timer_stop(snd_timer_t * timer);
extern void snd_timer_continue(snd_timer_t * timer);

extern snd_timer_t *snd_timer_open_always(char *owner, unsigned int resolution);
extern int snd_timer_close_always(snd_timer_t * timer);

extern unsigned int snd_timer_system_resolution(void);
extern snd_timer_t *snd_timer_open_system(char *owner);
extern int snd_timer_close_system(snd_timer_t * timer);

#endif				/* __TIMER_H */
