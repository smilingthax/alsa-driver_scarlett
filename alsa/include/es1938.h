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

#include "pcm1.h"
#include "mixer.h"
#include "midi.h"
#include "mpu401.h"
#include "ess_common.h"

#define SLIO_REG( solo, x ) ( (solo) -> io_port + ESSIO_REG_##x )

#define SLDM_REG( solo, x ) ( (solo) -> ddma_port + ESSDM_REG_##x )

#define SLSB_REG( solo, x ) ( (solo) -> sb_port + ESSSB_REG_##x )

#define SL_PCI_COMMAND 0x4
#define SL_PCI_LEGACYCONTROL 0x40
#define SL_PCI_CONFIG 0x50
#define SL_PCI_DDMACONTROL 0x60
#define SL_PCI_DDMA2CONTROL 0xF0

#define SL_MODE_PLAY 1
#define SL_MODE_CAPTURE 2
#define SL_CHANNEL1_ON 1
#define SL_CHANNEL2_ON 1
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

	unsigned char mode;
	unsigned char channel1;
	unsigned char channel2;

	struct pci_dev *pci;
	snd_card_t *card;
	snd_pcm_t *pcm;
	snd_pcm_t *pcm2;  
	snd_pcm_subchn_t *playback_subchn;
	snd_pcm1_subchn_t *playback_subchn1;
	snd_pcm_subchn_t *capture_subchn;
	snd_pcm1_subchn_t *capture_subchn1;
	snd_pcm_subchn_t *playback2_subchn;
	snd_pcm1_subchn_t *playback2_subchn1;
	snd_kmixer_t *mixer;
	snd_rawmidi_t *rmidi;
	snd_hwdep_t *fmsynth;	/* FM */

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
  
	spinlock_t reg_lock;
	spinlock_t mixer_lock;
        snd_info_entry_t *proc_entry;
};

esssolo_t *snd_solo_create(snd_card_t * card, struct pci_dev *pci,
			   snd_dma_t * dma1ptr,
			   snd_dma_t * dma2ptr,
			   snd_irq_t * irqptr,
			   int reverb, int mge);
void snd_solo_free(esssolo_t * solo);
void snd_solo_interrupt(esssolo_t * solo);

snd_pcm_t *snd_solo_pcm(esssolo_t * solo);
snd_pcm_t *snd_solo_pcm2(esssolo_t * solo);
snd_kmixer_t *snd_solo_mixer(esssolo_t * codec, int pcm1_num, int pcm2_num);
void snd_solo_midi(esssolo_t * solo, mpu401_t * mpu);

#endif				/* __ESSSOLO1_H */
