#define __NO_VERSION__
#include "config.h"

#include <linux/version.h>
#include <linux/config.h>

#if defined(CONFIG_MODVERSIONS) && !defined(__GENKSYMS__) && !defined(__DEPEND__)
#define MODVERSIONS
#include <linux/modversions.h>
#include "sndversions.h"
#endif

#include <linux/kmod.h>
#include <linux/devfs_fs_kernel.h>

#include "../alsa-kernel/core/wrappers.c"

#ifndef CONFIG_HAVE_STRLCPY
#define strlcpy snd_compat_strlcpy
#define strlcat snd_compat_strlcat
#ifndef BUG_ON
#define BUG_ON(x) /* nothing */
#endif
size_t snd_compat_strlcpy(char *dest, const char *src, size_t size)
{
	size_t ret = strlen(src);

	if (size) {
		size_t len = (ret >= size) ? size-1 : ret;
		memcpy(dest, src, len);
		dest[len] = '\0';
	}
	return ret;
}

size_t snd_compat_strlcat(char *dest, const char *src, size_t count)
{
	size_t dsize = strlen(dest);
	size_t len = strlen(src);
	size_t res = dsize + len;

	/* This would be a bug */
	BUG_ON(dsize >= count);

	dest += dsize;
	count -= dsize;
	if (len >= count)
		len = count-1;
	memcpy(dest, src, len);
	dest[len] = 0;
	return res;
}
#endif

#ifndef CONFIG_HAVE_SNPRINTF
#define vsnprintf snd_compat_vsnprintf

int snd_compat_vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
	char *ptr = (void *) __get_free_pages(GFP_KERNEL, 0);
	va_list args;
	if (ptr == NULL) {	/* should not happen - GFP_KERNEL has wait flag */
		if (size > 0)
			buf[0] = 0;
		return 0;
	}
	vsprintf(ptr, fmt, args);
	strlcpy(buf, ptr, size);
	free_pages((unsigned long) ptr, 0);
	return strlen(buf);
}

int snd_compat_snprintf(char *buf, size_t size, const char * fmt, ...)
{
	int res;

	va_start(args, fmt);
	res = snd_compat_vsnprintf(buf, size, args);
	va_end(args);
	return res;
}
#endif

#ifdef CONFIG_HAVE_OLD_REQUEST_MODULE
void snd_compat_request_module(const char *fmt, ...)
{
	char buf[64];
	va_list args;
	int n;

	va_start(args, fmt);
	n = vsnprintf(buf, 64, fmt, args);
	if (n < 64 && buf[0])
		request_module(buf);
	va_end(args);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 3, 0)

void snd_wrapper_request_region(unsigned long from, unsigned long extent, const char *name)
{
	return request_region(from, extent, name);
}

#ifdef CONFIG_OLD_KILL_FASYNC
void snd_wrapper_kill_fasync(struct fasync_struct **fp, int sig, int band)
{
	kill_fasync(*(fp), sig);
}
#endif

#endif /* < 2.3.0 */

#if defined(CONFIG_DEVFS_FS)

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 29)

void snd_compat_devfs_remove(const char *fmt, ...)
{
	char buf[64];
	va_list args;
	int n;

	va_start(args, fmt);
	n = vsnprintf(buf, 64, fmt, args);
	if (n < 64 && buf[0]) {
		devfs_handle_t de = devfs_get_handle(NULL, buf, 0, 0, 0, 0);
		devfs_unregister(de);
		devfs_put(de);
	}
	va_end(args);
}

#endif /* 2.5.29 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 67)

int snd_compat_devfs_mk_dir(const char *dir, ...)
{
	char buf[64];
	va_list args;
	int n;

	va_start(args, dir);
	n = vsnprintf(buf, 64, dir, args);
	va_end(args);
	if (n < 64 && buf[0]) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
		return devfs_mk_dir(NULL, buf, strlen(dir), NULL) ? -EIO : 0;
#else
		return devfs_mk_dir(NULL, buf, NULL) ? -EIO : 0;
#endif
	}
	return 0;
}

extern struct file_operations snd_fops;
int snd_compat_devfs_mk_cdev(dev_t dev, umode_t mode, const char *fmt, ...)
{
	char buf[64];
	va_list args;
	int n;

	va_start(args, fmt);
	n = vsnprintf(buf, 64, fmt, args);
	va_end(args);
	if (n < 64 && buf[0]) {
		devfs_register(NULL, buf, DEVFS_FL_DEFAULT,
			       major(dev), minor(dev), mode,
			       &snd_fops, NULL);
	}
	return 0;
}

#endif /* 2.5.67 */

#endif /* CONFIG_DEVFS_FS */
