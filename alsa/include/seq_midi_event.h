/*
 *  Defines for seq_event_midi.c
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

#ifndef __SEQ_MIDI_EVENT_H
#define __SEQ_MIDI_EVENT_H

/* Prototypes for seq_midi_event.c */
int snd_seq_event_port_attach(int client, snd_seq_port_callback_t *pcbp,
	 int cap, int type, char *portname);
int snd_seq_event_port_detach(int client, int port);

#endif /* __SEQ_MIDI_EVENT_H */
