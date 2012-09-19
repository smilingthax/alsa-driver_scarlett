#define __NO_VERSION__
#include <linux/config.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
#include "pmac_old.c"
#else
#include "../alsa-kernel/ppc/pmac.c"
#endif
