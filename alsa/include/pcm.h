#ifndef __PCM_H
#define __PCM_H

/*
 *  Digital Audio (PCM) abstract layer
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

#define SND_PCM_DEVICES		4

#define SND_PCM_TYPE_1		1

#define SND_PCM_DEFAULT_RATE	8000

typedef struct snd_stru_pcm_file snd_pcm_file_t;
typedef struct snd_stru_pcm_subchn snd_pcm_subchn_t;
typedef struct snd_stru_pcm_channel snd_pcm_channel_t;

struct snd_stru_pcm_file {
	snd_pcm_t *pcm;
	snd_pcm_subchn_t * playback;
	snd_pcm_subchn_t * capture;
	struct snd_stru_pcm_file * next;
	/* -- private section */
	void *private_data;
	void (*private_free)(void *private_data);
};

struct snd_stru_pcm_subchn {
	snd_pcm_t *pcm;
	snd_pcm_channel_t *pchn;
	int number;
	int capture;			/* capture flag */
#ifdef CONFIG_SND_OSSEMUL
	int oss;			/* open-sound-system flag */
#endif
	/* -- timer section -- */
	snd_timer_t *timer;		/* timer */ 
	unsigned int timer_resolution;	/* timer resolution */
	int timer_running;		/* time is running */
	spinlock_t timer_lock;
	/* -- /proc interface -- */
	void *proc_entry;
	void *proc_private;
	struct semaphore proc;
	void *proc_entry1;		/* for second midlevel code */
	/* -- next subchannel -- */
	struct snd_stru_pcm_subchn *next;
	struct snd_stru_pcm_file *file;
	/* -- private section -- */
	void *private_data;
	void (*private_free)(void *private_data);
};

struct snd_stru_pcm_channel {
	snd_pcm_t *pcm;
	snd_kswitch_list_t switches;
	/* -- subchannels -- */
	unsigned int subchn_count;
	unsigned int subchn_opened;
	snd_pcm_subchn_t *subchn;
	/* -- private section -- */
	void *private_data;
	void (*private_free)(void *private_data);
};

struct snd_stru_pcm {
	snd_card_t *card;
	unsigned int device;	/* device number */
#ifdef CONFIG_SND_OSSEMUL
	int ossreg;
#endif
	unsigned int info_flags;
	int type;		/* PCM type */
	char id[32];
	char name[80];
	snd_pcm_channel_t playback;
	snd_pcm_channel_t capture;
	snd_pcm_file_t *files;
	snd_minor_t *reg;
	snd_info_entry_t *dev;
#ifdef CONFIG_SND_OSSEMUL
	snd_info_entry_t *proc_oss_entry;
#endif
	struct semaphore open_mutex;
	int open_prefer_subchn;
	void *private_data;
	void (*private_free) (void *private_data);
};

struct snd_stru_pcm_notify {
	int (*n_register) (unsigned short minor, snd_pcm_t * pcm);
	int (*n_unregister) (unsigned short minor, snd_pcm_t * pcm);
	struct snd_stru_pcm_notify *next;
};

/*
 *  Registering
 */

extern snd_pcm_t *snd_pcm_devices[];

extern void snd_pcm_lock(int unlock);

extern snd_pcm_t *snd_pcm_new_device(snd_card_t * card,
				     char *id,
				     snd_minor_t * reg,
				     int playback_count,
				     int capture_count);
extern int snd_pcm_free(snd_pcm_t * pcm);
extern int snd_pcm_register(snd_pcm_t * pcm, int pcm_device);
extern int snd_pcm_unregister(snd_pcm_t * pcm);

extern int snd_pcm_notify(struct snd_stru_pcm_notify *notify, int nfree);

extern int snd_pcm_open(snd_pcm_t * pcm,
			int playback, int capture,
			snd_pcm_file_t ** pcm_file);
extern int snd_pcm_release(snd_pcm_file_t *pcm_file);
extern int snd_pcm_ioctl(snd_pcm_t * pcm, unsigned int cmd, unsigned long arg);

extern int snd_pcm_switch_add(snd_pcm_channel_t * pchn, snd_kswitch_t * ksw);
extern int snd_pcm_switch_remove(snd_pcm_channel_t * pchn, snd_kswitch_t * ksw);
extern snd_kswitch_t *snd_pcm_switch_new(snd_pcm_channel_t * pchn, snd_kswitch_t * ksw, void *private_data);
extern int snd_pcm_switch_change(snd_pcm_channel_t * pchn, snd_kswitch_t * ksw);

/*
 *  /proc interface
 */

extern void snd_pcm_proc_init(snd_pcm_t * pcm);
extern void snd_pcm_proc_done(snd_pcm_t * pcm);
extern void snd_pcm_proc_format(snd_pcm_subchn_t * subchn,
				snd_pcm_format_t * format);
extern void snd_pcm_proc_write(snd_pcm_subchn_t * subchn,
			       const void *buffer, unsigned int count);

#endif				/* __PCM_H */
