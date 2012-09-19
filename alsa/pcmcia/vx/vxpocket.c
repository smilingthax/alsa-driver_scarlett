#include <linux/config.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,15)
#include "vxpocket_old.c"
#else
#include "../../alsa-kernel/pcmcia/vx/vxpocket.c"
#endif

