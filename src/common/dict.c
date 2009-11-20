/* 
 * dict.c
 * by WN @ Nov. 19, 2009
 */

#include <common/mm.h>
#include <common/dict.h>
#include <common/bithacks.h>
#include <string.h>
#include <assert.h>

#define DICT_SMALL_SIZE	(1 << 3)

/* see python's code */
#define PERTURB_SHIFT	(5)

struct dict_t {
	int nr_fill;	/* active + dummy */
	int nr_used;	/* active */
	uint32_t mask;		/* mask is size - 1; size is always power of 2 */
	uint32_t flags;
	bool_t (*compare_key)(void*, void*);
	struct dict_entry_t * ptable;
	struct dict_entry_t smalltable[DICT_SMALL_SIZE];
};

static bool_t compare_str(void * a, void * b)
{
	char * sa, *sb;
	sa = a;
	sb = b;
	if (strcmp(sa, sb) == 0)
		return TRUE;
	return FALSE;
}

/* dummy key */
/* we shouldn't use
 *
 * static const char * = "<dummy>";
 *
 * because the key may be the string of "<dummy>".
 * */
static const char dummy[] = "<dummy>";

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

#define IS_FIXED(d)		(((d)->flags) & (DICT_FL_FIXED))
#define IS_STRKEY(d)	(((d)->flags) & (DICT_FL_STRKEY))
#define IS_STRVAL(d)	(((d)->flags) & (DICT_FL_STRVAL))

struct dict_t *
dict_create(int hint, uint32_t flags,
		bool_t (*compare_key)(void*, void*))
{
	struct dict_t * new_dict = mem_cache_zalloc(pdict_cache);
	assert(new_dict != NULL);
	new_dict->flags = flags;
	if (IS_STRKEY(new_dict))
		new_dict->compare_key = compare_str;
	else
		new_dict->compare_key = compare_key;

	new_dict->ptable = &(new_dict->smalltable[0]);

	TRACE(DICT, "a new dict is creating\n");
	if (IS_FIXED(new_dict)) {
		assert(hint > 0);
		TRACE(DICT, "creating a fixed size dict, hint is %d\n", hint);
		if (hint > DICT_SMALL_SIZE) {
			/* realloc table */
			/* first, round up */
			hint = pow2roundup(hint);
			TRACE(DICT, "round up to %d\n", hint);
			/* then, alloc the new table */
			new_dict->ptable = xcalloc(sizeof(struct dict_entry_t),
					hint);
			assert(new_dict->ptable != NULL);
			TRACE(DICT, "new table is alloced: %p\n", new_dict->ptable);
			/* all field in ptable should have been zeroed */

			new_dict->mask = hint - 1;
		}
	} else {
		TRACE(DICT, "creating a dynamic size dict\n");
		new_dict->mask = DICT_SMALL_SIZE - 1;
	}

	return new_dict;
}

/* this util iterate over the conflict link. return value: 
 *  NULL: it is a fidxed size dict, the key is not fount and
 * 		cannot be inserted into;
 * 	a slot with key == NULL: the key is not fount, and the conflict link
 * 		is over. If we want to insert an entry, the used and fill should
 * 		both increas.
 * 	a slot with key == dummy: the key is not found, but the conflict link
 * 		has empty slots.
 * 	a slot with key != dummy and not null: the key has found.
 */
static struct dict_entry_t *
lookup_entry(struct dict_t * dict, void * key, hashval_t hash)
{
	struct dict_entry_t * ep0 = dict->ptable;
	struct dict_entry_t * ep;
	struct dict_entry_t * free_slot = NULL;
	int i = hash & (dict->mask);
	hashval_t perturb;

	assert(key != NULL);

	int nr_checked = 0;

	perturb = hash;
	do {
		ep = &ep0[i & dict->mask];
		/* the most common case */
		if ((ep->key == key) || (ep->key == NULL))
			return ep;

		if (ep->key == NULL)
			break;

		if (ep->key == dummy) {
			/* we always return the last entry in the conflict
			 * link, therefore if we get an dummy slot,
			 * we know the conflict is long. (???) */
			free_slot = ep;
		} else {
			/* check whether two keys are same */
			if (IS_STRKEY(dict)) {
				if (strcmp(key, ep->key) == 0)
					return ep;
			} else {
				if (dict->compare_key != NULL)
					if ((dict->compare_key)(key, ep->key))
						return ep;
			}
		}

		i = (i << 2) + i + perturb + 1;
		perturb >>= PERTURB_SHIFT;
		nr_checked ++;
	} while (nr_checked <= (dict->mask + 1));

	if (free_slot != NULL)
		return free_slot;
	if (ep->key == NULL)
		return ep;
	/* if we didn't find free_slot and unused slot, and we exhaust the
	 * whole table, the dict is full. */
	return NULL;
}



static inline void
fill_strhash(struct dict_entry_t * entry)
{
	entry->hash = string_hash(entry->key);
}

static void
__expand_dict(struct dict_t * dict, int fator)
{
	uint32_t old_size = dict->mask + 1;
	uint32_t new_size = old_size * fator;
	uint32_t new_mask = new_size - 1;

	TRACE(DICT, "expanding dict %p\n", dict);

	assert(is_power_of_2(new_size));
	/* we alloc the new table */
	struct dict_entry_t * new_table = xcalloc(new_size,
			sizeof(struct dict_entry_t));
	assert(new_table != NULL);

	struct dict_entry_t * old_table = dict->ptable;
	dict->nr_fill = 0;
	dict->nr_used = 0;
	dict->mask = new_mask;
	dict->ptable = new_table;

	/* drain all entries into the new table */
	for (int i = 0; i < old_size; i++) {
		struct dict_entry_t * oep = &old_table[i];
		if (oep->key == NULL)
			continue;
		if (oep->key == dummy)
			continue;
		struct dict_entry_t * ep = lookup_entry(dict,
				oep->key, oep->hash);
		assert(ep != NULL);
		assert(ep->key == NULL);
		*ep = *oep;
		dict->nr_fill ++;
		dict->nr_used ++;
	}

	if (old_table != dict->smalltable)
		xfree(old_table);

	TRACE(DICT, "dict %p expansion over: fill=%d, used=%d, size=%d\n",
			dict, dict->nr_fill, dict->nr_used, dict->mask + 1);

	return;
}

static void
expand_dict(struct dict_t * dict)
{
	assert(!IS_FIXED(dict));
	if (dict->nr_fill * 3 >= (dict->mask + 1) * 2) {
		TRACE(DICT, "dict %p, fill=%d, used=%d, size=%d, need expansion\n",
				dict, dict->nr_fill, dict->nr_used, dict->mask + 1);
		__expand_dict(dict, 2);
	}
}

static struct dict_entry_t
__dict_insert(struct dict_t * dict, struct dict_entry_t * entry,
		bool_t can_expand)
{
	struct dict_entry_t retval;
	if (IS_STRKEY(dict))
		fill_strhash(entry);

	TRACE(DICT, "inserting item into %p with hash 0x%x\n",
			dict, entry->hash);

	/* search */
	struct dict_entry_t * ep;
	ep = lookup_entry(dict, entry->key, entry->hash);

	if (ep == NULL) {
		/* this is a fixed dict and cannot insert new entry */
		if (IS_FIXED(dict))
			THROW_VAL(EXP_DICT_FULL, dict,
					"fixed dict %p is full(%d/%d/%d), unable to insert", dict,
					dict->nr_used, dict->nr_fill, dict->mask + 1);
		TRACE(DICT, "lookup dict %p but return NULL, force dict expansion\n",
				dict);
		if (can_expand) {
			expand_dict(dict);
			ep = lookup_entry(dict, entry->key, entry->hash);
			assert(ep != NULL);
		} else {
			/* makes the 2 hash different */
			retval.hash = entry->hash - 1;
			return retval;
		}
	}

	if (ep->key == dummy) {
		*ep = *entry;
		dict->nr_used ++;
		retval.key = retval.data = NULL;
		retval.hash = entry->hash;

		/* we can expand the dict */
		if (can_expand)
			if (!IS_FIXED(dict))
				expand_dict(dict);
		return retval;
	}

	/* we still have unused slot */
	if (ep->key == NULL) {
		dict->nr_fill ++;
		dict->nr_used ++;
		retval = *ep;
		retval.hash = entry->hash;
		*ep = *entry;
		return retval;
	}

	/* the key is already inside the dict */
	retval = *ep;
	*ep = *entry;
	assert(retval.hash == entry->hash);
	return retval;
}

struct dict_entry_t
dict_insert(struct dict_t * dict, struct dict_entry_t * entry)
{
	return __dict_insert(dict, entry, TRUE);
}

struct dict_entry_t
dict_set(struct dict_t * dict, struct dict_entry_t * entry)
{
	return __dict_insert(dict, entry, FALSE);
}


// vim:ts=4:sw=4

