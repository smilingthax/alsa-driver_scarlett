/*
 *  Main header file for the ALSA sequencer
 *  Copyright (c) 1998 by Frank van de Pol <frank@vande-pol.demon.nl>
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
#define SND_SEQ_VERSION SND_PROTOCOL_VERSION (0, 0, 1)

/*                                   	*/
/* definion of sequencer event types 	*/
/*                                   	*/

	/* system messages */
#define SND_SEQ_EVENT_SYSTEM		0
#define SND_SEQ_EVENT_RESULT		1

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
#define SND_SEQ_EVENT_TICK		27	/* midi Real Time Tick message */
	
#define SND_SEQ_EVENT_TEMPO		30	/* (SMF) Tempo event */
#define SND_SEQ_EVENT_TIMESIGN		31	/* SMF Time Signature event */
#define SND_SEQ_EVENT_KEYSIGN		32	/* SMF Key Signature event */
	        
#define SND_SEQ_EVENT_SYSEX		40	/* system exclusive data (variable length) */

#define SND_SEQ_EVENT_HEARTBEAT		50	/* "active sensing" event */
#define SND_SEQ_EVENT_ECHO		51	/* echo event */

	/* system status messages */
#define SND_SEQ_EVENT_CLIENT_START	60	/* new client has connected */
#define SND_SEQ_EVENT_CLIENT_EXIT	61	/* client has left the system */
#define SND_SEQ_EVENT_CLIENT_CHANGE	62	/* client status/info has changed */
#define SND_SEQ_EVENT_PORT_START	63	/* new port was created */
#define SND_SEQ_EVENT_PORT_EXIT		64	/* port was deleted from system */
#define SND_SEQ_EVENT_PORT_CHANGE	65	/* port status/info has changed */

	/* synthesizer events */	
#define SND_SEQ_EVENT_SAMPLE		80	/* sample select */
#define SND_SEQ_EVENT_SAMPLE_START	81	/* voice start */
#define SND_SEQ_EVENT_SAMPLE_STOP	82	/* voice stop */
#define SND_SEQ_EVENT_SAMPLE_FREQ	83	/* playback frequency */
#define SND_SEQ_EVENT_SAMPLE_VOLUME	84	/* volume and balance */
#define SND_SEQ_EVENT_SAMPLE_LOOP	85	/* sample loop */
#define SND_SEQ_EVENT_SAMPLE_POSITION	86	/* sample position */

	/* instrument layer */
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

	/* hardware specific events - range 192-255 */

typedef unsigned char snd_seq_event_type;


	/* event address */
typedef struct {
	unsigned char queue;	/* Sequencer queue:       0..255, 255 = broadcast to all queues */
	unsigned char client;	/* Client number:         0..255, 255 = broadcast to all clients */
	unsigned char port;	/* Port within client:    0..255, 255 = broadcast to all ports */
	unsigned char channel;	/* Channel within client: 0..255, 255 = broadcast to all channels */
} snd_seq_addr_t;

#define SND_SEQ_ADDRESS_UNKNOWN		253	/* uknown source */
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

#define SND_SEQ_DEST_QUEUE		(0<<5)	/* normal destonation */
#define SND_SEQ_DEST_DIRECT		(1<<5)	/* bypass enqueing */
#define SND_SEQ_DEST_MASK		(1<<5)

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
	unsigned int std;
	unsigned short bank;
	unsigned short prg;
} snd_seq_instr_t;


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


	/* sequencer event */
typedef struct snd_seq_event_t {
	snd_seq_event_type type;	/* event type */
	unsigned char flags;		/* event flags */
	char unused1,
	     unused2;
	
	/* schedule time */
	union {
		snd_seq_tick_time_t tick;
		snd_seq_real_time_t real;
	} time;
			
	snd_seq_addr_t source;	/* source address */
	snd_seq_addr_t dest;	/* destination address */

	union {			/* event data... */
		snd_seq_ev_note note;
		snd_seq_ev_ctrl control;
		snd_seq_ev_raw8 raw8;
		snd_seq_ev_raw32 raw32;
		snd_seq_ev_ext ext;
		snd_seq_ev_ipcshm ipcshm;
		union {
			snd_seq_tick_time_t tick;
			snd_seq_real_time_t real;
		} time;
		snd_seq_addr_t addr;
		snd_seq_result_t result;
		snd_seq_ev_instr_begin_t instr_begin;
	} data;
} snd_seq_event_t;


typedef struct {
	int queues;			/* maximum queues count */
	int clients;			/* maximum clients count */
	int ports;			/* maximum ports per client */
	int channels;			/* maximum channels per port */
	char reserved[32];
} snd_seq_system_info_t;


	/* known client numbers */
#define SND_SEQ_CLIENT_SYSTEM		0
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
	char reserved[64];		/* for future use */
} snd_seq_client_info_t;


/* client pool size */
typedef struct {
	int client;			/* client number to inquire */
	int output_pool;		/* outgoing (write) pool size */
	int input_pool;			/* incoming (read) pool size */
	int output_room;		/* minimum free pool size for select/blocking mode */
	char reserved[64];
} snd_seq_client_pool_t;

	/* known port numbers */
#define SND_SEQ_PORT_SYSTEM_TIMER	0
#define SND_SEQ_PORT_SYSTEM_ANNOUNCE	1

	/* port capabilities (32 bits) */
#define SND_SEQ_PORT_CAP_IN		(1<<0)
#define SND_SEQ_PORT_CAP_OUT		(1<<1)

#define SND_SEQ_PORT_CAP_SYNC_IN	(1<<2)
#define SND_SEQ_PORT_CAP_SYNC_OUT	(1<<3)

#define SND_SEQ_PORT_CAP_DUPLEX		(1<<4)

#define SND_SEQ_PORT_CAP_SUBSCRIPTION	(1<<5)

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

typedef struct {
	int client;			/* client number */
	int port;			/* port number */
	char name[64];			/* port name */

	unsigned int capability;	/* port capability bits */
	unsigned int type;		/* port type bits */
	int midi_channels;		/* channels per MIDI port */
	int synth_voices;		/* voices per SYNTH port */

	int out_use;			/* R/O: subscribers for output (this port->sequencer) */
	int in_use;			/* R/O: subscribers for input (sequencer->this port) */

	void *kernel;			/* reserved for kernel use (must be NULL) */

	char reserved[64];		/* for future use */
} snd_seq_port_info_t;


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


/* queue owner */
typedef struct {
	int queue;			/* sequencer queue */

	/*
	 *  security settings, only owner of this queue can start/stop timer
	 *  etc. if the queue is locked for other clients
	 */
	int locked:1;			/* timing queue locked for other queues */
	int owner;			/* client id for owner of the queue */

	char reserved[32];		/* for the future use */
} snd_seq_queue_owner_t;


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
	int exclusive: 1,		/* exclusive mode */
	    realtime: 1;		/* realtime timestamp */
	char reserved[32];		/* for future use */
} snd_seq_port_subscribe_t;


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
	unsigned int type;              /* SND_SEQ_INSTR_FTYPE_* */
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

#endif /* __SND_ASEQUENCER_H */
