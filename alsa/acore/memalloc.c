#include <linux/config.h>
#include <linux/version.h>

#if defined(CONFIG_MODVERSIONS) && !defined(__GENKSYMS__) && !defined(__DEPEND__)
#define MODVERSIONS
#include <linux/modversions.h>
#include "sndversions.h"
#endif

#include "config.h"
#include "adriver.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0) && defined(CONFIG_PROC_FS)
#include <linux/proc_fs.h>
static inline struct proc_dir_entry *create_proc_read_entry(const char *name,
	mode_t mode, struct proc_dir_entry *base, 
	read_proc_t *read_proc, void * data)
{
	struct proc_dir_entry *res=create_proc_entry(name,mode,base);
	if (res) {
		res->read_proc=read_proc;
		res->data=data;
	}
	return res;
}
#endif

#include "../alsa-kernel/core/memalloc.c"

/* compatible functions */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0) && defined(CONFIG_PCI)
EXPORT_SYMBOL(snd_pci_compat_match_device);
EXPORT_SYMBOL(snd_pci_compat_register_driver);
EXPORT_SYMBOL(snd_pci_compat_unregister_driver);
EXPORT_SYMBOL(snd_pci_compat_get_size);
EXPORT_SYMBOL(snd_pci_compat_get_flags);
EXPORT_SYMBOL(snd_pci_compat_set_power_state);
EXPORT_SYMBOL(snd_pci_compat_enable_device);
EXPORT_SYMBOL(snd_pci_compat_find_capability);
EXPORT_SYMBOL(snd_pci_compat_alloc_consistent);
EXPORT_SYMBOL(snd_pci_compat_free_consistent);
EXPORT_SYMBOL(snd_pci_compat_dma_supported);
EXPORT_SYMBOL(snd_pci_compat_get_dma_mask);
EXPORT_SYMBOL(snd_pci_compat_set_dma_mask);
EXPORT_SYMBOL(snd_pci_compat_get_driver_data);
EXPORT_SYMBOL(snd_pci_compat_set_driver_data);
EXPORT_SYMBOL(snd_pci_compat_get_pci_driver);
#endif

#ifndef CONFIG_HAVE_VMALLOC_TO_PAGE
EXPORT_SYMBOL(snd_compat_vmalloc_to_page);
#endif
