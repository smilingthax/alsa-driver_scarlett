#define __NO_VERSION__
#include "config.h"

#include <linux/version.h>
#include <linux/config.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#if defined(CONFIG_MODVERSIONS) && !defined(__GENKSYMS__) && !defined(__DEPEND__)
#define MODVERSIONS
#include <linux/modversions.h>
#include "sndversions.h"
#endif
#endif

#include <linux/kmod.h>
#include <linux/devfs_fs_kernel.h>

#include "../alsa-kernel/core/wrappers.c"

#ifndef CONFIG_HAVE_STRLCPY
#define strlcat snd_compat_strlcat
#ifndef BUG_ON
#define BUG_ON(x) /* nothing */
#endif
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
int snd_compat_vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
#endif

#ifndef CONFIG_HAVE_SSCANF
#include <linux/ctype.h>

/* this function supports any format as long as it's %x  :-) */
int snd_compat_vsscanf(const char *buf, const char *fmt, va_list args)
{
	const char *str = buf;
	char *next;
	int num = 0;
	unsigned int *p;

	while (*fmt && *str) {
		while (isspace(*fmt))
			++fmt;

		if (!*fmt)
			break;

		if (fmt[0] != '%' || fmt[1] != 'x') {
			printk(KERN_ERR "snd_compat_vsscanf: format isn't %%x\n");
			return 0;
		}
		fmt += 2;

		while (isspace(*str))
			++str;

		if (!*str || !isxdigit(*str))
			break;

		p = (unsigned int*) va_arg(args, unsigned int*);
		*p = (unsigned int) simple_strtoul(str, &next, 0x10);
		++num;

		if (!next)
			break;
		str = next;
	}
	return num;
}

int snd_compat_sscanf(const char *buf, const char *fmt, ...)
{
	int res;
	va_list args;

	va_start(args, fmt);
	res = snd_compat_vsscanf(buf, fmt, args);
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

#ifndef CONFIG_HAVE_PCI_DEV_PRESENT
#include <linux/pci.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 3, 0)
/* for pci_device_id compatibility layer */
#include "compat_22.h"
#endif
int snd_pci_dev_present(const struct pci_device_id *ids)
{
	while (ids->vendor || ids->subvendor) {
		if (pci_find_device(ids->vendor, ids->subvendor, NULL))
			return 1;
		ids++;
	}
	return 0;
}
#endif

/*
 * msleep wrapper
 */
#ifndef CONFIG_HAVE_MSLEEP
#include <linux/delay.h>
void snd_compat_msleep(unsigned int msecs)
{
	unsigned long timeout = ((msecs) * HZ + 999) / 1000;

	while (timeout) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		timeout = schedule_timeout(timeout);
	}
}
#endif

#ifndef CONFIG_HAVE_MSLEEP_INTERRUPTIBLE
#include <linux/delay.h>
unsigned long snd_compat_msleep_interruptible(unsigned int msecs)
{
	unsigned long timeout = ((msecs) * HZ + 999) / 1000;

	while (timeout && !signal_pending(current)) {
		set_current_state(TASK_INTERRUPTIBLE);
		timeout = schedule_timeout(timeout);
	}
	return (timeout * 1000) / HZ;
}
#endif /* < 2.6.6 */
