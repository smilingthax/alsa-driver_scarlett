/*
 *  Various wrappers
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

#ifdef ALSA_BUILD
#include "config.h"
#endif

#include <linux/version.h>
#include <linux/config.h>
#ifdef ALSA_BUILD
#if defined(CONFIG_MODVERSIONS) && !defined(__GENKSYMS__) && !defined(__DEPEND__)
#define MODVERSIONS
#include <linux/modversions.h>
#include "sndversions.h"
#endif
#endif
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/vmalloc.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 3, 0)
void snd_wrapper_request_region(unsigned long from, unsigned long extent, const char *name)
{
	return request_region(from, extent, name);
}
#endif

#ifdef CONFIG_SND_DEBUG_MEMORY
void *snd_wrapper_kmalloc(size_t size, int flags)
{
	return kmalloc(size, flags);
}

void snd_wrapper_kfree(const void *obj)
{
	kfree(obj);
}

void *snd_wrapper_vmalloc(unsigned long size)
{
	return vmalloc(size);
}

void snd_wrapper_vfree(void *obj)
{
	vfree(obj);
}
#endif
