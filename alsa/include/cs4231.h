#ifndef __CS4231_H
#define __CS4231_H

/*
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>
 *  Definitions for CS4231 & InterWave chips & compatible chips
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
#include "timer.h"

/* IO ports */

#define CS4231P( codec, x ) ( (codec) -> port + c_d_c_CS4231##x )

#define c_d_c_CS4231REGSEL	0
#define c_d_c_CS4231REG		1
#define c_d_c_CS4231STATUS	2
#define c_d_c_CS4231PIO		3

/* codec registers */

#define CS4231_LEFT_INPUT	0x00	/* left input control */
#define CS4231_RIGHT_INPUT	0x01	/* right input control */
#define CS4231_AUX1_LEFT_INPUT	0x02	/* left AUX1 input control */
#define CS4231_AUX1_RIGHT_INPUT	0x03	/* right AUX1 input control */
#define CS4231_AUX2_LEFT_INPUT	0x04	/* left AUX2 input control */
#define CS4231_AUX2_RIGHT_INPUT	0x05	/* right AUX2 input control */
#define CS4231_LEFT_OUTPUT	0x06	/* left output control register */
#define CS4231_RIGHT_OUTPUT	0x07	/* right output control register */
#define CS4231_PLAYBK_FORMAT	0x08	/* clock and data format - playback - bits 7-0 MCE */
#define CS4231_IFACE_CTRL	0x09	/* interface control - bits 7-2 MCE */
#define CS4231_PIN_CTRL		0x0a	/* pin control */
#define CS4231_TEST_INIT	0x0b	/* test and initialization */
#define CS4231_MISC_INFO	0x0c	/* miscellaneaous information */
#define CS4231_LOOPBACK		0x0d	/* loopback control */
#define CS4231_PLY_UPR_CNT	0x0e	/* playback upper base count */
#define CS4231_PLY_LWR_CNT	0x0f	/* playback lower base count */
#define CS4231_ALT_FEATURE_1	0x10	/* alternate #1 feature enable */
#define CS4231_ALT_FEATURE_2	0x11	/* alternate #2 feature enable */
#define CS4231_LEFT_LINE_IN	0x12	/* left line input control */
#define CS4231_RIGHT_LINE_IN	0x13	/* right line input control */
#define CS4231_TIMER_LOW	0x14	/* timer low byte */
#define CS4231_TIMER_HIGH	0x15	/* timer high byte */
#define CS4231_LEFT_MIC_INPUT	0x16	/* left MIC input control register (InterWave only) */
#define CS4231_RIGHT_MIC_INPUT	0x17	/* right MIC input control register (InterWave only) */
#define CS4236_EXT_REG		0x17	/* extended register access */
#define CS4231_IRQ_STATUS	0x18	/* irq status register */
#define CS4231_LINE_LEFT_OUTPUT	0x19	/* left line output control register (InterWave only) */
#define CS4231_VERSION		0x19	/* CS4231(A) - version values */
#define CS4231_MONO_CTRL	0x1a	/* mono input/output control */
#define CS4231_LINE_RIGHT_OUTPUT 0x1b	/* right line output control register (InterWave only) */
#define CS4235_LEFT_MASTER	0x1b	/* left master output control */
#define CS4231_REC_FORMAT	0x1c	/* clock and data format - record - bits 7-0 MCE */
#define CS4231_PLY_VAR_FREQ	0x1d	/* playback variable frequency */
#define CS4235_RIGHT_MASTER	0x1d	/* right master output control */
#define CS4231_REC_UPR_CNT	0x1e	/* record upper count */
#define CS4231_REC_LWR_CNT	0x1f	/* record lower count */

/* definitions for codec register select port - CODECP( REGSEL ) */

#define CS4231_INIT		0x80	/* CODEC is initializing */
#define CS4231_MCE		0x40	/* mode change enable */
#define CS4231_TRD		0x20	/* transfer request disable */

/* definitions for codec status register - CODECP( STATUS ) */

#define CS4231_GLOBALIRQ	0x01	/* IRQ is active */

/* definitions for codec irq status */

#define CS4231_PLAYBACK_IRQ	0x10
#define CS4231_RECORD_IRQ	0x20
#define CS4231_TIMER_IRQ	0x40
#define CS4231_ALL_IRQS		0x70
#define CS4231_REC_UNDERRUN	0x08
#define CS4231_REC_OVERRUN	0x04
#define CS4231_PLY_OVERRUN	0x02
#define CS4231_PLY_UNDERRUN	0x01

/* definitions for CS4231_LEFT_INPUT and CS4231_RIGHT_INPUT registers */

#define CS4231_ENABLE_MIC_GAIN	0x20

#define CS4231_MIXS_LINE	0x00
#define CS4231_MIXS_AUX1	0x40
#define CS4231_MIXS_MIC		0x80
#define CS4231_MIXS_ALL		0xc0

/* definitions for clock and data format register - CS4231_PLAYBK_FORMAT */

#define CS4231_LINEAR_8		0x00	/* 8-bit unsigned data */
#define CS4231_ALAW_8		0x60	/* 8-bit A-law companded */
#define CS4231_ULAW_8		0x20	/* 8-bit U-law companded */
#define CS4231_LINEAR_16	0x40	/* 16-bit twos complement data - little endian */
#define CS4231_LINEAR_16_BIG	0xc0	/* 16-bit twos complement data - big endian */
#define CS4231_ADPCM_16		0xa0	/* 16-bit ADPCM */
#define CS4231_STEREO		0x10	/* stereo mode */
/* bits 3-1 define frequency divisor */
#define CS4231_XTAL1		0x00	/* 24.576 crystal */
#define CS4231_XTAL2		0x01	/* 16.9344 crystal */

/* definitions for interface control register - CS4231_IFACE_CTRL */

#define CS4231_RECORD_PIO	0x80	/* record PIO enable */
#define CS4231_PLAYBACK_PIO	0x40	/* playback PIO enable */
#define CS4231_CALIB_MODE	0x18	/* calibration mode bits */
#define CS4231_AUTOCALIB	0x08	/* auto calibrate */
#define CS4231_SINGLE_DMA	0x04	/* use single DMA channel */
#define CS4231_RECORD_ENABLE	0x02	/* record enable */
#define CS4231_PLAYBACK_ENABLE	0x01	/* playback enable */

/* definitions for pin control register - CS4231_PIN_CTRL */

#define CS4231_IRQ_ENABLE	0x02	/* enable IRQ */
#define CS4231_XCTL1		0x40	/* external control #1 */
#define CS4231_XCTL0		0x80	/* external control #0 */

/* definitions for test and init register - CS4231_TEST_INIT */

#define CS4231_CALIB_IN_PROGRESS 0x20	/* auto calibrate in progress */
#define CS4231_DMA_REQUEST	0x10	/* DMA request in progress */

/* definitions for misc control register - CS4231_MISC_INFO */

#define CS4231_MODE2		0x40	/* MODE 2 */
#define CS4231_IW_MODE3		0x6c	/* MODE 3 - InterWave enhanced mode */
#define CS4231_4236_MODE3	0xe0	/* MODE 3 - CS4236+ enhanced mode */

/* definitions for alternate feature 1 register - CS4231_ALT_FEATURE_1 */

#define	CS4231_DACZ		0x01	/* zero DAC when underrun */
#define CS4231_TIMER_ENABLE	0x40	/* codec timer enable */
#define CS4231_OLB		0x80	/* output level bit */

/* definitions for Extended Registers - CS4236+ */

#define CS4236_REG(reg)		(((reg << 2) & 0x10) | ((reg >> 4) & 0x0f))

#define CS4236_LEFT_LINE	0x08	/* left LINE alternate volume */
#define CS4236_RIGHT_LINE	0x18	/* right LINE alternate volume */
#define CS4236_LEFT_MIC		0x28	/* left MIC volume */
#define CS4236_RIGHT_MIC	0x38	/* right MIC volume */
#define CS4236_LEFT_MIX_CTRL	0x48	/* synthesis and left input mixer control */
#define CS4236_RIGHT_MIX_CTRL	0x58	/* right input mixer control */
#define CS4236_LEFT_FM		0x68	/* left FM volume */
#define CS4236_RIGHT_FM		0x78	/* right FM volume */
#define CS4236_LEFT_DSP		0x88	/* left DSP serial port volume */
#define CS4236_RIGHT_DSP	0x98	/* right DSP serial port volume */
#define CS4236_RIGHT_LOOPBACK	0xa8	/* right loopback monitor volume */
#define CS4236_DAC_MUTE		0xb8	/* DAC mute and IFSE enable */
#define CS4236_ADC_RATE		0xc8	/* indenpendent ADC sample frequency */
#define CS4236_DAC_RATE		0xd8	/* indenpendent DAC sample frequency */
#define CS4236_LEFT_MASTER	0xe8	/* left master digital audio volume */
#define CS4236_RIGHT_MASTER	0xf8	/* right master digital audio volume */
#define CS4236_LEFT_WAVE	0x0c	/* left wavetable serial port volume */
#define CS4236_RIGHT_WAVE	0x1c	/* right wavetable serial port volume */
#define CS4236_VERSION		0x9c	/* chip version and ID */

/* some structures */

struct snd_stru_cs4231_freq {
	unsigned int hertz;
	unsigned int rate;
	unsigned char bits;
};

/* defines for codec.mode */

#define CS4231_MODE_NONE	0x0000
#define CS4231_MODE_PLAY	0x0001
#define CS4231_MODE_RECORD	0x0002
#define CS4231_MODE_TIMER	0x0004
#define CS4231_MODE_OPEN	(CS4231_MODE_PLAY|CS4231_MODE_RECORD|CS4231_MODE_TIMER)

/* defines for codec.hardware */

#define CS4231_HW_DETECT        0x0000	/* let CS4231 driver detect chip */
#define CS4231_HW_DETECT3	0x0001	/* allow mode 3 */
#define CS4231_HW_TYPE_MASK	0xff00	/* type mask */
#define CS4231_HW_CS4231_MASK   0x0100	/* CS4231 serie */
#define CS4231_HW_CS4231        0x0100	/* CS4231 chip */
#define CS4231_HW_CS4231A       0x0101	/* CS4231A chip */
#define CS4231_HW_CS4232_MASK   0x0200	/* CS4232 serie */
#define CS4231_HW_CS4232        0x0200	/* CS4232 */
#define CS4231_HW_CS4232A       0x0201	/* CS4232A */
#define CS4231_HW_CS4236_MASK	0x0400	/* CS4236 serie */
#define CS4231_HW_CS4235	0x0400	/* CS4235 - Crystal Clear (tm) stereo enhancement */
#define CS4231_HW_CS4236        0x0401	/* CS4236 */
#define CS4231_HW_CS4236B       0x0402	/* CS4236B */
#define CS4231_HW_CS4237B       0x0403	/* CS4237B - SRS 3D */
#define CS4231_HW_CS4238B	0x0404	/* CS4238B - QSOUND 3D */
#define CS4231_HW_CS4239	0x0405	/* CS4239 - Crystal Clear (tm) stereo enhancement */
/* compatible, but clones */
#define CS4231_HW_INTERWAVE     0x1000	/* InterWave chip */
#define CS4231_HW_OPL3SA        0x1001	/* OPL3-SA chip */

typedef struct snd_stru_cs4231 cs4231_t;

struct snd_stru_cs4231 {
	unsigned short port;		/* base i/o port */
	unsigned short cport;		/* control base i/o port (CS4236) */
	unsigned short irq;		/* IRQ line */
	snd_irq_t * irqptr;		/* IRQ pointer */
	unsigned short dma1;		/* playback DMA */
	unsigned short dma2;		/* record DMA */
	snd_dma_t * dmaptr1;		/* DMA pointer - playback */
	snd_dma_t * dmaptr2;		/* DMA pointer - record */
	unsigned short version;		/* version of CODEC chip */
	unsigned short mode;		/* see to CS4231_MODE_XXXX */
	unsigned short hardware;	/* see to CS4231_HW_XXXX */
	unsigned short single_dma:1;	/* forced single DMA mode (GUS 16-bit daughter board) or dma1 == dma2 */

	snd_card_t *card;
	snd_pcm_t *pcm;
	snd_pcm_subchn_t *playback_subchn;
	snd_pcm1_subchn_t *playback_subchn1;
	snd_pcm_subchn_t *capture_subchn;
	snd_pcm1_subchn_t *capture_subchn1;
	snd_kmixer_t *mixer;
	snd_timer_t *timer;

	unsigned char image[32];	/* image */
	unsigned char eimage[32];	/* extended image */
	int mce_bit;
	int calibrate_mute;
	int sw_3d_bit;

	spinlock_t reg_lock;
	struct semaphore mce_mutex;
	struct semaphore open_mutex;

	snd_kmixer_element_t *me_mux_mic;
	snd_kmixer_element_t *me_mux_line;
	snd_kmixer_element_t *me_mux_aux1;
	snd_kmixer_element_t *me_mux_mix;

	unsigned int (*set_playback_rate) (cs4231_t * codec, unsigned int rate);
	unsigned int (*set_capture_rate) (cs4231_t * codec, unsigned int rate);
	void (*set_playback_format) (cs4231_t * codec, unsigned char pdfr);
	void (*set_capture_format) (cs4231_t * codec, unsigned char cdfr);
};

/* exported functions */

void snd_cs4231_out(cs4231_t * codec, unsigned char reg, unsigned char val);
unsigned char snd_cs4231_in(cs4231_t * codec, unsigned char reg);
void snd_cs4231_outm(cs4231_t * codec, unsigned char reg, unsigned char mask, unsigned char val);
void snd_cs4236_ext_out(cs4231_t * codec, unsigned char reg, unsigned char val);
unsigned char snd_cs4236_ext_in(cs4231_t * codec, unsigned char reg);
void snd_cs4231_mce_up(cs4231_t * codec);
void snd_cs4231_mce_down(cs4231_t * codec);

void snd_cs4231_interrupt(snd_pcm_t * pcm, unsigned char status);

extern snd_pcm_t *snd_cs4231_new_device(snd_card_t * card,
					unsigned short port,
					snd_irq_t * irqptr,
					snd_dma_t * dmaptr1,
					snd_dma_t * dmaptr2,
					unsigned short hardware,
					int timer_dev);

snd_kmixer_t *snd_cs4231_new_mixer(snd_pcm_t * pcm, int pcm_dev);

extern snd_pcm_t *snd_cs4236_new_device(snd_card_t * card,
					unsigned short port,
					unsigned short cport,
					snd_irq_t * irqptr,
					snd_dma_t * dmaptr1,
					snd_dma_t * dmaptr2,
					unsigned short hardware,
					int timer_dev);

snd_kmixer_t *snd_cs4236_new_mixer(snd_pcm_t * pcm, int pcm_device);

/*
 *  mixer library
 */

int snd_cs4231_mixer_stereo_volume(int w_flag, int *voices, cs4231_t *codec,
					int max, int invert, int shift,
					unsigned char left_reg,
					unsigned char right_reg);
int snd_cs4231_mixer_mono_volume(int w_flag, int *voices, cs4231_t *codec,
					int max, int invert, int shift,
					unsigned char reg);
int snd_cs4231_mixer_stereo_switch(int w_flag, unsigned int *bitmap, cs4231_t *codec,
					int bit, int invert,
					unsigned char left_reg,
					unsigned char right_reg);
int snd_cs4231_mixer_mono_switch(int w_flag, unsigned int *bitmap, cs4231_t *codec,
					int bit, int invert,
					unsigned char reg);
int snd_cs4231_mixer_line_volume(int w_flag, int *voices, cs4231_t *codec);
int snd_cs4231_mixer_line_switch(int w_flag, unsigned int *bitmap, cs4231_t *codec);
int snd_cs4231_mixer_aux1_volume(int w_flag, int *voices, cs4231_t *codec);
int snd_cs4231_mixer_aux1_switch(int w_flag, unsigned int *bitmap, cs4231_t *codec);
int snd_cs4231_mixer_aux2_volume(int w_flag, int *voices, cs4231_t *codec);
int snd_cs4231_mixer_aux2_switch(int w_flag, unsigned int *bitmap, cs4231_t *codec);
int snd_cs4231_mixer_monoin_volume(int w_flag, int *voices, cs4231_t *codec);
int snd_cs4231_mixer_monoin_switch(int w_flag, unsigned int *bitmap, cs4231_t *codec);
int snd_cs4231_mixer_mono_bypass_switch(int w_flag, unsigned int *bitmap, cs4231_t *codec);
int snd_cs4231_mixer_igain_volume(int w_flag, int *voices, cs4231_t *codec);
int snd_cs4231_mixer_dac_volume(int w_flag, int *voices, cs4231_t *codec);
int snd_cs4231_mixer_dac_switch(int w_flag, unsigned int *bitmap, cs4231_t *codec);

#ifdef CONFIG_SND_DEBUG
void snd_cs4231_debug(cs4231_t * codec);
#endif

#endif				/* __CS4231_H */
