#ifndef __MISC_H
#define __MISC_H

/*
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>
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

extern inline void snd_put_byte(unsigned char *array, unsigned int idx, unsigned char b)
{
	*(array + idx) = b;
}

extern inline unsigned char snd_get_byte(unsigned char *array, unsigned int idx)
{
	return *(array + idx);
}

#if defined( __i386__ )

extern inline void snd_put_word(unsigned char *array, unsigned int idx, unsigned short w)
{
	*(unsigned short *) (array + idx) = w;
}

extern inline unsigned short snd_get_word(unsigned char *array, unsigned int idx)
{
	return *(unsigned short *) (array + idx);
}

extern inline void snd_put_dword(unsigned char *array, unsigned int idx, unsigned int dw)
{
	*(unsigned int *) (array + idx) = dw;
}

extern inline unsigned int snd_get_dword(unsigned char *array, unsigned int idx)
{
	return *(unsigned int *) (array + idx);
}

#else

#ifdef SND_LITTLE_ENDIAN

extern inline void snd_put_word(unsigned char *array, unsigned int idx, unsigned short w)
{
	*(array + idx + 0) = (unsigned char) (w >> 0);
	*(array + idx + 1) = (unsigned char) (w >> 8);
}

extern inline unsigned short snd_get_word(unsigned char *array, unsigned int idx)
{
	return (*(array + idx + 0) << 0) |
	    (*(array + idx + 1) << 8);
}

extern inline void snd_put_dword(unsigned char *array, unsigned int idx, unsigned int dw)
{
	*(array + idx + 0) = (unsigned char) (dw >> 0);
	*(array + idx + 1) = (unsigned char) (dw >> 8);
	*(array + idx + 2) = (unsigned char) (dw >> 16);
	*(array + idx + 3) = (unsigned char) (dw >> 24);
}

extern inline unsigned int snd_get_dword(unsigned char *array, unsigned int idx)
{
	return (*(array + idx + 0) << 0) |
	    (*(array + idx + 1) << 8) |
	    (*(array + idx + 2) << 16) |
	    (*(array + idx + 3) << 24);
}

#else

extern inline void snd_put_word(unsigned char *array, unsigned int idx, unsigned short w)
{
	*(array + idx + 0) = (unsigned char) (w >> 8);
	*(array + idx + 1) = (unsigned char) (w >> 0);
}

extern inline unsigned short snd_get_word(unsigned char *array, unsigned int idx)
{
	return (*(array + idx + 0) << 8) |
	    (*(array + idx + 1) << 0);
}

extern inline void snd_put_dword(unsigned char *array, unsigned int idx, unsigned int dw)
{
	*(array + idx + 0) = (unsigned char) (dw >> 24);
	*(array + idx + 1) = (unsigned char) (dw >> 16);
	*(array + idx + 2) = (unsigned char) (dw >> 8);
	*(array + idx + 3) = (unsigned char) (dw >> 0);
}

extern inline unsigned int snd_get_dword(unsigned char *array, unsigned int idx)
{
	return (*(array + idx + 0) << 24) |
	    (*(array + idx + 1) << 16) |
	    (*(array + idx + 2) << 8) |
	    (*(array + idx + 3) << 0);
}

#endif

#endif

#endif				/* __MISC_H */
