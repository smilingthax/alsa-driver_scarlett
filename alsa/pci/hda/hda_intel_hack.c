#include "adriver.h"

/* workaround for the vga-switcheroo audio client handling */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 5, 0)
#undef CONFIG_VGA_SWITCHEROO
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 34) && (!defined(RHEL_RELEASE_CODE) || RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(6, 0))
#define vga_switcheroo_unregister_client(pci)
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 33)
#include <linux/pci.h>
#define pci_get_domain_bus_and_slot(d,b,f) pci_get_bus_and_slot(b,f)
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
struct kernel_param_ops {
  int (*set)(const char *val, const struct kernel_param *kp);
  int (*get)(char *buffer, const struct kernel_param *kp);
};
#define param_get_xint param_get_int
#endif
#endif

