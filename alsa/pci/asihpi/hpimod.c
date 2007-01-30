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
#include "hpi.h"
#include "hpidebug.h"
#include "hpimsgx.h"
#include <linux/slab.h>
#include <linux/module.h>	// MODULE_
#include <linux/pci.h>		// pci_find_device
#include <linux/interrupt.h>	// local_bh_*
#include <linux/version.h>
#include <asm/uaccess.h>
#include <linux/stringify.h>

int snd_asihpi_bind(adapter_t * hpi_card);
void snd_asihpi_unbind(adapter_t * hpi_card);

/* If this driver is only going to be accessed via the ioctl (i.e. not from ALSA driver)
   then spinlocks are not needed.
*/
#ifndef USE_SPINLOCK
#   define USE_SPINLOCK 1
#endif

/*  copy_to_user and friends can be used inside semaphore, but not inside spinlock
    in which case, data must be copied to a local buffer outside the spinlock
*/
#ifdef HPI_LOCKING
#define COPY_TO_LOCAL 1
#endif

#if (USE_SPINLOCK) && ! defined (HPI_LOCKING)
/* HPI_LOCKING causes spinlocks in this file to be defined to noops,
 because locking is down inside hpi
*/
#   define COPY_TO_LOCAL 1
#    define SPIN_LOCK_INIT spin_lock_init
#    define SPIN_LOCK_IRQSAVE spin_lock_irqsave
#   define SPIN_UNLOCK_IRQRESTORE spin_unlock_irqrestore
#else
#    define SPIN_LOCK_INIT(a)
#   define SPIN_LOCK_IRQSAVE(a,b)
#   define SPIN_UNLOCK_IRQRESTORE(a,b)
#   ifndef COPY_TO_LOCAL
#      define COPY_TO_LOCAL 0
#   endif
#endif

/* Report elapsed time of messages to kernel log */
#ifndef TIME_HPI_MESSAGES
#   define TIME_HPI_MESSAGES 0
#endif

#ifndef HPIMOD_DEFAULT_BUF_SIZE
#   define HPIMOD_DEFAULT_BUF_SIZE 192000
#endif

#ifndef KERNEL_VERSION
#  define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0))
# if (USE_SPINLOCK)
#  error "ALSA (requiring spinlocks) not currently supported on Linux version < 2.4.0 - please recompile without ALSA support"
# endif
#endif

#ifdef MODULE_LICENSE
// See the ``GPL LICENSED MODULES AND SYMBOLS'' section in the ``insmod'' manpage.
MODULE_LICENSE("GPL");
#endif

MODULE_AUTHOR("AudioScience <support@audioscience.com>");
MODULE_DESCRIPTION("AudioScience HPI");

static int major = 0;
#if COPY_TO_LOCAL
static int bufsize = HPIMOD_DEFAULT_BUF_SIZE;
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
/* old style parameters */
MODULE_PARM(major, "i");
MODULE_PARM(hpiDebugLevel, "0-6i");

#if COPY_TO_LOCAL
MODULE_PARM(bufsize, "i");
#endif

#else				/* new style params */
module_param(major, int, S_IRUGO);
#if COPY_TO_LOCAL
module_param(bufsize, int, S_IRUGO);
#endif
/* Allow the debug level to be changed after module load.
 E.g.   echo 2 > /sys/module/asihpi/parameters/hpiDebugLevel
*/
module_param(hpiDebugLevel, int, S_IRUGO | S_IWUSR);
#endif

MODULE_PARM_DESC(major, "Device major number");
MODULE_PARM_DESC(hpiDebugLevel,
		 "Debug level for Audioscience HPI 0=none 7=verbose");
MODULE_PARM_DESC(bufsize,
		 "Buffer size to allocate for data transfer from HPI ioctl ");

static int hpi_init(void);
/* Spinlocks (interrupt disabling on uniprocessor) supposed to prevent contention within and between ALSA
   or other kernel interrupt contexts and single userspace HPI program that made it past the semaphore.
   These are also needed when multiple ALSA streams are running
*/

/* List of adapters found */
static int adapter_count = 0;
static adapter_t adapters[HPI_MAX_ADAPTERS];

#define HOWNER_KERNEL ((void *)-1)

/* Ludwig Schwardt suggests using current->tgid instead of *file as the owner
identifier in issue #340 */

/* Wrapper function to HPI_Message to enable dumping of the
   message and response types.
*/
static void HPI_MessageF(HPI_MESSAGE * phm,
			 HPI_RESPONSE * phr, struct file *file)
{
	int nAdapter = phm->wAdapterIndex;

#if 0
	printk(KERN_INFO "HPI_MessageF ");
	if (in_interrupt())
		printk("in interrupt ");
	if (in_softirq())
		printk("in softirq ");
	if (in_irq())
		printk("in irq ");
	if (in_atomic())
		printk("in atomic ");
	if (irqs_disabled())
		printk("interrupts disabled ");
	printk("\n");
#endif
	if ((nAdapter >= HPI_MAX_ADAPTERS || nAdapter < 0) &&
	    (phm->wObject != HPI_OBJ_SUBSYSTEM)) {
		phr->wError = HPI_ERROR_INVALID_OBJ_INDEX;
	} else {
		HPI_MessageEx(phm, phr, file);
	}
}

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

//      HPI_DEBUG_LOG2(INFO,"hpi_open file %p, pid %d\n", file, current->pid);

	return 0;
}

static int hpi_release(struct inode *inode, struct file *file)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;

//      HPI_DEBUG_LOG2(INFO,"hpi_release file %p, pid %d\n", file, current->pid);
	//close the subsystem just in case the application forgot to..
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

#if  (COPY_TO_LOCAL!=1)
	mm_segment_t fs;
#endif
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
	// Response gets filled in either by copy from cache, or by HPI_Message()
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
			//printk("HPI data size %ld\n",size);

#if (COPY_TO_LOCAL)
			hm.u.d.u.Data.pbData = pa->pBuffer;
			/*
			   if (size > bufsize) {
			   size = bufsize;
			   hm.u.d.u.Data.dwDataSize = size;
			   }
			 */
#endif

			if (hm.wFunction == HPI_ISTREAM_READ)	// from card, WRITE to user mem
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

#if  (COPY_TO_LOCAL!=1)
			fs = get_fs();
			set_fs(get_ds());
			if (!access_ok
			    (wrflag ? VERIFY_WRITE : VERIFY_READ, ptr, size)) {
				set_fs(fs);
				up(&adapters[nAdapter].sem);
				return -EFAULT;
			}

			HPI_MessageF(&hm, &hr, file);

			set_fs(fs);
#else				//  COPY_TO_LOCAL==1
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
				//u32 uncopied_bytes;

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
#endif				// else COPY_TO_LOCAL==1

			up(&adapters[nAdapter].sem);
		}
	}

	//on return response size must be set
	if (!hr.wSize)
		return -EFAULT;

	/* Copy the response back to user space.  */
	uncopied_bytes = copy_to_user(phr, &hr, sizeof(hr));
	if (uncopied_bytes)
		return -EFAULT;
	return 0;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0))
static struct file_operations hpi_fops = {
	NULL,			/* llseek */
	NULL,			/* read */
	NULL,			/* write */
	NULL,			/* readdir */
	NULL,			/* poll */
	hpi_ioctl,		/* ioctl */
	NULL,			/* mmap */
	hpi_open,		/* open */
	NULL,			/* flush */
	hpi_release,		/* release */
	NULL,			/* fsync */
	NULL,			/* fasync */
	NULL,			/* check_media_change */
	NULL,			/* revalidate */
	NULL			/* lock */
};
#else
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
#endif

void HpiOs_LockedMem_FreeAll(void);

static int hpi_init(void)
{
	int i;
	/*  30 APRIL 2001: [AGE and REN]: This compiles cleanly on all
	   of 2.2.14-5.0, 2.2.17 and 2.4.4 versions of the kernel. In
	   pre-2.4 versions, init_MUTEX is a macro. In 2.4, it is an
	   inline function. In either case, it portably initializes
	   the mutex.
	 */

	for (i = 0; i < HPI_MAX_ADAPTERS; i++) {
		SPIN_LOCK_INIT(&adapters[i].spinlock);
		init_MUTEX(&adapters[i].sem);
	}

	return 0;
}

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
		return -ENODEV;
	}

	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_CREATE_ADAPTER);
	HPI_InitResponse(&hr, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_CREATE_ADAPTER,
			 HPI_ERROR_PROCESSING_MESSAGE);
	// set the adapter index to an invalid value
	hm.wAdapterIndex = -1;

	// fill in HPI_PCI information from kernel provided information
	for (idx = 0; idx < HPI_MAX_ADAPTER_MEM_SPACES; idx++) {
		HPI_DEBUG_LOG4(DEBUG, "Resource %d %s %lx-%lx\n", idx,
			       pci_dev->resource[idx].name,
			       (unsigned long)pci_dev->resource[idx].start,
			       (unsigned long)pci_dev->resource[idx].end);

		memlen = pci_resource_len(pci_dev, idx);
		if (memlen) {
			adapter.apRemappedMemBase[idx] =
			    ioremap(pci_resource_start(pci_dev, idx), memlen);
			if (!adapter.apRemappedMemBase[idx]) {
				HPI_DEBUG_LOG0(ERROR,
					       "ioremap failed, aborting\n");
				//unmap previously mapped pci mem space
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
	Pci.pOsData = (void *)pci_dev;

	hm.u.s.Resource.wBusType = HPI_BUS_PCI;
	hm.u.s.Resource.r.Pci = &Pci;

	//call CreateAdapterObject on the relevant hpi module
	HPI_MessageEx(&hm, &hr, HOWNER_KERNEL);

#if (COPY_TO_LOCAL)
	if ((adapter.pBuffer = vmalloc(bufsize)) == NULL) {
		HPI_DEBUG_LOG1(ERROR,
			       "HPI could not allocate kernel buffer size %d\n",
			       bufsize);
		goto err;
	}
#endif

	if (hr.wError == 0) {
		adapter.wAdapterIndex = hr.u.s.wAdapterIndex;
		hm.wAdapterIndex = hr.u.s.wAdapterIndex;

		err = HPI_AdapterOpen(NULL, hr.u.s.wAdapterIndex);
		if (err)
			goto err;

		printk(KERN_INFO "Probe found adapter ASI%04X HPI index #%d.\n",
		       hr.u.s.awAdapterList[hr.u.s.wAdapterIndex],
		       hr.u.s.wAdapterIndex);

		adapters[hr.u.s.wAdapterIndex] = adapter;
		SPIN_LOCK_INIT(&adapters[hr.u.s.wAdapterIndex].spinlock);
		init_MUTEX(&adapters[hr.u.s.wAdapterIndex].sem);

		adapters[hr.u.s.wAdapterIndex].snd_card_asihpi = NULL;
#ifdef ALSA_BUILD
		if (snd_asihpi_bind(&adapters[hr.u.s.wAdapterIndex]))
			goto err;
#endif

		pci_set_drvdata(pci_dev, &adapters[hr.u.s.wAdapterIndex]);
		adapter_count++;
		return 0;
	}

      err:
	// missing code to delete adapter if error happens after creation (unlikely)
	for (idx = 0; idx < HPI_MAX_ADAPTER_MEM_SPACES; idx++) {
		if (adapter.apRemappedMemBase[idx]) {
			iounmap(adapter.apRemappedMemBase[idx]);
			adapter.apRemappedMemBase[idx] = NULL;
		}
	}

#if (COPY_TO_LOCAL)
	if (adapter.pBuffer)
		vfree(adapter.pBuffer);
#endif

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

	// unmap PCI memory space, mapped during device init.
	for (idx = 0; idx < HPI_MAX_ADAPTER_MEM_SPACES; idx++) {
		if (pa->apRemappedMemBase[idx]) {
			iounmap(pa->apRemappedMemBase[idx]);
			pa->apRemappedMemBase[idx] = NULL;
		}
	}

#if (COPY_TO_LOCAL)
	if (pa->pBuffer)
		vfree(pa->pBuffer);
#endif

	pci_set_drvdata(pci_dev, NULL);
	printk(KERN_INFO
	       "PCI device (%04x:%04x,%04x:%04x,%04x), HPI index # %d, removed.\n",
	       pci_dev->vendor, pci_dev->device, pci_dev->subsystem_vendor,
	       pci_dev->subsystem_device, pci_dev->devfn, pa->wAdapterIndex);

}

// Module device table expects ints not pointers
// Avoid having to cast the function pointers to long int
#define HPI_4000 0x4000
#define HPI_6000 0x6000
#define HPI_6205 0x6205
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

static void hpimod_cleanup(int stage)
{

	HPI_MESSAGE hm;
	HPI_RESPONSE hr;

	HPI_DEBUG_LOG0(DEBUG, "cleanup_module\n");

	switch (stage) {
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
	hpimod_cleanup(2);
}

static int __init hpimod_init(void)
{
	int status;
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	u32 dwVersion = 0;

	hpi_init();

	/* HPI_DebugLevelSet(debug); now set directly as module param */
	printk(KERN_INFO "ASIHPI driver %s debug=%d ",
	       __stringify(DRIVER_VERSION), hpiDebugLevel);
	printk("Spinlock on=%d Local copy=%d\n", USE_SPINLOCK, COPY_TO_LOCAL);
	HPI_SubSysGetVersionEx(NULL, &dwVersion);
	printk(KERN_INFO "SubSys Version=%d.%02d.%02d\n",
	       HPI_VER_MAJOR(dwVersion),
	       HPI_VER_MINOR(dwVersion), HPI_VER_RELEASE(dwVersion));

	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_DRIVER_LOAD);
	HPI_MessageEx(&hm, &hr, HOWNER_KERNEL);

	// old version of below fn returned +ve number of devices, -ve error
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

	if (!major)		// Use dynamically allocated major number.
		major = status;

	return 0;

}

module_init(hpimod_init)
    module_exit(hpimod_exit)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))
// exported symbols for radio-asihpi
    EXPORT_SYMBOL(HPI_Message);
#endif

/***********************************************************
*/
