/*
 *   Generic MIDI synth driver for ALSA sequencer
 *   Copyright (c) 1998 by Frank van de Pol <F.K.W.van.de.Pol@inter.nl.net>
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
#ifndef __SND_SEQ_MIDISYNTH_H
#define __SND_SEQ_MIDISYNTH_H

#ifdef SNDCFG_SEQUENCER
/* register/unregister midisynth port */
extern int snd_seq_midisynth_register_port(snd_card_t *card, int device, char *portname);
extern int snd_seq_midisynth_unregister_port(snd_card_t *card, int device);
#endif
                        
#endif /* __SND_SEQ_MIDISYNTH_H */
