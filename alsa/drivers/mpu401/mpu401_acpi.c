/*
 * mpu401_acpi.c - driver for motherboard MPU-401 ports identified by ACPI PnP
 * Copyright (c) 2004 Clemens Ladisch <clemens@ladisch.de>
 *
 * based on 8250_acpi.c
 * Copyright (c) 2002-2003 Matthew Wilcox for Hewlett-Packard
 * Copyright (C) 2004 Hewlett-Packard Co
 *      Bjorn Helgaas <bjorn.helgaas@hp.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <sound/driver.h>
#include <linux/init.h>
#include <acpi/acpi_bus.h>
#include <sound/core.h>
#include <sound/mpu401.h>
#define SNDRV_GET_ID
#include <sound/initval.h>

MODULE_AUTHOR("Clemens Ladisch <clemens@ladisch.de>");
MODULE_DESCRIPTION("MPU-401 UART (ACPI)");
MODULE_LICENSE("GPL");
MODULE_CLASSES("{sound}");

static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX; /* Index 0-MAX */
static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR; /* ID for this card */
static int enable[SNDRV_CARDS] = SNDRV_DEFAULT_ENABLE_PNP; /* Enable this card */

MODULE_PARM(index, "1-" __MODULE_STRING(SNDRV_CARDS) "i");
MODULE_PARM_DESC(index, "Index value for MPU-401 device.");
MODULE_PARM_SYNTAX(index, SNDRV_INDEX_DESC);
MODULE_PARM(id, "1-" __MODULE_STRING(SNDRV_CARDS) "s");
MODULE_PARM_DESC(id, "ID string for MPU-401 device.");
MODULE_PARM_SYNTAX(id, SNDRV_ID_DESC);
MODULE_PARM(enable, "1-" __MODULE_STRING(SNDRV_CARDS) "i");
MODULE_PARM_DESC(enable, "Enable MPU-401 device.");
MODULE_PARM_SYNTAX(enable, SNDRV_ENABLE_DESC);

#define MPU401_PNP_ID "PNPB006"

struct mpu401_resources {
	unsigned long port;
	int irq;
};

static acpi_status __devinit snd_mpu401_acpi_resource(struct acpi_resource *res, void *data)
{
	struct mpu401_resources *resources = (struct mpu401_resources *)data;

	if (res->id == ACPI_RSTYPE_IRQ) {
		if (res->data.irq.number_of_interrupts > 0) {
#ifdef CONFIG_IA64
			resources->irq = acpi_register_irq(res->data.irq.interrupts[0],
							   res->data.irq.active_high_low,
							   res->data.irq.edge_level);
#else
			resources->irq = res->data.irq.interrupts[0];
#endif
		}
	} else if (res->id == ACPI_RSTYPE_IO) {
		if (res->data.io.range_length >= 2) {
			resources->port = res->data.io.min_base_address;
		}
	}
	return AE_OK;
}

static int __devinit snd_mpu401_acpi_add(struct acpi_device *device)
{
	static int dev;
	snd_card_t *card;
	struct mpu401_resources resources;
	acpi_status status;
	int err;

	if (dev >= SNDRV_CARDS)
		return -ENODEV;
	if (!enable[dev]) {
		++dev;
		return -ENOENT;
	}

	resources.port = -1;
	resources.irq = -1;
	status = acpi_walk_resources(device->handle, METHOD_NAME__CRS,
				     snd_mpu401_acpi_resource, &resources);
	if (ACPI_FAILURE(status))
		return -ENODEV;
	if (resources.port < 0 || resources.irq < 0) {
		snd_printk(KERN_ERR "no port or irq in %s _CRS\n",
			   acpi_device_bid(device));
		return -ENODEV;
	}

	card = snd_card_new(index[dev], id[dev], THIS_MODULE, 0);
	if (!card)
		return -ENOMEM;

	strcpy(card->driver, "MPU-401 (ACPI)");
	strcpy(card->shortname, "MPU-401 UART");
	snprintf(card->longname, sizeof(card->longname),
		 "%s at %#lx, irq %d, bus id %s", card->shortname,
		 resources.port, resources.irq, acpi_device_bid(device));

	err = snd_mpu401_uart_new(card, 0, MPU401_HW_MPU401, resources.port,
				  0, resources.irq, SA_INTERRUPT, NULL);
	if (err < 0) {
		printk(KERN_ERR "MPU401 not detected at %#lx\n", resources.port);
		snd_card_free(card);
		return err;
	}

	if ((err = snd_card_register(card)) < 0) {
		snd_card_free(card);
		return err;
	}
	acpi_driver_data(device) = card;
	++dev;
	return 0;
}

static int __devexit snd_mpu401_acpi_remove(struct acpi_device *device, int type)
{
	snd_card_t *card;

	if (!device || !acpi_driver_data(device))
		return -EINVAL;

	card = (snd_card_t *)acpi_driver_data(device);
	snd_card_free(card);
	acpi_driver_data(device) = NULL;
	return 0;
}

static struct acpi_driver snd_mpu401_acpi_driver = {
	.name = "snd-mpu401-acpi",
	.class = "",
	.ids = MPU401_PNP_ID,
	.ops = {
		.add = snd_mpu401_acpi_add,
		.remove = __devexit_p(snd_mpu401_acpi_remove),
	},
};

static int __init alsa_card_mpu401_acpi_init(void)
{
	int err;
	err = acpi_bus_register_driver(&snd_mpu401_acpi_driver);
	return err < 0 ? err : 0;
}

static void __exit alsa_card_mpu401_acpi_exit(void)
{
	acpi_bus_unregister_driver(&snd_mpu401_acpi_driver);
}

module_init(alsa_card_mpu401_acpi_init)
module_exit(alsa_card_mpu401_acpi_exit)

#ifndef MODULE

/* format is: snd-mpu401-acpi=enable,index,id */

static int __init alsa_card_mpu401_setup(char *str)
{
	static unsigned __initdata nr_dev = 0;

	if (nr_dev >= SNDRV_CARDS)
		return 0;
	(void)(get_option(&str, &enable[nr_dev]) == 2 &&
	       get_option(&str, &index[nr_dev]) == 2 &&
	       get_id(&str, &id[nr_dev]) == 2);
	nr_dev++;
	return 1;
}

__setup("snd-mpu401-acpi=", alsa_card_mpu401_setup);

#endif /* ifndef MODULE */

EXPORT_NO_SYMBOLS;
