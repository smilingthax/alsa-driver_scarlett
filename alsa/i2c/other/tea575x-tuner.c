#include "alsa-autoconf.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 1, 0)
#include "adriver.h"
#include "../../alsa-kernel/i2c/other/tea575x-tuner.c"
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30)
#include "tea575x-tuner-3.0.c"
#else
#include "tea575x-tuner-old.c"
#endif
