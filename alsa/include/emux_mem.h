#ifndef __EMUX_MEM_H
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
typedef struct snd_emux_memchunk snd_emux_memchunk_t;
typedef struct snd_emux_memhdr snd_emux_memhdr_t;
typedef struct snd_emux_mem_ops snd_emux_mem_ops_t;
typedef unsigned int snd_emux_unit_t;

/*
 * block type
 */
typedef enum {
	SND_EMUX_MEM_EMPTY = 0,	/* block is empty */
	SND_EMUX_MEM_USED	/* block is used */
} snd_emux_memblk_type_t;

/*
 * memory block
 */
struct snd_emux_memblk {
	snd_emux_memblk_type_t type;	/* availability (see above) */
	snd_emux_memchunk_t *chunk;	/* parent chunk */
	snd_emux_unit_t size;		/* (unit) size of this block */
	snd_emux_unit_t offset;		/* (unit) offset of this block (zero-offset) */
	snd_emux_memblk_t *next;	/* next block */
};

/*
 * memory chunk
 */
struct snd_emux_memchunk {
	snd_emux_unit_t size;		/* (unit) size of this chunk */
	snd_emux_unit_t max_empty;	/* max size of empty block */
	snd_emux_memhdr_t *hdr;		/* parent memory header */
	int nempties;			/* # of empty blocks */
	int nused;			/* # of used blocks */
	int locked;			/* chunk is locked to be freed */
	snd_emux_memblk_t *block;	/* block linked-list */
	void *buffer;			/* allocated (physical) buffer */
	unsigned int offset;		/* offset of allocated buffer */
	snd_emux_memchunk_t *next;	/* next chunk */
};

/*
 * chunk allocation operators
 * return zero if succeeded
 */
struct snd_emux_mem_ops {
	int (*alloc)(snd_emux_memhdr_t *hdr, snd_emux_memchunk_t *chunk, snd_emux_unit_t size);
	int (*free)(snd_emux_memhdr_t *hdr, snd_emux_memchunk_t *chunk);
};

/*
 * memory management information
 */
struct snd_emux_memhdr {
	int unitsize;			/* alignment size */
	snd_emux_memchunk_t *chunk;	/* chunk linked-list */
	int nchunks;			/* # of allocated chunks */
	snd_emux_mem_ops_t ops;		/* operators */
	void *private_data;		/* private data passed to operators */
};

/*
 * prototypes
 */
snd_emux_memblk_t *snd_emux_mem_alloc(snd_emux_memhdr_t *hdr, int size);
int snd_emux_mem_free(snd_emux_memhdr_t *hdr, snd_emux_memblk_t *blk);
snd_emux_memchunk_t *snd_emux_memchunk_new(snd_emux_memhdr_t *hdr);
int snd_emux_memchunk_free(snd_emux_memhdr_t *hdr, snd_emux_memchunk_t *chunk);

#endif /* __EMUX_MEM_H */
