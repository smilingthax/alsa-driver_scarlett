#include "alsa-autoconf.h"

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,15)
#include "vxpocket_old.c"
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,16)
#include "vxpocket-2.6.16.c"
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,34)
#include "vxpocket-2.6.34.c"
#else
#include "adriver.h"
#include "../../alsa-kernel/pcmcia/vx/vxpocket.c"
#endif

