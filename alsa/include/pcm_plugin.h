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

#define calloc(n, size) snd_kcalloc(n * size, GFP_KERNEL)
#define free(ptr) snd_kfree(ptr)
#define strdup(str) snd_kmalloc_strdup(str, GFP_KERNEL)

#define bswap_16(x) swab16((x))
#define bswap_32(x) swab32((x))
#define bswap_64(x) swab64((x))

typedef unsigned int bitset_t;

static inline size_t bitset_size(int nbits)
{
	return (nbits + sizeof(bitset_t) * 8 - 1) / (sizeof(bitset_t) * 8);
}

static inline bitset_t *bitset_alloc(int nbits)
{
	return calloc(bitset_size(nbits), sizeof(bitset_t));
}
	
static inline void bitset_set(bitset_t *bitmap, unsigned int pos)
{
	int bits = sizeof(*bitmap) * 8;
	bitmap[pos / bits] |= 1 << (pos % bits);
}

static inline void bitset_reset(bitset_t *bitmap, unsigned int pos)
{
	int bits = sizeof(*bitmap) * 8;
	bitmap[pos / bits] &= ~(1 << (pos % bits));
}

static inline int bitset_get(bitset_t *bitmap, unsigned int pos)
{
	int bits = sizeof(*bitmap) * 8;
	return !!(bitmap[pos / bits] & (1 << (pos % bits)));
}

static inline void bitset_copy(bitset_t *dst, bitset_t *src, unsigned int nbits)
{
	memcpy(dst, src, bitset_size(nbits) * sizeof(bitset_t));
}

static inline void bitset_and(bitset_t *dst, bitset_t *bs, unsigned int nbits)
{
	bitset_t *end = dst + bitset_size(nbits);
	while (dst < end)
		*dst++ &= *bs++;
}

static inline void bitset_or(bitset_t *dst, bitset_t *bs, unsigned int nbits)
{
	bitset_t *end = dst + bitset_size(nbits);
	while (dst < end)
		*dst++ |= *bs++;
}

static inline void bitset_zero(bitset_t *dst, unsigned int nbits)
{
	bitset_t *end = dst + bitset_size(nbits);
	while (dst < end)
		*dst++ = 0;
}

static inline void bitset_one(bitset_t *dst, unsigned int nbits)
{
	bitset_t *end = dst + bitset_size(nbits);
	while (dst < end)
		*dst++ = -1;
}

typedef struct snd_stru_pcm_plugin snd_pcm_plugin_t;
#define snd_pcm_plugin_handle_t snd_pcm_subchn_t

typedef enum {
	INIT = 0,
	PREPARE = 1,
	DRAIN = 2,
	FLUSH = 3,
	PAUSE = 4,
} snd_pcm_plugin_action_t;

typedef struct snd_stru_pcm_plugin_voice {
	void *aptr;			/* pointer to the allocated area */
	snd_pcm_voice_area_t area;
	unsigned int enabled:1;		/* voice need to be processed */
	unsigned int wanted:1;		/* voice is wanted */
} snd_pcm_plugin_voice_t;

struct snd_stru_pcm_plugin {
	char *name;			/* plug-in name */
	int channel;
	snd_pcm_format_t src_format;	/* source format */
	snd_pcm_format_t dst_format;	/* destination format */
	int src_width;			/* sample width in bits */
	int dst_width;			/* sample width in bits */
	ssize_t (*src_samples)(snd_pcm_plugin_t *plugin, size_t dst_samples);
	ssize_t (*dst_samples)(snd_pcm_plugin_t *plugin, size_t src_samples);
	int (*client_voices)(snd_pcm_plugin_t *plugin,
			     size_t samples,
			     snd_pcm_plugin_voice_t **voices);
	int (*src_voices_mask)(snd_pcm_plugin_t *plugin,
			       bitset_t *dst_vmask,
			       bitset_t **src_vmask);
	int (*dst_voices_mask)(snd_pcm_plugin_t *plugin,
			       bitset_t *src_vmask,
			       bitset_t **dst_vmask);
	ssize_t (*transfer)(snd_pcm_plugin_t *plugin,
			    const snd_pcm_plugin_voice_t *src_voices,
			    snd_pcm_plugin_voice_t *dst_voices,
			    size_t samples);
	int (*action)(snd_pcm_plugin_t *plugin,
		      snd_pcm_plugin_action_t action,
		      unsigned long data);
	int (*parameter_set)(snd_pcm_plugin_t *plugin,
			     const char *name,
			     unsigned long value);
	int (*parameter_get)(snd_pcm_plugin_t *plugin,
			     const char *name,
			     unsigned long *value);
	snd_pcm_plugin_t *prev;
	snd_pcm_plugin_t *next;
	snd_pcm_plugin_handle_t *handle;
	void *private_data;
	void (*private_free)(snd_pcm_plugin_t *plugin, void *private_data);
	snd_pcm_plugin_voice_t *src_voices;
	snd_pcm_plugin_voice_t *dst_voices;
	bitset_t *src_vmask;
	bitset_t *dst_vmask;
	char extra_data[0];
};

int snd_pcm_plugin_build(snd_pcm_plugin_handle_t *handle,
                         int channel,
                         const char *name,
                         snd_pcm_format_t *src_format,
                         snd_pcm_format_t *dst_format,
                         int extra,
                         snd_pcm_plugin_t **ret);
int snd_pcm_plugin_free(snd_pcm_plugin_t *plugin);
int snd_pcm_plugin_clear(snd_pcm_plugin_t **first);
ssize_t snd_pcm_plugin_src_samples_to_size(snd_pcm_plugin_t *plugin, size_t samples);
ssize_t snd_pcm_plugin_dst_samples_to_size(snd_pcm_plugin_t *plugin, size_t samples);
ssize_t snd_pcm_plugin_src_size_to_samples(snd_pcm_plugin_t *plugin, size_t size);
ssize_t snd_pcm_plugin_dst_size_to_samples(snd_pcm_plugin_t *plugin, size_t size);
ssize_t snd_pcm_plug_client_samples(snd_pcm_plugin_handle_t *handle, int channel, size_t drv_samples);
ssize_t snd_pcm_plug_slave_samples(snd_pcm_plugin_handle_t *handle, int channel, size_t clt_samples);
ssize_t snd_pcm_plug_client_size(snd_pcm_plugin_handle_t *handle, int channel, size_t drv_size);
ssize_t snd_pcm_plug_slave_size(snd_pcm_plugin_handle_t *handle, int channel, size_t clt_size);

int snd_pcm_plugin_build_block(snd_pcm_plugin_handle_t *handle,
			       int channel,
			       snd_pcm_plugin_handle_t *slave,
			       snd_pcm_format_t *format,
			       snd_pcm_plugin_t **r_plugin);
int snd_pcm_plugin_build_linear(snd_pcm_plugin_handle_t *handle,
				int channel,
				snd_pcm_format_t *src_format,
				snd_pcm_format_t *dst_format,
				snd_pcm_plugin_t **r_plugin);
int snd_pcm_plugin_build_mulaw(snd_pcm_plugin_handle_t *handle,
			       int channel,
			       snd_pcm_format_t *src_format,
			       snd_pcm_format_t *dst_format,
			       snd_pcm_plugin_t **r_plugin);
int snd_pcm_plugin_build_rate(snd_pcm_plugin_handle_t *handle,
			      int channel,
			      snd_pcm_format_t *src_format,
			      snd_pcm_format_t *dst_format,
			      snd_pcm_plugin_t **r_plugin);
int snd_pcm_plugin_build_route(snd_pcm_plugin_handle_t *handle,
			       int channel,
			       snd_pcm_format_t *src_format,
			       snd_pcm_format_t *dst_format,
			       int *ttable,
		               snd_pcm_plugin_t **r_plugin);
int snd_pcm_plugin_build_copy(snd_pcm_plugin_handle_t *handle,
			      int channel,
			      snd_pcm_format_t *format,
			      snd_pcm_plugin_t **r_plugin);

unsigned int snd_pcm_plug_formats(unsigned int formats);

int snd_pcm_plug_format(snd_pcm_plugin_handle_t *subchn,
			snd_pcm_channel_params_t *params,
			snd_pcm_channel_params_t *slave_params);

int snd_pcm_plug_slave_params(snd_pcm_channel_params_t *params,
			      snd_pcm_channel_info_t *slave_info,
			      snd_pcm_channel_params_t *slave_params);

int snd_pcm_plugin_append(snd_pcm_plugin_t *plugin);

ssize_t snd_pcm_plug_write_transfer(snd_pcm_plugin_handle_t *handle, snd_pcm_plugin_voice_t *src_voices, size_t size);
ssize_t snd_pcm_plug_read_transfer(snd_pcm_plugin_handle_t *handle, snd_pcm_plugin_voice_t *dst_voices_final, size_t size);

ssize_t snd_pcm_plug_client_voices_iovec(snd_pcm_plugin_handle_t *handle, int channel,
				     const struct iovec *vector, unsigned long count,
				     snd_pcm_plugin_voice_t **voices);
ssize_t snd_pcm_plug_client_voices_buf(snd_pcm_plugin_handle_t *handle, int channel,
				   char *buf, size_t count,
				   snd_pcm_plugin_voice_t **voices);

int snd_pcm_plugin_client_voices(snd_pcm_plugin_t *plugin,
                                 size_t samples,
                                 snd_pcm_plugin_voice_t **voices);

int snd_pcm_area_silence(const snd_pcm_voice_area_t *dst_voice, size_t dst_offset,
			 size_t samples, int format);
int snd_pcm_areas_silence(const snd_pcm_voice_area_t *dst_voices, size_t dst_offset,
			  size_t vcount, size_t samples, int format);
int snd_pcm_area_copy(const snd_pcm_voice_area_t *src_voice, size_t src_offset,
		      const snd_pcm_voice_area_t *dst_voice, size_t dst_offset,
		      size_t samples, int format);
int snd_pcm_areas_copy(const snd_pcm_voice_area_t *src_voices, size_t src_offset,
		       const snd_pcm_voice_area_t *dst_voices, size_t dst_offset,
		       size_t vcount, size_t samples, int format);

void *snd_pcm_plug_buf_alloc(snd_pcm_plugin_handle_t *pcm, int channel, size_t size);
void snd_pcm_plug_buf_unlock(snd_pcm_plugin_handle_t *pcm, int channel, void *ptr);
ssize_t snd_pcm_oss_write3(snd_pcm_subchn_t *subchn, const char *ptr, size_t size, int in_kernel);
ssize_t snd_pcm_oss_read3(snd_pcm_subchn_t *subchn, char *ptr, size_t size, int in_kernel);
ssize_t snd_pcm_oss_writev3(snd_pcm_subchn_t *subchn, const struct iovec *vector, int count, int in_kernel);
ssize_t snd_pcm_oss_readv3(snd_pcm_subchn_t *subchn, const struct iovec *vector, int count, int in_kernel);



#define ROUTE_PLUGIN_RESOLUTION 16

int getput_index(int format);
int copy_index(int format);
int conv_index(int src_format, int dst_format);

void zero_voice(snd_pcm_plugin_t *plugin,
		const snd_pcm_plugin_voice_t *dst_voice,
		size_t samples);

#define UNUSED __attribute__ ((unused))

#ifdef PLUGIN_DEBUG
#define pdprintf( args... ) printk( "plugin: " ##args)
#else
#define pdprintf( args... ) { ; }
#endif

#endif				/* __PCM_PLUGIN_H */
