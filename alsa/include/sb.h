#ifndef __SB_H
#define __SB_H

/*
 *  Header file for SoundBlaster cards
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>
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

#define SB_HW_AUTO		0
#define SB_HW_10		1
#define SB_HW_20		2
#define SB_HW_201		3
#define SB_HW_PRO		4
#define SB_HW_16		5
#define SB_HW_16CSP		6	/* SB16 with CSP chip */
#define SB_HW_ALS100		7	/* Avance Logic ALS100 chip */

#define SB_MODE8_HALT		0
#define SB_MODE8_PLAYBACK	1
#define SB_MODE8_CAPTURE	2

#define SB_OPEN_PCM		1
#define SB_OPEN_MIDI_INPUT	2
#define SB_OPEN_MIDI_OUTPUT	4
#define SB_OPEN_MIDI_TRIGGER	8

#define SB_MODE16_PLAYBACK	1
#define SB_MODE16_CAPTURE	2
#define SB_MODE16_PLAYBACK16	4
#define SB_MODE16_CAPTURE16	8
#define SB_MODE16_RATE_LOCK_P	16
#define SB_MODE16_RATE_LOCK_R	32
#define SB_MODE16_RATE_LOCK	(SB_MODE16_RATE_LOCK_P|SB_MODE16_RATE_LOCK_R)
#define SB_MODE16_AUTO		64

#define SB_MPU_INPUT		1

typedef struct snd_stru_sbmixer sbmixer_t;

struct snd_stru_sbmixer {
	unsigned long port;

	snd_kmixer_element_t *me_mux_mic;
	snd_kmixer_element_t *me_mux_line;
	snd_kmixer_element_t *me_mux_cd;

	snd_kmixer_element_t *me_mux;
	snd_kmixer_element_t *me_in_accu;
	snd_kmixer_element_t *me_out_accu;
	snd_kmixer_element_t *me_playback;
	snd_kmixer_element_t *me_capture;

	snd_kmixer_element_t *me_vol_igain;

	snd_kmixer_element_t *me_in_speaker;
	snd_kmixer_element_t *me_vol_speaker;

	snd_kmixer_element_t *me_in_mic;
	snd_kmixer_element_t *me_vol_mic;
	snd_kmixer_element_t *me_sw1_mic_output;
	snd_kmixer_element_t *me_sw1_mic_input;

	snd_kmixer_element_t *me_in_line;
	snd_kmixer_element_t *me_vol_line;
	snd_kmixer_element_t *me_sw1_line_output;
	snd_kmixer_element_t *me_sw3_line_input;

	snd_kmixer_element_t *me_in_cd;
	snd_kmixer_element_t *me_vol_cd;
	snd_kmixer_element_t *me_sw1_cd_output;
	snd_kmixer_element_t *me_sw3_cd_input;
	
	snd_kmixer_element_t *me_in_synth;
	snd_kmixer_element_t *me_vol_synth;
	snd_kmixer_element_t *me_sw3_synth_input;

	snd_kmixer_element_t *me_vol_pcm;

	snd_kmixer_element_t *me_vol_ogain;

	snd_kmixer_element_t *me_tone;

	snd_kmixer_element_t *me_out_master;
	snd_kmixer_element_t *me_sw1_3dse;
	snd_kmixer_element_t *me_vol_master;
	
	spinlock_t lock;
};

struct snd_stru_sbdsp {
	unsigned long port;		/* base port of DSP chip */
	unsigned long mpu_port;		/* MPU port for SB DSP 4.0+ */
	unsigned int irq;		/* IRQ number of DSP chip */
	snd_irq_t * irqptr;		/* IRQ pointer */
	unsigned short dma8;		/* 8-bit DMA */
	snd_dma_t * dma8ptr;		/* 8-bit DMA pointer */
	unsigned short dma16;		/* 16-bit DMA */
	snd_dma_t * dma16ptr;		/* 16-bit DMA pointer */
	unsigned short version;		/* version of DSP chip */
	unsigned short hardware;	/* see to SB_HW_XXXX */

	unsigned int open8;		/* see to SB_OPEN_XXXX */
	unsigned int mode8;		/* current mode of stream */
	unsigned char speed8;		/* input speed */
	unsigned char fmt8;		/* format */
	struct timer_list midi_timer;
	unsigned int p_dma_size;
	unsigned int p_frag_size;
	unsigned int c_dma_size;
	unsigned int c_frag_size;

	unsigned int mode16;		/* current 16-bit mode of streams */
	unsigned int force_mode16;	/* force 16-bit mode of streams */

	sbmixer_t mixer;		/* mixer */

	char name[32];

#ifdef CONFIG_SND_SB16_CSP
	void *csp_callbacks;
	void *csp_private_data;
	unsigned int csp_acquired;
#endif

	snd_card_t *card;
	snd_pcm_t *pcm;
	snd_pcm_substream_t *playback_substream;
	snd_pcm_substream_t *capture_substream;
	snd_kmixer_t *kmixer;

	spinlock_t reg_lock;
	spinlock_t open8_lock;
	spinlock_t open16_lock;
	spinlock_t midi_input_lock;

	snd_info_entry_t *proc_entry;
};

typedef struct snd_stru_sbdsp sbdsp_t;

/* I/O ports */

#define SBP(codec, x)		((codec)->port + s_b_SB_##x)
#define SBP1(port, x)		((port) + s_b_SB_##x)

#define s_b_SB_RESET		0x6
#define s_b_SB_READ		0xa
#define s_b_SB_WRITE		0xc
#define s_b_SB_COMMAND		0xc
#define s_b_SB_STATUS		0xc
#define s_b_SB_DATA_AVAIL	0xe
#define s_b_SB_DATA_AVAIL_16 	0xf
#define s_b_SB_MIXER_ADDR	0x4
#define s_b_SB_MIXER_DATA	0x5
#define s_b_SB_OPL3_LEFT	0x0
#define s_b_SB_OPL3_RIGHT	0x2
#define s_b_SB_OPL3_BOTH	0x8

#define SB_DSP_OUTPUT		0x14
#define SB_DSP_INPUT		0x24
#define SB_DSP_BLOCK_SIZE	0x48
#define SB_DSP_HI_OUTPUT	0x91
#define SB_DSP_HI_INPUT		0x99
#define SB_DSP_LO_OUTPUT_AUTO	0x1c
#define SB_DSP_LO_INPUT_AUTO	0x2c
#define SB_DSP_HI_OUTPUT_AUTO	0x90
#define SB_DSP_HI_INPUT_AUTO	0x98
#define SB_DSP_IMMED_INT	0xf2
#define SB_DSP_GET_VERSION	0xe1
#define SB_DSP_SPEAKER_ON	0xd1
#define SB_DSP_SPEAKER_OFF	0xd3
#define SB_DSP_DMA8_OFF		0xd0
#define SB_DSP_DMA8_ON		0xd4
#define SB_DSP_DMA8_EXIT	0xda
#define SB_DSP_DMA16_OFF	0xd5
#define SB_DSP_DMA16_ON		0xd6
#define SB_DSP_DMA16_EXIT	0xd9
#define SB_DSP_SAMPLE_RATE	0x40
#define SB_DSP_SAMPLE_RATE_OUT	0x41
#define SB_DSP_SAMPLE_RATE_IN	0x42
#define SB_DSP_MONO_8BIT	0xa0
#define SB_DSP_MONO_16BIT	0xa4
#define SB_DSP_STEREO_8BIT	0xa8
#define SB_DSP_STEREO_16BIT	0xac

#define SB_DSP_MIDI_INPUT_IRQ	0x31
#define SB_DSP_MIDI_OUTPUT	0x38

#define SB_DSP4_OUT8_AI		0xc6
#define SB_DSP4_IN8_AI		0xce
#define SB_DSP4_OUT16_AI	0xb6
#define SB_DSP4_IN16_AI		0xbe
#define SB_DSP4_MODE_UNS_MONO	0x00
#define SB_DSP4_MODE_SIGN_MONO	0x10
#define SB_DSP4_MODE_UNS_STEREO	0x20
#define SB_DSP4_MODE_SIGN_STEREO 0x30

#define SB_DSP4_OUTPUT		0x3c
#define SB_DSP4_INPUT_LEFT	0x3d
#define SB_DSP4_INPUT_RIGHT	0x3e

#define SB_DSP_ESS_GET_VERSION	0xe7
#define SB_DSP_ESS_EXTENDED	0xc6

#define SB_DSP_CAPTURE_SOURCE	0x0c
#define SB_DSP_MIXS_MIC0	0x00	/* same as MIC */
#define SB_DSP_MIXS_MIC		0x01
#define SB_DSP_MIXS_CD		0x03
#define SB_DSP_MIXS_LINE	0x07

/* registers for SB PRO mixer */
#define SB_DSP_MASTER_DEV	0x22
#define SB_DSP_PCM_DEV		0x04
#define SB_DSP_LINE_DEV		0x2e
#define SB_DSP_CD_DEV		0x28
#define SB_DSP_FM_DEV		0x26
#define SB_DSP_MIC_DEV		0x0a

/* registers (only for left channel) for SB 16 mixer */
#define SB_DSP4_MASTER_DEV	0x30
#define SB_DSP4_BASS_DEV	0x46
#define SB_DSP4_TREBLE_DEV	0x44
#define SB_DSP4_SYNTH_DEV	0x34
#define SB_DSP4_PCM_DEV		0x32
#define SB_DSP4_SPEAKER_DEV	0x3b
#define SB_DSP4_LINE_DEV	0x38
#define SB_DSP4_MIC_DEV		0x3a
#define SB_DSP4_OUTPUT_SW	0x3c
#define SB_DSP4_CD_DEV		0x36
#define SB_DSP4_IGAIN_DEV	0x3f
#define SB_DSP4_OGAIN_DEV	0x41
#define SB_DSP4_MIC_AGC		0x43

/* additional registers for SB 16 mixer */
#define SB_DSP4_IRQSETUP	0x80
#define SB_DSP4_DMASETUP	0x81
#define SB_DSP4_IRQSTATUS	0x82
#define SB_DSP4_MPUSETUP	0x84

#define SB_DSP4_3DSE		0x90

/* IRQ setting bitmap */
#define SB_IRQSETUP_IRQ9	0x01
#define SB_IRQSETUP_IRQ5	0x02
#define SB_IRQSETUP_IRQ7	0x04
#define SB_IRQSETUP_IRQ10	0x08

/* IRQ types */
#define SB_IRQTYPE_8BIT		0x01
#define SB_IRQTYPE_16BIT	0x02
#define SB_IRQTYPE_MPUIN	0x04

/* DMA setting bitmap */
#define SB_DMASETUP_DMA0	0x01
#define SB_DMASETUP_DMA1	0x02
#define SB_DMASETUP_DMA3	0x08
#define SB_DMASETUP_DMA5	0x20
#define SB_DMASETUP_DMA6	0x40
#define SB_DMASETUP_DMA7	0x80

/*
 *
 */

extern int snd_sb8dsp_command(sbdsp_t * codec, unsigned char val);
extern int snd_sb16dsp_command(sbdsp_t * codec, unsigned char val);
extern int snd_sb8dsp_get_byte(sbdsp_t * codec);
extern int snd_sb16dsp_get_byte(sbdsp_t * codec);
extern void snd_sb8mixer_write(sbmixer_t * mixer, unsigned char reg, unsigned char data);
extern void snd_sb16mixer_write(sbmixer_t * mixer, unsigned char reg, unsigned char data);
extern unsigned char snd_sb8mixer_read(sbmixer_t * mixer, unsigned char reg);
extern unsigned char snd_sb16mixer_read(sbmixer_t * mixer, unsigned char reg);
extern int snd_sb8dsp_reset(sbdsp_t * codec);
extern int snd_sb16dsp_reset(sbdsp_t * codec);
extern void snd_sb8dsp_free(void *);
extern void snd_sb16dsp_free(void *);

extern void snd_sb8dsp_interrupt(snd_pcm_t * pcm);
extern void snd_sb16dsp_interrupt(snd_pcm_t * pcm, unsigned short status);

extern int snd_sb8dsp_new_pcm(snd_card_t * card,
			      int device,
			      unsigned long port,
			      snd_irq_t * irqptr,
			      snd_dma_t * dma8ptr,
			      unsigned short hardware,
			      snd_pcm_t ** rpcm);
extern int snd_sb16dsp_new_pcm(snd_card_t * card,
			       int device,
			       unsigned long port,
			       snd_irq_t * irqptr,
			       snd_dma_t * dma8ptr,
			       snd_dma_t * dma16ptr,
			       unsigned short hardware,
			       snd_pcm_t ** rpcm);
extern int snd_sb8dsp_probe(snd_pcm_t * pcm);
extern int snd_sb16dsp_probe(snd_pcm_t * pcm);
extern int snd_sb16dsp_configure(snd_pcm_t * pcm);
extern int snd_sb8dsp_new_mixer(sbdsp_t * codec,
			        int device,
				snd_pcm_t * pcm,
				snd_kmixer_t ** rmixer);
extern int snd_sb16dsp_new_mixer(sbdsp_t * codec,
				 int device,
				 snd_pcm_t * pcm,
				 snd_kmixer_t ** rmixer);

extern int snd_sb8_playback_open(void *private_data, snd_pcm_substream_t *substream);
extern int snd_sb8_capture_open(void *private_data, snd_pcm_substream_t *substream);
extern int snd_sb8_playback_close(void *private_data, snd_pcm_substream_t *substream);
extern int snd_sb8_capture_close(void *private_data, snd_pcm_substream_t *substream);
extern int snd_sb16_playback_open(void *private_data, snd_pcm_substream_t *substream);
extern int snd_sb16_capture_open(void *private_data, snd_pcm_substream_t *substream);
extern int snd_sb16_playback_close(void *private_data, snd_pcm_substream_t *substream);
extern int snd_sb16_capture_close(void *private_data, snd_pcm_substream_t *substream);

extern void snd_sb16dsp_proc_init(snd_pcm_t * pcm);
extern void snd_sb16dsp_proc_done(snd_pcm_t * pcm);

extern void snd_sb8dsp_midi_interrupt(snd_rawmidi_t * rmidi);
extern int snd_sb8dsp_midi_new(sbdsp_t * codec, int device, snd_rawmidi_t ** rrawmidi);

#endif				/* __SB_H */
