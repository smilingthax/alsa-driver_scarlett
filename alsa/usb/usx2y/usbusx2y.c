/*
 * usbus428.c - ALSA USB US-428 Driver
 *
2004-01-14 Karsten Wiese
	Version 0.5.1:
	Runs with 2.6.1 kernel.

2003-12-30 Karsten Wiese
	Version 0.4.1:
	Fix 24Bit 4Channel capturing for the us428.

2003-11-27 Karsten Wiese, Martin Langer
	Version 0.4:
	us122 support.
	us224 could be tested by uncommenting the sections containing USB_ID_US224

2003-11-03 Karsten Wiese
	Version 0.3:
	24Bit support. 
	"arecord -D hw:1 -c 2 -r 48000 -M -f S24_3LE|aplay -D hw:1 -c 2 -r 48000 -M -f S24_3LE" works.

2003-08-22 Karsten Wiese
	Version 0.0.8:
	Removed EZUSB Firmware. First Stage Firmwaredownload is now done by tascam-firmware downloader.
	See:
	http://usb-midi-fw.sourceforge.net/tascam-firmware.tar.gz

2003-06-18 Karsten Wiese
	Version 0.0.5:
	changed to compile with kernel 2.4.21 and alsa 0.9.4

2002-10-16 Karsten Wiese
	Version 0.0.4:
	compiles again with alsa-current.
	USB_ISO_ASAP not used anymore (most of the time), instead
	urb->start_frame is calculated here now, some calls inside usb-driver don't need to happen anymore.

	To get the best out of this:
	Disable APM-support in the kernel as APM-BIOS calls (once each second) hard disable interrupt for many precious milliseconds.
	This helped me much on my slowish PII 400 & PIII 500.
	ACPI yet untested but might cause the same bad behaviour.
	Use a kernel with lowlatency and preemptiv patches applied.
	To autoload snd-usb-midi append a line 
		post-install snd-usb-us428 modprobe snd-usb-midi
	to /etc/modules.conf.

	known problems:
	sliders, knobs, lights not yet handled except MASTER Volume slider.
       	"pcm -c 2" doesn't work. "pcm -c 2 -m direct_interleaved" does.
	KDE3: "Enable full duplex operation" deadlocks.

	
2002-08-31 Karsten Wiese
	Version 0.0.3: audio also simplex;
	simplifying: iso urbs only 1 packet, melted structs.
	ASYNC_UNLINK not used anymore: no more crashes so far.....
	for alsa 0.9 rc3.

2002-08-09 Karsten Wiese
	Version 0.0.2: midi works with snd-usb-midi, audio (only fullduplex now) with i.e. bristol.
	The firmware has been sniffed from win2k us-428 driver 3.09.

 *   Copyright (c) 2002 Karsten Wiese
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
#define SNDRV_GET_ID
#include <sound/initval.h>
#include <sound/pcm.h>

#include <sound/rawmidi.h>
#include "usx2y.h"
#include "usbusx2y.h"
#include "usX2Yhwdep.h"



MODULE_AUTHOR("Karsten Wiese <annabellesgarden@yahoo.de>");
MODULE_DESCRIPTION("TASCAM "NAME_ALLCAPS" Version 0.4.1");
MODULE_LICENSE("GPL");
MODULE_CLASSES("{sound}");
MODULE_DEVICES("{{TASCAM(0x1604), "NAME_ALLCAPS"(0x8001)(0x8007) }}");

static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX; /* Index 0-max */
static char* id[SNDRV_CARDS] = SNDRV_DEFAULT_STR; /* Id for this card */
static int enable[SNDRV_CARDS] = SNDRV_DEFAULT_ENABLE_PNP; /* Enable this card */

MODULE_PARM(index, "1-" __MODULE_STRING(SNDRV_CARDS) "i");
MODULE_PARM_DESC(index, "Index value for "NAME_ALLCAPS".");
MODULE_PARM_SYNTAX(index, SNDRV_INDEX_DESC);
MODULE_PARM(id, "1-" __MODULE_STRING(SNDRV_CARDS) "s");
MODULE_PARM_DESC(id, "ID string for "NAME_ALLCAPS".");
MODULE_PARM_SYNTAX(id, SNDRV_ID_DESC);
MODULE_PARM(enable, "1-" __MODULE_STRING(SNDRV_CARDS) "i");
MODULE_PARM_DESC(enable, "Enable "NAME_ALLCAPS".");
MODULE_PARM_SYNTAX(enable, SNDRV_ENABLE_DESC);


static int snd_usX2Y_card_used[SNDRV_CARDS];

static void snd_usX2Y_usb_disconnect(struct usb_device* usb_device, void* ptr);
static void snd_usX2Y_card_private_free(snd_card_t *card);

#ifdef CONFIG_SND_DEBUG
/* 
 * pipe 4 is used for switching the lamps, setting samplerate, volumes ....   
 */
#ifndef OLD_USB
void snd_usX2Y_Out04Int(struct urb* urb, struct pt_regs *regs)
#else
void snd_usX2Y_Out04Int(struct urb* urb)
#endif
{
	if (urb->status) {
		int 		i;
		usX2Ydev_t*	usX2Y = urb->context;
		for (i = 0; i < 10 && usX2Y->AS04.urb[i] != urb; i++);
		snd_printd("snd_usX2Y_Out04Int() usX2Y->Seq04=%i urb %i status=%i\n", usX2Y->Seq04, i, urb->status);
	}
}
#endif

#ifndef OLD_USB
void snd_usX2Y_In04Int(struct urb* urb, struct pt_regs *regs)
#else
void snd_usX2Y_In04Int(struct urb* urb)
#endif
{
	int			err = 0;
	usX2Ydev_t		*usX2Y = urb->context;
	us428ctls_sharedmem_t	*us428ctls = usX2Y->us428ctls_sharedmem;

	usX2Y->In04IntCalls++;

	if (urb->status) {
		snd_printk( "Interrupt Pipe 4 came back with status=%i\n", urb->status);
		return;
	}

	//	printk("%i:0x%02X ", 8, (int)((unsigned char*)usX2Y->In04Buf)[8]); Master volume shows 0 here if fader is at max during boot ?!?
	if (us428ctls) {
		int diff = -1;
		if (-2 == us428ctls->CtlSnapShotLast) {
			diff = 0;
			memcpy(usX2Y->In04Last, usX2Y->In04Buf, sizeof(usX2Y->In04Last));
			us428ctls->CtlSnapShotLast = -1;
		} else {
			int i;
			for (i = 0; i < 21; i++) {
				if (usX2Y->In04Last[i] != ((char*)usX2Y->In04Buf)[i]) {
					if (diff < 0)
						diff = i;
					usX2Y->In04Last[i] = ((char*)usX2Y->In04Buf)[i];
				}
			}
		}
		if (0 <= diff) {
			int n = us428ctls->CtlSnapShotLast + 1;
			if (n >= N_us428_ctl_BUFS  ||  n < 0)
				n = 0;
			memcpy(us428ctls->CtlSnapShot + n, usX2Y->In04Buf, sizeof(us428ctls->CtlSnapShot[0]));
			us428ctls->CtlSnapShotDiffersAt[n] = diff;
			us428ctls->CtlSnapShotLast = n;
			wake_up(&usX2Y->us428ctls_wait_queue_head);
		}
	}
	
	
	if (usX2Y->US04) {
		if (0 == usX2Y->US04->submitted)
			do
				err = usb_submit_urb(usX2Y->US04->urb[usX2Y->US04->submitted++], GFP_ATOMIC);
			while (!err && usX2Y->US04->submitted < usX2Y->US04->len);
	} else
		if (us428ctls && us428ctls->p4outLast >= 0 && us428ctls->p4outLast < N_us428_p4out_BUFS) {
			if (us428ctls->p4outLast != us428ctls->p4outSent) {
				int j, send = us428ctls->p4outSent + 1;
				if (send >= N_us428_p4out_BUFS)
					send = 0;
				for (j = 0; j < URBS_AsyncSeq; ++j)
					if (0 == usX2Y->AS04.urb[j]->status) {
						us428_p4out_t *p4out = us428ctls->p4out + send;	// FIXME if more then 1 p4out is new, 1 gets lost.
						usb_fill_bulk_urb(usX2Y->AS04.urb[j], usX2Y->chip.dev,
								  usb_sndbulkpipe(usX2Y->chip.dev, 0x04), &p4out->val.vol, 
								  p4out->type == eLT_Light ? sizeof(us428_lights_t) : 5,
								  snd_usX2Y_Out04Int, usX2Y);
#ifdef OLD_USB
						usX2Y->AS04.urb[j]->transfer_flags = USB_QUEUE_BULK;
#endif
						usb_submit_urb(usX2Y->AS04.urb[j], GFP_ATOMIC);
						us428ctls->p4outSent = send;
						break;
					}
			}
		}

	if (err) {
		snd_printk("In04Int() usb_submit_urb err=%i\n", err);
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
	urb->dev = usX2Y->chip.dev;
	usb_submit_urb(urb, GFP_ATOMIC);
#endif
}

static void snd_usX2Y_unlinkSeq(snd_usX2Y_AsyncSeq_t* S)
{
	int	i;
	for (i = 0; i < URBS_AsyncSeq; ++i) {
		if (S[i].urb) {
			usb_unlink_urb(S->urb[i]);
			usb_free_urb(S->urb[i]);
			S->urb[i] = NULL;
		}
	}
	if (S->buffer)
		kfree(S->buffer);
}


static struct usb_device_id snd_usX2Y_usb_id_table[] = {
	{
		.match_flags =	USB_DEVICE_ID_MATCH_DEVICE,
		.idVendor =	0x1604,
		.idProduct =	USB_ID_US428 
	},
	{
		.match_flags =	USB_DEVICE_ID_MATCH_DEVICE,
		.idVendor =	0x1604,
		.idProduct =	USB_ID_US122 
	},
/* 	{ FIXME: uncomment  to test us224 support*/
/* 		.match_flags =	USB_DEVICE_ID_MATCH_DEVICE, */
/* 		.idVendor =	0x1604, */
/* 		.idProduct =	USB_ID_US224  */
/* 	}, */
	{ /* terminator */ }
};

static snd_card_t* snd_usX2Y_create_card(struct usb_device* device)
{
	int		dev;
	snd_card_t*	card;
	for (dev = 0; dev < SNDRV_CARDS; ++dev)
		if (enable[dev] && !snd_usX2Y_card_used[dev])
			break;
	if (dev >= SNDRV_CARDS)
		return NULL;
	card = snd_card_new(index[dev], id[dev], THIS_MODULE, sizeof(usX2Ydev_t));
	if (!card)
		return NULL;
	snd_usX2Y_card_used[usX2Y(card)->chip.index = dev] = 1;
	card->private_free = snd_usX2Y_card_private_free;
	usX2Y(card)->chip.dev = device;
	usX2Y(card)->chip.card = card;
	init_MUTEX (&usX2Y(card)->open_mutex);
	INIT_LIST_HEAD(&usX2Y(card)->chip.midi_list);
	usX2Y(card)->Seq04Complete = 1;
	strcpy(card->driver, "USB "NAME_ALLCAPS"");
	sprintf(card->shortname, "TASCAM "NAME_ALLCAPS"");
	sprintf(card->longname, "%s (%x:%x if %d at %03d/%03d)",
		card->shortname, 
		device->descriptor.idVendor, device->descriptor.idProduct,
		0,//us428(card)->usbmidi.ifnum,
		usX2Y(card)->chip.dev->bus->busnum, usX2Y(card)->chip.dev->devnum
		);
	snd_card_set_dev(card, &device->dev);
	return card;
}


static void* snd_usX2Y_usb_probe(struct usb_device* device, struct usb_interface *intf, const struct usb_device_id* device_id)
{
	int		err;
	snd_card_t*	card;
	if (device->descriptor.idVendor != 0x1604 ||
	    (device->descriptor.idProduct != USB_ID_US122 && /* device->descriptor.idProduct != USB_ID_US224 && */ device->descriptor.idProduct != USB_ID_US428) ||
	    !(card = snd_usX2Y_create_card(device)))
		return 0;
	if ((err = snd_usX2Y_hwdep_new(card, device)) < 0  ||
	    (err = snd_card_register(card)) < 0) {
		snd_card_free(card);
		return 0;
	}
	return card;
}

#ifndef OLD_USB
/*
 * new 2.5 USB kernel API
 */
static int snd_usX2Y_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	void *chip;
	chip = snd_usX2Y_usb_probe(interface_to_usbdev(intf), intf, id);
	if (chip) {
		dev_set_drvdata(&intf->dev, chip);
		return 0;
	} else
		return -EIO;
}

static void snd_usX2Y_disconnect(struct usb_interface *intf)
{
	snd_usX2Y_usb_disconnect(interface_to_usbdev(intf),
				 dev_get_drvdata(&intf->dev));
}
#else
/*
 * 2.4 USB kernel API
 */
static void *snd_usX2Y_probe(struct usb_device *dev, unsigned int ifnum, const struct usb_device_id *id)
{
	return snd_usX2Y_usb_probe(dev, usb_ifnum_to_if(dev, ifnum), id);
}
                                       
static void snd_usX2Y_disconnect(struct usb_device *dev, void *ptr)
{
	snd_usX2Y_usb_disconnect(dev, ptr);
}
#endif

MODULE_DEVICE_TABLE(usb, snd_usX2Y_usb_id_table);
static struct usb_driver snd_usX2Y_usb_driver = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 70)	/* FIXME: find right number */
 	.owner =	THIS_MODULE,
#endif
	.name =		"snd-usb-usx2y",
	.probe =	snd_usX2Y_probe,
	.disconnect =	snd_usX2Y_disconnect,
	.id_table =	snd_usX2Y_usb_id_table,
#ifdef OLD_USB
	.driver_list =	LIST_HEAD_INIT(snd_usX2Y_usb_driver.driver_list), 
#endif
};

static void snd_usX2Y_card_private_free(snd_card_t *card)
{
	if (usX2Y(card)->In04Buf)
		kfree(usX2Y(card)->In04Buf);
	usb_free_urb(usX2Y(card)->In04urb);
	if (usX2Y(card)->us428ctls_sharedmem)
		snd_free_pages(usX2Y(card)->us428ctls_sharedmem, sizeof(*usX2Y(card)->us428ctls_sharedmem));
	if (usX2Y(card)->chip.index >= 0  &&  usX2Y(card)->chip.index < SNDRV_CARDS)
		snd_usX2Y_card_used[usX2Y(card)->chip.index] = 0;
}

/*
 * Frees the device.
 */
static void snd_usX2Y_usb_disconnect(struct usb_device* device, void* ptr)
{
	if (ptr) {
		usX2Ydev_t* usX2Y = usX2Y((snd_card_t*)ptr);
		struct list_head* p;
		if (usX2Y->chip_status == USX2Y_STAT_CHIP_HUP)	// on 2.6.1 kernel snd_usbmidi_disconnect()
			return;					// calls us back. better leave :-) .
		usX2Y->chip_status = USX2Y_STAT_CHIP_HUP;
		snd_usX2Y_unlinkSeq(&usX2Y->AS04);
		usb_unlink_urb(usX2Y->In04urb);
		snd_card_disconnect((snd_card_t*)ptr);
		/* release the midi resources */
		list_for_each(p, &usX2Y->chip.midi_list) {
			snd_usbmidi_disconnect(p, &snd_usX2Y_usb_driver);
		}
		if (usX2Y->us428ctls_sharedmem) 
			wake_up(&usX2Y->us428ctls_wait_queue_head);
		snd_card_free_in_thread((snd_card_t*)ptr);
	}
}

static int __init snd_usX2Y_module_init(void)
{
	return usb_register(&snd_usX2Y_usb_driver);
}

static void __exit snd_usX2Y_module_exit(void)
{
	usb_deregister(&snd_usX2Y_usb_driver);
}

module_init(snd_usX2Y_module_init)
module_exit(snd_usX2Y_module_exit)

#ifndef MODULE

/* format is: snd-usb-usX2Y=enable,index,id */

static int __init snd_usX2Y_setup(char* str)
{
	static unsigned __initdata nr_dev = 0;

	if (nr_dev >= SNDRV_CARDS)
		return 0;
	(void)(get_option(&str, &enable[nr_dev]) == 2 &&
	       get_option(&str, &index[nr_dev]) == 2 &&
	       get_id(&str, &id[nr_dev]) == 2);
	nr_dev++;
	return 1;
}

__setup("snd-usb-usx2y=", snd_usX2Y_setup);

#endif /* !MODULE */

EXPORT_NO_SYMBOLS;
