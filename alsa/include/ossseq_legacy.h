/*
 * OSS compatible macro definitions
 *
 * Copyright (C) 1998,99 Takashi Iwai
 *
 * Most of the part is derived from <linux/soundcard.h>
 *               copyrighted by Hannu Savolainen 1993-1997
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __OSSSEQ_LEGACY_H
#define __OSSSEQ_LEGACY_H

/*
 * ioctl commands
 */

#define SND_OSS_SNDCTL_SEQ_RESET		_IO  ('Q', 0)
#define SND_OSS_SNDCTL_SEQ_SYNC			_IO  ('Q', 1)
#define SND_OSS_SNDCTL_SYNTH_INFO		_IOWR('Q', 2, oss_synth_info_t)
#define SND_OSS_SNDCTL_SEQ_CTRLRATE		_IOWR('Q', 3, int)
#define SND_OSS_SNDCTL_SEQ_GETOUTCOUNT		_IOR ('Q', 4, int)
#define SND_OSS_SNDCTL_SEQ_GETINCOUNT		_IOR ('Q', 5, int)
#define SND_OSS_SNDCTL_SEQ_PERCMODE		_IOW ('Q', 6, int)
#define SND_OSS_SNDCTL_SEQ_TESTMIDI		_IOW ('Q', 8, int)
#define SND_OSS_SNDCTL_SEQ_RESETSAMPLES		_IOW ('Q', 9, int)
#define SND_OSS_SNDCTL_SEQ_NRSYNTHS		_IOR ('Q',10, int)
#define SND_OSS_SNDCTL_SEQ_NRMIDIS		_IOR ('Q',11, int)
#define SND_OSS_SNDCTL_MIDI_INFO		_IOWR('Q',12, oss_midi_info_t)
#define SND_OSS_SNDCTL_SEQ_THRESHOLD		_IOW ('Q',13, int)
#define SND_OSS_SNDCTL_SYNTH_MEMAVL		_IOWR('Q',14, int)
#define SND_OSS_SNDCTL_FM_4OP_ENABLE		_IOW ('Q',15, int)
#define SND_OSS_SNDCTL_SEQ_PANIC		_IO  ('Q',17)
#define SND_OSS_SNDCTL_SEQ_OUTOFBAND		_IOW ('Q',18, oss_seq_event_t)
#define SND_OSS_SNDCTL_SEQ_GETTIME		_IOR ('Q',19, int)
#define SND_OSS_SNDCTL_SYNTH_ID			_IOWR('Q',20, oss_synth_info_t)
#define SND_OSS_SNDCTL_TMR_TIMEBASE		_IOWR('T', 1, int)
#define SND_OSS_SNDCTL_TMR_START		_IO  ('T', 2)
#define SND_OSS_SNDCTL_TMR_STOP			_IO  ('T', 3)
#define SND_OSS_SNDCTL_TMR_CONTINUE		_IO  ('T', 4)
#define SND_OSS_SNDCTL_TMR_TEMPO		_IOWR('T', 5, int)
#define SND_OSS_SNDCTL_TMR_SOURCE		_IOWR('T', 6, int)
#	define SND_TMR_INTERNAL		0x00000001
#	define SND_TMR_EXTERNAL		0x00000002
#		define SND_TMR_MODE_MIDI	0x00000010
#		define SND_TMR_MODE_FSK		0x00000020
#		define SND_TMR_MODE_CLS		0x00000040
#		define SND_TMR_MODE_SMPTE	0x00000080
#define SND_OSS_SNDCTL_TMR_METRONOME		_IOW ('T', 7, int)
#define SND_OSS_SNDCTL_TMR_SELECT		_IOW ('T', 8, int)
#define SND_OSS_SNDCTL_MIDI_PRETIME		_IOWR('m', 0, int)


/*
 * sequencer event packet for OUT_OF_BOUND ioctl
 */
typedef struct oss_seq_event_t {
	unsigned char arr[8];
} oss_seq_event_t;


/*
 * patch key
 */
#define SND_OSS_PATCHKEY(id) ((id<<8)|0xfd)


/*
 * GUS patch format
 */

typedef struct oss_patch_info_t oss_patch_info_t;

struct oss_patch_info_t {
	unsigned short key;		/* Use WAVE_PATCH here */
#define SND_OSS_WAVE_PATCH	SND_OSS_PATCHKEY(0x04)
#define SND_OSS_GUS_PATCH	SND_OSS_WAVE_PATCH

	short device_no;	/* Synthesizer number */
	short instr_no;		/* Midi pgm# */

	unsigned int mode;
/*
 * The least significant byte has the same format than the GUS .PAT
 * files
 */
#define SND_OSS_WAVE_16_BITS	0x01	/* bit 0 = 8 or 16 bit wave data. */
#define SND_OSS_WAVE_UNSIGNED	0x02	/* bit 1 = Signed - Unsigned data. */
#define SND_OSS_WAVE_LOOPING	0x04	/* bit 2 = looping enabled-1. */
#define SND_OSS_WAVE_BIDIR_LOOP	0x08	/* bit 3 = Set is bidirectional looping. */
#define SND_OSS_WAVE_LOOP_BACK	0x10	/* bit 4 = Set is looping backward. */
#define SND_OSS_WAVE_SUSTAIN_ON	0x20	/* bit 5 = Turn sustaining on. (Env. pts. 3)*/
#define SND_OSS_WAVE_ENVELOPES	0x40	/* bit 6 = Enable envelopes - 1 */
#define SND_OSS_WAVE_FAST_RELEASE 0x80	/* bit 7 = Shut off immediately after note off */
				/* 	(use the env_rate/env_offs fields). */
/* Linux specific bits */
#define SND_OSS_WAVE_VIBRATO	0x00010000	/* The vibrato info is valid */
#define SND_OSS_WAVE_TREMOLO	0x00020000	/* The tremolo info is valid */
#define SND_OSS_WAVE_SCALE	0x00040000	/* The scaling info is valid */
#define SND_OSS_WAVE_FRACTIONS	0x00080000	/* Fraction information is valid */
/* Reserved bits */
#define SND_OSS_WAVE_ROM	0x40000000	/* For future use */
#define SND_OSS_WAVE_MULAW	0x20000000	/* For future use */
/* Other bits must be zeroed */

	int len;	/* Size of the wave data in bytes */
	int loop_start, loop_end; /* Byte offsets from the beginning */

	unsigned int base_freq;
	unsigned int base_note;
	unsigned int high_note;
	unsigned int low_note;
	int panning;	/* -128=left, 127=right */
	int detuning;

       /* Envelope. Enabled by mode bit WAVE_ENVELOPES	*/
	unsigned char	env_rate[ 6 ];	 /* GUS HW ramping rate */
	unsigned char	env_offset[ 6 ]; /* 255 == 100% */

	/* 
	 * Enable by setting the mode bits WAVE_TREMOLO, WAVE_VIBRATO or
	 * WAVE_SCALE
	 */
	unsigned char	tremolo_sweep;
	unsigned char	tremolo_rate;
	unsigned char	tremolo_depth;
	
	unsigned char	vibrato_sweep;
	unsigned char	vibrato_rate;
	unsigned char	vibrato_depth;

	int		scale_frequency;
	unsigned int	scale_factor;		/* from 0 to 2048 or 0 to 2 */
	
	int		volume;
	int		fractions;
	int		reserved1;
	int		spare[2];
	char data[1];	/* The waveform data starts here */
};


/*
 * SYSEX patch -- does any driver support this?
 */
typedef struct oss_sysex_info_t oss_sysex_info_t;

struct oss_sysex_info_t {
	short key;		/* Use SYSEX_PATCH or MAUI_PATCH here */
#define SND_OSS_SYSEX_PATCH	SND_OSS_PATCHKEY(0x05)
#define SND_OSS_MAUI_PATCH	SND_OSS_PATCHKEY(0x06)
	short device_no;	/* Synthesizer number */
	int len;	/* Size of the sysex data in bytes */
	unsigned char data[1];	/* Sysex data starts here */
};


/*
 * /dev/sequencer input events.
 */
#define SND_OSS_SEQ_NOTEOFF		0
#define SND_OSS_SEQ_NOTEON		1
#define SND_OSS_SEQ_WAIT		SND_OSS_TMR_WAIT_ABS
#define SND_OSS_SEQ_PGMCHANGE		3
#define SND_OSS_SEQ_SYNCTIMER		SND_OSS_TMR_START
#define SND_OSS_SEQ_MIDIPUTC		5
#define SND_OSS_SEQ_ECHO		SND_OSS_TMR_ECHO
#define SND_OSS_SEQ_AFTERTOUCH		9
#define SND_OSS_SEQ_CONTROLLER		10
#define SND_OSS_SEQ_BALANCE		11
#define SND_OSS_SEQ_VOLMODE             12
#define   SND_OSS_VOL_METHOD_ADAGIO	1
#define   SND_OSS_VOL_METHOD_LINEAR	2
#define SND_OSS_SEQ_FULLSIZE		0xfd
#define SND_OSS_SEQ_PRIVATE		0xfe
#define SND_OSS_SEQ_EXTENDED		0xff

#define SND_OSS_EV_SEQ_LOCAL		0x80
#define SND_OSS_EV_TIMING		0x81
#define SND_OSS_EV_CHN_COMMON		0x92
#define SND_OSS_EV_CHN_VOICE		0x93
#define SND_OSS_EV_SYSEX		0x94

#define SND_OSS_MIDI_NOTEOFF		0x80
#define SND_OSS_MIDI_NOTEON		0x90
#define SND_OSS_MIDI_KEY_PRESSURE	0xA0
#define SND_OSS_MIDI_CTL_CHANGE		0xB0
#define SND_OSS_MIDI_PGM_CHANGE		0xC0
#define SND_OSS_MIDI_CHN_PRESSURE	0xD0
#define SND_OSS_MIDI_PITCH_BEND		0xE0
#define SND_OSS_MIDI_SYSTEM_PREFIX	0xF0

#define SND_OSS_TMR_WAIT_REL		1
#define SND_OSS_TMR_WAIT_ABS		2
#define SND_OSS_TMR_STOP		3
#define SND_OSS_TMR_START		4
#define SND_OSS_TMR_CONTINUE		5
#define SND_OSS_TMR_TEMPO		6
#define SND_OSS_TMR_ECHO		8
#define SND_OSS_TMR_CLOCK		9
#define SND_OSS_TMR_SPP			10
#define SND_OSS_TMR_TIMESIG		11


/*
 *	Midi controller numbers
 */
#define	   SND_OSS_CTL_BANK_SELECT		0x00
#define	   SND_OSS_CTL_MODWHEEL			0x01
#define    SND_OSS_CTL_BREATH			0x02
/*		undefined		0x03 */
#define    SND_OSS_CTL_FOOT			0x04
#define    SND_OSS_CTL_PORTAMENTO_TIME		0x05
#define    SND_OSS_CTL_DATA_ENTRY		0x06
#define    SND_OSS_CTL_MAIN_VOLUME		0x07
#define    SND_OSS_CTL_BALANCE			0x08
/*		undefined		0x09 */
#define    SND_OSS_CTL_PAN			0x0a
#define    SND_OSS_CTL_EXPRESSION		0x0b
/*		undefined		0x0c */
/*		undefined		0x0d */
/*		undefined		0x0e */
/*		undefined		0x0f */
#define    SND_OSS_CTL_GENERAL_PURPOSE1	0x10
#define    SND_OSS_CTL_GENERAL_PURPOSE2	0x11
#define    SND_OSS_CTL_GENERAL_PURPOSE3	0x12
#define    SND_OSS_CTL_GENERAL_PURPOSE4	0x13
/*		undefined		0x14 - 0x1f */
/*		undefined		0x20 */
#define    SND_OSS_CTL_DAMPER_PEDAL		0x40
#define    SND_OSS_CTL_SUSTAIN			0x40	/* Alias */
#define    SND_OSS_CTL_HOLD			0x40	/* Alias */
#define    SND_OSS_CTL_PORTAMENTO		0x41
#define    SND_OSS_CTL_SOSTENUTO		0x42
#define    SND_OSS_CTL_SOFT_PEDAL		0x43
/*		undefined		0x44 */
#define    SND_OSS_CTL_HOLD2			0x45
/*		undefined		0x46 - 0x4f */
#define    SND_OSS_CTL_GENERAL_PURPOSE5	0x50
#define    SND_OSS_CTL_GENERAL_PURPOSE6	0x51
#define    SND_OSS_CTL_GENERAL_PURPOSE7	0x52
#define    SND_OSS_CTL_GENERAL_PURPOSE8	0x53
/*		undefined		0x54 - 0x5a */
#define    SND_OSS_CTL_EXT_EFF_DEPTH		0x5b
#define    SND_OSS_CTL_TREMOLO_DEPTH		0x5c
#define    SND_OSS_CTL_CHORUS_DEPTH		0x5d
#define    SND_OSS_CTL_DETUNE_DEPTH		0x5e
#define    SND_OSS_CTL_CELESTE_DEPTH		0x5e	/* Alias for the above one */
#define    SND_OSS_CTL_PHASER_DEPTH		0x5f
#define    SND_OSS_CTL_DATA_INCREMENT		0x60
#define    SND_OSS_CTL_DATA_DECREMENT		0x61
#define    SND_OSS_CTL_NONREG_PARM_NUM_LSB	0x62
#define    SND_OSS_CTL_NONREG_PARM_NUM_MSB	0x63
#define    SND_OSS_CTL_REGIST_PARM_NUM_LSB	0x64
#define    SND_OSS_CTL_REGIST_PARM_NUM_MSB	0x65
/*		undefined		0x66 - 0x78 */
/*		reserved		0x79 - 0x7f */
#define    SND_OSS_CTRL_PITCH_BENDER		255
#define    SND_OSS_CTRL_PITCH_BENDER_RANGE	254
#define    SND_OSS_CTRL_EXPRESSION		253	/* Obsolete */
#define    SND_OSS_CTRL_MAIN_VOLUME		252	/* Obsolete */


/*
 * synth device information, returned by SYNTH_INFO ioctl.
 */

typedef struct oss_synth_info_t oss_synth_info_t;

struct oss_synth_info_t {
	char	name[30];
	int	device;		/* 0-N. INITIALIZE BEFORE CALLING */
	int	synth_type;
#define SND_OSS_SYNTH_TYPE_FM			0
#define SND_OSS_SYNTH_TYPE_SAMPLE		1
#define SND_OSS_SYNTH_TYPE_MIDI			2	/* Midi interface */

	int	synth_subtype;
#define SND_OSS_FM_TYPE_ADLIB			0x00
#define SND_OSS_FM_TYPE_OPL3			0x01
#define SND_OSS_MIDI_TYPE_MPU401		0x401

#define SND_OSS_SAMPLE_TYPE_BASIC		0x10
#define SND_OSS_SAMPLE_TYPE_GUS			SAMPLE_TYPE_BASIC
#define SND_OSS_SAMPLE_TYPE_AWE32		0x20

	int	perc_mode;	/* No longer supported */
	int	nr_voices;
	int	nr_drums;	/* Obsolete field */
	int	instr_bank_size;
	unsigned int	capabilities;	
#define SND_OSS_SYNTH_CAP_PERCMODE		0x00000001 /* No longer used */
#define SND_OSS_SYNTH_CAP_OPL3			0x00000002 /* Set if OPL3 supported */
#define SND_OSS_SYNTH_CAP_INPUT			0x00000004 /* Input (MIDI) device */
	int	dummies[19];	/* Reserve space */
};


/*
 * MIDI device information, returned by MIDI_INFO ioctl.
 */

typedef struct oss_midi_info_t oss_midi_info_t;

struct oss_midi_info_t {
	char		name[30];
	int		device;		/* 0-N. INITIALIZE BEFORE CALLING */
	unsigned int	capabilities;	/* To be defined later */
	int		dev_type;
	int		dummies[18];	/* Reserve space */
};

#endif
