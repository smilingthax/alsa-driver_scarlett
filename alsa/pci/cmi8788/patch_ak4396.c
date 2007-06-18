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
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <sound/core.h>

#include "cmi8788.h"
#include "codec.h"
#include "cmi_controller.h"


#define AK4396_CTL1        0x00
#define AK4396_CTL2        0x01
#define AK4396_CTL3        0x02
#define AK4396_LchATTCtl   0x03
#define AK4396_RchATTCtl   0x04


/*
 * Analog playback callbacks
 */
static int ak4396_playback_pcm_open(void                *hinfo,
				    cmi_codec           *codec,
				    struct snd_pcm_substream *substream )
{
	cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;

	return 0;
}

static int ak4396_playback_pcm_prepare(void                *hinfo,
				       cmi_codec           *codec,
				       struct snd_pcm_substream *substream )
{
	cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;

	return 0;
}

static int ak4396_playback_pcm_cleanup(void                *hinfo,
				       cmi_codec           *codec,
				       struct snd_pcm_substream *substream )
{
	cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;

	return 0;
}

/*
 * Analog capture
 */
static int ak4396_capture_pcm_prepare(void                *hinfo,
				      cmi_codec           *codec,
				      struct snd_pcm_substream *substream )
{
	cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;

	return 0;
}

static int ak4396_capture_pcm_cleanup(void                *hinfo,
				      cmi_codec           *codec,
				      struct snd_pcm_substream *substream )
{
	cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;

	return 0;
}


static cmi8788_pcm_stream ak4396_pcm_analog_playback = {
	.channels = 2,
	.ops = {
		.open    = ak4396_playback_pcm_open,
		.prepare = ak4396_playback_pcm_prepare,
		.cleanup = ak4396_playback_pcm_cleanup
	},
};

static cmi8788_pcm_stream ak4396_pcm_analog_capture = {
	.channels = 2,
	.ops = {
		.prepare = ak4396_capture_pcm_prepare,
		.cleanup = ak4396_capture_pcm_cleanup
	},
};


static int ak4396_build_pcms(cmi_codec *codec)
{
	cmi8788_pcm_stream *pcm_substream = codec->pcm_substream;

	pcm_substream[0] = ak4396_pcm_analog_playback;
	pcm_substream[1] = ak4396_pcm_analog_capture;

	return 0;
}

static int get_info(cmi_codec *codec, int *min_vol, int *max_vol)
{
	if (!codec || !min_vol || !max_vol)
		return -1;

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
static int put_volume(cmi_codec *codec, int l_vol, int r_vol)
{
	u8 data[2];
	int l_volume = 0, r_volume = 0;
	cmi8788_controller *controller;

	if (!codec)
		return -1;

	controller = codec->controller;
	if (!controller)
		return -1;

	l_volume = l_vol;
	if (l_vol >= 255)
		l_volume = 255;
	if (l_vol <= 0)
		l_volume = 0;
	r_volume = r_vol;
	if (r_vol >= 255)
		r_volume = 255;
	if (r_vol <= 0)
		r_volume = 0;

	data[0] = (u8)l_volume;
	/* 001xxxxx Binary */
	data[1] = AK4396_LchATTCtl | 0x20;
	controller->ops.spi_cmd(codec, data);
	/* udelay(10); */

	data[0] = (u8)r_volume;
	data[1] = AK4396_RchATTCtl | 0x20;
	controller->ops.spi_cmd(codec, data);
	/* udelay(10); */

	codec->left_vol  = l_volume;
	codec->right_vol = r_volume;

	return 0;
}

/*
 * The ak4396 does not support read command.
 */
static int get_volume(cmi_codec *codec, int *l_vol, int *r_vol)
{
	if (!codec || !l_vol || !r_vol)
		return -1;

	*l_vol = codec->left_vol;
	*r_vol = codec->right_vol;

	return 0;
}

static cmi8788_mixer_ops  ak_4396_mixer_ops =
{
	.get_info   = get_info,
	.get_volume = get_volume,
	.set_volume = put_volume,
};

/*
 * create mixer
 */
static int ak4396_build_controls(cmi_codec *codec)
{
	if (!codec)
		return -1;

	codec->mixer_ops = ak_4396_mixer_ops;

	return 0;
}

static int ak4396_init(cmi_codec *codec)
{
	cmi8788_controller *controller;
	u8 data[2];

	cmi_printk(("  >> ak4396_init\n"));

	if (!codec)
		return -1;

#if 1
	controller = codec->controller;
	if (!controller)
		return -1;

	codec->reg_len_flag = 0;
	codec->left_vol  = 255;
	codec->right_vol = 255;

	data[0] = 0x05; /* DIF2 DIF1 DIF0: 010 24Bit MSB justified */
	data[1] = AK4396_CTL1 | 0x20;
	controller->ops.spi_cmd(codec, data);

	data[0] = 0x02; /* DEM1 DEM0 : 01 off */
	data[1] = AK4396_CTL2 | 0x20;
	controller->ops.spi_cmd(codec, data);

	data[0] = 0x00; /* default */
	data[1] = AK4396_CTL3 | 0x20;
	controller->ops.spi_cmd(codec, data);

	data[0] = (u8)codec->left_vol;
	data[1] = AK4396_LchATTCtl | 0x20;
	controller->ops.spi_cmd(codec, data);

	data[0] = (u8)codec->right_vol;
	data[1] = AK4396_RchATTCtl | 0x20;
	controller->ops.spi_cmd(codec, data);
#endif

	cmi_printk(("  << ak4396_init\n"));

	return 0;
}

static void ak4396_free(cmi_codec *codec)
{
}

static cmi_codec_ops ak4396_patch_ops = {
	.build_controls = ak4396_build_controls,
	.build_pcms     = ak4396_build_pcms,
	.init           = ak4396_init,
	.free           = ak4396_free,
};

static int patch_ak4396(cmi_codec *codec)
{
	codec->patch_ops = ak4396_patch_ops;

	return 0;
}

/*
 * patch entries
 */
codec_preset snd_preset_ak4396[] = {
	{ .id = 0xFFFFFFFF, .name = "AK4396", .patch = patch_ak4396 },
	{ } /* terminator */
};
