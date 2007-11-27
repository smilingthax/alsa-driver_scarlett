#define __NO_VERSION__
/*
 * C-Media CMI8788 driver - mixer code
 *
 * Copyright (c) Clemens Ladisch <clemens@ladisch.de>
 *
 *
 *  This driver is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License, version 2.
 *
 *  This driver is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this driver; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <sound/driver.h>
#include <sound/ac97_codec.h>
#include <sound/control.h>
#include <sound/tlv.h>
#include "oxygen.h"

static int dac_volume_info(struct snd_kcontrol *ctl,
			   struct snd_ctl_elem_info *info)
{
	struct oxygen *chip = ctl->private_data;

	info->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	info->count = 8;
	info->value.integer.min = chip->model->dac_minimum_volume;
	info->value.integer.max = 0xff;
	return 0;
}

static int dac_volume_get(struct snd_kcontrol *ctl,
			  struct snd_ctl_elem_value *value)
{
	struct oxygen *chip = ctl->private_data;
	unsigned int i;

	mutex_lock(&chip->mutex);
	for (i = 0; i < 8; ++i)
		value->value.integer.value[i] = chip->dac_volume[i];
	mutex_unlock(&chip->mutex);
	return 0;
}

static int dac_volume_put(struct snd_kcontrol *ctl,
			  struct snd_ctl_elem_value *value)
{
	struct oxygen *chip = ctl->private_data;
	unsigned int i;
	int changed;

	changed = 0;
	mutex_lock(&chip->mutex);
	for (i = 0; i < 8; ++i)
		if (value->value.integer.value[i] != chip->dac_volume[i]) {
			chip->dac_volume[i] = value->value.integer.value[i];
			changed = 1;
		}
	if (changed)
		chip->model->update_dac_volume(chip);
	mutex_unlock(&chip->mutex);
	return changed;
}

static int dac_mute_get(struct snd_kcontrol *ctl,
			struct snd_ctl_elem_value *value)
{
	struct oxygen *chip = ctl->private_data;

	mutex_lock(&chip->mutex);
	value->value.integer.value[0] = !chip->dac_mute;
	mutex_unlock(&chip->mutex);
	return 0;
}

static int dac_mute_put(struct snd_kcontrol *ctl,
			  struct snd_ctl_elem_value *value)
{
	struct oxygen *chip = ctl->private_data;
	int changed;

	mutex_lock(&chip->mutex);
	changed = !value->value.integer.value[0] != chip->dac_mute;
	if (changed) {
		chip->dac_mute = !value->value.integer.value[0];
		chip->model->update_dac_mute(chip);
	}
	mutex_unlock(&chip->mutex);
	return changed;
}

static int upmix_info(struct snd_kcontrol *ctl, struct snd_ctl_elem_info *info)
{
	static const char *const names[3] = {
		"Front", "Front+Rear", "Front+Rear+Side"
	};
	info->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	info->count = 1;
	info->value.enumerated.items = 3;
	if (info->value.enumerated.item > 2)
		info->value.enumerated.item = 2;
	strcpy(info->value.enumerated.name, names[info->value.enumerated.item]);
	return 0;
}

static int upmix_get(struct snd_kcontrol *ctl, struct snd_ctl_elem_value *value)
{
	struct oxygen *chip = ctl->private_data;

	mutex_lock(&chip->mutex);
	value->value.enumerated.item[0] = chip->dac_routing;
	mutex_unlock(&chip->mutex);
	return 0;
}

void oxygen_update_dac_routing(struct oxygen *chip)
{
	/*
	 * hardware channel order: front, side, center/lfe, rear
	 * ALSA channel order:     front, rear, center/lfe, side
	 */
	static const unsigned int reg_values[3] = {
		0x6c00, 0x2c00, 0x2000
	};
	unsigned int reg_value;

	if ((oxygen_read8(chip, OXYGEN_PLAY_CHANNELS) &
	     OXYGEN_PLAY_CHANNELS_MASK) == OXYGEN_PLAY_CHANNELS_2)
		reg_value = reg_values[chip->dac_routing];
	else
		reg_value = 0x6c00;
	oxygen_write16_masked(chip, OXYGEN_PLAY_ROUTING, reg_value, 0xff00);
}

static int upmix_put(struct snd_kcontrol *ctl, struct snd_ctl_elem_value *value)
{
	struct oxygen *chip = ctl->private_data;
	int changed;

	mutex_lock(&chip->mutex);
	changed = value->value.enumerated.item[0] != chip->dac_routing;
	if (changed) {
		chip->dac_routing = min(value->value.enumerated.item[0], 2u);
		spin_lock_irq(&chip->reg_lock);
		oxygen_update_dac_routing(chip);
		spin_unlock_irq(&chip->reg_lock);
	}
	mutex_unlock(&chip->mutex);
	return changed;
}

static int ac97_switch_get(struct snd_kcontrol *ctl,
			   struct snd_ctl_elem_value *value)
{
	struct oxygen *chip = ctl->private_data;
	unsigned int index = ctl->private_value & 0xff;
	unsigned int bitnr = (ctl->private_value >> 8) & 0xff;
	int invert = ctl->private_value & (1 << 16);
	u16 reg;

	mutex_lock(&chip->mutex);
	reg = oxygen_read_ac97(chip, 0, index);
	mutex_unlock(&chip->mutex);
	if (!(reg & (1 << bitnr)) ^ !invert)
		value->value.integer.value[0] = 1;
	else
		value->value.integer.value[0] = 0;
	return 0;
}

static int ac97_switch_put(struct snd_kcontrol *ctl,
			   struct snd_ctl_elem_value *value)
{
	struct oxygen *chip = ctl->private_data;
	unsigned int index = ctl->private_value & 0xff;
	unsigned int bitnr = (ctl->private_value >> 8) & 0xff;
	int invert = ctl->private_value & (1 << 16);
	u16 oldreg, newreg;
	int change;

	mutex_lock(&chip->mutex);
	oldreg = oxygen_read_ac97(chip, 0, index);
	newreg = oldreg;
	if (!value->value.integer.value[0] ^ !invert)
		newreg |= 1 << bitnr;
	else
		newreg &= ~(1 << bitnr);
	change = newreg != oldreg;
	if (change) {
		oxygen_write_ac97(chip, 0, index, newreg);
		if (index == AC97_LINE)
			oxygen_write_ac97_masked(chip, 0, 0x72,
						 !!(newreg & 0x8000), 0x0001);
	}
	mutex_unlock(&chip->mutex);
	return change;
}

static int ac97_volume_info(struct snd_kcontrol *ctl,
			    struct snd_ctl_elem_info *info)
{
	info->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	info->count = 2;
	info->value.integer.min = 0;
	info->value.integer.max = 0x1f;
	return 0;
}

static int ac97_volume_get(struct snd_kcontrol *ctl,
			   struct snd_ctl_elem_value *value)
{
	struct oxygen *chip = ctl->private_data;
	unsigned int index = ctl->private_value;
	u16 reg;

	mutex_lock(&chip->mutex);
	reg = oxygen_read_ac97(chip, 0, index);
	mutex_unlock(&chip->mutex);
	value->value.integer.value[0] = 31 - (reg & 0x1f);
	value->value.integer.value[1] = 31 - ((reg >> 8) & 0x1f);
	return 0;
}

static int ac97_volume_put(struct snd_kcontrol *ctl,
			   struct snd_ctl_elem_value *value)
{
	struct oxygen *chip = ctl->private_data;
	unsigned int index = ctl->private_value;
	u16 oldreg, newreg;
	int change;

	mutex_lock(&chip->mutex);
	oldreg = oxygen_read_ac97(chip, 0, index);
	newreg = oldreg;
	newreg = (newreg & ~0x1f) |
		(31 - (value->value.integer.value[0] & 0x1f));
	newreg = (newreg & ~0x1f00) |
		((31 - (value->value.integer.value[0] & 0x1f)) << 8);
	change = newreg != oldreg;
	if (change)
		oxygen_write_ac97(chip, 0, index, newreg);
	mutex_unlock(&chip->mutex);
	return change;
}

#define AC97_SWITCH(xname, index, bitnr, invert) { \
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER, \
		.name = xname, \
		.info = snd_ctl_boolean_mono_info, \
		.get = ac97_switch_get, \
		.put = ac97_switch_put, \
		.private_value = ((invert) << 16) | ((bitnr) << 8) | (index), \
	}
#define AC97_VOLUME(xname, index) { \
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER, \
		.name = xname, \
		.info = ac97_volume_info, \
		.get = ac97_volume_get, \
		.put = ac97_volume_put, \
		.tlv.p = ac97_db_scale, \
		.private_value = (index), \
	}

static DECLARE_TLV_DB_SCALE(ac97_db_scale, -3450, 150, 0);

static const struct snd_kcontrol_new controls[] = {
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "PCM Playback Volume",
		.access = SNDRV_CTL_ELEM_ACCESS_READWRITE |
			  SNDRV_CTL_ELEM_ACCESS_TLV_READ,
		.info = dac_volume_info,
		.get = dac_volume_get,
		.put = dac_volume_put,
		.tlv.p = NULL, /* set later */
	},
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "PCM Playback Switch",
		.info = snd_ctl_boolean_mono_info,
		.get = dac_mute_get,
		.put = dac_mute_put,
	},
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "Stereo Upmixing",
		.info = upmix_info,
		.get = upmix_get,
		.put = upmix_put,
	},
	AC97_VOLUME("Mic Capture Volume", AC97_MIC),
	AC97_SWITCH("Mic Capture Switch", AC97_MIC, 15, 1),
	AC97_SWITCH("Mic Boost (+20dB)", AC97_MIC, 6, 0),
	AC97_SWITCH("Line Capture Switch", AC97_LINE, 15, 1),
	AC97_VOLUME("CD Capture Volume", AC97_CD),
	AC97_SWITCH("CD Capture Switch", AC97_CD, 15, 1),
	AC97_VOLUME("Aux Capture Volume", AC97_AUX),
	AC97_SWITCH("Aux Capture Switch", AC97_AUX, 15, 1),
};

int oxygen_mixer_init(struct oxygen *chip)
{
	unsigned int i;
	struct snd_kcontrol *ctl;
	int err;

	for (i = 0; i < ARRAY_SIZE(controls); ++i) {
		ctl = snd_ctl_new1(&controls[i], chip);
		if (!ctl)
			return -ENOMEM;
		if (!strcmp(ctl->id.name, "PCM Playback Volume"))
			ctl->tlv.p = chip->model->dac_tlv;
		else if (chip->model->cd_in_from_video_in &&
			 !strncmp(ctl->id.name, "CD Capture ", 11))
			ctl->private_value ^= AC97_CD ^ AC97_VIDEO;
		err = snd_ctl_add(chip->card, ctl);
		if (err < 0)
			return err;
	}
	return 0;
}
