/*
 *  Main kernel header file for the ALSA sequencer
 *  Copyright (c) 1999 by Jaroslav Kysela <perex@suse.cz>
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
#ifndef __SND_SEQ_INSTR_H
#define __SND_SEQ_INSTR_H

#include "asequencer.h"
#include "seq_kernel.h"

/* Instrument cluster */
typedef struct {
	snd_seq_instr_cluster_t cluster;
	char name[32];
	int priority;
} snd_seq_kcluster_t;

/* return pointer to private data */
#define KINSTR_DATA(kinstr)	(void *)(((char *)kinstr) + sizeof(snd_seq_kinstr_t))

/* Instrument structure */
typedef struct snd_stru_seq_kinstr {
	snd_seq_instr_t instr;
	char name[32];
	int type;			/* instrument type */
	struct snd_stru_seq_kinstr *next;
} snd_seq_kinstr_t;

/* List of all instruments */
typedef struct {
	void *private_data;		/* pointer to the driver privated data */
	int kinstr_len;			/* kinstr additional length */

	snd_seq_kinstr_t *hash[32];
	int count;			/* count of all instruments */
	
	snd_seq_kcluster_t *chash[32];
	int ccount;			/* count of all clusters */

	int owner;			/* current owner of the instrument list */
} snd_seq_kinstr_list_t;

/* Instrument operations - flags */
#define SND_SEQ_INSTR_OPS_DIRECT	(1<<0)	/* accept only direct events */

/* Instrument space */
#define SND_SEQ_INSTR_SPACE_KERNEL	0
#define SND_SEQ_INSTR_SPACE_USER	1

typedef struct {
	unsigned int flags;
	int (*reset)(void *private_data);
	int (*put)(void *private_data, snd_seq_kinstr_t *kinstr,
		   int space, char *instr_data, int len);
	int (*get)(void *private_data, snd_seq_kinstr_t *kinstr,
		   int space, char *instr_data, int len);
	int (*remove)(void *private_data, snd_seq_kinstr_t *kinstr);
} snd_seq_kinstr_ops_t;

/* instrument operations */
snd_seq_kinstr_list_t *snd_seq_instr_list_new(void);
snd_seq_kinstr_t *snd_seq_instr_find(snd_seq_kinstr_list_t *list,
				     snd_seq_instr_t *instr);
int snd_seq_instr_event(snd_seq_kinstr_ops_t *ops,
			snd_seq_kinstr_list_t *list,
			snd_seq_event_t *ev,
			int atomic);

#endif /* __SND_SEQ_INSTR_H */
