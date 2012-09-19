/*
 * C-Media CMI8788 driver for Asus Xonar cards
 *
 * Copyright (c) Clemens Ladisch <clemens@ladisch.de>
 *
 *
 *  This driver is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License, version 2.
 *
 *  This driver is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this driver; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

/*
 * CMI8788:
 *
 * SPI 0 -> 1st PCM1796 (front)
 * SPI 1 -> 2nd PCM1796 (surround)
 * SPI 2 -> 3rd PCM1796 (center/LFE)
 * SPI 4 -> 4th PCM1796 (back)
 *
 * GPIO 2 -> M0 of CS5381
 * GPIO 3 -> M1 of CS5381
 * GPIO 5 <- external power present (D2X only)
 * GPIO 7 -> ALT
 * GPIO 8 -> enable output to speakers
 */

#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <sound/ac97_codec.h>
#include <sound/control.h>
#include <sound/core.h>
#include <sound/initval.h>
#include <sound/pcm.h>
#include <sound/tlv.h>
#include "oxygen.h"
#include "cm9780.h"
#include "pcm1796.h"

MODULE_AUTHOR("Clemens Ladisch <clemens@ladisch.de>");
MODULE_DESCRIPTION("Asus AV200 driver");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("{{Asus,AV200}}");

static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;
static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;
static int enable[SNDRV_CARDS] = SNDRV_DEFAULT_ENABLE_PNP;

module_param_array(index, int, NULL, 0444);
MODULE_PARM_DESC(index, "card index");
module_param_array(id, charp, NULL, 0444);
MODULE_PARM_DESC(id, "ID string");
module_param_array(enable, bool, NULL, 0444);
MODULE_PARM_DESC(enable, "enable card");

static struct pci_device_id xonar_ids[] __devinitdata = {
	{ OXYGEN_PCI_SUBID(0x1043, 0x8269) }, /* Asus Xonar D2 */
	{ OXYGEN_PCI_SUBID(0x1043, 0x82b7) }, /* Asus Xonar D2X */
	{ }
};
MODULE_DEVICE_TABLE(pci, xonar_ids);


#define GPIO_CS5381_M_MASK	0x000c
#define GPIO_CS5381_M_SINGLE	0x0000
#define GPIO_CS5381_M_DOUBLE	0x0004
#define GPIO_CS5381_M_QUAD	0x0008
#define GPIO_EXT_POWER		0x0020
#define GPIO_ALT		0x0080
#define GPIO_OUTPUT_ENABLE	0x0100

struct xonar_data {
	u8 is_d2x;
	u8 has_power;
};

static void pcm1796_write(struct oxygen *chip, unsigned int codec,
			  u8 reg, u8 value)
{
	/* maps ALSA channel pair number to SPI output */
	static const u8 codec_map[4] = {
		0, 1, 2, 4
	};
	oxygen_write_spi(chip, OXYGEN_SPI_TRIGGER  |
			 OXYGEN_SPI_DATA_LENGTH_2 |
			 OXYGEN_SPI_CLOCK_160 |
			 (codec_map[codec] << OXYGEN_SPI_CODEC_SHIFT) |
			 OXYGEN_SPI_CEN_LATCH_CLOCK_HI,
			 (reg << 8) | value);
}

static void xonar_init(struct oxygen *chip)
{
	struct xonar_data *data = chip->model_data;
	unsigned int i;

	data->is_d2x = chip->pci->subsystem_device == 0x82b7;

	for (i = 0; i < 4; ++i) {
		pcm1796_write(chip, i, 18, PCM1796_FMT_24_LJUST | PCM1796_ATLD);
		pcm1796_write(chip, i, 19, PCM1796_FLT_SHARP | PCM1796_ATS_1);
		pcm1796_write(chip, i, 20, PCM1796_OS_64);
		pcm1796_write(chip, i, 21, 0);
		pcm1796_write(chip, i, 16, 0xff); /* set ATL/ATR after ATLD */
		pcm1796_write(chip, i, 17, 0xff);
	}

	oxygen_set_bits16(chip, OXYGEN_GPIO_CONTROL,
			  GPIO_CS5381_M_MASK | GPIO_ALT);
	oxygen_write16_masked(chip, OXYGEN_GPIO_DATA,
			      GPIO_CS5381_M_SINGLE,
			      GPIO_CS5381_M_MASK | GPIO_ALT);
	if (data->is_d2x) {
		oxygen_clear_bits16(chip, OXYGEN_GPIO_CONTROL,
				    GPIO_EXT_POWER);
		oxygen_set_bits16(chip, OXYGEN_GPIO_INTERRUPT_MASK,
				  GPIO_EXT_POWER);
		chip->interrupt_mask |= OXYGEN_INT_GPIO;
		data->has_power = !!(oxygen_read16(chip, OXYGEN_GPIO_DATA)
				     & GPIO_EXT_POWER);
	}
	oxygen_ac97_set_bits(chip, 0, CM9780_JACK, CM9780_FMIC2MIC);
	msleep(300);
	oxygen_set_bits16(chip, OXYGEN_GPIO_CONTROL, GPIO_OUTPUT_ENABLE);
	oxygen_set_bits16(chip, OXYGEN_GPIO_DATA, GPIO_OUTPUT_ENABLE);

	snd_component_add(chip->card, "PCM1796");
	snd_component_add(chip->card, "CS5381");
}

static void xonar_cleanup(struct oxygen *chip)
{
	oxygen_clear_bits16(chip, OXYGEN_GPIO_DATA, GPIO_OUTPUT_ENABLE);
}

static void set_pcm1796_params(struct oxygen *chip,
			       struct snd_pcm_hw_params *params)
{
#if 0
	unsigned int i;
	u8 value;

	value = params_rate(params) >= 96000 ? PCM1796_OS_32 : PCM1796_OS_64;
	for (i = 0; i < 4; ++i)
		pcm1796_write(chip, i, 20, value);
#endif
}

static void update_pcm1796_volume(struct oxygen *chip)
{
	unsigned int i;

	for (i = 0; i < 4; ++i) {
		pcm1796_write(chip, i, 16, chip->dac_volume[i * 2]);
		pcm1796_write(chip, i, 17, chip->dac_volume[i * 2 + 1]);
	}
}

static void update_pcm1796_mute(struct oxygen *chip)
{
	unsigned int i;
	u8 value;

	value = PCM1796_FMT_24_LJUST | PCM1796_ATLD;
	if (chip->dac_mute)
		value |= PCM1796_MUTE;
	for (i = 0; i < 4; ++i)
		pcm1796_write(chip, i, 18, value);
}

static void set_cs5381_params(struct oxygen *chip,
			      struct snd_pcm_hw_params *params)
{
	unsigned int value;

	if (params_rate(params) <= 54000)
		value = GPIO_CS5381_M_SINGLE;
	else if (params_rate(params) <= 108000)
		value = GPIO_CS5381_M_DOUBLE;
	else
		value = GPIO_CS5381_M_QUAD;
	oxygen_write16_masked(chip, OXYGEN_GPIO_DATA,
			      value, GPIO_CS5381_M_MASK);
}

static void xonar_gpio_changed(struct oxygen *chip)
{
	struct xonar_data *data = chip->model_data;
	u8 has_power;

	if (!data->is_d2x)
		return;
	has_power = !!(oxygen_read16(chip, OXYGEN_GPIO_DATA)
		       & GPIO_EXT_POWER);
	if (has_power != data->has_power) {
		data->has_power = has_power;
		if (has_power) {
			snd_printk(KERN_NOTICE "power restored\n");
		} else {
			snd_printk(KERN_CRIT
				   "Hey! Don't unplug the power cable!\n");
			/* TODO: stop PCMs */
		}
	}
}

static int pcm1796_volume_info(struct snd_kcontrol *ctl,
			       struct snd_ctl_elem_info *info)
{
	info->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	info->count = 8;
	info->value.integer.min = 0x0f;
	info->value.integer.max = 0xff;
	return 0;
}

static int alt_switch_get(struct snd_kcontrol *ctl,
			  struct snd_ctl_elem_value *value)
{
	struct oxygen *chip = ctl->private_data;

	value->value.integer.value[0] =
		!!(oxygen_read16(chip, OXYGEN_GPIO_DATA) & GPIO_ALT);
	return 0;
}

static int alt_switch_put(struct snd_kcontrol *ctl,
			  struct snd_ctl_elem_value *value)
{
	struct oxygen *chip = ctl->private_data;
	u16 old_bits, new_bits;
	int changed;

	spin_lock_irq(&chip->reg_lock);
	old_bits = oxygen_read16(chip, OXYGEN_GPIO_DATA);
	if (value->value.integer.value[0])
		new_bits = old_bits | GPIO_ALT;
	else
		new_bits = old_bits & ~GPIO_ALT;
	changed = new_bits != old_bits;
	if (changed)
		oxygen_write16(chip, OXYGEN_GPIO_DATA, new_bits);
	spin_unlock_irq(&chip->reg_lock);
	return changed;
}

static const struct snd_kcontrol_new alt_switch = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.name = "Analog Loopback Switch",
	.info = snd_ctl_boolean_mono_info,
	.get = alt_switch_get,
	.put = alt_switch_put,
};

static const DECLARE_TLV_DB_SCALE(pcm1796_db_scale, -12000, 50, 0);

static int xonar_control_filter(struct snd_kcontrol_new *template)
{
	if (!strcmp(template->name, "Master Playback Volume")) {
		template->access |= SNDRV_CTL_ELEM_ACCESS_TLV_READ;
		template->info = pcm1796_volume_info,
		template->tlv.p = pcm1796_db_scale;
	} else if (!strncmp(template->name, "CD Capture ", 11)) {
		/* CD in is actually connected to the video in pin */
		template->private_value ^= AC97_CD ^ AC97_VIDEO;
	}
	return 0;
}

static int xonar_mixer_init(struct oxygen *chip)
{
	return snd_ctl_add(chip->card, snd_ctl_new1(&alt_switch, chip));
}

static const struct oxygen_model model_xonar = {
	.shortname = "Asus AV200",
	.longname = "Asus Virtuoso 200",
	.chip = "AV200",
	.owner = THIS_MODULE,
	.init = xonar_init,
	.control_filter = xonar_control_filter,
	.mixer_init = xonar_mixer_init,
	.cleanup = xonar_cleanup,
	.set_dac_params = set_pcm1796_params,
	.set_adc_params = set_cs5381_params,
	.update_dac_volume = update_pcm1796_volume,
	.update_dac_mute = update_pcm1796_mute,
	.gpio_changed = xonar_gpio_changed,
	.model_data_size = sizeof(struct xonar_data),
	.pcm_dev_cfg = PLAYBACK_0_TO_I2S |
		       PLAYBACK_1_TO_SPDIF |
		       CAPTURE_0_FROM_I2S_2 |
		       CAPTURE_1_FROM_SPDIF,
	.dac_channels = 8,
	.misc_flags = OXYGEN_MISC_MIDI,
	.function_flags = OXYGEN_FUNCTION_SPI |
			  OXYGEN_FUNCTION_ENABLE_SPI_4_5,
	.dac_i2s_format = OXYGEN_I2S_FORMAT_LJUST,
	.adc_i2s_format = OXYGEN_I2S_FORMAT_LJUST,
};

static int __devinit xonar_probe(struct pci_dev *pci,
				 const struct pci_device_id *pci_id)
{
	static int dev;
	int err;

	if (dev >= SNDRV_CARDS)
		return -ENODEV;
	if (!enable[dev]) {
		++dev;
		return -ENOENT;
	}
	err = oxygen_pci_probe(pci, index[dev], id[dev], &model_xonar);
	if (err >= 0)
		++dev;
	return err;
}

static struct pci_driver xonar_driver = {
	.name = "AV200",
	.id_table = xonar_ids,
	.probe = xonar_probe,
	.remove = __devexit_p(oxygen_pci_remove),
};

static int __init alsa_card_xonar_init(void)
{
	return pci_register_driver(&xonar_driver);
}

static void __exit alsa_card_xonar_exit(void)
{
	pci_unregister_driver(&xonar_driver);
}

module_init(alsa_card_xonar_init)
module_exit(alsa_card_xonar_exit)
