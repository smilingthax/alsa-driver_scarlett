#ifndef __INITVAL_H
#define __INITVAL_H

/*
 *  Init values for soundcard modules
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef MODULE_GENERIC_STRING
#ifdef MODULE
#define MODULE_GENERIC_STRING(name, string) \
static const char __module_generic_string_##name [] \
  __attribute__ ((section(".modstring"))) = #name "=" string;
#else
#define MODULE_GENERIC_STRING(name, string)
#endif
#endif

#define MODULE_CLASSES(val) MODULE_GENERIC_STRING(info_classes, val)
#define MODULE_DEVICES(val) MODULE_GENERIC_STRING(info_devices, val)
#define MODULE_PARM_SYNTAX(id, val) MODULE_GENERIC_STRING(info_parm_##id, val)

#define SND_AUTO_PORT		0xffff
#define SND_AUTO_IRQ		0xffff
#define SND_AUTO_DMA		0xffff
#define SND_AUTO_DMA_SIZE	(0x7fffffff)

#define SND_DEFAULT_IDX1	(-1)
#define SND_DEFAULT_STR1	NULL
#define SND_DEFAULT_ENABLE1	1
#define SND_DEFAULT_PORT1	SND_AUTO_PORT
#define SND_DEFAULT_IRQ1	SND_AUTO_IRQ
#define SND_DEFAULT_DMA1	SND_AUTO_DMA
#define SND_DEFAULT_DMA_SIZE1	SND_AUTO_DMA_SIZE
#define SND_DEFAULT_PTR1	SND_DEFAULT_STR1

#define SND_DEFAULT_IDX		{ [0 ... (SND_CARDS-1)] = -1 }
#define SND_DEFAULT_STR		{ [0 ... (SND_CARDS-1)] = NULL }
#define SND_DEFAULT_ENABLE	{ 1, [1 ... (SND_CARDS-1)] = 0 }
#define SND_DEFAULT_PORT	{ SND_AUTO_PORT, [1 ... (SND_CARDS-1)] = -1 }
#define SND_DEFAULT_IRQ		{ [0 ... (SND_CARDS-1)] = SND_AUTO_IRQ }
#define SND_DEFAULT_DMA		{ [0 ... (SND_CARDS-1)] = SND_AUTO_DMA }
#define SND_DEFAULT_DMA_SIZE	{ [0 ... (SND_CARDS-1)] = SND_AUTO_DMA_SIZE }
#define SND_DEFAULT_PTR		SND_DEFAULT_STR

#define SND_BOOLEAN_TRUE_DESC	"allows:{{0,Disabled},{1,Enabled}},default:1,dialog:check"
#define SND_BOOLEAN_FALSE_DESC	"allows:{{0,Disabled},{1,Enabled}},default:0,dialog:check"

#define SND_ENABLED		"enable:(snd_enable)"

#define SND_INDEX_DESC		SND_ENABLED ",allows:{{0,7}},unique,skill:required,dialog:list"
#define SND_ID_DESC		SND_ENABLED ",unique"
#define SND_ENABLE_DESC		SND_BOOLEAN_FALSE_DESC
#define SND_ISAPNP_DESC		SND_ENABLED "," SND_BOOLEAN_TRUE_DESC
#define SND_DMA8_DESC		SND_ENABLED ",allows:{{0,1},{3}},dialog:list"
#define SND_DMA16_DESC		SND_ENABLED ",allows:{{5,7}},dialog:list"
#define SND_DMA_DESC		SND_ENABLED ",allows:{{0,1},{3},{5,7}},dialog:list"
#define SND_IRQ_DESC		SND_ENABLED ",allows:{{5},{7},{9},{10,12},{14,15}},dialog:list"
#define SND_DMA_SIZE_DESC	SND_ENABLED ",allows:{{4,128}},default:64,skill:advanced"
#define SND_DMA8_SIZE_DESC	SND_ENABLED ",allows:{{4, 64}},default:64,skill:advanced"
#define SND_DMA16_SIZE_DESC	SND_ENABLED ",allows:{{4,128}},default:64,skill:advanced"
#define SND_PORT12_DESC		SND_ENABLED ",allows:{{0,0x3fff}},base:16"
#define SND_PORT_DESC		SND_ENABLED ",allows:{{0,0xffff}},base:16"

#ifdef SND_LEGACY_AUTO_PROBE
static int snd_legacy_auto_probe(unsigned long *ports, int (*probe)(unsigned long port))
{
	int result = 0;	/* number of detected cards */

	while ((signed long)*ports != -1) {
		if (probe(*ports) >= 0)
			result++;
		ports++;
	}
	return result;
}
#endif

#ifdef SND_LEGACY_FIND_FREE_IOPORT
static long snd_legacy_find_free_ioport(long *port_table, long size)
{
	while (*port_table != -1) {
		if (!check_region(*port_table, size))
			return *port_table;
		port_table++;
	}
	return -1;
}
#endif

#ifdef SND_LEGACY_FIND_FREE_IRQ
static void snd_legacy_empty_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
}

static int snd_legacy_find_free_irq(int *irq_table)
{
	while (*irq_table != -1) {
		if (!request_irq(*irq_table, snd_legacy_empty_irq_handler,
				 SA_INTERRUPT, "ALSA Test IRQ", (void *) irq_table)) {
			free_irq(*irq_table, (void *) irq_table);
			return *irq_table;
		}
		irq_table++;
	}
	return -1;
}
#endif

#ifdef SND_LEGACY_FIND_FREE_DMA
static int snd_legacy_find_free_dma(int *dma_table)
{
	while (*dma_table != -1) {
		if (!request_dma(*dma_table, "ALSA Test DMA")) {
			free_dma(*dma_table);
			return *dma_table;
		}
		dma_table++;
	}
	return -1;
}
#endif

static inline unsigned long snd_dma_size(int size_kB, int min_kB, int max_kB)
{
	if (size_kB < min_kB)
		size_kB = min_kB;
	if (size_kB > max_kB)
		size_kB = max_kB;
	return (unsigned long)size_kB * 1024;
}

static inline int snd_legacy_dma_size(int dma, unsigned long size_kB)
{
	return snd_dma_size(size_kB, PAGE_SIZE / 1024, dma < 4 ? 64 : 128);
}

#endif				/* __INITVAL_H */
