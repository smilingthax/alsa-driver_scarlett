#ifndef __ES18XX_H
#define __ES18XX_H

/*
 *  Header file for ES18xx
 *  Copyright (c) by Christian Fischbach <fishbach@pool.informatik.rwth-aachen.de>
 *  Copyright (c) by Abramo Bagnara <abramo@alsa-project.org>
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

#include "pcm.h"

struct snd_stru_es18xx {
	unsigned long port;		/* port of ESS chip */
	unsigned long mpu_port;		/* MPU-401 port of ESS chip */
	unsigned long fm_port;		/* FM port */
	unsigned long ctrl_port;	/* Control port of ESS chip */
	unsigned long irq;		/* IRQ number of ESS chip */
	snd_irq_t * irqptr;		/* IRQ pointer */
	unsigned long dma1;		/* DMA 1 */
	snd_dma_t * dma1ptr;		/* DMA 1 pointer */
	unsigned long dma2;		/* DMA 2 */
	snd_dma_t * dma2ptr;		/* DMA 2 pointer */
	unsigned short version;		/* version of ESS chip */
	int caps;			/* Chip capabilities */
	unsigned short audio2_vol;	/* volume level of audio2 */

	unsigned short active;		/* active channel mask */
	unsigned int dma1_size;
	unsigned int dma2_size;
	unsigned int dma1_shift;
	unsigned int dma2_shift;

	snd_card_t *card;
	snd_pcm_t *pcm;
	snd_pcm_substream_t *playback_a_substream;
	snd_pcm_substream_t *capture_a_substream;
	snd_pcm_substream_t *playback_b_substream;

	snd_kcontrol_t *hw_volume;
	snd_kcontrol_t *hw_switch;
	snd_kcontrol_t *master_volume;
	snd_kcontrol_t *master_switch;

	spinlock_t reg_lock;
	spinlock_t mixer_lock;
	spinlock_t ctrl_lock;
};

#define AUDIO1_IRQ	0x01
#define AUDIO2_IRQ	0x02
#define HWV_IRQ		0x04
#define MPU_IRQ		0x08

#define ES18XX_PCM2	0x0001	/* Has two useable PCM */
#define ES18XX_SPATIALIZER 0x0002	/* Has 3D Spatializer */
#define ES18XX_RECMIX	0x0004	/* Has record mixer */
#define ES18XX_DUPLEX_MONO 0x0008	/* Has mono duplex only */
#define ES18XX_DUPLEX_SAME 0x0010	/* Playback and record must share the same rate */
#define ES18XX_NEW_RATE	0x0020	/* More precise rate setting */
#define ES18XX_AUXB	0x0040	/* AuxB mixer control */
#define ES18XX_HWV	0x0080	/* Has hardware volume */
#define ES18XX_MONO	0x0100	/* Mono_in mixer control */
#define ES18XX_I2S	0x0200	/* I2S mixer control */
#define ES18XX_MUTEREC	0x0400	/* Record source can be muted */
#define ES18XX_CONTROL	0x0800	/* Has control ports */

typedef struct snd_stru_es18xx es18xx_t;

#endif				/* __ES18xx_H */
