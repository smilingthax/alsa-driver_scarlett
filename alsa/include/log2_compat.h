#ifndef _LINUX_LOG2_H
#define _LINUX_LOG2_H

#include <linux/types.h>
#include <linux/bitops.h>

#ifndef bool
#define bool int
#endif

static inline __attribute__((const))
bool is_power_of_2(unsigned long n)
{
	return (n != 0 && ((n & (n - 1)) == 0));
}

#endif /* _LINUX_LOG2_H */
