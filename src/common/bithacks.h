/* 
 * bithacks.h
 * by WN @ Nov. 20, 2009
 */

#ifndef __BITHACKS_H
#define __BITHACKS_H
#include <stdint.h>

static inline int
is_power_of_2(uint32_t x)
{
	return (!(x & (x - 1)));
}

static inline int
count_1s(uint32_t c1)
{
	register uint32_t c2 = (c1 >> 1) & 033333333333;
	c2 = c1 - c2 - ((c2 >> 1) & 033333333333);
	return (((c2 + (c2 >> 3)) & 030707070707) % 077);
}

static inline int
count_1s_64(uint64_t c1)
{
	uint64_t c2 = (c1 >> 1) & 0x7777777777777777ULL;
	uint64_t c3 = (c1 >> 2) & 0x3333333333333333ULL;
	uint64_t c4 = (c1 >> 3) & 0x1111111111111111ULL;
	c1 = c1 - c2 - c3 - c4;
	return ((c1 + (c1 >> 4)) & 0x0F0F0F0F0F0F0F0FULL) % 0xFF;
}

static inline int
pow2roundup (int x)
{
    if (x <= 0)
        return 0;
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x+1;
}

static inline int
pow2rounddown (int x)
{
    if (x <= 0)
        return 0;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return (x >> 1) + 1;
}


#endif

// vim:ts=4:sw=4

