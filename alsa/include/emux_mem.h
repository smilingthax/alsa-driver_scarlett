#ifndef __EMUX_MEM_H
#define __EMUX_MEM_H
/*
 *  Copyright (C) 2000 Takashi Iwai <iwai@ww.uni-erlangen.de>
 *
 *  Memory management routines for control of EMU WaveTable chip
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
 */

typedef struct snd_emux_memblk snd_emux_memblk_t;
typedef struct snd_emux_memhdr snd_emux_memhdr_t;
typedef unsigned int snd_emux_unit_t;

/*
 * memory block
 */
struct snd_emux_memblk {
	snd_emux_unit_t size;		/* size of this block */
	snd_emux_unit_t offset;		/* zero-offset of this block */
	snd_emux_memblk_t *next;	/* next block */
};

#define snd_emux_memblk_argptr(blk)	(void*)((char*)(blk) + sizeof(snd_emux_memblk_t))

/*
 * memory management information
 */
struct snd_emux_memhdr {
	snd_emux_unit_t size;		/* size of whole data */
	snd_emux_memblk_t *block;	/* block linked-list */
	int nblocks;			/* # of allocated blocks */
	snd_emux_unit_t used;		/* used memory size */
	int block_extra_size;		/* extra data size of chunk */
	struct semaphore block_mutex;	/* lock */
};

/*
 * prototypes
 */
snd_emux_memhdr_t *snd_emux_memhdr_new(int memsize);
void snd_emux_memhdr_free(snd_emux_memhdr_t *hdr);
snd_emux_memblk_t *snd_emux_mem_alloc(snd_emux_memhdr_t *hdr, int size);
int snd_emux_mem_free(snd_emux_memhdr_t *hdr, snd_emux_memblk_t *blk);
int snd_emux_mem_avail(snd_emux_memhdr_t *hdr);

/* functions without mutex */
snd_emux_memblk_t *__snd_emux_mem_alloc(snd_emux_memhdr_t *hdr, int size, snd_emux_memblk_t **prevp);
int __snd_emux_mem_find_prev(snd_emux_memhdr_t *hdr, snd_emux_memblk_t *blk, snd_emux_memblk_t **prevp);
void __snd_emux_mem_free(snd_emux_memhdr_t *hdr, snd_emux_memblk_t *blk, snd_emux_memblk_t *prev);
snd_emux_memblk_t *__snd_emux_memblk_new(snd_emux_memhdr_t *hdr, snd_emux_unit_t units, snd_emux_memblk_t *prev);

#endif /* __EMUX_MEM_H */
