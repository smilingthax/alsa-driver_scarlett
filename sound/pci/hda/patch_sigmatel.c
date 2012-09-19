/*
 * Universal Interface for Intel High Definition Audio Codec
 *
 * HD audio interface patch for SigmaTel STAC92xx
 *
 * Copyright (c) 2005 Embedded Alley Solutions, Inc.
 * Matt Porter <mporter@embeddedalley.com>
 *
 * Based on patch_cmedia.c and patch_realtek.c
 * Copyright (c) 2004 Takashi Iwai <tiwai@suse.de>
 *
 *  This driver is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This driver is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <sound/driver.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <sound/core.h>
#include <sound/asoundef.h>
#include "hda_codec.h"
#include "hda_local.h"

#define NUM_CONTROL_ALLOC	32
#define STAC_HP_EVENT		0x37
#define STAC_UNSOL_ENABLE 	(AC_USRSP_EN | STAC_HP_EVENT)

#define STAC_REF		0
#define STAC_D945GTP3		1
#define STAC_D945GTP5		2
#define STAC_MACMINI		3
#define STAC_D965_2112		4
#define STAC_D965_284B		5
#define STAC_922X_MODELS	6	/* number of 922x models */

struct sigmatel_spec {
	struct snd_kcontrol_new *mixers[4];
	unsigned int num_mixers;

	int board_config;
	unsigned int surr_switch: 1;
	unsigned int line_switch: 1;
	unsigned int mic_switch: 1;
	unsigned int alt_switch: 1;
	unsigned int hp_detect: 1;
	unsigned int gpio_mute: 1;

	/* playback */
	struct hda_multi_out multiout;
	hda_nid_t dac_nids[5];

	/* capture */
	hda_nid_t *adc_nids;
	unsigned int num_adcs;
	hda_nid_t *mux_nids;
	unsigned int num_muxes;
	hda_nid_t dig_in_nid;

	/* pin widgets */
	hda_nid_t *pin_nids;
	unsigned int num_pins;
	unsigned int *pin_configs;
	unsigned int *bios_pin_configs;

	/* codec specific stuff */
	struct hda_verb *init;
	struct snd_kcontrol_new *mixer;

	/* capture source */
	struct hda_input_mux *input_mux;
	unsigned int cur_mux[3];

	/* i/o switches */
	unsigned int io_switch[2];

	struct hda_pcm pcm_rec[2];	/* PCM information */

	/* dynamic controls and input_mux */
	struct auto_pin_cfg autocfg;
	unsigned int num_kctl_alloc, num_kctl_used;
	struct snd_kcontrol_new *kctl_alloc;
	struct hda_input_mux private_imux;
};

static hda_nid_t stac9200_adc_nids[1] = {
        0x03,
};

static hda_nid_t stac9200_mux_nids[1] = {
        0x0c,
};

static hda_nid_t stac9200_dac_nids[1] = {
        0x02,
};

static hda_nid_t stac922x_adc_nids[2] = {
        0x06, 0x07,
};

static hda_nid_t stac9227_adc_nids[2] = {
        0x07, 0x08,
};

#if 0
static hda_nid_t d965_2112_dac_nids[3] = {
        0x02, 0x03, 0x05,
};
#endif

static hda_nid_t stac922x_mux_nids[2] = {
        0x12, 0x13,
};

static hda_nid_t stac9227_mux_nids[2] = {
        0x15, 0x16,
};

static hda_nid_t stac927x_adc_nids[3] = {
        0x07, 0x08, 0x09
};

static hda_nid_t stac927x_mux_nids[3] = {
        0x15, 0x16, 0x17
};

static hda_nid_t stac9205_adc_nids[2] = {
        0x12, 0x13
};

static hda_nid_t stac9205_mux_nids[2] = {
        0x19, 0x1a
};

static hda_nid_t stac9200_pin_nids[8] = {
	0x08, 0x09, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12,
};

static hda_nid_t stac922x_pin_nids[10] = {
	0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
	0x0f, 0x10, 0x11, 0x15, 0x1b,
};

static hda_nid_t stac927x_pin_nids[14] = {
	0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
	0x0f, 0x10, 0x11, 0x12, 0x13,
	0x14, 0x21, 0x22, 0x23,
};

static hda_nid_t stac9205_pin_nids[12] = {
	0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
	0x0f, 0x14, 0x16, 0x17, 0x18,
	0x21, 0x22,
	
};

static int stac92xx_mux_enum_info(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_info *uinfo)
{
	struct hda_codec *codec = snd_kcontrol_chip(kcontrol);
	struct sigmatel_spec *spec = codec->spec;
	return snd_hda_input_mux_info(spec->input_mux, uinfo);
}

static int stac92xx_mux_enum_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct hda_codec *codec = snd_kcontrol_chip(kcontrol);
	struct sigmatel_spec *spec = codec->spec;
	unsigned int adc_idx = snd_ctl_get_ioffidx(kcontrol, &ucontrol->id);

	ucontrol->value.enumerated.item[0] = spec->cur_mux[adc_idx];
	return 0;
}

static int stac92xx_mux_enum_put(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct hda_codec *codec = snd_kcontrol_chip(kcontrol);
	struct sigmatel_spec *spec = codec->spec;
	unsigned int adc_idx = snd_ctl_get_ioffidx(kcontrol, &ucontrol->id);

	return snd_hda_input_mux_put(codec, spec->input_mux, ucontrol,
				     spec->mux_nids[adc_idx], &spec->cur_mux[adc_idx]);
}

static struct hda_verb stac9200_core_init[] = {
	/* set dac0mux for dac converter */
	{ 0x07, AC_VERB_SET_CONNECT_SEL, 0x00},
	{}
};

static struct hda_verb stac922x_core_init[] = {
	/* set master volume and direct control */	
	{ 0x16, AC_VERB_SET_VOLUME_KNOB_CONTROL, 0xff},
	{}
};

static struct hda_verb stac9227_core_init[] = {
	/* set master volume and direct control */	
	{ 0x16, AC_VERB_SET_VOLUME_KNOB_CONTROL, 0xff},
	/* unmute node 0x1b */
	{ 0x1b, AC_VERB_SET_AMP_GAIN_MUTE, 0xb000},
	{}
};

static struct hda_verb d965_2112_core_init[] = {
	/* set master volume and direct control */	
	{ 0x16, AC_VERB_SET_VOLUME_KNOB_CONTROL, 0xff},
	/* unmute node 0x1b */
	{ 0x1b, AC_VERB_SET_AMP_GAIN_MUTE, 0xb000},
	/* select node 0x03 as DAC */	
	{ 0x0b, AC_VERB_SET_CONNECT_SEL, 0x01},
	{}
};

static struct hda_verb stac927x_core_init[] = {
	/* set master volume and direct control */	
	{ 0x24, AC_VERB_SET_VOLUME_KNOB_CONTROL, 0xff},
	{}
};

static struct hda_verb stac9205_core_init[] = {
	/* set master volume and direct control */	
	{ 0x24, AC_VERB_SET_VOLUME_KNOB_CONTROL, 0xff},
	{}
};

static struct snd_kcontrol_new stac9200_mixer[] = {
	HDA_CODEC_VOLUME("Master Playback Volume", 0xb, 0, HDA_OUTPUT),
	HDA_CODEC_MUTE("Master Playback Switch", 0xb, 0, HDA_OUTPUT),
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "Input Source",
		.count = 1,
		.info = stac92xx_mux_enum_info,
		.get = stac92xx_mux_enum_get,
		.put = stac92xx_mux_enum_put,
	},
	HDA_CODEC_VOLUME("Capture Volume", 0x0a, 0, HDA_OUTPUT),
	HDA_CODEC_MUTE("Capture Switch", 0x0a, 0, HDA_OUTPUT),
	HDA_CODEC_VOLUME("Capture Mux Volume", 0x0c, 0, HDA_OUTPUT),
	{ } /* end */
};

/* This needs to be generated dynamically based on sequence */
static struct snd_kcontrol_new stac922x_mixer[] = {
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "Input Source",
		.count = 1,
		.info = stac92xx_mux_enum_info,
		.get = stac92xx_mux_enum_get,
		.put = stac92xx_mux_enum_put,
	},
	HDA_CODEC_VOLUME("Capture Volume", 0x17, 0x0, HDA_INPUT),
	HDA_CODEC_MUTE("Capture Switch", 0x17, 0x0, HDA_INPUT),
	HDA_CODEC_VOLUME("Mux Capture Volume", 0x12, 0x0, HDA_OUTPUT),
	{ } /* end */
};

/* This needs to be generated dynamically based on sequence */
static struct snd_kcontrol_new stac9227_mixer[] = {
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "Input Source",
		.count = 1,
		.info = stac92xx_mux_enum_info,
		.get = stac92xx_mux_enum_get,
		.put = stac92xx_mux_enum_put,
	},
	HDA_CODEC_VOLUME("Capture Volume", 0x15, 0x0, HDA_OUTPUT),
	HDA_CODEC_MUTE("Capture Switch", 0x1b, 0x0, HDA_OUTPUT),
	{ } /* end */
};

static snd_kcontrol_new_t stac927x_mixer[] = {
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "Input Source",
		.count = 1,
		.info = stac92xx_mux_enum_info,
		.get = stac92xx_mux_enum_get,
		.put = stac92xx_mux_enum_put,
	},
	HDA_CODEC_VOLUME("InMux Capture Volume", 0x15, 0x0, HDA_OUTPUT),
	HDA_CODEC_VOLUME("InVol Capture Volume", 0x18, 0x0, HDA_INPUT),
	HDA_CODEC_MUTE("ADCMux Capture Switch", 0x1b, 0x0, HDA_OUTPUT),
	{ } /* end */
};

static snd_kcontrol_new_t stac9205_mixer[] = {
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "Input Source",
		.count = 1,
		.info = stac92xx_mux_enum_info,
		.get = stac92xx_mux_enum_get,
		.put = stac92xx_mux_enum_put,
	},
	HDA_CODEC_VOLUME("InMux Capture Volume", 0x19, 0x0, HDA_OUTPUT),
	HDA_CODEC_VOLUME("InVol Capture Volume", 0x1b, 0x0, HDA_INPUT),
	HDA_CODEC_MUTE("ADCMux Capture Switch", 0x1d, 0x0, HDA_OUTPUT),
	{ } /* end */
};

static int stac92xx_build_controls(struct hda_codec *codec)
{
	struct sigmatel_spec *spec = codec->spec;
	int err;
	int i;

	err = snd_hda_add_new_ctls(codec, spec->mixer);
	if (err < 0)
		return err;

	for (i = 0; i < spec->num_mixers; i++) {
		err = snd_hda_add_new_ctls(codec, spec->mixers[i]);
		if (err < 0)
			return err;
	}

	if (spec->multiout.dig_out_nid) {
		err = snd_hda_create_spdif_out_ctls(codec, spec->multiout.dig_out_nid);
		if (err < 0)
			return err;
	}
	if (spec->dig_in_nid) {
		err = snd_hda_create_spdif_in_ctls(codec, spec->dig_in_nid);
		if (err < 0)
			return err;
	}
	return 0;	
}

static unsigned int ref9200_pin_configs[8] = {
	0x01c47010, 0x01447010, 0x0221401f, 0x01114010,
	0x02a19020, 0x01a19021, 0x90100140, 0x01813122,
};

static unsigned int *stac9200_brd_tbl[] = {
	ref9200_pin_configs,
};

static struct hda_board_config stac9200_cfg_tbl[] = {
	{ .modelname = "ref",
	  .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x2668,	/* DFI LanParty */
	  .config = STAC_REF },
	{} /* terminator */
};

static unsigned int ref922x_pin_configs[10] = {
	0x01014010, 0x01016011, 0x01012012, 0x0221401f,
	0x01813122, 0x01011014, 0x01441030, 0x01c41030,
	0x40000100, 0x40000100,
};

static unsigned int d945gtp3_pin_configs[10] = {
	0x0221401f, 0x01a19022, 0x01813021, 0x01014010,
	0x40000100, 0x40000100, 0x40000100, 0x40000100,
	0x02a19120, 0x40000100,
};

static unsigned int d945gtp5_pin_configs[10] = {
	0x0221401f, 0x01011012, 0x01813024, 0x01014010,
	0x01a19021, 0x01016011, 0x01452130, 0x40000100,
	0x02a19320, 0x40000100,
};

static unsigned int *stac922x_brd_tbl[STAC_922X_MODELS] = {
	[STAC_REF] =	ref922x_pin_configs,
	[STAC_D945GTP3] = d945gtp3_pin_configs,
	[STAC_D945GTP5] = d945gtp5_pin_configs,
	[STAC_MACMINI] = d945gtp5_pin_configs,
};

static struct hda_board_config stac922x_cfg_tbl[] = {
	{ .modelname = "ref",
	  .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x2668,	/* DFI LanParty */
	  .config = STAC_REF },		/* SigmaTel reference board */
         /* Intel 945G based systems */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x0101,
	  .config = STAC_D945GTP3 },	/* Intel D945GTP - 3 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x0202,
	  .config = STAC_D945GTP3 },	/* Intel D945GNT - 3 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x0606,
	  .config = STAC_D945GTP3 },	/* Intel D945GTP - 3 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x0601,
	  .config = STAC_D945GTP3 },	/* Intel D945GTP - 3 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x0111,
	  .config = STAC_D945GTP3 },	/* Intel D945GZP - 3 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x1115,
	  .config = STAC_D945GTP3 },	/* Intel D945GPM - 3 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x1116,
	  .config = STAC_D945GTP3 },	/* Intel D945GBO - 3 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x1117,
	  .config = STAC_D945GTP3 },	/* Intel D945GPM - 3 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x1118,
	  .config = STAC_D945GTP3 },	/* Intel D945GPM - 3 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x1119,
	  .config = STAC_D945GTP3 },	/* Intel D945GPM - 3 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x8826,
	  .config = STAC_D945GTP3 },	/* Intel D945GPM - 3 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x5049,
	  .config = STAC_D945GTP3 },	/* Intel D945GCZ - 3 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x5055,
	  .config = STAC_D945GTP3 },	/* Intel D945GCZ - 3 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x5048,
	  .config = STAC_D945GTP3 },	/* Intel D945GPB - 3 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x0110,
	  .config = STAC_D945GTP3 },	/* Intel D945GLR - 3 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x0404,
	  .config = STAC_D945GTP5 },	/* Intel D945GTP - 5 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x0303,
	  .config = STAC_D945GTP5 },	/* Intel D945GNT - 5 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x0013,
	  .config = STAC_D945GTP5 },	/* Intel D955XBK - 5 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x0417,
	  .config = STAC_D945GTP5 },	/* Intel D975XBK - 5 Stack */
	  /* Intel 945P based systems */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x0b0b,
	  .config = STAC_D945GTP3 },	/* Intel D945PSN - 3 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x0112,
	  .config = STAC_D945GTP3 },	/* Intel D945PLN - 3 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x0d0d,
	  .config = STAC_D945GTP3 },	/* Intel D945PLM - 3 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x0909,
	  .config = STAC_D945GTP3 },	/* Intel D945PAW - 3 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x0505,
	  .config = STAC_D945GTP3 },	/* Intel D945PLM - 3 Stack */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x0707,
	  .config = STAC_D945GTP5 },	/* Intel D945PSV - 5 Stack */
	  /* other systems  */
	{ .pci_subvendor = 0x8384,
	  .pci_subdevice = 0x7680,
	  .config = STAC_MACMINI },	/* Apple Mac Mini (early 2006) */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x2112,
	  .config = STAC_D965_2112 },
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x284b,
	  .config = STAC_D965_284B },
	{} /* terminator */
};

static unsigned int ref927x_pin_configs[14] = {
	0x01813122, 0x01a19021, 0x01014010, 0x01016011,
	0x01012012, 0x01011014, 0x40000100, 0x40000100, 
	0x40000100, 0x40000100, 0x40000100, 0x01441030,
	0x01c41030, 0x40000100,
};

static unsigned int d965_2112_pin_configs[14] = {
	0x0221401f, 0x02a19120, 0x40000100, 0x01014011,
	0x01a19021, 0x01813024, 0x40000100, 0x40000100,
	0x40000100, 0x40000100, 0x40000100, 0x40000100,
	0x40000100, 0x40000100
};

static unsigned int *stac927x_brd_tbl[] = {
	[STAC_REF] =	ref927x_pin_configs,
	[STAC_D965_2112] = d965_2112_pin_configs,
};

static struct hda_board_config stac927x_cfg_tbl[] = {
	{ .modelname = "ref",
	  .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x2668,	/* DFI LanParty */
	  .config = STAC_REF },		/* SigmaTel reference board */
	/* SigmaTel 9227 reference board */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x284b,
	  .config = STAC_D965_284B },
	 /* Intel 946 based systems */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x3d01,
	  .config = STAC_D965_2112 }, /* D946  configuration */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0xa301,
	  .config = STAC_D965_2112 }, /* Intel D946GZT - 3 stack  */
	/* 965 based systems */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x2116,
	  .config = STAC_D965_2112 }, /* Intel D965 3Stack config */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x2115,
	  .config = STAC_D965_2112 }, /* Intel DQ965WC - 3 Stack  */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x2114,
	  .config = STAC_D965_2112 }, /* Intel D965 3Stack config */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x2113,
	  .config = STAC_D965_2112 }, /* Intel D965 3Stack config */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x2112,
	  .config = STAC_D965_2112 }, /* Intel DG965MS - 3 Stack  */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x2111,
	  .config = STAC_D965_2112 }, /* Intel D965 3Stack config */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x2110,
	  .config = STAC_D965_2112 }, /* Intel D965 3Stack config */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x2009,
	  .config = STAC_D965_2112 }, /* Intel D965 3Stack config */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x2008,
	  .config = STAC_D965_2112 }, /* Intel DQ965GF - 3 Stack  */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x2007,
	  .config = STAC_D965_2112 }, /* Intel D965 3Stack config */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x2006,
	  .config = STAC_D965_2112 }, /* Intel D965 3Stack config */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x2005,
	  .config = STAC_D965_2112 }, /* Intel D965 3Stack config */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x2004,
	  .config = STAC_D965_2112 }, /* Intel D965 3Stack config */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x2003,
	  .config = STAC_D965_2112 }, /* Intel D965 3Stack config */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x2002,
	  .config = STAC_D965_2112 }, /* Intel D965 3Stack config */
	{ .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x2001,
	  .config = STAC_D965_2112 }, /* Intel DQ965GF - 3 Stackg */
	{} /* terminator */
};

static unsigned int ref9205_pin_configs[12] = {
	0x40000100, 0x40000100, 0x01016011, 0x01014010,
	0x01813122, 0x01a19021, 0x40000100, 0x40000100, 
	0x40000100, 0x40000100, 0x01441030, 0x01c41030
};

static unsigned int *stac9205_brd_tbl[] = {
	ref9205_pin_configs,
};

static struct hda_board_config stac9205_cfg_tbl[] = {
	{ .modelname = "ref",
	  .pci_subvendor = PCI_VENDOR_ID_INTEL,
	  .pci_subdevice = 0x2668,	/* DFI LanParty */
	  .config = STAC_REF },		/* SigmaTel reference board */
	/* Dell laptops have BIOS problem */
	{ .pci_subvendor = PCI_VENDOR_ID_DELL, .pci_subdevice = 0x01b5,
	  .config = STAC_REF },	/* Dell Inspiron 630m */
	{ .pci_subvendor = PCI_VENDOR_ID_DELL, .pci_subdevice = 0x01c2,
	  .config = STAC_REF },	/* Dell Latitude D620 */
	{ .pci_subvendor = PCI_VENDOR_ID_DELL, .pci_subdevice = 0x01cb,
	  .config = STAC_REF },	/* Dell Latitude 120L */
	{} /* terminator */
};

static int stac92xx_save_bios_config_regs(struct hda_codec *codec)
{
	int i;
	struct sigmatel_spec *spec = codec->spec;
	
	if (! spec->bios_pin_configs) {
		spec->bios_pin_configs = kcalloc(spec->num_pins,
		                                 sizeof(*spec->bios_pin_configs), GFP_KERNEL);
		if (! spec->bios_pin_configs)
			return -ENOMEM;
	}
	
	for (i = 0; i < spec->num_pins; i++) {
		hda_nid_t nid = spec->pin_nids[i];
		unsigned int pin_cfg;
		
		pin_cfg = snd_hda_codec_read(codec, nid, 0, 
			AC_VERB_GET_CONFIG_DEFAULT, 0x00);	
		snd_printdd(KERN_INFO "hda_codec: pin nid %2.2x bios pin config %8.8x\n",
					nid, pin_cfg);
		spec->bios_pin_configs[i] = pin_cfg;
	}
	
	return 0;
}

static void stac92xx_set_config_regs(struct hda_codec *codec)
{
	int i;
	struct sigmatel_spec *spec = codec->spec;
	unsigned int pin_cfg;

	if (! spec->pin_nids || ! spec->pin_configs)
		return;

	for (i = 0; i < spec->num_pins; i++) {
		snd_hda_codec_write(codec, spec->pin_nids[i], 0,
				    AC_VERB_SET_CONFIG_DEFAULT_BYTES_0,
				    spec->pin_configs[i] & 0x000000ff);
		snd_hda_codec_write(codec, spec->pin_nids[i], 0,
				    AC_VERB_SET_CONFIG_DEFAULT_BYTES_1,
				    (spec->pin_configs[i] & 0x0000ff00) >> 8);
		snd_hda_codec_write(codec, spec->pin_nids[i], 0,
				    AC_VERB_SET_CONFIG_DEFAULT_BYTES_2,
				    (spec->pin_configs[i] & 0x00ff0000) >> 16);
		snd_hda_codec_write(codec, spec->pin_nids[i], 0,
				    AC_VERB_SET_CONFIG_DEFAULT_BYTES_3,
				    spec->pin_configs[i] >> 24);
		pin_cfg = snd_hda_codec_read(codec, spec->pin_nids[i], 0,
					     AC_VERB_GET_CONFIG_DEFAULT,
					     0x00);	
		snd_printdd(KERN_INFO "hda_codec: pin nid %2.2x pin config %8.8x\n", spec->pin_nids[i], pin_cfg);
	}
}

/*
 * Analog playback callbacks
 */
static int stac92xx_playback_pcm_open(struct hda_pcm_stream *hinfo,
				      struct hda_codec *codec,
				      struct snd_pcm_substream *substream)
{
	struct sigmatel_spec *spec = codec->spec;
	return snd_hda_multi_out_analog_open(codec, &spec->multiout, substream);
}

static int stac92xx_playback_pcm_prepare(struct hda_pcm_stream *hinfo,
					 struct hda_codec *codec,
					 unsigned int stream_tag,
					 unsigned int format,
					 struct snd_pcm_substream *substream)
{
	struct sigmatel_spec *spec = codec->spec;
	return snd_hda_multi_out_analog_prepare(codec, &spec->multiout, stream_tag, format, substream);
}

static int stac92xx_playback_pcm_cleanup(struct hda_pcm_stream *hinfo,
					struct hda_codec *codec,
					struct snd_pcm_substream *substream)
{
	struct sigmatel_spec *spec = codec->spec;
	return snd_hda_multi_out_analog_cleanup(codec, &spec->multiout);
}

/*
 * Digital playback callbacks
 */
static int stac92xx_dig_playback_pcm_open(struct hda_pcm_stream *hinfo,
					  struct hda_codec *codec,
					  struct snd_pcm_substream *substream)
{
	struct sigmatel_spec *spec = codec->spec;
	return snd_hda_multi_out_dig_open(codec, &spec->multiout);
}

static int stac92xx_dig_playback_pcm_close(struct hda_pcm_stream *hinfo,
					   struct hda_codec *codec,
					   struct snd_pcm_substream *substream)
{
	struct sigmatel_spec *spec = codec->spec;
	return snd_hda_multi_out_dig_close(codec, &spec->multiout);
}


/*
 * Analog capture callbacks
 */
static int stac92xx_capture_pcm_prepare(struct hda_pcm_stream *hinfo,
					struct hda_codec *codec,
					unsigned int stream_tag,
					unsigned int format,
					struct snd_pcm_substream *substream)
{
	struct sigmatel_spec *spec = codec->spec;

	snd_hda_codec_setup_stream(codec, spec->adc_nids[substream->number],
                                   stream_tag, 0, format);
	return 0;
}

static int stac92xx_capture_pcm_cleanup(struct hda_pcm_stream *hinfo,
					struct hda_codec *codec,
					struct snd_pcm_substream *substream)
{
	struct sigmatel_spec *spec = codec->spec;

	snd_hda_codec_setup_stream(codec, spec->adc_nids[substream->number], 0, 0, 0);
	return 0;
}

static struct hda_pcm_stream stac92xx_pcm_digital_playback = {
	.substreams = 1,
	.channels_min = 2,
	.channels_max = 2,
	/* NID is set in stac92xx_build_pcms */
	.ops = {
		.open = stac92xx_dig_playback_pcm_open,
		.close = stac92xx_dig_playback_pcm_close
	},
};

static struct hda_pcm_stream stac92xx_pcm_digital_capture = {
	.substreams = 1,
	.channels_min = 2,
	.channels_max = 2,
	/* NID is set in stac92xx_build_pcms */
};

static struct hda_pcm_stream stac92xx_pcm_analog_playback = {
	.substreams = 1,
	.channels_min = 2,
	.channels_max = 8,
	.nid = 0x02, /* NID to query formats and rates */
	.ops = {
		.open = stac92xx_playback_pcm_open,
		.prepare = stac92xx_playback_pcm_prepare,
		.cleanup = stac92xx_playback_pcm_cleanup
	},
};

static struct hda_pcm_stream stac92xx_pcm_analog_alt_playback = {
	.substreams = 1,
	.channels_min = 2,
	.channels_max = 2,
	.nid = 0x06, /* NID to query formats and rates */
	.ops = {
		.open = stac92xx_playback_pcm_open,
		.prepare = stac92xx_playback_pcm_prepare,
		.cleanup = stac92xx_playback_pcm_cleanup
	},
};

static struct hda_pcm_stream stac92xx_pcm_analog_capture = {
	.substreams = 2,
	.channels_min = 2,
	.channels_max = 2,
	/* NID is set in stac92xx_build_pcms */
	.ops = {
		.prepare = stac92xx_capture_pcm_prepare,
		.cleanup = stac92xx_capture_pcm_cleanup
	},
};

static int stac92xx_build_pcms(struct hda_codec *codec)
{
	struct sigmatel_spec *spec = codec->spec;
	struct hda_pcm *info = spec->pcm_rec;

	codec->num_pcms = 1;
	codec->pcm_info = info;

	info->name = "STAC92xx Analog";
	info->stream[SNDRV_PCM_STREAM_PLAYBACK] = stac92xx_pcm_analog_playback;
	info->stream[SNDRV_PCM_STREAM_CAPTURE] = stac92xx_pcm_analog_capture;
	info->stream[SNDRV_PCM_STREAM_CAPTURE].nid = spec->adc_nids[0];

	if (spec->alt_switch) {
		codec->num_pcms++;
		info++;
		info->name = "STAC92xx Analog Alt";
		info->stream[SNDRV_PCM_STREAM_PLAYBACK] = stac92xx_pcm_analog_alt_playback;
	}

	if (spec->multiout.dig_out_nid || spec->dig_in_nid) {
		codec->num_pcms++;
		info++;
		info->name = "STAC92xx Digital";
		if (spec->multiout.dig_out_nid) {
			info->stream[SNDRV_PCM_STREAM_PLAYBACK] = stac92xx_pcm_digital_playback;
			info->stream[SNDRV_PCM_STREAM_PLAYBACK].nid = spec->multiout.dig_out_nid;
		}
		if (spec->dig_in_nid) {
			info->stream[SNDRV_PCM_STREAM_CAPTURE] = stac92xx_pcm_digital_capture;
			info->stream[SNDRV_PCM_STREAM_CAPTURE].nid = spec->dig_in_nid;
		}
	}

	return 0;
}

static unsigned int stac92xx_get_vref(struct hda_codec *codec, hda_nid_t nid)
{
	unsigned int pincap = snd_hda_param_read(codec, nid,
						 AC_PAR_PIN_CAP);
	pincap = (pincap & AC_PINCAP_VREF) >> AC_PINCAP_VREF_SHIFT;
	if (pincap & AC_PINCAP_VREF_100)
		return AC_PINCTL_VREF_100;
	if (pincap & AC_PINCAP_VREF_80)
		return AC_PINCTL_VREF_80;
	if (pincap & AC_PINCAP_VREF_50)
		return AC_PINCTL_VREF_50;
	if (pincap & AC_PINCAP_VREF_GRD)
		return AC_PINCTL_VREF_GRD;
	return 0;
}

static void stac92xx_auto_set_pinctl(struct hda_codec *codec, hda_nid_t nid, int pin_type)

{
	snd_hda_codec_write(codec, nid, 0, AC_VERB_SET_PIN_WIDGET_CONTROL, pin_type);
}

static int stac92xx_io_switch_info(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	return 0;
}

static int stac92xx_io_switch_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct hda_codec *codec = snd_kcontrol_chip(kcontrol);
	struct sigmatel_spec *spec = codec->spec;
	int io_idx = kcontrol-> private_value & 0xff;

	ucontrol->value.integer.value[0] = spec->io_switch[io_idx];
	return 0;
}

static int stac92xx_io_switch_put(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
        struct hda_codec *codec = snd_kcontrol_chip(kcontrol);
	struct sigmatel_spec *spec = codec->spec;
        hda_nid_t nid = kcontrol->private_value >> 8;
	int io_idx = kcontrol-> private_value & 0xff;
        unsigned short val = ucontrol->value.integer.value[0];

	spec->io_switch[io_idx] = val;

	if (val)
		stac92xx_auto_set_pinctl(codec, nid, AC_PINCTL_OUT_EN);
	else {
		unsigned int pinctl = AC_PINCTL_IN_EN;
		if (io_idx) /* set VREF for mic */
			pinctl |= stac92xx_get_vref(codec, nid);
		stac92xx_auto_set_pinctl(codec, nid, pinctl);
	}
        return 1;
}

#define STAC_CODEC_IO_SWITCH(xname, xpval) \
	{ .iface = SNDRV_CTL_ELEM_IFACE_MIXER, \
	  .name = xname, \
	  .index = 0, \
          .info = stac92xx_io_switch_info, \
          .get = stac92xx_io_switch_get, \
          .put = stac92xx_io_switch_put, \
          .private_value = xpval, \
	}


enum {
	STAC_CTL_WIDGET_VOL,
	STAC_CTL_WIDGET_MUTE,
	STAC_CTL_WIDGET_IO_SWITCH,
};

static struct snd_kcontrol_new stac92xx_control_templates[] = {
	HDA_CODEC_VOLUME(NULL, 0, 0, 0),
	HDA_CODEC_MUTE(NULL, 0, 0, 0),
	STAC_CODEC_IO_SWITCH(NULL, 0),
};

/* add dynamic controls */
static int stac92xx_add_control(struct sigmatel_spec *spec, int type, const char *name, unsigned long val)
{
	struct snd_kcontrol_new *knew;

	if (spec->num_kctl_used >= spec->num_kctl_alloc) {
		int num = spec->num_kctl_alloc + NUM_CONTROL_ALLOC;

		knew = kcalloc(num + 1, sizeof(*knew), GFP_KERNEL); /* array + terminator */
		if (! knew)
			return -ENOMEM;
		if (spec->kctl_alloc) {
			memcpy(knew, spec->kctl_alloc, sizeof(*knew) * spec->num_kctl_alloc);
			kfree(spec->kctl_alloc);
		}
		spec->kctl_alloc = knew;
		spec->num_kctl_alloc = num;
	}

	knew = &spec->kctl_alloc[spec->num_kctl_used];
	*knew = stac92xx_control_templates[type];
	knew->name = kstrdup(name, GFP_KERNEL);
	if (! knew->name)
		return -ENOMEM;
	knew->private_value = val;
	spec->num_kctl_used++;
	return 0;
}

/* flag inputs as additional dynamic lineouts */
static int stac92xx_add_dyn_out_pins(struct hda_codec *codec, struct auto_pin_cfg *cfg)
{
	struct sigmatel_spec *spec = codec->spec;

	switch (cfg->line_outs) {
	case 3:
		/* add line-in as side */
		if (cfg->input_pins[AUTO_PIN_LINE]) {
			cfg->line_out_pins[3] = cfg->input_pins[AUTO_PIN_LINE];
			spec->line_switch = 1;
			cfg->line_outs++;
		}
		break;
	case 2:
		/* add line-in as clfe and mic as side */
		if (cfg->input_pins[AUTO_PIN_LINE]) {
			cfg->line_out_pins[2] = cfg->input_pins[AUTO_PIN_LINE];
			spec->line_switch = 1;
			cfg->line_outs++;
		}
		if (cfg->input_pins[AUTO_PIN_MIC]) {
			cfg->line_out_pins[3] = cfg->input_pins[AUTO_PIN_MIC];
			spec->mic_switch = 1;
			cfg->line_outs++;
		}
		break;
	case 1:
		/* add line-in as surr and mic as clfe */
		if (cfg->input_pins[AUTO_PIN_LINE]) {
			cfg->line_out_pins[1] = cfg->input_pins[AUTO_PIN_LINE];
			spec->line_switch = 1;
			cfg->line_outs++;
		}
		if (cfg->input_pins[AUTO_PIN_MIC]) {
			cfg->line_out_pins[2] = cfg->input_pins[AUTO_PIN_MIC];
			spec->mic_switch = 1;
			cfg->line_outs++;
		}
		break;
	}

	return 0;
}

/*
 * XXX The line_out pin widget connection list may not be set to the
 * desired DAC nid. This is the case on 927x where ports A and B can
 * be routed to several DACs.
 *
 * This requires an analysis of the line-out/hp pin configuration
 * to provide a best fit for pin/DAC configurations that are routable.
 * For now, 927x DAC4 is not supported and 927x DAC1 output to ports
 * A and B is not supported.
 */
/* fill in the dac_nids table from the parsed pin configuration */
static int stac92xx_auto_fill_dac_nids(struct hda_codec *codec,
				       const struct auto_pin_cfg *cfg)
{
	struct sigmatel_spec *spec = codec->spec;
	hda_nid_t nid;
	int i;

	/* check the pins hardwired to audio widget */
	for (i = 0; i < cfg->line_outs; i++) {
		nid = cfg->line_out_pins[i];
		spec->multiout.dac_nids[i] = snd_hda_codec_read(codec, nid, 0,
					AC_VERB_GET_CONNECT_LIST, 0) & 0xff;
	}

	spec->multiout.num_dacs = cfg->line_outs;

	return 0;
}

/* add playback controls from the parsed DAC table */
static int stac92xx_auto_create_multi_out_ctls(struct sigmatel_spec *spec,
					       const struct auto_pin_cfg *cfg)
{
	char name[32];
	static const char *chname[4] = {
		"Front", "Surround", NULL /*CLFE*/, "Side"
	};
	hda_nid_t nid;
	int i, err;

	for (i = 0; i < cfg->line_outs; i++) {
		if (!spec->multiout.dac_nids[i])
			continue;

		nid = spec->multiout.dac_nids[i];

		if (i == 2) {
			/* Center/LFE */
			if ((err = stac92xx_add_control(spec, STAC_CTL_WIDGET_VOL, "Center Playback Volume",
					       HDA_COMPOSE_AMP_VAL(nid, 1, 0, HDA_OUTPUT))) < 0)
				return err;
			if ((err = stac92xx_add_control(spec, STAC_CTL_WIDGET_VOL, "LFE Playback Volume",
					       HDA_COMPOSE_AMP_VAL(nid, 2, 0, HDA_OUTPUT))) < 0)
				return err;
			if ((err = stac92xx_add_control(spec, STAC_CTL_WIDGET_MUTE, "Center Playback Switch",
					       HDA_COMPOSE_AMP_VAL(nid, 1, 0, HDA_OUTPUT))) < 0)
				return err;
			if ((err = stac92xx_add_control(spec, STAC_CTL_WIDGET_MUTE, "LFE Playback Switch",
					       HDA_COMPOSE_AMP_VAL(nid, 2, 0, HDA_OUTPUT))) < 0)
				return err;
		} else {
			sprintf(name, "%s Playback Volume", chname[i]);
			if ((err = stac92xx_add_control(spec, STAC_CTL_WIDGET_VOL, name,
					       HDA_COMPOSE_AMP_VAL(nid, 3, 0, HDA_OUTPUT))) < 0)
				return err;
			sprintf(name, "%s Playback Switch", chname[i]);
			if ((err = stac92xx_add_control(spec, STAC_CTL_WIDGET_MUTE, name,
					       HDA_COMPOSE_AMP_VAL(nid, 3, 0, HDA_OUTPUT))) < 0)
				return err;
		}
	}

	if (spec->line_switch)
		if ((err = stac92xx_add_control(spec, STAC_CTL_WIDGET_IO_SWITCH, "Line In as Output Switch", cfg->input_pins[AUTO_PIN_LINE] << 8)) < 0)
			return err;

	if (spec->mic_switch)
		if ((err = stac92xx_add_control(spec, STAC_CTL_WIDGET_IO_SWITCH, "Mic as Output Switch", (cfg->input_pins[AUTO_PIN_MIC] << 8) | 1)) < 0)
			return err;

	return 0;
}

/* add playback controls for HP output */
static int stac92xx_auto_create_hp_ctls(struct hda_codec *codec, struct auto_pin_cfg *cfg)
{
	struct sigmatel_spec *spec = codec->spec;
	hda_nid_t pin = cfg->hp_pin;
	hda_nid_t nid;
	int i, err;
	unsigned int wid_caps;

	if (! pin)
		return 0;

	wid_caps = get_wcaps(codec, pin);
	if (wid_caps & AC_WCAP_UNSOL_CAP)
		spec->hp_detect = 1;

	nid = snd_hda_codec_read(codec, pin, 0, AC_VERB_GET_CONNECT_LIST, 0) & 0xff;
	for (i = 0; i < cfg->line_outs; i++) {
		if (! spec->multiout.dac_nids[i])
			continue;
		if (spec->multiout.dac_nids[i] == nid)
			return 0;
	}

	spec->multiout.hp_nid = nid;

	/* control HP volume/switch on the output mixer amp */
	if ((err = stac92xx_add_control(spec, STAC_CTL_WIDGET_VOL, "Headphone Playback Volume",
					HDA_COMPOSE_AMP_VAL(nid, 3, 0, HDA_OUTPUT))) < 0)
		return err;
	if ((err = stac92xx_add_control(spec, STAC_CTL_WIDGET_MUTE, "Headphone Playback Switch",
					HDA_COMPOSE_AMP_VAL(nid, 3, 0, HDA_OUTPUT))) < 0)
		return err;

	return 0;
}

/* create playback/capture controls for input pins */
static int stac92xx_auto_create_analog_input_ctls(struct hda_codec *codec, const struct auto_pin_cfg *cfg)
{
	struct sigmatel_spec *spec = codec->spec;
	struct hda_input_mux *imux = &spec->private_imux;
	hda_nid_t con_lst[HDA_MAX_NUM_INPUTS];
	int i, j, k;

	for (i = 0; i < AUTO_PIN_LAST; i++) {
		int index = -1;
		if (cfg->input_pins[i]) {
			imux->items[imux->num_items].label = auto_pin_cfg_labels[i];

			for (j=0; j<spec->num_muxes; j++) {
				int num_cons = snd_hda_get_connections(codec, spec->mux_nids[j], con_lst, HDA_MAX_NUM_INPUTS);
				for (k=0; k<num_cons; k++)
					if (con_lst[k] == cfg->input_pins[i]) {
						index = k;
					 	break;
					}
				if (index >= 0)
					break;
			}
			imux->items[imux->num_items].index = index;
			imux->num_items++;
		}
	}

	if (imux->num_items == 1) {
		/*
		 * Set the current input for the muxes.
		 * The STAC9221 has two input muxes with identical source
		 * NID lists.  Hopefully this won't get confused.
		 */
		for (i = 0; i < spec->num_muxes; i++) {
			snd_hda_codec_write(codec, spec->mux_nids[i], 0,
					    AC_VERB_SET_CONNECT_SEL,
					    imux->items[0].index);
		}
	}

	return 0;
}

static void stac92xx_auto_init_multi_out(struct hda_codec *codec)
{
	struct sigmatel_spec *spec = codec->spec;
	int i;

	for (i = 0; i < spec->autocfg.line_outs; i++) {
		hda_nid_t nid = spec->autocfg.line_out_pins[i];
		stac92xx_auto_set_pinctl(codec, nid, AC_PINCTL_OUT_EN);
	}
}

static void stac92xx_auto_init_hp_out(struct hda_codec *codec)
{
	struct sigmatel_spec *spec = codec->spec;
	hda_nid_t pin;

	pin = spec->autocfg.hp_pin;
	if (pin) /* connect to front */
		stac92xx_auto_set_pinctl(codec, pin, AC_PINCTL_OUT_EN | AC_PINCTL_HP_EN);
}

static int stac92xx_parse_auto_config(struct hda_codec *codec, hda_nid_t dig_out, hda_nid_t dig_in)
{
	struct sigmatel_spec *spec = codec->spec;
	int err;

	if ((err = snd_hda_parse_pin_def_config(codec, &spec->autocfg, NULL)) < 0)
		return err;
	if (! spec->autocfg.line_outs)
		return 0; /* can't find valid pin config */

	if ((err = stac92xx_add_dyn_out_pins(codec, &spec->autocfg)) < 0)
		return err;
	if (spec->multiout.num_dacs == 0)
		if ((err = stac92xx_auto_fill_dac_nids(codec, &spec->autocfg)) < 0)
			return err;

	if ((err = stac92xx_auto_create_multi_out_ctls(spec, &spec->autocfg)) < 0 ||
	    (err = stac92xx_auto_create_hp_ctls(codec, &spec->autocfg)) < 0 ||
	    (err = stac92xx_auto_create_analog_input_ctls(codec, &spec->autocfg)) < 0)
		return err;

	spec->multiout.max_channels = spec->multiout.num_dacs * 2;
	if (spec->multiout.max_channels > 2)
		spec->surr_switch = 1;

	if (spec->autocfg.dig_out_pin)
		spec->multiout.dig_out_nid = dig_out;
	if (spec->autocfg.dig_in_pin)
		spec->dig_in_nid = dig_in;

	if (spec->kctl_alloc)
		spec->mixers[spec->num_mixers++] = spec->kctl_alloc;

	spec->input_mux = &spec->private_imux;

	return 1;
}

/* add playback controls for HP output */
static int stac9200_auto_create_hp_ctls(struct hda_codec *codec,
					struct auto_pin_cfg *cfg)
{
	struct sigmatel_spec *spec = codec->spec;
	hda_nid_t pin = cfg->hp_pin;
	unsigned int wid_caps;

	if (! pin)
		return 0;

	wid_caps = get_wcaps(codec, pin);
	if (wid_caps & AC_WCAP_UNSOL_CAP)
		spec->hp_detect = 1;

	return 0;
}

static int stac9200_parse_auto_config(struct hda_codec *codec)
{
	struct sigmatel_spec *spec = codec->spec;
	int err;

	if ((err = snd_hda_parse_pin_def_config(codec, &spec->autocfg, NULL)) < 0)
		return err;

	if ((err = stac92xx_auto_create_analog_input_ctls(codec, &spec->autocfg)) < 0)
		return err;

	if ((err = stac9200_auto_create_hp_ctls(codec, &spec->autocfg)) < 0)
		return err;

	if (spec->autocfg.dig_out_pin)
		spec->multiout.dig_out_nid = 0x05;
	if (spec->autocfg.dig_in_pin)
		spec->dig_in_nid = 0x04;

	if (spec->kctl_alloc)
		spec->mixers[spec->num_mixers++] = spec->kctl_alloc;

	spec->input_mux = &spec->private_imux;

	return 1;
}

/*
 * Early 2006 Intel Macintoshes with STAC9220X5 codecs seem to have a
 * funky external mute control using GPIO pins.
 */

static void stac922x_gpio_mute(struct hda_codec *codec, int pin, int muted)
{
	unsigned int gpiostate, gpiomask, gpiodir;

	gpiostate = snd_hda_codec_read(codec, codec->afg, 0,
				       AC_VERB_GET_GPIO_DATA, 0);

	if (!muted)
		gpiostate |= (1 << pin);
	else
		gpiostate &= ~(1 << pin);

	gpiomask = snd_hda_codec_read(codec, codec->afg, 0,
				      AC_VERB_GET_GPIO_MASK, 0);
	gpiomask |= (1 << pin);

	gpiodir = snd_hda_codec_read(codec, codec->afg, 0,
				     AC_VERB_GET_GPIO_DIRECTION, 0);
	gpiodir |= (1 << pin);

	/* AppleHDA seems to do this -- WTF is this verb?? */
	snd_hda_codec_write(codec, codec->afg, 0, 0x7e7, 0);

	snd_hda_codec_write(codec, codec->afg, 0,
			    AC_VERB_SET_GPIO_MASK, gpiomask);
	snd_hda_codec_write(codec, codec->afg, 0,
			    AC_VERB_SET_GPIO_DIRECTION, gpiodir);

	msleep(1);

	snd_hda_codec_write(codec, codec->afg, 0,
			    AC_VERB_SET_GPIO_DATA, gpiostate);
}

static int stac92xx_init(struct hda_codec *codec)
{
	struct sigmatel_spec *spec = codec->spec;
	struct auto_pin_cfg *cfg = &spec->autocfg;
	int i;

	snd_hda_sequence_write(codec, spec->init);

	/* set up pins */
	if (spec->hp_detect) {
		/* Enable unsolicited responses on the HP widget */
		snd_hda_codec_write(codec, cfg->hp_pin, 0,
				AC_VERB_SET_UNSOLICITED_ENABLE,
				STAC_UNSOL_ENABLE);
		/* fake event to set up pins */
		codec->patch_ops.unsol_event(codec, STAC_HP_EVENT << 26);
		/* enable the headphones by default.  If/when unsol_event detection works, this will be ignored */
		stac92xx_auto_init_hp_out(codec);
	} else {
		stac92xx_auto_init_multi_out(codec);
		stac92xx_auto_init_hp_out(codec);
	}
	for (i = 0; i < AUTO_PIN_LAST; i++) {
		hda_nid_t nid = cfg->input_pins[i];
		if (nid) {
			unsigned int pinctl = AC_PINCTL_IN_EN;
			if (i == AUTO_PIN_MIC || i == AUTO_PIN_FRONT_MIC)
				pinctl |= stac92xx_get_vref(codec, nid);
			stac92xx_auto_set_pinctl(codec, nid, pinctl);
		}
	}
	if (cfg->dig_out_pin)
		stac92xx_auto_set_pinctl(codec, cfg->dig_out_pin,
					 AC_PINCTL_OUT_EN);
	if (cfg->dig_in_pin)
		stac92xx_auto_set_pinctl(codec, cfg->dig_in_pin,
					 AC_PINCTL_IN_EN);

	if (spec->gpio_mute) {
		stac922x_gpio_mute(codec, 0, 0);
		stac922x_gpio_mute(codec, 1, 0);
	}

	return 0;
}

static void stac92xx_free(struct hda_codec *codec)
{
	struct sigmatel_spec *spec = codec->spec;
	int i;

	if (! spec)
		return;

	if (spec->kctl_alloc) {
		for (i = 0; i < spec->num_kctl_used; i++)
			kfree(spec->kctl_alloc[i].name);
		kfree(spec->kctl_alloc);
	}

	if (spec->bios_pin_configs)
		kfree(spec->bios_pin_configs);

	kfree(spec);
}

static void stac92xx_set_pinctl(struct hda_codec *codec, hda_nid_t nid,
				unsigned int flag)
{
	unsigned int pin_ctl = snd_hda_codec_read(codec, nid,
			0, AC_VERB_GET_PIN_WIDGET_CONTROL, 0x00);
	snd_hda_codec_write(codec, nid, 0,
			AC_VERB_SET_PIN_WIDGET_CONTROL,
			pin_ctl | flag);
}

static void stac92xx_reset_pinctl(struct hda_codec *codec, hda_nid_t nid,
				  unsigned int flag)
{
	unsigned int pin_ctl = snd_hda_codec_read(codec, nid,
			0, AC_VERB_GET_PIN_WIDGET_CONTROL, 0x00);
	snd_hda_codec_write(codec, nid, 0,
			AC_VERB_SET_PIN_WIDGET_CONTROL,
			pin_ctl & ~flag);
}

static void stac92xx_unsol_event(struct hda_codec *codec, unsigned int res)
{
	struct sigmatel_spec *spec = codec->spec;
	struct auto_pin_cfg *cfg = &spec->autocfg;
	int i, presence;

	if ((res >> 26) != STAC_HP_EVENT)
		return;

	presence = snd_hda_codec_read(codec, cfg->hp_pin, 0,
			AC_VERB_GET_PIN_SENSE, 0x00) >> 31;

	if (presence) {
		/* disable lineouts, enable hp */
		for (i = 0; i < cfg->line_outs; i++)
			stac92xx_reset_pinctl(codec, cfg->line_out_pins[i],
						AC_PINCTL_OUT_EN);
		stac92xx_set_pinctl(codec, cfg->hp_pin, AC_PINCTL_OUT_EN);
	} else {
		/* enable lineouts, disable hp */
		for (i = 0; i < cfg->line_outs; i++)
			stac92xx_set_pinctl(codec, cfg->line_out_pins[i],
						AC_PINCTL_OUT_EN);
		stac92xx_reset_pinctl(codec, cfg->hp_pin, AC_PINCTL_OUT_EN);
	}
} 

#ifdef CONFIG_PM
static int stac92xx_resume(struct hda_codec *codec)
{
	struct sigmatel_spec *spec = codec->spec;
	int i;

	stac92xx_init(codec);
	stac92xx_set_config_regs(codec);
	for (i = 0; i < spec->num_mixers; i++)
		snd_hda_resume_ctls(codec, spec->mixers[i]);
	if (spec->multiout.dig_out_nid)
		snd_hda_resume_spdif_out(codec);
	if (spec->dig_in_nid)
		snd_hda_resume_spdif_in(codec);

	return 0;
}
#endif

static struct hda_codec_ops stac92xx_patch_ops = {
	.build_controls = stac92xx_build_controls,
	.build_pcms = stac92xx_build_pcms,
	.init = stac92xx_init,
	.free = stac92xx_free,
	.unsol_event = stac92xx_unsol_event,
#ifdef CONFIG_PM
	.resume = stac92xx_resume,
#endif
};

static int patch_stac9200(struct hda_codec *codec)
{
	struct sigmatel_spec *spec;
	int err;

	spec  = kzalloc(sizeof(*spec), GFP_KERNEL);
	if (spec == NULL)
		return -ENOMEM;

	codec->spec = spec;
	spec->num_pins = 8;
	spec->pin_nids = stac9200_pin_nids;
	spec->board_config = snd_hda_check_board_config(codec, stac9200_cfg_tbl);
	if (spec->board_config < 0) {
		snd_printdd(KERN_INFO "hda_codec: Unknown model for STAC9200, using BIOS defaults\n");
		err = stac92xx_save_bios_config_regs(codec);
		if (err < 0) {
			stac92xx_free(codec);
			return err;
		}
		spec->pin_configs = spec->bios_pin_configs;
	} else {
		spec->pin_configs = stac9200_brd_tbl[spec->board_config];
		stac92xx_set_config_regs(codec);
	}

	spec->multiout.max_channels = 2;
	spec->multiout.num_dacs = 1;
	spec->multiout.dac_nids = stac9200_dac_nids;
	spec->adc_nids = stac9200_adc_nids;
	spec->mux_nids = stac9200_mux_nids;
	spec->num_muxes = 1;

	spec->init = stac9200_core_init;
	spec->mixer = stac9200_mixer;

	err = stac9200_parse_auto_config(codec);
	if (err < 0) {
		stac92xx_free(codec);
		return err;
	}

	codec->patch_ops = stac92xx_patch_ops;

	return 0;
}

static int patch_stac922x(struct hda_codec *codec)
{
	struct sigmatel_spec *spec;
	int err;

	spec  = kzalloc(sizeof(*spec), GFP_KERNEL);
	if (spec == NULL)
		return -ENOMEM;

	codec->spec = spec;
	spec->num_pins = 10;
	spec->pin_nids = stac922x_pin_nids;
	spec->board_config = snd_hda_check_board_config(codec, stac922x_cfg_tbl);
	if (spec->board_config < 0) {
		snd_printdd(KERN_INFO "hda_codec: Unknown model for STAC922x, "
			"using BIOS defaults\n");
		err = stac92xx_save_bios_config_regs(codec);
		if (err < 0) {
			stac92xx_free(codec);
			return err;
		}
		spec->pin_configs = spec->bios_pin_configs;
	} else if (stac922x_brd_tbl[spec->board_config] != NULL) {
		spec->pin_configs = stac922x_brd_tbl[spec->board_config];
		stac92xx_set_config_regs(codec);
	}

	spec->adc_nids = stac922x_adc_nids;
	spec->mux_nids = stac922x_mux_nids;
	spec->num_muxes = 2;

	spec->init = stac922x_core_init;
	spec->mixer = stac922x_mixer;

	spec->multiout.dac_nids = spec->dac_nids;
	
	err = stac92xx_parse_auto_config(codec, 0x08, 0x09);
	if (err < 0) {
		stac92xx_free(codec);
		return err;
	}

	if (spec->board_config == STAC_MACMINI)
		spec->gpio_mute = 1;

	codec->patch_ops = stac92xx_patch_ops;

	return 0;
}

static int patch_stac927x(struct hda_codec *codec)
{
	struct sigmatel_spec *spec;
	int err;

	spec  = kzalloc(sizeof(*spec), GFP_KERNEL);
	if (spec == NULL)
		return -ENOMEM;

	codec->spec = spec;
	spec->num_pins = 14;
	spec->pin_nids = stac927x_pin_nids;
	spec->board_config = snd_hda_check_board_config(codec, stac927x_cfg_tbl);
	if (spec->board_config < 0) {
                snd_printdd(KERN_INFO "hda_codec: Unknown model for STAC927x, using BIOS defaults\n");
		err = stac92xx_save_bios_config_regs(codec);
		if (err < 0) {
			stac92xx_free(codec);
			return err;
		}
		spec->pin_configs = spec->bios_pin_configs;
	} else if (stac927x_brd_tbl[spec->board_config] != NULL) {
		spec->pin_configs = stac927x_brd_tbl[spec->board_config];
		stac92xx_set_config_regs(codec);
	}

	switch (spec->board_config) {
	case STAC_D965_2112:
		spec->adc_nids = stac927x_adc_nids;
		spec->mux_nids = stac927x_mux_nids;
		spec->num_muxes = 3;
		spec->init = d965_2112_core_init;
		spec->mixer = stac9227_mixer;
		break;
	case STAC_D965_284B:
		spec->adc_nids = stac9227_adc_nids;
		spec->mux_nids = stac9227_mux_nids;
		spec->num_muxes = 2;
		spec->init = stac9227_core_init;
		spec->mixer = stac9227_mixer;
		break;
	default:
		spec->adc_nids = stac927x_adc_nids;
		spec->mux_nids = stac927x_mux_nids;
		spec->num_muxes = 3;
		spec->init = stac927x_core_init;
		spec->mixer = stac927x_mixer;
	}

	spec->multiout.dac_nids = spec->dac_nids;

	err = stac92xx_parse_auto_config(codec, 0x1e, 0x20);
	if (err < 0) {
		stac92xx_free(codec);
		return err;
	}

	codec->patch_ops = stac92xx_patch_ops;

	return 0;
}

static int patch_stac9205(struct hda_codec *codec)
{
	struct sigmatel_spec *spec;
	int err;

	spec  = kzalloc(sizeof(*spec), GFP_KERNEL);
	if (spec == NULL)
		return -ENOMEM;

	codec->spec = spec;
	spec->num_pins = 14;
	spec->pin_nids = stac9205_pin_nids;
	spec->board_config = snd_hda_check_board_config(codec, stac9205_cfg_tbl);
	if (spec->board_config < 0) {
		snd_printdd(KERN_INFO "hda_codec: Unknown model for STAC9205, using BIOS defaults\n");
		err = stac92xx_save_bios_config_regs(codec);
		if (err < 0) {
			stac92xx_free(codec);
			return err;
		}
		spec->pin_configs = spec->bios_pin_configs;
	} else {
		spec->pin_configs = stac9205_brd_tbl[spec->board_config];
		stac92xx_set_config_regs(codec);
	}

	spec->adc_nids = stac9205_adc_nids;
	spec->mux_nids = stac9205_mux_nids;
	spec->num_muxes = 3;

	spec->init = stac9205_core_init;
	spec->mixer = stac9205_mixer;

	spec->multiout.dac_nids = spec->dac_nids;

	err = stac92xx_parse_auto_config(codec, 0x1f, 0x20);
	if (err < 0) {
		stac92xx_free(codec);
		return err;
	}

	codec->patch_ops = stac92xx_patch_ops;

	return 0;
}

/*
 * STAC9872 hack
 */

/* static config for Sony VAIO FE550G and Sony VAIO AR */
static hda_nid_t vaio_dacs[] = { 0x2 };
#define VAIO_HP_DAC	0x5
static hda_nid_t vaio_adcs[] = { 0x8 /*,0x6*/ };
static hda_nid_t vaio_mux_nids[] = { 0x15 };

static struct hda_input_mux vaio_mux = {
	.num_items = 2,
	.items = {
		/* { "HP", 0x0 }, */
		{ "Line", 0x1 },
		{ "Mic", 0x2 },
		{ "PCM", 0x3 },
	}
};

static struct hda_verb vaio_init[] = {
	{0x0a, AC_VERB_SET_PIN_WIDGET_CONTROL, PIN_HP }, /* HP <- 0x2 */
	{0x0f, AC_VERB_SET_PIN_WIDGET_CONTROL, PIN_OUT }, /* Speaker <- 0x5 */
	{0x0d, AC_VERB_SET_PIN_WIDGET_CONTROL, PIN_VREF80 }, /* Mic? (<- 0x2) */
	{0x0e, AC_VERB_SET_PIN_WIDGET_CONTROL, PIN_IN }, /* CD */
	{0x14, AC_VERB_SET_PIN_WIDGET_CONTROL, PIN_VREF80 }, /* Mic? */
	{0x15, AC_VERB_SET_CONNECT_SEL, 0x2}, /* mic-sel: 0a,0d,14,02 */
	{0x02, AC_VERB_SET_AMP_GAIN_MUTE, AMP_OUT_MUTE}, /* HP */
	{0x05, AC_VERB_SET_AMP_GAIN_MUTE, AMP_OUT_MUTE}, /* Speaker */
	{0x09, AC_VERB_SET_AMP_GAIN_MUTE, AMP_IN_MUTE(0)}, /* capture sw/vol -> 0x8 */
	{0x07, AC_VERB_SET_AMP_GAIN_MUTE, AMP_IN_UNMUTE(0)}, /* CD-in -> 0x6 */
	{0x15, AC_VERB_SET_AMP_GAIN_MUTE, AMP_OUT_UNMUTE}, /* Mic-in -> 0x9 */
	{}
};

static struct hda_verb vaio_ar_init[] = {
	{0x0a, AC_VERB_SET_PIN_WIDGET_CONTROL, PIN_HP }, /* HP <- 0x2 */
	{0x0f, AC_VERB_SET_PIN_WIDGET_CONTROL, PIN_OUT }, /* Speaker <- 0x5 */
	{0x0d, AC_VERB_SET_PIN_WIDGET_CONTROL, PIN_VREF80 }, /* Mic? (<- 0x2) */
	{0x0e, AC_VERB_SET_PIN_WIDGET_CONTROL, PIN_IN }, /* CD */
/*	{0x11, AC_VERB_SET_PIN_WIDGET_CONTROL, PIN_OUT },*/ /* Optical Out */
	{0x14, AC_VERB_SET_PIN_WIDGET_CONTROL, PIN_VREF80 }, /* Mic? */
	{0x15, AC_VERB_SET_CONNECT_SEL, 0x2}, /* mic-sel: 0a,0d,14,02 */
	{0x02, AC_VERB_SET_AMP_GAIN_MUTE, AMP_OUT_MUTE}, /* HP */
	{0x05, AC_VERB_SET_AMP_GAIN_MUTE, AMP_OUT_MUTE}, /* Speaker */
/*	{0x10, AC_VERB_SET_AMP_GAIN_MUTE, AMP_OUT_MUTE},*/ /* Optical Out */
	{0x09, AC_VERB_SET_AMP_GAIN_MUTE, AMP_IN_MUTE(0)}, /* capture sw/vol -> 0x8 */
	{0x07, AC_VERB_SET_AMP_GAIN_MUTE, AMP_IN_UNMUTE(0)}, /* CD-in -> 0x6 */
	{0x15, AC_VERB_SET_AMP_GAIN_MUTE, AMP_OUT_UNMUTE}, /* Mic-in -> 0x9 */
	{}
};

/* bind volumes of both NID 0x02 and 0x05 */
static int vaio_master_vol_put(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_value *ucontrol)
{
	struct hda_codec *codec = snd_kcontrol_chip(kcontrol);
	long *valp = ucontrol->value.integer.value;
	int change;

	change = snd_hda_codec_amp_update(codec, 0x02, 0, HDA_OUTPUT, 0,
					  0x7f, valp[0] & 0x7f);
	change |= snd_hda_codec_amp_update(codec, 0x02, 1, HDA_OUTPUT, 0,
					   0x7f, valp[1] & 0x7f);
	snd_hda_codec_amp_update(codec, 0x05, 0, HDA_OUTPUT, 0,
				 0x7f, valp[0] & 0x7f);
	snd_hda_codec_amp_update(codec, 0x05, 1, HDA_OUTPUT, 0,
				 0x7f, valp[1] & 0x7f);
	return change;
}

/* bind volumes of both NID 0x02 and 0x05 */
static int vaio_master_sw_put(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct hda_codec *codec = snd_kcontrol_chip(kcontrol);
	long *valp = ucontrol->value.integer.value;
	int change;

	change = snd_hda_codec_amp_update(codec, 0x02, 0, HDA_OUTPUT, 0,
					  0x80, (valp[0] ? 0 : 0x80));
	change |= snd_hda_codec_amp_update(codec, 0x02, 1, HDA_OUTPUT, 0,
					   0x80, (valp[1] ? 0 : 0x80));
	snd_hda_codec_amp_update(codec, 0x05, 0, HDA_OUTPUT, 0,
				 0x80, (valp[0] ? 0 : 0x80));
	snd_hda_codec_amp_update(codec, 0x05, 1, HDA_OUTPUT, 0,
				 0x80, (valp[1] ? 0 : 0x80));
	return change;
}

static struct snd_kcontrol_new vaio_mixer[] = {
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "Master Playback Volume",
		.info = snd_hda_mixer_amp_volume_info,
		.get = snd_hda_mixer_amp_volume_get,
		.put = vaio_master_vol_put,
		.tlv = { .c = snd_hda_mixer_amp_tlv },
		.private_value = HDA_COMPOSE_AMP_VAL(0x02, 3, 0, HDA_OUTPUT),
	},
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "Master Playback Switch",
		.info = snd_hda_mixer_amp_switch_info,
		.get = snd_hda_mixer_amp_switch_get,
		.put = vaio_master_sw_put,
		.private_value = HDA_COMPOSE_AMP_VAL(0x02, 3, 0, HDA_OUTPUT),
	},
	/* HDA_CODEC_VOLUME("CD Capture Volume", 0x07, 0, HDA_INPUT), */
	HDA_CODEC_VOLUME("Capture Volume", 0x09, 0, HDA_INPUT),
	HDA_CODEC_MUTE("Capture Switch", 0x09, 0, HDA_INPUT),
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "Capture Source",
		.count = 1,
		.info = stac92xx_mux_enum_info,
		.get = stac92xx_mux_enum_get,
		.put = stac92xx_mux_enum_put,
	},
	{}
};

static struct snd_kcontrol_new vaio_ar_mixer[] = {
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "Master Playback Volume",
		.info = snd_hda_mixer_amp_volume_info,
		.get = snd_hda_mixer_amp_volume_get,
		.put = vaio_master_vol_put,
		.private_value = HDA_COMPOSE_AMP_VAL(0x02, 3, 0, HDA_OUTPUT),
	},
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "Master Playback Switch",
		.info = snd_hda_mixer_amp_switch_info,
		.get = snd_hda_mixer_amp_switch_get,
		.put = vaio_master_sw_put,
		.private_value = HDA_COMPOSE_AMP_VAL(0x02, 3, 0, HDA_OUTPUT),
	},
	/* HDA_CODEC_VOLUME("CD Capture Volume", 0x07, 0, HDA_INPUT), */
	HDA_CODEC_VOLUME("Capture Volume", 0x09, 0, HDA_INPUT),
	HDA_CODEC_MUTE("Capture Switch", 0x09, 0, HDA_INPUT),
	/*HDA_CODEC_MUTE("Optical Out Switch", 0x10, 0, HDA_OUTPUT),
	HDA_CODEC_VOLUME("Optical Out Volume", 0x10, 0, HDA_OUTPUT),*/
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "Capture Source",
		.count = 1,
		.info = stac92xx_mux_enum_info,
		.get = stac92xx_mux_enum_get,
		.put = stac92xx_mux_enum_put,
	},
	{}
};

static struct hda_codec_ops stac9872_patch_ops = {
	.build_controls = stac92xx_build_controls,
	.build_pcms = stac92xx_build_pcms,
	.init = stac92xx_init,
	.free = stac92xx_free,
#ifdef CONFIG_PM
	.resume = stac92xx_resume,
#endif
};

enum { /* FE and SZ series. id=0x83847661 and subsys=0x104D0700 or 104D1000. */
       CXD9872RD_VAIO,
       /* Unknown. id=0x83847662 and subsys=0x104D1200 or 104D1000. */
       STAC9872AK_VAIO, 
       /* Unknown. id=0x83847661 and subsys=0x104D1200. */
       STAC9872K_VAIO,
       /* AR Series. id=0x83847664 and subsys=104D1300 */
       CXD9872AKD_VAIO 
     };

static struct hda_board_config stac9872_cfg_tbl[] = {
	{ .modelname = "vaio", .config = CXD9872RD_VAIO },
	{ .modelname = "vaio-ar", .config = CXD9872AKD_VAIO },
	{ .pci_subvendor = 0x104d, .pci_subdevice = 0x81e6,
	  .config = CXD9872RD_VAIO },
	{ .pci_subvendor = 0x104d, .pci_subdevice = 0x81ef,
	  .config = CXD9872RD_VAIO },
	{ .pci_subvendor = 0x104d, .pci_subdevice = 0x81fd,
	  .config = CXD9872AKD_VAIO },
	{}
};

static int patch_stac9872(struct hda_codec *codec)
{
	struct sigmatel_spec *spec;
	int board_config;

	board_config = snd_hda_check_board_config(codec, stac9872_cfg_tbl);
	if (board_config < 0)
		/* unknown config, let generic-parser do its job... */
		return snd_hda_parse_generic_codec(codec);
	
	spec  = kzalloc(sizeof(*spec), GFP_KERNEL);
	if (spec == NULL)
		return -ENOMEM;

	codec->spec = spec;
	switch (board_config) {
	case CXD9872RD_VAIO:
	case STAC9872AK_VAIO:
	case STAC9872K_VAIO:
		spec->mixer = vaio_mixer;
		spec->init = vaio_init;
		spec->multiout.max_channels = 2;
		spec->multiout.num_dacs = ARRAY_SIZE(vaio_dacs);
		spec->multiout.dac_nids = vaio_dacs;
		spec->multiout.hp_nid = VAIO_HP_DAC;
		spec->num_adcs = ARRAY_SIZE(vaio_adcs);
		spec->adc_nids = vaio_adcs;
		spec->input_mux = &vaio_mux;
		spec->mux_nids = vaio_mux_nids;
		break;
	
	case CXD9872AKD_VAIO:
		spec->mixer = vaio_ar_mixer;
		spec->init = vaio_ar_init;
		spec->multiout.max_channels = 2;
		spec->multiout.num_dacs = ARRAY_SIZE(vaio_dacs);
		spec->multiout.dac_nids = vaio_dacs;
		spec->multiout.hp_nid = VAIO_HP_DAC;
		spec->num_adcs = ARRAY_SIZE(vaio_adcs);
		spec->adc_nids = vaio_adcs;
		spec->input_mux = &vaio_mux;
		spec->mux_nids = vaio_mux_nids;
		break;
	}

	codec->patch_ops = stac9872_patch_ops;
	return 0;
}


/*
 * patch entries
 */
struct hda_codec_preset snd_hda_preset_sigmatel[] = {
 	{ .id = 0x83847690, .name = "STAC9200", .patch = patch_stac9200 },
 	{ .id = 0x83847882, .name = "STAC9220 A1", .patch = patch_stac922x },
 	{ .id = 0x83847680, .name = "STAC9221 A1", .patch = patch_stac922x },
 	{ .id = 0x83847880, .name = "STAC9220 A2", .patch = patch_stac922x },
 	{ .id = 0x83847681, .name = "STAC9220D/9223D A2", .patch = patch_stac922x },
 	{ .id = 0x83847682, .name = "STAC9221 A2", .patch = patch_stac922x },
 	{ .id = 0x83847683, .name = "STAC9221D A2", .patch = patch_stac922x },
 	{ .id = 0x83847618, .name = "STAC9227", .patch = patch_stac927x },
 	{ .id = 0x83847619, .name = "STAC9227", .patch = patch_stac927x },
 	{ .id = 0x83847616, .name = "STAC9228", .patch = patch_stac927x },
 	{ .id = 0x83847617, .name = "STAC9228", .patch = patch_stac927x },
 	{ .id = 0x83847614, .name = "STAC9229", .patch = patch_stac927x },
 	{ .id = 0x83847615, .name = "STAC9229", .patch = patch_stac927x },
 	{ .id = 0x83847620, .name = "STAC9274", .patch = patch_stac927x },
 	{ .id = 0x83847621, .name = "STAC9274D", .patch = patch_stac927x },
 	{ .id = 0x83847622, .name = "STAC9273X", .patch = patch_stac927x },
 	{ .id = 0x83847623, .name = "STAC9273D", .patch = patch_stac927x },
 	{ .id = 0x83847624, .name = "STAC9272X", .patch = patch_stac927x },
 	{ .id = 0x83847625, .name = "STAC9272D", .patch = patch_stac927x },
 	{ .id = 0x83847626, .name = "STAC9271X", .patch = patch_stac927x },
 	{ .id = 0x83847627, .name = "STAC9271D", .patch = patch_stac927x },
 	{ .id = 0x83847628, .name = "STAC9274X5NH", .patch = patch_stac927x },
 	{ .id = 0x83847629, .name = "STAC9274D5NH", .patch = patch_stac927x },
 	/* The following does not take into account .id=0x83847661 when subsys =
 	 * 104D0C00 which is STAC9225s. Because of this, some SZ Notebooks are
 	 * currently not fully supported.
 	 */
 	{ .id = 0x83847661, .name = "CXD9872RD/K", .patch = patch_stac9872 },
 	{ .id = 0x83847662, .name = "STAC9872AK", .patch = patch_stac9872 },
 	{ .id = 0x83847664, .name = "CXD9872AKD", .patch = patch_stac9872 },
 	{ .id = 0x838476a0, .name = "STAC9205", .patch = patch_stac9205 },
 	{ .id = 0x838476a1, .name = "STAC9205D", .patch = patch_stac9205 },
 	{ .id = 0x838476a2, .name = "STAC9204", .patch = patch_stac9205 },
 	{ .id = 0x838476a3, .name = "STAC9204D", .patch = patch_stac9205 },
 	{ .id = 0x838476a4, .name = "STAC9255", .patch = patch_stac9205 },
 	{ .id = 0x838476a5, .name = "STAC9255D", .patch = patch_stac9205 },
 	{ .id = 0x838476a6, .name = "STAC9254", .patch = patch_stac9205 },
 	{ .id = 0x838476a7, .name = "STAC9254D", .patch = patch_stac9205 },
	{} /* terminator */
};
