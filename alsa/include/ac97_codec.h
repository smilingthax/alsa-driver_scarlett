#ifndef __AC97_CODEC_H
#define __AC97_CODEC_H

/*
 *  Copyright (c) by Jaroslav Kysela <perex@jcu.cz>
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
 *  AC'97 codec registers
 */

#define AC97_RESET		0x00	/* Reset */
#define AC97_MASTER		0x02	/* Master Volume */
#define AC97_HEADPHONE		0x04	/* Headphone Volume (optional) */
#define AC97_MASTER_MONO	0x06	/* Master Volume Mono (optional) */
#define AC97_MASTER_TONE	0x08	/* Master Tone (Bass & Treble) (optional) */
#define AC97_PC_BEEP		0x0a	/* PC Beep Volume (optinal) */
#define AC97_PHONE		0x0c	/* Phone Volume (optional) */
#define AC97_MIC		0x0e	/* MIC Volume */
#define AC97_LINE		0x10	/* Line In Volume */
#define AC97_CD			0x12	/* CD Volume */
#define AC97_VIDEO		0x14	/* Video Volume (optional) */
#define AC97_AUX		0x16	/* AUX Volume (optional) */
#define AC97_PCM		0x18	/* PCM Volume */
#define AC97_REC_SEL		0x1a	/* Record Select */
#define AC97_REC_GAIN		0x1c	/* Record Gain */
#define AC97_REC_GAIN_MIC	0x1e	/* Record Gain MIC (optional) */
#define AC97_GENERAL_PURPOSE	0x20	/* General Purpose (optional) */
#define AC97_3D_CONTROL		0x22	/* 3D Control (optional) */
#define AC97_RESERVED		0x24	/* Reserved */
#define AC97_POWERDOWN		0x26	/* Powerdown control / status */
/* range 0x28-0x3a - AUDIO */
/* range 0x3c-0x58 - MODEM */
/* range 0x5a-0x7b - Vendor Specific */
#define AC97_VENDOR_ID1		0x7c	/* Vendor ID1 */
#define AC97_VENDOR_ID2		0x7e	/* Vendor ID2 / revision */

/*

 */

typedef struct snd_stru_ac97 ac97_t;

struct snd_stru_ac97 {
	void (*write) (void *private_data, unsigned short reg, unsigned short val);
	unsigned short (*read) (void *private_data, unsigned short reg);
	snd_info_entry_t *proc_entry;
	void *private_data;
	void (*private_free) (ac97_t * ac97);
	/* --- */
	int rev_is_not_rev;
	unsigned int id;	/* identification of codec */
	char name[64];		/* CODEC name */
	unsigned short caps;	/* capabilities (register 0) */
	unsigned short micgain;	/* mic gain is active */
	snd_kmixer_channel_t *mic_channel;
	snd_spin_define(access);
};

snd_kmixer_t *snd_ac97_mixer(snd_card_t * card, ac97_t * ac97);

/*
 *  ASAHI KASEI - AK4531 codec
 *  - not really AC'97 codec, but it uses very similar interface as AC'97
 */

/*
 *  AK4531 codec registers
 */

#define AK4531_LMASTER  0x00	/* master volume left */
#define AK4531_RMASTER  0x01	/* master volume right */
#define AK4531_LVOICE   0x02	/* voice volume left */
#define AK4531_RVOICE   0x03	/* voice volume right */
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
	void (*private_free) (ak4531_t * ak4531);
	/* --- */
	unsigned char regs[0x10];
	unsigned char out_sw1;
	unsigned char out_sw2;
	unsigned char lin_sw1;
	unsigned char rin_sw1;
	unsigned char lin_sw2;
	unsigned char rin_sw2;
	unsigned short adin;
	unsigned short micgain;
	snd_kmixer_channel_t *mic_channel;
	snd_spin_define(access);
};

snd_kmixer_t *snd_ak4531_mixer(snd_card_t * card, ak4531_t * ak4531);

#endif				/* __AC97_CODEC_H */
