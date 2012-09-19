/*
 * Driver for Digigram VXpocket 440 soundcard
 *
 * Copyright (c) 2002 by Takashi Iwai <tiwai@suse.de>
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
 */

/*
 * please add the following as /etc/pcmcia/vxpocket.conf:
 *

  device "snd-vxpocket"
     class "audio" module "snd-vxpocket"

  card "Digigram VX-POCKET440"
    manfid 0x01f1, 0x0100
    bind "snd-vxp440"

 */

#include <sound/driver.h>
#include <sound/core.h>
#include <pcmcia/version.h>
#include "vxpocket.h"
#define SNDRV_GET_ID
#include <sound/initval.h>


/*
 */

MODULE_AUTHOR("Takashi Iwai <tiwai@suse.de>");
MODULE_DESCRIPTION("Digigram VXpocket440");
MODULE_LICENSE("GPL");
MODULE_CLASSES("{sound}");
MODULE_DEVICES("{{Digigram,VXpocket440}}");

static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;	/* Index 0-MAX */
static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;	/* ID for this card */
static int enable[SNDRV_CARDS] = SNDRV_DEFAULT_ENABLE_PNP;	/* Enable switches */
static unsigned int irq_mask = 0xffff;
static int irq_list[4] = { -1 };

MODULE_PARM(index, "1-" __MODULE_STRING(SNDRV_CARDS) "i");
MODULE_PARM_DESC(index, "Index value for VXPocket440 soundcard.");
MODULE_PARM_SYNTAX(index, SNDRV_INDEX_DESC);
MODULE_PARM(id, "1-" __MODULE_STRING(SNDRV_CARDS) "s");
MODULE_PARM_DESC(id, "ID string for VXPocket440 soundcard.");
MODULE_PARM_SYNTAX(id, SNDRV_ID_DESC);
MODULE_PARM(enable, "1-" __MODULE_STRING(SNDRV_CARDS) "i");
MODULE_PARM_DESC(enable, "Enable VXPocket440 soundcard.");
MODULE_PARM_SYNTAX(enable, SNDRV_ENABLE_DESC);
MODULE_PARM(irq_mask, "i");
MODULE_PARM_DESC(irq_mask, "IRQ bitmask for VXPocket440 soundcard.");
MODULE_PARM(irq_list, "1-4i");
MODULE_PARM_DESC(irq_list, "List of Available interrupts for VXPocket440 soundcard.");
 

/*
 */

#include "xilinx_boot_vp4.c"
#include "xilinx_image_vp4.c"
#include "d563s3_boot.c"
#include "dsp_image_vp4.c"

static dev_info_t dev_info = "snd-vxp440";

static struct snd_vxp_entry hw_entry = {
	.name = "VXPocket440",
	.type = VXP_TYPE_VXP440,
	.dev_info = &dev_info,

	/* module parameters */
	.index_table = index,
	.id_table = id,
	.enable_table = enable,
	.irq_mask_p = &irq_mask,
	.irq_list = irq_list,

	/* images */
	.boot = { xilinx_boot_vxp440, sizeof(xilinx_boot_vxp440) },
	.xilinx = { xilinx_image_vxp440, sizeof(xilinx_image_vxp440) },
	.dsp_boot = { d563s3_boot, sizeof(d563s3_boot) },
	.dsp = { dsp_image_vxp440, sizeof(dsp_image_vxp440) },

	/* hardware specs */
	/* 1 DSP, 1 sync UER, 1 sync World Clock (NIY) */
	/* SMPTE (NIY) */
	/* 2 stereo analog input (line/micro) */
	/* 2 stereo analog output */
	/* Only output levels can be modified */
	/* UER, but only for the first two inputs and outputs. */
	.num_codecs = 2,
	.num_ins = 2,
	.num_outs = 2,
	
};

/*
 */
static dev_link_t *vxp_attach(void)
{
	return snd_vxpocket_attach(&hw_entry);
}

static void vxp_detach(dev_link_t *link)
{
	snd_vxpocket_detach(&hw_entry, link);
}


/*
 * Module entry points
 */

static int __init init_vxp440(void)
{
	servinfo_t serv;

	CardServices(GetCardServicesInfo, &serv);
	if (serv.Revision != CS_RELEASE_CODE) {
		printk(KERN_WARNING "init_vxp440: Card Services release does not match!\n");
		return -1;
	}
	register_pccard_driver(&dev_info, vxp_attach, vxp_detach);
	return 0;
}

static void __exit exit_vxp440(void)
{
        unregister_pccard_driver(&dev_info);
        snd_vxpocket_detach_all(&hw_entry);
}

module_init(init_vxp440);
module_exit(exit_vxp440);


EXPORT_NO_SYMBOLS; /* FIXME: for old kernels */
