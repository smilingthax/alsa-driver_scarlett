/*
 *   Kernel client MIDI driver for ALSA sequencer
 *   Create MIDI port for GUS external MIDI using midisynth driver
 *
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

#define SND_MAIN_OBJECT_FILE
#include "driver.h"
#include "midi.h"
#include "seq.h"
#include "seq_midisynth.h"


int init_module(void)
{
	/* register card 0, midi port 0 */
	snd_seq_midisynth_register_port(0, 0, NULL);
        
	return 0;
}


void cleanup_module(void)
{
	snd_seq_midisynth_unregister_port(0, 0);

}
