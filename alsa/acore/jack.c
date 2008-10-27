#define __NO_VERSION__
#include "adriver.h"
#include <linux/input.h>
#ifndef SW_LINEOUT_INSERT
#define SW_LINEOUT_INSERT	0x06  /* set = inserted */
#endif
#include "../alsa-kernel/core/jack.c"
