#ifndef __I2C_H
#define __I2C_H

/*
 * linux i2c interface.  Works a little bit like the scsi subsystem.
 * There are:
 *
 *     i2c          the basic control module        (like scsi_mod)
 *     bus driver   a driver with a i2c bus         (hostadapter driver)
 *     chip driver  a driver for a chip connected
 *                  to a i2c bus                    (cdrom/hd driver)
 *
 * a devices will be attached to one bus and one chip driver.  Every chip
 * driver gets a unique ID.
 *
 * A chip driver can provide a ioctl-like callback for the
 * communication with other parts of the kernel (not every i2c chip is
 * useful without other devices, a TV card tuner for example). 
 *
 * "i2c internal" parts of the structs: only the i2c module is allowed to
 * write to them, for others they are read-only.
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

#define SND_I2C_BUS_MAX       4	/* max # of bus drivers  */
#define SND_I2C_DRIVER_MAX    8	/* max # of chip drivers */
#define SND_I2C_DEVICE_MAX    8	/* max # of devices per bus/driver */

struct snd_i2c_bus;
struct snd_i2c_driver;
struct snd_i2c_device;

#define SND_I2C_DRIVERID_TEA6330	1

#define SND_I2C_BUSID_INTERWAVE		1

/*
 * struct for a driver for a i2c chip (tuner, soundprocessor,
 * videotext, ... ).
 *
 * a driver will register within the i2c module.  The i2c module will
 * callback the driver (i2c_attach) for every device it finds on a i2c
 * bus at the specified address.  If the driver decides to "accept"
 * the, device, it must return a struct i2c_device, and NULL
 * otherwise.
 *
 * i2c_detach = i2c_attach ** -1
 * 
 * i2c_command will be used to pass commands to the driver in a
 * ioctl-line manner.
 *
 */

struct snd_i2c_driver {
	char name[32];		/* some useful label         */
	int id;			/* device type ID            */
	unsigned char addr_l, addr_h;	/* address range of the chip */

	int (*attach) (struct snd_i2c_device * device);
	int (*detach) (struct snd_i2c_device * device);
	int (*command) (struct snd_i2c_device * device,
			unsigned int cmd, void *arg);

	/* i2c internal */
	struct snd_i2c_device *devices[SND_I2C_DEVICE_MAX];
	int devcount;
};


/*
 * this holds the informations about a i2c bus available in the system.
 * 
 * a chip with a i2c bus interface (like bt848) registers the bus within
 * the i2c module. This struct provides functions to access the i2c bus.
 * 
 * One must hold the spinlock to access the i2c bus (XXX: is the irqsave
 * required? Maybe better use a semaphore?).
 * [-AC-] having a spinlock_irqsave is only needed if we have drivers wishing
 *        to bang their i2c bus from an interrupt.
 * 
 * attach/detach_inform is a callback to inform the bus driver about
 * attached chip drivers.
 *
 */

#define SND_LOCK_I2C_BUS(bus) spin_lock_irqsave(&bus->lock, flags)
#define SND_UNLOCK_I2C_BUS(bus) spin_unlock_irqrestore(&bus->lock, flags)

struct snd_i2c_bus {
	char name[32];		/* some useful label */
	int id;
	void *data;		/* free for use by the bus driver */

	spinlock_t lock;

	/* attach/detach inform callbacks */
	void (*attach_inform) (struct snd_i2c_bus * bus, int id);
	void (*detach_inform) (struct snd_i2c_bus * bus, int id);

	/* Software I2C */
	void (*i2c_setlines) (struct snd_i2c_bus * bus, int ctrl, int data);
	int (*i2c_getdataline) (struct snd_i2c_bus * bus);

	/* Hardware I2C */
	int (*i2c_read) (struct snd_i2c_bus * bus, unsigned char addr);
	int (*i2c_write) (struct snd_i2c_bus * bus, unsigned char addr,
			  unsigned char b1, unsigned char b2, int both);

	/* internal data for i2c module */
	struct snd_i2c_device *devices[SND_I2C_DEVICE_MAX];
	int devcount;

	unsigned int private_value;
	void *private_data;
	void (*private_free)(struct snd_i2c_bus *bus);
};


/*
 * this holds per-device data for a i2c device
 *
 */

struct snd_i2c_device {
	char name[32];		/* some useful label */
	void *data;		/* free for use by the chip driver */
	unsigned char addr;	/* chip addr */

	/* i2c internal */
	struct snd_i2c_bus *bus;
	struct snd_i2c_driver *driver;
};


/* ------------------------------------------------------------------- */
/* i2c module functions                                                */

/* register/unregister a i2c bus */
struct snd_i2c_bus *snd_i2c_bus_new(int id, char *name);
int snd_i2c_bus_free(struct snd_i2c_bus *bus);
int snd_i2c_register_bus(struct snd_i2c_bus *bus);
int snd_i2c_unregister_bus(struct snd_i2c_bus *bus);

/* register/unregister a chip driver */
int snd_i2c_register_driver(struct snd_i2c_driver *driver);
int snd_i2c_unregister_driver(struct snd_i2c_driver *driver);

/* send a command to a chip using the ioctl-like callback interface */
int snd_i2c_control_device(struct snd_i2c_bus *bus, int id,
			   unsigned int cmd, void *arg);

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

#endif				/* __I2C_H */
