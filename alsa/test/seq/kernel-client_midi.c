/*
 *   Kernel client MIDI driver for ALSA sequencer
 *   Copyright (c) 1998 by Frank van de Pol <frank@vande-pol.demon.nl>
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

#define SND_MAIN_OBJECT_FILE
#include "driver.h"
#include "midi.h"
#include "seq.h"



/* uncomment next line to bounce received midi data directly to the output (ie. MIDI thru) */
/*#define TEST */



/* data for this midi synth driver */
typedef struct {
	int seq_client;
	int seq_port;	
	snd_rawmidi_t *rmidi;
} seq_midisynth_t;






/* fill standard header data, source port & channel are filled in */
static void snd_seq_midi_setheader(snd_seq_event_t * ev, int port, int channel)
{
	ev->flags = SND_SEQ_EVENT_LENGTH_FIXED;

	ev->source.queue = -1;
	ev->source.client = -1;	/* myid */
	ev->source.port = port;
	ev->source.channel = channel;

	ev->dest.queue = SND_SEQ_ADDRESS_SUBSCRIBERS;
	ev->dest.client = -1;
	ev->dest.port = -1;
	ev->dest.channel = channel;

	ev->flags = SND_SEQ_TIME_STAMP_REAL | SND_SEQ_TIME_MODE_REL;
	ev->time.real.tv_sec = 0;
	ev->time.real.tv_nsec = 0;

#ifdef TEST
	ev->dest.queue = 0;
	ev->dest.client = 1;
	ev->dest.port = port;
#endif
}



/*
 * the ALSA low-level midi routines appear to return 'whole' midi events and 
 * have already handled midi running state. I don't know if will remain in 
 * the future. If not, some more elaborate MIDI parser is needed.
 */
static void snd_midi_command(snd_rawmidi_t * rmidi, void *cmd_private_data, unsigned char *command, int count)
{
	seq_midisynth_t *msynth = (seq_midisynth_t*) cmd_private_data;
	int channel;
	int port;
	int client;

	if (msynth == NULL) {
		snd_printk("msynth == NULL\n");
		return;
	}	
	port = msynth->seq_port;
	client = msynth->seq_client;
	channel = command[0] & 0x0f;

	switch (command[0] & 0xf0) {

		case 0x80:	// note off

			if (count == 3) {
				snd_seq_event_t ev;

				snd_seq_midi_setheader(&ev, port, channel);

				ev.type = SND_SEQ_EVENT_NOTEOFF;
				ev.data.note.note = command[1];
				ev.data.note.velocity = command[2];

				snd_seq_kernel_client_enqueue(client, &ev);
				return;
			}
			break;

		case 0x90:	// note on

			if (count == 3) {
				snd_seq_event_t ev;

				snd_seq_midi_setheader(&ev, port, channel);

				ev.type = SND_SEQ_EVENT_NOTEON;
				ev.data.note.note = command[1];
				ev.data.note.velocity = command[2];

				snd_seq_kernel_client_enqueue(client, &ev);
				return;
			}
			break;

		case 0xa0:	// poly key pressure

			if (count == 3) {
				snd_seq_event_t ev;

				snd_seq_midi_setheader(&ev, port, channel);

				ev.type = SND_SEQ_EVENT_KEYPRESS;
				ev.data.control.param = command[1];
				ev.data.control.value = command[2];

				snd_seq_kernel_client_enqueue(client, &ev);
				return;
			}
			break;

		case 0xb0:	// control change

			if (count == 3) {
				snd_seq_event_t ev;

				snd_seq_midi_setheader(&ev, port, channel);

				ev.type = SND_SEQ_EVENT_CONTROLLER;
				ev.data.control.param = command[1];
				ev.data.control.value = command[2];

				snd_seq_kernel_client_enqueue(client, &ev);
				return;
			}
			break;

		case 0xc0:	// program change

			if (count == 2) {
				snd_seq_event_t ev;

				snd_seq_midi_setheader(&ev, port, channel);

				ev.type = SND_SEQ_EVENT_PGMCHANGE;
				ev.data.control.value = command[1];

				snd_seq_kernel_client_enqueue(client, &ev);
				return;
			}
			break;

		case 0xd0:	// channel pressure

			if (count == 2) {
				snd_seq_event_t ev;

				snd_seq_midi_setheader(&ev, port, channel);

				ev.type = SND_SEQ_EVENT_CHANPRESS;
				ev.data.control.value = command[1];

				snd_seq_kernel_client_enqueue(client, &ev);
				return;
			}
			break;

		case 0xe0:	// pitch bender

			if (count == 3) {
				snd_seq_event_t ev;

				snd_seq_midi_setheader(&ev, port, channel);

				ev.type = SND_SEQ_EVENT_PITCHBEND;
				ev.data.control.value = (command[1] & 0x7f) + ((command[2] & 0x7f) << 7) - 8192;

				snd_seq_kernel_client_enqueue(client, &ev);
				return;
			}
			break;

		case 0xf0:
			switch (command[0]) {
				case 0xf0:	/* sysex */
					{
						snd_seq_event_t ev;

						snd_seq_midi_setheader(&ev, port, channel);
						ev.flags = (ev.flags & ~SND_SEQ_EVENT_LENGTH_MASK) | SND_SEQ_EVENT_LENGTH_VARIABLE;
						ev.type = SND_SEQ_EVENT_SYSEX;
						ev.data.ext.ptr = snd_seq_ext_malloc(count);
						ev.data.ext.len = count;
						if (ev.data.ext.ptr) {
							memcpy(ev.data.ext.ptr, command, count);
							snd_seq_kernel_client_enqueue(client, &ev);
						} else {
							snd_printk("failed to get %d bytes for sysex\n", count);
						}
						return;
					}

				case 0xf1:	/* MTC quarter frame */
					if (count == 2) {
						snd_seq_event_t ev;

						snd_seq_midi_setheader(&ev, port, channel);

						ev.type = SND_SEQ_EVENT_QFRAME;
						ev.data.control.value = command[1];

						snd_seq_kernel_client_enqueue(client, &ev);
						return;
					}
					break;

				case 0xf2:	/* song position */
					if (count == 3) {
						snd_seq_event_t ev;

						snd_seq_midi_setheader(&ev, port, channel);

						ev.type = SND_SEQ_EVENT_SONGPOS;
						ev.data.control.value = (command[1] & 0x7f) + ((command[2] & 0x7f) << 7);

						snd_seq_kernel_client_enqueue(client, &ev);
						return;
					}
					break;

				case 0xf3:	/* song select */
					if (count == 2) {
						snd_seq_event_t ev;

						snd_seq_midi_setheader(&ev, port, channel);

						ev.type = SND_SEQ_EVENT_SONGSEL;
						ev.data.control.value = command[1];

						snd_seq_kernel_client_enqueue(client, &ev);
						return;
					}
					break;

				case 0xf4:	/* undefined */
					return;

				case 0xf5:	/* undefined */
					return;

				case 0xf6:	/* tune request */
					snd_printk("Rx: tune request\n");
					return;

				case 0xf7:	/* end of sysex */
					return;

					// system real-time messages

				case 0xf8:	/* timing clock */
					if (count == 1) {
						snd_seq_event_t ev;

						snd_seq_midi_setheader(&ev, port, channel);

						ev.type = SND_SEQ_EVENT_CLOCK;
						snd_seq_kernel_client_enqueue(client, &ev);
						return;
					}
				case 0xfa:	/* start */
					if (count == 1) {
						snd_seq_event_t ev;

						snd_seq_midi_setheader(&ev, port, channel);

						ev.type = SND_SEQ_EVENT_START;
						snd_seq_kernel_client_enqueue(client, &ev);
						return;
					}
				case 0xfb:	// continue

					if (count == 1) {
						snd_seq_event_t ev;

						snd_seq_midi_setheader(&ev, port, channel);

						ev.type = SND_SEQ_EVENT_CONTINUE;
						snd_seq_kernel_client_enqueue(client, &ev);
						return;
					}
				case 0xfc:	// stop

					if (count == 1) {
						snd_seq_event_t ev;

						snd_seq_midi_setheader(&ev, port, channel);

						ev.type = SND_SEQ_EVENT_STOP;
						snd_seq_kernel_client_enqueue(client, &ev);
						return;
					}
				case 0xfd:	/* undefined */
					return;

				case 0xfe:	// active sensing

					if (count == 1) {
						snd_seq_event_t ev;

						snd_seq_midi_setheader(&ev, port, channel);

						ev.type = SND_SEQ_EVENT_HEARTBEAT;
						snd_seq_kernel_client_enqueue(client, &ev);
						return;
					}
				case 0xff:	// system reset

					snd_printk("Rx: system reset\n");
					return;
			}
	}

	/* not a legal MIDI sequence.... */
	snd_printk("rx command '%s': ", rmidi->name);
	while (count-- > 0)
		printk("%02x:", *command++);
	printk("\n");
}



/* send data to specified midi device */
static void dump_midi(snd_rawmidi_t * rmidi, unsigned char *buf, int count)
{
	int done = snd_midi_transmit(rmidi, buf, count);

	if (done != count) {
		snd_printk("only wrote %d instead of %d bytes to midi device\n", done, count);
	}
}


static int event_process_midi(snd_seq_event_t * ev, void *private_data)
{
	seq_midisynth_t *msynth = (seq_midisynth_t*) private_data;
	int channel = ev->dest.channel;
	static unsigned char msg[10];	/* buffer for constructing midi messages */
	static unsigned char running_state;	/* FIXME: should be in struct passed to function so we can handle multiple ports */
	int client;
	snd_rawmidi_t *rmidi;

	if (msynth == NULL) {
		snd_printk("msynth == NULL\n");
		return 1;
	}	


	client = msynth->seq_client;
	rmidi = msynth->rmidi;




	/* decode actual event data... */
	switch (ev->type) {
		case SND_SEQ_EVENT_NOTE:
			/* note event with specified length, first trigger note on */
			msg[0] = (channel & 0x0f) | 0x90;	/* note on */
			msg[1] = ev->data.note.note & 0x7f;
			msg[2] = ev->data.note.velocity & 0x7f;
			if (running_state == msg[0]) {
				dump_midi(rmidi, msg + 1, 2);
			} else {
				running_state = msg[0];
				dump_midi(rmidi, msg, 3);
			}

			/* enqueue note off event */
			switch (ev->flags & SND_SEQ_TIME_STAMP_MASK) {
				case SND_SEQ_TIME_STAMP_TICK:
					ev->time.tick = ev->data.note.duration;
					break;
				case SND_SEQ_TIME_STAMP_REAL:
					ev->time.real.tv_sec = ev->data.note.duration / 1000;	/* unit for duration is ms */
					ev->time.real.tv_nsec = 1E6 * (ev->data.note.duration % 1000);
					break;
			}
			ev->flags = (ev->flags & ~SND_SEQ_TIME_MODE_MASK) | SND_SEQ_TIME_MODE_REL;
			ev->type = SND_SEQ_EVENT_NOTEOFF;
			snd_seq_kernel_client_enqueue(client, ev);
			break;


		case SND_SEQ_EVENT_NOTEOFF:
			msg[0] = (channel & 0x0f) | 0x80;	/* note off */
			msg[1] = ev->data.note.note & 0x7f;
			msg[2] = ev->data.note.velocity & 0x7f;
			if (running_state == msg[0]) {
				dump_midi(rmidi, msg + 1, 2);
			} else {
				running_state = msg[0];
				dump_midi(rmidi, msg, 3);
			}
			break;

		case SND_SEQ_EVENT_NOTEON:
			msg[0] = (channel & 0x0f) | 0x90;	/* note on */
			msg[1] = ev->data.note.note & 0x7f;
			msg[2] = ev->data.note.velocity & 0x7f;
			if (running_state == msg[0]) {
				dump_midi(rmidi, msg + 1, 2);
			} else {
				running_state = msg[0];
				dump_midi(rmidi, msg, 3);
			}
			break;

		case SND_SEQ_EVENT_KEYPRESS:
			msg[0] = (channel & 0x0f) | 0xa0;	/* polyphonic key pressure */
			msg[1] = ev->data.control.param & 0x7f;
			msg[2] = ev->data.control.value & 0x7f;
			if (running_state == msg[0]) {
				dump_midi(rmidi, msg + 1, 2);
			} else {
				running_state = msg[0];
				dump_midi(rmidi, msg, 3);
			}
			break;

		case SND_SEQ_EVENT_CONTROLLER:
			msg[0] = (channel & 0x0f) | 0xb0;	/* control change */
			msg[1] = ev->data.control.param & 0x7f;
			msg[2] = ev->data.control.value & 0x7f;
			if (running_state == msg[0]) {
				dump_midi(rmidi, msg + 1, 2);
			} else {
				running_state = msg[0];
				dump_midi(rmidi, msg, 3);
			}
			break;

		case SND_SEQ_EVENT_PGMCHANGE:
			msg[0] = (channel & 0x0f) | 0xc0;	/* program change */
			msg[1] = ev->data.control.value & 0x7f;
			if (running_state == msg[0]) {
				dump_midi(rmidi, msg + 1, 1);
			} else {
				running_state = msg[0];
				dump_midi(rmidi, msg, 2);
			}
			break;

		case SND_SEQ_EVENT_CHANPRESS:
			msg[0] = (channel & 0x0f) | 0xd0;	/* channel pressure */
			msg[1] = ev->data.control.value & 0x7f;
			if (running_state == msg[0]) {
				dump_midi(rmidi, msg + 1, 1);
			} else {
				running_state = msg[0];
				dump_midi(rmidi, msg, 2);
			}
			break;

		case SND_SEQ_EVENT_PITCHBEND:
			msg[0] = (channel & 0x0f) | 0xe0;	/* pitch bender */
			msg[1] = (ev->data.control.value + 8192) & 0x7f;	/* lsb */
			msg[2] = ((ev->data.control.value + 8192) >> 7) & 0x7f;		/* msb */
			if (running_state == msg[0]) {
				dump_midi(rmidi, msg + 1, 2);
			} else {
				running_state = msg[0];
				dump_midi(rmidi, msg, 3);
			}
			break;



		case SND_SEQ_EVENT_SYSEX:{
				unsigned char *sysex = (unsigned char *) ev->data.ext.ptr;

				//printf("Event  = System Exclusive len=%d\n", ev->data.ext.len);
				dump_midi(rmidi, sysex, ev->data.ext.len);
				snd_seq_ext_free(sysex, ev->data.ext.len);
				running_state = 0;
			}
			break;


		case SND_SEQ_EVENT_QFRAME:
			msg[0] = 0xf1;	/* MTC quarter frame */
			msg[1] = ev->data.control.value & 0x7f;
			dump_midi(rmidi, msg, 2);
			running_state = 0;
			break;

		case SND_SEQ_EVENT_CLOCK:
			msg[0] = 0xf8;
			dump_midi(rmidi, msg, 1);
			running_state = 0;
			break;

		case SND_SEQ_EVENT_START:
			msg[0] = 0xfa;
			dump_midi(rmidi, msg, 1);
			running_state = 0;
			break;

		case SND_SEQ_EVENT_CONTINUE:
			msg[0] = 0xfb;
			dump_midi(rmidi, msg, 1);
			running_state = 0;
			break;

		case SND_SEQ_EVENT_STOP:
			msg[0] = 0xfc;
			dump_midi(rmidi, msg, 1);
			running_state = 0;
			break;

		case SND_SEQ_EVENT_HEARTBEAT:
			msg[0] = 0xfe;	/* active sensing */
			dump_midi(rmidi, msg, 1);
			running_state = 0;
			break;


			/* messages which we are sending over the MIDI buss */
		case SND_SEQ_EVENT_TEMPO:
		case SND_SEQ_EVENT_TIMESIGN:
		case SND_SEQ_EVENT_KEYSIGN:
			break;


		default:
			snd_printk("Event  = Decoding for type %d is not implemented\n", ev->type);
	}
	return 1;
}


/* call-back function for event input */
static int event_input(snd_seq_event_t * ev, void *private_data)
{
	seq_midisynth_t *msynth = (seq_midisynth_t*) private_data;
	int port = ev->dest.port;	/* use for different devices... */

	if (msynth == NULL) {
		snd_printk("msynth == NULL\n");
		return 1;
	}	

	if (port == msynth->seq_port) {
		/* event is to be send to MIDI port */
		event_process_midi(ev, private_data);
	}
	/* FIXME: How to handle situation where the MIDI output queue is full??? 
	   should we sleep for a while, or simply not process the event (return failure code to 
	   higher level sequencer (<-- last option) */

	return 1;		/* success */
}



/*
 *  INIT PART
 */

 
static seq_midisynth_t msynth;


int init_module(void)
{
	int client;
	snd_seq_client_callback_t callbacks;
	snd_seq_client_info_t inf;
	snd_seq_port_info_t port;

	callbacks.input = event_input;

	client = snd_seq_register_kernel_client(&callbacks, &msynth);

	/* set our client name */
	strcpy(inf.name, "Kernel MIDI client");
	snd_seq_kernel_client_ctl(client, SND_SEQ_IOCTL_SET_CLIENT_INFO, &inf);
	

	/* open midi port */
	if ((snd_midi_open(0, 0, SND_RAWMIDI_LFLG_OUTPUT | SND_RAWMIDI_LFLG_INPUT, &msynth.rmidi)) < 0) {
		snd_printk("midi open failed!!!\n");
		return -EINVAL;
	}
	
	/* attach input handler */
	msynth.rmidi->input.u.p.command = snd_midi_command;
	msynth.rmidi->input.u.p.cmd_private_data = &msynth;
	snd_midi_start_input(msynth.rmidi);

	/* declare port */
	strcpy(port.name, msynth.rmidi->name);
	port.capability = SND_SEQ_PORT_CAP_MIDI_OUT | SND_SEQ_PORT_CAP_SYNC_OUT |
		SND_SEQ_PORT_CAP_MIDI_IN | SND_SEQ_PORT_CAP_SYNC_IN |
		SND_SEQ_PORT_CAP_SUBSCRIPTION;
	port.port_type = SND_SEQ_PORT_TYPE_MIDI_GENERIC;
	snd_seq_kernel_client_ctl(client, SND_SEQ_IOCTL_CREATE_PORT, &port);
	
	msynth.seq_client = client;
	msynth.seq_port = port.port;


	/* add midi thru for testing.... */
	{
		/* subscribe for events from the midi input device */
		snd_seq_port_subscribe_t subs;

		subs.sender.client = client;	/* this MIDI driver */
		subs.sender.port = msynth.seq_port;

		subs.dest.queue = 0;	/* use this queue for timestamps */
		subs.dest.client = client;	/* this MIDI driver */
		subs.dest.port = msynth.seq_port;
		
		snd_seq_kernel_client_ctl(client, SND_SEQ_IOCTL_SUBSCRIBE_PORT, &subs);
	}

	return 0;
}


void cleanup_module(void)
{
	snd_seq_unregister_kernel_client(msynth.seq_client);

	snd_midi_flush_output(msynth.rmidi);
	snd_midi_close(0, 0, SND_RAWMIDI_LFLG_OUTPUT | SND_RAWMIDI_LFLG_INPUT);
}
