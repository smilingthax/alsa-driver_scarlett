#define __NO_VERSION__
#include <linux/config.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
#define CONFIG_ADB_CUDA
#define CONFIG_ADB_PMU
#endif
#include "../alsa-kernel/ppc/burgundy.c"
