/*
 *  ISA Plug & Play support
 *  Copyright (c) by Jaroslav Kysela <perex@jcu.cz>
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

#include <linux/config.h>
#include <linux/version.h>

#define LinuxVersionCode(v, p, s) (((v)<<16)|((p)<<8)|(s))

#if LinuxVersionCode(2, 0, 0) > LINUX_VERSION_CODE
#error "This driver is designed only for Linux 2.0.0 and highter."
#endif
#if LinuxVersionCode(2, 1, 0) <= LINUX_VERSION_CODE
#define LINUX_2_1
#if LinuxVersionCode(2, 1, 127) > LINUX_VERSION_CODE
#error "This driver requires Linux 2.1.127 and highter."
#endif
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/string.h>
#include <linux/malloc.h>
#include <asm/io.h>

#ifndef __initfunc
#define __initfunc(__initarg) __initarg 
#else
#include <linux/init.h>
#endif

#include "isapnp.h"

int isapnp_rdp = 0;			/* Read Data Port */
int isapnp_verbose = 0;			/* verbose mode */
#ifdef MODULE_PARM
MODULE_AUTHOR("Jaroslav Kysela <perex@jcu.cz>");
MODULE_DESCRIPTION("Generic ISA Plug & Play support");
MODULE_PARM(isapnp_rdp, "i");
MODULE_PARM_DESC(isapnp_rdp, "ISA Plug & Play read data port");
MODULE_PARM(isapnp_verbose, "i");
MODULE_PARM_DESC(isapnp_verbose, "ISA Plug & Play verbose mode");
#endif

#define _PIDXR		0x279
#define _PNPWRP		0xa79

/* short tags */
#define _STAG_PNPVERNO		0x01
#define _STAG_LOGDEVID		0x02
#define _STAG_COMPATDEVID	0x03
#define _STAG_IRQ		0x04
#define _STAG_DMA		0x05
#define _STAG_STARTDEP		0x06
#define _STAG_ENDDEP		0x07
#define _STAG_IOPORT		0x08
#define _STAG_FIXEDIO		0x09
#define _STAG_VENDOR		0x0e
#define _STAG_END		0x0f
/* long tags */
#define _LTAG_MEMRANGE		0x81
#define _LTAG_ANSISTR		0x82
#define _LTAG_UNICODESTR	0x83
#define _LTAG_VENDOR		0x84
#define _LTAG_MEM32RANGE	0x85
#define _LTAG_FIXEDMEM32RANGE	0x86

extern void isapnp_proc_init(void);
extern void isapnp_proc_done(void);

struct isapnp_dev *isapnp_devices = NULL;	/* device list */
static unsigned char isapnp_checksum_value;
static struct semaphore isapnp_cfg_mutex = MUTEX;

/* delay ten microseconds */

static void isapnp_delay(int loops)
{
	int i;

	while (loops-- > 0)
		for (i = 0; i < 16; i++)
			inb(0x80);
}

void *isapnp_alloc(long size)
{
	void *result;

	result = kmalloc(size, GFP_KERNEL);
	if (!result)
		return NULL;
	memset(result, 0, size);
	return result;
}

static void isapnp_key(void)
{
	unsigned char code = 0x6a, msb;
	int i;

	outb(0x00, _PIDXR);
	outb(0x00, _PIDXR);

	outb(code, _PIDXR);

	for (i = 1; i < 32; i++) {
		msb = ((code & 0x01) ^ ((code & 0x02) >> 1)) << 7;
		code = (code >> 1) | msb;
		outb(code, _PIDXR);
	}
}

static void isapnp_wait(void)
{
	outb(0x02, _PIDXR);
	outb(0x02, _PNPWRP);	/* place all pnp cards in wait-for-key state */
}

void isapnp_wake(unsigned char csn)
{
	outb(0x03, _PIDXR);	/* select PWAKEI */
	outb(csn, _PNPWRP);	/* write csn */
}

void isapnp_logdev(unsigned char dev)
{
	outb(0x07, _PIDXR);	/* select PLDNI */
	outb(dev, _PNPWRP);	/* write dev */
}

void isapnp_activate(unsigned char value)
{
	if (value)
		value = 0x01;
	outb(ISAPNP_CFG_ACTIVATE, _PIDXR);
	outb(value, _PNPWRP);	/* write mode */
	isapnp_delay(25);
}

static void isapnp_peek(unsigned char *data, int bytes)
{
	int i, j;
	unsigned char d;

	for (i = 1; i <= bytes; i++) {
		for (j = 0; j < 10; j++) {
			outb(0x05, _PIDXR);	/* select PRESSI */
			d = inb(isapnp_rdp);
			if (d & 1)
				break;
			isapnp_delay(1);
		}
		if (!(d & 1)) {
			*data++ = 0xff;
			continue;
		}
		outb(0x04, _PIDXR);	/* select PRESDI */
		d = inb(isapnp_rdp);	/* read resource byte */
		isapnp_checksum_value += d;
		if (data != NULL)
			*data++ = d;
	}
}

static int isapnp_next_rdp(int rdp)
{
	while (rdp <= 0x3ff && check_region(rdp, 1))
		rdp += 4;
	return rdp;
}

__initfunc(static int isapnp_isolate_rdp_select(int rdp))
{
	isapnp_wait();
	isapnp_key();
	outb(0x02, _PIDXR);	/* control */
	outb(0x07, _PNPWRP);	/* reset driver */
	isapnp_delay(200);	/* delay 2000us */
	isapnp_key();
	isapnp_wake(0x00);
	outb(0x00, _PIDXR);
	rdp = isapnp_next_rdp(rdp);
	if (rdp > 0x3ff) {
		isapnp_wait();
		return -1;
	}
	outb((unsigned char) (rdp >> 2), _PNPWRP);
	isapnp_delay(100);	/* delay 1000us */
	outb(0x01, _PIDXR);
	isapnp_delay(100);	/* delay 1000us */
	return rdp;
}

/*
 *  Isolate (assign uniqued CSN) to all ISA PnP devices.
 */

__initfunc(static int isapnp_isolate(void))
{
	unsigned char checksum = 0x6a;
	unsigned char chksum = 0x00;
	unsigned char bit = 0x00;
	int rdp = 0x203;
	int data;
	int csn = 0;
	int i;
	int iteration = 1;

	if ((rdp = isapnp_isolate_rdp_select(rdp))<0)
		return -1;

	while (1) {
		for (i = 1; i <= 64; i++) {
			data = ((short) inb(rdp)) << 8;
			isapnp_delay(25);
			data = data | inb(rdp);
			isapnp_delay(25);
			if (data == 0x55aa)
				bit = 0x01;
			checksum = ((((checksum ^ (checksum >> 1)) & 0x01) ^ bit) << 7) | (checksum >> 1);
			bit = 0x00;
		}
		for (i = 65; i <= 72; i++) {
			data = ((short) inb(rdp)) << 8;
			isapnp_delay(25);
			data = data | inb(rdp);
			isapnp_delay(25);
			if (data == 0x55aa)
				chksum |= (1 << (i - 65));
		}
		if (checksum != 0x00 && checksum == chksum) {
			csn++;
			outb(0x06, _PIDXR);
			outb(csn, _PNPWRP);
			isapnp_delay(25);
			iteration++;
			isapnp_wake(0x00);
			outb(0x01, _PIDXR);
			goto __next;
		}
		if (iteration == 1) {
			rdp += 0x04;
			if ((rdp = isapnp_isolate_rdp_select(rdp))<0)
				return -1;
		} else if (iteration > 1) {
			break;
		}
	      __next:
		checksum = 0x6a;
		chksum = 0x00;
		bit = 0x00;
	}
	isapnp_rdp = rdp;
	isapnp_wait();
	return csn;
}

/*
 *  Read one tag from stream.
 */

__initfunc(static int isapnp_read_tag(unsigned char *type, unsigned short *size))
{
	unsigned char tag, tmp[2];

	isapnp_peek(&tag, 1);
	if (tag == 0)				/* invalid tag */
		return -1;
	if (tag & 0x80) {	/* large item */
		*type = tag;
		isapnp_peek(tmp, 2);
		*size = (tmp[1] << 8) | tmp[0];
	} else {
		*type = (tag >> 3) & 0x0f;
		*size = tag & 0x07;
	}
#if 0
	printk("tag = 0x%x, type = 0x%x, size = %i\n", tag, *type, *size);
#endif
	if (type == 0)				/* wrong type */
		return -1;
	if (*type == 0xff && *size == 0xffff)	/* probably invalid data */
		return -1;
	return 0;
}

/*
 *  Skip specified number of bytes from stream.
 */
 
__initfunc(static void isapnp_skip_bytes(int count))
{
	isapnp_peek(NULL, count);
}

/*
 *  Parse logical device tag.
 */

__initfunc(static struct isapnp_logdev *isapnp_parse_logdev(struct isapnp_dev *dev, int size, int number))
{
	unsigned char tmp[6];
	struct isapnp_logdev *logdev;

	isapnp_peek(tmp, size);
	logdev = isapnp_alloc(sizeof(struct isapnp_logdev));
	if (!logdev)
		return NULL;
	logdev->number = number;
	logdev->vendor = (tmp[1] << 8) | tmp[0];
	logdev->function = (tmp[3] << 8) | tmp[2];
	logdev->regs = tmp[4];
	logdev->dev = dev;
	if (size > 5)
		logdev->regs |= tmp[5] << 8;
	return logdev;
}

/*
 *  Build new resources structure
 */

__initfunc(static struct isapnp_resources *isapnp_build_resources(struct isapnp_logdev *logdev, int dependent))
{
	struct isapnp_resources *res, *ptr, *ptra;
	
	res = isapnp_alloc(sizeof(struct isapnp_resources));
	if (!res)
		return NULL;
	ptr = logdev->res;
	while (ptr && ptr->next)
		ptr = ptr->next;
	if (ptr && ptr->dependent && dependent) { /* add to another list */
		ptra = ptr->alt;
		while (ptra && ptra->alt)
			ptra = ptra->alt;
		if (!ptra)
			ptr->alt = res;
		else
			ptra->alt = res;
	} else {
		if (!ptr)
			logdev->res = res;
		else
			ptr->next = res;
	}
	if (dependent) {
		res->priority = dependent & 0xff;
		if (res->priority > ISAPNP_RES_PRIORITY_FUNCTIONAL)
			res->priority = ISAPNP_RES_PRIORITY_INVALID;
		res->dependent = 1;
	} else {
		res->priority = ISAPNP_RES_PRIORITY_PREFFERED;
		res->dependent = 0;
	}
	return res;
}

/*
 *  Add IRQ resource to resources list.
 */

__initfunc(static void isapnp_add_irq_resource(struct isapnp_logdev *logdev,
                                    	       struct isapnp_resources **res,
                                               int dependent, int size))
{
	unsigned char tmp[3];
	struct isapnp_irq *irq, *ptr;

	isapnp_peek(tmp, size);
	irq = isapnp_alloc(sizeof(struct isapnp_irq));
	if (!irq)
		return;
	if (*res == NULL) {
		*res = isapnp_build_resources(logdev, dependent);
		if (*res == NULL) {
			kfree(irq);
			return;
		}
	}
	irq->map = (tmp[1] << 8) | tmp[0];
	if (size > 2)
		irq->flags = tmp[2];
	else
		irq->flags = ISAPNP_IRQ_FLAG_HIGHEDGE;
	ptr = (*res)->irq;
	while (ptr && ptr->next)
		ptr = ptr->next;
	if (ptr)
		ptr->next = irq;
	else
		(*res)->irq = irq;
}

/*
 *  Add DMA resource to resources list.
 */

__initfunc(static void isapnp_add_dma_resource(struct isapnp_logdev *logdev,
                                    	       struct isapnp_resources **res,
                                    	       int dependent, int size))
{
	unsigned char tmp[2];
	struct isapnp_dma *dma, *ptr;

	isapnp_peek(tmp, size);
	dma = isapnp_alloc(sizeof(struct isapnp_dma));
	if (!dma)
		return;
	if (*res == NULL) {
		*res = isapnp_build_resources(logdev, dependent);
		if (*res == NULL) {
			kfree(dma);
			return;
		}
	}
	dma->map = tmp[0];
	dma->type = tmp[1] & 3;
	dma->flags = (tmp[1] >> 2) & 7;
	dma->speed = (tmp[1] >> 6) & 3;
	ptr = (*res)->dma;
	while (ptr && ptr->next)
		ptr = ptr->next;
	if (ptr)
		ptr->next = dma;
	else
		(*res)->dma = dma;
}

/*
 *  Add port resource to resources list.
 */

__initfunc(static void isapnp_add_port_resource(struct isapnp_logdev *logdev,
						struct isapnp_resources **res,
						int dependent, int size))
{
	unsigned char tmp[7];
	struct isapnp_port *port, *ptr;

	isapnp_peek(tmp, size);
	port = isapnp_alloc(sizeof(struct isapnp_port));
	if (!port)
		return;
	if (*res == NULL) {
		*res = isapnp_build_resources(logdev, dependent);
		if (*res == NULL) {
			kfree(port);
			return;
		}
	}
	port->min = (tmp[2] << 8) | tmp[1];
	port->max = (tmp[4] << 8) | tmp[3];
	port->align = tmp[5];
	port->size = tmp[6];
	port->flags = tmp[0] ? ISAPNP_PORT_FLAG_16BITADDR : 0;
	ptr = (*res)->port;
	while (ptr && ptr->next)
		ptr = ptr->next;
	if (ptr)
		ptr->next = port;
	else
		(*res)->port = port;
}

/*
 *  Add fixed port resource to resources list.
 */

__initfunc(static void isapnp_add_fixed_port_resource(struct isapnp_logdev *logdev,
						      struct isapnp_resources **res,
						      int dependent, int size))
{
	unsigned char tmp[3];
	struct isapnp_fixed_port *port, *ptr;

	isapnp_peek(tmp, size);
	port = isapnp_alloc(sizeof(struct isapnp_fixed_port));
	if (!port)
		return;
	if (*res == NULL) {
		*res = isapnp_build_resources(logdev, dependent);
		if (*res == NULL) {
			kfree(port);
			return;
		}
	}
	port->port = (tmp[1] << 8) | tmp[0];
	port->size = tmp[2];
	ptr = (*res)->fixport;
	while (ptr && ptr->next)
		ptr = ptr->next;
	if (ptr)
		ptr->next = port;
	else
		(*res)->fixport = port;
}

/*
 *  Add memory resource to resources list.
 */

__initfunc(static void isapnp_add_mem_resource(struct isapnp_logdev *logdev,
					       struct isapnp_resources **res,
					       int dependent, int size))
{
	unsigned char tmp[9];
	struct isapnp_mem *mem, *ptr;

	isapnp_peek(tmp, size);
	mem = isapnp_alloc(sizeof(struct isapnp_mem));
	if (!mem)
		return;
	if (*res == NULL) {
		*res = isapnp_build_resources(logdev, dependent);
		if (*res == NULL) {
			kfree(mem);
			return;
		}
	}
	mem->min = ((tmp[2] << 8) | tmp[1]) << 8;
	mem->max = ((tmp[4] << 8) | tmp[3]) << 8;
	mem->align = (tmp[6] << 8) | tmp[5];
	mem->size = ((tmp[8] << 8) | tmp[7]) << 8;
	mem->flags = tmp[0] & 7;
	mem->flags |= (tmp[0] >> 2) & 0x18;
	mem->type = (tmp[0] >> 3) & 3;
	ptr = (*res)->mem;
	while (ptr && ptr->next)
		ptr = ptr->next;
	if (ptr)
		ptr->next = mem;
	else
		(*res)->mem = mem;
}

/*
 *  Add 32-bit memory resource to resources list.
 */

__initfunc(static void isapnp_add_mem32_resource(struct isapnp_logdev *logdev,
						 struct isapnp_resources **res,
						 int dependent, int size))
{
	unsigned char tmp[17];
	struct isapnp_mem32 *mem32, *ptr;

	isapnp_peek(tmp, size);
	mem32 = isapnp_alloc(sizeof(struct isapnp_mem32));
	if (!mem32)
		return;
	if (*res == NULL) {
		*res = isapnp_build_resources(logdev, dependent);
		if (*res == NULL) {
			kfree(mem32);
			return;
		}
	}
	memcpy(mem32->data, tmp, 17);
	ptr = (*res)->mem32;
	while (ptr && ptr->next)
		ptr = ptr->next;
	if (ptr)
		ptr->next = mem32;
	else
		(*res)->mem32 = mem32;
}

/*
 *  Add 32-bit fixed memory resource to resources list.
 */

__initfunc(static void isapnp_add_fixed_mem32_resource(struct isapnp_logdev *logdev,
						       struct isapnp_resources **res,
						       int dependent, int size))
{
	unsigned char tmp[17];
	struct isapnp_fixed_mem32 *fmem32, *ptr;

	isapnp_peek(tmp, size);
	fmem32 = isapnp_alloc(sizeof(struct isapnp_mem32));
	if (!fmem32)
		return;
	if (*res == NULL) {
		*res = isapnp_build_resources(logdev, dependent);
		if (*res == NULL) {
			kfree(fmem32);
			return;
		}
	}
	memcpy(fmem32->data, tmp, 17);
	ptr = (*res)->fixmem32;
	while (ptr && ptr->next)
		ptr = ptr->next;
	if (ptr)
		ptr->next = fmem32;
	else
		(*res)->fixmem32 = fmem32;
}

/*
 *  Parse resource map for logical device.
 */

__initfunc(static int isapnp_create_logdev(struct isapnp_dev *dev,
					   unsigned short size))
{
	int number = 0, skip = 0, dependent = 0;
	unsigned char type, tmp[17];
	struct isapnp_logdev *logdev, *prev_logdev;
	struct isapnp_compatid *compat, *prev_compat = NULL;
	struct isapnp_resources *res = NULL;
	
	if ((logdev = isapnp_parse_logdev(dev, size, number++)) == NULL)
		return 1;
	dev->logdev = logdev;
	while (1) {
		if (isapnp_read_tag(&type, &size)<0)
			return 1;
		if (skip && type != _STAG_LOGDEVID && type != _STAG_END)
			goto __skip;
		switch (type) {
		case _STAG_LOGDEVID:
			prev_logdev = logdev;
			if (size >= 5 && size <= 6) {
				if ((logdev = isapnp_parse_logdev(dev, size, number++)) == NULL)
					return 1;
				prev_logdev->next = logdev;
				size = 0;
				skip = 0;
			} else {
				skip = 1;
			}
			res = NULL;
			dependent = 0;
			break;
		case _STAG_COMPATDEVID:
			if (size == 4) {
				compat = isapnp_alloc(sizeof(struct isapnp_compatid));
				if (compat) {
					isapnp_peek(tmp, 4);
					compat->vendor = (tmp[1] << 8) | tmp[0];
					compat->function = (tmp[3] << 8) | tmp[2];
					if (prev_compat)
						prev_compat->next = compat;
					else
						logdev->compat = compat;
					prev_compat = compat;
					size = 0;
				}
			}
			break;
		case _STAG_IRQ:
			if (size < 2 || size > 3)
				goto __skip;
			isapnp_add_irq_resource(logdev, &res, dependent, size);
			size = 0;
			break;
		case _STAG_DMA:
			if (size != 2)
				goto __skip;
			isapnp_add_dma_resource(logdev, &res, dependent, size);
			size = 0;
			break;
		case _STAG_STARTDEP:
			if (size > 1)
				goto __skip;
			res = NULL;
			dependent = 0x100 | ISAPNP_RES_PRIORITY_ACCEPTABLE;
			if (size > 0) {
				isapnp_peek(tmp, size);
				dependent = 0x100 | tmp[0];
				size = 0;
			}
			break;
		case _STAG_ENDDEP:
			if (size != 0)
				goto __skip;
			res = NULL;
			dependent = 0;
			break;
		case _STAG_IOPORT:
			if (size != 7)
				goto __skip;
			isapnp_add_port_resource(logdev, &res, dependent, size);
			size = 0;
			break;
		case _STAG_FIXEDIO:
			if (size != 3)
				goto __skip;
			isapnp_add_fixed_port_resource(logdev, &res, dependent, size);
			size = 0;
			break;
		case _STAG_VENDOR:
			break;
		case _LTAG_MEMRANGE:
			if (size != 9)
				goto __skip;
			isapnp_add_mem_resource(logdev, &res, dependent, size);
			size = 0;
			break;
		case _LTAG_ANSISTR:
			if (!logdev->identifier) {
				logdev->identifier = isapnp_alloc(size+1);
				if (logdev->identifier) {
					isapnp_peek(logdev->identifier, size);
					size = 0;
				}
			}
			break;
		case _LTAG_UNICODESTR:
			/* silently ignore */
			/* who use unicode for hardware identification? */
			break;
		case _LTAG_VENDOR:
			break;
		case _LTAG_MEM32RANGE:
			if (size != 17)
				goto __skip;
			isapnp_add_mem32_resource(logdev, &res, dependent, size);
			size = 0;
			break;
		case _LTAG_FIXEDMEM32RANGE:
			if (size != 17)
				goto __skip;
			isapnp_add_fixed_mem32_resource(logdev, &res, dependent, size);
			size = 0;
			break;
		case _STAG_END:
			if (size > 0)
				isapnp_skip_bytes(size);
			return 1;
		default:
			printk("isapnp: unexpected or unknown tag type 0x%x for logical device %i (device %i), ignored\n", type, logdev->number, dev->csn);
		}
	      __skip:
	      	if (size > 0)
		      	isapnp_skip_bytes(size);
	}
	return 0;
}

/*
 *  Parse resource map for ISA PnP device.
 */
 
__initfunc(static void isapnp_parse_resource_map(struct isapnp_dev *dev))
{
	unsigned char type, tmp[17];
	unsigned short size;
	
	while (1) {
		if (isapnp_read_tag(&type, &size)<0)
			return;
		switch (type) {
		case _STAG_PNPVERNO:
			if (size != 2)
				goto __skip;
			isapnp_peek(tmp, 2);
			dev->pnpver = tmp[0];
			dev->productver = tmp[1];
			size = 0;
			break;
		case _STAG_LOGDEVID:
			if (size >= 5 && size <= 6) {
				if (isapnp_create_logdev(dev, size)==1)
					return;
				size = 0;
			}
			break;
		case _STAG_VENDOR:
			break;
		case _LTAG_ANSISTR:
			if (!dev->identifier) {
				dev->identifier = isapnp_alloc(size+1);
				if (dev->identifier) {
					isapnp_peek(dev->identifier, size);
					size = 0;
				}
			}
			break;
		case _LTAG_UNICODESTR:
			/* silently ignore */
			/* who use unicode for hardware identification? */
			break;
		case _LTAG_VENDOR:
			break;
		case _STAG_END:
			if (size > 0)
				isapnp_skip_bytes(size);
			return;
		default:
			printk("isapnp: unexpected or unknown tag type 0x%x for device %i, ignored\n", type, dev->csn);
		}
	      __skip:
	      	if (size > 0)
		      	isapnp_skip_bytes(size);
	}
}

/*
 *  Compute ISA PnP checksum for first eight bytes.
 */

__initfunc(static unsigned char isapnp_checksum(unsigned char *data))
{
	int i, j;
	unsigned char checksum = 0x6a, bit, b;
	
	for (i = 0; i < 8; i++) {
		b = data[i];
		for (j = 0; j < 8; j++) {
			bit = 0;
			if (b & (1 << j))
				bit = 1;
			checksum = ((((checksum ^ (checksum >> 1)) & 0x01) ^ bit) << 7) | (checksum >> 1);
		}
	}
	return checksum;
}

/*
 *  Build device list for all present ISA PnP devices.
 */

__initfunc(static int isapnp_build_device_list(void))
{
	int csn;
	unsigned char header[9], checksum;
	struct isapnp_dev *dev, *prev = NULL;

	isapnp_wait();
	isapnp_key();
	for (csn = 1; csn <= 10; csn++) {
		isapnp_wake(csn);
		isapnp_peek(header, 9);
		checksum = isapnp_checksum(header);
#if 0
		printk("vendor: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
			header[0], header[1], header[2], header[3],
			header[4], header[5], header[6], header[7], header[8]);
		printk("checksum = 0x%x\n", checksum);
#endif
		if (checksum == 0x00 || checksum != header[8])	/* not valid CSN */
			continue;
		if ((dev = isapnp_alloc(sizeof(struct isapnp_dev))) == NULL)
			continue;
		dev->csn = csn;
		dev->vendor = (header[1] << 8) | header[0];
		dev->device = (header[3] << 8) | header[2];
		dev->serial = (header[7] << 24) | (header[6] << 16) | (header[5] << 8) | header[4];
		isapnp_checksum_value = 0x00;
		isapnp_parse_resource_map(dev);
		if (isapnp_checksum_value != 0x00)
			printk("isapnp: checksum for device %i isn't valid (0x%x)\n", csn, isapnp_checksum_value);
		dev->checksum = isapnp_checksum_value;
		if (!isapnp_devices)
			isapnp_devices = dev;
		else
			prev->next = dev;
		prev = dev;
	}
	return 0;
}

/*
 *  Basic configuration routines.
 */

int isapnp_cfg_begin(int csn, int logdev)
{
	if (csn < 1 || csn > 10 || logdev > 10)
		return -EINVAL;
	down(&isapnp_cfg_mutex);
	isapnp_wait();
	isapnp_key();
	isapnp_wake(csn);
	if (logdev >= 0)
		isapnp_logdev(logdev);
	return 0;
}

int isapnp_cfg_end(void)
{
	isapnp_wait();
	up(&isapnp_cfg_mutex);
	return 0;
}

unsigned char isapnp_cfg_get_byte(unsigned char idx)
{
	outb(idx, _PIDXR);
	return inb(isapnp_rdp);
}

unsigned short isapnp_cfg_get_word(unsigned char idx)
{
	unsigned short val;

	outb(idx, _PIDXR);
	val = ((unsigned short) inb(isapnp_rdp)) << 8;
	outb(idx + 1, _PIDXR);
	return val | inb(isapnp_rdp);
}

unsigned int isapnp_cfg_get_dword(unsigned char idx)
{
	unsigned int val;

	outb(idx, _PIDXR);
	val = ((unsigned int) inb(isapnp_rdp)) << 24;
	outb(idx + 1, _PIDXR);
	val |= ((unsigned int) inb(isapnp_rdp)) << 16;
	outb(idx + 2, _PIDXR);
	val |= ((unsigned int) inb(isapnp_rdp)) << 8;
	outb(idx + 3, _PIDXR);
	return val | inb(isapnp_rdp);
}

void isapnp_cfg_set_byte(unsigned char idx, unsigned char val)
{
	outb(idx, _PIDXR);
	outb(val, _PNPWRP);
}

void isapnp_cfg_set_word(unsigned char idx, unsigned short val)
{
	outb(idx, _PIDXR);
	outb(val >> 8, _PNPWRP);
	outb(idx + 1, _PIDXR);
	outb((unsigned char) val, _PNPWRP);
}

void isapnp_cfg_set_dword(unsigned char idx, unsigned int val)
{
	outb(idx, _PIDXR);
	outb(val >> 24, _PNPWRP);
	outb(idx + 1, _PIDXR);
	outb((unsigned char) (val >> 16), _PNPWRP);
	outb(idx + 2, _PIDXR);
	outb((unsigned char) (val >> 8), _PNPWRP);
	outb(idx + 3, _PIDXR);
	outb((unsigned char) val, _PNPWRP);
}

/*
 *  Device manager.
 */

struct isapnp_dev *isapnp_find_device(unsigned short vendor,
				      unsigned short device,
				      int index)
{
	struct isapnp_dev *dev;
	
	if (index < 0 || index > 255)
		return NULL;
	for (dev = isapnp_devices; dev; dev = dev->next) {
		if (dev->vendor && dev->device == device) {
			if (!index)
				return dev;
			index--;
		}
	}
	return NULL;
}

struct isapnp_logdev *isapnp_find_logdev(struct isapnp_dev *dev,
					 unsigned short vendor,
					 unsigned short function,
					 int index)
{
	struct isapnp_logdev *logdev;
	struct isapnp_compatid *compat;
	
	if (index < 0 || index > 255)
		return NULL;
	if (!dev) {	/* look for logical device in all devices */
		for (dev = isapnp_devices; dev; dev = dev->next)
			for (logdev = dev->logdev; logdev; logdev = logdev->next) {
				if (logdev->vendor == vendor && logdev->function == function) {
					if (!index)
						return logdev;
					index--;
					continue;
				} else {
					for (compat = logdev->compat; compat; compat = compat->next)
						if (compat->vendor == vendor && logdev->function == function) {
							if (!index)
								return logdev;
							index--;
							break;
						}
				}
			}
	} else {
		for (logdev = dev->logdev; logdev; logdev = logdev->next) {
			if (logdev->vendor == vendor && logdev->function == function) {
				if (!index)
					return logdev;
				index--;
				continue;
			} else {
				for (compat = logdev->compat; compat; compat = compat->next)
					if (compat->vendor == vendor && logdev->function == function) {
						if (!index)
							return logdev;
						index--;
						break;
					}
			}
		}		
	}
	return NULL;
}

int isapnp_config_init(struct isapnp_config *config, struct isapnp_logdev *logdev)
{
	struct isapnp_resources *res, *resa;
	struct isapnp_port *port;
	struct isapnp_irq *irq;
	struct isapnp_dma *dma;
	struct isapnp_mem *mem;
	int port_count, port_count1;
	int irq_count, irq_count1;
	int dma_count, dma_count1;
	int mem_count, mem_count1;

	if (!config || !logdev)
		return -EINVAL;
	memset(config, 0, sizeof(struct isapnp_config));
	config->irq[0] = config->irq[1] = 255;
	config->dma[0] = config->dma[1] = 255;
	for (res = logdev->res; res; res = res->next) {
		port_count = irq_count = dma_count = mem_count = 0;
		for (resa = res; resa; resa = resa->next) {
			port_count1 = 0;
			for (port = resa->port; port; port = port->next)
				port_count1++;
			if (port_count < port_count1)
				port_count = port_count1;
			irq_count1 = 0;
			for (irq = resa->irq; irq; irq = irq->next)
				irq_count1++;
			if (irq_count < irq_count1)
				irq_count = irq_count1;
			dma_count1 = 0;
			for (dma = resa->dma; dma; dma = dma->next)
				dma_count1++;
			if (dma_count < dma_count1)
				dma_count = dma_count1;
			mem_count1 = 0;
			for (mem = resa->mem; mem; mem = mem->next)
				mem_count1++;
			if (mem_count < mem_count1)
				mem_count = mem_count1;
		}
		config->port_count += port_count;
		config->irq_count += irq_count;
		config->dma_count += dma_count;
		config->mem_count += mem_count;
	}
	return 0;
}

int isapnp_configure(struct isapnp_config *config)
{
	struct isapnp_config result;
	struct isapnp_resources *res[16], *restmp;
	int res_count, res_idx;
	struct isapnp_port *port;
	struct isapnp_irq *irq;
	struct isapnp_dma *dma;
	struct isapnp_mem *mem;
	int port_idx[16];
	int irq_idx[16];
	int dma_idx[16];
	int mem_idx[16];
	int tmp, tmp1, auto;
	
	if (!config || !config->logdev)
		return -EINVAL;
	memcpy(&result, config, sizeof(struct isapnp_config));
	/* check if all values are set, otherwise try auto-configuration */
	for (tmp = auto = 0; !auto && tmp < config.port_count; tmp++) {
		if (!config.port[tmp])
			auto++;
	}
	for (tmp = 0; !auto && tmp < config.irq_count; tmp++) {
		if (config.irq[tmp] == 255)
			auto++;
	}
	for (tmp = 0; !auto && tmp < config.dma_count; tmp++) {
		if (config.dma[tmp] == 255)
			auto++;
	}
	for (tmp = 0; !auto && tmp < config.mem_count; tmp++) {
		if (!config.mem[tmp])
			auto++;
	}
	if (!auto)
		goto __skip_auto;
	/* set variables to initial values */
	res_count = 0;
	port_idx[0] = irq_idx[0] = dma_idx[0] = mem_idx[0] = 0;
	for (restmp = logdev->res; restmp && res_count < 15; restmp = restmp->next) {
		port_idx[res_count+1] = port_idx[res_count];
		irq_idx[res_count+1] = irq_idx[res_count];
		dma_idx[res_count+1] = dma_idx[res_count];
		mem_idx[res_count+1] = res_idx[res_count];
		for (port = restmp->port; port; port = port->next)
			port_idx[res_count+1]++;
		for (irq = restmp->irq; irq; irq = irq->next)
			irq_idx[res_count+1]++;
		for (dma = restmp->dma; dma; dma = dma->next)
			dma_idx[res_count+1]++;
		for (mem = restmp->mem; mem; mem = mem->next)
			mem_idx[res_count+1]++;
		res[res_count++] = restmp;
	}
	/* ok. we are looking for first valid configuration */
	for (res_idx = 0; res_idx < res_count; res_idx++) {
		for (tmp = 0; tmp < config.port_count; tmp++) {
			for (tmp1 = 0; tmp1 < res_count && tmp < port_idx[tmp1]; tmp1++);
			if (tmp1 >= res_count)
				return -ENOENT;
		}
	}
      __skip_auto:
      	/* we have valid configuration, try configure hardware */
	return -1;
}

/*
 *  Inititialization.
 */

#ifdef MODULE

static void isapnp_free_port(struct isapnp_port *port)
{
	struct isapnp_port *next;

	while (port) {
		next = port->next;
		kfree(port);
		port = next;
	}
}

static void isapnp_free_fixed_port(struct isapnp_fixed_port *fixport)
{
	struct isapnp_fixed_port *next;

	while (fixport) {
		next = fixport->next;
		kfree(fixport);
		fixport = next;
	}
}

static void isapnp_free_irq(struct isapnp_irq *irq)
{
	struct isapnp_irq *next;

	while (irq) {
		next = irq->next;
		kfree(irq);
		irq = next;
	}
}

static void isapnp_free_dma(struct isapnp_dma *dma)
{
	struct isapnp_dma *next;

	while (dma) {
		next = dma->next;
		kfree(dma);
		dma = next;
	}
}

static void isapnp_free_mem(struct isapnp_mem *mem)
{
	struct isapnp_mem *next;

	while (mem) {
		next = mem->next;
		kfree(mem);
		mem = next;
	}
}

static void isapnp_free_mem32(struct isapnp_mem32 *mem32)
{
	struct isapnp_mem32 *next;

	while (mem32) {
		next = mem32->next;
		kfree(mem32);
		mem32 = next;
	}
}

static void isapnp_free_fixed_mem32(struct isapnp_fixed_mem32 *fixmem32)
{
	struct isapnp_fixed_mem32 *next;

	while (fixmem32) {
		next = fixmem32->next;
		kfree(fixmem32);
		fixmem32 = next;
	}
}

static void isapnp_free_compatid(struct isapnp_compatid *compat)
{
	struct isapnp_compatid *next;

	while (compat) {
		next = compat->next;
		kfree(compat);
		compat = next;
	}
}

static void isapnp_free_resources(struct isapnp_resources *resources, int alt)
{
	struct isapnp_resources *next;

	while (resources) {
		next = alt ? resources->alt : resources->next;
		isapnp_free_port(resources->port);
		isapnp_free_fixed_port(resources->fixport);
		isapnp_free_irq(resources->irq);
		isapnp_free_dma(resources->dma);
		isapnp_free_mem(resources->mem);
		isapnp_free_mem32(resources->mem32);
		isapnp_free_fixed_mem32(resources->fixmem32);
		if (!alt && resources->alt)
			isapnp_free_resources(resources->alt, 1);
		kfree(resources);
		resources = next;
	}
}

static void isapnp_free_logdev(struct isapnp_logdev *logdev)
{
	struct isapnp_logdev *next;

	while (logdev) {
		next = logdev->next;
		if (logdev->identifier)
			kfree(logdev->identifier);
		isapnp_free_compatid(logdev->compat);
		isapnp_free_resources(logdev->res, 0);
		kfree(logdev);
		logdev = next;
	}
}

#endif /* MODULE */

static void isapnp_free_all_resources(void)
{
	struct isapnp_dev *dev, *devnext;

	release_region(_PIDXR, 1);
	release_region(_PNPWRP, 1);
	if (isapnp_rdp >= 0x203 && isapnp_rdp <= 0x3ff)
		release_region(isapnp_rdp, 1);
#ifdef MODULE
	for (dev = isapnp_devices; dev; dev = devnext) {
		devnext = dev->next;
		if (dev->identifier)
			kfree(dev->identifier);
		isapnp_free_logdev(dev->logdev);
		kfree(dev);
	}
	isapnp_proc_done();
#endif
}

__initfunc(int isapnp_init(void))
{
	int devices;
	struct isapnp_dev *dev;
	struct isapnp_logdev *logdev;

	if (check_region(_PIDXR, 1)) {
		printk("isapnp: Index Register 0x%x already used\n", _PIDXR);
		return -EBUSY;
	}
	if (check_region(_PNPWRP, 1)) {
		printk("isapnp: Write Data Register 0x%x already used\n", _PNPWRP);
		return -EBUSY;
	}
	if (isapnp_rdp >= 0x203 && isapnp_rdp <= 0x3ff) {
		if (check_region(isapnp_rdp, 1)) {
			printk("isapnp: Read Data Register 0x%x already used\n", isapnp_rdp);
			return -EBUSY;
		}
		request_region(isapnp_rdp, 1, "ISA PnP read");
	}
	request_region(_PIDXR, 1, "ISA PnP index");
	request_region(_PNPWRP, 1, "ISA PnP write");
	if (isapnp_rdp < 0x203 || isapnp_rdp > 0x3ff) {
		devices = isapnp_isolate();
		if (devices <= 0) {
			isapnp_free_all_resources();
			return -ENOENT;
		}
		if (isapnp_rdp < 0x203 || isapnp_rdp > 0x3ff) {
			isapnp_free_all_resources();
			return -ENOENT;
		}
		request_region(isapnp_rdp, 1, "ISA PnP read");
	}
	isapnp_build_device_list();
	devices = 0;
	for (dev = isapnp_devices; dev; dev = dev->next)
		devices++;
	if (devices <= 0) {
		isapnp_free_all_resources();
		return -ENOENT;
	}
	if (isapnp_verbose) {
		for (dev = isapnp_devices; dev; dev = dev->next) {
			printk( "isapnp: Device '%s'\n", dev->identifier?dev->identifier:"Unknown");
			for (logdev = dev->logdev; logdev; logdev = logdev->next)
				printk("isapnp:   Logical device '%s'\n", logdev->identifier?logdev->identifier:"Unknown");
		}
	}
	printk("isapnp: %i Plug & Play device%s detected total\n", devices, devices>1?"s":"");
	isapnp_proc_init();
	return 0;
}

#ifdef MODULE

int init_module(void)
{
	return isapnp_init();
}

void cleanup_module(void)
{
	isapnp_free_all_resources();
}

#endif
