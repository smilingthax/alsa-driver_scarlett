#ifndef __SNDPERSIST_H
#define __SNDPERSIST_H

/*
 *  Persistent data support
 *  Copyright (c) 1994-98 by Jaroslav Kysela <perex@suse.cz>
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

int snd_persist_store(char *key, const char *data, int data_len);
int snd_persist_restore(char *key, char *data, int data_len);
int snd_persist_length(char *key);
int snd_persist_present(char *key);
int snd_persist_remove(char *key);

#endif				/* SND_PERSIST_H__ */
