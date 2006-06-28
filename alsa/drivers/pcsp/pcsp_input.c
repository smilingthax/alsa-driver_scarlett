/*
 *  PC Speaker beeper driver for Linux
 *
 *  Copyright (c) 2002 Vojtech Pavlik
 *  Copyright (c) 1992 Orest Zborowski
 *
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <asm/8253pit.h>
#include <asm/i8253.h>
#include <asm/io.h>
#include "pcsp.h"


static void pcspkr_do_sound(unsigned int count)
{
	unsigned long flags;

	spin_lock_irqsave(&i8253_lock, flags);

	if (count) {
		/* enable counter 2 */
		outb_p(inb_p(0x61) | 3, 0x61);
		/* set command for counter 2, 2 byte write */
		outb_p(0xB6, 0x43);
		/* select desired HZ */
		outb_p(count & 0xff, 0x42);
		outb((count >> 8) & 0xff, 0x42);
	} else {
		/* disable counter 2 */
		outb(inb_p(0x61) & 0xFC, 0x61);
	}

	spin_unlock_irqrestore(&i8253_lock, flags);
}

static int pcspkr_input_event(struct input_dev *dev, unsigned int type,
		unsigned int code, int value)
{
	unsigned int count = 0;

	if (snd_pcsp_chip->timer_active || !snd_pcsp_chip->pcspkr)
		return 0;

	switch (type) {
	case EV_SND:
		switch (code) {
		case SND_BELL:
			if (value)
				value = 1000;
		case SND_TONE:
			break;
		default:
			return -1;
		}
		break;

	default:
		return -1;
	}

	if (value > 20 && value < 32767)
		count = PIT_TICK_RATE / value;

	pcspkr_do_sound(count);

	return 0;
}

int pcspkr_input_init(struct snd_pcsp *chip)
{
	int err;

	chip->input_dev = input_allocate_device();
	if (!chip->input_dev)
		return -ENOMEM;

	chip->input_dev->name = "PC Speaker";
	chip->input_dev->phys = "isa0061/input0";
	chip->input_dev->id.bustype = BUS_ISA;
	chip->input_dev->id.vendor = 0x001f;
	chip->input_dev->id.product = 0x0001;
	chip->input_dev->id.version = 0x0100;

	chip->input_dev->evbit[0] = BIT(EV_SND);
	chip->input_dev->sndbit[0] = BIT(SND_BELL) | BIT(SND_TONE);
	chip->input_dev->event = pcspkr_input_event;

	err = input_register_device(chip->input_dev);
	if (err) {
		input_free_device(chip->input_dev);
		chip->input_dev = NULL;
		return err;
	}

	return 0;
}

int pcspkr_input_remove(struct snd_pcsp *chip)
{
	if (!chip->input_dev)
		return 0;
    	/* turn off the speaker */
    	pcspkr_do_sound(0);
	input_unregister_device(chip->input_dev);

	return 0;
}
