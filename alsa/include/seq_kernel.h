/*
 *  Main kernel header file for the ALSA sequencer
 *  Copyright (c) 1998 by Frank van de Pol <frank@vande-pol.demon.nl>
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
#ifndef __SND_SEQ_KERNEL_H
#define __SND_SEQ_KERNEL_H

#ifndef ALSA_BUILD
#include <linux/asequencer.h>
#else
#include "asequencer.h"
#endif

/* maximum number of events dequeued per schedule interval */
#define SND_SEQ_MAX_DEQUEUE		50

/* maximum number of queues */
#define SND_SEQ_MAX_QUEUES		8

/* max number of concurrent clients */
#define SND_SEQ_MAX_CLIENTS 		192

/* max number of concurrent ports */
#define SND_SEQ_MAX_PORTS 		254

/* max number of events in memory pool */
#define SND_SEQ_MAX_EVENTS		2000

/* default number of events in memory pool */
#define SND_SEQ_DEFAULT_EVENTS		500

/* max number of events in memory pool for one client (outqueue) */
#define SND_SEQ_MAX_CLIENT_EVENTS	2000

/* default number of events in memory pool for one client (outqueue) */
#define SND_SEQ_DEFAULT_CLIENT_EVENTS	200

/* max delivery path length */
#define SND_SEQ_MAX_HOPS		10

/* max size of event size */
#define SND_SEQ_MAX_EVENT_LEN		4096

/* typedefs */
struct snd_seq_stru_user_client;
struct snd_seq_stru_kernel_client;
struct snd_seq_stru_client;
struct snd_seq_stru_queue;

typedef struct snd_seq_stru_user_client user_client_t;
typedef struct snd_seq_stru_kernel_client kernel_client_t;
typedef struct snd_seq_stru_client client_t;
typedef struct snd_seq_stru_queue queue_t;

/* call-backs for kernel client */

typedef struct {
	void *private_data;
	int allow_input: 1,
	    allow_output: 1;
	/*...*/
} snd_seq_client_callback_t;

/* call-backs for kernel port */
typedef int (snd_seq_kernel_port_subscribe_t)(void *private_data, snd_seq_port_subscribe_t *info);
typedef int (snd_seq_kernel_port_unsubscribe_t)(void *private_data, snd_seq_port_subscribe_t *info);
typedef int (snd_seq_kernel_port_use_t)(void *private_data, snd_seq_port_subscribe_t *info);
typedef int (snd_seq_kernel_port_unuse_t)(void *private_data, snd_seq_port_subscribe_t *info);
typedef int (snd_seq_kernel_port_input_t)(snd_seq_event_t *ev, int direct, void *private_data, int atomic, int hop);
typedef void (snd_seq_kernel_port_private_free_t)(void *private_data);

typedef struct {
	void *private_data;
	snd_seq_kernel_port_subscribe_t *subscribe;
	snd_seq_kernel_port_unsubscribe_t *unsubscribe;
	snd_seq_kernel_port_use_t *use;
	snd_seq_kernel_port_unuse_t *unuse;
	snd_seq_kernel_port_input_t *event_input;
	snd_seq_kernel_port_private_free_t *private_free;
	/*...*/
} snd_seq_port_callback_t;

/* interface for kernel client */
extern int snd_seq_create_kernel_client(snd_card_t *card, int client_index, snd_seq_client_callback_t *callback);
extern int snd_seq_delete_kernel_client(int client);
extern int snd_seq_kernel_client_enqueue(int client, snd_seq_event_t *ev, int atomic, int hop);
extern int snd_seq_kernel_client_dispatch(int client, snd_seq_event_t *ev, int atomic, int hop);
extern int snd_seq_kernel_client_ctl(int client, unsigned int cmd, void *arg);

/* allocation and releasing of external data (sysex etc.) */
extern void *snd_seq_ext_malloc(unsigned long size, int atomic);
extern void snd_seq_ext_free(void *obj, unsigned long size);
extern void snd_seq_ext_ref(void *obj);

/* port callback routines */
void snd_port_init_callback(snd_seq_port_callback_t *p);
snd_seq_port_callback_t *snd_port_alloc_callback(void);

/* port attach/detach */
int snd_seq_event_port_attach(int client, snd_seq_port_callback_t *pcbp,
			      int cap, int type, char *portname);
int snd_seq_event_port_detach(int client, int port);

#endif /* __SND_SEQ_KERNEL_H */
