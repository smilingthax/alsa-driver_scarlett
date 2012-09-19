#include "config.h"
#ifdef CONFIG_HAVE_DEPRECATED_CONFIG_H
#include <linux/autoconf.h>
#else
#include <linux/config.h>
#endif
#include <linux/version.h>

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,15)
#include "pdaudiocf_old.c"
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,16)
#include "pdaudiocf-2.6.16.c"
#else
#include "adriver.h"
#include "../../alsa-kernel/pcmcia/pdaudiocf/pdaudiocf.c"
#endif

