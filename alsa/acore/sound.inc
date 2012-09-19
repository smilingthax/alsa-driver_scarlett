/* misc.c */
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
#ifdef CONFIG_OLD_KILL_FASYNC
EXPORT_SYMBOL(snd_wrapper_kill_fasync);
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
#ifndef CONFIG_HAVE_PCI_DEV_PRESENT
EXPORT_SYMBOL(snd_pci_dev_present);
#endif
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
#endif
#ifdef CONFIG_CREATE_WORKQUEUE_FLAGS
EXPORT_SYMBOL(snd_compat_create_workqueue2);
#endif
