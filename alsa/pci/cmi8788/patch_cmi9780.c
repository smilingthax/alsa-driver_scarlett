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
#include "codec.h"
#include "cmi_controller.h"


/*
 * Analog playback callbacks
 */
static int cmi9780_playback_pcm_open(void                *hinfo,
				     cmi_codec           *codec,
				     struct snd_pcm_substream *substream )
{
	/* ŽýÍêÉÆ ÐèÒªÉèÖÃÏà¹ØµÄŒÄŽæÆ÷ */
	cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;

	return 0;
}

static int cmi9780_playback_pcm_prepare(void                *hinfo,
					cmi_codec           *codec,
					struct snd_pcm_substream *substream )
{
	/* ŽýÍêÉÆ ÐèÒªÉèÖÃÏà¹ØµÄŒÄŽæÆ÷ */
	cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;

	return 0;
}

static int cmi9780_playback_pcm_cleanup(void                *hinfo,
					cmi_codec           *codec,
					struct snd_pcm_substream *substream )
{
	/* ŽýÍêÉÆ ÐèÒªÉèÖÃÏà¹ØµÄŒÄŽæÆ÷ */
	cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;

	return 0;
}

/*
 * Analog capture
 */
static int cmi9780_capture_pcm_prepare(void                *hinfo,
				       cmi_codec           *codec,
				       struct snd_pcm_substream *substream )
{
	/* ŽýÍêÉÆ ÐèÒªÉèÖÃÏà¹ØµÄŒÄŽæÆ÷ */
	cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;

	return 0;
}

static int cmi9780_capture_pcm_cleanup(void                *hinfo,
				       cmi_codec           *codec,
				       struct snd_pcm_substream *substream )
{
	/* ŽýÍêÉÆ ÐèÒªÉèÖÃÏà¹ØµÄŒÄŽæÆ÷ */
	cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;

	return 0;
}


static cmi8788_pcm_stream cmi9780_pcm_analog_playback = {
	.channels = 2,
	.ops = {
		.open    = cmi9780_playback_pcm_open,
		.prepare = cmi9780_playback_pcm_prepare,
		.cleanup = cmi9780_playback_pcm_cleanup
	},
};

static cmi8788_pcm_stream cmi9780_pcm_analog_capture = {
	.channels = 2,
	.ops = {
		.prepare = cmi9780_capture_pcm_prepare,
		.cleanup = cmi9780_capture_pcm_cleanup
	},
};


static int cmi9780_build_pcms(cmi_codec *codec)
{
	cmi8788_pcm_stream  *pcm_substream = codec->pcm_substream;

	pcm_substream[0] = cmi9780_pcm_analog_playback;
	pcm_substream[1] = cmi9780_pcm_analog_capture;

	return 0;
}


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
static int get_info(cmi_codec *codec, int *min_vol, int *max_vol)
{
	if (!codec || !min_vol || !max_vol)
		return -1;

	*min_vol = 0;  /* mute */
	*max_vol = 32; /* 1-32 */

	cmi_printk(("   cmi9780 get_info(min %d, max %d)\n", *min_vol, *max_vol));

	return 0;
}

static int put_volume(cmi_codec *codec, int l_vol, int r_vol)
{
	cmi8788_controller *controller;
	int l_volume = 0, r_volume = 0;
	u32 val32 = 0;
	u8 reg_addr = 0x0E; /* Mic volume */
	u16 reg_data = 0x0808;
	int opera_source = MIC_VOL_SLIDER;

	if (!codec)
		return -1;
	controller = codec->controller;
	if (!controller)
		return -1;

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

	cmi_printk(("  AC97InChanCfg2 Write Reg 0x%0x = 0x%08x\n", AC97InChanCfg2, val32));
	controller->ops.ac97_cmd(codec, reg_addr, reg_data);

	return 0;
}

static int get_volume(cmi_codec *codec, int *l_vol, int *r_vol)
{
	int opera_source = MIC_VOL_SLIDER;

	if (!codec || !l_vol || !r_vol)
		return -1;

	opera_source = codec->volume_opera_source;

	cmi_printk(("   cmi9780 get_volume(opera_source %d, l_vol 0x%0x, r_vol 0x%0x)\n",opera_source,l_vol,r_vol));

	*l_vol = (int)(codec->volume[opera_source].left_vol);
	*r_vol = (int)(codec->volume[opera_source].right_vol);

	return 0;
}

static cmi8788_mixer_ops  cmi9780_mixer_ops =
{
	.get_info   = get_info,
	.get_volume = get_volume,
	.set_volume = put_volume,
};

/*
 * create mixer
 */
static int cmi9780_build_controls(cmi_codec *codec)
{
	if (!codec)
		return -1;

	codec->mixer_ops = cmi9780_mixer_ops;
	return 0;
}

static int cmi9780_init(cmi_codec *codec)
{
	cmi8788_controller *controller;
	int i = 0;

	cmi_printk(("  >> cmi9780_init\n"));

	if (!codec)
		return -1;

	controller = codec->controller;
	if (!controller)
		return -1;

	codec->addr = 0;
	codec->reg_len_flag = 0;

#if 0
	u8 reg_addr = 2; /* master volume */
	u16 reg_data = 0x1f1f; /* left right channel 46.5dB Attenuation */
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
#endif
	codec->left_vol  = 8;
	codec->right_vol = 8;

	codec->volume_opera_source = MIC_VOL_SLIDER;
	for (i = 0; i < MAX_VOL_SLIDER; i++) {
		codec->volume[i].left_vol = 8;
		codec->volume[i].right_vol = 8;
	}

	cmi_printk(("  << cmi9780_init\n"));

	return 0;
}

static void cmi9780_free(cmi_codec *codec)
{
	/* ŽýÍêÉÆ */

}

static cmi_codec_ops cmi9780_patch_ops = {
	.build_controls = cmi9780_build_controls,
	.build_pcms     = cmi9780_build_pcms,
	.init           = cmi9780_init,
	.free           = cmi9780_free,
};

static int patch_cmi9780(cmi_codec *codec)
{
	codec->patch_ops = cmi9780_patch_ops;
	return 0;
}

/*
 * patch entries
 */
codec_preset snd_preset_cmi9780[] = {
	{ .id = 0xFFFFFFFF, .name = "CMI9780", .patch = patch_cmi9780 },
	{ } /* terminator */
};
