/*
 *  Advanced Linux Sound Architecture - ALSA - Driver direct FM
 *
 *  Interface file between ALSA driver & user space
 *  Copyright (c) 1994-98 by Jaroslav Kysela <perex@suse.cz>,
 *                           4Front Technologies
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

#ifndef __ASOUND_FM_H
#define __ASOUND_FM_H

/****************************************************************************
 *                                                                          *
 *                  Direct FM interface - /dev/snd/fm?                      *
 *                                                                          *
 ****************************************************************************/

#define SND_DM_FM_VERSION	SND_PROTOCOL_VERSION(1, 0, 0)

#define SND_DM_FM_INFO_OPL2	0x00000001
#define SND_DM_FM_INFO_OPL3	0x00000002
#define SND_DM_FM_INFO_OPL4	0x00000004

#define SND_DM_FM_MODE_OPL2	0
#define SND_DM_FM_MODE_OPL3	1

struct snd_dm_fm_info {
	unsigned int type;	/* soundcard type */
	unsigned int flags;	/* see to SND_DM_FM_INFO_XXXX */
	unsigned char id[32];	/* ID of this direct FM device */
	unsigned char name[64];	/* name of this device */
	unsigned char reserved[64];	/* reserved for future... */
};

/*
 *  Data structure composing an FM "note" or sound event.
 */

struct snd_dm_fm_voice {
	unsigned char op;		/* operator cell (0 or 1) */
	unsigned char voice;		/* FM voice (0 to 17) */

	unsigned char am;		/* amplitude modulation */
	unsigned char vibrato;		/* vibrato effect */
	unsigned char do_sustain;	/* sustain phase */
	unsigned char kbd_scale;	/* keyboard scaling */
	unsigned char harmonic;		/* 4 bits: harmonic and multiplier */
	unsigned char scale_level;	/* 2 bits: decrease output freq rises */
	unsigned char volume;		/* 6 bits: volume */

	unsigned char attack;		/* 4 bits: attack rate */
	unsigned char decay;		/* 4 bits: decay rate */
	unsigned char sustain;		/* 4 bits: sustain level */
	unsigned char release;		/* 4 bits: release rate */

	unsigned char feedback;		/* 3 bits: feedback for op0 */
	unsigned char connection;	/* 0 for serial, 1 for parallel */
	unsigned char left;		/* stereo left */
	unsigned char right;		/* stereo right */
	unsigned char waveform;		/* 3 bits: waveform shape */
};

/*
 *  This describes an FM note by its voice, octave, frequency number (10bit)
 *  and key on/off.
 */

struct snd_dm_fm_note {
	unsigned char voice;	/* 0-17 voice channel */
	unsigned char octave;	/* 3 bits: what octave to play */
	unsigned int fnum;	/* 10 bits: frequency number */
	unsigned char key_on;	/* set for active, clear for silent */
};

/*
 *  FM parameters that apply globally to all voices, and thus are not "notes"
 */

struct snd_dm_fm_params {
	unsigned char am_depth;		/* amplitude modulation depth (1=hi) */
	unsigned char vib_depth;	/* vibrato depth (1=hi) */
	unsigned char kbd_split;	/* keyboard split */
	unsigned char rhythm;		/* percussion mode select */

	/* This block is the percussion instrument data */
	unsigned char bass;
	unsigned char snare;
	unsigned char tomtom;
	unsigned char cymbal;
	unsigned char hihat;
};

/*
 *  FM mode ioctl settings
 */

#define SND_DM_FM_IOCTL_PVERSION	_IOR ( 'F', 0x00, int )
#define SND_DM_FM_IOCTL_INFO		_IOR ( 'F', 0x01, struct snd_dm_fm_info )
#define SND_DM_FM_IOCTL_RESET		_IO  ( 'F', 0x02 )
#define SND_DM_FM_IOCTL_SET_MODE	_IOWR( 'F', 0x03, int )
#define SND_DM_FM_IOCTL_PLAY_NOTE	_IOWR( 'F', 0x04, struct snd_dm_fm_note )
#define SND_DM_FM_IOCTL_SET_VOICE	_IOWR( 'F', 0x05, struct snd_dm_fm_voice )
#define SND_DM_FM_IOCTL_SET_PARAMS	_IOWR( 'F', 0x06, struct snd_dm_fm_params )
#define SND_DM_FM_IOCTL_SET_CONNECTION	_IOWR( 'F', 0x07, int )		/* for OPL3 only */

#ifdef __SND_OSS_COMPAT__

#define SND_DM_FM_OSS_IOCTL_RESET	0x20
#define SND_DM_FM_OSS_IOCTL_PLAY_NOTE	0x21
#define SND_DM_FM_OSS_IOCTL_SET_VOICE	0x22
#define SND_DM_FM_OSS_IOCTL_SET_PARAMS	0x23
#define SND_DM_FM_OSS_IOCTL_SET_MODE	0x24
#define SND_DM_FM_OSS_IOCTL_SET_OPL	0x25

#endif

#endif				/* __ASOUND_FM_H */
