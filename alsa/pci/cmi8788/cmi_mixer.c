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
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <sound/core.h>
#include <sound/initval.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
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

#define GET_POINTER(pkcontrol)				\
	chip = snd_kcontrol_chip(pkcontrol);		\
	if (!chip)					\
		return -1;				\
	private_value = pkcontrol->private_value;	\
	codec = chip->codec_list;			\
	if (!codec)					\
		return -1;

static int snd_cmi8788_capture_source_info(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_info *uinfo)
{
	snd_cmi8788        *chip       = NULL;
	cmi_codec          *codec      = NULL;
	unsigned long       private_value = 0;

	if (!kcontrol || !uinfo)
		return -1;

	GET_POINTER(kcontrol);

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
	snd_cmi8788        *chip       = NULL;
	cmi_codec          *codec      = NULL;
	unsigned long       private_value = 0;

	if (!kcontrol || !ucontrol)
		return -1;

	GET_POINTER(kcontrol);

	if (chip->capture_source >= CAPTURE_MAX_SOURCE)
		chip->capture_source = CAPTURE_DIRECT_LINE_IN; /* CAPTURE_AC97_LINEIN; */

	ucontrol->value.enumerated.item[0] = chip->capture_source;
	/* ucontrol->value.enumerated.item[1] = chip->capture_source; */

	return 0;
}

static int snd_cmi8788_capture_source_put(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	snd_cmi8788        *chip       = NULL;
	cmi_codec          *codec      = NULL;
	unsigned long       private_value = 0;
	int val;

	if (!kcontrol || !ucontrol)
		return -1;

	GET_POINTER(kcontrol);

	val = CAPTURE_AC97_MIC;
	val = ucontrol->value.enumerated.item[0];

	if (val >= CAPTURE_MAX_SOURCE)
		val = CAPTURE_DIRECT_LINE_IN; /* CAPTURE_AC97_LINEIN; */

	if (chip->capture_source >= CAPTURE_MAX_SOURCE)
		chip->capture_source = CAPTURE_DIRECT_LINE_IN; /* CAPTURE_AC97_LINEIN; */

	if (val != chip->capture_source) {
		chip->capture_source = val;
		return 1;
	}

	return 0;
}


static int snd_cmi8788_playback_info(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_info *uinfo)
{
	snd_cmi8788        *chip       = NULL;
	cmi_codec          *codec      = NULL;
	unsigned long       private_value = 0;

	if (!kcontrol || !uinfo)
		return -1;

	GET_POINTER(kcontrol);

	switch (private_value) {
	case PLAYBACK_MASTER_VOL:
		codec = &chip->codec_list[0];
		break;
	case PLAYBACK_FRONT_VOL:
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
	default:
		codec = &chip->codec_list[0];
		break;
	}

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 255;

	if (codec && codec->mixer_ops.get_info) {
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
	snd_cmi8788        *chip       = NULL;
	cmi_codec          *codec      = NULL;
	unsigned long       private_value = 0;

	if (!kcontrol || !ucontrol)
		return -1;

	GET_POINTER(kcontrol);

	switch (private_value) {
	case PLAYBACK_MASTER_VOL:
		codec = &chip->codec_list[0];
		break;
	case PLAYBACK_FRONT_VOL:
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
	default:
		codec = &chip->codec_list[0];
		break;
	}

	if (codec && codec->mixer_ops.get_volume) {
		int l_vol = 0, r_vol = 0;

		codec->mixer_ops.get_volume(codec, &l_vol, &r_vol);

		ucontrol->value.integer.value[0] = l_vol;
		ucontrol->value.integer.value[1] = r_vol;
	}

	return 0;
}

static int snd_cmi8788_playback_put_volume(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	snd_cmi8788        *chip       = NULL;
	cmi_codec          *codec      = NULL;
	unsigned long       private_value = 0;
	int                 codec_num = 0;
	unsigned int idx;
	int l_vol = 0, r_vol = 0;
	int i = 0, setall = 0, change = 0;

	if (!kcontrol || !ucontrol)
		return -1;

	GET_POINTER(kcontrol);

	codec_num = chip->num_codecs;
	idx  = snd_ctl_get_ioff(kcontrol, &ucontrol->id);

	/* set CODEC register */
	/* codec->mixer_ops.put_volume(codec, vol); */
	switch (private_value) {
	case PLAYBACK_MASTER_VOL:
		setall = 1;
		codec = &chip->codec_list[0];
		break;
	case PLAYBACK_FRONT_VOL:
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
	default:
		codec = &chip->codec_list[0];
		break;
	}

	if (codec && codec->mixer_ops.get_volume)
		codec->mixer_ops.get_volume(codec, &l_vol, &r_vol);
	if (l_vol == ucontrol->value.integer.value[0] && r_vol == ucontrol->value.integer.value[1])
		change = 0;
	else
		change = 1;

	l_vol = ucontrol->value.integer.value[0];
	r_vol = ucontrol->value.integer.value[1];

	if (chip->first_set_playback_volume) {
		chip->first_set_playback_volume = 0;
		/*     return change; */
	}

	if (change) {
		if (!setall) {
			if (codec && codec->mixer_ops.set_volume && chip->playback_volume_init) {
				codec->mixer_ops.set_volume(codec, l_vol, r_vol);
			}
		} else {
			codec = chip->codec_list;
			for (i = 0; i < codec_num; i++) {
				if (codec && codec->mixer_ops.set_volume && chip->playback_volume_init) {
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
	snd_cmi8788        *chip       = NULL;
	cmi_codec          *codec      = NULL;
	unsigned long       private_value = 0;

	if (!kcontrol || !uinfo)
		return -1;

	GET_POINTER(kcontrol);

	switch (private_value) {
	case CAPTURE_MIC_VOL:
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
	default:
		codec = &chip->ac97_codec_list[0];
		codec->volume_opera_source = MIC_VOL_SLIDER;
		break;
	}

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 255;

	if (codec && codec->mixer_ops.get_info) {
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
	snd_cmi8788        *chip       = NULL;
	cmi_codec          *codec      = NULL;
	unsigned long       private_value = 0;
	int l_vol = 0, r_vol = 0;

	if (!kcontrol || !ucontrol)
		return -1;

	GET_POINTER(kcontrol);

	switch (private_value) {
	case CAPTURE_MIC_VOL:
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
	default:
		codec = &chip->ac97_codec_list[0];
		codec->volume_opera_source = MIC_VOL_SLIDER;
		break;
	}

	ucontrol->value.integer.value[0] = l_vol;
	ucontrol->value.integer.value[1] = r_vol;
	if (codec && codec->mixer_ops.get_volume) {
		codec->mixer_ops.get_volume(codec, &l_vol, &r_vol);
		if (ucontrol) {
			ucontrol->value.integer.value[0] = l_vol;
			ucontrol->value.integer.value[1] = r_vol;
		}
	}

	return 0;
}

static int snd_cmi8788_capture_put_volume(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	snd_cmi8788        *chip       = NULL;
	cmi_codec          *codec      = NULL;
	unsigned long       private_value = 0;
	int                 codec_num = 0;
	int l_vol = 0, r_vol = 0;
	int change = 0;

	if (!kcontrol || !ucontrol)
		return -1;

	GET_POINTER(kcontrol);

	codec_num = chip->num_codecs;

	/* set CODEC register */
	switch(private_value) {
	case CAPTURE_MIC_VOL:
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
	default:
		codec = &chip->ac97_codec_list[0];
		codec->volume_opera_source = MIC_VOL_SLIDER;
		break;
	}

	if (codec && codec->mixer_ops.get_volume)
		codec->mixer_ops.get_volume(codec, &l_vol, &r_vol);

	if (l_vol == ucontrol->value.integer.value[0] && r_vol == ucontrol->value.integer.value[1])
		change = 0;
	else
		change = 1;

	l_vol = ucontrol->value.integer.value[0];
	r_vol = ucontrol->value.integer.value[1];

	if (chip->first_set_capture_volume) {
		chip->first_set_capture_volume = 0;
		/*    return change; */
	}

	if (change) {
		if (codec && codec->mixer_ops.set_volume && chip->capture_volume_init) {
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
int __devinit snd_cmi8788_mixer_create(snd_cmi8788 *chip)
{
	unsigned int idx;
	struct snd_card *card = NULL;
	int i, codec_num;
	cmi_codec *codec = NULL;
	int err;

	if (!chip)
		return 0;

	card = chip->card;
	strcpy(card->mixername, "C-Media PCI8788");

	/* for playback and capture CODEC */
	codec_num = chip->num_codecs;
	for (i = 0; i < codec_num; i++) {
		codec = &chip->codec_list[i];
		if (!codec->patch_ops.build_controls)
			continue;

		err = codec->patch_ops.build_controls(codec);
		if (err < 0)
			return err;
	}

	/* for AC97 CODEC */
	codec_num = chip->num_ac97_codecs;
	codec = NULL;
	for (i = 0; i < codec_num; i++) {
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
