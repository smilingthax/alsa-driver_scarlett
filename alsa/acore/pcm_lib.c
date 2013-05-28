#define __NO_VERSION__
#include "adriver.h"
#if defined(RHEL_RELEASE_CODE) && RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(6, 0)
#include <linux/math64.h>
static inline struct timespec __ns_to_timespec(const s64 nsec)
{
	struct timespec ts;
	s32 rem;

	if (!nsec)
		return (struct timespec) {0, 0};

	ts.tv_sec = div_s64_rem(nsec, NSEC_PER_SEC, &rem);
	if (unlikely(rem < 0)) {
		ts.tv_sec--;
		rem += NSEC_PER_SEC;
	}
	ts.tv_nsec = rem;

	return ts;
}
#define ns_to_timespec __ns_to_timespec
#endif /* RHEL */

#include "../alsa-kernel/core/pcm_lib.c"

#ifdef CONFIG_SND_BIT32_EMUL_MODULE
int snd_pcm_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params);
EXPORT_SYMBOL(snd_pcm_hw_params);
#endif
