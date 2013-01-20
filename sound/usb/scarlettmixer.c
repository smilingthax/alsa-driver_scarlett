/*
 *   (Tentative) Scarlett 18i6 Driver for ALSA
 *
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
 * Many features of Scarlett 18i6 do not lend themselves to be implemented
 * as simple mixer-quirk -- or at least I don't see a way how to do that, yet.
 * Hence the top parts of this file is a 1:1 copy of select static functions
 * from mixer.c to implement the interface.
 * Suggestions how to avoid this code duplication are very welcome.
 */

/* Mixer Interface for the Focusrite Scarlett 18i6 audio interface.
 *
 * The protocol was reverse engineered by looking at communication between
 * Scarlett MixControl (v 1.2.128.0) and the Focusrite(R) Scarlett 18i6 (firmware
 * v305) using wireshark and usbmon in January 2013.
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
 * USB URB commands overview
 * wIndex
 * 0x01 Analog Input line/instrument impedance switch, wValue=0x0901 + channel, data=Line/Inst (2bytes)
 * 0x0a Master Volume, wValue=0x0200+bus data(2bytes); Bus Mute/Unmute wValue=0x0100+bus, data(2bytes)
 * 0x28 Clock source, wValue=0x0100, data=int,spdif,adat (1byte)
 * 0x29 Set Sample-rate, wValue=0x0100 data=samle-rate(4bytes)
 * 0x32 Assign mixer inputs, wValue=0x0600 + mixer-channel, data=input-to-connect(2bytes)
 * 0x33 Routing table, wValue=bus, data=input-to-connect(2bytes)
 * 0x34 ?? (clear mixer -- force assignment) used during factory-reset
 * 0x3c Matrix Mixer gains, wValue=mixer-node  data=gain(2bytes)
 *
 *
 *
 * <ditaa>
 *  /--------------\     18chn
 *  | Hardware  in +--+--------+---------------\
 *  \--------------/  |        |               |
 *                    |        |               v 18chn
 *                    |        |         +-----------+
 *                    |        |         | ALSA PCM  |
 *                    |        |         |   (DAW)   |
 *                    |        |         +-----+--=--+
 *                    |        |               | 6chn
 *                    |        |       /-------+
 *                    |        |       |       |
 *                    |        v       v       |
 *                    |      +-----------+     |
 *                    |      | Mixer     |     |
 *                    |      |    Router |     |
 *                    |      +-----+-----+     |
 *                    |            |           |
 *                    |            | 18chn     |
 *                    |            v           |
 *                    |      +-----------+     |
 *                    |      | Mixer     |     |
 *                    |      |    Matrix |     |
 *                    |      |           |     |
 *                    |      | 18x6 Gain |     |
 *                    |      |   stages  |     |
 *                    |      +-----+-----+     |
 *                    |            |           |
 *                    | 18chn      | 6chn      | 6chn
 *                    v            v           v
 *                  +----------------------------+
 *                  |           Router           |
 *                  +--------------+-------------+
 *                                 |
 *                                 | 6chn (3 stereo pairs)
 *                                 v
 *                  +----------------------------+
 *                  |      Master Gain Ctrl      |
 *                  +--------------+-------------+
 *                                 |
 *  /--------------\     6chn      |
 *  | Hardware out |<--------------/
 *  \--------------/
 * </ditaa>
 *
 */

/*****************************************************************************/
/*************** some unmodified static functions from mixer.c ***************/

#include <linux/bitops.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/usb.h>
#include <linux/usb/audio.h>
#include <linux/usb/audio-v2.h>

#include <sound/core.h>
#include <sound/control.h>
#include <sound/hwdep.h>
#include <sound/info.h>
#include <sound/tlv.h>

#include "usbaudio.h"
#include "mixer.h"
#include "helper.h"
#include "mixer_quirks.h"
#include "power.h"

#define MAX_ID_ELEMS	256

/*
 * convert from the byte/word on usb descriptor to the zero-based integer
 */
static int convert_signed_value(struct usb_mixer_elem_info *cval, int val)
{
	switch (cval->val_type) {
	case USB_MIXER_BOOLEAN:
		return !!val;
	case USB_MIXER_INV_BOOLEAN:
		return !val;
	case USB_MIXER_U8:
		val &= 0xff;
		break;
	case USB_MIXER_S8:
		val &= 0xff;
		if (val >= 0x80)
			val -= 0x100;
		break;
	case USB_MIXER_U16:
		val &= 0xffff;
		break;
	case USB_MIXER_S16:
		val &= 0xffff;
		if (val >= 0x8000)
			val -= 0x10000;
		break;
	}
	return val;
}

/*
 * convert from the zero-based int to the byte/word for usb descriptor
 */
static int convert_bytes_value(struct usb_mixer_elem_info *cval, int val)
{
	switch (cval->val_type) {
	case USB_MIXER_BOOLEAN:
		return !!val;
	case USB_MIXER_INV_BOOLEAN:
		return !val;
	case USB_MIXER_S8:
	case USB_MIXER_U8:
		return val & 0xff;
	case USB_MIXER_S16:
	case USB_MIXER_U16:
		return val & 0xffff;
	}
	return 0; /* not reached */
}

static int get_relative_value(struct usb_mixer_elem_info *cval, int val)
{
	if (! cval->res)
		cval->res = 1;
	if (val < cval->min)
		return 0;
	else if (val >= cval->max)
		return (cval->max - cval->min + cval->res - 1) / cval->res;
	else
		return (val - cval->min) / cval->res;
}

static int get_abs_value(struct usb_mixer_elem_info *cval, int val)
{
	if (val < 0)
		return cval->min;
	if (! cval->res)
		cval->res = 1;
	val *= cval->res;
	val += cval->min;
	if (val > cval->max)
		return cval->max;
	return val;
}


/*
 * retrieve a mixer value
 */

static int get_ctl_value_v2(struct usb_mixer_elem_info *cval, int request, int validx, int *value_ret)
{
	struct snd_usb_audio *chip = cval->mixer->chip;
	unsigned char buf[2 + 3*sizeof(__u16)]; /* enough space for one range */
	unsigned char *val;
	int idx = 0, ret, size;
	__u8 bRequest;

	if (request == UAC_GET_CUR) {
		bRequest = UAC2_CS_CUR;
		size = sizeof(__u16);
	} else {
		bRequest = UAC2_CS_RANGE;
		size = sizeof(buf);
	}

	memset(buf, 0, sizeof(buf));

	ret = snd_usb_autoresume(chip) ? -EIO : 0;
	if (ret)
		goto error;

	down_read(&chip->shutdown_rwsem);
	if (chip->shutdown)
		ret = -ENODEV;
	else {
		idx = snd_usb_ctrl_intf(chip) | (cval->id << 8);
		ret = snd_usb_ctl_msg(chip->dev, usb_rcvctrlpipe(chip->dev, 0), bRequest,
			      USB_RECIP_INTERFACE | USB_TYPE_CLASS | USB_DIR_IN,
			      validx, idx, buf, size);
	}
	up_read(&chip->shutdown_rwsem);
	snd_usb_autosuspend(chip);


	if (ret < 0) {
error:
		snd_printk(KERN_ERR "cannot get ctl value: req = %#x, wValue = %#x, wIndex = %#x, type = %d\n",
			   request, validx, idx, cval->val_type);
		return ret;
	}
#if 0 // rg debug XXX -- OK i was lying, here's a modification of the original code :)
	else snd_printk(KERN_ERR "req ctl value: req = %#x, rtype = %#x, wValue = %#x, wIndex = %#x, size = %d\n",
			request, (USB_RECIP_INTERFACE | USB_TYPE_CLASS | USB_DIR_IN), validx, idx, size);
#endif

	/* FIXME: how should we handle multiple triplets here? */

	switch (request) {
	case UAC_GET_CUR:
		val = buf;
		break;
	case UAC_GET_MIN:
		val = buf + sizeof(__u16);
		break;
	case UAC_GET_MAX:
		val = buf + sizeof(__u16) * 2;
		break;
	case UAC_GET_RES:
		val = buf + sizeof(__u16) * 3;
		break;
	default:
		return -EINVAL;
	}

	*value_ret = convert_signed_value(cval, snd_usb_combine_bytes(val, sizeof(__u16)));

	return 0;
}


/* private_free callback */
static void usb_mixer_elem_free(struct snd_kcontrol *kctl)
{
	kfree(kctl->private_data);
	kctl->private_data = NULL;
}


static void snd_usb_mixer_free(struct usb_mixer_interface *mixer)
{
	kfree(mixer->id_elems);
	if (mixer->urb) {
		kfree(mixer->urb->transfer_buffer);
		usb_free_urb(mixer->urb);
	}
	usb_free_urb(mixer->rc_urb);
	kfree(mixer->rc_setup_packet);
	kfree(mixer);
}

static int snd_usb_mixer_dev_free(struct snd_device *device)
{
	struct usb_mixer_interface *mixer = device->device_data;
	snd_usb_mixer_free(mixer);
	return 0;
}

static void snd_usb_mixer_dump_cval(struct snd_info_buffer *buffer,
				    int unitid,
				    struct usb_mixer_elem_info *cval)
{
	static char *val_types[] = {"BOOLEAN", "INV_BOOLEAN",
				    "S8", "U8", "S16", "U16"};
	snd_iprintf(buffer, "  Unit: %i\n", unitid);
	if (cval->elem_id)
		snd_iprintf(buffer, "    Control: name=\"%s\", index=%i\n",
				cval->elem_id->name, cval->elem_id->index);
	snd_iprintf(buffer, "    Info: id=%i, control=%i, cmask=0x%x, "
			    "channels=%i, type=\"%s\"\n", cval->id,
			    cval->control, cval->cmask, cval->channels,
			    val_types[cval->val_type]);
	snd_iprintf(buffer, "    Volume: min=%i, max=%i, dBmin=%i, dBmax=%i\n",
			    cval->min, cval->max, cval->dBmin, cval->dBmax);
}

static void snd_usb_mixer_proc_read(struct snd_info_entry *entry,
				    struct snd_info_buffer *buffer)
{
	struct snd_usb_audio *chip = entry->private_data;
	struct usb_mixer_interface *mixer;
	struct usb_mixer_elem_info *cval;
	int unitid;

	list_for_each_entry(mixer, &chip->mixer_list, list) {
		snd_iprintf(buffer,
			"USB Mixer: usb_id=0x%08x, ctrlif=%i, ctlerr=%i\n",
				chip->usb_id, snd_usb_ctrl_intf(chip),
				mixer->ignore_ctl_error);
		snd_iprintf(buffer, "Card: %s\n", chip->card->longname);
		for (unitid = 0; unitid < MAX_ID_ELEMS; unitid++) {
			for (cval = mixer->id_elems[unitid]; cval;
						cval = cval->next_id_elem)
				snd_usb_mixer_dump_cval(buffer, unitid, cval);
		}
	}
}


/****************** END unmodified static code from mixer.c ******************/
/*****************************************************************************/


/***************************** Low Level USB I/O *****************************/

static int get_ctl_urb2(struct snd_usb_audio *chip,
		int bRequest, int wValue, int wIndex,
		unsigned char *buf, int size)
{
	int ret, idx = 0;

	ret = snd_usb_autoresume(chip);
	if (ret < 0 && ret != -ENODEV) {
		ret = -EIO;
		goto error;
	}

	down_read(&chip->shutdown_rwsem);
	if (chip->shutdown)
		ret = -ENODEV;
	else {
		idx = snd_usb_ctrl_intf(chip) | wIndex;
		ret = snd_usb_ctl_msg(chip->dev, usb_rcvctrlpipe(chip->dev, 0), bRequest,
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
	return 0;
}

static int set_ctl_urb2(struct snd_usb_audio *chip,
		int request, int validx, int id,
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
		idx = snd_usb_ctrl_intf(chip) | (id);
		if (snd_usb_ctl_msg(chip->dev,
				    usb_sndctrlpipe(chip->dev, 0), request,
				    USB_RECIP_INTERFACE | USB_TYPE_CLASS | USB_DIR_OUT,
				    validx, idx, buf, val_len) >= 0) {
			err = 0;
			goto out;
		}
	}
	snd_printdd(KERN_ERR "cannot set ctl value: req = %#x, wValue = %#x, wIndex = %#x, len = %d, data = %#x/%#x\n",
		    request, validx, idx, val_len, buf[0], buf[1]);
	err = -EINVAL;

 out:
	up_read(&chip->shutdown_rwsem);
	snd_usb_autosuspend(chip);
#if 0 // rg debug XXX
	snd_printk(KERN_ERR "set ctl value: req = %#x, wValue = %#x, wIndex = %#x, len = %d, data = %#x/%#x\n",
		    request, validx, idx, val_len, buf[0], buf[1]);
#endif
	return err;
}

static int set_ctl_value(struct usb_mixer_elem_info *cval,
				int validx, int value_set)
{
	struct snd_usb_audio *chip = cval->mixer->chip;
	unsigned char buf[2];
	int val_len;

	validx += cval->idx_off;

	if (cval->id == 0x28)
		val_len = sizeof(__u8);
	else
		val_len = sizeof(__u16);


	value_set = convert_bytes_value(cval, value_set);
	buf[0] = value_set & 0xff;
	buf[1] = (value_set >> 8) & 0xff;

	return set_ctl_urb2(chip, UAC2_CS_CUR, validx, (cval->id << 8), buf, val_len);
}


/**************************** Scarlett 18i6 Mixer ****************************/

#include "scarlettmixer.h"
#define S18I6_MAX_CHANNELS	18

static inline int s18i6_ctl_urb(struct usb_mixer_elem_info *cval,
				  int channel, int *value)
{
	if (cval->id == 0x33) {
		/* Scarlett can't be queried for route assigns */
		*value = 0xfe;
		return 0;
	}
	if (cval->id == 0x3c) {
		/* Scarlett can't be queried for mixer gains */
		*value = 0xfe;
		return 0;
	}

	if (cval->id == 0x3c) // mixer-matrix volume --- ^^ currently unused
		return get_ctl_value_v2(cval, UAC_GET_CUR, cval->control, value);
	else
		return get_ctl_value_v2(cval, UAC_GET_CUR, ((cval->control << 8) | channel) + cval->idx_off, value);
}

static int s18i6_get_cur_mix_value(struct usb_mixer_elem_info *cval,
			     int channel, int index, int *value)
{
	int err;

	if (cval->cached & (1 << channel)) {
		*value = cval->cache_val[index];
		return 0;
	}
	err = s18i6_ctl_urb(cval, channel, value);
	if (err < 0) {
		if (!cval->mixer->ignore_ctl_error)
			snd_printd(KERN_ERR "cannot get current value for control %d ch %d: err = %d\n",
				   cval->control, channel, err);
		return err;
	}

	if (cval->id == 0x0a && cval->control == 0x01) {
		/* Scarlett mute */
		*value = !(*value); // amixer inverse boolean but device U8
	}
	if (cval->val_type == USB_MIXER_U8 && (cval->id == 0x33 || cval->id == 0x32)) {
		/* Scarlett mixer-in and route-source enum quirk */
		if (*value >= 0x20) { *value = -1; }
	}

	cval->cached |= 1 << channel;
	cval->cache_val[index] = *value;
	return 0;
}

static int s18i6_set_cur_mix_value(struct usb_mixer_elem_info *cval, int channel,
			     int index, int value)
{
	int err;
	unsigned int read_only = (channel == 0) ?
		cval->master_readonly :
		cval->ch_readonly & (1 << (channel - 1));

	if (read_only) {
		snd_printdd(KERN_INFO "%s(): channel %d of control %d is read_only\n",
			    __func__, channel, cval->control);
		return 0;
	}
	if (cval->id == 0x0a && cval->control == 0x01) {
		value = !(value); // amixer: inverse boolean but device U8
	}

	if (cval->id == 0x3c) // mixer-matrix volume
		err = set_ctl_value(cval, cval->control, value);
	else
		err = set_ctl_value(cval, (cval->control << 8) | channel, value);

	if (cval->id == 0x0a && cval->control == 0x01) {
		value = !(value); // amixer: inverse boolean but device U8
	}

	if (err < 0)
		return err;
	cval->cached |= 1 << channel;
	cval->cache_val[index] = value;
	return 0;
}


static int get_s18i6_get_min_max(struct usb_mixer_elem_info *cval, int mark_initialized)
{
	cval->min = 0;
	cval->max = cval->min + 1;
	cval->res = 1;
	cval->dBmin = cval->dBmax = 0;
	cval->initialized = mark_initialized;

	if (cval->val_type == USB_MIXER_BOOLEAN ||
	    cval->val_type == USB_MIXER_INV_BOOLEAN) {
		;
	} else if (cval->id == 0x0a && cval->control == 0x01) {
		/* mute -- which are actually U8's */
		;
	} else if (cval->val_type == USB_MIXER_U8 && cval->id == 0x28) {
		/* clock source */
		cval->min = 0x1;
		cval->max = 0x3;
	} else if (cval->control == 0x02 && cval->id == 0x0a) {
		/* bus volume */
		cval->min = -32768;
		cval->max = 1536;
		cval->res = 256;
		cval->dBmin = -128;
		cval->dBmax = 6;
	} else if (cval->id == 0x3c) {
		int in = (cval->control >> 3) &0xff;
		int out = (cval->control) &0x07;
		/* mixer volume */
		cval->min = -32768;
		cval->max = 1536;
		cval->res = 256;
		cval->dBmin = -128;
		cval->dBmax = 6;

		/* initialize mixer-matrix -- can not be queried */
		s18i6_set_cur_mix_value(cval, 0, 0, (out < 2 && (in%2) == out)? 0: -32768);

	} else if (cval->val_type == USB_MIXER_U8 && cval->id == 0x33) {
		/* bus assignment */
		cval->min = -1;
		cval->max = 0x1d;

		/* initialize routes -- can not be queried */
		switch (cval->cmask) {
			case 0:
				s18i6_set_cur_mix_value(cval, 0, 0, 0x18); // Mon L   <- Mix1
				break;
			case 1:
				s18i6_set_cur_mix_value(cval, 1, 0, 0x19); // Mon R   <- Mix2
				break;
			case 2:
				s18i6_set_cur_mix_value(cval, 2, 0, 0x18); // Phon L  <- Mix1
				break;
			case 4:
				s18i6_set_cur_mix_value(cval, 3, 0, 0x19); // Phon R  <- Mix2
				break;
			case 8:
				s18i6_set_cur_mix_value(cval, 4, 0, -1);   // SPDIF L <- off
				break;
			case 16:
				s18i6_set_cur_mix_value(cval, 5, 0, -1);   // SPDIF R <- off
				break;
			default:
				break;
		}

	} else if (cval->val_type == USB_MIXER_U8 && cval->id == 0x32) {
		/* mixer assignment */
		cval->min = -1;
		cval->max = 0x17;
	} else {
		cval->max = 0xffff;
	}
	return 0;
}


static int s18i6_info(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_info *uinfo)
{
	struct usb_mixer_elem_info *cval = kcontrol->private_data;

	if (!cval->initialized) {
		get_s18i6_get_min_max(cval, 1);
	}

	if (cval->val_type == USB_MIXER_U8 && (cval->id == 0x33 || cval->id == 0x32)) {
		/* Mixer Inputs and Route Source */
		static const char *texts[31] = {
			"Off", // 'off' == 0xff
			"DAW1", "DAW2", "DAW3", "DAW4", "DAW5", "DAW6",
			"ANALG1", "ANALG2", "ANALG3", "ANALG4",
			"ANALG5", "ANALG6", "ANALG7", "ANALG8",
			"SPDIF1", "SPDIF2",
			"ADAT1", "ADAT2", "ADAT3", "ADAT4",
			"ADAT5", "ADAT6", "ADAT7", "ADAT8",
			"Mix1", "Mix2", "Mix3", "Mix4", "Mix5", "Mix6"
		};

		uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
		uinfo->count = 1;
		uinfo->value.enumerated.items = cval->id == 0x33 ? 31 : 25;
		if (uinfo->value.enumerated.item > uinfo->value.enumerated.items - 1) {
			uinfo->value.enumerated.item = uinfo->value.enumerated.items - 1;
		}
		strcpy(uinfo->value.enumerated.name, texts[uinfo->value.enumerated.item]);
		return 0;
	}

	if (cval->val_type == USB_MIXER_U8 && cval->id == 0x28) {
		/* clock select */
		static const char *texts[3] = {
				       "Internal",
				       "S/PDIF",
				       "ADAT"
		};
		uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
		uinfo->count = 1;
		uinfo->value.enumerated.items = 3;
		if (uinfo->value.enumerated.item > uinfo->value.enumerated.items - 1) {
			uinfo->value.enumerated.item = uinfo->value.enumerated.items - 1;
		}
		strcpy(uinfo->value.enumerated.name, texts[uinfo->value.enumerated.item]);
		return 0;
	}

	if (cval->val_type == USB_MIXER_BOOLEAN && cval->id == 0x01) {
		/* Impedance */
		static const char *texts[2] = {
				       "Line",
				       "Instrument (Hi-Z)"
		};
		uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
		uinfo->count = 1;
		uinfo->value.enumerated.items = 2;
		if (uinfo->value.enumerated.item > uinfo->value.enumerated.items - 1) {
			uinfo->value.enumerated.item = uinfo->value.enumerated.items - 1;
		}
		strcpy(uinfo->value.enumerated.name, texts[uinfo->value.enumerated.item]);
		return 0;
	}

	uinfo->count = cval->channels;

	if (cval->id == 0x0a && cval->control == 0x01) {
		/* mute */
		uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
		uinfo->value.integer.min = 0;
		uinfo->value.integer.max = 1;
		return 0;
	}

	if (cval->val_type == USB_MIXER_BOOLEAN ||
	    cval->val_type == USB_MIXER_INV_BOOLEAN) {
		uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
		uinfo->value.integer.min = 0;
		uinfo->value.integer.max = 1;
	} else {
		uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
		uinfo->value.integer.min = 0;
		uinfo->value.integer.max = (cval->max - cval->min + cval->res - 1) / cval->res;
	}

	return 0;
}

static int s18i6_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct usb_mixer_elem_info *cval = kcontrol->private_data;
	int c, cnt, val, err;
	ucontrol->value.integer.value[0] = cval->min;
	if (cval->cmask) {
		cnt = 0;
		for (c = 0; c < S18I6_MAX_CHANNELS; c++) {
			if (!(cval->cmask & (1 << c)))
				continue;
			err = s18i6_get_cur_mix_value(cval, c + 1, cnt, &val);
			if (err < 0)
				return cval->mixer->ignore_ctl_error ? 0 : err;
			val = get_relative_value(cval, val);
			ucontrol->value.integer.value[cnt] = val;

			if (
					(cval->val_type == USB_MIXER_BOOLEAN && cval->id == 0x01)
					|| (cval->val_type == USB_MIXER_U8 && (cval->id == 0x33 || cval->id == 0x32))
					) {
				ucontrol->value.enumerated.item[cnt] = val;
			}
			if (cval->val_type == USB_MIXER_U8 && (cval->id == 0x33 || cval->id == 0x32)) {
				ucontrol->value.enumerated.item[cnt] = val;
			}
			cnt++;
		}
		return 0;
	} else {
		/* master channel */
		err = s18i6_get_cur_mix_value(cval, 0, 0, &val);
		if (err < 0)
			return cval->mixer->ignore_ctl_error ? 0 : err;
		val = get_relative_value(cval, val);
		ucontrol->value.integer.value[0] = val;

		if (
				(cval->val_type == USB_MIXER_U8 && cval->id == 0x28)
				|| (cval->val_type == USB_MIXER_U8 && (cval->id == 0x33 || cval->id == 0x32))
				){
			ucontrol->value.enumerated.item[0] = val;
		}
	}
	return 0;
}

static int s18i6_put(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct usb_mixer_elem_info *cval = kcontrol->private_data;
	int c, cnt, val, oval, err;
	int changed = 0;

	if (cval->cmask) {
		cnt = 0;
		for (c = 0; c < S18I6_MAX_CHANNELS; c++) {
			if (!(cval->cmask & (1 << c)))
				continue;
			err = s18i6_get_cur_mix_value(cval, c + 1, cnt, &oval);
			if (err < 0)
				return cval->mixer->ignore_ctl_error ? 0 : err;
			val = ucontrol->value.integer.value[cnt];
			val = get_abs_value(cval, val);
			if (oval != val) {
				s18i6_set_cur_mix_value(cval, c + 1, cnt, val);
				changed = 1;
			}
			cnt++;
		}
	} else {
		/* master channel */
		err = s18i6_get_cur_mix_value(cval, 0, 0, &oval);
		if (err < 0)
			return cval->mixer->ignore_ctl_error ? 0 : err;
		val = ucontrol->value.integer.value[0];
		val = get_abs_value(cval, val);
		if (val != oval) {
			s18i6_set_cur_mix_value(cval, 0, 0, val);
			changed = 1;
		}
	}
	return changed;
}

static int s18i6_func_info(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_info *uinfo)
{
	struct usb_mixer_elem_info *cval = kcontrol->private_data;
	switch(cval->id) {
		case 1:
			{
				static const char *texts[2] = { "Save", "Save" };
				uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
				uinfo->count = 1;
				uinfo->value.enumerated.items = 2;
				strcpy(uinfo->value.enumerated.name, texts[uinfo->value.enumerated.item]);
			}
			break;
		default:
			uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
			uinfo->value.integer.min = 0;
			uinfo->value.integer.max = 1;
			break;
	}
	return 0;
}

static int s18i6_func_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = 0;
	ucontrol->value.enumerated.item[0] = 0;
	return 0;
}

static int s18i6_func_put(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct usb_mixer_elem_info *cval = kcontrol->private_data;
	switch (cval->id) {
		case 1:
			{
				unsigned char buf[2];
				buf[0] = 0xa5;
				buf[1] = 0x00;
				if (!set_ctl_urb2(cval->mixer->chip, UAC2_CS_MEM, 0x005a, 0x3c00, buf, 1))
					snd_printk(KERN_INFO "Scarlett 18i6: Saved settings to hardware.\n");
			}
			break;
		default:
			snd_printk(KERN_ERR "Scarlett 18i6: undefined function.\n");
			break;
	}
	return 0;
}


static int s18i6_peak_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct usb_mixer_elem_info *cval = kcontrol->private_data;
	struct snd_usb_audio *chip = cval->mixer->chip;
	int ret, size, i;
	unsigned char buf[36];

	size = cval->channels * sizeof(__u16);
	memset(buf, 0, sizeof(buf));

	ret = get_ctl_urb2(chip, UAC2_CS_MEM, cval->control, (cval->id << 8), buf, size);

	if (ret < 0) {
		for (i = 0; i < cval->channels; ++i) {
			ucontrol->value.integer.value[i] = 0;
		}
		return ret;
	}

	for (i = 0; i < cval->channels; ++i) {
#ifdef LOGSCALEPEAK /* this won't ever fly -- but you get the idea :) */
		int v = snd_usb_combine_bytes(&buf[2*i], sizeof(__u16));
		if (v>0) {
			float db = 20.0 * log10(v / 65536.0);
			ucontrol->value.integer.value[i] = db;
		} else {
			ucontrol->value.integer.value[i] = -97;
		}
#else
		ucontrol->value.integer.value[i] = snd_usb_combine_bytes(&buf[2*i], sizeof(__u16));
#endif
	}
	return 0;
}

static int s18i6_peak_info(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_info *uinfo)
{
	struct usb_mixer_elem_info *cval = kcontrol->private_data;
	switch (cval->control) {
		case 0x0003: // DAW
			uinfo->count = 6;
			break;
		case 0x0001: // MIX
			uinfo->count = 6;
			break;
		case 0x0000: // Inputs
			uinfo->count = 18;
			break;
		default:
			uinfo->count = 0;
			break;
	}

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 0xffff;
	uinfo->value.integer.step = 1;
	return 0;
}


#ifdef LOGSCALEPEAK
static const DECLARE_TLV_DB_SCALE(db_scale_s18i6_peak, -9632, 100, 0);
#endif

static struct snd_kcontrol_new usb_s18i6_peakmeter_ctl = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.name = "",
	.info = s18i6_peak_info,
	.get =  s18i6_peak_get,
#ifdef LOGSCALEPEAK
	.access = SNDRV_CTL_ELEM_ACCESS_READ | SNDRV_CTL_ELEM_ACCESS_TLV_READ,
	.tlv = { .p = db_scale_s18i6_peak }
#else
	.access = SNDRV_CTL_ELEM_ACCESS_READ,
#endif
};

static const DECLARE_TLV_DB_SCALE(db_scale_s18i6_gain, -12800, 100, 0);

static struct snd_kcontrol_new usb_s18i6_volume_ctl = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE | SNDRV_CTL_ELEM_ACCESS_TLV_READ,
	.name = "",
	.info = s18i6_info,
	.get =  s18i6_get,
	.put =  s18i6_put,
	.tlv = { .p = db_scale_s18i6_gain }
};

static struct snd_kcontrol_new usb_s18i6_ctl = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.name = "",
	.info = s18i6_info,
	.get =  s18i6_get,
	.put =  s18i6_put,
};

static struct snd_kcontrol_new usb_s18i6_func = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.name = "",
	.info = s18i6_func_info,
	.get =  s18i6_func_get,
	.put =  s18i6_func_put,
};



static int s18i6_add_func(struct usb_mixer_interface *mixer, int func_id, char *name)
{
	struct usb_mixer_elem_info *cval;
	struct snd_kcontrol *kctl;

	cval = kzalloc(sizeof(*cval), GFP_KERNEL);
	if (! cval)
		return -ENOMEM;

	cval->mixer = mixer;
	cval->id = func_id;
	cval->control = 0;
	cval->val_type = USB_MIXER_BOOLEAN;
	cval->channels = 1;
	cval->cmask = 0;

	cval->min = 0;
	cval->max = 1;
	cval->res = 1;
	cval->dBmin = cval->dBmax = 0;
	cval->initialized = 1;

	kctl = snd_ctl_new1(&usb_s18i6_func, cval);

	if (! kctl) {
		snd_printk(KERN_ERR "cannot malloc kcontrol\n");
		kfree(cval);
		return -ENOMEM;
	}
	kctl->private_free = usb_mixer_elem_free;

	sprintf(kctl->id.name, "%s", name);

	snd_printdd(KERN_INFO "[%d] MU [%s] ch = %d, val = %d/%d\n",
				cval->id, kctl->id.name, cval->channels, cval->min, cval->max);

	snd_usb_mixer_add_control(mixer, kctl);

	return 0;
}


static int s18i6_add_peak_meter(struct usb_mixer_interface *mixer, int wValue, int chn, char *name)
{
	struct usb_mixer_elem_info *cval;
	struct snd_kcontrol *kctl;

	cval = kzalloc(sizeof(*cval), GFP_KERNEL);
	if (! cval)
		return -ENOMEM;

	cval->mixer = mixer;
	cval->id = 0x3c;
	cval->control = wValue;
	cval->val_type = USB_MIXER_S16;
	cval->channels = chn;
	cval->cmask = (1<<(chn+1))-1;

	cval->min = 0;
	cval->max = 65535;
	cval->res = 1;
	cval->dBmin = -128;
	cval->dBmax = 0;
	cval->initialized = 1;

	kctl = snd_ctl_new1(&usb_s18i6_peakmeter_ctl, cval);

	if (! kctl) {
		snd_printk(KERN_ERR "cannot malloc kcontrol\n");
		kfree(cval);
		return -ENOMEM;
	}
	kctl->private_free = usb_mixer_elem_free;

	sprintf(kctl->id.name, "%s", name);

	snd_printdd(KERN_INFO "[%d] MU [%s] ch = %d, val = %d/%d\n",
				cval->id, kctl->id.name, cval->channels, cval->min, cval->max);

	snd_usb_mixer_add_control(mixer, kctl);

	return 0;
}

static int s18i6_first_time_reset(struct usb_mixer_interface *mixer)
{
	int i;
	unsigned char buf[2];
	/* routes and mute registers do not represent the actual state of the
	 * device after power-cycles.
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

	memset(buf, 0, sizeof(buf));
	get_ctl_urb2(mixer->chip, UAC2_CS_CUR, 0x0106, 0x0a00, buf, 1);
	if (buf[0] == 0x01) {
		snd_printk(KERN_INFO "Scarlett 18i6: already initialized (no device power-cycle).\n");
		return 0;
	}

	snd_printk(KERN_INFO "Scarlett 18i6: initializing 18i6 mixer after device power-cycle.\n");

	// mark chip as initialized
	buf[0] = 0x01;
	set_ctl_urb2(mixer->chip, UAC2_CS_CUR, 0x0106, 0x0a00, buf, 2);

#if 0
	memset(buf, 0, sizeof(buf));
	get_ctl_urb2(mixer->chip, UAC2_CS_CUR, 0x0106, 0x0a00, buf, 1);
	snd_printk(KERN_ERR "18i6: check marker %x %x\n", buf[0], buf[1]);
#endif

	memset(buf, 0, sizeof(buf));

	/* reset device */
	for (i = 0; i < 18; ++i) {
		buf[0] = 6 + i;
		set_ctl_urb2(mixer->chip, UAC2_CS_CUR, i, 0x3400, buf, 2);
	}

	/* output buses and matrix mixer gains are skipped here
	 * they can never be queried and are always re-initialized during setup
	 */

	/* mixer inputs */
	for (i = 0; i < 8; ++i) { // Analog
		buf[0] = 6 + i;
		set_ctl_urb2(mixer->chip, UAC2_CS_CUR, 0x0600 + i , 0x3200, buf, 2);
	}
	for (i = 0; i < 6; ++i) { // ADAT
		buf[0] = 0x10 + i;
		set_ctl_urb2(mixer->chip, UAC2_CS_CUR, 0x0600 + i + 8 , 0x3200, buf, 2);
	}
	// SPDIF
	buf[0] = 0x0e; set_ctl_urb2(mixer->chip, UAC2_CS_CUR, 0x060e, 0x3200, buf, 2);
	buf[0] = 0x0f; set_ctl_urb2(mixer->chip, UAC2_CS_CUR, 0x060f, 0x3200, buf, 2);
	// DAW
	buf[0] = 0x00; set_ctl_urb2(mixer->chip, UAC2_CS_CUR, 0x0610, 0x3200, buf, 2);
	buf[0] = 0x01; set_ctl_urb2(mixer->chip, UAC2_CS_CUR, 0x0611, 0x3200, buf, 2);

	/* unmute buses */
	buf[0] = 0;
	for (i = 1; i < 5; ++i) { // ADAT
		set_ctl_urb2(mixer->chip, UAC2_CS_CUR, 0x0100 + i, 0x0a00, buf, 2);
	}

	/* impedance & 8i6 hi/low gain */
	buf[0] = 0;
	for (i = 1; i < 5; ++i) { // ADAT
		set_ctl_urb2(mixer->chip, UAC2_CS_CUR, 0x0800 + i, 0x0100, buf, 2);
	}

	return 0;
}

static int s18i6_add_mix_ctl(
		struct usb_mixer_interface *mixer,
		int wIndex, int wValue,
		int type, int cmask, int channels,
		char *name)
{
	struct usb_mixer_elem_info *cval;
	struct snd_kcontrol *kctl;

	cval = kzalloc(sizeof(*cval), GFP_KERNEL);
	if (! cval)
		return -ENOMEM;

	cval->mixer = mixer;
	cval->id = wIndex;
	cval->control = wValue;
	cval->val_type = type;
	cval->cmask = cmask;
	cval->channels = channels;

	get_s18i6_get_min_max(cval,
			(cval->id != 0x33 && cval->id != 0x3c) ? 1 : 0);

	if ( (wIndex == 0x0a && wValue == 0x02) || wIndex == 0x3c)
		kctl = snd_ctl_new1(&usb_s18i6_volume_ctl, cval);
	else
		kctl = snd_ctl_new1(&usb_s18i6_ctl, cval);

	if (! kctl) {
		snd_printk(KERN_ERR "cannot malloc kcontrol\n");
		kfree(cval);
		return -ENOMEM;
	}
	kctl->private_free = usb_mixer_elem_free;

	sprintf(kctl->id.name, "%s", name);

	snd_printdd(KERN_INFO "[%d] MU [%s] ch = %d, val = %d/%d\n",
				cval->id, kctl->id.name, cval->channels, cval->min, cval->max);

	snd_usb_mixer_add_control(mixer, kctl);

	return 0;
}

#define S18ADD(wI, wV, TY, CM, CN, LB) \
	if ((err = s18i6_add_mix_ctl(mixer, wI, wV, TY, CM, CN, LB))) { return err; }


static int s18i6_create_controls(struct usb_mixer_interface *mixer)
{
	int err, i, o;

	S18ADD(0x28, 0x01, USB_MIXER_U8, 0, 1, "Clock Selector");

	S18ADD(0x01, 0x09, USB_MIXER_BOOLEAN, 1, 1, "Impedance 1")
	S18ADD(0x01, 0x09, USB_MIXER_BOOLEAN, 2, 1, "Impedance 2");

	S18ADD(0x0a, 0x01, USB_MIXER_U8,  3, 2, "Mute Monitor");
	S18ADD(0x0a, 0x01, USB_MIXER_U8, 12, 2, "Mute Phones");
	S18ADD(0x0a, 0x01, USB_MIXER_U8,  0, 1, "Mute Master");

	/* bus attenuation */
	S18ADD(0x0a, 0x02, USB_MIXER_S16,  0, 1, "Att Master Volume");
	S18ADD(0x0a, 0x02, USB_MIXER_S16,  3, 2, "Att Monitor Volume");
	S18ADD(0x0a, 0x02, USB_MIXER_S16, 12, 2, "Att Phones Volume");

	/* output bus routing */
	S18ADD(0x33, 0x00, USB_MIXER_U8, 0, 1, "Bus Monitor L");
	S18ADD(0x33, 0x00, USB_MIXER_U8, 1, 1, "BUS Monitor R");

	S18ADD(0x33, 0x00, USB_MIXER_U8, 2, 1, "Bus Phones L");
	S18ADD(0x33, 0x00, USB_MIXER_U8, 4, 1, "Bus Phones R");

	S18ADD(0x33, 0x00, USB_MIXER_U8, 8, 1, "Bus SPDIF L");
	S18ADD(0x33, 0x00, USB_MIXER_U8, 16, 1, "Bus SPDIF R");

	/* mixer source selection */
	S18ADD(0x32, 0x06, USB_MIXER_U8, 0, 1, "Mixer In 01");
	for (i = 0; i < 17; ++i) {
		char mx[16];
		sprintf(mx, "Mixer In %02d",i+2);
		S18ADD(0x32, 0x06, USB_MIXER_U8, 1<<i, 1, mx);
		if (err) return err;
	}

	/* mixer matrix */
	for (i = 0; i < 18; ++i) {
		for (o = 0; o < 6; ++o) {
			char mx[16];
			int mtx = (i<<3) + (o&0x07);
			sprintf(mx, "Mx%02d>%d Volume",i+1,o+1);
			S18ADD(0x3c, 0x0100 + mtx, USB_MIXER_S16, 0, 1, mx);
			if (err) return err;
		}
	}

	s18i6_add_peak_meter(mixer, 0x0000, 18, "Mtr Input");
	s18i6_add_peak_meter(mixer, 0x0001,  6, "Mtr Mix");
	s18i6_add_peak_meter(mixer, 0x0003,  6, "Mtr DAW");

	s18i6_add_func(mixer, 1, "Save to HW");

	return 0;
}


/*
 * Create a mixer for the Focusrite(R) Scarlett
 */
int scarlett_mixer_create(struct snd_usb_audio *chip,
				       struct usb_interface *iface,
				       struct usb_driver *driver,
				       const struct snd_usb_audio_quirk *quirk)
{
	int ctrlif = quirk->ifnum;
	int ignore_error = 0;

	static struct snd_device_ops dev_ops = {
		.dev_free = snd_usb_mixer_dev_free
	};
	struct usb_mixer_interface *mixer;
	struct snd_info_entry *entry;
	int err = 0;

	if (quirk->ifnum < 0)
		return 0;

	strcpy(chip->card->mixername, "Scarlett Mixer");

	mixer = kzalloc(sizeof(*mixer), GFP_KERNEL);
	if (!mixer)
		return -ENOMEM;
	mixer->chip = chip;
	mixer->ignore_ctl_error = ignore_error;
	mixer->id_elems = kcalloc(MAX_ID_ELEMS, sizeof(*mixer->id_elems),
				  GFP_KERNEL);
	if (!mixer->id_elems) {
		kfree(mixer);
		return -ENOMEM;
	}

	mixer->hostif = &usb_ifnum_to_if(chip->dev, ctrlif)->altsetting[0];
	mixer->protocol = UAC_VERSION_2;

	if (s18i6_create_controls(mixer))
		goto _error;

	s18i6_first_time_reset(mixer);

	err = snd_device_new(chip->card, SNDRV_DEV_LOWLEVEL, mixer, &dev_ops);
	if (err < 0)
		goto _error;

	if (list_empty(&chip->mixer_list) &&
	    !snd_card_proc_new(chip->card, "usbmixer", &entry))
		snd_info_set_text_ops(entry, chip, snd_usb_mixer_proc_read);

	list_add(&mixer->list, &chip->mixer_list);

	return 0;

_error:
	snd_usb_mixer_free(mixer);
	return err;
	return 0;
}
