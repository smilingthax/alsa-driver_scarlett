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
	DRAIN = 1,
	FLUSH = 2
} snd_pcm_plugin_action_t;

#define snd_pcm_plugin_extra_data(plugin) (((char *)plugin) + sizeof(*plugin))

struct snd_stru_pcm_plugin {
	char *name;			/* plugin name */
	long (*transfer)(snd_pcm_plugin_t *plugin,
			 char *src_ptr, long src_size,
			 char *dst_ptr, long dst_size);
	long (*src_size)(snd_pcm_plugin_t *plugin, long dst_size);
	long (*dst_size)(snd_pcm_plugin_t *plugin, long src_size);
	int (*action)(snd_pcm_plugin_t *plugin, snd_pcm_plugin_action_t action);
	snd_pcm_plugin_t *prev;
	snd_pcm_plugin_t *next;
	void *private_data;
	void (*private_free)(snd_pcm_plugin_t *plugin, void *private_data);
};

extern snd_pcm_plugin_t *snd_pcm_plugin_build(const char *name, int extra);
extern int snd_pcm_plugin_free(snd_pcm_plugin_t *plugin);
extern int snd_pcm_plugin_clear(snd_pcm_plugin_t **first);

extern int snd_pcm_plugin_build_interleave(int src_interleave, int dst_interleave, int format, snd_pcm_plugin_t **r_plugin);
extern int snd_pcm_plugin_build_linear(int src_format, int dst_format, snd_pcm_plugin_t **r_plugin);
extern int snd_pcm_plugin_build_mulaw(int src_format, int dst_format, snd_pcm_plugin_t **r_plugin);

#endif				/* __PCM_PLUGIN_H */
