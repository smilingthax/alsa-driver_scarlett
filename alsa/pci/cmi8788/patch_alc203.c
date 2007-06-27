/*
 *  patch_alc203.c - Driver for C-Media CMI8788 PCI soundcards.
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

/*
 * audio interface patch for ALC203
 */

#include <sound/driver.h>
#include <sound/core.h>
#include "cmi8788.h"


/*
 * mixer
 */
static int put_volume(struct cmi_codec *codec, int l_vol, int r_vol)
{
	u16 reg_data;

	reg_data = ((r_vol & 0x3f) << 8) | (l_vol & 0x3f);
	snd_cmi_send_ac97_cmd(codec->chip, 2, reg_data);
	return 0;
}

static struct cmi8788_mixer_ops alc203_mixer_ops =
{
	/* .get_volume = get_volume, */
	.set_volume = put_volume,
};

/*
 * create mixer
 */
static int alc203_build_controls(struct cmi_codec *codec)
{
	codec->mixer_ops = alc203_mixer_ops;
	return 0;
}

static int alc203_init(struct cmi_codec *codec)
{
	/* master volume: left right channel 94.5dB Attenuation */
	snd_cmi_send_ac97_cmd(codec->chip, 0x02, 0x3f3f);

	/* Mic volume: left right channel +12dB Attenuation */
	snd_cmi_send_ac97_cmd(codec->chip, 0x0e, 0x0000);

	/* record select: default Mic in */
	snd_cmi_send_ac97_cmd(codec->chip, 0x1a, 0x0000);

	/* Record Gain Registers: left right channel 22.5 dB gain */
	snd_cmi_send_ac97_cmd(codec->chip, 0x1c, 0x0f0f);
	return 0;
}

struct cmi_codec_ops alc203_patch_ops = {
	.build_controls = alc203_build_controls,
	.init           = alc203_init,
};
