#include <linux/config.h>
#include <linux/version.h>

/* earlier kernels didn't export this variable */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,22)
#ifdef CONFIG_ACPI_BUS
static int acpi_disabled; /* dummy */
#endif
#endif

#include "../../alsa-kernel/drivers/mpu401/mpu401.c"
EXPORT_NO_SYMBOLS;
