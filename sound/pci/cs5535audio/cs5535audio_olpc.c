#include <sound/driver.h>
#include <sound/core.h>
#include <sound/info.h>
#include <sound/control.h>
#include <sound/ac97_codec.h>

#include <asm/olpc.h>
#include "cs5535audio.h"

/* OLPC has an additional feature on top of regular AD1888 codec
features. This is support for an analog input mode. This is a
2 step process. First, to turn off the AD1888 codec bias voltage
and high pass filter. Second, to tell the embedded controller to
reroute from a capacitive trace to a direct trace using an analog
switch. The *_ec()s are what talk to that controller */

static int snd_cs5535audio_ctl_info(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	return 0;
}

#define AD1888_VREFOUT_EN_BIT (1 << 2)
#define AD1888_HPF_EN_BIT (1 << 12)
static int snd_cs5535audio_ctl_get(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct cs5535audio *cs5535au = snd_kcontrol_chip(kcontrol);
	u8 val;

	val = snd_ac97_read(cs5535au->ac97, AC97_AD_TEST2);
	val >>= AC97_AD_HPFD_SHIFT;
	ucontrol->value.integer.value[0] = val & 0x1;

	return 0;
}

static int snd_cs5535audio_ctl_put(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	int err;
	struct cs5535audio *cs5535au = snd_kcontrol_chip(kcontrol);
	u8 value;
	struct snd_ac97 *ac97 = cs5535au->ac97;

	/* value is 1 if analog input is desired */
	value = ucontrol->value.integer.value[0];

	/* turns off High Pass Filter if 1 */
	if (value)
		err = snd_ac97_update_bits(ac97, AC97_AD_TEST2,
				AD1888_HPF_EN_BIT, AD1888_HPF_EN_BIT);
	else
		err = snd_ac97_update_bits(ac97, AC97_AD_TEST2,
				AD1888_HPF_EN_BIT, 0);
	if (err < 0)
		snd_printk(KERN_ERR "Error updating AD_TEST2 %d\n", err);

	/* B2 and newer writes directly to a GPIO pin */
	if (value)
		geode_gpio_set(OLPC_GPIO_MIC_AC, GPIO_OUTPUT_VAL);
	else
		geode_gpio_clear(OLPC_GPIO_MIC_AC, GPIO_OUTPUT_VAL);

	return 1;
}

static struct snd_kcontrol_new snd_cs5535audio_controls __devinitdata =
{
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.name = "Analog Input Switch",
	.info = snd_cs5535audio_ctl_info,
	.get = snd_cs5535audio_ctl_get,
	.put = snd_cs5535audio_ctl_put,
	.private_value = 0
};

void __devinit olpc_prequirks(struct snd_card *card,
		struct snd_ac97_template *ac97)
{
	if (!machine_is_olpc())
		return;

	/* invert EAPD if on an OLPC B3 or higher */
	if (olpc_board_at_least(olpc_board_pre(0xb3)))
		ac97->scaps |= AC97_SCAP_INV_EAPD;
}

int __devinit olpc_quirks(struct snd_card *card, struct snd_ac97 *ac97)
{
	if (!machine_is_olpc())
		return 0;

	/* setup callback for mixer control that does analog input mode */
	return snd_ctl_add(card, snd_ctl_new1(&snd_cs5535audio_controls,
						ac97->private_data));
}

