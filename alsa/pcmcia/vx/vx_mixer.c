/*
 * Driver for Digigram VXpocket soundcards
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

#include <sound/driver.h>
#include <sound/core.h>
#include <sound/control.h>
#include "vxpocket.h"
#include "vx_cmd.h"

#define chip_t vxpocket_t


/*
 * write a codec data (24bit)
 */
static void vx_write_codec_reg(vxpocket_t *chip, int codec, unsigned int data)
{
	int i;
	unsigned long flags;

	if (chip->is_stale)
		return;

	spin_lock_irqsave(&chip->lock, flags);
	/* Activate access to the corresponding codec register */
	if (! codec)
		vx_inb(chip, LOFREQ);
	else
		vx_inb(chip, CODEC2);
		
	/* We have to send 24 bits (3 x 8 bits). Start with most signif. Bit */
	for (i = 0; i < 24; i++, data <<= 1)
		vx_outb(chip, DATA, ((data & 0x800000) ? VXP_DATA_CODEC_MASK : 0));
	
	/* Terminate access to codec registers */
	vx_inb(chip, HIFREQ);
	spin_unlock_irqrestore(&chip->lock, flags);
}


/*
 * Data type used to access the Codec
 */
typedef union {
	u32 l;
#ifdef SNDRV_BIG_ENDIAN
	struct w {
		u16 h;
		u16 l;
	} w;
	struct b {
		u8 hh;
		u8 mh;
		u8 ml;
		u8 ll;
	} b;
#else /* LITTLE_ENDIAN */
	struct w {
		u16 l;
		u16 h;
	} w;
	struct b {
		u8 ll;
		u8 ml;
		u8 mh;
		u8 hh;
	} b;
#endif
} vx_codec_data_t;

#define SET_CDC_DATA_SEL(di,s)          ((di).b.mh = (u8) (s))
#define SET_CDC_DATA_REG(di,r)          ((di).b.ml = (u8) (r))
#define SET_CDC_DATA_VAL(di,d)          ((di).b.ll = (u8) (d))
#define SET_CDC_DATA_INIT(di)           ((di).l = 0L, SET_CDC_DATA_SEL(di,XX_CODEC_SELECTOR))

/*
 * set up codec register and write the value
 * @codec: the codec id, 0 or 1
 * @reg: register index
 * @val: data value
 */
static void vx_set_codec_reg(vxpocket_t *chip, int codec, int reg, int val)
{
	vx_codec_data_t data;
	/* DAC control register */
	SET_CDC_DATA_INIT(data);
	SET_CDC_DATA_REG(data, reg);
	SET_CDC_DATA_VAL(data, val);
	vx_write_codec_reg(chip, codec, data.l);
}


#define ANALOG_OUT_ATTEN_MIN	0x00
#define ANALOG_OUT_ATTEN_MAX	0xe3

/*
 * vx_set_analog_output_level - set the output attenuation level
 * @codec: the output codec, 0 or 1.  (on vxpocket, 0 only.)
 * @left: left attenuation level, 0 = 0 dB, 0xe3 = -113.5 dB
 * @right: right attenuation level
 */
static void vx_set_analog_output_level(vxpocket_t *chip, int codec, int left, int right)
{
	vx_set_codec_reg(chip, codec, XX_CODEC_LEVEL_LEFT_REGISTER, left);
	vx_set_codec_reg(chip, codec, XX_CODEC_LEVEL_RIGHT_REGISTER, right);
}


/*
 * vx_set_mic_boost - set mic boost level (on vxp440 only)
 * @boost: 0 = 20dB, 1 = +38dB
 */
static void vx_set_mic_boost(vxpocket_t *chip, int boost)
{
	unsigned long flags;

	if (chip->is_stale)
		return;

	spin_lock_irqsave(&chip->lock, flags);
	if (chip->regCDSP & P24_CDSP_MICS_SEL_MASK) {
		if (boost) {
			/* boost: 38 dB */
			chip->regCDSP &= ~P24_CDSP_MIC20_SEL_MASK;
			chip->regCDSP |=  P24_CDSP_MIC38_SEL_MASK;
		} else {
			/* minimum value: 20 dB */
			chip->regCDSP |=  P24_CDSP_MIC20_SEL_MASK;
			chip->regCDSP &= ~P24_CDSP_MIC38_SEL_MASK;
                }
		vx_outb(chip, CDSP, chip->regCDSP);
	}
	spin_unlock_irqrestore(&chip->lock, flags);
}

#define MIC_LEVEL_MIN	0
#define MIC_LEVEL_MAX	8
/*
 * remap the linear value (0-8) to the actual value (0-15)
 */
static int vx_compute_mic_level(int level)
{
	switch (level) {
	case 5: level = 6 ; break;
	case 6: level = 8 ; break;
	case 7: level = 11; break;
	case 8: level = 15; break;
	default: break ;
	}
	return level;
}

/*
 * vx_set_mic_level - set mic level (on vxpocket only)
 * @level: the mic level = 0 - 8 (max)
 */
static void vx_set_mic_level(vxpocket_t *chip, int level)
{
	unsigned long flags;

	if (chip->is_stale)
		return;

	spin_lock_irqsave(&chip->lock, flags);
	if (chip->regCDSP & VXP_CDSP_MIC_SEL_MASK) {
		level = vx_compute_mic_level(level);
		vx_outb(chip, MICRO, level);
	}
	spin_unlock_irqrestore(&chip->lock, flags);
}

/*
 * vx_toggle_dac_mute -  mute/unmute DAC
 * @mute: 0 = unmute, 1 = mute
 */

#define DAC_ATTEN_MIN	0x08
#define DAC_ATTEN_MAX	0x38

void vx_toggle_dac_mute(vxpocket_t *chip, int mute)
{
	int i;
	for (i = 0; i < chip->hw->num_codecs; i++)
		vx_set_codec_reg(chip, i, XX_CODEC_DAC_CONTROL_REGISTER,
				 mute ? DAC_ATTEN_MAX : DAC_ATTEN_MIN);
}

/*
 * vx_reset_codec - reset and initialize the codecs
 */
void vx_reset_codec(vxpocket_t *chip)
{
	int i;

	/* Set the reset CODEC bit to 1. */
	vx_outb(chip, CDSP, chip->regCDSP | VXP_CDSP_CODEC_RESET_MASK);
	vx_delay(chip, 10);
	/* Set the reset CODEC bit to 0. */
	vx_outb(chip, CDSP, chip->regCDSP & ~VXP_CDSP_CODEC_RESET_MASK);
	vx_delay(chip, 1);

	for (i = 0; i < chip->hw->num_codecs; i++) {
		/* DAC control register (change level when zero crossing + mute) */
		vx_set_codec_reg(chip, i, XX_CODEC_DAC_CONTROL_REGISTER, DAC_ATTEN_MAX);
		/* ADC control register */
		vx_set_codec_reg(chip, i, XX_CODEC_ADC_CONTROL_REGISTER, 0x00);
		/* Port mode register */
		vx_set_codec_reg(chip, i, XX_CODEC_PORT_MODE_REGISTER, 0x75); /* for VX22: 0x65 */
		/* Clock control register */
		vx_set_codec_reg(chip, i, XX_CODEC_CLOCK_CONTROL_REGISTER, 0x00);
	}

	/* mute analog output */
	for (i = 0; i < chip->hw->num_codecs; i++) {
		chip->output_level[i][0] = ANALOG_OUT_ATTEN_MAX;
		chip->output_level[i][1] = ANALOG_OUT_ATTEN_MAX;
		vx_set_analog_output_level(chip, i,
					   ANALOG_OUT_ATTEN_MAX,
					   ANALOG_OUT_ATTEN_MAX);
	}

	/* mute input levels */
	chip->mic_level = 0;
	if (chip->hw->type == VXP_TYPE_VXP440)
		vx_set_mic_boost(chip, 0);
	else
		vx_set_mic_level(chip, 0);
}

/*
 * change the audio input source
 * @src: the target source (VXP_AUDIO_SRC_XXX)
 */
static void vx_change_audio_source(vxpocket_t *chip, int src)
{
	unsigned long flags;

	if (chip->is_stale)
		return;

	spin_lock_irqsave(&chip->lock, flags);
	switch (src) {
	case VXP_AUDIO_SRC_DIGITAL:
		chip->regCDSP |= VXP_CDSP_DATAIN_SEL_MASK;
		vx_outb(chip, CDSP, chip->regCDSP);
		break;
	case VXP_AUDIO_SRC_LINE:
		chip->regCDSP &= ~VXP_CDSP_DATAIN_SEL_MASK;
		if (chip->hw->type == VXP_TYPE_VXP440)
			chip->regCDSP &= ~P24_CDSP_MICS_SEL_MASK;
		else
			chip->regCDSP &= ~VXP_CDSP_MIC_SEL_MASK;
		vx_outb(chip, CDSP, chip->regCDSP);
		break;
	case VXP_AUDIO_SRC_MIC:
		chip->regCDSP &= ~VXP_CDSP_DATAIN_SEL_MASK;
		/* reset mic levels */
		if (chip->hw->type == VXP_TYPE_VXP440) {
			chip->regCDSP &= ~P24_CDSP_MICS_SEL_MASK;
			if (chip->mic_level)
				chip->regCDSP |=  P24_CDSP_MIC38_SEL_MASK;
			else
				chip->regCDSP |= P24_CDSP_MIC20_SEL_MASK;
			vx_outb(chip, CDSP, chip->regCDSP);
		} else {
			chip->regCDSP |= VXP_CDSP_MIC_SEL_MASK;
			vx_outb(chip, CDSP, chip->regCDSP);
			vx_outb(chip, MICRO, vx_compute_mic_level(chip->mic_level));
			spin_lock_irqsave(&chip->lock, flags);
		}
		break;
	}
	spin_unlock_irqrestore(&chip->lock, flags);
}


/*
 * change the audio source if necessary and possible
 * returns 1 if the source is actually changed.
 */
int vx_sync_audio_source(vxpocket_t *chip)
{
	if (chip->audio_source_target == chip->audio_source ||
	    chip->pcm_running)
		return 0;
	vx_change_audio_source(chip, chip->audio_source_target);
	chip->audio_source = chip->audio_source_target;
	return 1;
}


/*
 * audio level, mute, monitoring
 */
struct vx_audio_level {
	unsigned int has_level: 1;
	unsigned int has_monitor_level: 1;
	unsigned int has_mute: 1;
	unsigned int has_monitor_mute: 1;
	unsigned int mute;
	unsigned int monitor_mute;
	short level;
	short monitor_level;
};

static int vx_adjust_audio_level(vxpocket_t *chip, int audio, int capture,
				 struct vx_audio_level *info)
{
	struct vx_rmh rmh;

	if (chip->is_stale)
		return -EBUSY;

        vx_init_rmh(&rmh, CMD_AUDIO_LEVEL_ADJUST);
	if (capture)
		rmh.Cmd[0] |= COMMAND_RECORD_MASK;
	/* Add Audio IO mask */
	rmh.Cmd[1] = 1 << audio;
	rmh.Cmd[2] = 0;
	if (info->has_level) {
		rmh.Cmd[0] |=  VALID_AUDIO_IO_DIGITAL_LEVEL;
		rmh.Cmd[2] |= info->level;
        }
	if (info->has_monitor_level) {
		rmh.Cmd[0] |=  VALID_AUDIO_IO_MONITORING_LEVEL;
		rmh.Cmd[2] |= ((unsigned int)info->monitor_level << 10);
        }
	if (info->has_mute) { 
		rmh.Cmd[0] |= VALID_AUDIO_IO_MUTE_LEVEL;
		if (info->mute)
			rmh.Cmd[2] |= AUDIO_IO_HAS_MUTE_LEVEL;
	}
	if (info->has_monitor_mute) {
		rmh.Cmd[0] |= VALID_AUDIO_IO_MUTE_MONITORING_1;
		if (info->monitor_mute)
			rmh.Cmd[2] |= AUDIO_IO_HAS_MUTE_MONITORING_1;
	}
	return vx_send_msg(chip, &rmh);
}

    
#if 0 // not used
static int vx_read_audio_level(vxpocket_t *chip, int audio, int capture,
			       struct vx_audio_level *info)
{
	int err;
	struct vx_rmh rmh;

	memset(info, 0, sizeof(*info));
        vx_init_rmh(&rmh, CMD_GET_AUDIO_LEVELS);
	if (capture)
		rmh.Cmd[0] |= COMMAND_RECORD_MASK;
	/* Add Audio IO mask */
	rmh.Cmd[1] = 1 << audio;
	err = vx_send_msg(chip, &rmh);
	if (err < 0)
		return err;
	info.level = rmh.Stat[0] & MASK_DSP_WORD_LEVEL;
	info.monitor_level = (rmh.Stat[0] >> 10) & MASK_DSP_WORD_LEVEL;
	info.mute = (rmh.Stat[i] & AUDIO_IO_HAS_MUTE_LEVEL) ? 1 : 0;
	info.monitor_mute = (rmh.Stat[i] & AUDIO_IO_HAS_MUTE_MONITORING_1) ? 1 : 0;
	return 0;
}
#endif

/*
 * set the monitoring level and mute state of the given audio
 */
static int vx_set_monitor_level(vxpocket_t *chip, int audio, int level, int active)
{
	struct vx_audio_level info;

	memset(&info, 0, sizeof(info));
	info.has_monitor_level = 1;
	info.monitor_level = level;
	info.has_monitor_mute = 1;
	info.monitor_mute = !active;
	chip->audio_monitor[audio] = level;
	chip->audio_monitor_active[audio] = active;
	return vx_adjust_audio_level(chip, audio, 0, &info); /* playback only */
}


/*
 * set the mute status of the given audio
 */
static int vx_set_audio_switch(vxpocket_t *chip, int audio, int active)
{
	struct vx_audio_level info;

	memset(&info, 0, sizeof(info));
	info.has_mute = 1;
	info.mute = !active;
	chip->audio_active[audio] = active;
	return vx_adjust_audio_level(chip, audio, 0, &info); /* playback only */
}

/*
 * set the mute status of the given audio
 */
static int vx_set_audio_gain(vxpocket_t *chip, int audio, int capture, int level)
{
	struct vx_audio_level info;

	memset(&info, 0, sizeof(info));
	info.has_level = 1;
	info.level = level;
	chip->audio_gain[capture][audio] = level;
	return vx_adjust_audio_level(chip, audio, capture, &info);
}

/*
 * reset all audio levels
 */
void vx_reset_audio_levels(vxpocket_t *chip)
{
	int i, c;
	struct vx_audio_level info;

	memset(chip->audio_gain, 0, sizeof(chip->audio_gain));
	memset(chip->audio_active, 0, sizeof(chip->audio_active));
	memset(chip->audio_monitor, 0, sizeof(chip->audio_monitor));
	memset(chip->audio_monitor_active, 0, sizeof(chip->audio_monitor_active));

	for (c = 0; c < 2; c++) {
		for (i = 0; i < chip->hw->num_ins * 2; i++) {
			memset(&info, 0, sizeof(info));
			if (c == 0) {
				info.has_monitor_level = 1;
				info.has_mute = 1;
				info.has_monitor_mute = 1;
			}
			info.has_level = 1;
			info.level = CVAL_0DB; /* default: 0dB */
			vx_adjust_audio_level(chip, i, c, &info);
			chip->audio_gain[c][i] = CVAL_0DB;
			chip->audio_monitor[i] = CVAL_0DB;
		}
	}
}


/*
 * VU, peak meter record
 */

#define VU_METER_CHANNELS	2

struct vx_vu_meter {
	int saturated;
	int vu_level;
	int peak_level;
};

/*
 * get the VU and peak meter values
 * @audio: the audio index
 * @capture: 0 = playback, 1 = capture operation
 * @info: the array of vx_vu_meter records (size = 2).
 */
static int vx_get_audio_vu_meter(vxpocket_t *chip, int audio, int capture, struct vx_vu_meter *info)
{
	struct vx_rmh rmh;
	int i, err;

	if (chip->is_stale)
		return -EBUSY;

	vx_init_rmh(&rmh, CMD_AUDIO_VU_PIC_METER);
	rmh.LgStat += 2 * VU_METER_CHANNELS;
	if (capture)
		rmh.Cmd[0] |= COMMAND_RECORD_MASK;
    
        /* Add Audio IO mask */
	rmh.Cmd[1] = 0;
	for (i = 0; i < VU_METER_CHANNELS; i++)
		rmh.Cmd[1] |= 1 << (audio + i);
	err = vx_send_msg(chip, &rmh);
	if (err < 0)
		return err;
	/* Read response */
	for (i = 0; i < 2 * VU_METER_CHANNELS; i +=2) {
		info->saturated = (rmh.Stat[0] & (1 << (audio + i))) ? 1 : 0;
		info->vu_level = rmh.Stat[i + 1];
		info->peak_level = rmh.Stat[i + 2];
		info++;
	}
	return 0;
}
   

/*
 * control API entries
 */

/*
 * output level control
 */
static int vx_output_level_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = ANALOG_OUT_ATTEN_MIN;
	uinfo->value.integer.max = ANALOG_OUT_ATTEN_MAX - ANALOG_OUT_ATTEN_MIN;
	return 0;
}

static int vx_output_level_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	vxpocket_t *chip = snd_kcontrol_chip(kcontrol);
	int codec = kcontrol->id.index;
	ucontrol->value.integer.value[0] = ANALOG_OUT_ATTEN_MAX - chip->output_level[codec][0];
	ucontrol->value.integer.value[1] = ANALOG_OUT_ATTEN_MAX - chip->output_level[codec][1];
	return 0;
}

static int vx_output_level_put(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	vxpocket_t *chip = snd_kcontrol_chip(kcontrol);
	int val[2];
	int codec = kcontrol->id.index;
	val[0] = ANALOG_OUT_ATTEN_MAX - ucontrol->value.integer.value[0];
	val[1] = ANALOG_OUT_ATTEN_MAX - ucontrol->value.integer.value[1];
	if (val[0] != chip->output_level[codec][0] ||
	    val[1] != chip->output_level[codec][1]) {
		vx_set_analog_output_level(chip, codec, val[0], val[1]);
		chip->output_level[codec][0] = val[0];
		chip->output_level[codec][1] = val[1];
		return 1;
	}
	return 0;
}

static snd_kcontrol_new_t vx_control_output_level = {
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name =		"Master Playback Volume",
	.info =		vx_output_level_info,
	.get =		vx_output_level_get,
	.put =		vx_output_level_put,
};

/*
 * mic level control (for VXPocket)
 */
static int vx_mic_level_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = MIC_LEVEL_MAX;
	return 0;
}

static int vx_mic_level_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	vxpocket_t *chip = snd_kcontrol_chip(kcontrol);
	ucontrol->value.integer.value[0] = chip->mic_level;
	return 0;
}

static int vx_mic_level_put(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	vxpocket_t *chip = snd_kcontrol_chip(kcontrol);
	if (chip->mic_level != ucontrol->value.integer.value[0]) {
		vx_set_mic_level(chip, ucontrol->value.integer.value[0]);
		chip->mic_level = ucontrol->value.integer.value[0];
		return 1;
	}
	return 0;
}

static snd_kcontrol_new_t vx_control_mic_level = {
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name =		"Mic Capture Volume",
	.info =		vx_mic_level_info,
	.get =		vx_mic_level_get,
	.put =		vx_mic_level_put,
};

/*
 * mic boost level control (for VXP440)
 */
static int vx_mic_boost_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	return 0;
}

static int vx_mic_boost_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	vxpocket_t *chip = snd_kcontrol_chip(kcontrol);
	ucontrol->value.integer.value[0] = chip->mic_level;
	return 0;
}

static int vx_mic_boost_put(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	vxpocket_t *chip = snd_kcontrol_chip(kcontrol);
	if (chip->mic_level != ucontrol->value.integer.value[0]) {
		vx_set_mic_boost(chip, ucontrol->value.integer.value[0]);
		chip->mic_level = ucontrol->value.integer.value[0];
		return 1;
	}
	return 0;
}

static snd_kcontrol_new_t vx_control_mic_boost = {
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name =		"Mic Boost",
	.info =		vx_mic_boost_info,
	.get =		vx_mic_boost_get,
	.put =		vx_mic_boost_put,
};

/*
 * audio source select
 */
static int vx_audio_src_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	static char *texts[3] = {
		"Line", "Mic", "Digital"
	};
	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items = 3;
	if (uinfo->value.enumerated.item > 2)
		uinfo->value.enumerated.item = 2;
	strcpy(uinfo->value.enumerated.name, texts[uinfo->value.enumerated.item]);
	return 0;
}

static int vx_audio_src_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	vxpocket_t *chip = snd_kcontrol_chip(kcontrol);
	ucontrol->value.enumerated.item[0] = chip->audio_source_target;
	return 0;
}

static int vx_audio_src_put(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	vxpocket_t *chip = snd_kcontrol_chip(kcontrol);
	if (chip->audio_source_target != ucontrol->value.enumerated.item[0]) {
		chip->audio_source_target = ucontrol->value.enumerated.item[0];
		vx_sync_audio_source(chip);
		return 1;
	}
	return 0;
}

static snd_kcontrol_new_t vx_control_audio_src = {
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name =		"Capture Source",
	.info =		vx_audio_src_info,
	.get =		vx_audio_src_get,
	.put =		vx_audio_src_put,
};

/*
 * Audio Gain
 */
static int vx_audio_gain_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = CVAL_MAX;
	return 0;
}

static int vx_audio_gain_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	vxpocket_t *chip = snd_kcontrol_chip(kcontrol);
	int audio = kcontrol->private_value & 0xff;
	int capture = (kcontrol->private_value >> 8) & 1;

	ucontrol->value.integer.value[0] = chip->audio_gain[capture][audio];
	ucontrol->value.integer.value[1] = chip->audio_gain[capture][audio+1];
	return 0;
}

static int vx_audio_gain_put(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	vxpocket_t *chip = snd_kcontrol_chip(kcontrol);
	int audio = kcontrol->private_value & 0xff;
	int capture = (kcontrol->private_value >> 8) & 1;

	if (ucontrol->value.integer.value[0] != chip->audio_gain[capture][audio] ||
	    ucontrol->value.integer.value[1] != chip->audio_gain[capture][audio+1]) {
		vx_set_audio_gain(chip, audio, capture, ucontrol->value.integer.value[0]);
		vx_set_audio_gain(chip, audio+1, capture, ucontrol->value.integer.value[1]);
		return 1;
	}
	return 0;
}

static int vx_audio_monitor_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	vxpocket_t *chip = snd_kcontrol_chip(kcontrol);
	int audio = kcontrol->private_value & 0xff;

	ucontrol->value.integer.value[0] = chip->audio_monitor[audio];
	ucontrol->value.integer.value[1] = chip->audio_monitor[audio+1];
	return 0;
}

static int vx_audio_monitor_put(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	vxpocket_t *chip = snd_kcontrol_chip(kcontrol);
	int audio = kcontrol->private_value & 0xff;

	if (ucontrol->value.integer.value[0] != chip->audio_monitor[audio] ||
	    ucontrol->value.integer.value[1] != chip->audio_monitor[audio+1]) {
		vx_set_monitor_level(chip, audio, ucontrol->value.integer.value[0],
				     chip->audio_monitor_active[audio]);
		vx_set_monitor_level(chip, audio+1, ucontrol->value.integer.value[1],
				     chip->audio_monitor_active[audio+1]);
		return 1;
	}
	return 0;
}

static int vx_audio_sw_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	return 0;
}

static int vx_audio_sw_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	vxpocket_t *chip = snd_kcontrol_chip(kcontrol);
	int audio = kcontrol->private_value & 0xff;

	ucontrol->value.integer.value[0] = chip->audio_active[audio];
	ucontrol->value.integer.value[1] = chip->audio_active[audio+1];
	return 0;
}

static int vx_audio_sw_put(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	vxpocket_t *chip = snd_kcontrol_chip(kcontrol);
	int audio = kcontrol->private_value & 0xff;

	if (ucontrol->value.integer.value[0] != chip->audio_active[audio] ||
	    ucontrol->value.integer.value[1] != chip->audio_active[audio+1]) {
		vx_set_audio_switch(chip, audio, ucontrol->value.integer.value[0]);
		vx_set_audio_switch(chip, audio+1, ucontrol->value.integer.value[1]);
		return 1;
	}
	return 0;
}

static int vx_monitor_sw_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	vxpocket_t *chip = snd_kcontrol_chip(kcontrol);
	int audio = kcontrol->private_value & 0xff;

	ucontrol->value.integer.value[0] = chip->audio_monitor_active[audio];
	ucontrol->value.integer.value[1] = chip->audio_monitor_active[audio+1];
	return 0;
}

static int vx_monitor_sw_put(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	vxpocket_t *chip = snd_kcontrol_chip(kcontrol);
	int audio = kcontrol->private_value & 0xff;

	if (ucontrol->value.integer.value[0] != chip->audio_monitor_active[audio] ||
	    ucontrol->value.integer.value[1] != chip->audio_monitor_active[audio+1]) {
		vx_set_monitor_level(chip, audio, chip->audio_monitor[audio],
				     ucontrol->value.integer.value[0]);
		vx_set_monitor_level(chip, audio+1, chip->audio_monitor[audio+1],
				     ucontrol->value.integer.value[1]);
		return 1;
	}
	return 0;
}

static snd_kcontrol_new_t vx_control_audio_gain = {
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	/* name will be filled later */
	.info =         vx_audio_gain_info,
	.get =          vx_audio_gain_get,
	.put =          vx_audio_gain_put
};
static snd_kcontrol_new_t vx_control_output_switch = {
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name =         "PCM Playback Switch",
	.info =         vx_audio_sw_info,
	.get =          vx_audio_sw_get,
	.put =          vx_audio_sw_put
};
static snd_kcontrol_new_t vx_control_monitor_gain = {
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name =         "Monitoring Volume",
	.info =         vx_audio_gain_info,	/* shared */
	.get =          vx_audio_monitor_get,
	.put =          vx_audio_monitor_put
};
static snd_kcontrol_new_t vx_control_monitor_switch = {
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name =         "Monitoring Switch",
	.info =         vx_audio_sw_info,	/* shared */
	.get =          vx_monitor_sw_get,
	.put =          vx_monitor_sw_put
};


/*
 * IEC958 status bits
 */
static int vx_iec958_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_IEC958;
	uinfo->count = 1;
	return 0;
}

static int vx_iec958_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	vxpocket_t *chip = snd_kcontrol_chip(kcontrol);

	ucontrol->value.iec958.status[0] = (chip->uer_bits >> 0) & 0xff;
	ucontrol->value.iec958.status[1] = (chip->uer_bits >> 8) & 0xff;
	ucontrol->value.iec958.status[2] = (chip->uer_bits >> 16) & 0xff;
	ucontrol->value.iec958.status[3] = (chip->uer_bits >> 24) & 0xff;
        return 0;
}

static int vx_iec958_mask_get(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	ucontrol->value.iec958.status[0] = 0xff;
	ucontrol->value.iec958.status[1] = 0xff;
	ucontrol->value.iec958.status[2] = 0xff;
	ucontrol->value.iec958.status[3] = 0xff;
        return 0;
}

static int vx_iec958_put(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	vxpocket_t *chip = snd_kcontrol_chip(kcontrol);
	unsigned int val;

	val = (ucontrol->value.iec958.status[0] << 0) |
	      (ucontrol->value.iec958.status[1] << 8) |
	      (ucontrol->value.iec958.status[2] << 16) |
	      (ucontrol->value.iec958.status[3] << 24);
	if (chip->uer_bits != val) {
		chip->uer_bits = val;
		vx_set_iec958_status(chip, val);
		return 1;
	}
	return 0;
}

static snd_kcontrol_new_t vx_control_iec958_mask = {
	.access =	SNDRV_CTL_ELEM_ACCESS_READ,
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name =		SNDRV_CTL_NAME_IEC958("",PLAYBACK,MASK),
	.info =		vx_iec958_info,	/* shared */
	.get =		vx_iec958_mask_get,
};

static snd_kcontrol_new_t vx_control_iec958 = {
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name =         SNDRV_CTL_NAME_IEC958("",PLAYBACK,DEFAULT),
	.info =         vx_iec958_info,
	.get =          vx_iec958_get,
	.put =          vx_iec958_put
};


/*
 * VU meter
 */

#define METER_MAX	0xff
#define METER_SHIFT	16

static int vx_vu_meter_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = METER_MAX;
	return 0;
}

static int vx_vu_meter_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	vxpocket_t *chip = snd_kcontrol_chip(kcontrol);
	struct vx_vu_meter meter[2];
	int audio = kcontrol->private_value & 0xff;
	int capture = (kcontrol->private_value >> 8) & 1;

	vx_get_audio_vu_meter(chip, audio, capture, meter);
	ucontrol->value.integer.value[0] = meter[0].vu_level >> METER_SHIFT;
	ucontrol->value.integer.value[1] = meter[1].vu_level >> METER_SHIFT;
	return 0;
}

static int vx_peak_meter_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	vxpocket_t *chip = snd_kcontrol_chip(kcontrol);
	struct vx_vu_meter meter[2];
	int audio = kcontrol->private_value & 0xff;
	int capture = (kcontrol->private_value >> 8) & 1;

	vx_get_audio_vu_meter(chip, audio, capture, meter);
	ucontrol->value.integer.value[0] = meter[0].peak_level >> METER_SHIFT;
	ucontrol->value.integer.value[1] = meter[1].peak_level >> METER_SHIFT;
	return 0;
}

static int vx_saturation_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	return 0;
}

static int vx_saturation_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	vxpocket_t *chip = snd_kcontrol_chip(kcontrol);
	struct vx_vu_meter meter[2];
	int audio = kcontrol->private_value & 0xff;

	vx_get_audio_vu_meter(chip, audio, 1, meter); /* capture only */
	ucontrol->value.integer.value[0] = meter[0].saturated;
	ucontrol->value.integer.value[1] = meter[1].saturated;
	return 0;
}

static snd_kcontrol_new_t vx_control_vu_meter = {
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.access =	SNDRV_CTL_ELEM_ACCESS_READ | SNDRV_CTL_ELEM_ACCESS_VOLATILE,
	/* name will be filled later */
	.info =		vx_vu_meter_info,
	.get =		vx_vu_meter_get,
};

static snd_kcontrol_new_t vx_control_peak_meter = {
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.access =	SNDRV_CTL_ELEM_ACCESS_READ | SNDRV_CTL_ELEM_ACCESS_VOLATILE,
	/* name will be filled later */
	.info =		vx_vu_meter_info,	/* shared */
	.get =		vx_peak_meter_get,
};

static snd_kcontrol_new_t vx_control_saturation = {
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name =		"Input Saturation",
	.access =	SNDRV_CTL_ELEM_ACCESS_READ | SNDRV_CTL_ELEM_ACCESS_VOLATILE,
	.info =		vx_saturation_info,
	.get =		vx_saturation_get,
};



/*
 *
 */

int snd_vxpocket_mixer_new(vxpocket_t *chip)
{
	int i, c, err;
	snd_kcontrol_new_t temp;
	snd_card_t *card = chip->card;
	char name[32];

	strcpy(card->mixername, "VXPocket");

	/* output level controls */
	for (i = 0; i < chip->hw->num_outs; i++) {
		temp = vx_control_output_level;
		temp.index = i;
		if ((err = snd_ctl_add(card, snd_ctl_new1(&temp, chip))) < 0)
			return err;
	}

	/* mic level */
	switch (chip->hw->type) {
	case VXP_TYPE_VXPOCKET:
		if ((err = snd_ctl_add(card, snd_ctl_new1(&vx_control_mic_level, chip))) < 0)
			return err;
		break;
	case VXP_TYPE_VXP440:
		if ((err = snd_ctl_add(card, snd_ctl_new1(&vx_control_mic_boost, chip))) < 0)
			return err;
		break;
	}
	/* PCM volumes, switches, monitoring */
	for (i = 0; i < chip->hw->num_outs; i++) {
		int val = i * 2;
		temp = vx_control_audio_gain;
		temp.index = i;
		temp.name = "PCM Playback Volume";
		temp.private_value = val;
		if ((err = snd_ctl_add(card, snd_ctl_new1(&temp, chip))) < 0)
			return err;
		temp = vx_control_output_switch;
		temp.index = i;
		temp.private_value = val;
		if ((err = snd_ctl_add(card, snd_ctl_new1(&temp, chip))) < 0)
			return err;
		temp = vx_control_monitor_gain;
		temp.index = i;
		temp.private_value = val;
		if ((err = snd_ctl_add(card, snd_ctl_new1(&temp, chip))) < 0)
			return err;
		temp = vx_control_monitor_switch;
		temp.index = i;
		temp.private_value = val;
		if ((err = snd_ctl_add(card, snd_ctl_new1(&temp, chip))) < 0)
			return err;
	}
	for (i = 0; i < chip->hw->num_outs; i++) {
		temp = vx_control_audio_gain;
		temp.index = i;
		temp.name = "PCM Capture Volume";
		temp.private_value = (i * 2) | (1 << 8);
		if ((err = snd_ctl_add(card, snd_ctl_new1(&temp, chip))) < 0)
			return err;
	}

	/* Audio source */
	if ((err = snd_ctl_add(card, snd_ctl_new1(&vx_control_audio_src, chip))) < 0)
		return err;
	/* IEC958 controls */
	if ((err = snd_ctl_add(card, snd_ctl_new1(&vx_control_iec958_mask, chip))) < 0)
		return err;
	if ((err = snd_ctl_add(card, snd_ctl_new1(&vx_control_iec958, chip))) < 0)
		return err;
	/* VU, peak, saturation meters */
	for (c = 0; c < 1; c++) {
		static char *dir[2] = { "Output", "Input" };
		for (i = 0; i < chip->hw->num_ins; i++) {
			int val = (i * 2) | (c << 8);
			if (c == 1) {
				temp = vx_control_saturation;
				temp.index = i;
				temp.private_value = val;
				if ((err = snd_ctl_add(card, snd_ctl_new1(&temp, chip))) < 0)
					return err;
			}
			sprintf(name, "%s VU Meter", dir[c]);
			temp = vx_control_vu_meter;
			temp.index = i;
			temp.name = name;
			temp.private_value = val;
			if ((err = snd_ctl_add(card, snd_ctl_new1(&temp, chip))) < 0)
				return err;
			sprintf(name, "%s Peak Meter", dir[c]);
			temp = vx_control_peak_meter;
			temp.index = i;
			temp.name = name;
			temp.private_value = val;
			if ((err = snd_ctl_add(card, snd_ctl_new1(&temp, chip))) < 0)
				return err;
		}
	}
	return 0;
}
