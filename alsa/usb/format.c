#define __NO_VERSION__
#include "usbaudio.inc"

#include <linux/usb.h>
#include <linux/usb/audio.h>
#include <linux/usb/audio-v2.h>
#ifndef UAC2_FORMAT_TYPE_I_RAW_DATA
#define UAC2_FORMAT_TYPE_I_RAW_DATA	(1 << 31)
#endif

#include "../alsa-kernel/usb/format.c"
