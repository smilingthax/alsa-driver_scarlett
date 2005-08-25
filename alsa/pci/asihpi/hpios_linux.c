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


HPI Operating System function implementation for Linux

(C) Copyright AudioScience Inc. 1997-2003
******************************************************************************/

#include <hpios.h>
#include <hpipci.h>
#include <hpidebug.h>

#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <asm/io.h>
#define __KERNEL_SYSCALLS__
#include <linux/fs.h>
#include <linux/unistd.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#endif

////////////////////////////////////////////////////////////////////////////////
// convert a dword to a string and print it
void HpiOs_DebugDword( HW32 dwX)
{
	short nC=0;
	short i=0;
	char sz[20];

	for(i=0;i<8;i++)
	{
		nC = (short)((dwX>>28) & 0xF);
		dwX = dwX<<4;
		if(nC>9)
			nC = nC + 'A' - 10;	  // 'A'-'F'
		else
			nC = nC + '0';		  // '0'-'9'
		sz[i] = (char)nC;
		sz[i+1] = 0;
	}
	HpiOs_DebugString(sz);
}

// print a string at current position( nVideoX,nVideoY)
// '\n' will cause CR/LF
void HpiOs_DebugString( char *pszString )
{
    //printk(pszString);
}

///////////////////////////////////////

#if DSPCODE_FILE
static int errno;	// kernel doesn't have this symbol

/* Now implemented directly in hpidspcd.c - needs to know code type
char *HpiOs_GetDspCodePath(void)
*/

void HpiOs_DebugInit(void)
{
}


HpiOs_FILE HpiOs_fopen_rb(const char *filename)
{
    HpiOs_FILE hFile;
    mm_segment_t fs;
    fs= get_fs();
    set_fs(get_ds());

    hFile = open(filename,O_RDONLY,0);

    set_fs(fs);

    return hFile;
}
// int fseek( FILE *stream, long offset, int origin );
int HpiOs_fseek(HpiOs_FILE stream, long offset, int origin)
{
    int retval;
    mm_segment_t fs;
    fs= get_fs();
    set_fs(get_ds());
    retval =lseek(stream,offset,origin);
    set_fs(fs);
    return (retval);

}
//size_t fread( void *buffer, size_t size, size_t count, FILE *stream );
int HpiOs_fread( void *buffer, size_t size, size_t count, HpiOs_FILE stream )
{
    int nBytes;
    mm_segment_t fs;
    fs= get_fs();
    set_fs(get_ds());
    nBytes = read(stream,buffer,(size*count));
    set_fs(fs);
    return (nBytes/size);
}

// int fclose( FILE *stream );
int HpiOs_fclose( HpiOs_FILE stream )
{
    int retval;
    mm_segment_t fs;
    fs= get_fs();
    set_fs(get_ds());
    retval=close(stream);
    set_fs(fs);
    return (retval);
}

#endif

#ifndef set_current_state
#define __set_current_state(state_value)	do { current->state = state_value; } while (0)
#ifdef __SMP__
#define set_current_state(state_value)		do { __set_current_state(state_value); mb(); } while (0)
#else
#define set_current_state(state_value)		__set_current_state(state_value)
#endif
#endif
////////////////////////////////////////////////////////////////////////////////
// Delay a set number of microseconds
void HpiOs_DelayMicroSeconds(HW32 dwNumMicroSec)
{
    if ( (dwNumMicroSec / 1000 >= 1000000 / HZ) && ! in_interrupt())
    {
	/* MUST NOT SCHEDULE IN INTERRUPT CONTEXT! */
	/* See  http://kernelnewbies.org/documents/kdoc/kernel-api/linuxkernelapi.html
	   schedule_timeout() can return early, with a return value of the
	   number of jiffies remaining, if the task state is INTERRUPTIBLE,
	   and the task receives a signal.
	   Setting the state to UNINTERRUPTIBLE stops it from returning early.
	*/
	set_current_state(TASK_UNINTERRUPTIBLE);
	schedule_timeout((HZ * dwNumMicroSec + (HZ-1)) / 1000000);
    } else if (dwNumMicroSec <= 2000)
	udelay(dwNumMicroSec);
    else
	mdelay(dwNumMicroSec / 1000);

}

#ifndef BOOLEAN
typedef unsigned char BOOLEAN;
#endif

HW16 HpiOs_UsbInit( void *pRes )
{
	return HPI_ERROR_INVALID_OPERATION;
}

HW16 HpiOs_UsbUnInit( void *pRes )
{
	return HPI_ERROR_INVALID_OPERATION;
}

HW16 HpiOs_UsbBulkWrite( void *pRes, HW16 wEp, HW32 *pdwData, HW32 dwLength, BOOLEAN bUserMem)
{
	return HPI_ERROR_INVALID_OPERATION;
}

HW16 HpiOs_UsbBulkRead( void *pRes, HW16 wEp, HW32 *pdwData, HW32 dwLength, BOOLEAN bUserMem)
{
	return HPI_ERROR_INVALID_OPERATION;
}


short HpiPci_FindDeviceEx(HPI_PCI *pHpiPci,
			  HW16 wDevIndex,
			  HW16 wPciVendorId,
			  HW16 wPciDevId,
			  HW16 wPciSubVendorId)
{
    if (HpiPci_FindDevice(pHpiPci, wDevIndex, wPciVendorId, wPciDevId))
		return 1;	//error
	if (pHpiPci->wSubSysVendorId != wPciSubVendorId)
		return 2;   // Distinguish error, tho make no use of this now 2003-10-31
    return 0;		//sucess!
}


short
HpiPci_FindDevice(HPI_PCI *pHpiPci,
		  HW16 wDevIndex,
		  HW16 wPciVendorId,
		  HW16 wPciDevId)
{
#define MAX_SLOTS       32
#define MAX_BUSES       2

    unsigned short SlotNumber;
    int            bFound;
    unsigned short wAdapterCount;
    struct pci_dev *pdev = NULL;

    SlotNumber = 0;
    bFound = 0;
    wAdapterCount = 0;

    HPI_PRINT_DEBUG("Vendor = 0x%x, Device =0x%x, Index = %d\n",
		    wPciVendorId, wPciDevId, wDevIndex);

    /* Find information about our device if known.  */
    while ((pdev = pci_find_device (wPciVendorId, wPciDevId, pdev)))
    {
	if (wAdapterCount == wDevIndex)
	{
	    bFound = 1;	// Found the ASI card.
	    break;
	}
	wAdapterCount++;
    }

    if (bFound == 1)
    {
	unsigned int memlen;
	unsigned int i;

	pHpiPci->pOsData=pdev;
	pci_set_drvdata(pdev,pHpiPci); // Can look up both ways now

	// We don't need the interrupt so don't request it.
	// Tell the HPI about the device.
	pHpiPci->wVendorId = wPciVendorId;
	pHpiPci->wDeviceId = wPciDevId;
	pHpiPci->wDeviceNumber = pdev->devfn;
	pHpiPci->wBusNumber = pdev->bus->number;
	pHpiPci->wInterrupt = pdev->irq;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
	pci_read_config_dword(pdev, PCI_SUBSYSTEM_VENDOR_ID, &pHpiPci->wSubSysVendorId);
	pci_read_config_dword(pdev, PCI_SUBSYSTEM_ID, &pHpiPci->wSubSysDeviceId);
	// pHpiPci->wSubSysVendorId = 0;
	// pHpiPci->wSubSysDeviceId = 0;
#else
	pHpiPci->wSubSysVendorId = pdev->subsystem_vendor;
	pHpiPci->wSubSysDeviceId = pdev->subsystem_device;
#endif
	
	HPI_PRINT_DEBUG("SubsysVendor = 0x%x, SubsysDevice =0x%x\n",
			pHpiPci->wSubSysVendorId, pHpiPci->wSubSysDeviceId);
	for(i=0; i<HPI_MAX_ADAPTER_MEM_SPACES; i++) {
	    HPI_PRINT_DEBUG("Resource %d %s %lx-%lx\n ",i,pdev->resource[i].name,pdev->resource[i].start,pdev->resource[i].end);
	}

	for(i=0; i<HPI_MAX_ADAPTER_MEM_SPACES; i++) {
		memlen=pci_resource_len(pdev,i);
		if (memlen) {
			pHpiPci->dwMemBase[i] = (HW32) ioremap(pci_resource_start(pdev,i), memlen);
			if (!pHpiPci->dwMemBase[i]) {
				HPI_PRINT_INFO("ioremap failed, aborting\n");
				/*unmap previously mapped pci mem space*/
				for(i=i-1; i >= 0; i--) {
					if ( pHpiPci->dwMemBase[i] ) {
						iounmap( (void *)pHpiPci->dwMemBase[i] );
						pHpiPci->dwMemBase[i] = 0;
					}
				}
				return 1;
			}
		} else pHpiPci->dwMemBase[i] = 0;
	}
	/*?	pHpiPci->dwPortBase = 0; */

	HPI_PRINT_DEBUG("Found device %d, MB0 = 0x%lx, MB1 = 0x%lx, irq = %d\n",
			wDevIndex,pHpiPci->dwMemBase[0] ,pHpiPci->dwMemBase[1], pdev->irq);

	return 0;	//success!
	}

    return 1;	//error!
}


short HpiPci_WriteConfig(HPI_PCI *pHpiPci, HW16 wPciConfigReg, HW32 dwData)
{
// Use pci_config_write_dword ?
    HPI_PRINT_DEBUG("Reg = 0x%x, Data = 0x%lx\n",
		    wPciConfigReg, dwData);
    return 0;
}

short HpiPci_ReadConfig(HPI_PCI *pHpiPci, HW16 wPciConfigReg, HW32 *pdwData)
{
// Use pci_config_read_dword ?
// need the Linux pdev pointer (store in HPI_PCI struct?)
//    pci_read_config_dword(pdev, wPciConfigReg, pdwData);
    return 0;
}

/** Details of a memory area allocated with  pci_alloc_consistent
Need all info for parameters to pci_free_consistent
*/
typedef struct {
    struct pci_dev *pdev;
    dma_addr_t dma_addr;
    void * cpu_addr;
    size_t size;
} HpiOs_LockedMem_Area;

// MAX_STREAMS applies to in and out streams separately. 
// +1 is for adapter Busmaster area
#define MAX_LOCKED_AREAS (HPI_MAX_ADAPTERS * (HPI_MAX_STREAMS * 2 + 1))

static HpiOs_LockedMem_Area memAreas[MAX_LOCKED_AREAS];
static int memInitialised = 0;

HW16 HpiOs_LockedMem_Alloc( HpiOs_LockedMem_Handle *pLockedMemHandle, HW32 dwSize, void *pOsReference )
{
    struct pci_dev  *pdev = *(struct pci_dev  **)pOsReference;
    HpiOs_LockedMem_Area * pMemArea;
    int i;

    if (!memInitialised) {
	memset(&memAreas, 0, sizeof(memAreas));
	memInitialised=1;
    }

    for (i=0; i<MAX_LOCKED_AREAS; i++) {
	pMemArea=&memAreas[i];
	if (pMemArea->size == 0)
	    break;
    }

    if (i==MAX_LOCKED_AREAS) {
	HPI_PRINT_DEBUG("No free locked_mem areas\n");
	pLockedMemHandle=0;
	return 1;
    }
 
    pMemArea->cpu_addr = pci_alloc_consistent(pMemArea->pdev, dwSize, &pMemArea->dma_addr);

    if (pMemArea->cpu_addr) {
	HPI_PRINT_DEBUG("Allocated %ld bytes, dma 0x%x vma 0x%x\n",dwSize, 
	(unsigned int)pMemArea->dma_addr,
	(unsigned int)pMemArea->cpu_addr );
	pMemArea->pdev=pdev;
	pMemArea->size=dwSize;
	*pLockedMemHandle = pMemArea;
	return 0;
    } else {
	HPI_PRINT_INFO("Failed to allocate %ld bytes locked memory\n",dwSize);
	pLockedMemHandle=0;
	return 1;
    }
}

HW16 HpiOs_LockedMem_Free( HpiOs_LockedMem_Handle LockedMemHandle )
{
    HpiOs_LockedMem_Area * pMemArea = (HpiOs_LockedMem_Area *)LockedMemHandle;

    if (!memInitialised)
	return 1;

    if (pMemArea->size) {
	pci_free_consistent(pMemArea->pdev,pMemArea->size,pMemArea->cpu_addr,pMemArea->dma_addr);

	HPI_PRINT_DEBUG("Freed %d bytes, dma 0x%x vma 0x%x\n",
	       pMemArea->size, (unsigned int)pMemArea->dma_addr,(unsigned int)pMemArea->cpu_addr );
	pMemArea->size=0;
    } else if (pMemArea->pdev) {
	HPI_PRINT_DEBUG("Mem area already freed\n");
    } 

    return 0;
}

void  HpiOs_LockedMem_FreeAll(void)
{
    int i;
    if (!memInitialised)
	return;

    for (i=0; i<MAX_LOCKED_AREAS; i++)
	HpiOs_LockedMem_Free( (HpiOs_LockedMem_Handle)(&memAreas[i]));
}

HW16 HpiOs_LockedMem_GetPhysAddr( HpiOs_LockedMem_Handle LockedMemHandle, HW32 *pPhysicalAddr )
{
    if (!memInitialised) {
	*pPhysicalAddr=0;
	return 1;
    }
    *pPhysicalAddr = ((HpiOs_LockedMem_Area *)LockedMemHandle)->dma_addr;
    return(0);
}

HW16 HpiOs_LockedMem_GetVirtAddr( HpiOs_LockedMem_Handle LockedMemHandle, void **ppvVirtualAddr )
{
    if (!memInitialised) {
	*ppvVirtualAddr=0;
	return 1;
    }

    *ppvVirtualAddr =((HpiOs_LockedMem_Area *) LockedMemHandle)->cpu_addr;
    return 0;
}

/////////////////////////////////////////////////////////////
