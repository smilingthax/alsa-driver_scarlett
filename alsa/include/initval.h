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

#ifndef SND_SKIP_EXPORT_NO_SYMBOLS
EXPORT_NO_SYMBOLS;
#endif

static inline int snd_legacy_auto_probe(int *ports, int (*probe)(int port))
{
	int result = 0;	/* number of detected cards */

	while (*ports >= 0) {
		if (probe(*ports) >= 0)
			result++;
		ports++;
	}
	return result;
}

#endif				/* __INITVAL_H */
