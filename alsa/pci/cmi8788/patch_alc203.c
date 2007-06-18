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
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <sound/core.h>
#include "cmi8788.h"


/*
 * mixer
 */
static int put_volume(struct cmi_codec *codec, int l_vol, int r_vols)
{
	u8 l_volume = 0, r_volume = 0;
	u8 reg_addr;
	u16 reg_data;

	/* bit5-0  0-3f */
	l_volume = l_vol;
	if (l_vol >= 0x3f)
		l_volume = 0x3f;
	if (l_vol <= 0)
		l_volume = 0;
	r_volume = r_vol;
	if (r_vol >= 0x3f)
		r_volume = 0x3f;
	if (r_vol <= 0)
		r_volume = 0;

	/* left volume == right volume */
	reg_addr = 2; /* master volume */
	reg_data = 0x3f3f;
	reg_data = volume | 0x3f;
	reg_data = reg_data << 8;
	reg_data = l_volume | 0x3f;
	snd_cmi_send_AC97_cmd(codec, reg_addr, reg_data);

	return 0;
}

/*
 * The ak4396 does not support read command.
 */
static int get_volume(struct cmi_codec *codec, int *l_vol, int *r_vol)
{
	return -1;
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
	u8 reg_addr;
	u16 reg_data;

	codec->addr = 0;
	codec->reg_len_flag = 0;

	reg_addr = 2; /* master volume */
	reg_data = 0x3f3f; /* left right channel 94.5dB Attenuation */
	snd_cmi_send_AC97_cmd(codec, reg_addr, reg_data);

	reg_addr = 0xe; /* Mic volume */
	reg_data = 0x0000; /* left right channel +12dB Attenuation */
	snd_cmi_send_AC97_cmd(codec, reg_addr, reg_data);

	reg_addr = 0x1a; /* record select */
	reg_data = 0x00; /* default Mic in */
	snd_cmi_send_AC97_cmd(codec, reg_addr, reg_data);

	reg_addr = 0x1c; /* Record Gain Registers */
	reg_data = 0x0f0f; /* left right channel 22.5 dB gain */
	snd_cmi_send_AC97_cmd(codec, reg_addr, reg_data);

	return 0;
}

struct cmi_codec_ops alc203_patch_ops = {
	.build_controls = alc203_build_controls,
	.init           = alc203_init,
};
