#ifndef __PCM_PARAMS_H
#define __PCM_PARAMS_H

/*
 *  PCM params helpers
 *  Copyright (c) by Abramo Bagnara <abramo@alsa-project.org>
 *
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

/* To share the same code we have  alsa-lib */
#define mask_bits(mask) (*(mask))
#define INLINE static inline
#define assert(a)

INLINE unsigned int ld2(u_int32_t v)
{
        unsigned r = 0;

        if (v >= 0x10000) {
                v >>= 16;
                r += 16;
        }
        if (v >= 0x100) {
                v >>= 8;
                r += 8;
        }
        if (v >= 0x10) {
                v >>= 4;
                r += 4;
        }
        if (v >= 4) {
                v >>= 2;
                r += 2;
        }
        if (v >= 2)
                r++;
        return r;
}

INLINE size_t mask_sizeof(void)
{
	return sizeof(mask_t);
}

INLINE void mask_none(mask_t *mask)
{
	mask_bits(mask) = 0;
}

INLINE void mask_all(mask_t *mask)
{
	mask_bits(mask) = ~0U;
}

INLINE void mask_load(mask_t *mask, unsigned int msk)
{
	mask_bits(mask) = msk;
}

INLINE int mask_empty(const mask_t *mask)
{
	return mask_bits(mask) == 0;
}

INLINE unsigned int mask_count(const mask_t *mask)
{
	return hweight32(mask_bits(mask));
}

INLINE unsigned int mask_min(const mask_t *mask)
{
	assert(!mask_empty(mask));
	return ffs(mask_bits(mask)) - 1;
}

INLINE unsigned int mask_max(const mask_t *mask)
{
	assert(!mask_empty(mask));
	return ld2(mask_bits(mask));
}

INLINE void mask_set(mask_t *mask, unsigned int val)
{
	assert(val <= MASK_MAX);
	mask_bits(mask) |= (1U << val);
}

INLINE void mask_reset(mask_t *mask, unsigned int val)
{
	assert(val <= MASK_MAX);
	mask_bits(mask) &= ~(1U << val);
}

INLINE void mask_set_range(mask_t *mask, unsigned int from, unsigned int to)
{
	assert(to <= MASK_MAX && from <= to);
	mask_bits(mask) |= ((1U << (from - to + 1)) - 1) << from;
}

INLINE void mask_reset_range(mask_t *mask, unsigned int from, unsigned int to)
{
	assert(to <= MASK_MAX && from <= to);
	mask_bits(mask) &= ~(((1U << (from - to + 1)) - 1) << from);
}

INLINE void mask_leave(mask_t *mask, unsigned int val)
{
	assert(val <= MASK_MAX);
	mask_bits(mask) &= 1U << val;
}

INLINE void mask_intersect(mask_t *mask, const mask_t *v)
{
	mask_bits(mask) &= mask_bits(v);
}

INLINE int mask_eq(const mask_t *mask, const mask_t *v)
{
	return mask_bits(mask) == mask_bits(v);
}

INLINE void mask_copy(mask_t *mask, const mask_t *v)
{
	mask_bits(mask) = mask_bits(v);
}

INLINE int mask_test(const mask_t *mask, unsigned int val)
{
	assert(val <= MASK_MAX);
	return mask_bits(mask) & (1U << val);
}

INLINE int mask_single(const mask_t *mask)
{
	assert(!mask_empty(mask));
	return !(mask_bits(mask) & (mask_bits(mask) - 1));
}

INLINE int mask_refine(mask_t *mask, const mask_t *v)
{
	mask_t old;
	assert(!mask_empty(mask));
	mask_copy(&old, mask);
	mask_intersect(mask, v);
	if (mask_empty(mask))
		return -EINVAL;
	return !mask_eq(mask, &old);
}

INLINE int mask_refine_first(mask_t *mask)
{
	assert(!mask_empty(mask));
	if (mask_single(mask))
		return 0;
	mask_leave(mask, mask_min(mask));
	return 1;
}

INLINE int mask_refine_last(mask_t *mask)
{
	assert(!mask_empty(mask));
	if (mask_single(mask))
		return 0;
	mask_leave(mask, mask_max(mask));
	return 1;
}

INLINE int mask_refine_min(mask_t *mask, unsigned int val)
{
	assert(!mask_empty(mask));
	if (mask_min(mask) >= val)
		return 0;
	mask_reset_range(mask, 0, val - 1);
	if (mask_empty(mask))
		return -EINVAL;
	return 1;
}

INLINE int mask_refine_max(mask_t *mask, unsigned int val)
{
	assert(!mask_empty(mask));
	if (mask_max(mask) <= val)
		return 0;
	mask_reset_range(mask, val + 1, MASK_MAX);
	if (mask_empty(mask))
		return -EINVAL;
	return 1;
}

INLINE int mask_refine_set(mask_t *mask, unsigned int val)
{
	int changed;
	assert(!mask_empty(mask));
	changed = !mask_single(mask);
	mask_leave(mask, val);
	if (mask_empty(mask))
		return -EINVAL;
	return changed;
}

INLINE int mask_value(const mask_t *mask)
{
	assert(!mask_empty(mask));
	return mask_min(mask);
}

INLINE void interval_all(interval_t *i)
{
	i->min = 1;
	i->max = UINT_MAX;
}

INLINE int interval_empty(const interval_t *i)
{
	return i->empty;
}

INLINE int interval_single(const interval_t *i)
{
	assert(!interval_empty(i));
	return (i->min == i->max || 
		(i->min + 1 == i->max && i->openmax));
}

INLINE int interval_value(const interval_t *i)
{
	assert(interval_single(i));
	return i->min;
}

INLINE int interval_min(const interval_t *i)
{
	assert(!interval_empty(i));
	return i->min;
}

INLINE int interval_max(const interval_t *i)
{
	unsigned int v;
	assert(!interval_empty(i));
	v = i->max;
	if (i->openmax)
		v--;
	return v;
}

INLINE int interval_test(const interval_t *i, unsigned int val)
{
	return !((i->min > val || (i->min == val && i->openmin) ||
		  i->max < val || (i->max == val && i->openmax)));
}

INLINE void interval_copy(interval_t *d, const interval_t *s)
{
	*d = *s;
}

INLINE void interval_setreal(interval_t *i)
{
	i->real = 1;
}

INLINE int interval_eq(const interval_t *i1, const interval_t *i2)
{
	if (i1->empty)
		return i2->empty;
	if (i2->empty)
		return i1->empty;
	return i1->min == i2->min && i1->openmin == i2->openmin &&
		i1->max == i2->max && i1->openmax == i2->openmax;
}

static inline unsigned int add(unsigned int a, unsigned int b)
{
	if (a >= UINT_MAX - b)
		return UINT_MAX;
	return a + b;
}

static inline unsigned int sub(unsigned int a, unsigned int b)
{
	if (a > b)
		return a - b;
	return 0;
}

#undef INLINE
#undef assert

#endif
