#ifndef __SND_I2C_H
#define __SND_I2C_H

/*
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
 *
 */

typedef struct _snd_i2c_device snd_i2c_device_t;
typedef struct _snd_i2c_bus snd_i2c_bus_t;

struct _snd_i2c_device {
	struct list_head list;
	snd_i2c_bus_t *bus;	/* I2C bus */
	char name[32];		/* some useful device name */
	unsigned char addr;	/* device address */
	unsigned long private_value;
	void *private_data;
	void (*private_free)(snd_i2c_device_t *device);
};

#define snd_i2c_device(n) list_entry(n, snd_i2c_device_t, list)

struct _snd_i2c_bus {
	snd_card_t *card;	/* card which I2C belongs to */
	char name[32];		/* some useful label */

	spinlock_t lock;

	snd_i2c_bus_t *master;	/* master bus when SCK/SCL is shared */
	struct list_head buses;	/* master: slave buses sharing SCK/SCL, slave: link list */

	struct list_head devices; /* attached devices to this bus */

	/* Software I2C */
	void (*i2c_setlines) (snd_i2c_bus_t * bus, int ctrl, int data);
	int (*i2c_getdataline) (snd_i2c_bus_t * bus);

	/* Hardware I2C */
	int (*i2c_read) (snd_i2c_bus_t * bus, unsigned char addr);
	int (*i2c_write) (snd_i2c_bus_t * bus, unsigned char addr, unsigned char b1, unsigned char b2, int both);

	unsigned long private_value;
	void *private_data;
	void (*private_free)(snd_i2c_bus_t *bus);
};

#define snd_i2c_slave_bus(n) list_entry(n, snd_i2c_bus_t, buses)

int snd_i2c_bus_create(snd_card_t *card, const char *name, snd_i2c_bus_t *master, snd_i2c_bus_t **ri2c);
int snd_i2c_device_create(snd_i2c_bus_t *bus, const char *name, unsigned char addr, snd_i2c_device_t **rdevice);
int snd_i2c_device_free(snd_i2c_device_t *device);

/* i2c bus access functions */
void snd_i2c_reset(snd_i2c_bus_t *bus);
void snd_i2c_start(snd_i2c_bus_t *bus);
void snd_i2c_stop(snd_i2c_bus_t *bus);
void snd_i2c_one(snd_i2c_bus_t *bus);
void snd_i2c_zero(snd_i2c_bus_t *bus);
int snd_i2c_ack(snd_i2c_bus_t *bus);

static inline void snd_i2c_lock_irq(snd_i2c_bus_t *bus) { spin_lock_irq(&(bus->master ? bus->master->lock : bus->lock)); }
static inline void snd_i2c_unlock_irq(snd_i2c_bus_t *bus) { spin_unlock_irq(&(bus->master ? bus->master->lock : bus->lock)); }

int snd_i2c_sendbyte(snd_i2c_bus_t *bus, unsigned char data, int wait_for_ack);
unsigned char snd_i2c_readbyte(snd_i2c_bus_t *bus, int last);

/* i2c (maybe) hardware functions */
int snd_i2c_read(snd_i2c_bus_t *bus, unsigned char addr);
int snd_i2c_write(snd_i2c_bus_t *bus, unsigned char addr, unsigned char b1, unsigned char b2, int both);
int snd_i2c_dev_read(snd_i2c_device_t *device);
int snd_i2c_dev_write(snd_i2c_device_t *device, unsigned char b1, unsigned char b2, int both);

#endif				/* __SND_I2C_H */
