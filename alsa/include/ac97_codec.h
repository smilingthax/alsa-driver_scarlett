#ifndef __AC97_CODEC_H
#define __AC97_CODEC_H

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
#define AC97_EXTENDED_STATUS	0x3a	/* Extended Status */
#define AC97_SURROUND_MASTER	0x38	/* Surround Master Volume */
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
	spinlock_t reg_lock;
	int rev_is_not_rev;
	unsigned int id;	/* identification of codec */
	char name[64];		/* CODEC name */
	unsigned short caps;	/* capabilities (register 0) */
	unsigned short micgain;	/* mic gain is active */
	unsigned short regs[0x3c]; /* register cache */
	unsigned char bass;	/* tone control - bass value */
	unsigned char treble;	/* tone control - treble value */
	unsigned char max_master; /* master maximum volume value */
	unsigned char max_master_mono; /* master mono maximum volume value */
	unsigned char max_headphone; /* headphone maximum volume value */
	unsigned char max_mono;	/* mono maximum volume value */
	unsigned char max_3d;	/* 3d maximum volume value */
	unsigned char shift_3d;	/* 3d shift value */

	snd_kmixer_element_t *me_mux_mic;
	snd_kmixer_element_t *me_mux_cd;
	snd_kmixer_element_t *me_mux_video;
	snd_kmixer_element_t *me_mux_aux;
	snd_kmixer_element_t *me_mux_line;
	snd_kmixer_element_t *me_mux_mix;
	snd_kmixer_element_t *me_mux_mono_mix;
	snd_kmixer_element_t *me_mux_phone;

	snd_kmixer_element_t *me_mux2_out_mono_accu;
	snd_kmixer_element_t *me_mux2_mic;

	snd_kmixer_element_t *me_accu;
	snd_kmixer_element_t *me_pcm_accu;
	snd_kmixer_element_t *me_bypass_accu;
	snd_kmixer_element_t *me_mono_accu;
	snd_kmixer_element_t *me_mono_accu_in;
	snd_kmixer_element_t *me_mux;
	snd_kmixer_element_t *me_mono_mux;
	snd_kmixer_element_t *me_playback;
	snd_kmixer_element_t *me_vol_pcm;
	snd_kmixer_element_t *me_sw_pcm;
	snd_kmixer_element_t *me_vol_pc_beep;
	snd_kmixer_element_t *me_sw_pc_beep;
	snd_kmixer_element_t *me_vol_phone;
	snd_kmixer_element_t *me_sw_phone;
	snd_kmixer_element_t *me_vol_mic;
	snd_kmixer_element_t *me_sw_mic;	
	snd_kmixer_element_t *me_vol_line;
	snd_kmixer_element_t *me_sw_line;
	snd_kmixer_element_t *me_vol_cd;
	snd_kmixer_element_t *me_sw_cd;
	snd_kmixer_element_t *me_vol_video;
	snd_kmixer_element_t *me_sw_video;
	snd_kmixer_element_t *me_vol_aux;
	snd_kmixer_element_t *me_sw_aux;
	snd_kmixer_element_t *me_tone;
	snd_kmixer_element_t *me_vol_master;
	snd_kmixer_element_t *me_sw_master;
	snd_kmixer_element_t *me_out_master;
	snd_kmixer_element_t *me_vol_headphone;
	snd_kmixer_element_t *me_sw_headphone;
	snd_kmixer_element_t *me_out_headphone;
	snd_kmixer_element_t *me_vol_master_mono;
	snd_kmixer_element_t *me_sw_master_mono;
	snd_kmixer_element_t *me_out_master_mono;
	snd_kmixer_element_t *me_vol_igain;
	snd_kmixer_element_t *me_sw_igain;
	snd_kmixer_element_t *me_vol_igain_mic;
	snd_kmixer_element_t *me_sw_igain_mic;
	snd_kmixer_element_t *me_capture;
};

snd_kmixer_t *snd_ac97_mixer(snd_card_t * card, ac97_t * ac97, int pcm_count, int *pcm_devs);

#endif				/* __AC97_CODEC_H */
