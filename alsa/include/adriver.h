#ifndef __SOUND_LOCAL_DRIVER_H
#define __SOUND_LOCAL_DRIVER_H

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

#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 2, 3)
#error "This driver requires Linux 2.2.3 or higher."
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 3, 1)
#  if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 0)
#  error "This code requires Linux 2.4.0-test1 and higher."
#  endif
#define LINUX_2_4__donotuse
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 2, 0)
#define LINUX_2_2
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

#ifdef CONFIG_HAVE_OLD_REQUEST_MODULE
#include <linux/kmod.h>
#undef request_module
void snd_compat_request_module(const char *name, ...);
#define request_module(name, args...) snd_compat_request_module(name, ##args)
#endif

#include <linux/compiler.h>
#ifndef __user
#define __user
#endif

#ifdef CONFIG_PCI
#include <linux/pci.h>
#endif

#ifdef LINUX_2_2
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 2, 18)
#include <linux/init.h>
#endif
#ifndef LINUX_2_4__donotuse
#include "compat_22.h"
#endif
#endif /* LINUX_2_2 */

#ifdef LINUX_2_4__donotuse
#include <linux/init.h>
#include <linux/pm.h>
#include <asm/page.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 3)
#define pci_set_dma_mask(pci, mask) pci->dma_mask = mask
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 7)
#define PCI_OLD_SUSPEND
#endif
#ifndef virt_to_page
#define virt_to_page(x) (&mem_map[MAP_NR(x)])
#endif
#define snd_request_region request_region
#ifndef rwlock_init
#define rwlock_init(x) do { *(x) = RW_LOCK_UNLOCKED; } while(0)
#endif
#ifndef list_for_each_safe
#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = pos->next)
#endif
#endif /* LINUX_2_4__donotuse */

#ifndef __devexit_p
#define __devexit_p(x) x
#endif

#include <linux/kdev_t.h>
#ifndef major
#define major(x) MAJOR(x)
#endif
#ifndef minor
#define minor(x) MINOR(x)
#endif
#ifndef mk_kdev
#define mk_kdev(maj, min) MKDEV(maj, min)
#endif
#ifndef DECLARE_BITMAP
#define DECLARE_BITMAP(name,bits) \
	unsigned long name[((bits)+BITS_PER_LONG-1)/BITS_PER_LONG]
#endif

#include <linux/sched.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 3) && !defined(need_resched)
#define need_resched() (current->need_resched)
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 4)
#include <linux/fs.h>
static inline struct proc_dir_entry *PDE(const struct inode *inode)
{
	return (struct proc_dir_entry *) inode->u.generic_ip;
}
#endif
#include <asm/io.h>
#if !defined(isa_virt_to_bus)
#if defined(virt_to_bus) || defined(__alpha__)
#define isa_virt_to_bus virt_to_bus
#endif
#endif

#if defined(CONFIG_ISAPNP) || (defined(CONFIG_ISAPNP_MODULE) && defined(MODULE))
#include <linux/isapnp.h>
#ifndef CONFIG_PNP
#define CONFIG_PNP
#endif
#if (defined(CONFIG_ISAPNP_KERNEL) && defined(ALSA_BUILD)) || (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 3, 30) && !defined(ALSA_BUILD))
#define isapnp_dev pci_dev
#define isapnp_card pci_bus
#endif
#undef __ISAPNP__
#define __ISAPNP__
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
#undef CONFIG_PNP
#endif
#endif

#if !defined(CONFIG_ISA) && defined(CONFIG_SND_ISA)
#define CONFIG_ISA
#endif

#ifndef MODULE_LICENSE
#define MODULE_LICENSE(license)
#endif

#ifndef CONFIG_HAVE_STRLCPY
size_t snd_compat_strlcpy(char *dest, const char *src, size_t size);
#define strlcpy(dest, src, size) snd_compat_strlcpy(dest, src, size)
size_t snd_compat_strlcat(char *dest, const char *src, size_t size);
#define strlcat(dest, src, size) snd_compat_strlcat(dest, src, size)
#endif

#ifndef CONFIG_HAVE_SNPRINTF
#include <stdarg.h>
int snd_compat_snprintf(char * buf, size_t size, const char * fmt, ...);
int snd_compat_vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
#define snprintf(buf,size,fmt,args...) snd_compat_snprintf(buf,size,fmt,##args)
#define vsnprintf(buf,size,fmt,args) snd_compat_vsnprintf(buf,size,fmt,args)
#endif

#if defined(__alpha__) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 3, 14)
#include <asm/io.h>
#undef writeb
#define writeb(v, a) do { __writeb((v),(a)); mb(); } while(0)
#undef writew
#define writew(v, a) do { __writew((v),(a)); mb(); } while(0)
#undef writel
#define writel(v, a) do { __writel((v),(a)); mb(); } while(0)
#undef writeq
#define writeq(v, a) do { __writeq((v),(a)); mb(); } while(0)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 28)
#include <linux/interrupt.h>
static inline void synchronize_irq_wrapper(unsigned int irq) { synchronize_irq(); }
#undef synchronize_irq
#define synchronize_irq(irq)	synchronize_irq_wrapper(irq)
#endif /* LINUX_VERSION_CODE < 2.5.28 */
#ifndef IRQ_NONE
#define IRQ_NONE	/*void*/
#define IRQ_HANDLED	/*void*/
#define IRQ_RETVAL(x)	/*void*/
typedef void irqreturn_t;
#endif

#ifndef min
/*
 * copied from the include/linux/kernel.h file
 * for compatibility with earlier kernels.
 */
#define min(x,y) ({ \
	const typeof(x) _x = (x); \
	const typeof(y) _y = (y); \
	(void) (&_x == &_y); \
	_x < _y ? _x : _y; })
#define max(x,y) ({ \
	const typeof(x) _x = (x);	\
	const typeof(y) _y = (y);	\
	(void) (&_x == &_y);		\
	_x > _y ? _x : _y; })
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#include <linux/devfs_fs_kernel.h>
#ifdef CONFIG_DEVFS_FS
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 29)
#include <linux/fs.h>
#undef register_chrdev
#define register_chrdev devfs_register_chrdev
#undef unregister_chrdev
#define unregister_chrdev devfs_unregister_chrdev
#undef devfs_remove
void snd_compat_devfs_remove(const char *fmt, ...);
#define devfs_remove snd_compat_devfs_remove
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 67)
#undef devfs_mk_dir
int snd_compat_devfs_mk_dir(const char *dir, ...);
#define devfs_mk_dir snd_compat_devfs_mk_dir
#undef devfs_mk_cdev
int snd_compat_devfs_mk_cdev(dev_t dev, umode_t mode, const char *fmt, ...);
#define devfs_mk_cdev snd_compat_devfs_mk_cdev
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 3, 0)
static inline void devfs_find_and_unregister (devfs_handle_t dir, const char *name,
					      unsigned int major, unsigned int minor,
                                              char type, int traverse_symlinks)
{
	devfs_handle_t master;
	master = devfs_find_handle(dir, name, strlen(name), major, minor, type, traverse_symlinks);
	devfs_unregister(master);
}
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
static inline void devfs_find_and_unregister (devfs_handle_t dir, const char *name,
					      unsigned int major, unsigned int minor,
                                              char type, int traverse_symlinks)
{
	devfs_handle_t master;
	master = devfs_find_handle(dir, name, major, minor, type, traverse_symlinks);
	devfs_unregister(master);
}
#endif
#else /* !CONFIG_DEVFS_FS */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 29)
static inline void devfs_remove(const char *fmt, ...) { }
#endif
#undef devfs_mk_dir
#define devfs_mk_dir(dir, args...) do { (void)(dir); } while (0)
#undef devfs_mk_cdev
#define devfs_mk_cdev(dev, mode, fmt, args...) do { (void)(dev); } while (0)
#endif /* CONFIG_DEVFS_FS */

/* workarounds for USB API */
#if defined(SND_NEED_USB_WRAPPER) && (defined(CONFIG_USB) || defined(CONFIG_USB_MODULE))

#include <linux/usb.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 20)
inline static int usb_make_path(struct usb_device *dev, char *buf, size_t size)
{
	int actual;
	actual = snprintf(buf, size, "%03d/%03d", dev->bus->busnum, dev->devnum);
	return (actual >= (int)size) ? -1 : actual;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
inline static struct urb *usb_alloc_urb_wrapper(int iso_packets, int flags)
{
	return usb_alloc_urb(iso_packets);
}
inline static int usb_submit_urb_wrapper(struct urb *urb, int flags)
{
	return usb_submit_urb(urb);
}
#undef usb_alloc_urb
#undef usb_submit_urb
#define usb_alloc_urb(n,flags) usb_alloc_urb_wrapper(n,flags)
#define usb_submit_urb(p,flags) usb_submit_urb_wrapper(p,flags)
#define OLD_USB
#endif /* LINUX_VERSION_CODE < 2.5.0 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 24)
int snd_hack_usb_set_interface(struct usb_device *dev, int interface, int alternate);
#undef usb_set_interface
#define usb_set_interface(dev,iface,alt) snd_hack_usb_set_interface(dev,iface,alt)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 45)
#define URB_ISO_ASAP		USB_ISO_ASAP
#define URB_ASYNC_UNLINK	USB_ASYNC_UNLINK
#define usb_fill_int_urb	FILL_INT_URB
#define usb_fill_bulk_urb	FILL_BULK_URB
#define usb_host_config		usb_config_descriptor
#define usb_host_interface	usb_interface_descriptor
#define usb_host_endpoint	usb_endpoint_descriptor
#define get_iface(cfg, num)	(&(cfg)->interface[num])
#define get_iface_desc(iface)	(iface)
#define get_endpoint(alt,ep)	(&(alt)->endpoint[ep])
#define get_ep_desc(ep)		(ep)
#define get_cfg_desc(cfg)	(cfg)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 45)
#define usb_pipe_needs_resubmit(pipe) (!usb_pipeint(pipe))
#endif

#endif /* SND_NEED_USB_WRAPPER && CONFIG_USB */

/* workqueue-alike; 2.5.45 */
#include <linux/workqueue.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 45) && !defined(__WORK_INITIALIZER)
struct work_struct {
	void (*func)(void *);
	void *data;
};
#define INIT_WORK(_work, _func, _data)			\
	do {						\
		(_work)->func = _func;			\
		(_work)->data = _data;			\
	} while (0)
#define __WORK_INITIALIZER(n, f, d) {			\
	.func = (f),					\
	.data = (d),					\
	}
#define DECLARE_WORK(n, f, d)				\
	struct work_struct n = __WORK_INITIALIZER(n, f, d)
int snd_compat_schedule_work(struct work_struct *work);
#define schedule_work(w) snd_compat_schedule_work(w)
#endif /* 2.5.45 */

/* 2.5 new modules */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
#define try_module_get(x) try_inc_mod_count(x)
static inline void module_put(struct module *module)
{
	if (module)
		__MOD_DEC_USE_COUNT(module);
}
#endif /* 2.5.0 */

/* gameport - 2.4 has different defines */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
#ifdef CONFIG_INPUT_GAMEPORT
#define CONFIG_GAMEPORT
#endif
#ifdef CONFIG_INPUT_GAMEPORT_MODULE
#define CONFIG_GAMEPORT_MODULE
#endif
#endif /* 2.5.0 */

/* vmalloc_to_page wrapper */
#ifndef CONFIG_HAVE_VMALLOC_TO_PAGE
struct page *snd_compat_vmalloc_to_page(void *addr);
#define vmalloc_to_page(addr) snd_compat_vmalloc_to_page(addr)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 69)
#include <linux/vmalloc.h>
static inline void *snd_compat_vmap(struct page **pages, unsigned int count, unsigned long flags, pgprot_t prot)
{
	return vmap(pages, count);
}
#undef vmap
#define vmap snd_compat_vmap
#endif

#include "amagic.h"

#endif /* __SOUND_LOCAL_DRIVER_H */
