/*
 *   32bit -> 64bit ioctl helpers
 *   Copyright (c) by Takashi Iwai <tiwai@suse.de>
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
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *
 * This file registers the converters from 32-bit ioctls to 64-bit ones.
 * The converter assumes that a 32-bit user-pointer can be casted by A(x)
 * macro to a valid 64-bit pointer which is accessible via copy_from/to_user.
 *
 */

#ifndef __ALSA_IOCTL32_H
#define __ALSA_IOCTL32_H

#if defined(CONFIG_X86_64)
#ifdef CONFIG_IA32_EMULATION
#define A(__x) ((unsigned long)(__x))
#define HAVE_IOCTL32
#endif

#elif defined(CONFIG_PPC64)
#include <asm/ppc32.h>
#define HAVE_IOCTL32

#elif defined(CONFIG_SPARC64)
#define A(__x) ((unsigned long)(__x))
#define HAVE_IOCTL32

/* mip64 has still no register/unreigster interface */
//#elif defined(CONFIG_MIPS32_COMPAT)
//#define A(__x) ((unsigned long)(__x))
//#define HAVE_IOCTL32

#else
// for test
#define A(__x) ((unsigned long)(__x))
#define HAVE_IOCTL32

#endif


/*
 */

#ifdef HAVE_IOCTL32

#define TO_PTR(x)  A(x)

#define COPY(x)  (dst->x = src->x)
#define CPTR(x)	 (dst->x = (typeof(dst->x))A(src->x))

#define convert_from_32(type, dstp, srcp)\
{\
	struct sndrv_##type *dst = dstp;\
	struct sndrv_##type##32 *src = srcp;\
	CVT_##sndrv_##type();\
}

#define convert_to_32(type, dstp, srcp)\
{\
	struct sndrv_##type *src = srcp;\
	struct sndrv_##type##32 *dst = dstp;\
	CVT_##sndrv_##type();\
}


#define DEFINE_ALSA_IOCTL(type) \
static int snd_ioctl32_##type(unsigned int fd, unsigned int cmd, unsigned long arg, struct file *file)\
{\
	struct sndrv_##type##32 data32;\
	struct sndrv_##type data;\
	int err;\
	if (copy_from_user(&data32, (void*)arg, sizeof(data32)))\
		return -EFAULT;\
	memset(&data, 0, sizeof(data));\
	convert_from_32(type, &data, &data32);\
	err = file->f_op->ioctl(file->f_dentry->d_inode, file, cmd, (unsigned long)&data);\
	if (err < 0)\
		return err;\
	if (cmd & (_IOC_READ << _IOC_DIRSHIFT)) {\
		convert_to_32(type, &data32, &data);\
		if (copy_to_user((void*)arg, &data32, sizeof(data32)))\
			return -EFAULT;\
	}\
	return err;\
}

struct ioctl32_mapper {
	unsigned int cmd;
	int (*handler)(unsigned int, unsigned int, unsigned long, struct file * filp);
	int registered;
};

int snd_ioctl32_register(struct ioctl32_mapper *mappers);
void snd_ioctl32_unregister(struct ioctl32_mapper *mappers);
int snd_ioctl32_init(void);
void snd_ioctl32_done(void);

int snd_pcm_ioctl32_init(void);
void snd_pcm_ioctl32_done(void);

int snd_rawmidi_ioctl32_init(void);
void snd_rawmidi_ioctl32_done(void);

int snd_timer_ioctl32_init(void);
void snd_timer_ioctl32_done(void);

#else /* !HAVE_IOCTL32 */

#define snd_ioctl32_init()
#define snd_ioctl32_done()

#define snd_pcm_ioctl32_init()
#define snd_pcm_ioctl32_done()

#define snd_rawmidi_ioctl32_init()
#define snd_rawmidi_ioctl32_done()

#define snd_timer_ioctl32_init()
#define snd_timer_ioctl32_done()

#endif /* HAVE_IOCTL32 */

#endif /* __ALSA_IOCTL32_H */
