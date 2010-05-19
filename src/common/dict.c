/* 
 * dict.c
 * by WN @ Nov. 19, 2009
 */

#include <common/debug.h>
#include <common/mm.h>
#include <common/init_cleanup_list.h>
#include <common/dict.h>
#include <common/bithacks.h>
#include <string.h>
#include <assert.h>

static bool_t compare_str(void * a, void * b, uintptr_t useless ATTR_UNUSED)
{
	char * sa, *sb;
	sa = a;
	sb = b;
	if (strcmp(sa, sb) == 0)
		return TRUE;
	return FALSE;
}

/* dummy key */
/* we shouldn't use the below statement
 *
 * static const char * = "<dummy>";
 *
 * if we do that, ld may link other string of "<dummy>"
 * with it.
 * */
static const char dummy_key[] = "<dummy_key>";

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
		bool_t (*compare_key)(void*, void*, uintptr_t),
		uintptr_t ck_arg)
{
	struct dict_t * new_dict = xcalloc(1, sizeof(struct dict_t));
	assert(new_dict != NULL);

	new_dict->flags = flags;
	if (IS_STRKEY(new_dict))
		new_dict->compare_key = compare_str;
	else
		new_dict->compare_key = compare_key;
	new_dict->compare_key_arg = ck_arg;

	new_dict->ptable = &(new_dict->smalltable[0]);

	new_dict->real_data_sz = sizeof(*new_dict);

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
			new_dict->real_data_sz += sizeof(struct dict_entry_t) * hint;
		} else {
			new_dict->mask = DICT_SMALL_SIZE - 1;
		}
	} else {
		TRACE(DICT, "creating a dynamic size dict\n");
		new_dict->mask = DICT_SMALL_SIZE - 1;
	}


	return new_dict;
}

void
dict_destroy(struct dict_t * dict,
		void  (*destroy_entry)(struct dict_entry_t * entry, uintptr_t arg),
		uintptr_t arg)
{
	/* we iterate over the table */
	assert(dict != NULL);
	assert(dict->ptable != NULL);
	TRACE(DICT, "destroying dict %p\n", dict);
	if (destroy_entry != NULL) {
		int sz = dict->mask + 1;
		for (int i = 0; i < sz; i++) {
			struct dict_entry_t * ep = &(dict->ptable[i]);
			if ((ep->key != NULL) && (ep->key != dummy_key))
				destroy_entry(ep, arg);
		}
	}

	if (dict->ptable != dict->smalltable)
		xfree_null(dict->ptable);
	xfree_null(dict);
}

/* this util iterate over the conflict link. return value: 
 *  NULL: it is a fidxed size dict, the key is not fount and
 * 		cannot be inserted into;
 * 	a slot with key == NULL: the key is not fount, and the conflict link
 * 		is over. If we want to insert an entry, the used and fill should
 * 		both be increased.
 * 	a slot with key == dummy_key: the key is not found, but the conflict link
 * 		has empty slots.
 * 	a slot with key != dummy_key and not null: the key has found.
 */
static struct dict_entry_t *
lookup_entry(struct dict_t * dict, void * key, hashval_t hash)
{
	struct dict_entry_t * ep0 = dict->ptable;
	struct dict_entry_t * ep;
	struct dict_entry_t * free_slot = NULL;
	int i = hash & (dict->mask);

	assert(key != NULL);

	unsigned int nr_checked = 0;

	TRACE(DICT, "begin lookup for hash 0x%x\n", hash);
	do {
		ep = &ep0[i & dict->mask];
		TRACE(DICT, "checking nr %d\n", i & dict->mask);
		/* the most common case */
		if ((ep->key == key) || (ep->key == NULL))
			return ep;

		if (ep->key == NULL)
			break;

		if (ep->key == dummy_key) {
			/* we always return the last entry in the conflict
			 * link, therefore if we get an dummy slot,
			 * we know the conflict is long. (???) */
			free_slot = ep;
		} else {
			/* check whether two keys are same */
			if (ep->hash == hash) {
				if (IS_STRKEY(dict)) {
					if (strcmp(key, ep->key) == 0)
						return ep;
				} else {
					if (dict->compare_key != NULL) {
						if ((dict->compare_key)(key, ep->key,
									dict->compare_key_arg))
							return ep;
					} else if (IS_STRKEY(dict)) {
						dict->compare_key = compare_str;
						if (compare_str(key, ep->key, 0))
							return ep;
					}
				}
			}
		}

		i = (i << 2) + i + 1;
		nr_checked ++;
	} while (nr_checked < (dict->mask + 1));

	if (free_slot != NULL)
		return free_slot;
	if (ep->key == NULL)
		return ep;
	/* if we didn't find free_slot and unused slot, and we exhaust the
	 * whole table, the dict is full. */
	assert(nr_checked == dict->mask + 1);
	return NULL;
}

static inline void
fill_strhash(struct dict_entry_t * entry)
{
	entry->hash = string_hash(entry->key);
}

/* insert entrys in resizing. when call this func,
 * the caller knows that there **MUST** have spaces
 * for the oep. the key, hash and value field of oep should
 * be set correctly. */
static void
__dict_insert_clean(struct dict_t * dict, struct dict_entry_t * oep)
{
	/* a simple version of lookup_entry */
	hashval_t hash = oep->hash;
	hashval_t mask = dict->mask;
	int i = hash & mask;
	struct dict_entry_t * ep0 = dict->ptable;
	struct dict_entry_t * ep = &ep0[hash & mask];
	while (ep->key != NULL) {
		i = (i << 2) + i + 1;
		ep = &ep0[i & mask];
	}
	*ep = *oep;
	dict->real_data_sz += GET_DICT_DATA_REAL_SZ(ep->data);
	dict->nr_fill ++;
	dict->nr_used ++;
}

static void
__expand_dict(struct dict_t * dict, int minused)
{
	int old_size = dict->mask + 1;
	int new_size = pow2roundup(minused);
	if (new_size < DICT_SMALL_SIZE) {
		new_size = DICT_SMALL_SIZE;
	}

	uint32_t new_mask = new_size - 1;

	TRACE(DICT, "resizing dict %p's size from %d to %d\n", dict,
			old_size, new_size);
	assert(dict->nr_used <= new_size);

	assert(is_power_of_2(new_size));
	/* we alloc the new table */
	/* we clone the old table for temporary storage */
	struct dict_entry_t * new_table;
	if ((new_size <= DICT_SMALL_SIZE) && (dict->ptable == dict->smalltable)) {
		dict->ptable = xmalloc(sizeof(dict->smalltable));
		assert(dict->ptable != NULL);
		memcpy(dict->ptable, dict->smalltable, sizeof(dict->smalltable));
		bzero(dict->smalltable, sizeof(dict->smalltable));
		new_table = dict->smalltable;
	} else {
		new_table = xcalloc(new_size,
				sizeof(struct dict_entry_t));
	}

	assert(new_table != NULL);

	struct dict_entry_t * old_table = dict->ptable;
	dict->nr_fill = 0;
	dict->nr_used = 0;
	dict->mask = new_mask;
	dict->ptable = new_table;

	/* drain all entries into the new table */
	dict->real_data_sz = sizeof(*dict);
	if (new_table != dict->smalltable)
		dict->real_data_sz += new_size * sizeof(struct dict_entry_t);

	for (int i = 0; i < old_size; i++) {
		struct dict_entry_t * oep = &old_table[i];
		if (oep->key == NULL)
			continue;
		if (oep->key == dummy_key)
			continue;
		__dict_insert_clean(dict, oep);
	}

	if (old_table != dict->smalltable)
		xfree_null(old_table);

	TRACE(DICT, "dict %p expansion over: fill=%d, used=%d, size=%d\n",
			dict, dict->nr_fill, dict->nr_used, dict->mask + 1);
	return;
}

static void
expand_dict(struct dict_t * dict)
{
	assert(!IS_FIXED(dict));
	if (dict->nr_fill * 3 >= ((int)(dict->mask + 1)) * 2) {
		TRACE(DICT, "dict %p, fill=%d, used=%d, size=%d, need expansion\n",
				dict, dict->nr_fill, dict->nr_used, dict->mask + 1);
		__expand_dict(dict, dict->nr_used * 2);
	}
}

static struct dict_entry_t
__dict_insert(struct dict_t * dict, struct dict_entry_t * entry,
		bool_t can_expand, bool_t no_replace)
{
	struct dict_entry_t retval;
	assert(dict != NULL);
	assert(entry != NULL);
	assert(entry->key != NULL);

	GET_DICT_DATA_FLAGS(entry->data) &= ~(DICT_DATA_FL_VANISHED);
	if (IS_STRKEY(dict))
		fill_strhash(entry);

	TRACE(DICT, "inserting item into %p with hash 0x%x\n",
			dict, entry->hash);

	/* search */
	struct dict_entry_t * ep;
	ep = lookup_entry(dict, entry->key, entry->hash);

	if (ep == NULL) {
		/* this is a fixed dict and cannot insert new entry */
		if (IS_FIXED(dict)) {
			TRACE(DICT, "dict %p is fixed and full\n", dict);
			THROW_VAL_FATAL(EXP_DICT_FULL, dict,
					"fixed dict %p is full(%d/%d/%d), unable to insert", dict,
					dict->nr_used, dict->nr_fill, dict->mask + 1);
		}
		TRACE(DICT, "lookup dict %p but return NULL, force dict expansion\n",
				dict);
		if (can_expand) {
			expand_dict(dict);
			ep = lookup_entry(dict, entry->key, entry->hash);
			assert(ep != NULL);
		} else {
			THROW_VAL_TAINTED(EXP_DICT_FULL, dict,
					"dict %p is full(%d/%d/%d) and doesn't allow expansion", dict,
					dict->nr_used, dict->nr_fill, dict->mask + 1);
		}
	}

	if (ep->key == dummy_key) {
		*ep = *entry;

		dict->real_data_sz += GET_DICT_DATA_REAL_SZ(ep->data);

		dict->nr_used ++;
		retval.key = retval.data.ptr = NULL;
		GET_DICT_DATA_FLAGS(retval.data) |= DICT_DATA_FL_VANISHED;
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
		retval.key = retval.data.ptr = NULL;
		GET_DICT_DATA_FLAGS(retval.data) |= DICT_DATA_FL_VANISHED;
		retval.hash = entry->hash;
		*ep = *entry;
		dict->real_data_sz += GET_DICT_DATA_REAL_SZ(ep->data);
		return retval;
	}

	/* the key is already inside the dict */

	retval = *ep;
	if (!no_replace) {
		dict->real_data_sz -= GET_DICT_DATA_REAL_SZ(ep->data);
		*ep = *entry;
		dict->real_data_sz += GET_DICT_DATA_REAL_SZ(ep->data);
		assert(retval.hash == entry->hash);
	}
	return retval;
}

static bool_t
__dict_replace(struct dict_t * dict, void * key, hashval_t hash,
		dict_data_t new_data, dict_data_t * pold_data)
{
	assert(dict != NULL);
	assert(key != NULL);

	if (IS_STRKEY(dict))
		hash = string_hash(key);
	TRACE(DICT, "replacing key %p with hash 0x%x\n", dict, hash);
	struct dict_entry_t * ep;
	ep = lookup_entry(dict, key, hash);
	if ((ep == NULL) || (ep->key == dummy_key) || (ep->key == NULL)) {
		TRACE(DICT, "doesn't found key %p in dict %p\n",
				key, dict);
		return FALSE;
	}
	assert(ep->hash == hash);
	dict->real_data_sz -= GET_DICT_DATA_REAL_SZ(ep->data);
	if (pold_data != NULL)
		*pold_data = ep->data;
	ep->data = new_data;
	dict->real_data_sz += GET_DICT_DATA_REAL_SZ(ep->data);
	return TRUE;
}

struct dict_entry_t
dict_insert(struct dict_t * dict, struct dict_entry_t * entry)
{
	assert(dict != NULL);
	assert(entry != NULL);
	return __dict_insert(dict, entry, TRUE, FALSE);
}

bool_t
dict_replace(struct dict_t * dict, void * key, hashval_t hash,
		dict_data_t new_data, dict_data_t * pold_data)
{
	assert(dict != NULL);
	assert(key != NULL);
	return __dict_replace(dict, key, hash, new_data, pold_data);
}

struct dict_entry_t
dict_set(struct dict_t * dict, struct dict_entry_t * entry)
{
	assert(dict != NULL);
	assert(entry != NULL);
	return __dict_insert(dict, entry, FALSE, FALSE);
}

struct dict_entry_t
dict_append(struct dict_t * dict, struct dict_entry_t * entry)
{
	assert(dict != NULL);
	assert(entry != NULL);
	return __dict_insert(dict, entry, TRUE, TRUE);
}

static struct dict_entry_t
__dict_remove(struct dict_t * dict, void * key, hashval_t hash)
{
	struct dict_entry_t retval;
	/* first we lookup the key */
	if (IS_STRKEY(dict))
		hash = string_hash((char*)key);

	struct dict_entry_t * ep = lookup_entry(dict, key, hash);
	if ((ep == NULL) || (ep->key == NULL) || (ep->key == dummy_key)) {
		retval.key = retval.data.ptr = NULL;
		GET_DICT_DATA_FLAGS(retval.data) |= DICT_DATA_FL_VANISHED;
		return retval;
	}

	retval = *ep;
	dict_invalid_entry(dict, ep);
	assert(!(DICT_DATA_NULL(retval.data)));

	/* this is time for shrink */
#if 0
	/* never shrink when del entries. */
	/* when insert entries, if nr_fill is too high,
	 * it will do the shrinking. */
	if (can_shrink && (!IS_FIXED(dict))) {
	}
#endif
	return retval;
}

struct dict_entry_t
dict_remove(struct dict_t * dict, void * key, hashval_t hash)
{
	assert(dict != NULL);
	return __dict_remove(dict, key, hash);
}

void
dict_invalid_entry(struct dict_t * d, struct dict_entry_t * e)
{
	assert(d != NULL);
	assert(e != NULL);
	
	d->real_data_sz -= GET_DICT_DATA_REAL_SZ(e->data);
	GET_DICT_DATA_REAL_SZ(e->data) = 0;

	e->key = (void*)dummy_key;
	e->hash = 0;
	e->data.ptr = NULL;
	GET_DICT_DATA_FLAGS(e->data) |= DICT_DATA_FL_VANISHED;
	d->nr_used --;
}

struct dict_entry_t
dict_get(struct dict_t * dict, void * key,
		hashval_t key_hash)
{
	assert(dict != NULL);
	assert(key != NULL);
	struct dict_entry_t retval;

	if (IS_STRKEY(dict))
		key_hash = string_hash(key);

	/* lookup! */
	struct dict_entry_t * ep = lookup_entry(dict, key,
			key_hash);

	if ((ep == NULL) || (ep->key == NULL) || (ep->key == dummy_key)) {
		retval.key = retval.data.ptr = NULL;
		GET_DICT_DATA_FLAGS(retval.data) = DICT_DATA_FL_VANISHED;
		return retval;
	}

	retval = *ep;
	assert(!(DICT_DATA_NULL(retval.data)));
	return retval;
}

struct dict_entry_t *
dict_get_next(struct dict_t * dict, struct dict_entry_t * entry)
{
	assert(dict != NULL);
	if (entry == NULL)
		entry = &(dict->ptable[0]);
	else
		entry ++;

	struct dict_entry_t * ep0, *epmax;
	ep0 = &(dict->ptable[0]);
	epmax = &(dict->ptable[dict->mask]);

	assert(entry >= ep0);
	for (; entry <= epmax; entry ++) {
		if ((entry->key != NULL) &&
				(entry->key != dummy_key))
			break;
	}

	if (entry > epmax)
		return NULL;
	assert(!(DICT_ENTRY_NODATA(entry)));
	return entry;
}

void
dict_set_private(struct dict_t * dict, uintptr_t pri)
{
	assert(dict != NULL);
	dict->private = pri;
}

uintptr_t
dict_get_private(struct dict_t * dict)
{
	assert(dict != NULL);
	return dict->private;
}

/* methods for special string based dicts */
struct dict_t *
strdict_create(int hint, uint32_t flags)
{
	uint32_t real_flags = DICT_FL_STRKEY;
	if (flags & STRDICT_FL_FIXED)
		real_flags |= DICT_FL_FIXED;
	if (flags & STRDICT_FL_MAINTAIN_REAL_SZ)
		real_flags |= DICT_FL_MAINTAIN_REAL_SZ;
	struct dict_t * dict = dict_create(hint, real_flags, NULL, 0);
	assert(dict != NULL);
	dict->private = (uintptr_t)flags;
	return dict;
}

static void
strdict_destroy_entry(struct dict_entry_t * ep, uintptr_t arg)
{
	if ((arg & STRDICT_FL_DUPKEY) && (ep->key != NULL))
		xfree_null(ep->key);
	if ((arg & STRDICT_FL_DUPDATA) && (ep->data.str != NULL))
		xfree_null((ep->data.str));
}

void
strdict_destroy(struct dict_t * dict)
{
	assert(dict != NULL);

	uint32_t flags = (uint32_t)(dict->private);
	if (flags & (STRDICT_FL_DUPKEY | STRDICT_FL_DUPDATA)) {
		dict_destroy(dict, strdict_destroy_entry, flags);
	} else {
		dict_destroy(dict, NULL, 0);
	}
}

dict_data_t
strdict_get(struct dict_t * dict, const char * key)
{
	assert(dict != NULL);
	assert(key != NULL);

	struct dict_entry_t e = dict_get(dict, (char*)key, 0);
	return e.data;
}

static dict_data_t
__strdict_insert(struct dict_t * dict,
		const char * key, dict_data_t data, bool_t append)
{
	struct dict_entry_t e, oe;
	uintptr_t flags = dict->private;

	assert(key != NULL);
	GET_DICT_DATA_FLAGS(e.data) = 0;
	if (flags & STRDICT_FL_DUPKEY)
		e.key = strdup(key);
	else
		e.key = (char*)key;

	if ((flags & STRDICT_FL_DUPDATA) && (data.str != NULL))
		e.data.str = strdup(data.str);
	else
		e.data = data;

	if (append) {
		oe = dict_append(dict, &e);
		if (!DICT_ENTRY_NODATA(&oe)) {
			if (flags & STRDICT_FL_DUPKEY)
				xfree_null(e.key);
			if (flags & STRDICT_FL_DUPDATA)
				xfree_null(e.data.str);
		}
	} else {
		oe = dict_insert(dict, &e);
		if ((oe.data.str != NULL) && (flags & STRDICT_FL_DUPDATA)) {
			xfree_null(oe.data.ptr);
		}
		if ((oe.key != NULL) && (flags & STRDICT_FL_DUPKEY)) {
			xfree_null(oe.key);
		}
	}
	return oe.data;
}

dict_data_t
strdict_insert(struct dict_t * dict,
		const char * key, dict_data_t data)
{
	assert(key != NULL);
	assert(dict != NULL);
	return __strdict_insert(dict, key, data, FALSE);
}

dict_data_t
strdict_append(struct dict_t * dict,
		const char * key, dict_data_t data)
{
	assert(key != NULL);
	assert(dict != NULL);
	return __strdict_insert(dict, key, data, TRUE);
}


bool_t
strdict_replace(struct dict_t * dict,
		const char * key, dict_data_t new_data, dict_data_t * pold_data)
{
	assert(key != NULL);
	assert(dict != NULL);
	return dict_replace(dict, (void*)key, 0, new_data, pold_data);
}

dict_data_t
strdict_remove(struct dict_t * dict,
		const char * key)
{
	struct dict_entry_t oe;
	uintptr_t flags = dict->private;

	assert(dict != NULL);
	assert(key != NULL);

	oe = dict_remove(dict, (void*)key, 0);
	if ((oe.data.str != NULL) && (flags & STRDICT_FL_DUPDATA)) {
		if (!(DICT_DATA_NULL(oe.data))) {
			if (oe.data.ptr != NULL)
				xfree_null(oe.data.ptr);
		}
		oe.data.ptr = NULL;
		GET_DICT_DATA_FLAGS(oe.data) |= DICT_DATA_FL_VANISHED;
	}
	if ((oe.key != NULL) && (flags & STRDICT_FL_DUPKEY))
		xfree_null(oe.key);
	return oe.data;
}

void
strdict_invalid_entry(struct dict_t * d, struct dict_entry_t * e)
{
	assert(d != NULL);
	assert(e != NULL);
	uintptr_t flags = d->private;
	if ((e->key != NULL) && (flags & STRDICT_FL_DUPKEY))
		xfree_null(e->key);
	if ((e->data.str != NULL) && (flags & STRDICT_FL_DUPDATA)) {
		if (!(DICT_ENTRY_NODATA(e)))
			xfree_null(e->data.ptr);
	}
	dict_invalid_entry(d, e);
}

// vim:ts=4:sw=4

