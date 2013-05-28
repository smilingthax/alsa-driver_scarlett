#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 0)
#if !defined(RHEL_RELEASE_CODE) || RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(6, 0)
#include <linux/compat.h>
#define compat_get_timespec	get_compat_timespec
#define compat_put_timespec	put_compat_timespec
#else
static inline int __get_compat_timespec(struct timespec *ts, const struct compat_timespec __user *cts)
{
	return (!access_ok(VERIFY_READ, cts, sizeof(*cts)) ||
		__get_user(ts->tv_sec, &cts->tv_sec) ||
		__get_user(ts->tv_nsec, &cts->tv_nsec)) ? -EFAULT : 0;
}
static inline int __put_compat_timespec(struct timespec *ts, const struct compat_timespec __user *cts)
{
	return (!access_ok(VERIFY_WRITE, cts, sizeof(*cts)) ||
		__put_user(ts->tv_sec, &cts->tv_sec) ||
		__put_user(ts->tv_nsec, &cts->tv_nsec)) ? -EFAULT : 0;
}
#define compat_get_timespec	__get_compat_timespec
#define compat_put_timespec	__put_compat_timespec
#endif /* RHEL */
#endif /* KERNEL_VERSION(3, 4, 0) */
#include "../alsa-kernel/core/pcm_compat.c"
