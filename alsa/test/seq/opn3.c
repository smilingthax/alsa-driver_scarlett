/*
 *  Simple test for ALSA sequencer
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include "seq.h"

void main(void)
{
	int f;
	char *name;
	int myid;
	snd_seq_client_info_t inf;

	f = open("/dev/sndseq", O_RDWR);

	if (ioctl(f, SND_SEQ_IOCTL_CLIENT_ID, &myid) < 0) {
		perror("ioctl");
		exit(1);
	}
	printf("My client id = %d\n", myid);

	/* set name */
	memset(&inf, 0, sizeof(snd_seq_client_info_t));
	strcpy(inf.name, "Test program");
	if (ioctl(f, SND_SEQ_IOCTL_SET_CLIENT_INFO, &inf) < 0) {
		perror("ioctl");
		exit(1);
	}
	


/*      
   inf.client = 1;
   if (ioctl(f, SND_SEQ_IOCTL_CLIENT_INFO, &inf) < 0) {
   perror("ioctl");
   exit(1);
   }

   printf("client = %d\n",inf.client);
   printf("type   = %d\n",inf.type);
   printf("name   = %s\n",inf.name);

 */

	{
		snd_seq_event_t ev;
		int written;

		ev.type = SND_SEQ_EVENT_NOTEON;

		ev.source.queue = -1;
		ev.source.client = -1;	/* myid */
		ev.source.port = 1;
		ev.source.channel = 1;

		ev.dest.queue = 1;
		ev.dest.client = 255;	/* broadcast ! */
		ev.dest.port = 0;
		ev.dest.channel = 0;

#if 0
		ev.flags = SND_SEQ_TIME_STAMP_REAL | SND_SEQ_TIME_MODE_ABS;
		ev.time.real.tv_sec = 4;
		ev.time.real.tv_nsec = 1;
#endif

		ev.flags = SND_SEQ_TIME_STAMP_REAL | SND_SEQ_TIME_MODE_REL;
		ev.time.real.tv_sec = 0;
		ev.time.real.tv_nsec = 0;
		ev.type = SND_SEQ_EVENT_START;

		written = write(f, &ev, sizeof(ev));
		printf("written = %d\n", written);


		ev.flags = SND_SEQ_TIME_STAMP_REAL | SND_SEQ_TIME_MODE_ABS;
		ev.time.real.tv_sec = 2;
		ev.time.real.tv_nsec = 0;

		ev.type = SND_SEQ_EVENT_NOTEON;
		ev.data.note.note = 60;
		ev.data.note.velocity = 100;

		written = write(f, &ev, sizeof(ev));
		printf("written = %d\n", written);


		ev.flags = SND_SEQ_TIME_STAMP_REAL | SND_SEQ_TIME_MODE_ABS;
		ev.time.real.tv_sec = 6;
		ev.time.real.tv_nsec = 0;

		ev.type = SND_SEQ_EVENT_NOTEOFF;
		ev.data.note.note = 60;
		ev.data.note.velocity = 100;

		written = write(f, &ev, sizeof(ev));
		printf("written = %d\n", written);



		ev.flags = SND_SEQ_TIME_STAMP_REAL | SND_SEQ_TIME_MODE_ABS;
		ev.time.real.tv_sec = 10;
		ev.time.real.tv_nsec = 0;
		ev.type = SND_SEQ_EVENT_STOP;
		
		written = write(f, &ev, sizeof(ev));
		printf("written = %d\n", written);

		

		ev.flags = SND_SEQ_TIME_STAMP_REAL | SND_SEQ_TIME_MODE_ABS;
		ev.time.real.tv_sec = 1;
		ev.time.real.tv_nsec = 0;
		ev.type = SND_SEQ_EVENT_CLOCK;
		
		written = write(f, &ev, sizeof(ev));
		printf("written = %d\n", written);


	}


	sleep(1);
	close(f);
}
