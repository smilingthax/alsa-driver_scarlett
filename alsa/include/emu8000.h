#ifndef __EMU8000_H
#define __EMU8000_H

/*
 * Definitions for the EMU-8000.
 *
 * Copyright (c) by Jaroslav Kysela <perex@jcu.cz>.
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

#include "synth.h"

typedef struct snd_emu8000 emu8000_t;

struct snd_emu8000 {
  unsigned short port;
  snd_spin_define( reg );
};

extern snd_synth_t *snd_emu8000_new_device( snd_card_t *card, unsigned short port );

#endif /* __EMU8000_H */
