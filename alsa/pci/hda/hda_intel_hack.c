#include "adriver.h"

/* workaround for the vga-switcheroo audio client handling */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 5, 0)
#undef CONFIG_VGA_SWITCHEROO
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 34)
#define vga_switcheroo_unregister_client(pci)
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 33)
#include <linux/pci.h>
#define pci_get_domain_bus_and_slot(d,b,f) pci_get_bus_and_slot(b,f)
#endif
#endif

