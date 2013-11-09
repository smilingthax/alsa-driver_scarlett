/*
 *   (Tentative) Scarlett 18i6 Driver for ALSA
 *
 *   Copyright (c) 2013 by Tobias Hoffmann
 *   Copyright (c) 2013 by Robin Gareus <robin@gareus.org>
 *   Copyright (c) 2002 by Takashi Iwai <tiwai@suse.de>
 *
 *   Many codes borrowed from audio.c by
 *	    Alan Cox (alan@lxorguk.ukuu.org.uk)
 *	    Thomas Sailer (sailer@ife.ee.ethz.ch)
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

/*
 * Rewritten and extended to support more models, e.g. Scarlett 18i8.
 * TODO? reset_first/ channel init
 * TODO... test meter?
 */

/*
 * Many features of Scarlett 18i6 do not lend themselves to be implemented
 * as simple mixer-quirk -- or at least I don't see a way how to do that, yet.
 * Hence the top parts of this file is a 1:1 copy of select static functions
 * from mixer.c to implement the interface.
 * Suggestions how to avoid this code duplication are very welcome.
 *
 * eventually this should either be integrated as quirk into mixer_quirks.c
 * or become a standalone module.
 *
 * This source hardcodes the URBs for the Scarlett,
 * Auto-detection via UAC2 is not feasible to properly discover the vast majority
 * of features. It's related to both Linux/ALSA's UAC2 as well as Focusrite's
 * implementation of it. Eventually quirks may be sufficient but right now
 * it's a major headache to work arount these things.
 *
 * NB. Neither the OSX nor the win driver provided by Focusrite performs
 * discovery, they seem to operate the same as this driver.
 */

/* Mixer Interface for the Focusrite Scarlett 18i6 audio interface.
 *
 * The protocol was reverse engineered by looking at communication between
 * Scarlett MixControl (v 1.2.128.0) and the Focusrite(R) Scarlett 18i6 (firmware
 * v305) using wireshark and usbmon in January 2013.
 * Extended in July 2013.
 *
 * this mixer gives complete access to all features of the device:
 *  - change Impedance of inputs (Line-in, Mic / Instrument, Hi-Z)
 *  - select clock source
 *  - dynamic input to mixer-matrix assignment
 *  - 18 x 6 mixer-matrix gain stages
 *  - bus routing & volume control
 *  - save setting to hardware
 *  - automatic re-initialization on connect if device was power-cycled
 *  - peak monitoring of all 3 buses (18 input, 6 DAW input, 6 route chanels)
 *  (changing the samplerate and buffersize is supported by the PCM interface)
 *
 *
 * USB URB commands overview (bRequest = 0x01 = UAC2_CS_CUR)
 * wIndex
 * 0x01 Analog Input line/instrument impedance switch, wValue=0x0901 + channel, data=Line/Inst (2bytes)
 *      pad (-10dB) switch, wValue=0x0b01 + channel, data=Off/On (2bytes)
 *      ?? wValue=0x0803/04, ?? (2bytes)
 * 0x0a Master Volume, wValue=0x0200+bus[0:all + only 1..4?] data(2bytes)
 *      Bus Mute/Unmute wValue=0x0100+bus[0:all + only 1..4?], data(2bytes)
 * 0x28 Clock source, wValue=0x0100, data={1:int,2:spdif,3:adat} (1byte)
 * 0x29 Set Sample-rate, wValue=0x0100, data=sample-rate(4bytes)
 * 0x32 Mixer mux, wValue=0x0600 + mixer-channel, data=input-to-connect(2bytes)
 * 0x33 Output mux, wValue=bus, data=input-to-connect(2bytes)
 * 0x34 Capture mux, wValue=0...18, data=input-to-connect(2bytes)
 * 0x3c Matrix Mixer gains, wValue=mixer-node  data=gain(2bytes)
 *      ?? [sometimes](4bytes, e.g 0x000003be 0x000003bf ...03ff)
 *
 * USB reads: (i.e. actually issued by original software)
 * 0x01 wValue=0x0901+channel (1byte!!), wValue=0x0b01+channed (1byte!!)
 * 0x29 wValue=0x0100 sample-rate(4bytes)
 *      wValue=0x0200 ?? 1byte (only once)
 * 0x2a wValue=0x0100 ?? 4bytes, sample-rate2 ??
 *
 * USB reads with bRequest = 0x03 = UAC2_CS_MEM
 * 0x3c wValue=0x0002 1byte: sync status (locked=1)
 *      wValue=0x0000 18*2byte: peak meter (inputs)
 *      wValue=0x0001 8(?)*2byte: peak meter (mix)
 *      wValue=0x0003 6*2byte: peak meter (pcm/daw)
 *
 * USB write with bRequest = 0x03
 * 0x3c Save settings to hardware: wValue=0x005a, data=0xa5
 *
 *
 * <ditaa>
 *  /--------------\    18chn            6chn    /--------------\
 *  | Hardware  in +--+-------\        /------+--+ ALSA PCM out |
 *  \--------------/  |       |        |      |  \--------------/
 *                    |       |        |      |
 *                    |       v        v      |
 *                    |   +---------------+   |
 *                    |    \ Matrix  Mux /    |
 *                    |     +-----+-----+     |
 *                    |           |           |
 *                    |           | 18chn     |
 *                    |           v           |
 *                    |     +-----------+     |
 *                    |     | Mixer     |     |
 *                    |     |    Matrix |     |
 *                    |     |           |     |
 *                    |     | 18x6 Gain |     |
 *                    |     |   stages  |     |
 *                    |     +-----+-----+     |
 *                    |           |           |
 *                    |           |           |
 *                    | 18chn     | 6chn      | 6chn
 *                    v           v           v
 *                    =========================
 *             +---------------+     +--—------------+
 *              \ Output  Mux /       \ Capture Mux /
 *               +-----+-----+         +-----+-----+
 *                     |                     |
 *                     | 6chn                |
 *                     v                     |
 *              +-------------+              |
 *              | Master Gain |              |
 *              +------+------+              |
 *                     |                     |
 *                     | 6chn                | 18chn
 *                     | (3 stereo pairs)    |
 *  /--------------\   |                     |   /--------------\
 *  | Hardware out |<--/                     \-->| ALSA PCM  in |
 *  \--------------/                             \--------------/
 * </ditaa>
 *
 */

#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/usb/audio-v2.h>

#include <sound/core.h>
#include <sound/control.h>
#include <sound/tlv.h>

#include "usbaudio.h"
#include "mixer.h"
#include "helper.h"
#include "power.h"

#include "scarlettmixer.h"

//#define WITH_METER
////#define WITH_LOGSCALEMETER

#define LEVEL_BIAS 128  /* some gui mixers can't handle negative ctl values (alsamixergui, qasmixer, ...) */

#ifndef LEVEL_BIAS
	#define LEVEL_BIAS 0
#endif

struct scarlett_enum_info {
	int start, len;
	const char **texts;
};

struct scarlett_device_info {
	int matrix_in;
	int matrix_out;
	int input_len;
	int output_len;

	int pcm_start;
	int analog_start;
	int spdif_start;
	int adat_start;
	int mix_start;

	struct scarlett_enum_info opt_master;
	struct scarlett_enum_info opt_matrix;

	int (*controls_fn)(struct usb_mixer_interface *mixer,
	                   const struct scarlett_device_info *info);

	int matrix_mux_init[];
};

struct scarlett_mixer_elem_info {
	struct usb_mixer_interface *mixer;

	/* URB command details */
	int wValue, index;
	int val_len;

	int count; /* number of channels, using ++wValue */

	const struct scarlett_enum_info *opt;

	int cached;
	int cache_val[MAX_CHANNELS];
};

static void scarlett_mixer_elem_free(struct snd_kcontrol *kctl)
{
	kfree(kctl->private_data);
	kctl->private_data = NULL;
}

/***************************** Low Level USB I/O *****************************/

// stripped down/adapted from get_ctl_value_v2
static int get_ctl_urb2(struct snd_usb_audio *chip,
		int bRequest, int wValue, int index,
		unsigned char *buf, int size)
{
	int ret, idx = 0;
	
	ret = snd_usb_autoresume(chip);
	if (ret < 0 && ret != -ENODEV) {
		ret = -EIO;
		goto error;
	}

	down_read(&chip->shutdown_rwsem);
	if (chip->shutdown) {
		ret = -ENODEV;
	} else {
		idx = snd_usb_ctrl_intf(chip) | (index << 8);
		ret = snd_usb_ctl_msg(chip->dev,
		                      usb_rcvctrlpipe(chip->dev, 0),
		                      bRequest,
		                      USB_RECIP_INTERFACE | USB_TYPE_CLASS | USB_DIR_IN,
		                      wValue, idx, buf, size);
	}
	up_read(&chip->shutdown_rwsem);
	snd_usb_autosuspend(chip);

	if (ret < 0) {
error:
		snd_printk(KERN_ERR "cannot get ctl value: req = %#x, wValue = %#x, wIndex = %#x, size = %d\n",
		           bRequest, wValue, idx, size);
		return ret;
	}
#if 0 /* rg debug XXX */
	snd_printk(KERN_ERR "req ctl value: req = %#x, rtype = %#x, wValue = %#x, wIndex = %#x, size = %d\n",
	           bRequest, (USB_RECIP_INTERFACE | USB_TYPE_CLASS | USB_DIR_IN), wValue, idx, size);
#endif
	return 0;
}

// adopted from snd_usb_mixer_set_ctl_value
static int set_ctl_urb2(struct snd_usb_audio *chip,
		int request, int wValue, int index,
		unsigned char *buf, int val_len)
{
	int idx = 0, err, timeout = 10;
	err = snd_usb_autoresume(chip);
	if (err < 0 && err != -ENODEV)
		return -EIO;
	down_read(&chip->shutdown_rwsem);
	while (timeout-- > 0) {
		if (chip->shutdown)
			break;
		idx = snd_usb_ctrl_intf(chip) | (index << 8);
		if (snd_usb_ctl_msg(chip->dev,
				    usb_sndctrlpipe(chip->dev, 0), request,
				    USB_RECIP_INTERFACE | USB_TYPE_CLASS | USB_DIR_OUT,
				    wValue, idx, buf, val_len) >= 0) {
			err = 0;
			goto out;
		}
	}
	snd_printdd(KERN_ERR "cannot set ctl value: req = %#x, wValue = %#x, wIndex = %#x, len = %d, data = %#x/%#x\n",
		    request, wValue, idx, val_len, buf[0], buf[1]);
	err = -EINVAL;

 out:
	up_read(&chip->shutdown_rwsem);
	snd_usb_autosuspend(chip);
#if 0 /* rg debug XXX */
	snd_printk(KERN_ERR "set ctl value: req = %#x, wValue = %#x, wIndex = %#x, len = %d, data = %#x/%#x\n",
		    request, wValue, idx, val_len, buf[0], buf[1]);
#endif
	return err;
}

/***************************** High Level USB *****************************/

static int set_ctl_value(struct scarlett_mixer_elem_info *elem, int channel, int value)
{
	struct snd_usb_audio *chip = elem->mixer->chip;
	unsigned char buf[2];
	int err;

	if (elem->val_len == 2) { /* S16 */
		buf[0] = value & 0xff;
		buf[1] = (value >> 8) & 0xff;
	} else { /* U8 */
		buf[0] = value & 0xff;
	}

	err = set_ctl_urb2(chip, UAC2_CS_CUR, elem->wValue + channel, elem->index, buf, elem->val_len);
	if (err < 0)
		return err;

	elem->cached |= 1 << channel;
	elem->cache_val[channel] = value;
	return 0;
}

/*
  TODO: can't read back any volume (master/mixer), only cache works [?]
    [return 0xfe for enums???]
*/
static int get_ctl_value(struct scarlett_mixer_elem_info *elem, int channel, int *value)
{
	struct snd_usb_audio *chip = elem->mixer->chip;
	unsigned char buf[2] = {0, 0};
	int err, val_len;

	if (elem->cached & (1 << channel)) {
		*value = elem->cache_val[channel];
		return 0;
	}

	val_len = elem->val_len;
	// quirk: write 2bytes, but read 1byte
	if ( (elem->index == 0x01)||  //  input impedance and input pad switch
	     ((elem->index == 0x0a)&&(elem->wValue < 0x0200))|| // bus mutes
	     (elem->index == 0x32)||(elem->index == 0x33) ) { // mux
		val_len = 1;
	}

	err = get_ctl_urb2(chip, UAC2_CS_CUR, elem->wValue + channel, elem->index, buf, val_len);
	if (err < 0) {
		snd_printd(KERN_ERR "cannot get current value for control %x ch %d: err = %d\n",
			   elem->wValue, channel, err);
		return err;
	}

	if (val_len == 2) { /* S16 */
		*value = buf[0] | ((unsigned int)buf[1] << 8);
		if (*value >= 0x8000)
			(*value) -= 0x10000;
	} else { /* U8 */
		*value = buf[0];
	}

	elem->cached |= 1 << channel;
	elem->cache_val[channel] = *value;

	return 0;
}

/********************** Enum Strings *************************/
static const char txtOff[] = "Off",
	txtPcm1[] = "PCM 1", txtPcm2[] = "PCM 2",
	txtPcm3[] = "PCM 3", txtPcm4[] = "PCM 4",
	txtPcm5[] = "PCM 5", txtPcm6[] = "PCM 6",
	txtPcm7[] = "PCM 7", txtPcm8[] = "PCM 8",
	txtPcm9[] = "PCM 9", txtPcm10[] = "PCM 10",
	txtPcm11[] = "PCM 11", txtPcm12[] = "PCM 12",
	txtPcm13[] = "PCM 13", txtPcm14[] = "PCM 14",
	txtPcm15[] = "PCM 15", txtPcm16[] = "PCM 16",
	txtPcm17[] = "PCM 17", txtPcm18[] = "PCM 18",
	txtPcm19[] = "PCM 19", txtPcm20[] = "PCM 20",
	txtAnlg1[] = "Analog 1", txtAnlg2[] = "Analog 2",
	txtAnlg3[] = "Analog 3", txtAnlg4[] = "Analog 4",
	txtAnlg5[] = "Analog 5", txtAnlg6[] = "Analog 6",
	txtAnlg7[] = "Analog 7", txtAnlg8[] = "Analog 8",
	txtSpdif1[] = "SPDIF 1", txtSpdif2[] = "SPDIF 2",
	txtAdat1[] = "ADAT 1", txtAdat2[] = "ADAT 2",
	txtAdat3[] = "ADAT 3", txtAdat4[] = "ADAT 4",
	txtAdat5[] = "ADAT 5", txtAdat6[] = "ADAT 6",
	txtAdat7[] = "ADAT 7", txtAdat8[] = "ADAT 8",
	txtMix1[] = "Mix A", txtMix2[] = "Mix B",
	txtMix3[] = "Mix C", txtMix4[] = "Mix D",
	txtMix5[] = "Mix E", txtMix6[] = "Mix F",
	txtMix7[] = "Mix G", txtMix8[] = "Mix H",
	txtMix9[] = "Mix I", txtMix10[] = "Mix J",
	txtMix11[] = "Mix K", txtMix12[] = "Mix L",
	txtMix13[] = "Mix M", txtMix14[] = "Mix N",
	txtMix15[] = "Mix O", txtMix16[] = "Mix P";

static const struct scarlett_enum_info opt_pad = {
	.start = 0,
	.len = 2,
	.texts = (const char *[]){
		txtOff, "-10dB"
	}
};

static const struct scarlett_enum_info opt_impedance = {
	.start = 0,
	.len = 2,
	.texts = (const char *[]){
		"Line", "Hi-Z"
	}
};

static const struct scarlett_enum_info opt_clock = {
	.start = 1,
	.len = 3,
	.texts = (const char *[]){
		"Internal", "SPDIF", "ADAT"
	}
};

static const struct scarlett_enum_info opt_sync = {
	.start = 0,
	.len = 2,
	.texts = (const char *[]){
		"No Lock", "Locked"
	}
};

static const struct scarlett_enum_info opt_save = {
	.start = 0,
	.len = 2,
	.texts = (const char *[]){
		"---", "Save"
	}
};

#ifdef WITH_LOGSCALEMETER
/* approx ( 20.0 * log10(x) ) for 16bit
 * map 0..65535 to range 0..194 // -97.0..0dB in .5dB steps */
static int sig_to_db(const int sig16bit)
{
	int i;
	const int dbtbl[148] = {
		13, 14, 15, 16, 16, 17, 18, 20, 21, 22, 23, 25, 26, 28, 29, 31, 33, 35,
		37, 39, 41, 44, 46, 49, 52, 55, 58, 62, 66, 69, 74, 78, 83, 87, 93, 98,
		104, 110, 117, 123, 131, 139, 147, 155, 165, 174, 185, 196, 207, 220,
		233, 246, 261, 276, 293, 310, 328, 348, 369, 390, 414, 438, 464, 491,
		521, 551, 584, 619, 655, 694, 735, 779, 825, 874, 926, 981, 1039, 1100,
		1165, 1234, 1308, 1385, 1467, 1554, 1646, 1744, 1847, 1957, 2072, 2195,
		2325, 2463, 2609, 2764, 2927, 3101, 3285, 3479, 3685, 3904, 4135, 4380,
		4640, 4915, 5206, 5514, 5841, 6187, 6554, 6942, 7353, 7789, 8250, 8739,
		9257, 9806, 10387, 11002, 11654, 12345, 13076, 13851, 14672, 15541,
		16462, 17437, 18471, 19565, 20724, 21952, 23253, 24631, 26090, 27636,
		29274, 31008, 32846, 34792, 36854, 39037, 41350, 43801, 46396, 49145,
		52057, 55142, 58409, 61870
	};

	if (sig16bit < 13) {
		switch (sig16bit) {
			case  0: return 0;
			case  1: return 1;  // -96.0dB
			case  2: return 13; // -90.5dB
			case  3: return 20; // -87.0dB
			case  4: return 26; // -84.0dB
			case  5: return 29; // -82.5dB
			case  6: return 32; // -81.0dB
			case  7: return 35; // -79.5dB
			case  8: return 37; // -78.5dB
			case  9: return 39; // -77.5dB
			case 10: return 41; // -76.5dB
			case 11: return 43; // -75.5dB
			case 12: return 45; // -74.5dB
		}
	}
	for (i = 0 ; i < 148; ++i) {
		if (sig16bit <= dbtbl[i]) return 46+i;
	}
	return 194;
}
#endif

static int scarlett_ctl_switch_info(struct snd_kcontrol *kctl, struct snd_ctl_elem_info *uinfo)
{
	struct scarlett_mixer_elem_info *elem = kctl->private_data;
	
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = elem->count;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	return 0;
}

static int scarlett_ctl_switch_get(struct snd_kcontrol *kctl, struct snd_ctl_elem_value *ucontrol)
{
	struct scarlett_mixer_elem_info *elem = kctl->private_data;
	int i, err, val;
	
	for (i = 0; i < elem->count; i++) {
		err = get_ctl_value(elem, i, &val);
		if (err < 0)
			return err;
		
		val = !val; // alsa uses 0: on, 1: off
		ucontrol->value.integer.value[i] = val;
	}
	
	return 0;
}

static int scarlett_ctl_switch_put(struct snd_kcontrol *kctl, struct snd_ctl_elem_value *ucontrol)
{
	struct scarlett_mixer_elem_info *elem = kctl->private_data;
	int i, changed = 0;
	int err, oval, val;
	
	for (i = 0; i < elem->count; i++) {
		err = get_ctl_value(elem, i, &oval);
		if (err < 0)
			return err;
		
		val = ucontrol->value.integer.value[i];
		val = !val;
		if (oval != val) {
			err = set_ctl_value(elem, i, val);
			if (err < 0)
				return err;
			
			changed = 1;
		}
	}
	
	return changed;
}

static int scarlett_ctl_info(struct snd_kcontrol *kctl, struct snd_ctl_elem_info *uinfo)
{
	struct scarlett_mixer_elem_info *elem = kctl->private_data;
	
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = elem->count;
	uinfo->value.integer.min = -128 + LEVEL_BIAS;
	uinfo->value.integer.max = (int)kctl->private_value + LEVEL_BIAS;
	uinfo->value.integer.step = 1;
	return 0;
}

static int scarlett_ctl_get(struct snd_kcontrol *kctl, struct snd_ctl_elem_value *ucontrol)
{
	struct scarlett_mixer_elem_info *elem = kctl->private_data;
	int i, err, val;
	
	for (i = 0; i < elem->count; i++) {
		err = get_ctl_value(elem, i, &val);
		if (err < 0)
			return err;
		
		val = clamp(val / 256, -128, (int)kctl->private_value) + LEVEL_BIAS;
		ucontrol->value.integer.value[i] = val;
	}
	
	return 0;
}

static int scarlett_ctl_put(struct snd_kcontrol *kctl, struct snd_ctl_elem_value *ucontrol)
{
	struct scarlett_mixer_elem_info *elem = kctl->private_data;
	int i, changed = 0;
	int err, oval, val;
	
	for (i = 0; i < elem->count; i++) {
		err = get_ctl_value(elem, i, &oval);
		if (err < 0)
			return err;
		
		val = ucontrol->value.integer.value[i] - LEVEL_BIAS;
		val = val * 256;
		if (oval != val) {
			err = set_ctl_value(elem, i, val);
			if (err < 0)
				return err;
			
			changed = 1;
		}
	}
	
	return changed;
}

static int scarlett_ctl_enum_info(struct snd_kcontrol *kctl, struct snd_ctl_elem_info *uinfo)
{
	struct scarlett_mixer_elem_info *elem = kctl->private_data;
	
	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = elem->count;
	uinfo->value.enumerated.items = elem->opt->len;
	if (uinfo->value.enumerated.item > uinfo->value.enumerated.items - 1) {
		uinfo->value.enumerated.item = uinfo->value.enumerated.items - 1;
	}
	strcpy(uinfo->value.enumerated.name, elem->opt->texts[uinfo->value.enumerated.item]);
	return 0;
}

static int scarlett_ctl_enum_get(struct snd_kcontrol *kctl, struct snd_ctl_elem_value *ucontrol)
{
	struct scarlett_mixer_elem_info *elem = kctl->private_data;
	int err, val;

	err = get_ctl_value(elem, 0, &val);
	if (err < 0)
		return err;

// snd_printk(KERN_WARNING "enum %s: %x %x\n", ucontrol->id.name, val, elem->opt->len);
	if ( (elem->opt->start == -1)&&(val > elem->opt->len) ) {
// >= 0x20 ???
		val = 0;
	} else {
		val = clamp(val - elem->opt->start, 0, elem->opt->len-1);
	}
	ucontrol->value.enumerated.item[0] = val;

	return 0;
}

static int scarlett_ctl_enum_put(struct snd_kcontrol *kctl, struct snd_ctl_elem_value *ucontrol)
{
	struct scarlett_mixer_elem_info *elem = kctl->private_data;
	int changed = 0;
	int err, oval, val;
	
	err = get_ctl_value(elem, 0, &oval);
	if (err < 0)
		return err;
	
	val = ucontrol->value.integer.value[0];
#if 0 // TODO?
	if (val == -1) {
		val = elem->enum->len + 1; /* only true for master, not for mixer [also master must be used] */
		// ... or? > 0x20,  18i8: 0x22
	} else
#endif
	val = val + elem->opt->start;
	if (oval != val) {
		err = set_ctl_value(elem, 0, val);
		if (err < 0)
			return err;
		
		changed = 1;
	}
	
	return changed;
}

static int scarlett_ctl_save_get(struct snd_kcontrol *kctl, struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.enumerated.item[0] = 0;
	return 0;
}

static int scarlett_ctl_save_put(struct snd_kcontrol *kctl, struct snd_ctl_elem_value *ucontrol)
{
	struct scarlett_mixer_elem_info *elem = kctl->private_data;
	int err;
	
	if (ucontrol->value.enumerated.item[0] > 0) {
		char buf[1] = { 0xa5 };
		
		err = set_ctl_urb2(elem->mixer->chip, UAC2_CS_MEM, 0x005a, 0x3c, buf, 1);
		if (err < 0)
			return err;
		
		snd_printk(KERN_INFO "Scarlett: Saved settings to hardware.\n");
	}
	
	return 0; // (?)
}

#ifdef WITH_METER
static int scarlett_ctl_meter_info(struct snd_kcontrol *kctl, struct snd_ctl_elem_info *uinfo)
{
	struct scarlett_mixer_elem_info *elem = kctl->private_data;
	
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = elem->count;
#ifdef WITH_LOGSCALEMETER
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 194;
#else
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 255; // 0xffff ?
#endif
	uinfo->value.integer.step = 1;
	return 0;
}
#endif

static int scarlett_ctl_meter_get(struct snd_kcontrol *kctl, struct snd_ctl_elem_value *ucontrol)
{
	struct scarlett_mixer_elem_info *elem = kctl->private_data;
	unsigned char buf[2 * MAX_CHANNELS] = {0, };
	int err, val, i;

	err = get_ctl_urb2(elem->mixer->chip, UAC2_CS_MEM, elem->wValue, elem->index, buf, elem->val_len * elem->count);
	if (err < 0) {
		snd_printd(KERN_ERR "cannot get current value for mem %x: err = %d\n",
			   elem->wValue, err);
		return err;
	}

	if (elem->val_len == 1) { /* single U8 */
		ucontrol->value.enumerated.item[0] = clamp((int)buf[0], 0, 1);
	} else { /* multiple S16 */
		for (i = 0; i < elem->count; i++) {
			val = buf[2*i] | ((unsigned int)buf[2*i + 1] << 8);
			if (val >= 0x8000)
				val -= 0x10000;
			
#ifdef WITH_LOGSCALEMETER
			ucontrol->value.integer.value[i] = sig_to_db(val);
#else
			ucontrol->value.integer.value[i] = clamp(val / 256, 0, 255);
#endif
		}
	}

	return 0;
}

static struct snd_kcontrol_new usb_scarlett_ctl_switch = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.name = "",
	.info = scarlett_ctl_switch_info,
	.get =  scarlett_ctl_switch_get,
	.put =  scarlett_ctl_switch_put,
};

static const DECLARE_TLV_DB_SCALE(db_scale_scarlett_gain, -12800, 100, 0);

static struct snd_kcontrol_new usb_scarlett_ctl = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE | SNDRV_CTL_ELEM_ACCESS_TLV_READ,
	.name = "",
	.info = scarlett_ctl_info,
	.get =  scarlett_ctl_get,
	.put =  scarlett_ctl_put,
	.private_value = 6,  // max value
	.tlv = { .p = db_scale_scarlett_gain }
};

static struct snd_kcontrol_new usb_scarlett_ctl_master = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE | SNDRV_CTL_ELEM_ACCESS_TLV_READ,
	.name = "",
	.info = scarlett_ctl_info,
	.get =  scarlett_ctl_get,
	.put =  scarlett_ctl_put,
//	.private_value = 0,  // max value, not 6 but 0
	.private_value = 6,  // max value
	.tlv = { .p = db_scale_scarlett_gain }
};

static struct snd_kcontrol_new usb_scarlett_ctl_enum = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.name = "",
	.info = scarlett_ctl_enum_info,
	.get =  scarlett_ctl_enum_get,
	.put =  scarlett_ctl_enum_put,
	// .private_value
};

static struct snd_kcontrol_new usb_scarlett_ctl_sync = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.access = SNDRV_CTL_ELEM_ACCESS_READ | SNDRV_CTL_ELEM_ACCESS_VOLATILE,
	.name = "",
	.info = scarlett_ctl_enum_info,
	.get =  scarlett_ctl_meter_get,
};

#ifdef WITH_METER
#ifdef WITH_LOGSCALEMETER
static const DECLARE_TLV_DB_SCALE(db_scale_scarlett_peak, -9700, 50, 0);
#endif

static struct snd_kcontrol_new usb_scarlett_ctl_meter = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
#ifdef WITH_LOGSCALEMETER
	.access = SNDRV_CTL_ELEM_ACCESS_READ | SNDRV_CTL_ELEM_ACCESS_VOLATILE | SNDRV_CTL_ELEM_ACCESS_TLV_READ,
	.tlv = { .p = db_scale_scarlett_peak },
#else
	.access = SNDRV_CTL_ELEM_ACCESS_READ | SNDRV_CTL_ELEM_ACCESS_VOLATILE,
#endif
	.name = "",
	.info = scarlett_ctl_meter_info,
	.get =  scarlett_ctl_meter_get,
};
#endif

static struct snd_kcontrol_new usb_scarlett_ctl_save = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.name = "",
	.info = scarlett_ctl_enum_info,
	.get =  scarlett_ctl_save_get,
	.get =  scarlett_ctl_save_put,
};

static int add_new_ctl(struct usb_mixer_interface *mixer,
                       const struct snd_kcontrol_new *ncontrol,
                       int index, int offset, int num,
                       int val_len, int count, const char *name,
                       const struct scarlett_enum_info *opt,
                       struct scarlett_mixer_elem_info **elem_ret)
{
	struct snd_kcontrol *kctl;
	struct scarlett_mixer_elem_info *elem;
	int err;
	
	elem = kzalloc(sizeof(*elem), GFP_KERNEL);
	if (!elem)
		return -ENOMEM;
	
	elem->mixer = mixer;
	elem->wValue = (offset << 8) | num;
	elem->index = index;
	elem->val_len = val_len;
	elem->count = count;
	elem->opt = opt;
	
	kctl = snd_ctl_new1(ncontrol, elem);
	if (!kctl) {
		snd_printk(KERN_ERR "cannot malloc kcontrol\n");
		kfree(elem);
		return -ENOMEM;
	}
	kctl->private_free = scarlett_mixer_elem_free;
	
	snprintf(kctl->id.name, sizeof(kctl->id.name), "%s", name);
	
	err = snd_ctl_add(mixer->chip->card, kctl);
	if (err < 0)
		return err;
	
	if (elem_ret) {
		*elem_ret = elem;
	}
	
	return 0;
}

static int init_ctl(struct scarlett_mixer_elem_info *elem, int value)
{
	int err, channel;
	
	for (channel = 0; channel < elem->count; channel++) {
		err = set_ctl_value(elem, channel, value);
		if (err < 0)
			return err;
	}
	
	return 0;
}

#define INIT(value) \
	err = init_ctl(elem, value); \
	if (err < 0) \
		return err;

#define CTL_SWITCH(cmd, off, no, count, name) \
	err = add_new_ctl(mixer, &usb_scarlett_ctl_switch, cmd, off, no, 2, count, name, NULL, &elem); \
	if (err < 0) \
		return err;

// no multichannel enum, always count == 1  (at least for now)
#define CTL_ENUM(cmd, off, no, name, opt) \
	err = add_new_ctl(mixer, &usb_scarlett_ctl_enum, cmd, off, no, 2, 1, name, opt, &elem); \
	if (err < 0) \
		return err;

#define CTL_MIXER(cmd, off, no, count, name) \
	err = add_new_ctl(mixer, &usb_scarlett_ctl, cmd, off, no, 2, count, name, NULL, &elem); \
	if (err < 0) \
		return err; \
	INIT(-32768); /* -128*256 */

#define CTL_MASTER(cmd, off, no, count, name) \
	err = add_new_ctl(mixer, &usb_scarlett_ctl_master, cmd, off, no, 2, count, name, NULL, &elem); \
	if (err < 0) \
		return err; \
	INIT(0);

#define CTL_PEAK(cmd, off, no, count, name)  /* but UAC2_CS_MEM */ \
	err = add_new_ctl(mixer, &usb_scarlett_ctl_meter, cmd, off, no, 2, count, name, NULL, NULL); \
	if (err < 0) \
		return err;

static int add_output_ctls(struct usb_mixer_interface *mixer,
                           int index, const char *name,
                           const struct scarlett_device_info *info)
{
	int err;
	char mx[45];
	struct scarlett_mixer_elem_info *elem;
	
	snprintf(mx, 45, "Master %d (%s) Playback Switch", index+1, name); /* mute */
	CTL_SWITCH(0x0a, 0x01, 2*index+1, 2, mx);
	
	snprintf(mx, 45, "Master %d (%s) Playback Volume", index+1, name);
	CTL_MASTER(0x0a, 0x02, 2*index+1, 2, mx);
	
	snprintf(mx, 45, "Master %dL (%s) Source Playback Enum", index+1, name);
	CTL_ENUM  (0x33, 0x00, 2*index, mx, &info->opt_master);
	INIT      (info->mix_start);

	snprintf(mx, 45, "Master %dR (%s) Source Playback Enum", index+1, name);
	CTL_ENUM  (0x33, 0x00, 2*index+1, mx, &info->opt_master);
	INIT      (info->mix_start + 1);

	return 0;
}

#define CTLS_OUTPUT(index, name) \
	err = add_output_ctls(mixer, index, name, info); \
	if (err < 0) \
		return err;


/********************** device-specific config *************************/
static int scarlet_s8i6_controls(struct usb_mixer_interface *mixer,
                                 const struct scarlett_device_info *info)
{
	struct scarlett_mixer_elem_info *elem;
	int err;

	CTLS_OUTPUT(0, "Monitor");
	CTLS_OUTPUT(1, "Headphone");
	CTLS_OUTPUT(2, "SPDIF");

	CTL_ENUM  (0x01, 0x09, 1, "Input 1 Impedance Switch", &opt_impedance);
	CTL_ENUM  (0x01, 0x09, 2, "Input 2 Impedance Switch", &opt_impedance);

	CTL_ENUM  (0x01, 0x0b, 3, "Input 3 Pad Switch", &opt_pad);
	CTL_ENUM  (0x01, 0x0b, 4, "Input 4 Pad Switch", &opt_pad);

	return 0;
}

static int scarlet_s18i6_controls(struct usb_mixer_interface *mixer,
                                  const struct scarlett_device_info *info)
{
	struct scarlett_mixer_elem_info *elem;
	int err;

	CTLS_OUTPUT(0, "Monitor");
	CTLS_OUTPUT(1, "Headphone");
	CTLS_OUTPUT(2, "SPDIF");

	CTL_ENUM  (0x01, 0x09, 1, "Input 1 Impedance Switch", &opt_impedance);
	CTL_ENUM  (0x01, 0x09, 2, "Input 2 Impedance Switch", &opt_impedance);

	return 0;
}

static int scarlet_s18i8_controls(struct usb_mixer_interface *mixer,
                                  const struct scarlett_device_info *info)
{
	struct scarlett_mixer_elem_info *elem;
	int err;

	CTLS_OUTPUT(0, "Monitor");
	CTLS_OUTPUT(1, "Headphone 1");
	CTLS_OUTPUT(2, "Headphone 2");
	CTLS_OUTPUT(3, "SPDIF");

	CTL_ENUM  (0x01, 0x09, 1, "Input 1 Impedance Switch", &opt_impedance);
	CTL_ENUM  (0x01, 0x0b, 1, "Input 1 Pad Switch", &opt_pad);

	CTL_ENUM  (0x01, 0x09, 2, "Input 2 Impedance Switch", &opt_impedance);
	CTL_ENUM  (0x01, 0x0b, 2, "Input 2 Pad Switch", &opt_pad);

	CTL_ENUM  (0x01, 0x0b, 3, "Input 3 Pad Switch", &opt_pad);
	CTL_ENUM  (0x01, 0x0b, 4, "Input 4 Pad Switch", &opt_pad);

	return 0;
}

static int scarlet_s18i20_controls(struct usb_mixer_interface *mixer,
                                  const struct scarlett_device_info *info)
{
//	struct scarlett_mixer_elem_info *elem;
	int err;

	CTLS_OUTPUT(0, "Monitor");   // 1/2
	CTLS_OUTPUT(1, "Line 3/4");
	CTLS_OUTPUT(2, "Line 5/6");
	CTLS_OUTPUT(3, "Line 7/8");  // = Headphone 1
	CTLS_OUTPUT(4, "Line 9/10"); // = Headphone 2
	CTLS_OUTPUT(5, "SPDIF");
	CTLS_OUTPUT(6, "ADAT 1/2");
	CTLS_OUTPUT(7, "ADAT 3/4");
	CTLS_OUTPUT(8, "ADAT 5/6");
	CTLS_OUTPUT(9, "ADAT 7/8");

/* ? real hardware switches
	CTL_ENUM  (0x01, 0x09, 1, "Input 1 Impedance Switch", &opt_impedance);
	CTL_ENUM  (0x01, 0x0b, 1, "Input 1 Pad Switch", &opt_pad);

	CTL_ENUM  (0x01, 0x09, 2, "Input 2 Impedance Switch", &opt_impedance);
	CTL_ENUM  (0x01, 0x0b, 2, "Input 2 Pad Switch", &opt_pad);

	CTL_ENUM  (0x01, 0x0b, 3, "Input 3 Pad Switch", &opt_pad);
	CTL_ENUM  (0x01, 0x0b, 4, "Input 4 Pad Switch", &opt_pad);
*/

	return 0;
}

static const char *s8i6_texts[] = {
	txtOff, /* 'off' == 0xff */
	txtPcm1, txtPcm2, txtPcm3, txtPcm4,
	txtPcm5, txtPcm6, txtPcm7, txtPcm8,
	txtPcm9, txtPcm10, txtPcm11, txtPcm12,
	txtAnlg1, txtAnlg2, txtAnlg3, txtAnlg4,
	txtSpdif1, txtSpdif2,
	txtMix1, txtMix2, txtMix3, txtMix4,
	txtMix5, txtMix6
};

/*  untested...  */
static const struct scarlett_device_info s8i6_info = {
	.matrix_in = 18,
	.matrix_out = 6,
	.input_len = 8,
	.output_len = 6,

	.pcm_start = 0,
	.analog_start = 12,
	.spdif_start = 16,
	.adat_start = 18,
	.mix_start = 18,

	.opt_master = {
		.start = -1,
		.len = 25,
		.texts = s8i6_texts
	},

	.opt_matrix = {
		.start = -1,
		.len = 19,
		.texts = s8i6_texts
	},

	.controls_fn = scarlet_s8i6_controls,
	.matrix_mux_init = {
		12, 13, 14, 15,                 // Analog -> 1..4
		16, 17,                          // SPDIF -> 5,6
		0, 1, 2, 3, 4, 5, 6, 7,     // PCM[1..12] -> 7..18
		8, 9, 10, 11
	}
};

static const char *s18i6_texts[] = {
	txtOff, /* 'off' == 0xff */
	txtPcm1, txtPcm2, txtPcm3, txtPcm4,
	txtPcm5, txtPcm6,
	txtAnlg1, txtAnlg2, txtAnlg3, txtAnlg4,
	txtAnlg5, txtAnlg6, txtAnlg7, txtAnlg8,
	txtSpdif1, txtSpdif2,
	txtAdat1, txtAdat2, txtAdat3, txtAdat4,
	txtAdat5, txtAdat6, txtAdat7, txtAdat8,
	txtMix1, txtMix2, txtMix3, txtMix4,
	txtMix5, txtMix6
};

static const struct scarlett_device_info s18i6_info = {
	.matrix_in = 18,
	.matrix_out = 6,
	.input_len = 18,
	.output_len = 6,

	.pcm_start = 0,
	.analog_start = 6,
	.spdif_start = 14,
	.adat_start = 16,
	.mix_start = 24,

	.opt_master = {
		.start = -1,
		.len = 31,
		.texts = s18i6_texts
	},

	.opt_matrix = {
		.start = -1,
		.len = 25,
		.texts = s18i6_texts
	},

	.controls_fn = scarlet_s18i6_controls,
	.matrix_mux_init = {
		 6,  7,  8,  9, 10, 11, 12, 13, // Analog -> 1..8
		16, 17, 18, 19, 20, 21,     // ADAT[1..6] -> 9..14
		14, 15,                          // SPDIF -> 15,16
		0, 1                          // PCM[1,2] -> 17,18
	}
};

static const char *s18i8_texts[] = {
	txtOff, /* 'off' == 0xff  (original software: 0x22) */
	txtPcm1, txtPcm2, txtPcm3, txtPcm4,
	txtPcm5, txtPcm6, txtPcm7, txtPcm8,
	txtAnlg1, txtAnlg2, txtAnlg3, txtAnlg4,
	txtAnlg5, txtAnlg6, txtAnlg7, txtAnlg8,
	txtSpdif1, txtSpdif2,
	txtAdat1, txtAdat2, txtAdat3, txtAdat4,
	txtAdat5, txtAdat6, txtAdat7, txtAdat8,
	txtMix1, txtMix2, txtMix3, txtMix4,
	txtMix5, txtMix6, txtMix7, txtMix8
};

static const struct scarlett_device_info s18i8_info = {
	.matrix_in = 18,
	.matrix_out = 8,
	.input_len = 18,
	.output_len = 8,

	.pcm_start = 0,
	.analog_start = 8,
	.spdif_start = 16,
	.adat_start = 18,
	.mix_start = 26,

	.opt_master = {
		.start = -1,
		.len = 35,
		.texts = s18i8_texts
	},

	.opt_matrix = {
		.start = -1,
		.len = 27,
		.texts = s18i8_texts
	},

	.controls_fn = scarlet_s18i8_controls,
	.matrix_mux_init = {
		 8,  9, 10, 11, 12, 13, 14, 15, // Analog -> 1..8
		18, 19, 20, 21, 22, 23,     // ADAT[1..6] -> 9..14
		16, 17,                          // SPDIF -> 15,16
		0, 1                          // PCM[1,2] -> 17,18
	}
};

static const char *s18i20_texts[] = {
	txtOff, /* 'off' == 0xff  (original software: 0x22) */
	txtPcm1, txtPcm2, txtPcm3, txtPcm4,
	txtPcm5, txtPcm6, txtPcm7, txtPcm8,
	txtPcm9, txtPcm10, txtPcm11, txtPcm12,
	txtPcm13, txtPcm14, txtPcm15, txtPcm16,
	txtPcm17, txtPcm18, txtPcm19, txtPcm20,
	txtAnlg1, txtAnlg2, txtAnlg3, txtAnlg4,
	txtAnlg5, txtAnlg6, txtAnlg7, txtAnlg8,
	txtSpdif1, txtSpdif2,
	txtAdat1, txtAdat2, txtAdat3, txtAdat4,
	txtAdat5, txtAdat6, txtAdat7, txtAdat8,
	txtMix1, txtMix2, txtMix3, txtMix4,
	txtMix5, txtMix6, txtMix7, txtMix8,
	txtMix9, txtMix10, txtMix11, txtMix12,
	txtMix13, txtMix14, txtMix15, txtMix16
};

/*  untested...  specs says 18x16 matrix, but how do the other 4 outputs work? */
static const struct scarlett_device_info s18i20_info = {
	.matrix_in = 18,
	.matrix_out = 16,
	.input_len = 18,
	.output_len = 20,

	.pcm_start = 0,
	.analog_start = 20,
	.spdif_start = 28,
	.adat_start = 30,
	.mix_start = 38,

	.opt_master = {
		.start = -1,
		.len = 55,
		.texts = s18i20_texts
	},

	.opt_matrix = {
		.start = -1,
		.len = 39,
		.texts = s18i20_texts
	},

	.controls_fn = scarlet_s18i20_controls,
	.matrix_mux_init = {
		20, 21, 22, 23, 24, 25, 26, 27, // Analog -> 1..8
		30, 31, 32, 33, 34, 35,     // ADAT[1..6] -> 9..14
		28, 29,                          // SPDIF -> 15,16
		0, 1                          // PCM[1,2] -> 17,18
	}
};

/*
int scarlett_reset(struct usb_mixer_interface *mixer)
{
	// TODO? save first-time init flag into device?

	// unmute [master +] mixes (switches are currently not initialized)
	// [set(get!) impedance: 0x01, 0x09, 1..2]
	// [set(get!) 0x01, 0x08, 3..4]
	// [set(get!) pad: 0x01, 0x0b, 1..4]

	// matrix inputs (currently in scarlett_mixer_controls)
}
*/

/*
 * Create and initialize a mixer for the Focusrite(R) Scarlett
 */
int scarlett_mixer_controls(struct usb_mixer_interface *mixer)
{
	int err, i, o;
	char mx[32];
	const struct scarlett_device_info *info;
	struct scarlett_mixer_elem_info *elem;

	CTL_SWITCH(0x0a, 0x01, 0, 1, "Master Playback Switch");
	CTL_MASTER(0x0a, 0x02, 0, 1, "Master Playback Volume");

	switch (mixer->chip->usb_id) {
	case USB_ID(0x1235, 0x8002): info = &s8i6_info; break;
	case USB_ID(0x1235, 0x8004): info = &s18i6_info; break;
	case USB_ID(0x1235, 0x8014): info = &s18i8_info; break;
	case USB_ID(0x1235, 0x800c): info = &s18i20_info; break;
	default: /* device not (yet) supported */
		return -EINVAL;
	}

	err = (*info->controls_fn)(mixer, info);
	if (err < 0)
		return err;

	for (i = 0; i < info->matrix_in; i++) {
		snprintf(mx, 32, "Matrix %02d Input Playback Route", i+1);
		CTL_ENUM  (0x32, 0x06, i, mx, &info->opt_matrix);
		INIT      (info->matrix_mux_init[i]);

		for (o = 0; o < info->matrix_out; o++) {
			sprintf(mx, "Matrix %02d Mix %c Playback Volume", i+1, o+'A');
			CTL_MIXER (0x3c, 0x00, (i<<3) + (o&0x07), 1, mx);
			if (  ( (o == 0)&&(info->matrix_mux_init[i] == info->pcm_start) )||
			      ( (o == 1)&&(info->matrix_mux_init[i] == info->pcm_start + 1) )  ) {
				INIT      (0);   // init hack: enable PCM 1 / 2 on Mix A / B
			}
			
		}
	}

	for (i = 0; i < info->input_len; i++) {
		snprintf(mx, 32, "Input Source %02d Capture Route", i+1);
		CTL_ENUM  (0x34, 0x00, i, mx, &info->opt_master);
		INIT      (info->analog_start + i);
	}

	/* val_len == 1 needed here */
	err = add_new_ctl(mixer, &usb_scarlett_ctl_enum, 0x28, 0x01, 0, 1,
	                  1, "Sample Clock Source", &opt_clock, NULL);
	if (err < 0)
		return err;

	/* val_len == 1 and UAC2_CS_MEM */
	err = add_new_ctl(mixer, &usb_scarlett_ctl_sync, 0x3c, 0x00, 2, 1,
	                  1, "Sample Clock Sync Status", &opt_sync, NULL);
	if (err < 0)
		return err;

	/* val_len == 1 and UAC2_CS_MEM */
	err = add_new_ctl(mixer, &usb_scarlett_ctl_save, 0x3c, 0x00, 0x5a, 1,
	                  1, "Save To HW", &opt_save, NULL);
	if (err < 0)
		return err;

#ifdef WITH_METER
	CTL_PEAK  (0x3c, 0x00, 0, info->input_len, "Input Meter");
	CTL_PEAK  (0x3c, 0x00, 1, info->matrix_out, "Matrix Meter");
	CTL_PEAK  (0x3c, 0x00, 3, info->output_len, "PCM Meter");
#endif

	/* initialize sampling rate to 48000 */
	err = set_ctl_urb2(mixer->chip, UAC2_CS_CUR, 0x0100, 0x29, "\x80\xbb\x00\x00", 4);
	if (err < 0)
		return err;

// TODO(?) scarlett_reset(mixer);

	return 0;
}


/**************************** OLD CODE ****************************/

#if 0
static int s18i6_first_time_reset(struct usb_mixer_interface *mixer)
{
	/* routes and mute registers do not represent the actual state of the
	 * device after power-cycles.
	 *
	 * However, after they have been set once, the values can be re-read
	 * between USB re-connects if the sound-card itself remains powered-on.
	 *
	 * -> reset the state IFF the device has not been initialized.
	 *
	 * to keep track of initialization, register 0x0106 at index 0x0a is used.
	 * (an unused 'mute' state of the device which retains R/W data).
	 * It is zero after a power-cycle (even if config was saved to the device)
	 * and retains r/w data after that.
	 */

// FIXME?  19 or 20 is not working.
//	int marker = 6; // 18i6
	int marker = 8; // 18i8

	memset(buf, 0, sizeof(buf));
	get_ctl_urb2(mixer->chip, UAC2_CS_CUR, (S18I6__MUTE << 8) + marker, S18I6_MASTER_VOLUME, buf, 1);
	if (buf[0] == 0x01) {
		snd_printk(KERN_INFO "Scarlett 18i6: already initialized (no device power-cycle).\n");
		return 0;
	}

	snd_printk(KERN_INFO "Scarlett 18i6: initializing 18i6 mixer after device power-cycle.\n");

	/* mark chip as initialized */
	buf[0] = 0x01;
	set_ctl_urb2(mixer->chip, UAC2_CS_CUR, (S18I6__MUTE << 8) + marker, S18I6_MASTER_VOLUME, buf, 2);

#if 0
	memset(buf, 0, sizeof(buf));
	get_ctl_urb2(mixer->chip, UAC2_CS_CUR, (S18I6__MUTE << 8) + marker, S18I6_MASTER_VOLUME, buf, 1);
	snd_printk(KERN_ERR "18i6: check marker %x %x\n", buf[0], buf[1]);
#endif
}
#endif
