#ifndef __SB16_CSP_H
#define __SB16_CSP_H

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

/* CSP modes */
#define SND_SB_CSP_MODE_NONE		0x00
#define SND_SB_CSP_MODE_DSP_READ	0x01	/* Record from DSP */
#define SND_SB_CSP_MODE_DSP_WRITE	0x02	/* Play to DSP */
#define SND_SB_CSP_MODE_QSOUND		0x04	/* QSound */

/* CSP load flags */
#define SND_SB_CSP_LOAD_FROMUSER	0x01
#define SND_SB_CSP_LOAD_INITBLOCK	0x02

/* CSP sample width */
#define SND_SB_CSP_SAMPLE_8BIT		0x01
#define SND_SB_CSP_SAMPLE_16BIT		0x02

/* CSP channels */
#define SND_SB_CSP_MONO			0x01
#define SND_SB_CSP_STEREO		0x02

/* CSP rates */
#define SND_SB_CSP_RATE_8000		0x01
#define SND_SB_CSP_RATE_11025		0x02
#define SND_SB_CSP_RATE_22050		0x04
#define SND_SB_CSP_RATE_44100		0x08
#define SND_SB_CSP_RATE_ALL		0x0f

/* CSP running state */
#define SND_SB_CSP_ST_IDLE		0x00
#define SND_SB_CSP_ST_LOADED		0x01
#define SND_SB_CSP_ST_RUNNING		0x02
#define SND_SB_CSP_ST_PAUSED		0x04
#define SND_SB_CSP_ST_AUTO		0x08
#define SND_SB_CSP_ST_QSOUND		0x10

/* maximum QSound value (180 degrees right) */
#define SND_SB_CSP_QSOUND_MAX_RIGHT	0x20

/* maximum microcode RIFF file size */
#define SND_SB_CSP_MAX_MICROCODE_FILE_SIZE	0x3000

/* microcode header */
typedef struct snd_sb_csp_mc_header {
	char codec_name[16];		/* id name of codec */
	unsigned short func_req;	/* requested function */
} snd_sb_csp_mc_header_t;

/* microcode to be loaded */
typedef struct snd_sb_csp_microcode {
	snd_sb_csp_mc_header_t info;
	unsigned char data[SND_SB_CSP_MAX_MICROCODE_FILE_SIZE];
} snd_sb_csp_microcode_t;

/* start CSP with sample_width in mono/stereo */
typedef struct snd_sb_csp_start {
	int sample_width;	/* sample width, look above */
	int channels;		/* channels, look above */
} snd_sb_csp_start_t;

/* CSP information */
typedef struct snd_sb_csp_info {
	char codec_name[16];		/* id name of codec */
	unsigned short func_nr;		/* function number */
	unsigned int acc_format;	/* accepted PCM formats */
	unsigned short acc_channels;	/* accepted channels */
	unsigned short acc_width;	/* accepted sample width */
	unsigned short acc_rates;	/* accepted sample rates */
	unsigned short csp_mode;	/* CSP mode, see above */
	unsigned short run_channels;	/* current channels  */
	unsigned short run_width;	/* current sample width */
	unsigned short version;		/* version id: 0x10 - 0x1f */
	unsigned short state;		/* state bits */
} snd_sb_csp_info_t;

/* HWDEP controls */
/* get CSP information */
#define SND_SB_CSP_IOCTL_INFO		_IOR('H', 0x10, snd_sb_csp_info_t)
/* load microcode to CSP */
#define SND_SB_CSP_IOCTL_LOAD_CODE	_IOW('H', 0x11, snd_sb_csp_microcode_t)
/* unload microcode from CSP */
#define SND_SB_CSP_IOCTL_UNLOAD_CODE	_IO('H', 0x12)
/* start CSP */
#define SND_SB_CSP_IOCTL_START		_IOW('H', 0x13, snd_sb_csp_start_t)
/* stop CSP */
#define SND_SB_CSP_IOCTL_STOP		_IO('H', 0x14)
/* pause CSP and DMA transfer */
#define SND_SB_CSP_IOCTL_PAUSE		_IO('H', 0x15)
/* restart CSP and DMA transfer */
#define SND_SB_CSP_IOCTL_RESTART	_IO('H', 0x16)

#ifdef __KERNEL__
#include "sb.h"
#include "hwdep.h"

/*
 * CSP private data
 */
typedef struct snd_sb_csp {
	sbdsp_t *codec;		/* SB16 DSP */
	int used;		/* usage flag - exclusive */
	char codec_name[16];	/* name of codec */
	unsigned short func_nr;	/* function number */
	unsigned int acc_format;	/* accepted PCM formats */
	int acc_channels;	/* accepted channels */
	int acc_width;		/* accepted sample width */
	int acc_rates;		/* accepted sample rates */
	int mode;		/* MODE */
	int run_channels;	/* current CSP channels */
	int run_width;		/* current sample width */
	int version;		/* CSP version (0x10 - 0x1f) */
	int running;		/* running state */

	snd_kmixer_t		*mixer;		/* mixer interface to attach */
	snd_kmixer_element_t	*mixer_dest;	/* output target */
	snd_kmixer_element_t	*qsound_element;
	snd_kmixer_group_t 	*qsound_group;

	spinlock_t q_lock;	/* locking */
	int q_enabled;		/* enabled flag */
	int qpos_left;		/* left position */
	int qpos_right;		/* right position */
	int qpos_changed;	/* position changed flag */

	struct semaphore access_mutex;	/* locking */
	snd_info_entry_t *proc;	/* proc interface */
} snd_sb_csp_t;

/*
 * CSP call-backs
 */
typedef struct {
	int (*csp_use) (snd_sb_csp_t * p);
	int (*csp_unuse) (snd_sb_csp_t * p);
	int (*csp_autoload) (snd_sb_csp_t * p, int pcm_sfmt, int play_rec_mode);
	int (*csp_start) (snd_sb_csp_t * p, int sample_width, int channels);
	int (*csp_stop) (snd_sb_csp_t * p);
	int (*csp_qsound_transfer) (snd_sb_csp_t * p);
} snd_sb_csp_callback_t;

#define SND_HWDEP_TYPE_SB16CSP  0x10	/* temporarily defined here */
void snd_sb_csp_register_callbacks(snd_sb_csp_callback_t ** callbacks_ptr);
int snd_sb_csp_new(sbdsp_t * codec, int device, snd_hwdep_t ** rhwdep);
#endif

#endif
