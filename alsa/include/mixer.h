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

typedef struct snd_stru_mixer_element snd_kmixer_element_t;
typedef struct snd_stru_mixer_group snd_kmixer_group_t;
typedef struct snd_stru_mixer_file snd_kmixer_file_t;

#define snd_mixer_ext_element_private_data(element)	(void *)(((char *)element) + sizeof(snd_kmixer_element_t))
#define snd_mixer_ext_group_private_data(group)		(void *)(((char *)group) + sizeof(snd_kmixer_group_t))

typedef int (snd_kmixer_element_info_t)(snd_kmixer_element_t *element, snd_kmixer_file_t * file, snd_mixer_element_info_t * info);
typedef int (snd_kmixer_element_control_t)(snd_kmixer_element_t *element, snd_kmixer_file_t * file, int w_flag, snd_mixer_element_t * uelement);
typedef int (snd_kmixer_group_control_t)(snd_kmixer_group_t *group, snd_kmixer_file_t * file, int w_flag, snd_mixer_group_t * ugroup);
typedef void (snd_kmixer_free_t)(void *private_data);

typedef struct snd_stru_mixer_element_new {
	char *name;			/* element name */
	int index;			/* extension to the element name */
	int type;			/* element type */
	int input_channels;
	int output_channels;
	int ext_size;			/* requested size of the extended area */
	void *ext_ptr;			/* pointer to the extended area */
	snd_kmixer_element_info_t *info;
	snd_kmixer_element_control_t *control;
	unsigned long private_value;
	void *private_data;
	snd_kmixer_free_t *private_free;
} snd_kmixer_element_new_t;

typedef struct snd_stru_mixer_route_info {
	int share_count;
	unsigned int *wires;
} snd_kmixer_route_info_t;

typedef struct snd_stru_mixer_route {
	snd_kmixer_route_info_t *info;
	snd_kmixer_element_t *route;
} snd_kmixer_route_t;

typedef struct snd_stru_mixer_element_route {
	int size;
	int count;
	snd_kmixer_route_t *routes;
} snd_kmixer_element_route_t;

struct snd_stru_mixer_element {
	char *name;			/* element name */
	int index;			/* extension to the element name */
	int type;			/* element type */
	int ext_size;			/* extension data size */
	int input_channels;
	int output_channels;
	snd_kmixer_element_info_t *info;
	snd_kmixer_element_control_t *control;
	unsigned long private_value;
	void *private_data;
	snd_kmixer_free_t *private_free;
	snd_kmixer_element_route_t routes_next;
	snd_kmixer_element_route_t routes_prev;
	int groups_size;
	int groups_count;
	snd_kmixer_group_t **groups;
	snd_kmixer_element_t *next;
};

typedef struct snd_stru_mixer_group_new {
	char *name;			/* group name */
	int index;			/* extension to the group name */
	int ext_size;			/* requested size of the extended area */
	void *ext_ptr;			/* pointer to the extended area */	
} snd_kmixer_group_new_t;

struct snd_stru_mixer_group {
	char *name;			/* group name */
	int index;			/* extension to the group name */
	int oss_dev;			/* OSS device */
	int ext_size;			/* extension data size */
	int elements_size;
	int elements_count;
	snd_kmixer_element_t **elements;
	snd_kmixer_group_control_t *control;
	unsigned long private_value;
	void *private_data;
	snd_kmixer_free_t *private_free;
	struct snd_stru_mixer_group *next;
};

typedef struct snd_stru_mixer_read {
	snd_mixer_read_t data;
	struct snd_stru_mixer_read *next;
} snd_kmixer_read_t;

struct snd_stru_mixer_file {
	int use;
	snd_kmixer_t *mixer;
	wait_queue_head_t change_sleep;
	spinlock_t read_lock;
	int read_active: 1,		/* read interface is activated */
	    rebuild: 1;			/* rebuild the mixer structure */
	snd_kmixer_read_t *first_item;
	snd_kmixer_read_t *last_item;
	snd_kmixer_group_t *ignore_group;
	unsigned int read_filter[8];
	struct snd_stru_mixer_file *next;
};

struct snd_stru_mixer {
	snd_card_t *card;
	int device;			/* device # */

	char id[64];
	unsigned char name[80];
	unsigned int attrib;

	struct semaphore lock;
	int elements_count;		/* channels count */
	snd_kmixer_element_t *elements;	/* first element */
	int groups_count;		/* groups count */
	snd_kmixer_group_t *groups;	/* first group */
	snd_kswitch_list_t switches;
	snd_kmixer_file_t *ffile;	/* first file */
	spinlock_t ffile_lock;
#ifdef CONFIG_SND_OSSEMUL
	int oss_change_count;
	int ossreg;
	int oss_recsrc;
	int oss_volume_levels[32][2];
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

extern int snd_mixer_new(snd_card_t * card, char *id, int device, snd_kmixer_t **rmixer);
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
extern int snd_mixer_element_value_change_all(snd_kmixer_t * mixer,
					snd_kmixer_element_t * element,
					int atomic);
extern int snd_mixer_element_value_change_all_file(snd_kmixer_file_t * mfile,
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
					char *nname, int nindex, int oss_dev);
extern snd_kmixer_group_t *snd_mixer_group_new(snd_kmixer_t * mixer,
					snd_kmixer_group_new_t * ngroup);
extern int snd_mixer_group_change(snd_kmixer_t * mixer,
				  snd_kmixer_group_t * group);
extern int snd_mixer_group_value_change(snd_kmixer_file_t * mfile,
					snd_kmixer_group_t * group,
					int atomic);
extern int snd_mixer_group_value_change_all(snd_kmixer_t * mixer,
					snd_kmixer_group_t * group,
					int atomic);
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

extern snd_kmixer_group_t *
	snd_mixer_lib_group(snd_kmixer_t *mixer, char *name, int index);

extern snd_kmixer_group_t *
	snd_mixer_lib_group_ctrl(snd_kmixer_t *mixer,
				 char *name,
				 int index,
				 int oss_dev,
				 snd_kmixer_group_control_t *control,
				 void *private_data);

struct snd_stru_mixer_lib_io {
	unsigned int attrib;
	int channels_count;
	snd_mixer_channel_t *channels;
};

extern snd_kmixer_element_t *
	snd_mixer_lib_io(snd_kmixer_t *mixer,
			 char *name,
			 int index,
			 int type,
			 unsigned int attrib,
			 int channels_count,
			 snd_mixer_channel_t *channels);
extern snd_kmixer_element_t *
	snd_mixer_lib_io_mono(snd_kmixer_t *mixer,
			      char *name,
			      int index,
			      int type,
			      unsigned int attrib);
extern snd_kmixer_element_t *
	snd_mixer_lib_io_stereo(snd_kmixer_t *mixer,
				char *name,
				int index,
				int type,
				unsigned int attrib);

struct snd_stru_mixer_lib_pcm1 {
	int devices_count;
	int *devices;
};

extern snd_kmixer_element_t *snd_mixer_lib_pcm1(snd_kmixer_t *mixer,
						char *name,
						int index,
						int type,
						int devices_count,
						int *devices);

struct snd_stru_mixer_lib_pcm2 {
	int device;
	int subdevice;
};

extern snd_kmixer_element_t *snd_mixer_lib_pcm2(snd_kmixer_t *mixer,
						char *name,
						int index,
						int type,
						int device,
						int subdevice);

struct snd_stru_mixer_lib_pcm3 {
	int device;
	int subdevice;
	int channel;
};

extern snd_kmixer_element_t *snd_mixer_lib_pcm3(snd_kmixer_t *mixer,
						char *name,
						int index,
						int type,
						int device,
						int subdevice,
						int channel);

struct snd_stru_mixer_lib_converter {
	unsigned int resolution;
};

extern snd_kmixer_element_t *
	snd_mixer_lib_converter(snd_kmixer_t *mixer,
				char *name,
				int index,
				int type,
				unsigned int resolution);

typedef int (snd_mixer_sw1_control_t)(snd_kmixer_element_t *element, int w_flag, unsigned int *bitmap);

struct snd_stru_mixer_lib_sw1 {
	int switches;				/* size in bits */
	snd_mixer_sw1_control_t *control;
};

extern snd_kmixer_element_t *
	snd_mixer_lib_sw1(snd_kmixer_t *mixer,
			  char *name,
			  int index,
			  int switches,
			  snd_mixer_sw1_control_t *control,
			  void *private_data);
extern snd_kmixer_element_t *
	snd_mixer_lib_sw1_value(snd_kmixer_t *mixer,
				char *name,
				int index,
				int switches,
				snd_mixer_sw1_control_t *control,
				void *private_data,
				unsigned long private_value);

typedef int (snd_mixer_sw2_control_t)(snd_kmixer_element_t *element, int w_flag, int *value);

struct snd_stru_mixer_lib_sw2 {
        snd_mixer_sw2_control_t *control;
};

extern snd_kmixer_element_t *
	snd_mixer_lib_sw2(snd_kmixer_t *mixer,
			  char *name,
			  int index,
			  snd_mixer_sw2_control_t *control,
			  void *private_data);
extern snd_kmixer_element_t *
	snd_mixer_lib_sw2_value(snd_kmixer_t *mixer,
				char *name,
				int index,
				snd_mixer_sw2_control_t *control,
				void *private_data,
				unsigned long private_value);

typedef int (snd_mixer_sw3_control_t)(snd_kmixer_element_t *element, int w_flag, unsigned int *bitmap);

struct snd_stru_mixer_lib_sw3 {
	int type;
	snd_mixer_sw3_control_t *control;
};

extern snd_kmixer_element_t *
	snd_mixer_lib_sw3(snd_kmixer_t *mixer,
			  char *name,
			  int index,
			  int type,
			  int channels_count,
			  snd_mixer_sw1_control_t *control,
			  void *private_data);
extern snd_kmixer_element_t *
	snd_mixer_lib_sw3_value(snd_kmixer_t *mixer,
				char *name,
				int index,
				int type,
				int channels_count,
				snd_mixer_sw1_control_t *control,
				void *private_data,
				unsigned long private_value);

typedef int (snd_mixer_volume1_control_t)(snd_kmixer_element_t *element, int w_flag, int *channels);

struct snd_stru_mixer_lib_volume1 {
	int channels;
	struct snd_mixer_element_volume1_range *ranges;
	snd_mixer_volume1_control_t *control;
};

extern snd_kmixer_element_t *
	snd_mixer_lib_volume1(snd_kmixer_t *mixer,
			      char *name,
			      int index,
			      int channels,
			      struct snd_mixer_element_volume1_range *range,
			      snd_mixer_volume1_control_t *control,
			      void *private_data);
extern snd_kmixer_element_t *
	snd_mixer_lib_volume1_value(snd_kmixer_t *mixer,
				    char *name,
				    int index,
				    int channels,
				    struct snd_mixer_element_volume1_range *range,
				    snd_mixer_volume1_control_t *control,
				    void *private_data,
				    unsigned long private_value);

struct snd_stru_mixer_lib_accu1 {
        int attenuation;
};

extern snd_kmixer_element_t *
	snd_mixer_lib_accu1(snd_kmixer_t *mixer,
			    char *name,
			    int index,
			    int attenuation);

struct snd_stru_mixer_lib_accu2 {
        int attenuation;
};

extern snd_kmixer_element_t *
	snd_mixer_lib_accu2(snd_kmixer_t *mixer,
			    char *name,
			    int index,
			    int attenuation);

typedef int (snd_mixer_accu3_control_t)(snd_kmixer_element_t *element, int w_flag, int *channels);

struct snd_stru_mixer_lib_accu3 {
	int channels;
	struct snd_mixer_element_accu3_range *ranges;
	snd_mixer_accu3_control_t *control;
	void *private_data;
	snd_kmixer_free_t *private_free;
};

extern snd_kmixer_element_t *
	snd_mixer_lib_accu3(snd_kmixer_t *mixer,
			    char *name,
			    int index,
			    int channels,
			    struct snd_mixer_element_accu3_range *range,
			    snd_mixer_accu3_control_t *control,
			    void *private_data);

typedef int (snd_mixer_mux1_control_t)(snd_kmixer_element_t *element, int w_flag, snd_kmixer_element_t **elements);

struct snd_stru_mixer_lib_mux1 {
	unsigned int attrib;
	int channels;
	snd_mixer_mux1_control_t *control;
};

extern snd_kmixer_element_t *
	snd_mixer_lib_mux1(snd_kmixer_t *mixer,
			   char *name,
			   int index,
			   unsigned int attrib,
			   int channels,
			   snd_mixer_mux1_control_t *control,
			   void *private_data);

typedef int (snd_mixer_mux2_control_t)(snd_kmixer_element_t *element, int w_flag, snd_kmixer_element_t **melement);

struct snd_stru_mixer_lib_mux2 {
	unsigned int attrib;
	snd_mixer_mux2_control_t *control;
};

extern snd_kmixer_element_t *
	snd_mixer_lib_mux2(snd_kmixer_t *mixer,
			   char *name,
			   int index,
			   unsigned int attrib,
			   snd_mixer_mux2_control_t *control,
			   void *private_data);

typedef int (snd_mixer_tone_control1_control_t)(snd_kmixer_element_t *element, int w_flag, struct snd_mixer_element_tone_control1 *tc1);

struct snd_stru_mixer_lib_tone_control1 {
	struct snd_mixer_element_tone_control1_info data;
	snd_mixer_tone_control1_control_t *control;
};

extern snd_kmixer_element_t *
	snd_mixer_lib_tone_control1(snd_kmixer_t *mixer,
				    char *name,
				    int index,
				    struct snd_mixer_element_tone_control1_info *info,
				    snd_mixer_tone_control1_control_t *control,
				    void *private_data);

typedef int (snd_mixer_pan_control1_control_t)(snd_kmixer_element_t *element, int w_flag, int *pan);

struct snd_stru_mixer_lib_pan_control1 {
	int pan;
	struct snd_mixer_element_pan_control1_range *ranges;
	snd_mixer_pan_control1_control_t *control;
};

extern snd_kmixer_element_t *
	snd_mixer_lib_pan_control1(snd_kmixer_t *mixer,
				   char *name,
				   int index,
				   int pan,
				   struct snd_mixer_element_pan_control1_range *ranges,
				   snd_mixer_pan_control1_control_t *control,
				   void *private_data);

typedef int (snd_mixer_3d_effect1_control_t)(snd_kmixer_element_t *element, int w_flag, struct snd_mixer_element_3d_effect1 *effect1);

struct snd_stru_mixer_lib_3d_effect1 {
	struct snd_mixer_element_3d_effect1_info data;
	snd_mixer_3d_effect1_control_t *control;
};

extern snd_kmixer_element_t *
	snd_mixer_lib_3d_effect1(snd_kmixer_t *mixer,
				 char *name,
				 int index,
				 struct snd_mixer_element_3d_effect1_info *info,
				 snd_mixer_3d_effect1_control_t *control,
				 void *private_data);

#endif				/* __MIXER_H */
