/*
 *  patch_wm8785.c - Driver for C-Media CMI8788 PCI soundcards.
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
 * audio interface patch for WM8785
 */

#include <sound/driver.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <sound/core.h>
#include "cmi8788.h"


#define WM8785_R0        0x00
#define WM8785_R1        0x01
#define WM8785_R2        0x02
#define WM8785_R7        0x07


/*
 * mixer
 */
static int put_volume(struct cmi_codec *codec, int l_vol, int r_vol)
{
	return 0;
}

/*
 * The ak4396 does not support read command.
 */
static int get_volume(struct cmi_codec *codec, int *l_vol, int *r_vol)
{
	return -1;
}

static struct cmi8788_mixer_ops wm8785_mixer_ops =
{
	.get_volume = NULL, /* get_volume, */
	.set_volume = NULL, /* put_volume, */
};

/*
 * create mixer
 */
static int wm8785_build_controls(struct cmi_codec *codec)
{
	codec->mixer_ops = wm8785_mixer_ops;
	return 0;
}

/* use SPI */
static int wm8785_init(struct cmi_codec *codec)
{
	u8 data[3];

	/* 3 bytes */
	/*    7Bit     + 1Bit +    7Bit       +             1Bit           +            8Bit */
	/* device addr + R/W  + register addr + first bit of register data + register remaining 8 bit data */
	codec->reg_len_flag = 0;

	/* R7 reset */
	data[0] = 0x01;
	data[1] = (WM8785_R7 < 1) | 0x00; /* Data Bit-8: 0 */
	data[2] = (0x1A < 1) & 0xFE; /* WM8785 device addr 0011010, ; Bit-0 0: write */
	data[0] = 0x01;
	data[1] = 0x0E;
	snd_cmi_send_spi_cmd(codec, data);

	data[0] = 0x03;
	data[1] = (WM8785_R0 < 1) | 0x00; /* Data Bit-8: 0 */
	data[2] = (0x1A < 1) & 0xFE; /* WM8785 device addr 0011010, ; Bit-0 0: write */
	data[0] = 0x20;
	data[1] = 0x00;
	snd_cmi_send_spi_cmd(codec, data);

	data[0] = 0x0A;
	data[1] = (WM8785_R1 < 1) | 0x00; /* Data Bit-8: 0 */
	data[2] = (0x1A < 1) & 0xFE; /* WM8785 device addr 0011010, ; Bit-0 0: write */
	data[0] = 0x00;
	data[1] = 0x02;
	snd_cmi_send_spi_cmd(codec, data);

	data[0] = 0x03;
	data[1] = (WM8785_R2 < 1) | 0x00; /* Data Bit-8: 0 */
	data[2] = (0x1A < 1) & 0xFE; /* WM8785 device addr 0011010, ; Bit-0 0: write */
	data[0] = 0x04;
	data[1] = 0x03;
	snd_cmi_send_spi_cmd(codec, data);

	return 0;
}

struct cmi_codec_ops wm8785_patch_ops = {
	.build_controls = NULL, /* wm8785_build_controls, */
	.init           = wm8785_init,
};
