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
#include "codec.h"
#include "cmi_controller.h"


/*
 * Analog playback callbacks
 */
static int alc203_playback_pcm_open(void                *hinfo,
				    cmi_codec           *codec,
				    struct snd_pcm_substream *substream )
{
	/* 待完善 需要设置相关的寄存器 */
	cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;

	return 0;
}

static int alc203_playback_pcm_prepare(void                *hinfo,
				       cmi_codec           *codec,
				       struct snd_pcm_substream *substream )
{
	/* 待完善 需要设置相关的寄存器 */
	cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;

	return 0;
}

static int alc203_playback_pcm_cleanup(void                *hinfo,
				       cmi_codec           *codec,
				       struct snd_pcm_substream *substream )
{
	/* 待完善 需要设置相关的寄存器 */
	cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;

	return 0;
}

/*
 * Analog capture
 */
static int alc203_capture_pcm_prepare(void                *hinfo,
				      cmi_codec           *codec,
				      struct snd_pcm_substream *substream )
{
	/* 待完善 需要设置相关的寄存器 */
	cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;

	return 0;
}

static int alc203_capture_pcm_cleanup(void                *hinfo,
				      cmi_codec           *codec,
				      struct snd_pcm_substream *substream )
{
	/* 待完善 需要设置相关的寄存器 */
	cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;

	return 0;
}


static cmi8788_pcm_stream alc203_pcm_analog_playback = {
	.channels = 2,
	.ops = {
		.open    = alc203_playback_pcm_open,
		.prepare = alc203_playback_pcm_prepare,
		.cleanup = alc203_playback_pcm_cleanup
	},
};

static cmi8788_pcm_stream alc203_pcm_analog_capture = {
	.channels = 2,
	.ops = {
		.prepare = alc203_capture_pcm_prepare,
		.cleanup = alc203_capture_pcm_cleanup
	},
};


static int alc203_build_pcms(cmi_codec *codec)
{
	cmi8788_pcm_stream *pcm_substream = codec->pcm_substream;

	pcm_substream[0] = alc203_pcm_analog_playback;
	pcm_substream[1] = alc203_pcm_analog_capture;

	return 0;
}

/*
 * mixer
 */
static int put_volume(cmi_codec *codec, int l_vol, int r_vols)
{
	cmi8788_controller *controller;
	u8 l_volume = 0, r_volume = 0;
	u8 reg_addr;
	u16 reg_data;

	if (!codec)
		return -1;

	controller = codec->controller;
	if (!controller)
		return -1;

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
	controller->ops.ac97_cmd(codec, reg_addr, reg_data);

	return 0;
}

/*
 * The ak4396 does not support read command.
 */
static int get_volume(cmi_codec *codec, int *l_vol, int *r_vol)
{
	return -1;
}

static cmi8788_mixer_ops  alc203_mixer_ops =
{
	/* .get_volume = get_volume, */
	.set_volume = put_volume,
};

/*
 * create mixer
 */
static int alc203_build_controls(cmi_codec *codec)
{
	if (!codec)
		return -1;

	codec->mixer_ops = alc203_mixer_ops;
	return 0;
}

static int alc203_init(cmi_codec *codec)
{
	cmi8788_controller *controller;
	u8 reg_addr;
	u16 reg_data;

	if (!codec)
		return -1;

	controller = codec->controller;
	if (!controller)
		return -1;

	codec->addr = 0;
	codec->reg_len_flag = 0;

	reg_addr = 2; /* master volume */
	reg_data = 0x3f3f; /* left right channel 94.5dB Attenuation */
	controller->ops.ac97_cmd(codec, reg_addr, reg_data);

	reg_addr = 0xe; /* Mic volume */
	reg_data = 0x0000; /* left right channel +12dB Attenuation */
	controller->ops.ac97_cmd(codec, reg_addr, reg_data);

	reg_addr = 0x1a; /* record select */
	reg_data = 0x00; /* default Mic in */
	controller->ops.ac97_cmd(codec, reg_addr, reg_data);

	reg_addr = 0x1c; /* Record Gain Registers */
	reg_data = 0x0f0f; /* left right channel 22.5 dB gain */
	controller->ops.ac97_cmd(codec, reg_addr, reg_data);

	return 0;
}

static void alc203_free(cmi_codec *codec)
{
	/* 待完善 */

}

static cmi_codec_ops alc203_patch_ops = {
	.build_controls = alc203_build_controls,
	.build_pcms     = alc203_build_pcms,
	.init           = alc203_init,
	.free           = alc203_free,
};

static int patch_alc203(cmi_codec *codec)
{
	codec->patch_ops = alc203_patch_ops;
	return 0;
}

/*
 * patch entries
 */
codec_preset snd_preset_alc203[] = {
	{ .id = 0xFFFFFFFF, .name = "ALC203", .patch = patch_alc203 },
	{ } /* terminator */
};
