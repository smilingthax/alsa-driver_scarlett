#include <linux/config.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#define SND_NEED_USB_WRAPPER
#define __NO_VERSION__
#endif
#include "../alsa-kernel/usb/usbmixer.c"
