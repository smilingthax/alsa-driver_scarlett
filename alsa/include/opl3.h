#ifndef __OPL3_H
#define __OPL3_H

/*
 * Definitions of the OPL-3 registers.
 *
 * Copyright (c) by Jaroslav Kysela <perex@suse.cz>,
 *                  Hannu Savolainen 1993-1996
 *
 *
 *      The OPL-3 mode is switched on by writing 0x01, to the offset 5
 *      of the right side.
 *
 *      Another special register at the right side is at offset 4. It contains
 *      a bit mask defining which voices are used as 4 OP voices.
 *
 *      The percussive mode is implemented in the left side only.
 *
 *      With the above exceptions the both sides can be operated independently.
 *      
 *      A 4 OP voice can be created by setting the corresponding
 *      bit at offset 4 of the right side.
 *
 *      For example setting the rightmost bit (0x01) changes the
 *      first voice on the right side to the 4 OP mode. The fourth
 *      voice is made inaccessible.
 *
 *      If a voice is set to the 2 OP mode, it works like 2 OP modes
 *      of the original YM3812 (AdLib). In addition the voice can 
 *      be connected the left, right or both stereo channels. It can
 *      even be left unconnected. This works with 4 OP voices also.
 *
 *      The stereo connection bits are located in the FEEDBACK_CONNECTION
 *      register of the voice (0xC0-0xC8). In 4 OP voices these bits are
 *      in the second half of the voice.
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

#include "hwdep.h"
#include "timer.h"

/*
 *    Register numbers for the global registers
 */

#define OPL3_REG_TEST			0x01
#define   OPL3_ENABLE_WAVE_SELECT	0x20

#define OPL3_REG_TIMER1			0x02
#define OPL3_REG_TIMER2			0x03
#define OPL3_REG_TIMER_CONTROL		0x04	/* Left side */
#define   OPL3_IRQ_RESET		0x80
#define   OPL3_TIMER1_MASK		0x40
#define   OPL3_TIMER2_MASK		0x20
#define   OPL3_TIMER1_START		0x01
#define   OPL3_TIMER2_START		0x02

#define OPL3_REG_CONNECTION_SELECT	0x04	/* Right side */
#define   OPL3_RIGHT_4OP_0		0x01
#define   OPL3_RIGHT_4OP_1		0x02
#define   OPL3_RIGHT_4OP_2		0x04
#define   OPL3_LEFT_4OP_0		0x08
#define   OPL3_LEFT_4OP_1		0x10
#define   OPL3_LEFT_4OP_2		0x20

#define OPL3_REG_MODE			0x05	/* Right side */
#define   OPL3_MODE2_ENABLE		0x01	/* OPL3 mode */
#define   OPL3_MODE3_ENABLE		0x02	/* OPL4 mode */

#define OPL3_REG_KBD_SPLIT		0x08	/* Left side */
#define   OPL3_COMPOSITE_SINE_WAVE_MODE	0x80	/* Don't use with OPL-3? */
#define   OPL3_KEYBOARD_SPLIT		0x40

#define OPL3_REG_PERCUSSION		0xbd	/* Left side only */
#define   OPL3_TREMOLO_DEPTH		0x80
#define   OPL3_VIBRATO_DEPTH		0x40
#define	  OPL3_PERCUSSION_ENABLE	0x20
#define   OPL3_BASSDRUM_ON		0x10
#define   OPL3_SNAREDRUM_ON		0x08
#define   OPL3_TOMTOM_ON		0x04
#define   OPL3_CYMBAL_ON		0x02
#define   OPL3_HIHAT_ON			0x01

/*
 *    Offsets to the register banks for operators. To get the
 *      register number just add the operator offset to the bank offset
 *
 *      AM/VIB/EG/KSR/Multiple (0x20 to 0x35)
 */
#define OPL3_REG_AM_VIB			0x20
#define   OPL3_TREMOLO_ON		0x80
#define   OPL3_VIBRATO_ON		0x40
#define   OPL3_SUSTAIN_ON		0x20
#define   OPL3_KSR			0x10	/* Key scaling rate */
#define   OPL3_MULTIPLE_MASK		0x0f	/* Frequency multiplier */

 /*
  *   KSL/Total level (0x40 to 0x55)
  */
#define OPL3_REG_KSL_LEVEL		0x40
#define   OPL3_KSL_MASK			0xc0	/* Envelope scaling bits */
#define   OPL3_TOTAL_LEVEL_MASK		0x3f	/* Strength (volume) of OP */

/*
 *    Attack / Decay rate (0x60 to 0x75)
 */
#define OPL3_REG_ATTACK_DECAY		0x60
#define   OPL3_ATTACK_MASK		0xf0
#define   OPL3_DECAY_MASK		0x0f

/*
 * Sustain level / Release rate (0x80 to 0x95)
 */
#define OPL3_REG_SUSTAIN_RELEASE	0x80
#define   OPL3_SUSTAIN_MASK		0xf0
#define   OPL3_RELEASE_MASK		0x0f

/*
 * Wave select (0xE0 to 0xF5)
 */
#define OPL3_REG_WAVE_SELECT		0xe0

/*
 *    Offsets to the register banks for voices. Just add to the
 *      voice number to get the register number.
 *
 *      F-Number low bits (0xA0 to 0xA8).
 */
#define OPL3_REG_FNUM_LOW		0xa0

/*
 *    F-number high bits / Key on / Block (octave) (0xB0 to 0xB8)
 */
#define OPL3_REG_KEYON_BLOCK		0xb0
#define	  OPL3_KEYON_BIT		0x20
#define	  OPL3_BLOCKNUM_MASK		0x1c
#define   OPL3_FNUM_HIGH_MASK		0x03

/*
 *    Feedback / Connection (0xc0 to 0xc8)
 *
 *      These registers have two new bits when the OPL-3 mode
 *      is selected. These bits controls connecting the voice
 *      to the stereo channels. For 4 OP voices this bit is
 *      defined in the second half of the voice (add 3 to the
 *      register offset).
 *
 *      For 4 OP voices the connection bit is used in the
 *      both halves (gives 4 ways to connect the operators).
 */
#define OPL3_REG_FEEDBACK_CONNECTION	0xc0
#define   OPL3_FEEDBACK_MASK		0x0e	/* Valid just for 1st OP of a voice */
#define   OPL3_CONNECTION_BIT		0x01
/*
 *    In the 4 OP mode there is four possible configurations how the
 *      operators can be connected together (in 2 OP modes there is just
 *      AM or FM). The 4 OP connection mode is defined by the rightmost
 *      bit of the FEEDBACK_CONNECTION (0xC0-0xC8) on the both halves.
 *
 *      First half      Second half     Mode
 *
 *                                       +---+
 *                                       v   |
 *      0               0               >+-1-+--2--3--4-->
 *
 *
 *                                      
 *                                       +---+
 *                                       |   |
 *      0               1               >+-1-+--2-+
 *                                                |->
 *                                      >--3----4-+
 *                                      
 *                                       +---+
 *                                       |   |
 *      1               0               >+-1-+-----+
 *                                                 |->
 *                                      >--2--3--4-+
 *
 *                                       +---+
 *                                       |   |
 *      1               1               >+-1-+--+
 *                                              |
 *                                      >--2--3-+->
 *                                              |
 *                                      >--4----+
 */
#define   OPL3_STEREO_BITS		0x30	/* OPL-3 only */
#define     OPL3_VOICE_TO_LEFT		0x10
#define     OPL3_VOICE_TO_RIGHT		0x20

/*
 *    Definition table for the physical voices
 */

struct snd_opl3_physical_voice_info {
	unsigned char num;
	unsigned char mode;	/* 0=unavailable, 2=2 OP, 4=4 OP */
	unsigned short ioaddr;	/* I/O port (left or right side) */
	unsigned char op[4];	/* Operator offsets */
};

/*

 */

#define OPL3_LEFT		0x00000000
#define OPL3_RIGHT		0x00000100

#define OPL3_HW_AUTO		0x0000
#define OPL3_HW_OPL2		0x0200
#define OPL3_HW_OPL3		0x0300
#define OPL3_HW_OPL3_SV		0x0301	/* S3 SonicVibes */
#define OPL3_HW_OPL3_CS		0x0302	/* CS4232/CS4236+ */
#define OPL3_HW_OPL3_FM801	0x0303	/* FM801 */
#define OPL3_HW_OPL4		0x0400
#define OPL3_HW_MASK		0xff00

typedef struct snd_opl3 opl3_t;

struct snd_opl3 {
	unsigned short l_port;
	unsigned short r_port;
	unsigned short hardware;
	unsigned short timer_enable;
	int timer_dev;		/* timer device number */
	snd_timer_t *timer1;
	snd_timer_t *timer2;
	spinlock_t reg_lock;
	spinlock_t timer_lock;
};

extern void snd_opl3_interrupt(snd_hwdep_t * hw);
extern int snd_opl3_new(snd_card_t * card,
			int device,
			unsigned short l_port,
			unsigned short r_port,
			unsigned short hardware,
			int timer_dev,
			snd_hwdep_t ** rawmidi);

#endif				/* __OPL3_H */
