#ifndef __STHREAD_H
#define __STHREAD_H

/*
 *  Sound Thread
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

typedef struct snd_stru_thread {
	void (*callback)(void *data);
	void *data;
	struct snd_stru_thread *next;
} snd_thread_t;

snd_thread_t *snd_thread_create(void (*callback)(void *data), void *data);
int snd_thread_remove(snd_thread_t *t);

#endif				/* __STHREAD_H */
