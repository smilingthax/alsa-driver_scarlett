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

#define SND_SERIAL_MAX_PORTS    4

//#define TX_BUFF_SIZE 32		/* Must be 2^n */
#define TX_BUFF_SIZE 256		/* Must be 2^n */
//#define TX_BUFF_SIZE 512		/* Must be 2^n */
#define TX_BUFF_MASK  (TX_BUFF_SIZE - 1)

typedef struct snd_stru_uart16550 {
	snd_card_t *card;
	snd_rawmidi_t *rmidi[SND_SERIAL_MAX_PORTS];

	int filemode;		//open status of file

	spinlock_t open_lock;

	int irq;

	unsigned short base;
	struct resource *res_base;

	unsigned char divisor;

	unsigned char old_divisor_lsb;
	unsigned char old_divisor_msb;
	unsigned char old_line_ctrl_reg;

	//parameter for using of write loop
	short int fifo_limit;	//used in uart16550

        short int fifo_count;	//used in uart16550

	// ports
	int midi_ports;
	int ports_count;
	int prev_ports;

	//write buffer and its writing/reading position
	char tx_buff[TX_BUFF_SIZE];
        int buff_in;
        int buff_out;
} snd_uart16550_t;

int snd_uart16550_detect(unsigned int io_base);
int snd_uart16550_create(snd_card_t * card,
			 unsigned int iobase,
			 int irq,
			 unsigned char divisor,
			 snd_uart16550_t **ruart);
int snd_uart16550_rmidi(snd_uart16550_t *uart, int device, int local_device, snd_rawmidi_t **rmidi);
