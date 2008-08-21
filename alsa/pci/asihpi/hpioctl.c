/*******************************************************************************

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

Common Linux HPI ioctl and module probe/remove functions
*******************************************************************************/
#define SOURCEFILE_NAME "hpioctl.c"

#include "hpi_internal.h"
#include "hpimsginit.h"
#include "hpidebug.h"
#include "hpimsgx.h"
#include "hpioctl.h"

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/moduleparam.h>
#include <asm/uaccess.h>
#include <linux/stringify.h>

#ifdef MODULE_FIRMWARE
MODULE_FIRMWARE("asihpi/dsp5000.bin");
MODULE_FIRMWARE("asihpi/dsp6200.bin");
MODULE_FIRMWARE("asihpi/dsp6205.bin");
MODULE_FIRMWARE("asihpi/dsp6400.bin");
MODULE_FIRMWARE("asihpi/dsp6600.bin");
MODULE_FIRMWARE("asihpi/dsp8700.bin");
MODULE_FIRMWARE("asihpi/dsp8900.bin");
#endif

static int prealloc_stream_buf;
module_param(prealloc_stream_buf, int,
	S_IRUGO
);
MODULE_PARM_DESC(prealloc_stream_buf,
	"Preallocate size for per-adapter stream buffer");

/* Allow the debug level to be changed after module load.
 E.g.   echo 2 > /sys/module/asihpi/parameters/hpiDebugLevel
*/
module_param(hpiDebugLevel, int,
	S_IRUGO | S_IWUSR
);
MODULE_PARM_DESC(hpiDebugLevel, "Debug verbosity 0..5");

/* List of adapters found */
static struct hpi_adapter adapters[HPI_MAX_ADAPTERS];

/* Wrapper function to HPI_Message to enable dumping of the
   message and response types.
*/
static void HPI_MessageF(
	struct hpi_message *phm,
	struct hpi_response *phr,
	struct file *file
)
{
	int nAdapter = phm->wAdapterIndex;

	if ((nAdapter >= HPI_MAX_ADAPTERS || nAdapter < 0) &&
		(phm->wObject != HPI_OBJ_SUBSYSTEM))
		phr->wError = HPI_ERROR_INVALID_OBJ_INDEX;
	else
		HPI_MessageEx(phm, phr, file);
}

/* This is called from hpifunc.c functions, called by ALSA
 * (or other kernel process) In this case there is no file descriptor
 * available for the message cache code
 */
void HPI_Message(
	struct hpi_message *phm,
	struct hpi_response *phr
)
{
	HPI_MessageF(phm, phr, HOWNER_KERNEL);
}

EXPORT_SYMBOL(HPI_Message);
/* export HPI_Message for radio-asihpi */

int asihpi_hpi_release(
	struct file *file
)
{
	struct hpi_message hm;
	struct hpi_response hr;

/* HPI_DEBUG_LOG(INFO,"hpi_release file %p, pid %d\n", file, current->pid); */
	/* close the subsystem just in case the application forgot to. */
	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_CLOSE);
	HPI_MessageEx(&hm, &hr, file);
	return 0;
}

long asihpi_hpi_ioctl(
	struct file *file,
	unsigned int cmd,
	unsigned long arg
)
{
	struct hpi_ioctl_linux __user *phpi_ioctl_data;
	void __user *phm;
	void __user *phr;
	struct hpi_message hm;
	struct hpi_response hr;
	u32 uncopied_bytes;
	struct hpi_adapter *pa = NULL;

	if (cmd != HPI_IOCTL_LINUX)
		return -EINVAL;

	phpi_ioctl_data = (struct hpi_ioctl_linux __user *)arg;

	/* Read the message and response pointers from user space.  */
	get_user(phm, &phpi_ioctl_data->phm);
	get_user(phr, &phpi_ioctl_data->phr);

	/* Now read the message size and data from user space.  */
	/* get_user(hm.wSize, (u16 __user *)phm); */
	uncopied_bytes = copy_from_user(&hm, phm, sizeof(hm));
	if (uncopied_bytes)
		return -EFAULT;

	pa = &adapters[hm.wAdapterIndex];
	hr.wSize = 0;
	if (hm.wObject == HPI_OBJ_SUBSYSTEM) {
		HPI_MessageF(&hm, &hr, file);
	} else {
		u16 __user *ptr = NULL;
		u32 size = 0;

		/* -1=no data 0=read from user mem, 1=write to user mem */
		int wrflag = -1;
		u32 nAdapter = hm.wAdapterIndex;

		if ((hm.wAdapterIndex > HPI_MAX_ADAPTERS) || (!pa->type)) {
			HPI_InitResponse(&hr, HPI_OBJ_ADAPTER,
				HPI_ADAPTER_OPEN,
				HPI_ERROR_BAD_ADAPTER_NUMBER);

			uncopied_bytes = copy_to_user(phr, &hr, sizeof(hr));
			if (uncopied_bytes)
				return -EFAULT;
			return 0;
		}

		if (mutex_lock_interruptible(&adapters[nAdapter].mutex))
			return -EINTR;

		/* Dig out any pointers embedded in the message.  */
		switch (hm.wFunction) {
		case HPI_SUBSYS_CREATE_ADAPTER:
		case HPI_SUBSYS_DELETE_ADAPTER:
			/* Application must not use these functions! */
			hr.wSize = sizeof(struct hpi_response_header);
			hr.wError = HPI_ERROR_INVALID_OPERATION;
			hr.wFunction = hm.wFunction;
			uncopied_bytes = copy_to_user(phr, &hr, hr.wSize);
			if (uncopied_bytes)
				return -EFAULT;
			return 0;
		case HPI_OSTREAM_WRITE:
		case HPI_ISTREAM_READ:
			/* Yes, sparse, this is correct. */
			ptr = (u16 __user *)hm.u.d.u.Data.pbData;
			size = hm.u.d.u.Data.dwDataSize;

			/* Allocate buffer according to application request.
			   ?Is it better to alloc/free for the duration
			   of the transaction?
			 */
			if (pa->buffer_size < size) {
				HPI_DEBUG_LOG(DEBUG,
					"Realloc adapter %d stream buffer "
					"from %zd to %d\n",
					hm.wAdapterIndex,
					pa->buffer_size, size);
				if (pa->pBuffer) {
					pa->buffer_size = 0;
					vfree(pa->pBuffer);
				}
				pa->pBuffer = vmalloc(size);
				if (pa->pBuffer)
					pa->buffer_size = size;
				else {
					HPI_DEBUG_LOG(ERROR,
						"HPI could not allocate "
						"stream buffer size %d\n",
						size);
					return -EINVAL;
				}

			}

			hm.u.d.u.Data.pbData = pa->pBuffer;
			if (hm.wFunction == HPI_ISTREAM_READ)
				/* from card, WRITE to user mem */
				wrflag = 1;
			else
				wrflag = 0;
			break;

		default:
			break;
		}

		if (wrflag == 0) {
			uncopied_bytes =
				copy_from_user(pa->pBuffer, ptr, size);
			if (uncopied_bytes)
				HPI_DEBUG_LOG(WARNING,
					"Missed %d of %d "
					"bytes from user\n",
					uncopied_bytes, size);
		}

		HPI_MessageF(&hm, &hr, file);

		if (wrflag == 1) {
			uncopied_bytes = copy_to_user(ptr, pa->pBuffer, size);
			if (uncopied_bytes)
				HPI_DEBUG_LOG(WARNING,
					"Missed %d of %d "
					"bytes to user\n",
					uncopied_bytes, size);
		}

		mutex_unlock(&adapters[nAdapter].mutex);
	}

	/* on return response size must be set */
	if (!hr.wSize)
		return -EFAULT;
	uncopied_bytes = copy_to_user(phr, &hr, sizeof(hr));
	if (uncopied_bytes)
		return -EFAULT;

	return 0;
}

int __devinit asihpi_adapter_probe(
	struct pci_dev *pci_dev,
	const struct pci_device_id *pci_id
)
{
	int err, idx;
	unsigned int memlen;
	struct hpi_message hm;
	struct hpi_response hr;
	struct hpi_adapter adapter;
	struct hpi_pci Pci;

	memset(&adapter, 0, sizeof(adapter));

	printk(KERN_DEBUG "Probe PCI device (%04x:%04x,%04x:%04x,%04x)\n",
		pci_dev->vendor, pci_dev->device, pci_dev->subsystem_vendor,
		pci_dev->subsystem_device, pci_dev->devfn);

	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_CREATE_ADAPTER);
	HPI_InitResponse(&hr, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_CREATE_ADAPTER,
		HPI_ERROR_PROCESSING_MESSAGE);

	hm.wAdapterIndex = -1;	/* an invalid index */

	/* fill in HPI_PCI information from kernel provided information */
	adapter.pci = pci_dev;

	for (idx = 0; idx < HPI_MAX_ADAPTER_MEM_SPACES; idx++) {
		HPI_DEBUG_LOG(DEBUG, "Resource %d %s %llx-%llx\n",
			idx, pci_dev->resource[idx].name,
			(unsigned long long)pci_resource_start(pci_dev, idx),
			(unsigned long long)pci_resource_end(pci_dev, idx));

		memlen = pci_resource_len(pci_dev, idx);
		if (memlen) {
			adapter.apRemappedMemBase[idx] =
				ioremap(pci_resource_start(pci_dev, idx),
				memlen);
			if (!adapter.apRemappedMemBase[idx]) {
				HPI_DEBUG_LOG(ERROR,
					"ioremap failed, aborting\n");
				/* unmap previously mapped pci mem space */
				goto err;
			}
		} else
			adapter.apRemappedMemBase[idx] = NULL;

		Pci.apMemBase[idx] = adapter.apRemappedMemBase[idx];
	}

	Pci.wBusNumber = pci_dev->bus->number;
	Pci.wVendorId = (u16)pci_dev->vendor;
	Pci.wDeviceId = (u16)pci_dev->device;
	Pci.wSubSysVendorId = (u16)(pci_dev->subsystem_vendor & 0xffff);
	Pci.wSubSysDeviceId = (u16)(pci_dev->subsystem_device & 0xffff);
	Pci.wDeviceNumber = pci_dev->devfn;
	Pci.wInterrupt = pci_dev->irq;
	Pci.pOsData = pci_dev;

	hm.u.s.Resource.wBusType = HPI_BUS_PCI;
	hm.u.s.Resource.r.Pci = &Pci;

	/* call CreateAdapterObject on the relevant hpi module */
	HPI_MessageEx(&hm, &hr, HOWNER_KERNEL);
	if (hr.wError)
		goto err;

	if (prealloc_stream_buf) {
		adapter.pBuffer = vmalloc(prealloc_stream_buf);
		if (!adapter.pBuffer) {
			HPI_DEBUG_LOG(ERROR,
				"HPI could not allocate "
				"kernel buffer size %d\n",
				prealloc_stream_buf);
			goto err;
		}
	}

	adapter.index = hr.u.s.wAdapterIndex;
	adapter.type = hr.u.s.awAdapterList[adapter.index];
	hm.wAdapterIndex = adapter.index;

	err = HPI_AdapterOpen(NULL, adapter.index);
	if (err)
		goto err;

	adapter.snd_card_asihpi = NULL;
	/* WARNING can't init mutex in 'adapter'
	 * and then copy it to adapters[] ?!?!
	 */
	adapters[hr.u.s.wAdapterIndex] = adapter;
	mutex_init(&adapters[adapter.index].mutex);
	pci_set_drvdata(pci_dev, &adapters[adapter.index]);

	printk(KERN_INFO
		"Probe found adapter ASI%04X HPI index #%d.\n",
		adapter.type, adapter.index);

	return 0;

err:
	for (idx = 0; idx < HPI_MAX_ADAPTER_MEM_SPACES; idx++) {
		if (adapter.apRemappedMemBase[idx]) {
			iounmap(adapter.apRemappedMemBase[idx]);
			adapter.apRemappedMemBase[idx] = NULL;
		}
	}

	if (adapter.pBuffer) {
		adapter.buffer_size = 0;
		vfree(adapter.pBuffer);
	}

	HPI_DEBUG_LOG(ERROR, "adapter_probe failed\n");
	return -ENODEV;
}

void __devexit asihpi_adapter_remove(
	struct pci_dev *pci_dev
)
{
	int idx;
	struct hpi_message hm;
	struct hpi_response hr;
	struct hpi_adapter *pa;
	pa = (struct hpi_adapter *)pci_get_drvdata(pci_dev);

	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_DELETE_ADAPTER);
	hm.wAdapterIndex = pa->index;
	HPI_MessageEx(&hm, &hr, HOWNER_KERNEL);

	/* unmap PCI memory space, mapped during device init. */
	for (idx = 0; idx < HPI_MAX_ADAPTER_MEM_SPACES; idx++) {
		if (pa->apRemappedMemBase[idx]) {
			iounmap(pa->apRemappedMemBase[idx]);
			pa->apRemappedMemBase[idx] = NULL;
		}
	}

	if (pa->pBuffer) {
		pa->buffer_size = 0;
		vfree(pa->pBuffer);
	}

	pci_set_drvdata(pci_dev, NULL);
	/*
	   printk(KERN_INFO "PCI device (%04x:%04x,%04x:%04x,%04x),"
	   " HPI index # %d, removed.\n",
	   pci_dev->vendor, pci_dev->device,
	   pci_dev->subsystem_vendor,
	   pci_dev->subsystem_device, pci_dev->devfn,
	   pa->index);
	 */
}

void __init asihpi_init(
	void
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	memset(adapters, 0, sizeof(adapters));

	printk(KERN_INFO "ASIHPI driver %d.%02d.%02d\n",
		HPI_VER_MAJOR(HPI_VER),
		HPI_VER_MINOR(HPI_VER), HPI_VER_RELEASE(HPI_VER));

	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_DRIVER_LOAD);
	HPI_MessageEx(&hm, &hr, HOWNER_KERNEL);
}

void asihpi_exit(
	void
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_DRIVER_UNLOAD);
	HPI_MessageEx(&hm, &hr, HOWNER_KERNEL);
}

/***********************************************************
*/
