#ifndef __ES1869_H
#define __ES1869_H

/*
 *  Header file for ES1869
 *  Copyright (c) by Christian Fischbach
 *  <fishbach@pool.informatik.rwth-aachen.de>
 *  Copyright (c) by Abramo Bagnara
 *  <abbagnara@racine.ra.it>
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

#include "pcm1.h"
#include "mixer.h"

struct snd_stru_es1869 {
	unsigned short port;		/* port of ESS chip */
	unsigned short mpu_port;	/* MPU-401 port of ESS chip */
	unsigned short fm_port;		/* FM port */
	unsigned short ctrl_port;	/* Control port of ESS chip */
	unsigned short irq;		/* IRQ number of ESS chip */
	unsigned short irqnum;		/* IRQ number (index) */
	unsigned short dma1;		/* DMA 1 */
	unsigned short dma1num;		/* DMA 1 index */
	unsigned short dma2;		/* DMA 2 */
	unsigned short dma2num;		/* DMA 2 index */
	unsigned short version;		/* version of ESS chip */
	unsigned short audio2_vol;	/* volume level of audio2 */

	unsigned short active;
        unsigned short open;

	snd_card_t *card;
	snd_pcm_t *pcm;
	snd_pcm_t *pcm2;

	snd_spin_define(reg);
	snd_spin_define(mixer);
	snd_spin_define(ctrl);
};

typedef struct snd_stru_es1869 es1869_t;


extern void snd_es1869_mixer_write(es1869_t * codec, unsigned char reg, unsigned char data);
extern unsigned char snd_es1869_mixer_read(es1869_t * codec, unsigned char reg);

extern void snd_es1869_interrupt(es1869_t * codec);

extern es1869_t *snd_es1869_new_device(snd_card_t * card,
				       unsigned short port,
				       unsigned short mpu_port,
				       unsigned short fm_port,
				       unsigned short irqnum,
				       unsigned short dma8num,
				       unsigned short hardware);
extern int snd_es1869_init(es1869_t * codec, int enable);
extern snd_pcm_t *snd_es1869_pcm(es1869_t * codec);
extern snd_pcm_t *snd_es1869_pcm2(es1869_t * codec);
extern snd_kmixer_t *snd_es1869_mixer(es1869_t * codec);

#endif				/* __ES1869_H */

