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

HPI Operating System function declarations

(C) Copyright AudioScience Inc. 1997-2003
******************************************************************************/
#ifndef _HPIOS_H_
#define _HPIOS_H_

/* /////////////////////////// MACROS /////////////////////////////////////// */
/* use compiler ID to indentify OS */
#if defined ( linux ) && ! defined ( HPI_OS_LINUX )
#define HPI_OS_LINUX
#endif

/* include os specific header file */

#define HPI_OS_DEFINED
#ifdef __KERNEL__
#include "hpios_linux_kernel.h"
#else
#include "hpios_linux.h"
#endif

#ifndef STR_SIZE
#define STR_SIZE(a) (a)
#endif

/* provide defaults until all OSes have these new typedefs */
#ifndef IMPLEMENTED_PTR32
typedef u32 PTR32;
typedef PTR32 PTR_AS_NUMBER;
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
/* monochrome screen debug functions */
void HpiOs_DebugInit(void);
void HpiOs_DebugString(char *pszString);	/* print a string to the monochrome display */
void HpiOs_DebugDword(u32 dwDword);

/* memory allocation */
void *HpiOs_MemAlloc(u32 dwSize);
void HpiOs_MemFree(void *ptr);

/* physical memory allocation */
#ifndef NO_HPIOS_LOCKEDMEM_OPS
void HpiOs_LockedMem_Init(void);
void HpiOs_LockedMem_FreeAll(void);
u16 HpiOs_LockedMem_Alloc(HpiOs_LockedMem_Handle * pLockedMemHandle, u32 dwSize,
			  void *pOsReference);
u16 HpiOs_LockedMem_Free(HpiOs_LockedMem_Handle LockedMemHandle);
u16 HpiOs_LockedMem_GetPhysAddr(HpiOs_LockedMem_Handle LockedMemHandle,
				u32 * pPhysicalAddr);
u16 HpiOs_LockedMem_GetVirtAddr(HpiOs_LockedMem_Handle LockedMemHandle,
				void **ppvVirtualAddr);

/* Old API, no longer use???
u16 HpiOs_AllocLockedMemory( u32 dwSize, void  **ppvLinear, u32 *pPhysical );
u16 HpiOs_FreeLockedMemory( void  *pvLinear );
*/
#endif

/* memory read/write */
u32 HpiOs_MemRead32(u32 dwAddress);
void HpiOs_MemWrite32(u32 dwAddress, u32 dwData);

/* port I/O */
/* ? void HpiOs_OutBuf8( u16 wDataPort, void *pbBuffer, u16 wLength ); */

/* timing/delay */
void HpiOs_DelayMicroSeconds(u32 dwNumMicroSec);

#ifndef NO_HPIOS_FILE_OPS
#ifndef HpiOs_fopen_rb		/* functions not implemented as macros in OS.h files */
u16 HpiOs_fopen_rb(const char *filename, HpiOs_FILE * pFile,
		   u32 * pdwOsErrorCode);
int HpiOs_fseek(HpiOs_FILE stream, long offset, int origin);
int HpiOs_fread(void *buffer, size_t size, size_t count, HpiOs_FILE stream);
int HpiOs_fclose(HpiOs_FILE stream);
#endif

char *HpiOs_GetDspCodePath(u32 nAdapter);
void HpiOs_SetDspCodePath(char *pPath);
#endif

/* /////////////////////////// HpiPCI_* PROTOTYPES /////////////////////////////////// */

struct sHPI_PCI;

typedef struct {
	u16 wVendorId;
	u16 wDeviceId;
	u16 wSubSysVendorId;
	u16 wSubSysDeviceId;

	u16 wClass;
	u16 wClassMask;

	u32 drvData;
} HPI_PCI_DEVICE_ID;

/* given the device index (Nth occurance), vendor and device id, returns the bus
,device number and resources (port,memory,irq) if present
*/
short HpiPci_FindDevice
    (struct sHPI_PCI *pHpiPci, u16 wDevIndex, u16 wPciVendorId, u16 wPciDevId);

/* given the device index (Nth occurance), vendor, device id and sub-vendor,
returns the bus, device number and resources (port,memory,irq) if present
*/
short HpiPci_FindDeviceEx
    (struct sHPI_PCI *pHpiPci,
     u16 wDevIndex, u16 wPciVendorId, u16 wPciDevId, u16 wPciSubVendorId);

short HpiPci_GetMemoryBase(struct sHPI_PCI *pHpiPci, u32 * pdwMemoryBase);

short HpiPci_WriteConfig
    (struct sHPI_PCI *pHpiPci, u16 wPciConfigReg, u32 dwData);

short HpiPci_WriteConfigFast
    (struct sHPI_PCI *pHpiPci, u16 wPciConfigReg, u32 dwData);

short HpiPci_ReadConfig
    (struct sHPI_PCI *pHpiPci, u16 wPciConfigReg, u32 * dwData);

void HpiPci_TranslateAddressRange
    (struct sHPI_PCI *Pci,
     int nBar, u16 * wSelectors, u16 wNumberOf64kSelectors);

void HpiPci_FreeSelectors(u16 * wSelectors, u16 wNumberOf64kSelectors);

#endif				/* _HPIOS_H_ */

/* /////////////////////////////////////////////////////////////////////////
*/
