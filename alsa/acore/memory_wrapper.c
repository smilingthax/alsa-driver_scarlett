#define __NO_VERSION__
#include <linux/config.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#if defined(CONFIG_MODVERSIONS) && !defined(__GENKSYMS__) && !defined(__DEPEND__)
#define MODVERSIONS
#include <linux/modversions.h>
#include "sndversions.h"
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 0)
#include <linux/ioport.h>
static inline void snd_memory_wrapper_request_region(unsigned long from, unsigned long extent, const char *name)
{
	request_region(from, extent, name);
}
#endif

#include "config.h"
#include "adriver.h"
#include <linux/mm.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 0)
#include <sound/memalloc.h>
#include "pci_compat_22.c"
#endif

/* vmalloc_to_page wrapper */
#ifndef CONFIG_HAVE_VMALLOC_TO_PAGE
#include <linux/highmem.h>
struct page *snd_compat_vmalloc_to_page(void *pageptr)
{
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *pte;
	unsigned long lpage;
	struct page *page;

	lpage = VMALLOC_VMADDR(pageptr);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
	spin_lock(&init_mm.page_table_lock);
#endif
	pgd = pgd_offset(&init_mm, lpage);
	pmd = pmd_offset(pgd, lpage);
	pte = pte_offset(pmd, lpage);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
	page = virt_to_page(pte_page(*pte));
#else
	page = pte_page(*pte);
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
	spin_unlock(&init_mm.page_table_lock);
#endif

	return page;
}    
#endif

#ifndef CONFIG_HAVE_STRLCPY
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
#endif

#ifndef CONFIG_HAVE_SNPRINTF
int snd_compat_vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
	char *ptr = (void *) __get_free_pages(GFP_KERNEL, 0);
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
	va_list args;

	va_start(args, fmt);
	res = snd_compat_vsnprintf(buf, size, fmt, args);
	va_end(args);
	return res;
}
#endif
