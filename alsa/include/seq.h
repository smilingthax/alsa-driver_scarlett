/*
 *  Main header file for the ALSA sequencer
 *  Copyright (c) 1998 by Frank van de Pol <F.K.W.van.de.Pol@inter.nl.net>
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
#ifndef __SND_SEQ_H
#define __SND_SEQ_H

#include <linux/ioctl.h>

/* version of the sequencer */
#define SND_SEQ_VERSION SND_PROTOCOL_VERSION (0, 0, 1)


/* maximum number of events dequeued per schedule interval */
#define SND_SEQ_MAX_DEQUEUE	50

/* max length of a client name */
#define SND_SEQ_MAX_CLIENT_NAME		32

/* max length of a port name */
#define SND_SEQ_MAX_PORT_NAME	32

/* maximum number of queues */
#define SND_SEQ_MAX_QUEUES 4

/* max number of concurrent clients */
#define SND_SEQ_MAX_CLIENTS 	64




/*                                   	*/
/* definion of sequencer event types 	*/
/*                                   	*/

	/* system messages */
#define SND_SEQ_EVENT_SYSTEM		0

	/* note messages */
#define SND_SEQ_EVENT_NOTE		1
#define SND_SEQ_EVENT_NOTEON		2
#define SND_SEQ_EVENT_NOTEOFF		3
	
	/* control messages */
#define SND_SEQ_EVENT_KEYPRESS		10
#define SND_SEQ_EVENT_CONTROLLER	11
#define SND_SEQ_EVENT_PGMCHANGE		12
#define SND_SEQ_EVENT_CHANPRESS		13
#define SND_SEQ_EVENT_PITCHBEND		14
#define SND_SEQ_EVENT_CONTROL14		15
#define SND_SEQ_EVENT_NONREGPARAM	16
#define SND_SEQ_EVENT_REGPARAM		17


	/* synchronisation messages */
#define SND_SEQ_EVENT_SONGPOS		20	/* Song Position Pointer with LSB and MSB values */
#define SND_SEQ_EVENT_SONGSEL		21	/* Song Select with song ID number */
#define SND_SEQ_EVENT_CLOCK		22	/* midi Real Time Clock message */
#define SND_SEQ_EVENT_START		23	/* midi Real Time Start message */
#define SND_SEQ_EVENT_CONTINUE		24	/* midi Real Time Continue message */
#define SND_SEQ_EVENT_STOP		25	/* midi Real Time Stop message */	
#define SND_SEQ_EVENT_QFRAME		26	/* Midi time code quarter frame */
	
#define SND_SEQ_EVENT_TEMPO		30	/* (SMF) Tempo event */
#define SND_SEQ_EVENT_TIMESIGN		31	/* SMF Time Signature event */
#define SND_SEQ_EVENT_KEYSIGN		32	/* SMF Key Signature event */
	        
#define SND_SEQ_EVENT_SYSEX		40	/* system exclusive data (variable length) */

#define SND_SEQ_EVENT_HEARTBEAT		50	/* "active sensing" event */

	/* system status messages */
#define SND_SEQ_EVENT_CLIENT_START	60	/* new client has connected */
#define SND_SEQ_EVENT_PORT_START	61	/* new port was created */
#define SND_SEQ_EVENT_CLIENT_EXIT	62	/* client has left the system */
#define SND_SEQ_EVENT_PORT_EXIT		63	/* port was deleted from system */

typedef unsigned char snd_seq_event_type;


	/* event address */
typedef struct {
	unsigned char queue;	/* Sequencer queue:       0..255, 255 = broadcast to all queues */
	unsigned char client;	/* Client number:         0..255, 255 = broadcast to all clients */
	unsigned char port;	/* Port within client:    0..255, 255 = broadcast to all ports */
	unsigned char channel;	/* Channel within client: 0..255, 255 = broadcast to all channels */
} snd_seq_addr_t;

#define SND_SEQ_ADDRESS_SUBSCRIBERS	254	/* send event to all subscribed ports */
#define SND_SEQ_ADDRESS_BROADCAST	255	/* send event to all queues/clients/ports/channels */


	/* event mode flag - NOTE: only 8 bits available! */
#define SND_SEQ_TIME_STAMP_TICK	(0<<0) /* timestamp in clock ticks */
#define SND_SEQ_TIME_STAMP_REAL	(1<<0) /* timestamp in real time */
#define SND_SEQ_TIME_STAMP_MASK	(1<<0)

#define SND_SEQ_TIME_MODE_ABS	(0<<1)	/* absolute timestamp */
#define SND_SEQ_TIME_MODE_REL	(1<<1)	/* relative to current time */
#define SND_SEQ_TIME_MODE_MASK	(1<<1)

#define SND_SEQ_EVENT_LENGTH_FIXED	(0<<2)	/* fixed event size */
#define SND_SEQ_EVENT_LENGTH_VARIABLE	(1<<2)	/* variable event size */
#define SND_SEQ_EVENT_LENGTH_MASK	(1<<2)

#define SND_SEQ_PRIORITY_NORMAL	(0<<3)	/* normal priority */
#define SND_SEQ_PRIORITY_HIGH	(1<<3)	/* event should be processed before others */
#define SND_SEQ_PRIORITY_MASK	(1<<3)


	/* note event */
typedef struct {
	unsigned char note;
	unsigned char velocity;
	unsigned int duration;
} snd_seq_ev_note;


	/* controller event */
typedef struct {
	unsigned int param;
	signed int value;
} snd_seq_ev_ctrl;


	/* generic set of bytes (8x8 bit) */
typedef struct {
	unsigned char d[8];	/* 8 bit value */
} snd_seq_ev_raw8;


	/* generic set of integers (2x32 bit) */
typedef struct {
	unsigned int d[2];	/* 32 bit value */
} snd_seq_ev_raw32;


	/* external stored data */
typedef struct {
	int len;	/* length of data */
	void *ptr;	/* pointer to data */
} snd_seq_ev_ext;


typedef struct {
	int	sec;		/* seconds */
	int	nsec;		/* nanoseconds */
} snd_seq_real_time_t;

typedef unsigned int snd_seq_tick_time_t;	/* midi ticks */


	/* sequencer event */
typedef struct snd_seq_event_t {
	snd_seq_event_type	type;	/* event type */
	unsigned char		flags;	/* event flags */
	char			unused1,
				unused2;
	
	/* schedule time */
	union {
		snd_seq_tick_time_t tick;
		snd_seq_real_time_t real;
	} time;
			
	snd_seq_addr_t	source;	/* source address */
	snd_seq_addr_t	dest;	/* destination address */
			
	union {			/* event data... */
		snd_seq_ev_note	 note;
		snd_seq_ev_ctrl  control;
		snd_seq_ev_raw8	 raw8;
		snd_seq_ev_raw32 raw32;		
		snd_seq_ev_ext	 ext;
	} data;
} snd_seq_event_t;


typedef struct {
	int client;				/* client number to inquire */
	int type;				/* client type */
	char name[SND_SEQ_MAX_CLIENT_NAME];	/* client name */
	/*... filters */
} snd_seq_client_info_t;


	/* port capabilities (32 bits) */
#define SND_SEQ_PORT_CAP_MIDI_IN	(1<<0)
#define SND_SEQ_PORT_CAP_MIDI_OUT	(1<<1)

#define SND_SEQ_PORT_CAP_SYNC_IN	(1<<2)
#define SND_SEQ_PORT_CAP_SYNC_OUT	(1<<3)

#define SND_SEQ_PORT_CAP_SUBSCRIPTION	(1<<4)
#define SND_SEQ_PORT_CAP_USE            (1<<5)


	/* port type 	FIXME: needs some betting scheme */
#define SND_SEQ_PORT_TYPE_MIDI_GENERIC	(1<<0)	/* generic MIDI device */ 
#define SND_SEQ_PORT_TYPE_MIDI_GM	(1<<1)	/* General MIDI compatible device */
#define SND_SEQ_PORT_TYPE_MIDI_GS	(1<<2)	/* GS compatible device */
#define SND_SEQ_PORT_TYPE_MIDI_XG	(1<<3)	/* XG compatible device */
/* other standards...*/
#define SND_SEQ_PORT_TYPE_SYNTH		(1<<10)	/* Synth device */
#define SND_SEQ_PORT_TYPE_SAMPLE	(1<<11)	/* Sampling device (support sample download) */
/*...*/
#define SND_SEQ_PORT_TYPE_APPLICATION	(1<<20)	/* application (sequencer/editor) */


	/* perhaps add a device ID for a certain port, and let a userland db
        provide details on the device capabilites (eg. available
        controllers, patches etc., ways to program the device.) This is sort
        of similar to the ID codes we see for PCI devices. */
    
/* port id's */
#ifdef 0
#define SND_SEQ_PORT_TYPE_BASE		(0<<30)
#define SND_SEQ_PORT_TYPE_SYS_TIMER	1	/* ALSA Sequencer timer */
#define SND_SEQ_PORT_TYPE_SYS_ANNOUNCE	2	/* ALSA System announcements */


#define SND_SEQ_PORT_TYPE_APPLICATION
#define SND_SEQ_PORT_TYPE_APPLICATION	1000	/* Generic application */

#define SND_SEQ_PORT_TYPE_DEVICE	(1<<30)
#define SND_SEQ_PORT_TYPE_DEVICE_MIDI		1 /* Generic MIDI */
#define SND_SEQ_PORT_TYPE_DEVICE_GUS_SYNTH 	2
#endif


typedef struct {
	int client;				/* client number */
	int port;				/* port number */
	char name[SND_SEQ_MAX_PORT_NAME];	/* port name */
	
	unsigned int capability;		/* port capability bits */
	unsigned int port_type;			/* port type bits */
			
} snd_seq_port_info_t;


typedef struct {
	int client;				/* client number */
	int port;				/* port number */
	
	/* FIXME:	... read/write mode */
	/* FIXME:	... exclusive y/n */
			
} snd_seq_port_use_t;



/* queue info/status */
typedef struct {
	int 	queue;	/* queue id */

	snd_seq_tick_time_t 	tick;		/* current tick */
	snd_seq_real_time_t 	time;		/* current time */
			
	int			running;	/* running state of queue */		
	unsigned int		tempo;		/* current tempo, us/tick */
	int			ppq;		/* time resolution, ticks/quarter */
	
	int flags;			/* running, sync status etc. */
	
	/* security settings, only owner of this queue can start/stop timer 
	   etc. if the queue is locked for other clients 
	*/
	int			owner;		/* client id for owner of the queue */
	int			locked:1;	/* timing queue locked for other queues */

	
	/* sync source */
	/* sync dest */
	
} snd_seq_queue_info_t;


/* call-backs for kernel client */
typedef int (snd_seq_kernel_client_input_t)( snd_seq_event_t *ev, void *private_data);

typedef struct {
	snd_seq_kernel_client_input_t *input;	
	/*...*/
} snd_seq_client_callback_t;


typedef struct {
	snd_seq_addr_t	sender;	/* sender address */
	snd_seq_addr_t	dest;	/* destination address */	
} snd_seq_port_subscribe_t;


/* ioctl()s definitions */

#define SND_SEQ_IOCTL_VERSION           _IOR ( 'S', 0x00, int )
#define SND_SEQ_IOCTL_CLIENT_ID		_IOR ( 'S', 0x01, int ) 

#define SND_SEQ_IOCTL_GET_CLIENT_INFO	_IOWR( 'S', 0x10, snd_seq_client_info_t )
#define SND_SEQ_IOCTL_SET_CLIENT_INFO	_IOWR( 'S', 0x11, snd_seq_client_info_t )

#define SND_SEQ_IOCTL_CREATE_PORT	_IOWR( 'S', 0x20, snd_seq_port_info_t )
#define SND_SEQ_IOCTL_DELETE_PORT	_IOWR( 'S', 0x21, snd_seq_port_info_t )
#define SND_SEQ_IOCTL_GET_PORT_INFO	_IOWR( 'S', 0x22, snd_seq_port_info_t )
#define SND_SEQ_IOCTL_SET_PORT_INFO	_IOWR( 'S', 0x23, snd_seq_port_info_t )

#define SND_SEQ_IOCTL_SUBSCRIBE_PORT	_IOWR( 'S', 0x30, snd_seq_port_subscribe_t )
#define SND_SEQ_IOCTL_UNSUBSCRIBE_PORT	_IOWR( 'S', 0x31, snd_seq_port_subscribe_t )
#define SND_SEQ_IOCTL_USE_PORT		_IOWR( 'S', 0x32, snd_seq_port_use_t )
#define SND_SEQ_IOCTL_UNUSE_PORT	_IOWR( 'S', 0x33, snd_seq_port_use_t )

#define SND_SEQ_IOCTL_GET_QUEUE_INFO	_IOWR( 'S', 0x40, snd_seq_queue_info_t )
#define SND_SEQ_IOCTL_SET_QUEUE_INFO	_IOWR( 'S', 0x41, snd_seq_queue_info_t )



/* interface for kernel client */
extern int snd_seq_register_kernel_client(snd_seq_client_callback_t *callback, void *private_data);
extern int snd_seq_unregister_kernel_client(int client);
extern int snd_seq_kernel_client_enqueue(int client, snd_seq_event_t *ev);
extern int snd_seq_kernel_client_dispatch(int client, snd_seq_event_t *ev);
extern int snd_seq_kernel_client_ctl(int client, unsigned int cmd, void *arg );

/* allocation and releasing of external data (sysex etc.) */
extern void *snd_seq_ext_malloc(unsigned long size);
extern void snd_seq_ext_free(void *obj, unsigned long size);

#endif
