#include <linux/config.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,5)
#define SND_NEED_USB_SET_INTERFACE
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#define SND_NEED_USB_WRAPPER
#endif
#include <sound/driver.h>
#include <linux/usb.h>
#endif

#ifdef OLD_USB
#define snd_usb_complete_callback(x) __old_ ## x
static void __old_snd_complete_urb(struct urb *urb);
static void __old_snd_complete_sync_urb(struct urb *urb);

static void * usb_audio_probe(struct usb_device *dev, unsigned int ifnum,
                              const struct usb_device_id *id);
static void usb_audio_disconnect(struct usb_device *dev, void *ptr);
static int usb_reset_configuration(struct usb_device *dev);
#endif
