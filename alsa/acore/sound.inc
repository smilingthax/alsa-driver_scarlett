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
EXPORT_SYMBOL(snd_compat_strlcpy);
EXPORT_SYMBOL(snd_compat_strlcat);
#endif
#ifndef CONFIG_HAVE_SNPRINTF
EXPORT_SYMBOL(snd_compat_snprintf);
EXPORT_SYMBOL(snd_compat_vsnprintf);
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
