#include "config.h"
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

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,15)
#include "vxpocket_old.c"
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,16)
#include "vxpocket-2.6.16.c"
#else
#include "adriver.h"
#include "../../alsa-kernel/pcmcia/vx/vxpocket.c"
#endif

