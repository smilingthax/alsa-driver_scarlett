#ifndef __SNDMAGIC_H
#define __SNDMAGIC_H

/*
 *  Magic allocation, deallocation, check
 *  Copyright (c) 2000 by Abramo Bagnara <abramo@alsa-project.org>
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


#ifdef CONFIG_SND_DEBUG
void *_snd_magic_kcalloc(size_t size, int flags, int magic);
void *_snd_magic_kmalloc(size_t size, int flags, int magic);
void _snd_magic_kfree(void *ptr);

#define snd_magic_kcalloc(type, extra, flags) _snd_magic_kcalloc(sizeof(type) + extra, flags, type##_magic)
#define snd_magic_kmalloc(type, extra, flags) _snd_magic_kmalloc(sizeof(type) + extra, flags, type##_magic)

static inline int _snd_magic_value(void *obj)
{
	return obj == NULL ? 0 : *(((int *)obj) - 1);
}

static inline int _snd_magic_bad(void *obj, int magic)
{
	return _snd_magic_value(obj) != magic;
}

#define snd_magic_cast(type, ptr, retval) ({\
	void *__ptr = ptr;\
	if (__ptr == NULL) {\
		snd_printk("NULL: %s: %i [%s]\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);\
		return retval;\
	}\
	if (_snd_magic_bad(__ptr, type##_magic)) {\
		snd_printk("MAGIC: %s: %i [%s] {0x%x}\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, _snd_magic_value(__ptr));\
		return retval;\
	}\
	__ptr;\
})

#define snd_magic_kfree _snd_magic_kfree

#define snd_pcm_t_magic				0xa15a0101
#define snd_pcm_file_t_magic			0xa15a0102
#define snd_pcm_subchn_t_magic			0xa15a0103
#define snd_pcm_proc_private_t_magic		0xa15a0104
#define snd_pcm_oss_file_t_magic		0xa15a0105

#define snd_info_private_data_t_magic		0xa15a0201
#define snd_control_t_magic			0xa15a0301
#define snd_rawmidi_t_magic			0xa15a0401
#define snd_timer_t_magic			0xa15a0501
#define snd_timer_user_t_magic			0xa15a0502
#define snd_hwdep_t_magic			0xa15a0601
#define snd_kmixer_t_magic			0xa15a0701
#define snd_kmixer_file_t_magic			0xa15a0702


#define snd_card_share_pcm_t_magic		0xa15a1001
#define snd_card_share_pcm_open_t_magic		0xa15a1002
#define snd_card_share_mixer_group_t_magic	0xa15a1003
#define snd_card_share_mixer_element_t_magic	0xa15a1004

#define es18xx_t_magic				0xa15a1101

#define trident_t_magic				0xa15a1201
#define snd_trident_voice_t_magic		0xa15a1202
#define es1938_t_magic				0xa15a1301
#define cs461x_t_magic				0xa15a1401
#define ensoniq_t_magic				0xa15a1501
#define sonicvibes_t_magic			0xa15a1601
#define mpu401_t_magic				0xa15a1701
#define fm801_t_magic				0xa15a1801
#define ac97_t_magic				0xa15a1901
#define ak4531_t_magic				0xa15a1a01
#define snd_uart16550_t_magic			0xa15a1b01

#else
#define snd_magic_kcalloc(type, extra, flags) snd_kcalloc(sizeof(type) + extra, flags, type##_magic)
#define snd_magic_kmalloc(type, extra, flags) snd_kmalloc(sizeof(type) + extra, flags, type##_magic)
#define snd_magic_cast(type, ptr, retval) ptr
#define snd_magic_kfree snd_kfree
#endif

#endif

