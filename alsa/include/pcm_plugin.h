#ifndef __PCM_PLUGIN_H
#define __PCM_PLUGIN_H

/*
 *  Digital Audio (Plugin interface) abstract layer
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

typedef struct snd_stru_pcm_plugin snd_pcm_plugin_t;

typedef enum {
	INIT = 0,
	PREPARE = 1,
	DRAIN = 2,
	FLUSH = 3
} snd_pcm_plugin_action_t;

#define snd_pcm_plugin_extra_data(plugin) (((char *)plugin) + sizeof(*plugin))

struct snd_stru_pcm_plugin {
	char *name;			/* plugin name */
	ssize_t (*transfer)(snd_pcm_plugin_t *plugin,
			    char *src_ptr, size_t src_size,
			    char *dst_ptr, size_t dst_size);
	ssize_t (*src_size)(snd_pcm_plugin_t *plugin, size_t dst_size);
	ssize_t (*dst_size)(snd_pcm_plugin_t *plugin, size_t src_size);
	int (*action)(snd_pcm_plugin_t *plugin,
		      snd_pcm_plugin_action_t action,
		      unsigned long data);
	snd_pcm_plugin_t *prev;
	snd_pcm_plugin_t *next;
	void *private_data;
	void (*private_free)(snd_pcm_plugin_t *plugin, void *private_data);
};

extern snd_pcm_plugin_t *snd_pcm_plugin_build(const char *name, int extra);
extern int snd_pcm_plugin_free(snd_pcm_plugin_t *plugin);
extern int snd_pcm_plugin_clear(snd_pcm_plugin_t **first);

int snd_pcm_plugin_build_interleave(snd_pcm_format_t *src_format,
				    snd_pcm_format_t *dst_format,
				    snd_pcm_plugin_t **r_plugin);
int snd_pcm_plugin_build_linear(snd_pcm_format_t *src_format,
				snd_pcm_format_t *dst_format,
				snd_pcm_plugin_t **r_plugin);
int snd_pcm_plugin_build_mulaw(snd_pcm_format_t *src_format,
			       snd_pcm_format_t *dst_format,
			       snd_pcm_plugin_t **r_plugin);
int snd_pcm_plugin_build_alaw(snd_pcm_format_t *src_format,
			      snd_pcm_format_t *dst_format,
			      snd_pcm_plugin_t **r_plugin);
int snd_pcm_plugin_build_adpcm(snd_pcm_format_t *src_format,
			       snd_pcm_format_t *dst_format,
			       snd_pcm_plugin_t **r_plugin);
int snd_pcm_plugin_build_rate(snd_pcm_format_t *src_format,
			      snd_pcm_format_t *dst_format,
			      snd_pcm_plugin_t **r_plugin);
int snd_pcm_plugin_build_voices(snd_pcm_format_t *src_format,
				snd_pcm_format_t *dst_format,
				snd_pcm_plugin_t **r_plugin);

unsigned int snd_pcm_plugin_formats(unsigned int formats);
int snd_pcm_plugin_hwparams(snd_pcm_channel_params_t *params,
			    snd_pcm_channel_info_t *hwinfo,
			    snd_pcm_channel_params_t *hwparams);

#ifdef PLUGIN_DEBUG
#define pdprintf( args... ) printk( "plugin: " ##args)
#else
#define pdprintf( args... ) { ; }
#endif

#endif				/* __PCM_PLUGIN_H */
