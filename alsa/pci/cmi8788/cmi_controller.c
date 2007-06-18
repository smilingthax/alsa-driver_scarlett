/*
 *  cmi_controller.c - Driver for C-Media CMI8788 PCI soundcards.
 *
 *      Copyright (C) 2005  C-media support
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
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *  Revision history
 *
 *    Weifeng Sui <weifengsui@163.com>
 */

#include <sound/driver.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <sound/core.h>

#include "cmi8788.h"
#include "codec.h"
#include "cmi_controller.h"

static int snd_cmi8788_bus_free(cmi8788_controller *bus)
{
	int i, codec_num;
	cmi_codec *codec = NULL;

	cmi_printk((">> snd_cmi8788_bus_free\n"));

	if (!bus)
		return 0;

	codec_num = bus->codec_num;

	for (i = 0; i < codec_num; i++) {
		codec = &bus->codec_list[i];
		if (codec)
			snd_cmi8788_codec_free(codec);
	}

	if (bus->ops.private_free)
		bus->ops.private_free(bus);

	kfree(bus);

	cmi_printk(("<< snd_cmi8788_bus_free\n"));

	return 0;
}

static int snd_cmi8788_bus_dev_free(struct snd_device *device)
{
	cmi8788_controller *bus = device->device_data;
	int iRet = 0;

	cmi_printk((">> snd_cmi8788_bus_dev_free\n"));

	iRet = snd_cmi8788_bus_free(bus);

	cmi_printk(("<< snd_cmi8788_bus_dev_free(iRet 0x%x)\n", iRet));

	return iRet;
}

/**
 * snd_cmi8788_controller_new - create a controller bus
 * @card: the card entry
 * @temp: the template for cmi_bus information
 * @busp: the pointer to store the created bus instance
 *
 * Returns 0 if successful, or a negative error code.
 */
int snd_cmi8788_controller_new(snd_cmi8788 *chip, const cmi_bus_template *temp, cmi8788_controller **busp)
{
	static struct snd_device_ops dev_ops = {
		.dev_free = snd_cmi8788_bus_dev_free,
	};
	cmi8788_controller *bus = NULL;
	int err;

	cmi_printk(("  >> snd_cmi8788_controller_new\n"));

	snd_assert(temp, return -EINVAL);
	snd_assert(temp->ops.get_response, return -EINVAL);

	if (busp)
		*busp = NULL;

	bus = kzalloc(sizeof(cmi8788_controller), GFP_KERNEL);
	if (!bus) {
		cmi_printk((KERN_ERR "can't allocate struct cmi8788_controller\n"));
		return -ENOMEM;
	}

	bus->card         = chip->card;
	bus->private_data = temp->private_data;
	bus->pci          = temp->pci;
	bus->ops          = temp->ops;

	/* init_MUTEX(&bus->cmd_mutex); */

	if (busp)
		*busp = bus;

	cmi_printk(("  << snd_cmi8788_controller_new\n"));

	return 0;
}
