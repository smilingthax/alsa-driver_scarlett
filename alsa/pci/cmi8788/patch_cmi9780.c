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
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pci.h>
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
	*min_vol = 0;  /* mute */
	*max_vol = 32; /* 1-32 */
	return 0;
}

static int put_volume(struct cmi_codec *codec, int l_vol, int r_vol)
{
	int l_volume = 0, r_volume = 0;
	u32 val32 = 0;
	u8 reg_addr = 0x0E; /* Mic volume */
	u16 reg_data = 0x0808;
	int opera_source = MIC_VOL_SLIDER;

	/* bit4-0  0-1f */
	l_volume = l_vol;
	if (l_vol >= 32)
		l_volume = 32;
	if (l_vol <= 0)
		l_volume = 0;
	r_volume = r_vol;
	if (r_vol >= 32)
		r_volume = 32;
	if (r_vol <= 0)
		r_volume = 0;

	opera_source = codec->volume_opera_source;
	codec->volume[opera_source].left_vol = (s16)l_volume;
	codec->volume[opera_source].right_vol = (s16)r_volume;
	reg_addr = volume_reg_addr[opera_source];

	l_volume = 32 - l_volume;
	r_volume = 32 - r_volume;

	/* set Mic Volume Register 0x0Eh */
	val32 = 0; /* Bit 31-24 */
	val32 &= 0xff000000; /* Bit-23: 0 write */
	val32 |= reg_addr << 16; /* Register 0x0E */
	if (codec->left_vol == 0 || codec->right_vol == 0) {
		val32 |= 0x8000; /* 0x0808 : mute */
	}
	if (l_volume > 0)
		l_volume -= 1;
	if (r_volume > 0)
		r_volume -= 1;
	val32 |= r_volume;
	val32 |= l_volume << 8;

	reg_data = 0;
	reg_data |= r_volume;
	reg_data |= l_volume << 8;

	snd_cmi_send_ac97_cmd(codec->chip, reg_addr, reg_data);

	return 0;
}

static int get_volume(struct cmi_codec *codec, int *l_vol, int *r_vol)
{
	int opera_source = MIC_VOL_SLIDER;

	opera_source = codec->volume_opera_source;

	*l_vol = (int)(codec->volume[opera_source].left_vol);
	*r_vol = (int)(codec->volume[opera_source].right_vol);

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
	int i = 0;

	codec->addr = 0;
	codec->reg_len_flag = 0;

#if 0
	u8 reg_addr = 2; /* master volume */
	u16 reg_data = 0x1f1f; /* left right channel 46.5dB Attenuation */
	snd_cmi_send_ac97_cmd(codec->chip, reg_addr, reg_data);

	reg_addr = 0xe; /* Mic volume */
	reg_data = 0x0000; /* left right channel +12dB Attenuation */
	snd_cmi_send_ac97_cmd(codec->chip, reg_addr, reg_data);

	reg_addr = 0x1a; /* record select */
	reg_data = 0x00; /* default Mic in */
	snd_cmi_send_ac97_cmd(codec->chip, reg_addr, reg_data);

	reg_addr = 0x1c; /* Record Gain Registers */
	reg_data = 0x0f0f; /* left right channel 22.5 dB gain */
	snd_cmi_send_ac97_cmd(codec->chip, reg_addr, reg_data);
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
