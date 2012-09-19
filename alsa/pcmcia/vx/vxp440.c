#include <linux/config.h>
#include <linux/version.h>

#if defined(CONFIG_MODVERSIONS) && !defined(__GENKSYMS__) && !defined(__DEPEND__)
#define MODVERSIONS
#include <linux/modversions.h>
#include "sndversions.h"
#endif

#include "config.h"
#include "adriver.h"
#include "compat_cs.h"

#include "../../alsa-kernel/pcmcia/vx/vxp440.c"
EXPORT_NO_SYMBOLS;
