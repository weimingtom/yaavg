/* 
 * cache.h
 * by WN @ Dec. 20, 2009
 */

#ifndef __CACHE_H
#define __CACHE_H

#include <common/defs.h>
#include <common/mm.h>
#include <common/dict.h>
#include <common/list.h>
#include <common/init_cleanup_list.h>


/* 
 * cache is originally design for store temporary store currently
 * unused bitmap.
 */

typedef void (*cache_destroy_t)(void * arg);
struct cache_t {
	const char * name;
	struct dict_t * dict;
	struct list_head list;
	int nr;
	int total_sz;
};

struct cache_entry_t {
	const char * id;
	int sz;
	void * destroy_arg;
	void * data;
	cache_destroy_t destroy;
	struct cache_t * cache;
	int ref_count;
};

/* 
 * cache_entry_destroy doesn't update dict's profiling infos,
 * the caller needs to do it by itself
 */
static void inline cache_entry_destroy(struct cache_entry_t * e)
{
	if (e == NULL)
		return;
	if (e->destroy)
		e->destroy(e->destroy_arg);
}

extern void
cache_init(struct cache_t * c, const char * name);

extern void
cache_insert(struct cache_t * cache,
		struct cache_entry_t * entry);

/* 
 * if the reference count of target entry is down to 0,
 * cache_rmeove_entry will destroy it.
 */
extern void
cache_remove_entry(struct cache_t * cache,
		const char * id);

/* 
 * get_entry will increase the ref counter
 */
extern struct cache_entry_t *
cache_get_entry(struct cache_t * cache,
		const char * id);
/* 
 * put_entry will decrease the ref counter,
 * if rec counter decrease to 0 and entry->cache == NULL,
 * put_entry will destroy it.
 */
extern void
cache_put_entry(struct cache_t * cache,
		struct cache_entry_t * entry);

extern void
cache_shrink(struct cache_t * cache);

extern void
caches_shrink(void);

#endif

// vim:ts=4:sw=4

