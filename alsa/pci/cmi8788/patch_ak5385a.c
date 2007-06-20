/*
 *  patch_ak5385.c - Driver for C-Media CMI8788 PCI soundcards.
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
 * audio interface patch for AK5385A
 */

#include <sound/driver.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <sound/core.h>
#include "cmi8788.h"

/*
 * 没有寄存器，不用专门的控制，只需要设置 CMI8788 就可以了
 */

static int ak5385a_init(struct cmi_codec *codec)
{
	codec->addr = CODEC_ADR_AK5385A; /* 0 */
	codec->reg_len_flag = 0;
	return 0;
}

struct cmi_codec_ops ak5385a_patch_ops = {
	.build_controls = NULL,
	.init           = NULL, /* ak5385a_init, */
};
