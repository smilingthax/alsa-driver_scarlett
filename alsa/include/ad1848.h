#ifndef __AD1848_H
#define __AD1848_H

/*
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>
 *  Definitions for AD1847/AD1848/CS4248 chips
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
#include "mixer.h"

/* IO ports */

#define AD1848P( codec, x ) ( (codec) -> port + c_d_c_AD1848##x )

#define c_d_c_AD1848REGSEL	0
#define c_d_c_AD1848REG		1
#define c_d_c_AD1848STATUS	2
#define c_d_c_AD1848PIO		3

/* codec registers */

#define AD1848_LEFT_INPUT	0x00	/* left input control */
#define AD1848_RIGHT_INPUT	0x01	/* right input control */
#define AD1848_AUX1_LEFT_INPUT	0x02	/* left AUX1 input control */
#define AD1848_AUX1_RIGHT_INPUT	0x03	/* right AUX1 input control */
#define AD1848_AUX2_LEFT_INPUT	0x04	/* left AUX2 input control */
#define AD1848_AUX2_RIGHT_INPUT	0x05	/* right AUX2 input control */
#define AD1848_LEFT_OUTPUT	0x06	/* left output control register */
#define AD1848_RIGHT_OUTPUT	0x07	/* right output control register */
#define AD1848_DATA_FORMAT	0x08	/* clock and data format - playback/capture - bits 7-0 MCE */
#define AD1848_IFACE_CTRL	0x09	/* interface control - bits 7-2 MCE */
#define AD1848_PIN_CTRL		0x0a	/* pin control */
#define AD1848_TEST_INIT	0x0b	/* test and initialization */
#define AD1848_MISC_INFO	0x0c	/* miscellaneaous information */
#define AD1848_LOOPBACK		0x0d	/* loopback control */
#define AD1848_DATA_UPR_CNT	0x0e	/* playback/capture upper base count */
#define AD1848_DATA_LWR_CNT	0x0f	/* playback/capture lower base count */

/* definitions for codec register select port - CODECP( REGSEL ) */

#define AD1848_INIT		0x80	/* CODEC is initializing */
#define AD1848_MCE		0x40	/* mode change enable */
#define AD1848_TRD		0x20	/* transfer request disable */

/* definitions for codec status register - CODECP( STATUS ) */

#define AD1848_GLOBALIRQ	0x01	/* IRQ is active */

/* definitions for AD1848_LEFT_INPUT and AD1848_RIGHT_INPUT registers */

#define AD1848_ENABLE_MIC_GAIN	0x20

#define AD1848_MIXS_LINE1	0x00
#define AD1848_MIXS_AUX1	0x40
#define AD1848_MIXS_LINE2	0x80
#define AD1848_MIXS_ALL		0xc0

/* definitions for clock and data format register - AD1848_PLAYBK_FORMAT */

#define AD1848_LINEAR_8		0x00	/* 8-bit unsigned data */
#define AD1848_ALAW_8		0x60	/* 8-bit A-law companded */
#define AD1848_ULAW_8		0x20	/* 8-bit U-law companded */
#define AD1848_LINEAR_16	0x40	/* 16-bit twos complement data - little endian */
#define AD1848_STEREO		0x10	/* stereo mode */
/* bits 3-1 define frequency divisor */
#define AD1848_XTAL1		0x00	/* 24.576 crystal */
#define AD1848_XTAL2		0x01	/* 16.9344 crystal */

/* definitions for interface control register - AD1848_IFACE_CTRL */

#define AD1848_CAPTURE_PIO	0x80	/* capture PIO enable */
#define AD1848_PLAYBACK_PIO	0x40	/* playback PIO enable */
#define AD1848_CALIB_MODE	0x18	/* calibration mode bits */
#define AD1848_AUTOCALIB	0x08	/* auto calibrate */
#define AD1848_SINGLE_DMA	0x04	/* use single DMA channel */
#define AD1848_CAPTURE_ENABLE	0x02	/* capture enable */
#define AD1848_PLAYBACK_ENABLE	0x01	/* playback enable */

/* definitions for pin control register - AD1848_PIN_CTRL */

#define AD1848_IRQ_ENABLE	0x02	/* enable IRQ */
#define AD1848_XCTL1		0x40	/* external control #1 */
#define AD1848_XCTL0		0x80	/* external control #0 */

/* definitions for test and init register - AD1848_TEST_INIT */

#define AD1848_CALIB_IN_PROGRESS 0x20	/* auto calibrate in progress */
#define AD1848_DMA_REQUEST	0x10	/* DMA request in progress */

/* some structures */

struct snd_stru_ad1848_freq {
	unsigned int hertz;
	unsigned int rate;
	unsigned char bits;
};

/* defines for codec.mode */

#define AD1848_MODE_NONE	0x0000
#define AD1848_MODE_PLAY	0x0001
#define AD1848_MODE_CAPTURE	0x0002
#define AD1848_MODE_TIMER	0x0004
#define AD1848_MODE_OPEN	(AD1848_MODE_PLAY|AD1848_MODE_CAPTURE|AD1848_MODE_TIMER)

/* defines for codec.hardware */

#define AD1848_HW_DETECT	0x0000	/* let AD1848 driver detect chip */
#define AD1848_HW_AD1847	0x0001	/* AD1847 chip */
#define AD1848_HW_AD1848	0x0002	/* AD1848 chip */
#define AD1848_HW_CS4248	0x0003	/* CS4248 chip */
#define AD1848_HW_CMI8330	0x0004	/* CMI8330 chip */

struct snd_stru_ad1848 {
	unsigned short port;		/* i/o port */
	unsigned short irq;		/* IRQ line */
	snd_irq_t * irqptr;		/* IRQ pointer */
	unsigned short dma;		/* data DMA */
	snd_dma_t * dmaptr;		/* data DMA pointer */
	unsigned short version;		/* version of CODEC chip */
	unsigned short mode;		/* see to AD1848_MODE_XXXX */
	unsigned short hardware;	/* see to AD1848_HW_XXXX */
	unsigned short single_dma:1;	/* forced single DMA mode (GUS 16-bit daughter board) or dma1 == dma2 */

	snd_pcm_t *pcm;
	snd_pcm_subchn_t *playback_subchn;
	snd_pcm_subchn_t *capture_subchn;
	snd_card_t *card;
	snd_kmixer_t *mixer;

	unsigned char image[32];	/* SGalaxy needs an access to extended registers */
	int mce_bit;
	int calibrate_mute;
	int dma_size;

	spinlock_t reg_lock;
	struct semaphore open_mutex;

	snd_kmixer_element_t *me_mux_line1;
	snd_kmixer_element_t *me_mux_aux1;
	snd_kmixer_element_t *me_mux_line2;
	snd_kmixer_element_t *me_mux_mix;

	snd_kmixer_element_t *me_mux;
	snd_kmixer_element_t *me_accu;
	snd_kmixer_element_t *me_dig_accu;
	snd_kmixer_element_t *me_vol_aux1;
	snd_kmixer_element_t *me_sw_aux1;
	snd_kmixer_element_t *me_vol_aux2;
	snd_kmixer_element_t *me_sw_aux2;
	snd_kmixer_element_t *me_vol_igain;
	snd_kmixer_element_t *me_capture;
	snd_kmixer_element_t *me_vol_loop;
	snd_kmixer_element_t *me_sw_loop;
	snd_kmixer_element_t *me_playback;
	snd_kmixer_element_t *me_vol_pcm;
	snd_kmixer_element_t *me_sw_pcm;
};

typedef struct snd_stru_ad1848 ad1848_t;

/* exported functions */

void snd_ad1848_out(ad1848_t * codec, unsigned char reg, unsigned char value);

void snd_ad1848_interrupt(snd_pcm_t * pcm, unsigned char status);

int snd_ad1848_new_pcm(snd_card_t * card, int device,
		       unsigned short port,
		       snd_irq_t * irqptr,
		       snd_dma_t * dmaptr,
		       unsigned short hardware,
		       snd_pcm_t ** rpcm);

int snd_ad1848_new_mixer(snd_pcm_t * pcm, int device, snd_kmixer_t ** rmixer);

int snd_ad1848_mixer_stereo_volume(void *private_data, int w_flag, int *voices,
					int bit, int invert, int shift,
					unsigned char left_reg,
					unsigned char right_reg);
int snd_ad1848_mixer_mono_volume(void *private_data, int w_flag, int *voices,
					int bit, int invert, int shift,
					unsigned char reg);
int snd_ad1848_mixer_stereo_switch(void *private_data, int w_flag, unsigned int *bitmap,
					int bit, int invert,
					unsigned char left_reg,
					unsigned char right_reg);
int snd_ad1848_mixer_mono_switch(void *private_data, int w_flag, int *value,
					int bit, int invert, unsigned char reg);

#ifdef CONFIG_SND_DEBUG
void snd_ad1848_debug(ad1848_t * codec);
#endif

#endif				/* __AD1848_H */
