#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)

#include <linux/usb.h>

static inline int usb_endpoint_num(const struct usb_endpoint_descriptor *epd)
{
	return epd->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
}

static inline int usb_endpoint_type(const struct usb_endpoint_descriptor *epd)
{
	return epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
}

static inline int usb_endpoint_dir_in(const struct usb_endpoint_descriptor *epd)
{
	return ((epd->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN);
}

static inline int usb_endpoint_dir_out(
				const struct usb_endpoint_descriptor *epd)
{
	return ((epd->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT);
}

static inline int usb_endpoint_xfer_bulk(
				const struct usb_endpoint_descriptor *epd)
{
	return ((epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
		USB_ENDPOINT_XFER_BULK);
}

static inline int usb_endpoint_xfer_control(
				const struct usb_endpoint_descriptor *epd)
{
	return ((epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
		USB_ENDPOINT_XFER_CONTROL);
}

static inline int usb_endpoint_xfer_int(
				const struct usb_endpoint_descriptor *epd)
{
	return ((epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
		USB_ENDPOINT_XFER_INT);
}

static inline int usb_endpoint_xfer_isoc(
				const struct usb_endpoint_descriptor *epd)
{
	return ((epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
		USB_ENDPOINT_XFER_ISOC);
}

static inline int usb_endpoint_is_bulk_in(
				const struct usb_endpoint_descriptor *epd)
{
	return (usb_endpoint_xfer_bulk(epd) && usb_endpoint_dir_in(epd));
}

static inline int usb_endpoint_is_bulk_out(
				const struct usb_endpoint_descriptor *epd)
{
	return (usb_endpoint_xfer_bulk(epd) && usb_endpoint_dir_out(epd));
}

static inline int usb_endpoint_is_int_in(
				const struct usb_endpoint_descriptor *epd)
{
	return (usb_endpoint_xfer_int(epd) && usb_endpoint_dir_in(epd));
}

static inline int usb_endpoint_is_int_out(
				const struct usb_endpoint_descriptor *epd)
{
	return (usb_endpoint_xfer_int(epd) && usb_endpoint_dir_out(epd));
}

static inline int usb_endpoint_is_isoc_in(
				const struct usb_endpoint_descriptor *epd)
{
	return (usb_endpoint_xfer_isoc(epd) && usb_endpoint_dir_in(epd));
}

static inline int usb_endpoint_is_isoc_out(
				const struct usb_endpoint_descriptor *epd)
{
	return (usb_endpoint_xfer_isoc(epd) && usb_endpoint_dir_out(epd));
}

static inline void *usb_get_intfdata(struct usb_interface *intf)
{
	return dev_get_drvdata(&intf->dev);
}

static inline void usb_set_intfdata(struct usb_interface *intf, void *data)
{
	dev_set_drvdata(&intf->dev, data);
}

#endif /* < 2.6.24 */
