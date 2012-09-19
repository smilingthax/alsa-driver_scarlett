#include "alsa-autoconf.h"

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,15)
#include "pdaudiocf_old.c"
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,16)
#include "pdaudiocf-2.6.16.c"
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,34)
#include "pdaudiocf-2.6.34.c"
#else
#include "adriver.h"
#include "../../alsa-kernel/pcmcia/pdaudiocf/pdaudiocf.c"
#endif

