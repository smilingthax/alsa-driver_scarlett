#ifndef __MIXER_OSS_H
#define __MIXER_OSS_H

/*
 *  OSS MIXER API
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>
 *
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
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifdef CONFIG_SND_OSSEMUL

typedef struct snd_stru_oss_mixer_channel snd_mixer_oss_channel_t;
typedef struct snd_stru_oss_file snd_mixer_oss_file_t;

typedef int (*snd_mixer_oss_get_volume_t)(snd_mixer_oss_file_t *fmixer, snd_mixer_oss_channel_t *chn, int *left, int *right);
typedef int (*snd_mixer_oss_put_volume_t)(snd_mixer_oss_file_t *fmixer, snd_mixer_oss_channel_t *chn, int left, int right);
typedef int (*snd_mixer_oss_get_recsrc_t)(snd_mixer_oss_file_t *fmixer, snd_mixer_oss_channel_t *chn, int *active);
typedef int (*snd_mixer_oss_put_recsrc_t)(snd_mixer_oss_file_t *fmixer, snd_mixer_oss_channel_t *chn, int active);
typedef int (*snd_mixer_oss_get_recsrce_t)(snd_mixer_oss_file_t *fmixer, int *active_index);
typedef int (*snd_mixer_oss_put_recsrce_t)(snd_mixer_oss_file_t *fmixer, int active_index);

struct snd_stru_oss_mixer_channel {
	int number;
	int stereo: 1;
	snd_mixer_oss_get_volume_t get_volume;
	snd_mixer_oss_put_volume_t put_volume;
	snd_mixer_oss_get_recsrc_t get_recsrc;
	snd_mixer_oss_put_recsrc_t put_recsrc;
	unsigned long private_value;
	void *private_data;
	void (*private_free)(snd_mixer_oss_channel_t *channel);
};

struct snd_stru_oss_mixer {
	snd_card_t *card;
	char id[16];
	char name[32];
	snd_mixer_oss_channel_t channels[32];	/* OSS mixer channels */
	unsigned int mask_recsrc;		/* exclusive recsrc mask */
	snd_mixer_oss_get_recsrce_t get_recsrc;
	snd_mixer_oss_put_recsrce_t put_recsrc;
	void *private_data_recsrc;
	void (*private_free_recsrc)(snd_mixer_oss_t *mixer);
	/* --- */
	int oss_recsrc;
};

struct snd_stru_oss_file {
	int volume[32][2];
	snd_card_t *card;
	snd_mixer_oss_t *mixer;
};

#endif				/* CONFIG_SND_OSSEMUL */

#endif				/* __MIXER_OSS_H */
