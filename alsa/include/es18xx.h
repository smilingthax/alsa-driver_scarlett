#ifndef __ES18xx_H
#define __ES18xx_H

/*
 *  Header file for ES18xx
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

struct snd_stru_es18xx {
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
	int caps;			/* Chip capabilities */
	unsigned short audio2_vol;	/* volume level of audio2 */

	unsigned short active;		/* active channel mask */

	snd_kmixer_element_t *mix_imux;
	snd_kmixer_element_t *mix_mic;
	snd_kmixer_element_t *mix_line;
	snd_kmixer_element_t *mix_cd;
	snd_kmixer_element_t *mix_iaccu;
	snd_kmixer_element_t *mix_oaccu;
	snd_kmixer_element_t *mix_igain_v;
	snd_kmixer_element_t *mix_opcm1_v, *mix_ipcm1_v;
	snd_kmixer_element_t *mix_opcm2_v;
	snd_kmixer_element_t *mix_omic_v, *mix_imic_v;
	snd_kmixer_element_t *mix_oline_v, *mix_iline_v;
	snd_kmixer_element_t *mix_ofm_v, *mix_ifm_v;
	snd_kmixer_element_t *mix_omono_v, *mix_imono_v;
	snd_kmixer_element_t *mix_ocd_v, *mix_icd_v;
	snd_kmixer_element_t *mix_oaux_v, *mix_iaux_v;
	snd_kmixer_element_t *mix_output_v, *mix_output_s;

	snd_card_t *card;
	snd_pcm_t *pcm;
	snd_pcm_subchn_t *playback_a_subchn;
	snd_pcm1_subchn_t *playback_a_subchn1;
	snd_pcm_subchn_t *capture_a_subchn;
	snd_pcm1_subchn_t *capture_a_subchn1;
	snd_pcm_subchn_t *playback_b_subchn;
	snd_pcm1_subchn_t *playback_b_subchn1;

	spinlock_t reg_lock;
	spinlock_t mixer_lock;
	spinlock_t ctrl_lock;
};

#define AUDIO1_IRQ	0x01
#define AUDIO2_IRQ	0x02
#define HWV_IRQ		0x04
#define MPU_IRQ		0x08

#define ES18XX_PCM2	0x0001	/* Has two useable PCM */
#define ES18XX_3D	0x0002	/* Has 3D Spatializer */
#define ES18XX_RECMIX	0x0004	/* Has record mixer */
#define ES18XX_DUPLEX_MONO 0x0008	/* Has mono duplex only */
#define ES18XX_DUPLEX_SAME 0x0010	/* Playback and record must share the same rate */
#define ES18XX_NEW_RATE	0x0020	/* More precise rate setting */
#define ES18XX_AUXB	0x0040	/* AuxB mixer control */
#define ES18XX_SPEAKER	0x0080	/* Speaker mixer control */
#define ES18XX_MONO	0x0100	/* Mono_in mixer control */
#define ES18XX_I2S	0x0200	/* I2S mixer control */
#define ES18XX_MUTEREC	0x0400	/* Record source can be muted */
#define ES18XX_CONTROL	0x0800	/* Has control ports */
#define ES18XX_HWV	0x1000	/* Has hardware volume */

typedef struct snd_stru_es18xx es18xx_t;


extern void snd_es18xx_mixer_write(es18xx_t * codec, unsigned char reg, unsigned char data);
extern unsigned char snd_es18xx_mixer_read(es18xx_t * codec, unsigned char reg);

extern void snd_es18xx_interrupt(es18xx_t * codec, unsigned char status);

extern es18xx_t *snd_es18xx_new_device(snd_card_t * card,
				       unsigned short port,
				       unsigned short mpu_port,
				       unsigned short fm_port,
				       snd_irq_t * irqnum,
				       snd_dma_t * dma1num,
				       snd_dma_t * dma2num);
extern int snd_es18xx_init(es18xx_t * codec, int enable);
extern snd_pcm_t *snd_es18xx_pcm(es18xx_t * codec);
extern snd_kmixer_t *snd_es18xx_mixer(es18xx_t * codec);

#endif				/* __ES18xx_H */

