#include "adriver.h"
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
#define HRTIMER_CB_IRQSAFE_UNLOCKED	HRTIMER_CB_IRQSAFE
#endif
#include "../../alsa-kernel/drivers/pcsp/pcsp.c"
