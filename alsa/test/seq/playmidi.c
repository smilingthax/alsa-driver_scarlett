/*
 *   MIDI file player for ALSA sequencer 
 *   (type 0 only!, the library that is used doesn't support merging of tracks)
 *
 *   Copyright (c) 1998 by Frank van de Pol <F.K.W.van.de.Pol@inter.nl.net>
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

/* define this if you want to send real-time time stamps instead of midi ticks to the ALSA sequencer */
/*#define USE_REALTIME */


#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "midifile.h"		/* SMF library */

#include "seq.h"		/* alsa sequencer */


FILE *F;
int seqfd;			/* file descriptor for ALSA sequencer */
int ppq = 96;

double local_secs = 0;
int local_ticks = 0;
int local_tempo = 500000;

extern void alsa_start_timer(void);
extern void alsa_stop_timer(void);


static inline double tick2time_dbl(int tick)
{
	return local_secs + ((double) (tick - local_ticks) * (double) local_tempo * 1.0E-6 / (double) ppq);
}

#ifdef USE_REALTIME
static void tick2time(snd_seq_real_time_t * tm, int tick)
{
	double secs = tick2time_dbl(tick);

	//double secs = ((double) tick * (double) local_tempo * 1.0E-6 / (double) ppq);

	tm->sec = secs;
	tm->nsec = (secs - tm->sec) * 1.0E9;

	//printf("secs = %lf  = %d.%03d\n", secs, tm->sec, tm->nsec);
}

#endif


/* sleep until sequencer has reached specified timestamp, to guard that we play too much events ahead */
void sleep_seq(int tick)
{
	snd_seq_queue_info_t queue_info;
	
	if (tick < 0)
		return;
		
	queue_info.queue = 1;	/* queue we're using */
	if (ioctl(seqfd, SND_SEQ_IOCTL_GET_QUEUE_INFO, &queue_info) < 0) {
		perror("ioctl");
	}
	
	// print overruns
	if (queue_info.tick >= (tick + ppq)) {
		printf("sleep %d %d  diff=%d\n",queue_info.tick, tick, tick-queue_info.tick);
	}
	while (queue_info.tick < tick) {
		usleep(0.2E6);
		if (ioctl(seqfd, SND_SEQ_IOCTL_GET_QUEUE_INFO, &queue_info) < 0) {
			perror("ioctl");
			break;
		}
		//printf("      %d %d\n",queue_info.tick, tick);
	}
}


/* write event to ALSA sequencer */
void write_ev_im(snd_seq_event_t * ev)
{
	int written;

	ev->flags &= ~SND_SEQ_EVENT_LENGTH_MASK;
	ev->flags |= SND_SEQ_EVENT_LENGTH_FIXED;
	

	written = 0;
	while (!written) {
		written = write(seqfd, ev, sizeof(*ev));
		if (!written)
			sleep(1);
	}
}

/* write event to ALSA sequencer */
void write_ev(snd_seq_event_t * ev)
{
	sleep_seq(ev->time.tick-ppq);
	write_ev_im(ev);
}

/* write variable length event to ALSA sequencer */
void write_ev_var(snd_seq_event_t * ev, int len, void *ptr)
{
	int bytes;
	int written;
	unsigned char *buf;

	//sleep_seq(ev->time.tick+ppq);

	ev->flags &= ~SND_SEQ_EVENT_LENGTH_MASK;
	ev->flags |= SND_SEQ_EVENT_LENGTH_VARIABLE;
	ev->data.ext.len = len;
	ev->data.ext.ptr = NULL;

	/* create interim buffer for storing header + event data */
	bytes = sizeof(snd_seq_event_t) + len;
	buf = malloc(bytes);
	if (buf == NULL) {
		fprintf(stderr, "malloc failed for variable event length\n");

		/* forget about this event... */
		return;
	}
	/* fill interim buffer */
	memcpy(buf, ev, sizeof(snd_seq_event_t));
	memcpy(buf + sizeof(snd_seq_event_t), ptr, len);

	written = 0;
	while (!written) {
		written = write(seqfd, buf, bytes);
		if (!written)
			sleep(1);
	}

	free(buf);
}


int mygetc(void)
{
	return (getc(F));
}

void mytext(int type, int leng, char *msg)
{
	char *p;
	char *ep = msg + leng;

	for (p = msg; p < ep; p++)
		putchar(isprint(*p) ? *p : '?');
	putchar('\n');
}

void do_header(int format, int ntracks, int division)
{
	printf("smf format %d, %d tracks, %d ppq\n", format, ntracks, division);
	ppq = division;

	if ((format != 0) || (ntracks != 1)) {
		printf("This player does not support merging of tracks.\n");
		alsa_stop_timer();
		exit(1);
	}
	/* set ppq */
	{
		snd_seq_queue_info_t queue_info;

		queue_info.queue = 1;	/* queue we're using */
		queue_info.ppq = ppq;
		queue_info.tempo = -1;	/* don't change */
		if (ioctl(seqfd, SND_SEQ_IOCTL_SET_QUEUE_INFO, &queue_info) < 0) {
			perror("ioctl");
			exit(1);
		}
		printf("ALSA Timer updated, PPQ = %d\n", queue_info.ppq);
	}

	/* start playing... */
	alsa_start_timer();
}

void do_tempo(int us)
{
	double bpm;
	snd_seq_event_t ev;

	bpm = 60.0E6 / (double) us;

	printf("tempo = %d us/beat\n", us);
	printf("tempo = %.2f bpm\n", bpm);

	/* store new tempo and timestamp of tempo change */
	local_secs = tick2time_dbl(Mf_currtime);
	local_ticks = Mf_currtime;
	local_tempo = us;


	/* and send tempo change event to the sequencer.... */
	ev.source.port = 1;
	ev.source.channel = 1;

	ev.dest.queue = 1;
	ev.dest.client = 255;	/* broadcast */
	ev.dest.port = 0;
	ev.dest.channel = 0;	/* don't care */

#ifdef USE_REALTIME
	ev.flags = SND_SEQ_TIME_STAMP_REAL | SND_SEQ_TIME_MODE_ABS;
	tick2time(&ev.time.real, Mf_currtime);
#else
	ev.flags = SND_SEQ_TIME_STAMP_TICK | SND_SEQ_TIME_MODE_ABS;
	ev.time.tick = Mf_currtime;
#endif

	ev.type = SND_SEQ_EVENT_TEMPO;
	ev.data.control.value = us;

	write_ev_im(&ev);

}

void do_noteon(int chan, int pitch, int vol)
{
	snd_seq_event_t ev;

	ev.source.port = 1;
	ev.source.channel = 1;

	ev.dest.queue = 1;
	ev.dest.client = 1;
	ev.dest.port = 1;
	ev.dest.channel = chan;

#ifdef USE_REALTIME
	ev.flags = SND_SEQ_TIME_STAMP_REAL | SND_SEQ_TIME_MODE_ABS;
	tick2time(&ev.time.real, Mf_currtime);
#else
	ev.flags = SND_SEQ_TIME_STAMP_TICK | SND_SEQ_TIME_MODE_ABS;
	ev.time.tick = Mf_currtime;
#endif

	ev.type = SND_SEQ_EVENT_NOTEON;
	ev.data.note.note = pitch;
	ev.data.note.velocity = vol;

	write_ev(&ev);

}


void do_noteoff(int chan, int pitch, int vol)
{
	snd_seq_event_t ev;

	ev.source.port = 1;
	ev.source.channel = 1;

	ev.dest.queue = 1;
	ev.dest.client = 1;
	ev.dest.port = 1;
	ev.dest.channel = chan;

#ifdef USE_REALTIME
	ev.flags = SND_SEQ_TIME_STAMP_REAL | SND_SEQ_TIME_MODE_ABS;
	tick2time(&ev.time.real, Mf_currtime);
#else
	ev.flags = SND_SEQ_TIME_STAMP_TICK | SND_SEQ_TIME_MODE_ABS;
	ev.time.tick = Mf_currtime;
#endif

	ev.type = SND_SEQ_EVENT_NOTEOFF;
	ev.data.note.note = pitch;
	ev.data.note.velocity = vol;

	write_ev(&ev);
}


void do_program(int chan, int program)
{
	snd_seq_event_t ev;

	ev.source.port = 1;
	ev.source.channel = 1;

	ev.dest.queue = 1;
	ev.dest.client = 1;
	ev.dest.port = 1;
	ev.dest.channel = chan;

#ifdef USE_REALTIME
	ev.flags = SND_SEQ_TIME_STAMP_REAL | SND_SEQ_TIME_MODE_ABS;
	tick2time(&ev.time.real, Mf_currtime);
#else
	ev.flags = SND_SEQ_TIME_STAMP_TICK | SND_SEQ_TIME_MODE_ABS;
	ev.time.tick = Mf_currtime;
#endif

	ev.type = SND_SEQ_EVENT_PGMCHANGE;
	ev.data.control.value = program;

	write_ev_im(&ev);
}


void do_parameter(int chan, int control, int value)
{
	snd_seq_event_t ev;

	ev.source.port = 1;
	ev.source.channel = 1;

	ev.dest.queue = 1;
	ev.dest.client = 1;
	ev.dest.port = 1;
	ev.dest.channel = chan;

#ifdef USE_REALTIME
	ev.flags = SND_SEQ_TIME_STAMP_REAL | SND_SEQ_TIME_MODE_ABS;
	tick2time(&ev.time.real, Mf_currtime);
#else
	ev.flags = SND_SEQ_TIME_STAMP_TICK | SND_SEQ_TIME_MODE_ABS;
	ev.time.tick = Mf_currtime;
#endif

	ev.type = SND_SEQ_EVENT_CONTROLLER;
	ev.data.control.param = control;
	ev.data.control.value = value;

	write_ev(&ev);
}


void do_pitchbend(int chan, int lsb, int msb)
{				/* !@#$% lsb & msb are in wrong order in docs */
	snd_seq_event_t ev;

	ev.source.port = 1;
	ev.source.channel = 1;

	ev.dest.queue = 1;
	ev.dest.client = 1;
	ev.dest.port = 1;
	ev.dest.channel = chan;

#ifdef USE_REALTIME
	ev.flags = SND_SEQ_TIME_STAMP_REAL | SND_SEQ_TIME_MODE_ABS;
	tick2time(&ev.time.real, Mf_currtime);
#else
	ev.flags = SND_SEQ_TIME_STAMP_TICK | SND_SEQ_TIME_MODE_ABS;
	ev.time.tick = Mf_currtime;
#endif

	ev.type = SND_SEQ_EVENT_PITCHBEND;
	ev.data.control.value = (lsb + (msb << 7)) - 8192;

	write_ev(&ev);
}

void do_pressure(int chan, int pitch, int pressure)
{
	snd_seq_event_t ev;

	ev.source.port = 1;
	ev.source.channel = 1;

	ev.dest.queue = 1;
	ev.dest.client = 1;
	ev.dest.port = 1;
	ev.dest.channel = chan;

#ifdef USE_REALTIME
	ev.flags = SND_SEQ_TIME_STAMP_REAL | SND_SEQ_TIME_MODE_ABS;
	tick2time(&ev.time.real, Mf_currtime);
#else
	ev.flags = SND_SEQ_TIME_STAMP_TICK | SND_SEQ_TIME_MODE_ABS;
	ev.time.tick = Mf_currtime;
#endif

	ev.type = SND_SEQ_EVENT_KEYPRESS;
	ev.data.control.param = pitch;
	ev.data.control.value = pressure;

	write_ev(&ev);
}

void do_chanpressure(int chan, int pressure)
{
	snd_seq_event_t ev;

	ev.source.port = 1;
	ev.source.channel = 1;

	ev.dest.queue = 1;
	ev.dest.client = 1;
	ev.dest.port = 1;
	ev.dest.channel = chan;

#ifdef USE_REALTIME
	ev.flags = SND_SEQ_TIME_STAMP_REAL | SND_SEQ_TIME_MODE_ABS;
	tick2time(&ev.time.real, Mf_currtime);
#else
	ev.flags = SND_SEQ_TIME_STAMP_TICK | SND_SEQ_TIME_MODE_ABS;
	ev.time.tick = Mf_currtime;
#endif

	ev.type = SND_SEQ_EVENT_CHANPRESS;
	ev.data.control.value = pressure;

	write_ev(&ev);
}

void do_sysex(int len, char *msg)
{
	snd_seq_event_t ev;

#if 0
	int c;

	printf("Sysex, len=%d\n", len);
	for (c = 0; c < len; c++) {
		printf("    %3d : %02x\n", c, (unsigned char) msg[c]);
	}
#endif


	ev.source.port = 1;
	ev.source.channel = 1;

	ev.dest.queue = 1;
	ev.dest.client = 1;
	ev.dest.port = 1;
	ev.dest.channel = 0;	/* don't care */

#ifdef USE_REALTIME
	ev.flags = SND_SEQ_TIME_STAMP_REAL | SND_SEQ_TIME_MODE_ABS;
	tick2time(&ev.time.real, Mf_currtime);
#else
	ev.flags = SND_SEQ_TIME_STAMP_TICK | SND_SEQ_TIME_MODE_ABS;
	ev.time.tick = Mf_currtime;
#endif

	ev.type = SND_SEQ_EVENT_SYSEX;

	write_ev_var(&ev, len, msg);
}

/* start timer */
void alsa_start_timer(void)
{
	snd_seq_event_t ev;

	ev.source.port = 0;
	ev.source.channel = 0;

	ev.dest.queue = 1;
	ev.dest.client = 0;	/* system */
	ev.dest.client = 255;	/* broadcast */
	ev.dest.port = 0;	/* timer */
	ev.dest.channel = 0;	/* don't care */

	ev.flags = SND_SEQ_TIME_STAMP_REAL | SND_SEQ_TIME_MODE_REL;
	ev.time.real.sec = 0;
	ev.time.real.nsec = 0;

	ev.type = SND_SEQ_EVENT_START;

	write_ev_im(&ev);
	usleep(0.1E6);
}

/* stop timer */
void alsa_stop_timer(void)
{

	snd_seq_event_t ev;

	ev.source.port = 0;
	ev.source.channel = 0;

	ev.dest.queue = 1;
	ev.dest.client = 0;	/* system */
	ev.dest.client = 255;	/* broadcast */
	ev.dest.port = 0;	/* timer */
	ev.dest.channel = 0;	/* don't care */

#ifdef USE_REALTIME
	ev.flags = SND_SEQ_TIME_STAMP_REAL | SND_SEQ_TIME_MODE_ABS;
	tick2time(&ev.time.real, Mf_currtime);
#else
	ev.flags = SND_SEQ_TIME_STAMP_TICK | SND_SEQ_TIME_MODE_ABS;
	ev.time.tick = Mf_currtime;
#endif

	ev.type = SND_SEQ_EVENT_STOP;

	write_ev_im(&ev);
}

int main(int argc, char *argv[])
{
	char *name;

#ifdef USE_REALTIME
	printf("ALSA MIDI Player, feeding events to real-time queue\n");
#else
	printf("ALSA MIDI Player, feeding events to song queue\n");
#endif

	/* open sequencer device */
	seqfd = open("/dev/sndseq", O_RDWR);
	if (seqfd < 0) {
		perror("open /dev/sndseq");
		exit(1);
	}
	/* set client name */
	name = "MIDI file player";
	if (ioctl(seqfd, SND_SEQ_IOCTL_SET_CLIENT_NAME, name) < 0) {
		perror("ioctl");
		exit(1);
	}
	if (argc > 1)
		F = fopen(argv[1], "r");
	else
		F = stdin;

	Mf_header = do_header;
	Mf_tempo = do_tempo;
	Mf_getc = mygetc;
	Mf_text = mytext;

	Mf_noteon = do_noteon;
	Mf_noteoff = do_noteoff;
	Mf_program = do_program;
	Mf_parameter = do_parameter;
	Mf_pitchbend = do_pitchbend;
	Mf_pressure = do_pressure;
	Mf_chanpressure = do_chanpressure;
	Mf_sysex = do_sysex;


	/* stop timer in case it was left running by a previous client */
	{
		snd_seq_event_t ev;

		ev.source.port = 0;
		ev.source.channel = 0;

		ev.dest.queue = 1;
		ev.dest.client = 0;	/* system */
		ev.dest.client = 255;	/* broadcast */
		ev.dest.port = 0;	/* timer */
		ev.dest.channel = 0;	/* don't care */

		ev.flags = SND_SEQ_TIME_STAMP_REAL | SND_SEQ_TIME_MODE_REL;
		ev.time.real.sec = 0;
		ev.time.real.nsec = 0;

		ev.type = SND_SEQ_EVENT_STOP;

		write_ev(&ev);
	}

	/* go.. go.. go.. */
	mfread();

	alsa_stop_timer();

	close(seqfd);

	printf("Stopping at %lf s,  tick %d\n", tick2time_dbl(Mf_currtime + 1), Mf_currtime + 1);

	exit(0);
}
