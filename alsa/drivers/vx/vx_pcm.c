#include "config.h"
#define __NO_VERSION__
/* to be in alsa-driver-specfici code */
#ifdef CONFIG_HAVE_DEPRECATED_CONFIG_H
#include <linux/autoconf.h>
#else
#include <linux/config.h>
#endif
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 2, 18)
#define vmalloc_32(x) vmalloc_nocheck(x)
#endif

#include "adriver.h"
#include "../../alsa-kernel/drivers/vx/vx_pcm.c"

