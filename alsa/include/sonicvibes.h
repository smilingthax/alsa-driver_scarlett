#ifndef __SONICVIBES_H
#define __SONICVIBES_H

/*
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>
 *  Definitions for S3 SonicVibes chip
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
#include "rawmidi.h"
#include "mpu401.h"

/*
 * Enhanced port direct registers
 */

#define SV_REG(sonic, x) ((sonic)->enh_port + SV_REG_##x)

#define SV_REG_CONTROL	0x00	/* R/W: CODEC/Mixer control register */
#define   SV_ENHANCED	  0x01	/* audio mode select - enhanced mode */
#define   SV_TEST	  0x02	/* test bit */
#define   SV_REVERB	  0x04	/* reverb enable */
#define   SV_WAVETABLE	  0x08	/* wavetable active / FM active if not set */
#define   SV_INTA	  0x20	/* INTA driving - should be always 1 */
#define   SV_RESET	  0x80	/* reset chip */
#define SV_REG_IRQMASK	0x01	/* R/W: CODEC/Mixer interrupt mask register */
#define   SV_DMAA_MASK	  0x01	/* mask DMA-A interrupt */
#define   SV_DMAC_MASK	  0x04	/* mask DMA-C interrupt */
#define   SV_SPEC_MASK	  0x08	/* special interrupt mask - should be always masked */
#define   SV_UD_MASK	  0x40	/* Up/Down button interrupt mask */
#define   SV_MIDI_MASK	  0x80	/* mask MIDI interrupt */
#define SV_REG_STATUS	0x02	/* R/O: CODEC/Mixer status register */
#define   SV_DMAA_IRQ	  0x01	/* DMA-A interrupt */
#define   SV_DMAC_IRQ	  0x04	/* DMA-C interrupt */
#define   SV_SPEC_IRQ	  0x08	/* special interrupt */
#define   SV_UD_IRQ	  0x40	/* Up/Down interrupt */
#define   SV_MIDI_IRQ	  0x80	/* MIDI interrupt */
#define SV_REG_INDEX	0x04	/* R/W: CODEC/Mixer index address register */
#define   SV_MCE          0x40	/* mode change enable */
#define   SV_TRD	  0x80	/* DMA transfer request disabled */
#define SV_REG_DATA	0x05	/* R/W: CODEC/Mixer index data register */

/*
 * Enhanced port indirect registers
 */

#define SV_IREG_LEFT_ADC	0x00	/* Left ADC Input Control */
#define SV_IREG_RIGHT_ADC	0x01	/* Right ADC Input Control */
#define SV_IREG_LEFT_AUX1	0x02	/* Left AUX1 Input Control */
#define SV_IREG_RIGHT_AUX1	0x03	/* Right AUX1 Input Control */
#define SV_IREG_LEFT_CD		0x04	/* Left CD Input Control */
#define SV_IREG_RIGHT_CD	0x05	/* Right CD Input Control */
#define SV_IREG_LEFT_LINE	0x06	/* Left Line Input Control */
#define SV_IREG_RIGHT_LINE	0x07	/* Right Line Input Control */
#define SV_IREG_MIC		0x08	/* MIC Input Control */
#define SV_IREG_GAME_PORT	0x09	/* Game Port Control */
#define SV_IREG_LEFT_SYNTH	0x0a	/* Left Synth Input Control */
#define SV_IREG_RIGHT_SYNTH	0x0b	/* Right Synth Input Control */
#define SV_IREG_LEFT_AUX2	0x0c	/* Left AUX2 Input Control */
#define SV_IREG_RIGHT_AUX2	0x0d	/* Right AUX2 Input Control */
#define SV_IREG_LEFT_ANALOG	0x0e	/* Left Analog Mixer Output Control */
#define SV_IREG_RIGHT_ANALOG	0x0f	/* Right Analog Mixer Output Control */
#define SV_IREG_LEFT_PCM	0x10	/* Left PCM Input Control */
#define SV_IREG_RIGHT_PCM	0x11	/* Right PCM Input Control */
#define SV_IREG_DMA_DATA_FMT	0x12	/* DMA Data Format */
#define SV_IREG_PC_ENABLE	0x13	/* Playback/Capture Enable Register */
#define SV_IREG_UD_BUTTON	0x14	/* Up/Down Button Register */
#define SV_IREG_REVISION	0x15	/* Revision */
#define SV_IREG_ADC_OUTPUT_CTRL	0x16	/* ADC Output Control */
#define SV_IREG_DMA_A_UPPER	0x18	/* DMA A Upper Base Count */
#define SV_IREG_DMA_A_LOWER	0x19	/* DMA A Lower Base Count */
#define SV_IREG_DMA_C_UPPER	0x1c	/* DMA C Upper Base Count */
#define SV_IREG_DMA_C_LOWER	0x1d	/* DMA C Lower Base Count */
#define SV_IREG_PCM_RATE_LOW	0x1e	/* PCM Sampling Rate Low Byte */
#define SV_IREG_PCM_RATE_HIGH	0x1f	/* PCM Sampling Rate High Byte */
#define SV_IREG_SYNTH_RATE_LOW	0x20	/* Synthesizer Sampling Rate Low Byte */
#define SV_IREG_SYNTH_RATE_HIGH 0x21	/* Synthesizer Sampling Rate High Byte */
#define SV_IREG_ADC_CLOCK	0x22	/* ADC Clock Source Selection */
#define SV_IREG_ADC_ALT_RATE	0x23	/* ADC Alternative Sampling Rate Selection */
#define SV_IREG_ADC_PLL_M	0x24	/* ADC PLL M Register */
#define SV_IREG_ADC_PLL_N	0x25	/* ADC PLL N Register */
#define SV_IREG_SYNTH_PLL_M	0x26	/* Synthesizer PLL M Register */
#define SV_IREG_SYNTH_PLL_N	0x27	/* Synthesizer PLL N Register */
#define SV_IREG_MPU401		0x2a	/* MPU-401 UART Operation */
#define SV_IREG_DRIVE_CTRL	0x2b	/* Drive Control */
#define SV_IREG_SRS_SPACE	0x2c	/* SRS Space Control */
#define SV_IREG_SRS_CENTER	0x2d	/* SRS Center Control */
#define SV_IREG_WAVE_SOURCE	0x2e	/* Wavetable Sample Source Select */
#define SV_IREG_ANALOG_POWER	0x30	/* Analog Power Down Control */
#define SV_IREG_DIGITAL_POWER	0x31	/* Digital Power Down Control */

#define SV_IREG_ADC_PLL		SV_IREG_ADC_PLL_M
#define SV_IREG_SYNTH_PLL	SV_IREG_SYNTH_PLL_M

/*
 *  DMA registers
 */

#define SV_DMA_ADDR0		0x00
#define SV_DMA_ADDR1		0x01
#define SV_DMA_ADDR2		0x02
#define SV_DMA_ADDR3		0x03
#define SV_DMA_COUNT0		0x04
#define SV_DMA_COUNT1		0x05
#define SV_DMA_COUNT2		0x06
#define SV_DMA_MODE		0x0b
#define SV_DMA_RESET		0x0d
#define SV_DMA_MASK		0x0f

/*
 *  Record sources
 */

#define SV_RECSRC_RESERVED	(0x00<<5)
#define SV_RECSRC_CD		(0x01<<5)
#define SV_RECSRC_DAC		(0x02<<5)
#define SV_RECSRC_AUX2		(0x03<<5)
#define SV_RECSRC_LINE		(0x04<<5)
#define SV_RECSRC_AUX1		(0x05<<5)
#define SV_RECSRC_MIC		(0x06<<5)
#define SV_RECSRC_OUT		(0x07<<5)

/*
 *  constants
 */

#define SV_FULLRATE		48000
#define SV_REFFREQUENCY		24576000
#define SV_ADCMULT		512

#define SV_MODE_PLAY		1
#define SV_MODE_CAPTURE		2

/*

 */

typedef struct snd_stru_sonicvibes sonicvibes_t;

struct snd_stru_sonicvibes {
	snd_dma_t * dma1ptr;
	snd_dma_t * dma2ptr;
	snd_irq_t * irqptr;

	unsigned int sb_port;
	unsigned int enh_port;
	unsigned int synth_port;
	unsigned int midi_port;
	unsigned int game_port;
	unsigned int dmaa_port;
	unsigned int dmac_port;

	unsigned char enable;
	unsigned char irqmask;
	unsigned char revision;
	unsigned char format;
	unsigned char srs_space;
	unsigned char srs_center;
	unsigned char mpu_switch;
	unsigned char wave_source;

	unsigned int mode;

	struct pci_dev *pci;
	snd_card_t *card;
	snd_pcm_t *pcm;
	snd_pcm_subchn_t *playback_subchn;
	snd_pcm_subchn_t *capture_subchn;
	snd_kmixer_t *mixer;
	snd_rawmidi_t *rmidi;
	snd_hwdep_t *fmsynth;	/* S3FM */

	spinlock_t reg_lock;
	snd_info_entry_t *proc_entry;

	unsigned int p_dma_size;
	unsigned int c_dma_size;

	snd_kswitch_t *switch_wavesource;
	snd_kswitch_t *switch_synth;
	snd_kswitch_t *switch_rxtosynth;
	snd_kswitch_t *switch_txtoext;

	snd_kmixer_element_t *me_mux_cd;
	snd_kmixer_element_t *me_mux_dac;
	snd_kmixer_element_t *me_mux_aux2;
	snd_kmixer_element_t *me_mux_line;
	snd_kmixer_element_t *me_mux_aux1;
	snd_kmixer_element_t *me_mux_mic;
	snd_kmixer_element_t *me_mux_out;

	snd_kmixer_element_t *me_accu;
	snd_kmixer_element_t *me_dig_accu;
	snd_kmixer_element_t *me_mux;
	snd_kmixer_element_t *me_vol_igain;
	snd_kmixer_element_t *me_vol_aux1;
	snd_kmixer_element_t *me_sw_aux1;
	snd_kmixer_element_t *me_vol_cd;
	snd_kmixer_element_t *me_sw_cd;
	snd_kmixer_element_t *me_vol_line;
	snd_kmixer_element_t *me_sw_line;
	snd_kmixer_element_t *me_vol_mic_boost;
	snd_kmixer_element_t *me_vol_mic;
	snd_kmixer_element_t *me_sw_mic;
	snd_kmixer_element_t *me_vol_synth;
	snd_kmixer_element_t *me_sw_synth;
	snd_kmixer_element_t *me_vol_aux2;
	snd_kmixer_element_t *me_sw_aux2;
	snd_kmixer_element_t *me_vol_master;
	snd_kmixer_element_t *me_sw_master;
	snd_kmixer_element_t *me_vol_pcm;
	snd_kmixer_element_t *me_sw_pcm;
	snd_kmixer_element_t *me_vol_loop;
	snd_kmixer_element_t *me_sw_loop;
	snd_kmixer_element_t *me_dac;
	snd_kmixer_element_t *me_adc;
	snd_kmixer_element_t *me_playback;
	snd_kmixer_element_t *me_capture;
};

int snd_sonicvibes_create(snd_card_t * card,
			  struct pci_dev *pci,
			  snd_dma_t * dma1ptr,
			  snd_dma_t * dma2ptr,
			  snd_irq_t * irqptr,
			  int reverb, int mge,
			  sonicvibes_t ** rsonic);
int snd_sonicvibes_free(sonicvibes_t * sonic);
void snd_sonicvibes_interrupt(sonicvibes_t * sonic);

int snd_sonicvibes_pcm(sonicvibes_t * sonic, int device, snd_pcm_t ** rpcm);
int snd_sonicvibes_mixer(sonicvibes_t * sonic, int device, snd_pcm_t * pcm, snd_kmixer_t ** rmixer);
void snd_sonicvibes_midi(sonicvibes_t * sonic, mpu401_t * mpu);

#endif				/* __SONICVIBES_H */
