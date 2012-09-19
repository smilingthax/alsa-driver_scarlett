/* misc.c */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#include <linux/device.h>
#include <linux/platform_device.h>
EXPORT_SYMBOL(snd_compat_driver_register);
EXPORT_SYMBOL(snd_compat_driver_unregister);
EXPORT_SYMBOL(snd_platform_device_register_simple);
EXPORT_SYMBOL(snd_platform_bus_type);
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
EXPORT_SYMBOL(try_inc_mod_count);
EXPORT_SYMBOL(snd_compat_request_region);
EXPORT_SYMBOL(snd_compat_release_resource);
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0) && defined(CONFIG_PM)
EXPORT_SYMBOL(pm_register);
EXPORT_SYMBOL(pm_unregister);
EXPORT_SYMBOL(pm_send);
#endif
#if defined(CONFIG_PNP) && defined(CONFIG_PM)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
#ifndef CONFIG_HAVE_PNP_SUSPEND
EXPORT_SYMBOL(snd_pnp_register_driver);
EXPORT_SYMBOL(snd_pnp_register_card_driver);
#endif
#endif
#endif
  /* wrappers */
#ifndef CONFIG_HAVE_STRLCPY
EXPORT_SYMBOL(snd_compat_strlcat);
#endif
#ifndef CONFIG_HAVE_SSCANF
EXPORT_SYMBOL(snd_compat_sscanf);
EXPORT_SYMBOL(snd_compat_vsscanf);
#endif
#ifdef CONFIG_HAVE_OLD_REQUEST_MODULE
EXPORT_SYMBOL(snd_compat_request_module);
#endif
#if defined(CONFIG_DEVFS_FS) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 29)
EXPORT_SYMBOL(snd_compat_devfs_remove);
#endif
#if defined(CONFIG_DEVFS_FS) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 67)
EXPORT_SYMBOL(snd_compat_devfs_mk_dir);
EXPORT_SYMBOL(snd_compat_devfs_mk_cdev);
#endif
#ifndef CONFIG_HAVE_KCALLOC
#ifndef CONFIG_SND_DEBUG_MEMORY
EXPORT_SYMBOL(snd_compat_kcalloc);
#endif
#endif
#ifndef CONFIG_HAVE_KSTRDUP
#ifndef CONFIG_SND_DEBUG_MEMORY
EXPORT_SYMBOL(snd_compat_kstrdup);
#endif
#endif
#ifndef CONFIG_HAVE_KZALLOC
#ifndef CONFIG_SND_DEBUG_MEMORY
EXPORT_SYMBOL(snd_compat_kzalloc);
#endif
#endif
#ifdef CONFIG_PCI
#ifndef CONFIG_HAVE_PCI_DEV_PRESENT
EXPORT_SYMBOL(snd_pci_dev_present);
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
#ifndef CONFIG_HAVE_NEW_PCI_SAVE_STATE
EXPORT_SYMBOL(snd_pci_compat_save_state);
EXPORT_SYMBOL(snd_pci_compat_restore_state);
#endif
#endif
#endif /* CONFIG_PCI */
#ifndef CONFIG_HAVE_MSLEEP
EXPORT_SYMBOL(snd_compat_msleep);
#endif
#ifndef CONFIG_HAVE_MSLEEP_INTERRUPTIBLE
EXPORT_SYMBOL(snd_compat_msleep_interruptible);
#endif
#ifdef SND_WORKQUEUE_COMPAT
EXPORT_SYMBOL(snd_compat_schedule_work);
EXPORT_SYMBOL(snd_compat_create_workqueue);
EXPORT_SYMBOL(snd_compat_flush_workqueue);
EXPORT_SYMBOL(snd_compat_destroy_workqueue);
EXPORT_SYMBOL(snd_compat_queue_work);
EXPORT_SYMBOL(snd_compat_queue_delayed_work);
EXPORT_SYMBOL(snd_compat_cancel_delayed_work);
#endif
#ifdef CONFIG_CREATE_WORKQUEUE_FLAGS
EXPORT_SYMBOL(snd_compat_create_workqueue2);
#endif
#ifdef CONFIG_SND_DEBUG_MEMORY
EXPORT_SYMBOL(snd_hidden_kmalloc);
EXPORT_SYMBOL(snd_hidden_kzalloc);
EXPORT_SYMBOL(snd_hidden_kcalloc);
EXPORT_SYMBOL(snd_hidden_kfree);
EXPORT_SYMBOL(snd_hidden_kstrdup);
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
#include <linux/firmware.h>
#ifdef NEEDS_COMPAT_FW_LOADER
EXPORT_SYMBOL(snd_compat_request_firmware);
EXPORT_SYMBOL(snd_compat_release_firmware);
#endif
#endif
#ifndef CONFIG_SND_NEW_IRQ_HANDLER
EXPORT_SYMBOL(snd_request_irq);
EXPORT_SYMBOL(snd_free_irq);
EXPORT_SYMBOL(snd_irq_regs);
#endif
