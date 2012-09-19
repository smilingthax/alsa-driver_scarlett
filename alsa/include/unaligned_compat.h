#ifndef __UNALIGNED_COMPAT_H
#define __UNALIGNED_COMPAT_H

#include <linux/kernel.h>
#include <asm/byteorder.h>

static inline void put_unaligned_le16(u16 val, void *p)
{
	*((__le16 *)p) = cpu_to_le16(val);
}

static inline void put_unaligned_le32(u32 val, void *p)
{
	*((__le32 *)p) = cpu_to_le32(val);
}

static inline void put_unaligned_le64(u64 val, void *p)
{
	*((__le64 *)p) = cpu_to_le64(val);
}

static inline void put_unaligned_be16(u16 val, void *p)
{
	*((__be16 *)p) = cpu_to_be16(val);
}

static inline void put_unaligned_be32(u32 val, void *p)
{
	*((__be32 *)p) = cpu_to_be32(val);
}

static inline void put_unaligned_be64(u64 val, void *p)
{
	*((__be64 *)p) = cpu_to_be64(val);
}

#endif /* __UNALIGNED_COMPAT_H */
