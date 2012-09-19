#include "config.h"
#define __NO_VERSION__
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
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
#include <linux/compiler.h>
#ifndef __iomem
#define __iomem
#endif
#ifndef __user
#define __user
#endif
#ifndef __kernel
#define __kernel
#endif
#ifndef __nocast
#define __nocast
#endif
#ifndef __force
#define __force
#endif
#ifndef __safe
#define __safe
#endif
#ifndef __bitwise
#define __bitwise
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#include "adriver.h"
#endif /* KERNEL < 2.6.0 */

#include "../alsa-kernel/core/memory.c"
