#include "../alsa-kernel/core/wrappers.c"

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 3, 0)

void snd_wrapper_request_region(unsigned long from, unsigned long extent, const char *name)
{
	return request_region(from, extent, name);
}

void snd_wrapper_kill_fasync(struct fasync_struct **fp, int sig, int band)
{
#ifdef CONFIG_OLD_KILL_FASYNC
	kill_fasync(*(fp), sig);
#else
	kill_fasync(*(fp), sig, band);
#endif
}

#endif /* < 2.3.0 */
