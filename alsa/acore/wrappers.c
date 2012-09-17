#include "alsa-autoconf.h"
#define __NO_VERSION__
#include <linux/string.h>
#include <linux/sched.h>

#include <linux/module.h>
#include <linux/kmod.h>
#include <linux/devfs_fs_kernel.h>

/* defined in adriver.h but we don't include it... */
#include <linux/compiler.h>
#ifndef __nocast
#define __nocast
#endif

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
EXPORT_SYMBOL(snd_compat_strlcat);
#endif

#ifndef CONFIG_HAVE_VSNPRINTF
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
EXPORT_SYMBOL(snd_compat_vsscanf);

int snd_compat_sscanf(const char *buf, const char *fmt, ...)
{
	int res;
	va_list args;

	va_start(args, fmt);
	res = snd_compat_vsscanf(buf, fmt, args);
	va_end(args);
	return res;
}
EXPORT_SYMBOL(snd_compat_sscanf);
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
EXPORT_SYMBOL(snd_compat_request_module);
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
EXPORT_SYMBOL(snd_compat_msleep);
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
EXPORT_SYMBOL(snd_compat_msleep_interruptible);
#endif /* < 2.6.6 */

/* wrapper for new irq handler type */
#ifndef CONFIG_SND_NEW_IRQ_HANDLER
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/slab.h>
typedef int (*snd_irq_handler_t)(int, void *);
struct irq_list {
	snd_irq_handler_t handler;
	int irq;
	void *data;
	struct list_head list;
};
	
struct pt_regs *snd_irq_regs;
EXPORT_SYMBOL(snd_irq_regs);

#if defined(IRQ_NONE) && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
static irqreturn_t irq_redirect(int irq, void *data, struct pt_regs *reg)
{
	struct irq_list *list = data;
	irqreturn_t val;
	snd_irq_regs = reg;
	val = list->handler(irq, list->data);
	snd_irq_regs = NULL;
	return val;
}
#else
static void irq_redirect(int irq, void *data, struct pt_regs *reg)
{
	struct irq_list *list = data;
	snd_irq_regs = reg;
	list->handler(irq, list->data);
	snd_irq_regs = NULL;
}
#endif

static LIST_HEAD(irq_list_head);
static DEFINE_MUTEX(irq_list_mutex);

int snd_request_irq(unsigned int irq, snd_irq_handler_t handler,
		    unsigned long irq_flags, const char *str, void *data)
{
	struct irq_list *list = kmalloc(sizeof(*list), GFP_KERNEL);
	int err;

	if (!list)
		return -ENOMEM;
	list->handler = handler;
	list->irq = irq;
	list->data = data;
	err = request_irq(irq, irq_redirect, irq_flags, str, list);
	if (err) {
		kfree(list);
		return err;
	}
	mutex_lock(&irq_list_mutex);
	list_add(&list->list, &irq_list_head);
	mutex_unlock(&irq_list_mutex);
	return 0;
}
EXPORT_SYMBOL(snd_request_irq);

void snd_free_irq(unsigned int irq, void *data)
{
	struct list_head *p;

	mutex_lock(&irq_list_mutex);
	list_for_each(p, &irq_list_head) {
		struct irq_list *list = list_entry(p, struct irq_list, list);
		if (list->irq == irq && list->data == data) {
			free_irq(irq, list);
			list_del(p);
			kfree(list);
			break;
		}
	}
	mutex_unlock(&irq_list_mutex);
}
EXPORT_SYMBOL(snd_free_irq);
#endif /* !CONFIG_SND_NEW_IRQ_HANDLER */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 33)
#include <linux/ctype.h>

char *compat_skip_spaces(const char *str)
{
	while (isspace(*str))
		++str;
	return (char *)str;
}
EXPORT_SYMBOL(compat_skip_spaces);
#endif /* < 2.6.33 */

#ifndef CONFIG_GCD
#include <linux/gcd.h>
#ifdef CONFIG_SND_COMPAT_GCD
/* Greatest common divisor */
unsigned long gcd(unsigned long a, unsigned long b)
{
	unsigned long r;
	if (a < b) {
		r = a;
		a = b;
		b = r;
	}
	while ((r = a % b) != 0) {
		a = b;
		b = r;
	}
	return b;
}
EXPORT_SYMBOL(gcd);
#endif /* CONFIG_SND_COMPAT_GCD */
#endif /* !CONFIG_GCD */
