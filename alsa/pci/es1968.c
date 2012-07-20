#include "adriver.h"
/* dummy v4l2_device definition */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 29)
struct v4l2_device {
	int dummy;
};
#define v4l2_file_operations	file_operations
#define v4l2_device_register(x, y) 0
#define v4l2_device_unregister(x)
#endif
#include "../alsa-kernel/pci/es1968.c"
EXPORT_NO_SYMBOLS;
