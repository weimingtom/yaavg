/* 
 * dict.h
 * by WN @ Nov. 18, 2009
 *
 * dict stores key/value pairs
 */

#ifndef YAAVG_DICT
#define YAAVG_DICT

#include <config.h>
#include <common/defs.h>
#include <common/exception.h>
#include <stdint.h>

__BEGIN_DECLS


typedef uint32_t hashval_t;

#define DICT_DATA_FL_VANISHED	(1UL)
typedef union {
		void * ptr;
		const char * str;
		int val;
		float flt;
		bool_t bol;
		/* void* is the longest field of the above fields */
		unsigned char flags[sizeof(void*) + 1];
} dict_data_t;

#define GET_DICT_DATA_FLAGS(d)	((d).flags[sizeof(void*)])


struct dict_entry_t {
	void * key;
	hashval_t hash;
	dict_data_t data;
};
#define DICT_SMALL_SIZE	(1 << 3)
struct dict_t {
	int nr_fill;	/* active + dummy */
	int nr_used;	/* active */
	uint32_t mask;		/* mask is size - 1; size is always power of 2 */
	uint32_t flags;
	uintptr_t private;	/* dict contains private data */
	uintptr_t compare_key_arg;	/* the 3rd argument passed to compare_key */
	bool_t (*compare_key)(void*, void*, uintptr_t);
	struct dict_entry_t * ptable;
	struct dict_entry_t smalltable[DICT_SMALL_SIZE];
};


/* if the flag DICT_FL_STRKEY is set, then
 * all hash in dict handler and entry are ignored
 */
#define DICT_FL_STRKEY	(1)
#define DICT_FL_STRVAL	(2)
/* if DICT_FL_FIXED set, the size of the dict is hints (if hints is too small,
 * use DICT_SMALL_SIZE instead) */
#define DICT_FL_FIXED	(4)

/* if compare_key is NULL:
 * if DICT_FL_STRKEY is set, then use strcmp to compare keys;
 * else, unless 2 pointers are same, two keys are different. */
extern struct dict_t *
dict_create(int hint, uint32_t flags,
		bool_t (*compare_key)(void * key1,
			void * key2, uintptr_t arg),
		uintptr_t ck_arg);

/* iterate over entrys, destroy them, then destroy
 * dict */
extern void
dict_destroy(struct dict_t * dict,
		void (*destroy_entry)(struct dict_entry_t * entry, uintptr_t arg),
		uintptr_t arg);

/* the return value is a copy of target entry.
 * data == NULL indicates an empty slot. */
extern struct dict_entry_t
dict_get(struct dict_t * dict, void * key,
		hashval_t key_hash);

/* the return value of dict_insert is a copy of the original
 * entry.
 *
 * insert will set return entry's hash value to entry->hash.
 * if DICT_FL_STRKEY is set, entry->hash will be modified. */
/* the retual value:
 *
 * if !(data.flags & DICT_DATA_FL_VANISHED), return value is the original entry,
 * we have a chance to clear the data.
 *
 * if fixed dict inserted into too much entries, a EXP_DICT_FULL
 * will be thrown.
 *
 * */
/* **NOTE**:
 * we shouldn't use entry to store the output entry. below code
 * is **WRONG**:
 *
 * struct dict_entry_t e;
 * e.key = ...;
 * e.data = ...;
 * e = dict_insert(dict, &e);
 * ....
 * */
extern struct dict_entry_t THROWS(EXP_DICT_FULL)
dict_insert(struct dict_t * dict, struct dict_entry_t * entry);

/* 
 * if the key is in the dict, dict_replace will replace the old data
 * by the new one, if the key doesn't exist, nothing will happen.
 * if return true, the key exists, pold_data is filled.
 * if false, the key doesn't exist.
 * dict_replace is used to control the key. sometime, such as static
 * string dict, the original key is statically allocated. if we use
 * dict_insert, it will be replaced. if insert may happen more than
 * once, the caller will be unable to know whether to free the key
 * returned by dict_insert.
 *
 * another solution is to modify dict_insert, makes it keep the old key
 * and return the new key.
 * However, replace is more powerful.
 */
extern bool_t
dict_replace(struct dict_t * dict, void * key, hashval_t hash,
		dict_data_t new_data, dict_data_t * pold_data);

/* the return value of dict_set is a copy of the original entry. */
/* set is different from insert: set never resize the dict, so set may fail,
 * but insert never fail (unless out of memory, or the dict is fixed size.).
 * When set failes, it will throw an EXP_DICT_FULL */
/* if there is no such key before the set, the DICT_DATA_FL_VANISHED will be set. */
extern struct dict_entry_t THROWS(EXP_DICT_FULL) dict_set(struct dict_t * dict,
		struct dict_entry_t * entry);

/* the return value is a copy of the original entry. the caller can destroy the
 * data. if there is no such key, the key and field of the return entry will be
 * set to NULL. if there is no such key before the set, the
 * DICT_DATA_FL_VANISHED will be set. */
extern struct dict_entry_t dict_remove(struct dict_t * dict, void * key,
		hashval_t hash);

/* if the pentry is NULL, return the first entry */
/* if the return value is NULL, there is no more entry. */
/* during the iteration, never call insert or remove,
 * only use get/set.
 * then, change the 'data' in return entry
 * is safe. **NEVER** modify the key field. */
extern struct dict_entry_t *
dict_get_next(struct dict_t * dict, struct dict_entry_t * entry);

/* 
 * used to invalid an entry directly
 */
extern void
dict_invalid_entry(struct dict_t * d, struct dict_entry_t * e);

/* special methods for string based dicts */
#define STRDICT_FL_DUPKEY	(1)
#define STRDICT_FL_DUPDATA	(2)
#define STRDICT_FL_FIXED	(4)

extern struct dict_t *
strdict_create(int hint, uint32_t flags);

extern void
strdict_destroy(struct dict_t * dict);

extern dict_data_t
strdict_get(struct dict_t * dict, const char * key);

extern dict_data_t THROWS(EXP_DICT_FULL)
strdict_insert(struct dict_t * dict,
		const char * key, dict_data_t data);

extern bool_t
strdict_replace(struct dict_t * dict,
		const char * key,
		dict_data_t new_data,
		dict_data_t * pold_data);

extern dict_data_t
strdict_remove(struct dict_t * dict,
		const char * key);

extern void
strdict_invalid_entry(struct dict_t * d, struct dict_entry_t * e);
__END_DECLS
#endif
// vim:ts=4:sw=4

