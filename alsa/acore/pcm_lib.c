#define __NO_VERSION__
#include <sound/driver.h>

#ifndef CONFIG_HAVE_DUMP_STACK
#define dump_stack()
#endif

#include "../alsa-kernel/core/pcm_lib.c"
