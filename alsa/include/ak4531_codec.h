#ifndef __AK4531_CODEC_H
#define __AK4531_CODEC_H

/*
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>
 *  Universal interface for Audio Codec '97
 *
 *  For more details look to AC '97 component specification revision 2.1
 *  by Intel Corporation (http://developer.intel.com).
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

#include "mixer.h"
#include "info.h"

/*
 *  ASAHI KASEI - AK4531 codec
 *  - not really AC'97 codec, but it uses very similar interface as AC'97
 */

/*
 *  AK4531 codec registers
 */

#define AK4531_LMASTER  0x00	/* master volume left */
#define AK4531_RMASTER  0x01	/* master volume right */
#define AK4531_LVOICE   0x02	/* channel volume left */
#define AK4531_RVOICE   0x03	/* channel volume right */
#define AK4531_LFM      0x04	/* FM volume left */
#define AK4531_RFM      0x05	/* FM volume right */
#define AK4531_LCD      0x06	/* CD volume left */
#define AK4531_RCD      0x07	/* CD volume right */
#define AK4531_LLINE    0x08	/* LINE volume left */
#define AK4531_RLINE    0x09	/* LINE volume right */
#define AK4531_LAUXA    0x0a	/* AUXA volume left */
#define AK4531_RAUXA    0x0b	/* AUXA volume right */
#define AK4531_MONO1    0x0c	/* MONO1 volume left */
#define AK4531_MONO2    0x0d	/* MONO1 volume right */
#define AK4531_MIC      0x0e	/* MIC volume */
#define AK4531_MONO_OUT 0x0f	/* Mono-out volume */
#define AK4531_OUT_SW1  0x10	/* Output mixer switch 1 */
#define AK4531_OUT_SW2  0x11	/* Output mixer switch 2 */
#define AK4531_LIN_SW1  0x12	/* Input left mixer switch 1 */
#define AK4531_RIN_SW1  0x13	/* Input right mixer switch 1 */
#define AK4531_LIN_SW2  0x14	/* Input left mixer switch 2 */
#define AK4531_RIN_SW2  0x15	/* Input right mixer switch 2 */
#define AK4531_RESET    0x16	/* Reset & power down */
#define AK4531_CLOCK    0x17	/* Clock select */
#define AK4531_AD_IN    0x18	/* AD input select */
#define AK4531_MIC_GAIN 0x19	/* MIC amplified gain */

typedef struct snd_stru_ak4531 ak4531_t;

struct snd_stru_ak4531 {
	void (*write) (void *private_data, unsigned short reg, unsigned short val);
	snd_info_entry_t *proc_entry;
	void *private_data;
	void (*private_free) (void *private_data);
	/* --- */
	unsigned char regs[0x20];
	spinlock_t reg_lock;

	snd_kmixer_element_t *me_in_accu;
	snd_kmixer_element_t *me_out_accu;
	snd_kmixer_element_t *me_mono_accu;
	snd_kmixer_element_t *me_vol_master;
	snd_kmixer_element_t *me_sw_master;
	snd_kmixer_element_t *me_vol_master_mono;
	snd_kmixer_element_t *me_sw_master_mono;
	snd_kmixer_element_t *me_playback;
	snd_kmixer_element_t *me_vol_pcm;
	snd_kmixer_element_t *me_sw_pcm;
	snd_kmixer_element_t *me_sw_pcm_out;
	snd_kmixer_element_t *me_sw_pcm_in;
	snd_kmixer_element_t *me_playback1;
	snd_kmixer_element_t *me_vol_pcm1;
	snd_kmixer_element_t *me_sw_pcm1;
	snd_kmixer_element_t *me_sw_pcm1_out;
	snd_kmixer_element_t *me_sw_pcm1_in;
	snd_kmixer_element_t *me_vol_cd;
	snd_kmixer_element_t *me_sw_cd;
	snd_kmixer_element_t *me_sw_cd_out;
	snd_kmixer_element_t *me_sw_cd_in;
	snd_kmixer_element_t *me_vol_line;
	snd_kmixer_element_t *me_sw_line;
	snd_kmixer_element_t *me_sw_line_out;
	snd_kmixer_element_t *me_sw_line_in;
	snd_kmixer_element_t *me_vol_aux;
	snd_kmixer_element_t *me_sw_aux;
	snd_kmixer_element_t *me_sw_aux_out;
	snd_kmixer_element_t *me_sw_aux_in;
	snd_kmixer_element_t *me_sw_mono_bypass;
	snd_kmixer_element_t *me_vol_mono;
	snd_kmixer_element_t *me_sw_mono;
	snd_kmixer_element_t *me_sw_mono_out;
	snd_kmixer_element_t *me_sw_mono_in;
	snd_kmixer_element_t *me_sw_mono1_bypass;
	snd_kmixer_element_t *me_vol_mono1;
	snd_kmixer_element_t *me_sw_mono1;
	snd_kmixer_element_t *me_sw_mono1_out;
	snd_kmixer_element_t *me_sw_mono1_in;
	snd_kmixer_element_t *me_vol_mic_gain;
	snd_kmixer_element_t *me_sw_mic_bypass;
	snd_kmixer_element_t *me_vol_mic;
	snd_kmixer_element_t *me_sw_mic;
	snd_kmixer_element_t *me_sw_mic_out;
	snd_kmixer_element_t *me_sw_mic_in;
	snd_kmixer_element_t *me_capture;
};

int snd_ak4531_mixer(snd_card_t * card, int device,
		     ak4531_t * ak4531, int pcm_count, int *pcm_devs,
		     snd_kmixer_t ** rmixer);

#endif				/* __AK4531_CODEC_H */
