#ifndef __ES1868_H
#define __ES1868_H

/*
 *  Header file for ES1868
 *  Copyright (c) by Christian Fischbach
 *  <fishbach@pool.informatik.rwth-aachen.de>
 *  Copyright (c) by Abramo Bagnara
 *  <abbagnara@racine.ra.it>
 *  Copyright (c) by Unai Uribarri
 *  <unai@dobra.aic.uniovi.es>
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

struct snd_stru_es1868 {
	unsigned short port;		/* port of ESS chip */
	unsigned short mpu_port;	/* MPU-401 port of ESS chip */
	unsigned short fm_port;		/* FM port */
	unsigned short ctrl_port;	/* Control port of ESS chip */
	unsigned short irq;		/* IRQ number of ESS chip */
	snd_irq_t * irqptr;		/* IRQ pointer */
	unsigned short dma1;		/* DMA 1 */
	snd_dma_t * dma1ptr;		/* DMA 1 pointer */
	unsigned short dma2;		/* DMA 2 */
	snd_dma_t * dma2ptr;		/* DMA 2 pointer */
	unsigned short version;		/* version of ESS chip */

	unsigned short open;		/* open channel mask */
	unsigned short active;		/* active channel mask */
	
	snd_card_t *card;
	snd_pcm_t *pcm;

	snd_spin_define(reg);
	snd_spin_define(mixer);
	snd_spin_define(ctrl);
};

typedef struct snd_stru_es1868 es1868_t;


extern void snd_es1868_mixer_write(es1868_t * codec, unsigned char reg, unsigned char data);
extern unsigned char snd_es1868_mixer_read(es1868_t * codec, unsigned char reg);

extern void snd_es1868_interrupt(es1868_t * codec, unsigned char status);
extern es1868_t *snd_es1868_new_device(snd_card_t * card,
				       unsigned short port,
				       unsigned short mpu_port,
				       unsigned short fm_port,
				       snd_irq_t * irqnum,
				       snd_dma_t * dma1num,
				       snd_dma_t * dma2num);
extern int snd_es1868_init(es1868_t * codec, int enable);
extern snd_pcm_t *snd_es1868_pcm(es1868_t * codec);
extern snd_kmixer_t *snd_es1868_mixer(es1868_t * codec);

#endif				/* __ES1868_H */

