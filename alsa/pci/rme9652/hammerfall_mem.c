/*
 *  Hacks for compilation for ALSA BUILD
 */

#include <linux/config.h>
#include <linux/version.h>
#if defined(ALSA_BUILD) && defined(CONFIG_MODVERSIONS) && !defined(__GENKSYMS__) && !defined(__DEPEND__)
#define MODVERSIONS
#include <linux/modversions.h>
#include "sndversions.h"
#endif
#include <linux/module.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 3, 0)
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/malloc.h>
#include <linux/init.h>
#include <asm/io.h>
#include "config.h"
#ifndef CONFIG_HAVE_DMA_ADDR_T
typedef unsigned long dma_addr_t;
#endif
#ifndef __init
#define __init
#endif
#ifndef __exit
#define __exit
#endif
#ifndef module_init
#define module_init(x)      int init_module(void) { return x(); }
#endif
#ifndef module_exit
#define module_exit(x)      void cleanup_module(void) { x(); }
#endif        
#define virt_to_page(x) (&mem_map[MAP_NR(x)])
#define pci_for_each_dev(dev) \
	for(dev = pci_devices; dev; dev = dev->next)
#endif

#ifndef MODULE_LICENSE
#define MODULE_LICENSE(x)
#endif

#include "../../alsa-kernel/pci/rme9652/hammerfall_mem.c"
