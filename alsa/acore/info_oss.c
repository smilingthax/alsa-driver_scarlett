#define __NO_VERSION__
#include <linux/version.h>
#ifdef CONFIG_HAVE_INIT_UTSNAME
#define init_utsname()	(&system_utsname)
#endif
#include "adriver.h"
#include "../alsa-kernel/core/info_oss.c"
