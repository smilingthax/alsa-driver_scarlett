#ifndef __DRIVER_H
#define __DRIVER_H

/*
 *  Main header file for the ALSA driver
 *  Copyright (c) 1994-2000 by Jaroslav Kysela <perex@suse.cz>
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

#define SNDRV_CARDS		8	/* number of supported soundcards - don't change - minor numbers */

#ifndef CONFIG_SND_MAJOR	/* standard configuration */
#define CONFIG_SND_MAJOR	116
#endif

#ifndef CONFIG_SND_DEBUG
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
#ifdef SNDRV_NO_MODVERS
#undef MODVERSIONS
#undef _set_ver
#endif
#endif /* ALSA_BUILD */

#ifndef SNDRV_MAIN_OBJECT_FILE
#define __NO_VERSION__
#endif
#include <linux/module.h>

#include <linux/utsname.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 7)
#include <linux/malloc.h>
#else
#include <linux/slab.h>
#endif
#include <linux/delay.h>
#include <linux/bitops.h>

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
#include <linux/pagemap.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <linux/reboot.h>

#ifdef LINUX_2_2
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 2, 18)
#include <linux/init.h>
#endif
#include "compat_22.h"
#endif
#ifdef LINUX_2_3
#include <linux/init.h>
#include <linux/pm.h>
#define PCI_GET_DRIVER_DATA(pci) pci->driver_data
#define PCI_SET_DRIVER_DATA(pci, data) pci->driver_data = data
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 3)
#define pci_set_dma_mask(pci, mask) pci->dma_mask = mask
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 7)
#define PCI_NEW_SUSPEND
#endif
#ifndef virt_to_page
#define virt_to_page(x) (&mem_map[MAP_NR(x)])
#endif
#define snd_request_region request_region
#ifndef rwlock_init
#define rwlock_init(x) do { *(x) = RW_LOCK_UNLOCKED; } while(0)
#endif
#define snd_kill_fasync(fp, sig, band) kill_fasync(fp, sig, band)
#if defined(__i386__) || defined(__ppc__)
/*
 * Here a dirty hack for 2.4 kernels.. See kernel/memory.c.
 */
#define HACK_PCI_ALLOC_CONSISTENT
void *snd_pci_hack_alloc_consistent(struct pci_dev *hwdev, size_t size,
				    dma_addr_t *dma_handle);
#undef pci_alloc_consistent
#define pci_alloc_consistent snd_pci_hack_alloc_consistent
#endif /* i386 or ppc */
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

#include <sound/asound.h>
#include <sound/asoundef.h>

/* Name change */
typedef struct timeval snd_timestamp_t;
typedef struct sndrv_interval snd_interval_t;
typedef enum sndrv_card_type snd_card_type;
typedef sndrv_pcm_uframes_t snd_pcm_uframes_t;
typedef sndrv_pcm_sframes_t snd_pcm_sframes_t;
typedef struct sndrv_aes_iec958 snd_aes_iec958_t;
typedef enum sndrv_hwdep_iface snd_hwdep_iface_t;
typedef struct sndrv_hwdep_info snd_hwdep_info_t;
typedef enum sndrv_pcm_class snd_pcm_class_t;
typedef enum sndrv_pcm_subclass snd_pcm_subclass_t;
typedef enum sndrv_pcm_stream snd_pcm_stream_t;
typedef enum sndrv_pcm_access snd_pcm_access_t;
typedef enum sndrv_pcm_format snd_pcm_format_t;
typedef enum sndrv_pcm_subformat snd_pcm_subformat_t;
typedef enum sndrv_pcm_state snd_pcm_state_t;
typedef union sndrv_pcm_sync_id snd_pcm_sync_id_t;
typedef struct sndrv_pcm_info snd_pcm_info_t;
typedef enum sndrv_pcm_hw_param snd_pcm_hw_param_t;
typedef struct sndrv_pcm_hw_params snd_pcm_hw_params_t;
typedef enum sndrv_pcm_start snd_pcm_start_t;
typedef enum sndrv_pcm_xrun snd_pcm_xrun_t;
typedef enum sndrv_pcm_tstamp snd_pcm_tstamp_t;
typedef struct sndrv_pcm_sw_params snd_pcm_sw_params_t;
typedef struct sndrv_pcm_channel_info snd_pcm_channel_info_t;
typedef struct sndrv_pcm_status snd_pcm_status_t;
typedef struct sndrv_pcm_mmap_status snd_pcm_mmap_status_t;
typedef struct sndrv_pcm_mmap_control snd_pcm_mmap_control_t;
typedef struct sndrv_xferi snd_xferi_t;
typedef struct sndrv_xfern snd_xfern_t;
typedef enum sndrv_rawmidi_stream snd_rawmidi_stream_t;
typedef struct sndrv_rawmidi_info snd_rawmidi_info_t;
typedef struct sndrv_rawmidi_params snd_rawmidi_params_t;
typedef struct sndrv_rawmidi_status snd_rawmidi_status_t;
typedef enum sndrv_timer_class snd_timer_class_t;
typedef enum sndrv_timer_slave_class snd_timer_slave_class_t;
typedef enum sndrv_timer_global snd_timer_global_t;
typedef struct sndrv_timer_id snd_timer_id_t;
typedef struct sndrv_timer_select snd_timer_select_t;
typedef struct sndrv_timer_info snd_timer_info_t;
typedef struct sndrv_timer_params snd_timer_params_t;
typedef struct sndrv_timer_status snd_timer_status_t;
typedef struct sndrv_timer_read snd_timer_read_t;
typedef struct sndrv_ctl_card_info snd_ctl_card_info_t;
typedef enum sndrv_ctl_elem_type snd_ctl_elem_type_t;
typedef enum sndrv_ctl_elem_iface snd_ctl_elem_iface_t;
typedef struct sndrv_ctl_elem_id snd_ctl_elem_id_t;
typedef struct sndrv_ctl_elem_list snd_ctl_elem_list_t;
typedef struct sndrv_ctl_elem_info snd_ctl_elem_info_t;
typedef struct sndrv_ctl_elem_value snd_ctl_elem_value_t;
typedef enum sndrv_ctl_event_type snd_ctl_event_type_t;
typedef struct sndrv_ctl_event snd_ctl_event_t;
typedef struct sndrv_xferv snd_xferv_t;

#ifdef CONFIG_SND_DEBUG_MEMORY
void *snd_wrapper_kmalloc(size_t, int);
#undef kmalloc
void snd_wrapper_kfree(const void *);
#undef kfree
void *snd_wrapper_vmalloc(size_t);
#undef vmalloc
void snd_wrapper_vfree(void *);
#undef vfree
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
static inline void dec_mod_count(struct module *module)
{
	if (module)
		__MOD_DEC_USE_COUNT(module);
}

#if defined(__alpha__) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 3, 14)
#undef writeb
#define writeb(v, a) do { __writeb((v),(a)); mb(); } while(0)
#undef writew
#define writew(v, a) do {__writew((v),(a)); mb(); } while(0)
#undef writel
#define writel(v, a) do {__writel((v),(a)); mb(); } while(0)
#undef writeq
#define writeq(v, a) do {__writeq((v),(a)); mb(); } while(0)
#endif

/* do we have virt_to_bus? */
#if defined(CONFIG_SPARC64)
#undef HAVE_VIRT_TO_BUS
#else
#define HAVE_VIRT_TO_BUS  1
#endif

/*
 *  ==========================================================================
 */

/* device allocation stuff */

#define SNDRV_DEV_TYPE_RANGE_SIZE		0x1000

typedef enum {
	SNDRV_DEV_TOPLEVEL =		(0*SNDRV_DEV_TYPE_RANGE_SIZE),
	SNDRV_DEV_LOWLEVEL_PRE,
	SNDRV_DEV_LOWLEVEL_NORMAL =	(1*SNDRV_DEV_TYPE_RANGE_SIZE),
	SNDRV_DEV_PCM,
	SNDRV_DEV_RAWMIDI,
	SNDRV_DEV_TIMER,
	SNDRV_DEV_SEQUENCER,
	SNDRV_DEV_HWDEP,
	SNDRV_DEV_LOWLEVEL =		(2*SNDRV_DEV_TYPE_RANGE_SIZE)
} snd_device_type_t;

typedef enum {
	SNDRV_DEV_BUILD = 0,
	SNDRV_DEV_REGISTERED = 1
} snd_device_state_t;

typedef enum {
	SNDRV_DEV_CMD_PRE = 0,
	SNDRV_DEV_CMD_NORMAL = 1,
	SNDRV_DEV_CMD_POST = 2
} snd_device_cmd_t;

typedef struct _snd_card snd_card_t;
typedef struct _snd_device snd_device_t;

typedef int (snd_dev_free_t)(snd_device_t *device);
typedef int (snd_dev_register_t)(snd_device_t *device);
typedef int (snd_dev_unregister_t)(snd_device_t *device);

typedef struct {
	snd_dev_free_t *dev_free;
	snd_dev_register_t *dev_register;
	snd_dev_unregister_t *dev_unregister;
} snd_device_ops_t;

struct _snd_device {
	struct list_head list;		/* list of registered devices */
	snd_card_t *card;		/* card which holds this device */
	snd_device_state_t state;	/* state of the device */
	snd_device_type_t type;		/* device type */
	void *device_data;		/* device structure */
	snd_device_ops_t *ops;		/* operations */
};

#define snd_device(n) list_entry(n, snd_device_t, list)

/* various typedefs */

typedef struct snd_info_entry snd_info_entry_t;
typedef struct _snd_pcm snd_pcm_t;
typedef struct _snd_pcm_str snd_pcm_str_t;
typedef struct _snd_pcm_substream snd_pcm_substream_t;
typedef struct _snd_mixer snd_kmixer_t;
typedef struct _snd_rawmidi snd_rawmidi_t;
typedef struct _snd_ctl_file snd_ctl_file_t;
typedef struct _snd_kcontrol snd_kcontrol_t;
typedef struct _snd_timer snd_timer_t;
typedef struct _snd_timer_instance snd_timer_instance_t;
typedef struct _snd_hwdep snd_hwdep_t;
#ifdef CONFIG_SND_OSSEMUL
typedef struct _snd_oss_mixer snd_mixer_oss_t;
#endif

/* main structure for soundcard */

struct _snd_card {
	int number;			/* number of soundcard (index to snd_cards) */

	char id[16];			/* id string of this card */
	char driver[16];		/* driver name */
	char shortname[32];		/* short name of this soundcard */
	char longname[80];		/* name of this soundcard */
	char mixername[80];		/* mixer name */
	char components[80];		/* card components delimited with space */

	struct module *module;		/* top-level module */

	void *private_data;		/* private data for soundcard */
	void (*private_free) (snd_card_t *card); /* callback for freeing of private data */

	struct list_head devices;	/* devices */

	unsigned int last_numid;	/* last used numeric ID */
	rwlock_t control_rwlock;	/* control list lock */
	rwlock_t control_owner_lock;	/* control list lock */
	int controls_count;		/* count of all controls */
	struct list_head controls;	/* all controls for this card */
	struct list_head ctl_files;	/* active control files */

	snd_info_entry_t *proc_root;	/* root for soundcard specific files */
	snd_info_entry_t *proc_id;	/* the card id */
	struct proc_dir_entry *proc_root_link;	/* number link to real id */

#ifdef CONFIG_SND_OSSEMUL
	snd_mixer_oss_t *mixer_oss;
	int mixer_oss_change_count;
#endif
};

/* device.c */

struct _snd_minor {
	struct list_head list;		/* list of all minors per card */
	int number;			/* minor number */
	int device;			/* device number */
	const char *comment;		/* for /proc/asound/devices */
	snd_info_entry_t *dev;		/* for /proc/asound/dev */
	struct file_operations *f_ops;	/* file operations */
};

typedef struct _snd_minor snd_minor_t;

/* sound.c */

extern int snd_ecards_limit;
extern int snd_device_mode;
extern int snd_device_gid;
extern int snd_device_uid;

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

#ifdef CONFIG_SND_DEBUG_MEMORY
void snd_memory_init(void);
void snd_memory_done(void);
int snd_memory_info_init(void);
int snd_memory_info_done(void);
void *snd_hidden_kmalloc(size_t size, int flags);
void snd_hidden_kfree(const void *obj);
void *snd_hidden_vmalloc(unsigned long size);
void snd_hidden_vfree(void *obj);
#define kmalloc(size, flags) snd_hidden_kmalloc(size, flags)
#define kfree(obj) snd_hidden_kfree(obj)
#define vmalloc(size) snd_hidden_vmalloc(size)
#define vfree(obj) snd_hidden_vfree(obj)
#endif
void *snd_kcalloc(size_t size, int flags);
char *snd_kmalloc_strdup(const char *string, int flags);
void *snd_malloc_pages(unsigned long size, unsigned int dma_flags);
void *snd_malloc_pages_fallback(unsigned long size, unsigned int dma_flags, unsigned long *res_size);
void snd_free_pages(void *ptr, unsigned long size);
#ifdef CONFIG_PCI
void *snd_malloc_pci_pages(struct pci_dev *pci, unsigned long size, dma_addr_t *dmaaddr);
void *snd_malloc_pci_pages_fallback(struct pci_dev *pci, unsigned long size, dma_addr_t *dmaaddr, unsigned long *res_size);
void snd_free_pci_pages(struct pci_dev *pci, unsigned long size, void *ptr, dma_addr_t dmaaddr);
#endif
int copy_to_user_fromio(void *dst, unsigned long src, size_t count);
int copy_from_user_toio(unsigned long dst, const void *src, size_t count);

/* init.c */

extern int snd_cards_count;
extern snd_card_t *snd_cards[SNDRV_CARDS];
extern rwlock_t snd_card_rwlock;
#ifdef CONFIG_SND_OSSEMUL
extern int (*snd_mixer_oss_notify_callback)(snd_card_t *card, int free_flag);
#endif

snd_card_t *snd_card_new(int idx, const char *id,
			 struct module *module, int extra_size);
int snd_card_free(snd_card_t *card);
int snd_card_register(snd_card_t *card);
int snd_card_info_init(void);
int snd_card_info_done(void);
int snd_component_add(snd_card_t *card, const char *component);

/* device.c */

int snd_device_new(snd_card_t *card, snd_device_type_t type,
		   void *device_data, snd_device_ops_t *ops);
int snd_device_register(snd_card_t *card, void *device_data);
int snd_device_register_all(snd_card_t *card);
int snd_device_free(snd_card_t *card, void *device_data);
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
#define snd_assert(expr, ...) do {\
	if (!(expr)) {\
		snd_printk("BUG? (%s) (called from %p)\n", __STRING(expr), __builtin_return_address(0));\
		__VA_ARGS__;\
	}\
} while (0)
#define snd_runtime_check(expr, ...) do {\
	if (!(expr)) {\
		snd_printk("ERROR (%s) (called from %p)\n", __STRING(expr), __builtin_return_address(0));\
		__VA_ARGS__;\
	}\
} while (0)

#else /* !CONFIG_SND_DEBUG */

#define snd_printd(...)	/* nothing */
#define snd_assert(expr, ...)	/* nothing */
#define snd_runtime_check(expr, ...) do { if (!(expr)) {__VA_ARGS__;} } while (0)

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
#define snd_assert(expr, args...) do {\
	if (!(expr)) {\
		snd_printk("BUG? (%s)\n", __STRING(expr));\
		##args;\
	}\
} while (0)
#define snd_runtime_check(expr, args...) do {\
	if (!(expr)) {\
		snd_printk("ERROR (%s)\n", __STRING(expr));\
		##args;\
	}\
} while (0)

#else /* !CONFIG_SND_DEBUG */

#define snd_printd(args...) /* nothing */
#define snd_assert(expr, args...)	/* nothing */
#define snd_runtime_check(expr, args...) do { if (!(expr)) {##args;} } while (0)

#endif /* CONFIG_SND_DEBUG */

#ifdef CONFIG_SND_DEBUG_DETECT
#define snd_printdd(args...) snd_printk(##args)
#else
#define snd_printdd(args...) /* nothing */
#endif

#endif /* NEW_MACRO_VARARGS */

#define snd_BUG() snd_assert(0, )

#define snd_timestamp_now(tstamp) do_gettimeofday(tstamp)
#define snd_timestamp_zero(tstamp) do { (tstamp)->tv_sec = 0; (tstamp)->tv_usec = 0; } while (0)
#define snd_timestamp_null(tstamp) ((tstamp)->tv_sec == 0 && (tstamp)->tv_usec ==0)

#define SNDRV_OSS_VERSION         ((3<<16)|(8<<8)|(1<<4)|(0))	/* 3.8.1a */

#endif				/* __DRIVER_H */
