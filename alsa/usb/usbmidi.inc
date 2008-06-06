#include "config.h"
#ifdef CONFIG_HAVE_DEPRECATED_CONFIG_H
#include <linux/autoconf.h>
#else
#include <linux/config.h>
#endif
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,5)
#define SND_NEED_USB_SET_INTERFACE
#endif
// #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#define SND_NEED_USB_WRAPPER
// #endif
#include "adriver.h"

// vim: ft=c
