/*
 *  ALSA sequencer test program, MIDI driver
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

/*
 * this program send received events out to the midi port. It can thus be
 * seen as a super simple user-land synth driver client. 
 */


#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>		/* real-time task */

#include "seq.h"		/* ALSA sequencer */


void dump_midi(int fd, unsigned char *buf, int count)
{
	int wr = 0;

	while (count > 0) {
		wr = write(fd, buf, count);
		count -= wr;
		buf += wr;
	}
}



int play_event(int midi_fd, snd_seq_event_t * ev)
{
	//int port = ev->dest.port;     /* use for different devices... */
	int channel = ev->dest.channel;
	static unsigned char msg[10];	/* buffer for constructing midi messages */
	static unsigned char running_state;

	/* decode actual event data... */
	switch (ev->type) {
		case SND_SEQ_EVENT_NOTE:
			printf("Event  = Note note=%d, velocity=%d, duration=%d\n",
			       ev->data.note.note,
			       ev->data.note.velocity,
			       ev->data.note.duration);
			break;

		case SND_SEQ_EVENT_NOTEON:
			msg[0] = (channel & 0x0f) | 0x90;	/* note on */
			msg[1] = ev->data.note.note & 0x7f;
			msg[2] = ev->data.note.velocity & 0x7f;
			if (running_state == msg[0]) {
				dump_midi(midi_fd, msg + 1, 2);
			} else {
				running_state = msg[0];
				dump_midi(midi_fd, msg, 3);
			}
			break;

		case SND_SEQ_EVENT_NOTEOFF:
			msg[0] = (channel & 0x0f) | 0x80;	/* note on */
			msg[1] = ev->data.note.note & 0x7f;
			msg[2] = ev->data.note.velocity & 0x7f;
			if (running_state == msg[0]) {
				dump_midi(midi_fd, msg + 1, 2);
			} else {
				running_state = msg[0];
				dump_midi(midi_fd, msg, 3);
			}
			break;

		case SND_SEQ_EVENT_PGMCHANGE:
			msg[0] = (channel & 0x0f) | 0xc0;	/* program change */
			msg[1] = ev->data.control.value & 0x7f;
			if (running_state == msg[0]) {
				dump_midi(midi_fd, msg + 1, 1);
			} else {
				running_state = msg[0];
				dump_midi(midi_fd, msg, 2);
			}
			break;

		case SND_SEQ_EVENT_PITCHBEND:
			msg[0] = (channel & 0x0f) | 0xe0;	/* pitch bender */
			msg[1] = (ev->data.control.value + 8192) & 0x7f;	/* lsb */
			msg[2] = ((ev->data.control.value + 8192) >> 7) & 0x7f;		/* msb */
			if (running_state == msg[0]) {
				dump_midi(midi_fd, msg + 1, 2);
			} else {
				running_state = msg[0];
				dump_midi(midi_fd, msg, 3);
			}
			break;

		case SND_SEQ_EVENT_CONTROLLER:
			msg[0] = (channel & 0x0f) | 0xb0;	/* control change */
			msg[1] = ev->data.control.param & 0x7f;
			msg[2] = ev->data.control.value & 0x7f;
			if (running_state == msg[0]) {
				dump_midi(midi_fd, msg + 1, 2);
			} else {
				running_state = msg[0];
				dump_midi(midi_fd, msg, 3);
			}
			break;


		case SND_SEQ_EVENT_CLOCK:
			msg[0] = 0xf8;
			dump_midi(midi_fd, msg, 1);
			running_state = 0;
			break;

		case SND_SEQ_EVENT_START:
			msg[0] = 0xfa;
			dump_midi(midi_fd, msg, 1);
			running_state = 0;
			break;

		case SND_SEQ_EVENT_CONTINUE:
			msg[0] = 0xfb;
			dump_midi(midi_fd, msg, 1);
			running_state = 0;
			break;

		case SND_SEQ_EVENT_STOP:
			msg[0] = 0xfc;
			dump_midi(midi_fd, msg, 1);
			running_state = 0;
			break;


		case SND_SEQ_EVENT_SYSEX:{
				unsigned char *sysex = (unsigned char *) ev + sizeof(snd_seq_event_t);

				//printf("Event  = System Exclusive len=%d\n", ev->data.ext.len);
				dump_midi(midi_fd, sysex, ev->data.ext.len);
				running_state = 0;
			}
			break;


		default:
			printf("Event  = Decoding for type %d is not implemented\n", ev->type);
	}


	switch (ev->flags & SND_SEQ_EVENT_LENGTH_MASK) {

		case SND_SEQ_EVENT_LENGTH_VARIABLE:
			return sizeof(snd_seq_event_t) + ev->data.ext.len;

		case SND_SEQ_EVENT_LENGTH_FIXED:
		default:
			return sizeof(snd_seq_event_t);
	}
}



/* buffer for receiving events */
#define BUFSIZE 4096

void main(void)
{
	int fd;

	snd_seq_client_info_t inf;
	char *name;
	int my_id;
	fd_set reads, writes;
	int have_data;
	int midi_fd;
	snd_seq_port_info_t port;
	unsigned char buf[BUFSIZE];

	/* open sequencer device */
	fd = open("/dev/sndseq", O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		perror("open /dev/sndseq");
		exit(1);
	}
	/* open midi device */
	midi_fd = open("/dev/sndmidi00", O_RDWR);
	if (fd < 0) {
		perror("open /dev/sndmidi00");
		exit(1);
	}
	if (ioctl(fd, SND_SEQ_IOCTL_CLIENT_ID, &my_id) < 0) {
		perror("ioctl");
		exit(1);
	}
	printf("My client id = %d\n", my_id);

	/* set name */
	memset(&inf, 0, sizeof(snd_seq_client_info_t));
	strcpy(inf.name, "Gravis Ultrasound MAX");
	if (ioctl(fd, SND_SEQ_IOCTL_SET_CLIENT_INFO, &inf) < 0) {
		perror("ioctl");
		exit(1);
	}
	
	strcpy(port.name, "GUS MIDI");
	port.capability = SND_SEQ_PORT_CAP_MIDI_OUT | SND_SEQ_PORT_CAP_SYNC_OUT |
		SND_SEQ_PORT_CAP_MIDI_IN | SND_SEQ_PORT_CAP_SYNC_IN;
	port.port_type = SND_SEQ_PORT_TYPE_MIDI_GENERIC;
	if (ioctl(fd, SND_SEQ_IOCTL_CREATE_PORT, &port) < 0) {
		perror("ioctl");
		exit(1);
	}
	/* and some non-existing devices */
	strcpy(port.name, "GUS Synth");
	port.capability = SND_SEQ_PORT_CAP_MIDI_OUT;
	port.port_type = SND_SEQ_PORT_TYPE_MIDI_GM | SND_SEQ_PORT_TYPE_MIDI_XG;
	if (ioctl(fd, SND_SEQ_IOCTL_CREATE_PORT, &port) < 0) {
		perror("ioctl");
		exit(1);
	}
	/* and some non-existing devices */
	strcpy(port.name, "GUS Mixer");
	port.capability = SND_SEQ_PORT_CAP_MIDI_OUT;
	port.port_type = SND_SEQ_PORT_TYPE_MIDI_GENERIC;
	if (ioctl(fd, SND_SEQ_IOCTL_CREATE_PORT, &port) < 0) {
		perror("ioctl");
		exit(1);
	}
	/* and some non-existing devices */
	strcpy(port.name, "GUS Audio");
	port.capability = SND_SEQ_PORT_CAP_SYNC_OUT;
	port.port_type = 0;
	if (ioctl(fd, SND_SEQ_IOCTL_CREATE_PORT, &port) < 0) {
		perror("ioctl");
		exit(1);
	}
	/* now go for real, we request to be run as a real-time task */
	{
		struct sched_param schedParam;

		memset(&schedParam, 0, sizeof(struct sched_param));

		schedParam.sched_priority = 10;
		if (sched_setscheduler(0, SCHED_RR, &schedParam) != 0) {
			perror("failed setting priority");
		} else {
			printf("Running with round-robin (RT) scheduling\n");
		}
	}

	have_data = 0;
	while (1) {
		struct timeval time;

		FD_ZERO(&reads);
		FD_ZERO(&writes);

		if (have_data)
			FD_SET(fd, &writes);
		FD_SET(fd, &reads);

		time.tv_sec = 1;
		time.tv_usec = 0;
		if (select(fd + 1, &reads, &writes, NULL, &time) == -1) {
			perror("select");
			exit(-1);
		}
		if (FD_ISSET(fd, &reads)) {
			int rd;
			unsigned char *ev = buf;

			rd = read(fd, ev, BUFSIZE);
			//printf("  ### Read %4d [%4d]\n", rd, BUFSIZE);
			while (rd > 0) {
				int processed;

				processed = play_event(midi_fd, (snd_seq_event_t *) ev);

				rd -= processed;
				ev += processed;
			}
		}
#ifdef 0
		if (FD_ISSET(fd, &writes)) {
			int wr;
		}
#endif
	}

	exit(0);
}
