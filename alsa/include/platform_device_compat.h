#ifndef __PLATFORM_DEVICE_COMPAT_H
#define __PLATFORM_DEVICE_COMPAT_H

#include <linux/device.h>

struct platform_driver {
	struct device_driver driver;
	int (*probe)(struct platform_device *);
	int (*remove)(struct platform_device *);
	void (*shutdown)(struct platform_device *);
	int (*suspend)(struct platform_device *, pm_message_t state);
	int (*resume)(struct platform_device *);
};

static int snd_platform_driver_probe(struct device *dev)
{
	struct platform_driver *drv;
	drv = (struct platform_driver *)dev->driver;
	return drv->probe(to_platform_device(dev));
}

static int snd_platform_driver_remove(struct device *dev)
{
	struct platform_driver *drv;
	drv = (struct platform_driver *)dev->driver;
	return drv->remove(to_platform_device(dev));
}

static int snd_platform_driver_suspend(struct device *dev, pm_message_t state
#ifdef CONFIG_SND_OLD_DRIVER_SUSPEND
				       , u32 level
#endif
				       )
{
	struct platform_driver *drv;
#ifdef CONFIG_SND_OLD_DRIVER_SUSPEND
	if (level != SUSPEND_DISABLE)
		return 0;
#endif
	drv = (struct platform_driver *)dev->driver;
	return drv->suspend(to_platform_device(dev), state);
}

static int snd_platform_driver_resume(struct device *dev
#ifdef CONFIG_SND_OLD_DRIVER_SUSPEND
				      , u32 level
#endif
				      )
{
	struct platform_driver *drv;
#ifdef CONFIG_SND_OLD_DRIVER_SUSPEND
	if (level != RESUME_ENABLE)
		return 0;
#endif
	drv = (struct platform_driver *)dev->driver;
	return drv->resume(to_platform_device(dev));
}

static inline int platform_driver_register(struct platform_driver *drv)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
	drv->driver.bus = &platform_bus_type;
#endif
	if (drv->probe)
		drv->driver.probe = snd_platform_driver_probe;
	if (drv->remove)
		drv->driver.remove = snd_platform_driver_remove;
	if (drv->suspend)
		drv->driver.suspend = snd_platform_driver_suspend;
	if (drv->resume)
		drv->driver.resume = snd_platform_driver_resume;
	return driver_register(&drv->driver);
}

static inline void platform_driver_unregister(struct platform_driver *drv)
{
	driver_unregister(&drv->driver);
}

#define platform_get_drvdata(_dev)	dev_get_drvdata(&(_dev)->dev)
#define platform_set_drvdata(_dev,data)	dev_set_drvdata(&(_dev)->dev, (data))

#endif
