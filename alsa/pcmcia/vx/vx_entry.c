#define __NO_VERSION__
#include <linux/config.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
#include <pcmcia/cs_types.h>
static void cs_error(client_handle_t handle, int func, int ret);
#endif

#include "../../alsa-kernel/pcmcia/vx/vx_entry.c"

/*
 * print the error message related with cs
 */
static void cs_error(client_handle_t handle, int func, int ret)
{
	error_info_t err = { func, ret };
	CardServices(ReportError, handle, &err);
}
