#ifndef __MPU401_H
#define __MPU401_H

/*
 *  Header file for MPU-401 and compatible cards
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

#include "midi.h"

#define MPU401_HW_MPU401		1	/* native MPU401 */
#define MPU401_HW_SB			2	/* SoundBlaster MPU-401 UART */
#define MPU401_HW_ES1688		3	/* AudioDrive ES1688 MPU-401 UART */
#define MPU401_HW_OPL3SA		4	/* Yamaha OPL3-SA */
#define MPU401_HW_SONICVIBES		5	/* S3 SonicVibes */
#define MPU401_HW_CS4232		6	/* CS4232 */

#define MPU401_MODE_INPUT		1
#define MPU401_MODE_OUTPUT		2
#define MPU401_MODE_INPUT_TRIGGER	4

typedef struct snd_stru_mpu401 mpu401_t;

struct snd_stru_mpu401 {
  unsigned short hardware;	/* MPU401_HW_XXXX */
  unsigned short port;		/* base port of MPU-401 chip */
  unsigned short irq;		/* IRQ number of MPU-401 chip */

  unsigned int mode;		/* MPU401_MODE_XXXX */

  void (*open_input)( mpu401_t *mpu );
  void (*close_input)( mpu401_t *mpu );
  void (*open_output)( mpu401_t *mpu );
  void (*close_output)( mpu401_t *mpu );
  void *private_data;

  snd_spin_define( open );
  snd_spin_define( input );
  snd_spin_define( output );
};

/* I/O ports */

#define MPU401C( mpu ) ( (mpu) -> port + 1 )
#define MPU401D( mpu ) ( (mpu) -> port + 0 )

/*
 *
 */

extern void snd_mpu401_uart_interrupt( snd_rawmidi_t *rmidi );

extern snd_rawmidi_t *snd_mpu401_uart_new_device( snd_card_t *card,
						  unsigned short hardware,
                                                  unsigned short port,
                                                  unsigned short irqnum );

#endif /* __MPU401_H */
