#ifndef __ULAW_H
#define __ULAW_H

/*
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

extern unsigned char snd_ulaw_dsp[];
extern unsigned char snd_ulaw_dsp_loud[];
extern unsigned char snd_dsp_ulaw[];
extern unsigned char snd_dsp_ulaw_loud[];

extern int snd_translate_to_user(unsigned char *table,
				 unsigned char *dest,
				 unsigned char *src,
				 unsigned int count);

extern int snd_translate_from_user(unsigned char *table,
				   unsigned char *dest,
				   unsigned char *src,
				   unsigned int count);

#endif				/* __ULAW_H */
