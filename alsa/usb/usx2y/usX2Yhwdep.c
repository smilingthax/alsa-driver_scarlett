#define __NO_VERSION__
/*
 * Driver for Tascam US-X2Y USB soundcards
 *
 * FPGA Loader + ALSA Startup
 *
 * Copyright (c) 2003 by Karsten Wiese <annabellesgarden@yahoo.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */
#define SND_NEED_USB_WRAPPER
#include <sound/driver.h>
#include <sound/core.h>
#include "usx2y.h"
#include <sound/memalloc.h>
#include "usbusx2y.h"
#include "usX2Yhwdep.h"


static void us428ctls_vm_open(struct vm_area_struct *area)
{
}

static void us428ctls_vm_close(struct vm_area_struct *area)
{
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
static struct page * us428ctls_vm_nopage(struct vm_area_struct *area, unsigned long address, int *type)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
static struct page * us428ctls_vm_nopage(struct vm_area_struct *area, unsigned long address, int no_share)
#else
static unsigned long us428ctls_vm_nopage(struct vm_area_struct *area, unsigned long address, int no_share)
#endif
{
	unsigned long offset;
	struct page * page;
	void *vaddr;

	snd_printd("ENTER, start %lXh, ofs %lXh, pgoff %ld, addr %lXh\n",
		   area->vm_start,
		   address - area->vm_start,
		   (address - area->vm_start) >> PAGE_SHIFT,
		   address);
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 3, 25)
	offset = area->vm_pgoff << PAGE_SHIFT;
#else
	offset = area->vm_offset;
#endif
	offset += address - area->vm_start;
	snd_assert((offset % PAGE_SIZE) == 0, return NOPAGE_OOM);
	vaddr = (char*)((usX2Ydev_t*)area->vm_private_data)->us428ctls_sharedmem + offset;
	page = virt_to_page(vaddr);
	get_page(page);
	snd_printd( "vaddr=%p made us428ctls_vm_nopage() return %p; offset=%lX\n", vaddr, page, offset);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
	if (type)
		*type = VM_FAULT_MINOR;
#endif

#ifndef LINUX_2_2
	return page;
#else
	/* why 2.2's kcomp.h redefines this? */
#ifdef page_address
#undef page_address
#endif
	return page_address(page);
#endif
}

static struct vm_operations_struct us428ctls_vm_ops = {
	.open = us428ctls_vm_open,
	.close = us428ctls_vm_close,
	.nopage = us428ctls_vm_nopage,
};

static int us428ctls_mmap(snd_hwdep_t * hw, struct file *filp, struct vm_area_struct *area)
{
	unsigned long	size = (unsigned long)(area->vm_end - area->vm_start);
	usX2Ydev_t	*us428 = (usX2Ydev_t*)hw->private_data;

	// FIXME this hwdep interface is used twice: fpga download and mmap for controlling Lights etc. Maybe better using 2 hwdep devs?
	// so as long as the device isn't fully initialised yet we return -EBUSY here.
 	if (!(((usX2Ydev_t*)hw->private_data)->chip_status & USX2Y_STAT_CHIP_INIT))
		return -EBUSY;

	/* if userspace tries to mmap beyond end of our buffer, fail */ 
        if (size > ((PAGE_SIZE - 1 + sizeof(us428ctls_sharedmem_t)) / PAGE_SIZE) * PAGE_SIZE) {
		snd_printd( "%lu > %lu\n", size, (unsigned long)sizeof(us428ctls_sharedmem_t)); 
                return -EINVAL;
	}

	if (!us428->us428ctls_sharedmem) {
		init_waitqueue_head(&us428->us428ctls_wait_queue_head);
		if(!(us428->us428ctls_sharedmem = snd_malloc_pages(sizeof(us428ctls_sharedmem_t), GFP_KERNEL)))
			return -ENOMEM;
		memset(us428->us428ctls_sharedmem, -1, sizeof(us428ctls_sharedmem_t));
		us428->us428ctls_sharedmem->CtlSnapShotLast = -2;
	}
	area->vm_ops = &us428ctls_vm_ops;
#ifdef VM_RESERVED
	area->vm_flags |= VM_RESERVED;
#endif
#ifndef LINUX_2_2
	area->vm_private_data = hw->private_data;
#else
	area->vm_private_data = (long)hw->private_data;
#endif
	return 0;
}

static unsigned int us428ctls_poll(snd_hwdep_t *hw, struct file *file, poll_table *wait)
{
	unsigned int	mask = 0;
	usX2Ydev_t	*us428 = (usX2Ydev_t*)hw->private_data;
	static unsigned	LastN;

	if (us428->chip_status & USX2Y_STAT_CHIP_HUP)
		return POLLHUP;

	poll_wait(file, &us428->us428ctls_wait_queue_head, wait);

	down(&us428->open_mutex);
	if (us428->us428ctls_sharedmem
	    && us428->us428ctls_sharedmem->CtlSnapShotLast != LastN) {
		mask |= POLLIN;
		LastN = us428->us428ctls_sharedmem->CtlSnapShotLast;
	}
	up(&us428->open_mutex);

	return mask;
}


static int usX2Y_hwdep_open(snd_hwdep_t *hw, struct file *file)
{
	return 0;
}

static int usX2Y_hwdep_release(snd_hwdep_t *hw, struct file *file)
{
	return 0;
}

static int usX2Y_hwdep_dsp_status(snd_hwdep_t *hw, snd_hwdep_dsp_status_t *info)
{
	static char *type_ids[USX2Y_TYPE_NUMS] = {
		[USX2Y_TYPE_122] = "us122",
		[USX2Y_TYPE_224] = "us224",
		[USX2Y_TYPE_428] = "us428",
	};
	int id = -1;

	switch (((usX2Ydev_t*)hw->private_data)->chip.dev->descriptor.idProduct) {
	case USB_ID_US122:
		id = USX2Y_TYPE_122;
		break;
	case USB_ID_US224:
		id = USX2Y_TYPE_224;
		break;
	case USB_ID_US428:
		id = USX2Y_TYPE_428;
		break;
	}
	if (0 > id)
		return -ENODEV;
	strcpy(info->id, type_ids[id]);
	info->num_dsps = 2;		// 0: Prepad Data, 1: FPGA Code
 	if (((usX2Ydev_t*)hw->private_data)->chip_status & USX2Y_STAT_CHIP_INIT) 
		info->chip_ready = 1;
 	info->version = USX2Y_DRIVER_VERSION; 
	return 0;
}

/*
 * Prepare some urbs
 */
static int snd_usX2Y_AsyncSeq04_init(usX2Ydev_t* usX2Y)
{
	int	err = 0,
		i;
	usX2Y->Seq04 = 0;

	if (NULL == (usX2Y->AS04.buffer = kmalloc(URB_DataLen_AsyncSeq*URBS_AsyncSeq, GFP_KERNEL))) {
		err = -ENOMEM;
	}else
		for (i = 0; i < URBS_AsyncSeq; ++i) {
			if (NULL == (usX2Y->AS04.urb[i] = usb_alloc_urb(0, GFP_KERNEL))) {
				err = -ENOMEM;
				break;
			}
			usb_fill_bulk_urb(	usX2Y->AS04.urb[i], usX2Y->chip.dev,
						usb_sndbulkpipe(usX2Y->chip.dev, 0x04),
						usX2Y->AS04.buffer + URB_DataLen_AsyncSeq*i, 0,
						snd_usX2Y_Out04Int, usX2Y
				);
		}
	return err;
}
static int snd_usX2Y_create_usbmidi(snd_card_t* card )
{
	static snd_usb_midi_endpoint_info_t quirk_data_1 = {
		.out_ep =0x06,
		.in_ep = 0x06,
		.out_cables =	0x001,
		.in_cables =	0x001
	};
	static snd_usb_audio_quirk_t quirk_1 = {
		.vendor_name =	"TASCAM",
		.product_name =	NAME_ALLCAPS,
		.ifnum = 	0,
       		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = &quirk_data_1
	};
	static snd_usb_midi_endpoint_info_t quirk_data_2 = {
		.out_ep =0x06,
		.in_ep = 0x06,
		.out_cables =	0x003,
		.in_cables =	0x003
	};
	static snd_usb_audio_quirk_t quirk_2 = {
		.vendor_name =	"TASCAM",
		.product_name =	"US428",
		.ifnum = 	0,
       		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = &quirk_data_2
	};
	struct usb_device *dev = usX2Y(card)->chip.dev;
	struct usb_interface *iface = get_iface(dev->actconfig, 0);
	snd_usb_audio_quirk_t *quirk = dev->descriptor.idProduct == USB_ID_US428 ? &quirk_2 : &quirk_1;

	snd_printd("snd_usX2Y_create_usbmidi \n");
	return snd_usb_create_midi_interface(&usX2Y(card)->chip, iface, quirk);
}

static int snd_usX2Y_create_alsa_devices(snd_card_t* card)
{
	int err;

	do {
		if ((err = snd_usX2Y_create_usbmidi(card)) < 0) {
			snd_printk("snd_usX2Y_create_alsa_devices: snd_usX2Y_create_usbmidi error %i \n", err);
			break;
		}
		if ((err = snd_usX2Y_audio_create(card)) < 0) 
			break;
		if ((err = snd_card_register(card)) < 0)
			break;
	} while (0);

	return err;
} 

static int snd_usX2Y_In04_init(usX2Ydev_t* usX2Y)
{
	int	err = 0;
	if (! (usX2Y->In04urb = usb_alloc_urb(0, GFP_KERNEL)))
		return -ENOMEM;

	if (! (usX2Y->In04Buf = kmalloc(21, GFP_KERNEL))) {
		usb_free_urb(usX2Y->In04urb);
		return -ENOMEM;
	}
	 
	init_waitqueue_head(&usX2Y->In04WaitQueue);
	usb_fill_int_urb(usX2Y->In04urb, usX2Y->chip.dev, usb_rcvintpipe(usX2Y->chip.dev, 0x4),
			 usX2Y->In04Buf, 21,
			 snd_usX2Y_In04Int, usX2Y,
			 10);
#ifdef OLD_USB
	usX2Y->In04urb->transfer_flags = USB_QUEUE_BULK;
#endif
	err = usb_submit_urb(usX2Y->In04urb, GFP_KERNEL);
	return err;
}

static int usX2Y_hwdep_dsp_load(snd_hwdep_t *hw, snd_hwdep_dsp_image_t *dsp)
{
	int	lret, err;
	char*	buf;
	snd_printd( "dsp_load %s\n", dsp->name);

	buf = dsp->image;

	err = -EINVAL;
	if (access_ok(VERIFY_READ, dsp->image, dsp->length)) {
		struct usb_device* dev = ((usX2Ydev_t*)hw->private_data)->chip.dev;
		buf = kmalloc(dsp->length, GFP_KERNEL);
		copy_from_user(buf, dsp->image, dsp->length);
		if ((err = usb_set_interface(dev, 0, 1)))
			snd_printk("usb_set_interface error \n");
		else
			err = usb_bulk_msg(dev, usb_sndbulkpipe(dev, 2), buf, dsp->length, &lret, 6*HZ);
		kfree(buf);
	}
	if (!err  &&  1 == dsp->index)
		do {
			set_current_state(TASK_UNINTERRUPTIBLE);
			schedule_timeout(HZ/4);			// give the device some time 
			if ((err = snd_usX2Y_AsyncSeq04_init((usX2Ydev_t*)hw->private_data))) {
				snd_printk("snd_usX2Y_AsyncSeq04_init error \n");
				break;
			}
			if ((err = snd_usX2Y_In04_init((usX2Ydev_t*)hw->private_data))) {
				snd_printk("snd_usX2Y_In04_init error \n");
				break;
			}
			if ((err = snd_usX2Y_create_alsa_devices(hw->card))) {
				snd_printk("snd_usX2Y_create_alsa_devices error %i \n", err);
				snd_card_free(hw->card);
				break;
			}
			((usX2Ydev_t*)hw->private_data)->chip_status |= USX2Y_STAT_CHIP_INIT; 
			snd_printd("%s: alsa all started\n", hw->name);
		} while (0);
	return err;
}


int snd_usX2Y_hwdep_new(snd_card_t* card, struct usb_device* device)
{
	int err;
	snd_hwdep_t *hw;

	if ((err = snd_hwdep_new(card, SND_USX2Y_LOADER_ID, 0, &hw)) < 0)
		return err;

	hw->iface = SNDRV_HWDEP_IFACE_USX2Y;
	hw->private_data = usX2Y(card);
	hw->ops.open = usX2Y_hwdep_open;
	hw->ops.release = usX2Y_hwdep_release;
	hw->ops.dsp_status = usX2Y_hwdep_dsp_status;
	hw->ops.dsp_load = usX2Y_hwdep_dsp_load;
	hw->ops.mmap = us428ctls_mmap;
	hw->ops.poll = us428ctls_poll;
	hw->exclusive = 1;
	sprintf(hw->name, "/proc/bus/usb/%03d/%03d", device->bus->busnum, device->devnum);
	usX2Y(card)->hwdep = hw;

	return 0;
}

