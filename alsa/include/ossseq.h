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

#ifndef __OSSSEQ_H
#define __OSSSEQ_H

#include "synth.h"
#include "asequencer.h"

/*
 * type definitions
 */
typedef struct snd_ossseq_arg_t snd_ossseq_arg_t;
typedef struct snd_ossseq_callback_t snd_ossseq_callback_t;

/*
 * argument structure for synthesizer operations
 */
struct snd_ossseq_arg_t {
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
struct snd_ossseq_callback_t {
	int (*open)(snd_ossseq_arg_t *p, void *closure);
	int (*close)(snd_ossseq_arg_t *p);
	int (*ioctl)(snd_ossseq_arg_t *p, unsigned int cmd, unsigned long arg);
	int (*load_patch)(snd_ossseq_arg_t *p, int format, const char *buf, int offs, int count);
};

/* flag: file_mode */
#define SND_OSSSEQ_FILE_ACMODE		3
#define SND_OSSSEQ_FILE_READ		1
#define SND_OSSSEQ_FILE_WRITE		2
#define SND_OSSSEQ_FILE_NONBLOCK	4

/* flag: seq_mode */
#define SND_OSSSEQ_MODE_SYNTH		0
#define SND_OSSSEQ_MODE_MUSIC		1

/* flag: event_passing */
#define SND_OSSSEQ_PROCESS_EVENTS	0	/* key == 255 is processed as velocity change */
#define SND_OSSSEQ_PASS_EVENTS		1	/* pass all events to callback */
#define SND_OSSSEQ_PROCESS_KEYPRESS	2	/* key >= 128 will be processed as key-pressure */

/* default control rate: fixed */
#define SND_OSSSEQ_CTRLRATE		100

/* default max queue length: configurable by module option */
#define SND_OSSSEQ_MAX_QLEN		1024


/*
 * registration of synth port:
 * returns a unique index if succeeded.
 * Arguments name, type, subtype and nvoices are used for OSS synth_info.
 * Callbacks oper must be given.  Private data pointer is passed only for
 * open callback.
 */
int snd_ossseq_synth_register(char *name, int type, int subtype, int nvoices,
			      snd_ossseq_callback_t *oper, void *private_data);
/*
 * unregistration of synth port:
 * give the registration index returned by ossseq_synth_register().
 */
int snd_ossseq_synth_unregister(int index);


#endif
