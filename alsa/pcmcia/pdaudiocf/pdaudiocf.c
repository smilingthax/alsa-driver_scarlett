#include <linux/config.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,15)
#include "pdaudiocf_old.c"
#else
#include "../../alsa-kernel/pcmcia/pdaudiocf/pdaudiocf.c"
#endif

