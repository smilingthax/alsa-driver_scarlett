#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35)
#include "../../alsa-kernel/pcmcia/pdaudiocf/pdaudiocf.h"
#elif LINUX_VERSION_CODE > KERNEL_VERSION(2,6,16)
#include "pdaudiocf-2.6.34.h"
#else
#include "pdaudiocf-old.h"
#endif
