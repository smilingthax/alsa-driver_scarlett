#include "adriver.h"

/* workaround for the vga-switcheroo audio client handling */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 5, 0)
#undef CONFIG_VGA_SWITCHEROO
#include <linux/vgaarb.h>
#undef vga_default_device
#define vga_default_device()	NULL
#endif

