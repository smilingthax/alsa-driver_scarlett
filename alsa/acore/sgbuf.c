#include "alsa-autoconf.h"
#define __NO_VERSION__

#define SKIP_HIDDEN_MALLOCS
#include "adriver.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
#define CONFIG_HAS_DMA 1
#endif
#include "../alsa-kernel/core/sgbuf.c"
