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
#include <asm/dma.h>
#include <asm/irq.h>

#ifndef __initfunc
#define __initfunc(__initarg) __initarg 
#else
#include <linux/init.h>
#endif

#include "isapnp.h"

#if 0
#define ISAPNP_REGION_OK
#endif

int isapnp_rdp = 0;			/* Read Data Port */
int isapnp_reset = 0;			/* reset all PnP cards (deactivate) */
int isapnp_verbose = 1;			/* verbose mode */
#ifdef MODULE_PARM
MODULE_AUTHOR("Jaroslav Kysela <perex@jcu.cz>");
MODULE_DESCRIPTION("Generic ISA Plug & Play support");
MODULE_PARM(isapnp_rdp, "i");
MODULE_PARM_DESC(isapnp_rdp, "ISA Plug & Play read data port");
MODULE_PARM(isapnp_reset, "i");
MODULE_PARM_DESC(isapnp_reset, "ISA Plug & Play reset all cards");
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

void isapnp_logdev(unsigned char logdev)
{
	outb(0x07, _PIDXR);	/* select PLDNI */
	outb(logdev, _PNPWRP);	/* write logical device */
}

void isapnp_activate(unsigned char logdev)
{
	outb(0x07, _PIDXR);	/* select PLDNI */
	outb(logdev, _PNPWRP);	/* write dev */
	outb(ISAPNP_CFG_ACTIVATE, _PIDXR);
	outb(0x01, _PNPWRP);	/* write mode */
	isapnp_delay(25);
}

void isapnp_deactivate(unsigned char logdev)
{
	outb(0x07, _PIDXR);	/* select PLDNI */
	outb(logdev, _PNPWRP);	/* write dev */
	outb(ISAPNP_CFG_ACTIVATE, _PIDXR);
	outb(0x00, _PNPWRP);	/* write mode */
}

__initfunc(static void isapnp_peek(unsigned char *data, int bytes))
{
	int i, j;
	unsigned char d;

	for (i = 1; i <= bytes; i++) {
		for (j = 0; j < 10; j++) {
			outb(0x05, _PIDXR);	/* select PRESSI */
			d = inb(isapnp_rdp);
			if (d & 1)
				break;
			isapnp_delay(1);;
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
#ifdef MODULE
	outb(isapnp_reset ? 0x07 : 0x06, _PNPWRP);	/* reset driver or csn */
#else
	outb(0x07, _PNPWRP);	/* reset driver */
#endif
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
	int rdp = /* 0x203 */ 0x213;	/* 0x203 - joystick range */
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
	res->logdev = logdev;
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
	irq->res = *res;
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
	dma->res = *res;
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
	port->res = *res;
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
	port->min = port->max = (tmp[1] << 8) | tmp[0];
	port->size = tmp[2];
	port->align = 0;
	port->flags = ISAPNP_PORT_FLAG_FIXED;
	port->res = *res;
	ptr = (*res)->port;
	while (ptr && ptr->next)
		ptr = ptr->next;
	if (ptr)
		ptr->next = port;
	else
		(*res)->port = port;
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
	mem->res = *res;
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
	mem32->res = *res;
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
	mem32->res = *res;
	ptr = (*res)->mem32;
	while (ptr && ptr->next)
		ptr = ptr->next;
	if (ptr)
		ptr->next = mem32;
	else
		(*res)->mem32 = mem32;
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

int isapnp_present(void)
{
	if (isapnp_devices)
		return 1;
	return 0;
}

int isapnp_cfg_begin(int csn, int logdev)
{
	if (csn < 1 || csn > 10 || logdev > 10)
		return -EINVAL;
	MOD_INC_USE_COUNT;
	down(&isapnp_cfg_mutex);
	isapnp_wait();
	isapnp_key();
	isapnp_wake(csn);
#if 1	/* to avoid malfunction when isapnptools is used */
	outb(0x00, _PIDXR);
	outb((unsigned char) (isapnp_rdp >> 2), _PNPWRP);
	isapnp_delay(100);	/* delay 1000us */
	outb(0x01, _PIDXR);
	isapnp_delay(100);	/* delay 1000us */
#endif
	if (logdev >= 0)
		isapnp_logdev(logdev);
	return 0;
}

int isapnp_cfg_end(void)
{
	isapnp_wait();
	up(&isapnp_cfg_mutex);
	MOD_DEC_USE_COUNT;
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
 *  Resource manager.
 */

struct isapnp_port *isapnp_find_port(struct isapnp_logdev *logdev, int index)
{
	struct isapnp_resources *res;
	struct isapnp_port *port;
	
	if (!logdev || index < 0 || index > 7)
		return NULL;
	for (res = logdev->res; res; res = res->next) {
		for (port = res->port; port; port = port->next) {
			if (!index)
				return port;
			index--;
		}
	}
	return NULL;
}

struct isapnp_irq *isapnp_find_irq(struct isapnp_logdev *logdev, int index)
{
	struct isapnp_resources *res, *resa;
	struct isapnp_irq *irq;
	int index1, index2, index3;
	
	if (!logdev || index < 0 || index > 7)
		return NULL;
	for (res = logdev->res; res; res = res->next) {
		index3 = 0;
		for (resa = res; resa; resa = resa->alt) {
			index1 = index;
			index2 = 0;
			for (irq = resa->irq; irq; irq = irq->next) {
				if (!index1)
					return irq;
				index1--;
				index2++;
			}
			if (index3 < index2)
				index3 = index2;
		}
		index -= index3;
	}
	return NULL;
}

struct isapnp_dma *isapnp_find_dma(struct isapnp_logdev *logdev, int index)
{
	struct isapnp_resources *res;
	struct isapnp_dma *dma;
	
	if (!logdev || index < 0 || index > 7)
		return NULL;
	for (res = logdev->res; res; res = res->next) {
		for (dma = res->dma; dma; dma = dma->next) {
			if (!index)
				return dma;
			index--;
		}
	}
	return NULL;
}

struct isapnp_mem *isapnp_find_mem(struct isapnp_logdev *logdev, int index)
{
	struct isapnp_resources *res;
	struct isapnp_mem *mem;
	
	if (!logdev || index < 0 || index > 7)
		return NULL;
	for (res = logdev->res; res; res = res->next) {
		for (mem = res->mem; mem; mem = mem->next) {
			if (!index)
				return mem;
			index--;
		}
	}
	return NULL;
}

struct isapnp_mem32 *isapnp_find_mem32(struct isapnp_logdev *logdev, int index)
{
	struct isapnp_resources *res;
	struct isapnp_mem32 *mem32;
	
	if (!logdev || index < 0 || index > 7)
		return NULL;
	for (res = logdev->res; res; res = res->next) {
		for (mem32 = res->mem32; mem32; mem32 = mem32->next) {
			if (!index)
				return mem32;
			index--;
		}
	}
	return NULL;
}

int isapnp_verify_port(struct isapnp_port *port, unsigned short base)
{
	if (!port)
		return -EINVAL;
	while (port) {
		if (port->min <= base && port->max >= base &&
		    ((base & (port->align-1)) == 0 || port->align == 0))
			return 0;
		if (port->res->alt)
			port = port->res->alt->port;
		else
			port = NULL;
	}
	return -ENOENT;
}

int isapnp_verify_irq(struct isapnp_irq *irq, unsigned char value)
{
	if (!irq || value > 15)
		return -EINVAL;
	while (irq) {
		if (irq->map & (1 << value))
			return 0;
		if (irq->res->alt)
			irq = irq->res->alt->irq;
		else
			irq = NULL;
	}
	return -ENOENT;
}

int isapnp_verify_dma(struct isapnp_dma *dma, unsigned char value)
{
	if (!dma || value > 7)
		return -EINVAL;
	while (dma) {
		if (dma->map & (1 << value))
			return 0;
		if (dma->res->alt)
			dma = dma->res->alt->dma;
		else
			dma = NULL;
	}
	return -ENOENT;
}

int isapnp_verify_mem(struct isapnp_mem *mem, unsigned int base)
{
	if (!mem)
		return -EINVAL;
	while (mem) {
		if (mem->min <= base && mem->max >= base &&
		    ((base & (mem->align-1)) == 0 || mem->align == 0))
			return 0;
		if (mem->res->alt)
			mem = mem->res->alt->mem;
		else
			mem = NULL;
	}
	return -ENOENT;
}

int isapnp_verify_mem32(struct isapnp_mem32 *mem32, unsigned int base)
{
	if (!mem32)
		return -EINVAL;
	while (mem32) {
		/* TODO!!! */
		if (mem32->res->alt)
			mem32 = mem32->res->alt->mem32;
		else
			mem32 = NULL;
	}
	return -ENOENT;
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
		if (dev->vendor == vendor && dev->device == device) {
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
	int tmp;

	if (!config || !logdev)
		return -EINVAL;
	memset(config, 0, sizeof(struct isapnp_config));
	config->logdev = logdev;
	config->irq[0] = config->irq[1] = ISAPNP_AUTO_IRQ;
	config->dma[0] = config->dma[1] = ISAPNP_AUTO_DMA;
	for (tmp = 0; tmp < 16; tmp++)
		config->irq_disable[tmp] = ISAPNP_AUTO_IRQ;
	for (tmp = 0; tmp < 8; tmp++)
		config->dma_disable[tmp] = ISAPNP_AUTO_DMA;
	for (res = logdev->res; res; res = res->next) {
		port_count = irq_count = dma_count = mem_count = 0;
		for (resa = res; resa; resa = resa->alt) {
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

struct isapnp_cfgtmp {
	struct isapnp_port *port[8];
	struct isapnp_irq *irq[2];
	struct isapnp_dma *dma[2];
	struct isapnp_mem *mem[4];
	struct isapnp_config *request;
	struct isapnp_config result;
};

static int isapnp_alternative_switch(struct isapnp_cfgtmp *cfg,
				     struct isapnp_resources *from,
				     struct isapnp_resources *to)
{
	int tmp;
	struct isapnp_port *port;
	struct isapnp_irq *irq;
	struct isapnp_dma *dma;
	struct isapnp_mem *mem;

	if (!cfg)
		return -EINVAL;
	/* process port settings */
	for (tmp = 0; tmp < cfg->result.port_count; tmp++) {
		if (cfg->request->port[tmp] != ISAPNP_AUTO_PORT)
			continue;		/* don't touch */
		port = cfg->port[tmp];
		if (!port) {
			cfg->port[tmp] = port = isapnp_find_port(cfg->result.logdev, tmp);
			if (!port)
				return -EINVAL;
		}
		if (from && port->res == from) {
			while (port->res != to) {
				if (!port->res->alt)
					return -EINVAL;
				cfg->port[tmp] = port = port->res->alt->port;
				cfg->result.port[tmp] = 0;	/* auto */
				if (!port)
					return -ENOENT;
			}
		}
	}
	/* process irq settings */
	for (tmp = 0; tmp < cfg->result.irq_count; tmp++) {
		if (cfg->request->irq[tmp] != ISAPNP_AUTO_IRQ)
			continue;		/* don't touch */
		irq = cfg->irq[tmp];
		if (!irq) {
			cfg->irq[tmp] = irq = isapnp_find_irq(cfg->result.logdev, tmp);
			if (!irq)
				return -EINVAL;
		}
		if (from && irq->res == from) {
			while (irq->res != to) {
				if (!irq->res->alt)
					return -EINVAL;
				cfg->irq[tmp] = irq = irq->res->alt->irq;
				cfg->result.irq[tmp] = 255;	/* auto */
				if (!irq)
					return -ENOENT;
			}
		}
	}
	/* process dma settings */
	for (tmp = 0; tmp < cfg->result.dma_count; tmp++) {
		if (cfg->request->dma[tmp] != ISAPNP_AUTO_DMA)
			continue;		/* don't touch */
		dma = cfg->dma[tmp];
		if (!dma) {
			cfg->dma[tmp] = dma = isapnp_find_dma(cfg->result.logdev, tmp);
			if (!dma)
				return -EINVAL;
		}
		if (from && dma->res == from) {
			while (dma->res != to) {
				if (!dma->res->alt)
					return -EINVAL;
				cfg->dma[tmp] = dma = dma->res->alt->dma;
				cfg->result.dma[tmp] = 255;	/* auto */
				if (!dma)
					return -ENOENT;
			}
		}
	}
	/* process memory settings */
	for (tmp = 0; tmp < cfg->result.mem_count; tmp++) {
		if (cfg->request->mem[tmp] != ISAPNP_AUTO_MEM)
			continue;		/* don't touch */
		mem = cfg->mem[tmp];
		if (!mem) {
			cfg->mem[tmp] = mem = isapnp_find_mem(cfg->result.logdev, tmp);
			if (!mem)
				return -EINVAL;
		}
		if (from && mem->res == from) {
			while (mem->res != to) {
				if (!mem->res->alt)
					return -EINVAL;
				cfg->mem[tmp] = mem = mem->res->alt->mem;
				cfg->result.mem[tmp] = 0;	/* auto */
				if (!mem)
					return -ENOENT;
			}
		}
	}
	return 0;
}

static int isapnp_check_port(struct isapnp_cfgtmp *cfg, int port, int size, int idx)
{
	int i, tmp, tsize;
	struct isapnp_port *xport;

	if (check_region(port, size))
		return 1;
	for (i = 0; i < 8; i++) {
		tmp = cfg->result.port_disable[i];
		if (tmp == ISAPNP_AUTO_PORT)
			continue;
		tsize = cfg->result.port_disable_size[i];
		if (tmp + tsize >= port && tmp <= port + tsize)
			return 1;
	}
	for (i = 0; i < cfg->result.port_count; i++) {
		if (i == idx)
			continue;
		tmp = cfg->request->port[i];
		if (!tmp) {		/* auto */
			xport = cfg->port[i];
			if (!xport)
				return 1;
			tmp = cfg->result.port[i];
			if (tmp == ISAPNP_AUTO_PORT)
				continue;
			if (tmp + xport->size >= port && tmp <= port + xport->size)
				return 1;
			continue;
		}
		if (port == tmp)
			return 1;
		xport = isapnp_find_port(cfg->result.logdev, i);
		if (!xport)
			return 1;
		if (tmp + xport->size >= port && tmp <= port + xport->size)
			return 1;
	}
	return 0;
}

static int isapnp_valid_port(struct isapnp_cfgtmp *cfg, int idx)
{
	int err;
	unsigned short *value;
	struct isapnp_port *port;

	if (!cfg || idx < 0 || idx > 7)
		return -EINVAL;
	if (cfg->result.port[idx])	/* don't touch */
		return 0;
      __again:
      	port = cfg->port[idx];
      	if (!port)
      		return -EINVAL;
      	value = &cfg->result.port[idx];
	if (*value == ISAPNP_AUTO_PORT) {
		if (!isapnp_check_port(cfg, *value = port->min, port->size, idx))
			return 0;
	}
	do {
		*value += port->align;
		if (*value > port->max || !port->align) {
			if (port->res && port->res->alt) {
				if ((err = isapnp_alternative_switch(cfg, port->res, port->res->alt))<0)
					return err;
				goto __again;
			}
			return -ENOENT;
		}
	} while (isapnp_check_port(cfg, *value, port->size, idx));
	return 0;
}

static void isapnp_test_handler(int irq, void *dev_id, struct pt_regs *regs)
{
}

static int isapnp_check_interrupt(struct isapnp_cfgtmp *cfg, int irq, int idx)
{
	int i;

	if (irq < 0 || irq > 15)
		return 1;
	for (i = 0; i < 16; i++)
		if (cfg->result.irq_disable[i] == irq)
			return 1;
	if (request_irq(irq, isapnp_test_handler, SA_INTERRUPT, "isapnp", NULL))
		return 1;
	free_irq(irq, NULL);
	for (i = 0; i < cfg->result.irq_count; i++) {
		if (i == idx)
			continue;
		if (cfg->result.irq[i] == ISAPNP_AUTO_IRQ)
			continue;
		if (cfg->result.irq[i] == irq)
			return 1;
	}
	return 0;
}

static int isapnp_valid_irq(struct isapnp_cfgtmp *cfg, int idx)
{
	/* IRQ priority: table is good for i386 */
	static unsigned short xtab[16] = {
		5, 10, 11, 12, 9, 14, 15, 7, 3, 4, 13, 0, 1, 6, 8, 2
	};
	int err, i;
	unsigned char *value;
	struct isapnp_irq *irq;

	if (!cfg || idx < 0 || idx > 1)
		return -EINVAL;
	if (cfg->result.irq[idx] != ISAPNP_AUTO_IRQ)	/* don't touch */
		return 0;
      __again:
      	irq = cfg->irq[idx];
      	if (!irq)
      		return -EINVAL;
      	value = &cfg->result.irq[idx];
	if (*value == ISAPNP_AUTO_IRQ) {
		for (i = 0; i < 16 && !(irq->map & (1<<xtab[i])); i++);
		if (i >= 16)
			return -ENOENT;
		if (!isapnp_check_interrupt(cfg, *value = xtab[i], idx))
			return 0;
	}
	do {
		for (i = 0; i < 16 && xtab[i] != *value; i++);
		for (i++; i < 16 && !(irq->map & (1<<xtab[i])); i++);
		if (i >= 16) {
			if (irq->res && irq->res->alt) {
				if ((err = isapnp_alternative_switch(cfg, irq->res, irq->res->alt))<0)
					return err;
				goto __again;
			}
			return -ENOENT;
		} else {
			*value = xtab[i];
		}
	} while (isapnp_check_interrupt(cfg, *value, idx));
	return 0;
}

static int isapnp_check_dma(struct isapnp_cfgtmp *cfg, int dma, int idx)
{
	int i;

	if (dma < 0 || dma == 4 || dma > 7)
		return 1;
	for (i = 0; i < 8; i++)
		if (cfg->result.dma_disable[i] == dma)
			return 1;
	if (request_dma(dma, "isapnp"))
		return 1;
	free_dma(dma);
	for (i = 0; i < cfg->result.dma_count; i++) {
		if (i == idx)
			continue;
		if (cfg->result.dma[i] == ISAPNP_AUTO_DMA)
			continue;
		if (cfg->result.dma[i] == dma)
			return 1;
	}
	return 0;
}

static int isapnp_valid_dma(struct isapnp_cfgtmp *cfg, int idx)
{
	int err, i;
	unsigned char *value;
	struct isapnp_dma *dma;

	if (!cfg || idx < 0 || idx > 1)
		return -EINVAL;
	if (cfg->result.dma[idx] != ISAPNP_AUTO_DMA)	/* don't touch */
		return 0;
      __again:
      	dma = cfg->dma[idx];
      	if (!dma)
      		return -EINVAL;
      	value = &cfg->result.dma[idx];
	if (*value == ISAPNP_AUTO_DMA) {
		for (i = 0; i < 8 && !(dma->map & (1<<i)); i++);
		if (i >= 8)
			return -ENOENT;
		if (!isapnp_check_dma(cfg, *value = i, idx))
			return 0;
	}
	do {
		for (i = *value + 1; i < 8 && !(dma->map & (1<<i)); i++);
		if (i >= 8) {
			if (dma->res && dma->res->alt) {
				if ((err = isapnp_alternative_switch(cfg, dma->res, dma->res->alt))<0)
					return err;
				goto __again;
			}
			return -ENOENT;
		} else {
			*value = i;
		}
	} while (isapnp_check_dma(cfg, *value, idx));
	return 0;
}

static int isapnp_check_mem(unsigned int addr, unsigned int size)
{
	return 0;	/* always valid - maybe wrong */
}

static int isapnp_valid_mem(struct isapnp_cfgtmp *cfg, int idx)
{
	int err;
	unsigned int *value;
	struct isapnp_mem *mem;

	if (!cfg || idx < 0 || idx > 3)
		return -EINVAL;
	if (cfg->result.mem[idx])	/* don't touch */
		return 0;
      __again:
      	mem = cfg->mem[idx];
      	if (!mem)
      		return -EINVAL;
      	value = &cfg->result.mem[idx];
	if (*value == ISAPNP_AUTO_MEM) {
		*value = mem->min;
		if (!isapnp_check_mem(*value, mem->size))
			return 0;
	}
	do {
		*value += mem->align;
		if (*value >= 8 || !mem->align) {
			if (mem->res && mem->res->alt) {
				if ((err = isapnp_alternative_switch(cfg, mem->res, mem->res->alt))<0)
					return err;
				goto __again;
			}
			return -ENOENT;
		}
	} while (isapnp_check_mem(*value, mem->size));
	return 0;
}

static int isapnp_check_valid(struct isapnp_cfgtmp *cfg)
{
	int tmp;
	
	for (tmp = 0; tmp < cfg->result.port_count; tmp++)
		if (cfg->result.port[tmp] == ISAPNP_AUTO_PORT)
			return -EAGAIN;
	for (tmp = 0; tmp < cfg->result.irq_count; tmp++)
		if (cfg->result.irq[tmp] == ISAPNP_AUTO_IRQ)
			return -EAGAIN;
	for (tmp = 0; tmp < cfg->result.dma_count; tmp++)
		if (cfg->result.dma[tmp] == ISAPNP_AUTO_DMA)
			return -EAGAIN;
	for (tmp = 0; tmp < cfg->result.mem_count; tmp++)
		if (cfg->result.mem[tmp] == ISAPNP_AUTO_MEM)
			return -EAGAIN;
	return 0;
}

int isapnp_configure(struct isapnp_config *config)
{
	struct isapnp_cfgtmp cfg;
	int tmp, fauto, err;
	
	if (!config || !config->logdev)
		return -EINVAL;
	memset(&cfg, 0, sizeof(cfg));
	cfg.request = config;
	memcpy(&cfg.result, config, sizeof(struct isapnp_config));
	/* check if all values are set, otherwise try auto-configuration */
	for (tmp = fauto = 0; !fauto && tmp < cfg.result.port_count; tmp++) {
		if (config->port[tmp] == ISAPNP_AUTO_PORT)
			fauto++;
	}
	for (tmp = 0; !fauto && tmp < cfg.result.irq_count; tmp++) {
		if (config->irq[tmp] == ISAPNP_AUTO_IRQ)
			fauto++;
	}
	for (tmp = 0; !fauto && tmp < cfg.result.dma_count; tmp++) {
		if (config->dma[tmp] == ISAPNP_AUTO_DMA)
			fauto++;
	}
	for (tmp = 0; !fauto && tmp < cfg.result.mem_count; tmp++) {
		if (config->mem[tmp] == ISAPNP_AUTO_MEM)
			fauto++;
	}
	if (!fauto)
		goto __skip_auto;
	/* set variables to initial values */
	if ((err = isapnp_alternative_switch(&cfg, NULL, NULL))<0)
		return err;
	/* find first valid configuration */
	fauto = 0;
	do {
		for (tmp = 0; tmp < cfg.result.port_count; tmp++)
			if ((err = isapnp_valid_port(&cfg, tmp))<0)
				return err;
		for (tmp = 0; tmp < cfg.result.irq_count; tmp++)
			if ((err = isapnp_valid_irq(&cfg, tmp))<0)
				return err;
		for (tmp = 0; tmp < cfg.result.dma_count; tmp++)
			if ((err = isapnp_valid_dma(&cfg, tmp))<0)
				return err;
		for (tmp = 0; tmp < cfg.result.mem_count; tmp++)
			if ((err = isapnp_valid_mem(&cfg, tmp))<0)
				return err;
	} while (isapnp_check_valid(&cfg)<0 && fauto++ < 20);
	if (fauto >= 20)
		return -EAGAIN;
      __skip_auto:
      	/* we have valid configuration, try configure hardware */
      	isapnp_logdev(cfg.result.logdev->number);
	for (tmp = 0; tmp < cfg.result.port_count; tmp++)
		isapnp_cfg_set_word(ISAPNP_CFG_PORT+(tmp<<1), cfg.result.port[tmp]);
	for (tmp = 0; tmp < cfg.result.irq_count; tmp++) {
		if (cfg.result.irq[tmp] == 2)
			cfg.result.irq[tmp] = 9;
		isapnp_cfg_set_byte(ISAPNP_CFG_IRQ+(tmp<<1), cfg.result.irq[tmp]);
	}
	for (tmp = 0; tmp < cfg.result.dma_count; tmp++)
		isapnp_cfg_set_byte(ISAPNP_CFG_DMA+tmp, cfg.result.dma[tmp]);
	for (tmp = 0; tmp < cfg.result.mem_count; tmp++)
		isapnp_cfg_set_word(ISAPNP_CFG_MEM+(tmp<<2), (cfg.result.mem[tmp] >> 8) & 0xffff);
      	memcpy(config, &cfg.result, sizeof(struct isapnp_config));
	return 0;
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
		isapnp_free_irq(resources->irq);
		isapnp_free_dma(resources->dma);
		isapnp_free_mem(resources->mem);
		isapnp_free_mem32(resources->mem32);
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

#ifdef ISAPNP_REGION_OK
	release_region(_PIDXR, 1);
#endif
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

#ifndef LINUX_2_1
static struct symbol_table isapnp_syms = {
	#include <linux/symtab_begin.h>
	X(isapnp_cfg_begin),
	X(isapnp_cfg_end),
	X(isapnp_cfg_get_byte),
	X(isapnp_cfg_get_word),
	X(isapnp_cfg_get_dword),
	X(isapnp_cfg_set_byte),
	X(isapnp_cfg_set_word),
	X(isapnp_cfg_set_dword),
	X(isapnp_wake),
	X(isapnp_logdev),
	X(isapnp_activate),
	X(isapnp_deactivate),
	X(isapnp_find_port),
	X(isapnp_find_irq),
	X(isapnp_find_dma),
	X(isapnp_find_mem),
	X(isapnp_find_mem32),
	X(isapnp_verify_port),
	X(isapnp_verify_irq),
	X(isapnp_verify_dma),
	X(isapnp_verify_mem),
	X(isapnp_verify_mem32),
	X(isapnp_find_device),
	X(isapnp_find_logdev),
	X(isapnp_config_init),
	X(isapnp_configure),
#include <linux/symtab_end.h>
};
#endif /* !LINUX_2_1 */

#ifdef LINUX_2_1
EXPORT_SYMBOL(isapnp_cfg_begin);
EXPORT_SYMBOL(isapnp_cfg_end);
EXPORT_SYMBOL(isapnp_cfg_get_byte);
EXPORT_SYMBOL(isapnp_cfg_get_word);
EXPORT_SYMBOL(isapnp_cfg_get_dword);
EXPORT_SYMBOL(isapnp_cfg_set_byte);
EXPORT_SYMBOL(isapnp_cfg_set_word);
EXPORT_SYMBOL(isapnp_cfg_set_dword);
EXPORT_SYMBOL(isapnp_wake);
EXPORT_SYMBOL(isapnp_logdev);
EXPORT_SYMBOL(isapnp_activate);
EXPORT_SYMBOL(isapnp_deactivate);
EXPORT_SYMBOL(isapnp_find_port);
EXPORT_SYMBOL(isapnp_find_irq);
EXPORT_SYMBOL(isapnp_find_dma);
EXPORT_SYMBOL(isapnp_find_mem);
EXPORT_SYMBOL(isapnp_find_mem32);
EXPORT_SYMBOL(isapnp_verify_port);
EXPORT_SYMBOL(isapnp_verify_irq);
EXPORT_SYMBOL(isapnp_verify_dma);
EXPORT_SYMBOL(isapnp_verify_mem);
EXPORT_SYMBOL(isapnp_verify_mem32);
EXPORT_SYMBOL(isapnp_find_device);
EXPORT_SYMBOL(isapnp_find_logdev);
EXPORT_SYMBOL(isapnp_config_init);
EXPORT_SYMBOL(isapnp_configure);
#endif /* LINUX_2_1 */

__initfunc(int isapnp_init(void))
{
	int devices;
	struct isapnp_dev *dev;
	struct isapnp_logdev *logdev;

#ifdef ISAPNP_REGION_OK
	if (check_region(_PIDXR, 1)) {
		printk("isapnp: Index Register 0x%x already used\n", _PIDXR);
		return -EBUSY;
	}
#endif
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
#ifdef ISAPNP_REGION_OK
	request_region(_PIDXR, 1, "ISA PnP index");
#endif
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
			if (isapnp_verbose < 2)
				continue;
			for (logdev = dev->logdev; logdev; logdev = logdev->next)
				printk("isapnp:   Logical device '%s'\n", logdev->identifier?logdev->identifier:"Unknown");
		}
	}
	printk("isapnp: %i Plug & Play device%s detected total\n", devices, devices>1?"s":"");
	isapnp_proc_init();
#ifndef LINUX_2_1
	if (register_symtab(&isapnp_syms))
		printk("isapnp: cannot register symtab!!!\n");
#endif
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
