#include "ppc-prom-hack.h"
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
#undef platform_device
#define platform_device		of_device
#endif
