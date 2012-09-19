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
#include "hpipci.h"
#include "hpidebug.h"
#include "hpimsgx.h"
#include <linux/slab.h>
#include <linux/module.h>	// MODULE_
#include <linux/pci.h>		// pci_find_device
#include <linux/interrupt.h>	// local_bh_*
#include <linux/version.h>
#include <asm/uaccess.h>
#include <linux/stringify.h>

#define TIME_HPI_MESSAGES 0

/* If this driver is only going to be accessed via the ioctl (i.e. not from ALSA driver)
   then spinlocks are not needed.
*/
#ifndef USE_SPINLOCK
#   define USE_SPINLOCK 1
#endif

/*  copy_to_user and friends can be used inside semaphore, but not inside spinlock 
    in which case, data must be copied to a local buffer outside the spinlock
*/
#if (USE_SPINLOCK)
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

/* hpiman.c::HPI_Message() is renamed to hpi_message for linux kernel compile to
   allow interception by a new version (see below).
*/

void hpi_message(const HPI_HSUBSYS * psubsys, HPI_MESSAGE * phm,
		 HPI_RESPONSE * phr);

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
static int debug = 0;
#if COPY_TO_LOCAL
static int bufsize = HPIMOD_DEFAULT_BUF_SIZE;
#endif

module_param(major, int, 0444);
MODULE_PARM_DESC(major, "Device major number");
module_param(debug, int, 0444);
MODULE_PARM_DESC(debug, "Debug level for Audioscience HPI 0=none 3=verbose");
#if COPY_TO_LOCAL
module_param(bufsize, int, 0444);
MODULE_PARM_DESC(bufsize,
		 "Buffer size to allocate for data transfer from HPI ioctl ");
#endif

static int hpi_init(void);
/* Spinlocks (interrupt disabling on uniprocessor) supposed to prevent contention within and between ALSA 
   or other kernel interrupt contexts and single userspace HPI program that made it past the semaphore.
   These are also needed when multiple ALSA streams are running
*/

typedef struct {
#if (USE_SPINLOCK)
	spinlock_t spinlock;
	unsigned long flags;
#endif
	/* Semaphores prevent contention for one card between multiple user programs (via ioctl) */
	struct semaphore sem;
	u16 type;

	char *pBuffer;
} adapter_t;

/* List of adapters found */
static u16 numAdapters;
static adapter_t adapters[HPI_MAX_ADAPTERS];

#if (TIME_HPI_MESSAGES)
static struct timeval t1;

#define START_TIME do_gettimeofday(&t1);
#define ELAPSED_TIME do {			\
		suseconds_t message_usec = 0;	\
		struct timeval t2;		\
		do_gettimeofday(&t2);		\
		message_usec = (t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec; \
		HPI_PRINT_DEBUG(KERN_INFO "ASIHPI message %ldus\n", message_usec); \
	} while (0)
#else
#define START_TIME
#define ELAPSED_TIME
#endif

/* Wrapper function to HPI_Message to enable dumping of the
   message and response types.  
*/
void HPI_MessageF(HPI_MESSAGE * phm, HPI_RESPONSE * phr, struct file *file)
{
	int nAdapter = phm->wAdapterIndex;

	if ((nAdapter >= HPI_MAX_ADAPTERS || nAdapter < 0) &&
	    (phm->wObject != HPI_OBJ_SUBSYSTEM)) {
		phr->wError = HPI_ERROR_INVALID_OBJ_INDEX;
	} else {
		SPIN_LOCK_IRQSAVE(&adapters[nAdapter].spinlock,
				  adapters[nAdapter].flags);
		START_TIME;
		HPIMSGX_MessageEx(0, phm, phr, (void *)file);
		ELAPSED_TIME;
		SPIN_UNLOCK_IRQRESTORE(&adapters[nAdapter].spinlock,
				       adapters[nAdapter].flags);
	}
	HPI_DEBUG_RESPONSE(phr);
}

/* This is called from hpifunc.c functions, called by ALSA (or other kernel process)
   In this case there is no file descriptor available for the message cache code
*/
#define HOWNER_KERNEL ((void *)-1)
void HPI_Message(const HPI_HSUBSYS * psubsys, HPI_MESSAGE * phm,
		 HPI_RESPONSE * phr)
{
	HPI_MessageF(phm, phr, HOWNER_KERNEL);
}

static int hpi_open(struct inode *inode, struct file *file)
{
	unsigned int minor = MINOR(inode->i_rdev);

	if (minor > 0)
		return -ENODEV;

	HPI_PRINT_DEBUG("hpi_open %p\n", (void *)file);

	return 0;
}

static int hpi_release(struct inode *inode, struct file *file)
{
	HPI_PRINT_DEBUG("hpi_release %p\n", (void *)file);
	HPIMSGX_MessageExAdapterCleanup(HPIMSGX_ALLADAPTERS, (void *)file);
	return 0;
}

static int
hpi_ioctl(struct inode *inode, struct file *file,
	  unsigned int cmd, unsigned long arg)
{
	struct hpi_ioctl_linux *phpi_ioctl_data;
	HPI_MESSAGE *phm;
	HPI_RESPONSE *phr;
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	u32 uncopied_bytes;

#if  (COPY_TO_LOCAL!=1)
	mm_segment_t fs;
#endif
#if (TIME_HPI_MESSAGES)
	struct timeval t2, t1;
	suseconds_t copy_usec = 0;
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
	if ((cmd == HPI_IOCTL_HARDRESET) /*&& (debug > 0) */ ) {
		/* reset the counter to 1, to allow unloading in case
		   of problems. Use 1, not 0, because the invoking
		   process has the device open.
		 */
		while (MOD_IN_USE)
			MOD_DEC_USE_COUNT;
		MOD_INC_USE_COUNT;
		return 0;
	}
#endif

	if (cmd != HPI_IOCTL_LINUX)
		return -EINVAL;

	phpi_ioctl_data = (void *)arg;

	/* Read the message and response pointers from user space.  */
	get_user(phm, &phpi_ioctl_data->phm);
	get_user(phr, &phpi_ioctl_data->phr);

	/* Now read the message size and data from user space.  */
	get_user(hm.wSize, (u16 *) phm);
	uncopied_bytes = copy_from_user(&hm, phm, hm.wSize);
	if (uncopied_bytes)
		return -EFAULT;

	hr.wSize = 0;
	// Response gets filled in either by copy from cache, or by HPI_Message()
	{
		/* Dig out any pointers embedded in the message.  */
		u16 *ptr = 0;
		u32 size = 0;

		int wrflag = -1;	/* -1=no data 0=read from user mem, 1=write to user mem */
		int nAdapter = phm->wAdapterIndex;
		switch (hm.wFunction) {
		case HPI_OSTREAM_WRITE:
		case HPI_ISTREAM_READ:
			ptr = (u16 *) hm.u.d.u.Data.dwpbData;
			size = hm.u.d.u.Data.dwDataSize;
			//printk("HPI data size %ld\n",size);

#if (COPY_TO_LOCAL)
			hm.u.d.u.Data.dwpbData =
			    (u32) adapters[nAdapter].pBuffer;
			if (!hm.u.d.u.Data.dwpbData) {
				HPI_PRINT_ERROR("NULL pBuffer for adapter %d\n",
						nAdapter);
				return -EFAULT;
			}

			if (size > bufsize) {
				size = bufsize;
				hm.u.d.u.Data.dwDataSize = size;
			}
#endif

			if (hm.wFunction == HPI_ISTREAM_READ)	// from card, WRITE to user mem
				wrflag = 1;
			else
				wrflag = 0;
			break;

#ifdef HAS_AES18
		case HPI_CONTROL_SET_STATE:
			ptr =
			    (u16 *) hm.u.cx.u.aes18tx_send_message.dwpbMessage;
			size = hm.u.cx.u.aes18tx_send_message.wMessageLength;
			wrflag = 0;
#if (COPY_TO_LOCAL)
			(char *)hm.u.cx.u.aes18tx_send_message.dwpbMessage =
			    adapters[nAdapter].pBuffer;
			if (!hm.u.cx.u.aes18tx_send_message.dwpbMessage) {
				HPI_PRINT_ERROR("NULL pBuffer for adapter %d\n",
						nAdapter);
				return -EFAULT;
			}
#endif
			break;

		case HPI_CONTROL_GET_STATE:
			ptr = (u16 *) hm.u.cx.u.aes18rx_get_message.dwpbMessage;
			size =
			    (u16 *) hm.u.cx.u.aes18rx_get_message.
			    wMessageLength;
			wrflag = 1;
#if (COPY_TO_LOCAL)
			(char *)hm.u.cx.u.aes18rx_get_message.dwpbMessage =
			    adapters[nAdapter].pBuffer;
			if (!hm.u.cx.u.aes18rx_get_message.dwpbMessage) {
				HPI_PRINT_ERROR("NULL pBuffer for adapter %d\n",
						nAdapter);
				return -EFAULT;
			}
#endif
			break;
#endif

		default:
			break;
		}

		if ((nAdapter >= HPI_MAX_ADAPTERS || nAdapter < 0) &&
		    (phm->wObject != HPI_OBJ_SUBSYSTEM))
			phr->wError = HPI_ERROR_INVALID_OBJ_INDEX;
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

# if (TIME_HPI_MESSAGES)
				do_gettimeofday(&t1);
# endif
				uncopied_bytes =
				    copy_from_user(adapters[nAdapter].pBuffer,
						   ptr, size);

# if (TIME_HPI_MESSAGES)
				do_gettimeofday(&t2);
				copy_usec =
				    (t2.tv_sec - t1.tv_sec) * 1000000 +
				    t2.tv_usec - t1.tv_usec;
				HPI_PRINT_DEBUG(KERN_INFO
						"ASIHPI play copy %ldus\n",
						copy_usec);
# endif
				if (uncopied_bytes) {
					HPI_PRINT_DEBUG
					    (KERN_WARNING
					     "Missed %d of %d bytes from user\n",
					     uncopied_bytes, size);
				}
			}

			HPI_MessageF(&hm, &hr, file);

			if (wrflag == 1) {
				u32 uncopied_bytes;

# if (TIME_HPI_MESSAGES)
				do_gettimeofday(&t1);
# endif
				uncopied_bytes =
				    copy_to_user(ptr,
						 adapters[nAdapter].pBuffer,
						 size);

# if (TIME_HPI_MESSAGES)
				do_gettimeofday(&t2);
				copy_usec =
				    (t2.tv_sec - t1.tv_sec) * 1000000 +
				    t2.tv_usec - t1.tv_usec;
				HPI_PRINT_DEBUG(KERN_INFO
						"ASIHPI rec copy %ldus\n",
						copy_usec);
# endif

				if (uncopied_bytes) {
					HPI_PRINT_DEBUG(KERN_WARNING
							"Missed %d of %d bytes to user\n",
							uncopied_bytes, size);
				}
			}
#endif				// else COPY_TO_LOCAL==1

			up(&adapters[nAdapter].sem);
		}
	}

	/* Copy the response back to user space.  */
	uncopied_bytes = copy_to_user(phr, &hr, hr.wSize);
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
	.ioctl = hpi_ioctl,
	.open = hpi_open,
	.release = hpi_release
};
#endif

void HpiOs_LockedMem_FreeAll(void);
void __exit cleanup_module(void)
{
	int a;
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;

	HPI_PRINT_DEBUG("cleanup_module\n");
	unregister_chrdev(major, "asihpi");

	// Close all adapters - all busmaster activity will cease.
	HPI_InitMessage(&hm, HPI_OBJ_ADAPTER, HPI_ADAPTER_CLOSE);
	for (a = 0; a < HPI_MAX_ADAPTERS; a++) {
		if (adapters[a].type != 0) {
			hm.wAdapterIndex = a;
			HPI_Message(0, &hm, &hr);
		}
	}

	// delete all adapters.
	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_DELETE_ADAPTER);
	for (a = 0; a < HPI_MAX_ADAPTERS; a++) {
		if (adapters[a].type != 0) {
			hm.wAdapterIndex = a;
			HPI_Message(0, &hm, &hr);
#if (COPY_TO_LOCAL)
			if (adapters[a].pBuffer)
				vfree(adapters[a].pBuffer);
#endif
		}
	}

	HpiOs_LockedMem_FreeAll();

	/*shouldn't we have an HPI_FreeSubSys call here? */
}

void H400_AdapterIndex(HPI_RESOURCE * res, short *wAdapterIndex);
void H600_AdapterIndex(HPI_RESOURCE * res, short *wAdapterIndex);
void H620_AdapterIndex(HPI_RESOURCE * res, short *wAdapterIndex);

void HPI_AdapterIndex(const HPI_HSUBSYS * phSubSys, struct pci_dev *pci_dev,
		      const struct pci_device_id *pci_id, short *wAdapterIndex)
{
	HPI_RESOURCE hResource;
	int idx;

	*wAdapterIndex = -1;

	for (idx = 0; idx < HPI_MAX_ADAPTER_MEM_SPACES; idx++) {
		hResource.r.Pci.dwMemBase[idx] =
		    pci_resource_start(pci_dev, idx);
		/* hResource.r.Pci.dwMemLength[idx] = pci_resource_len( pci_dev, idx ); */
	}

	hResource.wBusType = HPI_BUS_PCI;
	hResource.r.Pci.wBusNumber = pci_dev->bus->number;
	hResource.r.Pci.wVendorId = (u16) pci_dev->vendor;
	hResource.r.Pci.wDeviceId = (u16) pci_dev->device;
	hResource.r.Pci.wSubSysVendorId =
	    (u16) (pci_dev->subsystem_vendor & 0xffff);
	hResource.r.Pci.wSubSysDeviceId =
	    (u16) (pci_dev->subsystem_device & 0xffff);
	hResource.r.Pci.wDeviceNumber = pci_dev->devfn;
	hResource.r.Pci.wInterrupt = pci_dev->irq;

	H620_AdapterIndex(&hResource, wAdapterIndex);
	if (*wAdapterIndex >= 0)
		return;

	H400_AdapterIndex(&hResource, wAdapterIndex);
	if (*wAdapterIndex >= 0)
		return;

	H600_AdapterIndex(&hResource, wAdapterIndex);
	if (*wAdapterIndex >= 0)
		return;

}

int __init init_module(void)
{
	int status;

	printk(KERN_INFO "ASIHPI driver %s debug=%d ",
	       __stringify(DRIVER_VERSION), debug);
	printk("Spinlock on=%d Local copy=%d\n", USE_SPINLOCK, COPY_TO_LOCAL);

	HPI_DebugLevelSet(debug);

	if ((status = register_chrdev(major, "asihpi", &hpi_fops)) < 0) {
		printk(KERN_ERR
		       "HPI: failed with error %d for major number %d\n",
		       -status, major);
		return -EIO;
	}

	if (!major)		// Use dynamically allocated major number.
		major = status;

	status = hpi_init();

	if (status)
		cleanup_module();

	return status;
}

static int hpi_init(void)
{
	int i, err;
	u16 adapterList[HPI_MAX_ADAPTERS];
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

	/* Init the HPI and find/init all the adapters present.
	   Open all adapters and streams.  */
	if (!HPI_SubSysCreate()) {
		HPI_PRINT_ERROR("HPI subsys create failed.\n");
		return -EIO;
	}
	/* is this the right place for this call?  SHould it be attached to another file operation? */
	HPI_PRINT_DEBUG("ExAdapterReset\n");
	HPIMSGX_MessageExAdapterReset(HPIMSGX_ALLADAPTERS);
	HPI_PRINT_DEBUG("ExAdapterInit\n");
	if ((err = HPIMSGX_MessageExAdapterInit(0, NULL, NULL)) != 0) {
		HPI_PRINT_ERROR("%d from ExAdapterInit\n", err);
		//      return -ENODEV;
	}

	{

		HPI_SubSysFindAdapters(0, &numAdapters, adapterList,
				       HPI_MAX_ADAPTERS);

		if (numAdapters == 0) {
			HPI_PRINT_ERROR("No AudioScience adapters found\n");
			return -ENODEV;
		} else {
			for (i = 0; i < HPI_MAX_ADAPTERS; i++) {
				adapters[i].type = adapterList[i];
				if (adapters[i].type != 0) {
#if (COPY_TO_LOCAL)
					if (!
					    (adapters[i].pBuffer =
					     vmalloc(bufsize))) {
						HPI_PRINT_ERROR
						    ("HPI could not allocate kernel buffer size %d\n",
						     bufsize);
						return -ENOMEM;	//FIXME we fail to free the resources we allocated
					}
#endif
					printk(KERN_INFO
					       "Adapter %d is ASI%4x\n", i,
					       adapters[i].type);
				}
			}
		}
	}

	return 0;

}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))

EXPORT_SYMBOL(HPI_SampleClock_SetSource);
EXPORT_SYMBOL(HPI_LevelGetGain);
EXPORT_SYMBOL(HPI_InStreamStart);
EXPORT_SYMBOL(HPI_Multiplexer_GetSource);
EXPORT_SYMBOL(HPI_FormatCreate);
EXPORT_SYMBOL(HPI_InStreamQueryFormat);
EXPORT_SYMBOL(HPI_VolumeGetGain);
EXPORT_SYMBOL(HPI_GetErrorText);
EXPORT_SYMBOL(HPI_OutStreamOpen);
EXPORT_SYMBOL(HPI_SampleClock_GetSource);
EXPORT_SYMBOL(HPI_SubSysGetVersion);
EXPORT_SYMBOL(HPI_InStreamSetFormat);
EXPORT_SYMBOL(HPI_OutStreamWrite);
EXPORT_SYMBOL(HPI_Multiplexer_QuerySource);
EXPORT_SYMBOL(HPI_OutStreamStart);
EXPORT_SYMBOL(HPI_LevelSetGain);
EXPORT_SYMBOL(HPI_OutStreamClose);
EXPORT_SYMBOL(HPI_SampleClock_SetSampleRate);
EXPORT_SYMBOL(HPI_MixerOpen);
EXPORT_SYMBOL(HPI_InStreamReset);
EXPORT_SYMBOL(HPI_SubSysFindAdapters);
EXPORT_SYMBOL(HPI_OutStreamReset);
EXPORT_SYMBOL(HPI_AdapterOpen);
EXPORT_SYMBOL(HPI_SubSysCreate);
EXPORT_SYMBOL(HPI_SampleClock_GetSampleRate);
EXPORT_SYMBOL(HPI_AdapterGetInfo);
EXPORT_SYMBOL(HPI_OutStreamQueryFormat);
EXPORT_SYMBOL(HPI_MeterGetRms);
EXPORT_SYMBOL(HPI_ChannelModeGet);
EXPORT_SYMBOL(HPI_InStreamRead);
EXPORT_SYMBOL(HPI_VolumeQueryRange);
EXPORT_SYMBOL(HPI_Multiplexer_SetSource);
EXPORT_SYMBOL(HPI_VolumeSetGain);
EXPORT_SYMBOL(HPI_ChannelModeSet);
EXPORT_SYMBOL(HPI_InStreamStop);
EXPORT_SYMBOL(HPI_OutStreamGetInfoEx);
EXPORT_SYMBOL(HPI_InStreamOpen);
EXPORT_SYMBOL(HPI_OutStreamStop);
EXPORT_SYMBOL(HPI_InStreamGetInfoEx);
EXPORT_SYMBOL(HPI_MixerGetControlByIndex);
EXPORT_SYMBOL(HPI_InStreamClose);
EXPORT_SYMBOL(HPI_MixerGetControl);

EXPORT_SYMBOL(HPI_Tuner_SetGain);
EXPORT_SYMBOL(HPI_Tuner_GetGain);
EXPORT_SYMBOL(HPI_Tuner_SetBand);
EXPORT_SYMBOL(HPI_Tuner_GetBand);
EXPORT_SYMBOL(HPI_Tuner_SetFrequency);
EXPORT_SYMBOL(HPI_Tuner_GetFrequency);
EXPORT_SYMBOL(HPI_ControlQuery);

EXPORT_SYMBOL(HPI_SubSysFree);
EXPORT_SYMBOL(HPI_Tuner_GetRFLevel);
EXPORT_SYMBOL(HPI_AdapterClose);
EXPORT_SYMBOL(HPI_MixerClose);

EXPORT_SYMBOL(HPI_InStreamHostBufferAllocate);
EXPORT_SYMBOL(HPI_InStreamHostBufferFree);
EXPORT_SYMBOL(HPI_OutStreamHostBufferAllocate);
EXPORT_SYMBOL(HPI_OutStreamHostBufferFree);

EXPORT_SYMBOL(HPI_AESEBU_Receiver_GetSource);
EXPORT_SYMBOL(HPI_AESEBU_Receiver_SetSource);
EXPORT_SYMBOL(HPI_AESEBU_Transmitter_SetFormat);
EXPORT_SYMBOL(HPI_AESEBU_Transmitter_GetFormat);
EXPORT_SYMBOL(HPI_AESEBU_Transmitter_SetClockSource);
EXPORT_SYMBOL(HPI_AESEBU_Transmitter_GetClockSource);

EXPORT_SYMBOL(HPI_StreamEstimateBufferSize);

EXPORT_SYMBOL(HPI_AdapterIndex);

#endif

/***********************************************************
*/
