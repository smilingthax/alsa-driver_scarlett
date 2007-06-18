/*
 *  codec.c - Driver for C-Media CMI8788 PCI soundcards.
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

#include <sound/driver.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <sound/core.h>

#include "cmi8788.h"
#include "codec.h"
#include "cmi_controller.h"


void snd_cmi8788_codec_free(cmi_codec *codec)
{
	if (!codec)
		return;

	/* 释放 codec 内部申请的资源, codec 在外部释放, 在 controller 中释放 */
	if (codec && codec->patch_ops.free)
		codec->patch_ops.free(codec);
}

/**
 * snd_cmi8788_codec_new - create a codec
 * Returns 0 if successful, or a negative error code.
 */
int snd_cmi8788_codec_new(cmi8788_controller *controller, cmi_codec *codec, u32 addr, codec_preset *preset)
{
	int err;

	snd_assert(controller, return -EINVAL);

	codec->controller = controller;

	/* 可以不需要下面的这些信息,具体要在对应的 CODEC 里面重新赋值 */
	codec->vendor_id    = 0xFFFF;
	codec->subsystem_id = 0XFFFF;
	codec->revision_id  = 0XFFFF;

	codec->addr   = addr;
	codec->preset = preset;

	if (codec->preset && codec->preset->patch)
		err = codec->preset->patch(codec);
	else
		err = -1;

	if (err < 0) {
		if (codec && codec->patch_ops.free)
			codec->patch_ops.free(codec);
		return err;
	}

	return 0;
}
