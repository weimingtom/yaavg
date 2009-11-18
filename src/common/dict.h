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

struct dict_stat {
	int nr_slots;
	int nr_items;
	int longest_link;
};

struct dict_item_t {
	void * key;
	int key_len;
	uint32_t hash;
	void * data;
	int data_len;
};

/* if the flag DICT_FL_STRKEY is set, then
 * all hash in dict handler and item are ignored;
 * all key_len and related data are ignored. */
#define DICT_FL_STRKEY	(1)
/* if DICT_FL_STRVAL is set, all data_len is ignored. */
#define DICT_FL_STRVAL	(2)

extern struct dict_t *
dict_create(int hint, uint32_t flags);

extern void
dict_destroy(struct dict_t * dict);

/* the return value is pointer to
 * target item. data == NULL indicates a empty slot. */
/* never return NULL unless no more memory. */
extern struct dict_item_t *
dict_get(struct dict_t * dict, void * key,
		int key_len, uint32_t key_hash);

extern struct dict_item_t *
dict_set(struct dict_t * dict, struct dict_item_t * item);

extern void
dict_del(struct dict_t * dict, void * key, int key_len);

/* if the pitem is NULL, return the first item */
/* if the return value is NULL, there is no more item. */
extern struct dict_item_t *
dict_get_next(struct dict_t * dict, struct dict_item_t * item);

#endif
// vim:ts=4:sw=4

