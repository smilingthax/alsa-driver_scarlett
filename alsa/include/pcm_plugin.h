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

#define bswap_16(x) __swab16((x))
#define bswap_32(x) __swab32((x))

typedef struct snd_stru_pcm_plugin snd_pcm_plugin_t;
#define snd_pcm_plugin_handle_t snd_pcm_runtime_t

typedef enum {
	INIT = 0,
	PREPARE = 1,
	DRAIN = 2,
	FLUSH = 3
} snd_pcm_plugin_action_t;

typedef struct snd_stru_pcm_plugin_voice {
	void *aptr;			/* pointer to the allocated area */
	void *addr;			/* address to voice samples */
	unsigned int first;		/* offset to first voice in bits */
	unsigned int step;		/* offset to next voice in bits */
} snd_pcm_plugin_voice_t;

struct snd_stru_pcm_plugin {
	char *name;			/* plugin name */
	snd_pcm_format_t src_format;	/* source format */
	snd_pcm_format_t dst_format;	/* destination format */
	int src_width;			/* sample width in bits */
	int dst_width;			/* sample width in bits */                
	ssize_t (*src_samples)(snd_pcm_plugin_t *plugin, size_t dst_samples);
	ssize_t (*dst_samples)(snd_pcm_plugin_t *plugin, size_t src_samples);
#if 0
	int (*src_voices)(snd_pcm_plugin_t *plugin,
			  snd_pcm_plugin_voice_t **voices,
			  size_t samples,
			  void *(*plugin_alloc)(snd_pcm_plugin_handle_t *handle, size_t size));
	int (*dst_voices)(snd_pcm_plugin_t *plugin,
			  snd_pcm_plugin_voice_t **voices,
			  size_t samples,
			  void *(*plugin_alloc)(snd_pcm_plugin_handle_t *handle, size_t size));
#endif
	ssize_t (*transfer)(snd_pcm_plugin_t *plugin,
			    const snd_pcm_plugin_voice_t *src_voices,
			    const snd_pcm_plugin_voice_t *dst_voices,
			    size_t samples);
	int (*action)(snd_pcm_plugin_t *plugin,
		      snd_pcm_plugin_action_t action,
		      unsigned long data);
	snd_pcm_plugin_t *prev;
	snd_pcm_plugin_t *next;
	snd_pcm_plugin_handle_t *handle;
	void *private_data;
	void (*private_free)(snd_pcm_plugin_t *plugin, void *private_data);
	snd_pcm_plugin_voice_t *voices;
	void *extra_data;
};

snd_pcm_plugin_t *snd_pcm_plugin_build(snd_pcm_plugin_handle_t *handle,
				       const char *name,
				       snd_pcm_format_t *src_format,
				       snd_pcm_format_t *dst_format,
				       int extra);
int snd_pcm_plugin_free(snd_pcm_plugin_t *plugin);
int snd_pcm_plugin_clear(snd_pcm_plugin_t **first);
ssize_t snd_pcm_plugin_src_samples_to_size(snd_pcm_plugin_t *plugin, size_t samples);
ssize_t snd_pcm_plugin_dst_samples_to_size(snd_pcm_plugin_t *plugin, size_t samples);
ssize_t snd_pcm_plugin_src_size_to_samples(snd_pcm_plugin_t *plugin, size_t size);
ssize_t snd_pcm_plugin_dst_size_to_samples(snd_pcm_plugin_t *plugin, size_t size);
ssize_t snd_pcm_plugin_client_samples(snd_pcm_plugin_handle_t *handle, int channel, size_t drv_samples);
ssize_t snd_pcm_plugin_hardware_samples(snd_pcm_plugin_handle_t *handle, int channel, size_t clt_samples);
ssize_t snd_pcm_plugin_client_size(snd_pcm_plugin_handle_t *handle, int channel, size_t drv_size);
ssize_t snd_pcm_plugin_hardware_size(snd_pcm_plugin_handle_t *handle, int channel, size_t clt_size);

int snd_pcm_plugin_build_linear(snd_pcm_plugin_handle_t *handle,
				snd_pcm_format_t *src_format,
				snd_pcm_format_t *dst_format,
				snd_pcm_plugin_t **r_plugin);
int snd_pcm_plugin_build_mulaw(snd_pcm_plugin_handle_t *handle,
			       snd_pcm_format_t *src_format,
			       snd_pcm_format_t *dst_format,
			       snd_pcm_plugin_t **r_plugin);
int snd_pcm_plugin_build_rate(snd_pcm_plugin_handle_t *handle,
			      snd_pcm_format_t *src_format,
			      snd_pcm_format_t *dst_format,
			      snd_pcm_plugin_t **r_plugin);
int snd_pcm_plugin_build_route(snd_pcm_plugin_handle_t *handle,
			       snd_pcm_format_t *src_format,
			       snd_pcm_format_t *dst_format,
			       int *ttable,
		               snd_pcm_plugin_t **r_plugin);

unsigned int snd_pcm_plugin_formats(unsigned int formats);

int snd_pcm_plugin_format(snd_pcm_runtime_t *runtime,
			  snd_pcm_channel_params_t *params,
			  snd_pcm_channel_params_t *hwparams,
			  snd_pcm_channel_info_t *hwinfo);

int snd_pcm_plugin_hwparams(snd_pcm_channel_params_t *params,
			    snd_pcm_channel_info_t *hwinfo,
			    snd_pcm_channel_params_t *hwparams);

int snd_pcm_oss_plugin_append(snd_pcm_runtime_t *runtime, snd_pcm_plugin_t *plugin);

#define ROUTE_PLUGIN_RESOLUTION 16

int getput_index(int format);
int copy_index(int src_format, int dst_format);

void zero_voice(snd_pcm_plugin_t *plugin,
		const snd_pcm_plugin_voice_t *dst_voice,
		size_t samples);

#ifdef PLUGIN_DEBUG
#define pdprintf( args... ) printk( "plugin: " ##args)
#else
#define pdprintf( args... ) { ; }
#endif

#endif				/* __PCM_PLUGIN_H */
