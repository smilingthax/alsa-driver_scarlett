#define SND_NEED_USB_WRAPPER
#include <sound/driver.h>
#include <linux/usb.h>

#ifdef OLD_USB
#define snd_usb_complete_callback(x) __old_ ## x
static void __old_snd_complete_urb(struct urb *urb);
static void __old_snd_complete_sync_urb(struct urb *urb);

static void * usb_audio_probe(struct usb_device *dev, unsigned int ifnum,
                              const struct usb_device_id *id);
static void usb_audio_disconnect(struct usb_device *dev, void *ptr);
#endif
