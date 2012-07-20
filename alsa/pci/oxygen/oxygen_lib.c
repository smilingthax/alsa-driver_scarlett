#include "adriver.h"
#ifdef SND_COMPAT_DEV_PM_OPS
#define dev_pm_ops snd_compat_dev_pm_ops
#endif
#include "../../alsa-kernel/pci/oxygen/oxygen_lib.c"
