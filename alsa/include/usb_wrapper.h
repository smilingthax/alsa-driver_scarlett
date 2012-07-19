/* workarounds for USB API */
#include <linux/usb.h>

#ifndef HAVE_USB_BUFFERS /* defined in usb.h */
#define usb_buffer_alloc(dev, size, flags, dma) kmalloc(size, flags)
#define usb_buffer_free(dev, size, addr, dma) kfree(addr)
#undef URB_NO_TRANSFER_DMA_MAP
#define URB_NO_TRANSFER_DMA_MAP 0
#endif

#ifndef CONFIG_SND_HAVE_USB_ALLOC_COHERENT
#define usb_alloc_coherent	usb_buffer_alloc
#define usb_free_coherent	usb_buffer_free
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
#define usb_interrupt_msg	usb_bulk_msg
#endif

/*
 * USB 3.0
 */
#ifndef USB_SPEED_SUPER
#define USB_SPEED_SUPER		0 /* dummy */
#endif
