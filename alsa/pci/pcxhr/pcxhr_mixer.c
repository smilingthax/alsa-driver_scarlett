/*
 * Driver for Digigram pcxhr compatible soundcards
 *
 * mixer callbacks
 *
 * Copyright (c) 2004 by Digigram <alsa@digigram.com>
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
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <sound/core.h>
#include "pcxhr.h"
#include "pcxhr_hwdep.h"
#include "pcxhr_core.h"
#include <sound/control.h>
#include "pcxhr_mixer.h"


#define PCXHR_ANALOG_CAPTURE_LEVEL_MIN   0	/* -96.0 dB */
#define PCXHR_ANALOG_CAPTURE_LEVEL_MAX   255	/* +31.5 dB */
#define PCXHR_ANALOG_CAPTURE_ZERO_LEVEL  224	/* +16.0 dB ( +31.5 dB - fix level +15.5 dB ) */

#define PCXHR_ANALOG_PLAYBACK_LEVEL_MIN  0	/* -128.0 dB */
#define PCXHR_ANALOG_PLAYBACK_LEVEL_MAX  128	/*    0.0 dB */
#define PCXHR_ANALOG_PLAYBACK_ZERO_LEVEL 104	/*  -24.0 dB ( 0.0 dB - fix level +24.0 dB ) */

static int pcxhr_update_analog_audio_level(pcxhr_t* chip, int is_capture, int channel)
{
	int err, vol;
	pcxhr_rmh_t rmh;

	pcxhr_init_rmh(&rmh, CMD_ACCESS_IO_WRITE);
	if(is_capture) {
		rmh.cmd[0] |= IO_NUM_REG_IN_ANA_LEVEL;
		rmh.cmd[2] = chip->analog_capture_volume[channel];
	} else {
		rmh.cmd[0] |= IO_NUM_REG_OUT_ANA_LEVEL;
		if(chip->analog_playback_active[channel])
			vol = chip->analog_playback_volume[channel];
		else
			vol = PCXHR_ANALOG_PLAYBACK_LEVEL_MIN;
		rmh.cmd[2] = PCXHR_ANALOG_PLAYBACK_LEVEL_MAX - vol;	/* playback analog levels are inversed */
	}
	rmh.cmd[1]  = 1 << ((2 * chip->chip_idx) + channel);	/* audio mask */
	rmh.cmd_len = 3;
	err = pcxhr_send_msg(chip->mgr, &rmh);
	if(err<0) {
		snd_printk(KERN_DEBUG "error update_analog_audio_level card(%d) is_capture(%d) err(%x)\n", chip->chip_idx, is_capture, err);
		return -EINVAL;
	}
	return 0;
}

/*
 * analog level control
 */
static int pcxhr_analog_vol_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	if(kcontrol->private_value == 0) {	/* playback */
		uinfo->value.integer.min = PCXHR_ANALOG_PLAYBACK_LEVEL_MIN;	/* -128 dB */
		uinfo->value.integer.max = PCXHR_ANALOG_PLAYBACK_LEVEL_MAX;	/* 0 dB */
	} else {				/* capture */
		uinfo->value.integer.min = PCXHR_ANALOG_CAPTURE_LEVEL_MIN;	/* -96 dB */
		uinfo->value.integer.max = PCXHR_ANALOG_CAPTURE_LEVEL_MAX;	/* 31.5 dB */
	}
	return 0;
}

static int pcxhr_analog_vol_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pcxhr_t *chip = snd_kcontrol_chip(kcontrol);
	down(&chip->mgr->mixer_mutex);
	if(kcontrol->private_value == 0) {	/* playback */
		ucontrol->value.integer.value[0] = chip->analog_playback_volume[0];
		ucontrol->value.integer.value[1] = chip->analog_playback_volume[1];
	} else {				/* capture */
		ucontrol->value.integer.value[0] = chip->analog_capture_volume[0];
		ucontrol->value.integer.value[1] = chip->analog_capture_volume[1];
	}
	up(&chip->mgr->mixer_mutex);
	return 0;
}

static int pcxhr_analog_vol_put(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pcxhr_t *chip = snd_kcontrol_chip(kcontrol);
	int changed = 0;
	int is_capture, i;

	down(&chip->mgr->mixer_mutex);
	is_capture = (kcontrol->private_value != 0);
	for(i=0; i<2; i++) {
		int  new_volume = ucontrol->value.integer.value[i];
		int* stored_volume = is_capture ? &chip->analog_capture_volume[i] : &chip->analog_playback_volume[i];
		if(*stored_volume != new_volume) {
			*stored_volume = new_volume;
			changed = 1;
			pcxhr_update_analog_audio_level(chip, is_capture, i);
		}
	}
	up(&chip->mgr->mixer_mutex);
	return changed;
}

static snd_kcontrol_new_t pcxhr_control_analog_level = {
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	/* name will be filled later */
	.info =		pcxhr_analog_vol_info,
	.get =		pcxhr_analog_vol_get,
	.put =		pcxhr_analog_vol_put,
};

/* shared */
static int pcxhr_sw_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	return 0;
}

static int pcxhr_audio_sw_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pcxhr_t *chip = snd_kcontrol_chip(kcontrol);

	down(&chip->mgr->mixer_mutex);
	ucontrol->value.integer.value[0] = chip->analog_playback_active[0];
	ucontrol->value.integer.value[1] = chip->analog_playback_active[1];
	up(&chip->mgr->mixer_mutex);
	return 0;
}

static int pcxhr_audio_sw_put(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pcxhr_t *chip = snd_kcontrol_chip(kcontrol);
	int i, changed = 0;
	down(&chip->mgr->mixer_mutex);
	for(i=0; i<2; i++) {
		if(chip->analog_playback_active[i] != ucontrol->value.integer.value[i]) {
			chip->analog_playback_active[i] = ucontrol->value.integer.value[i];
			changed = 1;
			pcxhr_update_analog_audio_level(chip, 0, i);	/* update playback levels */
		}
	}
	up(&chip->mgr->mixer_mutex);
	return changed;
}

static snd_kcontrol_new_t pcxhr_control_output_switch = {
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name =		"Master Playback Switch",
	.info =		pcxhr_sw_info,		/* shared */
	.get =		pcxhr_audio_sw_get,
	.put =		pcxhr_audio_sw_put
};


#define PCXHR_DIGITAL_LEVEL_MIN		0x000	/* -110 dB */
#define PCXHR_DIGITAL_LEVEL_MAX		0x1ff	/* +18 dB */
#define PCXHR_DIGITAL_ZERO_LEVEL	0x1b7	/*  0 dB */


#define MORE_THAN_ONE_STREAM_LEVEL	0x000001
#define VALID_STREAM_PAN_LEVEL_MASK	0x800000
#define VALID_STREAM_LEVEL_MASK		0x400000
#define VALID_STREAM_LEVEL_1_MASK	0x200000
#define VALID_STREAM_LEVEL_2_MASK	0x100000

int pcxhr_update_playback_stream_level(pcxhr_t* chip, int idx)
{
	int err;
	pcxhr_rmh_t rmh;
	pcxhr_pipe_t *pipe = &chip->playback_pipe;
	int left, right;

	/* do only when pipe exists ! */
	if(pipe->status == PCXHR_PIPE_UNDEFINED)
		return 0;

	if(chip->digital_playback_active[idx][0])	left = chip->digital_playback_volume[idx][0];
	else						left = PCXHR_DIGITAL_LEVEL_MIN;
	if(chip->digital_playback_active[idx][1])	right = chip->digital_playback_volume[idx][1];
	else						right = PCXHR_DIGITAL_LEVEL_MIN;

	pcxhr_init_rmh(&rmh, CMD_STREAM_OUT_LEVEL_ADJUST);
	pcxhr_set_pipe_cmd_params(&rmh, 0, pipe->first_audio, 0, 1<<idx);	/* add pipe and stream mask */
	/* volume left->left / right->right panoramic level */
	rmh.cmd[0] |= MORE_THAN_ONE_STREAM_LEVEL;
	rmh.cmd[2]  = VALID_STREAM_PAN_LEVEL_MASK | VALID_STREAM_LEVEL_1_MASK;
	rmh.cmd[2] |= (left << 10);
	rmh.cmd[3]  = VALID_STREAM_PAN_LEVEL_MASK | VALID_STREAM_LEVEL_2_MASK;
	rmh.cmd[3] |= right;
	rmh.cmd_len = 4;

	err = pcxhr_send_msg(chip->mgr, &rmh);
	if(err<0) {
		snd_printk(KERN_DEBUG "error update_playback_stream_level card(%d) err(%x)\n", chip->chip_idx, err);
		return -EINVAL;
	}
	return 0;
}

#define AUDIO_IO_HAS_MUTE_LEVEL		0x400000
#define AUDIO_IO_HAS_MUTE_MONITOR_1	0x200000
#define VALID_AUDIO_IO_DIGITAL_LEVEL	0x000001
#define VALID_AUDIO_IO_MONITOR_LEVEL	0x000002
#define VALID_AUDIO_IO_MUTE_LEVEL	0x000004
#define VALID_AUDIO_IO_MUTE_MONITOR_1	0x000008

int pcxhr_update_audio_pipe_level(pcxhr_t* chip, int capture, int channel)
{
	int err;
	pcxhr_rmh_t rmh;
	pcxhr_pipe_t *pipe;

	if(capture)	pipe = &chip->capture_pipe[0];
	else		pipe = &chip->playback_pipe;
	/* only when pipe exists ! */
	if(pipe->status == PCXHR_PIPE_UNDEFINED)
		return 0;

	pcxhr_init_rmh(&rmh, CMD_AUDIO_LEVEL_ADJUST);
	pcxhr_set_pipe_cmd_params(&rmh, capture, 0, 0, 1 << (channel + pipe->first_audio));	/* add channel mask */
	/* TODO : if mask (3 << pipe->first_audio) is used, left and right channel will be programmed to the same params */
	if(capture) {
		rmh.cmd[0] |= VALID_AUDIO_IO_DIGITAL_LEVEL;
		/* VALID_AUDIO_IO_MUTE_LEVEL not yet handled (capture pipe level) */
		rmh.cmd[2] = chip->digital_capture_volume[channel];
	} else {
		rmh.cmd[0] |= VALID_AUDIO_IO_MONITOR_LEVEL | VALID_AUDIO_IO_MUTE_MONITOR_1;
		/* VALID_AUDIO_IO_DIGITAL_LEVEL and VALID_AUDIO_IO_MUTE_LEVEL not yet handled (playback pipe level) */
		rmh.cmd[2] = chip->monitoring_volume[channel] << 10;
		if(chip->monitoring_active[channel] == 0)
			rmh.cmd[2] |= AUDIO_IO_HAS_MUTE_MONITOR_1;
	}
	rmh.cmd_len = 3;

	err = pcxhr_send_msg(chip->mgr, &rmh);
	if(err<0) {
		snd_printk(KERN_DEBUG "error update_audio_level card(%d) err(%x)\n", chip->chip_idx, err);
		return -EINVAL;
	}
	return 0;
}


/* shared */
static int pcxhr_digital_vol_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = PCXHR_DIGITAL_LEVEL_MIN;   /* -109.5 dB */
	uinfo->value.integer.max = PCXHR_DIGITAL_LEVEL_MAX;   /*   18.0 dB */
	return 0;
}


static int pcxhr_pcm_vol_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pcxhr_t *chip = snd_kcontrol_chip(kcontrol);
	int idx = snd_ctl_get_ioffidx(kcontrol, &ucontrol->id);		/* index */
	int *stored_volume;
	int is_capture = kcontrol->private_value;
	down(&chip->mgr->mixer_mutex);
	if(is_capture) {
		stored_volume = chip->digital_capture_volume;		/* digital capture */
	} else {
		snd_assert ( idx < PCXHR_PLAYBACK_STREAMS ); 
		stored_volume = chip->digital_playback_volume[idx];	/* digital playback */
	}
	ucontrol->value.integer.value[0] = stored_volume[0];
	ucontrol->value.integer.value[1] = stored_volume[1];
	up(&chip->mgr->mixer_mutex);
	return 0;
}

static int pcxhr_pcm_vol_put(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pcxhr_t *chip = snd_kcontrol_chip(kcontrol);
	int idx = snd_ctl_get_ioffidx(kcontrol, &ucontrol->id);		/* index */
	int changed = 0;
	int is_capture = kcontrol->private_value;
	int* stored_volume;
	int i;
	down(&chip->mgr->mixer_mutex);
	if(is_capture) {
		stored_volume = chip->digital_capture_volume;		/* digital capture */
	} else {
		snd_assert ( idx < PCXHR_PLAYBACK_STREAMS ); 
		stored_volume = chip->digital_playback_volume[idx];	/* digital playback */
	}
	for(i=0; i<2; i++) {
		if(stored_volume[i] != ucontrol->value.integer.value[i]) {
			stored_volume[i] = ucontrol->value.integer.value[i];
			changed = 1;
			if(is_capture)	pcxhr_update_audio_pipe_level(chip, 1, i);	/* update capture volume */
		}
	}
	if((!is_capture) && changed)	pcxhr_update_playback_stream_level(chip, idx);	/* update playback volume */
	up(&chip->mgr->mixer_mutex);
	return changed;
}

static snd_kcontrol_new_t snd_pcxhr_pcm_vol =
{
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	/* name will be filled later */
	/* count will be filled later */
	.info =		pcxhr_digital_vol_info,		/* shared */
	.get =		pcxhr_pcm_vol_get,
	.put =		pcxhr_pcm_vol_put,
};


static int pcxhr_pcm_sw_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pcxhr_t *chip = snd_kcontrol_chip(kcontrol);
	int idx = snd_ctl_get_ioffidx(kcontrol, &ucontrol->id); /* index */
	snd_assert ( idx < PCXHR_PLAYBACK_STREAMS ); 
	down(&chip->mgr->mixer_mutex);
	ucontrol->value.integer.value[0] = chip->digital_playback_active[idx][0];
	ucontrol->value.integer.value[1] = chip->digital_playback_active[idx][1];
	up(&chip->mgr->mixer_mutex);
	return 0;
}

static int pcxhr_pcm_sw_put(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pcxhr_t *chip = snd_kcontrol_chip(kcontrol);
	int changed = 0;
	int idx = snd_ctl_get_ioffidx(kcontrol, &ucontrol->id); /* index */
	int i, j;
	snd_assert ( idx < PCXHR_PLAYBACK_STREAMS ); 
	down(&chip->mgr->mixer_mutex);
	j = idx;
	for(i=0; i<2; i++) {
		if(chip->digital_playback_active[j][i] != ucontrol->value.integer.value[i]) {
			chip->digital_playback_active[j][i] = ucontrol->value.integer.value[i];
			changed = 1;
		}
	}
	if(changed)	pcxhr_update_playback_stream_level(chip, idx);
	up(&chip->mgr->mixer_mutex);
	return changed;
}

static snd_kcontrol_new_t pcxhr_control_pcm_switch = {
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	/* name will be filled later */
	.count =	PCXHR_PLAYBACK_STREAMS,
	.info =		pcxhr_sw_info,		/* shared */
	.get =		pcxhr_pcm_sw_get,
	.put =		pcxhr_pcm_sw_put
};


/*
 * monitoring level control
 */

static int pcxhr_monitor_vol_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pcxhr_t *chip = snd_kcontrol_chip(kcontrol);
	down(&chip->mgr->mixer_mutex);
	ucontrol->value.integer.value[0] = chip->monitoring_volume[0];
	ucontrol->value.integer.value[1] = chip->monitoring_volume[1];
	up(&chip->mgr->mixer_mutex);
	return 0;
}

static int pcxhr_monitor_vol_put(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pcxhr_t *chip = snd_kcontrol_chip(kcontrol);
	int changed = 0;
	int i;
	down(&chip->mgr->mixer_mutex);
	for(i=0; i<2; i++) {
		if(chip->monitoring_volume[i] != ucontrol->value.integer.value[i]) {
			chip->monitoring_volume[i] = ucontrol->value.integer.value[i];
			if(chip->monitoring_active[i])				/* do only when monitoring is unmuted : */
				pcxhr_update_audio_pipe_level(chip, 0, i);	/* update monitoring volume and mute */
			changed = 1;
		}
	}
	up(&chip->mgr->mixer_mutex);
	return changed;
}

static snd_kcontrol_new_t pcxhr_control_monitor_vol = {
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name =         "Monitoring Volume",
	.info =		pcxhr_digital_vol_info,		/* shared */
	.get =		pcxhr_monitor_vol_get,
	.put =		pcxhr_monitor_vol_put,
};

/*
 * monitoring switch control
 */

static int pcxhr_monitor_sw_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pcxhr_t *chip = snd_kcontrol_chip(kcontrol);
	down(&chip->mgr->mixer_mutex);
	ucontrol->value.integer.value[0] = chip->monitoring_active[0];
	ucontrol->value.integer.value[1] = chip->monitoring_active[1];
	up(&chip->mgr->mixer_mutex);
	return 0;
}

static int pcxhr_monitor_sw_put(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	pcxhr_t *chip = snd_kcontrol_chip(kcontrol);
	int changed = 0;
	int i;
	down(&chip->mgr->mixer_mutex);
	for(i=0; i<2; i++) {
		if(chip->monitoring_active[i] != ucontrol->value.integer.value[i]) {
			chip->monitoring_active[i] = ucontrol->value.integer.value[i];
			changed |= (1<<i); /* mask 0x01 and 0x02 */
		}
	}
	if(changed & 0x01)	pcxhr_update_audio_pipe_level(chip, 0, 0);	/* update left monitoring volume and mute */
	if(changed & 0x02)	pcxhr_update_audio_pipe_level(chip, 0, 1);	/* update right monitoring volume and mute */

	up(&chip->mgr->mixer_mutex);
	return (changed != 0);
}

static snd_kcontrol_new_t pcxhr_control_monitor_sw = {
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name =         "Monitoring Switch",
	.info =         pcxhr_sw_info,		/* shared */
	.get =          pcxhr_monitor_sw_get,
	.put =          pcxhr_monitor_sw_put
};


static void pcxhr_reset_audio_levels(pcxhr_t *chip)
{
	int i;
/* only for test purpose, remove later */
#ifdef CONFIG_SND_DEBUG
	for(i=0; i<2; i++) {
		int j;
		chip->analog_capture_volume[i]  = PCXHR_ANALOG_CAPTURE_ZERO_LEVEL;
		chip->analog_playback_active[i] = 1;
		chip->analog_playback_volume[i] = PCXHR_ANALOG_PLAYBACK_ZERO_LEVEL;
		for(j=0; j<PCXHR_PLAYBACK_STREAMS; j++) {
			chip->digital_playback_active[j][i] = 1;
			chip->digital_playback_volume[j][i] = PCXHR_DIGITAL_ZERO_LEVEL;
		}
		chip->digital_capture_volume[i] = PCXHR_DIGITAL_ZERO_LEVEL;
		chip->monitoring_active[i] = 0;
		chip->monitoring_volume[i] = PCXHR_DIGITAL_ZERO_LEVEL;
	}
#endif
/* test end */
	for(i=0; i<2; i++) {
/* only for test purpose, remove later */
#ifdef CONFIG_SND_DEBUG
		/* analog volumes for playback (already is LEVEL_MIN after boot) */
		if(chip->nb_streams_play)	pcxhr_update_analog_audio_level(chip, 0, i);
		/* analog levels for capture (already is LEVEL_MIN after boot) */
		if(chip->nb_streams_capt)	pcxhr_update_analog_audio_level(chip, 1, i);
#endif
/* test end */
		/* digital capture level (default is 0dB unmuted after capture pipe allocation) */
		if(chip->nb_streams_capt)	pcxhr_update_audio_pipe_level(chip, 1, i);
		/* digital monitoring level (default is 0dB after playback pipe allocation, but muted!) */
		/* pcxhr_update_audio_pipe_level(chip, 0, i); */ /* no need to update */
	}
	if(chip->nb_streams_play) {
		for(i=0; i<PCXHR_PLAYBACK_STREAMS; i++) {
			/* digital playback stream levels (default is 0dB unmuted after pipe allocation) */
			pcxhr_update_playback_stream_level(chip, i);
		}
	}

	return;
}


int pcxhr_create_mixer(pcxhr_mgr_t *mgr)
{
	pcxhr_t *chip;
	int err, i;

	init_MUTEX(&mgr->mixer_mutex); /* can be in another place */

	for(i=0; i<mgr->num_cards; i++) {
		snd_kcontrol_new_t temp;
		chip = mgr->chip[i];

		if(chip->nb_streams_play) {
			/* analog output level control */
			temp = pcxhr_control_analog_level;
			temp.name = "Master Playback Volume";
			temp.private_value = 0; /* playback */
			if ((err = snd_ctl_add(chip->card, snd_ctl_new1(&temp, chip))) < 0)
				return err;
			/* output mute controls */
			if ((err = snd_ctl_add(chip->card, snd_ctl_new1(&pcxhr_control_output_switch, chip))) < 0)
				return err;

			temp = snd_pcxhr_pcm_vol;
			temp.name = "PCM Playback Volume";
			temp.count = PCXHR_PLAYBACK_STREAMS;
			temp.private_value = 0; /* playback */
			if ((err = snd_ctl_add(chip->card, snd_ctl_new1(&temp, chip))) < 0)
				return err;

			temp = pcxhr_control_pcm_switch;
			temp.name = "PCM Playback Switch";
			temp.private_value = 0; /* playback */
			if ((err = snd_ctl_add(chip->card, snd_ctl_new1(&temp, chip))) < 0)
				return err;
		}
		if(chip->nb_streams_capt) {
			/* analog input level control only on first two chips !*/
			temp = pcxhr_control_analog_level;
			temp.name = "Master Capture Volume";
			temp.private_value = 1; /* capture */
			if ((err = snd_ctl_add(chip->card, snd_ctl_new1(&temp, chip))) < 0)
				return err;

			temp = snd_pcxhr_pcm_vol;
			temp.name = "PCM Capture Volume";
			temp.count = 1;
			temp.private_value = 1; /* capture */
			if ((err = snd_ctl_add(chip->card, snd_ctl_new1(&temp, chip))) < 0)
				return err;
		}
		/* monitoring only if playback and capture device available */
		if((chip->nb_streams_capt>0) && (chip->nb_streams_play>0)) {
			/* monitoring */
			if ((err = snd_ctl_add(chip->card, snd_ctl_new1(&pcxhr_control_monitor_vol, chip))) < 0)
				return err;
			if ((err = snd_ctl_add(chip->card, snd_ctl_new1(&pcxhr_control_monitor_sw, chip))) < 0)
				return err;
		}

		/* init all mixer data and program the master volumes/switches */
		pcxhr_reset_audio_levels(chip);
	}
	return 0;
}
