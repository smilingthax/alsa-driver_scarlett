/*
 *  Main header file for the ALSA sequencer
 *  Copyright (c) 1998-1999 by Frank van de Pol <frank@vande-pol.demon.nl>
 *            (c) 1998/1999 by Jaroslav Kysela <perex@suse.cz>
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
#ifndef __SND_ASEQUENCER_H
#define __SND_ASEQUENCER_H

#ifndef __KERNEL__
#include <linux/ioctl.h>
#include <sys/ipc.h>
#endif

/* version of the sequencer */
#define SND_SEQ_VERSION SND_PROTOCOL_VERSION (1, 0, 0)

/*                                   	*/
/* definition of sequencer event types 	*/
/*                                   	*/

/* 0-4: system messages
 * event data type = snd_seq_result_t
 */
#define SND_SEQ_EVENT_SYSTEM		0
#define SND_SEQ_EVENT_RESULT		1
/* 2-4: reserved */

/* 5-9: note messages (channel specific)
 * event data type = snd_seq_ev_note
 */
#define SND_SEQ_EVENT_NOTE		5
#define SND_SEQ_EVENT_NOTEON		6
#define SND_SEQ_EVENT_NOTEOFF		7
#define SND_SEQ_EVENT_KEYPRESS		8
/* 9-10: reserved */
	
/* 10-19: control messages (channel specific)
 * event data type = snd_seq_ev_ctrl
 */
#define SND_SEQ_EVENT_CONTROLLER	10
#define SND_SEQ_EVENT_PGMCHANGE		11
#define SND_SEQ_EVENT_CHANPRESS		12
#define SND_SEQ_EVENT_PITCHBEND		13	/* from -8192 to 8191 */
#define SND_SEQ_EVENT_CONTROL14		14	/* 14 bit controller value */
#define SND_SEQ_EVENT_NONREGPARAM	15	/* 14 bit NRPN */
#define SND_SEQ_EVENT_REGPARAM		16	/* 14 bit RPN */
/* 18-19: reserved */

/* 20-29: synchronisation messages
 * event data type = snd_seq_ev_ctrl
 */
#define SND_SEQ_EVENT_SONGPOS		20	/* Song Position Pointer with LSB and MSB values */
#define SND_SEQ_EVENT_SONGSEL		21	/* Song Select with song ID number */
#define SND_SEQ_EVENT_QFRAME		22	/* midi time code quarter frame */
#define SND_SEQ_EVENT_TIMESIGN		23	/* SMF Time Signature event */
#define SND_SEQ_EVENT_KEYSIGN		24	/* SMF Key Signature event */
/* 25-29: reserved */
	        
/* 30-39: timer messages
 * event data type = snd_seq_ev_queue_control_t
 */
#define SND_SEQ_EVENT_START		30	/* midi Real Time Start message */
#define SND_SEQ_EVENT_CONTINUE		31	/* midi Real Time Continue message */
#define SND_SEQ_EVENT_STOP		32	/* midi Real Time Stop message */	
#define	SND_SEQ_EVENT_SETPOS_TICK	33	/* set tick queue position */
#define SND_SEQ_EVENT_SETPOS_TIME	34	/* set realtime queue position */
#define SND_SEQ_EVENT_TEMPO		35	/* (SMF) Tempo event */
#define SND_SEQ_EVENT_CLOCK		36	/* midi Real Time Clock message */
#define SND_SEQ_EVENT_TICK		37	/* midi Real Time Tick message */
/* 38-39: reserved */

/* 40-49: others
 * event data type = none
 */
#define SND_SEQ_EVENT_TUNE_REQUEST	40	/* tune request */
#define SND_SEQ_EVENT_RESET		41	/* reset to power-on state */
#define SND_SEQ_EVENT_SENSING		42	/* "active sensing" event */
/* 43-49: reserved */

/* 50-59: echo back, kernel private messages
 * event data type = any type
 */
#define SND_SEQ_EVENT_ECHO		50	/* echo event */
#define SND_SEQ_EVENT_OSS		51	/* OSS raw event */
/* 52-59: reserved */

/* 60-69: system status messages (broadcast for subscribers)
 * event data type = snd_seq_addr_t
 */
#define SND_SEQ_EVENT_CLIENT_START	60	/* new client has connected */
#define SND_SEQ_EVENT_CLIENT_EXIT	61	/* client has left the system */
#define SND_SEQ_EVENT_CLIENT_CHANGE	62	/* client status/info has changed */
#define SND_SEQ_EVENT_PORT_START	63	/* new port was created */
#define SND_SEQ_EVENT_PORT_EXIT		64	/* port was deleted from system */
#define SND_SEQ_EVENT_PORT_CHANGE	65	/* port status/info has changed */
#define SND_SEQ_EVENT_PORT_SUBSCRIBED	66	/* read port is subscribed */
#define SND_SEQ_EVENT_PORT_USED		67	/* write port is subscribed */
#define SND_SEQ_EVENT_PORT_UNSUBSCRIBED	68	/* read port is released */
#define SND_SEQ_EVENT_PORT_UNUSED	69	/* write port is released */

/* 70-79: synthesizer events
 * event data type = snd_seq_eve_sample_control_t
 */
#define SND_SEQ_EVENT_SAMPLE		70	/* sample select */
#define SND_SEQ_EVENT_SAMPLE_CLUSTER	71	/* sample cluster select */
#define SND_SEQ_EVENT_SAMPLE_START	72	/* voice start */
#define SND_SEQ_EVENT_SAMPLE_STOP	73	/* voice stop */
#define SND_SEQ_EVENT_SAMPLE_FREQ	74	/* playback frequency */
#define SND_SEQ_EVENT_SAMPLE_VOLUME	75	/* volume and balance */
#define SND_SEQ_EVENT_SAMPLE_LOOP	76	/* sample loop */
#define SND_SEQ_EVENT_SAMPLE_POSITION	77	/* sample position */
#define SND_SEQ_EVENT_SAMPLE_PRIVATE1	78	/* private (hardware dependent) event */

/* 80-89: reserved */

/* 90-99: user-defined events with fixed length
 * event data type = any
 */
#define SND_SEQ_EVENT_USR0		90
#define SND_SEQ_EVENT_USR1		91
#define SND_SEQ_EVENT_USR2		92
#define SND_SEQ_EVENT_USR3		93
#define SND_SEQ_EVENT_USR4		94
#define SND_SEQ_EVENT_USR5		95
#define SND_SEQ_EVENT_USR6		96
#define SND_SEQ_EVENT_USR7		97
#define SND_SEQ_EVENT_USR8		98
#define SND_SEQ_EVENT_USR9		99

/* 100-129: instrument layer
 * variable length data can be passed directly to the driver
 */
#define SND_SEQ_EVENT_INSTR_BEGIN	100	/* begin of instrument management */
#define SND_SEQ_EVENT_INSTR_END		102	/* end of instrument management */
#define SND_SEQ_EVENT_INSTR_PUT		103	/* put instrument to port */
#define SND_SEQ_EVENT_INSTR_GET		104	/* get instrument from port */
#define SND_SEQ_EVENT_INSTR_GET_RESULT	105	/* result */
#define SND_SEQ_EVENT_INSTR_FREE	106	/* free instrument(s) */
#define SND_SEQ_EVENT_INSTR_LIST	107	/* instrument list */
#define SND_SEQ_EVENT_INSTR_LIST_RESULT 108	/* result */
#define SND_SEQ_EVENT_INSTR_RESET	109	/* reset instrument memory */
#define SND_SEQ_EVENT_INSTR_INFO	110	/* instrument interface info */
#define SND_SEQ_EVENT_INSTR_INFO_RESULT 111	/* result */
#define SND_SEQ_EVENT_INSTR_STATUS	112	/* instrument interface status */
#define SND_SEQ_EVENT_INSTR_STATUS_RESULT 113	/* result */
#define SND_SEQ_EVENT_INSTR_CLUSTER	114	/* cluster parameters */
#define SND_SEQ_EVENT_INSTR_CLUSTER_GET	115	/* get cluster parameters */
#define SND_SEQ_EVENT_INSTR_CLUSTER_RESULT 116	/* result */
#define SND_SEQ_EVENT_INSTR_CHANGE	117	/* instrument change */
/* 118-129: reserved */

/* 130-139: variable length events
 * event data type = snd_seq_ev_ext
 * (SND_SEQ_EVENT_LENGTH_VARIABLE must be set)
 */
#define SND_SEQ_EVENT_SYSEX		130	/* system exclusive data (variable length) */
/* 131-134: reserved */
#define SND_SEQ_EVENT_USR_VAR0		135
#define SND_SEQ_EVENT_USR_VAR1		136
#define SND_SEQ_EVENT_USR_VAR2		137
#define SND_SEQ_EVENT_USR_VAR3		138
#define SND_SEQ_EVENT_USR_VAR4		139

/* 140-149: IPC shared memory events (*NOT SUPPORTED YET*)
 * event data type = snd_seq_ev_ipcshm
 * (SND_SEQ_EVENT_LENGTH_VARIPC must be set)
 */
#define SND_SEQ_EVENT_IPCSHM		140
/* 141-144: reserved */
#define SND_SEQ_EVENT_USR_VARIPC0	145
#define SND_SEQ_EVENT_USR_VARIPC1	146
#define SND_SEQ_EVENT_USR_VARIPC2	147
#define SND_SEQ_EVENT_USR_VARIPC3	148
#define SND_SEQ_EVENT_USR_VARIPC4	149

/* 150-191: reserved */

/* 192-254: hardware specific events */

/* 255: special event */
#define SND_SEQ_EVENT_NONE		255


typedef unsigned char snd_seq_event_type;


	/* event address */
typedef struct {
	unsigned char client;	/* Client number:         0..255, 255 = broadcast to all clients */
	unsigned char port;	/* Port within client:    0..255, 255 = broadcast to all ports */
} snd_seq_addr_t;

#define SND_SEQ_ADDRESS_UNKNOWN		253	/* unknown source */
#define SND_SEQ_ADDRESS_SUBSCRIBERS	254	/* send event to all subscribed ports */
#define SND_SEQ_ADDRESS_BROADCAST	255	/* send event to all queues/clients/ports/channels */


	/* event mode flag - NOTE: only 8 bits available! */
#define SND_SEQ_TIME_STAMP_TICK		(0<<0) /* timestamp in clock ticks */
#define SND_SEQ_TIME_STAMP_REAL		(1<<0) /* timestamp in real time */
#define SND_SEQ_TIME_STAMP_MASK		(1<<0)

#define SND_SEQ_TIME_MODE_ABS		(0<<1)	/* absolute timestamp */
#define SND_SEQ_TIME_MODE_REL		(1<<1)	/* relative to current time */
#define SND_SEQ_TIME_MODE_MASK		(1<<1)

#define SND_SEQ_EVENT_LENGTH_FIXED	(0<<2)	/* fixed event size */
#define SND_SEQ_EVENT_LENGTH_VARIABLE	(1<<2)	/* variable event size */
#define SND_SEQ_EVENT_LENGTH_VARUSR	(2<<2)	/* variable event size - user memory space */
#define SND_SEQ_EVENT_LENGTH_VARIPC	(3<<2)	/* variable event size - IPC */
#define SND_SEQ_EVENT_LENGTH_MASK	(3<<2)

#define SND_SEQ_PRIORITY_NORMAL		(0<<4)	/* normal priority */
#define SND_SEQ_PRIORITY_HIGH		(1<<4)	/* event should be processed before others */
#define SND_SEQ_PRIORITY_MASK		(1<<4)

#define SND_SEQ_DEST_QUEUE		(0<<5)	/* normal destination */
#define SND_SEQ_DEST_DIRECT		(1<<5)	/* bypass enqueueing */
#define SND_SEQ_DEST_MASK		(1<<5)


	/* note event */
typedef struct {
	unsigned char channel;
	unsigned char note;
	unsigned char velocity;
	unsigned char off_velocity;	/* only for SND_SEQ_EVENT_NOTE */
	unsigned int duration;		/* only for SND_SEQ_EVENT_NOTE */
} snd_seq_ev_note;

	/* controller event */
typedef struct {
	unsigned char channel;
	unsigned char unused1, unused2, unused3;	/* pad */
	unsigned int param;
	signed int value;
} snd_seq_ev_ctrl;

	/* generic set of bytes (12x8 bit) */
typedef struct {
	unsigned char d[12];	/* 8 bit value */
} snd_seq_ev_raw8;

	/* generic set of integers (3x32 bit) */
typedef struct {
	unsigned int d[3];	/* 32 bit value */
} snd_seq_ev_raw32;

	/* external stored data */
typedef struct {
	int len;		/* length of data */
	void *ptr;		/* pointer to data (note: maybe 64-bit) */
} snd_seq_ev_ext;

	/* external stored data - IPC shared memory */
typedef struct {
	int len;		/* length of data */
	key_t ipc;		/* IPC key */
} snd_seq_ev_ipcshm;

/* Instrument cluster type */
typedef unsigned long snd_seq_instr_cluster_t;

/* Instrument type */
typedef struct {
	snd_seq_instr_cluster_t cluster;
	unsigned int std;		/* the upper byte means a private instrument (owner - client #) */
	unsigned short bank;
	unsigned short prg;
} snd_seq_instr_t;

	/* sample number */
typedef struct {
	unsigned int std;
	unsigned short bank;
	unsigned short prg;
} snd_seq_ev_sample;

	/* sample cluster */
typedef struct {
	snd_seq_instr_cluster_t cluster;
} snd_seq_ev_cluster;

	/* sample position */
typedef unsigned int snd_seq_position_t; /* playback position (in samples) * 16 */

	/* sample stop mode */
typedef enum {
	SAMPLE_STOP_IMMEDIATELY = 0,	/* terminate playing immediately */
	SAMPLE_STOP_VENVELOPE = 1,	/* finish volume envelope */
	SAMPLE_STOP_LOOP = 2		/* terminate loop and finish wave */
} snd_seq_stop_mode_t;

	/* sample frequency */
typedef int snd_seq_frequency_t; /* playback frequency in HZ * 16 */

	/* sample volume control; if any value is set to -1 == do not change */
typedef struct {
	signed short volume;	/* range: 0-16383 */
	signed short lr;	/* left-right balance; range: 0-16383 */
	signed short fr;	/* front-rear balance; range: 0-16383 */
	signed short du;	/* down-up balance; range: 0-16383 */
} snd_seq_ev_volume;

	/* simple loop redefinition */
typedef struct {
	unsigned int start;	/* loop start (in samples) * 16 */
	unsigned int end;	/* loop end (in samples) * 16 */
} snd_seq_ev_loop;

typedef struct {
	unsigned char channel;
	unsigned char unused1, unused2, unused3;	/* pad */
	union {
		snd_seq_ev_sample sample;
		snd_seq_ev_cluster cluster;
		snd_seq_position_t position;
		snd_seq_stop_mode_t stop_mode;
		snd_seq_frequency_t frequency;
		snd_seq_ev_volume volume;
		snd_seq_ev_loop loop;
		unsigned char raw8[8];
	} param;
} snd_seq_ev_sample_control_t;



/* INSTR_BEGIN event */
typedef struct {
	long timeout;	/* zero = forever, otherwise timeout in ms */
} snd_seq_ev_instr_begin_t;

typedef struct {
	int event;		/* processed event type */
	int result;
} snd_seq_result_t;


typedef struct {
	long int tv_sec;	/* seconds */
	long int tv_nsec;	/* nanoseconds */
} snd_seq_real_time_t;

typedef unsigned int snd_seq_tick_time_t;	/* midi ticks */

typedef union {
	snd_seq_tick_time_t tick;
	snd_seq_real_time_t real;
} snd_seq_timestamp_t;

	/* queue timer control */
typedef struct {
	unsigned char queue;	/* affected queue */
	unsigned char unused1, unused2, unused3;	/* pad */
	union {
		signed int value;	/* affected value (e.g. tempo) */
		snd_seq_timestamp_t time;
	} param;
} snd_seq_ev_queue_control_t;

	/* sequencer event */
typedef struct snd_seq_event_t {
	snd_seq_event_type type;	/* event type */
	unsigned char flags;		/* event flags */
	char tag;
	
	unsigned char queue;		/* schedule queue */
	snd_seq_timestamp_t time;	/* schedule time */


	snd_seq_addr_t source;	/* source address */
	snd_seq_addr_t dest;	/* destination address */

	union {			/* event data... */
		snd_seq_ev_note note;
		snd_seq_ev_ctrl control;
		snd_seq_ev_raw8 raw8;
		snd_seq_ev_raw32 raw32;
		snd_seq_ev_ext ext;
		snd_seq_ev_ipcshm ipcshm;
		snd_seq_ev_queue_control_t queue;
		snd_seq_timestamp_t time;
		snd_seq_addr_t addr;
		snd_seq_result_t result;
		snd_seq_ev_instr_begin_t instr_begin;
		snd_seq_ev_sample_control_t sample;
	} data;
} snd_seq_event_t;


/*
 * type check macros
 */
/* result events: 0-4 */
#define snd_seq_ev_is_result_type(ev)	((ev)->type < 5)
/* channel specific events: 5-19 */
#define snd_seq_ev_is_channel_type(ev)	((ev)->type >= 5 && (ev)->type < 20)
/* note events: 5-9 */
#define snd_seq_ev_is_note_type(ev)	((ev)->type >= 5 && (ev)->type < 10)
/* control events: 10-19 */
#define snd_seq_ev_is_control_type(ev)	((ev)->type >= 10 && (ev)->type < 20)
/* queue control events: 30-39 */
#define snd_seq_ev_is_queue_type(ev)	((ev)->type >= 30 && (ev)->type < 40)
/* fixed length events: 0-99 */
#define snd_seq_ev_is_fixed_type(ev)	((ev)->type < 100)
/* instrument layer events: 100-129 */
#define snd_seq_ev_is_instr_type(ev)	((ev)->type >= 100 && (ev)->type < 130)
/* variable length events: 130-139 */
#define snd_seq_ev_is_variable_type(ev)	((ev)->type >= 130 && (ev)->type < 140)
/* ipc shmem events: 140-149 */
#define snd_seq_ev_is_varipc_type(ev)	((ev)->type >= 140 && (ev)->type < 150)

/*
 * macros to check event flags
 */
/* direct dispatched events */
#define snd_seq_ev_is_direct(ev)	(((ev)->flags & SND_SEQ_DEST_MASK) == SND_SEQ_DEST_DIRECT)
/* prior events */
#define snd_seq_ev_is_prior(ev)		(((ev)->flags & SND_SEQ_PRIORITY_MASK) == SND_SEQ_PRIORITY_HIGH)

/* event length type */
#define snd_seq_ev_length_type(ev)	((ev)->flags & SND_SEQ_EVENT_LENGTH_MASK)
#define snd_seq_ev_is_fixed(ev)		(snd_seq_ev_length_type(ev) == SND_SEQ_EVENT_LENGTH_FIXED)
#define snd_seq_ev_is_variable(ev)	(snd_seq_ev_length_type(ev) == SND_SEQ_EVENT_LENGTH_VARIABLE)
#define snd_seq_ev_is_varusr(ev)	(snd_seq_ev_length_type(ev) == SND_SEQ_EVENT_LENGTH_VARUSR)
#define snd_seq_ev_is_varipc(ev)	(snd_seq_ev_length_type(ev) == SND_SEQ_EVENT_LENGTH_VARIPC)

/* time-stamp type */
#define snd_seq_ev_timestamp_type(ev)	((ev)->flags & SND_SEQ_TIME_STAMP_MASK)
#define snd_seq_ev_is_tick(ev)		(snd_seq_ev_timestamp_type(ev) == SND_SEQ_TIME_STAMP_TICK)
#define snd_seq_ev_is_real(ev)		(snd_seq_ev_timestamp_type(ev) == SND_SEQ_TIME_STAMP_REAL)

/* time-mode type */
#define snd_seq_ev_timemode_type(ev)	((ev)->flags & SND_SEQ_TIME_MODE_MASK)
#define snd_seq_ev_is_abstime(ev)	(snd_seq_ev_timemode_type(ev) == SND_SEQ_TIME_MODE_ABS)
#define snd_seq_ev_is_reltime(ev)	(snd_seq_ev_timemode_type(ev) == SND_SEQ_TIME_MODE_REL)


	/* system information */
typedef struct {
	int queues;			/* maximum queues count */
	int clients;			/* maximum clients count */
	int ports;			/* maximum ports per client */
	int channels;			/* maximum channels per port */
	char reserved[32];
} snd_seq_system_info_t;


	/* known client numbers */
#define SND_SEQ_CLIENT_SYSTEM		0
#define SND_SEQ_CLIENT_DUMMY		62	/* dummy ports */
#define SND_SEQ_CLIENT_OSS		63	/* oss sequencer emulator */


	/* client types */
typedef enum {
	NO_CLIENT       = 0,
	USER_CLIENT     = 1,
	KERNEL_CLIENT   = 2
} snd_seq_client_type_t;
                        
	/* event filter flags */
#define SND_SEQ_FILTER_BROADCAST	(1<<0)	/* accept broadcast messages */
#define SND_SEQ_FILTER_MULTICAST	(1<<1)	/* accept multicast messages */
#define SND_SEQ_FILTER_USE_EVENT	(1<<31)	/* use event filter */

typedef struct {
	int client;			/* client number to inquire */
	snd_seq_client_type_t type;	/* client type */
	char name[64];			/* client name */
	unsigned int filter;		/* filter flags */
	unsigned char multicast_filter[8]; /* multicast filter bitmap */
	unsigned char event_filter[32];	/* event filter bitmap */
	char group[32];			/* group name */
	int num_ports;			/* RO: number of ports */
	char reserved[64];		/* for future use */
} snd_seq_client_info_t;


/* client pool size */
typedef struct {
	int client;			/* client number to inquire */
	int output_pool;		/* outgoing (write) pool size */
	int input_pool;			/* incoming (read) pool size */
	int output_room;		/* minimum free pool size for select/blocking mode */
	int output_free;		/* unused size */
	int input_free;			/* unused size */
	char reserved[64];
} snd_seq_client_pool_t;


/* Remove events by specified criteria */
typedef struct snd_seq_remove_events {
	int tick:1; 		/* True when time is in ticks */
	int input:1; 		/* Flush input queues */
	int output:1; 		/* Flush output queues */

	int  remove_mode;	/* Flags that determine what gets removed */

	snd_seq_timestamp_t time;

	unsigned char queue;	/* Queue for REMOVE_DEST */
	snd_seq_addr_t dest;	/* Address for REMOVE_DEST */
	unsigned char channel;	/* Channel for REMOVE_DEST */

	int  type;	/* For REMOVE_EVENT_TYPE */
	char  tag;	/* Tag for REMOVE_TAG */

	int  reserved[10];	/* To allow for future binary compatibility */

} snd_seq_remove_events_t;

/* Flush mode flags */
#define SND_SEQ_REMOVE_DEST		(1<<0)	/* Restrict by destination q:client:port */
#define SND_SEQ_REMOVE_DEST_CHANNEL	(1<<1)	/* Restrict by channel */
#define SND_SEQ_REMOVE_TIME_BEFORE	(1<<2)	/* Restrict to before time */
#define SND_SEQ_REMOVE_TIME_AFTER	(1<<3)	/* Restrict to time or after */
#define SND_SEQ_REMOVE_EVENT_TYPE	(1<<4)	/* Restrict to event type */
#define SND_SEQ_REMOVE_IGNORE_OFF 	(1<<5)	/* Do not flush off events */
#define SND_SEQ_REMOVE_TAG_MATCH 	(1<<6)	/* Restrict to events with given tag */


	/* known port numbers */
#define SND_SEQ_PORT_SYSTEM_TIMER	0
#define SND_SEQ_PORT_SYSTEM_ANNOUNCE	1

	/* port capabilities (32 bits) */
#define SND_SEQ_PORT_CAP_READ		(1<<0)	/* readable from this port */
#define SND_SEQ_PORT_CAP_WRITE		(1<<1)	/* writable to this port */

#define SND_SEQ_PORT_CAP_SYNC_READ	(1<<2)
#define SND_SEQ_PORT_CAP_SYNC_WRITE	(1<<3)

#define SND_SEQ_PORT_CAP_DUPLEX		(1<<4)

#define SND_SEQ_PORT_CAP_SUBS_READ	(1<<5)	/* allow read subscription */
#define SND_SEQ_PORT_CAP_SUBS_WRITE	(1<<6)	/* allow write subscription */
#define SND_SEQ_PORT_CAP_NO_EXPORT	(1<<7)	/* routing not allowed */

	/* port type */
#define SND_SEQ_PORT_TYPE_SPECIFIC	(1<<0)	/* hardware specific */
#define SND_SEQ_PORT_TYPE_MIDI_GENERIC	(1<<1)	/* generic MIDI device */
#define SND_SEQ_PORT_TYPE_MIDI_GM	(1<<2)	/* General MIDI compatible device */
#define SND_SEQ_PORT_TYPE_MIDI_GS	(1<<3)	/* GS compatible device */
#define SND_SEQ_PORT_TYPE_MIDI_XG	(1<<4)	/* XG compatible device */
#define SND_SEQ_PORT_TYPE_MIDI_MT32	(1<<5)	/* MT-32 compatible device */

/* other standards...*/
#define SND_SEQ_PORT_TYPE_SYNTH		(1<<10)	/* Synth device */
#define SND_SEQ_PORT_TYPE_DIRECT_SAMPLE	(1<<11)	/* Sampling device (support sample download) */
#define SND_SEQ_PORT_TYPE_SAMPLE	(1<<12)	/* Sampling device (sample can be downloaded at any time) */
/*...*/
#define SND_SEQ_PORT_TYPE_APPLICATION	(1<<20)	/* application (sequencer/editor) */

/* standard group names */
#define SND_SEQ_GROUP_SYSTEM		"system"
#define SND_SEQ_GROUP_DEVICE		"device"
#define SND_SEQ_GROUP_APPLICATION	"application"

typedef struct {
	int client;			/* client number */
	int port;			/* port number */
	char name[64];			/* port name */
	char group[32];			/* group name (copied from client) */

	unsigned int capability;	/* port capability bits */
	unsigned int cap_group;		/* permission to group */
	unsigned int type;		/* port type bits */
	int midi_channels;		/* channels per MIDI port */
	int midi_voices;		/* voices per MIDI port */
	int synth_voices;		/* voices per SYNTH port */

	int read_use;			/* R/O: subscribers for output (from this port) */
	int write_use;			/* R/O: subscribers for input (to this port) */

	void *kernel;			/* reserved for kernel use (must be NULL) */

	char reserved[64];		/* for future use */
} snd_seq_port_info_t;


/* queue information */
typedef struct {
	int queue;			/* queue id */

	/*
	 *  security settings, only owner of this queue can start/stop timer
	 *  etc. if the queue is locked for other clients
	 */
	int owner;			/* client id for owner of the queue */
	int locked:1;			/* timing queue locked for other queues */

	char name[64];			/* name of this queue */
	char reserved[64];		/* for future use */
} snd_seq_queue_info_t;

typedef snd_seq_queue_info_t snd_seq_queue_owner_t; /* alias */


/* queue flags */
#define SND_SEQ_QUEUE_FLG_SYNC_LOST	(1<<0)	/* synchronization was lost */

/* queue info/status */
typedef struct {
	int queue;			/* queue id */
	int events;			/* read-only - queue size */

	snd_seq_tick_time_t tick;	/* current tick */
	snd_seq_real_time_t time;	/* current time */
			
	int running;			/* running state of queue */		

	int flags;			/* various flags */

	char reserved[64];		/* for the future */
} snd_seq_queue_status_t;


/* queue tempo */
typedef struct {
	int queue;			/* sequencer queue */
	unsigned int tempo;		/* current tempo, us/tick */
	int ppq;			/* time resolution, ticks/quarter */
	char reserved[32];		/* for the future */
} snd_seq_queue_tempo_t;


/* sequencer timer sources */
#define SND_SEQ_TIMER_MASTER		0	/* master timer */
#define SND_SEQ_TIMER_SLAVE		1	/* slave timer */
#define SND_SEQ_TIMER_MIDI_CLOCK	2	/* Midi Clock (CLOCK event) */
#define SND_SEQ_TIMER_MIDI_TICK		3	/* Midi Timer Tick (TICK event) */

/* queue timer info */
typedef struct {
	int queue;			/* sequencer queue */

	/* source timer selection */
	int type;			/* timer type */
	int slave;			/* timer slave type */
	int number;			/* timer number/identification */
	int resolution;			/* timer resolution in Hz */

	/* MIDI timer parameters */
	int midi_client;		/* sequencer client */
	int midi_port;			/* sequencer port */

	/* tick & real-time queue synchronization */
	long int sync_tick_resolution;	/* resolution per 10ms midi tick (TICK event) (ticks * 1000000) */
	long int sync_real_resolution;	/* resolution per midi tick (CLOCK event) or zero (nanoseconds) */

	char reserved[64];		/* for the future use */
} snd_seq_queue_timer_t;


/* synchronization types */
#define SND_SEQ_SYNC_NONE		0	/* none synchronization */
#define SND_SEQ_SYNC_MTC		1	/* Midi Time Code synchronization */

typedef struct {
	int queue;			/* sequencer queue */

	/* synchronization */
	int sync_client;		/* sequencer client */
	int sync_port;			/* sequencer port */
	int sync_type;			/* synchronization type */

	snd_seq_tick_time_t sync_tick;	/* last synchronization tick */
	snd_seq_real_time_t sync_time;	/* last synchronization time */

	char reserved[64];		/* for the future use */
} snd_seq_queue_sync_t;


typedef struct {
	int queue;			/* sequencer queue */
	int client;			/* sequencer client */
	int used;			/* queue is used with this client (must be set for accepting events) */
	/* per client watermarks */
	char reserved[64];		/* for future use */
} snd_seq_queue_client_t;


typedef struct {
	snd_seq_addr_t sender;		/* sender address */
	snd_seq_addr_t dest;		/* destination address */
	unsigned char queue;		/* input time-stamp queue (optional) */
	int exclusive: 1,		/* exclusive mode */
	    realtime: 1,		/* realtime timestamp */
	    convert_time: 1;		/* convert timestamp */
	int midi_channels;		/* midi channels setup, zero = do not care */
	int midi_voices;		/* midi voices setup, zero = do not care */
	int synth_voices;		/* synth voices setup, zero = do not care */
	char reserved[32];		/* for future use */
} snd_seq_port_subscribe_t;

/* type of query subscription */
#define SND_SEQ_QUERY_SUBS_READ		0
#define SND_SEQ_QUERY_SUBS_WRITE	1

typedef struct {
	int client;
	int port;
	int type;	/* READ or WRITE */
	int index;	/* 0..N-1 */
	int num_subs;	/* R/O: number of subscriptions on this port */
	snd_seq_addr_t addr;	/* R/O: result */
	unsigned char queue;	/* R/O: result */
	int exclusive: 1;	/* R/O: result */
	int realtime: 1;	/* R/O: result */
	int convert_time: 1;	/* R/O: result */
	char reserved[64];	/* for future use */
} snd_seq_query_subs_t;


/*
 *  Instrument abstraction layer
 *     - based on events
 */

/* instrument types */
#define SND_SEQ_INSTR_ATYPE_DATA	0	/* instrument data */
#define SND_SEQ_INSTR_ATYPE_ALIAS	1	/* instrument alias */

/* instrument ASCII identifiers */
#define SND_SEQ_INSTR_ID_DLS1		"DLS1"
#define SND_SEQ_INSTR_ID_DLS2		"DLS2"
#define SND_SEQ_INSTR_ID_SIMPLE		"Simple Wave"
#define SND_SEQ_INSTR_ID_SOUNDFONT	"SoundFont"
#define SND_SEQ_INSTR_ID_GUS_PATCH	"GUS Patch"
#define SND_SEQ_INSTR_ID_INTERWAVE	"InterWave FFFF"
#define SND_SEQ_INSTR_ID_OPL2		"OPL2"
#define SND_SEQ_INSTR_ID_OPL3		"OPL3"
#define SND_SEQ_INSTR_ID_OPL4		"OPL4"

/* instrument types */
#define SND_SEQ_INSTR_TYPE0_DLS1	(1<<0)	/* MIDI DLS v1 */
#define SND_SEQ_INSTR_TYPE0_DLS2	(1<<1)	/* MIDI DLS v2 */
#define SND_SEQ_INSTR_TYPE1_SIMPLE	(1<<0)	/* Simple Wave */
#define SND_SEQ_INSTR_TYPE1_SOUNDFONT	(1<<1)	/* EMU SoundFont */
#define SND_SEQ_INSTR_TYPE1_GUS_PATCH	(1<<2)	/* Gravis UltraSound Patch */
#define SND_SEQ_INSTR_TYPE1_INTERWAVE	(1<<3)	/* InterWave FFFF */
#define SND_SEQ_INSTR_TYPE2_OPL2	(1<<0)	/* Yamaha OPL2 */
#define SND_SEQ_INSTR_TYPE2_OPL3	(1<<1)	/* Yamaha OPL3 */
#define SND_SEQ_INSTR_TYPE2_OPL4	(1<<2)	/* Yamaha OPL4 */

/* put commands */
#define SND_SEQ_INSTR_PUT_CMD_CREATE	0
#define SND_SEQ_INSTR_PUT_CMD_REPLACE	1
#define SND_SEQ_INSTR_PUT_CMD_MODIFY	2
#define SND_SEQ_INSTR_PUT_CMD_ADD	3
#define SND_SEQ_INSTR_PUT_CMD_REMOVE	4

/* get commands */
#define SND_SEQ_INSTR_GET_CMD_FULL	0
#define SND_SEQ_INSTR_GET_CMD_PARTIAL	1

/* query flags */
#define SND_SEQ_INSTR_QUERY_FOLLOW_ALIAS (1<<0)

/* free commands */
#define SND_SEQ_INSTR_FREE_CMD_ALL	0
#define SND_SEQ_INSTR_FREE_CMD_PRIVATE	1
#define SND_SEQ_INSTR_FREE_CMD_CLUSTER	2
#define SND_SEQ_INSTR_FREE_CMD_SINGLE	3

/* instrument data */
typedef struct {
	char name[32];			/* instrument name */
	char reserved[16];		/* for the future use */
	int type;			/* instrument type */
	union {
		char format[16];	/* format identifier */
		snd_seq_instr_t alias;
	} data;
} snd_seq_instr_data_t;

/* INSTR_PUT, data are stored in one block (extended or IPC), header + data */

typedef struct {
	snd_seq_instr_t id;		/* instrument identifier */
	int cmd;			/* put command */
	char reserved[16];		/* for the future */
	long len;			/* real instrument data length (without header) */
	snd_seq_instr_data_t data;	/* instrument data */
} snd_seq_instr_put_t;

/* INSTR_GET, data are stored in one block (extended or IPC), header + data */

typedef struct {
	snd_seq_instr_t id;		/* instrument identifier */
	unsigned int flags;		/* query flags */
	int cmd;			/* query command */
	char reserved[16];		/* reserved for the future use */
	long len;			/* real instrument data length (without header) */
} snd_seq_instr_get_t;

typedef struct {
	int result;			/* operation result */
	snd_seq_instr_t id;		/* requested instrument identifier */
	char reserved[16];		/* reserved for the future use */
	long len;			/* real instrument data length (without header) */
	snd_seq_instr_data_t data;	/* instrument data */
} snd_seq_instr_get_result_t;

/* INSTR_FREE */

typedef struct {
	char reserved[16];		/* for the future use */
	unsigned int cmd;               /* SND_SEQ_INSTR_FREE_CMD_* */
	union {
		snd_seq_instr_t instr;
		snd_seq_instr_cluster_t cluster;
		char data8[16];
	} data;
} snd_seq_instr_free_t;

/* INSTR_INFO */

typedef struct {
	int result;			/* operation result */
	unsigned int formats[8];	/* bitmap of supported formats */
	int ram_count;			/* count of RAM banks */
	long ram_sizes[16];		/* size of RAM banks */
	int rom_count;			/* count of ROM banks */
	long rom_sizes[8];		/* size of ROM banks */
	char reserved[128];
}  snd_seq_instr_info_t;

/* INSTR_STATUS */

typedef struct {
	int result;			/* operation result */
	long free_ram[16];		/* free RAM in banks */
	int instrument_count;		/* count of downloaded instruments */
	char reserved[128];
} snd_seq_instr_status_t;

/* INSTR_CLUSTER_SET */

typedef struct {
	snd_seq_instr_cluster_t cluster;  /* cluster identifier */
	char name[32];			/* cluster name */
	int priority;			/* cluster priority */
	char reserved[64];		/* for the future use */
} snd_seq_instr_cluster_set_t;

/* INSTR_CLUSTER_GET */

typedef struct {
	snd_seq_instr_cluster_t cluster;  /* cluster identifier */
	char name[32];			/* cluster name */
	int priority;			/* cluster priority */
	char reserved[64];		/* for the future use */
} snd_seq_instr_cluster_get_t;

/*
 *  IOCTL commands
 */

#define SND_SEQ_IOCTL_PVERSION		_IOR ('S', 0x00, int)
#define SND_SEQ_IOCTL_CLIENT_ID		_IOR ('S', 0x01, int)
#define SND_SEQ_IOCTL_SYSTEM_INFO	_IOWR('S', 0x02, snd_seq_system_info_t)

#define SND_SEQ_IOCTL_GET_CLIENT_INFO	_IOWR('S', 0x10, snd_seq_client_info_t)
#define SND_SEQ_IOCTL_SET_CLIENT_INFO	_IOW ('S', 0x11, snd_seq_client_info_t)

#define SND_SEQ_IOCTL_CREATE_PORT	_IOWR('S', 0x20, snd_seq_port_info_t)
#define SND_SEQ_IOCTL_DELETE_PORT	_IOW ('S', 0x21, snd_seq_port_info_t)
#define SND_SEQ_IOCTL_GET_PORT_INFO	_IOWR('S', 0x22, snd_seq_port_info_t)
#define SND_SEQ_IOCTL_SET_PORT_INFO	_IOW ('S', 0x23, snd_seq_port_info_t)

#define SND_SEQ_IOCTL_SUBSCRIBE_PORT	_IOW ('S', 0x30, snd_seq_port_subscribe_t)
#define SND_SEQ_IOCTL_UNSUBSCRIBE_PORT	_IOW ('S', 0x31, snd_seq_port_subscribe_t)

#define SND_SEQ_IOCTL_CREATE_QUEUE	_IOWR('S', 0x32, snd_seq_queue_info_t)
#define SND_SEQ_IOCTL_DELETE_QUEUE	_IOW ('S', 0x33, snd_seq_queue_info_t)
#define SND_SEQ_IOCTL_GET_QUEUE_INFO	_IOWR('S', 0x34, snd_seq_queue_info_t)
#define SND_SEQ_IOCTL_SET_QUEUE_INFO	_IOWR('S', 0x35, snd_seq_queue_info_t)
#define SND_SEQ_IOCTL_GET_NAMED_QUEUE	_IOWR('S', 0x36, snd_seq_queue_info_t)
#define SND_SEQ_IOCTL_GET_QUEUE_STATUS	_IOWR('S', 0x40, snd_seq_queue_status_t)
#define SND_SEQ_IOCTL_GET_QUEUE_TEMPO	_IOWR('S', 0x41, snd_seq_queue_tempo_t)
#define SND_SEQ_IOCTL_SET_QUEUE_TEMPO	_IOW ('S', 0x42, snd_seq_queue_tempo_t)
#define SND_SEQ_IOCTL_GET_QUEUE_OWNER	_IOWR('S', 0x43, snd_seq_queue_owner_t)
#define SND_SEQ_IOCTL_SET_QUEUE_OWNER	_IOW ('S', 0x44, snd_seq_queue_owner_t)
#define SND_SEQ_IOCTL_GET_QUEUE_TIMER   _IOWR('S', 0x45, snd_seq_queue_timer_t)
#define SND_SEQ_IOCTL_SET_QUEUE_TIMER	_IOW ('S', 0x46, snd_seq_queue_timer_t)
#define SND_SEQ_IOCTL_GET_QUEUE_SYNC	_IOWR('S', 0x47, snd_seq_queue_sync_t)
#define SND_SEQ_IOCTL_SET_QUEUE_SYNC	_IOW ('S', 0x48, snd_seq_queue_sync_t)
#define SND_SEQ_IOCTL_GET_QUEUE_CLIENT	_IOWR('S', 0x49, snd_seq_queue_client_t)
#define SND_SEQ_IOCTL_SET_QUEUE_CLIENT	_IOW ('S', 0x4a, snd_seq_queue_client_t)
#define SND_SEQ_IOCTL_GET_CLIENT_POOL	_IOWR('S', 0x4b, snd_seq_client_pool_t)
#define SND_SEQ_IOCTL_SET_CLIENT_POOL	_IOW ('S', 0x4c, snd_seq_client_pool_t)
#define SND_SEQ_IOCTL_REMOVE_EVENTS	_IOW ('S', 0x4e, snd_seq_remove_events_t)
#define SND_SEQ_IOCTL_QUERY_SUBS	_IOWR('S', 0x4f, snd_seq_query_subs_t)
#define SND_SEQ_IOCTL_GET_SUBSCRIPTION	_IOWR('S', 0x50, snd_seq_port_subscribe_t)
#define SND_SEQ_IOCTL_QUERY_NEXT_CLIENT	_IOWR('S', 0x51, snd_seq_client_info_t)
#define SND_SEQ_IOCTL_QUERY_NEXT_PORT	_IOWR('S', 0x52, snd_seq_port_info_t)

#endif /* __SND_ASEQUENCER_H */
