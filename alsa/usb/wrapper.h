#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
inline static urb_t *usb_alloc_urb_wrapper(int iso_packets, int flags)
{
	return usb_alloc_urb(iso_packets);
}
inline static int usb_submit_urb_wrapper(urb_t* urb, int flags)
{
	return usb_submit_urb(urb);
}
#define usb_alloc_urb(n,flags) usb_alloc_urb_wrapper(n,flags)
#define usb_submit_urb(p,flags) usb_submit_urb_wrapper(p,flags)
#endif /* LINUX_VERSION_CODE < 2.5.0 */


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
static int hack_usb_set_interface(struct usb_device *dev, int interface, int alternate)
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

#define usb_set_interface(dev,iface,alt) hack_usb_set_interface(dev,iface,alt)

#endif /* LINUX_VERSION < 2.5.24 */
