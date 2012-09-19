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

#ifdef ALSA_BUILD
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
#define irqs_disabled()							\
({									\
	unsigned long flags;						\
	__asm__ __volatile__("pushfl ; popl %0" : "=g"(flags) : );	\
	!(flags & (1 << 9));						\
})
#endif
#endif

//Use the kernel firmware loader
#define DSPCODE_FIRMWARE 1

/* //////////////////////////////////////////////////////////////////////// */

struct hpi_ioctl_linux {
	void __user *phm;
	void __user *phr;
};

#define HPI_IOCTL_LINUX _IOWR('H', 1, struct hpi_ioctl_linux)

typedef int HpiOs_FILE;
/* return value from fopen */
#define NO_FILE -1

// Needs to be something that is valid for both user and kernel compilation
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
#define HOUTBUF8(p,a,l) outsb(p,a,l)

/* Memory read/write */
/*? Casting to (void __iomem *) here may hide problems??? */
#define HPIOS_MEMWRITE32(a,d)       writel((d),(void __iomem *)(a))
#define HPIOS_MEMREAD32(a)          ((volatile u32)readl((void __iomem *)a))
#define HPIOS_MEMWRITEBLK32(f,t,n) HpiOs_MemWriteBlk32(f,t,n)
/* based on linux/string.h */
static inline void HpiOs_MemWriteBlk32(const u32 * from, u32 * to, size_t n)
{
	int d0, d1, d2;

	__asm__ __volatile__("rep \n\t movsl":"=&c"(d0), "=&D"(d1), "=&S"(d2)
			     :"0"(n), "1"((long)to), "2"((long)from)
			     :"memory");
}

#if 1

#include <linux/spinlock.h>

#define HPI_LOCKING

#define DSPLOCK_TYPE spinlock_t
#define OSTYPE_VALIDFLAG int

#define IN_LOCK_BH 1
#define IN_LOCK_IRQ 0

typedef struct {
	DSPLOCK_TYPE lock;
	int lock_context;
} HPIOS_SPINLOCK;

// macro to access wrapping struct fields

// evaluates to a pointer to the lock object, used only internally
#define lock_obj( obj ) (&(obj)->lock)

#if 1
// Generic macro to initialize a lock obj, args are the lock and an os function to be invoked,
// the lock will be the function's only parameter
#define GEN_LOCK_INIT( macro_name, lock_type, init_op )\
static inline void _##macro_name( lock_type* l ) {\
init_op(&l->lock);\
}
#endif

/** Generic macro to grab a lock, parameters are:
macro_name: target macro name
lock_type: type of the lock parame#define HpiOs_Msgxlock_Init( obj ) _DSP_LOCK_INIT( obj )
#define HpiOs_Msgxlock_Lock( obj ) _DSP_LOCK( obj )
#define HpiOs_Msgxlock_UnLock( obj ) _DSP_UNLOCK( obj )

#define HpiOs_Dsplock_Init( obj ) _DSP_LOCK_INIT( &(obj)->dspLock )
#define HpiOs_Dsplock_Lock( obj ) _DSP_LOCK( &(obj)->dspLock )
#define HpiOs_Dsplock_UnLock( obj ) _DSP_UNLOCK( &(obj)->dspLock )
ter the macro will accept
lock_op: locking function that will be applied to (&param->lock)
*/

#define GEN_LOCK( macro_name, lock_type, lock_op )\
static inline void _##macro_name( lock_type* l ) {\
lock_op(&l->lock);\
}

#define GEN_UNLOCK( macro_name, lock_type, unlock_op )\
static inline void _##macro_name( lock_type* l ) {\
unlock_op(&l->lock);\
}

/** Same as above but we pass an unsigned long flags arg to save cpu's irq flags */

#define GEN_LOCK_FLAGS( macro_name, lock_type, lock_op )\
static inline void _##macro_name( lock_type* l, unsigned long *flags ) {\
unsigned long f;\
lock_op(&l->lock, f);\
*flags = f;\
}

#define GEN_UNLOCK_FLAGS( macro_name, lock_type, unlock_op )\
static inline void _##macro_name( lock_type* l, unsigned long *flags ) {\
unsigned long f;\
f = *flags;\
unlock_op(&l->lock, f);\
}

GEN_LOCK_INIT(HPI_LOCK_INIT, HPIOS_SPINLOCK, spin_lock_init)

    GEN_LOCK(HPI_LOCK, HPIOS_SPINLOCK, spin_lock)
    GEN_UNLOCK(HPI_UNLOCK, HPIOS_SPINLOCK, spin_unlock)

    GEN_LOCK(HPI_LOCK_BH, HPIOS_SPINLOCK, spin_lock_bh)
    GEN_UNLOCK(HPI_UNLOCK_BH, HPIOS_SPINLOCK, spin_unlock_bh)

    GEN_LOCK_FLAGS(HPI_LOCK_IRQ, HPIOS_SPINLOCK, spin_lock_irqsave)
    GEN_UNLOCK_FLAGS(HPI_UNLOCK_IRQ, HPIOS_SPINLOCK, spin_unlock_irqrestore)
#ifdef ALSA_BUILD
static inline void cond_lock(HPIOS_SPINLOCK * l)
{
	if (irqs_disabled()) {
// NO bh or isr can execute on this processor, so ordinary lock will do
		_HPI_LOCK(l);
		l->lock_context = IN_LOCK_IRQ;
	} else {
		_HPI_LOCK_BH(l);
		l->lock_context = IN_LOCK_BH;
	}
}

static inline void cond_unlock(HPIOS_SPINLOCK * l)
{
	if (l->lock_context == IN_LOCK_BH) {
		_HPI_UNLOCK_BH(l);
	} else {
		_HPI_UNLOCK(l);
	}
}

#define HpiOs_Msgxlock_Init( obj ) _HPI_LOCK_INIT( obj )
#define HpiOs_Msgxlock_Lock( obj, f ) cond_lock( obj )
#define HpiOs_Msgxlock_UnLock( obj, f ) cond_unlock( obj )

#define HpiOs_Dsplock_Init( obj ) _HPI_LOCK_INIT( &(obj)->dspLock )
#define HpiOs_Dsplock_Lock( obj, f ) cond_lock( &(obj)->dspLock )
#define HpiOs_Dsplock_UnLock( obj, f ) cond_unlock( &(obj)->dspLock )

#define HPIOS_LOCK_FLAGS(name)

#else				// not ALSA_BUILD
#define HpiOs_Msgxlock_Init( obj ) _HPI_LOCK_INIT( obj )
#define HpiOs_Msgxlock_Lock( obj, f ) _HPI_LOCK( obj )
#define HpiOs_Msgxlock_UnLock( obj, f ) _HPI_UNLOCK( obj )
#define HpiOs_Dsplock_Init( obj ) _HPI_LOCK_INIT( &(obj)->dspLock )
#define HpiOs_Dsplock_Lock( obj, f ) _HPI_LOCK( &(obj)->dspLock )
#define HpiOs_Dsplock_UnLock( obj, f ) _HPI_UNLOCK( &(obj)->dspLock )
#define HPIOS_LOCK_FLAGS(name)
#endif				// not ALSA_BUILD
#endif				//1 locking enabled
typedef struct {

/* Spinlock prevent contention for one card between multiple user programs (via ioctl) */
	spinlock_t spinlock;
	unsigned long flags;

/* Semaphores prevent contention for one card between multiple user programs (via ioctl) */
	struct semaphore sem;
	u16 wAdapterIndex;
	u16 type;

/* ALSA card structure */
	void *snd_card_asihpi;

	char *pBuffer;
	void __iomem *dwRemappedMemBase[HPI_MAX_ADAPTER_MEM_SPACES];

} adapter_t;

#ifdef ALSA_BUILD
#define HPI_INCLUDE_4100 1
#define HPI_INCLUDE_4300 1
#define HPI_INCLUDE_5000 1
#define HPI_INCLUDE_6000 1
#define HPI_INCLUDE_6400 1
#define HPI_INCLUDE_6600 1
#define HPI_INCLUDE_8700 1
#endif

///////////////////////////////////////////////////////////////////////////
