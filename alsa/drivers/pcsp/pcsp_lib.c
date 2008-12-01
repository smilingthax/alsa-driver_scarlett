#include "adriver.h"
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
#define hrtimer_get_expires(t)	(t)->expires
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
#define ns_to_ktime(ns)		ktime_set(0, ns)
#define HRTIMER_CB_IRQSAFE_UNLOCKED	HRTIMER_CB_IRQSAFE
#endif
#include "../../alsa-kernel/drivers/pcsp/pcsp_lib.c"
