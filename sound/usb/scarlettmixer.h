#ifndef __USBSCARLETTMIXER_H
#define __USBSCARLETTMIXER_H

int scarlett_mixer_create(struct snd_usb_audio *chip,
				       struct usb_interface *iface,
				       struct usb_driver *driver,
				       const struct snd_usb_audio_quirk *quirk);

#endif /* __USBSCARLETTMIXER_H */
