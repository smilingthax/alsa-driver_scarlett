/******************************************************************************
Copyright (C) 1997-2003 AudioScience, Inc. All rights reserved.

This software is provided 'as-is', without any express or implied warranty.
In no event will AudioScience Inc. be held liable for any damages arising
from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This copyright notice and list of conditions may not be altered or removed 
   from any source distribution.

AudioScience, Inc. <support@audioscience.com>

( This license is GPL compatible see http://www.gnu.org/licenses/license-list.html#GPLCompatibleLicenses )

HPI Operating System Specific macros for Linux

(C) Copyright AudioScience Inc. 1997-2003
******************************************************************************/

#include <linux/ioctl.h>

struct hpi_ioctl_linux
{
  HPI_MESSAGE *phm;
  HPI_RESPONSE *phr;
};
#define HPI_IOCTL_LINUX _IOWR('H', 1, struct hpi_ioctl_linux)

typedef int	HpiOs_FILE;
/* return value from fopen */
#define NO_FILE -1

//#define NO_HPIOS_LOCKEDMEM_OPS
// Needs to be something that is valid for both user and kernel compilation
// Cast to *HpiOs_LockedMem_Area in hpios_linux.c
typedef void *  HpiOs_LockedMem_Handle;


/****************** KERNEL ***************/
#include <asm/io.h>

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#endif

/* I/O read/write.  */
#define HOUT8(a,d)       outb(a,d)
#define HINP8(a)         inb(a)
#define HOUTBUF8(p,a,l) outsb(p,a,l)

/* Memory read/write */
#define HPIOS_MEMWRITE32(a,d)       writel((d),(HW32 *)(a))
#define HPIOS_MEMREAD32(a)          ((volatile HW32)readl((HW32 *)(a)))
#define HPIOS_MEMWRITEBLK32(f,t,n) HpiOs_MemWriteBlk32(f,t,n)
#define HpiOs_MemWriteBlk32(from,to,n) memcpy_fromio(to,from,(n)*4)

///////////////////////////////////////////////////////////////////////////

#define HPI_INCLUDE_4100
#define HPI_INCLUDE_4300
#define HPI_INCLUDE_4500
#define HPI_INCLUDE_5000
#define HPI_INCLUDE_6000
#define HPI_INCLUDE_6400
#define HPI_INCLUDE_8700
