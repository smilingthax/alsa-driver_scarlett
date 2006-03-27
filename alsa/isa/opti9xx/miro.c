#include <linux/version.h>

#include "../../alsa-kernel/isa/opti9xx/opti92x-ad1848.c"
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#ifndef __isapnp_now__
#include "miro.isapnp"
#endif
EXPORT_NO_SYMBOLS;
#endif
