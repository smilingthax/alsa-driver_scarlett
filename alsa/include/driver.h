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

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#define NEW_MACRO_VARARGS
#endif

#ifdef ALSA_BUILD
#include "config.h"
#endif

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

#ifdef ALSA_BUILD
#define MODULE
#endif

#include <linux/config.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 2, 3)
#error "This driver requires Linux 2.2.3 and higher."
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 2, 0)
#define LINUX_2_2
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 3, 1)
#define LINUX_2_3
#endif
#if defined(LINUX_2_3) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 0)
#error "This code requires Linux 2.4.0-test1 and higher."
#endif

#ifdef ALSA_BUILD
#if defined(CONFIG_MODVERSIONS) && !defined(__GENKSYMS__) && !defined(__DEPEND__)
#define MODVERSIONS
#include <linux/modversions.h>
#include "sndversions.h"
#endif
#ifdef SND_NO_MODVERS
#undef MODVERSIONS
#undef _set_ver
#endif
#endif

#ifndef SND_MAIN_OBJECT_FILE
#define __NO_VERSION__
#endif
#include <linux/module.h>

#include <linux/utsname.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/malloc.h>
#include <linux/delay.h>

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

#ifdef LINUX_2_2
#include "compat_22.h"
#endif
#ifdef LINUX_2_3
#include <linux/init.h>
#include <linux/pm.h>
#define PCI_GET_DRIVER_DATA(pci) pci->driver_data
#define PCI_SET_DRIVER_DATA(pci, data) pci->driver_data = data
#ifndef virt_to_page
#define virt_to_page(x) (&mem_map[MAP_NR(x)])
#endif
#endif

#if defined(CONFIG_ISAPNP) || (defined(CONFIG_ISAPNP_MODULE) && defined(MODULE))
#if (defined(CONFIG_ISAPNP_KERNEL) && defined(ALSA_BUILD)) || (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 3, 30) && !defined(ALSA_BUILD))
#include <linux/isapnp.h>
#define isapnp_dev pci_dev
#define isapnp_card pci_bus
#else
#include "isapnp.h"
#endif
#undef __ISAPNP__
#define __ISAPNP__
#endif

#ifdef CONFIG_SND_OSSEMUL
#define __SND_OSS_COMPAT__
#endif
#ifndef ALSA_BUILD
#include <linux/asound.h>
#else
#include "asound.h"
#endif

#include "sndmagic.h"

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

typedef struct snd_stru_dma_area snd_dma_area_t;
typedef struct snd_stru_dma snd_dma_t;

struct snd_stru_dma_area {
	struct list_head list;		/* list of DMA areas */
	volatile unsigned short
	 mmaped:1,			/* mmaped area to user space */
	 mmap_free:1;			/* free mmaped buffer */
	unsigned char *buf;		/* pointer to DMA buffer */
	unsigned long size;		/* real size of DMA buffer */
	char *owner;			/* owner of this DMA channel */
	snd_dma_t *dma;
};

#define snd_dma_area(n) list_entry(n, snd_dma_area_t, list)

struct snd_stru_dma {
	struct list_head list;		/* list of DMA channels */
	int type;			/* dma type - see SND_DMA_TYPE_XXXX */
	int multi: 1;			/* multi area support */
	unsigned long dma;		/* DMA number */
	int addressbits;		/* physical wired bits (24-64) */
	char *name;			/* pointer to name */
	long rsize;			/* requested size of DMA buffer */
	char *multi_match[2];		/* allowed owners for multi alloc */
	struct semaphore mutex;		/* snd_dma_malloc/free */
	struct list_head areas;		/* DMA areas */
};

#define snd_dma(n) list_entry(n, snd_dma_t, list)

#define SND_IRQ_TYPE_ISA	0	/* ISA IRQ */
#define SND_IRQ_TYPE_PCI	1	/* PCI IRQ (shareable) */
#define SND_IRQ_TYPE_EISA	SND_IRQ_TYPE_PCI

typedef struct snd_stru_irq {
	struct list_head list;		/* list of IRQ channels */
	int type;
	unsigned long irq;
	char *name;
	void *dev_id;
} snd_irq_t;

#define snd_irq(n) list_entry(n, snd_irq_t, list)

typedef struct snd_stru_port {
	struct list_head list;		/* list of port numbers */
	unsigned long port;
	unsigned long size;
	char *name;
#ifdef LINUX_2_3
	struct resource *res;
#endif
} snd_port_t;

#define snd_port(n) list_entry(n, snd_port_t, list)

typedef void (snd_irq_handler_t) (int irq, void *dev_id, struct pt_regs *regs);

typedef struct snd_stru_vma {
	struct list_head list;		/* list of all VMAs */
	struct vm_area_struct *area;
	void *notify_client;
	void *notify_data;
	long notify_size;
	void (*notify)(void *notify_client, void *notify_data);
} snd_vma_t;

#define snd_vma(n) list_entry(n, snd_vma_t, list)

#define SND_DEV_TYPE_RANGE_SIZE	0x1000

typedef enum {
	SND_DEV_LOWLEVEL_PRE = 0,
	SND_DEV_LOWLEVEL_NORMAL = SND_DEV_TYPE_RANGE_SIZE,
	SND_DEV_PCM,
	SND_DEV_RAWMIDI,
	SND_DEV_TIMER,
	SND_DEV_SEQUENCER,
	SND_DEV_HWDEP,
	SND_DEV_LOWLEVEL = (2*SND_DEV_TYPE_RANGE_SIZE)
} snd_device_type_t;

typedef enum {
	SND_DEV_BUILD = 0,
	SND_DEV_REGISTERED = 1
} snd_device_state_t;

typedef enum {
	SND_DEV_CMD_PRE = 0,
	SND_DEV_CMD_NORMAL = 1,
	SND_DEV_CMD_POST = 2
} snd_device_cmd_t;

typedef struct snd_stru_card snd_card_t;
typedef struct snd_stru_device snd_device_t;

typedef int (snd_dev_free_t)(snd_device_t *device);
typedef int (snd_dev_register_t)(snd_device_t *device);
typedef int (snd_dev_unregister_t)(snd_device_t *device);

typedef struct {
	snd_dev_free_t *dev_free;
	snd_dev_register_t *dev_register;
	snd_dev_unregister_t *dev_unregister;
} snd_device_ops_t;

struct snd_stru_device {
	struct list_head list;		/* list of registered devices */
	snd_card_t *card;		/* card which holds this device */
	snd_device_state_t state;	/* state of the device */
	snd_device_type_t type;		/* device type */
	void *device_data;		/* device structure */
	int number;			/* device number */
	void *arg;			/* optional argument (dynamically allocated) */
	int size;			/* size of an optional argument */
	snd_device_ops_t *ops;		/* operations */
	snd_device_t *parent;		/* must be freed at first */
};

#define snd_device(n) list_entry(n, snd_device_t, list)

/* various typedefs */

typedef struct snd_info_entry snd_info_entry_t;
typedef struct snd_stru_pcm snd_pcm_t;
typedef struct snd_stru_mixer snd_kmixer_t;
typedef struct snd_stru_rawmidi snd_rawmidi_t;
typedef struct snd_stru_kctl snd_kctl_t;
typedef struct snd_stru_kcontrol snd_kcontrol_t;
typedef struct snd_stru_timer snd_timer_t;
typedef struct snd_stru_timer_instance snd_timer_instance_t;
typedef struct snd_stru_hwdep snd_hwdep_t;
#ifdef CONFIG_SND_OSSEMUL
typedef struct snd_stru_oss_mixer snd_mixer_oss_t;
#endif

/* main structure for soundcard */

struct snd_stru_card {
	int number;				/* number of soundcard (index to snd_cards) */

	unsigned int type;			/* type (number ID) of soundcard */

	char id[16];				/* id string of this card */
	char abbreviation[16];			/* abbreviation of soundcard name */
	char shortname[32];			/* short name of this soundcard */
	char longname[80];			/* name of this soundcard */
	char mixerid[16];			/* mixer ID */
	char mixername[80];			/* mixer name */

	struct list_head ports;			/* list of I/O ports for this card */
	struct list_head irqs;			/* list of IRQs for this card */
	struct list_head dmas;			/* list of DMAs for this card */

	void (*use_inc) (snd_card_t *card);	/* increment use count */
	void (*use_dec) (snd_card_t *card);	/* decrement use count */

	void *private_data;			/* private data for soundcard */
	void (*private_free) (snd_card_t *card); /* callback for freeing of private data */

	struct list_head devices;		/* devices */

	unsigned int last_numid;		/* last used numeric ID */
	rwlock_t control_rwlock;	        /* control list lock */
	rwlock_t control_owner_lock;		/* control list lock */
	int controls_count;			/* count of all controls */
	struct list_head controls;		/* all controls for this card */
	struct list_head control_files;		/* active control files */

	struct proc_dir_entry *proc_dir;	/* root for soundcard specific files */
	struct proc_dir_entry *proc_dir_link;	/* number link to real id */
	struct list_head info_entries;		/* info entries */

#ifdef CONFIG_SND_OSSEMUL
	snd_mixer_oss_t *mixer_oss;
	int mixer_oss_change_count;
#endif
};

/* device.c */

typedef loff_t (snd_llseek_t) (struct file *file, loff_t offset, int orig);
typedef ssize_t (snd_read_t) (struct file *file, char *buf, size_t count);
typedef ssize_t (snd_write_t) (struct file *file, const char *buf, size_t count);
typedef ssize_t (snd_readv_t) (struct file *file, const struct iovec *vector, unsigned long count);
typedef ssize_t (snd_writev_t) (struct file *file, const struct iovec *vector, unsigned long count);
typedef int (snd_open_t) (unsigned short minor, int cardnum, int device, struct file *file);
typedef int (snd_release_t) (unsigned short minor, int cardnum, int device, struct file *file);
typedef unsigned int (snd_poll_t) (struct file *file, poll_table *wait);
typedef int (snd_ioctl_t) (struct file *file, unsigned int cmd, unsigned long arg);
typedef int (snd_mmap_t) (struct inode *inode, struct file *file, struct vm_area_struct *vma);

struct snd_stru_minor {
	char *comment;			/* for /dev/sndinfo */
	snd_info_entry_t *dev;		/* for /proc/asound/dev */

	snd_llseek_t *llseek;
	snd_read_t *read;
	snd_write_t *write;
	snd_readv_t *readv;
	snd_writev_t *writev;
	snd_open_t *open;
	snd_release_t *release;
	snd_poll_t *poll;
	snd_ioctl_t *ioctl;
	snd_mmap_t *mmap;
};

typedef struct snd_stru_minor snd_minor_t;

/* sound.c */

int snd_ecards_limit;
int snd_device_mode;
int snd_device_gid;
int snd_device_uid;

void snd_request_card(int card);

int snd_register_device(int type, snd_card_t *card, int dev, snd_minor_t *reg, const char *name);
int snd_unregister_device(int type, snd_card_t *card, int dev);

#ifdef CONFIG_SND_OSSEMUL
int snd_register_oss_device(int type, snd_card_t *card, int dev, snd_minor_t *reg, const char *name);
int snd_unregister_oss_device(int type, snd_card_t *card, int dev);
#endif

int snd_minor_info_init(void);
int snd_minor_info_done(void);

/* sound_oss.c */

#ifdef CONFIG_SND_OSSEMUL

int snd_minor_info_oss_init(void);
int snd_minor_info_oss_done(void);

int snd_oss_init_module(void);
void snd_oss_cleanup_module(void);

#endif

/* memory.c */

void snd_memory_init(void);
void snd_memory_done(void);
void *snd_kmalloc(size_t size, int flags);
void _snd_kfree(void *obj);
#ifdef CONFIG_SND_DEBUG
#define snd_kfree(obj) { \
	if (obj == NULL) \
		snd_printk("kfree(NULL)\n"); \
	else _snd_kfree(obj); \
} while (0)
#else
#define snd_kfree _snd_kfree
#endif
void *snd_kcalloc(size_t size, int flags);
char *snd_kmalloc_strdup(const char *string, int flags);
void *snd_malloc_pages(unsigned long size, int *pg, int dma);
void *snd_vmalloc(unsigned long size);
void snd_vfree(void *obj);
void snd_free_pages(void *ptr, unsigned long size);
int snd_dma_malloc(snd_card_t *card, snd_dma_t *dma, char *owner, snd_dma_area_t **rarea);
void snd_dma_free(snd_card_t *card, snd_dma_area_t *area);
#ifdef CONFIG_SND_DEBUG_MEMORY
int snd_memory_info_init(void);
int snd_memory_info_done(void);
void snd_memory_debug1(void);
#endif

/* vma.c */

void snd_vma_add(snd_vma_t *vma);
void snd_vma_disconnect(void *notify_client);

/* init.c */

extern int snd_cards_count;
extern snd_card_t *snd_cards[SND_CARDS];
extern rwlock_t snd_card_rwlock;
#ifdef CONFIG_SND_OSSEMUL
extern int (*snd_mixer_oss_notify_callback)(snd_card_t *card, int free_flag);
#endif

snd_card_t *snd_card_new(int idx, char *id,
			 void (*use_inc) (snd_card_t *card),
			 void (*use_dec) (snd_card_t *card),
			 int extra_size);
int snd_card_free(snd_card_t *card);
int snd_card_register(snd_card_t *card);
int snd_card_info_init(void);
int snd_card_info_done(void);

int snd_check_ioport(snd_card_t *card, unsigned long port, unsigned long size);
int snd_register_ioport(snd_card_t *card,
			unsigned long port, unsigned long size,
			char *name, snd_port_t **rport);
int snd_unregister_ioport(snd_card_t *card, snd_port_t *port);
int snd_unregister_ioports(snd_card_t *card);

int snd_register_dma_channel(snd_card_t *card, char *name, unsigned long number,
			     int type, long rsize, long *possible_numbers,
			     snd_dma_t **rdma);
int snd_unregister_dma_channels(snd_card_t *card);

int snd_register_interrupt(snd_card_t *card, char *name, unsigned long number,
			   int type, snd_irq_handler_t *handler,
			   void *dev_id, long *possible_numbers,
			   snd_irq_t **rirq);
int snd_unregister_interrupts(snd_card_t *card);

/* device.c */

int snd_device_new(snd_card_t *card, snd_device_type_t type,
		   void *device_data, int number, snd_device_ops_t *ops,
		   snd_device_t **rdev);
int snd_device_free(snd_card_t *card, void *device_data);
int snd_device_register(snd_card_t *card, void *device_data);
int snd_device_unregister(snd_card_t *card, void *device_data);
int snd_device_register_all(snd_card_t *card);
int snd_device_free_all(snd_card_t *card, snd_device_cmd_t cmd);

/* isadma.c */

#define DMA_MODE_NO_ENABLE	0x0100

void snd_dma_program(unsigned long dma, const void *buf, unsigned int size, unsigned short mode);
void snd_dma_disable(unsigned long dma);
unsigned int snd_dma_residue(unsigned long dma);

/* misc.c */

int snd_task_name(struct task_struct *task, char *name, size_t size);

/* --- */

#ifdef NEW_MACRO_VARARGS

/*
 *  VARARGS section
 */

#define snd_printk(...) do {\
	printk("ALSA %s:%d: ", __FILE__, __LINE__); \
 	printk(__VA_ARGS__); \
} while (0)

#ifdef CONFIG_SND_DEBUG

#define snd_printd(...) snd_printk(__VA_ARGS__)
#define snd_debug_check(expr, ...) do {\
	if (expr) {\
		snd_printk("BUG? (%s)\n", __STRING(expr));\
		__VA_ARGS__;\
	}\
} while (0)
#define snd_error_check(expr, ...) do {\
	if (expr) {\
		snd_printk("ERROR (%s)\n", __STRING(expr));\
		__VA_ARGS__;\
	}\
} while (0)

#else /* !CONFIG_SND_DEBUG */

#define snd_printd(...)	/* nothing */
#define snd_debug_check(expr, ...)	/* nothing */
#define snd_error_check(expr, ...) do { if (expr) {__VA_ARGS__;} } while (0)

#endif /* CONFIG_SND_DEBUG */

#ifdef CONFIG_SND_DEBUG_DETECT
#define snd_printdd(...) snd_printk(__VA_ARGS__)
#else
#define snd_printdd(...) /* nothing */
#endif

#else /* !NEW_MACRO_VARARGS */

/*
 *  Old args section...
 */

#define snd_printk(args...) do {\
	printk("ALSA %s:%d: ", __FILE__, __LINE__); \
 	printk(##args); \
} while (0)

#ifdef CONFIG_SND_DEBUG

#define snd_printd(args...) snd_printk(##args)
#define snd_debug_check(expr, args...) do {\
	if (expr) {\
		snd_printk("BUG? (%s)\n", __STRING(expr));\
		##args;\
	}\
} while (0)
#define snd_error_check(expr, args...) do {\
	if (expr) {\
		snd_printk("ERROR (%s)\n", __STRING(expr));\
		##args;\
	}\
} while (0)

#else /* !CONFIG_SND_DEBUG */

#define snd_printd(args...) /* nothing */
#define snd_debug_check(expr, args...)	/* nothing */
#define snd_error_check(expr, args...) do { if (expr) {##args;} } while (0)

#endif /* CONFIG_SND_DEBUG */

#ifdef CONFIG_SND_DEBUG_DETECT
#define snd_printdd(args...) snd_printk(##args)
#else
#define snd_printdd(args...) /* nothing */
#endif

#endif /* NEW_MACRO_VARARGS */

#define snd_BUG() snd_debug_check(0, )

#define snd_alloc_check(function, args)  ({\
	void *__ptr;\
	__ptr = function args;\
	if (__ptr == NULL)\
		return -ENOMEM;\
	__ptr;\
})
#define snd_kmalloc_check(size, flags) snd_alloc_check(snd_kmalloc, (size, flags))
#define snd_kcalloc_check(size, flags) snd_alloc_check(snd_kcalloc, (size, flags))

#define snd_timestamp_now(tstamp) do_gettimeofday(tstamp)
#define snd_timestamp_zero(tstamp) do { (tstamp)->tv_sec = 0; (tstamp)->tv_usec = 0; } while (0)
#define snd_timestamp_null(tstamp) ((tstamp)->tv_sec == 0 && (tstamp)->tv_usec ==0)

#define SND_OSS_VERSION         ((3<<16)|(8<<8)|(1<<4)|(0))	/* 3.8.1a */

#endif				/* __DRIVER_H */
