#include "../alsa-kernel/core/sound.c"

/* misc.c */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
EXPORT_SYMBOL(try_inc_mod_count);
EXPORT_SYMBOL(snd_compat_request_region);
EXPORT_SYMBOL(snd_compat_release_resource);
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 19)
EXPORT_SYMBOL(snd_compat_vmalloc_to_page);
#endif
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
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0) && defined(CONFIG_PM)
EXPORT_SYMBOL(pm_register);
EXPORT_SYMBOL(pm_unregister);
EXPORT_SYMBOL(pm_send);
#endif
  /* wrappers */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
EXPORT_SYMBOL(snd_wrapper_kill_fasync);
#endif
