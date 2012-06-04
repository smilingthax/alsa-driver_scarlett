#include "alsa-autoconf.h"
#include "adriver.h"
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 0)
#include "tea575x-tuner-3.3.c"
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 5, 0)
#define v4l2_disable_ioctl(x, y) /* NOP */
#endif
#include "../../alsa-kernel/i2c/other/tea575x-tuner.c"
#endif
