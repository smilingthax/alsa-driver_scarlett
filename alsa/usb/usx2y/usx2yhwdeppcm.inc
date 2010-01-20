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

#define USB_2_6_18 (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18))
#if USB_2_6_18
#define USX2Y_URB_FRAME_MASK -1
#else
#define USX2Y_URB_FRAME_MASK 0xffff
#endif

#include "adriver.h"
#include "../usbcompat.h"
