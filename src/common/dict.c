/* 
 * dict.c
 * by WN @ Nov. 19, 2009
 */

#include <common/mm.h>
#include <common/dict.h>
#include <string.h>

#define DICT_SMALL_SIZE	(1 << 3)


struct dict_t {
	int nr_fill;	/* active + dummy */
	int nr_used;	/* active */
	int mask;		/* mask is size - 1; size is always power of 2 */
	uint32_t flags;
	bool_t (*compare_key)(void*, int, void*, int);
	struct dict_item_t * table;
	struct dict_item_t smalltable[DICT_SMALL_SIZE];
};

static DEFINE_MEM_CACHE(__dict_t_cache, "cache of dict_t",
		sizeof(struct dict_t));
static struct mem_cache_t * pdict_cache = &__dict_t_cache;

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

#define IS_FIXED(f)		((f) & (DICT_FL_FIXED))
#define IS_STRKEY(f)	((f) & (DICT_FL_STRKEY))
#define IS_STRVAL(f)	((f) & (DICT_FL_STRVAL))

struct dict_t *
dict_create(int hint, uint32_t flags,
		bool_t (*compare_key)(void*, int, void*, int))
{
	struct dict_t * new_dict = mem_cache_zalloc(pdict_cache);
	new_dict->mask = DICT_SMALL_SIZE - 1;
	new_dict->flags = flags;
	new_dict->compare_key = compare_key;

	if (IS_FIXED(flags)) {
		assert(hint > 0);
		if (hint > DICT_SMALL_SIZE) {
			/* realloc table */
		}
	}

	return new_dict;
}

// vim:ts=4:sw=4

