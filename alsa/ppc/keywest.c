#define __NO_VERSION__
#include <linux/config.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
#define CONFIG_ADB_CUDA
#define CONFIG_ADB_PMU
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0) /* FIXME: which version exactly? */
#define i2c_device_name(x)	((x)->name)
#define i2c_set_clientdata(x,p)	((x)->data = (p))
#endif

#include "../alsa-kernel/ppc/keywest.c"
