#include <linux/config.h>
#include <linux/version.h>

/* FIXME: correct version? */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#ifdef CONFIG_ACPI_BUS
static int acpi_disabled; /* dummy */
#endif
#endif

#include "../../alsa-kernel/drivers/mpu401/mpu401.c"
EXPORT_NO_SYMBOLS;
