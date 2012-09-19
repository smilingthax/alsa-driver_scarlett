/*
 * Driver for Digigram pcxhr soundcards
 *
 * main header file
 *
 * Copyright (c) 2004 by Digigram <alsa@digigram.com>
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
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#ifndef __SOUND_PCXHR_H
#define __SOUND_PCXHR_H

#include <linux/interrupt.h>
#include <sound/pcm.h>

#define PCXHR_DRIVER_VERSION		0x000100	/* 0.1.0 */

#define PCXHR_MAX_CARDS			6
#define PCXHR_PLAYBACK_STREAMS		4

#define PCXHR_GRANULARITY		96	/* transfer granularity (should be min 96 and multiple of 48) */


typedef struct snd_pcxhr pcxhr_t;
typedef struct snd_pcxhr_mgr pcxhr_mgr_t;

typedef struct snd_pcxhr_stream pcxhr_stream_t;
typedef struct snd_pcxhr_pipe pcxhr_pipe_t;

enum pcxhr_clock_type {
	PCXHR_CLOCK_TYPE_INTERNAL = 0,
	PCXHR_CLOCK_TYPE_WORD_CLOCK,
	PCXHR_CLOCK_TYPE_AES_SYNC,
	PCXHR_CLOCK_TYPE_AES_1,
	PCXHR_CLOCK_TYPE_AES_2,
	PCXHR_CLOCK_TYPE_AES_3,
	PCXHR_CLOCK_TYPE_AES_4,
};

struct snd_pcxhr_mgr {
	unsigned int num_cards;
	pcxhr_t *chip[PCXHR_MAX_CARDS];

	struct pci_dev *pci;

	int irq;

	/* card access with 1 mem bar and 2 io bar's */
	unsigned long port[3];

	/* share the name */
	char shortname[32];		/* short name of this soundcard */
	char longname[96];		/* name of this soundcard */

	/* message tasklet */
	struct tasklet_struct msg_taskq;
	/* trigger tasklet */
	struct tasklet_struct trigger_taskq;

	spinlock_t lock;		/* interrupt spinlock */
	spinlock_t msg_lock;		/* message spinlock */

	struct semaphore setup_mutex;	/* mutex used in hw_params, open and close */
	struct semaphore mixer_mutex;	/* mutex for mixer */

	/* hardware interface */
	unsigned int dsp_loaded;	/* bit flags of loaded dsp indices */
	unsigned int dsp_version;	/* read from embedded once firmware is loaded */
	int board_has_analog;		/* if 0 the board is digital only */
	int mono_capture;		/* if 1 the board does mono capture */
	int playback_chips;		/* 4 or 6 */
	int capture_chips;		/* 4 or 1 */
	int firmware_num;		/* 41 or 42 */

	struct snd_dma_buffer hostport;

	enum pcxhr_clock_type use_clock_type;	/* clock type selected by mixer */
	enum pcxhr_clock_type cur_clock_type;	/* current clock type synced */
	int sample_rate;
	int ref_count_rate;

	int timer_toggle;		/* timer interrupt toggles between the two values 0x200 and 0x300 */
	int timer_err;			/* timer interrupt toggle errors */
	unsigned int src_it_dsp;	/* dsp interrupt source */
	unsigned int io_num_reg_cont;	/* backup of IO_NUM_REG_CONT */
	unsigned int codec_speed;	/* speed mode of the codecs */
	unsigned int sample_rate_real;	/* current real sample rate */
	int last_reg_stat;
};


enum pcxhr_stream_status {
	PCXHR_STREAM_STATUS_FREE,
	PCXHR_STREAM_STATUS_OPEN,
	PCXHR_STREAM_STATUS_SCHEDULE_RUN,
	PCXHR_STREAM_STATUS_RUNNING,
	PCXHR_STREAM_STATUS_SCHEDULE_STOP,
	PCXHR_STREAM_STATUS_STOPPED,
	PCXHR_STREAM_STATUS_PAUSED
};

struct snd_pcxhr_stream {
	snd_pcm_substream_t *substream;
	snd_pcm_format_t format;
	pcxhr_pipe_t *pipe;

	enum pcxhr_stream_status status;	/* free, open, running, draining, pause */

	int timer_elapsed;			/* timer: samples elapsed since last call to snd_pcm_period_elapsed() */
	snd_pcm_uframes_t timer_abs_samples;	/* timer: samples elapsed since TRIGGER_START */
	int timer_abs_adjusted;

	int channels;
};


enum pcxhr_pipe_status {
	PCXHR_PIPE_UNDEFINED,
	PCXHR_PIPE_STOPPED,
	PCXHR_PIPE_TOGGLE,
	PCXHR_PIPE_RUNNING
};

struct snd_pcxhr_pipe {
	enum pcxhr_pipe_status status;
	int is_capture;		/* this is a capture pipe */
	int first_audio;	/* first audio num */
};


struct snd_pcxhr {
	snd_card_t *card;
	pcxhr_mgr_t *mgr;
	int chip_idx;		/* zero based */

	snd_pcm_t *pcm;		/* PCM */

	pcxhr_pipe_t playback_pipe;		/* 1 stereo pipe only */
	pcxhr_pipe_t capture_pipe[2];		/* 1 stereo pipe or 2 mono pipes */

	pcxhr_stream_t playback_stream[PCXHR_PLAYBACK_STREAMS];
	pcxhr_stream_t capture_stream[2];	/* 1 stereo stream or 2 mono streams */
	int nb_streams_play;
	int nb_streams_capt;

	int analog_playback_active[2];		/* Mixer : Master Playback active (!mute) */
	int analog_playback_volume[2];		/* Mixer : Master Playback Volume */
	int analog_capture_volume[2];		/* Mixer : Master Capture Volume */
	int digital_playback_active[PCXHR_PLAYBACK_STREAMS][2];	/* Mixer : Digital Playback Active [streams][stereo]*/
	int digital_playback_volume[PCXHR_PLAYBACK_STREAMS][2];	/* Mixer : Digital Playback Volume [streams][stereo]*/
	int digital_capture_volume[2];		/* Mixer : Digital Capture Volume [stereo] */
	int monitoring_active[2];		/* Mixer : Monitoring Active */
	int monitoring_volume[2];		/* Mixer : Monitoring Volume */
	int audio_capture_source;		/* Mixer : Audio Capture Source */
	unsigned char aes_bits[5];		/* Mixer : IEC958_AES bits */
};

struct pcxhr_hostport
{
	char purgebuffer[6];
	char reserved[2];
};

/* exported */
int pcxhr_create_pcm(pcxhr_t* chip);
int pcxhr_set_clock(pcxhr_mgr_t *mgr, unsigned int rate);
int pcxhr_get_external_clock(pcxhr_mgr_t *mgr, enum pcxhr_clock_type clock_type, int *sample_rate);

#endif /* __SOUND_PCXHR_H */
