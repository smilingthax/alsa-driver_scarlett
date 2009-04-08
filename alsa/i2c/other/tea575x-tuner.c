#include "config.h"
#ifdef CONFIG_HAVE_DEPRECATED_CONFIG_H
#include <linux/autoconf.h>
#else
#include <linux/config.h>
#endif
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30)
#include "adriver.h"
#include "../../alsa-kernel/i2c/other/tea575x-tuner.c"
#else
#include "tea575x-tuner-old.h"
#include "tea575x-tuner-old.c"
#endif
