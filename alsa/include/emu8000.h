#ifndef __EMU8000_H
#define __EMU8000_H
/*
 *  Defines for the emu8000 (AWE32/64)
 *
 *  Copyright (C) 1999 Steve Ratcliffe
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
 *  Changes:
 *     19980228   Takashi Iwai    Made soundfont lists linked.
 */

#include "hwdep.h"
#include "seq_kernel.h"
#include "seq_device.h"


/*
 * Hardware parameters.
 */
#define EMU8000_MAX_DRAM (28 * 1024 * 1024) /* Max on-board mem is 28Mb ???*/
#define EMU8000_DRAM_OFFSET 0x200000	/* Beginning of on board ram */
#define EMU8000_CHANNELS   32	/* Number of hardware channels */
#define EMU8000_MAX_PORTS	32	/* max number of ports */

/* Flags to set a dma channel to read or write */
#define EMU8000_RAM_READ   0
#define EMU8000_RAM_WRITE  1

#define NELEM(arr) (sizeof(arr)/sizeof((arr)[0]))

/* Check that correct emu structure is in use.  Mainly for development */
#define EMU8000_MAGIC	(0x7e591a1e)
#if 0
#define EMU8000_CHECK(emu) (emu == NULL) ? \
		(snd_printk("EMU800: Bad emu pointer (%s:%d)\n", __FILE__ , __LINE__) \
			, -1) : ((emu->magic != EMU8000_MAGIC) ? \
		snd_printk("EMU800: Bad magic number in emu->magic (%s:%d)\n", \
			__FILE__, __LINE__), -1: 0)
#else
#define EMU8000_CHECK(emu) (! (emu))
#endif


/*
 * Structure to hold all state information for the emu8000 driver.
 *
 * Note 1: The chip supports 32 channels in hardware this is max_channels
 * some of the channels may be used for other things so max_voices is
 * the number in use for wave voices.
 */
typedef struct snd_emu8000 {
	int  magic;		/* Magic number to detect incrorect structures */

	unsigned short port1;	/* Port usually base+0 */
	unsigned short port2;	/* Port usually at base+0x400 */
	unsigned short port3;	/* Port usually at base+0x800 */
	unsigned short last_reg;/* Last register command */

	snd_card_t *card;		/* The card that this belongs to */

	int mem_size;		/* Amount of ram on board */
	int max_channels;	/* Number of channels on the chip (note 1) */
	int max_voices;		/* Number of channels that we are using as voices */

	struct snd_sf_list *sflist;	/* Soundfont list */

	snd_info_entry_t *rom_proc_entry; /* The on board rom */
	snd_info_entry_t *ram_proc_entry; /* The on board ram */
	snd_info_entry_t *patch_proc_entry; /* Patch device */

	struct emu8000_voice *voices;	/* Voices (EMU 'channel') */

	int  client;		/* For the sequencer client */
	int  ports[EMU8000_MAX_PORTS];	/* The ports for this device */

	int used;

	/*  these parameters are not used actually yet.. - iwai */
	int chorus_mode;
	int reverb_mode;
	int bass_level;
	int treble_level;

	int use_time;	/* allocation counter */

	spinlock_t voice_lock;	/* Lock for voice access */
	spinlock_t reg_lock;	/* Lock for chip register access */
	wait_queue_head_t wait;	/* Lock for waits */
	struct semaphore register_mutex;
	struct semaphore patch_mutex;

#ifdef CONFIG_SND_OSSEMUL
	snd_seq_device_t *oss_synth;
#endif

} emu8000_t;

#define EMU8000_UPDATE_VOLUME	(1<<0)
#define EMU8000_UPDATE_PITCH	(1<<1)
#define EMU8000_UPDATE_PAN	(1<<2)
#define EMU8000_UPDATE_FMMOD	(1<<3)
#define EMU8000_UPDATE_TREMFREQ	(1<<4)
#define EMU8000_UPDATE_FM2FRQ2	(1<<5)
#define EMU8000_UPDATE_Q	(1<<6)

/* sequencer device id */
#define SND_SEQ_DEV_ID_EMU8000	"synth-emu8000"


#endif
