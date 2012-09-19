#include "adriver.h"
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
#define hrtimer_get_expires(t)	(t)->expires
#endif
#include "../../alsa-kernel/drivers/pcsp/pcsp_lib.c"
