/*
 *  Midi channel definition for optional channel management.
 *
 *  Copyright (C) 1999 Steve Ratcliffe
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

#ifndef __SEQ_MIDI_EMUL_H
#define __SEQ_MIDI_EMUL_H

#include "seq_kernel.h"

/*
 * This structure is used to keep track of the current state on each
 * channel.  All drivers for hardware that does not understand midi
 * directly will probably need to use this structure.
 */
typedef struct snd_midi_channel {
	void *private;		/* A back pointer to driver data */
	int  number;		/* The channel number */
	int  client;		/* The client associated with this channel */
	int  port;		/* The port associated with this channel */

	unsigned char midi_mode;	/* GM, GS, XG etc */
	unsigned int 
		drum_channel:1,		/* Drum channel */
		param_type:1		/* RPN/NRPN */
		;

	unsigned char midi_aftertouch;	/* Aftertouch (key pressure) */
	unsigned char midi_pressure;	/* Channel pressure */
	unsigned char midi_program;	/* Instrument number */
	short midi_pitchbend;		/* Pitch bend amount */

	unsigned char control[128];	/* Current value of all controls */
	unsigned char note[128];	/* Current status for all notes */

	short gm_rpn_pitch_bend_range;	/* Pitch bend range */
	short gm_rpn_fine_tuning; 	/* Master fine tuning */
	short gm_rpn_coarse_tuning;	/* Master coarse tuning */

} snd_midi_channel_t;

/*
 * A structure that represets a set of channels bound to a port.  There
 * would usually be 16 channels per port.  But fewer could be used for
 * particular cases.
 * The channel set consists of information describing the client and
 * port for this midi synth and an array of snd_midi_channel_t structures.
 * A driver that had no need for snd_midi_channel_t could still use the
 * channel set type if it wished with the channel array null.
 */
typedef struct snd_midi_channel_set {
	void *private_data;		/* Driver data */
	int  client;			/* Client for this port */
	int  port;			/* The port number */

	int  max_channels;		/* Size of the channels array */
	snd_midi_channel_t *channels;

	unsigned char midi_mode;	/* MIDI operating mode */
	unsigned char gs_master_volume;	/* SYSEX master volume: 0-127 */
	unsigned char gs_chorus_mode;
	unsigned char gs_reverb_mode;

} snd_midi_channel_set_t;

typedef struct snd_seq_midi_op {
	void (*note_on)(void *private_data, int note, int vel, snd_midi_channel_t *chan);
	void (*note_off)(void *private_data,int note, int vel, snd_midi_channel_t *chan); /* release note */
	void (*key_press)(void *private_data, int note, int vel, snd_midi_channel_t *chan);
	void (*note_terminate)(void *private_data, int note, snd_midi_channel_t *chan); /* terminate note immediately */
	void (*control)(void *private_data, int type, snd_midi_channel_t *chan);
	void (*reset)(void *private);
	void (*nrpn)(void *private_data, snd_midi_channel_t *chan, snd_midi_channel_set_t *chset);
	void (*sysex)(void *private_data, unsigned char *buf, int len, int parsed, snd_midi_channel_set_t *chset);
} snd_midi_op_t;

/*
 * These defines are used so that pitchbend, aftertouch etc, can be
 * distinguished from controller values.
 */
/* 0-127 controller values */
#define SND_MCTL_PITCHBEND	0x80
#define SND_MCTL_AFTERTOUCH	0x81
#define SND_MCTL_CHAN_PRESSURE	0x82

/*
 * These names exist to allow symbolic access to the controls array.
 * The usage is eg: chan->gm_bank_select.  Another implementation would
 * be really have these members in the struct, and not the array.
 */
#define gm_bank_select		control[0]
#define gm_modulation		control[1]
#define gm_breath		control[2]
#define gm_foot_pedal		control[4]
#define gm_portamento_time	control[5]
#define gm_data_entry		control[6]
#define gm_volume		control[7]
#define gm_balance		control[8]
#define gm_pan			control[10]
#define gm_expression		control[11]
#define gm_effect_control1	control[12]
#define gm_effect_control2	control[13]
#define gm_slider1		control[16]
#define gm_slider2		control[17]
#define gm_slider3		control[18]
#define gm_slider4		control[19]

#define gm_bank_select_lsb	control[32]
#define gm_modulation_wheel_lsb	control[33]
#define gm_breath_lsb		control[34]
#define gm_foot_pedal_lsb	control[36]
#define gm_portamento_time_lsb	control[37]
#define gm_data_entry_lsb	control[38]
#define gm_volume_lsb		control[39]
#define gm_balance_lsb		control[40]
#define gm_pan_lsb		control[42]
#define gm_expression_lsb	control[43]
#define gm_effect_control1_lsb	control[44]
#define gm_effect_control2_lsb	control[45]

#define gm_sustain	 	control[SND_MCTL_SUSTAIN]
#define gm_hold			gm_sustain
#define gm_portamento		control[SND_MCTL_PORTAMENTO]
#define gm_sustenuto		control[SND_MCTL_SUSTENUTO]

/*
 * These macros give the complete value of the controls that consist
 * of coarse and fine pairs.  Of course the fine controls are seldom used
 * but there is no harm in being complete.
 */
#define SND_GM_BANK_SELECT(cp)		(((cp)->control[0]<<7)|((cp)->control[32]))
#define SND_GM_MODULATION_WHEEL(cp)	(((cp)->control[1]<<7)|((cp)->control[33]))
#define SND_GM_BREATH(cp)		(((cp)->control[2]<<7)|((cp)->control[34]))
#define SND_GM_FOOT_PEDAL(cp)		(((cp)->control[4]<<7)|((cp)->control[36]))
#define SND_GM_PORTAMENTO_TIME(cp)	(((cp)->control[5]<<7)|((cp)->control[37]))
#define SND_GM_DATA_ENTRY(cp)		(((cp)->control[6]<<7)|((cp)->control[38]))
#define SND_GM_VOLUME(cp)		(((cp)->control[7]<<7)|((cp)->control[39]))
#define SND_GM_BALANCE(cp)		(((cp)->control[8]<<7)|((cp)->control[40]))
#define SND_GM_PAN(cp)			(((cp)->control[10]<<7)|((cp)->control[42]))
#define SND_GM_EXPRESSION(cp)		(((cp)->control[11]<<7)|((cp)->control[43]))


/* MIDI mode */
#define SND_MIDI_MODE_NONE	0	/* Generic midi */
#define SND_MIDI_MODE_GM	1
#define SND_MIDI_MODE_GS	2
#define SND_MIDI_MODE_XG	3
#define SND_MIDI_MODE_MT32	4

/* MIDI note state */
#define SND_MIDI_NOTE_OFF		0x00
#define SND_MIDI_NOTE_ON		0x01
#define SND_MIDI_NOTE_RELEASED		0x02
#define SND_MIDI_NOTE_SUSTENUTO		0x04
 
#define SND_MIDI_PARAM_TYPE_REGISTERED		0
#define SND_MIDI_PARAM_TYPE_NONREGISTERED	1

/* SYSEX parse flag */
enum {
	SND_MIDI_SYSEX_NOT_PARSED = 0,
	SND_MIDI_SYSEX_GM_ON,	
	SND_MIDI_SYSEX_GS_ON,	
	SND_MIDI_SYSEX_GS_RESET,	
	SND_MIDI_SYSEX_GS_CHORUS_MODE,
	SND_MIDI_SYSEX_GS_REVERB_MODE,
	SND_MIDI_SYSEX_GS_MASTER_VOLUME,
	SND_MIDI_SYSEX_GS_PROGRAM,
	SND_MIDI_SYSEX_GS_DRUM_CHANNEL,
	SND_MIDI_SYSEX_XG_ON,	
};

/* Prototypes for midi_process.c */
void snd_midi_process_event(snd_midi_op_t *ops, snd_seq_event_t *ev,
			    snd_midi_channel_set_t *chanset);
void snd_midi_channel_set_clear(snd_midi_channel_set_t *chset);
void snd_midi_channel_init(snd_midi_channel_t *p, int n);
snd_midi_channel_t *snd_midi_channel_init_set(int n);
snd_midi_channel_set_t *snd_midi_channel_alloc_set(int n);
void snd_midi_channel_free_set(snd_midi_channel_set_t *chset);

#endif /* __SEQ_MIDI_EMUL_H */
