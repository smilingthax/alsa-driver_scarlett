#include <linux/config.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,5)
#define SND_NEED_USB_SET_INTERFACE
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#define SND_NEED_USB_WRAPPER
#endif
#define __NO_VERSION__
#include <sound/driver.h>
#include <linux/usb.h>
#endif

#ifdef OLD_USB
#define snd_usb_complete_callback(x) __old_ ## x
static void __old_i_usX2Y_urb_complete(struct urb *urb);
#endif

#include "../../alsa-kernel/usb/usx2y/usbusx2yaudio.c"

/*
 * compatible layers
 */
#ifdef OLD_USB
static void __old_i_usX2Y_urb_complete(struct urb *urb)
{
	i_usX2Y_urb_complete(urb, NULL);
}
#endif
