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

HPI Operating System function declarations

(C) Copyright AudioScience Inc. 1997-2003
******************************************************************************/
#ifndef _HPIOS_H_
#define _HPIOS_H_

/* /////////////////////////// MACROS /////////////////////////////////////// */
/* use compiler ID to indentify OS */
#if defined(linux) && !defined(HPI_OS_LINUX)
#define HPI_OS_LINUX
#endif

#ifndef HPI_OS_LINUX

#if defined(__DSP563C__) || defined(_C56)
#define HPI_OS_DSP_563XX
#endif

#if defined(_TMS320C6X)
#define HPI_OS_DSP_C6000
#endif

#endif /* ndef HPIOSLINUX */

/* include os specific header file */
#ifdef HPI_OS_DOS
#define HPI_OS_DEFINED
#include "hpiosdos.h"
#endif

#ifdef HPI_OS_LINUX
#define HPI_OS_DEFINED
#ifdef __KERNEL__
#include "hpios_linux_kernel.h"
#else
#include "hpios_linux.h"
#endif
#endif

#ifdef HPI_OS_WDM
#define HPI_OS_DEFINED
#include "hpios_wdm.h"
#endif

#ifdef HPI_OS_WIN16
#define HPI_OS_DEFINED
#include "hpiosw16.h"
#endif

#ifdef HPI_OS_WIN32_USER
#define HPI_OS_DEFINED
#include "hpios_win32_user.h"
#endif

#ifdef HPI_OS_WIN95_KERN
#define HPI_OS_DEFINED
#include "hpios_win95_kern.h"
#endif

#ifdef HPI_OS_WINNT_KERN
#define HPI_OS_DEFINED
#include "hpios_winnt_kern.h"
#endif

#ifdef HPI_OS_DSP_C6000
#define HPI_OS_DEFINED
#define HPI_ON_DSP
#include "hpios_c6000.h"
#endif

#ifdef HPI_OS_DSP_563XX
#define HPI_OS_DEFINED
#define HPI_ON_DSP
#include "hpios56k.h"
#endif

#ifndef HPI_OS_DEFINED
#error "Define one of the HPI_OS_xxxx constants in your make file."
#endif

#ifndef HPI_OS_LINUX
/*////////////////////////////////////////////////////////////////////////// */
/* HPI IOCTL definitions, shared by windows user and kernel modes */
#define HPI_IOCTL_WINNT      CTL_CODE(50000,0xA00,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define HPI_IOCTL_WIN95      0x101


#ifndef HPI_API
#define HPI_API
#endif

#endif // not LINUX

#ifndef STR_SIZE
#define STR_SIZE(a) (a)
#endif

#ifndef __user
#define __user
#endif
#ifndef __iomem
#define __iomem
#endif

#ifndef HPI_LOCKING

#define HPIOS_LOCK_FLAGS(name)

#define DSPLOCK_TYPE int
#define OSTYPE_VALIDFLAG int

typedef struct {
    DSPLOCK_TYPE lock;
} HPIOS_SPINLOCK;


#define HpiOs_Msgxlock_Init( obj )
#define HpiOs_Msgxlock_Lock( obj, flags )
#define HpiOs_Msgxlock_UnLock( obj, flags )

#define HpiOs_Dsplock_Init( obj )
#define HpiOs_Dsplock_Lock( obj, flags )
#define HpiOs_Dsplock_UnLock( obj, flags )

#endif

/* /////////////////////////// PROTOTYPES /////////////////////////////////// */

/* memory allocation */
void *HpiOs_MemAlloc( HW32 dwSize );
void HpiOs_MemFree( void *ptr );

/* physical memory allocation */
#ifndef NO_HPIOS_LOCKEDMEM_OPS
void HpiOs_LockedMem_Init( void );
void HpiOs_LockedMem_FreeAll( void );

/** Allocate and map an area of locked memory for bus master DMA operations.

On success, *pLockedMemeHandle is a valid handle, and 0 is returned
On error *pLockedMemHandle=NULL, non-zero returned.

If this function succeeds, then HpiOs_LockedMem_GetVirtAddr() and
HpiOs_LockedMem_GetPyhsAddr() will always succed on the returned handle.
*/
HW16 HpiOs_LockedMem_Alloc( 
	HpiOs_LockedMem_Handle *pLockedMemHandle, /**< memory handle */
	HW32 dwSize,  /**< Size in bytes to allocate */
	void *pOsReference /**< OS specific data required for memory allocation */ 
	);
	
/** Free mapping and memory represented by LockedMemHandle

Returns 0 on success, 1 if handle is NULL
*/	
HW16 HpiOs_LockedMem_Free( HpiOs_LockedMem_Handle LockedMemHandle );

/** Get the physical PCI address of memory represented by LockedMemHandle.

If handle is NULL *pPhysicalAddr is set to zero and return 1 
*/
HW16 HpiOs_LockedMem_GetPhysAddr( HpiOs_LockedMem_Handle LockedMemHandle, HW32 *pPhysicalAddr );

/** Get the CPU address of of memory represented by LockedMemHandle.

If handle is NULL *ppvVirtualAddr is set to NULL and return 1 
*/
HW16 HpiOs_LockedMem_GetVirtAddr( HpiOs_LockedMem_Handle LockedMemHandle, void HFAR **ppvVirtualAddr );

#endif

/* memory read/write */
HW32 HpiOs_MemRead32( HW32 dwAddress );
void HpiOs_MemWrite32(HW32 dwAddress, HW32 dwData );

/* port I/O */
/* ? void HpiOs_OutBuf8( HW16 wDataPort, void *pbBuffer, HW16 wLength ); */

/* timing/delay */
void HpiOs_DelayMicroSeconds( HW32 dwNumMicroSec );

#ifndef NO_HPIOS_FILE_OPS
#ifndef HpiOs_fopen_rb /* functions not implemented as macros in OS.h files */
HW16 HpiOs_fopen_rb(const char *filename,HpiOs_FILE *pFile,HW32 *pdwOsErrorCode);
int HpiOs_fseek(HpiOs_FILE stream, long offset, int origin);
int HpiOs_fread( void *buffer, size_t size, size_t count, HpiOs_FILE stream );
int HpiOs_fclose( HpiOs_FILE stream );
#endif

char *HpiOs_GetDspCodePath(HW32 nAdapter);
void HpiOs_SetDspCodePath(char * pPath);
#endif

/* /////////////////////////// HpiPCI_* PROTOTYPES /////////////////////////////////// */

struct sHPI_PCI;
struct sHPI_MESSAGE;
struct sHPI_RESPONSE;

typedef void HPI_HandlerFunc(struct sHPI_MESSAGE *, struct sHPI_RESPONSE *);

typedef struct {
	HW16 wVendorId;
	HW16 wDeviceId;
	HW16 wSubSysVendorId;
	HW16 wSubSysDeviceId;

	HW16 wClass;
	HW16 wClassMask;

	HPI_HandlerFunc * drvData;
} HPI_PCI_DEVICE_ID;


/* given the device index (Nth occurance), vendor and device id, returns the bus
   ,device number and resources (port,memory,irq) if present
*/
short HpiPci_FindDevice
(
	struct sHPI_PCI	*pHpiPci,
	HW16 wDevIndex,
	HW16 wPciVendorId,
	HW16 wPciDevId
);

/* given the device index (Nth occurance), vendor, device id and sub-vendor,
   returns the bus, device number and resources (port,memory,irq) if present
*/
short HpiPci_FindDeviceEx
(
	struct sHPI_PCI	*pHpiPci,
	HW16 wDevIndex,
	HW16 wPciVendorId,
	HW16 wPciDevId,
	HW16 wPciSubVendorId
);

short HpiPci_GetMemoryBase
(
	struct sHPI_PCI	*pHpiPci,
	HW32 *pdwMemoryBase
);



short HpiPci_WriteConfig
(	struct sHPI_PCI	*pHpiPci,
	HW16 wPciConfigReg,
	HW32 dwData
);

short HpiPci_WriteConfigFast
(
	struct sHPI_PCI	*pHpiPci,
	HW16 wPciConfigReg,
	HW32 dwData
);

short HpiPci_ReadConfig
(
	struct sHPI_PCI	*pHpiPci,

	HW16 wPciConfigReg,
	HW32 *dwData
);

void HpiPci_TranslateAddressRange
(
	struct sHPI_PCI	*Pci,
	int		nBar,
	HW16		*wSelectors,
	HW16		wNumberOf64kSelectors
);

void HpiPci_FreeSelectors
(
	HW16		*wSelectors,
	HW16		wNumberOf64kSelectors
);

#endif /* _HPIOS_H_ */

/* /////////////////////////////////////////////////////////////////////////
*/
