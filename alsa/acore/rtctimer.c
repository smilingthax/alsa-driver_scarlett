#include "adriver.h"
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 21)
static inline int is_power_of_2(unsigned long n)
{
	return (n != 0 && ((n & (n - 1)) == 0));
}
#endif
#include "../alsa-kernel/core/rtctimer.c"
EXPORT_NO_SYMBOLS;
