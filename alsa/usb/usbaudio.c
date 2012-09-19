#define SND_NEED_USB_WRAPPER
#include <sound/driver.h>
#include <linux/usb.h>

#ifdef OLD_USB
#define snd_usb_complete_callback(x) __old_ ## x
static void __old_snd_complete_urb(struct urb *urb);
static void __old_snd_complete_sync_urb(struct urb *urb);

static void * usb_audio_probe(struct usb_device *dev, unsigned int ifnum,
                              const struct usb_device_id *id);
static void usb_audio_disconnect(struct usb_device *dev, void *ptr);
#endif

#include "../alsa-kernel/usb/usbaudio.c"

#ifdef OLD_USB
/*
 * 2.4 USB kernel API
 */
static void *usb_audio_probe(struct usb_device *dev, unsigned int ifnum,
			     const struct usb_device_id *id)
{
	return snd_usb_audio_probe(dev, usb_ifnum_to_if(dev, ifnum), id);
}
                                       
static void usb_audio_disconnect(struct usb_device *dev, void *ptr)
{
	snd_usb_audio_disconnect(dev, ptr);
}

static void __old_snd_complete_urb(struct urb *urb)
{
	snd_complete_urb(urb, NULL);
}

static void __old_snd_complete_sync_urb(struct urb *urb)
{
	snd_complete_sync_urb(urb, NULL);
}
#endif

/*
 * workarounds / hacks for the older kernels follow below
 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 24)
/* M-Audio Quattro has weird alternate settings.  the altsetting jumps
 * from 0 to 4 or 3 insuccessively, and this screws up
 * usb_set_interface() (at least on 2.4.18/19 and 2.4.21).
 */

/*
 * the following is a stripped version of usb_set_interface() with the fix
 * for insuccessive altsetting numbers.
 */

/* stripped version for isochronos only */
static void hack_usb_set_maxpacket(struct usb_device *dev)
{
	int i, b;

	for (i=0; i<dev->actconfig->bNumInterfaces; i++) {
		struct usb_interface *ifp = dev->actconfig->interface + i;
		struct usb_interface_descriptor *as = ifp->altsetting + ifp->act_altsetting;
		struct usb_endpoint_descriptor *ep = as->endpoint;
		int e;

		for (e=0; e<as->bNumEndpoints; e++) {
			b = ep[e].bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
			if (usb_endpoint_out(ep[e].bEndpointAddress)) {
				if (ep[e].wMaxPacketSize > dev->epmaxpacketout[b])
					dev->epmaxpacketout[b] = ep[e].wMaxPacketSize;
			}
			else {
				if (ep[e].wMaxPacketSize > dev->epmaxpacketin [b])
					dev->epmaxpacketin [b] = ep[e].wMaxPacketSize;
			}
		}
	}
}

/* stripped version */
int snd_hack_usb_set_interface(struct usb_device *dev, int interface, int alternate)
{
	struct usb_interface *iface;
	struct usb_interface_descriptor *iface_as;
	int i, ret;

	iface = usb_ifnum_to_if(dev, interface);
	if (!iface)
		return -EINVAL;
	if (iface->num_altsetting == 1)
		return 0;
	if (alternate < 0 || alternate >= iface->num_altsetting)
		return -EINVAL;

	if ((ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
				   USB_REQ_SET_INTERFACE, USB_RECIP_INTERFACE,
				   iface->altsetting[alternate].bAlternateSetting,
				   interface, NULL, 0, HZ * 5)) < 0)
		return ret;

	iface->act_altsetting = alternate;
	iface_as = &iface->altsetting[alternate];
	for (i = 0; i < iface_as->bNumEndpoints; i++) {
		u8 ep = iface_as->endpoint[i].bEndpointAddress;
		usb_settoggle(dev, ep&USB_ENDPOINT_NUMBER_MASK, usb_endpoint_out(ep), 0);
	}
	hack_usb_set_maxpacket(dev);
	return 0;
}

#endif /* LINUX_VERSION < 2.5.24 */


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
/*
 * 2.2 compatible layer
 */
#undef usb_driver
#undef usb_device_id
#undef usb_register
#undef usb_deregister
#undef usb_driver_claim_interface

#define MAX_USB_DRIVERS	5
struct snd_usb_reg_table {
	struct usb_driver driver;
	struct snd_compat_usb_driver *orig;
};

static struct snd_usb_reg_table my_usb_drivers[MAX_USB_DRIVERS];

static void *snd_usb_compat_probe(struct usb_device *dev, unsigned int ifnum)
{
	struct usb_config_descriptor *config = dev->actconfig;	
	struct snd_compat_usb_driver *p;
	struct usb_interface_descriptor *alts = config->interface[ifnum].altsetting;
	const struct snd_compat_usb_device_id *tbl;
	struct snd_compat_usb_device_id id;
	int i;

	for (i = 0; i < MAX_USB_DRIVERS; i++) {
		if (! (p = my_usb_drivers[i].orig))
			continue;
		for (tbl = p->id_table; tbl->match_flags; tbl++) {
			/* we are too lazy to check all entries... */
			if ((tbl->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
			    tbl->idVendor != dev->descriptor.idVendor)
				return NULL;
			if ((tbl->match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
			    tbl->idProduct != dev->descriptor.idProduct)
				return NULL;
			if ((tbl->match_flags & USB_DEVICE_ID_MATCH_INT_CLASS) &&
			    tbl->bInterfaceClass != alts->bInterfaceClass)
				return NULL;
			if ((tbl->match_flags & USB_DEVICE_ID_MATCH_INT_SUBCLASS) &&
			    tbl->bInterfaceSubClass != alts->bInterfaceSubClass)
				return NULL;
		}
		id = *tbl;
		id.idVendor = dev->descriptor.idVendor;
		id.idProduct = dev->descriptor.idProduct;
		id.bInterfaceClass = alts->bInterfaceClass;
		id.bInterfaceSubClass = alts->bInterfaceSubClass;
		return p->probe(dev, ifnum, &id);
	}
	return NULL;
}

int snd_compat_usb_register(struct snd_compat_usb_driver *driver)
{
	int i;
	struct usb_driver *drv;

	for (i = 0; i < MAX_USB_DRIVERS; i++) {
		if (! my_usb_drivers[i].orig)
			break;
	}
	if (i >= MAX_USB_DRIVERS)
		return -ENOMEM;
	my_usb_drivers[i].orig = driver;
	drv = &my_usb_drivers[i].driver;
	drv->name = driver->name;
	drv->probe = snd_usb_compat_probe;
	drv->disconnect = driver->disconnect;
	INIT_LIST_HEAD(&drv->driver_list);
	usb_register(drv);
	return 0;
}

static struct snd_usb_reg_table *find_matching_usb_driver(struct snd_compat_usb_driver *driver)
{
	int i;
	for (i = 0; i < MAX_USB_DRIVERS; i++) {
		if (my_usb_drivers[i].orig == driver)
			return &my_usb_drivers[i];
	}
	return NULL;
}

void snd_compat_usb_deregister(struct snd_compat_usb_driver *driver)
{
	struct snd_usb_reg_table *tbl;	
	if ((tbl = find_matching_usb_driver(driver)) != NULL)
		tbl->orig = NULL;
}

void snd_compat_usb_driver_claim_interface(struct snd_compat_usb_driver *driver, struct usb_interface *iface, void *ptr)
{
	struct snd_usb_reg_table *tbl;
	if ((tbl = find_matching_usb_driver(driver)) != NULL)
		usb_driver_claim_interface(&tbl->driver, iface, ptr);
}

#endif /* LINUX_VERSION < 2.3.0 */


/*
 * symbols
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 24)
EXPORT_SYMBOL(snd_hack_usb_set_interface);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 3, 0)
EXPORT_SYMBOL(snd_compat_usb_register);
EXPORT_SYMBOL(snd_compat_usb_deregister);
EXPORT_SYMBOL(snd_compat_usb_driver_claim_interface);
#endif
#else
EXPORT_NO_SYMBOLS;
#endif
