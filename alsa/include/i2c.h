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

#define SND_LOCK_I2C_BUS(bus) spin_lock_irqsave(&bus->lock, flags)
#define SND_UNLOCK_I2C_BUS(bus) spin_unlock_irqrestore(&bus->lock, flags)

typedef struct snd_i2c_bus snd_i2c_bus_t;

struct snd_i2c_bus {
	snd_card_t *card;	/* card which I2C belongs to */
	char name[32];		/* some useful label */
	void *data;		/* free for use by the bus driver */

	spinlock_t lock;

	/* Software I2C */
	void (*i2c_setlines) (snd_i2c_bus_t * bus, int ctrl, int data);
	int (*i2c_getdataline) (snd_i2c_bus_t * bus);

	/* Hardware I2C */
	int (*i2c_read) (snd_i2c_bus_t * bus, unsigned char addr);
	int (*i2c_write) (snd_i2c_bus_t * bus, unsigned char addr,
			  unsigned char b1, unsigned char b2, int both);

	unsigned long private_value;
	void *private_data;
	void (*private_free)(snd_i2c_bus_t *bus);
};


int snd_i2c_bus_create(snd_card_t *card, char *name, snd_i2c_bus_t **ri2c);

/* i2c bus access functions */
void snd_i2c_reset(struct snd_i2c_bus *bus);
void snd_i2c_start(struct snd_i2c_bus *bus);
void snd_i2c_stop(struct snd_i2c_bus *bus);
void snd_i2c_one(struct snd_i2c_bus *bus);
void snd_i2c_zero(struct snd_i2c_bus *bus);
int snd_i2c_ack(struct snd_i2c_bus *bus);

int snd_i2c_sendbyte(struct snd_i2c_bus *bus, unsigned char data, int wait_for_ack);
unsigned char snd_i2c_readbyte(struct snd_i2c_bus *bus, int last);

/* i2c (maybe) hardware functions */
int snd_i2c_read(struct snd_i2c_bus *bus, unsigned char addr);
int snd_i2c_write(struct snd_i2c_bus *bus, unsigned char addr,
		  unsigned char b1, unsigned char b2, int both);

#endif				/* __SND_I2C_H */
