/*
 * PC-Speaker driver for Linux
 *
 * Mixer implementation.
 * Copyright (C) 2001-2004  Stas Sergeev
 */

#include <sound/driver.h>
#include <sound/core.h>
#include <sound/control.h>
#include "pcsp_defs.h"
#include "pcsp_tabs.h"

/*
   calculate a translation-table for PC-Speaker
*/
void pcsp_calc_voltab(pcsp_t *chip)
{
	int i;
	unsigned int j;
	for (i = 0; i < 256; i++) {
		j = (((pcsp_tabs[chip->gain][i] - 128)
			    * (signed)chip->volume) / PCSP_MAX_VOLUME) + 128;
		chip->vl_tab[i] = j * MIN_DIV / 256;
	}
}

static int pcsp_volume_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = PCSP_MAX_VOLUME;
	return 0;
}

static int pcsp_volume_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pcsp_t *chip = snd_kcontrol_chip(kcontrol);
	ucontrol->value.integer.value[0] = chip->volume;
	return 0;
}

static int pcsp_volume_put(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pcsp_t *chip = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	int changed = 0;
	int vol = ucontrol->value.integer.value[0];
	if (vol != chip->volume) {
		spin_lock_irqsave(&chip->lock, flags);
		chip->volume = vol;
		pcsp_calc_voltab(chip);
		spin_unlock_irqrestore(&chip->lock, flags);
		changed = 1;
	}
	return changed;
}

static int pcsp_gain_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = pcsp_max_gain;
	return 0;
}

static int pcsp_gain_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pcsp_t *chip = snd_kcontrol_chip(kcontrol);
	ucontrol->value.integer.value[0] = chip->gain;
	return 0;
}

static int pcsp_gain_put(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pcsp_t *chip = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	int changed = 0;
	int gain = ucontrol->value.integer.value[0];
	if (gain != chip->gain) {
		spin_lock_irqsave(&chip->lock, flags);
		chip->gain = gain;
		pcsp_calc_voltab(chip);
		spin_unlock_irqrestore(&chip->lock, flags);
		changed = 1;
	}
	return changed;
}

static int pcsp_treble_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	pcsp_t *chip = snd_kcontrol_chip(kcontrol);
	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items = chip->max_treble + 1;
	if (uinfo->value.enumerated.item > chip->max_treble)
		uinfo->value.enumerated.item = chip->max_treble;
	sprintf(uinfo->value.enumerated.name, "%d", PCSP_RATE);
	return 0;
}

static int pcsp_treble_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pcsp_t *chip = snd_kcontrol_chip(kcontrol);
	ucontrol->value.enumerated.item[0] = chip->treble;
	return 0;
}

static int pcsp_treble_put(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	unsigned long flags;
	pcsp_t *chip = snd_kcontrol_chip(kcontrol);
	int changed = 0;
	int treble = ucontrol->value.enumerated.item[0];
	if (treble != chip->treble) {
		spin_lock_irqsave(&chip->lock, flags);
		chip->treble = treble;
		chip->reset_timer = 1;
		pcsp_calc_voltab(chip);
		spin_unlock_irqrestore(&chip->lock, flags);
		changed = 1;
	}
	return changed;
}

#define PCSP_MIXER_CONTROL(ctl_type, ctl_name) \
{ \
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER, \
	.name =		ctl_name " Playback Volume", \
	.info =		pcsp_##ctl_type##_info, \
	.get =		pcsp_##ctl_type##_get, \
	.put =		pcsp_##ctl_type##_put, \
}

static snd_kcontrol_new_t __initdata snd_pcsp_controls[] = {
	PCSP_MIXER_CONTROL(volume, "Master"),
	PCSP_MIXER_CONTROL(gain, "PCM"),
	PCSP_MIXER_CONTROL(treble, "BaseFRQ"),
};

int __init snd_pcsp_new_mixer(pcsp_t *chip)
{
	snd_card_t *card = chip->card;
	int i, err;

	for (i = 0; i < ARRAY_SIZE(snd_pcsp_controls); i++) {
		if ((err = snd_ctl_add(card, snd_ctl_new1(
				snd_pcsp_controls + i, chip))) < 0)
			return err;
	}

	strcpy(card->mixername, "PC-Speaker");

	return 0;
}
