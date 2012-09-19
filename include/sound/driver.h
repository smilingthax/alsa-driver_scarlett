#ifndef __SOUND_DRIVER_H
#define __SOUND_DRIVER_H

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
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

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

#include <linux/module.h>

#include <linux/utsname.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
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
#ifndef LINUX_2_3
#include "compat_22.h"
#endif
#endif /* LINUX_2_2 */

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
#ifndef list_for_each_safe
#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = pos->next)
#endif
#endif
#ifndef __devexit_p
#define __devexit_p(x) x
#endif
#ifndef major
#define major(x) MAJOR(x)
#endif
#ifndef minor
#define minor(x) MINOR(x)
#endif
#ifndef mk_kdev
#define mk_kdev(x) MKDEV(x)
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

#if !defined(CONFIG_ISA) && defined(CONFIG_SND_ISA)
#define CONFIG_ISA
#endif

#ifndef MODULE_LICENSE
#define MODULE_LICENSE(license)
#endif

#include <sound/asound.h>
#include <sound/asoundef.h>

/* Typedef's */
typedef struct timeval snd_timestamp_t;
typedef struct sndrv_interval snd_interval_t;
typedef enum sndrv_card_type snd_card_type;
typedef struct sndrv_xferi snd_xferi_t;
typedef struct sndrv_xfern snd_xfern_t;
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

#endif /* __SOUND_DRIVER_H */
