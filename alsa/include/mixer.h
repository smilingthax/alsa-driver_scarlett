#ifndef __MIXER_H
#define __MIXER_H

/*
 *  Abstraction layer for MIXER
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

#define SND_MIXER_CMD_READ	0
#define SND_MIXER_CMD_WRITE	1
#define SND_MIXER_CMD_STORE	2
#define SND_MIXER_CMD_RESTORE	3
#define SND_MIXER_CMD_SIZE	4

typedef struct snd_stru_mixer_file snd_kmixer_file_t;

typedef int (snd_kmixer_element_info_t)(snd_kmixer_file_t * file, snd_mixer_element_info_t * info, void *private_data);
typedef int (snd_kmixer_element_control_t)(snd_kmixer_file_t * file, int cmd, void * data, int size, void *private_data);
typedef int (snd_kmixer_free_t)(void *private_data);

typedef struct snd_stru_mixer_element_new {
	char *name;			/* element name */
	int index;			/* extension to the element name */
	int type;			/* element type */
	snd_kmixer_element_info_t *info;
	snd_kmixer_element_control_t *control;
	void *private_data;
	snd_kmixer_free_t *private_free;
} snd_kmixer_element_new_t;

typedef struct snd_stru_mixer_element_route {
	int size;
	int count;
	struct snd_stru_mixer_element **routes;
} snd_kmixer_element_route_t;

typedef struct snd_stru_mixer_element {
	char *name;			/* element name */
	int index;			/* extension to the element name */
	int type;			/* element type */
	snd_kmixer_element_info_t *info;
	snd_kmixer_element_control_t *control;
	void *private_data;
	snd_kmixer_free_t *private_free;
	snd_kmixer_element_route_t routes_next;
	snd_kmixer_element_route_t routes_prev;
	struct snd_stru_mixer_element *next;
} snd_kmixer_element_t;

typedef struct snd_stru_mixer_group_new {
	char *name;			/* group name */
	int index;			/* extension to the group name */
} snd_kmixer_group_new_t;

typedef struct snd_stru_mixer_group {
	char *name;			/* group name */
	int index;			/* extension to the group name */
	int elements_size;
	int elements_count;
	snd_kmixer_element_t **elements;
	struct snd_stru_mixer_group *next;
} snd_kmixer_group_t;

typedef struct snd_stru_mixer_read {
	snd_mixer_read_t data;
	struct snd_stru_mixer_read *next;
} snd_kmixer_read_t;

struct snd_stru_mixer_file {
	snd_kmixer_t *mixer;
	snd_spin_define(change_lock);
	snd_sleep_define(change);
	snd_spin_define(read_lock);
	int read_active: 1,		/* read interface is activated */
	    rebuild: 1;			/* rebuild the mixer structure */
	int pool_size;
	snd_kmixer_read_t *pool;	/* atomic pool */
	snd_kmixer_read_t *first_item;
	snd_kmixer_read_t *last_item;
	struct snd_stru_mixer_file *next;
};

struct snd_stru_mixer {
	snd_card_t *card;
	int device;			/* device # */

	char id[32];
	unsigned char name[80];
	unsigned int attrib;

	snd_mutex_define(lock);
	int elements_count;		/* channels count */
	snd_kmixer_element_t *elements;	/* first element */
	int groups_count;		/* groups count */
	snd_kmixer_group_t *groups;	/* first group */
	snd_kswitch_list_t switches;
	snd_kmixer_file_t *ffile;	/* first file */
#ifdef SNDCFG_OSSEMUL
	int oss_change_count;
	int ossreg;
#endif

	void *private_data;
	void (*private_free) (void *private_data);

	snd_info_entry_t *proc_entry;
};

struct snd_stru_mixer_notify {
	int (*n_register) (unsigned short minor, snd_kmixer_t * mixer);
	int (*n_unregister) (unsigned short minor, snd_kmixer_t * mixer);
	struct snd_stru_mixer_notify *next;
};

extern snd_kmixer_t *snd_mixer_new(snd_card_t * card, char *id);
extern int snd_mixer_free(snd_kmixer_t * mixer);
extern int snd_mixer_register(snd_kmixer_t * mixer, int device);
extern int snd_mixer_unregister(snd_kmixer_t * mixer);
extern int snd_mixer_notify(struct snd_stru_mixer_notify * notify, int nfree);

extern int snd_mixer_lock(snd_kmixer_t * mixer, int up);

extern snd_kmixer_element_t *snd_mixer_element_find(snd_kmixer_t * mixer,
					char *name, int index, int type);
extern int snd_mixer_element_add(snd_kmixer_t * mixer,
					snd_kmixer_element_t * element);
extern int snd_mixer_element_remove(snd_kmixer_t * mixer,
					snd_kmixer_element_t * element);
extern int snd_mixer_element_rename(snd_kmixer_t * mixer,
					char *name, int index, int type,
					char *nname, int nindex);
extern snd_kmixer_element_t *snd_mixer_element_new(snd_kmixer_t * mixer,
					snd_kmixer_element_new_t * nelement);
extern int snd_mixer_element_change(snd_kmixer_t * mixer,
					snd_kmixer_element_t * element);
extern int snd_mixer_element_value_change(snd_kmixer_file_t * mfile,
					snd_kmixer_element_t * element,
					int atomic);
extern int snd_mixer_element_route_add(snd_kmixer_t * mixer,
					snd_kmixer_element_t * src_element,
					snd_kmixer_element_t * dst_element);
extern int snd_mixer_element_route_remove(snd_kmixer_t * mixer,
					  snd_kmixer_element_t * src_element,
					  snd_kmixer_element_t * dst_element);

extern snd_kmixer_group_t *snd_mixer_group_find(snd_kmixer_t * mixer,
					char *name, int index);
extern int snd_mixer_group_add(snd_kmixer_t * mixer,
					snd_kmixer_group_t * group);
extern int snd_mixer_group_remove(snd_kmixer_t * mixer,
					snd_kmixer_group_t * group);
extern int snd_mixer_group_rename(snd_kmixer_t * mixer,
					char *name, int index,
					char *nname, int nindex);
extern snd_kmixer_group_t *snd_mixer_group_new(snd_kmixer_t * mixer,
					snd_kmixer_group_new_t * ngroup);
extern int snd_mixer_group_change(snd_kmixer_t * mixer,
				  snd_kmixer_group_t * group);
extern int snd_mixer_group_element_add(snd_kmixer_t * mixer,
					snd_kmixer_group_t * group,
					snd_kmixer_element_t * element);
extern int snd_mixer_group_element_remove(snd_kmixer_t * mixer,
					snd_kmixer_group_t * group,
					snd_kmixer_element_t * element);

extern int snd_mixer_switch_add(snd_kmixer_t * mixer,
				snd_kswitch_t * kswitch);
extern int snd_mixer_switch_remove(snd_kmixer_t * mixer,
				   snd_kswitch_t * kswitch);
extern snd_kswitch_t *snd_mixer_switch_new(snd_kmixer_t * mixer,
					   snd_kswitch_t * kswitch,
					   void *private_data);
extern int snd_mixer_switch_change(snd_kmixer_t * mixer,
				   snd_kswitch_t * kswitch);

/*
 *  library
 */

static inline void snd_mixer_set_bit(unsigned int *bitmap, int bit, int val)
{
	if (val) {
		bitmap[bit >> 5] |= 1 << (bit & 31);
	} else {
		bitmap[bit >> 5] &= ~(1 << (bit & 31));
	}
}

static inline int snd_mixer_get_bit(unsigned int *bitmap, int bit)
{
	return (bitmap[bit >> 5] & (1 << (bit & 31))) ? 1 : 0;
}

extern snd_kmixer_group_t *snd_mixer_lib_group(snd_kmixer_t *mixer,
					       char *name,
					       int index);

struct snd_stru_mixer_lib_io {
	unsigned int attribute;
	int voices_count;
	snd_mixer_voice_t *voices;
};

extern snd_kmixer_element_t *snd_mixer_lib_io(snd_kmixer_t *mixer,
					      char *name,
					      int index,
					      int type,
					      unsigned int attribute,
					      int voices_count,
					      snd_mixer_voice_t *voices);
extern snd_kmixer_element_t *snd_mixer_lib_io_mono(snd_kmixer_t *mixer,
						   char *name,
						   int index,
						   int type,
						   unsigned int attribute);
extern snd_kmixer_element_t *snd_mixer_lib_io_stereo(snd_kmixer_t *mixer,
						     char *name,
						     int index,
						     int type,
						     unsigned int attribute);

struct snd_stru_mixer_lib_pcm {
	int devices_count;
	int *devices;
};

extern snd_kmixer_element_t *snd_mixer_lib_pcm(snd_kmixer_t *mixer,
					       char *name,
					       int index,
					       int type,
					       int devices_count,
					       int *devices);

struct snd_stru_mixer_lib_converter {
	unsigned int resolution;
};

extern snd_kmixer_element_t *snd_mixer_lib_converter(snd_kmixer_t *mixer,
						     char *name,
						     int index,
						     int type,
						     unsigned int resolution);

typedef int (snd_mixer_sw1_control_t)(int w_flag, unsigned int *bitmap, void *private_data);

struct snd_stru_mixer_lib_sw1 {
	int switches;				/* size in bits */
	snd_mixer_sw1_control_t *control;
	void *private_data;
	snd_kmixer_free_t *private_free;
};

extern snd_kmixer_element_t *snd_mixer_lib_sw1(snd_kmixer_t *mixer,
				char *name,
				int index,
				int switches,
				snd_mixer_sw1_control_t *control,
				void *private_data);

typedef int (snd_mixer_sw2_control_t)(int w_flag, int *value, void *private_data);

struct snd_stru_mixer_lib_sw2 {
        snd_mixer_sw2_control_t *control;
        void *private_data;
        snd_kmixer_free_t *private_free;
};

extern snd_kmixer_element_t *snd_mixer_lib_sw2(snd_kmixer_t *mixer,
				char *name,
				int index,
				snd_mixer_sw2_control_t *control,
				void *private_data);

typedef int (snd_mixer_sw3_control_t)(int w_flag, unsigned int *bitmap, void *private_data);

struct snd_stru_mixer_lib_sw3 {
	int type;
	int voices_count;
	snd_mixer_voice_t *voices;
	snd_mixer_sw1_control_t *control;
	void *private_data;
	snd_kmixer_free_t *private_free;
};

extern snd_kmixer_element_t *snd_mixer_lib_sw3(snd_kmixer_t *mixer,
				char *name,
				int index,
				int type,
				int voices_count,
				snd_mixer_voice_t *voices,
				snd_mixer_sw1_control_t *control,
				void *private_data);

typedef int (snd_mixer_volume1_control_t)(int w_flag, int *voices, void *private_data);

struct snd_stru_mixer_lib_volume1 {
	int voices;
	struct snd_mixer_element_volume1_range *ranges;
	snd_mixer_volume1_control_t *control;
	void *private_data;
	snd_kmixer_free_t *private_free;
};

extern snd_kmixer_element_t *snd_mixer_lib_volume1(snd_kmixer_t *mixer,
				char *name,
				int index,
				int voices,
				struct snd_mixer_element_volume1_range *range,
				snd_mixer_volume1_control_t *control,
				void *private_data);

struct snd_stru_mixer_lib_accu1 {
        int attenuation;
};

extern snd_kmixer_element_t *snd_mixer_lib_accu1(snd_kmixer_t *mixer,
						 char *name,
						 int index,
						 int attenuation);

struct snd_stru_mixer_lib_accu2 {
        int attenuation;
};

extern snd_kmixer_element_t *snd_mixer_lib_accu2(snd_kmixer_t *mixer,
						 char *name,
						 int index,
						 int attenuation);

typedef int (snd_mixer_accu3_control_t)(int w_flag, int *voices, void *private_data);

struct snd_stru_mixer_lib_accu3 {
	int voices;
	struct snd_mixer_element_accu3_range *ranges;
	snd_mixer_accu3_control_t *control;
	void *private_data;
	snd_kmixer_free_t *private_free;
};

extern snd_kmixer_element_t *snd_mixer_lib_accu3(snd_kmixer_t *mixer,
				char *name,
				int index,
				int voices,
				struct snd_mixer_element_accu3_range *range,
				snd_mixer_accu3_control_t *control,
				void *private_data);

typedef int (snd_mixer_mux1_control_t)(int w_flag, snd_kmixer_element_t **elements, void *private_data);

struct snd_stru_mixer_lib_mux1 {
	unsigned int attribute;
	int voices;
	snd_mixer_mux1_control_t *control;
	void *private_data;
	snd_kmixer_free_t *private_free;
};

extern snd_kmixer_element_t *snd_mixer_lib_mux1(snd_kmixer_t *mixer,
					char *name,
					int index,
					unsigned int attribute,
					int voices,
					snd_mixer_mux1_control_t *control,
					void *private_data);

typedef int (snd_mixer_mux2_control_t)(int w_flag, snd_kmixer_element_t **element, void *private_data);

struct snd_stru_mixer_lib_mux2 {
	unsigned int attribute;
	snd_mixer_mux2_control_t *control;
	void *private_data;
	snd_kmixer_free_t *private_free;
};

extern snd_kmixer_element_t *snd_mixer_lib_mux2(snd_kmixer_t *mixer,
					char *name,
					int index,
					unsigned int attribute,
					snd_mixer_mux2_control_t *control,
					void *private_data);

typedef int (snd_mixer_tone_control1_control_t)(int w_flag, struct snd_mixer_element_tone_control1 *tc1, void *private_data);

struct snd_stru_mixer_lib_tone_control1 {
	struct snd_mixer_element_tone_control1_info data;
	snd_mixer_tone_control1_control_t *control;
	void *private_data;
	snd_kmixer_free_t *private_free;
};

extern snd_kmixer_element_t *snd_mixer_lib_tone_control1(snd_kmixer_t *mixer,
					char *name,
					int index,
					struct snd_mixer_element_tone_control1_info *info,
					snd_mixer_tone_control1_control_t *control,
					void *private_data);

typedef int (snd_mixer_3d_effect1_control_t)(int w_flag, struct snd_mixer_element_3d_effect1 *effect1, void *private_data);

struct snd_stru_mixer_lib_3d_effect1 {
	struct snd_mixer_element_3d_effect1_info data;
	snd_mixer_3d_effect1_control_t *control;
	void *private_data;
	snd_kmixer_free_t *private_free;
};

extern snd_kmixer_element_t *snd_mixer_lib_3d_effect1(snd_kmixer_t *mixer,
					char *name,
					int index,
					struct snd_mixer_element_3d_effect1_info *info,
					snd_mixer_3d_effect1_control_t *control,
					void *private_data);

#endif				/* __MIXER_H */
