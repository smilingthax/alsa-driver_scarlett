#define __NO_VERSION__

#include <linux/config.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,5)
#define __KERNEL_SYSCALLS__
#include <linux/unistd.h>
static int errno;
#endif

#include "../../alsa-kernel/isa/wavefront/wavefront_synth.c"
