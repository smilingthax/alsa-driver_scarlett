#include <linux/config.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
#define pci_device(chip) pci_name(chip->pci)
#endif

#include "../../alsa-kernel/pci/echoaudio/echo3g.c"

EXPORT_NO_SYMBOLS;
