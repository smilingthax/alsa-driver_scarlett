/*
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>
 *  Routines for control of EMU10K1 MPU-401 in UART mode
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

#define __NO_VERSION__
#include <sound/driver.h>
#include <sound/core.h>
#include <sound/emu10k1.h>

#define EMU10K1_MIDI_MODE_INPUT		(1<<0)
#define EMU10K1_MIDI_MODE_OUTPUT	(1<<1)

/*

 */

static void snd_emu10k1_midi_interrupt(emu10k1_t *emu, unsigned int status)
{
	snd_rawmidi_t *rmidi;
	unsigned char byte, bstatus;

	if ((rmidi = emu->rmidi) == NULL) {
		snd_emu10k1_intr_disable(emu, status & (INTE_MIDITXENABLE|INTE_MIDIRXENABLE));
		return;
	}

	spin_lock(&emu->midi_input_lock);
	bstatus = inb(emu->port + MUSTAT);
	if ((status & IPR_MIDIRECVBUFEMPTY) && !(bstatus & 0x80)) {
		byte = inb(emu->port + MUDATA);
		spin_unlock(&emu->midi_input_lock);
		if (emu->midi_substream_input)
			snd_rawmidi_receive(emu->midi_substream_input, &byte, 1);
		spin_lock(&emu->midi_input_lock);
	}
	spin_unlock(&emu->midi_input_lock);

	spin_lock(&emu->midi_output_lock);
	bstatus = inb(emu->port + MUSTAT);
	if ((status & IPR_MIDITRANSBUFEMPTY) && !(bstatus & 0x40)) {
		if (emu->midi_substream_output &&
		    snd_rawmidi_transmit(emu->midi_substream_output, &byte, 1) == 1) {
			outb(byte, emu->port + MUDATA);
		} else {
			snd_emu10k1_intr_disable(emu, INTE_MIDITXENABLE);
		}
	}
	spin_unlock(&emu->midi_output_lock);
}

/*

 */

static void snd_emu10k1_midi_cmd(emu10k1_t * emu, unsigned char cmd, int ack)
{
	unsigned long flags;
	int timeout, ok;

	spin_lock_irqsave(&emu->midi_input_lock, flags);
	outb(0x00, emu->port + MUDATA);
	for (timeout = 100000; timeout > 0 && !(inb(emu->port + MUSTAT) & 0x80); timeout--)
		inb(emu->port + MUDATA);
#ifdef CONFIG_SND_DEBUG
	if (timeout <= 0)
		snd_printk("midi_cmd: clear rx timeout (status = 0x%x)\n", inb(emu->port + MUSTAT));
#endif
	outb(cmd, emu->port + MUSTAT);
	if (ack) {
		ok = 0;
		timeout = 10000;
		while (!ok && timeout-- > 0) {
			if (!(inb(emu->port + MUSTAT) & 0x80)) {
				if (inb(emu->port + MUDATA) == 0xfe)
					ok = 1;
			}
		}
	} else {
		ok = 1;
	}
	spin_unlock_irqrestore(&emu->midi_input_lock, flags);
	if (!ok)
		snd_printk("midi_cmd: 0x%x failed at 0x%lx (status = 0x%x, data = 0x%x)!!!\n", cmd, emu->port, inb(emu->port + MUSTAT), inb(emu->port + MUDATA));
}

static int snd_emu10k1_midi_input_open(snd_rawmidi_substream_t * substream)
{
	unsigned long flags;
	emu10k1_t *emu;

	emu = snd_magic_cast(emu10k1_t, substream->rmidi->private_data, return -ENXIO);
#if 0
	snd_printk("[0x%x] MPU-401 command port - 0x%x\n", (emu), inb(MPU401C(emu)));
	snd_printk("[0x%x] MPU-401 data port - 0x%x\n", MPU401D(emu), inb(MPU401D(emu)));
#endif
	spin_lock_irqsave(&emu->midi_open_lock, flags);
	emu->midi_mode |= EMU10K1_MIDI_MODE_INPUT;
	emu->midi_substream_input = substream;
	if (!(emu->midi_mode & EMU10K1_MIDI_MODE_OUTPUT)) {
		spin_unlock_irqrestore(&emu->midi_open_lock, flags);
		snd_emu10k1_midi_cmd(emu, 0xff, 1);	/* reset */
		snd_emu10k1_midi_cmd(emu, 0x3f, 1);	/* enter UART mode */
	} else {
		spin_unlock_irqrestore(&emu->midi_open_lock, flags);
	}
	return 0;
}

static int snd_emu10k1_midi_output_open(snd_rawmidi_substream_t * substream)
{
	unsigned long flags;
	emu10k1_t *emu;

	emu = snd_magic_cast(emu10k1_t, substream->rmidi->private_data, return -ENXIO);
	spin_lock_irqsave(&emu->midi_open_lock, flags);
	emu->midi_mode |= EMU10K1_MIDI_MODE_OUTPUT;
	emu->midi_substream_output = substream;
	if (!(emu->midi_mode & EMU10K1_MIDI_MODE_INPUT)) {
		spin_unlock_irqrestore(&emu->midi_open_lock, flags);
		snd_emu10k1_midi_cmd(emu, 0xff, 1);	/* reset */
		snd_emu10k1_midi_cmd(emu, 0x3f, 1);	/* enter UART mode */
	} else {
		spin_unlock_irqrestore(&emu->midi_open_lock, flags);
	}
	return 0;
}

static int snd_emu10k1_midi_input_close(snd_rawmidi_substream_t * substream)
{
	unsigned long flags;
	emu10k1_t *emu;

	emu = snd_magic_cast(emu10k1_t, substream->rmidi->private_data, return -ENXIO);
	spin_lock_irqsave(&emu->midi_open_lock, flags);
	snd_emu10k1_intr_disable(emu, INTE_MIDIRXENABLE);
	emu->midi_mode &= ~EMU10K1_MIDI_MODE_INPUT;
	emu->midi_substream_input = NULL;
	if (!(emu->midi_mode & EMU10K1_MIDI_MODE_OUTPUT)) {
		spin_unlock_irqrestore(&emu->midi_open_lock, flags);
		snd_emu10k1_midi_cmd(emu, 0xff, 0);	/* reset */
	} else {
		spin_unlock_irqrestore(&emu->midi_open_lock, flags);
	}
	return 0;
}

static int snd_emu10k1_midi_output_close(snd_rawmidi_substream_t * substream)
{
	unsigned long flags;
	emu10k1_t *emu;

	emu = snd_magic_cast(emu10k1_t, substream->rmidi->private_data, return -ENXIO);
	spin_lock_irqsave(&emu->midi_open_lock, flags);
	snd_emu10k1_intr_disable(emu, INTE_MIDITXENABLE);
	emu->midi_mode &= ~EMU10K1_MIDI_MODE_OUTPUT;
	emu->midi_substream_output = NULL;
	if (!(emu->midi_mode & EMU10K1_MIDI_MODE_INPUT)) {
		spin_unlock_irqrestore(&emu->midi_open_lock, flags);
		snd_emu10k1_midi_cmd(emu, 0xff, 0);	/* reset */
	} else {
		spin_unlock_irqrestore(&emu->midi_open_lock, flags);
	}
	return 0;
}

static void snd_emu10k1_midi_input_trigger(snd_rawmidi_substream_t * substream, int up)
{
	emu10k1_t *emu = snd_magic_cast(emu10k1_t, substream->rmidi->private_data, return);

	if (up)
		snd_emu10k1_intr_enable(emu, INTE_MIDIRXENABLE);
	else
		snd_emu10k1_intr_disable(emu, INTE_MIDIRXENABLE);
}

static void snd_emu10k1_midi_output_trigger(snd_rawmidi_substream_t * substream, int up)
{
	unsigned long flags;
	emu10k1_t *emu = snd_magic_cast(emu10k1_t, substream->rmidi->private_data, return);

	if (up) {
		int max = 4;
		unsigned char byte;
	
		while (max > 0) {
			spin_lock_irqsave(&emu->midi_output_lock, flags);
			if (!(inb(emu->port + MUSTAT) & 0x40)) {
				if (!(emu->midi_mode & EMU10K1_MIDI_MODE_OUTPUT) ||
				    snd_rawmidi_transmit(substream, &byte, 1) != 1) {
					spin_unlock_irqrestore(&emu->midi_output_lock, flags);
					return;
				}
				outb(byte, emu->port + MUDATA);
				spin_unlock_irqrestore(&emu->midi_output_lock, flags);
				// snd_printk("uart: tx = 0x%x\n", byte);
				max--;
			} else {
				spin_unlock_irqrestore(&emu->midi_output_lock, flags);
				break;
			}
		}
		snd_emu10k1_intr_enable(emu, INTE_MIDITXENABLE);
	} else {
		snd_emu10k1_intr_disable(emu, INTE_MIDITXENABLE);
	}
}

/*

 */

static snd_rawmidi_ops_t snd_emu10k1_midi_output =
{
	open:		snd_emu10k1_midi_output_open,
	close:		snd_emu10k1_midi_output_close,
	trigger:	snd_emu10k1_midi_output_trigger,
};

static snd_rawmidi_ops_t snd_emu10k1_midi_input =
{
	open:		snd_emu10k1_midi_input_open,
	close:		snd_emu10k1_midi_input_close,
	trigger:	snd_emu10k1_midi_input_trigger,
};

static void snd_emu10k1_midi_free(snd_rawmidi_t *rmidi)
{
	emu10k1_t *emu = snd_magic_cast(emu10k1_t, rmidi->private_data, return);
	emu->mpu401_interrupt = NULL;
	emu->rmidi = NULL;
}

int snd_emu10k1_midi(emu10k1_t *emu, int device, snd_rawmidi_t ** rrawmidi)
{
	snd_rawmidi_t *rmidi;
	int err;

	if (rrawmidi)
		*rrawmidi = NULL;
	if ((err = snd_rawmidi_new(emu->card, "EMU10K1 MPU", device, 1, 1, &rmidi)) < 0)
		return err;
	spin_lock_init(&emu->midi_open_lock);
	spin_lock_init(&emu->midi_input_lock);
	spin_lock_init(&emu->midi_output_lock);
	strcpy(rmidi->name, "EMU10K1 MPU-401 (UART)");
	snd_rawmidi_set_ops(rmidi, SNDRV_RAWMIDI_STREAM_OUTPUT, &snd_emu10k1_midi_output);
	snd_rawmidi_set_ops(rmidi, SNDRV_RAWMIDI_STREAM_INPUT, &snd_emu10k1_midi_input);
	rmidi->info_flags |= SNDRV_RAWMIDI_INFO_OUTPUT |
	                     SNDRV_RAWMIDI_INFO_INPUT |
	                     SNDRV_RAWMIDI_INFO_DUPLEX;
	rmidi->private_data = emu;
	rmidi->private_free = snd_emu10k1_midi_free;
	emu->rmidi = rmidi;
	if (rrawmidi)
		*rrawmidi = rmidi;
	emu->mpu401_interrupt = snd_emu10k1_midi_interrupt;
	return 0;
}
