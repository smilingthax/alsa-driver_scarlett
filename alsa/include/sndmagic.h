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
void *_snd_magic_kcalloc(unsigned long magic, size_t size, int flags);
void *_snd_magic_kmalloc(unsigned long magic, size_t size, int flags);
void _snd_magic_kfree(void *ptr);

#define snd_magic_kcalloc(type, extra, flags) (type *) _snd_magic_kcalloc(type##_magic, sizeof(type) + extra, flags)
#define snd_magic_kmalloc(type, extra, flags) (type *) _snd_magic_kmalloc(type##_magic, sizeof(type) + extra, flags)

static inline unsigned long _snd_magic_value(void *obj)
{
	return obj == NULL ? -1 : *(((unsigned long *)obj) - 1);
}

static inline int _snd_magic_bad(void *obj, unsigned long magic)
{
	return _snd_magic_value(obj) != magic;
}

#define snd_magic_cast1(t, expr, cmd) snd_magic_cast(t, expr, cmd)

#ifdef NEW_MACRO_VARARGS
#define snd_magic_cast(type, ptr, ...) (type *) ({\
	void *__ptr = ptr;\
	unsigned long __magic = _snd_magic_value(__ptr);\
	if (__magic != type##_magic) {\
		snd_printk("bad MAGIC (0x%lx)\n", __magic);\
		__VA_ARGS__;\
	}\
	__ptr;\
})
#else
#define snd_magic_cast(type, ptr, action...) (type *) ({\
	void *__ptr = ptr;\
	unsigned long __magic = _snd_magic_value(__ptr);\
	if (__magic != type##_magic) {\
		snd_printk("bad MAGIC (0x%lx)\n", __magic);\
		##action;\
	}\
	__ptr;\
})
#endif

#define snd_magic_kfree _snd_magic_kfree

#define snd_device_t_magic			0xa15a00ff
#define snd_pcm_t_magic				0xa15a0101
#define snd_pcm_file_t_magic			0xa15a0102
#define snd_pcm_substream_t_magic		0xa15a0103
#define snd_pcm_proc_private_t_magic		0xa15a0104
#define snd_pcm_oss_file_t_magic		0xa15a0105
#define snd_mixer_oss_t_magic			0xa15a0106

#define snd_info_private_data_t_magic		0xa15a0201
#define snd_kctl_t_magic			0xa15a0301
#define snd_kcontrol_t_magic			0xa15a0302
#define snd_rawmidi_t_magic			0xa15a0401
#define snd_rawmidi_file_t_magic		0xa15a0402
#define snd_virmidi_t_magic			0xa15a0403
#define snd_virmidi_dev_t_magic			0xa15a0404
#define snd_timer_t_magic			0xa15a0501
#define snd_timer_user_t_magic			0xa15a0502
#define snd_hwdep_t_magic			0xa15a0601
#define snd_seq_device_t_magic			0xa15a0701


#define snd_card_share_pcm_t_magic		0xa15a1001
#define snd_card_share_slave_t_magic		0xa15a1002
#define snd_card_share_mixer_group_t_magic	0xa15a1003
#define snd_card_share_mixer_element_t_magic	0xa15a1004

#define es18xx_t_magic				0xa15a1101
#define trident_t_magic				0xa15a1201
#define es1938_t_magic				0xa15a1301
#define cs461x_t_magic				0xa15a1401
#define ensoniq_t_magic				0xa15a1501
#define sonicvibes_t_magic			0xa15a1601
#define mpu401_t_magic				0xa15a1701
#define fm801_t_magic				0xa15a1801
#define ac97_t_magic				0xa15a1901
#define ak4531_t_magic				0xa15a1a01
#define snd_uart16550_t_magic			0xa15a1b01
#define emu10k1_t_magic				0xa15a1c01
#define emu10k1_pcm_t_magic			0xa15a1c02
#define snd_gus_card_t_magic			0xa15a1d01
#define gus_pcm_private_t_magic			0xa15a1d02
#define gus_proc_private_t_magic		0xa15a1d03
#define tea6330t_t_magic			0xa15a1e01
#define ad1848_t_magic				0xa15a1f01
#define cs4231_t_magic				0xa15a2001
#define es1688_t_magic				0xa15a2101
#define opti93x_t_magic				0xa15a2201
#define emu8000_t_magic				0xa15a2301
#define emu8000_proc_private_t_magic		0xa15a2302
#define snd_emux_t_magic			0xa15a2303
#define snd_emux_port_t_magic			0xa15a2304
#define sb_t_magic				0xa15a2401
#define snd_sb_csp_t_magic			0xa15a2402
#define snd_card_dummy_t_magic			0xa15a2501
#define snd_card_dummy_pcm_t_magic		0xa15a2502
#define opl3_t_magic				0xa15a2601
#define snd_seq_dummy_port_t_magic		0xa15a2701
#define ice1712_t_magic				0xa15a2801
#define ad1816a_t_magic				0xa15a2901
#define intel8x0_t_magic			0xa15a2a01
#define es1968_t_magic				0xa15a2b01
#define esschanp_t_magic			0xa15a2b02
#define esschanc_t_magic			0xa15a2b03
#define esmdma_t_magic				0xa15a2b04
#define via686a_t_magic				0xa15a2c01
#define pdplus_t_magic				0xa15a2d01
#define cmipci_t_magic				0xa15a2e01
#define ymfpci_t_magic				0xa15a2f01
#define ymfpci_pcm_t_magic			0xa15a2f02
#define cs4281_t_magic				0xa15a3001
#define snd_i2c_bus_t_magic			0xa15a3101

#else
#define snd_magic_kcalloc(type, extra, flags) (type *) snd_kcalloc(sizeof(type) + extra, flags)
#define snd_magic_kmalloc(type, extra, flags) (type *) snd_kmalloc(sizeof(type) + extra, flags)
#define snd_magic_cast(type, ptr, retval) (type *) ptr
#define snd_magic_cast1(type, ptr, retval) snd_magic_cast(type, ptr, retval)
#define snd_magic_kfree snd_kfree
#define _snd_magic_kfree _snd_kfree
#endif

#endif

