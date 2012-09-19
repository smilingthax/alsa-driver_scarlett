#include "config.h"
#include <linux/version.h>
#ifdef CONFIG_HAVE_DEPRECATED_CONFIG_H
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33)
#include <generated/autoconf.h>
#else
#include <linux/autoconf.h>
#endif
#else
#include <linux/config.h>
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,15)
#include "pdaudiocf_old.c"
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,16)
#include "pdaudiocf-2.6.16.c"
#else
#include "adriver.h"
#include "../../alsa-kernel/pcmcia/pdaudiocf/pdaudiocf.c"
#endif

