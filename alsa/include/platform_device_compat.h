#ifndef __PLATFORM_DEVICE_COMPAT_H
#define __PLATFORM_DEVICE_COMPAT_H

#include <linux/device.h>

#define SND_COMPAT_PLATFORM_DEVICE

struct platform_driver {
	struct device_driver real_driver;
	int (*probe)(struct platform_device *);
	int (*remove)(struct platform_device *);
	void (*shutdown)(struct platform_device *);
	struct snd_compat_dev_pm_driver driver;
};

int platform_driver_register(struct platform_driver *driver);
static inline void platform_driver_unregister(struct platform_driver *drv)
{
	driver_unregister(&drv->real_driver);
}

#define platform_get_drvdata(_dev)	dev_get_drvdata(&(_dev)->dev)
#define platform_set_drvdata(_dev,data)	dev_set_drvdata(&(_dev)->dev, (data))

#endif
