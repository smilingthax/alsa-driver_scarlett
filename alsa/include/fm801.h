#ifndef __FM801_H
#define __FM801_H

/*
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>
 *  Definitions for ForteMedia FM801 chip
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
#include "midi.h"
#include "ac97_codec.h"

#ifndef PCI_VENDOR_ID_FORTEMEDIA
#define PCI_VENDOR_ID_FORTEMEDIA	0x1319
#endif
#ifndef PCI_DEVICE_ID_FORTEMEDIA_FM801
#define PCI_DEVICE_ID_FORTEMEDIA_FM801	0x0801
#endif

/*
 *  Direct registers
 */

#define FM801_REG(codec, reg)	(codec->port + FM801_##reg)

#define FM801_PCM_VOL		0x00	/* PCM Output Volume */
#define FM801_FM_VOL		0x02	/* FM Output Volume */
#define FM801_I2S_VOL		0x04	/* I2S Volume */
#define FM801_REC_SRC		0x06	/* Resourd Source */
#define FM801_PLY_CTRL		0x08	/* Playback Control */
#define FM801_PLY_COUNT		0x0a	/* Playback Count */
#define FM801_PLY_BUF1		0x0c	/* Playback Bufer I */
#define FM801_PLY_BUF2		0x10	/* Playback Buffer II */
#define FM801_CAP_CTRL		0x14	/* Capture Control */
#define FM801_CAP_COUNT		0x16	/* Capture Count */
#define FM801_CAP_BUF1		0x18	/* Capture Buffer I */
#define FM801_CAP_BUF2		0x1c	/* Capture Buffer II */
#define FM801_CODEC_CTRL	0x22	/* Codec Control */
#define FM801_I2S_MODE		0x24	/* I2S Mode Control */
#define FM801_VOLUME		0x26	/* Volume Up/Down/Mute Status */
#define FM801_I2C_CTRL		0x29	/* I2C Control */
#define FM801_AC97_CMD		0x2a	/* AC'97 Command */
#define FM801_AC97_DATA		0x2c	/* AC'97 Data */
#define FM801_MPU401_DATA	0x30	/* MPU401 Data */
#define FM801_MPU401_CMD	0x31	/* MPU401 Command */
#define FM801_GPIO_CTRL		0x52	/* General Purpose I/O Control */
#define FM801_GEN_CTRL		0x54	/* General Control */
#define FM801_IRQ_MASK		0x56	/* Interrupt Mask */
#define FM801_IRQ_STATUS	0x5a	/* Interrupt Status */
#define FM801_OPL3_BANK0	0x68	/* OPL3 Status Read / Bank 0 Write */
#define FM801_OPL3_DATA0	0x69	/* OPL3 Data 0 Write */
#define FM801_OPL3_BANK1	0x6a	/* OPL3 Bank 1 Write */
#define FM801_OPL3_DATA1	0x6b	/* OPL3 Bank 1 Write */
#define FM801_POWERDOWN		0x70	/* Blocks Power Down Control */

#define FM801_AC97_ADDR		(0<<10)

/* playback and record control register bits */
#define FM801_BUF1_LAST		(1<<1)
#define FM801_BUF2_LAST		(1<<2)
#define FM801_START		(1<<5)
#define FM801_PAUSE		(1<<6)
#define FM801_IMMED_STOP	(1<<7)
#define FM801_RATE_SHIFT	8
#define FM801_RATE_MASK		(15 << FM801_RATE_SHIFT)
#define FM801_16BIT		(1<<14)
#define FM801_STEREO		(1<<15)

/* IRQ status bits */
#define FM801_IRQ_PLAYBACK	(1<<8)
#define FM801_IRQ_CAPTURE	(1<<9)
#define FM801_IRQ_VOLUME	(1<<14)
#define FM801_IRQ_MPU		(1<<15)
	
/*

 */

typedef struct snd_stru_fm801 fm801_t;

struct snd_stru_fm801 {
	snd_dma_t * dma1ptr;	/* DAC1 */
	snd_dma_t * dma2ptr;	/* ADC */
	snd_irq_t * irqptr;

	unsigned short port;	/* I/O port number */

	unsigned short ply_ctrl; /* playback control */
	unsigned short cap_ctrl; /* capture control */

	unsigned long ply_buffer;
	unsigned int ply_buf;
	unsigned int ply_count;
	unsigned int ply_size;
	unsigned int ply_pos;

	unsigned long cap_buffer;
	unsigned int cap_buf;
	unsigned int cap_count;
	unsigned int cap_size;
	unsigned int cap_pos;

	ac97_t *ac97;

	struct pci_dev *pci;
	snd_card_t *card;
	snd_pcm_t *pcm;
	snd_kmixer_t *mixer;
	snd_rawmidi_t *rmidi;

	spinlock_t reg_lock;
	snd_info_entry_t *proc_entry;
};

fm801_t *snd_fm801_create(snd_card_t * card, struct pci_dev *pci,
			      	   snd_dma_t * dma1ptr,
				   snd_dma_t * dma2ptr,
			           snd_irq_t * irqptr);
void snd_fm801_free(fm801_t * codec);
void snd_fm801_interrupt(fm801_t * codec, unsigned short status);

snd_pcm_t *snd_fm801_pcm(fm801_t * codec);
snd_kmixer_t *snd_fm801_mixer(fm801_t * codec, int pcm_dev);
snd_rawmidi_t *snd_fm801_midi(fm801_t * codec);

#endif				/* __AUDIOPCI_H */
