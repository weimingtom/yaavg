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

struct dict_t;

typedef uint32_t hashval_t;

struct dict_stat {
	int nr_slots;
	int nr_entrys;
	int longest_link;
};

struct dict_entry_t {
	void * key;
	void * data;
	hashval_t hash;
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
			void * key2));

/* iterate over entrys, destroy them, then destroy
 * dict */
extern void
dict_destroy(struct dict_t * dict,
		void (*destroy_entry)(struct dict_entry_t * entry));

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
 * if (data != NULL), return value is the original entry,
 * we have a chance to clear the data.
 *
 * if fixed dict inserted into too much entries, a EXP_DICT_FULL
 * will be thrown.
 *
 * */
extern struct dict_entry_t THROWS(EXP_DICT_FULL)
dict_insert(struct dict_t * dict, struct dict_entry_t * entry);

/* the return value of dict_set is a copy of the original entry. */
/* set is different from insert: set never resize the dict, so set may fail,
 * but insert never fail (unless out of memory, or the dict is fixed size.).  When
 * set success, the hash in return entry should be same as entry->hash (it may
 * be changed by dict_set). When set failes, they are different. */
extern struct dict_entry_t THROWS(EXP_DICT_FULL)
dict_set(struct dict_t * dict, struct dict_entry_t * entry);


/* the return value is a copy of the original entry. the caller
 * can destroy the data. if there is no such key, the key and data field
 * of the return entry will be set to NULL. */
extern struct dict_entry_t
dict_remove(struct dict_t * dict, void * key);

/* del will guarantee no resize the dict */
extern struct dict_entry_t
dict_del(struct dict_t * dict, void * key);

/* if the pentry is NULL, return the first entry */
/* if the return value is NULL, there is no more entry. */
/* during the iteration, never call insert or remove,
 * only use get/set and del.
 * then, change the 'data' in return entry
 * is safe. **NEVER** modify the key field. */
extern struct dict_entry_t *
dict_get_next(struct dict_t * dict, struct dict_entry_t * entry);

__END_DECLS
#endif
// vim:ts=4:sw=4

