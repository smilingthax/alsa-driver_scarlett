#include "alsa-autoconf.h"
#include "adriver.h"
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 0)
#include "tea575x-tuner-3.3.c"
#else
#include "../../alsa-kernel/i2c/other/tea575x-tuner.c"
#endif
