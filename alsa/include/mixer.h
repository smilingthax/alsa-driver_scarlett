#ifndef __MIXER_H
#define __MIXER_H

/*
 *  Abstraction layer for MIXER
 *  Copyright (c) by Jaroslav Kysela <perex@jcu.cz>
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

#define SND_MIXER_CHANNELS	16	/* max channels per card */
#define SND_MIXER_SWITCHES	16	/* max mixer switches per card */

#define SND_MIXER_PRI_MASTER		0x00000100
#define SND_MIXER_PRI_MASTER1		0x00000101
#define SND_MIXER_PRI_MASTERD		0x00000102
#define SND_MIXER_PRI_MASTERD1		0x00000103
#define SND_MIXER_PRI_HEADPHONE		0x00000104
#define SND_MIXER_PRI_MASTER_MONO	0x00000105
#define SND_MIXER_PRI_3D		0x00000108
#define SND_MIXER_PRI_3D_VOLUME		0x00000109
#define SND_MIXER_PRI_3D_CENTER		0x0000010a
#define SND_MIXER_PRI_3D_SPACE		0x0000010b
#define SND_MIXER_PRI_3D_DEPTH		0x0000010c
#define SND_MIXER_PRI_BASS		0x00000110
#define SND_MIXER_PRI_TREBLE		0x00000120
#define SND_MIXER_PRI_FADER		0x00000140
#define SND_MIXER_PRI_SYNTHESIZER	0x00000200
#define SND_MIXER_PRI_SYNTHESIZER1	0x00000300
#define SND_MIXER_PRI_FM		0x00000400
#define SND_MIXER_PRI_EFFECT		0x00000500
#define SND_MIXER_PRI_DSP		0x00000580
#define SND_MIXER_PRI_PCM		0x00000600
#define SND_MIXER_PRI_PCM1		0x00000700
#define SND_MIXER_PRI_LINE		0x00000800
#define SND_MIXER_PRI_MIC		0x00000900
#define SND_MIXER_PRI_CD		0x00000a00
#define SND_MIXER_PRI_VIDEO		0x00000a80
#define SND_MIXER_PRI_PHONE		0x00000a90
#define SND_MIXER_PRI_GAIN		0x00000b00
#define SND_MIXER_PRI_MIC_GAIN		0x00000b80
#define SND_MIXER_PRI_IGAIN		0x00000c00
#define SND_MIXER_PRI_OGAIN		0x00000d00
#define SND_MIXER_PRI_LOOPBACK		0x00000e00
#define SND_MIXER_PRI_SPEAKER		0x00000f00
#define SND_MIXER_PRI_MONO		0x00000f80
#define SND_MIXER_PRI_MONO1		0x00000f81
#define SND_MIXER_PRI_MONO2		0x00000f82
#define SND_MIXER_PRI_AUXA		0xf0000000
#define SND_MIXER_PRI_AUXB		0xf0000100
#define SND_MIXER_PRI_AUXC		0xf0000200
#define SND_MIXER_PRI_PARENT		0xffffffff
#define SND_MIXER_PRI_HIDDEN		0xffffffff

/* volume types */
#define SND_MIX_VOL_OUTPUT	0
#define SND_MIX_VOL_INPUT	1

/* control commands */
#define SND_MIX_CTRL_MUTE_BASE	0
#define SND_MIX_CTRL_MUTE_OUT	SND_MIX_CTRL_MUTE_BASE | SND_MIX_VOL_OUTPUT
#define SND_MIX_CTRL_MUTE_IN	SND_MIX_CTRL_MUTE_BASE | SND_MIX_VOL_INPUT

#define SND_MIX_CTRL_ROUTE_BASE	2
#define SND_MIX_CTRL_ROUTE_OUT	SND_MIX_CTRL_ROUTE_BASE | SND_MIX_VOL_OUTPUT
#define SND_MIX_CTRL_ROUTE_IN	SND_MIX_CTRL_ROUTE_BASE | SND_MIX_VOL_INPUT

/* mute values */
#define SND_MIX_MUTE_LEFT	1
#define SND_MIX_MUTE_RIGHT	2
#define SND_MIX_MUTE		(SND_MIX_MUTE_LEFT|SND_MIX_MUTE_RIGHT)

/* route values */
#define SND_MIX_ROUTE_LTOR	1
#define SND_MIX_ROUTE_RTOL	2
#define SND_MIX_ROUTE_SWAP	(SND_MIX_ROUTE_LTOR|SND_MIX_ROUTE_RTOL)

#define SND_MIX_HW_CHANGE	1
#define SND_MIX_HW_ADD_VOLUME	2
#define SND_MIX_HW_XOR_MUTE	4
#define SND_MIX_HW_JOIN_MUTE	8

typedef struct snd_stru_mixer_channel snd_kmixer_channel_t;
typedef struct snd_stru_mixer_switch snd_kmixer_switch_t;
typedef struct snd_stru_mixer_file snd_kmixer_file_t;

struct snd_stru_mixer_channel_hw {
	unsigned int priority;		/* this is sorting key - highter value = end */
	unsigned int parent_priority;	/* parent.... */
	char name[16];			/* device name */
	unsigned short ossdev;		/* assigned OSS device number */
	unsigned int caps;		/* capabilities - SND_MIXER_CINFO_CAP_XXXX */
	unsigned int ocaps;		/* output capabilities */
	unsigned int icaps;		/* input capabilities */
	int omin, omax;			/* output min and max left & right value */
	int omin_dB, omax_dB;		/* output min_dB, max_dB */
	int imin, imax;			/* input min and max left & right value */
	int imin_dB, imax_dB;		/* input min_dB, max_dB */

	int (*set_control) (snd_kmixer_t * mixer,
			    snd_kmixer_channel_t * channel,
			    unsigned int ctrl, long val);
	int (*set_volume_level) (snd_kmixer_t * mixer,
				 snd_kmixer_channel_t * channel,
				 unsigned int type, int left, int right);
	unsigned int private_value;	/* can be used by low-level driver */
};

struct snd_stru_mixer_channel_direction {
	unsigned char aleft;	/* application - 0 to 100 */
	unsigned char aright;	/* application - 0 to 100 */
	int uleft;		/* user - 0 to max */
	int uright;		/* user - 0 to max */
	int left;		/* real - 0 to max */
	int right;		/* real - 0 to max */
	unsigned char umute;	/* user mute */
	unsigned char kmute;	/* kernel mute */

	unsigned char mute;	/* real mute flags */
	unsigned char route;	/* route path */
};

struct snd_stru_mixer_channel {
	unsigned short channel;	/* channel index */

	unsigned char pad;	/* reserved */

	/* output volumes */
	struct snd_stru_mixer_channel_direction output;
	/* input volumes (for recording) */
	struct snd_stru_mixer_channel_direction input;

	unsigned int private_value;
	void *private_data;	/* not freed by kernel/mixer.c */

	struct snd_stru_mixer_channel_hw hw;	/* readonly variables */
};

struct snd_stru_mixer_switch {
	char name[32];
	int (*get_switch) (snd_kmixer_t * mixer, snd_kmixer_switch_t * kswitch, snd_mixer_switch_t * uswitch);
	int (*set_switch) (snd_kmixer_t * mixer, snd_kmixer_switch_t * kswitch, snd_mixer_switch_t * uswitch);
	unsigned int private_value;
	void *private_data;	/* not freed by mixer.c */
};

struct snd_stru_mixer_file {
	snd_kmixer_t *mixer;
	int osscompat;		/* oss compatible mode */
	int exact;		/* exact mode for this file */
	volatile unsigned int changes;
	volatile unsigned int ochanges;
	volatile unsigned int ichanges;
	volatile unsigned int schanges;
	snd_sleep_define(change);
	struct snd_stru_mixer_file *next;
};

struct snd_stru_mixer_hw {
	unsigned int caps;
};

struct snd_stru_mixer {
	snd_card_t *card;
	unsigned int device;		/* device # */
#ifdef SNDCFG_OSSEMUL
	int ossreg;			/* flag if device is registered for OSS */
#endif

	char id[32];
	unsigned char name[80];

	unsigned int channels_count;	/* channels count */
	unsigned int channels_visible;	/* channels count exclude hidden */
	snd_kmixer_channel_t *channels[SND_MIXER_CHANNELS];

	unsigned int switches_count;
	snd_kmixer_switch_t *switches[SND_MIXER_SWITCHES];

	int modify_counter;		/* for OSS emulation */

	void *private_data;
	void (*private_free) (void *private_data);
	unsigned int private_value;

	struct snd_stru_mixer_hw hw;	/* readonly variables */

	snd_kmixer_file_t *ffile;	/* first file */
	snd_mutex_define(ffile);
	snd_spin_define(lock);

	snd_info_entry_t *proc_entry;
};

extern void snd_mixer_set_kernel_mute(snd_kmixer_t * mixer,
				      unsigned int priority,
				      unsigned int type,
				      unsigned short mute);

extern snd_kmixer_t *snd_mixer_new(snd_card_t * card, char *id);
extern int snd_mixer_free(snd_kmixer_t * mixer);

extern snd_kmixer_channel_t *snd_mixer_new_channel(snd_kmixer_t * mixer,
					struct snd_stru_mixer_channel_hw *hw);
extern void snd_mixer_reorder_channel(snd_kmixer_t * mixer,
				      snd_kmixer_channel_t * channel);

extern snd_kmixer_switch_t *snd_mixer_new_switch(snd_kmixer_t * mixer,
					snd_kmixer_switch_t * kswitch);

extern int snd_mixer_register(snd_kmixer_t * mixer, int device);
extern int snd_mixer_unregister(snd_kmixer_t * mixer);

extern snd_kmixer_channel_t *snd_mixer_find_channel(snd_kmixer_t * mixer,
						    unsigned int priority);
extern void snd_mixer_hardware_volume(snd_kmixer_t *mixer,
				      unsigned int priority,
				      unsigned int flags, unsigned int type,
				      int left, int right, unsigned short mute);

#endif				/* __MIXER_H */
