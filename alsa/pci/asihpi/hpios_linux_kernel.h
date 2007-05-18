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
#define HPI_OS_DEFINED
#define HPI_DEBUG_VERBOSE
#define HPI_KERNEL_MODE

#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif

#include <asm/io.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/version.h>
#include <linux/firmware.h>
#include <linux/interrupt.h>

#if ( LINUX_VERSION_CODE >= KERNEL_VERSION ( 2 , 5 , 0 ) )
#    include <linux/device.h>
#endif

#define INLINE inline

#if __GNUC__ >= 3
#define DEPRECATED __attribute__((deprecated))
#endif

#define __pack__ __attribute__ ((packed))

#ifndef __user
#define __user
#endif
#ifndef __iomem
#define __iomem
#endif

#define NO_HPIOS_FILE_OPS

#ifdef CONFIG_64BIT
#define HPI64BIT
#endif

/* //////////////////////////////////////////////////////////////////////// */

struct hpi_ioctl_linux {
	void __user *phm;
	void __user *phr;
};

#define HPI_IOCTL_LINUX _IOWR('H', 1, struct hpi_ioctl_linux)

// Cast to *HpiOs_LockedMem_Area in hpios_linux.c
typedef void *HpiOs_LockedMem_Handle;

#define HPIOS_DEBUG_PRINTF printk

#define HPI_DEBUG_FLAG_ERROR   KERN_ERR
#define HPI_DEBUG_FLAG_WARNING KERN_WARNING
#define HPI_DEBUG_FLAG_NOTICE  KERN_NOTICE
#define HPI_DEBUG_FLAG_INFO    KERN_INFO
#define HPI_DEBUG_FLAG_DEBUG   KERN_DEBUG
#define HPI_DEBUG_FLAG_VERBOSE KERN_DEBUG	/* kernel has no verbose */

/* I/O read/write.  */
#define HOUT8(a,d)       outb(a,d)
#define HINP8(a)         inb(a)
#define HOUTBUF8(p,a,l)  outsb(p,a,l)

/* Memory read/write */
#if ( LINUX_VERSION_CODE > KERNEL_VERSION ( 2 , 6 , 8 ) )

#define HPIOS_MEMWRITE32(a,d) iowrite32((d),(a))
#define HPIOS_MEMREAD32(a) ioread32((a))
#define HPIOS_MEMWRITEBLK32(from,to,nwords) iowrite32_rep(to,from,nwords)
#define HPIOS_MEMREADBLK32(from,to,nwords)  ioread32_rep(from,to,nwords)

#else

#define HPIOS_MEMWRITE32(a,d) writel((d),(a))
#define HPIOS_MEMREAD32(a) readl((a))
/* BLK versions replaced by loops in the code */
#endif

#include <linux/spinlock.h>

#define HPI_LOCKING

typedef struct {
	spinlock_t lock;
	int lock_context;
} HPIOS_SPINLOCK;

/* The reason for all this evilness is that ALSA calls some of a drivers
operators in atomic context, and some not.  But all our functions channel through
the HPI_Message conduit, so we can't handle the different context per function
*/

#define IN_LOCK_BH 1
#define IN_LOCK_IRQ 0
static inline void cond_lock(HPIOS_SPINLOCK * l)
{
	if (irqs_disabled()) {
// NO bh or isr can execute on this processor, so ordinary lock will do
		spin_lock(&((l)->lock));
		l->lock_context = IN_LOCK_IRQ;
	} else {
		spin_lock_bh(&((l)->lock));
		l->lock_context = IN_LOCK_BH;
	}
}

static inline void cond_unlock(HPIOS_SPINLOCK * l)
{
	if (l->lock_context == IN_LOCK_BH) {
		spin_unlock_bh(&((l)->lock));
	} else {
		spin_unlock(&((l)->lock));
	}
}

#define HpiOs_Msgxlock_Init( obj )      spin_lock_init( &(obj)->lock )
#define HpiOs_Msgxlock_Lock( obj, f )   cond_lock( obj )
#define HpiOs_Msgxlock_UnLock( obj, f ) cond_unlock( obj )

#define HpiOs_Dsplock_Init( obj )       spin_lock_init( &(obj)->dspLock.lock )
#define HpiOs_Dsplock_Lock( obj, f )    cond_lock( &(obj)->dspLock )
#define HpiOs_Dsplock_UnLock( obj, f )  cond_unlock( &(obj)->dspLock )

#define HPIOS_LOCK_FLAGS(name)

#ifdef CONFIG_SND_DEBUG
#define HPI_DEBUG
#endif

#define HPI_ALIST_LOCKING
#define HpiOs_Alistlock_Init( obj )     spin_lock_init( &((obj)->aListLock.lock) )
#define HpiOs_Alistlock_Lock(obj, f )   spin_lock( &((obj)->aListLock.lock) )
#define HpiOs_Alistlock_UnLock(obj, f ) spin_unlock( &((obj)->aListLock.lock) )

typedef struct {

/* Spinlock prevent contention for one card between user prog and alsa driver */
	spinlock_t spinlock;
	unsigned long flags;

/* Semaphores prevent contention for one card between multiple user programs (via ioctl) */
	struct semaphore sem;
	u16 wAdapterIndex;
	u16 type;

/* ALSA card structure */
	void *snd_card_asihpi;

	char *pBuffer;
	void __iomem *apRemappedMemBase[HPI_MAX_ADAPTER_MEM_SPACES];

} adapter_t;

///////////////////////////////////////////////////////////////////////////
