#define SND_NEED_USB_WRAPPER
#define __NO_VERSION__
#include <sound/driver.h>
#include <linux/usb.h>

#ifdef OLD_USB
#define snd_usb_complete_callback(x) __old_ ## x
static void __old_snd_usbmidi_in_urb_complete(struct urb* urb);
static void __old_snd_usbmidi_in_midiman_complete(struct urb* urb);
static void __old_snd_usbmidi_out_urb_complete(struct urb* urb);
#endif

#include "../alsa-kernel/usb/usbmidi.c"

#ifdef OLD_USB
static void __old_snd_usbmidi_in_urb_complete(struct urb* urb)
{
	snd_usbmidi_in_urb_complete(urb, NULL);
}

static void __old_snd_usbmidi_in_midiman_complete(struct urb* urb)
{
	snd_usbmidi_in_midiman_complete(urb, NULL);
}

static void __old_snd_usbmidi_out_urb_complete(struct urb* urb)
{
	snd_usbmidi_out_urb_complete(urb, NULL);
}
#endif
