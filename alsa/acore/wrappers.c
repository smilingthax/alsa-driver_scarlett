#define __NO_VERSION__
#include "config.h"

#include <linux/version.h>
#include <linux/config.h>

#if defined(CONFIG_MODVERSIONS) && !defined(__GENKSYMS__) && !defined(__DEPEND__)
#define MODVERSIONS
#include <linux/modversions.h>
#include "sndversions.h"
#endif

#include <linux/devfs_fs_kernel.h>

#include "../alsa-kernel/core/wrappers.c"

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 3, 0)

void snd_wrapper_request_region(unsigned long from, unsigned long extent, const char *name)
{
	return request_region(from, extent, name);
}

void snd_wrapper_kill_fasync(struct fasync_struct **fp, int sig, int band)
{
#ifdef CONFIG_OLD_KILL_FASYNC
	kill_fasync(*(fp), sig);
#else
	kill_fasync(*(fp), sig, band);
#endif
}

#if defined(CONFIG_DEVFS_FS) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 29)

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 10)
#define vsnprintf(buf,size,fmt,args) vsprintf(buf,fmt,args)
#endif

void snd_compat_devfs_remove(const char *fmt, ...)
{
	char buf[64];
	va_list args;
	int n;

	va_start(args, fmt);
	n = vsnprintf(buf, 64, fmt, args);
	if (n < 64 && buf[0]) {
		devfs_handle_t de = devfs_find_handle(NULL, buf, 0, 0, 0, 0);
		devfs_unregister(de);
		devfs_put(de);
	}
}

#endif

#endif /* < 2.3.0 */
