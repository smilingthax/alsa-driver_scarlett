#ifndef __MINORS_H
#define __MINORS_H

/*
 *  MINOR numbers
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

#define SND_MINOR_OSS_MIXER	0	/* /dev/mixer - OSS 3.XX compatible */
#define SND_MINOR_OSS_SEQUENCER	1	/* /dev/sequencer - OSS 3.XX compatible */
#define	SND_MINOR_OSS_MIDI	2	/* /dev/midi - native midi interface - OSS 3.XX compatible - UART */
#define SND_MINOR_OSS_PCM_8	3	/* /dev/dsp - 8bit PCM - OSS 3.XX compatible */
#define SND_MINOR_OSS_AUDIO	4	/* /dev/audio - SunSparc compatible */
#define SND_MINOR_OSS_PCM_16	5	/* /dev/dsp16 - 16bit PCM - OSS 3.XX compatible */
#define SND_MINOR_OSS_SNDSTAT	6	/* /dev/sndstat - for compatibility with OSS */
#define SND_MINOR_OSS_RESERVED7	7	/* reserved for future use */
#define SND_MINOR_OSS_MUSIC	8	/* /dev/music - OSS 3.XX compatible */
#define SND_MINOR_OSS_DMMIDI	9	/* /dev/dmmidi0 - this device can have another minor # with OSS */
#define SND_MINOR_OSS_DMFM	10	/* /dev/dmfm0 - this device can have another minor # with OSS */
#define SND_MINOR_OSS_MIXER1	11	/* alternate mixer */
#define SND_MINOR_OSS_PCM1	12	/* alternate PCM (GF-A-1) */
#define SND_MINOR_OSS_MIDI1	13	/* alternate midi - SYNTH */
#define SND_MINOR_OSS_DMMIDI1	14	/* alternate dmmidi - SYNTH */

#define SND_MINOR_OSS_MASK	0x000f

#define SND_MINOR_BEGIN		128
/* global (not per soundcard) devices */
#define SND_MINOR_SEQUENCER	/* 136 */ (SND_MINOR_BEGIN+8)
/* local (per soundcard) devices */
#define SND_MINOR_CONTROL	/* 144 */ (SND_MINOR_BEGIN+16)
#define SND_MINOR_MIXER		/* 152 */ (SND_MINOR_CONTROL+SND_CARDS)
#define SND_MINOR_MIXERS	2
#define SND_MINOR_PCM		/* 168 */ (SND_MINOR_MIXER+(SND_CARDS*SND_MINOR_MIXERS))
#define SND_MINOR_PCMS		4
#define SND_MINOR_RAWMIDI	/* 200 */ (SND_MINOR_PCM+(SND_CARDS*SND_MINOR_PCMS))
#define SND_MINOR_RAWMIDIS	4
#define SND_MINOR_RES1		/* 232 */ (SND_MINOR_RAWMIDI+(SND_CARDS*SND_MINOR_RAWMIDIS))
#define SND_MINOR_RES2		/* 240 */ (SND_MINOR_RES1+SND_CARDS)
#define SND_MINOR_FM		/* 248 */ (SND_MINOR_RES2+SND_CARDS)

#endif /* __MINORS_H */
