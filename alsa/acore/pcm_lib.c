#define __NO_VERSION__
#include <sound/driver.h>

#include "../alsa-kernel/core/pcm_lib.c"

#ifdef CONFIG_SND_BIT32_EMUL_MODULE
int snd_pcm_hw_params(snd_pcm_substream_t *substream, snd_pcm_hw_params_t *params);
EXPORT_SYMBOL(snd_pcm_hw_params);
#endif
