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

#include "seq_kernel.h"

/* Instrument cluster */
typedef struct snd_struct_seq_kcluster {
	snd_seq_instr_cluster_t cluster;
	char name[32];
	int priority;
	struct snd_struct_seq_kcluster *next;
} snd_seq_kcluster_t;

/* return pointer to private data */
#define KINSTR_DATA(kinstr)	(void *)(((char *)kinstr) + sizeof(snd_seq_kinstr_t))

/* Instrument structure */
typedef struct snd_stru_seq_kinstr {
	snd_seq_instr_t instr;
	char name[32];
	int type;			/* instrument type */
	int use;			/* use count */
	int add_len;			/* additional length */
	struct snd_stru_seq_kinstr *next;
} snd_seq_kinstr_t;

#define SND_SEQ_INSTR_HASH_SIZE		32

/* List of all instruments */
typedef struct {
	snd_seq_kinstr_t *hash[SND_SEQ_INSTR_HASH_SIZE];
	int count;			/* count of all instruments */
	
	snd_seq_kcluster_t *chash[SND_SEQ_INSTR_HASH_SIZE];
	int ccount;			/* count of all clusters */

	int owner;			/* current owner of the instrument list */

	spinlock_t lock;
	spinlock_t ops_lock;
	struct semaphore ops_mutex;
	unsigned long ops_flags;
} snd_seq_kinstr_list_t;

/* Instrument operations - flags */
#define SND_SEQ_INSTR_OPS_DIRECT	(1<<0)	/* accept only direct events */

typedef struct snd_seq_kinstr_ops {
	void *private_data;
	unsigned int flags;
	long add_len;			/* additional length */
	char *instr_type;
	int (*put)(void *private_data, snd_seq_kinstr_t *kinstr,
		   char *instr_data, long len, int atomic);
	int (*get)(void *private_data, snd_seq_kinstr_t *kinstr,
		   char *instr_data, long len, int atomic);
	int (*remove)(void *private_data, snd_seq_kinstr_t *kinstr, int atomic);
	struct snd_seq_kinstr_ops *next;
} snd_seq_kinstr_ops_t;

/* instrument operations */
snd_seq_kinstr_list_t *snd_seq_instr_list_new(void);
void snd_seq_instr_list_free(snd_seq_kinstr_list_t **list);
snd_seq_kinstr_t *snd_seq_instr_find(snd_seq_kinstr_list_t *list,
				     snd_seq_instr_t *instr);
int snd_seq_instr_event(snd_seq_kinstr_ops_t *ops,
			snd_seq_kinstr_list_t *list,
			snd_seq_event_t *ev,
			int atomic);

#endif /* __SND_SEQ_INSTR_H */
