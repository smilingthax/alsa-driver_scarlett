#include "alsa-autoconf.h"
#include "adriver.h"
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 0)
#include "tea575x-tuner-3.3.c"
#elif LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 5, 0)
#define v4l2_disable_ioctl(x, y) /* NOP */
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)
#define V4L2_TUNER_CAP_HWSEEK_BOUNDED 0
#endif
#include "tea575x-tuner-3.6.c"
#else
#include "../../alsa-kernel/i2c/other/tea575x-tuner.c"
#endif
