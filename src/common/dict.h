/* 
 * dict.h
 * by WN @ Nov. 18, 2009
 *
 * dict stores key/value pairs
 */

#ifndef YAAVG_DICT
#define YAAVG_DICT

#include <common/defs.h>
#include <stdint.h>

struct dict_t;

typedef uint32_t hashval_t;

struct dict_stat {
	int nr_slots;
	int nr_items;
	int longest_link;
};

struct dict_item_t {
	void * key;
	int key_len;
	hashval_t hash;
	void * data;
	int data_len;
};

/* if the flag DICT_FL_STRKEY is set, then
 * all hash in dict handler and item are ignored;
 * all key_len and related data are ignored. */
#define DICT_FL_STRKEY	(1)
/* if DICT_FL_STRVAL is set, all data_len is ignored. */
#define DICT_FL_STRVAL	(2)

/* if compare_key is NULL:
 * if DICT_FL_STRKEY is set, then use strcmp to compare keys;
 * else, unless 2 pointers are same, two keys are different. */
extern struct dict_t *
dict_create(int hint, uint32_t flags,
		bool_t (*compare_key)(void * key1, int key1_len,
			void * key2, int key2_len));

/* iterate over items, destroy them, then destroy
 * dict */
extern void
dict_destroy(struct dict_t * dict,
		void (*destroy_item)(struct dict_item_t * item));

/* the return value is a copy of target item.
 * data == NULL indicates an empty slot. */
/* never return NULL unless no more memory. */
/* if DICT_FL_STRKEY is set, key_hash and key_len are ignored */
extern struct dict_item_t
dict_get(struct dict_t * dict, void * key,
		int key_len, hashval_t key_hash);

/* the return value of dict_insert is a copy of the original
 * item.
 *
 * insert will set return item's hash value to item->hash.
 * if DICT_FL_STRKEY is set, item->hash will be modified. */
extern struct dict_item_t
dict_insert(struct dict_t * dict, struct dict_item_t * item);

/* the return value of dict_set is a copy of the original
 * item. */
/* set is different from insert: set never resize the dict,
 * so set may fail, but insert never fail (unless no memory).
 * When set success, the hash in return item should be same as
 * item->hash (it may be changed by dict_set). When set failes,
 * they are different. */
extern struct dict_item_t
dict_set(struct dict_t * dict, struct dict_item_t * item);


/* the return value is a copy of the original item. the caller
 * can destroy the data. if there is no such key, the key field
 * of the return item will be set to NULL. */
extern struct dict_item_t
dict_remove(struct dict_t * dict, void * key, int key_len);

/* del will guarantee no resize the dict */
extern struct dict_item_t
dict_del(struct dict_t * dict, void * key, int key_len);

/* if the pitem is NULL, return the first item */
/* if the return value is NULL, there is no more item. */
/* during the iteration, never call insert or remove,
 * only use get/set and del.
 * then, change the 'data' and 'data_len' in return item
 * is safe. **NEVER** modify the key and key_len field. */
extern struct dict_item_t *
dict_get_next(struct dict_t * dict, struct dict_item_t * item);

#endif
// vim:ts=4:sw=4

