/* -*- linux-c -*- */
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

Linux HPI driver module
*******************************************************************************/
#define SOURCEFILE_NAME "hpimod.c"

#include "hpi.h"
#include "hpidebug.h"
#include "hpimsgx.h"
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <asm/uaccess.h>
#include <linux/stringify.h>

int snd_asihpi_bind(adapter_t * hpi_card);
void snd_asihpi_unbind(adapter_t * hpi_card);

#ifndef HPIMOD_DEFAULT_BUF_SIZE
#   define HPIMOD_DEFAULT_BUF_SIZE 192000
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AudioScience <support@audioscience.com>");
MODULE_DESCRIPTION("AudioScience HPI");

#ifdef MODULE_FIRMWARE
MODULE_FIRMWARE("asihpi/dsp2400.bin");
MODULE_FIRMWARE("asihpi/dsp4100.bin");
MODULE_FIRMWARE("asihpi/dsp4300.bin");
MODULE_FIRMWARE("asihpi/dsp5000.bin");
MODULE_FIRMWARE("asihpi/dsp6200.bin");
MODULE_FIRMWARE("asihpi/dsp6205.bin");
MODULE_FIRMWARE("asihpi/dsp6413.bin");
MODULE_FIRMWARE("asihpi/dsp6600.bin");
MODULE_FIRMWARE("asihpi/dsp8713.bin");
MODULE_FIRMWARE("asihpi/dsp8900.bin");
#endif

static int major = 0;
static int bufsize = HPIMOD_DEFAULT_BUF_SIZE;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
/* old style parameters */
MODULE_PARM(major, "i");
MODULE_PARM(hpiDebugLevel, "0-6i");
MODULE_PARM(bufsize, "i");

#else				/* new style params */
module_param(major, int, S_IRUGO);
module_param(bufsize, int, S_IRUGO);
/* Allow the debug level to be changed after module load.
 E.g.   echo 2 > /sys/module/asihpi/parameters/hpiDebugLevel
*/
module_param(hpiDebugLevel, int, S_IRUGO | S_IWUSR);
#endif

MODULE_PARM_DESC(major, "Device major number");
MODULE_PARM_DESC(hpiDebugLevel,
		 "Debug level for Audioscience HPI 0=none 6=verbose");
MODULE_PARM_DESC(bufsize,
		 "Buffer size to allocate for data transfer from HPI ioctl ");

/* List of adapters found */
static int adapter_count = 0;
static adapter_t adapters[HPI_MAX_ADAPTERS];

/* Wrapper function to HPI_Message to enable dumping of the
   message and response types.
*/
static void HPI_MessageF(HPI_MESSAGE * phm,
			 HPI_RESPONSE * phr, struct file *file)
{
	int nAdapter = phm->wAdapterIndex;

	if ((nAdapter >= HPI_MAX_ADAPTERS || nAdapter < 0) &&
	    (phm->wObject != HPI_OBJ_SUBSYSTEM)) {
		phr->wError = HPI_ERROR_INVALID_OBJ_INDEX;
	} else {
		HPI_MessageEx(phm, phr, file);
	}
}

#define HOWNER_KERNEL ((void *)-1)
/* This is called from hpifunc.c functions, called by ALSA (or other kernel process)
   In this case there is no file descriptor available for the message cache code
*/
void HPI_Message(HPI_MESSAGE * phm, HPI_RESPONSE * phr)
{
	HPI_MessageF(phm, phr, HOWNER_KERNEL);
}

static int hpi_open(struct inode *inode, struct file *file)
{
	unsigned int minor = MINOR(inode->i_rdev);

	if (minor > 0)
		return -ENODEV;

/*	HPI_DEBUG_LOG2(INFO,"hpi_open file %p, pid %d\n", file, current->pid); */
	return 0;
}

static int hpi_release(struct inode *inode, struct file *file)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;

/*	HPI_DEBUG_LOG2(INFO,"hpi_release file %p, pid %d\n", file, current->pid); */
	/* close the subsystem just in case the application forgot to. */
	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_CLOSE);
	HPI_MessageEx(&hm, &hr, file);
	return 0;
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,11)
static long hpi_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#else
static int hpi_ioctl(struct inode *inode, struct file *file,
		     unsigned int cmd, unsigned long arg)
#endif
{
	struct hpi_ioctl_linux __user *phpi_ioctl_data;
	void __user *phm;
	void __user *phr;
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	u32 uncopied_bytes;
	adapter_t *pa;

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

	if (hm.wAdapterIndex > HPI_MAX_ADAPTERS) {
		HPI_InitResponse(&hr, HPI_OBJ_ADAPTER, HPI_ADAPTER_OPEN,
				 HPI_ERROR_BAD_ADAPTER_NUMBER);
		/* Copy the response back to user space.  */
		uncopied_bytes = copy_to_user(phr, &hr, sizeof(hr));
		if (uncopied_bytes)
			return -EFAULT;
		return 0;
	}

	pa = &adapters[hm.wAdapterIndex];
	hr.wSize = 0;
	/* Response gets filled in either by copy from cache, or by HPI_Message() */
	{
		/* Dig out any pointers embedded in the message.  */
		u16 __user *ptr = NULL;
		u32 size = 0;

		int wrflag = -1;	/* -1=no data 0=read from user mem, 1=write to user mem */
		int nAdapter = hm.wAdapterIndex;
		switch (hm.wFunction) {
		case HPI_OSTREAM_WRITE:
		case HPI_ISTREAM_READ:
			/* Yes, sparse, this is correct. */
			ptr = (u16 __user *) hm.u.d.u.Data.pbData;
			size = hm.u.d.u.Data.dwDataSize;

			hm.u.d.u.Data.pbData = pa->pBuffer;
			/*
			   if (size > bufsize) {
			   size = bufsize;
			   hm.u.d.u.Data.dwDataSize = size;
			   }
			 */

			if (hm.wFunction == HPI_ISTREAM_READ)	/* from card, WRITE to user mem */
				wrflag = 1;
			else
				wrflag = 0;
			break;

		default:
			break;
		}

		if ((nAdapter >= HPI_MAX_ADAPTERS || nAdapter < 0) &&
		    (hm.wObject != HPI_OBJ_SUBSYSTEM))
			hr.wError = HPI_ERROR_INVALID_OBJ_INDEX;
		else {
			if (down_interruptible(&adapters[nAdapter].sem))
				return (-EINTR);

			if (wrflag == 0) {

				if (size > bufsize) {
					up(&adapters[nAdapter].sem);
					HPI_DEBUG_LOG2(WARNING,
						       "Requested transfer of %d bytes, but max buffer size in %d bytes, returning error.\n",
						       size, bufsize);
					return -EINVAL;
				}

				uncopied_bytes =
				    copy_from_user(pa->pBuffer, ptr, size);
				if (uncopied_bytes) {
					HPI_DEBUG_LOG2(WARNING,
						       "Missed %d of %d bytes from user\n",
						       uncopied_bytes, size);
				}
			}

			HPI_MessageF(&hm, &hr, file);

			if (wrflag == 1) {
				if (size > bufsize) {
					up(&adapters[nAdapter].sem);
					HPI_DEBUG_LOG2(WARNING,
						       "Requested transfer of %d bytes, but max buffer size in %d bytes, returning error.\n",
						       size, bufsize);
					return -EINVAL;
				}

				uncopied_bytes =
				    copy_to_user(ptr, pa->pBuffer, size);
				if (uncopied_bytes) {
					HPI_DEBUG_LOG2(WARNING,
						       "Missed %d of %d bytes to user\n",
						       uncopied_bytes, size);
				}
			}

			up(&adapters[nAdapter].sem);
		}
	}

	/* on return response size must be set */
	if (!hr.wSize)
		return -EFAULT;

	/* Copy the response back to user space.  */
	uncopied_bytes = copy_to_user(phr, &hr, sizeof(hr));
	if (uncopied_bytes)
		return -EFAULT;
	return 0;
}

static struct file_operations hpi_fops = {
	.owner = THIS_MODULE,
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,11)
	.unlocked_ioctl = hpi_ioctl,
#else
	.ioctl = hpi_ioctl,
#endif
	.open = hpi_open,
	.release = hpi_release
};

static int __devinit adapter_probe(struct pci_dev *pci_dev,
				   const struct pci_device_id *pci_id)
{
	int err, idx;
	unsigned int memlen;
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	adapter_t adapter;
	HPI_PCI Pci;

	memset(&adapter, 0, sizeof(adapter));

	printk(KERN_DEBUG "Probe PCI device (%04x:%04x,%04x:%04x,%04x)\n",
	       pci_dev->vendor, pci_dev->device, pci_dev->subsystem_vendor,
	       pci_dev->subsystem_device, pci_dev->devfn);

	if (HPI_SubSysCreate() == NULL) {
		printk(KERN_ERR "SubSysCreate failed.\n");
		goto err;
	}

	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_CREATE_ADAPTER);
	HPI_InitResponse(&hr, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_CREATE_ADAPTER,
			 HPI_ERROR_PROCESSING_MESSAGE);

	hm.wAdapterIndex = -1;	/* an invalid index */

	/* fill in HPI_PCI information from kernel provided information */
	adapter.pci = pci_dev;

	for (idx = 0; idx < HPI_MAX_ADAPTER_MEM_SPACES; idx++) {
		HPI_DEBUG_LOG4(DEBUG, "Resource %d %s %llx-%llx\n", idx,
			       pci_dev->resource[idx].name,
			       (unsigned long long)pci_resource_start(pci_dev,
								      idx),
			       (unsigned long long)pci_resource_end(pci_dev,
								    idx));

		memlen = pci_resource_len(pci_dev, idx);
		if (memlen) {
			adapter.apRemappedMemBase[idx] =
			    ioremap(pci_resource_start(pci_dev, idx), memlen);
			if (!adapter.apRemappedMemBase[idx]) {
				HPI_DEBUG_LOG0(ERROR,
					       "ioremap failed, aborting\n");
				/* unmap previously mapped pci mem space */
				goto err;
			}
		} else
			adapter.apRemappedMemBase[idx] = NULL;

		Pci.apMemBase[idx] = adapter.apRemappedMemBase[idx];
	}

	Pci.wBusNumber = pci_dev->bus->number;
	Pci.wVendorId = (u16) pci_dev->vendor;
	Pci.wDeviceId = (u16) pci_dev->device;
	Pci.wSubSysVendorId = (u16) (pci_dev->subsystem_vendor & 0xffff);
	Pci.wSubSysDeviceId = (u16) (pci_dev->subsystem_device & 0xffff);
	Pci.wDeviceNumber = pci_dev->devfn;
	Pci.wInterrupt = pci_dev->irq;
	Pci.pOsData = pci_dev;

	hm.u.s.Resource.wBusType = HPI_BUS_PCI;
	hm.u.s.Resource.r.Pci = &Pci;

	/* call CreateAdapterObject on the relevant hpi module */
	HPI_MessageEx(&hm, &hr, HOWNER_KERNEL);

	if ((adapter.pBuffer = vmalloc(bufsize)) == NULL) {
		HPI_DEBUG_LOG1(ERROR,
			       "HPI could not allocate kernel buffer size %d\n",
			       bufsize);
		goto err;
	}

	if (hr.wError == 0) {
		adapter.wAdapterIndex = hr.u.s.wAdapterIndex;
		hm.wAdapterIndex = hr.u.s.wAdapterIndex;

		err = HPI_AdapterOpen(NULL, hr.u.s.wAdapterIndex);
		if (err)
			goto err;

		adapter.snd_card_asihpi = NULL;
		/* WARNING can't init sem in 'adapter' and then copy it to adapters[] ?!?! */
		adapters[hr.u.s.wAdapterIndex] = adapter;
		init_MUTEX(&adapters[hr.u.s.wAdapterIndex].sem);
#ifdef ALSA_BUILD
		if (snd_asihpi_bind(&adapters[hr.u.s.wAdapterIndex])) {
			HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM,
					HPI_SUBSYS_DELETE_ADAPTER);
			hm.wAdapterIndex = adapter.wAdapterIndex;
			HPI_MessageEx(&hm, &hr, HOWNER_KERNEL);
			goto err;
		}
#endif

		pci_set_drvdata(pci_dev, &adapters[hr.u.s.wAdapterIndex]);
		adapter_count++;

		printk(KERN_INFO "Probe found adapter ASI%04X HPI index #%d.\n",
		       hr.u.s.awAdapterList[hr.u.s.wAdapterIndex],
		       hr.u.s.wAdapterIndex);

		return 0;
	}

      err:
	for (idx = 0; idx < HPI_MAX_ADAPTER_MEM_SPACES; idx++) {
		if (adapter.apRemappedMemBase[idx]) {
			iounmap(adapter.apRemappedMemBase[idx]);
			adapter.apRemappedMemBase[idx] = NULL;
		}
	}

	if (adapter.pBuffer)
		vfree(adapter.pBuffer);

	HPI_DEBUG_LOG0(ERROR, "adapter_probe failed\n");
	return -ENODEV;
}

static void __devexit adapter_remove(struct pci_dev *pci_dev)
{
	int idx;
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	adapter_t *pa;
	pa = (adapter_t *) pci_get_drvdata(pci_dev);

#ifdef ALSA_BUILD
	if (pa->snd_card_asihpi)
		snd_asihpi_unbind(pa);
#endif

	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_DELETE_ADAPTER);
	hm.wAdapterIndex = pa->wAdapterIndex;
	HPI_MessageEx(&hm, &hr, HOWNER_KERNEL);

	/* unmap PCI memory space, mapped during device init. */
	for (idx = 0; idx < HPI_MAX_ADAPTER_MEM_SPACES; idx++) {
		if (pa->apRemappedMemBase[idx]) {
			iounmap(pa->apRemappedMemBase[idx]);
			pa->apRemappedMemBase[idx] = NULL;
		}
	}

	if (pa->pBuffer)
		vfree(pa->pBuffer);

	pci_set_drvdata(pci_dev, NULL);
	printk(KERN_INFO
	       "PCI device (%04x:%04x,%04x:%04x,%04x), HPI index # %d, removed.\n",
	       pci_dev->vendor, pci_dev->device, pci_dev->subsystem_vendor,
	       pci_dev->subsystem_device, pci_dev->devfn, pa->wAdapterIndex);
}

/* Module device table expects ints not pointers
 * Avoid having to cast the function pointers to long int
 */
#define HPI_4000 0x4000
#define HPI_6000 0x6000
#define HPI_6205 0x6205
#define HPI_COMMON 0
static struct pci_device_id asihpi_pci_tbl[] = {
#include "hpipcida.h"
};

MODULE_DEVICE_TABLE(pci, asihpi_pci_tbl);

static struct pci_driver asihpi_pci_driver = {
	.name = "asihpi",
	.id_table = asihpi_pci_tbl,
	.probe = adapter_probe,
	.remove = __devexit_p(adapter_remove),
};

static struct class *asihpi_class;

static void hpimod_cleanup(int stage)
{

	HPI_MESSAGE hm;
	HPI_RESPONSE hr;

	HPI_DEBUG_LOG0(DEBUG, "cleanup_module\n");

	switch (stage) {
	case 3:
		class_device_destroy(asihpi_class, MKDEV(major, 0));
		class_destroy(asihpi_class);
	case 2:
		unregister_chrdev(major, "asihpi");
	case 1:
		pci_unregister_driver(&asihpi_pci_driver);
	case 0:
		HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM,
				HPI_SUBSYS_DRIVER_UNLOAD);
		HPI_MessageEx(&hm, &hr, HOWNER_KERNEL);
	}
}

static void __exit hpimod_exit(void)
{
	hpimod_cleanup(3);
}

static int __init hpimod_init(void)
{
	int status, i;
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	u32 dwVersion = 0;

	for (i = 0; i < HPI_MAX_ADAPTERS; i++) {
		init_MUTEX(&adapters[i].sem);
	}

	/* HPI_DebugLevelSet(debug); now set directly as module param */
	printk(KERN_INFO "ASIHPI driver %s debug=%d \n",
	       __stringify(DRIVER_VERSION), hpiDebugLevel);
	HPI_SubSysGetVersionEx(NULL, &dwVersion);
	printk(KERN_INFO "SubSys Version=%d.%02d.%02d\n",
	       HPI_VER_MAJOR(dwVersion),
	       HPI_VER_MINOR(dwVersion), HPI_VER_RELEASE(dwVersion));

	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_DRIVER_LOAD);
	HPI_MessageEx(&hm, &hr, HOWNER_KERNEL);

	/* old version of below fn returned +ve number of devices, -ve error */
	if ((status = pci_register_driver(&asihpi_pci_driver)) < 0) {
		HPI_DEBUG_LOG1(ERROR, "HPI: pci_register_driver returned %d\n",
			       status);
		hpimod_cleanup(0);
		return status;
	}

	/* note need to remove this if we want driver to stay loaded with no devices
	   and devices can be hotplugged later
	 */
	if (!adapter_count) {
		HPI_DEBUG_LOG0(INFO, "No adapters found");
		hpimod_cleanup(1);
		return -ENODEV;
	}

	if ((status = register_chrdev(major, "asihpi", &hpi_fops)) < 0) {
		printk(KERN_ERR
		       "HPI: failed with error %d for major number %d\n",
		       -status, major);
		hpimod_cleanup(1);
		return -EIO;
	}

	if (!major)		/* Use dynamically allocated major number. */
		major = status;

	/* would like to create device in "sound" class (usually created by alsa)
	   but don't know how */
	asihpi_class = class_create(THIS_MODULE, "asihpi");
	if (IS_ERR(asihpi_class)) {
		printk(KERN_ERR "Error creating asihpi class.\n");
		hpimod_cleanup(2);
	} else {
		class_device_create(asihpi_class, NULL, MKDEV(major, 0), NULL,
				    "asihpi");
	}
	return 0;

}

module_init(hpimod_init)
    module_exit(hpimod_exit)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))
/* exported symbols for radio-asihpi */
    EXPORT_SYMBOL(HPI_Message);
#endif

/***********************************************************
*/
