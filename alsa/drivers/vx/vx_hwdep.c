#include "config.h"
#define __NO_VERSION__
/* to be in alsa-driver-specfici code */
#include <linux/version.h>
#ifdef CONFIG_HAVE_DEPRECATED_CONFIG_H
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33)
#include <generated/autoconf.h>
#else
#include <linux/autoconf.h>
#endif
#else
#include <linux/config.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#define CONFIG_USE_VXLOADER
#endif
#include "adriver.h"
#include "../../alsa-kernel/drivers/vx/vx_hwdep.c"
