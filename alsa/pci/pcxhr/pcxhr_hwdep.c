#define __NO_VERSION__
/* to be in alsa-driver-specfici code */
#include <linux/config.h>
#include <linux/version.h>
/* should be in alsa-driver tree only */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#define CONFIG_USE_PCXHRLOADER
#endif
#include "../../alsa-kernel/pci/pcxhr/pcxhr_hwdep.c"
