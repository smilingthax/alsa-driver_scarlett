/*
 * OSS compatible sequencer driver
 *
 * Copyright (C) 1998,99 Takashi Iwai
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

#ifndef __SEQ_OSS_H
#define __SEQ_OSS_H

#ifndef ALSA_BUILD
#include <linux/asequencer.h>
#else
#include "asequencer.h"
#endif

/*
 * type definitions
 */
typedef struct snd_seq_oss_arg_t snd_seq_oss_arg_t;
typedef struct snd_seq_oss_callback_t snd_seq_oss_callback_t;

/*
 * argument structure for synthesizer operations
 */
struct snd_seq_oss_arg_t {
	/* given by OSS sequencer */
	int app_index;	/* application unique index */
	int file_mode;	/* file mode - see below */
	int seq_mode;	/* sequencer mode - see below */

	/* following must be initialized in open callback */
	snd_seq_addr_t addr;	/* opened port address */
	void *private_data;	/* private data for lowlevel drivers */

	/* note-on event passing mode: initially given by OSS seq,
	 * but configurable by drivers - see below
	 */
	int event_passing;
};


/*
 * synthesizer operation callbacks
 */
struct snd_seq_oss_callback_t {
	int (*open)(snd_seq_oss_arg_t *p, void *closure);
	int (*close)(snd_seq_oss_arg_t *p);
	int (*ioctl)(snd_seq_oss_arg_t *p, unsigned int cmd, unsigned long arg);
	int (*load_patch)(snd_seq_oss_arg_t *p, int format, const char *buf, int offs, int count);
	int (*reset)(snd_seq_oss_arg_t *p);
	int (*raw_event)(snd_seq_oss_arg_t *p, unsigned char *data);
};

/* flag: file_mode */
#define SND_SEQ_OSS_FILE_ACMODE		3
#define SND_SEQ_OSS_FILE_READ		1
#define SND_SEQ_OSS_FILE_WRITE		2
#define SND_SEQ_OSS_FILE_NONBLOCK	4

/* flag: seq_mode */
#define SND_SEQ_OSS_MODE_SYNTH		0
#define SND_SEQ_OSS_MODE_MUSIC		1

/* flag: event_passing */
#define SND_SEQ_OSS_PROCESS_EVENTS	0	/* key == 255 is processed as velocity change */
#define SND_SEQ_OSS_PASS_EVENTS		1	/* pass all events to callback */
#define SND_SEQ_OSS_PROCESS_KEYPRESS	2	/* key >= 128 will be processed as key-pressure */

/* default control rate: fixed */
#define SND_SEQ_OSS_CTRLRATE		100

/* default max queue length: configurable by module option */
#define SND_SEQ_OSS_MAX_QLEN		1024


/*
 * data pointer to snd_seq_register_device
 */
typedef struct snd_seq_oss_reg {
	int type;
	int subtype;
	int nvoices;
	snd_seq_oss_callback_t oper;
	void *private_data;
} snd_seq_oss_reg_t;

/* device id */
#define SND_SEQ_DEV_OSS		"seq-oss"

/*
 * registration of synth port:
 * returns a unique index if succeeded.
 * Arguments name, type, subtype and nvoices are used for OSS synth_info.
 * Callbacks oper must be given.  Private data pointer is passed only for
 * open callback.
 */
/*int snd_seq_oss_synth_register(char *name, int type, int subtype, int nvoices,
			      snd_seq_oss_callback_t *oper, void *private_data);*/
/*
 * unregistration of synth port:
 * give the registration index returned by seq_oss_synth_register().
 */
/*int snd_seq_oss_synth_unregister(int index);*/


#endif
