
#ifndef __AD1816A_H
#define __AD1816A_H

/*
    ad1816a.h - definitions for ADI SoundPort AD1816A chip.
    Copyright (C) 1999-2000 by Massimo Piccioni <dafastidio@libero.it>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "pcm.h"
#include "mixer.h"

#define AD1816A_REG(r)			(codec->port + r)

#define AD1816A_CHIP_STATUS		0x00
#define AD1816A_INDIR_ADDR		0x00
#define AD1816A_INTERRUPT_STATUS	0x01
#define AD1816A_INDIR_DATA_LOW		0x02
#define AD1816A_INDIR_DATA_HIGH		0x03
#define AD1816A_PIO_DEBUG		0x04
#define AD1816A_PIO_STATUS		0x05
#define AD1816A_PIO_DATA		0x06
#define AD1816A_RESERVED_7		0x07
#define AD1816A_PLAYBACK_CONFIG		0x08
#define AD1816A_CAPTURE_CONFIG		0x09
#define AD1816A_RESERVED_10		0x0a
#define AD1816A_RESERVED_11		0x0b
#define AD1816A_JOYSTICK_RAW_DATA	0x0c
#define AD1816A_JOYSTICK_CTRL		0x0d
#define AD1816A_JOY_POS_DATA_LOW	0x0e
#define AD1816A_JOY_POS_DATA_HIGH	0x0f

#define AD1816A_LOW_BYTE_TMP		0x00
#define AD1816A_INTERRUPT_ENABLE	0x01
#define AD1816A_EXTERNAL_CTRL		0x01
#define AD1816A_PLAYBACK_SAMPLE_RATE	0x02
#define AD1816A_CAPTURE_SAMPLE_RATE	0x03
#define AD1816A_VOICE_ATT		0x04
#define AD1816A_FM_ATT			0x05
#define AD1816A_I2S_1_ATT		0x06
#define AD1816A_I2S_0_ATT		0x07
#define AD1816A_PLAYBACK_BASE_COUNT	0x08
#define AD1816A_PLAYBACK_CURR_COUNT	0x09
#define AD1816A_CAPTURE_BASE_COUNT	0x0a
#define AD1816A_CAPTURE_CURR_COUNT	0x0b
#define AD1816A_TIMER_BASE_COUNT	0x0c
#define AD1816A_TIMER_CURR_COUNT	0x0d
#define AD1816A_MASTER_ATT		0x0e
#define AD1816A_CD_GAIN_ATT		0x0f
#define AD1816A_SYNTH_GAIN_ATT		0x10
#define AD1816A_VID_GAIN_ATT		0x11
#define AD1816A_LINE_GAIN_ATT		0x12
#define AD1816A_MIC_GAIN_ATT		0x13
#define AD1816A_PHONE_IN_GAIN_ATT	0x13
#define AD1816A_ADC_SOURCE_SEL		0x14
#define AD1816A_ADC_PGA			0x14
#define AD1816A_CHIP_CONFIG		0x20
#define AD1816A_DSP_CONFIG		0x21
#define AD1816A_FM_SAMPLE_RATE		0x22
#define AD1816A_I2S_1_SAMPLE_RATE	0x23
#define AD1816A_I2S_0_SAMPLE_RATE	0x24
#define AD1816A_RESERVED_37		0x25
#define AD1816A_PROGRAM_CLOCK_RATE	0x26
#define AD1816A_3D_PHAT_CTRL		0x27
#define AD1816A_PHONE_OUT_ATT		0x27
#define AD1816A_RESERVED_40		0x28
#define AD1816A_HW_VOL_BUT		0x29
#define AD1816A_DSP_MAILBOX_0		0x2a
#define AD1816A_DSP_MAILBOX_1		0x2b
#define AD1816A_POWERDOWN_CTRL		0x2c
#define AD1816A_VERSION_ID		0x2d
#define AD1816A_RESERVED_46		0x2e

#define AD1816A_READY			0x80

#define AD1816A_PLAYBACK_IRQ_PENDING	0x80
#define AD1816A_CAPTURE_IRQ_PENDING	0x40

#define AD1816A_PLAYBACK_ENABLE		0x01
#define AD1816A_PLAYBACK_PIO		0x02
#define AD1816A_CAPTURE_ENABLE		0x01
#define AD1816A_CAPTURE_PIO		0x02

#define AD1816A_FMT_LINEAR_8		0x00
#define AD1816A_FMT_ULAW_8		0x08
#define AD1816A_FMT_LINEAR_16_LIT	0x10
#define AD1816A_FMT_ALAW_8		0x18
#define AD1816A_FMT_LINEAR_16_BIG	0x30
#define AD1816A_FMT_ALL			0x38
#define AD1816A_FMT_STEREO		0x04

#define AD1816A_PLAYBACK_IRQ_ENABLE	0x8000
#define AD1816A_CAPTURE_IRQ_ENABLE	0x4000

#define AD1816A_SRC_CD			0x02
#define AD1816A_SRC_LINE		0x00
#define AD1816A_SRC_MIC			0x05
#define AD1816A_SRC_OUT			0x01
#define AD1816A_SRC_PHONE_IN		0x06
#define AD1816A_SRC_SYNTH		0x03
#define AD1816A_SRC_VIDEO		0x04
#define AD1816A_SRC_MONO		0x05
#define AD1816A_SRC_MASK		0x07

#define AD1816A_CAPTURE_NOT_EQUAL	0x1000
#define AD1816A_WSS_ENABLE		0x8000

typedef struct snd_stru_ad1816a ad1816a_t;

struct snd_stru_ad1816a {
	unsigned short port;
	unsigned short irq;
	unsigned short dma1;
	unsigned short dma2;
	snd_dma_t *dma1ptr;
	snd_dma_t *dma2ptr;
	unsigned short version;

	spinlock_t lock;

	unsigned short mode;

	snd_card_t *card;
	snd_pcm_t *pcm;

	snd_pcm_subchn_t *playback_subchn;
	snd_pcm_subchn_t *capture_subchn;
	unsigned int p_dma_size;
	unsigned int c_dma_size;

	snd_kmixer_t *mixer;

	snd_kmixer_element_t *me_accu;
	snd_kmixer_element_t *me_mux;

	snd_kmixer_element_t *me_mux_cd;
	snd_kmixer_element_t *me_mux_line;
	snd_kmixer_element_t *me_mux_mic;
	snd_kmixer_element_t *me_mux_out;
	snd_kmixer_element_t *me_mux_phone_in;
	snd_kmixer_element_t *me_mux_synth;
	snd_kmixer_element_t *me_mux_video;

	snd_kmixer_element_t *me_vol_adc;
	snd_kmixer_element_t *me_vol_cd;
	snd_kmixer_element_t *me_vol_fm;
	snd_kmixer_element_t *me_vol_line;
	snd_kmixer_element_t *me_vol_master;
	snd_kmixer_element_t *me_vol_mic;
	snd_kmixer_element_t *me_vol_phone_in;
	snd_kmixer_element_t *me_vol_phone_out;
	snd_kmixer_element_t *me_vol_synth;
	snd_kmixer_element_t *me_vol_video;
	snd_kmixer_element_t *me_vol_voice;
	snd_kmixer_element_t *me_vol_3d_phat;

	snd_kmixer_element_t *me_sw_adc;
	snd_kmixer_element_t *me_sw_cd;
	snd_kmixer_element_t *me_sw_fm;
	snd_kmixer_element_t *me_sw_line;
	snd_kmixer_element_t *me_sw_master;
	snd_kmixer_element_t *me_sw_mic;
	snd_kmixer_element_t *me_sw_mic_gain;
	snd_kmixer_element_t *me_sw_phone_in;
	snd_kmixer_element_t *me_sw_phone_out;
	snd_kmixer_element_t *me_sw_synth;
	snd_kmixer_element_t *me_sw_video;
	snd_kmixer_element_t *me_sw_voice;
	snd_kmixer_element_t *me_sw_3d_phat;

	snd_kmixer_element_t *me_in_cd;
	snd_kmixer_element_t *me_in_line;
	snd_kmixer_element_t *me_in_mic;
	snd_kmixer_element_t *me_in_phone;
	snd_kmixer_element_t *me_in_synth;
	snd_kmixer_element_t *me_in_video;

	snd_kmixer_element_t *me_out_master;
	snd_kmixer_element_t *me_out_phone;

	snd_kmixer_element_t *me_adc;
	snd_kmixer_element_t *me_capture;
	snd_kmixer_element_t *me_dac;
	snd_kmixer_element_t *me_dig_accu;
	snd_kmixer_element_t *me_playback;
};

#define AD1816A_MODE_PLAYBACK	0x01
#define AD1816A_MODE_CAPTURE	0x02
#define AD1816A_MODE_OPEN	(AD1816A_MODE_PLAYBACK | AD1816A_MODE_CAPTURE)


extern void snd_ad1816a_interrupt(snd_pcm_t *pcm, unsigned char status);

extern int snd_ad1816a_new_pcm(snd_card_t *card, int device,
			       unsigned short port, snd_irq_t *irqptr,
			       snd_dma_t *dma1ptr, snd_dma_t *dma2ptr,
			       snd_pcm_t **rpcm);

extern int snd_ad1816a_new_mixer(snd_pcm_t *pcm, int device,
				 snd_kmixer_t **rmixer);

#endif	/* __AD1816A_H */

