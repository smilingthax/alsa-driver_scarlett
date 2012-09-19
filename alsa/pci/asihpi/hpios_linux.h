/******************************************************************************

    AudioScience HPI driver
    Copyright (C) 1997-2003  AudioScience Inc. <support@audioscience.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of version 2 of the GNU General Public License as
    published by the Free Software Foundation;

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

HPI Operating System Specific macros for Linux

(C) Copyright AudioScience Inc. 1997-2003
******************************************************************************/

#ifdef ALSA_BUILD
#define HPI_INCLUDE_4100
#define HPI_INCLUDE_4300
#define HPI_INCLUDE_4500
#define HPI_INCLUDE_5000
#define HPI_INCLUDE_6000
#define HPI_INCLUDE_6400
#define HPI_INCLUDE_8700
#define USE_SPINLOCK 1
#define HPI_DEBUG 1
#endif

#include <linux/ioctl.h>

#define _test

#if defined ( _test )
/*  was stripped by preprocessing scripts */
#else

#define

#endif

struct hpi_ioctl_linux {
	HPI_MESSAGE *phm;
	HPI_RESPONSE *phr;
};
#define HPI_IOCTL_LINUX _IOWR('H', 1, struct hpi_ioctl_linux)
#define HPI_IOCTL_HARDRESET _IO('H', 2)

typedef int HpiOs_FILE;
/* return value from fopen */
#define NO_FILE -1

//#define NO_HPIOS_LOCKEDMEM_OPS
// Needs to be something that is valid for both user and kernel compilation
// Cast to *HpiOs_LockedMem_Area in hpios_linux.c
typedef void *HpiOs_LockedMem_Handle;

/****************** KERNEL ***************/
#ifdef __KERNEL__

#include <asm/io.h>

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#endif

#define HPI_DEBUG_STRING printk
/* I/O read/write.  */
#define HOUT8(a,d)       outb(a,d)
#define HINP8(a)         inb(a)
#define HOUTBUF8(p,a,l) outsb(p,a,l)

/* Memory read/write */
#define HPIOS_MEMWRITE32(a,d)       writel((d),(u32 *)(a))
#define HPIOS_MEMREAD32(a)          ((volatile u32)readl((u32 *)(a)))
#define HPIOS_MEMWRITEBLK32(f,t,n) HpiOs_MemWriteBlk32(f,t,n)
/* based on linux/string.h */
static inline void HpiOs_MemWriteBlk32(const u32 * from, u32 * to, size_t n)
{
	int d0, d1, d2;

	__asm__ __volatile__("rep \n\t movsl":"=&c"(d0), "=&D"(d1), "=&S"(d2)
			     :"0"(n), "1"((long)to), "2"((long)from)
			     :"memory");
}

/****************** USER ***************/
#else

#include <stdio.h>
#define HPI_DEBUG_STRING printf
#endif

///////////////////////////////////////////////////////////////////////////
