#include <linux/types.h>

/* FIXME: it is valid for x86_64, check other archs */

typedef s32             compat_time_t;

struct compat_timespec {
	compat_time_t   tv_sec;
	s32             tv_nsec;
};

struct compat_timeval {
	compat_time_t   tv_sec;
	s32             tv_usec;
};
