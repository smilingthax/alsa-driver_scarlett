#define __NO_VERSION__
#include "adriver.h"
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
#define CONFIG_ADB_CUDA
#define CONFIG_ADB_PMU
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 34)
#define of_machine_is_compatible	machine_is_compatible
#endif
#include "../alsa-kernel/ppc/burgundy.c"
