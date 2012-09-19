#include "config.h"
/* to be in alsa-driver-specfici code */
#ifdef CONFIG_HAVE_DEPRECATED_CONFIG_H
#include <linux/autoconf.h>
#else
#include <linux/config.h>
#endif
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
#define spin_lock_bh spin_lock
#define spin_unlock_bh spin_unlock
#endif

#include "adriver.h"
#include "../../alsa-kernel/drivers/vx/vx_core.c"
