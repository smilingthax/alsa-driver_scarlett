/*
 *  ALSA sequencer test program, packet decoder
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


#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include "seq.h"

int decode_event(snd_seq_event_t * ev)
{
	printf("Type   = %d\n", ev->type);
	printf("Flags  = 0x%02x\n", ev->flags);
	switch (ev->flags & SND_SEQ_TIME_STAMP_MASK) {
		case SND_SEQ_TIME_STAMP_TICK:
			printf("Time   = %d ticks\n",
			       ev->time.tick);
			break;
		case SND_SEQ_TIME_STAMP_REAL:
			printf("Time   = %d.%09d\n",
			       ev->time.real.sec,
			       ev->time.real.nsec);
			break;
	}
	printf("Source = %d.%d.%d.%d\n",
	       ev->source.queue,
	       ev->source.client,
	       ev->source.port,
	       ev->source.channel);
	printf("Dest   = %d.%d.%d.%d\n",
	       ev->dest.queue,
	       ev->dest.client,
	       ev->dest.port,
	       ev->dest.channel);

	/* decode actual event data... */
	switch (ev->type) {
		case SND_SEQ_EVENT_NOTE:
			printf("Event  = Note note=%d, velocity=%d, duration=%d\n",
			       ev->data.note.note,
			       ev->data.note.velocity,
			       ev->data.note.duration);
			break;

		case SND_SEQ_EVENT_NOTEON:
			printf("Event  = Note On note=%d, velocity=%d\n",
			       ev->data.note.note,
			       ev->data.note.velocity);
			break;

		case SND_SEQ_EVENT_NOTEOFF:
			printf("Event  = Note Off note=%d, velocity=%d\n",
			       ev->data.note.note,
			       ev->data.note.velocity);
			break;

		case SND_SEQ_EVENT_SYSEX:{
				unsigned char *sysex = (unsigned char *) ev + sizeof(snd_seq_event_t);
				int c;

				printf("Event  = System Exclusive len=%d\n",
				       ev->data.ext.len);

				for (c = 0; c < ev->data.ext.len; c++) {
					printf("    %3d : %02x\n", c, sysex[c]);
				}
			}
			break;

		default:
			printf("Event  = Decoding for type %d is not implemented\n",
			       ev->type);
	}


	switch (ev->flags & SND_SEQ_EVENT_LENGTH_MASK) {
		case SND_SEQ_EVENT_LENGTH_FIXED:
			return sizeof(snd_seq_event_t);

		case SND_SEQ_EVENT_LENGTH_VARIABLE:
			return sizeof(snd_seq_event_t) + ev->data.ext.len;
	}
}




#define BUFSIZE 4096

void main(void)
{
	int fd;
	snd_seq_client_info_t inf;
	char *name;
	int c;
	fd_set reads, writes;
	int have_data;
	unsigned char buf[BUFSIZE];


	fd = open("/dev/sndseq", O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		perror("open /dev/sndseq");
		exit(1);
	}
	if (ioctl(fd, SND_SEQ_IOCTL_CLIENT_ID, &c) < 0) {
		perror("ioctl");
		exit(1);
	}
	printf("My client id = %d\n", c);


	/* set name */
	memset(&inf, 0, sizeof(snd_seq_client_info_t));
	strcpy(inf.name, "Event decoder");
	if (ioctl(fd, SND_SEQ_IOCTL_SET_CLIENT_INFO, &inf) < 0) {
		perror("ioctl");
		exit(1);
	}

	have_data = 0;
	while (1) {
		struct timeval time;
		int written;


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
			printf("  ### Read %d [%d]\n", rd, BUFSIZE);

			while (rd > 0) {
				int processed;

				processed = decode_event((snd_seq_event_t *) ev);

				rd -= processed;
				ev += processed;
			}
		}
#ifdef 0
		if (FD_ISSET(fd, &writes)) {
			ev.type = SND_SEQ_EVENT_NOTEON;

			ev.dest.queue = 1;
			ev.dest.client = 0;	/* broadcast ! */
			ev.dest.port = 2;
			ev.dest.channel = 3;

			ev.data.note.note = 60;
			ev.data.note.velocity = 100;

			written = write(fd, &ev, sizeof(ev));
			printf("written = %d\n", written);

			have_data = 0;
		}
#endif
	}

	exit(0);
}
