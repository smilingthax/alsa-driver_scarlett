#define __NO_VERSION__
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
#define init_utsname()	(&system_utsname)
#endif
#include "adriver.h"
#include "../alsa-kernel/core/info_oss.c"
