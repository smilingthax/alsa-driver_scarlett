/*
 *  Test for ALSA sequencer, show current time
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

 /* simple client that displays the sequencer's current time */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <curses.h>
#include <string.h>
#include "seq.h"

int seqfd;			/* file descriptor for ALSA sequencer */


/* write event to ALSA sequencer */
void write_ev(snd_seq_event_t * ev)
{
	int written;

	ev->flags &= ~SND_SEQ_EVENT_LENGTH_MASK;
	ev->flags |= SND_SEQ_EVENT_LENGTH_FIXED;

	written = 0;
	while (!written) {
		written = write(seqfd, ev, sizeof(*ev));
		if (!written)
			usleep(0.05E6);
	}
}



void main(void)
{
	char *name;
	int myid;
	snd_seq_client_info_t inf;
	snd_seq_queue_info_t queue_info;
	int keypress;
	int queue = 1;

	seqfd = open("/dev/sndseq", O_RDWR);

	if (ioctl(seqfd, SND_SEQ_IOCTL_CLIENT_ID, &myid) < 0) {
		perror("ioctl");
		exit(1);
	}
	printf("My client id = %d\n", myid);

	/* set name */
	memset(&inf, 0, sizeof(snd_seq_client_info_t));
	strcpy(inf.name, "Time Display");
	if (ioctl(seqfd, SND_SEQ_IOCTL_SET_CLIENT_INFO, &inf) < 0) {
		perror("ioctl");
		exit(1);
	}

	/* curses init.. */
	initscr();
	cbreak();
	noecho();
	timeout(0);
	keypad(stdscr, TRUE);
	scrollok(stdscr, TRUE);
	clear();

	keypress = getch();
	while (keypress != 'q') {
		queue_info.queue = queue;
		if (ioctl(seqfd, SND_SEQ_IOCTL_GET_QUEUE_INFO, &queue_info) < 0) {
			perror("ioctl");
			break;
		}
		mvprintw(3, 5, "   A L S A    S E Q U E N C E R   \n");

		mvprintw(5, 5, "Queue  : %d   ", queue_info.queue);
		mvprintw(6, 5, "State  : %s", queue_info.running ? "Running" : "Stopped");
		mvprintw(7, 5, "Time   : %d.%09d s          ", queue_info.time.sec, queue_info.time.nsec);
		mvprintw(8, 5, "Tick   : %d        ", queue_info.tick);

		mvprintw(10, 5, "PPQ    : %d ticks/beat        ", queue_info.ppq);
		mvprintw(11, 5, "Tempo  : %d us/beat           ", queue_info.tempo);
		mvprintw(12, 5, "BPM    : %.2f beats/minute       ", 60.0E6 / (double) queue_info.tempo);

		mvprintw(20, 5, "q - quit");
		mvprintw(21, 5, "up/down : change tempo");

		wrefresh(stdscr);

		/* vary update rate depending on running state of timer */
		if (queue_info.running) {
			usleep(0.05E6);
		} else {
			usleep(0.5E6);
		}

		keypress = getch();
		switch (keypress) {
			case KEY_UP:
				{
					snd_seq_event_t ev;

					/* and send tempo change event to the sequencer.... */
					ev.source.port = 1;
					ev.source.channel = 1;

					ev.dest.queue = 1;
					ev.dest.client = 255;	/* broadcast */
					ev.dest.port = 0;
					ev.dest.channel = 0;	/* don't care */

					ev.flags = SND_SEQ_TIME_STAMP_TICK | SND_SEQ_TIME_MODE_ABS;
					ev.time.tick = 0;	/* now */

					ev.type = SND_SEQ_EVENT_TEMPO;
					
					ev.data.control.value = (60.0E6 / ((60.0E6 / (double) queue_info.tempo) + 1.0));

					write_ev(&ev);

				}
				break;

			case KEY_DOWN:
				{
					snd_seq_event_t ev;

					/* and send tempo change event to the sequencer.... */
					ev.source.port = 1;
					ev.source.channel = 1;

					ev.dest.queue = 1;
					ev.dest.client = 255;	/* broadcast */
					ev.dest.port = 0;
					ev.dest.channel = 0;	/* don't care */

					ev.flags = SND_SEQ_TIME_STAMP_TICK | SND_SEQ_TIME_MODE_ABS;
					ev.time.tick = 0;	/* now */

					ev.type = SND_SEQ_EVENT_TEMPO;
					
					ev.data.control.value = (60.0E6 / ((60.0E6 / (double) queue_info.tempo) - 1.0));

					write_ev(&ev);

				}
				break;
		}
	}

	endwin();
	close(seqfd);
}
