#ifndef __ESSSOLO1_H
#define __ESSSOLO1_H

/*
 *  Copyright (c) by Jaromir Koutek <miri@punknet.cz>,
 *                   Jaroslav Kysela <perex@suse.cz>
 *  Definitions for ESS Solo-1
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

#include "sndpci.h"
#include "pcm1.h"
#include "mixer.h"
#include "midi.h"
#include "mpu401.h"

#define SLIO_REG( solo, x ) ( (solo) -> io_port + SLIO_REG_##x )

#define SLIO_REG_AUDIO2DMAADDR 0
#define SLIO_REG_AUDIO2DMACOUNT 4
#define SLIO_REG_AUDIO2MODE 6
#define SLIO_REG_IRQCONTROL 7

#define SLDM_REG( solo, x ) ( (solo) -> ddma_port + SLDM_REG_##x )

#define SLDM_REG_DMABASE 0
#define SLDM_REG_DMACOUNT 4
#define SLDM_REG_DMACOMMAND 8
#define SLDM_REG_DMASTATUS 8
#define SLDM_REG_DMAMODE 0x0b
#define SLDM_REG_DMACLEAR 0x0d
#define SLDM_REG_DMAMASK 0x0f

#define SLSB_REG( solo, x ) ( (solo) -> sb_port + SLSB_REG_##x )

#define SLSB_REG_MIXERADDR 4
#define SLSB_REG_MIXERDATA 5

#define SLSB_IREG_AUDIO1 0x14
#define SLSB_IREG_MICMIX 0x1a
#define SLSB_IREG_RECSRC 0x1c
#define SLSB_IREG_MASTER 0x32
#define SLSB_IREG_FM 0x36
#define SLSB_IREG_AUXACD 0x38
#define SLSB_IREG_AUXB 0x3a
#define SLSB_IREG_PCSPEAKER 0x3c
#define SLSB_IREG_LINE 0x3e
#define SLSB_IREG_MASTER_LEFT 0x60
#define SLSB_IREG_MASTER_RIGHT 0x62
#define SLSB_IREG_SPATCONTROL 0x50
#define SLSB_IREG_SPATLEVEL 0x52
#define SLSB_IREG_AUDIO2SAMPLE 0x70
#define SLSB_IREG_AUDIO2MODE 0x71
#define SLSB_IREG_AUDIO2FILTER 0x72
#define SLSB_IREG_AUDIO2TCOUNTL 0x74
#define SLSB_IREG_AUDIO2TCOUNTH 0x76
#define SLSB_IREG_AUDIO2CONTROL1 0x78
#define SLSB_IREG_AUDIO2CONTROL2 0x7a
#define SLSB_IREG_AUDIO2 0x7c

#define SLSB_REG_RESET 6

#define SLSB_REG_READDATA 0x0a
#define SLSB_REG_WRITEDATA 0x0c
#define SLSB_REG_READSTATUS 0x0c

#define SLSB_REG_STATUS 0x0e

#define SL_PCI_CONFIG 0x50
#define SL_PCI_DDMACONTROL 0x60

#define SL_CMD_EXTSAMPLERATE 0xa1
#define SL_CMD_FILTERDIV 0xa2
#define SL_CMD_DMACNTRELOADL 0xa4
#define SL_CMD_DMACNTRELOADH 0xa5
#define SL_CMD_ANALOGCONTROL 0xa8
#define SL_CMD_IRQCONTROL 0xb1
#define SL_CMD_DRQCONTROL 0xb2
#define SL_CMD_SETFORMAT 0xb6
#define SL_CMD_SETFORMAT2 0xb7
#define SL_CMD_DMACONTROL 0xb8
#define SL_CMD_DMATYPE 0xb9
#define SL_CMD_READREG 0xc0
#define SL_CMD_ENABLEEXT 0xc6
#define SL_CMD_PAUSEDMA 0xd0
#define SL_CMD_ENABLEAUDIO1 0xd1
#define SL_CMD_CONTDMA 0xd4

#define SL_RECSRC_MIC 0
#define SL_RECSRC_AUXACD 2
#define SL_RECSRC_AUXB 5
#define SL_RECSRC_LINE 6
#define SL_RECSRC_NONE 7

/*

 */

typedef struct snd_stru_solo esssolo_t;

struct snd_stru_solo {
	snd_dma_t * dma1ptr;
	snd_dma_t * dma2ptr;
	snd_irq_t * irqptr;

	unsigned int io_port;
	unsigned int sb_port;
	unsigned int vc_port;
	unsigned int mpu_port;
	unsigned int game_port;
	unsigned int ddma_port;


	unsigned char enable;
	unsigned char irqmask;
	unsigned char revision;
	unsigned char format;
	unsigned char srs_space;
	unsigned char srs_center;
	unsigned char mpu_switch;
	unsigned char wave_source;

	unsigned int mode;

	struct snd_pci_dev *pci;
	snd_card_t *card;
	snd_pcm_t *pcm;
	snd_kmixer_t *mixer;
	snd_rawmidi_t *rmidi;
	snd_synth_t *synth;	/* S3FM */

	 snd_spin_define(reg);
	snd_info_entry_t *proc_entry;
};

esssolo_t *snd_solo_create(snd_card_t * card, struct snd_pci_dev *pci,
			   snd_dma_t * dma1ptr,
			   snd_dma_t * dma2ptr,
			   snd_irq_t * irqptr,
			   int reverb, int mge);
void snd_solo_free(esssolo_t * solo);
void snd_solo_interrupt(esssolo_t * solo);

snd_pcm_t *snd_solo_pcm(esssolo_t * solo);
snd_kmixer_t *snd_solo_mixer(esssolo_t * solo);
void snd_solo_midi(esssolo_t * solo, mpu401_t * mpu);

#endif				/* __ESSSOLO1_H */
