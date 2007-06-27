/*
 *  patch_cmi9780.c - Driver for C-Media CMI8788 PCI soundcards.
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
 * audio interface patch for CMI9780
 */

#include <sound/driver.h>
#include <sound/core.h>
#include "cmi8788.h"


/* ac97 volumes slider to register address */
static u8 volume_reg_addr[MAX_VOL_SLIDER] = {
	0x02, /* MASTER_VOL_SLIDER */
	0x0A, /* PCBEEP_VOL_SLIDER */
	0x0E, /* MIC_VOL_SLIDER */
	0x10, /* LINEIN_VOL_SLIDER */
	0x12, /* CD_VOL_SLIDER */
	0x14, /* VIDEO_VOL_SLIDER */
	0x16, /* AUX_VOL_SLIDER */
};

/*
 * mixer , volume
 */
static int get_info(struct cmi_codec *codec, int *min_vol, int *max_vol)
{
	*min_vol = 0;
	*max_vol = 31;
	return 0;
}

static int put_volume(struct cmi_codec *codec, int l_vol, int r_vol)
{
	u8 reg_addr;
	u16 reg_data;
	int opera_source;

	opera_source = codec->volume_opera_source;
	codec->volume[opera_source].left_vol = (s16)l_vol;
	codec->volume[opera_source].right_vol = (s16)r_vol;
	reg_addr = volume_reg_addr[opera_source];
	reg_data =(((31 - l_vol) & 0x1f) << 8) | ((31 - r_vol) & 0x1f);
	snd_cmi_send_ac97_cmd(codec->chip, reg_addr, reg_data);
	return 0;
}

static int get_volume(struct cmi_codec *codec, int *l_vol, int *r_vol)
{
	*l_vol = codec->volume[codec->volume_opera_source].left_vol;
	*r_vol = codec->volume[codec->volume_opera_source].right_vol;
	return 0;
}

static struct cmi8788_mixer_ops cmi9780_mixer_ops =
{
	.get_info   = get_info,
	.get_volume = get_volume,
	.set_volume = put_volume,
};

/*
 * create mixer
 */
static int cmi9780_build_controls(struct cmi_codec *codec)
{
	codec->mixer_ops = cmi9780_mixer_ops;
	return 0;
}

static int cmi9780_init(struct cmi_codec *codec)
{
	int i;

#if 0
	/* master volume: left right channel 46.5dB Attenuation */
	snd_cmi_send_ac97_cmd(codec->chip, 0x02, 0x1f1f);

	/* Mic volume: left right channel +12dB Attenuation */
	snd_cmi_send_ac97_cmd(codec->chip, 0x0e, 0x0000);

	/* record select: default Mic in */
	snd_cmi_send_ac97_cmd(codec->chip, 0x1a, 0x0000);

	/* Record Gain Registers: left right channel 22.5 dB gain */
	snd_cmi_send_ac97_cmd(codec->chip, 0x1c, 0x0f0f);
#endif
	codec->left_vol  = 8;
	codec->right_vol = 8;

	codec->volume_opera_source = MIC_VOL_SLIDER;
	for (i = 0; i < MAX_VOL_SLIDER; i++) {
		codec->volume[i].left_vol = 8;
		codec->volume[i].right_vol = 8;
	}

	return 0;
}

struct cmi_codec_ops cmi9780_patch_ops = {
	.build_controls = cmi9780_build_controls,
	.init           = cmi9780_init,
};
