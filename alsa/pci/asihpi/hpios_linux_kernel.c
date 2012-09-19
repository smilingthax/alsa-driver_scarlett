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
#define SOURCEFILE_NAME "hpios_linux_kernel.c"
#include "hpidebug.h"
#include <linux/delay.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2 , 6 , 14)
void HpiOs_DelayMicroSeconds(
	u32 dwNumMicroSec
)
{
	if ((usecs_to_jiffies(dwNumMicroSec) > 1) && !in_interrupt()) {
/* MUST NOT SCHEDULE IN INTERRUPT CONTEXT! */
/* See  http:kernelnewbies.org/documents/kdoc/kernel-api/linuxkernelapi.html
schedule_timeout() can return early, with a return value of the
number of jiffies remaining, if the task state is INTERRUPTIBLE,
and the task receives a signal.
Setting the state to UNINTERRUPTIBLE stops it from returning early.
*/
		schedule_timeout_uninterruptible(usecs_to_jiffies
			(dwNumMicroSec));
	} else if (dwNumMicroSec <= 2000)
		udelay(dwNumMicroSec);
	else
		mdelay(dwNumMicroSec / 1000);

}
#else
void HpiOs_DelayMicroSeconds(
	u32 dwNumMicroSec
)
{
	if ((dwNumMicroSec / 1000 >= 1000000 / HZ) && !in_interrupt()) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout((HZ * dwNumMicroSec + (HZ - 1)) / 1000000);
	} else if (dwNumMicroSec <= 2000)
		udelay(dwNumMicroSec);
	else
		mdelay(dwNumMicroSec / 1000);

}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2 , 6 , 14)
static struct kmem_cache *memAreaCache;
#else
static struct kmem_cache_s *memAreaCache;
#endif

void HpiOs_LockedMem_Init(
	void
)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2 , 6 , 23)
	memAreaCache = kmem_cache_create("asihpi_mem_area",
		sizeof(struct consistent_dma_area),
		0, SLAB_HWCACHE_ALIGN, NULL);
#else
	memAreaCache = kmem_cache_create("asihpi_mem_area",
		sizeof(struct consistent_dma_area),
		0, SLAB_HWCACHE_ALIGN, NULL, NULL);
#endif
	if (memAreaCache == NULL)
		HPI_DEBUG_LOG(ERROR, "Mem area cache\n");
}

/** Allocated an area of locked memory for bus master DMA operations.

On error, return -ENOMEM, and *pLockedMemHandle=NULL
*/
u16 HpiOs_LockedMem_Alloc(
	struct consistent_dma_area **pLockedMemHandle,
	u32 dwSize,
	struct pci_dev *pdev
)
{
	struct consistent_dma_area *pMemArea;

	*pLockedMemHandle = NULL;
	if (!memAreaCache)
		return -ENOMEM;

	pMemArea = kmem_cache_alloc(memAreaCache, GFP_KERNEL);

	if (pMemArea == NULL) {
		HPI_DEBUG_LOG(WARNING,
			"Couldn't allocate mem control struct\n");
		return -ENOMEM;
	}

	pMemArea->cpu_addr =
		pci_alloc_consistent(pdev, dwSize, &pMemArea->dma_addr);

	if (pMemArea->cpu_addr) {
		HPI_DEBUG_LOG(INFO, "Allocated %d bytes, dma 0x%x vma %p\n",
			dwSize,
			(unsigned int)pMemArea->dma_addr, pMemArea->cpu_addr);
		pMemArea->pdev = pdev;
		pMemArea->size = dwSize;
		*pLockedMemHandle = pMemArea;
		return 0;
	} else {
		HPI_DEBUG_LOG(WARNING,
			"Failed to allocate %d bytes locked memory\n",
			dwSize);
		kmem_cache_free(memAreaCache, pMemArea);
		return -ENOMEM;
	}
}

u16 HpiOs_LockedMem_Free(
	struct consistent_dma_area *LockedMemHandle
)
{
	struct consistent_dma_area *pMemArea =
		(struct consistent_dma_area *)LockedMemHandle;

	if (!LockedMemHandle)
		return 1;

	if (pMemArea->size) {
		pci_free_consistent(pMemArea->pdev, pMemArea->size,
			pMemArea->cpu_addr, pMemArea->dma_addr);
		HPI_DEBUG_LOG(INFO, "Freed %lu bytes, dma 0x%x vma %p\n",
			(unsigned long)pMemArea->size,
			(unsigned int)pMemArea->dma_addr, pMemArea->cpu_addr);
		pMemArea->size = 0;
	}
	kmem_cache_free(memAreaCache, pMemArea);
	return 0;
}

void HpiOs_LockedMem_FreeAll(
	void
)
{
	if (!memAreaCache)
		return;

	kmem_cache_destroy(memAreaCache);
}

u16 HpiOs_LockedMem_GetPhysAddr(
	struct consistent_dma_area *LockedMemHandle,
	u32 *pPhysicalAddr
)
{
	if (!LockedMemHandle) {
		*pPhysicalAddr = 0;
		return 1;
	}
	*pPhysicalAddr =
		((struct consistent_dma_area *)LockedMemHandle)->dma_addr;
	return (0);
}

u16 HpiOs_LockedMem_GetVirtAddr(
	struct consistent_dma_area *LockedMemHandle,
	void **ppvVirtualAddr
)
{
	if (!LockedMemHandle) {
		*ppvVirtualAddr = NULL;
		return 1;
	}

	*ppvVirtualAddr =
		((struct consistent_dma_area *)LockedMemHandle)->cpu_addr;
	return 0;
}
