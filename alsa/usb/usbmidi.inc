#include <linux/config.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,5)
#define SND_NEED_USB_SET_INTERFACE
#endif
// #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#define SND_NEED_USB_WRAPPER
// #endif
#include <sound/driver.h>
#include <linux/usb.h>

#ifdef OLD_USB
#define snd_usb_complete_callback(x) __old_ ## x
static void __old_snd_usbmidi_in_urb_complete(struct urb* urb);
static void __old_snd_usbmidi_in_midiman_complete(struct urb* urb);
static void __old_snd_usbmidi_out_urb_complete(struct urb* urb);
#endif

// vim: ft=c
