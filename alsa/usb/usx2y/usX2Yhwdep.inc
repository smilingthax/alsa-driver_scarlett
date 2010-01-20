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

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,5)
#define SND_NEED_USB_SET_INTERFACE
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#define SND_NEED_USB_WRAPPER
#endif
#define __NO_VERSION__
#endif
#include "adriver.h"
#include "../usbcompat.h"
