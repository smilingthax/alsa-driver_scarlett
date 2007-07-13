#define __NO_VERSION__
/*
 *  cmi_mixer.c - Driver for C-Media CMI8788 PCI soundcards.
 *
 *      Copyright (C) 2005  C-media support
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
 *  Revision history
 *
 *    Weifeng Sui <weifengsui@163.com>
 */

#include <sound/driver.h>
#include <sound/core.h>
#include <sound/control.h>
#include "cmi8788.h"


/* record switch */
static struct cmi8788_input_mux cmi8788_basic_input = {
	.num_items = 2,
	.items = {
		{ "AC97-Mic",    CAPTURE_AC97_MIC },
		{ "DirectLinein",CAPTURE_DIRECT_LINE_IN },
		/* { "AC97-Linein", CAPTURE_AC97_LINEIN }, */
	}
};

static int snd_cmi8788_capture_source_info(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items = cmi8788_basic_input.num_items;

	if (uinfo->value.enumerated.item >= uinfo->value.enumerated.items)
		uinfo->value.enumerated.item = uinfo->value.enumerated.items - 1;

	strcpy(uinfo->value.enumerated.name, cmi8788_basic_input.items[uinfo->value.enumerated.item].label);
	return 0;
}

static int snd_cmi8788_capture_source_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct cmi8788 *chip = snd_kcontrol_chip(kcontrol);

	if (chip->capture_source >= CAPTURE_MAX_SOURCE)
		chip->capture_source = CAPTURE_DIRECT_LINE_IN; /* CAPTURE_AC97_LINEIN; */
	ucontrol->value.enumerated.item[0] = chip->capture_source;
	return 0;
}

static int snd_cmi8788_capture_source_put(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct cmi8788 *chip = snd_kcontrol_chip(kcontrol);
	int val;

	val = ucontrol->value.enumerated.item[0];
	if (val >= CAPTURE_MAX_SOURCE)
		val = CAPTURE_DIRECT_LINE_IN; /* CAPTURE_AC97_LINEIN; */
	if (val != chip->capture_source) {
		chip->capture_source = val;
		return 1;
	}
	return 0;
}


static int snd_cmi8788_playback_info(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_info *uinfo)
{
	struct cmi8788 *chip = snd_kcontrol_chip(kcontrol);
	struct cmi_codec *codec;

	switch (kcontrol->private_value) {
	case PLAYBACK_MASTER_VOL:
	case PLAYBACK_FRONT_VOL:
	default:
		codec = &chip->codec_list[0];
		break;
	case PLAYBACK_SIDE_VOL:
		codec = &chip->codec_list[1];
		break;
	case PLAYBACK_CENTER_VOL:
		codec = &chip->codec_list[2];
		break;
	case PLAYBACK_BACK_VOL:
		codec = &chip->codec_list[3];
		break;
	}

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 255;

	if (codec->mixer_ops.get_info) {
		int min_vol = 0, max_vol = 255;
		if (0 == codec->mixer_ops.get_info(codec, &min_vol, &max_vol)) {
			uinfo->value.integer.min = min_vol;
			uinfo->value.integer.max = max_vol;
		}
	}
	return 0;
}

static int snd_cmi8788_playback_get_volume(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct cmi8788 *chip = snd_kcontrol_chip(kcontrol);
	struct cmi_codec *codec;

	switch (kcontrol->private_value) {
	case PLAYBACK_MASTER_VOL:
	case PLAYBACK_FRONT_VOL:
	default:
		codec = &chip->codec_list[0];
		break;
	case PLAYBACK_SIDE_VOL:
		codec = &chip->codec_list[1];
		break;
	case PLAYBACK_CENTER_VOL:
		codec = &chip->codec_list[2];
		break;
	case PLAYBACK_BACK_VOL:
		codec = &chip->codec_list[3];
		break;
	}

	if (codec->mixer_ops.get_volume) {
		int l_vol = 0, r_vol = 0;

		codec->mixer_ops.get_volume(codec, &l_vol, &r_vol);

		ucontrol->value.integer.value[0] = l_vol;
		ucontrol->value.integer.value[1] = r_vol;
	}
	return 0;
}

static int snd_cmi8788_playback_put_volume(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct cmi8788 *chip = snd_kcontrol_chip(kcontrol);
	struct cmi_codec *codec;
	int l_vol = 0, r_vol = 0;
	int i, setall = 0, change;

	/* set CODEC register */
	/* codec->mixer_ops.put_volume(codec, vol); */
	switch (kcontrol->private_value) {
	case PLAYBACK_MASTER_VOL:
		setall = 1;
		/* fall through */
	case PLAYBACK_FRONT_VOL:
	default:
		codec = &chip->codec_list[0];
		break;
	case PLAYBACK_SIDE_VOL:
		codec = &chip->codec_list[1];
		break;
	case PLAYBACK_CENTER_VOL:
		codec = &chip->codec_list[2];
		break;
	case PLAYBACK_BACK_VOL:
		codec = &chip->codec_list[3];
		break;
	}

	if (codec->mixer_ops.get_volume)
		codec->mixer_ops.get_volume(codec, &l_vol, &r_vol);
	change = l_vol != ucontrol->value.integer.value[0] ||
		 r_vol != ucontrol->value.integer.value[1];
	l_vol = ucontrol->value.integer.value[0];
	r_vol = ucontrol->value.integer.value[1];
	if (change) {
		if (!setall) {
			if (codec->mixer_ops.set_volume && chip->playback_volume_init) {
				codec->mixer_ops.set_volume(codec, l_vol, r_vol);
			}
		} else {
			codec = chip->codec_list;
			for (i = 0; i < chip->num_codecs; i++) {
				if (codec->mixer_ops.set_volume && chip->playback_volume_init) {
					codec->mixer_ops.set_volume(codec, l_vol, r_vol);
					codec++;
				}
			}
		}
	}
	return change;
}


static int snd_cmi8788_capture_info(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_info *uinfo)
{
	struct cmi8788 *chip = snd_kcontrol_chip(kcontrol);
	struct cmi_codec *codec;

	switch (kcontrol->private_value) {
	case CAPTURE_MIC_VOL:
	default:
		codec = &chip->ac97_codec_list[0];
		codec->volume_opera_source = MIC_VOL_SLIDER;
		break;
	case CAPTURE_LINEIN_VOL:
		codec = &chip->codec_list[4];
		break;
	case CAPTURE_AC97LINE_VOL:
		codec = &chip->ac97_codec_list[0];
		codec->volume_opera_source = LINEIN_VOL_SLIDER;
		break;
	}

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 255;

	if (codec->mixer_ops.get_info) {
		int min_vol = 0, max_vol = 255;
		if (0 == codec->mixer_ops.get_info(codec, &min_vol, &max_vol)) {
			uinfo->value.integer.min = min_vol;
			uinfo->value.integer.max = max_vol;
		}
	}
	return 0;
}

static int snd_cmi8788_capture_get_volume(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct cmi8788 *chip = snd_kcontrol_chip(kcontrol);
	struct cmi_codec *codec;
	int l_vol = 0, r_vol = 0;

	switch (kcontrol->private_value) {
	case CAPTURE_MIC_VOL:
	default:
		codec = &chip->ac97_codec_list[0];
		codec->volume_opera_source = MIC_VOL_SLIDER;
		break;
	case CAPTURE_LINEIN_VOL:
		codec = &chip->codec_list[4];
		break;
	case CAPTURE_AC97LINE_VOL:
		codec = &chip->ac97_codec_list[0];
		codec->volume_opera_source = LINEIN_VOL_SLIDER;
		break;
	}
	ucontrol->value.integer.value[0] = l_vol;
	ucontrol->value.integer.value[1] = r_vol;
	if (codec->mixer_ops.get_volume) {
		codec->mixer_ops.get_volume(codec, &l_vol, &r_vol);
		ucontrol->value.integer.value[0] = l_vol;
		ucontrol->value.integer.value[1] = r_vol;
	}
	return 0;
}

static int snd_cmi8788_capture_put_volume(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct cmi8788 *chip = snd_kcontrol_chip(kcontrol);
	struct cmi_codec *codec;
	int l_vol = 0, r_vol = 0;
	int change;

	/* set CODEC register */
	switch(kcontrol->private_value) {
	case CAPTURE_MIC_VOL:
	default:
		codec = &chip->ac97_codec_list[0];
		codec->volume_opera_source = MIC_VOL_SLIDER;
		break;
	case CAPTURE_LINEIN_VOL:
		codec = &chip->codec_list[4];
		break;
	case CAPTURE_AC97LINE_VOL:
		codec = &chip->ac97_codec_list[0];
		codec->volume_opera_source = LINEIN_VOL_SLIDER;
		break;
	}

	if (codec->mixer_ops.get_volume)
		codec->mixer_ops.get_volume(codec, &l_vol, &r_vol);
	change = l_vol != ucontrol->value.integer.value[0] ||
		 r_vol != ucontrol->value.integer.value[1];
	l_vol = ucontrol->value.integer.value[0];
	r_vol = ucontrol->value.integer.value[1];
	if (change) {
		if (codec->mixer_ops.set_volume && chip->capture_volume_init) {
			codec->mixer_ops.set_volume(codec, l_vol, r_vol);
		}
	}
	return change;
}


static struct snd_kcontrol_new snd_cmi8788_capture_source_ctl __devinitdata = {
	.name  = "Capture Source",
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.info  = snd_cmi8788_capture_source_info,
	.get   = snd_cmi8788_capture_source_get,
	.put   = snd_cmi8788_capture_source_put,
};

#define CMI8788_CAPTURE_VOL_STEREO(xname, pri_value)  \
{                                             \
	.name  = xname,                           \
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,      \
	.info  = snd_cmi8788_capture_info,        \
	.get   = snd_cmi8788_capture_get_volume,  \
	.put   = snd_cmi8788_capture_put_volume,  \
	.private_value = (unsigned long)pri_value,\
}

static struct snd_kcontrol_new snd_cmi8788_capture_mixers[] __devinitdata={
	CMI8788_CAPTURE_VOL_STEREO("AC97-Mic Capture Volume",    CAPTURE_MIC_VOL ),
	CMI8788_CAPTURE_VOL_STEREO("DirectLinein Capture Volume",CAPTURE_LINEIN_VOL ),
	/* CMI8788_CAPTURE_VOL_STEREO("AC97-Linein Capture Volume", CAPTURE_AC97LINE_VOL ), */
};

#define CMI8788_PLAYBACK_VOL_STEREO(xname, pri_value)  \
{                                             \
	.name  = xname,                           \
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,      \
	.info  = snd_cmi8788_playback_info,       \
	.get   = snd_cmi8788_playback_get_volume, \
	.put   = snd_cmi8788_playback_put_volume, \
	.private_value = (unsigned long)pri_value,\
}

static struct snd_kcontrol_new snd_cmi8788_playback_mixers[] __devinitdata={
	CMI8788_PLAYBACK_VOL_STEREO("Master Playback Volume",PLAYBACK_MASTER_VOL ),
	CMI8788_PLAYBACK_VOL_STEREO("AnalogFront Playback Volume", PLAYBACK_FRONT_VOL ),
	CMI8788_PLAYBACK_VOL_STEREO("AnalogSide Playback Volume",  PLAYBACK_SIDE_VOL),
	CMI8788_PLAYBACK_VOL_STEREO("AnalogCenter Playback Volume",PLAYBACK_CENTER_VOL ),
	CMI8788_PLAYBACK_VOL_STEREO("AnalogRear Playback Volume",  PLAYBACK_BACK_VOL ),
};


/*
 * constructor
 */
int __devinit snd_cmi8788_mixer_create(struct cmi8788 *chip)
{
	unsigned int idx;
	int i;
	struct cmi_codec *codec;
	int err;

	strcpy(chip->card->mixername, "C-Media PCI8788");

	/* for playback and capture CODEC */
	for (i = 0; i < chip->num_codecs; i++) {
		codec = &chip->codec_list[i];
		if (!codec->patch_ops.build_controls)
			continue;

		err = codec->patch_ops.build_controls(codec);
		if (err < 0)
			return err;
	}

	/* for AC97 CODEC */
	for (i = 0; i < chip->num_ac97_codecs; i++) {
		codec = &chip->ac97_codec_list[i];
		if (!codec->patch_ops.build_controls)
			continue;

		err = codec->patch_ops.build_controls(codec);
		if (err < 0)
			return err;
	}

	/* capture source control */
	err = snd_ctl_add(chip->card, snd_ctl_new1(&snd_cmi8788_capture_source_ctl, chip));
	/* capture volume control */
	for (idx = 0; idx < ARRAY_SIZE(snd_cmi8788_capture_mixers); idx++) {
		if ((err = snd_ctl_add(chip->card, snd_ctl_new1(&snd_cmi8788_capture_mixers[idx], chip))) < 0)
			return err;
	}
	/* playback volume control */
	for (idx = 0; idx < ARRAY_SIZE(snd_cmi8788_playback_mixers); idx++) {
		if ((err = snd_ctl_add(chip->card, snd_ctl_new1(&snd_cmi8788_playback_mixers[idx], chip))) < 0)
			return err;
	}

	return err;
}
