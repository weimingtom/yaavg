/* 
 * dict.c
 * by WN @ Nov. 19, 2009
 */

#include <common/dict.h>
#include <string.h>

static hashval_t
string_hash(char * s)
{
	int len, len2;
	unsigned char *p;
	hashval_t x;

	len2 = len = strlen(s);
	p = (unsigned char *)s;
	x = *p << 7;
	while (--len >= 0)
		x = (1000003*x) ^ *p++;
	x ^= len2;
	return x;
}

// vim:ts=4:sw=4

