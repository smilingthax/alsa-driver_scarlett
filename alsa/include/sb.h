#ifndef __SB_H
#define __SB_H

/*
 *  Header file for SoundBlaster cards
 *  Copyright (c) by Jaroslav Kysela <perex@jcu.cz>
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

#define SB_HW_AUTO		0
#define SB_HW_10		1
#define SB_HW_20		2
#define SB_HW_201		3
#define SB_HW_PRO		4
#define SB_HW_16		5

#define SB_MODE8_HALT		0
#define SB_MODE8_PLAYBACK	1
#define SB_MODE8_RECORD		2

#define SB_OPEN_PCM		1
#define SB_OPEN_MIDI_INPUT	2
#define SB_OPEN_MIDI_OUTPUT	4
#define SB_OPEN_MIDI_TRIGGER	8

#define SB_MODE16_PLAYBACK	1
#define SB_MODE16_RECORD	2
#define SB_MODE16_PLAYBACK16	4
#define SB_MODE16_RECORD16	8
#define SB_MODE16_RATE_LOCK_P	16
#define SB_MODE16_RATE_LOCK_R	32
#define SB_MODE16_RATE_LOCK	(SB_MODE16_RATE_LOCK_P|SB_MODE16_RATE_LOCK_R)
#define SB_MODE16_AUTO		64

#define SB_MPU_INPUT		1

typedef struct snd_stru_sbmixer sbmixer_t;

struct snd_stru_sbmixer {
  unsigned short port;
  unsigned char record_source;
  unsigned char left_input_mask;
  unsigned char right_input_mask;
  unsigned char mono;		/* for update inputs */
  void (*update_inputs)( sbmixer_t * );
  snd_spin_define( mixer );
};

struct snd_stru_sbdsp {
  unsigned short port;		/* base port of DSP chip */
  unsigned short mpu_port;	/* MPU port for SB DSP 4.0+ */
  unsigned short irq;		/* IRQ number of DSP chip */
  unsigned short irqnum;	/* IRQ number (index) */
  unsigned short dma8;		/* 8-bit DMA */
  unsigned short dma8num;	/* 8-bit DMA index */
  unsigned short dma16;		/* 16-bit DMA */
  unsigned short dma16num;	/* 16-bit DMA index */
  unsigned short version;	/* version of DSP chip */
  unsigned short hardware;	/* see to SB_HW_XXXX */

  unsigned int open8;		/* see to SB_OPEN_XXXX */
  unsigned int mode8;		/* current mode of stream */
  unsigned char speed8;		/* input speed */
  unsigned char fmt8;		/* format */
  unsigned int count8;		/* size of one block for SB 1.0 */

  unsigned int mode16;		/* current 16-bit mode of streams */
  unsigned int force_mode16;	/* force 16-bit mode of streams */

  sbmixer_t mixer;		/* mixer */

  char name[32];

  snd_card_t *card;
  snd_pcm_t *pcm;

  snd_spin_define( reg );
  snd_spin_define( open8 );
  snd_spin_define( open16 );
  snd_spin_define( midi_input );

  snd_info_entry_t *proc_entry;
};

typedef struct snd_stru_sbdsp sbdsp_t;
                        
/* I/O ports */

#define SBP( codec, x ) ( (codec) -> port + s_b_SB_##x )
#define SBP1( port, x ) ( (port) + s_b_SB_##x )

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

#define SB_DSP_RECORD_SOURCE    0x0c
#define SB_DSP_MIXS_NONE        0x00
#define SB_DSP_MIXS_MIC         0x01
#define SB_DSP_MIXS_CD          0x03
#define SB_DSP_MIXS_LINE        0x07

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

/*
 *
 */

extern int snd_sbdsp_command( sbdsp_t *codec, unsigned char val );
extern int snd_sbdsp_get_byte( sbdsp_t *codec );
extern void snd_sbmixer_write( sbmixer_t *mixer, unsigned char reg, unsigned char data );
extern unsigned char snd_sbmixer_read( sbmixer_t *mixer, unsigned char reg );
extern int snd_sbdsp_reset( sbdsp_t *codec );
extern void snd_sbdsp_free( void * );

extern void snd_sbdsp_sb8_interrupt( snd_pcm_t *pcm );
extern void snd_sbdsp_sb16_interrupt( snd_pcm_t *pcm, unsigned short status );

extern snd_pcm_t *snd_sbdsp_new_device( snd_card_t *card,
                                        unsigned short port,
                                        unsigned short irqnum,
                                        unsigned short dma8num,
                                        unsigned short dma16num,
                                        unsigned short hardware );
extern int snd_sbdsp_probe( snd_pcm_t *pcm );
extern int snd_sbdsp_sb16_configure( snd_pcm_t *pcm );
extern snd_kmixer_t *snd_sbdsp_new_mixer( snd_card_t *card,
                                          sbmixer_t *sbmix,
                                          unsigned short hardware );

extern void snd_sb16_proc_init( snd_pcm_t *pcm );
extern void snd_sb16_proc_done( snd_pcm_t *pcm ); 

extern void snd_sbdsp_midi_interrupt( snd_rawmidi_t *rmidi );
extern snd_rawmidi_t *snd_sbdsp_midi_new_device( snd_card_t *card, snd_pcm_t *pcm );

#endif /* __SB_H */
