#define __NO_VERSION__
/*
 *  patch_ak4396.c - Driver for C-Media CMI8788 PCI soundcards.
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
 * audio interface patch for AK4396, only SPI
 */

#include <sound/driver.h>
#include <sound/core.h>
#include "cmi8788.h"


#define AK4396_CTL1        0x00
#define AK4396_CTL2        0x01
#define AK4396_CTL3        0x02
#define AK4396_LchATTCtl   0x03
#define AK4396_RchATTCtl   0x04


static int get_info(struct cmi_codec *codec, int *min_vol, int *max_vol)
{
	*min_vol = 0;
	*max_vol = 255;
	return 0;
}

/*
 * mixer linear volume
 * The data (which include address, r/w, and data bits)
 * The data consists of Chip address(2bits CAD0/1).Read/Write(1bit:fixed to "1").
 *   Register address(MSB first, 5bits) and Control data(MSB first,8bits).
 *   C1 C0 R/W A4 A3 A2 A1 A0 D7 D6 D5 D4 D3 D2 D1 D0
 */
static int put_volume(struct cmi_codec *codec, int l_vol, int r_vol)
{
	u8 data[2];

	data[0] = (u8)l_vol;
	/* 001xxxxx Binary */
	data[1] = AK4396_LchATTCtl | 0x20;
	snd_cmi_send_spi_cmd(codec, data);
	/* udelay(10); */

	data[0] = (u8)r_vol;
	data[1] = AK4396_RchATTCtl | 0x20;
	snd_cmi_send_spi_cmd(codec, data);
	/* udelay(10); */

	codec->left_vol  = l_vol;
	codec->right_vol = r_vol;
	return 0;
}

/*
 * The ak4396 does not support read command.
 */
static int get_volume(struct cmi_codec *codec, int *l_vol, int *r_vol)
{
	*l_vol = codec->left_vol;
	*r_vol = codec->right_vol;
	return 0;
}

static struct cmi8788_mixer_ops ak_4396_mixer_ops =
{
	.get_info   = get_info,
	.get_volume = get_volume,
	.set_volume = put_volume,
};

/*
 * create mixer
 */
static int ak4396_build_controls(struct cmi_codec *codec)
{
	codec->mixer_ops = ak_4396_mixer_ops;
	return 0;
}

static int ak4396_init(struct cmi_codec *codec)
{
	u8 data[2];

#if 1
	codec->reg_len_flag = 0;
	codec->left_vol  = 255;
	codec->right_vol = 255;

	data[0] = 0x05; /* DIF2 DIF1 DIF0: 010 24Bit MSB justified */
	data[1] = AK4396_CTL1 | 0x20;
	snd_cmi_send_spi_cmd(codec, data);

	data[0] = 0x02; /* DEM1 DEM0 : 01 off */
	data[1] = AK4396_CTL2 | 0x20;
	snd_cmi_send_spi_cmd(codec, data);

	data[0] = 0x00; /* default */
	data[1] = AK4396_CTL3 | 0x20;
	snd_cmi_send_spi_cmd(codec, data);

	data[0] = (u8)codec->left_vol;
	data[1] = AK4396_LchATTCtl | 0x20;
	snd_cmi_send_spi_cmd(codec, data);

	data[0] = (u8)codec->right_vol;
	data[1] = AK4396_RchATTCtl | 0x20;
	snd_cmi_send_spi_cmd(codec, data);
#endif

	return 0;
}

struct cmi_codec_ops ak4396_patch_ops = {
	.build_controls = ak4396_build_controls,
	.init           = ak4396_init,
};
