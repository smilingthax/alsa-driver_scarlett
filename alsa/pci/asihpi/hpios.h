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

#define HPI_OS_DEFINED
#ifdef __KERNEL__
#include "hpios_linux_kernel.h"
#else
#include "hpios_linux.h"
#endif

#ifndef STR_SIZE
#define STR_SIZE(a) (a)
#endif

#ifndef __user
#define __user
#endif
#ifndef __iomem
#define __iomem
#endif

/* /////////////////////////// PROTOTYPES /////////////////////////////////// */

/* memory allocation */
#ifndef HpiOs_MemAlloc
void *HpiOs_MemAlloc(u32 dwSize);
#endif
#ifndef HpiOs_MemFree
void HpiOs_MemFree(void *ptr);
#endif

/* physical memory allocation */
#ifndef NO_HPIOS_LOCKEDMEM_OPS
void HpiOs_LockedMem_Init(void);
void HpiOs_LockedMem_FreeAll(void);
#define HpiOs_LockedMem_Prepare( a, b, c, d );
#define HpiOs_LockedMem_Unprepare( a )

/** Allocate and map an area of locked memory for bus master DMA operations.

On success, *pLockedMemeHandle is a valid handle, and 0 is returned
On error *pLockedMemHandle=NULL, non-zero returned.

If this function succeeds, then HpiOs_LockedMem_GetVirtAddr() and
HpiOs_LockedMem_GetPyhsAddr() will always succed on the returned handle.
*/
u16 HpiOs_LockedMem_Alloc(HpiOs_LockedMem_Handle * pLockedMemHandle,
					  /**< memory handle */
			  u32 dwSize,
	     /**< Size in bytes to allocate */
			  struct pci_dev *pOsReference
			      /**< OS specific data required for memory allocation */
    );

/** Free mapping and memory represented by LockedMemHandle

Returns 0 on success, 1 if handle is NULL
*/
u16 HpiOs_LockedMem_Free(HpiOs_LockedMem_Handle LockedMemHandle);

/** Get the physical PCI address of memory represented by LockedMemHandle.

If handle is NULL *pPhysicalAddr is set to zero and return 1
*/
u16 HpiOs_LockedMem_GetPhysAddr(HpiOs_LockedMem_Handle LockedMemHandle,
				u32 * pPhysicalAddr);

/** Get the CPU address of of memory represented by LockedMemHandle.

If handle is NULL *ppvVirtualAddr is set to NULL and return 1
*/
u16 HpiOs_LockedMem_GetVirtAddr(HpiOs_LockedMem_Handle LockedMemHandle,
				void **ppvVirtualAddr);

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
struct sHPI_MESSAGE;
struct sHPI_RESPONSE;

typedef void HPI_HandlerFunc(struct sHPI_MESSAGE *, struct sHPI_RESPONSE *);

typedef struct {
	u16 wVendorId;
	u16 wDeviceId;
	u16 wSubSysVendorId;
	u16 wSubSysDeviceId;

	u16 wClass;
	u16 wClassMask;

	HPI_HandlerFunc *drvData;
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
