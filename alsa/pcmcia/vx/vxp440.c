#include <linux/config.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
#if defined(CONFIG_MODVERSIONS) && !defined(__GENKSYMS__) && !defined(__DEPEND__)
#define MODVERSIONS
#include <linux/modversions.h>
#include "sndversions.h"
#endif
#endif

#include "config.h"
#include "adriver.h"
#include "compat_cs.h"

#include "../../alsa-kernel/pcmcia/vx/vxp440.c"
EXPORT_NO_SYMBOLS;
