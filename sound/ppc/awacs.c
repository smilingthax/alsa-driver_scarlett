/*
 * PMac AWACS lowlevel functions
 *
 * Copyright (c) by Takashi Iwai <tiwai@suse.de>
 * code based on dmasound.c.
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


#define __NO_VERSION__
#include <sound/driver.h>
#include <asm/io.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <sound/core.h>
#include "pmac.h"

#define chip_t pmac_t


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0) || defined(CONFIG_ADB_CUDA)
#define PMAC_AMP_AVAIL
#endif

#ifdef PMAC_AMP_AVAIL
typedef struct awacs_amp {
	unsigned char amp_master;
	unsigned char amp_vol[2][2];
	unsigned char amp_tone[2];
} awacs_amp_t;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
#define CHECK_CUDA_AMP() (adb_hardware == ADB_VIACUDA)
#else
#define CHECK_CUDA_AMP() (sys_ctrler == SYS_CTRLER_CUDA)
#endif

#endif /* PMAC_AMP_AVAIL */


/*
 * write AWACS register
 */
static void
snd_pmac_awacs_write(pmac_t *chip, int val)
{
	long timeout = 5000000;

	if (chip->model <= PMAC_SCREAMER)
		return;

	while (in_le32(&chip->awacs->codec_ctrl) & MASK_NEWECMD) {
		if (! --timeout) {
			snd_printd("snd_pmac_awacs_write timeout\n");
			break;
		}
	}
	out_le32(&chip->awacs->codec_ctrl, val | (chip->subframe << 22));
}

static void
snd_pmac_awacs_write_reg(pmac_t *chip, int reg, int val)
{
	snd_pmac_awacs_write(chip, val | (reg << 12));
	chip->awacs_reg[reg] = val;
}

static void
snd_pmac_awacs_write_noreg(pmac_t *chip, int reg, int val)
{
	snd_pmac_awacs_write(chip, val | (reg << 12));
}

#ifdef CONFIG_PMAC_PBOOK
static void
screamer_recalibrate(pmac_t *chip)
{
	/* Sorry for the horrible delays... I hope to get that improved
	 * by making the whole PM process asynchronous in a future version
	 */
	mdelay(750);
	snd_pmac_awacs_write_noreg(chip, 1,
				   chip->awacs_reg[1] | MASK_RECALIBRATE | MASK_CMUTE | MASK_AMUTE);
	mdelay(1000);
	snd_pmac_awacs_write_noreg(chip, 1, chip->awacs_reg[1]);
}
#endif


/*
 * additional callback to set the pcm format
 */
static void snd_pmac_awacs_set_format(pmac_t *chip)
{
	chip->awacs_reg[1] &= ~MASK_SAMPLERATE;
	chip->awacs_reg[1] |= chip->rate_index << 3;
	snd_pmac_awacs_write_reg(chip, 1, chip->awacs_reg[1]);
}


/*
 * AWACS volume callbacks
 */
/*
 * volumes: 0-15 stereo
 */
static int snd_pmac_awacs_info_volume(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 15;
	return 0;
}
 
static int snd_pmac_awacs_get_volume(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pmac_t *chip = snd_kcontrol_chip(kcontrol);
	int reg = kcontrol->private_value & 0xff;
	int lshift = (kcontrol->private_value >> 8) & 0xff;
	unsigned long flags;

	spin_lock_irqsave(&chip->reg_lock, flags);
	ucontrol->value.integer.value[0] = 0x0f - ((chip->awacs_reg[reg] >> lshift) & 0xf);
	ucontrol->value.integer.value[1] = 0x0f - (chip->awacs_reg[reg] & 0xf);
	spin_unlock_irqrestore(&chip->reg_lock, flags);
	return 0;
}

static int snd_pmac_awacs_put_volume(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pmac_t *chip = snd_kcontrol_chip(kcontrol);
	int reg = kcontrol->private_value & 0xff;
	int lshift = (kcontrol->private_value >> 8) & 0xff;
	int val, oldval;
	unsigned long flags;

	spin_lock_irqsave(&chip->reg_lock, flags);
	oldval = chip->awacs_reg[reg];
	val = oldval & ~(0xf | (0xf << lshift));
	val |= ((0x0f - (ucontrol->value.integer.value[0] & 0xf)) << lshift);
	val |= 0x0f - (ucontrol->value.integer.value[1] & 0xf);
	if (oldval != val)
		snd_pmac_awacs_write_reg(chip, reg, val);
	spin_unlock_irqrestore(&chip->reg_lock, flags);
	return oldval != reg;
}


#define AWACS_VOLUME(xname, xreg, xshift) \
{ iface: SNDRV_CTL_ELEM_IFACE_MIXER, name: xname, index: 0, \
  info: snd_pmac_awacs_info_volume, \
  get: snd_pmac_awacs_get_volume, \
  put: snd_pmac_awacs_put_volume, \
  private_value: (xreg) | ((xshift) << 8) }

/*
 * mute master/ogain for AWACS: mono
 */
static int snd_pmac_awacs_get_switch(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pmac_t *chip = snd_kcontrol_chip(kcontrol);
	int reg = kcontrol->private_value & 0xff;
	int shift = (kcontrol->private_value >> 8) & 0xff;
	int invert = (kcontrol->private_value >> 16) & 1;
	int val;
	unsigned long flags;

	spin_lock_irqsave(&chip->reg_lock, flags);
	val = (chip->awacs_reg[reg] >> shift) & 1;
	spin_unlock_irqrestore(&chip->reg_lock, flags);
	if (invert)
		val = 1 - val;
	ucontrol->value.integer.value[0] = val;
	return 0;
}

static int snd_pmac_awacs_put_switch(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pmac_t *chip = snd_kcontrol_chip(kcontrol);
	int reg = kcontrol->private_value & 0xff;
	int shift = (kcontrol->private_value >> 8) & 0xff;
	int invert = (kcontrol->private_value >> 16) & 1;
	int mask = 1 << shift;
	int val, changed;
	unsigned long flags;

	spin_lock_irqsave(&chip->reg_lock, flags);
	val = chip->awacs_reg[reg] & ~mask;
	if (ucontrol->value.integer.value[0] != invert)
		val |= mask;
	changed = chip->awacs_reg[reg] != val;
	if (changed)
		snd_pmac_awacs_write_reg(chip, reg, val);
	spin_unlock_irqrestore(&chip->reg_lock, flags);
	return changed;
}

#define AWACS_SWITCH(xname, xreg, xshift, xinvert) \
{ iface: SNDRV_CTL_ELEM_IFACE_MIXER, name: xname, index: 0, \
  info: snd_pmac_boolean_mono_info, \
  get: snd_pmac_awacs_get_switch, \
  put: snd_pmac_awacs_put_switch, \
  private_value: (xreg) | ((xshift) << 8) | ((xinvert) << 16) }


#ifdef PMAC_AMP_AVAIL
/*
 * G3 desktop uses TDA7433 connected via i2c address 0x45 (= 0x8a),
 * accessed through cuda
 */
static void awacs_set_cuda(int reg, int val)
{
	struct adb_request req;
	cuda_request(&req, NULL, 5, CUDA_PACKET, CUDA_GET_SET_IIC, 0x8a, reg, val);
	while (! req.complete)
		cuda_poll();
}

/*
 * level = 0 - 14, 7 = 0 dB
 */
static void awacs_amp_set_tone(pmac_t *chip, int bass, int treble)
{
	awacs_amp_t *amp = chip->mixer_data;
	if (! amp)
		return;

	amp->amp_tone[0] = bass;
	amp->amp_tone[1] = treble;
	if (bass > 7)
		bass = (14 - bass) + 8;
	if (treble > 7)
		treble = (14 - treble) + 8;
	awacs_set_cuda(2, (bass << 4) | treble);
}

/*
 * vol = 0 - 31, 32 = mute bit, stereo
 */
static void awacs_amp_set_vol(pmac_t *chip, int index, int lvol, int rvol)
{
	awacs_amp_t *amp = chip->mixer_data;
	if (! amp)
		return;

	amp->amp_vol[index][0] = lvol;
	amp->amp_vol[index][1] = rvol;

	/* volume is inversed (attenuation) */
	lvol = (lvol & 32) | (31 - (lvol & 31));
	rvol = (rvol & 32) | (31 - (rvol & 31));

	/* turn on speaker */
	awacs_set_cuda(3 + index, lvol);
	awacs_set_cuda(5 + index, rvol);
}

/*
 * 0 = -79 dB, 79 = 0 dB, 99 = +20 dB
 */
static void awacs_amp_set_master(pmac_t *chip, int vol)
{
	awacs_amp_t *amp = chip->mixer_data;
	if (! amp)
		return;

	amp->amp_master = vol;
	if (vol <= 79)
		vol = 32 + (79 - vol);
	else
		vol = 32 - (vol - 79);
	awacs_set_cuda(1, vol);
}

static void awacs_amp_free(pmac_t *chip)
{
	awacs_amp_t *amp = chip->mixer_data;
	if (! amp)
		return;
	kfree(amp);
	chip->mixer_data = NULL;
}


/*
 * mixer controls
 */
static int snd_pmac_awacs_info_volume_amp(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 31;
	return 0;
}
 
static int snd_pmac_awacs_get_volume_amp(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pmac_t *chip = snd_kcontrol_chip(kcontrol);
	int index = kcontrol->private_value;
	awacs_amp_t *amp = chip->mixer_data;
	if (! amp) return -EINVAL;
	ucontrol->value.integer.value[0] = amp->amp_vol[index][0] & 31;
	ucontrol->value.integer.value[1] = amp->amp_vol[index][1] & 31;
	return 0;
}

static int snd_pmac_awacs_put_volume_amp(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pmac_t *chip = snd_kcontrol_chip(kcontrol);
	int index = kcontrol->private_value;
	int vol[2];
	int changed;
	awacs_amp_t *amp = chip->mixer_data;
	if (! amp) return -EINVAL;

	vol[0] = (ucontrol->value.integer.value[0] & 31) | (amp->amp_vol[index][0] & 32);
	vol[1] = (ucontrol->value.integer.value[1] & 31) | (amp->amp_vol[index][1] & 32);
	changed = vol[0] != amp->amp_vol[index][0] || vol[1] != amp->amp_vol[index][1];
	if (changed)
		awacs_amp_set_vol(chip, index, vol[0], vol[1]);
	return changed;
}

static int snd_pmac_awacs_get_switch_amp(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pmac_t *chip = snd_kcontrol_chip(kcontrol);
	int index = kcontrol->private_value;
	awacs_amp_t *amp = chip->mixer_data;
	if (! amp) return -EINVAL;
	ucontrol->value.integer.value[0] = (amp->amp_vol[index][0] & 32) ? 0 : 1;
	ucontrol->value.integer.value[1] = (amp->amp_vol[index][1] & 32) ? 0 : 1;
	return 0;
}

static int snd_pmac_awacs_put_switch_amp(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pmac_t *chip = snd_kcontrol_chip(kcontrol);
	int index = kcontrol->private_value;
	int vol[2];
	int changed;
	awacs_amp_t *amp = chip->mixer_data;
	if (! amp) return -EINVAL;

	vol[0] = (ucontrol->value.integer.value[0] ? 0 : 32) | (amp->amp_vol[index][0] & 31);
	vol[1] = (ucontrol->value.integer.value[1] ? 0 : 32) | (amp->amp_vol[index][1] & 31);
	changed = vol[0] != amp->amp_vol[index][0] || vol[1] != amp->amp_vol[index][1];
	if (changed)
		awacs_amp_set_vol(chip, index, vol[0], vol[1]);
	return changed;
}

static int snd_pmac_awacs_info_tone_amp(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 14;
	return 0;
}
 
static int snd_pmac_awacs_get_tone_amp(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pmac_t *chip = snd_kcontrol_chip(kcontrol);
	int index = kcontrol->private_value;
	awacs_amp_t *amp = chip->mixer_data;
	if (! amp) return -EINVAL;
	ucontrol->value.integer.value[0] = amp->amp_tone[index];
	return 0;
}

static int snd_pmac_awacs_put_tone_amp(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pmac_t *chip = snd_kcontrol_chip(kcontrol);
	int index = kcontrol->private_value;
	awacs_amp_t *amp = chip->mixer_data;
	if (! amp) return -EINVAL;
	if (ucontrol->value.integer.value[0] != amp->amp_tone[index]) {
		amp->amp_tone[index] = ucontrol->value.integer.value[0];
		awacs_amp_set_tone(chip, amp->amp_tone[0], amp->amp_tone[1]);
		return 1;
	}
	return 0;
}

static int snd_pmac_awacs_info_master_amp(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 99;
	return 0;
}
 
static int snd_pmac_awacs_get_master_amp(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pmac_t *chip = snd_kcontrol_chip(kcontrol);
	awacs_amp_t *amp = chip->mixer_data;
	if (! amp) return -EINVAL;
	ucontrol->value.integer.value[0] = amp->amp_master;
	return 0;
}

static int snd_pmac_awacs_put_master_amp(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pmac_t *chip = snd_kcontrol_chip(kcontrol);
	awacs_amp_t *amp = chip->mixer_data;
	if (! amp) return -EINVAL;
	if (ucontrol->value.integer.value[0] != amp->amp_master) {
		amp->amp_master = ucontrol->value.integer.value[0];
		awacs_amp_set_master(chip, amp->amp_master);
		return 1;
	}
	return 0;
}

static snd_kcontrol_new_t snd_pmac_awacs_amp_vol[] __initdata = {
	{ iface: SNDRV_CTL_ELEM_IFACE_MIXER,
	  name: "PC Speaker Playback Volume",
	  info: snd_pmac_awacs_info_volume_amp,
	  get: snd_pmac_awacs_get_volume_amp,
	  put: snd_pmac_awacs_put_volume_amp,
	  private_value: 1,
	},
	{ iface: SNDRV_CTL_ELEM_IFACE_MIXER,
	  name: "Amp Headphone Playback Volume",
	  info: snd_pmac_awacs_info_volume_amp,
	  get: snd_pmac_awacs_get_volume_amp,
	  put: snd_pmac_awacs_put_volume_amp,
	  private_value: 0,
	},
	{ iface: SNDRV_CTL_ELEM_IFACE_MIXER,
	  name: "Amp Headphone Playback Switch",
	  info: snd_pmac_boolean_mono_info,
	  get: snd_pmac_awacs_get_switch_amp,
	  put: snd_pmac_awacs_put_switch_amp,
	  private_value: 0,
	},
	{ iface: SNDRV_CTL_ELEM_IFACE_MIXER,
	  name: "Tone Control - Bass",
	  info: snd_pmac_awacs_info_tone_amp,
	  get: snd_pmac_awacs_get_tone_amp,
	  put: snd_pmac_awacs_put_tone_amp,
	  private_value: 0,
	},
	{ iface: SNDRV_CTL_ELEM_IFACE_MIXER,
	  name: "Tone Control - Treble",
	  info: snd_pmac_awacs_info_tone_amp,
	  get: snd_pmac_awacs_get_tone_amp,
	  put: snd_pmac_awacs_put_tone_amp,
	  private_value: 1,
	},
	{ iface: SNDRV_CTL_ELEM_IFACE_MIXER,
	  name: "Amp Master Playback Volume",
	  info: snd_pmac_awacs_info_master_amp,
	  get: snd_pmac_awacs_get_master_amp,
	  put: snd_pmac_awacs_put_master_amp,
	},
};

static snd_kcontrol_new_t snd_pmac_awacs_amp_sw __initdata = {
	iface: SNDRV_CTL_ELEM_IFACE_MIXER,
	name: "PC Speaker Playback Switch",
	info: snd_pmac_boolean_mono_info,
	get: snd_pmac_awacs_get_switch_amp,
	put: snd_pmac_awacs_put_switch_amp,
	private_value: 1,
};

#endif /* PMAC_AMP_AVAIL */


/*
 * mic boost for screamer
 */
static int snd_pmac_screamer_mic_boost_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 2;
	return 0;
}

static int snd_pmac_screamer_mic_boost_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pmac_t *chip = snd_kcontrol_chip(kcontrol);
	int val;
	unsigned long flags;

	spin_lock_irqsave(&chip->reg_lock, flags);
	if (chip->awacs_reg[6] & MASK_MIC_BOOST)
		val = 2;
	else if (chip->awacs_reg[0] & MASK_GAINLINE)
		val = 1;
	else
		val = 0;
	spin_unlock_irqrestore(&chip->reg_lock, flags);
	ucontrol->value.integer.value[0] = val;
	return 0;
}

static int snd_pmac_screamer_mic_boost_put(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pmac_t *chip = snd_kcontrol_chip(kcontrol);
	int changed = 0;
	int val0, val6;
	unsigned long flags;

	spin_lock_irqsave(&chip->reg_lock, flags);
	val0 = chip->awacs_reg[0] & ~MASK_GAINLINE;
	val6 = chip->awacs_reg[6] & ~MASK_MIC_BOOST;
	if (ucontrol->value.integer.value[0] > 0) {
		val0 |= MASK_GAINLINE;
		if (ucontrol->value.integer.value[0] > 1)
			val6 |= MASK_MIC_BOOST;
	}
	if (val0 != chip->awacs_reg[0]) {
		snd_pmac_awacs_write_reg(chip, 0, val0);
		changed = 1;
	}
	if (val6 != chip->awacs_reg[6]) {
		snd_pmac_awacs_write_reg(chip, 6, val6);
		changed = 1;
	}
	spin_unlock_irqrestore(&chip->reg_lock, flags);
	return changed;
}

/*
 * lists of mixer elements
 */
static snd_kcontrol_new_t snd_pmac_awacs_mixers[] __initdata = {
	AWACS_VOLUME("Master Playback Volume", 2, 6),
	AWACS_SWITCH("Master Capture Switch", 1, SHIFT_LOOPTHRU, 0),
	AWACS_VOLUME("Capture Volume", 0, 4),
	AWACS_SWITCH("Line Capture Switch", 0, SHIFT_MUX_LINE, 0),
	AWACS_SWITCH("CD Capture Switch", 0, SHIFT_MUX_CD, 0),
	AWACS_SWITCH("Mic Capture Switch", 0, SHIFT_MUX_MIC, 0),
};

static snd_kcontrol_new_t snd_pmac_awacs_master_sw __initdata =
AWACS_SWITCH("Master Playback Switch", 1, SHIFT_HDMUTE, 1);

static snd_kcontrol_new_t snd_pmac_awacs_mic_boost[] __initdata = {
	AWACS_SWITCH("Mic Boost", 0, SHIFT_GAINLINE, 0),
};

static snd_kcontrol_new_t snd_pmac_screamer_mic_boost[] __initdata = {
	{ iface: SNDRV_CTL_ELEM_IFACE_MIXER,
	  name: "Mic Boost",
	  info: snd_pmac_screamer_mic_boost_info,
	  get: snd_pmac_screamer_mic_boost_get,
	  put: snd_pmac_screamer_mic_boost_put,
	},
};

static snd_kcontrol_new_t snd_pmac_awacs_speaker_vol[] __initdata = {
	AWACS_VOLUME("PC Speaker Playback Volume", 4, 6),
};
static snd_kcontrol_new_t snd_pmac_awacs_speaker_sw __initdata =
AWACS_SWITCH("PC Speaker Playback Switch", 1, SHIFT_SPKMUTE, 1);


#define num_controls(ary) (sizeof(ary) / sizeof(snd_kcontrol_new_t))

/*
 * add new mixer elements to the card
 */
static int build_mixers(pmac_t *chip, int nums, snd_kcontrol_new_t *mixers)
{
	int i, err;

	for (i = 0; i < nums; i++) {
		if ((err = snd_ctl_add(chip->card, snd_ctl_new1(&mixers[i], chip))) < 0)
			return err;
	}
	return 0;
}

#ifdef CONFIG_PMAC_PBOOK
static void snd_pmac_awacs_resume(pmac_t *chip)
{
	snd_pmac_awacs_write_reg(chip, 0, chip->awacs_reg[0]);
	snd_pmac_awacs_write_reg(chip, 1, chip->awacs_reg[1]);
	snd_pmac_awacs_write_reg(chip, 2, chip->awacs_reg[2]);
	snd_pmac_awacs_write_reg(chip, 4, chip->awacs_reg[4]);
	if (chip->model == PMAC_SCREAMER) {
		snd_pmac_awacs_write_reg(chip, 5, chip->awacs_reg[5]);
		snd_pmac_awacs_write_reg(chip, 6, chip->awacs_reg[6]);
		snd_pmac_awacs_write_reg(chip, 7, chip->awacs_reg[7]);
		screamer_recalibrate(chip);
	}
#ifdef PMAC_AMP_AVAIL
	if (chip->mixer_data) {
		awacs_amp_t *amp = chip->mixer_data;
		awacs_amp_set_vol(chip, 0, amp->amp_vol[0][0], amp->amp_vol[0][1]);
		awacs_amp_set_vol(chip, 1, amp->amp_vol[1][0], amp->amp_vol[1][1]);
		awacs_amp_set_tone(chip, amp->amp_tone[0], amp->amp_tone[1]);
		awacs_amp_set_master(chip, amp->amp_master);
	}
#endif
}
#endif /* CONFIG_PMAC_PBOOK */

#ifdef PMAC_SUPPORT_AUTOMUTE
/*
 * auto-mute stuffs
 */
static int snd_pmac_awacs_detect_headphone(pmac_t *chip)
{
	return (in_le32(&chip->awacs->codec_stat) & chip->hp_stat_mask) ? 1 : 0;
}

static void snd_pmac_awacs_update_automute(pmac_t *chip, int do_notify)
{
	if (chip->auto_mute) {
		int reg = chip->awacs_reg[1] | (MASK_AMUTE|MASK_CMUTE);
		if (snd_pmac_awacs_detect_headphone(chip))
			reg &= ~MASK_AMUTE;
		else
			reg &= ~MASK_CMUTE;
		if (do_notify && reg == chip->awacs_reg[1])
			return;
		snd_pmac_awacs_write_reg(chip, 1, reg);
#if 0
//#ifdef PMAC_AMP_AVAIL
		if (chip->mixer_data) {
			awacs_amp_t *amp = chip->mixer_data;
			if (reg & MASK_CMUTE) {
				chip->amp_vol[0] |= 32;
				chip->amp_vol[1] |= 32;
			} else {
				chip->amp_vol[0] &= 31;
				chip->amp_vol[1] &= 31;
			}
			snd_pmac_awacs_enable_amp(chip, chip->amp_vol[0], chip->amp_vol[1]);
		}
#endif
		if (do_notify) {
			snd_ctl_notify(chip->card, SNDRV_CTL_EVENT_MASK_VALUE,
				       &chip->master_sw_ctl->id);
			snd_ctl_notify(chip->card, SNDRV_CTL_EVENT_MASK_VALUE,
				       &chip->speaker_sw_ctl->id);
		}
	}
}
#endif /* PMAC_SUPPORT_AUTOMUTE */


/*
 * initialize chip
 */
int __init
snd_pmac_awacs_init(pmac_t *chip)
{
	int err, vol;

	snd_pmac_awacs_write_reg(chip, 0, MASK_MUX_CD | 0xff);
	/* FIXME: Only machines with external SRS module need MASK_PAROUT */
	chip->awacs_reg[1] = MASK_CMUTE | MASK_AMUTE;
	if (chip->has_iic || chip->device_id == 0x5 ||
	    /*chip->_device_id == 0x8 || */
	    chip->device_id == 0xb)
		chip->awacs_reg[1] |= MASK_PAROUT;
	snd_pmac_awacs_write_reg(chip, 1, chip->awacs_reg[1]);
	/* get default volume from nvram */
	// vol = (~nvram_read_byte(0x1308) & 7) << 1;
	vol = 0x0f;

	snd_pmac_awacs_write_reg(chip, 2, vol + (vol << 6));
	snd_pmac_awacs_write_reg(chip, 4, vol + (vol << 6));
	if (chip->model == PMAC_SCREAMER) {
		snd_pmac_awacs_write_reg(chip, 5, 0);
		snd_pmac_awacs_write_reg(chip, 6, 0);
		snd_pmac_awacs_write_reg(chip, 7, 0);
	}

#ifdef CONFIG_PMAC_PBOOK
	/* Recalibrate chip */
	if (chip->model == PMAC_SCREAMER)
		screamer_recalibrate(chip);
#endif

	if (chip->model <= PMAC_SCREAMER && chip->revision == 0) {
		chip->revision =
			(in_le32(&chip->awacs->codec_stat) >> 12) & 0xf;
		if (chip->revision == 3) {
#ifdef PMAC_AMP_AVAIL
			if (CHECK_CUDA_AMP()) {
				chip->mixer_data = kmalloc(sizeof(*amp), GFP_KERNEL);
				if (! chip->mixer_data)
					return -ENOMEM;
				chip->mixer_free = awacs_amp_free;
				awacs_amp_set_vol(chip, 0, 32, 32);
				awacs_amp_set_vol(chip, 1, 31, 31);
				awacs_amp_set_tone(chip, 7, 7);
				awacs_amp_set_master(chip, 79);
			}
#endif /* PMAC_AMP_AVAIL */
		}
	}

	if (chip->hp_stat_mask == 0) {
		/* set headphone-jack detection bit */
		switch (chip->model) {
		case PMAC_AWACS:
			chip->hp_stat_mask = 0x04;
			break;
		case PMAC_SCREAMER:
			switch (chip->device_id) {
			case 0x08:
				/* 1 = side jack, 2 = front jack */
				chip->hp_stat_mask = 0x03;
				break;
			case 0x00:
			case 0x05:
				chip->hp_stat_mask = 0x04;
				break;
			default:
				chip->hp_stat_mask = 0x08;
				break;
			}
			break;
		default:
			break;
		}
	}

	/*
	 * build mixers
	 */
	strcpy(chip->card->mixername, "PowerMac AWACS");

	if ((err = build_mixers(chip, num_controls(snd_pmac_awacs_mixers),
				snd_pmac_awacs_mixers)) < 0)
		return err;
	chip->master_sw_ctl = snd_ctl_new1(&snd_pmac_awacs_master_sw, chip);
	if ((err = snd_ctl_add(chip->card, chip->master_sw_ctl)) < 0)
		return err;
#ifdef PMAC_AMP_AVAIL
	if (chip->mixer_data) {
		if ((err = build_mixers(chip, num_controls(snd_pmac_awacs_amp_vol),
					snd_pmac_awacs_amp_vol)) < 0)
			return err;
		chip->speaker_sw_ctl = snd_ctl_new1(&snd_pmac_awacs_amp_sw, chip);
		if ((err = snd_ctl_add(chip->card, chip->speaker_sw_ctl)) < 0)
			return err;
	} else {
#endif /* PMAC_AMP_AVAIL */
		if ((err = build_mixers(chip, num_controls(snd_pmac_awacs_speaker_vol),
					snd_pmac_awacs_speaker_vol)) < 0)
			return err;
		chip->speaker_sw_ctl = snd_ctl_new1(&snd_pmac_awacs_speaker_sw, chip);
		if ((err = snd_ctl_add(chip->card, chip->speaker_sw_ctl)) < 0)
			return err;
#ifdef PMAC_AMP_AVAIL
	}
#endif /* PMAC_AMP_AVAIL */

#ifdef PMAC_SUPPORT_AUTOMUTE
	if ((err = snd_pmac_add_automute(chip)) < 0)
		return err;
#endif
	if (chip->model == PMAC_SCREAMER) {
		if ((err = build_mixers(chip, num_controls(snd_pmac_screamer_mic_boost),
					snd_pmac_screamer_mic_boost)) < 0)
			return err;
	} else {
		if ((err = build_mixers(chip, num_controls(snd_pmac_awacs_mic_boost),
					snd_pmac_awacs_mic_boost)) < 0)
			return err;
	}

	/*
	 * set lowlevel callbacks
	 */
	chip->set_format = snd_pmac_awacs_set_format;
#ifdef CONFIG_PMAC_PBOOK
	chip->resume = snd_pmac_awacs_resume;
#endif
#ifdef PMAC_SUPPORT_AUTOMUTE
	chip->detect_headphone = snd_pmac_awacs_detect_headphone;
	chip->update_automute = snd_pmac_awacs_update_automute;
	snd_pmac_awacs_update_automute(chip, 0); /* update the status only */
#endif

	return 0;
}
