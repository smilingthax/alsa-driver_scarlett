#include "config.h"
#ifdef CONFIG_HAVE_DEPRECATED_CONFIG_H
#include <linux/autoconf.h>
#else
#include <linux/config.h>
#endif
#include <linux/version.h>

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,29)
#include "tea575x-tuner-old.c"
#else
#include "adriver.h"
#include "../../alsa-kernel/sound/i2c/other/tea575a-tuner.c"
#endif
