/*
 *   uart16550.h
 *   Copyright (c)  by Isaku Yamahata <yamahata@private.email.ne.jp>
 *   Fri Jan 8 1999 first version
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


#include "driver.h"

extern int snd_uart16550_new_device (snd_card_t* card,
				     int device,
				     unsigned short irq_number,
				     unsigned short iobase,
				     unsigned char divisor,
				     int polled,
				     snd_rawmidi_t ** rrawmidi);

extern int snd_uart16550_set_param (snd_rawmidi_t* rmidi,
				    unsigned short irq_number,
				    unsigned short iobase,
				    unsigned char divisor,
				    int polled);

extern int snd_uart16550_detect (unsigned short io_base);

#ifdef CONFIG_SND_SEQUENCER
extern int snd_seq_uart16550_register_port(snd_card_t * card, snd_rawmidi_t * rmidi, int device);
extern int snd_seq_uart16550_unregister_port(snd_card_t * card, snd_rawmidi_t * rmidi);
#endif
