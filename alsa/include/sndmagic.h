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

static inline int _snd_magic_bad(void *obj, int magic)
{
	return *(((int *)obj) - 1) != magic;
}

#define snd_magic_cast(type, ptr, retval) ({\
	void *__ptr = ptr;\
	if (__ptr == NULL) {\
		snd_printk("NULL: %s: %i [%s]\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);\
		return retval;\
	}\
	if (_snd_magic_bad(__ptr, type##_magic)) {\
		snd_printk("MAGIC: %s: %i [%s]\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);\
		return retval;\
	}\
	__ptr;\
})

#define snd_magic_kfree _snd_magic_kfree

#define snd_pcm_t_magic				0x1234ab02
#define snd_pcm_file_t_magic			0x1234ab03
#define snd_pcm_subchn_t_magic			0x1234ab04
#define snd_pcm_proc_private_t_magic		0x1234ab05
#define snd_pcm_oss_file_t_magic		0x1235ab03

#define snd_info_private_data_t_magic		0x1236ab02
#define snd_control_t_magic			0x1237ab02
#define snd_rawmidi_t_magic			0x1238ab02
#define snd_timer_t_magic			0x1239ab02
#define snd_timer_user_t_magic			0x1239ab03
#define snd_hwdep_t_magic			0x123aab02
#define snd_kmixer_t_magic			0x123bab02
#define snd_kmixer_file_t_magic			0x123bab03


#define snd_card_share_pcm_t_magic		0xa3120202
#define snd_card_share_pcm_open_t_magic		0xa3120203
#define snd_card_share_mixer_group_t_magic	0xa3120204
#define snd_card_share_mixer_element_t_magic	0xa3120205

#define es18xx_t_magic				0xabcd1202

#else
#define snd_magic_kcalloc(type, extra, flags) snd_kcalloc(sizeof(type) + extra, flags, type##_magic)
#define snd_magic_kmalloc(type, extra, flags) snd_kmalloc(sizeof(type) + extra, flags, type##_magic)
#define snd_magic_cast(type, ptr, retval) ptr
#define snd_magic_kfree snd_kfree
#endif

#endif

