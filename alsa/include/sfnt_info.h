/*
 *  Soundfont defines
 *
 *  Copyright (C) 1999 Takashi Iwai
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

#ifndef __SFNT_INFO_H
#define __SFNT_INFO_H

#include "seq_oss_legacy.h"

/*
 * patch information record
 */

/* patch interface header: 16 bytes */
typedef struct soundfont_patch_info_t {
	short key;			/* use the key below */
#define SND_OSS_SOUNDFONT_PATCH		SND_OSS_PATCHKEY(0x07)

	short device_no;		/* synthesizer number */
	unsigned short sf_id;		/* file id (should be zero) */
	short optarg;			/* optional argument */
	int len;			/* data length (without this header) */

	short type;			/* patch operation type */
#define SND_SFNT_LOAD_INFO		0	/* awe_voice_rec */
#define SND_SFNT_LOAD_DATA		1	/* awe_sample_info */
#define SND_SFNT_OPEN_PATCH	2	/* awe_open_parm */
#define SND_SFNT_CLOSE_PATCH	3	/* none */
	/* 4 is obsolete */
#define SND_SFNT_REPLACE_DATA	5	/* awe_sample_info (optarg=#channels)*/
#define SND_SFNT_MAP_PRESET	6	/* awe_voice_map */
	/* 7 is not used */
#define SND_SFNT_PROBE_DATA		8	/* optarg=sample */

	short reserved;			/* word alignment data */

	/* the actual patch data begins after this */
} soundfont_patch_info_t;


/*
 * open patch
 */

#define SND_SFNT_PATCH_NAME_LEN	32

typedef struct soundfont_open_parm_t {
	unsigned short type;		/* sample type */
#define SND_SFNT_PAT_TYPE_MISC	0
#define SND_SFNT_PAT_TYPE_MAP	7
#define SND_SFNT_PAT_LOCKED	0x100	/* lock the samples */
#define SND_SFNT_PAT_SHARED	0x200	/* sample is shared */

	short reserved;
	char name[SND_SFNT_PATCH_NAME_LEN];
} soundfont_open_parm_t;


/*
 * raw voice information record
 */

/* wave table envelope & effect parameters to control EMU8000 */
typedef struct soundfont_voice_parm_t {
	unsigned short moddelay;	/* modulation delay (0x8000) */
	unsigned short modatkhld;	/* modulation attack & hold time (0x7f7f) */
	unsigned short moddcysus;	/* modulation decay & sustain (0x7f7f) */
	unsigned short modrelease;	/* modulation release time (0x807f) */
	short modkeyhold, modkeydecay;	/* envelope change per key (not used) */
	unsigned short voldelay;	/* volume delay (0x8000) */
	unsigned short volatkhld;	/* volume attack & hold time (0x7f7f) */
	unsigned short voldcysus;	/* volume decay & sustain (0x7f7f) */
	unsigned short volrelease;	/* volume release time (0x807f) */
	short volkeyhold, volkeydecay;	/* envelope change per key (not used) */
	unsigned short lfo1delay;	/* LFO1 delay (0x8000) */
	unsigned short lfo2delay;	/* LFO2 delay (0x8000) */
	unsigned short pefe;		/* modulation pitch & cutoff (0x0000) */
	unsigned short fmmod;		/* LFO1 pitch & cutoff (0x0000) */
	unsigned short tremfrq;		/* LFO1 volume & freq (0x0000) */
	unsigned short fm2frq2;		/* LFO2 pitch & freq (0x0000) */
	unsigned char cutoff;		/* initial cutoff (0xff) */
	unsigned char filterQ;		/* initial filter Q [0-15] (0x0) */
	unsigned char chorus;		/* chorus send (0x00) */
	unsigned char reverb;		/* reverb send (0x00) */
	unsigned short reserved[4];	/* not used */
} soundfont_voice_parm_t;

/* redefinition of the same structure, but using byte parameters:
 * WARNING: this definition based on little endian.
 */
typedef struct soundfont_voice_parm_block_t {
	unsigned short moddelay;	/* modulation delay (0x8000) */
	unsigned char modatk, modhld;
	unsigned char moddcy, modsus;
	unsigned char modrel, moddummy;
	short modkeyhold, modkeydecay;	/* envelope change per key (not used) */
	unsigned short voldelay;	/* volume delay (0x8000) */
	unsigned char volatk, volhld;
	unsigned char voldcy, volsus;
	unsigned char volrel, voldummy;
	short volkeyhold, volkeydecay;	/* envelope change per key (not used) */
	unsigned short lfo1delay;	/* LFO1 delay (0x8000) */
	unsigned short lfo2delay;	/* LFO2 delay (0x8000) */
	unsigned char env1fc, env1pit;
	unsigned char lfo1fc, lfo1pit;
	unsigned char lfo1freq, lfo1vol;
	unsigned char lfo2freq, lfo2pit;
	unsigned char cutoff;		/* initial cutoff (0xff) */
	unsigned char filterQ;		/* initial filter Q [0-15] (0x0) */
	unsigned char chorus;		/* chorus send (0x00) */
	unsigned char reverb;		/* reverb send (0x00) */
	unsigned short reserved[4];	/* not used */
} soundfont_voice_parm_block_t;


/* wave table parameters: 92 bytes */
typedef struct soundfont_voice_info_t {
	unsigned short sf_id;		/* file id (should be zero) */
	unsigned short sample;		/* sample id */
	int start, end;			/* sample offset correction */
	int loopstart, loopend;		/* loop offset correction */
	short rate_offset;		/* sample rate pitch offset */
	unsigned short mode;		/* sample mode */
#define SND_SFNT_MODE_ROMSOUND		0x8000
#define SND_SFNT_MODE_STEREO		1
#define SND_SFNT_MODE_LOOPING		2
#define SND_SFNT_MODE_NORELEASE		4	/* obsolete */
#define SND_SFNT_MODE_INIT_PARM		8

	short root;			/* midi root key */
	short tune;			/* pitch tuning (in cents) */
	char low, high;			/* key note range */
	char vellow, velhigh;		/* velocity range */
	char fixkey, fixvel;		/* fixed key, velocity */
	char pan, fixpan;		/* panning, fixed panning */
	short exclusiveClass;		/* exclusive class (0 = none) */
	unsigned char amplitude;	/* sample volume (127 max) */
	unsigned char attenuation;	/* attenuation (0.375dB) */
	short scaleTuning;		/* pitch scale tuning(%), normally 100 */
	soundfont_voice_parm_t parm;	/* voice envelope parameters */
	unsigned short sample_mode;	/* sample mode_flag (set by driver) */
} soundfont_voice_info_t;


/* instrument info header: 4 bytes */
typedef struct soundfont_voice_rec_hdr_t {
	unsigned char bank;		/* midi bank number */
	unsigned char instr;		/* midi preset number */
	char nvoices;			/* number of voices */
	char write_mode;		/* write mode; normally 0 */
#define SND_SFNT_WR_APPEND		0	/* append anyway */
} soundfont_voice_rec_hdr_t;


/*
 * sample wave information
 */

/* wave table sample header: 32 bytes */
typedef struct soundfont_sample_info_t {
	unsigned short sf_id;		/* file id (should be zero) */
	unsigned short sample;		/* sample id */
	int start, end;			/* start & end offset */
	int loopstart, loopend;		/* loop start & end offset */
	int size;			/* size (0 = ROM) */
	short dummy;			/* not used */
	unsigned short mode_flags;	/* mode flags */
#define SND_SFNT_SAMPLE_8BITS		1	/* wave data is 8bits */
#define SND_SFNT_SAMPLE_UNSIGNED	2	/* wave data is unsigned */
#define SND_SFNT_SAMPLE_NO_BLANK	4	/* no blank loop is attached */
#define SND_SFNT_SAMPLE_SINGLESHOT	8	/* single-shot w/o loop */
#define SND_SFNT_SAMPLE_BIDIR_LOOP	16	/* bidirectional looping */
#define SND_SFNT_SAMPLE_STEREO_LEFT	32	/* stereo left sound */
#define SND_SFNT_SAMPLE_STEREO_RIGHT	64	/* stereo right sound */
#define SND_SFNT_SAMPLE_REVERSE_LOOP	128	/* reverse looping */
	unsigned int truesize;		/* used memory size (set by driver) */
} soundfont_sample_info_t;


/*
 * voice preset mapping (aliasing)
 */

typedef struct soundfont_voice_map_t {
	int map_bank, map_instr, map_key;	/* key = -1 means all keys */
	int src_bank, src_instr, src_key;
} soundfont_voice_map_t;


#endif
