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

EXPORT_SYMBOL(snd_usb_create_midi_interface);
EXPORT_SYMBOL(snd_usbmidi_input_stop);
EXPORT_SYMBOL(snd_usbmidi_input_start);
EXPORT_SYMBOL(snd_usbmidi_disconnect);
