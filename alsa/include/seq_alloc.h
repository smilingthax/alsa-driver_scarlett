#ifndef __SEQ_ALLOC_H
#define __SEQ_ALLOC_H
/*
 *  Allocation routines header.
 *
 *  Copyright (C) 1999 Steve Ratcliffe
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

#include "seq_kernel.h"

/* Prototypes for seq_alloc.c */
void snd_init_port_callback(snd_seq_port_callback_t *p);
snd_seq_port_callback_t *snd_alloc_port_callback(void);
void snd_init_midi_channel(snd_midi_channel_t *p, int n);
struct snd_midi_channel *snd_alloc_midi_channels(int n);
struct snd_midi_channel_set *snd_alloc_midi_channel_set(int n);

#endif
