#define __NO_VERSION__
#include "adriver.h"
#include <linux/smp_lock.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <sound/core.h>

#ifndef CONFIG_HAVE_KZALLOC
#ifndef CONFIG_SND_DEBUG_MEMORY
/* Don't put this to wrappers.c.  We need to call the kmalloc wrapper here. */
void *snd_compat_kzalloc(size_t size, unsigned int __nocast flags)
{
	void *ret;
	ret = kmalloc(size, flags);
	if (ret)
		memset(ret, 0, size);
	return ret;
}
EXPORT_SYMBOL(snd_compat_kzalloc);
#endif
#endif

#ifndef CONFIG_HAVE_KCALLOC
#ifndef CONFIG_SND_DEBUG_MEMORY
/* Don't put this to wrappers.c.  We need to call the kmalloc wrapper here. */
void *snd_compat_kcalloc(size_t n, size_t size, unsigned int __nocast flags)
{
	if (n != 0 && size > INT_MAX / n)
		return NULL;
	return snd_compat_kzalloc(n * size, flags);
}
EXPORT_SYMBOL(snd_compat_kcalloc);
#endif
#endif

#ifndef CONFIG_HAVE_KSTRDUP
#ifndef CONFIG_SND_DEBUG_MEMORY
char *snd_compat_kstrdup(const char *s, unsigned int __nocast gfp_flags)
{
	int len;
	char *buf;

	if (!s) return NULL;

	len = strlen(s) + 1;
	buf = kmalloc(len, gfp_flags);
	if (buf)
		memcpy(buf, s, len);
	return buf;
}
EXPORT_SYMBOL(snd_compat_kstrdup);
#endif
#endif

#ifndef CONFIG_HAVE_KSTRNDUP
#ifndef CONFIG_SND_DEBUG_MEMORY
char *snd_compat_kstrndup(const char *s, size_t maxlen,
			  unsigned int __nocast gfp_flags)
{
	int len;
	char *buf;

	if (!s) return NULL;

	len = strlen(s);
	if (len >= maxlen)
		len = maxlen;
	buf = kmalloc(len + 1, gfp_flags);
	if (buf) {
		memcpy(buf, s, len);
		buf[len] = 0;
	}
	return buf;
}
EXPORT_SYMBOL(snd_compat_kstrndup);
#endif
#endif

#ifdef CONFIG_CREATE_WORKQUEUE_FLAGS

#include <linux/workqueue.h>

struct workqueue_struct *snd_compat_create_workqueue2(const char *name)
{
	return create_workqueue(name, 0);
}
EXPORT_SYMBOL(snd_compat_create_workqueue2);

#endif


/*
 * PnP suspend/resume wrapper
 */
#if defined(CONFIG_PNP) && defined(CONFIG_PM)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
#ifndef CONFIG_HAVE_PNP_SUSPEND

#include <linux/pm.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
#include <linux/pm_legacy.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 15) || defined(CONFIG_PM_LEGACY)
#define SUPPORT_PM
#endif

#ifdef SUPPORT_PM
struct snd_pnp_pm_devs {
	void *dev;
	void *driver;
	struct pm_dev *pm;
};

static struct snd_pnp_pm_devs snd_pm_devs[16]; /* FIXME */

static void register_pnp_pm_callback(void *dev, void *driver, pm_callback callback)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(snd_pm_devs); i++) {
		if (snd_pm_devs[i].dev)
			continue;
		snd_pm_devs[i].pm = pm_register(PM_ISA_DEV, 0, callback);
		if (snd_pm_devs[i].pm) {
			snd_pm_devs[i].dev = dev;
			snd_pm_devs[i].driver = driver;
			snd_pm_devs[i].pm->data = &snd_pm_devs[i];
		}
		return;
	}
}

static void unregister_pnp_pm_callback(void *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(snd_pm_devs); i++) {
		if (snd_pm_devs[i].dev == dev) {
			snd_pm_devs[i].dev = NULL;
			snd_pm_devs[i].driver = NULL;
			if (snd_pm_devs[i].pm) {
				pm_unregister(snd_pm_devs[i].pm);
				snd_pm_devs[i].pm = NULL;
			}
			return;
		}
	}
}

static int snd_pnp_dev_pm_callback(struct pm_dev *dev, pm_request_t req, void *data)
{
	struct snd_pnp_pm_devs *pm = dev->data;
	struct pnp_dev *pdev = pm->dev;
	struct snd_pnp_driver *driver = pm->driver;

	switch (req) {
	case PM_SUSPEND:
		driver->suspend(pdev, PMSG_SUSPEND);
		break;
	case PM_RESUME:
		driver->resume(pdev);
		break;
	}
	return 0;
}

static int snd_pnp_dev_probe(struct pnp_dev *dev, const struct pnp_device_id *dev_id)
{
	struct snd_pnp_driver *driver = (struct snd_pnp_driver *)dev->driver;
	int err = driver->probe(dev, dev_id);
	if (err >= 0)
		register_pnp_pm_callback(dev, driver, snd_pnp_dev_pm_callback);
	return err;
}

static void snd_pnp_dev_remove(struct pnp_dev *dev)
{
	struct snd_pnp_driver *driver = (struct snd_pnp_driver *)dev->driver;
	unregister_pnp_pm_callback(dev);
	driver->remove(dev);
}
#endif /* SUPPORT_PM */

#undef pnp_register_driver

int snd_pnp_register_driver(struct snd_pnp_driver *driver)
{
	driver->real_driver.name = driver->name;
	driver->real_driver.id_table = driver->id_table;
	driver->real_driver.flags = driver->flags;
#ifdef SUPPORT_PM
	if (driver->suspend || driver->resume) {
		driver->real_driver.probe = snd_pnp_dev_probe;
		driver->real_driver.remove = snd_pnp_dev_remove;
	} else
#endif
	{
		driver->real_driver.probe = driver->probe;
		driver->real_driver.remove = driver->remove;
	}
	return pnp_register_driver(&driver->real_driver);
}
EXPORT_SYMBOL(snd_pnp_register_driver);

#ifdef SUPPORT_PM
/*
 * for card
 */
static int snd_pnp_card_pm_callback(struct pm_dev *dev, pm_request_t req, void *data)
{
	struct snd_pnp_pm_devs *pm = dev->data;
	struct pnp_card_link *pdev = pm->dev;
	struct snd_pnp_card_driver *driver = pm->driver;

	switch (req) {
	case PM_SUSPEND:
		driver->suspend(pdev, PMSG_SUSPEND);
		break;
	case PM_RESUME:
		driver->resume(pdev);
		break;
	}
	return 0;
}

static int snd_pnp_card_probe(struct pnp_card_link *dev, const struct pnp_card_device_id *dev_id)
{
	struct snd_pnp_card_driver *driver = (struct snd_pnp_card_driver *)dev->driver;
	int err = driver->probe(dev, dev_id);
	if (err >= 0)
		register_pnp_pm_callback(dev, driver, snd_pnp_card_pm_callback);
	return err;
}

static void snd_pnp_card_remove(struct pnp_card_link *dev)
{
	struct snd_pnp_card_driver *driver = (struct snd_pnp_card_driver *)dev->driver;
	unregister_pnp_pm_callback(dev);
	driver->remove(dev);
}
#endif /* SUPPORT_PM */

#undef pnp_register_card_driver

int snd_pnp_register_card_driver(struct snd_pnp_card_driver *driver)
{
	driver->real_driver.name = driver->name;
	driver->real_driver.id_table = driver->id_table;
	driver->real_driver.flags = driver->flags;
#ifdef SUPPORT_PM
	if (driver->suspend || driver->resume) {
		driver->real_driver.probe = snd_pnp_card_probe;
		driver->real_driver.remove = snd_pnp_card_remove;
	} else
#endif
	{
		driver->real_driver.probe = driver->probe;
		driver->real_driver.remove = driver->remove;
	}
	return pnp_register_card_driver(&driver->real_driver);
}
EXPORT_SYMBOL(snd_pnp_register_card_driver);

#endif /* ! CONFIG_HAVE_PNP_SUSPEND */
#endif /* 2.6 */
#endif /* PNP && PM */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 22)
void snd_compat_print_hex_dump_bytes(const char *prefix_str, int prefix_type,
				     const void *buf, size_t len)
{
	size_t off;
	unsigned int i;

	for (off = 0; off < len; off += 16) {
		printk(KERN_DEBUG "%s", prefix_str);
		if (prefix_type == DUMP_PREFIX_OFFSET)
			printk(" %.4x:", (unsigned int)off);
		for (i = 0; i < 16 && off + i < len; ++i)
			printk(" %02x", ((const u8*)buf)[off + i]);
		printk("\n");
	}
}
EXPORT_SYMBOL(snd_compat_print_hex_dump_bytes);
#endif

/* ISA drivers */
#ifdef CONFIG_ISA
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
#include <linux/device.h>
#include <linux/isa.h>
#ifdef HAVE_DUMMY_SND_ISA_WRAPPER
#include <linux/list.h>
#include <linux/err.h>

#define MAX_ISA_NUMS	8

struct isa_platform_link {
	struct platform_driver platform;
	struct isa_driver *isa;
	unsigned int cards;
	struct platform_device *pdevs[MAX_ISA_NUMS];
#ifdef CONFIG_PM
	struct snd_compat_dev_pm_ops pm;
#endif
	struct list_head list;
};
	
static LIST_HEAD(snd_isa_link_list);

static struct isa_platform_link *get_isa_link(struct isa_driver *driver)
{
	struct isa_platform_link *p;
	list_for_each_entry(p, &snd_isa_link_list, list) {
		if (p->isa == driver)
			return p;
	}
	return NULL;
}

static struct isa_platform_link *get_pdev_link(struct platform_device *pdev)
{
	struct platform_driver *pdrv = container_of(pdev->dev.driver,
						    struct platform_driver,
						    real_driver);
	return (struct isa_platform_link *)pdrv;
}

static int snd_isa_platform_probe(struct platform_device *pdev)
{
	struct isa_platform_link *p = get_pdev_link(pdev);
	int n = pdev->id;

	if (!p)
		return -EINVAL;
	p = get_pdev_link(pdev);
	if (p->isa->match)
		if (!p->isa->match(&pdev->dev, n))
			return -EINVAL;
	return p->isa->probe(&pdev->dev, n);
}

static int snd_isa_platform_remove(struct platform_device *pdev)
{
	struct isa_platform_link *p = get_pdev_link(pdev);
	int n = pdev->id;

	if (!p)
		return 0;
	p->isa->remove(&pdev->dev, n);
	return 0;
}

#ifdef CONFIG_PM
static int snd_isa_platform_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct isa_platform_link *p = get_pdev_link(pdev);
	int n = pdev->id;

	if (!p)
		return -EINVAL;
	return p->isa->suspend(dev, n, PMSG_SUSPEND);
}

static int snd_isa_platform_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct isa_platform_link *p = get_pdev_link(pdev);
	int n = pdev->id;

	if (!p)
		return -EINVAL;
	return p->isa->resume(dev, n);
}
#endif

int snd_isa_register_driver(struct isa_driver *driver, unsigned int nums)
{
	int i, cards, err;
	struct isa_platform_link *p;

	if (nums >= MAX_ISA_NUMS)
		return -EINVAL;

	if (get_isa_link(driver))
		return -EBUSY;

	p = kzalloc(sizeof(*p), GFP_KERNEL);
	if (!p)
		return -ENOMEM;
	INIT_LIST_HEAD(&p->list);
	p->isa = driver;
	p->platform.probe = snd_isa_platform_probe;
	p->platform.driver.name = driver->driver.name;
	p->platform.driver.owner = driver->driver.owner;
	if (driver->remove)
		p->platform.remove = snd_isa_platform_remove;
#ifdef CONFIG_PM
	if (driver->suspend)
		p->pm.suspend = snd_isa_platform_suspend;
	if (driver->resume)
		p->pm.resume = snd_isa_platform_resume;
	if (driver->suspend || driver->resume)
		p->platform.driver.pm = &p->pm;
#endif
	err = platform_driver_register(&p->platform);
	if (err < 0) {
		kfree(p);
		return err;
	}
	list_add(&p->list, &snd_isa_link_list);

	cards = 0;
	for (i = 0; i < nums; i++) {
		struct platform_device *device;
		device = platform_device_register_simple((char *)
							 driver->driver.name,
							 i, NULL, 0);
		if (IS_ERR(device))
			continue;
		p->pdevs[i] = device;
		cards++;
	}
	if (!cards) {
		snd_isa_unregister_driver(driver);
		return -ENODEV;
       }
       return 0;
}
EXPORT_SYMBOL(snd_isa_register_driver);

void snd_isa_unregister_driver(struct isa_driver *driver)
{
	struct isa_platform_link *p = get_isa_link(driver);
	int i;

	if (!p)
		return;
	for (i = 0; i < MAX_ISA_NUMS; i++)
		if (p->pdevs[i])
			platform_device_unregister(p->pdevs[i]);
	platform_driver_unregister(&p->platform);
	list_del(&p->list);
	kfree(p);
}
EXPORT_SYMBOL(snd_isa_unregister_driver);

#endif /* HAVE_DUMMY_SND_ISA_WRAPPER */
#endif /* < 2.6.18 */
#endif /* CONFIG_ISA */

/*
 * new pm_ops wrapper for PCI device
 */
#if defined(CONFIG_PCI) && defined(SND_COMPAT_DEV_PM_OPS)
#include <linux/pci.h>
static int snd_pci_suspend(struct pci_dev *pci, pm_message_t state)
{
	struct snd_compat_pci_driver *driver =
		(struct snd_compat_pci_driver *)pci->driver;
	return driver->driver.pm->suspend(&pci->dev);
}

static int snd_pci_resume(struct pci_dev *pci)
{
	struct snd_compat_pci_driver *driver =
		(struct snd_compat_pci_driver *)pci->driver;
	return driver->driver.pm->resume(&pci->dev);
}

int snd_compat_pci_register_driver(struct snd_compat_pci_driver *driver)
{
	driver->real_driver.name = driver->name;
	driver->real_driver.id_table = driver->id_table;
	driver->real_driver.probe = driver->probe;
	driver->real_driver.remove = driver->remove;
	driver->real_driver.shutdown = driver->shutdown;
	if (driver->driver.pm) {
		if (driver->driver.pm->suspend)
			driver->real_driver.suspend = snd_pci_suspend;
		if (driver->driver.pm->resume)
			driver->real_driver.resume = snd_pci_resume;
	}
	return snd_orig_pci_register_driver(&driver->real_driver);
}
EXPORT_SYMBOL(snd_compat_pci_register_driver);
#endif /* CONFIG_PCI && SND_COMPAT_DEV_PM_OPS */

/*
 * new pm_ops wrapper for platform_device (>= 2.6.15)
 */
#if !defined(SND_COMPAT_PLATFORM_DEVICE) && defined(SND_COMPAT_DEV_PM_OPS)
static int snd_platform_suspend(struct platform_device *dev, pm_message_t state)
{
	struct platform_driver *drv =
		(struct platform_driver *)dev->dev.driver;
	return drv->driver.pm->suspend(&dev->dev);
}

static int snd_platform_resume(struct platform_device *dev)
{
	struct platform_driver *drv =
		(struct platform_driver *)dev->dev.driver;
	return drv->driver.pm->resume(&dev->dev);
}

int snd_compat_platform_driver_register(struct snd_compat_platform_driver *driver)
{
	driver->real_driver.driver.name = driver->driver.name;
	driver->real_driver.driver.owner = driver->driver.owner;
	driver->real_driver.probe = driver->probe;
	driver->real_driver.remove = driver->remove;
	driver->real_driver.shutdown = driver->shutdown;
	if (driver->driver.pm) {
		if (driver->driver.pm->suspend)
			driver->real_driver.suspend = snd_platform_suspend;
		if (driver->driver.pm->resume)
			driver->real_driver.resume = snd_platform_resume;
	}
	return snd_orig_platform_driver_register(&driver->real_driver);
}
EXPORT_SYMBOL(snd_compat_platform_driver_register);
#endif /* !SND_COMPAT_PLATFORM_DEVICE && SND_COMPAT_DEV_PM_OPS */

/*
 * platfrom_device wrapper for older (<= 2.6.14) kernels
 */
#ifdef SND_COMPAT_PLATFORM_DEVICE
static int snd_platform_probe(struct device *dev)
{
	struct platform_driver *drv;
	drv = (struct platform_driver *)dev->driver;
	return drv->probe(to_platform_device(dev));
}

static int snd_platform_remove(struct device *dev)
{
	struct platform_driver *drv;
	drv = (struct platform_driver *)dev->driver;
	return drv->remove(to_platform_device(dev));
}

static int snd_platform_suspend(struct device *dev, pm_message_t state
#ifdef CONFIG_SND_OLD_DRIVER_SUSPEND
					, u32 level
#endif
					)
{
	struct platform_driver *drv =
		(struct platform_driver *)dev->dev.driver;
	return drv->driver.pm->suspend(dev);
}

static int snd_platform_resume(struct platform_device *dev)
{
	struct platform_driver *drv =
		(struct platform_driver *)dev->dev.driver;
	return drv->driver.pm->resume(dev);
}

#undef platform_driver_register

int platform_driver_register(struct platform_driver *driver)
{
	driver->real_driver.bus = &platform_bus_type;
	driver->real_driver.name = driver->driver.name;
	driver->real_driver.owner = driver->driver.owner;
	if (driver->driver.probe)
		driver->real_driver.probe = snd_platform_probe;
	if (driver->driver.remove)
		driver->real_driver.remove = snd_platform_remove;
	if (driver->driver.shutdown)
		driver->real_driver.shutdown = snd_platform_shutdown;
	if (driver->driver.pm) {
		if (driver->dev.pm->suspend)
			driver->real_driver.suspend = snd_platform_suspend;
		if (driver->dev.pm->resume)
			driver->real_driver.resume = snd_platform_resume;
	}
	return driver_register(&driver->real_driver);
}
EXPORT_SYMBOL(platform_driver_register);
#endif /* SND_COMPAT_PLATFORM_DEVICE */
