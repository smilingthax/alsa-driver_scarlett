#ifndef __DRIVER_H
#define __DRIVER_H

/*
 *  Main header file for the ALSA driver
 *  Copyright (c) 1994-98 by Jaroslav Kysela <perex@suse.cz>
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

#include "config.h"

#define SND_CARDS		8	/* number of supported soundcards - don't change - minor numbers */

#ifndef CONFIG_SND_MAJOR	/* standard configuration */
#define CONFIG_SND_MAJOR	116
#endif

#if !defined(CONFIG_SND_DEBUG)
#undef CONFIG_SND_DEBUG_MEMORY
#endif

/*
 *  ==========================================================================
 */

#define __KERNEL__
#define MODULE

#define LinuxVersionCode(v, p, s) (((v)<<16)|((p)<<8)|(s))

#include <linux/config.h>
#include <linux/version.h>

#if LinuxVersionCode(2, 2, 3) > LINUX_VERSION_CODE
#error "This driver requires Linux 2.2.3 and highter."
#endif
#if LinuxVersionCode(2, 2, 0) <= LINUX_VERSION_CODE
#define LINUX_2_2
#endif
#if LinuxVersionCode(2, 3, 1) <= LINUX_VERSION_CODE
#define LINUX_2_3
#endif

#if defined(CONFIG_MODVERSIONS) && !defined(__GENKSYMS__) && !defined(__DEPEND__)
#define MODVERSIONS
#include <linux/modversions.h>
#include "sndversions.h"
#endif
#ifndef SND_MAIN_OBJECT_FILE
#define __NO_VERSION__
#endif
#ifdef SND_NO_MODVERS
#undef MODVERSIONS
#undef _set_ver
#endif
#include <linux/module.h>

#include <linux/utsname.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/malloc.h>

#include <linux/ioport.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/dma.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/string.h>

#ifdef CONFIG_PCI
#include <linux/pci.h>
#endif
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>

#ifdef CONFIG_ISAPNP
#ifdef CONFIG_ISAPNP_KERNEL
#include <linux/isapnp.h>
#else
#include "isapnp.h"
#endif
#endif

#include "asound.h"
#include "asoundid.h"

static inline mm_segment_t snd_enter_user(void)
{
	mm_segment_t fs = get_fs();
	set_fs(get_ds());
	return fs;
}
static inline void snd_leave_user(mm_segment_t fs)
{
	set_fs(fs);
}

#ifndef LINUX_2_3
#define init_MUTEX(x) *(x) = MUTEX
#define DECLARE_MUTEX(x) struct semaphore x = MUTEX
typedef struct wait_queue * wait_queue_head_t;
#define init_waitqueue_head(x) *(x) = NULL
#endif

/*
 *  ==========================================================================
 */

/* auto values */

#define SND_AUTO_PORT		0x00ff
#define SND_AUTO_IRQ		16
#define SND_AUTO_DMA		8
#define SND_AUTO_DMA_SIZE	(0x7fffffff)

/* DMA */

#define SND_DMA_TYPE_ISA	0	/* ISA DMA */
#define SND_DMA_TYPE_PCI	1	/* PCI DMA (anywhere in kernel memory) */
#define SND_DMA_TYPE_PCI_16MB	2	/* PCI DMA (must be in low 16MB memory) */
#define SND_DMA_TYPE_HARDWARE	3	/* buffer mapped from device */

typedef struct snd_stru_dma {
	int type;			/* dma type - see SND_DMA_TYPE_XXXX */
	volatile unsigned short
	 mmaped:1,			/* mmaped area to user space */
	 mmap_free:1;			/* free mmaped buffer */
	int dma;			/* DMA number */
	char *name;			/* pointer to name */
	unsigned char *buf;		/* pointer to DMA buffer */
	long size;			/* real size of DMA buffer */
	long rsize;			/* requested size of DMA buffer */
	char *owner;			/* owner of this DMA channel */
	char *soft_owner;		/* owner of soft DMA channel */
	struct vm_area_struct *vma;	/* virtual memory area */
	struct semaphore lock;		/* mutex for locking */
	struct semaphore mutex;		/* snd_dma_malloc/free */
	struct snd_stru_dma *next;
} snd_dma_t;

#define SND_IRQ_TYPE_ISA	0	/* ISA IRQ */
#define SND_IRQ_TYPE_PCI	1	/* PCI IRQ (shareable) */
#define SND_IRQ_TYPE_EISA	SND_IRQ_TYPE_PCI

typedef struct snd_stru_irq {
	int type;		/* see to SND_IRQ_TYPE_XXXX */
	unsigned short irq;
	char *name;
	void *dev_id;
	struct snd_stru_irq *next;
} snd_irq_t;

typedef struct snd_stru_port {
	unsigned short port;
	unsigned short size;
	char *name;
	struct snd_stru_port *next;
} snd_port_t;

typedef void (snd_irq_handler_t) (int irq, void *dev_id, struct pt_regs * regs);

/* various typedefs */

typedef struct snd_stru_switch snd_kswitch_t;
typedef struct snd_stru_switch_list snd_kswitch_list_t;
typedef struct snd_stru_card snd_card_t;
typedef struct snd_info_entry snd_info_entry_t;
typedef struct snd_stru_pcm snd_pcm_t;
typedef struct snd_stru_mixer snd_kmixer_t;
typedef struct snd_stru_rawmidi snd_rawmidi_t;
typedef struct snd_stru_control snd_control_t;
typedef struct snd_stru_timer snd_timer_t;
typedef struct snd_stru_timer_instance snd_timer_instance_t;
typedef struct snd_stru_synth snd_synth_t;

/* main structure for soundcard */

#include "switch.h"

struct snd_stru_card {
	int number;				/* number of soundcard (index to snd_cards) */

	unsigned int type;			/* type (number ID) of soundcard */

	char id[16];				/* id string of this card */
	char abbreviation[16];			/* abbreviation of soundcard name */
	char shortname[32];			/* short name of this soundcard */
	char longname[80];			/* name of this soundcard */

	snd_dma_t *dmas;			/* pointer to first DMA */
	snd_irq_t *irqs;			/* pointer to first IRQ */
	snd_port_t *ports;			/* pointer to first I/O port */

	void (*use_inc) (snd_card_t * card);	/* increment use count */
	void (*use_dec) (snd_card_t * card);	/* decrement use count */

	struct semaphore control;		/* control card mutex */

	void *private_data;			/* private data for soundcard */
	void (*private_free) (void *private);	/* callback for freeing of private data */

	void *static_data;			/* private static data for soundcard */

	snd_kswitch_list_t switches;		/* switches */
	snd_control_t *fcontrol;		/* first control file */

	struct proc_dir_entry *proc_dir;	/* root for soundcard specific files */
	struct proc_dir_entry *proc_dir_link;	/* number link to real id */
	snd_info_entry_t *info_entries;		/* info entries */
};

/* device.c */

typedef int (snd_unregister_t) (unsigned short minor);

typedef long long (snd_lseek_t) (struct file * file, long long offset, int orig);
typedef long (snd_read_t) (struct file * file, char *buf, long count);
typedef long (snd_write_t) (struct file * file, const char *buf, long count);
typedef int (snd_open_t) (unsigned short minor, int cardnum, int device, struct file * file);
typedef int (snd_release_t) (unsigned short minor, int cardnum, int device, struct file * file);
typedef unsigned int (snd_poll_t) (struct file * file, poll_table * wait);
typedef int (snd_ioctl_t) (struct file * file, unsigned int cmd, unsigned long arg);
typedef int (snd_mmap_t) (struct inode * inode, struct file * file, struct vm_area_struct * vma);

struct snd_stru_minor {
	char *comment;			/* for /dev/sndinfo */
	snd_info_entry_t *dev;		/* for /proc/asound/dev */

	snd_unregister_t *unregister;

	snd_lseek_t *lseek;
	snd_read_t *read;
	snd_write_t *write;
	snd_open_t *open;
	snd_release_t *release;
	snd_poll_t *poll;
	snd_ioctl_t *ioctl;
	snd_mmap_t *mmap;
};

typedef struct snd_stru_minor snd_minor_t;

/* sound.c */

extern int snd_ecards_limit;
extern int snd_device_mode;
extern int snd_device_gid;
extern int snd_device_uid;

extern int snd_register_device(int type, snd_card_t * card, int dev, snd_minor_t * reg, const char *name);
extern int snd_unregister_device(int type, snd_card_t * card, int dev);

#ifdef CONFIG_SND_OSSEMUL
extern int snd_register_oss_device(int type, snd_card_t * card, int dev, snd_minor_t * reg, const char *name);
extern int snd_unregister_oss_device(int type, snd_card_t * card, int dev);
#endif

extern int snd_minor_info_init(void);
extern int snd_minor_info_done(void);

extern int snd_ioctl_in(long *addr);
extern int snd_ioctl_out(long *addr, int value);

/* sound_oss.c */

#ifdef CONFIG_SND_OSSEMUL

int snd_minor_info_oss_init(void);
int snd_minor_info_oss_done(void);

int snd_oss_init_module(void);
void snd_oss_cleanup_module(void);

#endif

/* memory.c */

extern void snd_malloc_init(void);
extern void snd_malloc_done(void);
extern void *snd_malloc(unsigned long size);
extern void *snd_calloc(unsigned long size);
extern void snd_free(void *obj, unsigned long size);
extern char *snd_malloc_strdup(char *string);
extern void snd_free_str(char *string);
extern char *snd_malloc_pages(unsigned long size, int *pg, int dma);
extern void snd_free_pages(char *ptr, unsigned long size);
extern int snd_dma_malloc(snd_card_t * card, snd_dma_t * dma, char *owner, int soft);
extern void snd_dma_free(snd_card_t * card, snd_dma_t * dma, int soft);
extern int snd_dma_soft_grab(snd_card_t * card, snd_dma_t * dma);
extern void snd_dma_soft_release(snd_card_t * card, snd_dma_t * dma);
extern void snd_dma_notify_vma_close(struct vm_area_struct *area);
extern int snd_memory_info_init(void);
extern int snd_memory_info_done(void);
#ifdef CONFIG_SND_DEBUG_MEMORY
extern void snd_memory_debug1(void);
#endif

/* init.c */

extern int snd_cards_count;
extern unsigned int snd_cards_bitmap;
extern snd_card_t *snd_cards[SND_CARDS];

extern void snd_driver_init(void);

extern snd_card_t *snd_card_new(int idx, char *id,
			void (*use_inc) (snd_card_t * card),
			void (*use_dec) (snd_card_t * card));
extern int snd_card_free(snd_card_t * card);
extern int snd_card_register(snd_card_t * card);
extern int snd_card_unregister(snd_card_t * card);
extern int snd_card_info_init(void);
extern int snd_card_info_done(void);

extern void snd_delay(int loops);

extern int snd_check_ioport(snd_card_t * card, int port, int size);
extern int snd_register_ioport(snd_card_t * card, int port, int size, char *name,
                               snd_port_t ** rport);
extern int snd_unregister_ioport(snd_card_t * card, snd_port_t * port);
extern int snd_unregister_ioports(snd_card_t * card);

extern int snd_register_dma_channel(snd_card_t * card, char *name, int number,
				    int type, int rsize, int *possible_numbers,
				    snd_dma_t ** rdma);
extern int snd_unregister_dma_channels(snd_card_t * card);

extern int snd_register_interrupt(snd_card_t * card, char *name, int number,
				  int type, snd_irq_handler_t * handler,
				  void *dev_id, int *possible_numbers,
				  snd_irq_t ** rirq);
extern int snd_unregister_interrupts(snd_card_t * card);

/* isadma.c */

#define DMA_MODE_NO_ENABLE	0x0100

extern void snd_dma_program(int dma, const void *buf, unsigned int size, unsigned short mode);
extern unsigned int snd_dma_residue(int dma);

/* misc.c */

extern int snd_task_name(struct task_struct *task, char *name, int size);

/* --- */

#define snd_printk( args... ) printk( "snd: " ##args )
#ifdef CONFIG_SND_DEBUG
#define snd_printd( args... ) snd_printk( ##args )
#else
#define snd_printd( args... )	/* nothing */
#endif

#ifdef CONFIG_SND_DEBUG_DETECT
#define snd_printdd( args... ) snd_printk( ##args )
#else
#define snd_printdd( args... )	/* nothing */
#endif

#define SND_OSS_VERSION         ((3<<16)|(8<<8)|(1<<4)|(0))	/* 3.8.1a */

#endif				/* __DRIVER_H */
