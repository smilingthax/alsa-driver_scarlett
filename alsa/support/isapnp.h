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

#define ISAPNP_IRQ_FLAG_HIGHEDGE	(1<<0)
#define ISAPNP_IRQ_FLAG_LOWEDGE		(1<<1)
#define ISAPNP_IRQ_FLAG_HIGHLEVEL	(1<<2)
#define ISAPNP_IRQ_FLAG_LOWLEVEL	(1<<3)

struct isapnp_irq {
	unsigned short map;		/* bitmaks for IRQ lines */
	unsigned short flags;		/* IRQ flags */
	struct isapnp_resources *res;	/* parent */
	struct isapnp_irq *next;	/* next IRQ */
};

#define ISAPNP_DMA_TYPE_8BIT		0
#define ISAPNP_DMA_TYPE_8AND16BIT	1
#define ISAPNP_DMA_TYPE_16BIT		2

#define ISAPNP_DMA_FLAG_MASTER		(1<<0)
#define ISAPNP_DMA_FLAG_BYTE		(1<<1)
#define ISAPNP_DMA_FLAG_WORD		(1<<2)

#define ISAPNP_DMA_SPEED_COMPATIBLE	0
#define ISAPNP_DMA_SPEED_TYPEA		1
#define ISAPNP_DMA_SPEED_TYPEB		2
#define ISAPNP_DMA_SPEED_TYPEF		3

struct isapnp_dma {
	unsigned char map;		/* bitmask for DMA channels */
	unsigned char type;		/* DMA type */
	unsigned char flags;		/* DMA flags */
	unsigned char speed;		/* DMA speed */
	struct isapnp_resources *res;	/* parent */
	struct isapnp_dma *next;	/* next port */
};

#define ISAPNP_MEM_FLAG_WRITEABLE	(1<<0)
#define ISAPNP_MEM_FLAG_CACHEABLE	(1<<1)
#define ISAPNP_MEM_FLAG_RANGELENGTH	(1<<2)
#define ISAPNP_MEM_FLAG_SHADOWABLE	(1<<4)
#define ISAPNP_MEM_FLAG_EXPANSIONROM	(1<<5)

#define ISAPNP_MEM_TYPE_8BIT		0
#define ISAPNP_MEM_TYPE_16BIT		1
#define ISAPNP_MEM_TYPE_8AND16BIT	2

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

#define ISAPNP_RES_PRIORITY_PREFFERED	0
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
	struct isapnp_logdev *logdev;	/* parent */
	struct isapnp_resources *alt;	/* alternative resource (aka dependent resources) */
	struct isapnp_resources *next;	/* next resource */
};

struct isapnp_compatid {
	unsigned short vendor;		/* vendor ID */
	unsigned short function;	/* function ID */
	struct isapnp_compatid *next;	/* next compatible device */
};

struct isapnp_logdev {
	unsigned short number;		/* logical device number */
	unsigned short vendor;		/* vendor ID */
	unsigned short function;	/* function ID */
	unsigned short regs;		/* supported reserved registers */
	char *identifier;		/* ANSI identifier string */
	struct isapnp_compatid *compat;	/* compatible devices */
	struct isapnp_resources *res;	/* resources */
	struct isapnp_dev *dev;		/* parent device */
	struct isapnp_logdev *next;	/* next logical device */
};

struct isapnp_dev {
	unsigned int csn;		/* Card Select Number */
	unsigned short vendor;		/* vendor ID */
	unsigned short device;		/* device ID */
	unsigned int serial;		/* serial number */
	unsigned char pnpver;		/* Plug & Play version */
	unsigned char productver;	/* product version */
	char *identifier;		/* ANSI identifier string */
	struct isapnp_logdev *logdev;	/* first logical device */
	unsigned char checksum;		/* if zero - checksum passed */
	unsigned char pad1;		/* not used */
	struct isapnp_dev *next;	/* next device in list */
};

struct isapnp_config {
	struct isapnp_logdev *logdev;	/* device */	
	int port_count;
	unsigned short port[8];
	int irq_count;
	unsigned char irq[2];
	int dma_count;
	unsigned char dma[2];
	int mem_count;
	unsigned int mem[4];
};

/* lowlevel configuration */
int isapnp_cfg_begin(int csn, int logdev);
int isapnp_cfg_end(void);
unsigned char isapnp_cfg_get_byte(unsigned char idx);
unsigned short isapnp_cfg_get_word(unsigned char idx);
unsigned int isapnp_cfg_get_dword(unsigned char idx);
void isapnp_cfg_set_byte(unsigned char idx, unsigned char val);
void isapnp_cfg_set_word(unsigned char idx, unsigned short val);
void isapnp_cfg_set_dword(unsigned char idx, unsigned int val);
void isapnp_wake(unsigned char csn);
void isapnp_logdev(unsigned char dev);
void isapnp_activate(unsigned char value);
/* manager */
struct isapnp_port *isapnp_find_port(struct isapnp_logdev *logdev, int index);
struct isapnp_irq *isapnp_find_irq(struct isapnp_logdev *logdev, int index);
struct isapnp_dma *isapnp_find_dma(struct isapnp_logdev *logdev, int index);
struct isapnp_mem *isapnp_find_mem(struct isapnp_logdev *logdev, int index);
struct isapnp_mem32 *isapnp_find_mem32(struct isapnp_logdev *logdev, int index);
int isapnp_verify_port(struct isapnp_port *port, unsigned short base);
int isapnp_verify_irq(struct isapnp_irq *irq, unsigned char value);
int isapnp_verify_dma(struct isapnp_dma *dma, unsigned char value);
int isapnp_verify_mem(struct isapnp_mem *mem, unsigned int base);
int isapnp_verify_mem32(struct isapnp_mem32 *mem32, unsigned int base);
struct isapnp_dev *isapnp_find_device(unsigned short vendor,
				      unsigned short device,
				      int index);
struct isapnp_logdev *isapnp_find_logdev(struct isapnp_dev *dev,
					 unsigned short vendor,
					 unsigned short function,
					 int index);
int isapnp_config_init(struct isapnp_config *config, struct isapnp_logdev *logdev);
int isapnp_configure(struct isapnp_config *config);
