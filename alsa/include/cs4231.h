#ifndef __CS4231_H
#define __CS4231_H

/*
 *  Copyright (c) by Jaroslav Kysela <perex@jcu.cz>
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

struct snd_stru_cs4231_image {
  unsigned char lic;		/* 00: left input (ADC) control */
  unsigned char ric;		/* 01: right input (ADC) control */
  unsigned char la1ic;		/* 02: left AUX #1 input control */
  unsigned char ra1ic;		/* 03: right AUX #1 input control */
  unsigned char la2ic;		/* 04: left AUX #2 input control */
  unsigned char ra2ic;		/* 05: right AUX #2 input control */
  unsigned char loc;		/* 06: left output (DAC) control */
  unsigned char roc;		/* 07: right output (DAC) control*/
  unsigned char pdfr;		/* 08: clock and data format - playback */
  unsigned char ic;		/* 09: interface control */
  unsigned char pc;		/* 0a: pin control */
  unsigned char ti;		/* 0b: test and initialization */
  unsigned char mi;		/* 0c: miscellaneaous information */
  unsigned char lbc;		/* 0d: loopback control */
  unsigned char pbru;		/* 0e: playback upper base count */
  unsigned char pbrl;		/* 0f: playback lower base count */
  unsigned char afei;		/* 10: alternate #1 feature enable */
  unsigned char afeii;		/* 11: alternate #2 feature enable */
  unsigned char llic;		/* 12: left line input control */
  unsigned char rlic;		/* 13: right line input control */
  unsigned char tlb;		/* 14: timer low byte */
  unsigned char thb;		/* 15: timer high byte */
  unsigned char la3ic;		/* 16: left MIC input control register (InterWave only) */
  unsigned char ra3ic;		/* 17: right MIC input control register (InterWave only) */
  unsigned char afs;		/* 18: irq status register */
  unsigned char lamic;		/* 19: left line output control register (InterWave only) */
  unsigned char mioc;		/* 1a: mono input/output control */
  unsigned char ramic;		/* 1b: right line output control register (InterWave only) */
  unsigned char cdfr;		/* 1c: clock and data format - record */
  unsigned char pdfr_var;	/* 1d: playback variable frequency (InterWave only) */
  unsigned char cbru;		/* 1e: record upper count */
  unsigned char cbrl;		/* 1f: record lower count */
};

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
#define CS4231_REC_FORMAT	0x1c	/* clock and data format - record - bits 7-0 MCE */
#define CS4231_PLY_VAR_FREQ	0x1d	/* playback variable frequency */
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
#define CS4231_4236_MODE3	0x60	/* MODE 3 - CS4236+ enhanced mode */

/* definitions for alternate feature 1 register - CS4231_ALT_FEATURE_1 */

#define	CS4231_DACZ		0x01	/* zero DAC when underrun */
#define CS4231_TIMER_ENABLE	0x40	/* codec timer enable */
#define CS4231_OLB		0x80	/* output level bit */

/* definitions for Extended Registers - CS4236+ */

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

#define CS4231_HW_DETECT        0x0000  /* let CS4231 driver detect chip */
#define CS4231_HW_CS4231_MASK   0x0100  /* CS4231 serie */
#define CS4231_HW_CS4231        0x0100  /* CS4231 chip */
#define CS4231_HW_CS4231A       0x0101  /* CS4231A chip */
#define CS4231_HW_CS4232_MASK   0x0200  /* CS4232 serie */
#define CS4231_HW_CS4232        0x0200  /* CS4232 */
#define CS4231_HW_CS4232A       0x0201  /* CS4232A */
#define CS4231_HW_CS4236_MASK	0x0400	/* CS4236 serie */
#define CS4231_HW_CS4235	0x0400	/* CS4235 - Crystal Clear (tm) stereo enhancement */
#define CS4231_HW_CS4236        0x0401  /* CS4236 */
#define CS4231_HW_CS4236B       0x0402  /* CS4236B */
#define CS4231_HW_CS4237B       0x0403  /* CS4237B - SRS 3D */
#define CS4231_HW_CS4238B	0x0404	/* CS4238B - QSOUND 3D */
#define CS4231_HW_CS4239	0x0405	/* CS4239 - Crystal Clear (tm) stereo enhancement */
/* compatible, but clones */
#define CS4231_HW_INTERWAVE     0x1000  /* InterWave chip */
#define CS4231_HW_OPL3SA        0x1001  /* OPL3-SA chip */

struct snd_stru_cs4231 {
  unsigned short port;		/* i/o port */
  unsigned short irq;		/* IRQ line */
  unsigned short irqnum;	/* IRQ number */
  unsigned short dma1;		/* playback DMA */
  unsigned short dma2;		/* record DMA */
  unsigned short dmanum1;	/* DMA number - playback */
  unsigned short dmanum2;	/* DMA number - record */
  unsigned short version;	/* version of CODEC chip */
  unsigned short mode;		/* see to CS4231_MODE_XXXX */
  unsigned short hardware;	/* see to CS4231_HW_XXXX */
  unsigned short single_dma: 1;	/* forced single DMA mode (GUS 16-bit daughter board) or dma1 == dma2 */

  snd_pcm_t *pcm;
  snd_card_t *card;
  snd_kmixer_t *mixer;
  snd_timer_t *timer;
  
  struct snd_stru_cs4231_image image;
  int mce_bit;
  int calibrate_mute;

  snd_spin_define( reg );
  snd_mutex_define( mce );
  snd_mutex_define( open );

#if 0
  int playback_fifo_size;		/* -1 = disabled */
  int record_fifo_size;			/* -1 = disabled */
  snd_mem_block_t *playback_fifo_block;
  snd_mem_block_t *record_fifo_block;
  unsigned short interwave_fifo_reg;
  unsigned char interwave_serial_port;
#endif
};

typedef struct snd_stru_cs4231 cs4231_t;

/* exported functions */

void snd_cs4231_interrupt( snd_pcm_t *pcm, unsigned char status );

extern snd_pcm_t *snd_cs4231_new_device( snd_card_t *card,
                                         unsigned short port,
                                         unsigned short irqnum,
                                         unsigned short dmanum1,
                                         unsigned short dmanum2,
                                         unsigned short hardware );

snd_kmixer_t *snd_cs4231_new_mixer( snd_pcm_t *pcm );

#ifdef SNDCFG_DEBUG
void snd_cs4231_debug( cs4231_t *codec );
#endif

#endif /* __CS4231_H */
