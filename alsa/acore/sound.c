#include "../alsa-kernel/core/sound.c"

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
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
EXPORT_SYMBOL(snd_wrapper_kill_fasync);
#endif
