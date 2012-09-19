#include <linux/config.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
#define PMAC_SUPPORT_PCM_BEEP
#endif

#include "../alsa-kernel/ppc/powermac.c"
EXPORT_NO_SYMBOLS;
