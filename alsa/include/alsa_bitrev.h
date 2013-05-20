#ifndef _ALSA_BITREV_H
#define _ALSA_BITREV_H
#ifndef CONFIG_BITREVERSE

#include <linux/types.h>

static u8 bitrev8(u8 val)
{
	int bit;
	unsigned char res = 0;
	for (bit = 0; bit < 8; bit++) {
		res <<= 1;
		res |= val & 1;
		val >>= 1;
	}
	return res;
}

static inline u16 bitrev16(u16 x)
{
	return (bitrev8(x & 0xff) << 8) | bitrev8(x >> 8);
}

static inline u32 bitrev32(u32 x)
{
	return (bitrev16(x & 0xffff) << 16) | bitrev16(x >> 16);
}

#endif /* CONFIG_BITREVERSE */
#endif /* _ALSA_BITREV_H */
