#include "config.h"
#define __NO_VERSION__
/* to be in alsa-driver-specfici code */
#ifdef CONFIG_HAVE_DEPRECATED_CONFIG_H
#include <linux/autoconf.h>
#else
#include <linux/config.h>
#endif
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#define CONFIG_USE_VXLOADER
#endif
#include "adriver.h"
#include "../../alsa-kernel/drivers/vx/vx_hwdep.c"
