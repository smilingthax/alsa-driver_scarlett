///////////////////////////////////////////////////////////////////////////
//      MOTU Midi Timepiece ALSA Header
//      Copyright by Michael T. Mayers 1999
//      Thanks to John Galbraith
///////////////////////////////////////////////////////////////////////////
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either version 2 of the License, or
//      (at your option) any later version.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
///////////////////////////////////////////////////////////////////////////

#include "../include/driver.h"
#include "../include/initval.h"
#include "../include/rawmidi.h"
#include "../include/seq_kernel.h"
#include "../include/seq_midi_emul.h"

///////////////////////////////////////////////////////////////////
//      defines

//#define USE_FAKE_MTP //       dont actually read/write to MTP device (for debugging without an actual unit) (does not work yet)

// io resources (make these module options?)
#define MTPAV_IOBASE 0x378
#define MTPAV_IRQ    7

// parallel port usage masks
#define SIGS_BYTE 0x08
#define SIGS_RFD 0x80
#define SIGS_IRQ 0x40
#define SIGS_IN0 0x10
#define SIGS_IN1 0x20

#define SIGC_WRITE 0x04
#define SIGC_READ 0x08
#define SIGC_INTEN 0x10

#define DREG 0
#define SREG 1
#define CREG 2

//
#define MTPAV_MODE_INPUT_OPENED		0x01
#define MTPAV_MODE_OUTPUT_OPENED	0x02
#define MTPAV_MODE_INPUT_TRIGGERED	0x04
#define MTPAV_MODE_OUTPUT_TRIGGERED	0x08

#if 0
#define NUMPORTS 9		// TOALL and 1..8
#else
/* FIXME: we have currently only 8 minor numbers */
#define NUMPORTS 8		// TOALL and 1..7
#endif
			// possible hardware ports (selected by 0xf5 port message)
			//      0x01 .. 0x08    this MTP's ports 1..8
			//      0x09 .. 0x10    networked MTP's ports (9..16)
			//      0x11            networked MTP's computer port
			//      0x63            to ADAT

///////////////////////////////////////////////////////////////////
//      types

typedef unsigned char U8;
typedef unsigned short int U16;
typedef unsigned long int U32;
typedef signed char S8;
typedef signed short int S16;
typedef signed long int S32;
typedef unsigned char UBOOL;

#define TRUE 1
#define FALSE 0

///////////////////////////////////////////////////////////////////

typedef struct Smtp {
	snd_card_t *card;
	unsigned long port;
	struct resource *res_port;
	snd_rawmidi_t *rmidi[NUMPORTS];
	int irq;
	spinlock_t spinlock;
	U8 mode[NUMPORTS];
	int istimer;
	struct timer_list timer;

	U32 inmidiport;
	U32 inmidistate;

	U32 outmidiport;
} TSmtp;

typedef struct Smtp_rawmidi_privdata {
	U32 port;
	TSmtp *card;

} TSmtp_rawmidi_privdata;

/////////////////////////////////////////////////////////////////////
//      protos

extern TSmtp *mtp_card;
