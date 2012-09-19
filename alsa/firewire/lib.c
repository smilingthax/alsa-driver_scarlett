#include "adriver.h"
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 5, 0)
const char *fw_rcode_string(int rcode);
#endif
#include "../alsa-kernel/firewire/lib.c"

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 5, 0)
const char *fw_rcode_string(int rcode)
{
	static const char *const names[] = {
		[RCODE_COMPLETE]       = "no error",
		[RCODE_CONFLICT_ERROR] = "conflict error",
		[RCODE_DATA_ERROR]     = "data error",
		[RCODE_TYPE_ERROR]     = "type error",
		[RCODE_ADDRESS_ERROR]  = "address error",
		[RCODE_SEND_ERROR]     = "send error",
		[RCODE_CANCELLED]      = "timeout",
		[RCODE_BUSY]           = "busy",
		[RCODE_GENERATION]     = "bus reset",
		[RCODE_NO_ACK]         = "no ack",
	};

	if ((unsigned int)rcode < ARRAY_SIZE(names) && names[rcode])
		return names[rcode];
	else
		return "unknown";
}
#endif /* < 3.5.0 */
