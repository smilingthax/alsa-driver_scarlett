/*
 *  Copyright (c) 1999 by Uros Bizjak <uros@kss-loka.si>
 *                        Takashi Iwai <iwai@ww.uni-erlangen.de>
 *
 *  SB16ASP/AWE32 CSP control
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

#ifndef __SB_CSP_H
#define __SB_CSP_H

/* CSP modes */
#define SND_SB_CSP_MODE_NONE		0
#define SND_SB_CSP_MODE_DSP_WRITE	1	/* using pcm_format */
#define SND_SB_CSP_MODE_DSP_READ	2	/* ?? */
#define SND_SB_CSP_MODE_DSP_RW		3	/* ?? */
#define SND_SB_CSP_MODE_QSOUND		4	/* QSound - signed 16bit, stereo only? */
/*..others..*/

/* maximum size of micro code */
#define SND_SB_CSP_MAX_MICROCODE_SIZE	2048

/* microcode to be loaded */
typedef struct snd_sb_csp_mc_header {
	unsigned int pcm_format;	/* see asound.h */
	unsigned short csp_mode;	/* see above */
	unsigned short channels;	/* 1=mono, 2=stereo */
	unsigned short init_size;	/* size of initiailization code */
	unsigned short main_size;	/* size of main code */
} snd_sb_csp_mc_header_t;

typedef struct snd_sb_csp_microcode {
	snd_sb_csp_mc_header_t info;
	unsigned char init_code[SND_SB_CSP_MAX_MICROCODE_SIZE];	/* code */
	unsigned char main_code[SND_SB_CSP_MAX_MICROCODE_SIZE];	/* code */
} snd_sb_csp_microcode_t;

/* running state */
#define SND_SB_CSP_ST_RUNNING	0x01
#define SND_SB_CSP_ST_PAUSE	0x02
#define SND_SB_CSP_ST_QSOUND	0x04

/* CSP information */
typedef struct snd_sb_csp_info {
	unsigned int pcm_format;	/* PCM format */
	unsigned short csp_mode;	/* CSP mode */
	unsigned short channels;	/* channels: 1-2 */
	unsigned short version;		/* version id: 0x10 - 0x1f */
	unsigned short state;		/* state bits */
} snd_sb_csp_info_t;

/* QSound channel positions */
typedef struct snd_sb_csp_qsound {
	/* positions is from 0x00 to 0x20
	 * 33 Qsound positions in 180 degree field
	 */
	unsigned char left;		/* left position */
	unsigned char right;		/* right position */
} snd_sb_csp_qsound_t;

/* HWDEP controls */
/* get CSP information */
#define SND_SB_CSP_IOCTL_INFO		_IOR('H', 0x10, snd_sb_csp_info_t)
/* load microcode to CSP */
#define SND_SB_CSP_IOCTL_LOAD_CODE	_IOW('H', 0x11, snd_sb_csp_microcode_t)
/* start CSP */
#define SND_SB_CSP_IOCTL_START		_IO('H', 0x12)
/* stop CSP */
#define SND_SB_CSP_IOCTL_STOP		_IO('H', 0x13)
/* pause CSP and DMA transfer */
#define SND_SB_CSP_IOCTL_PAUSE		_IO('H', 0x14)
/* restart CSP and DMA transfer */
#define SND_SB_CSP_IOCTL_RESTART	_IO('H', 0x15)
/* start QSound codec */
#define SND_SB_CSP_IOCTL_QSOUND_START	_IO('H', 0x20)
/* stop QSound codec */
#define SND_SB_CSP_IOCTL_QSOUND_STOP	_IO('H', 0x21)
/* set QSound channel positions */
#define SND_SB_CSP_IOCTL_QSOUND_POS	_IOW('H', 0x22, snd_sb_csp_qsound_t)
/* get current QSound channel positions */
#define SND_SB_CSP_IOCTL_QSOUND_GET_POS	_IOR('H', 0x23, snd_sb_csp_qsound_t)


#ifdef __KERNEL__
#include "sb.h"
#include "hwdep.h"
#define SND_HWDEP_TYPE_SBCSP	0x10	/* temporarily defined here */
snd_hwdep_t * snd_sb_csp_new_device(sbdsp_t *codec);
#endif

#endif
