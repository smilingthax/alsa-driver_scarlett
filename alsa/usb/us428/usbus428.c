/*
 * usbus428.c - ALSA USB US-428 Driver
 *
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

#include <sound/driver.h>
#include <sound/core.h>
#include <sound/seq_device.h>
#define SNDRV_GET_ID
#include <sound/initval.h>
#include <sound/pcm.h>

#include <sound/rawmidi.h>
#include "usx2y.h"
#include "usbus428.h"
#include "usX2Yhwdep.h"



MODULE_AUTHOR("Karsten Wiese <annabellesgarden@yahoo.de>");
MODULE_DESCRIPTION("TASCAM "NAME_ALLCAPS" Version 0.1");
MODULE_LICENSE("GPL");
MODULE_CLASSES("{sound}");
MODULE_DEVICES("{{TASCAM(0x1604), "NAME_ALLCAPS"(0x8001) }}");

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


static int snd_us428_card_used[SNDRV_CARDS];

static void snd_us428_usb_disconnect(struct usb_device* usb_device, void* ptr);
static void snd_us428_card_private_free(snd_card_t *card);

#ifdef CONFIG_SND_DEBUG
/* 
 * pipe 4 is used for switching the lamps, setting samplerate, volumes ....   
 */
void snd_us428_Out04Int(urb_t* urb)
{
	if (urb->status) {
		int 		i;
		us428dev_t*	us428 = urb->context;
		for (i = 0; i < 10 && us428->AS04.urb[i] != urb; i++);
		snd_printd("snd_us428_Out04Int() us428->Seq04=%i urb %i status=%i\n", us428->Seq04, i, urb->status);
	}
}
#else
#define snd_us428_Out04Int 0
#endif

void snd_us428_In04Int(urb_t* urb){
	int			err = 0;
	us428dev_t		*us428 = urb->context;
	us428ctls_sharedmem_t	*us428ctls = us428->us428ctls_sharedmem;
	
	us428->In04IntCalls++;

	if (urb->status){
		snd_printk( "Interrupt Pipe 4 came back with status=%i\n", urb->status);
		return;
	}

        {
		int diff = -1, i;
	//	printk("%i:0x%02X ", 8, (int)((unsigned char*)us428->In04Buf)[8]); Master volume shows 0 here if fader is at max during boot ?!?
		for (i = 0; i < 21; i++) {
			if (us428->In04Last[i] != ((char*)us428->In04Buf)[i]) {
				if (diff < 0)
					diff = i;
				us428->In04Last[i] = ((char*)us428->In04Buf)[i];
			}
		}
		if (diff >= 0  &&  us428ctls) {
			int n = us428ctls->CtlSnapShotLast + 1;
			if (n >= N_us428_ctl_BUFS  ||  n < 0)
				n = 0;
			memcpy(us428ctls->CtlSnapShot + n, us428->In04Buf, sizeof(us428ctls->CtlSnapShot[0]));
			us428ctls->CtlSnapShotDiffersAt[n] = diff;
			us428ctls->CtlSnapShotLast = n;
			wake_up(&us428->us428ctls_wait_queue_head);
		}
	}
	
	
	if (us428->US04) {
		if (0 == us428->US04->submitted)
			do
				err = usb_submit_urb(us428->US04->urb[us428->US04->submitted++]);
			while (!err && us428->US04->submitted < us428->US04->len);
	} else
		if (us428ctls && us428ctls->p4outLast >= 0 && us428ctls->p4outLast < N_us428_p4out_BUFS) {
			if (us428ctls->p4outLast != us428ctls->p4outSent) {
				int j, send = us428ctls->p4outSent + 1;
				if (send >= N_us428_p4out_BUFS)
					send = 0;
				while (us428ctls->p4out[send].type == eLT_Light && send != us428ctls->p4outLast)
					if (++send >= N_us428_p4out_BUFS)
						send = 0;
				for (j = 0; j < URBS_AsyncSeq; ++j)
					if (0 == us428->AS04.urb[j]->status) {
						us428_p4out_t *p4out = us428ctls->p4out + send;	// FIXME if more then 1 p4out is new, 1 gets lost.
						usb_fill_bulk_urb(us428->AS04.urb[j], us428->chip.dev,
								  usb_sndbulkpipe(us428->chip.dev, 0x04), &p4out->vol, 
								  p4out->type == eLT_Light ? sizeof(us428_lights_t) : sizeof(usX2Y_volume_t),
								  snd_us428_Out04Int, us428);
						us428->AS04.urb[j]->transfer_flags = USB_QUEUE_BULK;
						usb_submit_urb(us428->AS04.urb[j]);
						us428ctls->p4outSent = send;
						break;
					}
			}
		}

	if (err){
		snd_printk("In04Int() usb_submit_urb err=%i\n", err);
	}
}

static void snd_us428_unlinkSeq(snd_us428_AsyncSeq_t* S)
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


static struct usb_device_id snd_us428_usb_id_table[] = {
	{
		.match_flags =	USB_DEVICE_ID_MATCH_DEVICE,
		.idVendor =	0x1604,
		.idProduct =	0x8001 
	},
	{ /* terminator */ }
};

static snd_card_t* snd_us428_create_card(struct usb_device* device)
{
	int		err = 0, dev;
	snd_card_t*	card = NULL;
	do {
		for (dev = 0; dev < SNDRV_CARDS; ++dev)
			if (enable[dev] && !snd_us428_card_used[dev])
				break;

		if (dev >= SNDRV_CARDS) {
			err = -ENOENT;
			break;
		}
		card = snd_card_new(index[dev], id[dev], THIS_MODULE, sizeof(us428dev_t));
		if (!card) {
			err = -ENOMEM;
			break;
		}
		snd_us428_card_used[us428(card)->chip.index = dev] = 1;
		card->private_free = snd_us428_card_private_free;
		us428(card)->chip.dev = device;
		us428(card)->chip.card = card;
		init_MUTEX (&us428(card)->open_mutex);
		INIT_LIST_HEAD(&us428(card)->chip.midi_list);
		us428(card)->Seq04Complete = 1;
		us428(card)->stride = 4;		// 16 Bit 
		strcpy(card->driver, "USB "NAME_ALLCAPS"");
		sprintf(card->shortname, "TASCAM "NAME_ALLCAPS"");
		sprintf(card->longname, "%s (%x:%x if %d at %03d/%03d)",
			card->shortname, 
			snd_us428_usb_id_table[0].idVendor, snd_us428_usb_id_table[0].idProduct,
			0,//us428(card)->usbmidi.ifnum,
			us428(card)->chip.dev->bus->busnum, us428(card)->chip.dev->devnum
			);
	} while (0);
	return card;
}


static void* snd_us428_probe(struct usb_device* device, unsigned int ifnum, const struct usb_device_id* device_id)
{
	int		err;
	snd_card_t*	card;
	
			/* See if the device offered us matches what we can accept */
	if (device->descriptor.idVendor != 0x1604 || device->descriptor.idProduct != 0x8001)
		return 0;

	if (!(card = snd_us428_create_card(device)))
		return 0;

	if ((err = snd_usX2Y_hwdep_new(card, device)) < 0) {
		snd_card_free(card);
		return 0;
	}
	if ((err = snd_card_register(card)) < 0) {
		snd_card_free(card);
		return 0;
	}
	return card;
}


MODULE_DEVICE_TABLE(usb, snd_us428_usb_id_table);
static struct usb_driver snd_us428_usb_driver = {
	.name =		"snd-usb-us428",
	.probe =	snd_us428_probe,
	.disconnect =	snd_us428_usb_disconnect,
	.id_table =	snd_us428_usb_id_table,
	.driver_list =	LIST_HEAD_INIT(snd_us428_usb_driver.driver_list),
};

static void snd_us428_card_private_free(snd_card_t *card)
{
	snd_printd("\n");
	if (us428(card)->In04Buf)
		kfree(us428(card)->In04Buf);
	usb_free_urb(us428(card)->In04urb);
	if (us428(card)->us428ctls_sharedmem)
		snd_free_pages(us428(card)->us428ctls_sharedmem, sizeof(*us428(card)->us428ctls_sharedmem));
	if (us428(card)->chip.index >= 0  &&  us428(card)->chip.index < SNDRV_CARDS)
		snd_us428_card_used[us428(card)->chip.index] = 0;
}

/*
 * Frees the device.
 */
static void snd_us428_usb_disconnect(struct usb_device* device, void* ptr)
{
	if (ptr) {
		us428dev_t* us428 = us428((snd_card_t*)ptr);
		struct list_head* p;
		us428->chip_status = USX2Y_STAT_CHIP_HUP;
		snd_us428_unlinkSeq(&us428->AS04);
		usb_unlink_urb(us428->In04urb);
		/* release the midi resources */
		list_for_each(p, &us428->chip.midi_list) {
			snd_usbmidi_disconnect(p, &snd_us428_usb_driver);
		}
		snd_card_disconnect((snd_card_t*)ptr);
		if (us428->us428ctls_sharedmem) 
			wake_up(&us428->us428ctls_wait_queue_head);
		snd_card_free_in_thread((snd_card_t*)ptr);
	}
}

static int __init snd_us428_module_init(void)
{
	return usb_register(&snd_us428_usb_driver);
}

static void __exit snd_us428_module_exit(void)
{
	usb_deregister(&snd_us428_usb_driver);
}

module_init(snd_us428_module_init)
module_exit(snd_us428_module_exit)

#ifndef MODULE

/* format is: snd-usb-us428=enable,index,id */

static int __init snd_us428_setup(char* str)
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

__setup("snd-usb-us428=", snd_us428_setup);

#endif /* !MODULE */

EXPORT_NO_SYMBOLS;
