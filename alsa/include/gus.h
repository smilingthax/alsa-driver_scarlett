#ifndef __GUS_H
#define __GUS_H

/*
 *  Global structures used for GUS part of ALSA driver
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
#include "synth.h"
#include "timer.h"

/* IO ports */

#define GUSP( gus, x )			((gus) -> gf1.port + SND_g_u_s_##x)

#define SND_g_u_s_MIDICTRL		(0x320-0x220)
#define SND_g_u_s_MIDISTAT		(0x320-0x220)
#define SND_g_u_s_MIDIDATA		(0x321-0x220)

#define SND_g_u_s_GF1PAGE		(0x322-0x220)
#define SND_g_u_s_GF1REGSEL		(0x323-0x220)
#define SND_g_u_s_GF1DATALOW		(0x324-0x220)
#define SND_g_u_s_GF1DATAHIGH		(0x325-0x220)
#define SND_g_u_s_IRQSTAT		(0x226-0x220)
#define SND_g_u_s_TIMERCNTRL		(0x228-0x220)
#define SND_g_u_s_TIMERDATA		(0x229-0x220)
#define SND_g_u_s_DRAM			(0x327-0x220)
#define SND_g_u_s_MIXCNTRLREG		(0x220-0x220)
#define SND_g_u_s_IRQDMACNTRLREG	(0x22b-0x220)
#define SND_g_u_s_REGCNTRLS		(0x22f-0x220)
#define SND_g_u_s_BOARDVERSION		(0x726-0x220)
#define SND_g_u_s_MIXCNTRLPORT		(0x726-0x220)
#define SND_g_u_s_IVER			(0x325-0x220)
#define SND_g_u_s_MIXDATAPORT		(0x326-0x220)
#define SND_g_u_s_MAXCNTRLPORT		(0x326-0x220)

/* GF1 registers */

/* global registers */
#define SND_GF1_GB_ACTIVE_VOICES		0x0e
#define SND_GF1_GB_VOICES_IRQ			0x0f
#define SND_GF1_GB_GLOBAL_MODE			0x19
#define SND_GF1_GW_LFO_BASE			0x1a
#define SND_GF1_GB_VOICES_IRQ_READ		0x1f
#define SND_GF1_GB_DRAM_DMA_CONTROL		0x41
#define SND_GF1_GW_DRAM_DMA_LOW			0x42
#define SND_GF1_GW_DRAM_IO_LOW			0x43
#define SND_GF1_GB_DRAM_IO_HIGH			0x44
#define SND_GF1_GB_SOUND_BLASTER_CONTROL	0x45
#define SND_GF1_GB_ADLIB_TIMER_1		0x46
#define SND_GF1_GB_ADLIB_TIMER_2		0x47
#define SND_GF1_GB_RECORD_RATE			0x48
#define SND_GF1_GB_REC_DMA_CONTROL		0x49
#define SND_GF1_GB_JOYSTICK_DAC_LEVEL		0x4b
#define SND_GF1_GB_RESET			0x4c
#define SND_GF1_GB_DRAM_DMA_HIGH		0x50
#define SND_GF1_GW_DRAM_IO16			0x51
#define SND_GF1_GW_MEMORY_CONFIG		0x52
#define SND_GF1_GB_MEMORY_CONTROL		0x53
#define SND_GF1_GW_FIFO_RECORD_BASE_ADDR	0x54
#define SND_GF1_GW_FIFO_PLAY_BASE_ADDR		0x55
#define SND_GF1_GW_FIFO_SIZE			0x56
#define SND_GF1_GW_INTERLEAVE			0x57
#define SND_GF1_GB_COMPATIBILITY		0x59
#define SND_GF1_GB_DECODE_CONTROL		0x5a
#define SND_GF1_GB_VERSION_NUMBER		0x5b
#define SND_GF1_GB_MPU401_CONTROL_A		0x5c
#define SND_GF1_GB_MPU401_CONTROL_B		0x5d
#define SND_GF1_GB_EMULATION_IRQ		0x60
/* voice specific registers */
#define SND_GF1_VB_ADDRESS_CONTROL		0x00
#define SND_GF1_VW_FREQUENCY			0x01
#define SND_GF1_VW_START_HIGH			0x02
#define SND_GF1_VW_START_LOW			0x03
#define SND_GF1_VA_START			SND_GF1_VW_START_HIGH
#define SND_GF1_VW_END_HIGH			0x04
#define SND_GF1_VW_END_LOW			0x05
#define SND_GF1_VA_END				SND_GF1_VW_END_HIGH
#define SND_GF1_VB_VOLUME_RATE			0x06
#define SND_GF1_VB_VOLUME_START			0x07
#define SND_GF1_VB_VOLUME_END			0x08
#define SND_GF1_VW_VOLUME			0x09
#define SND_GF1_VW_CURRENT_HIGH			0x0a
#define SND_GF1_VW_CURRENT_LOW			0x0b
#define SND_GF1_VA_CURRENT			SND_GF1_VW_CURRENT_HIGH
#define SND_GF1_VB_PAN				0x0c
#define SND_GF1_VW_OFFSET_RIGHT			0x0c
#define SND_GF1_VB_VOLUME_CONTROL		0x0d
#define SND_GF1_VB_UPPER_ADDRESS		0x10
#define SND_GF1_VW_EFFECT_HIGH			0x11
#define SND_GF1_VW_EFFECT_LOW			0x12
#define SND_GF1_VA_EFFECT			SND_GF1_VW_EFFECT_HIGH
#define SND_GF1_VW_OFFSET_LEFT			0x13
#define SND_GF1_VB_ACCUMULATOR			0x14
#define SND_GF1_VB_MODE				0x15
#define SND_GF1_VW_EFFECT_VOLUME		0x16
#define SND_GF1_VB_FREQUENCY_LFO		0x17
#define SND_GF1_VB_VOLUME_LFO			0x18
#define SND_GF1_VW_OFFSET_RIGHT_FINAL		0x1b
#define SND_GF1_VW_OFFSET_LEFT_FINAL		0x1c
#define SND_GF1_VW_EFFECT_VOLUME_FINAL		0x1d

/* ICS registers */

#define SND_ICS_MIC_DEV		0
#define SND_ICS_LINE_DEV	1
#define SND_ICS_CD_DEV		2
#define SND_ICS_GF1_DEV		3
#define SND_ICS_NONE_DEV	4
#define SND_ICS_MASTER_DEV	5

/* LFO */

#define SND_LFO_TREMOLO		0
#define SND_LFO_VIBRATO		1

/* misc */

#define SND_GF1_DMA_UNSIGNED	0x80
#define SND_GF1_DMA_16BIT	0x40
#define SND_GF1_DMA_IRQ		0x20
#define SND_GF1_DMA_WIDTH16	0x04
#define SND_GF1_DMA_READ	0x02	/* read from GUS's DRAM */
#define SND_GF1_DMA_ENABLE	0x01

/* ramp ranges */

#define SND_GF1_ATTEN( x )	(snd_gf1_atten_table[ x ])
#define SND_GF1_VOLUME( x )	(snd_gf1_atten_table[ x ] << 1)
#define SND_GF1_MIN_VOLUME	1800
#define SND_GF1_MAX_VOLUME	4095
#define SND_GF1_MIN_OFFSET	(SND_GF1_MIN_VOLUME>>4)
#define SND_GF1_MAX_OFFSET	255
#define SND_GF1_MAX_TDEPTH	90

/* defines for memory manager */

#define SND_GF1_MEM_BLOCK_16BIT		0x0001

#define SND_GF1_MEM_OWNER_DRIVER	0x0001
#define SND_GF1_MEM_OWNER_WAVE		0x0002
#define SND_GF1_MEM_OWNER_USER		0x0003
#define SND_GF1_MEM_OWNER_SERVER	0x0004

/* modes */

#define SND_GF1_MODE_ENGINE		0x0001
#define SND_GF1_MODE_TIMER		0x0002
#define SND_GF1_MODE_PCM_PLAY		0x0004
#define SND_GF1_MODE_PCM_RECORD		0x0008
#define SND_GF1_MODE_PCM		0x000c

/* indexs for voice_ranges array */

#define SND_GF1_VOICE_RANGES		3
#define SND_GF1_VOICE_RANGE_SYNTH	0
#define SND_GF1_VOICE_RANGE_PCM		1
#define SND_GF1_VOICE_RANGE_EFFECT	2

/* constants for interrupt handlers */

#define SND_GF1_HANDLER_MIDI_OUT	0x00010000
#define SND_GF1_HANDLER_MIDI_IN		0x00020000
#define SND_GF1_HANDLER_TIMER1		0x00040000
#define SND_GF1_HANDLER_TIMER2		0x00080000
#define SND_GF1_HANDLER_RANGE		0x00100000
#define SND_GF1_HANDLER_DMA_WRITE	0x00200000
#define SND_GF1_HANDLER_DMA_READ	0x00400000
#define SND_GF1_HANDLER_ALL		(0xffff0000&~SND_GF1_HANDLER_RANGE)

/* constants for DMA flags */

#define SND_GF1_DMA_TRIGGER		1
#define SND_GF1_DMA_SLEEP		2

/* --- */

struct snd_stru_gus_card;
typedef struct snd_stru_gus_card snd_gus_card_t;

/* GF1 specific structure */

typedef struct snd_stru_gf1_bank_info {
  unsigned int address;
  unsigned int size;
} snd_gf1_bank_info_t;

typedef struct snd_stru_gf1_mem_block {
  unsigned short flags;			/* flags - SND_GF1_MEM_BLOCK_XXXX */
  unsigned short owner;			/* owner - SND_GF1_MEM_OWNER_XXXX */
  unsigned short lock;			/* locked by - SND_GF1_MEM_OWNER_XXXX */
  unsigned int share;			/* share count */
  unsigned int share_id1, share_id2;	/* share ID */
  unsigned int ptr;
  unsigned int size;
  union {
    char *name;
    void *data;
  } data;
  struct snd_stru_gf1_mem_block *next;
  struct snd_stru_gf1_mem_block *prev;
} snd_gf1_mem_block_t;

typedef struct snd_stru_gf1_mem {
  snd_gf1_bank_info_t banks_8 [ 4 ];
  snd_gf1_bank_info_t banks_16[ 4 ];
  snd_gf1_mem_block_t *first;
  snd_gf1_mem_block_t *last;
  snd_info_entry_t *info_entry;
  snd_mutex_define( memory );
  snd_mutex_define( memory_find );
} snd_gf1_mem_t;

typedef struct snd_gf1_dma_block {
  void *buffer;			/* buffer in computer's RAM */
  unsigned int addr;		/* address in onboard memory */
  unsigned int count;		/* count in bytes */
  unsigned int cmd;		/* DMA command (format) */
} snd_gf1_dma_block_t;

struct snd_stru_gf1 {

  unsigned int enh_mode: 1,	/* enhanced mode (GFA1) */
  	       hw_lfo: 1,	/* use hardware LFO */
  	       sw_lfo: 1;	/* use software LFO */
  
  unsigned short port;		/* port of GF1 chip */
  unsigned short irq;		/* IRQ number of GF1 chip */
  unsigned short irqnum;	/* IRQ index */
  unsigned short dma1;		/* DMA1 number */
  unsigned short dma1num;	/* DMA1 index */
  unsigned short dma2;		/* DMA2 number */
  unsigned short dma2num;	/* DMA2 number */
  unsigned int memory;		/* GUS's DRAM size in bytes */
  unsigned int rom_memory;	/* GUS's ROM size in bytes */
  unsigned int rom_present;	/* bitmask */
  unsigned int rom_banks;	/* GUS's ROM banks */

  snd_gf1_mem_t mem_alloc;

  /* registers */
  unsigned short reg_page;
  unsigned short reg_regsel;
  unsigned short reg_data8;
  unsigned short reg_data16;
  unsigned short reg_irqstat;
  unsigned short reg_dram;
  unsigned short reg_timerctrl;
  unsigned short reg_timerdata;
  /* --------- */

  unsigned char active_voice;	/* selected voice (GF1PAGE register) */
  unsigned char active_voices;	/* all active voices */

  unsigned int default_voice_address;

  unsigned short playback_freq;	/* GF1 playback (mixing) frequency */
  unsigned short mode;		/* see to SND_GF1_MODE_XXXX */

  unsigned char *lfos;

  /* special interrupt handlers / callbacks */
  
  struct SND_STRU_GF1_VOICE_RANGE {
    unsigned short rvoices;     /* requested voices */
    unsigned short voices;      /* total voices for this range */
    unsigned short min, max;    /* minimal & maximal voice */
#ifdef SNDCFG_INTERRUPTS_PROFILE
    unsigned int interrupt_stat_wave;
    unsigned int interrupt_stat_volume;
#endif
    void (*interrupt_handler_wave)( snd_gus_card_t *gus, int voice );
    void (*interrupt_handler_volume)( snd_gus_card_t *gus, int voice );
    void (*voices_change_start)( snd_gus_card_t *gus );
    void (*voices_change_stop)( snd_gus_card_t *gus );
    void (*volume_change)( snd_gus_card_t *gus );
  } voice_ranges[ SND_GF1_VOICE_RANGES ];

  /* interrupt handlers */
  
  void (*interrupt_handler_midi_out)( snd_gus_card_t *gus );
  void (*interrupt_handler_midi_in)( snd_gus_card_t *gus );
  void (*interrupt_handler_timer1)( snd_gus_card_t *gus );
  void (*interrupt_handler_timer2)( snd_gus_card_t *gus );
  void (*interrupt_handler_dma_write)( snd_gus_card_t *gus );
  void (*interrupt_handler_dma_read)( snd_gus_card_t *gus );

  /* synthesizer */
  
  snd_synth_t *synth;

  /* timer */
  
  unsigned short timer_enabled;
  snd_timer_t *timer1;
  snd_timer_t *timer2;

  /* midi */
  
  unsigned short uart_cmd;
  unsigned int uart_framing;
  unsigned int uart_overrun;

  /* dma operations */

  unsigned int dma_flags;
  unsigned int dma_shared;
  snd_gf1_dma_block_t *dma_data;
  unsigned int dma_blocks;
  unsigned int dma_used;
  unsigned int dma_head;
  unsigned int dma_tail;

  /* pcm */
  
  unsigned char *pcm_buffer;
  unsigned int pcm_memory;
  unsigned short pcm_pflags;
  unsigned int pcm_pos;
  unsigned char pcm_voice_ctrl, pcm_ramp_ctrl;
  unsigned int pcm_bpos;
  unsigned int pcm_blocks;
  unsigned int pcm_block_size;
  unsigned int pcm_mmap_mask;
  unsigned short pcm_volume_level_left;
  unsigned short pcm_volume_level_right;
  unsigned char pcm_volume[32];
  unsigned char pcm_pan[32];
  unsigned char pcm_rcntrl_reg;
};

/* main structure for GUS card */

struct snd_stru_gus_card {
  snd_card_t *card;

  unsigned int equal_irq:1,	/* GF1 and CODEC shares IRQ (GUS MAX only) */
  	       equal_dma:1,	/* if dma channels are equal (not valid for daughter board) */
  	       ics_flag:1,	/* have we ICS mixer chip */
  	       ics_flipped:1,	/* ICS mixer have flipped some channels? */
  	       codec_flag:1,	/* have we CODEC chip? */
  	       max_flag:1,	/* have we GUS MAX card? */
  	       max_ctrl_flag:1,	/* have we original GUS MAX card? */
  	       daughter_flag:1, /* have we daughter board? */
  	       interwave:1,	/* hey - we have InterWave card */
  	       ess_flag:1,	/* ESS chip found... GUS Extreme */
  	       uart_enable:1;	/* enable MIDI UART */
  unsigned short revision;	/* revision of chip */
  unsigned short max_cntrl_val;	/* GUS MAX control value */
  unsigned short mix_cntrl_reg;	/* mixer control register */
  unsigned short joystick_dac;	/* joystick DAC level */

  struct snd_stru_gf1 gf1;	/* gf1 specific variables */
  snd_pcm_t *pcm;
  snd_rawmidi_t *midi_uart;

  snd_spin_define( reg );
  snd_spin_define( active_voice );
  snd_spin_define( playback );
  snd_spin_define( dma );
  snd_spin_define( pcm_volume_level );
  snd_spin_define( uart_cmd );
  snd_spin_define( neutral );
  snd_mutex_define( dma );
  snd_sleep_define( neutral );
};

/* I/O functions for GF1/InterWave chip - gus_io.c */

static inline void snd_gf1_select_voice( snd_gus_card_t *gus, int voice )
{
  unsigned long flags;
  
  snd_spin_lock( gus, active_voice, &flags );
  if ( voice != gus -> gf1.active_voice ) {
    gus -> gf1.active_voice = voice;
    outb( voice, GUSP( gus, GF1PAGE ) );
  }
  snd_spin_unlock( gus, active_voice, &flags );
}

static inline void snd_gf1_uart_cmd( snd_gus_card_t *gus, unsigned char b )
{
  outb( gus -> gf1.uart_cmd = b, GUSP( gus, MIDICTRL ) );
}
  
static inline unsigned char snd_gf1_uart_stat( snd_gus_card_t *gus )
{
  return inb( GUSP( gus, MIDISTAT ) );
}
    
static inline void snd_gf1_uart_put( snd_gus_card_t *gus, unsigned char b )
{
  outb( b, GUSP( gus, MIDIDATA ) );
}
      
static inline unsigned char snd_gf1_uart_get( snd_gus_card_t *gus )
{
  return inb( GUSP( gus, MIDIDATA ) );
}        

extern void snd_gf1_delay( snd_gus_card_t *gus );

extern void snd_gf1_ctrl_stop( snd_gus_card_t *gus, unsigned char reg );

extern void snd_gf1_write8( snd_gus_card_t *gus, unsigned char reg, unsigned char data );
extern unsigned char snd_gf1_look8( snd_gus_card_t *gus, unsigned char reg );
extern inline unsigned char snd_gf1_read8( snd_gus_card_t *gus, unsigned char reg ) { return snd_gf1_look8( gus, reg | 0x80 ); }
extern void snd_gf1_write16( snd_gus_card_t *gus, unsigned char reg, unsigned int data );
extern unsigned short snd_gf1_look16( snd_gus_card_t *gus, unsigned char reg );
extern inline unsigned short snd_gf1_read16( snd_gus_card_t *gus, unsigned char reg ) { return snd_gf1_look16( gus, reg | 0x80 ); }
extern void snd_gf1_adlib_write( snd_gus_card_t *gus, unsigned char reg, unsigned char data );
extern void snd_gf1_dram_addr( snd_gus_card_t *gus, unsigned int addr );
extern void snd_gf1_poke( snd_gus_card_t *gus, unsigned int addr, unsigned char data );
extern unsigned char snd_gf1_peek( snd_gus_card_t *gus, unsigned int addr );
extern void snd_gf1_pokew( snd_gus_card_t *gus, unsigned int addr, unsigned short data );
extern unsigned short snd_gf1_peekw( snd_gus_card_t *gus, unsigned int addr );
extern void snd_gf1_dram_setmem( snd_gus_card_t *gus, unsigned int addr, unsigned short value, unsigned int count );
extern void snd_gf1_write_addr( snd_gus_card_t *gus, unsigned char reg, unsigned int addr, short w_16bit );
extern unsigned int snd_gf1_read_addr( snd_gus_card_t *gus, unsigned char reg, short w_16bit );
extern void snd_gf1_i_ctrl_stop( snd_gus_card_t *gus, unsigned char reg );
extern void snd_gf1_i_write8( snd_gus_card_t *gus, unsigned char reg, unsigned char data );
extern unsigned char snd_gf1_i_look8( snd_gus_card_t *gus, unsigned char reg );
extern void snd_gf1_i_write16( snd_gus_card_t *gus, unsigned char reg, unsigned int data );
extern inline unsigned char snd_gf1_i_read8( snd_gus_card_t *gus, unsigned char reg ) { return snd_gf1_i_look8( gus, reg | 0x80 ); }
extern unsigned short snd_gf1_i_look16( snd_gus_card_t *gus, unsigned char reg );
extern inline unsigned short snd_gf1_i_read16( snd_gus_card_t *gus, unsigned char reg ) { return snd_gf1_i_look16( gus, reg | 0x80 ); }
extern void snd_gf1_i_adlib_write( snd_gus_card_t *gus, unsigned char reg, unsigned char data );
extern void snd_gf1_i_write_addr( snd_gus_card_t *gus, unsigned char reg, unsigned int addr, short w_16bit );
extern unsigned int snd_gf1_i_read_addr( snd_gus_card_t *gus, unsigned char reg, short w_16bit );

extern void snd_gf1_reselect_active_voices( snd_gus_card_t *gus );

/* gus_lfo.c */

struct SND_STRU_IW_LFO_PROGRAM {
  unsigned short freq_and_control;
  unsigned char depth_final;
  unsigned char depth_inc;
  unsigned short twave;
  unsigned short depth;           
};

#if 0
extern void snd_gf1_lfo_effect_interrupt( snd_gus_card_t *gus, snd_gf1_voice_t *voice );
#endif
extern void snd_gf1_lfo_init( snd_gus_card_t *gus );
extern void snd_gf1_lfo_done( snd_gus_card_t *gus );
extern void snd_gf1_lfo_program( snd_gus_card_t *gus, int voice, int lfo_type, struct SND_STRU_IW_LFO_PROGRAM *program );
extern void snd_gf1_lfo_enable( snd_gus_card_t *gus, int voice, int lfo_type );
extern void snd_gf1_lfo_disable( snd_gus_card_t *gus, int voice, int lfo_type );
extern void snd_gf1_lfo_change_freq( snd_gus_card_t *gus, int voice, int lfo_type, int freq );
extern void snd_gf1_lfo_change_depth( snd_gus_card_t *gus, int voice, int lfo_type, int depth );
extern void snd_gf1_lfo_setup( snd_gus_card_t *gus, int voice, int lfo_type, int freq, int current_depth, int depth, int sweep, int shape );
extern void snd_gf1_lfo_shutdown( snd_gus_card_t *gus, int voice, int lfo_type );
#if 0
extern void snd_gf1_lfo_command( snd_gus_card_t *gus, int voice, unsigned char *command );
#endif

/* gus_mem.c */

int snd_gf1_mem_xfree( snd_gf1_mem_t *alloc, snd_gf1_mem_block_t *block );
snd_gf1_mem_block_t *snd_gf1_mem_look( snd_gf1_mem_t *alloc, unsigned int address );
snd_gf1_mem_block_t *snd_gf1_mem_share( snd_gf1_mem_t *alloc, unsigned int share_id1, unsigned int share_id2 );
snd_gf1_mem_block_t *snd_gf1_mem_alloc( snd_gf1_mem_t *alloc, char *name, int size, int w_16, int align );
int snd_gf1_mem_free( snd_gf1_mem_t *alloc, unsigned int address );
int snd_gf1_mem_init( snd_gus_card_t *gus );
int snd_gf1_mem_done( snd_gus_card_t *gus );

/* gus_dma.c */

void snd_gf1_dma_program( snd_gus_card_t *gus, unsigned int addr,
                          const void *buf, unsigned int count,
                          unsigned int cmd );
void snd_gf1_dma_ack( snd_gus_card_t *gus );
int snd_gf1_dma_init( snd_gus_card_t *gus );
int snd_gf1_dma_done( snd_gus_card_t *gus );
int snd_gf1_dma_transfer_block( snd_gus_card_t *gus,
                                unsigned int addr,
                                void *buffer,
                                unsigned int count,
                                unsigned int cmd );                                                                                                                                

/* gus_volume.c */

unsigned short snd_gf1_lvol_to_gvol_raw( unsigned int vol );
unsigned int snd_gf1_gvol_to_lvol_raw( unsigned short gf1_vol );
unsigned int snd_gf1_calc_ramp_rate( snd_gus_card_t *gus,
				     unsigned short start,
				     unsigned short end,
				     unsigned int us );
unsigned short snd_gf1_translate_freq( snd_gus_card_t *gus, unsigned int freq2 );
unsigned short snd_gf1_compute_pitchbend( unsigned short pitchbend, unsigned short sens );
unsigned short snd_gf1_compute_freq( unsigned int freq,
				     unsigned int rate,
				     unsigned short mix_rate );

/* gus_reset.c */

void snd_gf1_set_default_handlers( snd_gus_card_t *gus, unsigned int what );
void snd_gf1_smart_stop_voice( snd_gus_card_t *gus, unsigned short voice );
void snd_gf1_stop_voice( snd_gus_card_t *gus, unsigned short voice );
void snd_gf1_clear_voices( snd_gus_card_t *gus, unsigned short v_min, unsigned short v_max );
void snd_gf1_stop_voices( snd_gus_card_t *gus, unsigned short v_min, unsigned short v_max );
int snd_gf1_start( snd_gus_card_t *gus );
int snd_gf1_stop( snd_gus_card_t *gus );
void snd_gf1_open( snd_gus_card_t *gus, unsigned short mode );
void snd_gf1_close( snd_gus_card_t *gus, unsigned short mode );

/* gus_mixer.c */

snd_kmixer_t *snd_gf1_new_mixer( snd_gus_card_t *gus );

/* gus_pcm.c */

snd_pcm_t *snd_gf1_pcm_new_device( snd_gus_card_t *gus, snd_kmixer_t *mixer );

#ifdef SNDCFG_DEBUG
extern void snd_gf1_print_voice_registers( snd_gus_card_t *gus );
extern void snd_gf1_print_global_registers( snd_gus_card_t *gus );
extern void snd_gf1_print_setup_registers( snd_gus_card_t *gus );
extern void snd_gf1_peek_print_block( snd_gus_card_t *gus, unsigned int addr, int count, int w_16bit );
#endif

/* gus.c */

snd_gus_card_t *snd_gus_new_card( snd_card_t *card,
                                  unsigned short port,
                                  unsigned short irqnum,
                                  unsigned short dma1num,
                                  unsigned short dma2num );
int snd_gus_set_port( snd_gus_card_t *card, unsigned short port );
int snd_gus_detect_memory( snd_gus_card_t *gus );
int snd_gus_init_dma_irq( snd_gus_card_t *gus, int latches );
int snd_gus_attach_synthesizer( snd_gus_card_t *gus );
int snd_gus_detach_synthesizer( snd_gus_card_t *gus );
int snd_gus_check_version( snd_gus_card_t *gus );

/* gus_irq.c */

void snd_gus_interrupt( snd_gus_card_t *gus, unsigned char status );

/* gus_uart.c */

snd_rawmidi_t *snd_gf1_rawmidi_new_device( snd_gus_card_t *gus );

#if 0
extern void snd_engine_instrument_register(
		unsigned short mode,
		struct SND_STRU_INSTRUMENT_VOICE_COMMANDS *voice_cmds,
		struct SND_STRU_INSTRUMENT_NOTE_COMMANDS *note_cmds,
		struct SND_STRU_INSTRUMENT_CHANNEL_COMMANDS *channel_cmds );
extern int snd_engine_instrument_register_ask( unsigned short mode );
#endif

#endif /* __GUS_H */
