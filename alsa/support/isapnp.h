/*
 *  ISA Plug & Play support
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>
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

#ifndef LINUX_ISAPNP_H
#define LINUX_ISAPNP_H

/*
 *  Configuration registers (TODO: change by specification)
 */ 

#define ISAPNP_CFG_ACTIVATE		0x30	/* byte */
#define ISAPNP_CFG_MEM			0x40	/* 4 * dword */
#define ISAPNP_CFG_PORT			0x60	/* 8 * word */
#define ISAPNP_CFG_IRQ			0x70	/* 2 * word */
#define ISAPNP_CFG_DMA			0x74	/* 2 * byte */

/*
 *
 */

#define ISAPNP_VENDOR(a,b,c)	(((((a)-'A'+1)&0x3f)<<2)|\
				((((b)-'A'+1)&0x18)>>3)|((((b)-'A'+1)&7)<<13)|\
				((((c)-'A'+1)&0x1f)<<8))
#define ISAPNP_DEVICE(x)	((((x)&0xf000)>>8)|\
				 (((x)&0x0f00)>>8)|\
				 (((x)&0x00f0)<<8)|\
				 (((x)&0x000f)<<8))
#define ISAPNP_FUNCTION(x)	ISAPNP_DEVICE(x)

/*
 *
 */

#ifdef __KERNEL__

#define ISAPNP_PORT_FLAG_16BITADDR	(1<<0)
#define ISAPNP_PORT_FLAG_FIXED		(1<<1)

struct isapnp_port {
	unsigned short min;		/* min base number */
	unsigned short max;		/* max base number */
	unsigned char align;		/* align boundary */
	unsigned char size;		/* size of range */
	unsigned short flags;		/* port flags */
	struct isapnp_resources *res;	/* parent */
	struct isapnp_port *next;	/* next port */
};

struct isapnp_irq {
	unsigned short map;		/* bitmaks for IRQ lines */
	unsigned short flags;		/* IRQ flags */
	struct isapnp_resources *res;	/* parent */
	struct isapnp_irq *next;	/* next IRQ */
};

struct isapnp_dma {
	unsigned char map;		/* bitmask for DMA channels */
	unsigned char type;		/* DMA type */
	unsigned char flags;		/* DMA flags */
	unsigned char speed;		/* DMA speed */
	struct isapnp_resources *res;	/* parent */
	struct isapnp_dma *next;	/* next port */
};

struct isapnp_mem {
	unsigned int min;		/* min base number */
	unsigned int max;		/* max base number */
	unsigned int align;		/* align boundary */
	unsigned int size;		/* size of range */
	unsigned short flags;		/* memory flags */
	unsigned short type;		/* memory type */
	struct isapnp_resources *res;	/* parent */
	struct isapnp_mem *next;	/* next memory resource */
};

struct isapnp_mem32 {
	/* TODO */
	unsigned char data[17];
	struct isapnp_resources *res;	/* parent */
	struct isapnp_mem32 *next;	/* next 32-bit memory resource */
};

#define ISAPNP_RES_PRIORITY_PREFERRED	0
#define ISAPNP_RES_PRIORITY_ACCEPTABLE	1
#define ISAPNP_RES_PRIORITY_FUNCTIONAL	2
#define ISAPNP_RES_PRIORITY_INVALID	65535

struct isapnp_resources {
	unsigned short priority;	/* priority */
	unsigned short dependent;	/* dependent resources */
	struct isapnp_port *port;	/* first port */
	struct isapnp_irq *irq;		/* first IRQ */
	struct isapnp_dma *dma;		/* first DMA */
	struct isapnp_mem *mem;		/* first memory resource */
	struct isapnp_mem32 *mem32;	/* first 32-bit memory */
	struct isapnp_dev *dev;		/* parent */
	struct isapnp_resources *alt;	/* alternative resource (aka dependent resources) */
	struct isapnp_resources *next;	/* next resource */
};

#ifndef LINUX_VERSION_CODE
#include <linux/version.h>
#endif

#ifndef LinuxVersionCode
#define LinuxVersionCode(v, p, s) (((v)<<16)|((p)<<8)|(s))
#endif

#include <linux/pci.h>

#if LinuxVersionCode(2, 3, 13) > LINUX_VERSION_CODE

struct resource {
	const char *name;
	unsigned long start, end;
	unsigned long flags;
	unsigned char bits;		/* decoded bits */
	unsigned char fixed;		/* fixed range */
	unsigned short hw_flags;	/* hardware flags */
	unsigned short type;		/* region type */
	struct resource *parent, *sibling, *child;
};

#define DEVICE_COUNT_COMPATIBLE	4
#define DEVICE_COUNT_DMA	2
#define DEVICE_COUNT_RESOURCE	12

#define DEVICE_IRQ_NOTSET	0xffffffff
#define DEVICE_IRQ_AUTO		0xfffffffe
#define DEVICE_DMA_NOTSET	0xff
#define DEVICE_DMA_AUTO		0xfe
#define DEVICE_IO_NOTSET	(~0)
#define DEVICE_IO_AUTO		((~0)-1)

#define DEVICE_IRQ_FLAG_HIGHEDGE	(1<<0)
#define DEVICE_IRQ_FLAG_LOWEDGE		(1<<1)
#define DEVICE_IRQ_FLAG_HIGHLEVEL	(1<<2)
#define DEVICE_IRQ_FLAG_LOWLEVEL	(1<<3)

#define DEVICE_DMA_TYPE_8BIT		0
#define DEVICE_DMA_TYPE_8AND16BIT	1
#define DEVICE_DMA_TYPE_16BIT		2

#define DEVICE_DMA_FLAG_MASTER		(1<<0)
#define DEVICE_DMA_FLAG_BYTE		(1<<1)
#define DEVICE_DMA_FLAG_WORD		(1<<2)

#define DEVICE_DMA_SPEED_COMPATIBLE	0
#define DEVICE_DMA_SPEED_TYPEA		1
#define DEVICE_DMA_SPEED_TYPEB		2
#define DEVICE_DMA_SPEED_TYPEF		3

#define DEVICE_IO_FLAG_WRITEABLE	(1<<0)
#define DEVICE_IO_FLAG_CACHEABLE	(1<<1)
#define DEVICE_IO_FLAG_RANGELENGTH	(1<<2)
#define DEVICE_IO_FLAG_SHADOWABLE	(1<<4)
#define DEVICE_IO_FLAG_EXPANSIONROM	(1<<5)

#define DEVICE_IO_TYPE_8BIT		0
#define DEVICE_IO_TYPE_16BIT		1
#define DEVICE_IO_TYPE_8AND16BIT	2

/*
 * There is one pci_dev structure for each slot-number/function-number
 * combination:
 */
struct isapnp_dev {
	int active;			/* device is active */
	int ro;				/* Read/Only */

	struct isapnp_card *bus;	/* bus this device is on */
	struct isapnp_dev *sibling;	/* next device on this bus */
	struct isapnp_dev *next;	/* chain of all devices */

	void		*sysdata;	/* hook for sys-specific extension */
	struct proc_dir_entry *procent;	/* device entry in /proc/bus/pci */

	unsigned int	devfn;		/* encoded device & function index */
	unsigned short	vendor;
	unsigned short	device;
	unsigned int	class;		/* 3 bytes: (base,sub,prog-if) */
	unsigned int	hdr_type;	/* PCI header type */
	unsigned int	master : 1;	/* set if device is master capable */

	unsigned short regs;		/* supported reserved registers */

	/* device is compatible with these IDs */
	unsigned short vendor_compatible[DEVICE_COUNT_COMPATIBLE];
	unsigned short device_compatible[DEVICE_COUNT_COMPATIBLE];

	char		name[48];

	/*
	 * In theory, the irq level can be read from configuration
	 * space and all would be fine.  However, old PCI chips don't
	 * support these registers and return 0 instead.  For example,
	 * the Vision864-P rev 0 chip can uses INTA, but returns 0 in
	 * the interrupt line and pin registers.  pci_init()
	 * initializes this field with the value at PCI_INTERRUPT_LINE
	 * and it is the job of pcibios_fixup() to change it if
	 * necessary.  The field must not be 0 unless the device
	 * cannot generate interrupts at all.
	 */
	unsigned int	irq;		/* irq generated by this device */
	unsigned short	irq_flags;	/* irq type */
	unsigned int	irq2;
	unsigned short	irq2_flags;	
	unsigned char	dma[DEVICE_COUNT_DMA];
	unsigned char	dma_type[DEVICE_COUNT_DMA];
	unsigned char	dma_flags[DEVICE_COUNT_DMA];
	unsigned char	dma_speed[DEVICE_COUNT_DMA];

	/* Base registers for this device, can be adjusted by
	 * pcibios_fixup() as necessary.
	 */
	struct resource resource[DEVICE_COUNT_RESOURCE];
	unsigned long	rom_address;

	int (*prepare)(struct isapnp_dev *dev);
	int (*activate)(struct isapnp_dev *dev);
	int (*deactivate)(struct isapnp_dev *dev);
};

struct isapnp_card {
	struct isapnp_card *parent;	/* parent bus this bridge is on */
	struct isapnp_card *children;	/* chain of P2P bridges on this bus */
	struct isapnp_card *next;	/* chain of all PCI buses */

	struct isapnp_dev *self;	/* bridge device as seen by parent */
	struct isapnp_dev *devices;	/* devices behind this bridge */

	void		*sysdata;	/* hook for sys-specific extension */
	struct proc_dir_entry *procdir;	/* directory entry in /proc/bus/pci */

	unsigned char	number;		/* bus number */
	unsigned char	primary;	/* number of primary bridge */
	unsigned char	secondary;	/* number of secondary bridge */
	unsigned char	subordinate;	/* max number of subordinate buses */

	char name[48];
	unsigned short vendor;
	unsigned short device;
	unsigned int serial;		/* serial number */
	unsigned char pnpver;		/* Plug & Play version */
	unsigned char productver;	/* product version */
	unsigned char checksum;		/* if zero - checksum passed */
	unsigned char pad1;
};

#else

#define isapnp_dev pci_dev
#define isapnp_card pci_bus

#endif

#ifdef CONFIG_ISAPNP

/* lowlevel configuration */
int isapnp_present(void);
int isapnp_cfg_begin(int csn, int device);
int isapnp_cfg_end(void);
unsigned char isapnp_read_byte(unsigned char idx);
unsigned short isapnp_read_word(unsigned char idx);
unsigned int isapnp_read_dword(unsigned char idx);
void isapnp_write_byte(unsigned char idx, unsigned char val);
void isapnp_write_word(unsigned char idx, unsigned short val);
void isapnp_write_dword(unsigned char idx, unsigned int val);
void isapnp_wake(unsigned char csn);
void isapnp_device(unsigned char device);
void isapnp_activate(unsigned char device);
void isapnp_deactivate(unsigned char device);
/* manager */
struct isapnp_card *isapnp_find_card(unsigned short vendor,
				     unsigned short device,
				     struct isapnp_card *from);
struct isapnp_dev *isapnp_find_dev(struct isapnp_card *card,
				   unsigned short vendor,
				   unsigned short function,
				   struct isapnp_dev *from);
/* init/main.c */
int isapnp_init(void);

#else /* !CONFIG_ISAPNP */

/* lowlevel configuration */
extern inline int isapnp_present(void) { return 0; }
extern inline int isapnp_cfg_begin(int csn, int device) { return -ENODEV; }
extern inline int isapnp_cfg_end(void) { return -ENODEV; }
extern inline unsigned char isapnp_read_byte(unsigned char idx) { return 0xff; }
extern inline unsigned short isapnp_read_word(unsigned char idx) { return 0xffff; }
extern inline unsigned int isapnp_read_dword(unsigned char idx) { return 0xffffffff; }
extern inline void isapnp_write_byte(unsigned char idx, unsigned char val) { ; }
extern inline void isapnp_write_word(unsigned char idx, unsigned short val) { ; }
extern inline void isapnp_write_dword(unsigned char idx, unsigned int val) { ; }
extern void isapnp_wake(unsigned char csn) { ; }
extern void isapnp_device(unsigned char device) { ; }
extern void isapnp_activate(unsigned char device) { ; }
extern void isapnp_deactivate(unsigned char device) { ; }
/* manager */
extern struct isapnp_card *isapnp_find_card(unsigned short vendor,
				            unsigned short device,
				            struct isapnp_card *from) { return NULL; }
extern struct isapnp_dev *isapnp_find_dev(struct isapnp_card *card,
					  unsigned short vendor,
					  unsigned short function,
					  struct isapnp_dev *from) { return NULL; }

#endif /* CONFIG_ISAPNP */

#endif /* __KERNEL__ */
#endif /* LINUX_ISAPNP_H */
