#ifndef __SOUND_LOCAL_DRIVER_H
#define __SOUND_LOCAL_DRIVER_H

/*
 *  Main header file for the ALSA driver
 *  Copyright (c) 1994-2000 by Jaroslav Kysela <perex@perex.cz>
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

#include "config-top.h"

/* number of supported soundcards */
#ifdef CONFIG_SND_DYNAMIC_MINORS
#define SNDRV_CARDS 32
#else
#define SNDRV_CARDS 8		/* don't change - minor numbers */
#endif

#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
#include <generated/autoconf.h>
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18)
#include <linux/autoconf.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 8)
#error "This driver requires Linux 2.6.8 or higher."
#endif

#ifndef RHEL_RELEASE_VERSION
#define RHEL_RELEASE_VERSION(a, b) (((a) << 8) | (b))
#endif

#include <linux/module.h>

#ifdef CONFIG_HAVE_OLD_REQUEST_MODULE
#include <linux/kmod.h>
#undef request_module
void snd_compat_request_module(const char *name, ...);
#define request_module(name, args...) snd_compat_request_module(name, ##args)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
#include <linux/compiler.h>
#ifndef __iomem
#define __iomem
#endif
#ifndef __user
#define __user
#endif
#ifndef __kernel
#define __kernel
#endif
#ifndef __nocast
#define __nocast
#endif
#ifndef __force
#define __force
#endif
#ifndef __safe
#define __safe
#endif
#include <linux/types.h>
#ifndef __bitwise
#define __bitwise
typedef __u16 __le16;
typedef __u16 __be16;
typedef __u32 __le32;
typedef __u32 __be32;
#endif
#ifndef __deprecated
# define __deprecated           /* unimplemented */
#endif
#endif /* < 2.6.9 */

/* other missing types */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
#if !defined(RHEL_RELEASE_CODE) || RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(5, 4)
typedef unsigned int fmode_t;
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 13)
#ifdef CONFIG_SND_ISA
#ifndef CONFIG_ISA_DMA_API
#define CONFIG_ISA_DMA_API
#endif
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 9)
#include <linux/interrupt.h>
#ifndef in_atomic
#define in_atomic()	in_interrupt()
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14) && \
	!defined(CONFIG_HAVE_GFP_T)
typedef unsigned __nocast gfp_t;
#endif

#ifndef CONFIG_HAVE_GFP_DMA32
#define GFP_DMA32 0		/* driver must check for 32-bit address */
#endif

#include <linux/wait.h>
#ifndef wait_event_timeout
#define __wait_event_timeout(wq, condition, ret)			\
do {									\
	DEFINE_WAIT(__wait);						\
									\
	for (;;) {							\
		prepare_to_wait(&wq, &__wait, TASK_UNINTERRUPTIBLE);	\
		if (condition)						\
			break;						\
		ret = schedule_timeout(ret);				\
		if (!ret)						\
			break;						\
	}								\
	finish_wait(&wq, &__wait);					\
} while (0)
#define wait_event_timeout(wq, condition, timeout)			\
({									\
	long __ret = timeout;						\
	if (!(condition))						\
		__wait_event_timeout(wq, condition, __ret);		\
	__ret;								\
})
#endif
#ifndef wait_event_interruptible_timeout
#define __wait_event_interruptible_timeout(wq, condition, ret)		\
do {									\
	DEFINE_WAIT(__wait);						\
									\
	for (;;) {							\
		prepare_to_wait(&wq, &__wait, TASK_INTERRUPTIBLE);	\
		if (condition)						\
			break;						\
		if (!signal_pending(current)) {				\
			ret = schedule_timeout(ret);			\
			if (!ret)					\
				break;					\
			continue;					\
		}							\
		ret = -ERESTARTSYS;					\
		break;							\
	}								\
	finish_wait(&wq, &__wait);					\
} while (0)
#define wait_event_interruptible_timeout(wq, condition, timeout)	\
({									\
	long __ret = timeout;						\
	if (!(condition))						\
		__wait_event_interruptible_timeout(wq, condition, __ret); \
	__ret;								\
})
#endif

#ifndef CONFIG_HAVE_STRLCPY
size_t snd_compat_strlcpy(char *dest, const char *src, size_t size);
#define strlcpy(dest, src, size) snd_compat_strlcpy(dest, src, size)
size_t snd_compat_strlcat(char *dest, const char *src, size_t size);
#define strlcat(dest, src, size) snd_compat_strlcat(dest, src, size)
#endif

#ifndef CONFIG_HAVE_SNPRINTF
int snd_compat_snprintf(char * buf, size_t size, const char * fmt, ...);
#define snprintf(buf,size,fmt,args...) snd_compat_snprintf(buf,size,fmt,##args)
#endif
#ifndef CONFIG_HAVE_VSNPRINTF
#include <stdarg.h>
int snd_compat_vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
#define vsnprintf(buf,size,fmt,args) snd_compat_vsnprintf(buf,size,fmt,args)
#endif

#ifndef CONFIG_HAVE_SCNPRINTF
#define scnprintf(buf,size,fmt,args...) snprintf(buf,size,fmt,##args)
#define vscnprintf(buf,size,fmt,args) vsnprintf(buf,size,fmt,args)
#endif

#ifndef CONFIG_HAVE_SSCANF
#include <stdarg.h>
int snd_compat_sscanf(const char * buf, const char * fmt, ...);
int snd_compat_vsscanf(const char * buf, const char * fmt, va_list args);
#define sscanf(buf,fmt,args...) snd_compat_sscanf(buf,fmt,##args)
#define vsscanf(buf,fmt,args) snd_compat_vsscanf(buf,fmt,args)
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

#ifndef min_t
#define min_t(type,x,y) \
	({ type __x = (x); type __y = (y); __x < __y ? __x: __y; })
#define max_t(type,x,y) \
	({ type __x = (x); type __y = (y); __x > __y ? __x: __y; })
#endif

#ifndef container_of
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#ifndef ALIGN
#define ALIGN(x,a) (((x)+(a)-1)&~((a)-1))
#endif

#ifndef roundup
#define roundup(x, y) ((((x) + ((y) - 1)) / (y)) * (y))
#endif

#ifndef BUG_ON
#define BUG_ON(x) /* nothing */
#endif

#ifndef BUILD_BUG_ON
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))
#endif

#if defined(SND_NEED_USB_WRAPPER) && (defined(CONFIG_USB) || defined(CONFIG_USB_MODULE))
#include "usb_wrapper.h"
#endif

#ifdef CONFIG_CREATE_WORKQUEUE_FLAGS
#include <linux/workqueue.h>
#undef create_workqueue
struct workqueue_struct *snd_compat_create_workqueue2(const char *name);
#define create_workqueue(name) snd_compat_create_workqueue2(name)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 6)
#include <linux/workqueue.h>
#ifndef create_singlethread_workqueue
#define create_singlethread_workqueue(name) create_workqueue(name)
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 20)
#include <linux/workqueue.h>
/* redefine INIT_WORK() */
static inline void snd_INIT_WORK(struct work_struct *w, void (*f)(struct work_struct *))
{
	INIT_WORK(w, (void(*)(void*))(f), w);
}
#undef INIT_WORK
#define INIT_WORK(w,f) snd_INIT_WORK(w,f)
#ifndef SND_WORKQUEUE_COMPAT
#define delayed_work snd_delayed_work
#endif
/* delayed_work wrapper */
struct delayed_work {
	struct work_struct work;
};
#undef INIT_DELAYED_WORK
#define INIT_DELAYED_WORK(_work, _func)	INIT_WORK(&(_work)->work, _func)
#ifndef SND_WORKQUEUE_COMPAT
/* redefine *_delayed_work() */
#define queue_delayed_work(wq,_work,delay) \
	queue_delayed_work(wq, &(_work)->work, delay)
#define schedule_delayed_work(_work,delay) \
	schedule_delayed_work(&(_work)->work, delay)
#define cancel_delayed_work(_work) \
	cancel_delayed_work(&(_work)->work)
#define work_pending(work) \
	test_bit(0, &(work)->pending)
#define delayed_work_pending(w) \
	work_pending(&(w)->work)
#endif /* !SND_WORKQUEUE_COMPAT */
#endif /* < 2.6.20 */

/* vmalloc_to_page wrapper */
#ifndef CONFIG_HAVE_VMALLOC_TO_PAGE
struct page *snd_compat_vmalloc_to_page(void *addr);
#define vmalloc_to_page(addr) snd_compat_vmalloc_to_page(addr)
#endif

#define EXPORT_NO_SYMBOLS

/* MODULE_ALIAS & co. */
#ifndef MODULE_ALIAS
#define MODULE_ALIAS(x)
#define MODULE_ALIAS_CHARDEV_MAJOR(x)
#endif

#ifndef MODULE_FIRMWARE
#define MODULE_FIRMWARE(x)
#endif

#ifndef CONFIG_HAVE_PCI_CONSISTENT_DMA_MASK
#define pci_set_consistent_dma_mask(p,x) 0 /* success */
#endif

/* sysfs */
#ifndef CONFIG_SND_NESTED_CLASS_DEVICE
#include <linux/device.h>
#define class_device_create(cls,prt,devnum,args...) class_device_create(cls,devnum,##args)
#endif

/* msleep */
#ifndef CONFIG_HAVE_MSLEEP
void snd_compat_msleep(unsigned int msecs);
#define msleep snd_compat_msleep
#endif

#ifndef CONFIG_HAVE_MSLEEP_INTERRUPTIBLE
#include <linux/delay.h>
unsigned long snd_compat_msleep_interruptible(unsigned int msecs);
#define msleep_interruptible snd_compat_msleep_interruptible
#ifndef ssleep
#define ssleep(x) msleep((unsigned int)(x) * 1000)
#endif
#endif

/* msecs_to_jiffies */
#ifndef CONFIG_HAVE_MSECS_TO_JIFFIES
#include <linux/jiffies.h>
#if defined(DESKTOP_HZ) && LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#define HAVE_VARIABLE_HZ	/* 2.4 SUSE kernel, HZ is a variable */
#endif
static inline unsigned int jiffies_to_msecs(const unsigned long j)
{
#ifndef HAVE_VARIABLE_HZ
	#if HZ <= 1000 && !(1000 % HZ)
		return (1000 / HZ) * j;
	#elif HZ > 1000 && !(HZ % 1000)
		return (j + (HZ / 1000) - 1)/(HZ / 1000);
	#else
		return (j * 1000) / HZ;
	#endif
#else
	if (HZ <= 1000 && !(1000 % HZ))
		return (1000 / HZ) * j;
	else if (HZ > 1000 && !(HZ % 1000))
		return (j + (HZ / 1000) - 1)/(HZ / 1000);
	else
		return (j * 1000) / HZ;
#endif
}
static inline unsigned long msecs_to_jiffies(const unsigned int m)
{
	if (m > jiffies_to_msecs(MAX_JIFFY_OFFSET))
		return MAX_JIFFY_OFFSET;
#ifndef HAVE_VARIABLE_HZ
	#if HZ <= 1000 && !(1000 % HZ)
		return (m + (1000 / HZ) - 1) / (1000 / HZ);
	#elif HZ > 1000 && !(HZ % 1000)
		return m * (HZ / 1000);
	#else
		return (m * HZ + 999) / 1000;
	#endif
#else
	if (HZ <= 1000 && !(1000 % HZ))
		return (m + (1000 / HZ) - 1) / (1000 / HZ);
	else if (HZ > 1000 && !(HZ % 1000))
		return m * (HZ / 1000);
	else
		return (m * HZ + 999) / 1000;
#endif
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
#include <linux/dma-mapping.h>

#ifndef DMA_32BIT_MASK
#define DMA_32BIT_MASK 0xffffffff
#endif
#ifndef DMA_31BIT_MASK
#define DMA_31BIT_MASK	0x000000007fffffffULL
#endif
#ifndef DMA_30BIT_MASK
#define DMA_30BIT_MASK	0x000000003fffffffULL
#endif
#ifndef DMA_28BIT_MASK
#define DMA_28BIT_MASK	0x000000000fffffffULL
#endif
#ifndef DMA_24BIT_MASK
#define DMA_24BIT_MASK	0x0000000000ffffffULL
#endif
#ifndef DMA_BIT_MASK
#define DMA_BIT_MASK(n)	(((n) == 64) ? ~0ULL : ((1ULL<<(n))-1))
#endif
#ifndef __GFP_ZERO
#define __GFP_ZERO	0x4000
#define CONFIG_HAVE_OWN_GFP_ZERO 1
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 0)
#include <linux/moduleparam.h>
#ifndef param_get_bint
#define bint bool
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 10)
#include <linux/moduleparam.h>
#undef module_param_array
#define module_param_array(name, type, nump, perm) \
	static int boot_devs_##name; \
	module_param_array_named(name, name, type, boot_devs_##name, perm)
#endif /* < 2.6.10 */
#endif /* < 3.2.0 */

/* dump_stack hack */
#ifndef CONFIG_HAVE_DUMP_STACK
#undef dump_stack
#define dump_stack()
#endif

#ifdef CONFIG_PCI
#ifndef CONFIG_HAVE_PCI_DEV_PRESENT
#include <linux/pci.h>
#ifndef PCI_DEVICE
#define PCI_DEVICE(vend,dev) \
	.vendor = (vend), .device = (dev), \
	.subvendor = PCI_ANY_ID, .subdevice = PCI_ANY_ID
#endif
int snd_pci_dev_present(const struct pci_device_id *ids);
#define pci_dev_present(x) snd_pci_dev_present(x)
#endif
#endif

/*
 * memory allocator wrappers
 */
#if defined(CONFIG_SND_DEBUG_MEMORY) && !defined(SKIP_HIDDEN_MALLOCS)

#include <linux/slab.h>
void *snd_hidden_kmalloc(size_t size, gfp_t flags);
void *snd_hidden_kzalloc(size_t size, gfp_t flags);
void *snd_hidden_kcalloc(size_t n, size_t size, gfp_t flags);
char *snd_hidden_kstrdup(const char *s, gfp_t flags);
char *snd_hidden_kstrndup(const char *s, size_t len, gfp_t flags);
void snd_hidden_kfree(const void *obj);
size_t snd_hidden_ksize(const void *obj);

static inline void *snd_wrapper_kmalloc(size_t size, gfp_t flags)
{
	return kmalloc(size, flags);
}
static inline void snd_wrapper_kfree(const void *obj)
{
	kfree(obj);
}

#define kmalloc(size, flags) snd_hidden_kmalloc(size, flags)
#define kzalloc(size, flags) snd_hidden_kzalloc(size, flags)
#define kcalloc(n, size, flags) snd_hidden_kcalloc(n, size, flags)
#define kstrdup(s, flags)  snd_hidden_kstrdup(s, flags)
#define kstrndup(s, len, flags)  snd_hidden_kstrndup(s, len, flags)
#define ksize(obj) snd_hidden_ksize(obj)
#define kfree(obj) snd_hidden_kfree(obj)

#define kmalloc_nocheck(size, flags) snd_wrapper_kmalloc(size, flags)
#define kfree_nocheck(obj) snd_wrapper_kfree(obj)

#else /* ! CONFIG_SND_DEBUG_MEMORY */

#define kmalloc_nocheck(size, flags) kmalloc(size, flags)
#define kfree_nocheck(obj) kfree(obj)

#ifndef CONFIG_HAVE_KCALLOC
void *snd_compat_kcalloc(size_t n, size_t size, gfp_t gfp_flags);
#define kcalloc(n,s,f) snd_compat_kcalloc(n,s,f)
#endif

#ifndef CONFIG_HAVE_KSTRDUP
char *snd_compat_kstrdup(const char *s, gfp_t gfp_flags);
#define kstrdup(s,f) snd_compat_kstrdup(s,f)
#endif

#ifndef CONFIG_HAVE_KSTRNDUP
char *snd_compat_kstrndup(const char *s, size_t len, gfp_t gfp_flags);
#define kstrndup(s,l,f) snd_compat_kstrndup(s,l,f)
#endif

#ifndef CONFIG_HAVE_KZALLOC
void *snd_compat_kzalloc(size_t n, gfp_t gfp_flags);
#define kzalloc(s,f) snd_compat_kzalloc(s,f)
#endif

#endif /* CONFIG_SND_DEBUG_MEMORY */

/* DEFINE_SPIN/RWLOCK (up to 2.6.11-rc2) */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 11)
#include <linux/spinlock.h>
#ifndef DEFINE_SPINLOCK
#define DEFINE_SPINLOCK(x) spinlock_t x = SPIN_LOCK_UNLOCKED
#define DEFINE_RWLOCK(x) rwlock_t x = RW_LOCK_UNLOCKED
#endif
#endif

/* pm_message_t type */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
#include <linux/pm.h>
#ifndef PMSG_FREEZE
typedef u32 __bitwise pm_message_t;
#define PMSG_FREEZE	3
#define PMSG_SUSPEND	3
#define PMSG_ON		0
#endif
#endif

/* PMSG_IS_AUTO macro */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0)
#include <linux/pm.h>
#ifndef PMSG_IS_AUTO
#define PMSG_IS_AUTO(msg)	(((msg).event & PM_EVENT_AUTO) != 0)
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
#ifdef CONFIG_PCI
#include <linux/pci.h>
#ifndef PCI_D0
#define PCI_D0     0
#define PCI_D1     1
#define PCI_D2     2
#define PCI_D3hot  3
#define PCI_D3cold 4
#define pci_choose_state(pci,state)	((state) ? PCI_D3hot : PCI_D0)
#endif
#endif
#endif

/* vprintk */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 9)
#include <linux/kernel.h>
static inline void snd_compat_vprintk(const char *fmt, va_list args)
{
	char tmpbuf[512];
	vsnprintf(tmpbuf, sizeof(tmpbuf), fmt, args);
	printk(tmpbuf);
}
#define vprintk snd_compat_vprintk
#endif

#if defined(CONFIG_GAMEPORT) || defined(CONFIG_GAMEPORT_MODULE)
#define wait_ms gameport_wait_ms
#include <linux/gameport.h>
#undef wait_ms
#ifndef to_gameport_driver
#include <linux/slab.h>
/* old gameport interface */
struct snd_gameport {
	struct gameport gp;
	void *port_data;
};
static inline struct gameport *gameport_allocate_port(void)
{
	struct snd_gameport *gp;
	gp = kzalloc(sizeof(*gp), GFP_KERNEL);
	if (gp)
		return &gp->gp;
	return NULL;
}
#define gameport_free_port(gp)	kfree(gp)
static inline void snd_gameport_unregister_port(struct gameport *gp)
{
	gameport_unregister_port(gp);
	kfree(gp);
}
#undef gameport_unregister_port
#define gameport_unregister_port(gp)	snd_gameport_unregister_port(gp)
#define gameport_set_port_data(gp,r) (((struct snd_gameport *)(gp))->port_data = (r))
#define gameport_get_port_data(gp) ((struct snd_gameport *)(gp))->port_data
#define gameport_set_dev_parent(gp,xdev)
#define gameport_set_name(gp,x)
#define gameport_set_phys(gp,x,y)
#endif /* to_gameport_driver */
#endif /* GAMEPORT || GAMEPORT_MODULE */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 29)
#define SND_COMPAT_DEV_PM_OPS
#endif

#ifdef SND_COMPAT_DEV_PM_OPS
#include <linux/device.h>
#include <linux/pm.h>

/* so far only handle simple cases */
struct snd_compat_dev_pm_ops {
	int (*suspend)(struct device *dev);
	int (*resume)(struct device *dev);
};

struct snd_compat_dev_pm_driver {
	const char *name;
	struct module *owner;
	const struct snd_compat_dev_pm_ops *pm;
};

#define SIMPLE_DEV_PM_OPS(name, suspend_fn, resume_fn) \
const struct snd_compat_dev_pm_ops name = { \
	.suspend = suspend_fn, \
	.resume = resume_fn, \
}

#ifdef CONFIG_PCI
#include <linux/pci.h>

struct snd_compat_pci_driver {
	struct pci_driver real_driver;
	char *name;
	const struct pci_device_id *id_table;
	int (*probe)(struct pci_dev *dev, const struct pci_device_id *id);
	void (*remove)(struct pci_dev *dev);
	void (*shutdown)(struct pci_dev *dev);
	struct snd_compat_dev_pm_driver driver;
};

int snd_compat_pci_register_driver(struct snd_compat_pci_driver *driver);
static inline void snd_compat_pci_unregister_driver(struct snd_compat_pci_driver *driver)
{
	pci_unregister_driver(&driver->real_driver);
}

static inline int snd_orig_pci_register_driver(struct pci_driver *driver)
{
	return pci_register_driver(driver);
}

#define pci_driver		snd_compat_pci_driver
#undef pci_register_driver
#define pci_register_driver	snd_compat_pci_register_driver
#undef pci_unregister_driver
#define pci_unregister_driver	snd_compat_pci_unregister_driver
#endif /* CONFIG_PCI */

/*
 */
#include <linux/platform_device.h>

/* defined in platform_device_compat.h */
#ifndef SND_COMPAT_PLATFORM_DEVICE

struct snd_compat_platform_driver {
	struct platform_driver real_driver;
	int (*probe)(struct platform_device *);
	int (*remove)(struct platform_device *);
	void (*shutdown)(struct platform_device *);
	struct snd_compat_dev_pm_driver driver;
};

int snd_compat_platform_driver_register(struct snd_compat_platform_driver *driver);
static inline void snd_compat_platform_driver_unregister(struct snd_compat_platform_driver *driver)
{
	platform_driver_unregister(&driver->real_driver);
}

static inline int snd_orig_platform_driver_register(struct platform_driver *driver)
{
	return platform_driver_register(driver);
}

#define platform_driver		snd_compat_platform_driver
#undef platform_driver_register
#define platform_driver_register	snd_compat_platform_driver_register
#undef platform_driver_unregister
#define platform_driver_unregister	snd_compat_platform_driver_unregister
#endif /* !SND_COMPAT_PLATFORM_DEVICE */

#endif /* SND_COMPAT_DEV_PM_OPS */

/* wrapper for getnstimeofday()
 * it's needed for recent 2.6 kernels, too, due to lack of EXPORT_SYMBOL
 */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 14) && !defined(CONFIG_TIME_INTERPOLATION)
#define getnstimeofday(x) do { \
	struct timeval __x; \
	do_gettimeofday(&__x); \
	(x)->tv_sec = __x.tv_sec;	\
	(x)->tv_nsec = __x.tv_usec * 1000; \
} while (0)
#endif

/* schedule_timeout_[un]interruptible() wrappers */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
#include <linux/sched.h>
#define schedule_timeout_interruptible(x) ({set_current_state(TASK_INTERRUPTIBLE); schedule_timeout(x);})
#define schedule_timeout_uninterruptible(x) ({set_current_state(TASK_UNINTERRUPTIBLE); schedule_timeout(x);})
#endif

#if defined(CONFIG_PNP) && defined(CONFIG_PM)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
#ifndef CONFIG_HAVE_PNP_SUSPEND
#include <linux/pnp.h>
#define HAVE_PNP_PM_HACK
struct snd_pnp_driver {
	struct pnp_driver real_driver;
	char *name;
	const struct pnp_device_id *id_table;
	unsigned int flags;
	int  (*probe)  (struct pnp_dev *dev, const struct pnp_device_id *dev_id);
	void (*remove) (struct pnp_dev *dev);
	int (*suspend) (struct pnp_dev *dev, pm_message_t state);
	int (*resume) (struct pnp_dev *dev);
};	

int snd_pnp_register_driver(struct snd_pnp_driver *driver);
static inline void snd_pnp_unregister_driver(struct snd_pnp_driver *driver)
{
	pnp_unregister_driver(&driver->real_driver);
}

#define pnp_driver	snd_pnp_driver
#undef pnp_register_driver
#define pnp_register_driver	snd_pnp_register_driver
#undef pnp_unregister_driver
#define pnp_unregister_driver	snd_pnp_unregister_driver

struct snd_pnp_card_driver {
	struct pnp_card_driver real_driver;
	char * name;
	const struct pnp_card_device_id *id_table;
	unsigned int flags;
	int  (*probe)  (struct pnp_card_link *card, const struct pnp_card_device_id *card_id);
	void (*remove) (struct pnp_card_link *card);
	int (*suspend) (struct pnp_card_link *dev, pm_message_t state);
	int (*resume) (struct pnp_card_link *dev);
};

int snd_pnp_register_card_driver(struct snd_pnp_card_driver *driver);
static inline void snd_pnp_unregister_card_driver(struct snd_pnp_card_driver *driver)
{
	pnp_unregister_card_driver(&driver->real_driver);
}

#define pnp_card_driver		snd_pnp_card_driver
#undef pnp_register_card_driver
#define pnp_register_card_driver	snd_pnp_register_card_driver
#undef pnp_unregister_card_driver
#define pnp_unregister_card_driver	snd_pnp_unregister_card_driver

#endif /* ! CONFIG_HAVE_PNP_SUSPEND */
#endif /* 2.6 */
#endif /* CONFIG_PNP && CONFIG_PM */

/*
 * Another wrappers for pnp_register_*driver()
 * They shouldn't return positive values in the new API
 */
#ifdef CONFIG_PNP
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0) && \
	LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 17)
#ifndef HAVE_PNP_PM_HACK
#include <linux/pnp.h>
static inline int snd_pnp_register_driver(struct pnp_driver *driver)
{
	int err = pnp_register_driver(driver);
	return err < 0 ? err : 0;
}
#define pnp_register_driver	snd_pnp_register_driver

static inline int snd_pnp_register_card_driver(struct pnp_card_driver *drv)
{
	int err = pnp_register_card_driver(drv);
	return err < 0 ? err : 0;
}
#define pnp_register_card_driver	snd_pnp_register_card_driver

#endif /* HAVE_PNP_PM_HACK */
#endif /* < 2.6.17 */
#endif /* PNP */

/*
 * old defines
 */
#define OPL3_HW_OPL3_PC98	0x0305	/* PC9800 */

/*
 * IRQF_* flags
 */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 17)
#include <linux/interrupt.h>
#ifndef IRQF_SHARED
#include <linux/signal.h>
#define IRQF_SHARED			SA_SHIRQ
#define IRQF_DISABLED			SA_INTERRUPT
#define IRQF_SAMPLE_RANDOM		SA_SAMPLE_RANDOM
#define IRQF_PERCPU			SA_PERCPU
#ifdef SA_PROBEIRQ
#define IRQF_PROBE_SHARED		SA_PROBEIRQ
#else
#define IRQF_PROBE_SHARED		0 /* dummy */
#endif
#endif /* IRQ_SHARED */
#endif /* <= 2.6.17 */

/*
 * lockdep macros
 */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 17)
#define lockdep_set_class(lock, key)		do { (void)(key); } while (0)
#define down_read_nested(sem, subclass)		down_read(sem)
#define down_write_nested(sem, subclass)	down_write(sem)
#define down_read_non_owner(sem)		down_read(sem)
#define up_read_non_owner(sem)			up_read(sem)
#define spin_lock_nested(lock, x)		spin_lock(lock)
#define spin_lock_irqsave_nested(lock, f, x)	spin_lock_irqsave(lock, f)
#endif

/*
 * PPC-specfic
 */
#ifdef CONFIG_PPC
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
#include <linux/interrupt.h>
#ifndef NO_IRQ
#define NO_IRQ	(-1)
#endif
#define irq_of_parse_and_map(node, x) \
	(((node) && (node)->n_intrs > (x)) ? (node)->intrs[x].line : NO_IRQ)
#endif /* < 2.6.18 */
#endif /* PPC */

/* SEEK_XXX */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
#include <linux/fs.h>
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#endif

/* kmemdup() wrapper */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19) || defined(CONFIG_SND_DEBUG_MEMORY)
#include <linux/string.h>
#include <linux/slab.h>
static inline void *snd_kmemdup(const void *src, size_t len, gfp_t gfp)
{
	void *dst = kmalloc(len, gfp);
	if (!dst)
		return NULL;
	memcpy(dst, src, len);
	return dst;
}
#define kmemdup	snd_kmemdup
#endif

/* wrapper for new irq handler type */
#ifndef CONFIG_SND_NEW_IRQ_HANDLER
#include <linux/interrupt.h>
typedef irqreturn_t (*snd_irq_handler_t)(int, void *);
#undef irq_handler_t
#define irq_handler_t snd_irq_handler_t
int snd_request_irq(unsigned int, irq_handler_t handler,
		    unsigned long, const char *, void *);
void snd_free_irq(unsigned int, void *);
#undef request_irq
#define request_irq	snd_request_irq
#undef free_irq
#define free_irq	snd_free_irq
extern struct pt_regs *snd_irq_regs;
#define get_irq_regs()	snd_irq_regs
#endif /* !CONFIG_SND_NEW_IRQ_HANDLER */

/* pci_intx() wrapper */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
#ifdef CONFIG_PCI
#undef pci_intx
#define pci_intx(pci,x)
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 20)
#define CONFIG_SYSFS_DEPRECATED	1
#endif

#ifndef CONFIG_HAVE_IS_POWER_OF_2
static inline __attribute__((const))
int is_power_of_2(unsigned long n)
{
	return n != 0 && ((n & (n - 1)) == 0);
}
#endif

#ifdef CONFIG_PCI
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 23)
#define snd_pci_revision(pci)	((pci)->revision)
#else
#include <linux/pci.h>
static inline unsigned char snd_pci_revision(struct pci_dev *pci)
{
	unsigned char rev;
	pci_read_config_byte(pci, PCI_REVISION_ID, &rev);
	return rev;
}
#endif
#endif /* PCI */

/* BIT* macros */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
#include <linux/bitops.h>
/*
#ifndef BIT
#define BIT(nr)			(1UL << (nr))
#endif
*/
#ifndef BIT_MASK
#define BIT_MASK(nr)		(1UL << ((nr) % BITS_PER_LONG))
#endif
#ifndef BIT_WORD
#define BIT_WORD(nr)		((nr) / BITS_PER_LONG)
#endif
#ifndef BITS_PER_BYTE
#define BITS_PER_BYTE		8
#endif
#endif

#ifndef CONFIG_HAVE_FFS
#if defined(__i386__)
static inline unsigned long __ffs(unsigned long word)
{
	__asm__("bsfl %1,%0"
		:"=r" (word)
		:"rm" (word));
	return word;
}
#else
static inline unsigned long __ffs(unsigned long word)
{
	__asm__("need_asm_for_your_arch_in_adriver.h");
	return word;
}
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 22)
#ifndef uninitialized_var
#define uninitialized_var(x) x = x
#endif
#endif /* <2.6.0 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
typedef unsigned long uintptr_t;
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 16)
#include <linux/time.h>
#define do_posix_clock_monotonic_gettime getnstimeofday
#endif

/* undefine SNDRV_CARDS again - it'll be re-defined in sound/core.h */
#undef SNDRV_CARDS

/* put_unaligned_*() */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
#include <asm/unaligned.h>
static inline void put_unaligned_le16(u16 val, void *p)
{
	val = cpu_to_le16(val);
	put_unaligned(val, (u16 *)p);
}

static inline void put_unaligned_le32(u32 val, void *p)
{
	val = cpu_to_le32(val);
	put_unaligned(val, (u32 *)p);
}

static inline void put_unaligned_le64(u64 val, void *p)
{
	val = cpu_to_le64(val);
	put_unaligned(val, (u64 *)p);
}

static inline void put_unaligned_be16(u16 val, void *p)
{
	val = cpu_to_be16(val);
	put_unaligned(val, (u16 *)p);
}

static inline void put_unaligned_be32(u32 val, void *p)
{
	val = cpu_to_be32(val);
	put_unaligned(val, (u32 *)p);
}

static inline void put_unaligned_be64(u64 val, void *p)
{
	val = cpu_to_be64(val);
	put_unaligned(val, (u64 *)p);
}

static inline u16 get_unaligned_le16(void *p)
{
	u16 val = get_unaligned((u16 *)p);
	return cpu_to_le16(val);
}

static inline u32 get_unaligned_le32(void *p)
{
	u32 val = get_unaligned((u32 *)p);
	return cpu_to_le32(val);
}

static inline u64 get_unaligned_le64(void *p)
{
	u64 val = get_unaligned((u64 *)p);
	return cpu_to_le64(val);
}
static inline u16 get_unaligned_be16(void *p)
{
	u16 val = get_unaligned((u16 *)p);
	return cpu_to_be16(val);
}

static inline u32 get_unaligned_be32(void *p)
{
	u32 val = get_unaligned((u32 *)p);
	return cpu_to_be32(val);
}

static inline u64 get_unaligned_be64(void *p)
{
	u64 val = get_unaligned((u64 *)p);
	return cpu_to_be64(val);
}
#endif

/* list_first_entry */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 22)
#include <linux/list.h>
#ifndef list_first_entry
#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)
#endif
#endif /* < 2.6.22 */

#ifndef upper_32_bits
#define upper_32_bits(n) ((u32)(((n) >> 16) >> 16))
#endif
#ifndef lower_32_bits
#define lower_32_bits(n) ((u32)(n))
#endif

#ifndef CONFIG_HAVE_PAGE_TO_PFN
#define page_to_pfn(page)       (page_to_phys(page) >> PAGE_SHIFT)
#endif

/* strto*() wrappers */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 25)
/* lazy wrapper - always returns 0 */
#define XXX_DEFINE_STRTO(name, type) \
static inline int __strict_strto##name(const char *cp, unsigned int base, \
				       type *valp) \
{ \
	*valp = simple_strto##name(cp, NULL, base); \
	return 0; \
}
XXX_DEFINE_STRTO(l, long);
XXX_DEFINE_STRTO(ul, unsigned long);
XXX_DEFINE_STRTO(ll, long long);
XXX_DEFINE_STRTO(ull, unsigned long long);
#undef XXX_DEFINE_STRTO
#define strict_strtol	__strict_strtol
#define strict_strtoll	__strict_strtoll
#define strict_strtoul	__strict_strtoul
#define strict_strtoull	__strict_strtoull
#endif /* < 2.6.25 */

/* pr_xxx() macros */
#ifndef pr_emerg
#define pr_emerg(fmt, arg...) \
	printk(KERN_EMERG fmt, ##arg)
#endif
#ifndef pr_alert
#define pr_alert(fmt, arg...) \
	printk(KERN_ALERT fmt, ##arg)
#endif
#ifndef pr_crit
#define pr_crit(fmt, arg...) \
	printk(KERN_CRIT fmt, ##arg)
#endif
#ifndef pr_err
#define pr_err(fmt, arg...) \
	printk(KERN_ERR fmt, ##arg)
#endif
#ifndef pr_warning
#define pr_warning(fmt, arg...) \
	printk(KERN_WARNING fmt, ##arg)
#endif
#ifndef pr_warn
#define pr_warn(fmt, arg...) \
	printk(KERN_WARNING fmt, ##arg)
#endif
#ifndef pr_notice
#define pr_notice(fmt, arg...) \
	printk(KERN_NOTICE fmt, ##arg)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26) && \
    LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
static inline const char *dev_name(struct device *dev)
{
	return dev->bus_id;
}

/* FIXME: return value is invalid */
#define dev_set_name(dev, fmt, args...) \
	snprintf((dev)->bus_id, sizeof((dev)->bus_id), fmt, ##args) 
#endif /* < 2.6.26 */

#ifndef WARN
#define WARN(condition, arg...) ({ \
	int __ret_warn_on = !!(condition);				\
	if (unlikely(__ret_warn_on)) {					\
		printk("WARNING: at %s:%d %s()\n",			\
			__FILE__, __LINE__, __func__);			\
		printk(arg);						\
		dump_stack();						\
	}								\
	unlikely(__ret_warn_on);					\
})
#endif

/* force to redefine WARN_ON() */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
#undef WARN_ON
#undef WARN_ON_ONCE
#undef WARN_ONCE
#endif

#ifndef WARN_ON
#define WARN_ON(condition) ({						\
	int __ret_warn_on = !!(condition);				\
	if (unlikely(__ret_warn_on)) {					\
		printk("WARNING: at %s:%d %s()\n",			\
			__FILE__, __LINE__, __func__);			\
		dump_stack();						\
	}								\
	unlikely(__ret_warn_on);					\
})
#endif

#ifndef WARN_ON_ONCE
#define WARN_ON_ONCE(condition) ({                              \
        static int __warned;                                    \
        int __ret_warn_once = !!(condition);                    \
                                                                \
        if (unlikely(__ret_warn_once))                          \
                if (WARN_ON(!__warned))                         \
                        __warned = 1;                           \
        unlikely(__ret_warn_once);                              \
})
#endif

#ifndef WARN_ONCE
#define WARN_ONCE(condition, format...)	({			\
	static int __warned;					\
	int __ret_warn_once = !!(condition);			\
								\
	if (unlikely(__ret_warn_once))				\
		if (WARN(!__warned, format)) 			\
			__warned = 1;				\
	unlikely(__ret_warn_once);				\
})
#endif

/*
 * wrapper for older SPARC codes with SBUS/EBUS specific bus
 */
#if defined(CONFIG_SBUS) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
struct sbus_dev;
#define snd_dma_sbus_data(sbus)        ((struct device *)(sbus))
#define SNDRV_DMA_TYPE_SBUS            4       /* SBUS continuous */
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 22)
#define DUMP_PREFIX_NONE	0
#define DUMP_PREFIX_OFFSET	1
void snd_compat_print_hex_dump_bytes(const char *prefix_str, int prefix_type,
				     const void *buf, size_t len);
#define print_hex_dump_bytes(a,b,c,d) snd_compat_print_hex_dump_bytes(a,b,c,d)
#endif

/*
 * pci_ioremap_bar() wrapper
 */
#ifdef CONFIG_PCI
#ifndef CONFIG_HAVE_PCI_IOREMAP_BAR
#include <linux/pci.h>
static inline void *pci_ioremap_bar(struct pci_dev *pdev, int bar)
{
	return ioremap_nocache(pci_resource_start(pdev, bar),
			       pci_resource_len(pdev, bar));
}
#endif
#endif

/*
 * definition of type 'bool'
 */
#ifndef CONFIG_SND_HAS_BUILTIN_BOOL
typedef int _Bool;
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
#ifndef bool	/* just to be sure */
#if !defined(RHEL_RELEASE_CODE) || RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(5, 4)
typedef _Bool bool;
#endif
#define true	1
#define false	0
#endif
#endif

/* memdup_user() wrapper */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 30) || \
	defined(CONFIG_SND_DEBUG_MEMORY)
#include <linux/err.h>
#include <asm/uaccess.h>
static inline void *snd_memdup_user(const void __user *src, size_t len)
{
	void *p = kmalloc(len, GFP_KERNEL);
	if (!p)
		return ERR_PTR(-ENOMEM);
	if (copy_from_user(p, src, len)) {
		kfree(p);
		return ERR_PTR(-EFAULT);
	}
	return p;
}
#define memdup_user snd_memdup_user
#endif

/* PCI_VEDEVICE() */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 20)
#include <linux/pci.h>
#ifndef PCI_VDEVICE
#define PCI_VDEVICE(vendor, device)		\
	PCI_VENDOR_ID_##vendor, (device),	\
	PCI_ANY_ID, PCI_ANY_ID, 0, 0
#endif
#endif /* < 2.6.20 */

/* put_pid() function was not exported before 2.6.19 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19)
#define CONFIG_SND_HAS_REFCOUNTED_STRUCT_PID
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19)
#define CONFIG_SND_HAS_TASK_PID
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
#define CONFIG_SND_HAS_PID_VNR
#endif
#ifndef CONFIG_SND_HAS_REFCOUNTED_STRUCT_PID
/* use nr as pointer */
struct pid;
#define get_pid(p) (p)
#define put_pid(p)
#define task_pid(t) ((struct pid *)((t)->pid))
#define pid_vnr(p) ((pid_t)(p))
#else
#ifndef CONFIG_SND_HAS_TASK_PID
static inline struct pid *task_pid(struct task_struct *task)
{
	return task->pids[PIDTYPE_PID].pid;
}
#endif
#ifndef CONFIG_SND_HAS_PID_VNR
static inline pid_t pid_vnr(struct pid *pid)
{
	return pid ? pid->nr : 0;
}
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 29)
#define pci_clear_master(x)
#endif

/* some new macros */
#ifndef FIELD_SIZEOF
#define FIELD_SIZEOF(t, f) (sizeof(((t*)0)->f))
#endif
#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#endif
#ifndef roundup
#define roundup(x, y) ((((x) + ((y) - 1)) / (y)) * (y))
#endif
#ifndef DIV_ROUND_CLOSEST
#define DIV_ROUND_CLOSEST(x, divisor)(			\
{							\
	typeof(divisor) __divisor = divisor;		\
	(((x) + ((__divisor) / 2)) / (__divisor));	\
}							\
)
#endif

/* skip_spaces() wrapper */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 33)
char *compat_skip_spaces(const char *);
#define skip_spaces	compat_skip_spaces
#endif

/* DEFINE_PCI_DEVICE_TABLE() */
#ifdef CONFIG_PCI
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 25)
#ifndef DEFINE_PCI_DEVICE_TABLE
/* originally it's __devinitconst but we use __devinitdata to be compatible
 * with older kernels
 */
#define DEFINE_PCI_DEVICE_TABLE(_table) \
	const struct pci_device_id _table[] __devinitdata
#endif
#endif /* < 2.6.25 */
#endif /* CONFIG_PCI */

/* blocking_notifier */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 17)
#include <linux/notifier.h>
#ifndef BLOCKING_INIT_NOTIFIER_HEAD
struct blocking_notifier_head {
	struct rw_semaphore rwsem;
	struct notifier_block *head;
};

#define BLOCKING_INIT_NOTIFIER_HEAD(name) do {	\
		init_rwsem(&(name)->rwsem);	\
		(name)->head = NULL;		\
	} while (0)

static inline int
blocking_notifier_call_chain(struct blocking_notifier_head *nh,
			     unsigned long val, void *v)
{
	int ret;
	down_read(&nh->rwsem);
	ret = notifier_call_chain(&nh->head, val, v);
	up_read(&nh->rwsem);
	return ret;
}

static inline int
blocking_notifier_chain_register(struct blocking_notifier_head *nh,
				 struct notifier_block *nb)
{
	int ret;
	down_write(&nh->rwsem);
	ret = notifier_chain_register(&nh->head, nb);
	up_write(&nh->rwsem);
	return ret;
}

static inline int
blocking_notifier_chain_unregister(struct blocking_notifier_head *nh,
				   struct notifier_block *nb)
{
	int ret;
	down_write(&nh->rwsem);
	ret = notifier_chain_unregister(&nh->head, nb);
	up_write(&nh->rwsem);
	return ret;
}
#endif /* BLOCKING_INIT_NOTIFIER_HEAD */
#endif /* <2.6.17 */

/* noop_llseek() */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35)
#define noop_llseek	NULL
#endif

/* hex_to_bin() */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35)
/* RHEL 6.1 kernels has version 2.6.32, but already have hex_to_bin() */
#if !defined(RHEL_RELEASE_CODE) || RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(6,1)
static inline int hex_to_bin(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	return -1;
}
#endif
#endif

#ifndef CONFIG_HAVE_VZALLOC
#include <linux/vmalloc.h>
static inline void *vzalloc(unsigned long size)
{
	void *p = vmalloc(size);
	if (p)
		memset(p, 0, size);
	return p;
}
#endif

/* flush_delayed_work_sync() wrapper */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 37)
#include <linux/workqueue.h>
static inline bool flush_work_sync(struct work_struct *work)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
	/* XXX */
	flush_scheduled_work();
	return true;
#else
	if (!flush_work(work))
		return false;
	while (flush_work(work))
		;
	return true;
#endif
}

static inline bool flush_delayed_work_sync(struct delayed_work *dwork)
{
	bool ret;
	ret = cancel_delayed_work(dwork);
	if (ret) {
		schedule_delayed_work(dwork, 0);
		flush_scheduled_work();
	}
	return ret;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
/* XXX this is a workaround; these are really different, but almost same
 * as used in the usual free/error path
 */
#define cancel_delayed_work_sync flush_delayed_work_sync
#endif

/* cancel_work_sync wrapper */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 22)
/* XXX */
#define cancel_work_sync(w)	flush_scheduled_work()
#endif

/* to_delayed_work() */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 30)
#define to_delayed_work(w) \
	((struct delayed_work *)container_of(w, struct delayed_work, work))
#endif

#endif /* < 2.6.37 */

/* pm_wakeup_event() wrapper */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 37)
#define pm_wakeup_event(dev, msec)
#endif

/* request_any_context_irq() wrapper */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35)
#define request_any_context_irq(irq, fn, flags, name, dev_id) \
	request_irq(irq, fn, flags, name, dev_id)
#endif

/* usleep_range() wrapper */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
/* FIXME: assuming msleep() */
#define usleep_range(x, y)	msleep(((x) + 999) / 1000)
#endif

/* hack - CONFIG_SND_HDA_INPUT_JACK can be wrongly set for older kernels */
#ifndef CONFIG_SND_JACK
#undef CONFIG_SND_HDA_INPUT_JACK
#endif

/* krealloc() wrapper */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 22)) || \
	(defined(CONFIG_SND_DEBUG_MEMORY) && !defined(SKIP_HIDDEN_MALLOCS))
#include <linux/slab.h>
static inline void *snd_compat_krealloc(const void *p, size_t new_size, gfp_t flags)
{
	void *n;
	if (!p)
		return kmalloc(new_size, flags);
	if (!new_size) {
		kfree(p);
		return NULL;
	}
	n = kmalloc(new_size, flags);
	if (!n)
		return NULL;
	memcpy(n, p, min(new_size, (size_t)ksize(p)));
	kfree(p);
	return n;
}
#define krealloc(p, s, f)	snd_compat_krealloc(p, s, f)
#endif

/* Workaround for IRQF_DISABLED removal in the upstream code */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 1, 0)
#include <linux/interrupt.h>
static inline int
request_irq_with_irqf_check(unsigned int irq, irq_handler_t handler,
			    unsigned long flags, const char *name, void *dev)
{
	if (!(flags & ~IRQF_PROBE_SHARED))
		flags |= IRQF_DISABLED;
	return request_irq(irq, handler, flags, name, dev);
}
#undef request_irq
#define request_irq(irq, fn, flags, name, dev_id) \
	request_irq_with_irqf_check(irq, fn, flags, name, dev_id)
#endif

/* if no __printf() macro is defined, just ignore it
 * (to be safer than defining gcc-specific)
 */
#ifndef __printf
#define __printf(a,b)	/*nop*/
#endif

/* module_driver() wrapper */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 0)
#include <linux/device.h>
#ifndef module_driver
#define module_driver(__driver, __register, __unregister) \
static int __init __driver##_init(void) { return __register(&(__driver)); } \
module_init(__driver##_init); \
static void __exit __driver##_exit(void) { __unregister(&(__driver)); } \
module_exit(__driver##_exit);
#endif
#endif /* < 3.4 */

/* module_platform_driver() wrapper */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0)
#include <linux/platform_device.h>
#ifndef module_platform_driver
#define module_platform_driver(__platform_driver) \
	module_driver(__platform_driver, platform_driver_register, \
			platform_driver_unregister)
#endif
#endif /* < 3.2 */

/* module_usb_driver() wrapper */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 0)
#define module_usb_driver(__usb_driver) \
    module_driver(__usb_driver, usb_register, \
			usb_deregister)
#define module_i2c_driver(__i2c_driver) \
	module_driver(__i2c_driver, i2c_add_driver, \
			i2c_del_driver)
#define module_spi_driver(__spi_driver) \
	module_driver(__spi_driver, spi_register_driver, \
			spi_unregister_driver)
#endif /* < 3.3 */

/* module_pci_driver() wrapper */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 0)
#include <linux/pci.h>
#ifndef module_pci_driver
#define module_pci_driver(__pci_driver) \
	module_driver(__pci_driver, pci_register_driver, \
		       pci_unregister_driver)
#endif
#endif /* < 3.4 */

/* some old kernels define info(), and this breaks the build badly */
#ifdef info
#undef info
#endif

#endif /* __SOUND_LOCAL_DRIVER_H */
